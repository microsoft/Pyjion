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
#include <util.h>
#include <pyjit.h>

class EmissionTest {
private:
    py_ptr<PyCodeObject> m_code;
    py_ptr<PyjionJittedCode> m_jittedcode;

    PyObject* run() {
        auto sysModule = PyObject_ptr(PyImport_ImportModule("sys"));
        auto globals = PyObject_ptr(PyDict_New());
        auto builtins = PyThreadState_GET()->interp->builtins;
        PyDict_SetItemString(globals.get(), "__builtins__", builtins);
        PyDict_SetItemString(globals.get(), "sys", sysModule.get());

        // Don't DECREF as frames are recycled.
        auto frame = PyFrame_New(PyThreadState_Get(), m_code.get(), globals.get(), PyObject_ptr(PyDict_New()).get());

        auto res = m_jittedcode->j_evalfunc(m_jittedcode->j_evalstate, frame);

        return res;
    }

public:
    EmissionTest(const char *code) {
        m_code.reset(CompileCode(code));
        if (m_code.get() == nullptr) {
            FAIL("failed to compile code");
        }
        m_code->co_extra = (PyObject *)jittedcode_new_direct();
        if (!jit_compile(m_code.get())) {
            FAIL("failed to JIT code");
        }
        m_jittedcode.reset((PyjionJittedCode *)m_code->co_extra);
    }

    std::string returns() {
        auto res = PyObject_ptr(run());
        REQUIRE(res.get() != nullptr);
        REQUIRE(!PyErr_Occurred());

        auto repr = PyUnicode_AsUTF8(PyObject_Repr(res.get()));
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
        auto excType = PyErr_Occurred();
        PyErr_Clear();
        return excType;
    }
};

TEST_CASE("General list unpacking", "[list][BUILD_LIST_UNPACK][emission]") {
    SECTION("common case") {
        auto t = EmissionTest("def f(): return [1, *[2], 3]");
        CHECK(t.returns() == "[1, 2, 3]");
    }

    SECTION("unpacking a non-iterable") {
        auto t = EmissionTest("def f(): return [1, *2, 3]");
        CHECK(t.raises() == PyExc_TypeError);
    }
}

TEST_CASE("General tuple unpacking", "[tuple][BUILD_TUPLE_UNPACK][emission]") {
    SECTION("common case") {
        auto t = EmissionTest("def f(): return (1, *(2,), 3)");
        CHECK(t.returns() == "(1, 2, 3)");
    }

    SECTION("unpacking a non-iterable") {
        auto t = EmissionTest("def f(): return (1, *2, 3)");
        CHECK(t.raises() == PyExc_TypeError);
    }
}

TEST_CASE("General set unpacking", "[set][BUILD_SET_UNPACK][emission]") {
    SECTION("common case") {
        auto t = EmissionTest("def f(): return {1, *[2], 3}");
        CHECK(t.returns() == "{1, 2, 3}");
    }

    SECTION("unpacking a non-iterable") {
        auto t = EmissionTest("def f(): return {1, *2, 3}");
        CHECK(t.raises() == PyExc_TypeError);
    }
}

TEST_CASE("General dict unpacking", "[dict][BUILD_MAP_UNPACK][emission]") {
    SECTION("common case") {
        auto t = EmissionTest("def f(): return {1:'a', **{2:'b'}, 3:'c'}");
        CHECK(t.returns() == "{1: 'a', 2: 'b', 3: 'c'}");
    }

    SECTION("unpacking a non-mapping") {
        auto t = EmissionTest("def f(): return {1:'a', **{2}, 3:'c'}");
        CHECK(t.raises() == PyExc_TypeError);
    }
}
