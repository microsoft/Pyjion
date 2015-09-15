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

### Download dependencies
Run `GetDeps.bat` to use git to download [CoreCLR](https://github.com/dotnet/coreclr) and hg to download [Python](https://hg.python.org/cpython).  It will also patch Python to have JIT support and CoreCLR to disable COM support.

### Build Dependencies
Run `BuildDeps.cmd` to build CoreCLR and Python (which includes downloading Python's dependencies).

### Building
* From Visual Studio
  1. Open the `pyjion.sln` file
  2. Build the solution

### Testing
* From Visual Studio
  1. Copy `Python\PCbuild\amd64\python36_d.dll` to `x64\Debug\`
  2. Set the `Test` solution as the StartUp project
  3. Run the solution (i.e., press F5)

### Running
1. Copy `pyjit.dll` to `Python\PCbuild\amd64\'
  - For a debug build, copy the file `x64\Debug\pyjit_d.dll`
  - For a release build, copy the file `x64\Release\pyjit.dll`
2. Go into the `Python\` directory and launch `python.bat`


### Known Issues
You'll need to run `git clean -d -f -x` in CoreCLR when switching between release and debug builds.