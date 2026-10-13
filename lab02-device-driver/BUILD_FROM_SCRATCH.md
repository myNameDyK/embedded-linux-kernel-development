# LAB-02 — Build From Scratch

This guide describes how to reproduce LAB-02 from a working LAB-01 environment.

LAB-02 adds:

- A Linux character device driver
- `/dev/lab2`
- procfs interface
- sysfs attributes
- Automatic driver loading during boot
- Driver test scripts
- MTD/JFFS2 experiment using `nandsim`

---

## 1. System Overview

The complete LAB-02 workflow is:

```text
LAB-01 working system
        |
        v
Create LAB-02 workspace
        |
        v
Build lab2_driver.ko
        |
        v
Copy module into BusyBox rootfs
        |
        v
Configure inittab and rcS
        |
        v
Package initramfs
        |
        v
Boot QEMU
        |
        v
Test /dev/lab2
        |
        +----> procfs
        |
        +----> sysfs
        |
        v
MTD/JFFS2 experiment
```

Target environment:

```text
Architecture    : ARMv7-A
CPU             : Cortex-A9
QEMU machine    : vexpress-a9
Linux kernel    : 5.15
Cross compiler  : arm-linux-gnueabihf-
Root filesystem : BusyBox initramfs
Memory          : 512 MB
SMP             : 2 CPUs
```

---

## 2. Check LAB-01

LAB-02 is built on top of LAB-01.

Check that the LAB-01 outputs exist:

```bash
ls -lh ~/embedded_lab1/output/zImage
ls -lh ~/embedded_lab1/output/vexpress-v2p-ca9.dtb
ls -lh ~/embedded_lab1/output/initramfs.cpio.gz
```

Check BusyBox:

```bash
ls -lh ~/embedded_lab1/rootfs/initramfs/bin/busybox
```

Check the Linux kernel source:

```bash
ls ~/embedded_lab1/kernel/linux-5.15
```

The expected LAB-01 structure is approximately:

```text
~/embedded_lab1/
├── kernel/
│   └── linux-5.15/
├── output/
│   ├── zImage
│   ├── vexpress-v2p-ca9.dtb
│   └── initramfs.cpio.gz
└── rootfs/
    └── initramfs/
```

---

## 3. Install Required Host Tools

Update the package list:

```bash
sudo apt update
```

Install the required packages:

```bash
sudo apt install -y \
    build-essential \
    gcc-arm-linux-gnueabihf \
    qemu-system-arm \
    cpio \
    gzip \
    mtd-utils \
    squashfs-tools
```

Check the ARM cross compiler:

```bash
arm-linux-gnueabihf-gcc --version
```

---

## 4. Create the LAB-02 Workspace

Create the LAB-02 directories:

```bash
mkdir -p ~/embedded_lab2/{driver,rootfs/initramfs,mtd,output,configs}
```

Check:

```bash
tree -L 3 ~/embedded_lab2
```

Expected structure:

```text
~/embedded_lab2/
├── configs/
├── driver/
├── mtd/
├── output/
└── rootfs/
    └── initramfs/
```

---

## 5. Copy the LAB-01 Root Filesystem

LAB-02 reuses the BusyBox root filesystem from LAB-01.

Copy it:

```bash
cp -a ~/embedded_lab1/rootfs/initramfs/. \
      ~/embedded_lab2/rootfs/initramfs/
```

Check BusyBox:

```bash
ls -lh ~/embedded_lab2/rootfs/initramfs/bin/busybox
```

---

## 6. Copy the LAB-02 Driver Source

The Git repository contains:

```text
lab02-device-driver/driver/
├── Makefile
├── lab2_driver.c
└── lab2_driver.ko
```

Copy the source and Makefile into the working directory:

```bash
cp ~/embedded-linux-kernel-development/lab02-device-driver/driver/lab2_driver.c \
   ~/embedded_lab2/driver/
```

```bash
cp ~/embedded-linux-kernel-development/lab02-device-driver/driver/Makefile \
   ~/embedded_lab2/driver/
```

Check:

```bash
ls -la ~/embedded_lab2/driver
```

---

## 7. Driver Makefile

