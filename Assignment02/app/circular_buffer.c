/*
 * File: circular_buffer.c
 * Description: Thread-safe circular buffer
 * for the Sensor Monitoring System.
 */

#include <stdlib.h>
#include "sms.h"

/* Initialize circular buffer */
int ring_init(Ring *r)
{
    r->data = calloc(SMS_CAP, sizeof(*r->data));

    if (!r->data) {
        return -1;
    }

    r->head = 0;
    r->tail = 0;
    r->size = 0;
    r->produced = 0;
    r->consumed = 0;
    r->overflow = 0;
    r->closed = 0;

    pthread_mutex_init(&r->mu, NULL);
    pthread_cond_init(&r->not_empty, NULL);

    return 0;
}

/* Close buffer and wake waiting consumers */
void ring_close(Ring *r)
{
    pthread_mutex_lock(&r->mu);

    r->closed = 1;

    pthread_cond_broadcast(&r->not_empty);

    pthread_mutex_unlock(&r->mu);
}

/* Destroy buffer and release resources */
void ring_destroy(Ring *r)
{
    pthread_cond_destroy(&r->not_empty);

    pthread_mutex_destroy(&r->mu);

    free(r->data);
}

/* Push sensor data into circular buffer */
int ring_push(Ring *r, SensorData s)
{
    int result = 0;

    pthread_mutex_lock(&r->mu);

    if (r->closed) {
        result = -1;
    }
    else if (r->size >= SMS_CAP) {
        r->overflow++;
        result = -1;
    }
    else {
        r->data[r->head] = s;

        r->head = (r->head + 1) % SMS_CAP;

        r->size++;
        r->produced++;

        pthread_cond_signal(&r->not_empty);
    }

    pthread_mutex_unlock(&r->mu);

    return result;
}

/* Pop sensor data from circular buffer */
int ring_pop(Ring *r, SensorData *s)
{
    pthread_mutex_lock(&r->mu);

    while (r->size == 0 && !r->closed) {
        pthread_cond_wait(&r->not_empty, &r->mu);
    }

    if (r->size == 0 && r->closed) {
        pthread_mutex_unlock(&r->mu);
        return -1;
    }

    *s = r->data[r->tail];

    r->tail = (r->tail + 1) % SMS_CAP;

    r->size--;
    r->consumed++;

    pthread_mutex_unlock(&r->mu);

    return 0;
}