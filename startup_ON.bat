@echo off
REM Enable KursorFlame autostart with Windows (per-user, no admin needed)
setlocal
set "TARGET=%~dp0run.bat"
set "SHORTCUT=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\KursorFlame.lnk"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$s=(New-Object -COM WScript.Shell).CreateShortcut('%SHORTCUT%'); $s.TargetPath='%TARGET%'; $s.WorkingDirectory='%~dp0'; $s.WindowStyle=7; $s.Description='KursorFlame v3.0 autostart'; $s.Save()"
if exist "%SHORTCUT%" (
    echo Autostart enabled: %SHORTCUT%
) else (
    echo Failed to create autostart shortcut.
)
timeout /t 3 >nul
