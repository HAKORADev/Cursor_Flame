@echo off
REM Disable KursorFlame autostart with Windows
set "SHORTCUT=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\KursorFlame.lnk"
if exist "%SHORTCUT%" (
    del /q "%SHORTCUT%"
    echo Autostart disabled.
) else (
    echo Autostart was not enabled.
)
timeout /t 2 >nul
