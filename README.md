# Pyjion

## Pre-Reqs (all of which need to be reachable on your PATH)
* For CPython
  * [Mercurial](https://mercurial.selenic.com/)
  * [TortoiseSVN](http://tortoisesvn.net/) (required to get external dependencies)
* For CoreCLR
  * [Git](http://www.git-scm.com/)
  * [CMake](http://www.cmake.org/)
* [Visual Studio](https://www.visualstudio.com/)

## Getting Started

### Run `GetDeps.bat`

This will use git to download CoreCLR and hg to download Python.  It will also patch Python to have JIT support.

### Run `BuildDeps.cmd`

This will build CoreCLR and Python.

### Running
* For a debug build
  1. Copy the file `Pyjion\x64\Debug\pyjit.dll`
  2. Launch `python_d.exe`
* For a release build
  1. Copy the file `Pyjion\x64\Release\pyjit.dll`
  2. Launch `python.exe`


### Known Issues
You'll need to run `git clean -d -f -x` in CoreCLR when switching between release and debug builds.