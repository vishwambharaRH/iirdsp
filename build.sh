#!/bin/bash
# Quick Build & Test Guide for iirdsp

set -e

PROJECT_DIR="/Users/vishwam/VSCode/iirdsp"
BUILD_DIR="$PROJECT_DIR/build"

echo "=========================================="
echo "iirdsp Build & Test Quickstart"
echo "=========================================="
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "1. Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo ""
echo "2. Building library and examples..."
make -j4

# Run tests
echo ""
echo "3. Running tests..."
./test_impulse
echo ""

# Run example
echo "4. Running ECG example..."
./ecg_desktop
echo ""

echo "=========================================="
echo "✓ All builds and tests successful!"
echo "=========================================="
echo ""
echo "Output binaries:"
echo "  - libiirdsp_core.a (static library)"
echo "  - ecg_desktop (example executable)"
echo "  - test_impulse (test executable)"
echo ""
