# LAB-03 — EMBEDDED LINUX SYSTEMS

**Remote Debugging | Memory Analysis | Performance Profiling | Real-Time Latency**

**Date:** October 17, 2026  
**Course:** Embedded Linux Systems  
**Topic:** Kernel Debugging & Embedded Application Performance Analysis

---

## 1. Project Overview

LAB-03 focuses on building and testing an Embedded Linux environment to identify programming errors, analyze memory usage, evaluate application performance, and measure operating system scheduling latency.

The system is deployed in two environments:

- **Host:** AlmaLinux 9.8, x86_64 architecture.
- **Target:** ARMv7-A Linux, emulated using QEMU with the `vexpress-a9` machine and ARM Cortex-A9 CPU.

Applications are developed in C and compiled for either x86_64 or ARM, depending on the testing requirements.

The project consists of four main components:

1. **Remote Debugging:** Using GDB and gdbserver to identify programming errors in ARM applications.
2. **Memory Analysis:** Using Valgrind to detect and fix memory leaks.
3. **Performance Profiling:** Analyzing system calls, CPU hotspots, and hardware performance counters.
4. **Real-Time Analysis:** Measuring Linux scheduling latency under normal and stressed system conditions.

All results are organized into source code, executables, log files, charts, and PDF reports to support verification, evaluation, and experiment reproducibility.

---

## 2. System Architecture

### 2.1. Host–Target Model

The system follows a Host–Target architecture.

```text
                 ALMALINUX HOST (x86_64)
+------------------------------------------------------+
|                                                      |
|  C Source Code                                       |
|       |                                              |
|       +--> GCC x86_64                                |
|       |       |                                      |
|       |       +--> Valgrind                          |
|       |       +--> strace                            |
|       |       +--> perf                              |
|       |                                              |
|       +--> ARM Cross Compiler                        |
|               |                                      |
|               +--> ARM Executables                   |
|                        |                             |
|                        v                             |
|                  ARM initramfs                       |
|                        |                             |
|                        v                             |
|                   QEMU ARMv7                         |
|                                                      |
|  GDB Host <------ UART / TCP ------> gdbserver       |
|                                                      |
|  cyclictest + stress-ng                              |
|       |                                              |
|       +--> Scheduling Latency Measurement            |
|                                                      |
+------------------------------------------------------+
                         |
                         v
                  QEMU ARM TARGET
+------------------------------------------------------+
|                                                      |
|  ARM Cortex-A9 (vexpress-a9)                         |
|  Linux Kernel 5.15                                   |
|  BusyBox Root Filesystem                             |
|                                                      |
|  /usr/bin/buggy_app                                  |
|  /usr/bin/leaky_app_arm                               |
|  /usr/bin/leaky_app_arm_probe                         |
|  /usr/bin/perf_target_arm                             |
|  /usr/bin/gdbserver                                  |
|  /usr/bin/strace                                     |
|                                                      |
|  /proc/<PID>/status                                  |
|  /proc/<PID>/maps                                    |
|                                                      |
+------------------------------------------------------+
```

### 2.2. Roles of Each Environment

**AlmaLinux Host**

The Host is responsible for application development, compilation, remote debugging control, and performance analysis.

Main tools:

- `gcc`: Compiles x86_64 applications.
- ARM cross-compiler: Compiles applications for ARM.
- `gdb`: Remotely debugs ARM applications.
- `valgrind`: Detects memory leaks.
- `strace`: Collects and analyzes system call statistics.
- `perf`: Profiles CPU performance.
- `cyclictest`: Measures scheduling latency.
- `stress-ng`: Generates CPU, I/O, and memory workloads.
- `gnuplot`: Generates latency charts.

**QEMU ARM Target**

The Target emulates an ARM Embedded Linux environment, executes ARM binaries, and provides runtime information about application behavior.

System configuration:

| Component | Configuration |
|---|---|
| Machine | QEMU `vexpress-a9` |
| CPU | ARM Cortex-A9 |
| Architecture | ARMv7-A |
| Kernel | Linux 5.15 |
| RAM | 512 MB |
| Root filesystem | BusyBox initramfs |
| GDB server | ARM gdbserver |
| Profiling | ARM strace |
| Console | UART PL011 |

For Remote Debugging, GDB on the Host communicates with gdbserver through a virtual UART connection forwarded to TCP `127.0.0.1:1234`.

