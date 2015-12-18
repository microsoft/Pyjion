@echo off
REM ########################################################
REM # This script downloads CoreCLR and the CPython code bases
REM # so that you can build them and use them with Pyjion.
REM # These are locked back to specific versions currently.

echo Grabbing CoreCLR...

REM ########################################################
REM # Clone CoreCLR code base
git clone https://github.com/dotnet/coreclr CoreCLR
pushd CoreCLR
git reset --hard b16ff5935ff9df3211798f18f0c951666ae27774

REM ########################################################
REM # Apply changes to disable COM interop support

echo Disabling COM interop support...
git apply ..\Patches\CoreCLR\src\inc\utilcode.h
git apply ..\Patches\CoreCLR\src\utilcode\CMakeLists.txt
git apply ..\Patches\CoreCLR\src\utilcode\util.cpp
git apply ..\Patches\CoreCLR\src\CMakeLists.txt
git apply ..\Patches\CoreCLR\build.cmd
git apply ..\Patches\CoreCLR\clr.coreclr.props
git apply ..\Patches\CoreCLR\clr.defines.targets
git apply ..\Patches\CoreCLR\clr.desktop.props
git apply ..\Patches\CoreCLR\CMakeLists.txt
popd

REM ########################################################
REM # Clone CPython code base

echo Getting CPython...
git clone https://github.com/python/cpython.git Python
pushd Python
git reset --hard efe0e11c78f890146375f1d4cbed4b513cdffa3c

REM ########################################################
REM # Apply changes to integrate JIT support into CPython

echo Applying changes to enable JIT support...
git apply ..\Patches\python.diff
popd
