# Assignment 02 – Sensor Monitoring System

**Môn học:** Embedded Linux Systems  
**Nền tảng kiểm thử:** QEMU Vexpress-A9, ARMv7, Linux 5.15  
**Ngôn ngữ lập trình:** C – Linux Kernel Module, POSIX Threads, POSIX Message Queue  

## 1. Giới thiệu

Dự án xây dựng **hệ thống giám sát nhiệt độ và độ ẩm giả lập** trên Embedded Linux. Một kernel module tạo mẫu cảm biến; chương trình `sms_app` thu thập dữ liệu định kỳ, tính trung bình trượt, phát hiện nhiệt độ/độ ẩm vượt ngưỡng hoặc biến động nhiệt độ bất thường, phát cảnh báo và ghi log. Hệ thống được kiểm thử cả trên Ubuntu x86_64 và QEMU ARMv7.

## 2. Cấu trúc thư mục

```text
assignment02/
├── README.md                        # Hướng dẫn và kết quả thực nghiệm
├── diagrams/                        # Hai sơ đồ hệ thống
│   ├── system_architecture.png
│   └── application_state_machine.png
├── app/
│   ├── sms.h                        # Khai báo cấu trúc và giao diện chung
│   ├── sms_app.c                    # Main, CLI, tạo/join thread, thống kê
│   ├── sensor_thread.c              # Thu thập mẫu, lịch lấy mẫu, jitter
│   ├── processor_thread.c           # Trung bình trượt và phát hiện alert
│   ├── alert_thread.c               # Nhận cảnh báo từ POSIX MQ
│   ├── logger_thread.c              # Log queue, ghi file, log rotation
│   ├── circular_buffer.c            # Ring buffer dùng chung
│   ├── Makefile                     # Build ứng dụng ARM và x86
│   ├── sms_app                      # Binary ARM 
│   └── sms_app_x86                  # Binary x86 
├── driver/
│   ├── sms_sensor_driver.c          # Kernel driver mô phỏng cảm biến
│   ├── sms_sensor.h                 # Định nghĩa ioctl
│   ├── Makefile                     # Build module ARM
│   └── sms_sensor.ko                # Module sau khi build 
├── kernel/linux-5.15/              # Kernel tree riêng 
├── rootfs/                         # Rootfs QEMU 
├── output/
│   └── initramfs_asgn2.cpio.gz     # Initramfs sau khi đóng gói
├── logs/                            # Valgrind / GDB / strace / perf / jitter
└── Report/                         # Báo cáo DOCX 
```

> `kernel/`, `rootfs/`, `output/` có thể rất lớn; chỉ commit các thành phần được yêu cầu nộp hoặc đủ điều kiện phân phối. 

## 3. Thiết kế hệ thống

### 3.1. Sơ đồ kiến trúc

<img width="984" height="337" alt="system_architecture" src="https://github.com/user-attachments/assets/e043d4f8-6f46-4a31-a389-bcaefeb45cc2" />


*Hình 1. System Architecture, thể hiện Kernel Space, 4 worker threads và luồng dữ liệu/IPC.*

- **Kernel module `sms_sensor.ko`:** Cung cấp `/dev/sms_sensor` để `read()` mẫu cảm biến, `ioctl()` để cấu hình khoảng lấy mẫu, và `/proc/sms_stats` để xem thống kê.
- **Sensor Thread:** Lấy mẫu định kỳ qua `clock_nanosleep(... TIMER_ABSTIME ...)`, hỗ trợ `--mock`, đẩy dữ liệu vào **Circular Buffer**.
- **Processor Thread:** Nhận mẫu, tính trung bình 10 mẫu gần nhất, phát hiện `TEMP_HIGH`, `HUMID_HIGH`, `TEMP_SPIKE`.
- **Alert Thread:** Nhận thông báo qua **POSIX Message Queue** và ghi nhận cảnh báo.
- **Logger Thread:** Lấy dữ liệu từ **LogQueue** rồi ghi `/tmp/sms.log`.

**Lưu ý về hình:** `/proc/sms_stats` là giao diện đọc thống kê bởi người dùng/terminal, không phải một nguồn mẫu trực tiếp cho Sensor Thread. Luồng mẫu log chính là Processor Thread → LogQueue → Logger Thread.

