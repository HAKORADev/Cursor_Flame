@echo off
REM KursorFlame v3.0 - Windows Native Launcher
REM Starts the KursorFlame overlay. Press Ctrl+Alt+E to toggle, Ctrl+Alt+Q to quit.
title KursorFlame
cd /d "%~dp0"
start "" "KursorFlame.exe"
echo KursorFlame started. Hotkeys:
echo    Ctrl+Alt+E  -  Toggle effect ON/OFF
echo    Ctrl+Alt+Q  -  Quit KursorFlame
timeout /t 3 >nul