The Makefile uses the LAB-01 Linux 5.15 kernel tree:

```makefile
KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

obj-m += lab2_driver.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) clean
```

Important variables:

```text
KDIR
    Linux kernel source used to build the module.

ARCH=arm
    Build for ARM architecture.

CROSS_COMPILE=arm-linux-gnueabihf-
    Use the ARM cross compiler.

obj-m += lab2_driver.o
    Build lab2_driver.c as a loadable kernel module.
```

---

## 8. Prepare the Kernel Tree

If the LAB-01 kernel was already successfully built, the kernel tree is normally ready.

Otherwise:

```bash
cd ~/embedded_lab1/kernel/linux-5.15
```

Set the target architecture:

```bash
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
```

Configure the kernel:

```bash
make vexpress_defconfig
```

Prepare it for external modules:

```bash
make modules_prepare
```

The module should be built against the same kernel configuration used to generate the LAB-01 `zImage`.

---

## 9. Build the LAB-02 Driver

Go to the driver directory:

```bash
cd ~/embedded_lab2/driver
```

Remove old build files:

```bash
make clean
```

Build the module:

```bash
make
```

After a successful build:

```bash
ls -lh lab2_driver.ko
```

Check its architecture:

```bash
file lab2_driver.ko
```

It must be an ARM kernel module, not x86-64.

Check module information:

```bash
modinfo lab2_driver.ko
```

Check kernel compatibility:

```bash
modinfo lab2_driver.ko | grep vermagic
```

---

## 10. LAB-02 Character Driver

The driver registers:

```text
Device name : lab2
Major       : 240
Minor       : 0
Buffer size : 1024 bytes
```

Main file operations:

```text
open()
read()
write()
release()
```

The driver uses:

```text
copy_from_user()
copy_to_user()
mutex
```

Main userspace interface:

```text
/dev/lab2
```

The data flow is:

```text
Userspace
   |
   | open/read/write/close
   v
VFS
   |
   v
lab2_fops
   |
   +---- lab2_open()
   |
   +---- lab2_read()
   |
   +---- lab2_write()
   |
   +---- lab2_release()
   |
   v
device_buffer[1024]
```

---

## 11. Install the Module into the Root Filesystem

Create the module directory:

```bash
mkdir -p ~/embedded_lab2/rootfs/initramfs/lib/modules/5.15.0
```

Copy the driver:

```bash
cp ~/embedded_lab2/driver/lab2_driver.ko \
   ~/embedded_lab2/rootfs/initramfs/lib/modules/5.15.0/
```

Check:

```bash
ls -lh \
~/embedded_lab2/rootfs/initramfs/lib/modules/5.15.0/lab2_driver.ko
```

---

## 12. Copy LAB-02 Rootfs Configuration Files

Return to the Git repository:

```bash
cd ~/embedded-linux-kernel-development
```

Copy the hostname file:

```bash
cp lab02-device-driver/rootfs/etc/hostname \
   ~/embedded_lab2/rootfs/initramfs/etc/hostname
```

Copy `inittab`:

```bash
cp lab02-device-driver/rootfs/etc/inittab \
   ~/embedded_lab2/rootfs/initramfs/etc/inittab
```

Copy startup script:

```bash
cp lab02-device-driver/rootfs/etc/init.d/rcS \
   ~/embedded_lab2/rootfs/initramfs/etc/init.d/rcS
```

Copy the character-device test:

```bash
cp lab02-device-driver/rootfs/usr/bin/test_driver.sh \
   ~/embedded_lab2/rootfs/initramfs/usr/bin/test_driver.sh
```

Copy the procfs/sysfs test:

```bash
cp lab02-device-driver/rootfs/usr/bin/test_procfs.sh \
   ~/embedded_lab2/rootfs/initramfs/usr/bin/test_procfs.sh
```

Set executable permissions:

```bash
chmod +x ~/embedded_lab2/rootfs/initramfs/etc/init.d/rcS
chmod +x ~/embedded_lab2/rootfs/initramfs/usr/bin/test_driver.sh
chmod +x ~/embedded_lab2/rootfs/initramfs/usr/bin/test_procfs.sh
```