### 3.2. State Machine
<img width="450" height="371" alt="application_state_machine" src="https://github.com/user-attachments/assets/9fdeb246-0f4e-4858-a572-dc08c4be283a" />


*Hình 2. Application State Machine: INIT → RUNNING → SHUTDOWN → EXIT, kèm nhánh lỗi.*

- **INIT:** Khởi tạo buffer, mutex, hàng đợi, signal handler và các thread.
- **RUNNING:** Thu thập, xử lý, cảnh báo và ghi log định kỳ.
- **SHUTDOWN:** Khi hết thời gian, nhận `SIGINT`/`SIGTERM` hoặc phải dừng vì lỗi: yêu cầu dừng các thread, đóng hàng đợi, `join`, flush log và giải phóng bộ nhớ.
- **ERROR/EXIT:** Nếu khởi tạo thất bại, dọn dẹp những tài nguyên đã tạo rồi thoát.

## 4. Môi trường và cách chạy

**Công cụ đã dùng:** QEMU ARM 10.2.1, GCC cross-compiler `arm-linux-gnueabihf-gcc` 15.2.0, Valgrind 3.26.0; kernel guest Linux 5.15.0.

### 4.1. Build chương trình

Trên **Ubuntu Host**, từ thư mục repository:

```bash
cd app
make host       # tạo sms_app_x86
make arm        # tạo sms_app cho ARMv7
```

Kernel driver cần build dựa trên **đúng cây kernel ARM 5.15 đã build cho QEMU** và Makefile cấu hình `KDIR` tương ứng. Trong quá trình thực hiện, cờ ARMv7-A được dùng để khắc phục lỗi assembler (`isb`, `dmb`).

### 4.2. Boot QEMU

Ví dụ theo đường dẫn của môi trường đã triển khai:

```bash
qemu-system-arm \
  -M vexpress-a9 -cpu cortex-a9 -m 512M \
  -kernel ~/assignment02/kernel/linux-5.15/arch/arm/boot/zImage \
  -dtb ~/assignment02/kernel/linux-5.15/arch/arm/boot/dts/vexpress-v2p-ca9.dtb \
  -initrd ~/assignment02/output/initramfs_asgn2.cpio.gz \
  -append "console=ttyAMA0 rdinit=/init" -nographic
```

Trong **QEMU** (`/ #`):

```sh
mkdir -p /dev/mqueue
mount -t mqueue none /dev/mqueue    # nếu chưa mount
insmod /lib/modules/5.15.0/sms_sensor.ko
cat /proc/sms_stats
/usr/bin/sms_app --other -i 500 -d 60 -v
cat /proc/sms_stats
tail -20 /tmp/sms.log
```

Chạy chế độ giả lập dữ liệu ngay tại userspace:

```sh
/usr/bin/sms_app --mock --other -i 500 -d 10 -v
```

Để kiểm thử dừng bằng SIGINT, chạy `-d 0` rồi nhấn **Ctrl+C**. Thoát QEMU `-nographic`: **Ctrl+A**, sau đó **X**.

> Ứng dụng ARM được liên kết động; initramfs phải có `/lib/ld-linux-armhf.so.3`, `libc.so.6`, `libm.so.6`. Kernel sử dụng cho QEMU phải bật `CONFIG_POSIX_MQUEUE=y`.

## 5. Kết quả kiểm thử thực tế

### Task 1 – Kernel Driver và multithreading

Trong phép thử đọc driver với khoảng mẫu **500 ms**, thời gian **60 giây**:

| Chỉ số | Kết quả |
|---|---:|
| Mẫu tạo / mẫu xử lý | **117 / 117** |
| Buffer overflow | **0** |
| Cảnh báo nhiệt độ cao (`TEMP_HIGH`) | 15 |
| Cảnh báo độ ẩm cao (`HUMID_HIGH`) | 22 |
| Cảnh báo biến thiên nhiệt độ (`TEMP_SPIKE`) | 76 |
| Log dropped | **0** |

