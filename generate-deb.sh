#!/bin/bash
set -e

if [ ! -d "build" ]; then
    echo "ERROR: build/ directory not found. Run build-package.sh first."
    exit 1
fi

echo "=== Generating .deb ==="
cd build && cpack -G DEB && cd ..

echo "=== Done ==="
ls -lh build/*.deb
