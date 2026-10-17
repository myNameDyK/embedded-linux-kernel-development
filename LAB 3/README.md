# LAB-03 — EMBEDDED LINUX SYSTEMS
**Remote Debugging | Memory Analysis | Performance Profiling | Real-Time Latency**

**Sinh viên:** Nguyễn Cao Kỳ  
**MSSV:** SE201611  
**Ngày thực hiện:** 17/10/2026  
**Môn học:** Embedded Linux Systems  
**Chủ đề:** Debugging Kernel & Phân tích Hiệu năng Ứng dụng Nhúng

---

## 1. Tổng quan dự án

LAB-03 tập trung vào việc xây dựng và kiểm thử một môi trường Embedded Linux nhằm phát hiện lỗi chương trình, phân tích sử dụng bộ nhớ, đánh giá hiệu năng và đo độ trễ đáp ứng của hệ điều hành.

Hệ thống được triển khai trên hai môi trường:

- **Host:** AlmaLinux 9.8, kiến trúc x86_64.
- **Target:** Linux ARMv7-A, giả lập bằng QEMU với bo mạch `vexpress-a9` và CPU Cortex-A9.

Các ứng dụng được phát triển bằng ngôn ngữ C, biên dịch cho cả x86_64 và ARM tùy mục đích kiểm thử.

Dự án gồm bốn nhóm chức năng chính:

1. **Remote Debugging:** Sử dụng GDB và gdbserver để phát hiện lỗi trong chương trình ARM.
2. **Memory Analysis:** Dùng Valgrind phát hiện và khắc phục memory leak.
3. **Performance Profiling:** Phân tích system call, CPU hotspot và hardware performance counters.
4. **Real-Time Analysis:** Đo scheduling latency của Linux trong điều kiện bình thường và khi hệ thống chịu tải.

Toàn bộ kết quả được lưu thành source code, executable, log, biểu đồ và báo cáo PDF nhằm phục vụ việc kiểm tra, đánh giá và tái hiện thí nghiệm.

---

## 2. Kiến trúc hệ thống

### 2.1. Mô hình Host–Target

Hệ thống được tổ chức theo kiến trúc Host–Target.

```text
                 ALMALINUX HOST (x86_64)
+------------------------------------------------------+
|                                                      |
|  Source Code C                                       |
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

### 2.2. Vai trò của từng môi trường

**AlmaLinux Host**

Host đảm nhiệm việc phát triển, biên dịch, điều khiển quá trình debugging và phân tích hiệu năng.

Các công cụ chính:

- `gcc`: Biên dịch ứng dụng x86_64.
- ARM cross-compiler: Biên dịch chương trình cho ARM.
- `gdb`: Debug ứng dụng ARM từ xa.
- `valgrind`: Phát hiện memory leak.
- `strace`: Thống kê lời gọi hệ thống.
- `perf`: Phân tích hiệu năng CPU.
- `cyclictest`: Đo scheduling latency.
- `stress-ng`: Tạo tải CPU, I/O và bộ nhớ.
- `gnuplot`: Tạo biểu đồ latency.

**QEMU ARM Target**

Target giả lập môi trường Embedded Linux ARM, chạy các executable ARM và cung cấp thông tin về hoạt động của chương trình.

Cấu hình sử dụng:

| Thành phần | Cấu hình |
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

Trong phần Remote Debugging, GDB trên Host giao tiếp với gdbserver qua UART ảo được chuyển tiếp sang TCP `127.0.0.1:1234`.

Phần Real-Time Latency được đo trực tiếp trên Host để tránh sai lệch do QEMU mô phỏng.

---

## 3. Cấu trúc thư mục dự án

```text
SE201611_NguyenCaoKy_Lab03/
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
|-- bao_cao/
    |
    |-- SE201611_Lab03_BaoCao.pdf