---

## 13. BusyBox `inittab`

The LAB-02 configuration is:

```text
::sysinit:/etc/init.d/rcS
::askfirst:-/bin/sh
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
```

Boot sequence:

```text
Linux kernel
      |
      v
/sbin/init
      |
      v
/etc/inittab
      |
      v
/etc/init.d/rcS
```

`::sysinit` tells BusyBox init to execute:

```text
/etc/init.d/rcS
```

during system initialization.

---

## 14. LAB-02 `rcS`

The startup script performs these main operations:

```text
mount /proc
mount /sys
mount /tmp
mount /dev
set hostname
load lab2_driver.ko
create/check /dev/lab2
set device permissions
```

The module is loaded from:

```text
/lib/modules/5.15.0/lab2_driver.ko
```

using:

```sh
insmod /lib/modules/5.15.0/lab2_driver.ko
```

If required, the device node is created with:

```sh
mknod /dev/lab2 c 240 0
```

Then:

```sh
chmod 666 /dev/lab2
```

---

## 15. Verify the Root Filesystem

Go to:

```bash
cd ~/embedded_lab2/rootfs/initramfs
```

Check the module:

```bash
ls -la lib/modules/5.15.0/lab2_driver.ko
```

Check BusyBox:

```bash
ls -la bin/busybox
```

Check initialization files:

```bash
ls -la etc/inittab
ls -la etc/init.d/rcS
```

Check test scripts:

```bash
ls -la usr/bin/test_driver.sh
ls -la usr/bin/test_procfs.sh
```

Check module architecture:

```bash
file lib/modules/5.15.0/lab2_driver.ko
```

---

## 16. Package the LAB-02 Initramfs

Create the output directory:

```bash
mkdir -p ~/embedded_lab2/output
```

Go to the root of the initramfs:

```bash
cd ~/embedded_lab2/rootfs/initramfs
```

Create the archive:

```bash
find . | cpio -H newc -o | gzip \
    > ~/embedded_lab2/output/initramfs_lab2.cpio.gz
```

Check the result:

```bash
ls -lh ~/embedded_lab2/output/initramfs_lab2.cpio.gz
```

---

## 17. Boot LAB-02 with QEMU

Run:

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -cpu cortex-a9 \
  -m 512M \
  -smp 2 \
  -nographic \
  -kernel ~/embedded_lab1/output/zImage \
  -dtb ~/embedded_lab1/output/vexpress-v2p-ca9.dtb \
  -initrd ~/embedded_lab2/output/initramfs_lab2.cpio.gz \
  -append 'console=ttyAMA0,115200 rdinit=/sbin/init mem=512M'
```

Expected LAB-02 boot messages include:

```text
[ LAB-02 ] Starting system initialization...
[ LAB-02 ] Loading lab2_driver module...
[ LAB-02 ] lab2_driver loaded OK
[ LAB-02 ] /dev/lab2 created (major=240, minor=0)
[ LAB-02 ] System initialization complete!
```

Press Enter when BusyBox displays:

```text
Please press Enter to activate this console.
```

---

## 18. Check the Driver in QEMU

Inside QEMU:

```sh
lsmod
```

Or:

```sh
lsmod | grep lab2
```

Check the device:

```sh
ls -l /dev/lab2
```

Expected device type:

```text
c
```

with:

```text
major = 240
minor = 0
```

Check kernel messages:

```sh
dmesg | grep lab2
```

---

## 19. Test Character Device Write

Write data:

```sh
echo "Hello from userspace, LAB-02!" > /dev/lab2
```

The flow is:

```text
echo
 |
 v
open("/dev/lab2")
 |
 v
lab2_open()
 |
 v
lab2_write()
 |
 v
copy_from_user()
 |
 v
device_buffer
 |
 v
lab2_release()
```

---

## 20. Test Character Device Read

Read:

```sh
cat /dev/lab2
```

Expected:

```text
Hello from userspace, LAB-02!
```

Read flow:

```text
device_buffer
 |
 v
lab2_read()
 |
 v
copy_to_user()
 |
 v
