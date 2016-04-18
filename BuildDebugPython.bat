@echo off

echo Building CPython in debug mode for x64 ...
pushd Python\PCBuild

call get_externals.bat
call .\build.bat -c Debug -p x64

echo Copying .lib and .dll files to appropriate places ...
copy amd64\python35_d.lib ..\..\Libs\Debug\x64\
copy amd64\python35_d.dll ..\..\x64\Debug

popd