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

#include <Python.h>
#include "pyjit.h"
#include "pycomp.h"

#ifdef WINDOWS
#include <libloaderapi.h>
typedef ICorJitCompiler* (__cdecl* GETJIT)();
#endif

HINSTANCE            g_pMSCorEE;

// Tracks types for a function call.  Each argument has a SpecializedTreeNode with
// children for the subsequent arguments.  When we get to the leaves of the tree
// we'll have a jitted code object & optimized evalutation function optimized
// for those arguments.  
struct SpecializedTreeNode {
	vector<PyTypeObject*> types;
	Py_EvalFunc addr;
	JittedCode* jittedCode;
	int hitCount;

	explicit SpecializedTreeNode(vector<PyTypeObject*>& types) : types(types) {
		addr = nullptr;
		jittedCode = nullptr;
		hitCount = 0;
	}

	~SpecializedTreeNode() {
		delete jittedCode;
	}
};


PyjionJittedCode::~PyjionJittedCode() {
	for (auto & cur : j_optimized) {
		delete cur;
	}
}

PyObject* Jit_EvalTrace(PyjionJittedCode* state, PyFrameObject *frame);
PyObject* Jit_EvalHelper(void* state, PyFrameObject*frame) {
#ifdef DUMP_TRACES
    printf("Invoking trace %s from %s line %d %p %p\r\n",
        PyUnicode_AsUTF8(frame->f_code->co_name),
        PyUnicode_AsUTF8(frame->f_code->co_filename),
        frame->f_code->co_firstlineno,
        state,
        frame
    );
#endif

    PyThreadState *tstate = PyThreadState_GET();
    if (tstate->use_tracing) {
        if (tstate->c_tracefunc != nullptr) {
            return _PyEval_EvalFrameDefault(tstate, frame, 0);
        }
    }

    if (Py_EnterRecursiveCall("")) {
        return nullptr;
    }

	frame->f_executing = 1;
    auto res = ((Py_EvalFunc)state)(nullptr, frame);

#ifdef DUMP_TRACES
    if (res != nullptr)
        printf("Returning from %s, value %s\r\n", PyUnicode_AsUTF8(frame->f_code->co_name), PyUnicode_AsUTF8(PyObject_Repr(res)));
#endif

    Py_LeaveRecursiveCall();
    frame->f_executing = 0;
    return res;
}

static Py_tss_t* g_extraSlot;

bool JitInit() {
	g_extraSlot = PyThread_tss_alloc();
	PyThread_tss_create(g_extraSlot);
#ifdef WINDOWS
	auto clrJitHandle = LoadLibrary(TEXT("clrjit.dll"));
	if (clrJitHandle == nullptr) {
	    PyErr_SetString(PyExc_RuntimeError, "Failed to load clrjit.dll, check that .NET is installed.");
        return false;
	}
	auto getJit = (GETJIT)GetProcAddress(clrJitHandle, "getJit");
	if (getJit == nullptr) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to load clrjit.dll::getJit(), check that the correct version of .NET is installed.");
        return false;
	}
#endif
	g_jit = getJit();

    g_emptyTuple = PyTuple_New(0);
    return true;
}

bool jit_compile(PyCodeObject* code) {
    // TODO : support for generator expressions
    if (strcmp(PyUnicode_AsUTF8(code->co_name), "<genexpr>") == 0) {
        return false;
    }
#ifdef DUMP_TRACES
    static int compileCount = 0, failCount = 0;
    printf("Tracing %s from %s line %d #%d (%d failures so far)\r\n",
           PyUnicode_AsUTF8(code->co_name),
           PyUnicode_AsUTF8(code->co_filename),
           code->co_firstlineno,
           ++compileCount,
           failCount);
#endif

    auto jittedCode = PyJit_EnsureExtra((PyObject *) code);

    jittedCode->j_evalfunc = &Jit_EvalTrace;
    return true;
}

AbstractValueKind GetAbstractType(PyTypeObject* type) {
    if (type == nullptr) {
        return AVK_Any;
    } else if (type == &PyLong_Type) {
        return AVK_Integer;
    }
    else if (type == &PyFloat_Type) {
        return AVK_Float;
    }
    else if (type == &PyDict_Type) {
        return AVK_Dict;
    }
    else if (type == &PyTuple_Type) {
        return AVK_Tuple;
    }
    else if (type == &PyList_Type) {
        return AVK_List;
    }
    else if (type == &PyBool_Type) {
        return AVK_Bool;
    }
    else if (type == &PyUnicode_Type) {
        return AVK_String;
    }
    else if (type == &PyBytes_Type) {
        return AVK_Bytes;
    }
    else if (type == &PySet_Type) {
        return AVK_Set;
    }
    else if (type == &_PyNone_Type) {
        return AVK_None;
    }
    else if (type == &PyFunction_Type) {
        return AVK_Function;
    }
    else if (type == &PySlice_Type) {
        return AVK_Slice;
    }
    else if (type == &PyComplex_Type) {
        return AVK_Complex;
    }
    return AVK_Any;
}

