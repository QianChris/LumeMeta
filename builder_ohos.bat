@echo off
:: NOTE: we do not keep these settings local. (they should be already set anyway)
if "%OHOS_NDK_PATH%"=="" (
    echo OHOS_NDK_PATH NOT SET!
    exit /b 1
)
if NOT DEFINED CMAKE_MAKE_PROGRAM (
    echo CMAKE_MAKE_PROGRAM NOT SET!
    exit /b 1
)
if NOT DEFINED CMAKE_COMMAND (
    echo CMAKE_COMMAND NOT SET!
    exit /b 1
)
if "%ABI%"=="" (
    set ABI=arm64-v8a
    echo ABI was not set. using default.
)
SETLOCAL EnableDelayedExpansion
if "%1"=="" (
    echo Missing Source path
    exit /b -1
)
if "%2"=="" (
    echo Missing Build path
    exit /b -1
)
set SOURCE_PATH=%~f1
set BUILD_PATH=%~f2

if "%SOURCE_PATH:~-1%"=="\" set "SOURCE_PATH=%SOURCE_PATH:~0,-1%"
if "%BUILD_PATH:~-1%"=="\" set "BUILD_PATH=%BUILD_PATH:~0,-1%"

set BUILD_TYPE=%3
for /f "tokens=3,* delims= " %%a in ("%*") do set OPTIONS=%%b

rem build %1 repo.
rem copy api folder. and binaries
echo building %SOURCE_PATH% as %BUILD_TYPE% (BUILD_PATH %BUILD_PATH%)
echo with options "%OPTIONS%"

echo Using NDK: %NDK_PATH% API: %API_VERSION% ABI: %ABI%

set BASE_OHOS_OPTIONS=-DCMAKE_TOOLCHAIN_FILE:FILEPATH="%OHOS_NDK_PATH%build\cmake\ohos.toolchain.cmake" -DOHOS_ARCH=%ABI% -DOHOS_STL=c++_static -DCMAKE_MAKE_PROGRAM=!CMAKE_MAKE_PROGRAM!

if "%BUILD_TYPE%"=="release" (
set FLAGS=-DCMAKE_BUILD_TYPE=Release %OPTIONS%
)
if "%BUILD_TYPE%"=="debug" (
set FLAGS=-DCMAKE_BUILD_TYPE=Debug %OPTIONS%
)
set PATHS=-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH="%BUILD_PATH%\bin\%BUILD_TYPE%\%ABI%" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH="%BUILD_PATH%\bin\%BUILD_TYPE%\%ABI%"

set BUILD_OPTIONS=%BASE_OHOS_OPTIONS% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DMETA_TESTS_ENABLED=OFF -DMETA_SUBMODULE_ENABLED=ON -DBUILD_DEPS=OFF 

if exist CMakeCache.txt (
    !CMAKE_COMMAND! --build %BUILD_PATH%\%BUILD_TYPE% --target clean
    del CMakeCache.txt
)
!CMAKE_COMMAND! -GNinja %BUILD_OPTIONS% %FLAGS% %PATHS% -S %SOURCE_PATH% -B %BUILD_PATH%\%BUILD_TYPE%
if %errorlevel% neq 0 exit /b %errorlevel%
!CMAKE_COMMAND! --build %BUILD_PATH%\%BUILD_TYPE%
if %errorlevel% neq 0 exit /b %errorlevel%

if "%BUILD_TYPE%"=="release" (
    if exist %BUILD_PATH%\bin\release\%ABI% ( 
        echo separate and strip debug symbols from release.
        pushd %BUILD_PATH%\bin\release\%ABI%\
        for %%f in (*.so) do (
            "%OHOS_NDK_PATH%llvm\bin\llvm-objcopy.exe" --only-keep-debug %%f %%f.sym
            "%OHOS_NDK_PATH%llvm\bin\llvm-strip.exe" --strip-debug --strip-unneeded %%f
            "%OHOS_NDK_PATH%llvm\bin\llvm-objcopy.exe"  --add-gnu-debuglink=%%f.sym %%f
        )
        popd
    )
)