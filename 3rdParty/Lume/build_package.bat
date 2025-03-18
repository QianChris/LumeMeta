@echo off
SETLOCAL EnableDelayedExpansion

if "%1"=="" (
    echo Missing source path
    exit /b -1
)
if "%2"=="" (
    echo Missing package cache path
    exit /b -1
)

set PACKAGE_NAME=LumeEngine
set SRC_PATH=%~f1
set PACKAGE_CACHE_PATH=%~f2
if "%SRC_PATH:~-1%"=="\" set "SRC_PATH=%SRC_PATH:~0,-1%"
if "%PACKAGE_CACHE_PATH:~-1%"=="\" set "PACKAGE_CACHE_PATH=%PACKAGE_CACHE_PATH:~0,-1%"
set MY_PATH=%~dp0

echo building package %PACKAGE_NAME% from %SRC_PATH% to %PACKAGE_CACHE_PATH%

if exist %PACKAGE_CACHE_PATH%\%PACKAGE_NAME% (
    echo Deleting old package
    rmdir %PACKAGE_CACHE_PATH%\%PACKAGE_NAME% /S /Q
)
call %MY_PATH%\build.bat %SRC_PATH% %PACKAGE_NAME% %PACKAGE_CACHE_PATH%\build_temp %PACKAGE_CACHE_PATH%
if %errorlevel% neq 0 exit /b %errorlevel%
call %MY_PATH%\build_android.bat %SRC_PATH% %PACKAGE_NAME% %PACKAGE_CACHE_PATH%\build_temp %PACKAGE_CACHE_PATH%
if %errorlevel% neq 0 exit /b %errorlevel%
echo Package %PACKAGE_NAME% build success.