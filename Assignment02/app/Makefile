CC := gcc
ARM_CC := arm-linux-gnueabihf-gcc

SRC := sms_app.c circular_buffer.c sensor_thread.c processor_thread.c alert_thread.c logger_thread.c

CFLAGS := -std=gnu11 -O2 -g -Wall -Wextra -Werror -pthread
LIBS := -pthread -lrt -lm

.PHONY: all host arm clean

all: host

host:
	$(CC) $(CFLAGS) $(SRC) -o sms_app_x86 $(LIBS)

arm:
	$(ARM_CC) $(CFLAGS) $(SRC) -o sms_app $(LIBS)

clean:
	rm -f sms_app sms_app_x86
