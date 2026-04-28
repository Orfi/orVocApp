@echo off
setlocal EnableDelayedExpansion

rem ========================================================================
rem orVocApp — Windows build + NSIS installer
rem
rem Configures in Release, builds, runs tests, then generates an NSIS .exe
rem installer via CPack. windeployqt is invoked by the install(CODE ...)
rem rule in orVocApp/CMakeLists.txt so it always runs against a Release exe.
rem
rem Overrides via environment:
rem   QT_PREFIX_PATH   Qt install root (default: C:\Qt\6.10.2\mingw_64)
rem   MINGW_PATH       MinGW toolchain (default: C:\Qt\Tools\mingw1310_64)
rem   NINJA_PATH       Ninja install (default: C:\Qt\Tools\Ninja)
rem ========================================================================

if "%QT_PREFIX_PATH%"=="" set "QT_PREFIX_PATH=C:\Qt\6.10.2\mingw_64"
if "%MINGW_PATH%"==""    set "MINGW_PATH=C:\Qt\Tools\mingw1310_64"
if "%NINJA_PATH%"==""    set "NINJA_PATH=C:\Qt\Tools\Ninja"

if not exist "%QT_PREFIX_PATH%\bin\qmake.exe" (
    echo ERROR: Qt not found at %QT_PREFIX_PATH%
    echo        Set QT_PREFIX_PATH to your Qt install, e.g. C:\Qt\6.10.2\mingw_64
    exit /b 1
)
if not exist "%MINGW_PATH%\bin\g++.exe" (
    echo ERROR: MinGW g++ not found at %MINGW_PATH%\bin\g++.exe
    exit /b 1
)
if not exist "%NINJA_PATH%\ninja.exe" (
    echo ERROR: Ninja not found at %NINJA_PATH%\ninja.exe
    exit /b 1
)

where makensis.exe >nul 2>&1
if errorlevel 1 (
    rem Not on PATH — fall back to the standard NSIS install location
    if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" (
        set "PATH=%ProgramFiles(x86)%\NSIS;%PATH%"
    ) else if exist "%ProgramFiles%\NSIS\makensis.exe" (
        set "PATH=%ProgramFiles%\NSIS;%PATH%"
    ) else (
        echo ERROR: NSIS not found ^(makensis.exe is not on PATH^).
        echo        Install it from https://nsis.sourceforge.io/Download
        echo        or run: choco install nsis -y
        exit /b 1
    )
)

set "PATH=%QT_PREFIX_PATH%\bin;%MINGW_PATH%\bin;%NINJA_PATH%;%PATH%"

echo === Configuring (Release, Ninja) ===
cmake -S . -B build -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_PREFIX_PATH%" ^
    -DCMAKE_CXX_COMPILER="%MINGW_PATH:\=/%/bin/g++.exe" ^
    -DCMAKE_MAKE_PROGRAM="%NINJA_PATH:\=/%/ninja.exe"
if errorlevel 1 exit /b 1

echo === Building ===
cmake --build build
if errorlevel 1 exit /b 1

echo === Running tests ===
ctest --test-dir build --output-on-failure
if errorlevel 1 exit /b 1

echo === Generating NSIS installer (runs windeployqt at install time) ===
cpack -G NSIS --config build\CPackConfig.cmake -B build
if errorlevel 1 exit /b 1

echo.
echo === Done ===
dir /b build\orvocapp*.exe 2>nul
