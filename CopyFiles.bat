@echo off

echo Copying Python/Lib for testing purposes ...
mkdir x64\Debug\Lib
xcopy .\Python\Lib .\x64\Debug\Lib /E

echo Copying Python\PCbuild\amd64\python36_d.dll for testing purposes ...
if EXIST Python\PCbuild\amd64\python36_d.dll copy Python\PCbuild\amd64\python36_d.dll x64\Debug
if EXIST Python\PCbuild\amd64\python36.dll copy Python\PCbuild\amd64\python36.dll x64\Release

REM echo Copying Libs\Debug\x64 for testing purposes
REM if EXIST Libs\Debug\x64\clrjit.lib copy Libs\Debug\x64\clrjit.lib x64\Debug\clrjit.lib
REM REM if EXIST Libs\Debug\x64\clrjit.dll copy Libs\Debug\x64\clrjit.dll x64\Debug\clrjit.dll
REM if EXIST Libs\Debug\x64\gcinfo.lib copy Libs\Debug\x64\gcinfo.lib x64\Debug\gcinfo.lib
REM if EXIST Libs\Debug\x64\utilcode.lib copy Libs\Debug\x64\utilcode.lib x64\Debug\utilcode.lib



echo Copying Pyjion to add the JIT to Python ...
if EXIST x64\Release\pyjion.pyd copy x64\Release\pyjion.pyd Python\PCbuild\amd64\pyjion.pyd
if EXIST x64\Debug\pyjion_d.pyd copy x64\Debug\pyjion_d.pyd Python\PCbuild\amd64\pyjion_d.pyd
