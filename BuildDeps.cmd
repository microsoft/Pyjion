@echo off

:: 32-bit builds not supported as they have not been tested.
set __BuildArch=x64
set __BuildType=Debug


:Arg_Loop
if "%1" == "" goto ArgsDone
if /i "%1" == "/?" goto Usage

if /i "%1" == "Debug"    (set __BuildType=Debug&shift&goto Arg_Loop)
if /i "%1" == "Release"   (set __BuildType=Release&shift&goto Arg_Loop)

echo Invalid commandline argument: %1
goto Usage

:ArgsDone

:BuildCoreCLR
pushd coreclr
call build.cmd %__BuildArch% %__BuildType% skipmscorlib skiptests
IF ERRORLEVEL 1 goto Error

mkdir ..\Libs\%__BuildType%\%__BuildArch%\
copy bin\obj\Windows_NT.%__BuildArch%.%__BuildType%\src\jit\dll\%__BuildType%\clrjit.lib ..\Libs\%__BuildType%\%__BuildArch%\
copy bin\obj\Windows_NT.%__BuildArch%.%__BuildType%\src\utilcode\dyncrt\%__BuildType%\utilcode.lib ..\Libs\%__BuildType%\%__BuildArch%\
copy bin\obj\Windows_NT.%__BuildArch%.%__BuildType%\src\gcinfo\lib\%__BuildType%\gcinfo.lib ..\Libs\%__BuildType%\%__BuildArch%\
popd

:BuildPython
:: Keep BuildDebugPython.bat in sync w/ any relevant changes made here.
pushd Python\PCBuild

call get_externals.bat
IF ERRORLEVEL 1 goto Error
call .\build.bat -c %__BuildType% -p %__BuildArch%
IF ERRORLEVEL 1 goto Error

if /i "%__BuildArch%" == "x64" set arch=amd64
set SUFFIX=
if /i "%__BuildType%" == "Debug" set SUFFIX=_d

copy %arch%\python35%SUFFIX%.lib ..\..\Libs\%__BuildType%\%__BuildArch%\
popd

:Done
echo All built...
exit /b 0

:Error
echo something failed to build...
exit /b 1

:Usage
echo.
echo Usage:
echo %0 [BuildType] where:
echo.
echo BuildType can be: Debug, Release
exit /b 1