cat
```

The driver uses the file offset to return EOF after all available data has been read.

---

## 21. Run the Driver Test Script

Inside QEMU:

```sh
test_driver.sh
```

The script performs:

```text
Check module
Check /dev/lab2
Write data
Read data
Perform multiple write/read tests
Display driver dmesg
```

Expected final message:

```text
=== Test PASSED ===
```

---

## 22. procfs Interface

The driver creates:

```text
/proc/lab2_info
```

Check it:

```sh
cat /proc/lab2_info
```

It reports information such as:

```text
Driver name
Major number
Buffer size
Data length
Open count
Last data
```

Example structure:

```text
LAB-02 Driver Statistics
======================
Driver name   : lab2
Major number  : 240
Buffer size   : 1024 bytes
Data length   : ...
Open count    : ...
Last data     : [...]
```

---

## 23. sysfs Interface

The driver creates a device class:

```text
/sys/class/lab2_class/
```

The device is:

```text
/sys/class/lab2_class/lab2/
```

Available attributes:

```text
buffer_len
open_count
last_data
```

Check them:

```sh
cat /sys/class/lab2_class/lab2/buffer_len
```

```sh
cat /sys/class/lab2_class/lab2/open_count
```

```sh
cat /sys/class/lab2_class/lab2/last_data
```

---

## 24. Run the procfs/sysfs Test

Inside QEMU:

```sh
test_procfs.sh
```

The script:

```text
checks the driver
reads /proc/lab2_info
writes test data to /dev/lab2
reads /proc/lab2_info again
reads sysfs attributes
```

Expected final message:

```text
=== procfs/sysfs Test PASSED ===
```

---

## 25. Check Kernel Logs

Use:

```sh
dmesg | grep lab2
```

Example messages:

```text
lab2_driver: initializing module
lab2_driver: loaded, major=240
lab2_driver: /proc/lab2_info created
lab2_driver: device opened
lab2_driver: received ... bytes
lab2_driver: sent ... bytes to user
lab2_driver: device closed
```

---

# MTD AND JFFS2

## 26. Important Safety Warning

The following MTD/JFFS2 experiment runs on the **Ubuntu host**.

It is not the normal QEMU character-driver test.

Commands such as:

```text
flash_erase
nandwrite
```

are destructive.

Do NOT assume:

```text
/dev/mtd0
```

is safe.

A physical computer may expose firmware flash as an MTD device.

Always run:

```bash
cat /proc/mtd
```

and verify that the target device is explicitly:

```text
NAND simulator partition 0
```

before running erase or write commands.

---

## 27. Install MTD Utilities

On the Ubuntu host:

```bash
sudo apt install -y mtd-utils
```

Available utilities include:

```text
mtdinfo
flash_erase
nandwrite
nanddump
mkfs.jffs2
```

---

## 28. Load `nandsim`

Load the NAND simulator:

```bash
sudo modprobe nandsim first_id_byte=0x20 second_id_byte=0x35
```

Load the block interface:

```bash
sudo modprobe mtdblock
```

Load JFFS2 support:

```bash
sudo modprobe jffs2
```

The simulated NAND configuration corresponds to approximately:

```text
Size       : 32 MiB
Page size  : 512 bytes
Erase size : 16 KiB
```

---

## 29. Find the Correct MTD Device

Run:

```bash
cat /proc/mtd
```

Example:

```text
dev:    size   erasesize  name
mtd1: 02000000 00004000 "NAND simulator partition 0"
```

Also:

```bash
ls -l /dev/mtd*
```

In this example, the simulator is:

```text
/dev/mtd1
```

and the block interface is:

```text
/dev/mtdblock1
```

Your system may use a different number.

---

## 30. Set MTD Variables

Only after confirming the simulator from `/proc/mtd`, set:

```bash
MTD=/dev/mtd1
MTDBLOCK=/dev/mtdblock1
```

Replace `1` with the actual simulator number on your computer.

Check:

```bash
sudo mtdinfo "$MTD"
```

Do not continue unless the selected device is definitely the NAND simulator.

---

## 31. Create JFFS2 Test Data

Remove old temporary data:

```bash
rm -rf /tmp/jffs2_data
```

Create directories:

```bash
mkdir -p /tmp/jffs2_data/logs
```

Create sample files:

```bash
echo 'Hello from JFFS2!' \
    > /tmp/jffs2_data/hello.txt
