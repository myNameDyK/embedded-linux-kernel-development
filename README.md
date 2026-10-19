# Embedded Linux Systems: ARM/QEMU Labs and Assignments

This repository collects a sequence of Embedded Linux laboratories and assignments built around an ARMv7-A target running under QEMU. The work progresses from Linux system bring-up and root-filesystem construction to kernel-driver development, userspace/kernel interfaces, remote debugging, performance analysis, real-time latency measurement, and a multithreaded sensor-monitoring application.

The repository is organized as a course workspace rather than a single application. Each Lab or Assignment is self-contained and has its own `README.md` with detailed build, run, test, and analysis procedures. This top-level README provides the technical overview and navigation for the complete repository.

## Technical Scope

| Area | Topics covered |
| --- | --- |
| System bring-up | ARM cross-compilation, Linux kernel, Device Tree, BusyBox, initramfs, QEMU boot |
| Kernel development | Loadable kernel modules, character devices, `file_operations`, miscdevice |
| Kernel interfaces | `/dev`, procfs, sysfs, ioctl, userspace/kernel data transfer |
| Storage | MTD concepts and JFFS2 experiments |
| Debugging | GDB, gdbserver, remote ARM debugging, backtraces, register and memory inspection |
| Memory analysis | Valgrind, leak detection, `/proc/<PID>/status`, process memory maps |
| Performance analysis | strace, perf, system-call profiling, CPU hotspot analysis |
| Real-time analysis | cyclictest, stress-ng, scheduling latency and jitter |
| Userspace systems | POSIX threads, circular buffers, POSIX message queues, logging and signal handling |

## Target and Development Environment

| Component | Configuration |
| --- | --- |
| Target architecture | ARMv7-A |
| Processor model | ARM Cortex-A9 |
| Emulated platform | QEMU `vexpress-a9` |
| Linux kernel | Linux 5.15 |
| Userspace | BusyBox / initramfs |
| Cross toolchain | GNU `arm-linux-gnueabihf-` |
| Host architecture | x86_64 Linux |
| Host distributions | Ubuntu or AlmaLinux, depending on the exercise |
| Main languages | C, shell |
| Debug/profiling tools | GDB, gdbserver, Valgrind, strace, perf |
| Real-time tools | cyclictest, stress-ng |

Exact host versions, tool versions, kernel configuration, command lines, and exercise-specific assumptions are documented in the corresponding module README.

## Host–Target Model

```text
x86_64 Linux host
|
|-- native GCC
|-- ARM cross-compiler
|-- GDB / gdbserver client
|-- Valgrind
|-- strace / perf
|-- cyclictest / stress-ng
|
+--> ARM kernel, modules, applications and initramfs
        |
        v
QEMU vexpress-a9
|
|-- ARM Cortex-A9
|-- Linux 5.15
|-- BusyBox userspace
|-- kernel modules
|-- ARM test applications
|
+--> serial console, /proc, /sys, /dev and debug interfaces
```

Some tools run on the x86_64 development host, while target binaries and kernel modules run inside the ARM system. This distinction is kept explicit throughout the repository because it affects build commands, runtime libraries, profiling results, and debugging procedures.

## Repository Map

| Module | Focus | Detailed documentation |
| --- | --- | --- |
| Lab 01 | Build and boot a minimal ARM Embedded Linux system | [Lab01/README.md](Lab01/README.md) |
| Lab 02 | Character device driver, procfs/sysfs, system integration and JFFS2 | [Lab02/README.md](Lab02/README.md) |
| Lab 03 | Remote debugging, memory analysis, profiling and real-time latency | [Lab03/README.md](Lab03/README.md) |
| Assignment 01 | ioctl character driver and misc counter driver | [Assignment01/README.md](Assignment01/README.md) |
| Assignment 02 | Multithreaded sensor-monitoring system with kernel/user-space integration | [Assignment02/README.md](Assignment02/README.md) |

Top-level layout:

```text
embedded-linux-kernel-development/
├── Lab01/
├── Lab02/
├── Lab03/
├── Assignment01/
├── Assignment02/
├── .gitignore
└── README.md
```

Each module contains only the files relevant to that exercise, such as source code, configuration files, root filesystems, generated images, logs, diagrams, reports, or test artifacts.

## Lab 01: Linux Kernel Build and Boot

Lab 01 establishes the base ARM Embedded Linux environment.

The completed system contains:

- a Linux 5.15 ARM kernel image;
- the `vexpress-v2p-ca9.dtb` Device Tree Blob;
- BusyBox userspace;
- a complete initramfs filesystem;
- saved kernel and BusyBox configurations;
- bootable output artifacts for QEMU.

The laboratory demonstrates the relationship between the kernel, Device Tree, initial RAM filesystem, BusyBox initialization, and the QEMU machine model.

See [Lab 01 documentation](Lab01/README.md) for the boot command, initialization sequence, configuration details, and verification procedure.

## Lab 02: Character Device Driver and JFFS2

Lab 02 extends the Lab 01 system with a loadable Linux character device driver.

The driver exposes a `/dev/lab2` interface and demonstrates:

- `open`, `read`, `write`, and `release` file operations;
- safe userspace/kernel-space data transfer;
- shared-state protection;
- runtime information through procfs;
- device attributes through sysfs;
- automatic module integration into the target initramfs;
- shell-based validation;
- an MTD/JFFS2 experiment on the development host.

