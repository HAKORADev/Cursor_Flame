@echo off
REM KursorFlame v3.0 - Stop all running instances
taskkill /F /IM KursorFlame.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo KursorFlame stopped.
) else (
    echo KursorFlame was not running.
)
timeout /t 2 >nul
