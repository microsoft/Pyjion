PEP: NNN
Title: Adding a JIT entrypoint API to CPython
Version: $Revision$
Last-Modified: $Date$
Author: Brett Cannon <brett@python.org>,
        Dino Viehland <dinov@microsoft.com>
Status: Draft
Type: Standards Track
Content-Type: text/x-rst
Created: NN-Lll-NNNN
Post-History: NN-Lll-NNNN


Abstract
========

This PEP proposes to expand CPython's C API [#c-api]_ to allow for
just-in-time compilers (JITs) to participate in the execution of
Python code. The use of a JIT will be optional for CPython and no
specific JIT will be distributed with CPython itself.

Rationale
=========

Allowing for Python code to execute faster is always a laudable goal.
Another laudable goal is code maintenance. And yet a third laudable
goal is platform portability. Unfortunately these three goals can
conflict, occasionally leading to ideas that help one of these goals
having to be turned down due to the cost of the other two goals.

CPython has always held the code maintenance and portability goals
very highly. That has historically meant that if a proposal came
forward that increased CPython's speed it had to be evaluated from the
perspective of how much it complicated CPython's code base as well as
whether it would work on most operating systems. The idea of adding a
just-in-time compiler (JIT) to CPython has always been rejected due to
either portability and/or code maintenance reasons.

But we believe we have a solution to the common issues with adding a
JIT to CPython. First, we do not want to add a JIT *directly*
to CPython but instead an API that *allows* a JIT to be used. This
alleviates the typical portability issue by simply not making it the
responsibility of CPython to provide a JIT that works across supported
platforms. Exposing an API instead of using a specific JIT with
CPython also allows for the best JIT to be used per platform and
workload. Finally, it allows for easier experimentation with CPython
by JIT authors. This could help facilitate an ecosystem of JITs for
CPython.

The second part of our solution for overcoming traditional objections
at adding a JIT to CPython is to expose a *small* expansion of the C
API of CPython with a *simple* entrypoint at which CPython's eval loop
interacts with any potential JIT. By keeping the semantics of the
proposed JIT API small and simple it is easy to comprehend for both
CPython contributors and JIT authors as well as making the maintenance
burden extremely small on CPython contributors themselves.


Proposal
========

Expanding ``PyCodeObject``
--------------------------

