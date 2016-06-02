#!/bin/bash
echo Patching CoreCLR....
pushd CoreCLR
git apply ../Patches/CoreCLR/src/inc/utilcode.h
git apply ../Patches/CoreCLR/src/utilcode/CMakeLists.txt
git apply ../Patches/CoreCLR/src/utilcode/util.cpp
git apply ../Patches/CoreCLR/src/CMakeLists.txt
git apply ../Patches/CoreCLR/build.cmd
git apply ../Patches/CoreCLR/clr.coreclr.props
git apply ../Patches/CoreCLR/clr.defines.targets
git apply ../Patches/CoreCLR/clr.desktop.props
git apply ../Patches/CoreCLR/CMakeLists.txt
popd

echo Applying changes to enable JIT support in Python...
pushd Python
git apply ../Patches/Python/Include/ceval.h
git apply ../Patches/Python/Include/code.h
git apply ../Patches/Python/Include/dictobject.h
git apply ../Patches/Python/Include/pystate.h
git apply ../Patches/Python/Objects/codeobject.c
git apply ../Patches/Python/Python/ceval.c
git apply ../Patches/Python/Python/pylifecycle.c
git apply ../Patches/Python/Python/pystate.c
popd
