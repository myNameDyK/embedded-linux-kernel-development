#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H
#include <linux/ioctl.h>
#define SMS_DEVICE_PATH "/dev/sms_sensor"
#define SMS_IOC_MAGIC 's'
#define SMS_SET_INTERVAL _IOW(SMS_IOC_MAGIC, 1, unsigned int)
#endif
