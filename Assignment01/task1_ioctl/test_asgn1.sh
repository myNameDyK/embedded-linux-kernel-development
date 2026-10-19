#!/bin/sh

KO="/lib/modules/5.15.0/asgn1_driver.ko"
DEV="/dev/asgn1"
TOOL="/usr/bin/asgn1_ioctl_test"

echo "===== ASSIGNMENT 01 - TASK 1 TEST ====="

insmod "$KO" || exit 1

if [ ! -e "$DEV" ]; then                # "-e" tuong duong la ton tai duong dan, ! -e la chua ton tai
    mknod "$DEV" c 241 0 || exit 1
fi

echo "[1] GET_VERSION"
"$TOOL" version || exit 1








#========== test set mode 1 = nomarl mode
echo "[2] ECHO MODE"
"$TOOL" mode 1 || exit 1

echo -n "hello-normal mode" > "$DEV"
RESULT="$(cat "$DEV")"

echo "read = $RESULT"

#==========test set mode 0  kernel mode 
echo "[3] KERNEL MODE"
"$TOOL" mode 0 || exit 1

echo -n "hello-kernel" > "$DEV"
RESULT="$(cat "$DEV")"

echo "read = [$RESULT]"

#============== test get stats
echo "[4] GET_STATS"
"$TOOL" stats || exit 1


#======test reset buf
echo "[5] RESET_BUFFER"
"$TOOL" reset || exit 1

RESULT="$(cat "$DEV")"
echo "after reset = [$RESULT]"

"$TOOL" stats || exit 1






#===== unload va in ra log =======
echo "[6] UNLOAD MODULE"
rmmod asgn1_driver || exit 1

echo "[7] DMESG"
dmesg | grep asgn1

echo "===== TASK 1 FINISHED ====="