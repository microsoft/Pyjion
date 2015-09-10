@echo off

set __BuildArch=x64
set __BuildType=Debug


:Arg_Loop
if "%1" == "" goto ArgsDone
if /i "%1" == "/?" goto Usage
if /i "%1" == "x64"    (set __BuildArch=x64&&shift&goto Arg_Loop)

if /i "%1" == "Debug"    (set __BuildType=Debug&shift&goto Arg_Loop)
if /i "%1" == "Release"   (set __BuildType=Release&shift&goto Arg_Loop)

echo Invalid commandline argument: %1
goto Usage

:ArgsDone

:BuildCoreCLR

pushd coreclr
call build.cmd %__BuildArch% %__BuildType%
IF ERRORLEVEL 1 goto Error
popd

mkdir ..\Libs\%__BuildType%\%__BuildArch%\
copy bin\obj\Windows_NT.%__BuildArch%.%__BuildType%\src\jit\%__BuildType%\clrjit.lib ..\Libs\%__BuildType%\%__BuildArch%\
copy bin\obj\Windows_NT.%__BuildArch%.%__BuildType%\src\utilcode\dyncrt\%__BuildType%\utilcode.lib ..\Libs\%__BuildType%\%__BuildArch%\
copy bin\obj\Windows_NT.%__BuildArch%.%__BuildType%\src\gcinfo\%__BuildType%\gcinfo.lib ..\Libs\%__BuildType%\%__BuildArch%\
:BuildPython
cd Python\PCBuild

call get_externals.bat
IF ERRORLEVEL 1 goto Error
call .\build.bat -c %__BuildType% -p %__BuildArch__%
IF ERRORLEVEL 1 goto Error

if /i "%__BuildArch%" == "x64" set arch=amd64
set SUFFIX=
if /i "%__BuildType%" == "Debug" set SUFFIX=_d

copy %arch%\python36%SUFFIX%.lib ..\..\Libs\%__BuildType%\%__BuildArch%\


:Done
echo All built...
exit /b 0

:Error
echo something failed to build...
exit /b 1

:Usage
echo.
echo Usage:
echo %0 BuildArch BuildType where:
echo.
echo BuildArch can be: x64
echo BuildType can be: Debug, Release
echo Clean - optional argument to force a clean build.
echo linuxmscorlib - Build mscorlib for Linux
echo osxmscorlib - Build mscorlib for OS X
exit /b 1
