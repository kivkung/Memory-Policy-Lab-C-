@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 src\main.c src\simulator.c src\input.c src\display.c -o build\simulator.exe
if errorlevel 1 exit /b 1
echo Built build\simulator.exe
