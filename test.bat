@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc tests\test_simulator.c src\simulator.c src\input.c -o build\test_simulator.exe
if errorlevel 1 exit /b 1
build\test_simulator.exe
if errorlevel 1 exit /b 1