PyTypeObject* GetArgType(int arg, PyObject** locals) {
    auto objValue = locals[arg];
    PyTypeObject* type = nullptr;
    if (objValue != nullptr) {
        type = objValue->ob_type;
    }
    return type;
}

PyObject* Jit_EvalGeneric(PyjionJittedCode* state, PyFrameObject*frame) {
    auto trace = (PyjionJittedCode*)state;
    return Jit_EvalHelper((void*)trace->j_generic, frame);
}

#define MAX_TRACE 5

PyObject* Jit_EvalTrace(PyjionJittedCode* state, PyFrameObject *frame) {
	// Walk our tree of argument types to find the SpecializedTreeNode which
    // corresponds with our sets of arguments here.
    auto trace = (PyjionJittedCode*)state;
    auto tstate = PyThreadState_Get();
	SpecializedTreeNode* target = nullptr;

    // record the new trace...
    if (trace->j_optimized.size() < MAX_TRACE) {
        int argCount = frame->f_code->co_argcount + frame->f_code->co_kwonlyargcount;
        vector<PyTypeObject*> types;
        for (int i = 0; i < argCount; i++) {
            auto type = GetArgType(i, frame->f_localsplus);
            types.push_back(type);
        }
		target = new SpecializedTreeNode(types);
        trace->j_optimized.push_back(target);
	}

	if (target != nullptr && !trace->j_failed) {
		if (target->addr != nullptr) {
			// we have a specialized function for this, just invoke it
			auto res = Jit_EvalHelper((void*)target->addr, frame);
			return res;
		}

		target->hitCount++;
		// we've recorded these types before...
		// No specialized function yet, let's see if we should create one...
		if (target->hitCount >= trace->j_specialization_threshold) {
			// Compile and run the now compiled code...
			PythonCompiler jitter((PyCodeObject*)trace->j_code);
			AbstractInterpreter interp((PyCodeObject*)trace->j_code, &jitter);
			int argCount = frame->f_code->co_argcount + frame->f_code->co_kwonlyargcount;

			// provide the interpreter information about the specialized types
			for (int i = 0; i < argCount; i++) {
				auto type = GetAbstractType(GetArgType(i, frame->f_localsplus));
                interp.setLocalType(i, type);
			}

			auto res = interp.compile();
#ifdef DUMP_TRACES
			printf("Tracing %s from %s line %d (%s)\r\n",
				PyUnicode_AsUTF8(frame->f_code->co_name),
				PyUnicode_AsUTF8(frame->f_code->co_filename),
				frame->f_code->co_firstlineno,
                PyUnicode_AsUTF8(PyObject_Repr(frame->f_code->co_consts))
			);
#endif

			if (res == nullptr) {
				static int failCount;
#ifdef DUMP_TRACES
                printf("Result from compile operation indicates failure, defaulting to EFD.\r\n");
#endif
				trace->j_failed = true;
				return _PyEval_EvalFrameDefault(tstate, frame, 0);
			}

			// Update the jitted information for this tree node
			target->addr = (Py_EvalFunc)res->get_code_addr();
			assert(target->addr != nullptr);
			target->jittedCode = res;

			trace->j_il = res->get_il();
			trace->j_ilLen = res->get_il_len();
            trace->j_nativeSize = res->get_native_size();
            trace->j_generic = target->addr;
            trace->j_evalfunc = Jit_EvalGeneric;

			return Jit_EvalHelper((void*)target->addr, frame);
		}
	}

	auto res = _PyEval_EvalFrameDefault(tstate, frame, 0);
	return res;
}


PyjionJittedCode* PyJit_EnsureExtra(PyObject* codeObject) {
	auto index = (ssize_t)PyThread_tss_get(g_extraSlot);
	if (index == 0) {
		index = _PyEval_RequestCodeExtraIndex(PyjionJitFree);
		if (index == -1) {
			return nullptr;
		}

		PyThread_tss_set(g_extraSlot, (LPVOID)((index << 1) | 0x01));
	}
	else {
		index = index >> 1;
	}

	PyjionJittedCode *jitted = nullptr;
	if (_PyCode_GetExtra(codeObject, index, (void**)&jitted)) {
		PyErr_Clear();
		return nullptr;
	}

	if (jitted == nullptr) {
	    jitted = new PyjionJittedCode(codeObject);
		if (jitted != nullptr) {
			if (_PyCode_SetExtra(codeObject, index, jitted)) {
				PyErr_Clear();

				delete jitted;
				return nullptr;
			}
		}
	}
	return jitted;
}

