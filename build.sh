#!/bin/bash
set -e

echo "=> Checking dependencies..."
if ! command -v cmake &> /dev/null; then
    echo "CMake could not be found. Please install CMake."
    exit 1
fi

echo "=> Building project..."
mkdir -p build
cd build
cmake ..
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

echo "=> Build successful! Artifacts are in build/"
