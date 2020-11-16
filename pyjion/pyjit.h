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

#ifndef PYJION_PYJIT_H
#define PYJION_PYJIT_H

#define FEATURE_NO_HOST

#include <cstdint>
#include <windows.h>
#include <cwchar>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <cfloat>
#include <share.h>
#include <intrin.h>

#include <Python.h>

#include <vector>
#include <unordered_map>

#include <frameobject.h>
#include <Python.h>


struct SpecializedTreeNode;
class PyjionJittedCode;

bool JitInit();
PyObject* PyJit_EvalFrame(PyThreadState *, PyFrameObject *, int);
PyjionJittedCode* PyJit_EnsureExtra(PyObject* codeObject);

class PyjionJittedCode;
typedef PyObject* (*Py_EvalFunc)(PyjionJittedCode*, struct _frame*);

static PY_UINT64_T HOT_CODE = 0;
static PY_UINT64_T jitPassCounter = 0;
static PY_UINT64_T jitFailCounter = 0;

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
	std::vector<SpecializedTreeNode*> j_optimized;
	Py_EvalFunc j_generic;
    uint8_t *j_il;
    unsigned int j_ilLen;
    unsigned long j_nativeSize;

	explicit PyjionJittedCode(PyObject* code) {
		j_code = code;
		j_run_count = 0;
		j_failed = false;
		j_evalfunc = nullptr;
		j_specialization_threshold = HOT_CODE;
		j_generic = nullptr;
	}

	~PyjionJittedCode();
};

bool jit_compile(PyCodeObject* code);

#endif