@echo off
setlocal

cd /d "%~dp0"

echo Starting Vico GATT status watcher...
echo Project: %CD%
echo.

python tools\send_status_ble.py status.json --watch --keep-connected --no-ack --interval 0.2 --debounce 0.1 %*

echo.
echo Watcher stopped. Press any key to close this window.
pause >nul