Real-Time Latency measurements are performed directly on the Host to avoid timing distortions introduced by QEMU emulation.

---

## 3. Project Directory Structure

```text
Lab03/
|
|-- README.md
|-- README_NopBai.txt
|
|-- apps/
|   |
|   |-- buggy_app.c
|   |-- buggy_app
|   |
|   |-- leaky_app.c
|   |-- leaky_app_buggy.c
|   |-- leaky_app
|   |
|   |-- leaky_app_fixed.c
|   |-- leaky_app_fixed
|   |
|   |-- leaky_app_arm
|   |-- leaky_app_arm_probe.c
|   |-- leaky_app_arm_probe
|   |-- measure_leaky_arm.sh
|   |
|   |-- perf_target.c
|   |-- perf_target
|   |-- perf_target_profile.c
|   |-- perf_target_profile
|   |-- perf_target_arm
|
|-- output/
|   |
|   |-- initramfs_lab3.cpio.gz
|
|-- logs/
|   |
|   |-- gdb_session.txt
|   |-- gdbserver_dependencies.txt
|   |
|   |-- valgrind_report.txt
|   |-- valgrind_fixed.txt
|   |-- leaky_app_fix.diff
|   |-- b3_qemu_console.log
|   |
|   |-- strace_full.txt
|   |-- strace_summary.txt
|   |-- perf_profile.data
|   |-- perf_report.txt
|   |-- perf_stat.txt
|   |-- c3_qemu_console.log
|   |
|   |-- d_host_kernel_info.txt
|   |-- d_standard_environment.txt
|   |-- d_stress_environment.txt
|   |-- d_stressng.txt
|   |-- cyclictest_standard.txt
|   |-- cyclictest_stress.txt
|   |-- hist_standard.txt
|   |-- hist_stress.txt
|   |-- d_latency_summary.txt
|   |-- latency_histogram.gnuplot
|   |-- latency_histogram.png
|   |
|   |-- ... additional experiment logs
|
|-- report/
    |
    |-- Lab03_Report.pdf
```

### 3.1. `apps/` Directory

This directory contains all source files and compiled executables.

| File | Description |
|---|---|
| `buggy_app.c` | C application containing three intentional programming errors for GDB practice |
| `buggy_app` | ARM executable used for Remote Debugging |
| `leaky_app.c` | Original source code containing memory leaks |
| `leaky_app_buggy.c` | Backup of the original source code before modification |
| `leaky_app` | Original x86_64 executable with memory leaks |
| `leaky_app_fixed.c` | Modified source code with proper memory deallocation |
| `leaky_app_fixed` | Fixed x86_64 executable |
| `leaky_app_arm` | ARM executable of the original application |
| `leaky_app_arm_probe.c` | Source code containing checkpoints for ARM memory measurements |
| `leaky_app_arm_probe` | ARM executable with memory measurement checkpoints |
| `measure_leaky_arm.sh` | Script for collecting VmRSS, VmSize, and memory mapping information |
| `perf_target.c` | Application generating CPU-intensive and I/O workloads |
| `perf_target` | Original x86_64 executable |
| `perf_target_profile.c` | Modified application with an extended computational workload for profiling |
| `perf_target_profile` | Executable used for performance profiling |
| `perf_target_arm` | ARM executable used for system call analysis |

### 3.2. `output/` Directory

This directory contains the ARM root filesystem image.

**`initramfs_lab3.cpio.gz`**

This filesystem is loaded during QEMU startup and contains BusyBox, ARM applications, and debugging tools.

The image is used together with the Linux kernel and Device Tree generated in LAB-01.

### 3.3. `logs/` Directory

This directory stores the experimental evidence and measurement results.

- **GDB logs:** Store breakpoints, backtraces, register values, and variable inspection results.
- **Valgrind logs:** Record memory allocation errors before and after fixing the source code.
- **strace/perf logs:** Contain system call statistics, CPU hotspots, and performance counter measurements.
- **cyclictest/stress-ng logs:** Store scheduling latency results, histograms, and experimental configurations.

The original log files are preserved to ensure that the reported results can be traced back to their experimental evidence.

### 3.4. `report/` Directory

This directory contains the official PDF report:

`Lab03_Report.pdf`

The report presents the experimental methodology, results, technical analysis, and answers to the LAB-03 assignment questions.

