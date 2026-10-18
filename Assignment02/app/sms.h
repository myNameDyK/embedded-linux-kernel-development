#ifndef SMS_APP_H
#define SMS_APP_H
#define _GNU_SOURCE
#include <pthread.h>
#include <mqueue.h>
#include <signal.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#define SMS_CAP 64
#define SMS_LOG_CAP 256
#define SMS_LOG_LEN 256
#define SMS_LOG_PATH "/tmp/sms.log"
#define SMS_ALARM_NAME "/sms_alarm_reference"
typedef struct { uint64_t ts_ms; double temp, hum; } SensorData;
typedef struct {
SensorData *data;
size_t head, tail, size, produced, consumed, overflow;
int closed;
pthread_mutex_t mu;
pthread_cond_t not_empty;
} Ring;
typedef struct {
char lines[SMS_LOG_CAP][SMS_LOG_LEN];
size_t head, tail, size, dropped;
int closed;
pthread_mutex_t mu;
pthread_cond_t available;
} LogQueue;
typedef struct {
int interval_ms, duration_s, verbose, mock, fifo;
double temp_limit, hum_limit;
const char *device_path;
_Atomic int stop;
Ring ring;
LogQueue logs;
mqd_t alarms;
pthread_t sensor, processor, alert, logger;
pthread_mutex_t counters_mu;
size_t alerts[3];
long *jitters;
size_t njitter, jitter_cap;
int sensor_error;
} App;
int ring_init(Ring *r);
void ring_close(Ring *r);
void ring_destroy(Ring *r);
int ring_push(Ring *r, SensorData s);
int ring_pop(Ring *r, SensorData *s);
void log_init(LogQueue *q);
void log_close(LogQueue *q);
void log_destroy(LogQueue *q);
void log_push(App *app, const char *format, ...) __attribute__((format(printf, 2, 3)));
void *sensor_main(void *arg);
void *processor_main(void *arg);
void *alert_main(void *arg);
void *logger_main(void *arg);
void print_jitter(const App *app);
#endif
