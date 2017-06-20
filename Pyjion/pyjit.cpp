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

#include "pyjit.h"

#include <vector>
#include "ipycomp.h"
#include "absvalue.h"
#include "absint.h"

using namespace std;

HINSTANCE            g_pMSCorEE;

IPythonCompiler* CreateCLRCompiler(Module* intrins, Module* userModule);

// Tracks types for a function call.  Each argument has a SpecializedTreeNode with
// children for the subsequent arguments.  When we get to the leaves of the tree
// we'll have a jitted code object & optimized evalutation function optimized
// for those arguments.  
struct SpecializedTreeNode {
#ifdef TRACE_TREE
	vector<pair<PyTypeObject*, SpecializedTreeNode*>> children;
#else
	vector<PyTypeObject*> types;
#endif
	Py_EvalFunc addr;
	JittedCode* jittedCode;
	int hitCount;

#ifdef TRACE_TREE
	SpecializedTreeNode() {
#else
	SpecializedTreeNode(vector<PyTypeObject*>& types) : types(types) {
#endif
		addr = nullptr;
		jittedCode = nullptr;
		hitCount = 0;
	}

#ifdef TRACE_TREE
	SpecializedTreeNode* getNextNode(PyTypeObject* type) {
		for (auto cur = children.begin(); cur != children.end(); cur++) {
			if (cur->first == type) {
				return cur->second;
			}
		}

		auto res = new SpecializedTreeNode();
		children.push_back(pair<PyTypeObject*, SpecializedTreeNode*>(type, res));
		return res;
	}
#endif

	~SpecializedTreeNode() {
		delete jittedCode;
#ifdef TRACE_TREE
		for (auto cur = children.begin(); cur != children.end(); cur++) {
			delete cur->second;
		}
#endif
	}
};


PyjionJittedCode::~PyjionJittedCode() {
#ifdef TRACE_TREE
	delete funcs;
#else
	for (auto cur = j_optimized.begin(); cur != j_optimized.end(); cur++) {
		delete *cur;
	}
#endif
}

PyObject* Jit_EvalHelper(void* state, PyFrameObject*frame) {
    PyThreadState *tstate = PyThreadState_GET();
    if (tstate->use_tracing) {
        if (tstate->c_tracefunc != NULL) {
            return _PyEval_EvalFrameDefault(frame, 0);
        }
    }

    if (Py_EnterRecursiveCall("")) {
        return NULL;
    }

	frame->f_executing = 1;
    auto res = ((Py_EvalFunc)state)(nullptr, frame);

    Py_LeaveRecursiveCall();
    frame->f_executing = 0;

    return _Py_CheckFunctionResult(NULL, res, "Jit_EvalHelper");
}

static DWORD g_extraSlot;
PyObject* g_emptyTuple;

extern "C" __declspec(dllexport) void JitInit() {
	g_extraSlot = TlsAlloc();

	g_jit = getJit();

    g_emptyTuple = PyTuple_New(0);
}

#ifdef NO_TRACE

unordered_map<PyjionJittedCode*, JittedCode*> g_pyjionJittedCode;

__declspec(dllexport) bool jit_compile(PyCodeObject* code) {
    if (strcmp(PyUnicode_AsUTF8(code->co_name), "<module>") == 0) {
        return false;
    }

    auto jittedCode = (PyjionJittedCode *)code->co_extra;

#ifdef DEBUG_TRACE
    static int compileCount = 0, failCount = 0;
    printf("Compiling %s from %s line %d #%d (%d failures so far)\r\n",
        PyUnicode_AsUTF8(code->co_name),
        PyUnicode_AsUTF8(code->co_filename),
        code->co_firstlineno,
        ++compileCount,
        failCount);
#endif

	IPythonCompiler* comp = CreateCLRCompiler();
    AbstractInterpreter interp(code, comp);
    auto res = interp.compile();
	delete comp;

    if (res == nullptr) {
#ifdef DEBUG_TRACE
        printf("Compilation failure #%d\r\n", ++failCount);
#endif
        jittedCode->j_failed = true;
        return false;
    }

    g_pyjionJittedCode[jittedCode] = res;
    jittedCode->j_evalfunc = &Jit_EvalHelper;
    jittedCode->j_evalstate = res->get_code_addr();
    return true;
}

#else

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
        // We currently only generate optimal code for ints and floats,
        // so don't bother specializing on other types...
        if (type != &PyLong_Type && type != &PyFloat_Type) {
            type = nullptr;
        }
    }
    return type;
}