---

## 4. Implementation and Experimental Results

### 4.1. Part A — Remote Debugging with GDB

**Objective**

Establish a remote debugging connection from the x86_64 Host to an application running on ARM QEMU and identify the causes of three C programming errors.

**Tools**

- GDB on the Host.
- gdbserver on ARM.
- QEMU ARMv7.
- UART-to-TCP forwarding.

**Investigated Errors**

| Error | Cause | Evidence |
|---|---|---|
| Array Out-of-Bounds | Loop condition uses `i <= len` | `i = 5`, `len = 5` |
| Use-After-Free | A pointer is used after `free()` | Pointer `0x8dda8` is still returned |
| Integer Overflow | Multiplication exceeds the range of a signed 32-bit integer | Observed value: `1,410,065,408` |

For the integer overflow case, the mathematical operation:

`100000 × 100000 = 10000000000`

exceeds the representable range of a signed 32-bit `int`.

The debugging process uses breakpoints, `print`, `backtrace`, and `info registers` to inspect program execution.

**Results**

All three programming errors and their associated source locations were identified.

The errors were analyzed using GDB breakpoints and runtime inspection. The results do not imply that all three errors necessarily caused application crashes.

**Evidence:** `logs/gdb_session.txt`.

---

### 4.2. Part B — Memory Analysis with Valgrind

**Objective**

Detect memory leaks, modify the source code to fix them, and verify that no memory leaks remain at program termination in the tested execution.

**Procedure**

1. Execute the original `leaky_app` using Valgrind.
2. Analyze allocation stack traces.
3. Identify dynamically allocated memory blocks that were not released.
4. Add the necessary `free()` calls at appropriate locations.
5. Run Valgrind again on the corrected executable.
6. Cross-compile the application and inspect its memory usage on ARM.

**Valgrind Results**

| Metric | Before Fix | After Fix |
|---|---:|---:|
| Definitely lost | 16,900 bytes | 0 bytes |
| Lost blocks | 250 | 0 |
| Allocations | 352 | 352 |
| Frees | 102 | 352 |
| Error contexts | 3 | 0 |

The detected memory leaks were associated with:

- Name storage: 2,100 bytes.
- Score storage: 2,000 bytes.
- Message processing: 12,800 bytes.

Total confirmed memory leak:

**16,900 bytes**

After fixing the source code, Valgrind reported:

`ERROR SUMMARY: 0 errors from 0 contexts`

**ARM Memory Analysis**

The `leaky_app_arm_probe` application includes two `SIGSTOP` checkpoints to capture process memory information before and after processing.

| Metric | Before Processing | After Processing |
|---|---:|---:|
| VmSize | 776 KB | 776 KB |
| VmRSS | 4 KB | 4 KB |
| VmData | 152 KB | 152 KB |

The unchanged VmRSS values across the two snapshots do not prove the absence of memory leaks because additional allocations may occur within already allocated heap regions.

Valgrind provides the primary evidence for identifying and fixing the memory leaks.

**Evidence:** `valgrind_report.txt`, `valgrind_fixed.txt`, `b3_qemu_console.log`.

---

### 4.3. Part C — Performance Profiling

**Objective**

Evaluate application performance from two perspectives:

- System calls and the overhead of interacting with the kernel.
- CPU hotspots and computational resource utilization.

**Workload**

The `perf_target` application performs three main types of operations:

1. CPU-intensive computations using `sqrt()` and `log()`.
2. Linear searches over data arrays.
3. File I/O operations that write 10,000 lines.

**strace Results on x86_64**

| System Call | Calls | % Syscall Time |
|---|---:|---:|
| write | 70 | 30.10% |
| execve | 1 | 20.23% |
| munmap | 3 | 12.85% |
| close | 4 | 12.35% |
| mmap | 14 | 9.45% |

Total system calls: **122**

Total measured syscall time: **1.206 ms**

**perf Results**

To increase the number of collected CPU samples, `perf_target_profile` executes 3,000 computational iterations instead of the 50 iterations used in the original application.

| Metric | Result |
|---|---:|
| CPU samples | 395 |
| Lost samples | 0 |
| heavy_compute overhead | 51.39% |
| Elapsed time | 1.9806 seconds |
| CPU cycles | 4,841,444,106 |
| Instructions | 21,608,569,672 |
| IPC | 4.46 |
| Cache misses | 488,237 |
| Branch misses | 13,534 |

