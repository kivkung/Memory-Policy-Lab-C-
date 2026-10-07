@echo off
setlocal
cd /d "%~dp0"
if exist "C:\msys64\ucrt64\bin\gcc.exe" set "PATH=C:\msys64\ucrt64\bin;%PATH%"
if not exist build mkdir build
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -static src\main.c src\simulator.c src\input.c src\display.c src\search.c src\results.c -o build\memory_policy_lab.exe
if errorlevel 1 exit /b 1
echo CLI built at build\memory_policy_lab.exe
