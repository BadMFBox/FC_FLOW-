#!/usr/bin/env bash
set -euo pipefail

echo "╔══════════════════════════════════════════════════════╗"
echo "║   FC_FLOW v1.1.0 — CrossBolt Protocol Build         ║"
echo "╚══════════════════════════════════════════════════════╝"

for dep in g++ cmake pkg-config; do
    if ! command -v $dep &> /dev/null; then
        echo "ERROR: $dep not found"
        exit 1
    fi
done

if ! pkg-config --exists libsodium; then
    echo "ERROR: libsodium not found"
    exit 1
fi

mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/sovereign ..
make -j$(nproc)

if command -v ctest &> /dev/null; then
    echo ""
    echo "Running tests..."
    ctest --output-on-failure
fi

echo ""
echo "✅ Build complete"
