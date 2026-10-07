#!/bin/bash

set -e

echo "Downloading Linux Kernel 5.15..."
mkdir -p kernel
cd kernel
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.tar.xz
tar -xf linux-5.15.tar.xz
cd ..

echo "Downloading U-Boot 2022.04..."
mkdir -p uboot
cd uboot
wget https://ftp.denx.de/pub/u-boot/u-boot-2022.04.tar.bz2
tar -xf u-boot-2022.04.tar.bz2
cd ..

echo "Downloading BusyBox 1.35.0..."
mkdir -p rootfs
cd rootfs
wget https://busybox.net/downloads/busybox-1.35.0.tar.bz2
tar -xf busybox-1.35.0.tar.bz2
cd ..

echo "Source download completed."
