@echo off
:: NOTE: we do not keep these settings local. (they should be already set anyway)
if "%NDK_PATH%"=="" (
    set NDK_PATH=c:\android\ndk\22.1.7171670
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
for /f "tokens=2,* delims= " %%a in ("%*") do set OPTIONS=%%b

rem build %1 repo.
rem copy api folder. and binaries
echo building %SOURCE_PATH% as %BUILD_TYPE% (BUILD_PATH %BUILD_PATH%)
echo with options "%OPTIONS%"

echo Using NDK: %NDK_PATH% API: %API_VERSION% ABI: %ABI%

set BASE_ANDROID_OPTIONS=-DANDROID_NDK:PATH="%NDK_PATH%" -DCMAKE_ANDROID_NDK:PATH="%NDK_PATH%" -DCMAKE_TOOLCHAIN_FILE:FILEPATH="%NDK_PATH%\build\cmake\android.toolchain.cmake" -DANDROID_PLATFORM=android-%API_VERSION% -DCMAKE_SYSTEM_VERSION=%API_VERSION% -DCMAKE_SYSTEM_NAME=Android
set ARCH_ANDROID_OPTIONS=-DANDROID_ABI=%ABI% -DCMAKE_ANDROID_ARCH_ABI=%ABI%
set ANDROID_OPTIONS=%BASE_ANDROID_OPTIONS% %ARCH_ANDROID_OPTIONS% -DANDROID_STL=c++_static

if "%BUILD_TYPE%"=="release" (
set FLAGS=-DCMAKE_BUILD_TYPE=Release %OPTIONS%
)
if "%BUILD_TYPE%"=="debug" (
set FLAGS=-DCMAKE_BUILD_TYPE=Debug %OPTIONS%
)
set PATHS=-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH="%BUILD_PATH%\bin\%BUILD_TYPE%\%ABI%" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH="%BUILD_PATH%\bin\%BUILD_TYPE%\%ABI%"

set BUILD_OPTIONS=%ANDROID_OPTIONS% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DMETA_TESTS_ENABLED=OFF -DMETA_SUBMODULE_ENABLED=ON -DBUILD_DEPS=OFF 

if exist CMakeCache.txt (
    cmake --build %BUILD_PATH%\%BUILD_TYPE% --target clean
    del CMakeCache.txt
)
cmake -GNinja %BUILD_OPTIONS% %FLAGS% %PATHS% -S %SOURCE_PATH% -B %BUILD_PATH%\%BUILD_TYPE%
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build %BUILD_PATH%\%BUILD_TYPE%
if %errorlevel% neq 0 exit /b %errorlevel%

if "%BUILD_TYPE%"=="release" (
    if exist %BUILD_PATH%\bin\release\%ABI% ( 
        echo separate and strip debug symbols from release.
        pushd %BUILD_PATH%\bin\release\%ABI%\
        for %%f in (*.so) do %NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-objcopy.exe --only-keep-debug %%f %%f.sym
        for %%f in (*.so) do %NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-strip.exe --strip-debug --strip-unneeded %%f
        for %%f in (*.so) do %NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-objcopy.exe  --add-gnu-debuglink=%%f.sym %%f
        popd
    )
)