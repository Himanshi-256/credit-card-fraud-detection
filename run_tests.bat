@echo off
setlocal
cd /d "%~dp0"
set "PATH=%~dp0.tools\w64devkit\bin;%PATH%"

echo ========================================================
echo   Running Fraud Detection Unit and Integration Tests
echo ========================================================

if not exist "backend\build\test_backend.exe" (
    echo Building tests with CMake...
    cmake -B backend\build -S backend -G "Ninja" -DCMAKE_C_COMPILER="%~dp0.tools\w64devkit\bin\gcc.exe" -DCMAKE_CXX_COMPILER="%~dp0.tools\w64devkit\bin\g++.exe" -DCMAKE_MAKE_PROGRAM="C:\Python314\Scripts\ninja.exe"
    cmake --build backend\build
)

backend\build\test_backend.exe
pause
