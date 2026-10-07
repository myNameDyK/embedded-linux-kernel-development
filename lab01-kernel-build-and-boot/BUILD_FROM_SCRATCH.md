# Build Lab 01 From Scratch

This guide rebuilds the complete Lab 01 system from the original Linux Kernel, U-Boot, and BusyBox sources.

## Target

| Item | Value |
|---|---|
| Architecture | ARMv7-A |
| CPU | Cortex-A9 |
| Platform | ARM Versatile Express |
| QEMU machine | `vexpress-a9` |
| Linux Kernel | 5.15 |
| U-Boot | 2022.04 |
| BusyBox | 1.35.0 |
| Cross compiler | `arm-linux-gnueabihf-` |

The final system contains a Linux Kernel, Device Tree, U-Boot, and a BusyBox-based initramfs.

---

# 1. Install Build Tools

```bash
sudo apt update && sudo apt upgrade -y

sudo apt install -y build-essential git wget curl bison flex
sudo apt install -y libncurses-dev libssl-dev libelf-dev
sudo apt install -y gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
sudo apt install -y qemu-system-arm qemu-utils
sudo apt install -y cpio
```

These packages install the compiler, Kernel build tools, QEMU, and archive utilities on the Ubuntu host.

Check the two main tools:

```bash
arm-linux-gnueabihf-gcc --version
qemu-system-arm --version
```

> **Important:** These tools are installed into the host system. They are not stored inside the Lab folders.

---

# 2. Create the Workspace

```bash
mkdir -p ~/embedded_lab1/{kernel,uboot,rootfs,output}
cd ~/embedded_lab1
```

This creates the four main directories used by the Lab.

```text
embedded_lab1/
├── kernel/
├── uboot/
├── rootfs/
└── output/
```

`kernel`, `uboot`, and `rootfs` contain source and build files.  
`output` collects the final boot files.

---

# 3. Download the Sources

```bash
cd ~/embedded_lab1/kernel
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.tar.xz
tar -xf linux-5.15.tar.xz

cd ~/embedded_lab1/uboot
wget https://ftp.denx.de/pub/u-boot/u-boot-2022.04.tar.bz2
tar -xf u-boot-2022.04.tar.bz2

cd ~/embedded_lab1/rootfs
wget https://busybox.net/downloads/busybox-1.35.0.tar.bz2
tar -xf busybox-1.35.0.tar.bz2
```

`wget` downloads each source archive.  
`tar -xf` extracts it into a source directory.

After this step:

```text
kernel/linux-5.15/
uboot/u-boot-2022.04/
rootfs/busybox-1.35.0/
```

---

# 4. Build the Linux Kernel

## 4.1 Set the Cross-Compile Environment

```bash
cd ~/embedded_lab1/kernel/linux-5.15

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
unset KCFLAGS
```

`ARCH=arm` selects the ARM Kernel architecture.  
`CROSS_COMPILE` selects the ARM toolchain.

`unset KCFLAGS` removes any old global Kernel compiler flags.

Check:

```bash
echo $ARCH
echo $CROSS_COMPILE
```

Expected:

```text
arm
arm-linux-gnueabihf-
```

---

## 4.2 Load the VExpress Default Configuration

```bash
make vexpress_defconfig
```

This creates:

```text
linux-5.15/.config
```

The configuration is based on the ARM Versatile Express platform.

Check:

```bash
grep -E 'CONFIG_ARCH_VEXPRESS|CONFIG_CPU_V7' .config
```

Expected:

```text
CONFIG_ARCH_VEXPRESS=y
CONFIG_CPU_V7=y
```

> **Important:** `vexpress_defconfig` gives a working base configuration. `menuconfig` is used later only to check or modify specific options.

---

## 4.3 GCC 15 Workaround

Linux 5.15 may fail to build ARMv7 code with newer GCC versions.

Back up the ARM Makefile:

```bash
cp arch/arm/Makefile arch/arm/Makefile.bak
```

Apply the ARMv7-A flag:

