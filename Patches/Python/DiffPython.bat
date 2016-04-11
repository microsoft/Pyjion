@echo off
REM ########################################################
echo Gathering changes to enable JIT support in Python...
pushd ..\..\Python
git diff Include\ceval.h > ..\Patches\Python\Include\ceval.h
git diff Include\code.h > ..\Patches\Python\Include\code.h
git diff Include\dictobject.h > ..\Patches\Python\Include\dictobject.h
git diff Include\pystate.h > ..\Patches\Python\Include\pystate.h
git diff Objects\codeobject.c > ..\Patches\Python\Objects\codeobject.c
git diff Python\ceval.c > ..\Patches\Python\Python\ceval.c
git diff Python\pylifecycle.c > ..\Patches\Python\Python\pylifecycle.c
git diff Python\pystate.c > ..\Patches\Python\Python\pystate.c
popd