// This is our replacement evaluation function.  We lookup our corresponding jitted code
// and dispatch to it if it's already compiled.  If it hasn't yet been compiled we'll
// eventually compile it and invoke it.  If it's not time to compile it yet then we'll
// invoke the default evaluation function.
PyObject* PyJit_EvalFrame(PyThreadState *ts, PyFrameObject *f, int throwflag) {
	auto jitted = PyJit_EnsureExtra((PyObject*)f->f_code);
	if (jitted != nullptr && !throwflag) {
		if (jitted->j_evalfunc != nullptr) {
#ifdef DUMP_TRACES
			printf("Calling %s (already jitted) from %s line %d %p (%s)\r\n",
				PyUnicode_AsUTF8(f->f_code->co_name),
				PyUnicode_AsUTF8(f->f_code->co_filename),
				f->f_code->co_firstlineno,
				jitted,
				PyUnicode_AsUTF8(PyObject_Repr(f->f_code->co_consts))
			);
#endif
			return jitted->j_evalfunc(jitted, f);
		}
		else if (!jitted->j_failed && jitted->j_run_count++ >= jitted->j_specialization_threshold) {
			if (jit_compile(f->f_code)) {
				// execute the jitted code...
#ifdef DUMP_TRACES
				printf("Calling first %s from %s line %d %p\r\n",
					PyUnicode_AsUTF8(f->f_code->co_name),
					PyUnicode_AsUTF8(f->f_code->co_filename),
					f->f_code->co_firstlineno,
					jitted
				);
#endif
				jitPassCounter++;
				return jitted->j_evalfunc(jitted, f);
			}

			// no longer try and compile this method...
            jitFailCounter++;
			jitted->j_failed = true;
		}
	}
#ifdef DUMP_TRACES
	printf("Falling to EFD %s from %s line %d %p\r\n",
		PyUnicode_AsUTF8(f->f_code->co_name),
		PyUnicode_AsUTF8(f->f_code->co_filename),
		f->f_code->co_firstlineno,
		jitted
	);
#endif

	auto res = _PyEval_EvalFrameDefault(ts, f, throwflag);
#ifdef DUMP_TRACES
	printf("Returning EFD %s from %s line %d %p\r\n",
		PyUnicode_AsUTF8(f->f_code->co_name),
		PyUnicode_AsUTF8(f->f_code->co_filename),
		f->f_code->co_firstlineno,
		jitted
	);
#endif
	return res;
}

void PyjionJitFree(void* obj) {
    free(obj);
}

static PyInterpreterState* inter(){
    return PyInterpreterState_Main();
}

