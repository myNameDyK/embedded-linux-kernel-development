# Lab 01: Linux kernel build and boot

## Introduction

This laboratory develops a minimal Embedded Linux system for an ARM Cortex-A9 target and validates the completed system by running it on QEMU with the ARM Versatile Express `vexpress-a9` machine model.

The system consists of a Linux 5.15.0 kernel image, a Device Tree Blob for the emulated hardware platform, and a BusyBox 1.35.0 userspace packaged as an initial RAM filesystem. The repository also preserves the Linux kernel and BusyBox configuration files together with the complete root filesystem used to generate the final boot image.

The laboratory establishes the fundamental workflow required to configure, cross-compile, assemble, and boot an Embedded Linux system for a target architecture different from the development host.

## Objectives

The main objectives of this laboratory are:

1. Configure a Linux kernel for an ARMv7-A target.
2. Cross-compile the kernel using an ARM GNU/Linux toolchain.
3. Use a Device Tree Blob to describe the target hardware platform.
4. Configure and build a minimal userspace with BusyBox.
5. Construct an initial RAM filesystem from the generated root filesystem.
6. Boot the complete system with QEMU using the `vexpress-a9` machine model.
7. Verify the kernel, initialization process, virtual filesystems, and interactive userspace after boot.

## Development and target environment

The completed system uses the following target configuration.

| Item | Configuration |
| --- | --- |
| Target architecture | ARMv7-A |
| Processor | ARM Cortex-A9 |
| Emulated platform | ARM Versatile Express |
| QEMU machine | `vexpress-a9` |
| Linux kernel | 5.15.0 |
| Userspace | BusyBox 1.35.0 |
| Cross-compilation toolchain | `arm-linux-gnueabihf-` |
| System memory | 512 MB |
| Virtual processors | 2 |
| Root filesystem format | Compressed initramfs |

The Linux kernel configuration identifies the target as Linux/ARM 5.15.0. The BusyBox configuration identifies BusyBox version 1.35.0. Both configurations are preserved in the `configs` directory.

## Repository organization

The Lab 01 materials are organized into three functional groups: build configurations, generated boot artifacts, and the root filesystem used to construct the initramfs image.

| Directory | Contents | Purpose |
| --- | --- | --- |
| `configs/` | `kernel.config`, `busybox.config` | Preserves the Linux kernel and BusyBox configurations used for the target build |
| `output/` | `zImage`, `vexpress-v2p-ca9.dtb`, `initramfs.cpio.gz` | Contains the final artifacts required to boot the system with QEMU |
| `rootfs/initramfs/` | BusyBox userspace, initialization files, and filesystem hierarchy | Contains the complete root filesystem before it is packaged into the compressed initramfs image |

This organization separates configuration data from generated artifacts and runtime filesystem content, making each part of the system easier to inspect and maintain.

## System architecture

### Linux kernel

The file `output/zImage` is the compressed Linux kernel image built for the ARMv7-A target.

The configuration used for the kernel build is preserved in `configs/kernel.config`. Keeping this file makes the selected kernel configuration available for inspection and future reproduction.

The kernel is loaded directly by QEMU in the current Lab 01 execution procedure.

### Device Tree

The file `output/vexpress-v2p-ca9.dtb` is the Device Tree Blob used for the ARM Versatile Express Cortex-A9 platform.

The Device Tree provides the Linux kernel with a description of the emulated hardware platform, including the devices and resources that are available during system initialization.

The Device Tree Blob is supplied to QEMU together with the kernel image during boot.

### BusyBox userspace

The userspace environment is based on BusyBox 1.35.0.

The BusyBox configuration is stored in `configs/busybox.config`, while the generated filesystem is preserved under `rootfs/initramfs`.

BusyBox provides the shell and the standard command-line utilities required by the minimal Embedded Linux environment. The root filesystem also contains the initialization files required to prepare the system after the kernel starts the first userspace process.

### Initial RAM filesystem

The final initial RAM filesystem is stored in `output/initramfs.cpio.gz`.

This archive contains the userspace filesystem loaded into memory during the boot process. Its uncompressed source is preserved in `rootfs/initramfs`.

Maintaining both the filesystem tree and the compressed boot image makes it possible to inspect the userspace configuration independently from the generated artifact used by QEMU.

## System initialization

The current QEMU command uses the following kernel parameter:

```text
rdinit=/sbin/init
```

As a result, the kernel starts BusyBox `/sbin/init` directly as the first userspace process.

BusyBox `init` then reads the configuration file located at `rootfs/initramfs/etc/inittab`.

The current `inittab` configuration performs the following actions:

```text
::sysinit:/etc/init.d/rcS
::askfirst:-/bin/sh
::ctrlaltdel:/sbin/reboot
::shutdown:/sbin/swapoff -a
::shutdown:/bin/umount -a -r
::restart:/sbin/init
```

