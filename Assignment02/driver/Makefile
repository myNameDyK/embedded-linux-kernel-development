obj-m += sms_sensor.o
sms_sensor-y := sms_sensor_driver.o

KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

KCFLAGS := -march=armv7-a -Wa,-march=armv7-a

.PHONY: all arm clean

all: arm

arm:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) KCFLAGS="$(KCFLAGS)" modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
