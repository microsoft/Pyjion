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

XXX


Implementation
==============

XXX


Open Issues
===========

XXX


Rejected Ideas
==============

XXX


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
