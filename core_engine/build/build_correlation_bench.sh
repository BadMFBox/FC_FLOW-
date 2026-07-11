#!/usr/bin/env bash
set -euo pipefail

echo "Building governor correlation benchmark..."

g++ -std=c++23 -O3 -march=native -flto \
    -I../include \
    ../src/crypto/crossbolt.cpp \
    bench_with_governor_log.cpp \
    -o bench_governor_log \
    -pthread \
    -lsodium \
    -lssl \
    -lcrypto

if [ $? -eq 0 ]; then
    echo "✓ Build successful"
    echo ""
    echo "Running benchmark..."
    taskset -c 4 ./bench_governor_log
else
    echo "✗ Build failed"
    exit 1
fi
