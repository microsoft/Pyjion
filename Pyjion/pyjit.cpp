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
#include "pycomp.h"

#include <vector>
//#define NO_TRACE
//#define TRACE_TREE

HINSTANCE            g_pMSCorEE;
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

static Py_ssize_t g_extraIndex;
extern "C" __declspec(dllexport) void JitInit() {
    g_jit = getJit();

    g_emptyTuple = PyTuple_New(0);

	g_extraIndex = _PyEval_RequestCodeExtraIndex(PyjionJitFree);
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

    PythonCompiler jitter(code);
    AbstractInterpreter interp(code, &jitter);
    auto res = interp.compile();

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


// Holds onto all of the data that we use while tracing, and the specialized function(s)
// that we produce.
struct TraceInfo {
    PyCodeObject *code;
#ifdef TRACE_TREE
    SpecializedTreeNode* funcs;
#else
    vector<SpecializedTreeNode*> opt;
#endif
    Py_EvalFunc Generic;
	PyjionJittedCode* jittedCode;

    TraceInfo(PyCodeObject *code, PyjionJittedCode* jittedCode) {
        this->code = code;
		this->jittedCode = jittedCode;
#ifdef TRACE_TREE
        funcs = new SpecializedTreeNode();
#endif
        Generic = nullptr;
    }

    ~TraceInfo() {
#ifdef TRACE_TREE
        delete funcs;
#else
        for (auto cur = opt.begin(); cur != opt.end(); cur++) {
            delete *cur;
        }
#endif
    }
};

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

PyObject* Jit_EvalGeneric(void* state, PyFrameObject*frame) {
    auto trace = (TraceInfo*)state;
    return Jit_EvalHelper(trace->Generic, frame);
}

#define MAX_TRACE 5

PyObject* Jit_EvalTrace(void* state, PyFrameObject *frame) {
    // Walk our tree of argument types to find the SpecializedTreeNode which
    // corresponds with our sets of arguments here.
    auto trace = (TraceInfo*)state;

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
        PythonCompiler jitter(trace->code);
        AbstractInterpreter interp(trace->code, &jitter);

        // provide the interpreter information about the specialized types
        for (int i = 0; i < argCount; i++) {
            auto type = GetAbstractType(GetArgType(i, frame->f_localsplus));
            interp.set_local_type(i, type);
        }

        auto res = interp.compile();
        bool isSpecialized = false;
        for (int i = 0; i < argCount; i++) {
            auto type = GetAbstractType(GetArgType(i, frame->f_localsplus));
            if (type == AVK_Integer || type == AVK_Float) {
                if (!interp.get_local_info(0, i).ValueInfo.needs_boxing()) {
                    isSpecialized = true;
                }
            }
        }

        printf("Tracing %s from %s line %d %s\r\n",
            PyUnicode_AsUTF8(frame->f_code->co_name),
            PyUnicode_AsUTF8(frame->f_code->co_filename),
            frame->f_code->co_firstlineno,
            isSpecialized ? "specialized" : ""
            );

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
    for (auto cur = trace->opt.begin(); cur != trace->opt.end(); cur++) {
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
    if (trace->opt.size() < MAX_TRACE) {
        int argCount = frame->f_code->co_argcount + frame->f_code->co_kwonlyargcount;
        vector<PyTypeObject*> types;
        for (int i = 0; i < argCount; i++) {
            auto type = GetArgType(i, frame->f_localsplus);
            types.push_back(type);
        }
		target = new SpecializedTreeNode(types);
        trace->opt.push_back(target);
    }
#endif

	if (target != nullptr) {
		if (target->addr != nullptr) {
			// we have a specialized function for this, just invoke it
			return Jit_EvalHelper(target->addr, frame);
		}

		target->hitCount++;
		// we've recorded these types before...
		// No specialized function yet, let's see if we should create one...
		if (target->hitCount >= trace->jittedCode->j_specialization_threshold) {
			// Compile and run the now compiled code...
			PythonCompiler jitter(trace->code);
			AbstractInterpreter interp(trace->code, &jitter);
			int argCount = frame->f_code->co_argcount + frame->f_code->co_kwonlyargcount;

			// provide the interpreter information about the specialized types
			for (int i = 0; i < argCount; i++) {
				auto type = GetAbstractType(GetArgType(i, frame->f_localsplus));
				interp.set_local_type(i, type);
			}

			auto res = interp.compile();
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
#ifdef DEBUG_TRACE
				printf("Compilation failure #%d\r\n", ++failCount);
#endif
				return _PyEval_EvalFrameDefault(frame, 0);
			}

			// Update the jitted information for this tree node
			target->addr = (Py_EvalFunc)res->get_code_addr();
			//opt->jittedCode = res;
			if (!isSpecialized) {
				// We didn't produce a specialized function, force all code down
				// the generic code path.
				trace->Generic = target->addr;
				trace->jittedCode->j_evalfunc = Jit_EvalGeneric;
			}

			// And finally dispatch to the newly compiled code
			return Jit_EvalHelper(target->addr, frame);
		}
	}
	
	return _PyEval_EvalFrameDefault(frame, 0);
}

__declspec(dllexport) bool jit_compile(PyCodeObject* code) {
    if (strcmp(PyUnicode_AsUTF8(code->co_name), "<module>") == 0) {
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
    TraceInfo* trace = new TraceInfo(code, jittedCode);
    if (trace == nullptr) {
        return false;
    }

    jittedCode->j_evalfunc = &Jit_EvalTrace;
    jittedCode->j_evalstate = trace;
    return true;
}

#endif

static PY_UINT64_T HOT_CODE = 20000;

extern "C" __declspec(dllexport) PyjionJittedCode* PyJit_EnsureExtra(PyObject* codeObject) {
	PyjionJittedCode *jitted = nullptr;
	if (_PyCode_GetExtra(codeObject, g_extraIndex, (void**)&jitted)) {
		return nullptr;
	}

	if (jitted == nullptr) {
	    jitted = new PyjionJittedCode();
		if (jitted != nullptr) {
			if (_PyCode_SetExtra(codeObject, g_extraIndex, jitted)) {
				delete jitted;
				return nullptr;
			}
		}
	}
	return jitted;
}

extern "C" __declspec(dllexport) PyObject *PyJit_EvalFrame(PyFrameObject *f, int throwflag) {
	auto jitted = PyJit_EnsureExtra((PyObject*)f->f_code);
	if (jitted != nullptr) {
		if (!throwflag) {
			if (!jitted->j_failed) {
				if (jitted->j_evalfunc != nullptr) {
					return jitted->j_evalfunc(jitted->j_evalstate, f);
				}
				else if (jitted->j_run_count++ > HOT_CODE) {
					if (jit_compile(f->f_code)) {
						// execute the jitted code...
						return jitted->j_evalfunc(jitted->j_evalstate, f);
					}

					// no longer try and compile this method...
					jitted->j_failed = true;
				}
			}
		}
	}

    return _PyEval_EvalFrameDefault(f, throwflag);
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
#else
    delete (TraceInfo*)function->j_evalstate;
#endif
	delete obj;
}

static PyMethodDef PyjionMethods[] = {
	{NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef pyjionmodule = {
	PyModuleDef_HEAD_INIT,
	"pyjion",   /* name of module */
	"Pyjion JIT for CPython 3.6.x", /* module documentation, may be NULL */
	-1,       /* size of per-interpreter state of the module,
			  or -1 if the module keeps state in global variables. */
	PyjionMethods
}; 

PyMODINIT_FUNC PyInit_pyjion(void)
{
	// Install our frame evaluation function
	auto ts = PyThreadState_Get();
	ts->interp->eval_frame = PyJit_EvalFrame;

	return PyModule_Create(&pyjionmodule);
}