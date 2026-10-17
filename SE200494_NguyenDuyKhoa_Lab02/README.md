# Lab 02: Character Device Driver and JFFS2

## Introduction

This laboratory extends the Embedded Linux environment developed in Lab 01 by introducing a loadable Linux character device driver and integrating it into the target root filesystem.

The implementation provides a character device interface for communication between userspace and kernel space, runtime statistics through procfs, device attributes through sysfs, and automatic driver initialization during system startup. The laboratory also includes an independent MTD and JFFS2 experiment performed on the Ubuntu development host using a simulated NAND flash device.

The completed system was validated on an ARM Cortex-A9 target emulated with QEMU using the `vexpress-a9` machine model and a Linux 5.15.0 kernel.

## Objectives

The main objectives of this laboratory are:

1. Implement and cross-compile a loadable Linux kernel module for an ARM target.
2. Register a character device and expose it through `/dev/lab2`.
3. Implement the standard `open`, `read`, `write`, and `release` file operations.
4. Transfer data safely between userspace and kernel space.
5. Protect shared driver state during operations that modify common data.
6. Export driver information through procfs.
7. Export device attributes through sysfs.
8. Integrate the kernel module into the Embedded Linux root filesystem.
9. Load the module automatically during system initialization.
10. Validate the driver with repeatable shell-based test procedures.
11. Create and verify a JFFS2 filesystem on simulated NAND flash.

## Development and target environment

The driver and target filesystem were developed for the following environment.

| Item | Configuration |
| --- | --- |
| Target architecture | ARMv7-A |
| Processor | ARM Cortex-A9 |
| Emulated platform | ARM Versatile Express |
| QEMU machine | `vexpress-a9` |
| Linux kernel | 5.15.0 |
| Userspace | BusyBox |
| Cross-compilation toolchain | `arm-linux-gnueabihf-` |
| Character device | `/dev/lab2` |
| Device major number | 240 |
| Device minor number | 0 |
| Driver buffer size | 1024 bytes |
| Root filesystem | Compressed initramfs |

The kernel module is compiled against the same Linux 5.15 source tree used for the target kernel. This is required because a loadable kernel module must be built for the kernel configuration and architecture on which it will be loaded.

## Repository organization

The Lab 02 materials are separated according to their role in driver development, target execution, generated output, and experimental evidence.

| Directory | Contents | Purpose |
| --- | --- | --- |
| `driver/` | Driver source, Makefile, and compiled kernel module | Contains the kernel-space implementation and build configuration |
| `rootfs/initramfs/` | BusyBox userspace, initialization files, kernel module, and test scripts | Contains the complete filesystem used by the target system |
| `output/` | `initramfs_lab2.cpio.gz` | Contains the compressed root filesystem used to boot Lab 02 |
| `logs/` | Boot, driver, procfs/sysfs, and MTD/JFFS2 logs | Preserves the results produced during validation |

The repository therefore keeps source code, runtime files, generated artifacts, and captured results separate while retaining all files required to inspect the completed laboratory work.

## Character device driver

### Driver registration

The driver implementation is stored in `driver/lab2_driver.c`.

The module registers a character device with the following configuration:

| Property | Value |
| --- | --- |
| Device name | `lab2` |
| Class name | `lab2_class` |
| Major number | 240 |
| Minor number | 0 |
| Internal buffer size | 1024 bytes |

The device is registered with the Linux kernel through `register_chrdev`. A device class is then created with `class_create`, and the corresponding device object is created with `device_create`.

The resulting device is represented in userspace as `/dev/lab2`.

### File operations

The driver defines a `struct file_operations` containing implementations for the following operations:

| Operation | Driver function | Purpose |
| --- | --- | --- |
| Open | `lab2_open` | Records a new device open operation |
| Read | `lab2_read` | Transfers data from the kernel buffer to userspace |
| Write | `lab2_write` | Transfers data from userspace into the kernel buffer |
| Release | `lab2_release` | Handles closing of the device |

The driver stores data in an internal 1024-byte buffer named `device_buffer`.

A separate `buffer_len` value records the number of valid bytes currently stored in the buffer.

### Userspace and kernel-space data transfer

The read and write operations use the standard Linux kernel userspace access functions.

