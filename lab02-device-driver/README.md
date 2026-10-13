# LAB-02 — Embedded Linux Device Driver & File System

This directory contains the implementation for **LAB-02: Device Driver & File System Management** on an ARMv7-A Cortex-A9 target using QEMU `vexpress-a9`.

LAB-02 extends the LAB-01 Linux system by adding a loadable character device driver, procfs/sysfs interfaces, automatic module loading during boot, and MTD/JFFS2 experiments.

## Main Features

- ARM Linux kernel module: `lab2_driver.ko`
- Character device with fixed major number `240`
- Device interface: `/dev/lab2`
- Supported operations:
  - `open`
  - `read`
  - `write`
  - `release`
- Safe userspace/kernel data transfer with:
  - `copy_from_user()`
  - `copy_to_user()`
- Mutex protection for shared driver data
- procfs statistics:
  - `/proc/lab2_info`
- sysfs attributes:
  - `/sys/class/lab2_class/lab2/buffer_len`
  - `/sys/class/lab2_class/lab2/open_count`
  - `/sys/class/lab2_class/lab2/last_data`
- BusyBox boot integration through `/etc/inittab` and `/etc/init.d/rcS`
- Test scripts for the character device, procfs, and sysfs
- MTD/JFFS2 experiment using `nandsim` on the Ubuntu host

## Repository Layout

```text
lab02-device-driver/
├── driver/
│   ├── Makefile
│   ├── lab2_driver.c
│   └── lab2_driver.ko
└── rootfs/
    ├── init
    ├── etc/
    │   ├── hostname
    │   ├── inittab
    │   ├── passwd
    │   └── init.d/
    │       └── rcS
    └── usr/
        └── bin/
            ├── test_driver.sh
            └── test_procfs.sh
```

The complete runtime root filesystem is generated from the LAB-01 BusyBox rootfs. Only LAB-02-specific files are kept in this repository.

## Target Environment

- Architecture: ARMv7-A
- CPU: Cortex-A9
- QEMU machine: `vexpress-a9`
- Linux kernel: 5.15
- Cross compiler: `arm-linux-gnueabihf-`
- Root filesystem: BusyBox initramfs
- RAM: 512 MB
- SMP: 2 CPUs

LAB-01 must already provide:

```text
~/embedded_lab1/output/zImage
~/embedded_lab1/output/vexpress-v2p-ca9.dtb
~/embedded_lab1/rootfs/initramfs/
~/embedded_lab1/kernel/linux-5.15/
```

## Driver Architecture

```text
Userspace
   |
   | open / read / write / close
   v
VFS
   |
   v
lab2_fops
   |
   +--> lab2_open()
   +--> lab2_read()
   +--> lab2_write()
   +--> lab2_release()
   |
   v
device_buffer[1024]
```

The driver also exports internal state through:

```text
/proc/lab2_info
/sys/class/lab2_class/lab2/
```

## Build the Driver

```bash
cd ~/embedded_lab2/driver
make clean
make
```

Check the generated module:

```bash
file lab2_driver.ko
modinfo lab2_driver.ko
```

The module must be built for ARM and against the same Linux 5.15 kernel used by QEMU.

## Boot Integration

During boot, BusyBox `init` reads:

```text
/etc/inittab
```

which runs:

```text
/etc/init.d/rcS
```

`rcS` mounts the virtual filesystems, loads `lab2_driver.ko`, ensures `/dev/lab2` exists, and prepares the system for testing.

Runtime flow:

```text
Linux kernel
    ↓
/sbin/init
    ↓
/etc/inittab
    ↓
/etc/init.d/rcS
    ↓
insmod lab2_driver.ko
    ↓
/dev/lab2 + /proc/lab2_info + sysfs attributes
    ↓
BusyBox shell
```

## QEMU Boot

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

## Testing in QEMU

Check the module and device:

```sh
lsmod | grep lab2
ls -l /dev/lab2
```

Test basic read/write:

```sh
echo "Hello from userspace, LAB-02!" > /dev/lab2
cat /dev/lab2
```

Run the provided scripts:

```sh
test_driver.sh
test_procfs.sh
```

Inspect procfs:

```sh
cat /proc/lab2_info
```

Inspect sysfs:

```sh
cat /sys/class/lab2_class/lab2/buffer_len
cat /sys/class/lab2_class/lab2/open_count
cat /sys/class/lab2_class/lab2/last_data
```

Inspect kernel logs:

```sh
dmesg | grep lab2
```

## MTD/JFFS2

The MTD/JFFS2 part is performed on the **Ubuntu host**, not inside the normal QEMU LAB-02 flow.

`nandsim` is used to emulate raw NAND flash in RAM. A JFFS2 image is created with `mkfs.jffs2`, written to the simulated MTD device with `nandwrite`, and mounted through its matching `mtdblock` device.

> **Safety:** Never assume that `/dev/mtd0` is the simulator. On a real host, an existing MTD device may contain firmware. Always inspect `/proc/mtd` and verify that the target is explicitly named **NAND simulator partition 0** before running `flash_erase` or `nandwrite`.

See [`BUILD_FROM_SCRATCH.md`](BUILD_FROM_SCRATCH.md) for the complete reproduction procedure.

## Notes

- `lab2_driver.c` currently uses major number `240`.
- The driver buffer size is `1024` bytes.
- `device_create()` exposes the device through the Linux device model under `/sys/class/lab2_class/lab2`.
- `proc_create()` creates `/proc/lab2_info`.
- The current QEMU command uses `rdinit=/sbin/init`; therefore BusyBox `/sbin/init` is selected directly as PID 1.
- The `nandsim` device exists in RAM. Its contents survive unmount/remount while the simulator remains loaded, but are not persistent across unloading `nandsim` or rebooting the host.

## Author

Nguyen Duy Khoa  
Embedded Linux Systems
