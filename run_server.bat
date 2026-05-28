@echo off
setlocal

rem Change this path if Qt Creator uses another build folder.
set "BUILD_DIR=%~dp0build"

if exist "%BUILD_DIR%\MoneySharkServer.exe" (
    "%BUILD_DIR%\MoneySharkServer.exe" --port 45454
    exit /b %ERRORLEVEL%
)

if exist "%~dp0MoneySharkServer.exe" (
    "%~dp0MoneySharkServer.exe" --port 45454
    exit /b %ERRORLEVEL%
)

echo MoneySharkServer.exe was not found.
echo Edit BUILD_DIR in this file or build the project first.
pause
exit /b 1
