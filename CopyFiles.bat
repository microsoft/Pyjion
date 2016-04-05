@echo off

echo Copying Python/Lib for testing purposes ...
mkdir x64\Debug\Lib
xcopy .\Python\Lib .\x64\Debug\Lib /E

echo Copying Python\PCbuild\amd64\python35_d.dll for testing purposes ...
copy Python\PCbuild\amd64\python35_d.dll x64\Debug

echo Copying x64\Debug\pyjit.dll to add the JIT to Python ...
copy x64\Debug\pyjit.dll Python\PCbuild\amd64
