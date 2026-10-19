/*
 * File: sensor_thread.c
 * Description: Sensor data acquisition thread.
 *
 * Functions:
 * - Read sensor data from kernel driver.
 * - Support mock sensor mode.
 * - Apply real-time scheduling.
 * - Measure sampling jitter.
 * - Send samples to circular buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sched.h>

#include "sms.h"
#include "../driver/sms_sensor.h"

/* =========================================================
 * 1. ADD MILLISECONDS TO TIMESPEC
 * ========================================================= */

static void add_ms(struct timespec *t, int ms)
{
    t->tv_sec += ms / 1000;

    t->tv_nsec +=
        (long)(ms % 1000) * 1000000L;

    if (t->tv_nsec >= 1000000000L) {
        t->tv_sec++;
        t->tv_nsec -= 1000000000L;
    }
}

/* =========================================================
 * 2. CALCULATE TIME DIFFERENCE IN MICROSECONDS
 * ========================================================= */

static long diff_us(
    const struct timespec *a,
    const struct timespec *b
)
{
    return
        (long)(a->tv_sec - b->tv_sec) * 1000000L
        + (a->tv_nsec - b->tv_nsec) / 1000L;
}

/* =========================================================
 * 3. SENSOR THREAD
 * ========================================================= */

void *sensor_main(void *arg)
{
    App *app = arg;

    int fd = -1;

    /* -----------------------------------------------------
     * 3.1 Open sensor device
     * ----------------------------------------------------- */

    if (!app->mock) {

        unsigned int interval =
            (unsigned int)app->interval_ms;

        fd = open(
            app->device_path,
            O_RDONLY
        );

        if (fd < 0) {

            perror("open sensor device");

            app->sensor_error = 1;
            app->stop = 1;

            return NULL;
        }

        /* Configure sensor sampling interval */

        if (ioctl(
                fd,
                SMS_SET_INTERVAL,
                &interval
            ) < 0) {

            perror("ioctl SMS_SET_INTERVAL");

            close(fd);

            app->sensor_error = 1;
            app->stop = 1;

            return NULL;
        }
    }

    /* -----------------------------------------------------
     * 3.2 Configure real-time scheduling
     * ----------------------------------------------------- */

    if (app->fifo) {

        struct sched_param p = {
            .sched_priority = 50
        };

        int rc = pthread_setschedparam(
            pthread_self(),
            SCHED_FIFO,
            &p
        );

        if (rc != 0) {

            fprintf(
                stderr,
                "SCHED_FIFO unavailable (%s); "
                "continuing SCHED_OTHER\n",
                strerror(rc)
            );
        }
    }

    /* -----------------------------------------------------
     * 3.3 Initialize sampling timer
     * ----------------------------------------------------- */

    struct timespec expected;

    clock_gettime(
        CLOCK_MONOTONIC,
        &expected
    );

    add_ms(
        &expected,
        app->interval_ms
    );

    /* =====================================================
     * 4. MAIN SENSOR LOOP
     * ===================================================== */

    while (!app->stop) {

        int rc;

        /* -------------------------------------------------
         * 4.1 Sleep until expected sampling time
         * ------------------------------------------------- */

        do {

            rc = clock_nanosleep(
                CLOCK_MONOTONIC,
                TIMER_ABSTIME,
                &expected,
                NULL
            );

        } while (rc == EINTR && !app->stop);

        if (app->stop) {
            break;
        }

        if (rc != 0) {

            fprintf(
                stderr,
                "clock_nanosleep failed: %s\n",
                strerror(rc)
            );

            app->sensor_error = 1;
            app->stop = 1;

            break;
        }

        /* -------------------------------------------------
         * 4.2 Measure actual wake-up time
         * ------------------------------------------------- */

        struct timespec actual;

        clock_gettime(
            CLOCK_MONOTONIC,
            &actual
        );

        /* -------------------------------------------------
         * 4.3 Calculate and store jitter
         * ------------------------------------------------- */

        if (app->njitter < app->jitter_cap) {

            app->jitters[app->njitter++] =
                diff_us(
                    &actual,
                    &expected
                );
        }

        /* -------------------------------------------------
         * 4.4 Prepare sensor data
         * ------------------------------------------------- */

        SensorData d;

        /* ================================================
         * 5. MOCK SENSOR MODE
         * ================================================ */

        if (app->mock) {

            struct timespec real;

            clock_gettime(
                CLOCK_REALTIME,
                &real
            );

            /* Timestamp in milliseconds */

            d.ts_ms =
                (uint64_t)real.tv_sec * 1000
                + (uint64_t)real.tv_nsec / 1000000;

            /* Generate temperature: 15.0 - 45.0 C */

            d.temp =
                15.0
                + (double)(rand() % 301) / 10.0;

            /* Generate humidity: 30.0 - 90.0 % */

            d.hum =
                30.0
                + (double)(rand() % 601) / 10.0;
        }

        /* ================================================
         * 6. REAL SENSOR DEVICE MODE
         * ================================================ */

        else {

            char buf[128] = {0};

        ssize_t n;
        int retries = 0;

        do {
            n = read(fd, buf, sizeof(buf) - 1);

            if (n >= 0) {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN && retries < 10) {
                struct timespec delay = {
                    .tv_sec = 0,
                    .tv_nsec = 1000000L
                };

                nanosleep(&delay, NULL);

                retries++;
                continue;
            }

            break;

        } while (!app->stop);

            /* --------------------------------------------
             * 6.1 Handle read errors
             * -------------------------------------------- */

            if (n <= 0) {

                if (
                    n < 0 &&
                    (errno == EAGAIN || errno == EINTR)
                ) {

                    add_ms(
                        &expected,
                        app->interval_ms
                    );

                    continue;
                }

                log_push(
                    app,
                    "WARN device read failed "
                    "n=%zd errno=%d",
                    n,
                    n < 0 ? errno : 0
                );

                add_ms(
                    &expected,
                    app->interval_ms
                );

                continue;
            }

            /* --------------------------------------------
             * 6.2 Parse CSV sensor data
             * -------------------------------------------- */

            unsigned long long timestamp;

            if (sscanf(
                    buf,
                    "%llu,%lf,%lf",
                    &timestamp,
                    &d.temp,
                    &d.hum
                ) != 3) {

                log_push(
                    app,
                    "WARN malformed device sample"
                );

                add_ms(
                    &expected,
                    app->interval_ms
                );

                continue;
            }

            d.ts_ms = timestamp;
        }

        /* -------------------------------------------------
         * 7. Push data to circular buffer
         * ------------------------------------------------- */

        if (ring_push(&app->ring, d) != 0) {

            log_push(
                app,
                "WARN circular buffer overflow "
                "or buffer closed"
            );
        }

        /* -------------------------------------------------
         * 8. Print sensor data in verbose mode
         * ------------------------------------------------- */

        if (app->verbose) {

            printf(
                "SAMPLE ts=%llu "
                "temp=%.1f hum=%.1f\n",
                (unsigned long long)d.ts_ms,
                d.temp,
                d.hum
            );
        }

        /* -------------------------------------------------
         * 9. Calculate next sampling time
         * ------------------------------------------------- */

        add_ms(
            &expected,
            app->interval_ms
        );

        /* -------------------------------------------------
         * 10. Resynchronize if far behind schedule
         * ------------------------------------------------- */

        if (
            diff_us(&actual, &expected)
            > (long)app->interval_ms * 1000L
        ) {

            expected = actual;

            add_ms(
                &expected,
                app->interval_ms
            );
        }
    }

    /* -----------------------------------------------------
     * 11. Close sensor device
     * ----------------------------------------------------- */

    if (fd >= 0) {
        close(fd);
    }

    return NULL;
}