# Contributing to Pyjion

## Code of Conduct
This project's code of conduct can be found in the
[CODE_OF_CONDUCT.md file](https://github.com/Microsoft/Pyjion/blob/master/CODE_OF_CONDUCT.md)
(v1.4.0 of the http://contributor-covenant.org/ CoC).

## CLA
If your contribution is large enough you will be asked to sign the Microsoft CLA (the CLA bot will tell you if it's necessary).

## Development
### Pre-Reqs (all of which need to be reachable on your PATH)
* For CoreCLR
  * [Git](http://www.git-scm.com/)
  * [CMake](http://www.cmake.org/)
* For CPython
  * Git
  * [TortoiseSVN](http://tortoisesvn.net/) (required to get external dependencies)
* [Visual Studio](https://www.visualstudio.com/)

### Getting Started

This repository uses [Git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules), which means the best way to clone this repository is with the `--recursive` flag:

```shell
git clone --recursive https://github.com/Microsoft/Pyjion.git
```

#### Patching dependencies
Run `PatchDeps.bat` to patch Python to have JIT support and CoreCLR to disable COM support.

#### Build Dependencies
Run `BuildDeps.cmd` to build CoreCLR and Python (which includes downloading Python's dependencies).

#### Building
* From Visual Studio
  1. Open the `pyjion.sln` file
  2. Build the solution (should default to a debug build for x64)
* Using the "Developer Command Prompt for VS"
  1. Run `DebugBuild.bat`
* Run `CopyFiles.bat` to copy files to key locations

#### Testing
  1. Run `x64\Debug\Test.exe`
  2. Run `x64\Debug\Tests.exe`
  3. Run `Python\python.bat -m test -n -f Tests\python_tests.txt`

#### Running
1. Copy `x64\Debug\pyjit.dll` to `Python\PCbuild\amd64\` (initially done by `CopyFiles.bat`, so only do as necessary after rebuilding Pyjion)
2. Go into the `Python` directory and launch `python.bat`

#### Faster development loop
Once you have done the above steps you can avoid running them again during development in most situations.
The commands discussed in this section assume they are run from a Developer Command Prompt for VS for access to `msbuild`.

The `BuildDebugPython.bat` file will build CPython in debug mode for x64 as well as copy the appropriate files where they need to be to run Pyjion's tests.
The `DebugBuild.bat` file will build Pyjion in debug mode for x64 and copy the appropriate files to CPython for use in testing.

### Known Issues
You'll need to run `git clean -d -f -x` in CoreCLR when switching between release and debug builds.