PyObject* Jit_EvalGeneric(PyjionJittedCode* state, PyFrameObject*frame) {
    auto trace = (PyjionJittedCode*)state;
    return Jit_EvalHelper(trace->j_generic, frame);
}

#define MAX_TRACE 5

PyObject* Jit_EvalTrace(PyjionJittedCode* state, PyFrameObject *frame) {
	// Walk our tree of argument types to find the SpecializedTreeNode which
    // corresponds with our sets of arguments here.
    auto trace = (PyjionJittedCode*)state;

#ifdef TRACE_TREE
    // no match on specialized functions...

    SpecializedTreeNode* curNode = trace->funcs;
    int argCount = frame->f_code->co_argcount + frame->f_code->co_kwonlyargcount;
    for (int i = 0; i < argCount; i++) {
        curNode = curNode->getNextNode(GetArgType(i, frame->f_localsplus));
    }

    curNode->hitCount += 1;

    if (curNode->addr != nullptr) {
        // we have a specialized function for this, just invoke it
        return Jit_EvalHelper(curNode->addr, frame);
    }

    // No specialized function yet, let's see if we should create one...
    auto jittedCode = (PyjionJittedCode *)trace->code->co_extra;
    if (curNode->hitCount > jittedCode->j_specialization_threshold) {
        // Compile and run the now compiled code...
		IPythonCompiler* comp = CreateCLRCompiler();
        AbstractInterpreter interp(trace->code, &jitter);

        // provide the interpreter information about the specialized types
        for (int i = 0; i < argCount; i++) {
            auto type = GetAbstractType(GetArgType(i, frame->f_localsplus));
            interp.set_local_type(i, type);
        }

        auto res = interp.compile();
		delete comp;
        bool isSpecialized = false;
        for (int i = 0; i < argCount; i++) {
            auto type = GetAbstractType(GetArgType(i, frame->f_localsplus));
            if (type == AVK_Integer || type == AVK_Float) {
                if (!interp.get_local_info(0, i).ValueInfo.needs_boxing()) {
                    isSpecialized = true;
                }
            }
        }
		/*
        printf("Tracing %s from %s line %d %s\r\n",
            PyUnicode_AsUTF8(frame->f_code->co_name),
            PyUnicode_AsUTF8(frame->f_code->co_filename),
            frame->f_code->co_firstlineno,
            isSpecialized ? "specialized" : ""
            );
			*/
        if (res == nullptr) {
#ifdef DEBUG_TRACE
            printf("Compilation failure #%d\r\n", ++failCount);
#endif
            return PyEval_EvalFrameDefault(frame, 0);
        }

        // Update the jitted information for this tree node
        curNode->addr = (Py_EvalFunc)res->get_code_addr();
        curNode->jittedCode = res;
        if (!isSpecialized) {
            trace->Generic = curNode->addr;
            PyjionJittedCode* pyjionCode = (PyjionJittedCode*)frame->f_code->co_extra;
            pyjionCode->j_evalfunc = Jit_EvalGeneric;
        }

        // And finally dispatch to the newly compiled code
        return Jit_EvalHelper(curNode->addr, frame);
    }
#else

	SpecializedTreeNode* target = nullptr;
    for (auto cur = trace->j_optimized.begin(); cur != trace->j_optimized.end() && target == nullptr; cur++) {
        PyObject** locals = frame->f_localsplus;

		target = *cur;
        for (auto type = target->types.begin(); type != target->types.end(); type++, locals++) {
            PyTypeObject *argType = nullptr;
            if (*locals != nullptr) {
                argType = (*locals)->ob_type;
                // We currently only generate optimal code for ints and floats,
                // so don't bother specializing on other types...
                if (argType != &PyLong_Type && argType != &PyFloat_Type) {
                    argType = nullptr;
                }
            }

            if (*type != argType) {
				target = nullptr;
                break;
            }
        }
    }

    // record the new trace...
    if (target == nullptr && trace->j_optimized.size() < MAX_TRACE) {
        int argCount = frame->f_code->co_argcount + frame->f_code->co_kwonlyargcount;
        vector<PyTypeObject*> types;
        for (int i = 0; i < argCount; i++) {
            auto type = GetArgType(i, frame->f_localsplus);
            types.push_back(type);
        }
		target = new SpecializedTreeNode(types);
        trace->j_optimized.push_back(target);
	}
#endif

	if (target != nullptr && !trace->j_failed) {
		if (target->addr != nullptr) {
			// we have a specialized function for this, just invoke it
			/*printf("Invoking trace %s from %s line %d %p %p\r\n",
				PyUnicode_AsUTF8(frame->f_code->co_name),
				PyUnicode_AsUTF8(frame->f_code->co_filename),
				frame->f_code->co_firstlineno,
				target->addr,
				frame
			);*/
			auto res = Jit_EvalHelper(target->addr, frame);
			//printf("Returning from %s", PyUnicode_AsUTF8(frame->f_code->co_name));
			return res;
		}

		target->hitCount++;
		// we've recorded these types before...
		// No specialized function yet, let's see if we should create one...
		if (target->hitCount >= trace->j_specialization_threshold) {
			// Compile and run the now compiled code...
			IPythonCompiler* comp = CreateCLRCompiler();
			AbstractInterpreter interp((PyCodeObject*)trace->j_code, comp);
			int argCount = frame->f_code->co_argcount + frame->f_code->co_kwonlyargcount;

			// provide the interpreter information about the specialized types
			for (int i = 0; i < argCount; i++) {
				auto type = GetAbstractType(GetArgType(i, frame->f_localsplus));
				interp.set_local_type(i, type);
			}

			auto res = interp.compile();
			delete comp;
			bool isSpecialized = false;
			for (int i = 0; i < argCount; i++) {
				auto type = GetAbstractType(GetArgType(i, frame->f_localsplus));
				if (type == AVK_Integer || type == AVK_Float) {
					if (!interp.get_local_info(0, i).ValueInfo.needs_boxing()) {
						isSpecialized = true;
					}
				}
			}
#if DEBUG_TRACE
			printf("Tracing %s from %s line %d %s\r\n",
				PyUnicode_AsUTF8(frame->f_code->co_name),
				PyUnicode_AsUTF8(frame->f_code->co_filename),
				frame->f_code->co_firstlineno,
				isSpecialized ? "specialized" : ""
			);
#endif

			if (res == nullptr) {
#if DEBUG_TRACE
				static int failCount;
				printf("Compilation failure #%d\r\n", ++failCount);
#endif
				trace->j_failed = true;
				return _PyEval_EvalFrameDefault(frame, 0);
			}

			// Update the jitted information for this tree node
			target->addr = (Py_EvalFunc)res->get_code_addr();
			//opt->jittedCode = res;
			if (!isSpecialized) {
				// We didn't produce a specialized function, force all code down
				// the generic code path.
				trace->j_generic = target->addr;
				trace->j_evalfunc = Jit_EvalGeneric;
			}
			
			/*printf("Entering %s from %s line %d %s\r\n",
				PyUnicode_AsUTF8(frame->f_code->co_name),
				PyUnicode_AsUTF8(frame->f_code->co_filename),
				frame->f_code->co_firstlineno,
				isSpecialized ? "specialized" : ""
			);*/

			// And finally dispatch to the newly compiled code
			/*printf("Invoking trace %s from %s line %d %p\r\n",
				PyUnicode_AsUTF8(frame->f_code->co_name),
				PyUnicode_AsUTF8(frame->f_code->co_filename),
				frame->f_code->co_firstlineno,
				target->addr
			);*/
			return Jit_EvalHelper(target->addr, frame);
		}
	}
	
	/*printf("Invoking default %s from %s line %d %s %p %p\r\n",
		PyUnicode_AsUTF8(frame->f_code->co_name),
		PyUnicode_AsUTF8(frame->f_code->co_filename),
		frame->f_code->co_firstlineno,
		target->addr,
		frame
	);*/
	auto res = _PyEval_EvalFrameDefault(frame, 0);
	/*printf("Returning default %s from %s line %d %s %p\r\n",
		PyUnicode_AsUTF8(frame->f_code->co_name),
		PyUnicode_AsUTF8(frame->f_code->co_filename),
		frame->f_code->co_firstlineno,
		target->addr
	);*/
	return res;
}

