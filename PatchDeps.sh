#!/bin/bash
echo Patching CoreCLR....
pushd CoreCLR
git apply ../Patches/CoreCLR/src/jit/CMakeLists.txt
git apply ../Patches/CoreCLR/src/inc/gcinfoencoder.h
popd
