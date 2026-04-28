@echo off
setlocal EnableDelayedExpansion

rem ========================================================================
rem orVocApp — Generate NSIS installer from an existing build
rem
rem Expects build\ to be populated by a prior build-package.bat run
rem (or manual `cmake --build build` with Release config).
rem ========================================================================

if not exist "build\CPackConfig.cmake" (
    echo ERROR: build\CPackConfig.cmake not found.
    echo        Run build-package.bat first to configure + build the project.
    exit /b 1
)

if not exist "build\orVocApp\orVocApp.exe" (
    echo ERROR: build\orVocApp\orVocApp.exe not found.
    echo        Build did not complete. Run build-package.bat.
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

if "%QT_PREFIX_PATH%"=="" set "QT_PREFIX_PATH=C:\Qt\6.10.2\mingw_64"
if "%MINGW_PATH%"==""    set "MINGW_PATH=C:\Qt\Tools\mingw1310_64"

rem windeployqt runs via install(CODE ...); it needs Qt bin on PATH
set "PATH=%QT_PREFIX_PATH%\bin;%MINGW_PATH%\bin;%PATH%"

echo === Generating NSIS installer ===
cpack -G NSIS --config build\CPackConfig.cmake -B build
if errorlevel 1 exit /b 1

echo.
echo === Done ===
dir /b build\orvocapp*.exe 2>nul
