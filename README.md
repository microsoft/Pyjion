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
1. Copy `Python\PCbuild\amd64\python36_d.dll` to `x64\Debug\`
2. Copy `Python\Lib\` to `x64\Debug\`
3. Run the tests
  * From Visual Studio
    4. Set the `Test` solution as the StartUp project
    5. Run the solution (i.e., press F5)
  * From Powershell
    4. Run `x64\Debug\Test.exe`

### Running
1. Copy `x64\Debug\pyjit.dll` to `Python\PCbuild\amd64\'
2. Go into the `Python\` directory and launch `python.bat`


### Known Issues
You'll need to run `git clean -d -f -x` in CoreCLR when switching between release and debug builds.