__declspec(dllexport) bool jit_compile(PyCodeObject* code) {
	if (strcmp(PyUnicode_AsUTF8(code->co_name), "<module>") == 0 ||
		strcmp(PyUnicode_AsUTF8(code->co_name), "<genexpr>") == 0) {
        return false;
    }
#ifdef DEBUG_TRACE
    static int compileCount = 0, failCount = 0;
    printf("Tracing %s from %s line %d #%d (%d failures so far)\r\n",
        PyUnicode_AsUTF8(code->co_name),
        PyUnicode_AsUTF8(code->co_filename),
        code->co_firstlineno,
        ++compileCount,
        failCount);
#endif

	auto jittedCode = PyJit_EnsureExtra((PyObject*)code);

	jittedCode->j_evalfunc = &Jit_EvalTrace;
    return true;
}

#endif


extern "C" __declspec(dllexport) PyjionJittedCode* PyJit_EnsureExtra(PyObject* codeObject) {
	ssize_t index = (ssize_t)TlsGetValue(g_extraSlot);
	if (index == 0) {
		index = _PyEval_RequestCodeExtraIndex(PyjionJitFree);
		if (index == -1) {
			return nullptr;
		}

		TlsSetValue(g_extraSlot, (LPVOID)((index << 1) | 0x01));
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

// This is our replacement evaluation function.  We lookup our coorespanding jitted code
// and dispatch to it if it's already compiled.  If it hasn't yet been compiled we'll
// eventually compile it and invoke it.  If it's not time to compile it yet then we'll
// invoke the default evaluation function.
extern "C" __declspec(dllexport) PyObject *PyJit_EvalFrame(PyFrameObject *f, int throwflag) {
	auto err = GetLastError();

	auto jitted = PyJit_EnsureExtra((PyObject*)f->f_code);
	if (jitted != nullptr && !throwflag) {
		if (jitted->j_evalfunc != nullptr) {
			SetLastError(err);
			/*
			printf("Calling %s from %s line %d %p\r\n",
				PyUnicode_AsUTF8(f->f_code->co_name),
				PyUnicode_AsUTF8(f->f_code->co_filename),
				f->f_code->co_firstlineno,
				jitted
			);*/
			return jitted->j_evalfunc(jitted, f);
		}
		else if (!jitted->j_failed && jitted->j_run_count++ >= jitted->j_specialization_threshold) {
			if (jit_compile(f->f_code)) {
				// execute the jitted code...
				SetLastError(err);
				/*printf("Calling first %s from %s line %d %p\r\n",
					PyUnicode_AsUTF8(f->f_code->co_name),
					PyUnicode_AsUTF8(f->f_code->co_filename),
					f->f_code->co_firstlineno,
					jitted
				);*/
				return jitted->j_evalfunc(jitted, f);
			}

			// no longer try and compile this method...
			jitted->j_failed = true;
		}
	}
	SetLastError(err);
	/*
	printf("Falling to EFD %s from %s line %d %s %p\r\n",
		PyUnicode_AsUTF8(f->f_code->co_name),
		PyUnicode_AsUTF8(f->f_code->co_filename),
		f->f_code->co_firstlineno,
		jitted
	);*/

	auto res = _PyEval_EvalFrameDefault(f, throwflag);
	/*
	printf("Returning EFD %s from %s line %d %s %p\r\n",
		PyUnicode_AsUTF8(f->f_code->co_name),
		PyUnicode_AsUTF8(f->f_code->co_filename),
		f->f_code->co_firstlineno,
		jitted
	);*/
	return res;
}

void PyjionJitFree(void* obj) {
	PyjionJittedCode* function = (PyjionJittedCode*)obj;
#ifdef NO_TRACE
    auto find = g_pyjionJittedCode.find(function);
    if (find != g_pyjionJittedCode.end()) {
        auto code = find->second;

        delete code;
        g_pyjionJittedCode.erase(function);
    }
#endif
	delete obj;
}

static PyObject *pyjion_enable(PyObject *self, PyObject* args) {
	auto ts = PyThreadState_Get();
	auto prev = ts->interp->eval_frame;
	ts->interp->eval_frame = PyJit_EvalFrame;
	return prev == PyJit_EvalFrame ? Py_False : Py_True;
}

static PyObject *pyjion_disable(PyObject *self, PyObject* args) {
	auto ts = PyThreadState_Get();
	auto prev = ts->interp->eval_frame;
	ts->interp->eval_frame = _PyEval_EvalFrameDefault;
	return prev == PyJit_EvalFrame ? Py_True : Py_False;
}

static PyObject *pyjion_status(PyObject *self, PyObject* args) {
	auto ts = PyThreadState_Get();
	auto prev = ts->interp->eval_frame;
	return prev == PyJit_EvalFrame ? Py_True : Py_False;
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
	"pyjion",   /* name of module */
	"Pyjion - A Just in Time Compiler for CPython 3.6.x", /* module documentation, may be NULL */
	-1,       /* size of per-interpreter state of the module,
			  or -1 if the module keeps state in global variables. */
	PyjionMethods
}; 

PyMODINIT_FUNC PyInit_pyjion(void)
{
	// Install our frame evaluation function
	JitInit();

	auto ts = PyThreadState_Get();
	ts->interp->eval_frame = PyJit_EvalFrame;

	return PyModule_Create(&pyjionmodule);
}
