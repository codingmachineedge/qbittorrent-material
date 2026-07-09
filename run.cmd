@echo off
REM qBittorrent Material - one-click build & run (Windows, double-clickable).
REM Delegates to run.ps1 which auto-installs Qt, vcpkg+libtorrent, builds, and runs.
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1" %*
echo.
echo (window stays open so you can read any messages)
pause
endlocal