During system initialization, BusyBox executes `/etc/init.d/rcS`.

The current `rcS` script mounts the procfs, sysfs, and tmpfs virtual filesystems, prints the boot completion message, and configures the system hostname as `embedded-lab`.

The message displayed during initialization is:

```text
Boot completed! Welcome to Embedded Linux Lab 1
```

The root filesystem also contains a top-level `/init` script. That script mounts procfs and sysfs before executing `/sbin/init`. However, the current QEMU boot command specifies `rdinit=/sbin/init`, so `/sbin/init` is selected directly and the top-level `/init` script is not the entry point used by this boot configuration.

This distinction is important when interpreting the userspace initialization sequence.

## Running the system with QEMU

From the `SE200494_NguyenDuyKhoa_Lab01` directory, the completed system can be started with the following command.

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -cpu cortex-a9 \
  -m 512M \
  -smp 2 \
  -nographic \
  -kernel output/zImage \
  -dtb output/vexpress-v2p-ca9.dtb \
  -initrd output/initramfs.cpio.gz \
  -append 'console=ttyAMA0,115200 rdinit=/sbin/init mem=512M'
```

The command creates an emulated ARM Versatile Express system using a Cortex-A9 processor model.

The `-kernel` option supplies the Linux kernel image, the `-dtb` option supplies the Device Tree Blob, and the `-initrd` option supplies the compressed initial RAM filesystem.

The `-nographic` option redirects the emulated serial console to the terminal. The kernel command line configures `ttyAMA0` as the system console and specifies `/sbin/init` as the initial userspace process.

## Boot sequence

With the current configuration, the startup process occurs in the following order.

1. QEMU creates the ARM Versatile Express virtual hardware platform.
2. QEMU loads the Linux kernel image from `output/zImage`.
3. QEMU provides the Device Tree Blob from `output/vexpress-v2p-ca9.dtb`.
4. QEMU provides the compressed initial RAM filesystem from `output/initramfs.cpio.gz`.
5. The Linux kernel initializes the ARM platform and the required kernel subsystems.
6. The kernel starts `/sbin/init` according to the `rdinit=/sbin/init` kernel parameter.
7. BusyBox `init` reads `/etc/inittab`.
8. The `sysinit` entry in `inittab` executes `/etc/init.d/rcS`.
9. The `rcS` script prepares the virtual filesystems and configures the hostname.
10. BusyBox provides an interactive shell on the serial console.

The successful completion of this sequence produces a minimal but operational Embedded Linux environment.

## System verification

After the BusyBox shell becomes available, the running system can be inspected with the following commands.

```sh
uname -a
cat /proc/cpuinfo
hostname
mount
ps
```

The `uname -a` command confirms the running kernel and target architecture. The `/proc/cpuinfo` interface provides information about the emulated processor. The `hostname` command confirms the hostname configured during initialization.

The `mount` command can be used to verify the active filesystems, including the virtual filesystems initialized by the startup script. The `ps` command confirms that the expected userspace processes are running.

The root filesystem can also be inspected with:

```sh
ls /
ls /bin
ls /sbin
ls /proc
ls /sys
```

A successful Lab 01 boot is characterized by the following conditions:

1. The Linux kernel completes initialization without a fatal error.
2. The message `Boot completed! Welcome to Embedded Linux Lab 1` is displayed.
3. An interactive BusyBox shell becomes available.
4. The procfs and sysfs virtual filesystems can be accessed.
5. Standard BusyBox commands execute correctly in the target environment.

## Configuration and build artifacts

The principal files retained for Lab 01 are listed below.

| Path | Purpose |
| --- | --- |
| `configs/kernel.config` | Linux 5.15.0 kernel configuration used for the target build |
| `configs/busybox.config` | BusyBox 1.35.0 configuration used for the userspace build |
| `output/zImage` | Compressed Linux kernel image for ARMv7-A |
| `output/vexpress-v2p-ca9.dtb` | Device Tree Blob for the Versatile Express Cortex-A9 platform |
| `output/initramfs.cpio.gz` | Compressed initial RAM filesystem used by QEMU |
| `rootfs/initramfs/` | Complete userspace filesystem before compression |

These files document both the configuration state and the resulting bootable system.

## Reproducibility and scope

The original Linux kernel and BusyBox source trees are not included in the Lab 01 submission directory.

Instead, the submission preserves the kernel configuration, BusyBox configuration, complete generated root filesystem, and final boot artifacts. These files provide the configuration and runtime state associated with the validated Lab 01 system.

Reproducing the build requires the corresponding Linux kernel and BusyBox source versions together with an ARM cross-compilation environment. The saved configuration files can then be applied to the appropriate source trees before rebuilding the kernel and BusyBox userspace.

The files in the `output` directory are the generated artifacts used for the completed Lab 01 system.

## Author

Nguyen Duy Khoa

Student ID: SE200494
