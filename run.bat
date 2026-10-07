@echo off
setlocal
cd /d "%~dp0"
if not exist memory_policy_lab.exe (
  call build.bat
  if errorlevel 1 (
    pause
    exit /b 1
  )
)
start "" "%~dp0memory_policy_lab.exe"
