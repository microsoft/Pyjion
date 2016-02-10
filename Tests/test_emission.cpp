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
  Test JIT code emission.
*/

#include "stdafx.h"
#include "catch.hpp"
#include "testing_util.h"
#include <Python.h>
#include <frameobject.h>
#include <pyjit.h>

class EmissionTest {
private:
    PyCodeObject *m_code;
    PyJittedCode *m_jittedcode;

    PyObject *run() {
        auto sysModule = PyImport_ImportModule("sys");
        auto globals = PyDict_New();
        auto builtins = PyThreadState_GET()->interp->builtins;
        PyDict_SetItemString(globals, "__builtins__", builtins);
        PyDict_SetItemString(globals, "sys", sysModule);

        /* XXX Why?
        PyRun_String("finalized = False\nclass RefCountCheck:\n    def __del__(self):\n        print('finalizing')\n        global finalized\n        finalized = True\n    def __add__(self, other):\n        return self", Py_file_input, globals, globals);
        if (PyErr_Occurred()) {
        PyErr_Print();
        return "";
        }*/

        // Don't DECREF as frames are recycled.
        auto frame = PyFrame_New(PyThreadState_Get(), m_code, globals, PyDict_New());

        auto res = m_jittedcode->j_evalfunc(m_jittedcode->j_evalstate, frame);
        Py_DECREF(globals);
        Py_DECREF(sysModule);

        return res;
    }

public:
    EmissionTest(const char *code) {
        m_code = CompileCode(code);
        if (m_code == nullptr) {
            FAIL("failed to compile code");
        }
        m_jittedcode = JitCompile(m_code);
        if (m_jittedcode == nullptr) {
            FAIL("failed to JIT code");
        }
    }

    ~EmissionTest() {
        Py_XDECREF(m_code);
        Py_XDECREF(m_jittedcode);
    }

    std::string& returns() {
        auto res = run();
        REQUIRE(res != nullptr);
        REQUIRE(!PyErr_Occurred());

        auto repr = PyUnicode_AsUTF8(PyObject_Repr(res));
        Py_DECREF(res);
        auto tstate = PyThreadState_GET();
        REQUIRE(tstate->exc_value == nullptr);
        REQUIRE(tstate->exc_traceback == nullptr);
        if (tstate->exc_type != nullptr) {
            REQUIRE(tstate->exc_type == Py_None);
        }

        return std::string(repr);
    }

    PyObject* raises() {
        auto res = run();
        REQUIRE(res == nullptr);
        return PyErr_Occurred();
    }
};

TEST_CASE("General list unpacking", "[list][BUILD_LIST_UNPACK][emission]") {
    SECTION("common case") {
        auto t = EmissionTest("def f(): return [1, *[2], 3]");
        REQUIRE(t.returns() == "[1, 2, 3]");
    }

    SECTION("unpacking a non-iterable") {
        auto t = EmissionTest("def f(): return [1, *2, 3]");
        REQUIRE(t.raises() == PyExc_TypeError);
    }
}