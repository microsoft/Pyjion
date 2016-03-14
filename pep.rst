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

XXX


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
