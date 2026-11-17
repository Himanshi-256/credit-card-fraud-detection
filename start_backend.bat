@echo off
setlocal
cd /d "%~dp0"
set "PATH=%~dp0.tools\w64devkit\bin;%PATH%"

echo ========================================================
echo   Starting Credit Card Fraud Detection Backend (C++)
echo ========================================================

if not exist "backend\build\fraud_detection_backend.exe" (
    echo Binary not found. Building project with CMake and Ninja...
    cmake -B backend\build -S backend -G "Ninja" -DCMAKE_C_COMPILER="%~dp0.tools\w64devkit\bin\gcc.exe" -DCMAKE_CXX_COMPILER="%~dp0.tools\w64devkit\bin\g++.exe" -DCMAKE_MAKE_PROGRAM="C:\Python314\Scripts\ninja.exe"
    cmake --build backend\build
)

backend\build\fraud_detection_backend.exe --port 8080 --db backend\database\fraud_detection.db --csv backend\dataset\transactions.csv
pause