```bash
sed -i 's@arch-\$(CONFIG_CPU_32v7)[[:space:]]*=.*@arch-$(CONFIG_CPU_32v7) =-D__LINUX_ARM_ARCH__=7 -march=armv7-a@' arch/arm/Makefile
```

Check:

```bash
grep -n 'CONFIG_CPU_32v7)' arch/arm/Makefile
```

Expected line:

```text
arch-$(CONFIG_CPU_32v7) =-D__LINUX_ARM_ARCH__=7 -march=armv7-a
```

> **Important:** This workaround is only needed if the older Linux 5.15 source has compatibility problems with a newer GCC toolchain.

---

## 4.4 Check the Kernel Configuration

```bash
make menuconfig
```

Check these options:

| Menu | Required setting |
|---|---|
| Platform selection | ARM Versatile Express platform support |
| General setup | Initial RAM filesystem and RAM disk support |
| Device Drivers → Character devices → Serial drivers | ARM AMBA PL011 serial port support |
| Kernel Features | SMP, if multiple CPU cores are required |

PL011 is required for the `ttyAMA0` QEMU console.

Save the configuration before exiting.

---

## 4.5 Build the Kernel and Device Tree

```bash
make -j$(nproc) zImage dtbs modules
make modules_prepare
```

This builds:

| Target | Result |
|---|---|
| `zImage` | Compressed ARM Linux Kernel |
| `dtbs` | Device Tree binaries |
| `modules` | Kernel features configured as modules |
| `modules_prepare` | Prepares this Kernel tree for future external driver builds |

Check the two important outputs:

```bash
ls -lh arch/arm/boot/zImage
ls -lh arch/arm/boot/dts/vexpress-v2p-ca9.dtb
```

Copy them to the final output directory:

```bash
cp arch/arm/boot/zImage ~/embedded_lab1/output/
cp arch/arm/boot/dts/vexpress-v2p-ca9.dtb ~/embedded_lab1/output/
```

---

# 5. Build U-Boot

```bash
cd ~/embedded_lab1/uboot/u-boot-2022.04

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

make vexpress_ca9x4_defconfig
make -j$(nproc)
```

`vexpress_ca9x4_defconfig` selects the U-Boot configuration for the VExpress Cortex-A9 platform.

Check the result:

```bash
file u-boot
```

Copy the final U-Boot ELF:

```bash
cp u-boot ~/embedded_lab1/output/
```

Optional U-Boot test:

```bash
qemu-system-arm \
    -M vexpress-a9 \
    -cpu cortex-a9 \
    -m 512M \
    -nographic \
    -kernel ~/embedded_lab1/output/u-boot
```

A successful boot should reach the U-Boot prompt:

```text
=>
```

---

# 6. Build BusyBox

```bash
cd ~/embedded_lab1/rootfs/busybox-1.35.0

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

make defconfig
make menuconfig
```

Configure these important options:

| Option | Value |
|---|---|
| Build static binary | Enabled |
| Cross compiler prefix | `arm-linux-gnueabihf-` |
| Additional CFLAGS | `-march=armv7-a -mfpu=neon-vfpv3 -mfloat-abi=hard` |
| Networking Utilities → `tc` | Disabled |

Static linking allows BusyBox to run without shared libraries inside the small root filesystem.

Build and install:

```bash
make -j$(nproc)
make install
```

The result is placed in:

```text
busybox-1.35.0/_install/
```

Check:

```bash
file _install/bin/busybox
```

It should be an ARM executable and statically linked.

> **Note:** If `make menuconfig` fails because of the old BusyBox ncurses test on newer GCC, use the compatibility fix documented in the original Lab before reopening `menuconfig`.

---

# 7. Create the Root Filesystem

Create the minimal directory structure:

```bash
mkdir -p ~/embedded_lab1/rootfs/initramfs
cd ~/embedded_lab1/rootfs/initramfs

mkdir -p bin sbin usr/bin usr/sbin
mkdir -p dev proc sys tmp etc
mkdir -p lib/arm-linux-gnueabihf
```

