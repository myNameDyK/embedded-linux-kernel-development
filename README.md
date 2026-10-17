# Embedded Linux Systems — LAB-01 & LAB-02

**Sinh viên:** Phạm Võ Hoàng Nhân · **MSSV:** SE200778 · **Lớp:** IC2008  
**Giảng viên:** Phạm Thế Vinh  
**Nền tảng:** ARMv7 Cortex-A9, QEMU `vexpress-a9`

Hai bài thực hành nối tiếp nhau: **LAB-01 xây dựng hệ thống Linux tối giản và khởi động đến BusyBox shell**; **LAB-02 sử dụng kernel và môi trường đó để phát triển character device driver, xuất thông tin qua procfs/sysfs và thực hành filesystem trên flash giả lập**.

## Báo cáo và sơ đồ

| Bài thực hành | Nội dung chính | Báo cáo chi tiết | Sơ đồ PDF |
| --- | --- | --- | --- |
| LAB-01 | Build kernel, U-Boot, BusyBox và initramfs; quan sát chuỗi khởi động | [Báo cáo LAB-01](SE200778_PhamVoHoangNhan_Lab01/SE200778_PhamVoHoangNhan_Lab01_BaoCao.pdf) | [Chuỗi khởi động](SE200778_PhamVoHoangNhan_Lab01/diagrams/Embedded_Linux_Boot.drawio.pdf) |
| LAB-02 | Character device driver, procfs/sysfs, MTD/JFFS2 và tự nạp module khi boot | [Báo cáo LAB-02](SE200778_PhamVoHoangNhan_Lab02/SE200778_PhamVoHoangNhan_Lab02_BaoCao.pdf) | [Kiến trúc driver](SE200778_PhamVoHoangNhan_Lab02/diagrams/Lab2_Character_Driver.drawio.pdf) |

## Môi trường thực hành

| Thành phần | Cấu hình được ghi nhận trong báo cáo |
| --- | --- |
| Host | Ubuntu 26.04 |
| Toolchain | `arm-linux-gnueabihf`, GCC 15.2 |
| Target | QEMU `vexpress-a9`, Cortex-A9, ARMv7-A |
| Linux kernel | 5.15.0, `vexpress_defconfig`, hỗ trợ SMP |
| U-Boot | 2022.04 |
| BusyBox | 1.35.0, build static |
| Môi trường LAB-02 | Phần A, B, D chạy trong QEMU; phần C (MTD/JFFS2) chạy trên host |

## LAB-01 — Cấu hình kernel và khởi động hệ thống nhúng

LAB-01 xây dựng các thành phần nền tảng: kernel `zImage`, Device Tree `vexpress-v2p-ca9.dtb`, BusyBox và root filesystem đóng gói thành `initramfs.cpio.gz`.

![Sơ đồ chuỗi khởi động Embedded Linux](SE200778_PhamVoHoangNhan_Lab01/diagrams/Embedded_Linux_Boot.drawio.pdf)


Nội dung chính:

- Cấu hình và build kernel cho `vexpress-a9`, giữ UART PL011 để có console `ttyAMA0` và bật SMP.
- Khảo sát biến môi trường, địa chỉ nạp và `bootargs` trong U-Boot.
- Quan sát kernel panic khi chưa có root filesystem, sau đó bổ sung initramfs để boot đến shell.
- Kiểm tra hệ thống bằng `uname -a`, `mount` và `ps`; xác nhận BusyBox init chạy với PID 1.
- Phân tích toolchain, memory map, thanh ghi MIDR, static linking và SMP; ghi lại các lỗi tương thích khi build mã nguồn cũ bằng GCC mới.

**Lưu ý khi đọc sơ đồ:** ROM Bootloader thể hiện giai đoạn khởi động tổng quát trên board thật. Trong thí nghiệm QEMU, báo cáo mô tả cả cách khởi chạy U-Boot bằng `-kernel u-boot` và cách nạp trực tiếp kernel bằng `-kernel zImage -dtb ... -initrd ...`.