The repository keeps the driver source, compiled module, target root filesystem, generated initramfs, validation logs, diagrams, and report material together under `Lab02/`.

See [Lab 02 documentation](Lab02/README.md) for the driver interface, build process, target integration, test procedures, and JFFS2 experiment.

## Lab 03: Debugging and Performance Analysis

Lab 03 focuses on debugging and performance analysis across both the Linux host and the ARM target.

The work is divided into four areas:

1. Remote debugging of ARM applications with GDB and gdbserver.
2. Memory-error and leak analysis with Valgrind and Linux `/proc` interfaces.
3. System-call and CPU profiling with strace and perf.
4. Scheduling-latency analysis with cyclictest and stress-ng.

The module retains test applications, ARM and host executables, profiling data, console logs, latency measurements, plots, and the generated ARM initramfs used during the experiments.

Real-time latency measurements are performed on the host where appropriate rather than treating QEMU timing as equivalent to physical hardware timing.

See [Lab 03 documentation](Lab03/README.md) for the experimental setup, measured results, debugging workflow, and analysis.

## Assignment 01: ioctl and Misc Device Drivers

Assignment 01 develops two kernel-driver exercises on top of the ARM Linux environment.

The first task implements a character driver with:

- custom ioctl commands;
- a shared userspace/kernel ioctl header;
- runtime statistics;
- procfs and sysfs interfaces;
- a userspace ioctl test utility.

The second task implements a counter device using the Linux miscdevice framework, together with repeatable test scripts and recorded logs.

This assignment emphasizes interface design between userspace and kernel space rather than only basic module loading.

See [Assignment 01 documentation](Assignment01/README.md) for the ioctl definitions, driver behavior, build instructions, and validation results.

## Assignment 02: Sensor Monitoring System

Assignment 02 combines kernel-space and userspace components into a larger Embedded Linux application.

The repository contains:

- a simulated sensor kernel-driver implementation;
- a multithreaded C application;
- a circular buffer for sample transfer;
- processing and alert threads;
- POSIX message-queue communication;
- asynchronous logging;
- scheduling and jitter measurements;
- GDB, Valgrind, strace, and perf evidence;
- system architecture and application-state diagrams.

The assignment demonstrates how lower-level topics from the laboratories can be combined into a concurrent userspace application interacting with a Linux device driver.

See [Assignment 02 documentation](Assignment02/README.md) for the architecture, build procedure, runtime modes, profiling results, and limitations.

## Learning Progression

```text
Lab 01
System bring-up
    |
    v
Lab 02
Kernel module and device-driver integration
    |
    v
Lab 03
Debugging, memory analysis and performance profiling
    |
    v
Assignment 01
Kernel/user-space control interfaces
    |
    v
Assignment 02
Integrated multithreaded Embedded Linux application
```

This progression follows a practical Embedded Linux workflow: establish a bootable target, add kernel functionality, learn how to observe and debug the system, and then integrate those components into larger applications.

## Getting Started

Clone the repository:

```bash
git clone https://github.com/myNameDyK/embedded-linux-kernel-development.git
cd embedded-linux-kernel-development
```

There is intentionally no single top-level build command because the modules target different stages of the Embedded Linux workflow and use different host-side tools.

For a first pass through the repository, follow the module documentation in this order:

```text
Lab01/README.md
Lab02/README.md
Lab03/README.md
Assignment01/README.md
Assignment02/README.md
```

## Reproducibility Notes

Embedded Linux artifacts are sensitive to target architecture, kernel configuration, toolchain, and runtime environment. When rebuilding or reusing this repository:

- build kernel modules against a kernel tree and configuration compatible with the target kernel;
- build ARM userspace programs with an ARM-compatible toolchain and provide matching runtime libraries for dynamically linked binaries;
- keep the Device Tree, kernel image, and QEMU machine configuration consistent;
- prefer rebuilding prebuilt modules and binaries when the toolchain or target kernel changes;
- treat host-side profiling results as environment-specific measurements rather than universal performance numbers;
- use QEMU for functional validation and debugging, but do not interpret emulator timing as a direct measurement of physical ARM hardware;
- preserve configuration files, logs, and reports when reproducing experiments because they are part of the validation record.

## Validation and Evidence

Depending on the module, the repository preserves:

- kernel and BusyBox configuration files;
- Linux kernel modules and ARM executables;
- initramfs images;
- boot and test logs;
- GDB sessions;
- Valgrind reports;
- strace and perf output;
- cyclictest and stress-ng measurements;
- architecture diagrams;
- laboratory reports.

This makes it possible to inspect not only how each exercise was implemented, but also how it was validated.

## Documentation Structure

Documentation is split into two levels:

- this root README gives the architecture, scope, environment, progression, and navigation for the repository;
- each Lab or Assignment README contains detailed commands, implementation notes, measurements, and exercise-specific conclusions.

This keeps the repository landing page concise while preserving detailed technical information close to the files it describes.

## Repository Status

This repository is maintained as an educational Embedded Linux coursework archive. It is intended for study, reproduction of the laboratory workflows, and review of the resulting artifacts. It is not a production Board Support Package, and any reuse on a different board, kernel, or toolchain should begin by validating the target-specific assumptions documented in each module.
