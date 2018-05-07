@echo off
REM ########################################################
REM # Apply changes to disable COM interop support

echo Disabling COM interop support in CoreCLR...
pushd CoreCLR
git apply ..\Patches\CoreCLR\src\inc\utilcode.h
git apply ..\Patches\CoreCLR\src\utilcode\CMakeLists.txt
git apply ..\Patches\CoreCLR\src\utilcode\longfilepathwrappers.cpp
git apply ..\Patches\CoreCLR\src\utilcode\util.cpp
git apply ..\Patches\CoreCLR\src\CMakeLists.txt
git apply ..\Patches\CoreCLR\build.cmd
git apply ..\Patches\CoreCLR\clr.coreclr.props
git apply ..\Patches\CoreCLR\clr.defines.targets
git apply ..\Patches\CoreCLR\clr.desktop.props
git apply ..\Patches\CoreCLR\CMakeLists.txt
popd
