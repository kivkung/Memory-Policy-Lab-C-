@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc tests\test_simulator.c src\simulator.c src\input.c -o build\test_simulator.exe
if errorlevel 1 exit /b 1
build\test_simulator.exe
if errorlevel 1 exit /b 1
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc tests\test_search.c src\search.c src\simulator.c src\input.c -o build\test_search.exe
if errorlevel 1 exit /b 1
build\test_search.exe
if errorlevel 1 exit /b 1
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc tests\test_results.c src\results.c src\input.c -o build\test_results.exe
if errorlevel 1 exit /b 1
build\test_results.exe
if errorlevel 1 exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File tests\test_cli.ps1
if errorlevel 1 exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File tests\test_results_cli.ps1
if errorlevel 1 exit /b 1
