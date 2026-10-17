LAB-03 Embedded Linux Systems
Sinh vien: Nguyen Cao Ky
MSSV: SE201611

apps/: C source + shell scripts + ARM and x86_64 binaries
output/initramfs_lab3.cpio.gz: QEMU ARMv7 rootfs
logs/: GDB, Valgrind, strace, perf, cyclictest, histogram evidence
bao_cao/SE201611_Lab03_BaoCao.pdf: Bao cao 7 muc + phu luc GDB

Chu y:
- perf_report/perf_stat: ban profiling ITERATIONS=3000.
- strace x86_64 va ARM: ban goc ITERATIONS=50.
- PREEMPT_RT chi so sanh ly thuyet; khong chay kernel RT thuc te.
