/*
 * File: processor_thread.c
 * Description: Process sensor samples, calculate rolling
 * averages, and detect abnormal sensor values.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include "sms.h"

/* =========================================================
 * 1. SEND ALARM TO POSIX MESSAGE QUEUE
 * ========================================================= */

static void alarm_send(
    App *app,
    const SensorData *s,
    int type,
    double value,
    double threshold
)
{
    static const char *names[] = {
        "TEMP_HIGH",
        "HUMID_HIGH",
        "TEMP_SPIKE"
    };

    char msg[128];

    /* Check alarm type */
    if (type < 0 || type >= 3) {
        return;
    }

    /* Format alarm message */
    snprintf(
        msg,
        sizeof(msg),
        "%llu %s %.2f %.2f",
        (unsigned long long)s->ts_ms,
        names[type],
        value,
        threshold
    );

    /* Send alarm to Alert Thread */
    if (mq_send(
            app->alarms,
            msg,
            strlen(msg) + 1,
            0
        ) == -1) {

        log_push(
            app,
            "WARN alarm queue send failed: %s (%s)",
            names[type],
            strerror(errno)
        );
    }
}

/* =========================================================
 * 2. PROCESSOR THREAD
 * ========================================================= */

void *processor_main(void *arg)
{
    App *app = arg;

    SensorData s;

    /* Arrays for rolling average */
    double temps[10] = {0};
    double hums[10] = {0};

    /* Running sums */
    double sum_t = 0;
    double sum_h = 0;

    /* Previous temperature */
    double prev_temp = 0;

    /* Rolling window variables */
    int pos = 0;
    int size = 0;
    int has_prev = 0;

    /* =====================================================
     * 3. RECEIVE SENSOR DATA FROM CIRCULAR BUFFER
     * ===================================================== */

    while (ring_pop(&app->ring, &s) == 0) {

        /* -------------------------------------------------
         * 3.1 Update rolling average window
         * ------------------------------------------------- */

        if (size == 10) {

            /* Remove oldest sample */
            sum_t -= temps[pos];
            sum_h -= hums[pos];

        } else {

            /* Window has not reached 10 samples */
            size++;
        }

        /* Store current sample */
        temps[pos] = s.temp;
        hums[pos] = s.hum;

        /* Add current sample to sums */
        sum_t += s.temp;
        sum_h += s.hum;

        /* Move to next circular position */
        pos = (pos + 1) % 10;

        /* -------------------------------------------------
         * 3.2 Detect high temperature
         * ------------------------------------------------- */

        if (s.temp > app->temp_limit) {

            alarm_send(
                app,
                &s,
                0,
                s.temp,
                app->temp_limit
            );
        }

        /* -------------------------------------------------
         * 3.3 Detect high humidity
         * ------------------------------------------------- */

        if (s.hum > app->hum_limit) {

            alarm_send(
                app,
                &s,
                1,
                s.hum,
                app->hum_limit
            );
        }

        /* -------------------------------------------------
         * 3.4 Detect temperature spike
         * ------------------------------------------------- */

        if (has_prev &&
            fabs(s.temp - prev_temp) > 5.0) {

            alarm_send(
                app,
                &s,
                2,
                fabs(s.temp - prev_temp),
                5.0
            );
        }

        /* -------------------------------------------------
         * 3.5 Update previous temperature
         * ------------------------------------------------- */

        prev_temp = s.temp;
        has_prev = 1;

        /* -------------------------------------------------
         * 3.6 Send processed sample to Logger Thread
         * ------------------------------------------------- */

        log_push(
            app,
            "SAMPLE ts=%llu temp=%.2f hum=%.2f "
            "avg_temp=%.2f avg_hum=%.2f",
            (unsigned long long)s.ts_ms,
            s.temp,
            s.hum,
            sum_t / size,
            sum_h / size
        );
    }

    return NULL;
}