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
git reset --hard f1d4fb9741887eefcf35981fe6c8d4807b1a5f7d

REM ########################################################
REM # Apply changes to disable COM interop support

echo Disabling COM interop support...

git apply ..\coreclr.diff
popd

REM ########################################################
REM # Clone CPython code base

echo Getting CPython...
hg clone -r 96609 https://hg.python.org/cpython/ Python

REM ########################################################
REM # Apply changes to integrate JIT support into CPython

echo Applying changes to enable JIT support...
pushd Python
hg import --no-commit ..\python.diff 
popd