The `heavy_compute()` function was identified as the largest CPU hotspot in the profiling experiment.

A significant number of CPU samples were also associated with `libm.so.6`, consistent with the workload's use of mathematical functions such as `sqrt()` and `log()`.

**Profiling on ARM QEMU**

| Metric | Result |
|---|---:|
| Architecture | ARMv7 |
| Execution time | 33.68 seconds |
| Total system calls | 100 |
| Total syscall time | 17.364 ms |

The x86_64 and ARM measurements were performed on different architectures and execution environments.

QEMU introduces emulation overhead, so the ARM QEMU execution time should not be interpreted as the actual performance of a physical Cortex-A9 processor.

Additionally, the 3,000-iteration profiling workload must not be directly compared with the 50-iteration ARM workload.

**Evidence:** `strace_summary.txt`, `perf_report.txt`, `perf_stat.txt`, `c3_qemu_console.log`.

---

### 4.4. Part D — Real-Time Latency Analysis

**Objective**

Evaluate Linux thread wake-up and scheduling latency under normal system conditions and under additional computational load.

The measurements were performed directly on the AlmaLinux Host rather than inside QEMU.

**cyclictest Configuration**

| Parameter | Value |
|---|---|
| Kernel | Linux 5.14 PREEMPT_DYNAMIC |
| Threads | 4 |
| Scheduling policy | SCHED_FIFO |
| Priority | 80 |
| Interval | 1000 µs |
| Loops | 10,000 per thread |
| Memory locking | Enabled |
| Histogram limit | 5000 µs |

**Baseline Results**

| Thread | Min (µs) | Avg (µs) | Max (µs) |
|---|---:|---:|---:|
| 0 | 7 | 14 | 121 |
| 1 | 6 | 14 | 124 |
| 2 | 8 | 15 | 142 |
| 3 | 7 | 13 | 188 |

Maximum observed latency: **188 µs**

No samples exceeded 500 µs among the 40,000 recorded samples.

**Results Under stress-ng Load**

The stress workload consisted of:

- 16 CPU workers.
- 4 I/O workers.
- 2 memory workers.

| Thread | Min (µs) | Avg (µs) | Max (µs) |
|---|---:|---:|---:|
| 0 | 6 | 12 | 2,042 |
| 1 | 4 | 10 | 1,121 |
| 2 | 4 | 10 | 964 |
| 3 | 6 | 19 | 3,919 |

Maximum observed latency: **3,919 µs**

The histogram recorded **42 samples exceeding 500 µs** out of 39,968 recorded samples.

**Comparison**

| Metric | Baseline | Under Stress |
|---|---:|---:|
| Maximum latency | 188 µs | 3,919 µs |
| Samples over 500 µs | 0 | 42 |
| 500 µs deadline | Met during the test | Not met during the test |

**Analysis**

The results demonstrate that low average scheduling latency does not guarantee that a system will always meet its timing deadlines.

Under heavy system load, occasional latency spikes can delay task execution beyond the required response time.

For a robotic control system with a strict deadline below 500 µs, PREEMPT_RT and additional real-time system configuration and testing should be considered.

However, this project only performed direct measurements using a `PREEMPT_DYNAMIC` kernel. PREEMPT_RT was discussed theoretically and was not separately evaluated through experiments.

**Chart:** `logs/latency_histogram.png`.

---

## 5. Reproduction Instructions

The following commands assume that the user is working from the root directory of the extracted LAB-03 project.

### 5.1. Run the Memory Leak Test

```bash
./apps/leaky_app

valgrind --leak-check=full \
  --show-leak-kinds=all \
  ./apps/leaky_app
```

After fixing the memory leaks:

```bash
valgrind --leak-check=full \
  --show-leak-kinds=all \
  ./apps/leaky_app_fixed
```

### 5.2. Run strace on the Host

```bash
strace -c ./apps/perf_target
```

### 5.3. Run perf record/report

```bash
perf record -g \
  --call-graph fp \
  -F 199 \
  -e cpu-clock:u \
  -- ./apps/perf_target_profile
```

View the profiling report:

```bash
perf report --stdio
```

### 5.4. Run perf stat

