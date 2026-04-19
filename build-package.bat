@echo off
setlocal

if "%QT_PREFIX_PATH%"=="" (
    echo ERROR: Set QT_PREFIX_PATH to your Qt installation, e.g. C:\Qt\6.10.2\msvc2019_64
    exit /b 1
)

echo === Configuring ===
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_PREFIX_PATH%"
if %errorlevel% neq 0 exit /b %errorlevel%

echo === Building ===
cmake --build build --config Release
if %errorlevel% neq 0 exit /b %errorlevel%

echo === Running tests ===
cd build
ctest --output-on-failure -C Release
if %errorlevel% neq 0 (
    cd ..
    exit /b %errorlevel%
)
cd ..

echo === Running windeployqt ===
"%QT_PREFIX_PATH%\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler build\orVocab\Release\orVocApp.exe
if %errorlevel% neq 0 exit /b %errorlevel%

echo === Generating installer ===
cd build
cpack -G NSIS -C Release
cd ..

echo === Done ===
dir /b build\orvocapp*.exe
