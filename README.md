Pyjion - A JIT for Python based upon CoreCLR
=======

## Pre-Reqs (all of which need to be reachable on your PATH)
* For CoreCLR
  * [Git](http://www.git-scm.com/)
  * [CMake](http://www.cmake.org/)
* For CPython
  * Git
  * [TortoiseSVN](http://tortoisesvn.net/) (required to get external dependencies)
* [Visual Studio](https://www.visualstudio.com/)

## Getting Started

This repository uses [Git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules), which means the best way to clone this repository is with the `--recursive` flag:

```shell
git clone --recursive https://github.com/Microsoft/Pyjion.git
```

### Patching dependencies

Run `PatchDeps.bat` to patch Python to have JIT support and CoreCLR to disable COM support.

### Build Dependencies
Run `BuildDeps.cmd` to build CoreCLR and Python (which includes downloading Python's dependencies).

### Building
* From Visual Studio
  1. Open the `pyjion.sln` file
  2. Build the solution
* Run `CopyFiles.bat` to copy files to key locations

### Testing
* From Visual Studio
  4. Set the `Test` solution as the StartUp project
  5. Run the solution (i.e., press F5)
* From Powershell
  4. Run `x64\Debug\Test.exe`

If the output window closes and return an exit code of 0 then the tests passed.

### Running
1. Copy `x64\Debug\pyjit.dll` to `Python\PCbuild\amd64\` (initially done by `CopyFiles.bat`, so only do as necessary after rebuilding Pyjion)
2. Go into the `Python` directory and launch `python.bat`


### Known Issues
You'll need to run `git clean -d -f -x` in CoreCLR when switching between release and debug builds.
