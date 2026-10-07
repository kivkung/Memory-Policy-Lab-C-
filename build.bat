@echo off
setlocal
cd /d "%~dp0"
if exist "C:\msys64\ucrt64\bin\gcc.exe" set "PATH=C:\msys64\ucrt64\bin;%PATH%"
where gcc >nul 2>nul
if errorlevel 1 (
  echo GCC not found. Install MSYS2 UCRT64 GCC or add gcc to PATH.
  exit /b 1
)
if not exist build mkdir build
windres -Isrc src\gui.rc -o build\gui-resource.o
if errorlevel 1 exit /b 1
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -static -municode -mwindows src\gui.c src\simulator.c src\input.c src\search.c build\gui-resource.o -o memory_policy_lab.exe -lcomctl32 -lcomdlg32 -lgdi32 -luxtheme
if errorlevel 1 exit /b 1
echo Built memory_policy_lab.exe
