/*
 * File: sms_app.c
 * Description: Main application for Sensor Monitoring System.
 * Educational example: review and test before submission.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/mman.h>
#include <getopt.h>

#include "sms.h"

/* =========================================================
 * 1. SIGNAL HANDLING
 * ========================================================= */

static volatile sig_atomic_t signal_seen = 0;

static void on_signal(int signum)
{
    (void)signum;
    signal_seen = 1;
}

/* =========================================================
 * 2. JITTER CALCULATION
 * ========================================================= */

static int cmp_long(const void *a, const void *b)
{
    long x = *(const long *)a;
    long y = *(const long *)b;

    return (x > y) - (x < y);
}

void print_jitter(const App *app)
{
    size_t n = app->njitter;
    size_t hist[4] = {0};

    long *sorted;
    double sum = 0;

    if (!n) {
        puts("No jitter samples");
        return;
    }

    sorted = malloc(n * sizeof(*sorted));

    if (!sorted) {
        return;
    }

    memcpy(
        sorted,
        app->jitters,
        n * sizeof(*sorted)
    );

    qsort(
        sorted,
        n,
        sizeof(*sorted),
        cmp_long
    );

    for (size_t i = 0; i < n; ++i) {

        long v = app->jitters[i];

        sum += v;

        if (v < 100) {
            hist[0]++;
        }
        else if (v < 500) {
            hist[1]++;
        }
        else if (v < 1000) {
            hist[2]++;
        }
        else {
            hist[3]++;
        }
    }

    size_t p99 = (size_t)ceil(0.99 * (double)n);

    if (p99) {
        p99--;
    }

    printf(
        "JITTER n=%zu min=%ld avg=%.2f max=%ld p99=%ld us\n",
        n,
        sorted[0],
        sum / n,
        sorted[n - 1],
        sorted[p99]
    );

    printf(
        "HIST [<100us]=%zu "
        "[100-500us]=%zu "
        "[500-1000us]=%zu "
        "[>=1ms]=%zu\n",
        hist[0],
        hist[1],
        hist[2],
        hist[3]
    );

    free(sorted);
}

/* =========================================================
 * 3. USAGE INFORMATION
 * ========================================================= */

static void usage(const char *prog)
{
    fprintf(
        stderr,
        "Usage: %s "
        "[-i ms] "
        "[-t Celsius] "
        "[-h humidity] "
        "[-d seconds] "
        "[-v] "
        "[--mock] "
        "[--other] "
        "[--device path]\n",
        prog
    );
}

/* =========================================================
 * 4. MAIN APPLICATION
 * ========================================================= */