```

```bash
echo 'Embedded Linux Lab2' \
    > /tmp/jffs2_data/info.txt
```

Create a log:

```bash
date > /tmp/jffs2_data/logs/boot.log
```

Check:

```bash
find /tmp/jffs2_data -type f -print
```

---

## 32. Create the JFFS2 Image

Run:

```bash
sudo mkfs.jffs2 \
  --root=/tmp/jffs2_data \
  --eraseblock=0x4000 \
  --output=/tmp/rootfs.jffs2 \
  --pad \
  --no-cleanmarkers
```

Explanation:

```text
--root
    Source directory.

--eraseblock=0x4000
    16 KiB erase block.

--output
    Output JFFS2 image.

--pad
    Pad the filesystem image.

--no-cleanmarkers
    Do not place clean markers in the data area.
```

Check:

```bash
ls -lh /tmp/rootfs.jffs2
```

---

## 33. Verify the MTD Device Again

Before erasing:

```bash
cat /proc/mtd
```

Then:

```bash
sudo mtdinfo "$MTD"
```

Make sure the selected device is:

```text
NAND simulator partition 0
```

Do not continue if it is a real firmware device.

---

## 34. Erase the Simulated NAND

Only after verification:

```bash
sudo flash_erase "$MTD" 0 0
```

This erases all erase blocks of the selected simulated NAND device.

---

## 35. Write the JFFS2 Image

Write the image:

```bash
sudo nandwrite -p "$MTD" /tmp/rootfs.jffs2
```

The `-p` option pads data to the NAND page size when necessary.

---

## 36. Mount JFFS2

Create a mount point:

```bash
sudo mkdir -p /mnt/jffs2
```

Mount:

```bash
sudo mount -t jffs2 "$MTDBLOCK" /mnt/jffs2
```

Check:

```bash
ls -la /mnt/jffs2
```

Read the files:

```bash
cat /mnt/jffs2/hello.txt
```

Expected:

```text
Hello from JFFS2!
```

Check:

```bash
cat /mnt/jffs2/info.txt
```

Expected:

```text
Embedded Linux Lab2
```

Check log:

```bash
cat /mnt/jffs2/logs/boot.log
```

---

## 37. Test Unmount and Remount

Unmount:

```bash
sudo umount /mnt/jffs2
```

Mount again:

```bash
sudo mount -t jffs2 "$MTDBLOCK" /mnt/jffs2
```

Check the data:

```bash
cat /mnt/jffs2/hello.txt
```

The file should still exist.

However, `nandsim` stores the simulated NAND in RAM.

Therefore, the data does not survive:

```text
rmmod nandsim
```

or a host reboot.

---

## 38. Optional NAND Dump

The simulated NAND can be dumped for inspection:

```bash
sudo nanddump \
    -f /tmp/nand_dump.bin \
    "$MTD"
```

Check:

```bash
ls -lh /tmp/nand_dump.bin
```

---

# REBUILD WORKFLOW

## 39. Rebuild After Modifying `lab2_driver.c`

Whenever the driver source changes:

```bash
cd ~/embedded_lab2/driver
```

Clean:

```bash
make clean
```

Build:

```bash
make
```

Copy the new module:

```bash
cp lab2_driver.ko \
   ~/embedded_lab2/rootfs/initramfs/lib/modules/5.15.0/
```

Repackage the initramfs:

```bash
cd ~/embedded_lab2/rootfs/initramfs
```

```bash
find . | cpio -H newc -o | gzip \
    > ~/embedded_lab2/output/initramfs_lab2.cpio.gz
```

Then boot QEMU again.

The development loop is:

```text
Edit lab2_driver.c
       |
       v
make
       |
       v
lab2_driver.ko
       |
       v
Copy into rootfs
       |
       v
Repack initramfs
       |
       v
Boot QEMU
       |
       v
Run tests
       |
       v
