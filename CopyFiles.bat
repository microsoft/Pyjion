@echo off

echo Copying Python/Lib for testing purposes ...
mkdir x64\Debug\Lib
xcopy .\Python\Lib .\x64\Debug\Lib /E

echo Copying Python\PCbuild\amd64\python35_d.dll for testing purposes ...
if EXIST Python\PCbuild\amd64\python36_d.dll copy Python\PCbuild\amd64\python36_d.dll x64\Debug
if EXIST Python\PCbuild\amd64\python36.dll copy Python\PCbuild\amd64\python36.dll x64\Release

echo Copying Pyjion to add the JIT to Python ...
if EXIST x64\Release\pyjion.pyd copy x64\Release\pyjion.pyd Python\PCbuild\amd64\pyjion.pyd
if EXIST x64\Debug\pyjion_d.pyd copy x64\Debug\pyjion_d.pyd Python\PCbuild\amd64\pyjion_d.pyd
