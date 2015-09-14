# Pyjion

## Pre-Reqs
* For CPython
  * [Mercurial](https://mercurial.selenic.com/)
  * [TortoiseSVN](http://tortoisesvn.net/) (required to get external dependencies)
* For CoreCLR
  * [Git](http://www.git-scm.com/)
  * [CMake](http://www.cmake.org/)
* [Visual Studio](https://www.visualstudio.com/)

## Getting Started

### Run `GetDeps.bat`

This will use git to download [CoreCLR](https://github.com/dotnet/coreclr) and hg to download [Python](https://hg.python.org/cpython).  It will also patch Python to have JIT support and CoreCLR to disable COM support.

### Run `BuildDeps.cmd`

This will build CoreCLR and Python. If the Python build fails, then check that you have the necessary [build dependencies](https://docs.python.org/devguide/setup.html#build-dependencies).

### Running
* For a debug build
  1. Copy the file `Pyjion\x64\Debug\pyjit.dll`
  2. Launch `python_d.exe`
* For a release build
  1. Copy the file `Pyjion\x64\Release\pyjit.dll`
  2. Launch `python.exe`


### Known Issues
You'll need to run `git clean -d -f -x` in CoreCLR when switching between release and debug builds.