Check dmesg
```

---

# TROUBLESHOOTING

## 40. `insmod: invalid module format`

Check the module architecture:

```bash
file ~/embedded_lab2/driver/lab2_driver.ko
```

Check `vermagic`:

```bash
modinfo ~/embedded_lab2/driver/lab2_driver.ko \
    | grep vermagic
```

Inside QEMU:

```sh
uname -r
```

The module must be compiled for ARM and for the kernel used by QEMU.

---

## 41. `/dev/lab2` Does Not Exist

Inside QEMU:

```sh
lsmod | grep lab2
```

Check:

```sh
dmesg | grep lab2
```

If the module is loaded but the node is missing:

```sh
mknod /dev/lab2 c 240 0
chmod 666 /dev/lab2
```

---

## 42. `/proc/lab2_info` Does Not Exist

Check module initialization:

```sh
dmesg | grep lab2
```

The driver creates `/proc/lab2_info` during `lab2_init()`.

---

## 43. sysfs Attributes Are Missing

Check:

```sh
ls -la /sys/class/lab2_class/lab2
```

Expected attributes:

```text
buffer_len
open_count
last_data
```

---

## 44. `cat /dev/lab2` Does Not Stop

The character driver's `read()` function must eventually return:

```text
0
```

to indicate EOF.

The LAB-02 driver uses:

```text
*offset
```

to determine when all data has been read.

---

## 45. Wrong MTD Device Number

Never assume:

```text
/dev/mtd0
```

Run:

```bash
cat /proc/mtd
```

Find:

```text
NAND simulator partition 0
```

Then use the matching:

```text
/dev/mtdX
/dev/mtdblockX
```

---

# FINAL CHECKLIST

Before considering LAB-02 complete:

```text
[ ] LAB-01 zImage exists

[ ] LAB-01 vexpress-v2p-ca9.dtb exists

[ ] LAB-01 BusyBox rootfs exists

[ ] lab2_driver.c exists

[ ] Makefile exists

[ ] lab2_driver.ko builds successfully

[ ] lab2_driver.ko is ARM

[ ] Kernel vermagic matches

[ ] lab2_driver.ko is copied into:
    /lib/modules/5.15.0/

[ ] /etc/inittab exists

[ ] /etc/init.d/rcS exists

[ ] rcS is executable

[ ] test_driver.sh is executable

[ ] test_procfs.sh is executable

[ ] initramfs_lab2.cpio.gz is generated

[ ] QEMU boots successfully

[ ] lab2_driver appears in lsmod

[ ] /dev/lab2 exists

[ ] Writing to /dev/lab2 works

[ ] Reading from /dev/lab2 works

[ ] /proc/lab2_info works

[ ] sysfs buffer_len works

[ ] sysfs open_count works

[ ] sysfs last_data works

[ ] test_driver.sh passes

[ ] test_procfs.sh passes

[ ] dmesg contains lab2_driver messages

[ ] MTD/JFFS2 experiment uses only verified nandsim device
```

---

## LAB-02 Final Runtime Flow

```text
HOST
 |
 +-- Linux 5.15 source
 |
 +-- arm-linux-gnueabihf-gcc
 |
 +-- lab2_driver.c
 |       |
 |       v
 |   lab2_driver.ko
 |
 +-- BusyBox rootfs
 |       |
 |       +-- /etc/inittab
 |       +-- /etc/init.d/rcS
 |       +-- /lib/modules/5.15.0/lab2_driver.ko
 |       +-- /usr/bin/test_driver.sh
 |       +-- /usr/bin/test_procfs.sh
 |       |
 |       v
 |   initramfs_lab2.cpio.gz
 |
 v
QEMU vexpress-a9
 |
 v
Linux kernel
 |
 v
/sbin/init
 |
 v
/etc/inittab
 |
 v
/etc/init.d/rcS
 |
 v
insmod lab2_driver.ko
 |
 +----> /dev/lab2
 |
 +----> /proc/lab2_info
 |
 +----> /sys/class/lab2_class/lab2/
 |
 v
BusyBox shell
 |
 +----> test_driver.sh
 |
 +----> test_procfs.sh
```