`lab2_write` uses `copy_from_user` to copy data supplied by a userspace process into the driver buffer.

`lab2_read` uses `copy_to_user` to return data from the driver buffer to the requesting userspace process.

The read implementation also uses the file offset to determine whether data remains available. When the offset reaches the current data length, the operation returns end-of-file.

Write requests larger than the available driver buffer are limited to 1023 bytes so that the final byte remains available for string termination.

### Shared state protection

The driver defines a Linux mutex for operations that modify shared state.

The mutex protects updates to the device open counter and protects the write-side update of the shared buffer and its associated length value.

This prevents concurrent modifications of those values from occurring simultaneously.

## procfs interface

The driver creates the following procfs entry:

```text
/proc/lab2_info
```

The entry is created with read-only permissions and reports the current driver state.

The implementation uses the Linux `seq_file` interface together with `single_open`, `seq_read`, `seq_lseek`, and `single_release`.

A typical report contains the driver name, major number, buffer capacity, current data length, open count, and the most recently stored data when the buffer is not empty.

The interface can be inspected with:

```sh
cat /proc/lab2_info
```

Immediately after the final Lab 02 system was booted, the interface reported an empty buffer and an open count of zero:

```text
LAB-02 Driver Statistics
======================
Driver name   : lab2
Major number  : 240
Buffer size   : 1024 bytes
Data length   : 0 bytes
Open count    : 0
```

This confirms that the module initialized successfully before any character-device test operation was performed.

## sysfs interface

The driver also exports three read-only attributes through the Linux device model.

| Attribute | Location | Reported information |
| --- | --- | --- |
| `buffer_len` | `/sys/class/lab2_class/lab2/buffer_len` | Number of valid bytes currently stored in the driver buffer |
| `open_count` | `/sys/class/lab2_class/lab2/open_count` | Number of completed device open operations |
| `last_data` | `/sys/class/lab2_class/lab2/last_data` | Most recent data stored in the driver buffer |

The attributes are implemented with `DEVICE_ATTR_RO` and are registered as a sysfs attribute group associated with the `lab2` device object.

They can be inspected directly with:

```sh
cat /sys/class/lab2_class/lab2/buffer_len
cat /sys/class/lab2_class/lab2/open_count
cat /sys/class/lab2_class/lab2/last_data
```

During the final procfs and sysfs validation, the attributes reported:

```text
buffer_len : 30
open_count : 9
last_data  : Embedded Linux Lab2 Test Data
```

These values correspond to the data and access operations performed by the supplied test scripts.

## System integration

### Kernel module installation

The compiled module used by the target filesystem is stored at:

```text
rootfs/initramfs/lib/modules/5.15.0/lab2_driver.ko
```

The final module was rebuilt from the current `lab2_driver.c` source for the ARM target and then copied into the Lab 02 root filesystem.

The module stored in `driver/lab2_driver.ko` and the module installed in the root filesystem were verified to be identical after the final build.

The `file` utility identified the generated module as a 32-bit ARM EABI5 relocatable ELF object, confirming that it was compiled for the target architecture rather than the Ubuntu host.

### BusyBox initialization

The target uses BusyBox `/sbin/init` and the configuration stored in `rootfs/initramfs/etc/inittab`.

The `sysinit` entry executes:

```text
/etc/init.d/rcS
```

The Lab 02 `rcS` script performs the system initialization required by the driver environment. It mounts the procfs, sysfs, tmpfs, and devtmpfs filesystems, ensures that essential device nodes are available, configures the hostname as `embedded-lab2`, and loads the Lab 02 kernel module.

The module is loaded from:

```text
/lib/modules/5.15.0/lab2_driver.ko
```

If module loading succeeds, the script ensures that `/dev/lab2` exists with major number 240 and minor number 0 and sets the device permissions to allow read and write access.

During the verified boot, the initialization process reported:

```text
[ LAB-02 ] Loading lab2_driver module...
lab2_driver: initializing module
lab2_driver: loaded, major=240
lab2_driver: /proc/lab2_info created
[ LAB-02 ] lab2_driver loaded OK
[ LAB-02 ] /dev/lab2 created (major=240, minor=0)
[ LAB-02 ] System initialization complete!
```