static PyObject *pyjion_enable(PyObject *self, PyObject* args) {
    auto prev = _PyInterpreterState_GetEvalFrameFunc(inter());
    _PyInterpreterState_SetEvalFrameFunc(inter(), PyJit_EvalFrame);
    if (prev == PyJit_EvalFrame) {
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

static PyObject *pyjion_disable(PyObject *self, PyObject* args) {
	auto prev = _PyInterpreterState_GetEvalFrameFunc(inter());
    _PyInterpreterState_SetEvalFrameFunc(inter(), _PyEval_EvalFrameDefault);
    if (prev == PyJit_EvalFrame) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *pyjion_status(PyObject *self, PyObject* args) {
	auto prev = _PyInterpreterState_GetEvalFrameFunc(inter());
    if (prev == PyJit_EvalFrame) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *pyjion_stats(PyObject *self, PyObject* func) {
    auto res = PyDict_New();
    if (res == nullptr) {
        return nullptr;
    }

    PyDict_SetItemString(res, "failed", PyLong_FromLongLong(jitFailCounter));
    PyDict_SetItemString(res, "compiled", PyLong_FromLongLong(jitPassCounter));

    return res;
}

static PyObject *pyjion_info(PyObject *self, PyObject* func) {
	PyObject* code;
	if (PyFunction_Check(func)) {
		code = ((PyFunctionObject*)func)->func_code;
	}
	else if (PyCode_Check(func)) {
		code = func;
	}
	else {
		PyErr_SetString(PyExc_TypeError, "Expected function or code");
		return nullptr;
	}
	auto res = PyDict_New();
	if (res == nullptr) {
		return nullptr;
	}

	PyjionJittedCode* jitted = PyJit_EnsureExtra(code);

	PyDict_SetItemString(res, "failed", jitted->j_failed ? Py_True : Py_False);
	PyDict_SetItemString(res, "compiled", jitted->j_evalfunc != nullptr ? Py_True : Py_False);
	
	auto runCount = PyLong_FromLongLong(jitted->j_run_count);
	PyDict_SetItemString(res, "run_count", runCount);
	Py_DECREF(runCount);
	
	return res;
}

static PyObject *pyjion_dump_il(PyObject *self, PyObject* func) {
    PyObject* code;
    if (PyFunction_Check(func)) {
        code = ((PyFunctionObject*)func)->func_code;
    }
    else if (PyCode_Check(func)) {
        code = func;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Expected function or code");
        return nullptr;
    }

    PyjionJittedCode* jitted = PyJit_EnsureExtra(code);
    if (jitted->j_failed || jitted->j_evalfunc == nullptr)
         Py_RETURN_NONE;

    auto res = PyByteArray_FromStringAndSize(reinterpret_cast<const char *>(jitted->j_il), jitted->j_ilLen);
    if (res == nullptr) {
        return nullptr;
    }
    return res;
}

static PyObject *pyjion_dump_native(PyObject *self, PyObject* func) {
    PyObject* code;
    if (PyFunction_Check(func)) {
        code = ((PyFunctionObject*)func)->func_code;
    }
    else if (PyCode_Check(func)) {
        code = func;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Expected function or code");
        return nullptr;
    }

    PyjionJittedCode* jitted = PyJit_EnsureExtra(code);
    if (jitted->j_failed || jitted->j_evalfunc == nullptr)
        Py_RETURN_NONE;

    auto res = PyByteArray_FromStringAndSize(reinterpret_cast<const char *>(jitted->j_evalfunc), jitted->j_nativeSize);
    if (res == nullptr) {
        return nullptr;
    }
    return res;
}

static PyObject *pyjion_set_threshold(PyObject *self, PyObject* args) {
	if (!PyLong_Check(args)) {
		PyErr_SetString(PyExc_TypeError, "Expected int for new threshold");
		return nullptr;
	}

	auto newValue = PyLong_AsLongLong(args);
	if (newValue < 0) {
		PyErr_SetString(PyExc_ValueError, "Expected positive threshold");
		return nullptr;
	}

	auto prev = PyLong_FromLongLong(HOT_CODE);
	HOT_CODE = PyLong_AsLongLong(args);
	return prev;
}

static PyObject *pyjion_get_threshold(PyObject *self, PyObject* args) {
	return PyLong_FromLongLong(HOT_CODE);
}

static PyMethodDef PyjionMethods[] = {
	{ 
		"enable",  
		pyjion_enable, 
		METH_NOARGS, 
		"Enable the JIT.  Returns True if the JIT was enabled, False if it was already enabled." 
	},
	{ 
		"disable", 
		pyjion_disable, 
		METH_NOARGS, 
		"Disable the JIT.  Returns True if the JIT was disabled, False if it was already disabled." 
	},
	{
		"status",
		pyjion_status,
		METH_NOARGS,
		"Gets the current status of the JIT.  Returns True if it is enabled or False if it is disabled."
	},
	{
		"info",
		pyjion_info,
		METH_O,
		"Returns a dictionary describing information about a function or code objects current JIT status."
	},
    {
            "stats",
            pyjion_stats,
            METH_O,
            "Returns a dictionary with stats about the JIT compiler."
    },
    {
        "dump_il",
        pyjion_dump_il,
        METH_O,
        "Outputs the IL for the compiled code object."
    },
    {
        "dump_native",
        pyjion_dump_native,
        METH_O,
        "Outputs the machine code for the compiled code object."
    },
	{
		"set_threshold",
		pyjion_set_threshold,
		METH_O,
		"Sets the number of times a method needs to be executed before the JIT is triggered."
	},
	{
		"get_threshold",
		pyjion_get_threshold,
		METH_O,
		"Gets the number of times a method needs to be executed before the JIT is triggered."
	},
	{NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef pyjionmodule = {
	PyModuleDef_HEAD_INIT,
	"_pyjion",   /* name of module */
	"Pyjion - A Just-in-Time Compiler for CPython", /* module documentation, may be NULL */
	-1,       /* size of per-interpreter state of the module,
			  or -1 if the module keeps state in global variables. */
	PyjionMethods
}; 

PyMODINIT_FUNC PyInit__pyjion(void)
{
	// Install our frame evaluation function
	if (JitInit())
	    return PyModule_Create(&pyjionmodule);
	else
	    return nullptr;
}
