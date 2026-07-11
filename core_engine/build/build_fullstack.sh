#!/usr/bin/env bash
set -euo pipefail

BINARY="bench_fullstack"
CORE_TARGET=4
FIFO_PRIORITY=40

echo "========================================================="
echo "   FC_FLOW — FULL-STACK BUILD (Burners + Telemetry)    "
echo "========================================================="

if [ -f "$BINARY" ]; then
    rm "$BINARY"
fi

echo "[*] Compiling $BINARY with -march=native, -flto, -O3..."

g++ -std=c++23 -O3 -march=native -flto -fomit-frame-pointer \
    -I../include \
    ../src/crypto/crossbolt.cpp \
    ../src/enforcement/monitus.cpp \
    ../src/burner/fuel_flow.cpp \
    ../src/burner/ram_wiper.cpp \
    ../src/burner/token.cpp \
    ../src/burner/stats.cpp \
    ../src/burner/predator_gate.cpp \
    ../src/enforcement/burner_bridge.cpp \
    ../tests/cpp/bench_fullstack.cpp \
    -o "$BINARY" \
    -pthread \
    -lsodium \
    -lssl \
    -lcrypto

echo "✓ Compilation successful."
echo "[*] Initializing environment isolation rules..."

if [ "$(id -u)" -eq 0 ]; then
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