The kernel also reported that the driver is an out-of-tree module. This is expected for a separately compiled laboratory module and does not indicate a module-loading failure.

## Building the driver

The driver Makefile uses the Linux kernel build system and the ARM cross-compilation toolchain.

The current build configuration specifies:

```make
KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-
obj-m += lab2_driver.o
```

From the `driver` directory, the module can be rebuilt with:

```bash
make clean
make
```

The final build completed successfully and generated `lab2_driver.ko`.

The resulting module was verified with:

```bash
file lab2_driver.ko
```

The final build produced an ARM module identified as:

```text
ELF 32-bit LSB relocatable, ARM, EABI5
```

This confirms that the module is suitable for the ARMv7 target system.

## Running the system with QEMU

The final Lab 02 system uses the Linux kernel and Device Tree generated in Lab 01 together with the Lab 02 initramfs image.

From the Lab 02 directory, the system can be started with:

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -cpu cortex-a9 \
  -m 512M \
  -smp 2 \
  -nographic \
  -kernel ~/embedded_lab1/output/zImage \
  -dtb ~/embedded_lab1/output/vexpress-v2p-ca9.dtb \
  -initrd output/initramfs_lab2.cpio.gz \
  -append 'console=ttyAMA0,115200 rdinit=/sbin/init mem=512M'
```

The final initramfs image was rebuilt after recompiling the current version of the driver. The archive was verified to contain the driver module, the startup script, and both Lab 02 test scripts before the final QEMU boot was performed.

## Character device verification

The supplied script `test_driver.sh` performs the primary functional test of the character device.

It verifies that the module is loaded, confirms the `/dev/lab2` device node, writes data to the device, reads the same data back, performs repeated write and read operations, and inspects the corresponding kernel messages.

The script is executed inside the target system with:

```sh
test_driver.sh
```

During final validation, the first data transfer wrote:

```text
Hello from userspace, LAB-02!
```

and returned the same value through the character device.

The script then successfully transferred the values `Message_1`, `Message_2`, and `Message_3`.

The final result was:

```text
=== Test PASSED ===
```

The test output also confirmed that the device node had the expected device numbers:

```text
crw-rw-rw-    1 root     0         240,   0 /dev/lab2
```

## procfs and sysfs verification

The script `test_procfs.sh` validates the driver information interfaces.

The script first reads `/proc/lab2_info`, writes a known data value to `/dev/lab2`, reads the procfs information again, and then retrieves all three sysfs attributes.

It is executed with:

```sh
test_procfs.sh
```

The final test wrote:

```text
Embedded Linux Lab2 Test Data
```

After the write operation, `/proc/lab2_info` reported a data length of 30 bytes and an open count of 9.

The sysfs interface reported the same 30-byte data length and the same current data value.

The test completed with:

```text
=== procfs/sysfs Test PASSED ===
```

The agreement between the procfs state, sysfs attributes, and character-device operation demonstrates that the exported interfaces represent the same internal driver state.

## Kernel log verification

The driver uses the Linux kernel logging interface to record initialization and character-device operations.

Kernel messages can be inspected with:

```sh
dmesg | grep lab2
```

During the final validation, the kernel log recorded module initialization, device open and close operations, data received from userspace, and data returned to userspace.

These messages provide an additional method for confirming the sequence of operations performed during the driver tests.

## MTD and JFFS2 experiment

### Experimental environment

The MTD and JFFS2 portion of Lab 02 was performed on the Ubuntu development host rather than inside the normal QEMU character-driver environment.

A NAND device was simulated with the Linux `nandsim` mechanism and was then used as the storage device for a JFFS2 filesystem.

Before any erase or write operation, `/proc/mtd` was inspected to identify the simulated device.

During the recorded experiment, the host reported:

```text
mtd0: 02000000 00001000 "BIOS"
mtd1: 02000000 00004000 "NAND simulator partition 0"
```

For that specific execution, `/dev/mtd1` was therefore identified as the simulated NAND device.

The simulator reported a capacity of 32 MiB and an eraseblock size of 16 KiB.

### JFFS2 image creation

A source directory was prepared with sample files and a log subdirectory.

The JFFS2 image was then created with an eraseblock size matching the NAND simulator:

```bash
sudo mkfs.jffs2 \
  --root=/tmp/jffs2_data \
  --eraseblock=0x4000 \
  --output=/tmp/rootfs.jffs2 \
  --pad \
  --no-cleanmarkers
