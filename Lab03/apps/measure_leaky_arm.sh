#!/bin/sh

set -eu

APP=/usr/bin/leaky_app_arm_probe

echo "===== LAB03 ARM MEMORY MEASUREMENT ====="

"$APP" &
PID=$!

echo "TARGET_PID=$PID"

wait_for_stop() {
    count=0

    while [ "$count" -lt 30 ]; do
        if [ ! -r "/proc/$PID/status" ]; then
            echo "ERROR: Process disappeared"
            return 1
        fi

        if grep -q '^State:.*T' "/proc/$PID/status"; then
            return 0
        fi

        sleep 1
        count=$((count + 1))
    done

    echo "ERROR: Timeout waiting for process stop"
    return 1
}

show_memory() {
    grep -E \
      '^(Name|State|VmSize|VmRSS|VmData|VmStk|VmExe|VmLib|Threads):' \
      "/proc/$PID/status"

    echo "--- MEMORY MAP ---"
    head -15 "/proc/$PID/maps"
}

echo
echo "===== SNAPSHOT 1: BEFORE ALLOCATION ====="

wait_for_stop
show_memory

echo
echo "===== RESUME APPLICATION ====="

kill -CONT "$PID"

sleep 2

echo
echo "===== SNAPSHOT 2: AFTER PROCESSING ====="

wait_for_stop
show_memory

echo
echo "===== FINISH APPLICATION ====="

kill -CONT "$PID"
wait "$PID"

echo
echo "===== ARM MEMORY MEASUREMENT COMPLETE ====="
