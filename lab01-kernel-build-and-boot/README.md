# Lab 01 - Linux Kernel Build and Boot

This lab builds a minimal Embedded Linux system for an ARMv7-A Cortex-A9 target and runs it on QEMU using the `vexpress-a9` platform.

The system is built from Linux Kernel 5.15, U-Boot 2022.04, and BusyBox 1.35.0 using the `arm-linux-gnueabihf` cross-compilation toolchain.

The purpose of this lab is to understand the basic Embedded Linux boot process and learn how to configure, cross-compile, and combine the Kernel, Device Tree, bootloader, and root filesystem into a bootable system.

## Target Platform

| Component | Configuration |
|---|---|
| Architecture | ARMv7-A |
| CPU | ARM Cortex-A9 |
| Platform | ARM Versatile Express |
| QEMU Machine | `vexpress-a9` |
| Memory | 512 MB |
| Linux Kernel | 5.15 |
| U-Boot | 2022.04 |
| BusyBox | 1.35.0 |
| Cross Compiler | `arm-linux-gnueabihf-` |

## Repository Structure

```text
lab01-kernel-build-and-boot/
├── README.md
├── BUILD_FROM_SCRATCH.md
├── kernel/
│   └── kernel.config
├── uboot/
│   └── uboot.config
├── rootfs/
│   ├── busybox.config
│   └── initramfs/
│       ├── init
│       └── etc/
│           ├── hostname
│           ├── passwd
│           ├── inittab
│           └── init.d/
│               └── rcS
├── output/
│   ├── zImage
│   ├── vexpress-v2p-ca9.dtb
│   ├── u-boot
│   └── initramfs.cpio.gz
└── scripts/
    └── fetch_sources.sh
```

The original Linux Kernel, U-Boot, and BusyBox source trees are not included in this repository because they can be downloaded directly from their official upstream sources.

---

# 1. Ready-to-Use Output

The `output/` directory contains the final products generated during this lab.

| File | Description |
|---|---|
| `zImage` | Compressed Linux Kernel image built for ARMv7-A |
| `vexpress-v2p-ca9.dtb` | Device Tree Blob for the VExpress Cortex-A9 platform |
| `u-boot` | U-Boot bootloader built for the VExpress Cortex-A9 target |
| `initramfs.cpio.gz` | Compressed minimal root filesystem containing BusyBox and startup files |

These files can be used directly without rebuilding the complete project.

## Direct QEMU Boot

From the `lab01-kernel-build-and-boot` directory:

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

This method boots the Linux Kernel directly with QEMU.

U-Boot is not required for this direct boot process. It is included as a separate build output for studying the role of a bootloader in an Embedded Linux system.

---

# 2. Configuration and Study Files

The repository also contains the configuration and startup files used to produce the final system.

| File | Purpose |
|---|---|
| `kernel/kernel.config` | Linux Kernel configuration used for the final Kernel build |
| `uboot/uboot.config` | U-Boot configuration for the VExpress Cortex-A9 target |
| `rootfs/busybox.config` | BusyBox configuration used for the minimal userspace |
| `rootfs/initramfs/init` | Initial startup script in the root filesystem |
| `rootfs/initramfs/etc/inittab` | BusyBox init configuration |
| `rootfs/initramfs/etc/init.d/rcS` | System initialization script |
| `rootfs/initramfs/etc/hostname` | Hostname configuration |
| `rootfs/initramfs/etc/passwd` | Minimal root user configuration |

These files are useful for studying how each part of the system was configured and how the Embedded Linux userspace starts after the Kernel boots.

The generated outputs can be used directly, while the configuration files can be inspected, modified, or reused for further experiments.

---

# Rebuilding the Project

A separate guide is provided for anyone who wants to reproduce the complete project from the original source code.

See:

[`BUILD_FROM_SCRATCH.md`](BUILD_FROM_SCRATCH.md)

The build guide covers environment setup, source download, Kernel configuration, U-Boot, BusyBox, initramfs creation, and QEMU boot.
