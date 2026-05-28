@echo off
setlocal

rem Change this path if Qt Creator uses another build folder.
set "BUILD_DIR=%~dp0build"

if exist "%BUILD_DIR%\MoneyShark.exe" (
    start "" "%BUILD_DIR%\MoneyShark.exe"
    exit /b 0
)

if exist "%~dp0MoneyShark.exe" (
    start "" "%~dp0MoneyShark.exe"
    exit /b 0
)

echo MoneyShark.exe was not found.
echo Edit BUILD_DIR in this file or build the project first.
pause
exit /b 1
