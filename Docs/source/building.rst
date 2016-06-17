Building
========

Windows
-------

Building the dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~

With the dependencies listed in the getting started guide, you also need MSBuild, which comes with Visual Studio. The instructions here can be used with the free Community edition of Visual Studio.

Pyjion requires a series of patches to be applied to the CoreCLR and Python source before they are compiled. You must run ``PatchDeps.bat`` once and once only, if you accidentally run it twice you can reset the submodules.

To reset your submodules you can run

.. sourcecode:: bash

    cd CoreCLR
    git reset --hard
    cd Python
    git reset --hard

Once ``PatchDeps.bat`` has executed successfully, you need to build the dependencies.

Before you can build Pyjion, you need to build the dependent projects (CoreCLR and Python) that were pulled in as submodules. This is a helper script ``BuildDeps.bat`` in the root of the project to do this.

``BuildDeps.bat`` uses the build.cmd command that comes inside Visual Studio build tools, the easiest way to call this is within the **Developer Command Prompt for VS20xx**, located in your start menu.

Open the command prompt and run ``BuildDeps.bat``.

.. note::
    If you get an error saying CMake is not installed, check that you have installed it and also look at $PATH to check that ``C:\Program Files (x86)\CMake\bin`` is in your PATH list. Close the command window and open it again.
    
You should then get an output similar to the following, this will take **at least 5-10 minutes to complete** as you are building CoreCLR and Python from source::

    Commencing CoreCLR Repo build
    
    Checking pre-requisites...
    
    Commencing build of native components for Windows_NT.x64.Debug
    
    -- The C compiler identification is MSVC 19.0.23506.0
    -- The CXX compiler identification is MSVC 19.0.23506.0
    -- Check for working C compiler using: Visual Studio 14 2015 Win64
    -- Check for working C compiler using: Visual Studio 14 2015 Win64 -- works
    -- Detecting C compiler ABI info
    -- Detecting C compiler ABI info - done
    -- Check for working CXX compiler using: Visual Studio 14 2015 Win64
    -- Check for working CXX compiler using: Visual Studio 14 2015 Win64 -- works
    -- Detecting CXX compiler ABI info
    -- Detecting CXX compiler ABI info - done
    -- Detecting CXX compile features
    -- Detecting CXX compile features - done
    -- The ASM_MASM compiler identification is MSVC
    -- Found assembler: C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/x86_amd64/ml64.exe
    -- Configuring done

Now you have compiled CoreCLR and Python you can proceed to building the Pyjion project

Building the project
~~~~~~~~~~~~~~~~~~~~

Open up Pyjion.sln in Visual Studio, if you haven't built any C++ projects for a while, Visual Studio may prompt you to download and install dependencies.

Press CTRL+SHIFT+B to compile the solution, if this fails, check:
- The patches were applied by ``PatchDeps.bat``
- When you ran ``BuildDeps.bat`` there were no errors

Once the project is build you can copy the binaries to a distribution of Python (in the Python folder) using the JIT module.

This is the final step, which is completed by running ``CopyFiles.bat`` at the command line.
