@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo  GroundAndreas - Building with MSVC (Visual Studio)
echo ========================================================

cd /d "%~dp0"

:: Auto-detect CMake in PATH, WinGet, or Visual Studio
where cmake >nul 2>nul
if errorlevel 1 (
    for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\Kitware.CMake_*") do (
        for /d %%C in ("%%D\cmake-*\bin") do (
            if exist "%%C\cmake.exe" set "PATH=%%C;!PATH!"
        )
    )
)
where cmake >nul 2>nul
if errorlevel 1 (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;!PATH!"
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;!PATH!"
    )
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake could not be found. Please ensure CMake or Visual Studio is installed.
    exit /b 1
)

:: Configure
echo [1/3] Configuring CMake (Win32)...
cmake -B "build-msvc" -G "Visual Studio 17 2022" -A Win32
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b %errorlevel%
)

:: Build
echo [2/3] Compiling Release binaries...
cmake --build "build-msvc" --config Release
if errorlevel 1 (
    echo [ERROR] Compilation failed.
    exit /b %errorlevel%
)

:: Package and create Zip archive
echo [3/3] Creating zip package...
set "STAGE_DIR=package-msvc"
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
if not exist "dist" mkdir "dist"

mkdir "%STAGE_DIR%\plugins"
mkdir "%STAGE_DIR%\components"
mkdir "%STAGE_DIR%\include"
mkdir "%STAGE_DIR%\scriptfiles"
mkdir "%STAGE_DIR%\filterscripts"

if exist "build-msvc\Release\GroundAndreas.dll" (
    copy /Y "build-msvc\Release\GroundAndreas.dll" "%STAGE_DIR%\plugins\GroundAndreas.dll" >nul
)
if exist "build-msvc\components\Release\GroundAndreas.dll" (
    copy /Y "build-msvc\components\Release\GroundAndreas.dll" "%STAGE_DIR%\components\GroundAndreas.dll" >nul
)
if exist "GroundAndreas.inc" (
    copy /Y "GroundAndreas.inc" "%STAGE_DIR%\include\GroundAndreas.inc" >nul
)
if exist "scriptfiles\SAfull.hmap" (
    copy /Y "scriptfiles\SAfull.hmap" "%STAGE_DIR%\scriptfiles\SAfull.hmap" >nul
)
if exist "FS_GroundAndreas_Demo.pwn" (
    copy /Y "FS_GroundAndreas_Demo.pwn" "%STAGE_DIR%\FS_GroundAndreas_Demo.pwn" >nul
    copy /Y "FS_GroundAndreas_Demo.pwn" "%STAGE_DIR%\filterscripts\FS_GroundAndreas_Demo.pwn" >nul
)
if exist "FS_GroundAndreas_Demo.amx" (
    copy /Y "FS_GroundAndreas_Demo.amx" "%STAGE_DIR%\FS_GroundAndreas_Demo.amx" >nul
    copy /Y "FS_GroundAndreas_Demo.amx" "%STAGE_DIR%\filterscripts\FS_GroundAndreas_Demo.amx" >nul
)
if exist "LICENSE" (
    copy /Y "LICENSE" "%STAGE_DIR%\LICENSE" >nul
)

:: Clean up previous files in dist
if exist "dist\GroundAndreas-Windows.zip" del /f /q "dist\GroundAndreas-Windows.zip"
if exist "dist\plugins" rmdir /s /q "dist\plugins"
if exist "dist\components" rmdir /s /q "dist\components"
if exist "dist\include" rmdir /s /q "dist\include"
if exist "dist\scriptfiles" rmdir /s /q "dist\scriptfiles"

:: Generate zip archive
where tar.exe >nul 2>nul
if not errorlevel 1 (
    tar.exe -a -c -f "dist\GroundAndreas-Windows.zip" -C "%STAGE_DIR%" *
) else (
    powershell -Command "Import-Module Microsoft.PowerShell.Archive; Compress-Archive -Path '%STAGE_DIR%\*' -DestinationPath 'dist\GroundAndreas-Windows.zip' -Force"
)

:: Clean up staging folder
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"

echo [SUCCESS] Package created: dist\GroundAndreas-Windows.zip

:: Optional deployment if target path is passed as argument or env var
set "TARGET_DIR=%~1"
if "%TARGET_DIR%"=="" set "TARGET_DIR=%OMP_SERVER_DIR%"

if not "%TARGET_DIR%"=="" (
    if exist "%TARGET_DIR%" (
        echo.
        echo Deploying to target directory: %TARGET_DIR%
        if exist "build-msvc\components\Release\GroundAndreas.dll" if exist "%TARGET_DIR%\components" copy /Y "build-msvc\components\Release\GroundAndreas.dll" "%TARGET_DIR%\components\GroundAndreas.dll" >nul
        if exist "build-msvc\Release\GroundAndreas.dll" if exist "%TARGET_DIR%\plugins" copy /Y "build-msvc\Release\GroundAndreas.dll" "%TARGET_DIR%\plugins\GroundAndreas.dll" >nul
        if exist "GroundAndreas.inc" if exist "%TARGET_DIR%\pawno\include" copy /Y "GroundAndreas.inc" "%TARGET_DIR%\pawno\include\GroundAndreas.inc" >nul
        if exist "scriptfiles\SAfull.hmap" if exist "%TARGET_DIR%\scriptfiles" copy /Y "scriptfiles\SAfull.hmap" "%TARGET_DIR%\scriptfiles\SAfull.hmap" >nul
        echo Deployment complete.
    )
)

echo ========================================================
echo  Build finished successfully!
echo ========================================================
