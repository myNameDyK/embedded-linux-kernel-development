#!/bin/sh

echo '=== Test procfs & sysfs ==='

if lsmod | grep -q '^lab2_driver'; then
    echo 'lab2_driver already loaded'
else
    insmod /lib/modules/5.15.0/lab2_driver.ko
fi

[ -c /dev/lab2 ] || mknod /dev/lab2 c 240 0

echo '--- /proc/lab2_info (before write) ---'
cat /proc/lab2_info

echo 'Embedded Linux Lab2 Test Data' > /dev/lab2

echo '--- /proc/lab2_info (after write) ---'
cat /proc/lab2_info

SYSFS=/sys/class/lab2_class/lab2

echo '--- sysfs attributes ---'
echo "buffer_len : $(cat $SYSFS/buffer_len)"
echo "open_count : $(cat $SYSFS/open_count)"
echo "last_data  : $(cat $SYSFS/last_data)"

echo '=== procfs/sysfs Test PASSED ==='
