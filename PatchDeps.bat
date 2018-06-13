@echo off
REM ########################################################
REM # Apply changes to disable COM interop support

echo Disabling COM interop support in CoreCLR...
pushd CoreCLR
git apply ..\Patches\CoreCLR\src\jit\CMakeLists.txt
git apply ..\Patches\CoreCLR\src\inc\gcinfoencoder.h
popd
