# Pyjion
A JIT for Python based upon CoreCLR

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

## Motivation
The general motivation for this project is to make the CPython VM for
Python 3 be faster. To accomplish this the project is attempting to
add a JIT interface to CPython along with a proof-of-concept JIT.

### Designing a JIT interface for CPython
One key aspect of this project is to design a C API for CPython that
will allow for plugging a JIT into the CPython VM. This should allow
for excellent compatibility with with C extension modules which can
be important to certain Python sub-communities such as the scientific
computing community. It also allows for leveraging a well-tested code
base in the form of CPython instead of having to re-implement all of
Python's semantics. It will also hopefully lead to a proliferation of
JIT runtimes for Python, allowing people to choose a JIT that best
fits their workload.

When the word "JIT" and "Python" are mentioned together, people
typically think of [PyPy](http://pypy.org/). Compared to Pyjion, PyPy
doesn't natively support CPython C extension modules (support can be
added if an extension module uses
[CFFI](https://cffi.readthedocs.org)). This incompatibility can be
critical in fields like scientific computing where a large body of
code pre-dates Python and exists in other languages such as C, C++,
and Fortran (hence one of the reasons that
[NumPyPy](https://bitbucket.org/pypy/numpy) is being developed
instead of using [NumPy](http://www.numpy.org/) directly).

There is also [Pyston](http://pyston.org/) which is attempting to add
a JIT to CPython. But they are targetting Python 2.7 as a language
target instead of Python 3. They are also not trying to design a JIT
API for CPython itself, but instead have forked CPython.

#### JIT framework for Python
A side-effect of designing a JIT interface for CPython is it also
allows for designing a JIT framework for Python. The hope is that if
the JIT API that is being developed is accepted then this project
will spawn a C++ framework to handle common JIT needs such as type
inference. This would allow for other projects to focus on JIT code
emission and not on common needs that all JITs would have.

### Proof-of-concept JIT using CoreCLR
This project is initially using
[CoreCLR](https://github.com/dotnet/coreclr) as the JIT runtime to
verify that the JIT API being developed for CPython covers the needs
a JIT may need. It also allows us to try out the CoreCLR JIT to see
if its performance characteristics work for Python.