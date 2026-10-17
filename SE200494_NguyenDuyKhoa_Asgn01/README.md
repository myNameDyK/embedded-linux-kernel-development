# Assignment 01: IOCTL and Misc Counter Drivers

## Introduction

Assignment 01 develops two Linux kernel modules for an ARM Embedded Linux target.

Task 1 extends a character device driver with an IOCTL control interface, runtime statistics, procfs information, and sysfs attributes. A dedicated userspace program is used to issue IOCTL requests to the driver.

Task 2 implements a counter device using the Linux miscdevice framework. The device automatically increments its counter after each read, accepts non-negative integer values through write operations, and exposes runtime statistics through procfs.

Both kernel modules are built for the Linux 5.15 ARM target used in the previous laboratories.

## Assignment objectives

The assignment focuses on the following kernel programming concepts:

1. Implementing character-device file operations.
2. Communicating between userspace and kernel space.
3. Defining and processing custom IOCTL commands.
4. Sharing IOCTL definitions between kernel and userspace code.
5. Maintaining driver runtime statistics.
6. Using procfs and sysfs to expose driver state.
7. Registering a device through the Linux miscdevice framework.
8. Protecting shared state with mutexes and atomic operations.
9. Cross-compiling kernel modules and userspace programs for ARM.
10. Validating driver behavior with repeatable shell scripts.

## Development environment

The assignment uses the same ARM Embedded Linux environment established in the previous laboratories.

| Item | Configuration |
| --- | --- |
| Target architecture | ARMv7-A |
| Processor | ARM Cortex-A9 |
| Linux kernel | 5.15.0 |
| Cross compiler | `arm-linux-gnueabihf-` |
| Kernel source | `~/embedded_lab1/kernel/linux-5.15` |
| Task 1 device | `/dev/asgn1` |
| Task 2 device | `/dev/counter` |
| Task 1 major number | 241 |
| Task 2 registration | Linux miscdevice framework |

Both kernel modules were rebuilt from the current source code and verified as 32-bit ARM EABI5 kernel modules.

The Task 1 userspace IOCTL utility was also cross-compiled successfully as a 32-bit ARM executable.

## Repository organization

The assignment is divided into two driver tasks and a set of recorded validation logs.

| Directory | Contents | Purpose |
| --- | --- | --- |
| `task1_ioctl/` | IOCTL driver, shared header, userspace IOCTL utility, Makefile, test script, and kernel module | Implements and tests the IOCTL-based character driver |
| `task2_misc/` | Misc counter driver, Makefile, test script, and kernel module | Implements and tests the counter miscdevice |
| `logs/` | Task 1, Task 2, and kernel logs | Preserves evidence from the completed validation procedures |

## Task 1: IOCTL character device driver

### Driver interface

Task 1 is implemented in:

```text
task1_ioctl/asgn1_driver.c
```

The driver registers a character device named `asgn1` with major number 241 and minor number 0.

The userspace device node used by the test procedure is:

```text
/dev/asgn1
```

The driver maintains an internal 1024-byte buffer and implements the standard file operations:

| Operation | Function |
| --- | --- |
| Open | `asgn1_open` |
| Read | `asgn1_read` |
| Write | `asgn1_write` |
| Release | `asgn1_release` |
| IOCTL | `asgn1_ioctl` |

Data is transferred between userspace and kernel space using `copy_from_user` and `copy_to_user`.

A mutex protects shared driver state during operations that access or modify common data.

### Shared IOCTL definitions

The IOCTL command definitions are stored in:

```text
task1_ioctl/asgn1_ioctl.h
```

This header is shared by both the kernel module and the userspace IOCTL test program.

The driver defines four commands:

| Command | Direction | Function |
| --- | --- | --- |
| `ASGN1_RESET_BUFFER` | No argument | Clears the current buffer and resets read/write counters |
| `ASGN1_GET_STATS` | Kernel to userspace | Returns runtime driver statistics |
| `ASGN1_SET_MODE` | Userspace to kernel | Changes the current write mode |
| `ASGN1_GET_VERSION` | Kernel to userspace | Returns the driver version string |

All commands use the same IOCTL magic value.

### Driver statistics

The statistics structure returned by `ASGN1_GET_STATS` contains:

```c
struct asgn1_stats {
    int open_count;
    int write_count;
    int read_count;
    int buffer_len;
    int last_write_size;
};
```

These values allow the userspace test utility to inspect the internal state of the driver without accessing kernel memory directly.

