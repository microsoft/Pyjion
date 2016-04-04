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

unordered_map<PyJittedCode*, JittedCode*> g_jittedCode;
unordered_map<PyjionJittedCode*, JittedCode*> g_pyjionJittedCode;

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
    jittedCode->j_evalfunc = &Jit_EvalHelper; //(Py_EvalFunc)res->get_code_addr();
    jittedCode->j_evalstate = res->get_code_addr();
    return jittedCode;
}

extern "C" __declspec(dllexport) void JitFree(PyJittedCode* function) {
    auto find = g_jittedCode.find(function);
    if (find != g_jittedCode.end()) {
        auto code = find->second;

        delete code;
        g_jittedCode.erase(function);
    }
}

extern "C" __declspec(dllexport) void JitInit() {
    g_jit = getJit();

    g_emptyTuple = PyTuple_New(0);
}

static PyObject *
jittedcode_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_Size(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "JittedCode takes no arguments");
        return NULL;
    }
    return (PyObject *)PyObject_New(PyjionJittedCode, &PyjionJittedCode_Type);
}

extern "C" void PyjionJitFree(PyjionJittedCode* function) {
    auto find = g_pyjionJittedCode.find(function);
    if (find != g_pyjionJittedCode.end()) {
        auto code = find->second;

        delete code;
        g_pyjionJittedCode.erase(function);
    }
}

static void
jittedcode_dealloc(PyObject *co) {
    PyjionJitFree((PyjionJittedCode *)co);
}

PyTypeObject PyjionJittedCode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "pyjionjittedcode",                 /* tp_name */
    sizeof(PyjionJittedCode),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    jittedcode_dealloc,                 /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    jittedcode_new,                     /* tp_new */
};