```bash
perf stat \
  -e task-clock,cycles:u,instructions:u,cache-misses:u,branches:u,branch-misses:u \
  -- ./apps/perf_target_profile
```

### 5.5. Boot ARM QEMU

The Linux kernel and DTB generated in LAB-01 must be available.

```bash
qemu-system-arm \
  -M vexpress-a9 \
  -cpu cortex-a9 \
  -m 512M \
  -smp 2 \
  -display none \
  -monitor none \
  -serial stdio \
  -nic none \
  -kernel "$HOME/embedded_lab1/output/zImage" \
  -dtb "$HOME/embedded_lab1/output/vexpress-v2p-ca9.dtb" \
  -initrd "./output/initramfs_lab3.cpio.gz" \
  -append 'console=ttyAMA0,115200 rdinit=/sbin/init mem=512M'
```

Inside ARM QEMU:

```bash
/usr/bin/measure_leaky_arm.sh
```

Analyze system calls:

```bash
/usr/bin/strace -c /usr/bin/perf_target_arm
```

### 5.6. Run cyclictest on the Host

```bash
sudo /usr/bin/cyclictest \
  --default-system \
  --mlockall \
  --threads=4 \
  --priority=80 \
  --policy=fifo \
  --interval=1000 \
  --loops=10000 \
  --histogram=5000 \
  --histfile=/tmp/lab03_hist_recheck.txt \
  --quiet
```

**Note:** This command should not be executed inside ARM QEMU.

Repeated measurements may produce different results depending on the system state, CPU load, and scheduling conditions.

---

## 6. Technical Issues and Limitations

### 6.1. GDB Remote Connection

The QEMU build used in this project does not support SLIRP user-mode networking.

Therefore, the remote GDB connection was established through UART-to-TCP forwarding instead of an IP network connection inside the virtual machine.

### 6.2. Memory Leak Detection

VmRSS is not a direct memory leak detection mechanism.

Valgrind was used to identify dynamically allocated memory blocks that were not properly released.

### 6.3. CPU Profiling

`perf` collects CPU execution samples using statistical sampling.

Therefore, a function's reported overhead percentage does not necessarily represent its exact percentage of total application execution time.

The profiling version extends the computational workload to improve the reliability of the collected samples.

### 6.4. ARM Performance

QEMU emulates an ARM system, so execution time measurements are affected by emulation overhead and the Host configuration.

Accurate evaluation of physical Cortex-A9 performance would require additional experiments on corresponding ARM hardware.

### 6.5. Real-Time Constraints

The tested kernel uses `PREEMPT_DYNAMIC`, not PREEMPT_RT.

The limited number of test iterations is insufficient to guarantee that timing deadlines will always be met under all operating conditions.

The stress-test histogram contains 39,968 recorded samples, and this value is preserved according to the original experimental data.

---

## 7. Submission Files and Experimental Evidence

**Official Report**

`report/Lab03_Report.pdf`

**Remote Debugging**

`logs/gdb_session.txt`

**Memory Analysis**

`logs/valgrind_report.txt`  
`logs/valgrind_fixed.txt`  
`logs/b3_qemu_console.log`

**Performance Profiling**

`logs/strace_summary.txt`  
`logs/perf_report.txt`  
`logs/perf_stat.txt`  
`logs/c3_qemu_console.log`

**Real-Time Analysis**

`logs/hist_standard.txt`  
`logs/hist_stress.txt`  
`logs/d_latency_summary.txt`  
`logs/latency_histogram.png`

---

## 8. Conclusion

LAB-03 successfully implemented and evaluated several essential tools for Embedded Linux application development, debugging, and performance analysis.

The main achievements include:

- Identifying three C programming errors using GDB Remote Debugging.
- Detecting 16,900 bytes of memory leaks and correcting the source code until Valgrind reported zero errors.
- Identifying `heavy_compute()` as the largest CPU hotspot in the profiling workload.
- Collecting and analyzing system calls on both x86_64 and ARM QEMU.
- Measuring scheduling latency on the Host and observing latency spikes exceeding the 500 µs deadline under system stress.
- Organizing experimental logs, histograms, and a PDF report to support verification and evaluation.

The project demonstrates the differences between programming logic errors, memory management problems, CPU performance limitations, and scheduling latency in Embedded Linux systems.

---

**Embedded Linux Systems — LAB-03**  
**October 17, 2026**
