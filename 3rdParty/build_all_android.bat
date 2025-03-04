@echo off
cls
SETLOCAL EnableDelayedExpansion
if "%NDK_PATH%"=="" (
    set NDK_PATH=c:\android\ndk\21.3.6528147
    rem set NDK_PATH=c:\android\ndk\24.0.8215888
    echo NDK_PATH was not set. using default.
)
if "%API_VERSION%"=="" (
    set API_VERSION=29
    echo API_VERSION was not set. using default.
)
if "%ABI%"=="" (
    set ABI=arm64-v8a
    echo ABI was not set. using default.
)

if "%1"=="" (
    set SOURCE_PATH=%CD%\..\submodules
    echo Building 3rdparty deps from submodules
) else (
    set SOURCE_PATH=%1
    echo Building 3rdparty deps from %SOURCE_PATH%
)
pushd LumeBase
call build_android.bat %SOURCE_PATH%\LumeBase
if %errorlevel% neq 0 exit /b %errorlevel%
popd

pushd Lume
call build_android.bat %SOURCE_PATH%\LumeEngine
if %errorlevel% neq 0 exit /b %errorlevel%
popd

echo Completed!