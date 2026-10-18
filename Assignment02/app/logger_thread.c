/*
 * File: logger_thread.c
 * Description: Thread-safe logging system for SMS.
 *
 * Features:
 * - Receive log messages from multiple threads.
 * - Store messages in a shared log queue.
 * - Write messages to /tmp/sms.log.
 * - Rotate log file when size exceeds 1 MB.
 * - Flush log data every 5 seconds.
 * - Support graceful shutdown.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#include "sms.h"

/* =========================================================
 * 1. INITIALIZE LOG QUEUE
 * ========================================================= */

void log_init(LogQueue *q)
{
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    q->closed = 0;
    q->dropped = 0;

    pthread_mutex_init(&q->mu, NULL);
    pthread_cond_init(&q->available, NULL);
}

/* =========================================================
 * 2. CLOSE LOG QUEUE
 * ========================================================= */

void log_close(LogQueue *q)
{
    pthread_mutex_lock(&q->mu);

    q->closed = 1;

    pthread_cond_broadcast(&q->available);

    pthread_mutex_unlock(&q->mu);
}

/* =========================================================
 * 3. DESTROY LOG QUEUE
 * ========================================================= */

void log_destroy(LogQueue *q)
{
    pthread_cond_destroy(&q->available);
    pthread_mutex_destroy(&q->mu);
}

/* =========================================================
 * 4. PUSH MESSAGE INTO LOG QUEUE
 * ========================================================= */

void log_push(App *app, const char *format, ...)
{
    char line[SMS_LOG_LEN];

    struct timespec now;
    struct tm tm;

    int n;
    va_list args;

    /* -----------------------------------------------------
     * 4.1 Get current timestamp
     * ----------------------------------------------------- */

    clock_gettime(CLOCK_REALTIME, &now);

    localtime_r(&now.tv_sec, &tm);

    /* Format: YYYY-MM-DDTHH:MM:SS */

    n = (int)strftime(
        line,
        sizeof(line),
        "%Y-%m-%dT%H:%M:%S",
        &tm
    );

    if (n == 0) {
        return;
    }

    /* Add milliseconds */

    n += snprintf(
        line + n,
        sizeof(line) - (size_t)n,
        ".%03ld ",
        now.tv_nsec / 1000000L
    );

    if (n < 0 || (size_t)n >= sizeof(line)) {
        return;
    }

    /* -----------------------------------------------------
     * 4.2 Append log message
     * ----------------------------------------------------- */

    va_start(args, format);

    vsnprintf(
        line + n,
        sizeof(line) - (size_t)n,
        format,
        args
    );

    va_end(args);

    /* -----------------------------------------------------
     * 4.3 Access shared log queue
     * ----------------------------------------------------- */

    LogQueue *q = &app->logs;

    pthread_mutex_lock(&q->mu);

    /* Queue is full or closed */

    if (q->size == SMS_LOG_CAP || q->closed) {

        q->dropped++;
    }
    else {

        snprintf(
            q->lines[q->tail],
            SMS_LOG_LEN,
            "%s",
            line
        );

        q->tail = (q->tail + 1) % SMS_LOG_CAP;

        q->size++;

        pthread_cond_signal(&q->available);
    }

    pthread_mutex_unlock(&q->mu);
}

/* =========================================================
 * 5. LOG ROTATION
 * ========================================================= */

static FILE *rotate_if_needed(FILE *f)
{
    long size;

    if (!f) {
        return NULL;
    }

    size = ftell(f);

    if (size < 0 || size <= 1024 * 1024) {
        return f;
    }

    /* Close current log file */

    fclose(f);

    /* Rename sms.log to sms.log.1 */

    rename(
        SMS_LOG_PATH,
        SMS_LOG_PATH ".1"
    );

    /* Create new active log file */

    return fopen(
        SMS_LOG_PATH,
        "a"
    );
}

/* =========================================================
 * 6. LOGGER THREAD
 * ========================================================= */

void *logger_main(void *arg)
{
    App *app = arg;

    LogQueue *q = &app->logs;

    FILE *file = fopen(
        SMS_LOG_PATH,
        "a"
    );

    struct timespec next_flush;

    if (!file) {

        perror("fopen sms.log");

        return NULL;
    }

    /* -----------------------------------------------------
     * 6.1 Initialize flush timer
     * ----------------------------------------------------- */

    clock_gettime(
        CLOCK_REALTIME,
        &next_flush
    );

    next_flush.tv_sec += 5;

    /* =====================================================
     * 6.2 MAIN LOGGER LOOP
     * ===================================================== */

    for (;;) {

        char line[SMS_LOG_LEN] = {0};

        int have = 0;
        int closed;

        /* -----------------------------------------------
         * 6.3 Lock log queue
         * ----------------------------------------------- */

        pthread_mutex_lock(&q->mu);

        /* -----------------------------------------------
         * 6.4 Wait for a message
         * ----------------------------------------------- */

        while (!q->size && !q->closed) {

            int rc = pthread_cond_timedwait(
                &q->available,
                &q->mu,
                &next_flush
            );

            if (rc == ETIMEDOUT) {
                break;
            }

            if (rc != 0) {
                fprintf(
                    stderr,
                    "pthread_cond_timedwait: %s\n",
                    strerror(rc)
                );

                break;
            }
        }

        /* -----------------------------------------------
         * 6.5 Retrieve a log message
         * ----------------------------------------------- */

        if (q->size) {

            snprintf(
                line,
                sizeof(line),
                "%s",
                q->lines[q->head]
            );

            q->head = (q->head + 1) % SMS_LOG_CAP;

            q->size--;

            have = 1;
        }

        /* Check whether shutdown is complete */

        closed = q->closed && !q->size && !have;

        pthread_mutex_unlock(&q->mu);

        /* -----------------------------------------------
         * 6.6 Write message into file
         * ----------------------------------------------- */

        if (have) {

            fprintf(
                file,
                "%s\n",
                line
            );

            /* Check log rotation */

            file = rotate_if_needed(file);

            if (!file) {
                break;
            }
        }

        /* -----------------------------------------------
         * 6.7 Flush every 5 seconds
         * ----------------------------------------------- */

        struct timespec now;

        clock_gettime(
            CLOCK_REALTIME,
            &now
        );

        if (now.tv_sec >= next_flush.tv_sec) {

            fflush(file);

            next_flush = now;

            next_flush.tv_sec += 5;
        }

        /* -----------------------------------------------
         * 6.8 Exit if queue is closed and empty
         * ----------------------------------------------- */

        if (closed) {
            break;
        }
    }

    /* -----------------------------------------------------
     * 6.9 Final flush and cleanup
     * ----------------------------------------------------- */

    if (file) {

        fflush(file);

        fclose(file);
    }

    return NULL;
}