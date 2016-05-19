.. Pyjion documentation master file, created by
   sphinx-quickstart on Thu May 19 09:26:14 2016.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Pyjion
======

Pyjion is an implementation and technical proof of concept for a JIT API for CPython. 

Project Goals
-------------

There are three goals for this project.

1. **Add a C API to CPython for plugging in a JIT** - make it so that CPython can have a JIT plugged in as desired (CPython is the Python implementation you download from https://www.python.org/). That would allow for an ecosystem of JIT implementations for Python where users can choose the JIT that works best for their use-case. And by using CPython we hope to have compatibility with all code that it can run (both Python code as well as C extension modules).
2. **Develop a JIT module using CoreCLR utilizing the C API mentioned in goal #1** -  develop a JIT for CPython using the JIT provided by the CoreCLR. It's cross-platform, liberally licensed, and the original creator of Pyjion has a lot of experience with it.
3. **Develop a C++ framework that any JIT targetting the API in goal #1 can use to make development easier** - abstract out all of the common bits required to write a JIT implementation for CPython. The idea is to create a framework where JIT implementations only have to worry about JIT-specific stuff like how to do addition and not when to do addition.

Follow the :doc:`Getting Started <gettingstarted>` guide for a walkthrough of how to use this project.

Documentation
=============

Main
----

.. toctree::
    :glob:
    :maxdepth: 3

    gettingstarted
    building
    using