Two new fields are to be added to the ``PyCodeObject`` struct
[#pycodeobject]_ along with a constant::

  typedef struct {
     ...
     PyJittedCode *co_jitted;  /* Stores struct to JIT details. */
     PY_UINT64_T co_run_count;  /* The number of times the code object has run. */
  } PyCodeObject;

  /* Constant to represent when a JIT cannot compile a code object;
     should not compare equal to NULL. */
  const PyJittedCode *PY_JIT_FAILED = 1;

The ``co_jitted`` field stores a pointer to a ``PyJittedCode`` struct
which stores the details of the compiled JIT code (``PyJittedCode`` is
explained later). By adding a level of indirection for JIT compilation
details per code object, the memory impact of this API is minimized
when a JIT is not used/triggered.

The ``co_run_count`` field counts the numbers of executions for the
code object. This allows for JIT compilation to only be triggered for
"hot" code objects instead of all code objects. This helps mitigate
JIT compilation overhead by only triggering compilation for heavily
used code objects. (If issue #26219 [#26219]_ is accepted then the run
count per code object will already be present in CPython).

The ``PY_JIT_FAILED`` constant is used to signify when a JIT attempted
to compile a code object and failed. Setting this constant prevents
attempting to compile the code object in the future. The value should
be a memory address that has nearly no chance of ever being a valid
memory address to a ``PyJittedCode`` struct.


``PyJittedCode``
----------------

The ``PyJittedCode`` struct carries all relevant details about what
the JIT produced for the code object it is attached to::

  typedef PyObject* (__stdcall*  Py_EvalFunc)(void*, struct _frame*);

  typedef struct {
      PyObject_HEAD
      Py_EvalFunc jit_evalfunc;
      void *jit_evalstate;
  }

The ``jit_evalfunc`` points to a trampoline function provided by the
JIT to handle the execution of a Python frame and returning the
resulting ``PyObject`` (much like ``PyEval_EvalFrameEx()``
[#pyeval_evalframeex]_). A per-code object trampoline can be specified
to allow for such use-cases as tracing where a tracing-specific
trampoline could be used initially and then eventually substituted for
a non-tracing trampoline.

The ``jit_evalstate`` pointer is an opaque void pointer for the JIT
to store whatever data it needs in relation to the code object.
Most likely it will at least store the compiled code from the JIT, but
it could store other bookkeeping details such as traced type
information, etc.


Expanding ``PyInterpreterState``
--------------------------------

The entrypoints for the JIT are per-interpreter::

  typedef PyObject* (__stdcall *PyJITCompileFunction)(PyObject*);
  typedef void* (__stdcall *PyJITFreeFunction)(PyObject*);

  typedef struct {
      ...
      PyJITCompileFunction jit_compile;
      PyJITFreeFunction jit_free;
  }

The ``jit_compile`` field holds a function pointer for a function that
takes a ``PyCodeObject`` and attempts to compile it to a
``PyJittedCode`` object which will be set on the code object it was
compiled for.

The ``jit_free`` field stores a function pointer to a function which
is used to free ``PyJittedCode`` objects.


Changes to ``Python/ceval.c``
-----------------------------

The start of ``PyEval_EvalFrameEx()`` [#pyeval_evalframeex]_ will
be changed to have the following semantics::

  // Number of executions required before an attempt is made to JIT
  // a code object. JIT compilers are expected to set this to an
  // appropriate value themselves. The initial value is set to the
  // highest value possible. The initial value is set to the highest
  // value possible to minimize work in discovering that no JIT is
  // set while still allowing for JIT compilation in the future in
  // case a JIT is set up later.
  PY_UINT64_T PyJIT_HOT_CODE = 9223372036854775807;

  PyEval_EvalFrameEx(PyFrameObject *f, int throwflag)
  {
      PyCodeObject *code = f->f_code;

      if (code->co_jitted != NULL) {
          if (code->co_jitted == PY_JIT_FAILED) {
              // JIT compilation previously failed.
              return PyEval_EvalFrameEx_NoJIT(f, throwflag);
          }
          else {
              // Previously JIT compiled.
              return code->co_jitted->jit_evalfunc(code->co_jitted->jit_evalstate, f);
          }
      }

     if (code->co_run_count++ > PyJIT_HOT_CODE) {
         PyThreadState *tstate = PyThreadState_GET();
         PyInterpreterState *interp = tstate->interp;
         if (interp->jit_compile != NULL) {
             code->co_jitted = interp->jit_compile((PyObject*)code);
             if (code->co_jitted != NULL && code->co_jitted != PY_JIT_FAILED) {
                 // Compilation succeeded!
                 return code->co_jitted->jit_evalfunc(code->co_jitted->jit_evalstate, f);
             }

             // Compilation failed, so no longer try and compile this
             // method.
             code->co_compilefailed = PY_JIT_FAILED;
         }
     }

     // Fall-through; use CPython's normal eval loop.
     return PyEval_EvalFrameEx_NoJit(f, throwflag);


Implementation
==============

A set of patches implementing the proposed API is available through
the Pyjion project [#pyjion]_. The project also includes a
proof-of-concept JIT using the CoreCLR JIT [#coreclr]_ (called
RyuJIT).


Open Issues
===========

Provisionally accept the proposed changes
-----------------------------------------

While PEP 411 introduced the concept of provisionally accepted
packages in Python's standard library, the concept has yet to be
applied to CPython's C API. Due to the unknown payoff from adding this
API to CPython, it may make sense to provisionally accept this PEP
with a goal to validate its usefulness based on whether JITs emerge
which make use of the proposed API.


Make the proposed API a compile-time option
-------------------------------------------

While the API is small and performance impact of executions with no
JIT in use should be minimal (the default, no-JIT case consists of
1 ``!=`` comparison, a ``>`` comparison, and an increment), there will
always be some overhead. If the C API is deemed worth having but the
performance cost in the non-JIT case is considered too high, the API
could become a compile-time option. This is obviously not preferred as
it makes it more of a burden to use the new C API.


How to specify what JIT to use?
-------------------------------

Should JITs be an explicit ``-X`` flag for CPython? Or should a JIT
simply be like any other extension module that gets imported and it is
up to the module to register the necessary functions during module
initialization?



Rejected Ideas
==============

A separate boolean to flag when a code object cannot be compiled
----------------------------------------------------------------

In the first proof-of-concept of the proposed API there was a
``co_compilefailed`` flag on code objects that was set by the JIT when
it was unable to compile the code object. This was eventually removed
as it was deemed unnecessary when ``co_jitted`` could be assigned a
constant value for the same purpose, eliminating the need for memory
per code object just for this flag.


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
   (https://docs.python.org/3/c-api/veryhigh.html#c.PyEval_EvalFrameEx)

.. [#26219] Issue #26219: implement per-opcode cache in ceval
   (http://bugs.python.org/issue26219)


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
