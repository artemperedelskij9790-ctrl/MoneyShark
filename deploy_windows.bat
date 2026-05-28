@echo off
setlocal

rem Change this path if Qt Creator uses another build folder.
set "BUILD_DIR=%~dp0build"
rem Change this path if MSYS2/Qt is installed elsewhere.
set "QT_ROOT=D:\msys64\ucrt64"

echo Money Shark deployment
echo Build directory: %BUILD_DIR%

if not exist "%BUILD_DIR%\MoneyShark.exe" (
    echo MoneyShark.exe was not found in %BUILD_DIR%.
    echo Edit BUILD_DIR in this file and run it again.
    pause
    exit /b 1
)

where windeployqt >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Running windeployqt for MoneyShark.exe...
    windeployqt "%BUILD_DIR%\MoneyShark.exe"
    if exist "%BUILD_DIR%\MoneySharkServer.exe" (
        echo Running windeployqt for MoneySharkServer.exe...
        windeployqt "%BUILD_DIR%\MoneySharkServer.exe"
    )
) else (
    echo windeployqt was not found in PATH.
    echo Add Qt bin directory to PATH or run this script from a Qt command prompt.
)

if exist "%QT_ROOT%\share\qt6\plugins\tls" (
    echo Copying Qt TLS plugins...
    if not exist "%BUILD_DIR%\tls" mkdir "%BUILD_DIR%\tls"
    xcopy /Y /I "%QT_ROOT%\share\qt6\plugins\tls\*.dll" "%BUILD_DIR%\tls\" >nul
)

if exist "%QT_ROOT%\bin\libssl-3-x64.dll" (
    echo Copying OpenSSL runtime...
    copy /Y "%QT_ROOT%\bin\libssl-3-x64.dll" "%BUILD_DIR%\" >nul
)
if exist "%QT_ROOT%\bin\libcrypto-3-x64.dll" (
    copy /Y "%QT_ROOT%\bin\libcrypto-3-x64.dll" "%BUILD_DIR%\" >nul
)

echo Copying MinGW runtime dependencies...
for %%F in (
    libsqlite3-0.dll
    libgcc_s_seh-1.dll
    libstdc++-6.dll
    libwinpthread-1.dll
    libb2-1.dll
    libbrotlicommon.dll
    libbrotlidec.dll
    libbz2-1.dll
    libdouble-conversion.dll
    libfreetype-6.dll
    libglib-2.0-0.dll
    libgraphite2.dll
    libharfbuzz-0.dll
    libiconv-2.dll
    libicudt78.dll
    libicuin78.dll
    libicuuc78.dll
    libintl-8.dll
    libmd4c.dll
    libpcre2-16-0.dll
    libpcre2-8-0.dll
    libpng16-16.dll
    libzstd.dll
    zlib1.dll
) do (
    if exist "%QT_ROOT%\bin\%%F" copy /Y "%QT_ROOT%\bin\%%F" "%BUILD_DIR%\" >nul
)

echo Creating desktop shortcut...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$desktop=[Environment]::GetFolderPath('Desktop'); $shell=New-Object -ComObject WScript.Shell; $shortcut=$shell.CreateShortcut((Join-Path $desktop 'Money Shark.lnk')); $shortcut.TargetPath=(Join-Path '%BUILD_DIR%' 'MoneyShark.exe'); $shortcut.WorkingDirectory='%BUILD_DIR%'; $shortcut.IconLocation=(Join-Path '%BUILD_DIR%' 'MoneyShark.exe'); $shortcut.Save()"

echo Done.
pause