## LAB-02 — Device driver và quản lý filesystem

LAB-02 tiếp tục dùng kernel 5.15.0 từ LAB-01. Module `lab2_driver.ko` cung cấp character device `/dev/lab2`, nhận dữ liệu từ user space và trả lại dữ liệu qua các hàm trong bảng `file_operations`.

![Sơ đồ kiến trúc character device driver LAB-02](SE200778_PhamVoHoangNhan_Lab02/diagrams/Lab2_Character_Driver.drawio.pdf)


| Phần | Nội dung | Kết quả được trình bày trong báo cáo |
| --- | --- | --- |
| A — Character device | Build module ARM, đăng ký major 240, xử lý `open/read/write/release` | Kiểm tra `file`, `modinfo`, `lsmod`, ghi/đọc `/dev/lab2`, chạy `test_driver.sh` và xem `dmesg` |
| B — procfs và sysfs | Công bố trạng thái buffer và bộ đếm mở thiết bị | Đọc `/proc/lab2_info` và các thuộc tính `buffer_len`, `open_count`, `last_data` trong `/sys/class/lab2_class/lab2/` |
| C — MTD và JFFS2 | Giả lập NAND bằng `nandsim` trên host | Khảo sát MTD, tạo và ghi image JFFS2, mount qua `mtdblock1`, đọc file và xem dump thô |
| D — BusyBox init | Tích hợp module vào rootfs và nạp bằng `rcS` | Boot log cho thấy module được nạp và `/dev/lab2` được tạo tự động |

**Đặc điểm của bản driver trong báo cáo:** `open_count` đếm tổng số lần mở thiết bị. Đường ghi dùng `lab2_mutex`; báo cáo cũng chỉ ra đường đọc và các hàm sysfs còn cần đồng bộ thêm khi có truy cập đồng thời.

**Thiết bị flash trong thí nghiệm:** trên host của báo cáo, NAND giả lập là `mtd1`; `mtd0` mang tên `BIOS` là flash firmware thật. Chỉ số thiết bị cần được xác định theo môi trường thực tế.

## Mối liên hệ giữa hai bài

| Thành phần từ LAB-01 | Vai trò trong LAB-02 |
| --- | --- |
| Kernel 5.15.0 và cấu hình ARM | Là kernel đích để build và nạp `lab2_driver.ko`; vermagic của module phải phù hợp |
| BusyBox và initramfs | Cung cấp shell, các lệnh kiểm thử và nơi đặt module/script trong rootfs |
| `/etc/inittab` và `/etc/init.d/rcS` | Được mở rộng để mount filesystem, nạp module và tạo device node khi boot |
| Console QEMU | Dùng theo dõi boot log, kiểm tra driver và đọc procfs/sysfs |

Đọc **LAB-01 trước** để nắm chuỗi khởi động và môi trường chạy, sau đó đọc **LAB-02** để theo dõi cách driver hoạt động trong hệ thống đó. Ảnh chụp kết quả, câu trả lời phân tích và các khó khăn thực tế nằm trong từng báo cáo PDF.

## Thư mục tài liệu của sinh viên

| Đường dẫn | Nội dung |
| --- | --- |
| [SE200778_PhamVoHoangNhan_Lab01/](SE200778_PhamVoHoangNhan_Lab01/) | Báo cáo LAB-01 và thư mục `diagrams/` chứa sơ đồ PDF, ảnh PNG |
| [SE200778_PhamVoHoangNhan_Lab02/](SE200778_PhamVoHoangNhan_Lab02/) | Báo cáo LAB-02 và thư mục `diagrams/` chứa sơ đồ PDF, ảnh PNG |

Các bài thực hành đã có trong repository: [Nguyễn Duy Khoa — LAB-01](SE200494_NguyenDuyKhoa_Lab01/), [LAB-02](SE200494_NguyenDuyKhoa_Lab02/), [Assignment 01](SE200494_NguyenDuyKhoa_Asgn01/).
