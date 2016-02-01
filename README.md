# Pyjion
Designing a JIT API for CPython

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
  2. Build the solution
* Run `CopyFiles.bat` to copy files to key locations

#### Testing
  1. Run `x64\Debug\Test.exe`
  2. Run `x64\Debug\Tests.exe`

#### Running
1. Copy `x64\Debug\pyjit.dll` to `Python\PCbuild\amd64\` (initially done by `CopyFiles.bat`, so only do as necessary after rebuilding Pyjion)
2. Go into the `Python` directory and launch `python.bat`


### Known Issues
You'll need to run `git clean -d -f -x` in CoreCLR when switching between release and debug builds.

## FAQ

### What are the goals of this project?
There are three goals for this project.

1. Add a C API to CPython for plugging in a JIT
2. Develop a JIT module using [CoreCLR](https://github.com/dotnet/coreclr) utilizing the C API mentioned in goal #1
3. Develop a C++ framework

Goal #1 is to make it so that CPython have a JIT plugged in as desired (CPython
is the Python implementation you download from https://www.python.org/). That
would allow for an ecosystem of JIT implementations for Python where users can
choose the JIT that works best for their use-case. And by using CPython we hope
to have compatibility with all code that it can run (both Python code as well
as C extension modules).

Goal #2 is to develop a JIT for CPython using the JIT provided by the
[CoreCLR](https://github.com/dotnet/coreclr). It's cross-platform, liberally
licensed, and the original creator of Pyjion has a lot of experience with it.

Goal #3 is to abstract out all of the common bits required to write a JIT
implementation for CPython. The idea is to create a framework where JIT
implementations only have to worry about JIT-specific stuff like _how_ to do
addition and not _when_ to do addition.

### How do you pronounce "Pyjion"?
Like the word "pigeon". @DinoV wanted a name that had something with "Python"
-- the "Py" part -- and something with "JIT" -- the "JI" part -- and have it be
pronounceable.

### How do this compare to ...
#### [PyPy](http://pypy.org/)?
[PyPy](http://pypy.org/) is an implementation of Python with its own JIT. The
biggest difference compared to Pyjion is that PyPy doesn't support C extension
modules without modification unless they use [CFFI](cffi.readthedocs.org).
Pyjion also aims to support many JIT compilers while PyPy only supports their
own custom JIT compiler.

#### [Pyston](http://pyston.org)?
[Pyston](http://pyston.org) is an implementation of Python using
[LLVM](http://llvm.org/) as a JIT compiler. Compared to Pyjion, Pyston has
partial CPython C API support but not complete support. Pyston also only
supports LLVM as a JIT compiler.

#### [Numba](http://numba.pydata.org/)?
[Numba](http://numba.pydata.org/) is a JIT compiler for "array-oriented and
math-heavy Python code". This means that Numba is focused on scientific
computing while Pyjion tries to optimize all Python code. Numba also only
supports LLVM.

#### [IronPython](http://ironpython.net/)?
[IronPython](http://ironpython.net/) is an implementation of Python that is
implemented using [.NET](http://microsoft.com/NET). While IronPython tries to
be usable from within .NET, Pyjion does not have an compatibility with .NET.
This also means IronPython cannot use C extension modules while Pyjion can.

#### [Psyco](http://psyco.sourceforge.net/)?
[Psyco](http://psyco.sourceforge.net/) was a module that monkeypatched CPython
to add a custom JIT compiler. Pyjion wants to introduce a proper C API for
adding a JIT compiler to CPython instead of monkeypatching it. It should be
noted the creator of Psyco went on to be one of the co-founders of PyPy.

#### [Unladen Swallow](https://en.wikipedia.org/wiki/Unladen_Swallow)?
[Unladen Swallow](https://en.wikipedia.org/wiki/Unladen_Swallow) was an attempt
to make LLVM be a JIT compiler for CPython. Unfortunately the project lost
funding before finishing their work after having to spend a large amount of
time fixing issues in LLVM's JIT compiler (which has greatly improved over the
subsequent years).

### Are you going to support OS X and/or Linux?
Yes! Goals #1 and #3 are entirely platform-agnostic while goal #2 of using
CoreCLR as a JIT compiler is not an impedence to supporting OS X or Linux as
it already supports the major OSs. The only reason Pyjion doesn't directly
support Linux or OS X is entirely momentum/laziness: since the work is being
driven by Microsoft employees, it simply meant it was easier to get going on
Windows.

### Will this ever ship with CPython?
Goal #1 is explicitly to add a C API to CPython to support JIT compilers. There
is no expectation, though, to ship a JIT compiler with CPython. This is because
CPython compiles with nothing more than a C89 compiler, which allows it to run
on many platforms. But adding a JIT compiler to CPython would immediately limit
it to only the platforms that the JIT supports.

### Does this help with using CPython w/ .NET or UWP?
No.