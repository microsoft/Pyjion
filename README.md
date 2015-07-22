# Pyjion

## Pre-Reqs
Git
Mercurial
CMake (requried for CoreCLR build)
TortoiseSVN (required for Python build to get external dependencies)
Visual Studio

## Getting Started

### Run GetDeps.bat

This will use git to download CoreCLR and hg to download Python.  It will also patch Python to have JIT support.

### Run BuildDeps.cmd

This will build CoreCLR and Python (the Python build might fail for you based upon external dependency issues...)

### Running
Copy the build Pyjion\x64\Debug\pyjit.dll or Pyjion\x64\Release\pyjit.dll
Launch python_d.exe or python.exe


### Known Issues
You'll need to run git clean -d -f -x in CoreCLR when switching between release and debug builds.