Copy the BusyBox installation:

```bash
cp -r ~/embedded_lab1/rootfs/busybox-1.35.0/_install/bin/* bin/
cp -r ~/embedded_lab1/rootfs/busybox-1.35.0/_install/sbin/* sbin/
cp -r ~/embedded_lab1/rootfs/busybox-1.35.0/_install/usr/* usr/
```

Because BusyBox is static, shared libraries are not required.

---

# 8. Create the Startup Files

Create `/init`:

```bash
cat > init << 'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
exec /sbin/init
EOF

chmod +x init
```

Create `/etc/inittab`:

```bash
cat > etc/inittab << 'EOF'
::sysinit:/etc/init.d/rcS
::askfirst:-/bin/sh
::ctrlaltdel:/sbin/reboot
::shutdown:/sbin/swapoff -a
::shutdown:/bin/umount -a -r
::restart:/sbin/init
EOF
```

Create `/etc/init.d/rcS`:

```bash
mkdir -p etc/init.d

cat > etc/init.d/rcS << 'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t tmpfs none /tmp
echo 'Boot completed! Welcome to Embedded Linux Lab 1'
hostname embedded-lab
EOF

chmod +x etc/init.d/rcS
```

Create the minimal account and hostname files:

```bash
echo 'root:x:0:0:root:/root:/bin/sh' > etc/passwd
echo 'embedded-lab' > etc/hostname
```

---

# 9. Create Device Nodes

```bash
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
sudo mknod -m 666 dev/zero c 1 5
sudo mknod -m 444 dev/random c 1 8
```

These create the minimum character devices required by the small userspace.

> **Important:** `/dev/console` is especially important during system startup.

---

# 10. Package the initramfs

```bash
cd ~/embedded_lab1/rootfs/initramfs

find . | cpio -H newc -o | gzip \
    > ~/embedded_lab1/output/initramfs.cpio.gz
```

This packs the root filesystem into a compressed CPIO archive.

The final output directory should now contain:

```text
output/
├── zImage
├── vexpress-v2p-ca9.dtb
├── u-boot
└── initramfs.cpio.gz
```

---

# 11. Boot the Complete System

```bash
qemu-system-arm \
    -M vexpress-a9 \
    -cpu cortex-a9 \
    -m 512M \
    -smp 2 \
    -nographic \
    -kernel ~/embedded_lab1/output/zImage \
    -dtb ~/embedded_lab1/output/vexpress-v2p-ca9.dtb \
    -initrd ~/embedded_lab1/output/initramfs.cpio.gz \
    -append 'console=ttyAMA0,115200 rdinit=/sbin/init mem=512M'
```

The important QEMU options are:

| Option | Purpose |
|---|---|
| `-M vexpress-a9` | Emulate the VExpress-A9 machine |
| `-cpu cortex-a9` | Use a Cortex-A9 CPU |
| `-m 512M` | Provide 512 MB RAM |
| `-smp 2` | Emulate two CPU cores |
| `-kernel` | Load the Linux Kernel |
| `-dtb` | Load the Device Tree |
| `-initrd` | Load the initramfs |
| `console=ttyAMA0,115200` | Send the Linux console to the PL011 UART |
| `rdinit=/sbin/init` | Start BusyBox init from the initramfs |

A successful boot should eventually show:

```text
Boot completed! Welcome to Embedded Linux Lab 1
/ #
```

---

# Important Notes

- Run Kernel `make` commands inside `~/embedded_lab1/kernel/linux-5.15`.
- `vexpress_defconfig` creates the initial `.config`.
- `menuconfig` edits that `.config`.
- `zImage` and `.dtb` are built inside the Kernel source tree and then copied to `output/`.
- `make modules_prepare` is useful later when building external drivers.
- BusyBox is built statically so the initramfs can remain small.
- In this Lab, the initramfs is used directly as the root filesystem.
- QEMU can boot the Kernel directly. U-Boot is built and tested separately.
