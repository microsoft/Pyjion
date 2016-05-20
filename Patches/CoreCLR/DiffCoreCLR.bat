@echo off
REM ########################################################
echo Gathering changes to disable COM interop support in CoreCLR...
pushd ..\..\CoreCLR
git diff v1.0.0-rc2 src\inc\utilcode.h > ..\Patches\CoreCLR\src\inc\utilcode.h
git diff v1.0.0-rc2 src\utilcode\CMakeLists.txt > ..\Patches\CoreCLR\src\utilcode\CMakeLists.txt
git diff v1.0.0-rc2 src\utilcode\util.cpp > ..\Patches\CoreCLR\src\utilcode\util.cpp
git diff v1.0.0-rc2 src\CMakeLists.txt > ..\Patches\CoreCLR\src\CMakeLists.txt
git diff v1.0.0-rc2 build.cmd > ..\Patches\CoreCLR\build.cmd
git diff v1.0.0-rc2 clr.coreclr.props > ..\Patches\CoreCLR\clr.coreclr.props
git diff v1.0.0-rc2 clr.defines.targets > ..\Patches\CoreCLR\clr.defines.targets
git diff v1.0.0-rc2 clr.desktop.props > ..\Patches\CoreCLR\clr.desktop.props
git diff v1.0.0-rc2 CMakeLists.txt > ..\Patches\CoreCLR\CMakeLists.txt
popd