### Driver operating modes

The driver supports two write modes.

Mode `1` operates as normal mode. Data written from userspace is stored directly in the internal buffer and can later be read through `/dev/asgn1`.

Mode `0` operates as kernel mode. In this mode, the driver adds the prefix:

```text
[KERNEL]
```

before storing the userspace data.

For example, writing:

```text
hello-kernel
```

in kernel mode produces:

```text
[KERNEL] hello-kernel
```

when the device is read.

### RESET_BUFFER behavior

The `ASGN1_RESET_BUFFER` command clears the device buffer and resets the following statistics:

```text
buffer_len
write_count
read_count
```

The open counter is not reset because issuing the IOCTL command itself requires opening the device.

The current implementation also preserves `last_write_size` when the buffer is reset.

This behavior is visible in the recorded Task 1 test results.

## Task 1 procfs interface

Task 1 creates:

```text
/proc/asgn1_info
```

The interface is implemented with the Linux `seq_file` API.

It reports information including:

```text
Driver name
Major number
Buffer size
Current data length
Open count
Last stored data
```

The procfs entry is read-only and can be inspected with:

```sh
cat /proc/asgn1_info
```

## Task 1 sysfs interface

The driver creates the class:

```text
/sys/class/asgn1_class/
```

and exposes three read-only attributes for the `asgn1` device:

```text
buffer_len
open_count
last_data
```

These attributes provide another method for examining the current driver state through the Linux device model.

## Task 1 userspace IOCTL utility

The source file:

```text
task1_ioctl/asgn1_ioctl_test.c
```

is an important part of Task 1.

A shell script cannot directly issue arbitrary Linux IOCTL requests. Therefore, this small userspace C program opens `/dev/asgn1` and calls the `ioctl()` system call using the commands defined in `asgn1_ioctl.h`.

The program supports:

```text
reset
stats
mode
version
```

Examples:

```sh
asgn1_ioctl_test version
asgn1_ioctl_test stats
asgn1_ioctl_test mode 0
asgn1_ioctl_test mode 1
asgn1_ioctl_test reset
```

The program was successfully cross-compiled for ARM with:

```bash
arm-linux-gnueabihf-gcc \
  -O2 -Wall \
  -o asgn1_ioctl_test \
  asgn1_ioctl_test.c
```

The resulting executable was verified as an ARM EABI5 userspace binary.

The target root filesystem used during validation contained the compiled utility at:

```text
/usr/bin/asgn1_ioctl_test
```

while this assignment directory preserves its source code.

## Building Task 1

The Task 1 Makefile uses the Linux kernel build system:

```make
KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

obj-m += asgn1_driver.o
```

The module can be rebuilt from `task1_ioctl` with:

```bash
make clean
make
```

The current source was successfully rebuilt and produced:

```text
asgn1_driver.ko: ELF 32-bit LSB relocatable, ARM, EABI5
```

## Testing Task 1

The automated test is provided in:

```text
task1_ioctl/test_asgn1.sh
```

The script performs the following validation sequence:

1. Loads `asgn1_driver.ko`.
2. Creates `/dev/asgn1` with major 241 if required.
3. Executes `GET_VERSION`.
4. Selects normal mode and verifies normal write/read behavior.
5. Selects kernel mode and verifies the `[KERNEL]` prefix.
6. Retrieves driver statistics with `GET_STATS`.
7. Clears the buffer with `RESET_BUFFER`.
8. Retrieves statistics again after reset.
9. Unloads the module.
10. Displays relevant kernel log messages.

The recorded result confirms successful operation of all four IOCTL commands.

Normal mode returned:

```text
hello-normal mode
```

Kernel mode returned:

```text
[KERNEL] hello-kernel
```

Before reset, the recorded statistics were:

```text
OPEN COUNT:        8
WRITE COUNT:       2
READ COUNT:        2
BUFFER LEN:        21
LAST WRITE SIZE:   12
```

After `RESET_BUFFER`, the buffer length, read count, and write count were zero while the device remained operational.

## Task 2: Misc counter device

### Miscdevice registration

Task 2 is implemented in:

```text
task2_misc/counter_driver.c
```

Instead of manually registering a fixed character-device major number, this task uses the Linux miscdevice framework.

The driver defines:

```c
static struct miscdevice counter_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "counter",
    .fops  = &counter_fops,
};
```

The miscdevice framework therefore allocates the minor number dynamically and exposes the device as:

