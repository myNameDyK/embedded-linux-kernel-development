# Assignment 02 – Sensor Monitoring System

| Item | Configuration |
| --- | --- |
| Course | Embedded Linux Systems |
| Test platform | QEMU Vexpress-A9, ARMv7, Linux 5.15 |
| Programming | C, Linux Kernel Module, POSIX Threads, POSIX Message Queue |

## 1. Introduction

This project implements a **simulated temperature and humidity monitoring system** on Embedded Linux. A kernel module generates sensor samples, while the `sms_app` application periodically collects data, calculates moving averages, detects temperature and humidity threshold violations or abnormal temperature changes, generates alerts, and writes logs. The system is tested on both Ubuntu x86_64 and QEMU ARMv7.

## 2. Directory Structure

```text
Assignment02/
├── README.md                        # Instructions and experimental results
├── diagrams/                        # Two system diagrams
│   ├── system_architecture.png
│   └── application_state_machine.png
├── app/
│   ├── sms.h                        # Shared structures and interfaces
│   ├── sms_app.c                    # Main, CLI, thread creation/joining, statistics
│   ├── sensor_thread.c              # Sampling, sampling schedule, jitter
│   ├── processor_thread.c           # Moving averages and alert detection
│   ├── alert_thread.c               # Alert reception through POSIX MQ
│   ├── logger_thread.c              # Log queue, file writing, log rotation
│   ├── circular_buffer.c            # Shared ring buffer
│   ├── Makefile                     # ARM and x86 application builds
│   ├── sms_app                      # ARM binary
│   └── sms_app_x86                  # x86 binary
├── driver/
│   ├── sms_sensor_driver.c          # Simulated sensor kernel driver
│   ├── sms_sensor.h                 # ioctl definitions
│   ├── Makefile                     # ARM kernel module build
│   └── sms_sensor.ko                # Compiled kernel module
├── kernel/linux-5.15/               # Dedicated kernel source tree
├── rootfs/                          # QEMU root filesystem
├── output/
│   └── initramfs_asgn2.cpio.gz      # Packaged initramfs
├── logs/                            # Valgrind / GDB / strace / perf / jitter
└── report/                          # DOCX report
```

> `kernel/`, `rootfs/`, and `output/` may be very large. Commit only components required for submission or permitted for redistribution.

## 3. System Design

### 3.1. System Architecture

<img width="984" height="337" alt="system_architecture" src="https://github.com/user-attachments/assets/e043d4f8-6f46-4a31-a389-bcaefeb45cc2" />

*Figure 1. System architecture showing kernel space, four worker threads, and data and IPC flows.*

- **Kernel module `sms_sensor.ko`:** Exposes `/dev/sms_sensor` for reading sensor samples through `read()`, supports sampling interval configuration through `ioctl()`, and provides statistics through `/proc/sms_stats`.
- **Sensor Thread:** Samples periodically using `clock_nanosleep(... TIMER_ABSTIME ...)`, supports `--mock`, and pushes data into the **Circular Buffer**.
- **Processor Thread:** Receives samples, calculates the average of the latest 10 samples, and detects `TEMP_HIGH`, `HUMID_HIGH`, and `TEMP_SPIKE`.
- **Alert Thread:** Receives notifications through a **POSIX Message Queue** and records alerts.
- **Logger Thread:** Retrieves data from **LogQueue** and writes it to `/tmp/sms.log`.

**Diagram note:** `/proc/sms_stats` is an interface for users to read statistics through the terminal, rather than a direct sample source for the Sensor Thread. The main sample logging path is Processor Thread → LogQueue → Logger Thread.

### 3.2. State Machine

<img width="450" height="371" alt="application_state_machine" src="https://github.com/user-attachments/assets/9fdeb246-0f4e-4858-a572-dc08c4be283a" />

*Figure 2. Application state machine: INIT → RUNNING → SHUTDOWN → EXIT, including an error path.*

- **INIT:** Initializes buffers, mutexes, queues, signal handlers, and threads.
- **RUNNING:** Periodically collects and processes samples, generates alerts, and writes logs.
- **SHUTDOWN:** When the configured duration expires, `SIGINT`/`SIGTERM` is received, or an error requires termination, requests thread termination, closes queues, joins threads, flushes logs, and releases memory.
- **ERROR/EXIT:** If initialization fails, cleans up resources that have already been created and exits.

## 4. Environment and Execution

**Tools used:** QEMU ARM 10.2.1, GCC cross-compiler `arm-linux-gnueabihf-gcc` 15.2.0, Valgrind 3.26.0, and Linux 5.15.0 as the guest kernel.

### 4.1. Building the Application

On the **Ubuntu host**, from the repository directory:

```bash
cd app
make host       # Build sms_app_x86
make arm        # Build sms_app for ARMv7
```

The kernel driver must be built against the **same ARM Linux 5.15 kernel tree used to build the QEMU guest kernel**, with `KDIR` configured accordingly in the Makefile. During development, ARMv7-A flags were used to resolve assembler errors involving `isb` and `dmb`.

### 4.2. Booting QEMU

Example using the paths from the implemented environment:

