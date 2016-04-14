PEP: NNN
Title: Adding a frame evaluation API to CPython
Version: $Revision$
Last-Modified: $Date$
Author: Brett Cannon <brett@python.org>,
        Dino Viehland <dinov@microsoft.com>
Status: Draft
Type: Standards Track
Content-Type: text/x-rst
Created: DD-Mmm-YYYY
Post-History: DD-Mmm-YYYY


Abstract
========

This PEP proposes to expand CPython's C API [#c-api]_ to allow for
the specification of a per-interpreter function pointer to handle the
evaluation of frames [#pyeval_evalframeex]_. This proposal also
suggests adding a new field to code objects [#pycodeobject]_ to store
arbitrary data for use by the frame evaluation function.


Rationale
=========

Adding more flexibility to Python is always a laudable goal when it
doesn't come at a performance, maintenance, or mental comprehension
cost. One place where flexibility has been left out is in the direct
execution of Python code. While Python's C API [#c-api]_ allows for
constructing the data going into a frame object and then evaluating it
via ``PyEval_EvalFrameEx()`` [#pyeval_evalframeex]_, control over the
execution of Python code comes down to individual objects instead of a
hollistic control of execution at the frame level.

While wanting to have influence over frame evaluation may seem a bit
too low-level, it does open the possibility for things such as a JIT
to be introduced into CPython without CPython itself having to provide
one. By allowing external C code to control frame evaluation, a JIT
can participate in the execution of Python code at the key point where
evaluation occurs. This then allows for a JIT to conditionally
recompile Python bytecode to machine code as desired while still
allowing for executing regular CPython bytecode when running the JIT
is not desired. This can be accomplished by allowing interpreters to
specify what function to call to evaluate a frame. And by placing the
API at the frame evaluation level it allows for a complete view of the
execution environment of the code for the JIT.

This ability to specify a frame evaluation function also allows for
other use-cases beyond just opening CPython up to a JIT. For instance,
it would not be difficult to implement a tracing or profiling function
at the call level with this API. While CPython does provide the
ability to set a tracing or profiling function at the Python level,
this would be able to match the data collection of the profiler and
quite possibly be faster for tracing by simply skipping per-line
tracing support.

It also opens up the possibility of debugging where the frame
evaluation function only performs special debugging work when it
detects it is about to execute a specific code object. In that
instance the bytecode could be theoretically rewritten to inject a
breakpoint function call at the proper point for help in debugging all
while not having to do a heavy-handed approach as required by
``sys.settrace()``.

To help facilitate this JIT use-case, we are also proposing the adding
of a "scratch space" on code objects via a new field. This will allow
per-code object data to be stored with the code object itself for easy
retrieval by the frame evaluation function as necessary. The field
itself will simply be a ``PyObject *`` type so that any data stored in
the field will participate in normal object memory management.


Proposal
========

All proposed C API changes below will not be part of the stable ABI.


Expanding ``PyCodeObject``
--------------------------

One field is to be added to the ``PyCodeObject`` struct
[#pycodeobject]_::

  typedef struct {
     ...
     PyObject *co_extra;  /* "Scratch space" for the code object. */
  } PyCodeObject;

The ``co_extra`` will be ``NULL`` by default and will not be used by
CPython itself. Third-party code is free to use the field as desired.
The field will be freed like all other fields on ``PyCodeObject``
during deallocation using ``Py_XDECREF()``.

There is no attempt to try and have the field work with multiple
third-party projects simultaneously to simplify usage and to minimize
over-extending ``PyCodeObject`` with more fields than necessary (and
hence raising memory usage). If two or more projects wish to use the
field it will be up to them to coordinate the multiple uses of the
field (e.g. using a dictionary in the field, creating a common object
type that can be shared, etc.).


Expanding ``PyInterpreterState``
--------------------------------

The entrypoint for the frame evalution function is per-interpreter::

  // Same type signature as PyEval_EvalFrameEx().
  typedef PyObject* (__stdcall *PyFrameEvalFunction)(PyFrameObject*, int);

  typedef struct {
      ...
      PyFrameEvalFunction eval_frame;
  } PyInterpreterState;

By default, the ``eval_frame`` field will be initialized to a function
pointer that represents what ``PyEval_EvalFrameEx()`` currently is
(called ``PyEval_EvalFrameStandard()``, discussed later in this PEP).
Third-party code may then set their own frame evaluation function
instead to control the execution of Python code. Pointer comparison
can be used to detect if the field is set to
``PyEval_EvalFrameStandard()`` and thus has not been mutated yet.


Changes to ``Python/ceval.c``
-----------------------------

``PyEval_EvalFrameEx()`` [#pyeval_evalframeex]_ as it currently stands
will be renamed to ``PyEval_EvalFrameStandard()``. The new
``PyEval_EvalFrameEx()`` will then become::

    PyObject *
    PyEval_EvalFrameEx(PyFrameObject *frame, int throwflag)
    {
        PyThreadState *tstate = PyThreadState_GET();
        return tstate->interp->eval_frame(frame, throwflag);
    }

This allows third-party code to place themselves directly in the path
of Python code execution for it to do what it desires to influence
execution.


Performance impact
==================

As this PEP is proposing an API to add pluggability, performance
impact is considered only in the case where no third-party code has
made any changes.

XXX pybench

A run of the Python benchmark suite [#py-benchmarks]_ showed no
measurable cost in performance.

In terms of memory impact, since there are typically not many CPython
interpreters executing in a single process that means the impact of
``co_extra`` being added to ``PyCodeObject`` is the only worry.
According to [#code-object-count]_, a run of the Python test suite
results in about 72,395 code objects being created. On a 64-bit
CPU that would result in 4,633,280 bytes of extra memory being used if
all code objects were alive at once.


Example Usage
=============

The Pyjion project [#pyjion]_ has used this proposed API to implement
a JIT for CPython using the CoreCLR's JIT [#coreclr]_. Each code
object has its ``co_extra`` field set to a ``PyjionJittedCode`` object
which stores four pieces of information:

1. Execution count
2. A boolean representing whether a previous attempt to JIT failed
3. A function pointer to a trampoline (which can be type tracing or not)
4. A void pointer to any JIT-compiled machine code

The frame evaluation function has (roughly) the following algorithm::

    def eval_frame(frame, throw_flag):
        pyjion_code = frame.code.co_extra
        if not pyjion_code:
            frame.code.co_extra = PyjionJittedCode()
        elif not pyjion_code.jit_failed:
            if not pyjion_code.jit_code:
                return pyjion_code.eval(pyjion_code.jit_code, frame)
            elif pyjion_code.exec_count > 20_000:
                if jit_compile(frame):
                    return pyjion_code.eval(pyjion_code.jit_code, frame)
                else:
                    pyjion_code.jit_failed = True
        pyjion_code.exec_count += 1
        return PyEval_EvalFrameStandard(frame, throw_flag)

The key point, though, is that all of this work and logic is separate
from CPython and yet with the proposed API changes it is able to
provide a JIT that is compliant with Python semantics (as of this
writing, performance is almost equivalent to CPython without the new
API). This means there's nothing technically preventing others from
implementing their own JITs for CPython by utilizing the proposed API.


Other JITs
----------

It should be mentioned that the Pyston team was consulted on an
earlier version of this PEP that was more JIT-specific and they were
not interested in utilizing the changes proposed. Because they want
control over memory layout they had no interest in directly supporting
CPython itself. An informal discusion with a developer on the PyPy
team led to a similar comment.


XXX Numba?
XXX PTVS?


Implementation
==============

A set of patches implementing the proposed API is available through
the Pyjion project [#pyjion]_. In its current form it does do more
changes to CPython than just this proposed API, but that is for ease
of development instead of strict requirements to accomplish its goals.


Open Issues
===========

Provisionally accept the proposed changes
-----------------------------------------

While PEP 411 introduced the concept of provisionally accepted
packages in Python's standard library, the concept has yet to be
applied to CPython's C API. Due to the unknown payoff from adding this
API to CPython, it may make sense to provisionally accept this PEP
with a goal to validate its usefulness based on whether usage emerges.


Make the proposed API a compile-time option
-------------------------------------------

While the API is small and seems to show no performance impact beyond
a slight increase in memory usage, some my view it as still too costly
to have available by default.


Rejected Ideas
==============

A JIT-specific C API
--------------------

Originally this PEP was going to propose a much larger API change
which was more JIT-specific. After soliciting feedback from the Numba
team [#numba]_, though, it became clear that the API was unnecessarily
large. The realization was made that all that was truly needed was the
opportunity to provide a trampoline function to handle execution of
Python code that had been JIT-compiled and a way to attach that
compiled machine code along with other critical data to the
corresponding Python code object. Once it was shown that there was no
loss in functionality or in performance while minimizing the API
changes required, the proposal was changed to its current form.


References
==========

.. [#pyjion] Pyjion project
   (https://github.com/microsoft/pyjion)

.. [#c-api] CPython's C API
   (https://docs.python.org/3/c-api/index.html)

.. [#pycodeobject] ``PyCodeObject``
   (https://docs.python.org/3/c-api/code.html#c.PyCodeObject)

.. [#coreclr] .NET Core Runtime (CoreCLR)
   (https://github.com/dotnet/coreclr)

.. [#pyeval_evalframeex] ``PyEval_EvalFrameEx()``
   (https://docs.python.org/3/c-api/veryhigh.html?highlight=pyframeobject#c.PyEval_EvalFrameEx)

.. [#pycodeobject] ``PyCodeObject``
   (https://docs.python.org/3/c-api/code.html#c.PyCodeObject)

.. [#numba] Numba
   (http://numba.pydata.org/)

.. [#code-object-count] [Python-Dev] Opcode cache in ceval loop
   (https://mail.python.org/pipermail/python-dev/2016-February/143025.html)

.. [#py-benchmarks] Python benchmark suite
   (https://hg.python.org/benchmarks)

.. [#pyston] Pyston
   (http://pyston.org)

.. [#pypy] PyPy
   (http://pypy.org/)


Copyright
=========

This document has been placed in the public domain.



..
   Local Variables:
   mode: indented-text
   indent-tabs-mode: nil
   sentence-end-double-space: t
   fill-column: 70
   coding: utf-8
   End:
