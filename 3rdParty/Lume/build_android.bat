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

echo building engine from %SRC_PATH% to %DST_PATH% (building in %BUILD_PATH%, using packages from %PACKAGE_CACHE_PATH%)

rem first "build"/install the api.
rem (so the find_package in the agpengine cmakelists.txt works. the order of operations is a bit weird.)
if NOT exist %DST_PATH% (
    mkdir %DST_PATH%
)
xcopy %SRC_PATH%\api\core\*.* %DST_PATH%\api\core\ /S /Y /Q
xcopy %SRC_PATH%\api\platform\*.* %DST_PATH%\api\platform\ /S /Y /Q
xcopy %SRC_PATH%\cmake\cmake_tools.cmake %DST_PATH%\cmake\ /Y /Q
xcopy %SRC_PATH%\tools\*.exe %DST_PATH%\tools\ /Y /Q
xcopy %SRC_PATH%\ecshelper %DST_PATH%\ecshelper\ /S /Y /Q
xcopy %SRC_PATH%\src\ecs\ecs.natvis %DST_PATH%\api\ /Y /Q
xcopy %SRC_PATH%\src\static_plugin_decl.h %DST_PATH%\src\ /Y /Q
rem install our findmodules.
xcopy %TEMPLATE_PATH%\cmake\*.* %DST_PATH%\cmake\ /Y /Q

rem disable debug/validation
set FLAGS=-DCORE_VALIDATION_ENABLED=OFF
rem MUST force the BASE_ROOT_DIRECTORY here.. since it is used BEFORE finding the base.. so the findpackage wont work.
set FLAGS=-DBASE_ROOT_DIRECTORY:PATH="%PACKAGE_CACHE_PATH%\LumeBase" %FLAGS%
set MODULES="%PACKAGE_CACHE_PATH%\LumeBase\cmake"
set MODULES=%MODULES%;"%DST_PATH%\cmake"
set FLAGS=-DCMAKE_MODULE_PATH:PATH=%MODULES% %FLAGS%
set FLAGS=-DCORE_UNPACKED_LIB_DIR:PATH="%BUILD_PATH%\unpacked" %FLAGS%

if exist %BUILD_PATH% (
    rmdir %BUILD_PATH% /S /Q
)
call %TEMPLATE_PATH%\..\build_android.bat %SRC_PATH% %BUILD_PATH% debug %FLAGS% 
if %errorlevel% neq 0 exit /b %errorlevel%
call %TEMPLATE_PATH%\..\build_android.bat %SRC_PATH% %BUILD_PATH% release %FLAGS%
if %errorlevel% neq 0 exit /b %errorlevel%


xcopy %BUILD_PATH%\bin\debug\%ABI%\libAGPEngineDLL.so %DST_PATH%\bin\Debug\%ABI%\ /Y /Q
xcopy %BUILD_PATH%\bin\release\%ABI%\libAGPEngineDLL.so %DST_PATH%\bin\Release\%ABI%\ /Y /Q
xcopy %BUILD_PATH%\bin\release\%ABI%\libAGPEngineDLL.so.sym %DST_PATH%\bin\Release\%ABI%\ /Y /Q

echo Android %2 Success.