```bash
qemu-system-arm \
  -M vexpress-a9 -cpu cortex-a9 -m 512M \
  -kernel ~/Assignment02/kernel/linux-5.15/arch/arm/boot/zImage \
  -dtb ~/Assignment02/kernel/linux-5.15/arch/arm/boot/dts/vexpress-v2p-ca9.dtb \
  -initrd ~/Assignment02/output/initramfs_asgn2.cpio.gz \
  -append "console=ttyAMA0 rdinit=/init" -nographic
```

Inside **QEMU** (`/ #`):

```sh
mkdir -p /dev/mqueue
mount -t mqueue none /dev/mqueue    # If not already mounted
insmod /lib/modules/5.15.0/sms_sensor.ko
cat /proc/sms_stats
/usr/bin/sms_app --other -i 500 -d 60 -v
cat /proc/sms_stats
tail -20 /tmp/sms.log
```

Run with simulated data generated directly in userspace:

```sh
/usr/bin/sms_app --mock --other -i 500 -d 10 -v
```

To test shutdown through SIGINT, run with `-d 0` and press **Ctrl+C**. To exit QEMU in `-nographic` mode, press **Ctrl+A**, followed by **X**.

> The ARM application is dynamically linked. The initramfs must contain `/lib/ld-linux-armhf.so.3`, `libc.so.6`, and `libm.so.6`. The QEMU guest kernel must have `CONFIG_POSIX_MQUEUE=y` enabled.

## 5. Observed Test Results

### Task 1 – Kernel Driver and Multithreading

The driver sampling test used a **500 ms** sampling interval and a **60-second** duration:

| Metric | Result |
|---|---:|
| Samples produced / processed | **117 / 117** |
| Buffer overflow | **0** |
| High-temperature alerts (`TEMP_HIGH`) | 15 |
| High-humidity alerts (`HUMID_HIGH`) | 22 |
| Temperature-change alerts (`TEMP_SPIKE`) | 76 |
| Dropped log messages | **0** |

The actual sample count was **117**, compared with approximately **120 expected samples**, a difference of 3 samples (2.5%). The `/tmp/sms.log` file contained SAMPLE and ALERT entries. During development, missed samples caused by `EAGAIN` were reduced by adding short retries in the Sensor Thread.

### Task 2 – Memory Management

On the host, Valgrind was used to check `sms_app_x86 --mock --other -i 500 -d 35`:

```text
HEAP SUMMARY:
  in use at exit: 0 bytes in 0 blocks
  total heap usage: 21 allocs, 21 frees, 15,216 bytes allocated
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts
```

In QEMU, the memory footprint was measured during a 70-second test:

| Measurement time | VmRSS | VmSize |
|---|---:|---:|
| 5 seconds | 34,128 kB | 37,036 kB |
| 30 seconds | 34,128 kB | 37,036 kB |
| 55 seconds | 34,128 kB | 37,036 kB |

No increase in RSS or virtual memory was observed at these three measurement points. This alone does not prove the absence of memory leaks, but it is consistent with the Valgrind results from the observed test run.

### Task 3 – GDB, strace, and perf

- **GDB:** Set a breakpoint at `ring_pop()` and inspected the buffer variables `head`, `tail`, `size`, `produced`, and `consumed`, together with the thread list and thread call stacks.
- **strace:** Saved statistics using `strace -f -c`. The screenshot shows prominent system calls including `clock_nanosleep`, `futex`, and `mq_timedreceive`.
- **perf:** Generated `perf_sms.data` and `perf_sms.txt`. **No CPU hotspot conclusion is presented yet**, pending a full review of `perf_sms.txt`.

### Task 4 – Real-Time Scheduling

A Sensor Thread was verified to use **SCHED_FIFO with RT priority 50** (`policy=1`, `rt_priority=50`). During the memory-locking test, `VmLck = 37,088 kB`.

| Metric | SCHED_OTHER | SCHED_FIFO |
|---|---:|---:|
| Jitter samples | **1,039** | **1,040** |
| Min (µs) | 427 | 360 |
| Avg (µs) | 5,032.42 | 5,054.56 |
| Max (µs) | 9,811 | 9,997 |
| P99 (µs) | **9,554** | **9,647** |
| Histogram `<100 / 100–500 / 500–1000 / ≥1000 µs` | 0 / 3 / 64 / 972 | 0 / 6 / 50 / 984 |

FIFO P99 was **93 µs higher (~0.97%)** in this measurement; no P99 improvement was observed. Execution through QEMU may be affected by CPU scheduling on the Ubuntu host. A single measurement should not be generalized into a conclusion about SCHED_FIFO.

## 6. Limitations and Future Work

- Carefully review synchronization of the shared stop variables (`app->stop`, `sensor_error`) and verify the absence of race conditions and deadlocks across all execution paths.
- Review the actual perf results to add hotspot analysis and evidence-based optimization proposals.
- Test log rotation when the log file exceeds 1 MB and perform additional independent jitter measurements.
- Explore PREEMPT_RT, thread affinity configuration, and ways to reduce host scheduling effects during QEMU measurements.
