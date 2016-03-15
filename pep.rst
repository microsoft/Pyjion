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

This PEP proposes to expand CPython's C API [#c-api]_to allow for
just-in-time compilers (JITs) to participate in the execution of
Python code. Use of a JIT will be optional for CPython and no specific
JIT will be distributed with CPython itself.

Rationale
=========

Allowing for Python code to execute faster is always a laudable goal.
Another laudable goal is code maintenance. And yet a third laudable
goal is platform portability. Unfortunately these three goals can
conflict, occasionally leading to ideas that help one of these goals
having to be turned down due to the cost of the other two goals.

CPython has always held the code maintenance and portability goals
very highly. That has historically meant that if a proposal came
forward that increased CPython's speed was proposed it had to be
evaluated from the perspective of how much it complicated CPython's
code base as well as whether it would work on most operating
systems. The idea of adding a just-in-time compiler (JIT) to CPython
has always been rejected due to either portability or code
maintenance reasons.

But we believe we have a solution to the common issues with adding a
JIT to CPython. First, we do not want to add a JIT *directly*
to CPython but instead an API that *allows* a JIT to be used. This
alleviates the typical portability issue by simply not making it the
responsibility of CPython to provide a JIT that works across at least
across all major if not any platform that supports C89 + Amendment 1
(CPython's current portability requirement). Exposing an API instead
of using a specific JIT with CPython also allows for the best JIT to
be used per platform and workload. Finally, it allows for easier
experimentation by JIT authors with CPython to grow the potential
ecosystem of JITs for CPython.

The second part of our solution for overcoming traditional objections
at adding a JIT to CPython is to expose a *small* expansion of the C
API of CPython with a *simple* entrypoint at which CPython's eval loop
interacts with any potential JIT. By keeping the semantics of the
proposed JIT API small and simple it is easy to comprehend for both
CPython contributors and JIT authors as well as making the maintenance
burden extremely small on CPython contributors themselves.


Proposal
========

The overall proposal involves expanding code object, providing
function entrypoints for JITs, and changes to the eval loop of
CPython.


Expanding ``PyCodeObject``
--------------------------

Two new fields are to be added to the ``PyCodeObject`` struct
[#pycodeobject]_ along with a sentinel constant::

  struct {
     ...
     PyJittedCode *co_jitted;  /* Stores struct to JIT details. */
     PY_UINT64_T co_run_count;  /* The number of times the code object has run. */
  } PyCodeObject;

  /* Constant to represent when a JIT cannot compile a code object. */
  #define PY_JIT_FAILED 1

The ``co_jitted`` field stores a pointer to a ``PyJittedCode`` struct
which stores the details of the compiled JIT code (``PyJittedCode`` is
explained later). By adding a level of indirection for JIT compilation
details per code object minimizes the memory impact of this API when a
JIT is not used/triggered.

The ``co_run_count`` field counts the numbers of executions for the
code object. This allows for JIT compilation to only be triggered for
"hot" code objects instead of all code objects. This helps mitigate
JIT compilation overhead by only triggering compilation for heavily
used code objects.

The ``PY_JIT_FAILED`` constant is used to signify when a JIT attempted
to compile a code object and failed. This presents attempting to
compile the code object in the future. The value should be a memory
address that has nearly no chance of ever being a valid memory address
to a ``PyJittedCode`` struct.


``PyJittedCode``
----------------

XXX


Changes to ``Python/ceval.c``
-----------------------------

XXX


Implementation
==============

XXX


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
``co_compilefailed`` flag that was set by the JIT when it was unable
to compile the code object. This was eventually removed as it was
deemed unnecessary when ``co_jitted`` could hold a sentinel value for
the same purpose, eliminating the need for memory per code object.


References
==========

.. [#pyjion] Pyjion project
   (https://github.com/microsoft/pyjion)

.. [#c-api] CPython's C API
   (https://docs.python.org/3/c-api/index.html)


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
