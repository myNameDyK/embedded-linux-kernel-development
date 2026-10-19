/*
 * File: alert_thread.c
 * Description: Handle sensor alerts using POSIX message queue.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sms.h"

/* =========================================================
 * ALERT THREAD
 * ========================================================= */

void *alert_main(void *arg)
{
    App *app = arg;

    char msg[128];

    while (1) {

        /* -------------------------------------------------
         * 1. Calculate timeout for message receiving
         * ------------------------------------------------- */

        struct timespec until;

        clock_gettime(CLOCK_REALTIME, &until);

        until.tv_nsec += 100000000;

        if (until.tv_nsec >= 1000000000L) {
            until.tv_sec++;
            until.tv_nsec -= 1000000000L;
        }

        /* -------------------------------------------------
         * 2. Receive alert from POSIX message queue
         * ------------------------------------------------- */

        ssize_t n = mq_timedreceive(
            app->alarms,
            msg,
            sizeof(msg),
            NULL,
            &until
        );

        if (n >= 0) {

            unsigned long long ts;
            char kind[32];

            double val;
            double limit;

            /* Ensure message is null-terminated */
            msg[
                n < (ssize_t)sizeof(msg)
                    ? n
                    : (ssize_t)sizeof(msg) - 1
            ] = '\0';

            /* ---------------------------------------------
             * 3. Parse alert message
             * --------------------------------------------- */

            if (sscanf(
                    msg,
                    "%llu %31s %lf %lf",
                    &ts,
                    kind,
                    &val,
                    &limit
                ) == 4) {

                int type;

                /* -----------------------------------------
                 * 4. Determine alert type
                 * ----------------------------------------- */

                if (strcmp(kind, "TEMP_HIGH") == 0) {
                    type = 0;
                }
                else if (strcmp(kind, "HUMID_HIGH") == 0) {
                    type = 1;
                }
                else if (strcmp(kind, "TEMP_SPIKE") == 0) {
                    type = 2;
                }
                else {
                    fprintf(
                        stderr,
                        "Unknown alert type: %s\n",
                        kind
                    );

                    continue;
                }

                /* -----------------------------------------
                 * 5. Update alert counters
                 * ----------------------------------------- */

                pthread_mutex_lock(&app->counters_mu);

                app->alerts[type]++;

                pthread_mutex_unlock(&app->counters_mu);

                /* -----------------------------------------
                 * 6. Print alert to stderr
                 * ----------------------------------------- */

                fprintf(
                    stderr,
                    "[ALERT][%llu] TYPE: %s "
                    "VALUE: %.2f THRESHOLD: %.2f\n",
                    ts,
                    kind,
                    val,
                    limit
                );

                /* -----------------------------------------
                 * 7. Send alert to Logger Thread
                 * ----------------------------------------- */

                log_push(
                    app,
                    "ALERT ts=%llu type=%s "
                    "value=%.2f threshold=%.2f",
                    ts,
                    kind,
                    val,
                    limit
                );
            }
        }

        /* -------------------------------------------------
         * 8. Handle message queue errors
         * ------------------------------------------------- */

        else if (
            errno != ETIMEDOUT &&
            errno != EINTR &&
            errno != EAGAIN
        ) {

            perror("mq_timedreceive");

            break;
        }

        /* -------------------------------------------------
         * 9. Check shutdown condition
         * ------------------------------------------------- */

        if (app->stop) {

            struct mq_attr info;

            if (
                !mq_getattr(app->alarms, &info) &&
                info.mq_curmsgs == 0
            ) {
                break;
            }
        }
    }

    return NULL;
}