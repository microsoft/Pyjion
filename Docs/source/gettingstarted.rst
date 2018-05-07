Getting Started
===============

This project depends on submodules for Python, the CoreCLR project and Test/Catch

To recursively clone the project use

.. sourcecode:: bash

  git clone --recursive https://github.com/microsoft/Pyjion.git

If you did not clone the repository with --recursive, you can still download submodules within git.

.. sourcecode:: bash

  git pull --recurse-submodules

Installing dependencies
-----------------------

Before you build the project, you need to install dependencies and have them avaialable in your PATH.

- `CMake`_ - You need CMake installed before building


Building
--------

Follow the :doc:`building guide <building>` to build the binaries to be installed into your Python distribution.


Trying it out
-------------

Once you have build the Python distribution with the JIT module loaded using the instructions in the :doc:`building guide <building>` you can simply
enter the Python CLI by running ``Python\python.bat`` and start using the Python 3.5.1 distribution as you would normally.

.. _`CMake`: https://cmake.org/download/