int main(int argc, char **argv)
{
    App app = {
        .interval_ms = 500,
        .duration_s = 60,
        .temp_limit = 40,
        .hum_limit = 80,
        .fifo = 1,
        .device_path = "/dev/sms_sensor",
        .alarms = (mqd_t)-1
    };

    int opt;
    int started = 0;
    int rc = 1;

    /* -----------------------------------------------------
     * 4.1 Command-line options
     * ----------------------------------------------------- */

    static const struct option options[] = {
        {"mock", no_argument, NULL, 1000},
        {"other", no_argument, NULL, 1001},
        {"device", required_argument, NULL, 1002},
        {NULL, 0, NULL, 0}
    };

    while ((opt = getopt_long(
                argc,
                argv,
                "i:t:h:d:v",
                options,
                NULL)) != -1) {

        char *end;

        switch (opt) {

        case 'i': {
            long v = strtol(optarg, &end, 10);

            if (*end || v < 1 || v > 60000) {
                usage(argv[0]);
                return 2;
            }

            app.interval_ms = (int)v;
            break;
        }

        case 't':

            app.temp_limit = strtod(optarg, &end);

            if (*end || !isfinite(app.temp_limit)) {
                return 2;
            }

            break;

        case 'h':

            app.hum_limit = strtod(optarg, &end);

            if (*end || !isfinite(app.hum_limit)) {
                return 2;
            }

            break;

        case 'd': {
            long v = strtol(optarg, &end, 10);

            if (*end || v < 0 || v > 86400) {
                return 2;
            }

            app.duration_s = (int)v;
            break;
        }

        case 'v':

            app.verbose = 1;
            break;

        case 1000:

            app.mock = 1;
            break;

        case 1001:

            app.fifo = 0;
            break;

        case 1002:

            app.device_path = optarg;
            break;

        default:

            usage(argv[0]);
            return 2;
        }
    }

    if (optind != argc) {
        usage(argv[0]);
        return 2;
    }

    /* -----------------------------------------------------
     * 4.2 Allocate jitter buffer
     * ----------------------------------------------------- */

    app.jitter_cap = app.duration_s
        ? (size_t)(
            app.duration_s * 1000LL / app.interval_ms + 128
        )
        : 16384;

    app.jitters = calloc(
        app.jitter_cap,
        sizeof(*app.jitters)
    );

    if (!app.jitters) {
        return 1;
    }

    /* -----------------------------------------------------
     * 4.3 Initialize shared circular buffer
     * ----------------------------------------------------- */

    if (ring_init(&app.ring)) {
        free(app.jitters);
        return 1;
    }

    /* -----------------------------------------------------
     * 4.4 Initialize logging system
     * ----------------------------------------------------- */

    log_init(&app.logs);

    pthread_mutex_init(
        &app.counters_mu,
        NULL
    );

    /* -----------------------------------------------------
     * 4.5 Create POSIX message queue
     * ----------------------------------------------------- */

    mq_unlink(SMS_ALARM_NAME);

    struct mq_attr attr = {
        .mq_maxmsg = 10,
        .mq_msgsize = 128
    };

    app.alarms = mq_open(
        SMS_ALARM_NAME,
        O_CREAT | O_RDWR,
        0600,
        &attr
    );

    if (app.alarms == (mqd_t)-1) {

        perror("mq_open (mount /dev/mqueue?)");

        goto cleanup;
    }

    /* -----------------------------------------------------
     * 4.6 Register signal handlers
     * ----------------------------------------------------- */

    struct sigaction sa = {
        .sa_handler = on_signal
    };

    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* -----------------------------------------------------
     * 4.7 Create Logger Thread
     * ----------------------------------------------------- */

    if (pthread_create(
            &app.logger,
            NULL,
            logger_main,
            &app)) {

        goto cleanup;
    }

    started = 1;

    /* -----------------------------------------------------
     * 4.8 Create Alert Thread
     * ----------------------------------------------------- */

    if (pthread_create(
            &app.alert,
            NULL,
            alert_main,
            &app)) {

        goto shutdown;
    }

    started = 2;

    /* -----------------------------------------------------
     * 4.9 Create Processor Thread
     * ----------------------------------------------------- */

    if (pthread_create(
            &app.processor,
            NULL,
            processor_main,
            &app)) {

        goto shutdown;
    }

    started = 3;

    /* -----------------------------------------------------
     * 4.10 Create Sensor Thread
     * ----------------------------------------------------- */

    if (pthread_create(
            &app.sensor,
            NULL,
            sensor_main,
            &app)) {

        goto shutdown;
    }

    started = 4;

    /* -----------------------------------------------------
     * 4.11 Lock application memory
     * ----------------------------------------------------- */

    if (mlockall(MCL_CURRENT | MCL_FUTURE)) {

        fprintf(
            stderr,
            "mlockall unavailable (%s); "
            "measurement continues without locked pages\n",
            strerror(errno)
        );
    }

    /* -----------------------------------------------------
     * 4.12 Main execution loop
     * ----------------------------------------------------- */

    struct timespec start;
    struct timespec now;

    struct timespec wait = {
        .tv_sec = 0,
        .tv_nsec = 100000000
    };

    clock_gettime(
        CLOCK_MONOTONIC,
        &start
    );

    while (!app.stop) {

        if (signal_seen) {
            app.stop = 1;
            break;
        }

        nanosleep(&wait, NULL);

        clock_gettime(
            CLOCK_MONOTONIC,
            &now
        );

        if (app.duration_s &&
            now.tv_sec - start.tv_sec >= app.duration_s) {

            app.stop = 1;
        }
    }

    rc = app.sensor_error ? 1 : 0;

    /* -----------------------------------------------------
     * 4.13 Graceful shutdown
     * ----------------------------------------------------- */

shutdown:

    app.stop = 1;

    if (started >= 4) {
        pthread_join(app.sensor, NULL);
    }

    ring_close(&app.ring);

    if (started >= 3) {
        pthread_join(app.processor, NULL);
    }

    if (started >= 2) {
        pthread_join(app.alert, NULL);
    }

    log_close(&app.logs);

    if (started >= 1) {
        pthread_join(app.logger, NULL);
    }

    munlockall();

    /* -----------------------------------------------------
     * 4.14 Print final summary
     * ----------------------------------------------------- */

    printf(
        "SUMMARY produced=%zu consumed=%zu "
        "overflow=%zu alerts(T/H/S)=%zu/%zu/%zu "
        "log_dropped=%zu\n",
        app.ring.produced,
        app.ring.consumed,
        app.ring.overflow,
        app.alerts[0],
        app.alerts[1],
        app.alerts[2],
        app.logs.dropped
    );

    print_jitter(&app);

    /* -----------------------------------------------------
     * 4.15 Cleanup resources
     * ----------------------------------------------------- */

cleanup:

    if (app.alarms != (mqd_t)-1) {
        mq_close(app.alarms);
    }

    mq_unlink(SMS_ALARM_NAME);

    pthread_mutex_destroy(&app.counters_mu);

    log_destroy(&app.logs);

    ring_destroy(&app.ring);

    free(app.jitters);

    return rc;
}