```

### 3.1. Thư mục `apps/`

Chứa toàn bộ mã nguồn và các chương trình đã biên dịch.

| File | Chức năng |
|---|---|
| `buggy_app.c` | Chương trình chứa ba lỗi cố ý để thực hành GDB |
| `buggy_app` | Executable ARM phục vụ Remote Debugging |
| `leaky_app.c` | Source gốc chứa memory leak |
| `leaky_app_buggy.c` | Bản lưu source trước khi sửa |
| `leaky_app` | Executable x86_64 chưa sửa |
| `leaky_app_fixed.c` | Source đã bổ sung giải phóng bộ nhớ |
| `leaky_app_fixed` | Executable x86_64 đã sửa |
| `leaky_app_arm` | Executable ARM của chương trình gốc |
| `leaky_app_arm_probe.c` | Source có checkpoint để đo bộ nhớ ARM |
| `leaky_app_arm_probe` | Executable ARM có checkpoint |
| `measure_leaky_arm.sh` | Script thu thập VmRSS, VmSize và memory map |
| `perf_target.c` | Chương trình tạo workload CPU và I/O |
| `perf_target` | Executable x86_64 gốc |
| `perf_target_profile.c` | Phiên bản tăng thời gian tính toán để profiling |
| `perf_target_profile` | Executable sử dụng trong perf |
| `perf_target_arm` | Executable ARM phục vụ so sánh system call |

### 3.2. Thư mục `output/`

Chứa image root filesystem của hệ thống ARM.

**`initramfs_lab3.cpio.gz`**

Đây là filesystem được nạp khi QEMU khởi động, chứa BusyBox, các chương trình thực hành và công cụ debug dành cho ARM.

Image được sử dụng cùng Linux kernel và Device Tree đã xây dựng từ LAB-01.

### 3.3. Thư mục `logs/`

Chứa toàn bộ dữ liệu chứng minh kết quả thực nghiệm.

Nhóm GDB lưu breakpoint, backtrace, thanh ghi và giá trị biến.

Nhóm Valgrind lưu kết quả phát hiện lỗi cấp phát bộ nhớ trước và sau khi sửa.

Nhóm strace/perf lưu các system call, CPU hotspot và performance counters.

Nhóm cyclictest/stress-ng lưu độ trễ lập lịch, histogram và các điều kiện đo.

Các file log được giữ để có thể truy vết kết quả trong báo cáo về dữ liệu gốc.

### 3.4. Thư mục `bao_cao/`

Chứa báo cáo PDF chính thức:

`SE201611_Lab03_BaoCao.pdf`

Báo cáo trình bày phương pháp, kết quả, phân tích kỹ thuật và các câu hỏi của đề LAB-03.

---

## 4. Nội dung triển khai

### 4.1. Part A — Remote Debugging với GDB

**Mục tiêu**

Thiết lập kết nối debugging từ Host x86_64 đến chương trình chạy trên ARM QEMU, từ đó xác định nguyên nhân của ba lỗi C.

**Công cụ**

- GDB trên Host.
- gdbserver trên ARM.
- QEMU ARMv7.
- UART-to-TCP forwarding.

**Các lỗi được kiểm tra**

| Lỗi | Nguyên nhân | Bằng chứng |
|---|---|---|
| Array Out-of-Bounds | Vòng lặp dùng `i <= len` | `i = 5`, `len = 5` |
| Use-After-Free | Sử dụng con trỏ sau `free()` | Con trỏ `0x8dda8` vẫn được trả về |
| Integer Overflow | Phép nhân vượt phạm vi signed 32-bit integer | Giá trị quan sát `1.410.065.408` |

Trong lỗi integer overflow, phép tính:

`100000 × 100000 = 10000000000`

vượt quá phạm vi biểu diễn của kiểu `int` 32-bit.

Quá trình debugging sử dụng breakpoint, `print`, `backtrace` và `info registers`.

**Kết quả**

Xác định được ba lỗi và vị trí liên quan trong source. Các lỗi được phân tích tại breakpoint; không kết luận rằng cả ba đều gây crash.

**Bằng chứng:** `logs/gdb_session.txt`.

---

### 4.2. Part B — Memory Analysis với Valgrind

**Mục tiêu**

Phát hiện memory leak, sửa source code và xác nhận chương trình không còn leak khi kết thúc.

**Quy trình**

1. Chạy `leaky_app` chưa sửa bằng Valgrind.
2. Phân tích allocation stack trace.
3. Xác định các vùng cấp phát chưa được giải phóng.
4. Bổ sung `free()` vào vị trí phù hợp.
5. Chạy Valgrind lại trên executable đã sửa.
6. Cross-compile và quan sát bộ nhớ tiến trình ARM.

**Kết quả Valgrind**

| Metric | Trước sửa | Sau sửa |
|---|---:|---:|
| Definitely lost | 16.900 bytes | 0 bytes |
| Lost blocks | 250 | 0 |
| Allocations | 352 | 352 |
| Frees | 102 | 352 |
| Error contexts | 3 | 0 |

Các vùng leak được xác định gồm:

- Bộ nhớ lưu tên: 2.100 bytes.
- Bộ nhớ lưu điểm số: 2.000 bytes.
- Bộ nhớ xử lý messages: 12.800 bytes.

Tổng memory leak được xác nhận:

**16.900 bytes**.

Sau khi sửa, Valgrind ghi nhận:

`ERROR SUMMARY: 0 errors from 0 contexts`

**Phân tích bộ nhớ trên ARM**

Chương trình `leaky_app_arm_probe` được bổ sung hai checkpoint `SIGSTOP` để đọc bộ nhớ tiến trình trước và sau xử lý.

| Metric | Trước xử lý | Sau xử lý |
|---|---:|---:|
| VmSize | 776 KB | 776 KB |
| VmRSS | 4 KB | 4 KB |
| VmData | 152 KB | 152 KB |

Việc VmRSS không thay đổi trong hai snapshot không chứng minh chương trình không bị leak, vì bộ nhớ có thể được cấp phát trong vùng heap đã tồn tại.

Bằng chứng xác định và khắc phục memory leak được lấy từ Valgrind.

**Bằng chứng:** `valgrind_report.txt`, `valgrind_fixed.txt`, `b3_qemu_console.log`.

---

### 4.3. Part C — Performance Profiling

**Mục tiêu**

Đánh giá hiệu năng ứng dụng ở hai góc độ:

- System calls và chi phí tương tác với kernel.
- CPU hotspots và mức sử dụng tài nguyên xử lý.

**Workload**

Chương trình `perf_target` tạo ra ba nhóm tác vụ:

1. CPU-intensive computation thông qua `sqrt()` và `log()`.
2. Linear search trên mảng dữ liệu.
3. File I/O bằng cách ghi 10.000 dòng.

**Kết quả strace trên x86_64**

| System call | Calls | % syscall time |
|---|---:|---:|
| write | 70 | 30,10% |
| execve | 1 | 20,23% |
| munmap | 3 | 12,85% |
| close | 4 | 12,35% |
| mmap | 14 | 9,45% |

Tổng số system call: **122**.

Tổng thời gian syscall được đo: **1,206 ms**.

**Kết quả perf**

Để tăng số lượng mẫu CPU, phiên bản `perf_target_profile` sử dụng 3.000 vòng tính toán thay vì 50 vòng trong chương trình gốc.

| Metric | Kết quả |
|---|---:|
| CPU samples | 395 |
| Lost samples | 0 |
| heavy_compute overhead | 51,39% |
| Elapsed time | 1,9806 giây |
| CPU cycles | 4.841.444.106 |
| Instructions | 21.608.569.672 |
| IPC | 4,46 |
| Cache misses | 488.237 |
| Branch misses | 13.534 |

`heavy_compute()` là CPU hotspot lớn nhất được xác định trong phép đo.

Ngoài ra, nhiều mẫu CPU xuất hiện trong `libm.so.6`, phù hợp với workload sử dụng các phép toán `sqrt()` và `log()`.

**Profiling trên ARM QEMU**

| Metric | Kết quả |
|---|---:|
| Architecture | ARMv7 |
| Execution time | 33,68 giây |
| Total system calls | 100 |
| Total syscall time | 17,364 ms |

Các phép đo x86_64 và ARM sử dụng kiến trúc cùng môi trường thực thi khác nhau. QEMU cũng tạo thêm overhead mô phỏng, nên không thể xem thời gian trên ARM QEMU là hiệu năng của CPU Cortex-A9 vật lý.

Bản profiling 3.000 vòng cũng không được dùng để so sánh trực tiếp với bản ARM 50 vòng.

**Bằng chứng:** `strace_summary.txt`, `perf_report.txt`, `perf_stat.txt`, `c3_qemu_console.log`.

---

### 4.4. Part D — Real-Time Latency Analysis

**Mục tiêu**

Đánh giá độ trễ đánh thức và lập lịch thread của Linux trong điều kiện bình thường và khi hệ thống chịu tải.

Phép đo được thực hiện trên Host AlmaLinux, không chạy trong QEMU.

**Cấu hình cyclictest**

| Parameter | Value |
|---|---|
| Kernel | Linux 5.14 PREEMPT_DYNAMIC |
| Threads | 4 |
| Scheduling policy | SCHED_FIFO |
| Priority | 80 |
| Interval | 1000 µs |
| Loops | 10.000 mỗi thread |
| Memory locking | Enabled |
| Histogram limit | 5000 µs |

**Kết quả Baseline**

| Thread | Min (µs) | Avg (µs) | Max (µs) |
|---|---:|---:|---:|
| 0 | 7 | 14 | 121 |
| 1 | 6 | 14 | 124 |
| 2 | 8 | 15 | 142 |
| 3 | 7 | 13 | 188 |

Max latency lớn nhất: **188 µs**.

Không ghi nhận mẫu nào vượt 500 µs trong 40.000 mẫu.

**Kết quả khi có tải stress-ng**

Workload gồm 16 CPU workers, 4 I/O workers và 2 memory workers.

| Thread | Min (µs) | Avg (µs) | Max (µs) |
|---|---:|---:|---:|
| 0 | 6 | 12 | 2.042 |
| 1 | 4 | 10 | 1.121 |
| 2 | 4 | 10 | 964 |
| 3 | 6 | 19 | 3.919 |

Max latency lớn nhất: **3.919 µs**.

Histogram ghi nhận **42 mẫu vượt 500 µs** trên tổng số 39.968 mẫu được thống kê.

**So sánh**

| Metric | Baseline | Under Stress |
|---|---:|---:|
| Maximum latency | 188 µs | 3.919 µs |
| Samples over 500 µs | 0 | 42 |
| Deadline 500 µs | Đạt trong phép đo | Không đạt trong phép đo |

**Đánh giá**

Kết quả cho thấy độ trễ trung bình thấp không đồng nghĩa hệ thống luôn đáp ứng deadline.

Khi có tải, các latency spike có thể làm hệ thống phản hồi muộn hơn thời điểm yêu cầu.

Đối với một hệ thống điều khiển robot có deadline nghiêm ngặt dưới 500 µs, cần cân nhắc PREEMPT_RT cùng các biện pháp cấu hình và kiểm thử bổ sung.

Dự án chỉ đo trực tiếp trên kernel `PREEMPT_DYNAMIC`. PREEMPT_RT được thảo luận về mặt lý thuyết và chưa có kết quả thực nghiệm riêng.

**Biểu đồ:** `logs/latency_histogram.png`.

---

## 5. Hướng dẫn chạy lại

Các ví dụ dưới đây giả định người dùng đang đứng trong thư mục gốc của bài LAB-03 sau khi giải nén.

### 5.1. Chạy Memory Leak Test

```bash
./apps/leaky_app

