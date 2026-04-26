#!/bin/bash
set -e

QT_PREFIX="${QT_PREFIX_PATH:-$HOME/Qt/6.10.2/gcc_64}"
BUILD_DIR="${BUILD_DIR:-build}"

echo "=== Configuring ==="
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QT_PREFIX"

echo "=== Building ==="
cmake --build "$BUILD_DIR"

echo "=== Running tests ==="
cd "$BUILD_DIR" && ctest --output-on-failure && cd - >/dev/null

echo "=== Generating .deb ==="
cd "$BUILD_DIR" && cpack -G DEB && cd - >/dev/null

echo "=== Done ==="
ls -lh "$BUILD_DIR"/*.deb
