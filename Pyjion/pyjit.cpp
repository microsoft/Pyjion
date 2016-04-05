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

PyObject* __stdcall Jit_EvalHelper(void* state, PyFrameObject*frame) {
    PyThreadState *tstate = PyThreadState_GET();
    if (tstate->use_tracing) {
        if (tstate->c_tracefunc != NULL) {
            return PyEval_EvalFrameEx_NoJit(frame, 0);
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

#ifdef NO_TRACE

unordered_map<PyJittedCode*, JittedCode*> g_jittedCode;

extern "C" __declspec(dllexport) PyJittedCode* JitCompile(PyCodeObject* code) {
    if (strcmp(PyUnicode_AsUTF8(code->co_name), "<module>") == 0) {
        return nullptr;
    }
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
        return nullptr;
    }

    auto jittedCode = (PyJittedCode*)PyJittedCode_New();
    if (jittedCode == nullptr) {
        // OOM
        delete res;
        return nullptr;
    }

    g_jittedCode[jittedCode] = res;
    jittedCode->j_evalfunc = &Jit_EvalHelper;
    jittedCode->j_evalstate = res->get_code_addr();
    return jittedCode;
}
#else

// Tracks types for a function call.  Each argument has a SpecializedTreeNode with
// children for the subsequent arguments.  When we get to the leaves of the tree
// we'll have a jitted code object & optimized evalutation function optimized
// for those arguments.  
struct SpecializedTreeNode {
    vector<pair<PyTypeObject*, SpecializedTreeNode*>> children;
    Py_EvalFunc addr;
    JittedCode* jittedCode;
    int hitCount;

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

    ~SpecializedTreeNode() {
        delete jittedCode;
        for (auto cur = children.begin(); cur != children.end(); cur++) {
            delete cur->second;
        }
    }
};


// Holds onto all of the data that we use while tracing, and the specialized function(s)
// that we produce.
struct TraceInfo {
    PyCodeObject *code;
    SpecializedTreeNode* funcs;

    TraceInfo(PyCodeObject *code) {
        this->code = code;
        funcs = new SpecializedTreeNode();
    }

    ~TraceInfo() {
        delete funcs;
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

PyObject* __stdcall Jit_EvalTrace(void* state, PyFrameObject*frame) {
    // Walk our tree of argument types to find the SpecializedTreeNode which
    // corresponds with our sets of arguments here.
    auto trace = (TraceInfo*)state;
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
    if (curNode->hitCount > 500) {
        // Compile and run the now compiled code...
        PythonCompiler jitter(trace->code);
        AbstractInterpreter interp(trace->code, &jitter);

        // provide the interpreter information about the specialized types
        for (int i = 0; i < argCount; i++) {
            auto type = GetAbstractType(GetArgType(i, frame->f_localsplus));
            interp.set_local_type(i, type);
        }

        auto res = interp.compile();

        if (res == nullptr) {
#ifdef DEBUG_TRACE
            printf("Compilation failure #%d\r\n", ++failCount);
#endif
            return PyEval_EvalFrameEx_NoJit(frame, 0);
        }

        // Update the jitted information for this tree node
        curNode->addr = (Py_EvalFunc)res->get_code_addr();
        curNode->jittedCode = res;

        // And finally dispatch to the newly compiled code
        return Jit_EvalHelper(curNode->addr, frame);
    }

    return PyEval_EvalFrameEx_NoJit(frame, 0);
}

extern "C" __declspec(dllexport) PyJittedCode* JitCompile(PyCodeObject* code) {
    if (strcmp(PyUnicode_AsUTF8(code->co_name), "<module>") == 0) {
        return nullptr;
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

    TraceInfo* trace = new TraceInfo(code);
    if (trace == nullptr) {
        return nullptr;
    }
    PyJittedCode* res = (PyJittedCode*)PyJittedCode_New();
    if (res == nullptr) {
        delete trace;
        return nullptr;
    }

    res->j_evalfunc = &Jit_EvalTrace;
    res->j_evalstate = trace;
    return res;
}

#endif

extern "C" __declspec(dllexport) void JitFree(PyJittedCode* function) {
#ifdef NO_TRACE
    auto find = g_jittedCode.find(function);
    if (find != g_jittedCode.end()) {
        auto code = find->second;

        delete code;
        g_jittedCode.erase(function);
    }
#else
    delete (TraceInfo*)function->j_evalstate;
#endif
}

extern "C" __declspec(dllexport) void JitInit() {
    g_jit = getJit();

    g_emptyTuple = PyTuple_New(0);
}

