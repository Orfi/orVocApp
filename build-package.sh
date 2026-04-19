#!/bin/bash
set -e

QT_PREFIX="${QT_PREFIX_PATH:-$HOME/Qt/6.10.2/gcc_64}"

echo "=== Configuring ==="
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QT_PREFIX"

echo "=== Building ==="
cmake --build build

echo "=== Running tests ==="
cd build && ctest --output-on-failure && cd ..

echo "=== Generating .deb ==="
cd build && cpack -G DEB && cd ..

echo "=== Done ==="
ls -lh build/*.deb
