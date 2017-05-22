/*
 * The MIT License (MIT)
 *
 * Copyright (c) Microsoft Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef PYJIT_H
#define PYJIT_H

#define FEATURE_NO_HOST
#define USE_STL
#include <stdint.h>
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <float.h>
#include <share.h>
#include <cstdlib>
#include <intrin.h>

#include <Python.h>

#include <vector>
#include <unordered_map>

#include <frameobject.h>
#include <Python.h>

class PyjionJittedCode;

extern "C" __declspec(dllexport) void JitInit();
extern "C" __declspec(dllexport) PyObject *PyJit_EvalFrame(PyFrameObject *, int);
extern "C" __declspec(dllexport) PyjionJittedCode* PyJit_EnsureExtra(PyObject* codeObject);

typedef PyObject* (*Py_EvalFunc)(void*, struct _frame*);

void PyjionJitFree(void* obj);
/* Jitted code object.  This object is returned from the JIT implementation.  The JIT can allocate
a jitted code object and fill in the state for which is necessary for it to perform an evaluation. */

class PyjionJittedCode {
public:
	PY_UINT64_T j_run_count;
	bool j_failed;
	Py_EvalFunc j_evalfunc;
	void* j_evalstate;          /* opaque value, allows the JIT to track any relevant state */
	PY_UINT64_T j_specialization_threshold;

	PyjionJittedCode() {
		this->j_run_count = 0;
		this->j_failed = false;
		this->j_evalfunc = nullptr;
		this->j_evalstate = nullptr;
		this->j_specialization_threshold = 500;
	}
};
__declspec(dllexport) bool jit_compile(PyCodeObject* code);

#endif