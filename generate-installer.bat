@echo off
setlocal

if not exist build (
    echo ERROR: build\ directory not found. Run build-package.bat first.
    exit /b 1
)

echo === Generating installer ===
cd build
cpack -G NSIS -C Release
cd ..

echo === Done ===
dir /b build\orvocapp*.exe
