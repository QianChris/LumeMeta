@echo off
cls
SETLOCAL EnableDelayedExpansion

if "%1"=="" (
    set SOURCE_PATH=%CD%\..\submodules
    echo Building 3rdparty deps from submodules
) else (
    set SOURCE_PATH=%1
    echo Building 3rdparty deps from %SOURCE_PATH%
)

pushd LumeBase
call build.bat %SOURCE_PATH%\LumeBase
if %errorlevel% neq 0 exit /b %errorlevel%
popd

pushd Lume
call build.bat %SOURCE_PATH%\LumeEngine
if %errorlevel% neq 0 exit /b %errorlevel%
popd