```text
/dev/counter
```

### Counter behavior

The driver maintains three atomic integer values:

```text
counter
read_count
write_count
```

Using atomic variables allows these small shared counters to be updated safely without requiring an additional mutex.

### Read operation

Reading `/dev/counter` returns the current counter value.

After the value has been successfully copied to userspace, the driver increments both:

```text
counter
read_count
```

For example, three consecutive reads from the initial state produce:

```text
0
1
2
```

and leave the counter value equal to `3`.

The file position is also checked so that a single open/read sequence reaches end-of-file instead of allowing utilities such as `cat` to read indefinitely.

### Write operation

Writing a valid non-negative decimal integer sets the current counter value.

The driver converts userspace text to an integer using `kstrtoint`.

For example:

```sh
echo "100" > /dev/counter
```

sets the counter to 100.

The driver increments `write_count` after a successful write.

Negative numbers and text that cannot be converted into a valid integer are rejected with an error.

## Task 2 procfs interface

Task 2 creates:

```text
/proc/counter_info
```

The procfs entry reports:

```text
Counter value
Read count
Write count
```

It can be inspected with:

```sh
cat /proc/counter_info
```

The interface is implemented with the Linux `seq_file` API.

## Building Task 2

The Task 2 Makefile uses the same ARM kernel build environment:

```make
KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

obj-m += counter_driver.o
```

The module can be rebuilt from `task2_misc` with:

```bash
make clean
make
```

The current source was successfully rebuilt and produced:

```text
counter_driver.ko: ELF 32-bit LSB relocatable, ARM, EABI5
```

## Testing Task 2

The automated test is:

```text
task2_misc/test_counter.sh
```

The script loads the module and first checks the initial state:

```text
Counter value : 0
Read count    : 0
Write count   : 0
```

It then performs three reads. The recorded values were:

```text
0
1
2
```

After those reads, procfs reported:

```text
Counter value : 3
Read count    : 3
Write count   : 0
```

The script then writes the value `100`.

The next read returns:

```text
100
```

and the following procfs state confirms that the read incremented the counter:

```text
Counter value : 101
Read count    : 4
Write count   : 1
```

The script also resets the counter by writing `0`, verifies the next read, and tests invalid input.

Both of the following values were rejected correctly:

```text
-1
abc
```

The module was then unloaded successfully.

## Test logs

The assignment preserves three validation logs.

| Log file | Purpose |
| --- | --- |
| `logs/test_asgn1_output.txt` | Complete Task 1 IOCTL test output |
| `logs/test_counter_output.txt` | Complete Task 2 counter-device test output |
| `logs/dmesg_full.txt` | Kernel boot and driver messages recorded during assignment validation |

The Task 1 log confirms successful IOCTL operations, normal and kernel modes, statistics retrieval, buffer reset, and module unload.

The Task 2 log confirms automatic counter increments, counter updates through writes, procfs statistics, invalid-input rejection, and module unload.

## Principal files

| Path | Purpose |
| --- | --- |
| `task1_ioctl/asgn1_driver.c` | Task 1 kernel driver |
| `task1_ioctl/asgn1_ioctl.h` | Shared IOCTL definitions and statistics structure |
| `task1_ioctl/asgn1_ioctl_test.c` | Userspace program used to issue IOCTL requests |
| `task1_ioctl/Makefile` | Task 1 ARM kernel-module build configuration |
| `task1_ioctl/asgn1_driver.ko` | Compiled Task 1 kernel module |
| `task1_ioctl/test_asgn1.sh` | Task 1 validation script |
| `task2_misc/counter_driver.c` | Task 2 miscdevice driver |
| `task2_misc/Makefile` | Task 2 ARM kernel-module build configuration |
| `task2_misc/counter_driver.ko` | Compiled Task 2 kernel module |
| `task2_misc/test_counter.sh` | Task 2 validation script |
| `logs/` | Captured assignment test evidence |

## Reproducibility

Both kernel modules depend on the Linux 5.15 target kernel environment used in the previous laboratories.

The Makefiles expect the kernel source tree at:

```text
~/embedded_lab1/kernel/linux-5.15
```

A compatible ARM cross compiler must also be available through:

```text
arm-linux-gnueabihf-
```

The current source for both tasks was rebuilt successfully before this README was prepared.

The userspace IOCTL utility was also independently cross-compiled and verified as an ARM executable.

The recorded logs demonstrate the runtime behavior of both drivers on the target Embedded Linux environment.
