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

/**
  Testing utility code.
*/

#include "stdafx.h"
#include "catch.hpp"
#include <Python.h>

PyCodeObject* CompileCode(const char* code) {
    auto globals = PyDict_New();
    auto builtins = PyThreadState_GET()->interp->builtins;
    PyDict_SetItemString(globals, "__builtins__", builtins);

    auto locals = PyDict_New();
    PyRun_String(code, Py_file_input, globals, locals);
    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_DECREF(globals);
        FAIL("error occurred during Python compilation");
        return nullptr;
    }
    auto func = PyObject_GetItem(locals, PyUnicode_FromString("f"));
    auto codeObj = (PyCodeObject*)PyObject_GetAttrString(func, "__code__");

    Py_DECREF(globals);
    Py_DECREF(locals);
    Py_DECREF(func);

    return codeObj;
}