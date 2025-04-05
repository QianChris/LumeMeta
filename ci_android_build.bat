@echo off
cls
SETLOCAL EnableDelayedExpansion
if "%1"=="" (
    set SOURCE_PATH=%CD%
) else (
    set SOURCE_PATH=%1
)

if NOT DEFINED CMAKE_MAKE_PROGRAM (
    echo Warning: CMAKE_MAKE_PROGRAM not set using "ninja" from path.
    WHERE /Q ninja.exe
    if %errorlevel% neq 0 (
        echo ninja.exe not found.
        exit /b 1
    )
    rem try to run it
    ninja --version
    set CMAKE_MAKE_PROGRAM=ninja
) else (
    rem remove all quotes...
    Set _somevariable=%CMAKE_MAKE_PROGRAM:"=%
    rem then add the wrappers back. to support both quoted and non quoted nicely.
    set CMAKE_MAKE_PROGRAM="!_somevariable!"
    echo Running "ninja" command !CMAKE_MAKE_PROGRAM!
    if NOT EXIST !CMAKE_MAKE_PROGRAM! (
        echo "ninja" not found.
        exit /b 1
    )
    rem try to run it
    !CMAKE_MAKE_PROGRAM! --version
)
if NOT DEFINED CMAKE_COMMAND (
    echo Warning: CMAKE_COMMAND not set using CMAKE from path.
    WHERE /Q cmake.exe
    if %errorlevel% neq 0 (
        echo cmake.exe not found.
        exit /b 1
    )
    rem try to run it
    cmake --version
    set CMAKE_COMMAND=cmake
) else (
    rem remove all quotes...
    Set _somevariable=%CMAKE_COMMAND:"=%
    rem then add the wrappers back. to support both quoted and non quoted nicely.
    set CMAKE_COMMAND="!_somevariable!"

    echo Running CMake command !CMAKE_COMMAND!
    if NOT EXIST !CMAKE_COMMAND! (
        echo CMake not found.
        exit /b 1
    )
    rem try to run it.. check version
    !CMAKE_COMMAND! --version
    if %errorlevel% neq 0 (
        echo CMake not found.
        exit /b 1
    )
)
echo:
rem build lume metaobject repo.
rem copy api folder. and binaries
echo building meta object from %SOURCE_PATH%
mkdir build_android
pushd build_android

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
echo Using NDK: %NDK_PATH% API: %API_VERSION% ABI: %ABI%
echo:
echo:

set BASE_ANDROID_OPTIONS=-DANDROID_NDK:PATH="%NDK_PATH%" -DCMAKE_ANDROID_NDK:PATH="%NDK_PATH%" -DCMAKE_TOOLCHAIN_FILE:FILEPATH="%NDK_PATH%\build\cmake\android.toolchain.cmake" -DANDROID_PLATFORM=android-%API_VERSION% -DCMAKE_SYSTEM_VERSION=%API_VERSION% -DCMAKE_SYSTEM_NAME=Android
set ARCH_ANDROID_OPTIONS=-DANDROID_ABI=%ABI% -DCMAKE_ANDROID_ARCH_ABI=%ABI%
set ANDROID_OPTIONS=%BASE_ANDROID_OPTIONS% %ARCH_ANDROID_OPTIONS% -DANDROID_STL=c++_static -DCMAKE_MAKE_PROGRAM=!CMAKE_MAKE_PROGRAM!

set RELEASE_FLAGS=-DCMAKE_BUILD_TYPE=Release -DUI_TESTS_ENABLED=false
set DEBUG_FLAGS=-DCMAKE_BUILD_TYPE=Debug -DUI_TESTS_ENABLED=false

set RELEASE_PATHS=-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH="%CD%\..\bin\release\%ABI%" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH="%CD%\..\bin\release\%ABI%"
set DEBUG_PATHS=-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH="%CD%\..\bin\debug\%ABI%" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH="%CD%\..\bin\debug\%ABI%"

set RELEASE_OPTIONS=%RELEASE_FLAGS% %RELEASE_PATHS%
set DEBUG_OPTIONS=%DEBUG_FLAGS% %DEBUG_PATHS%

set BUILD_OPTIONS=%ANDROID_OPTIONS% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
if NOT DEFINED BUILD_META_OBJECT_DEPS (
    set BUILD_OPTIONS=%BUILD_OPTIONS% -DBUILD_DEPS=true
) else (
    set BUILD_OPTIONS=%BUILD_OPTIONS% -DBUILD_DEPS=%BUILD_META_OBJECT_DEPS%
)

mkdir release
pushd release

if exist CMakeCache.txt (
    !CMAKE_COMMAND!  --build . --target clean
    if %errorlevel% neq 0 exit /b %errorlevel%
    del CMakeCache.txt
)
!CMAKE_COMMAND!  -GNinja %BUILD_OPTIONS% %RELEASE_OPTIONS% %SOURCE_PATH%
if %errorlevel% neq 0 exit /b %errorlevel%
!CMAKE_COMMAND!  --build . --target LumeMeta
if %errorlevel% neq 0 exit /b %errorlevel%
popd release
mkdir debug
pushd debug
if exist CMakeCache.txt (
    !CMAKE_COMMAND!  --build . --target clean
    if %errorlevel% neq 0 exit /b %errorlevel%
    del CMakeCache.txt
)
!CMAKE_COMMAND!  -GNinja %BUILD_OPTIONS% %DEBUG_OPTIONS% %SOURCE_PATH% 
if %errorlevel% neq 0 exit /b %errorlevel%
!CMAKE_COMMAND!  --build . --target LumeMeta
if %errorlevel% neq 0 exit /b %errorlevel%
popd debug
popd

if exist bin\release ( 
echo separate and strip debug symbols from release.
pushd bin\release\%ABI%\
echo %CD%\bin\release\%ABI%\
for %%f in (*.so) do %NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-objcopy.exe --only-keep-debug %%f %%f.sym
for %%f in (*.so) do %NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-strip.exe --strip-debug --strip-unneeded %%f
for %%f in (*.so) do %NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-objcopy.exe  --add-gnu-debuglink=%%f.sym %%f
popd
)