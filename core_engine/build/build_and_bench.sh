#!/usr/bin/env bash
set -euo pipefail

# --- CONFIGURATION ---
BINARY="bench_7tier_telemetry"
CORE_TARGET=4  # Target high-performance core (Adjust based on your CPU topology)
FIFO_PRIORITY=40  # Reduced from 99 for container environments

echo "========================================================="
echo "   FC_FLOW — AGGRESSIVE BUILD & ISOLATION PIPELINE       "
echo "========================================================="

# 1. Clean old artifact
if [ -f "$BINARY" ]; then
    rm "$BINARY"
fi

# 2. Compile with bare-metal microarchitectural optimizations
echo "[*] Compiling $BINARY with C++23, -O3, Native Arch, and LTO..."

g++ -std=c++23 -O3 -march=native -flto -fomit-frame-pointer \
    -I../include \
    ../src/crypto/crossbolt.cpp \
    ../src/enforcement/monitus.cpp \
    ../tests/cpp/bench_7tier_telemetry.cpp \
    -o "$BINARY" \
    -pthread \
    -lsodium

echo "✓ Compilation successful."

# 3. Try real-time scheduling, fall back to core pinning if blocked
echo "[*] Initializing environment isolation rules..."

if [ "$(id -u)" -eq 0 ]; then
    # Try SCHED_FIFO with lower priority first
    if chrt -f "$FIFO_PRIORITY" true 2>/dev/null; then
        echo "[+] SCHED_FIFO Priority $FIFO_PRIORITY granted. Launching on Core $CORE_TARGET..."
        echo "========================================================="
        exec chrt -f "$FIFO_PRIORITY" taskset -c "$CORE_TARGET" ./"$BINARY"
    else
        echo "[!] SCHED_FIFO blocked by kernel. Falling back to core pinning only."
        echo "[*] Launching with standard priority pinned to Core $CORE_TARGET..."
        echo "========================================================="
        exec taskset -c "$CORE_TARGET" ./"$BINARY"
    fi
else
    echo "[!] Warning: Not running as root. Using core pinning only."
    echo "[*] Launching with standard priority pinned to Core $CORE_TARGET..."
    echo "========================================================="
    exec taskset -c "$CORE_TARGET" ./"$BINARY"
fi
