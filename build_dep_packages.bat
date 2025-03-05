@echo off
cls
SETLOCAL EnableDelayedExpansion

if "%1"=="" (
    set PACKAGE_CACHE=%CD%\package_cache
) else (
    set PACKAGE_CACHE=%~f1
)

if "%2"=="" (
    set SOURCE_PATH=%CD%\submodules
) else (
    set SOURCE_PATH=%~f2
)



if "%SOURCE_PATH:~-1%"=="\" set "SOURCE_PATH=%SOURCE_PATH:~0,-1%"
if "%PACKAGE_CACHE:~-1%"=="\" set "PACKAGE_CACHE=%PACKAGE_CACHE:~0,-1%"


echo Building 3rdparty deps from %SOURCE_PATH%
echo Using %PACKAGE_CACHE% as package cache

if NOT exist %PACKAGE_CACHE% (
    mkdir %PACKAGE_CACHE%
)

echo %TIME% > %PACKAGE_CACHE%\..\BuildStart.txt
call 3rdParty\LumeBase\build_package.bat %SOURCE_PATH%\LumeBase %PACKAGE_CACHE%
if %errorlevel% neq 0 exit /b %errorlevel%

call 3rdParty\Lume\build_package.bat %SOURCE_PATH%\LumeEngine %PACKAGE_CACHE%
if %errorlevel% neq 0 exit /b %errorlevel%

echo %TIME% > %PACKAGE_CACHE%\..\BuildEnd.txt
echo Completed!