Số mẫu thực tế là **117** so với khoảng **120 mẫu** dự kiến; sai lệch 3 mẫu (2,5%). File `/tmp/sms.log` đã ghi được các dòng SAMPLE và ALERT. Trong quá trình phát triển, lỗi bỏ mẫu do `EAGAIN` đã được cải thiện bằng retry ngắn ở Sensor Thread.

### Task 2 – Memory Management

Trên Host, Valgrind kiểm tra `sms_app_x86 --mock --other -i 500 -d 35`:

```text
HEAP SUMMARY:
  in use at exit: 0 bytes in 0 blocks
  total heap usage: 21 allocs, 21 frees, 15,216 bytes allocated
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts
```

Trên QEMU, memory footprint trong phép thử 70 giây:

| Thời điểm | VmRSS | VmSize |
|---|---:|---:|
| 5 giây | 34.128 kB | 37.036 kB |
| 30 giây | 34.128 kB | 37.036 kB |
| 55 giây | 34.128 kB | 37.036 kB |

Không quan sát thấy tăng RSS/virtual memory tại ba mốc trên. Điều này không tự nó chứng minh không có leak, nhưng phù hợp với kết quả Valgrind ở lần thử đã thực hiện.

### Task 3 – GDB, strace và perf

- **GDB:** Đặt breakpoint tại `ring_pop()`, kiểm tra các biến buffer `head`, `tail`, `size`, `produced`, `consumed` và danh sách/call stack các thread.
- **strace:** Đã lưu thống kê `strace -f -c`; từ ảnh chụp, các syscall nổi bật gồm `clock_nanosleep`, `futex` và `mq_timedreceive`.
- **perf:** Đã tạo các file `perf_sms.data` và `perf_sms.txt`. **Chưa đưa ra kết luận về CPU hotspot** vì cần đối chiếu đầy đủ nội dung `perf_sms.txt`.

### Task 4 – Real-Time Scheduling

Đã xác minh một Sensor Thread có **SCHED_FIFO, RT priority 50** (`policy=1`, `rt_priority=50`). Trong phép kiểm tra khóa bộ nhớ: `VmLck = 37.088 kB`.

| Chỉ số | SCHED_OTHER | SCHED_FIFO |
|---|---:|---:|
| Jitter samples | **1.039** | **1.040** |
| Min (µs) | 427 | 360 |
| Avg (µs) | 5.032,42 | 5.054,56 |
| Max (µs) | 9.811 | 9.997 |
| P99 (µs) | **9.554** | **9.647** |
| Histogram `<100 / 100–500 / 500–1000 / ≥1000 µs` | 0 / 3 / 64 / 972 | 0 / 6 / 50 / 984 |

P99 của FIFO **cao hơn 93 µs (~0,97%)** trong phép đo này; không quan sát thấy cải thiện P99. Việc chạy qua QEMU có thể gây ảnh hưởng bởi lập lịch CPU của Ubuntu Host. Không suy rộng một phép đo thành kết luận chung cho SCHED_FIFO.

## 6. Hạn chế và hướng phát triển

- Cần rà soát kỹ việc đồng bộ biến dừng giữa các thread (`app->stop`, `sensor_error`) và chứng minh không có race/deadlock trong tất cả đường chạy.
- Cần đọc kết quả perf thực tế để bổ sung phân tích hotspot và đề xuất tối ưu có căn cứ.
- Nên kiểm thử log rotation khi tệp log vượt 1 MB và thực hiện thêm các phép đo jitter độc lập.
- Có thể nghiên cứu PREEMPT_RT, cấu hình thread affinity và giảm tác động lịch của Host khi đo trên QEMU.

## 7. Báo cáo và minh chứng

Báo cáo Assignment 02 bao gồm kiến trúc, giải thích quản lý thread và IPC, bộ nhớ, debug/profiling, jitter và bài học rút ra. Video demo theo đề dài **3–5 phút**, trình bày boot QEMU, nạp module, chạy `sms_app -v`, cảnh báo, log và Ctrl+C shutdown.

Các kết quả ở README dựa trên output Terminal và hình ảnh đã ghi nhận trong quá trình làm Assignment 02; phần chưa kiểm chứng được đánh dấu rõ. **Không sử dụng README này để thay thế báo cáo PDF chính thức.**
