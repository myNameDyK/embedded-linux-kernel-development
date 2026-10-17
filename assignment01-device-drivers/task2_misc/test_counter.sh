#!/bin/sh

KO="/lib/modules/5.15.0/counter_driver.ko"
DEV="/dev/counter"

echo "===== ASSIGNMENT 01 - TASK 2 TEST ====="

insmod "$KO" || exit 1      




echo "[1] INITIAL STATS"
cat /proc/counter_info


# ==test read /auto increment
echo "[2] THREE READS - expected: 0, 1, 2"

cat "$DEV"
cat "$DEV"
cat "$DEV"

echo "[3] STATS AFTER THREE READS"
cat /proc/counter_info

# ===== test set counter
echo "[4] SET COUNTER = 100"

echo "100" > "$DEV"
echo "next read:"
cat "$DEV"

cat /proc/counter_info


# ===Test reset counter 
echo "[5] RESET COUNTER = 0"

echo "0" > "$DEV"
echo "next read:"
cat "$DEV"

cat /proc/counter_info


# ====== test so am
echo "[6] INVALID NEGATIVE VALUE"

if echo "-1" > "$DEV" 2>/dev/null; then
    echo "ERROR: negative value was accepted"
else
    echo "negative value rejected correctly"
fi


# ===== Test kieu text ko hop le
echo "[7] INVALID TEXT VALUE"

if echo "abc" > "$DEV" 2>/dev/null; then
    echo "ERROR: text value was accepted"
else
    echo "text value rejected correctly"
fi


# ===== Unload =====
echo "[8] UNLOAD MODULE"

rmmod counter_driver || exit 1


# ===== Kernel log =====
echo "[9] DMESG"

dmesg | grep counter_driver

echo "===== TASK 2 FINISHED ====="