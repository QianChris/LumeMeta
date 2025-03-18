@echo off
SETLOCAL EnableDelayedExpansion
if "%1"=="" (
    echo Missing source path
    exit /b -1
)
if "%2"=="" (
    echo Missing package name
    exit /b -1
)
if "%3"=="" (
    echo Missing build path
    exit /b -1
)
if "%4"=="" (
    echo Missing package cache path
    exit /b -1
)
set SRC_PATH=%~f1
set BUILD_PATH=%~f3
set PACKAGE_CACHE_PATH=%~f4
set TEMPLATE_PATH=%~dp0
if "%SRC_PATH:~-1%"=="\" set "SRC_PATH=%SRC_PATH:~0,-1%"
if "%BUILD_PATH:~-1%"=="\" set "BUILD_PATH=%BUILD_PATH:~0,-1%"
if "%PACKAGE_CACHE_PATH:~-1%"=="\" set "PACKAGE_CACHE_PATH=%PACKAGE_CACHE_PATH:~0,-1%"
if "%TEMPLATE_PATH:~-1%"=="\" set "TEMPLATE_PATH=%TEMPLATE_PATH:~0,-1%"
set DST_PATH=%PACKAGE_CACHE_PATH%\%2

echo building LumeBase from %SRC_PATH% to %DST_PATH% (building in %BUILD_PATH%, using packages from %PACKAGE_CACHE_PATH%)

if exist %BUILD_PATH% (
    rmdir %BUILD_PATH% /S /Q
)

call %TEMPLATE_PATH%\..\build_android.bat %SRC_PATH% %BUILD_PATH% debug -DCORE_UNPACKED_LIB_DIR:PATH="%BUILD_PATH%\unpacked"
if %errorlevel% neq 0 exit /b %errorlevel%
call %TEMPLATE_PATH%\..\build_android.bat %SRC_PATH% %BUILD_PATH% release -DCORE_UNPACKED_LIB_DIR:PATH="%BUILD_PATH%\unpacked"
if %errorlevel% neq 0 exit /b %errorlevel%


if NOT exist %DST_PATH% (
    mkdir %DST_PATH%
)

xcopy %SRC_PATH%\api\*.* %DST_PATH%\api\ /S /Y /Q
xcopy %TEMPLATE_PATH%\cmake\*.* %DST_PATH%\cmake\ /Y /Q
xcopy %SRC_PATH%\cmake\cmake_tools.cmake %DST_PATH%\cmake\ /Y /Q
rem get the most fresh secure c extract and copy stuff from it
for /f "delims=" %%a in (' dir /OD /B %BUILD_PATH%\unpacked\libboundscheck-* ') do set "SECURE_C_SRC=%%a"
xcopy %BUILD_PATH%\unpacked\%SECURE_C_SRC%\include\ %DST_PATH%\securec\ /S /Y /Q

rem copy the securec library binary..
xcopy %BUILD_PATH%\debug\3rdparty\securec\libsecurec.a %DST_PATH%\bin\Debug\%ABI%\ /Y /Q
xcopy %BUILD_PATH%\release\3rdparty\securec\libsecurec.a %DST_PATH%\bin\Release\%ABI%\ /Y /Q

echo Android %2 Success.