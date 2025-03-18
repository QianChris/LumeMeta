@echo off
C:\Android\ndk\25.0.8775105\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-objcopy.exe --only-keep-debug %1 %1.sym
C:\Android\ndk\25.0.8775105\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-strip.exe --strip-debug --strip-unneeded %1
C:\Android\ndk\25.0.8775105\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-objcopy.exe  --add-gnu-debuglink=%1.sym %1