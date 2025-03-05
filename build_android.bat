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

echo building MetaObject from %SRC_PATH% to %DST_PATH% (building in %BUILD_PATH%, using packages from %PACKAGE_CACHE_PATH%)

rem first "build"/install the api.
rem (so the find_package in the agpengine cmakelists.txt works. the order of operations is a bit weird.)
if NOT exist %DST_PATH% (
    mkdir %DST_PATH%
)
xcopy %SRC_PATH%\include\*.* %DST_PATH%\include\ /S /Y /Q
rem install our findmodules.
xcopy %TEMPLATE_PATH%\cmake\*.* %DST_PATH%\cmake\ /Y /Q

set FLAGS=-DBASE_ROOT_DIRECTORY:PATH="%PACKAGE_CACHE_PATH%\LumeBase" %FLAGS%
set FLAGS=-DCORE_ROOT_DIRECTORY:PATH="%PACKAGE_CACHE_PATH%\LumeEngine" %FLAGS%
set MODULES="%PACKAGE_CACHE_PATH%\LumeBase\cmake"
set MODULES=%MODULES%;"%PACKAGE_CACHE_PATH%\LumeEngine\cmake"
set MODULES=%MODULES%;"%DST_PATH%\cmake"
set FLAGS=-DCMAKE_MODULE_PATH:PATH=%MODULES% %FLAGS%

if exist %BUILD_PATH% (
    rmdir %BUILD_PATH% /S /Q
)
call %TEMPLATE_PATH%\builder_android.bat %SRC_PATH% %BUILD_PATH% debug %FLAGS% 
if %errorlevel% neq 0 exit /b %errorlevel%
call %TEMPLATE_PATH%\builder_android.bat %SRC_PATH% %BUILD_PATH% release %FLAGS%
if %errorlevel% neq 0 exit /b %errorlevel%


xcopy %BUILD_PATH%\bin\debug\%ABI%\libPluginMetaObject.so %DST_PATH%\bin\Debug\%ABI%\ /Y /Q
xcopy %BUILD_PATH%\bin\release\%ABI%\libPluginMetaObject.so %DST_PATH%\bin\Release\%ABI%\ /Y /Q
xcopy %BUILD_PATH%\bin\release\%ABI%\libPluginMetaObject.so.sym %DST_PATH%\bin\Release\%ABI%\ /Y /Q

echo Android %2 Success.