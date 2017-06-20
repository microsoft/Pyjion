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
#include <wchar.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <float.h>
//#include <share.h>
#include <cstdlib>
//#include <intrin.h>

#include <Python.h>
#include <frameobject.h>

#include <vector>
#include <unordered_map>


 //#define NO_TRACE
 //#define TRACE_TREE

struct SpecializedTreeNode;
class PyjionJittedCode;

extern "C" __declspec(dllexport) void JitInit();
extern "C" __declspec(dllexport) PyObject *PyJit_EvalFrame(PyFrameObject *, int);
extern "C" __declspec(dllexport) PyjionJittedCode* PyJit_EnsureExtra(PyObject* codeObject);

class PyjionJittedCode;
typedef PyObject* (*Py_EvalFunc)(PyjionJittedCode*, struct _frame*);

static PY_UINT64_T HOT_CODE = 20000;

void PyjionJitFree(void* obj);

/* Jitted code object.  This object is returned from the JIT implementation.  The JIT can allocate
a jitted code object and fill in the state for which is necessary for it to perform an evaluation. */

class PyjionJittedCode {
public:
	PY_UINT64_T j_run_count;
	bool j_failed;
	Py_EvalFunc j_evalfunc;
	PY_UINT64_T j_specialization_threshold;
	PyObject* j_code;
#ifdef TRACE_TREE
	SpecializedTreeNode* funcs;
#else
	std::vector<SpecializedTreeNode*> j_optimized;
#endif
	Py_EvalFunc j_generic;

	PyjionJittedCode(PyObject* code) {
		j_code = code;
		j_run_count = 0;
		j_failed = false;
		j_evalfunc = nullptr;
		j_specialization_threshold = HOT_CODE;
#ifdef TRACE_TREE
		funcs = new SpecializedTreeNode();
#endif
		j_generic = nullptr;
	}

	~PyjionJittedCode();
};
__declspec(dllexport) bool jit_compile(PyCodeObject* code);

#endif