valgrind --leak-check=full \
  --show-leak-kinds=all \
  ./apps/leaky_app
```

Sau khi sửa:

```bash
valgrind --leak-check=full \
  --show-leak-kinds=all \
  ./apps/leaky_app_fixed
```

### 5.2. Chạy strace trên Host

```bash
strace -c ./apps/perf_target
```

### 5.3. Chạy perf record/report

```bash
perf record -g \
  --call-graph fp \
  -F 199 \
  -e cpu-clock:u \
  -- ./apps/perf_target_profile
```

Xem kết quả:

```bash
perf report --stdio
```

### 5.4. Chạy perf stat

```bash
perf stat \
  -e task-clock,cycles:u,instructions:u,cache-misses:u,branches:u,branch-misses:u \
  -- ./apps/perf_target_profile
```

### 5.5. Khởi động ARM QEMU

Cần chuẩn bị kernel và DTB từ LAB-01.

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

Trong ARM QEMU:

```bash
/usr/bin/measure_leaky_arm.sh
```

Phân tích system call:

```bash
/usr/bin/strace -c /usr/bin/perf_target_arm
```

### 5.6. Chạy cyclictest trên Host

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

Lưu ý: Không chạy lệnh này trong ARM QEMU. Kết quả đo lại có thể khác kết quả gốc tùy trạng thái hệ thống và tải CPU.

---

## 6. Các vấn đề kỹ thuật và giới hạn

### 6.1. GDB Remote Connection

QEMU sử dụng không hỗ trợ SLIRP user networking. Vì vậy, kết nối GDB được thực hiện bằng UART-to-TCP thay cho mạng IP bên trong máy ảo.

### 6.2. Memory Leak Detection

VmRSS không phải công cụ xác định memory leak trực tiếp. Valgrind được sử dụng để xác minh các allocation không được giải phóng.

### 6.3. CPU Profiling

`perf` thu thập các mẫu CPU theo phương pháp sampling. Phần trăm overhead không đồng nghĩa tuyệt đối với phần trăm tổng thời gian thực thi ứng dụng.

Phiên bản profiling kéo dài workload nhằm tăng độ tin cậy của kết quả thống kê.

### 6.4. ARM Performance

QEMU mô phỏng hệ thống ARM nên kết quả thời gian chịu ảnh hưởng từ overhead của trình giả lập và cấu hình Host.

Để đánh giá hiệu năng Cortex-A9 thực tế, cần chạy thêm trên phần cứng ARM tương ứng.

### 6.5. Real-Time Constraints

Kernel được kiểm thử là Linux `PREEMPT_DYNAMIC`, không phải PREEMPT_RT.

Phép đo với số vòng giới hạn không đủ để bảo đảm deadline trong mọi điều kiện hoạt động.

Histogram stress cũng chỉ ghi nhận 39.968 mẫu, được giữ nguyên theo dữ liệu thực tế.

---

## 7. Tài liệu và kết quả nộp bài

**Báo cáo chính thức**

`bao_cao/SE201611_Lab03_BaoCao.pdf`

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

## 8. Kết luận

LAB-03 đã triển khai và kiểm thử các công cụ quan trọng trong quá trình phát triển và phân tích ứng dụng Embedded Linux.

Các kết quả chính bao gồm:

- Xác định ba lỗi lập trình C bằng GDB Remote Debugging.
- Phát hiện 16.900 bytes memory leak và sửa để Valgrind ghi nhận 0 errors.
- Xác định `heavy_compute()` là CPU hotspot lớn nhất trong workload profiling.
- Thu thập và phân tích system calls trên cả x86_64 và ARM QEMU.
- Đo scheduling latency trên Host và phát hiện các latency spike vượt deadline 500 µs khi hệ thống chịu tải.
- Tạo đầy đủ log, histogram và báo cáo PDF để phục vụ đánh giá.

Dự án giúp làm rõ sự khác biệt giữa lỗi logic chương trình, lỗi quản lý bộ nhớ, giới hạn hiệu năng CPU và độ trễ lập lịch trong hệ thống Embedded Linux.

---

**Nguyễn Cao Kỳ — SE201611**  
**Embedded Linux Systems — LAB-03**  
**17/10/2026**