```

The generated image was written to the simulated NAND device with `nandwrite`.

The corresponding block device was then mounted as a JFFS2 filesystem.

### Runtime write and persistence verification

After mounting the filesystem, the original files were read successfully.

Additional runtime data was then written to the mounted JFFS2 filesystem.

The filesystem was synchronized, unmounted, and mounted again. After the remount, both the appended log data and the newly created runtime file were still present.

This result verified that the filesystem retained the written data across an unmount and remount cycle while the simulated NAND device remained active.

### Raw NAND inspection

The experiment also used `nanddump` to read the raw NAND contents.

The recorded result reported:

```text
ECC failed: 0
ECC corrected: 0
Number of bad blocks: 0
Number of bbt blocks: 0
```

The hexadecimal dump contained the JFFS2 data written during the experiment, providing an additional low-level confirmation that filesystem data had been programmed into the simulated NAND storage.

### Safety considerations

MTD device numbers must never be assumed to have a fixed meaning.

The recorded Ubuntu host contained a real device named `BIOS` at `/dev/mtd0`, while the NAND simulator appeared as `/dev/mtd1`.

For this reason, `/proc/mtd` and the device information must be inspected before executing destructive operations such as `flash_erase` or `nandwrite`.

An erase or write command should only be executed after the target device has been explicitly identified as the intended NAND simulator.

The device number used during this laboratory must therefore not be treated as a universal value for another computer or another execution.

## Test results and logs

The `logs` directory preserves the experimental evidence generated during Lab 02.

| Log file | Recorded evidence |
| --- | --- |
| `boot_log.txt` | Linux and BusyBox startup together with automatic Lab 02 module initialization |
| `driver_test.txt` | Character-device loading, device-node verification, write/read operations, and final driver test result |
| `procfs_test.txt` | procfs state, sysfs attributes, and validation of the exported driver information |
| `mtd_jffs2.txt` | MTD identification, NAND erase, JFFS2 image creation, NAND programming, mount operations, persistence test, and raw NAND dump |

These logs provide reproducible evidence for the principal functional tests described in this README.

## Configuration and build artifacts

The principal Lab 02 files are summarized below.

| Path | Purpose |
| --- | --- |
| `driver/lab2_driver.c` | Linux character-device driver source code |
| `driver/Makefile` | ARM kernel-module build configuration |
| `driver/lab2_driver.ko` | Compiled ARM kernel module |
| `rootfs/initramfs/lib/modules/5.15.0/lab2_driver.ko` | Kernel module installed in the target root filesystem |
| `rootfs/initramfs/etc/init.d/rcS` | BusyBox startup script responsible for module initialization |
| `rootfs/initramfs/usr/bin/test_driver.sh` | Character-device validation script |
| `rootfs/initramfs/usr/bin/test_procfs.sh` | procfs and sysfs validation script |
| `output/initramfs_lab2.cpio.gz` | Final compressed Lab 02 root filesystem |
| `logs/` | Captured boot and test evidence |

The driver module installed in the root filesystem was synchronized with the module generated from the corrected final Makefile before the final initramfs image was created and tested.

## Reproducibility and scope

Lab 02 depends on the Linux 5.15.0 kernel environment established during Lab 01.

The driver Makefile expects the kernel source tree at:

```text
~/embedded_lab1/kernel/linux-5.15
```

The QEMU execution procedure also uses the Lab 01 kernel image and Device Tree Blob.

A rebuilt kernel module must be compiled against a compatible target kernel before it is installed in the Lab 02 root filesystem.

The final `initramfs_lab2.cpio.gz` image in this repository was rebuilt after the current Lab 02 module was compiled, copied into the root filesystem, and verified as an ARM EABI5 kernel module.

The character-device, procfs, and sysfs tests were then executed successfully against that rebuilt image.

The MTD/JFFS2 experiment is independent of the QEMU character-device test and was performed on the Ubuntu host using a RAM-backed NAND simulator.
