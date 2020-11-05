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
  Test instructions that require different scopes
*/

#include <catch2/catch.hpp>
#include "testing_util.h"
#include <Python.h>
#include <frameobject.h>
#include <util.h>
#include <pyjit.h>

class ScopedCompilerTest {
private:
    py_ptr <PyCodeObject> m_code;
    py_ptr <PyjionJittedCode> m_jittedcode;

    PyObject *run() {
        auto sysModule = PyObject_ptr(PyImport_ImportModule("sys"));
        auto globals = PyObject_ptr(PyDict_New());
        auto locals = PyObject_ptr(PyDict_New());
        auto builtins = PyEval_GetBuiltins();
        PyDict_SetItemString(globals.get(), "__builtins__", builtins);
        PyDict_SetItemString(globals.get(), "sys", sysModule.get());

        PyDict_SetItemString(globals.get(), "gvar1", PyUnicode_FromString("hello"));

        PyDict_SetItemString(locals.get(), "loc1", PyUnicode_FromString("hello"));
        PyDict_SetItemString(locals.get(), "loc2", PyUnicode_FromString("hello"));

        // Don't DECREF as frames are recycled.
        auto frame = PyFrame_New(PyThreadState_Get(), m_code.get(), globals.get(), locals.get());
        auto prev = _PyInterpreterState_GetEvalFrameFunc(PyInterpreterState_Main());
        _PyInterpreterState_SetEvalFrameFunc(PyInterpreterState_Main(), PyJit_EvalFrame);
        auto res = m_jittedcode->j_evalfunc(m_jittedcode.get(), frame);
        _PyInterpreterState_SetEvalFrameFunc(PyInterpreterState_Main(), prev);
        //Py_DECREF(frame);
        size_t collected = PyGC_Collect();
        printf("Collected %zu values\n", collected);
        REQUIRE(!m_jittedcode->j_failed);
        return res;
    }

public:
    explicit ScopedCompilerTest(const char *code) {
        PyErr_Clear();
        m_code.reset(CompileCode(code, {"loc1", "loc2"}, {"gvar1"}));
        if (m_code.get() == nullptr) {
            FAIL("failed to compile code");
        }
        auto jitted = PyJit_EnsureExtra((PyObject *) *m_code);
        if (!jit_compile(m_code.get())) {
            FAIL("failed to JIT code");
        }
        m_jittedcode.reset(jitted);
    }

    std::string returns() {
        auto res = PyObject_ptr(run());
        REQUIRE(res.get() != nullptr);
        if (PyErr_Occurred()) {
            PyErr_PrintEx(-1);
            FAIL("Error on Python execution");
            return nullptr;
        }

        auto repr = PyUnicode_AsUTF8(PyObject_Repr(res.get()));
        auto tstate = PyThreadState_GET();
        REQUIRE(tstate->curexc_value == nullptr);
        REQUIRE(tstate->curexc_traceback == nullptr);
        if (tstate->curexc_type != nullptr) {
            REQUIRE(tstate->curexc_type == Py_None);
        }

        return std::string(repr);
    }

    PyObject *raises() {
        auto res = run();
        REQUIRE(res == nullptr);
        auto excType = PyErr_Occurred();
        PyErr_Print();
        PyErr_Clear();
        return excType;
    }
};

TEST_CASE("Test globals") {
    SECTION("test load global") {
        auto t = ScopedCompilerTest(
                "def f():\n  return gvar1"
        );
        CHECK(t.returns() == "'hello'");
    }

    SECTION("test set global") {
        auto t = ScopedCompilerTest(
                "def f():\n  global gvar1\n  gvar1 = 'goodbye'\n  return gvar1"
        );
        CHECK(t.returns() == "'goodbye'");
    }

    SECTION("test delete name with the same name as global raises unbound local") {
        auto t = ScopedCompilerTest(
                "def f():\n  del gvar1\n  return gvar1"
        );
        CHECK(t.raises() == PyExc_UnboundLocalError);
    }

    SECTION("test explicit global") {
        auto t = ScopedCompilerTest(
                "def f():\n    global gvar1\n    gvar1 = 2\n    return gvar1"
        );
        CHECK(t.returns() == "2");
    }

    SECTION("test delete explicit global") {
        auto t = ScopedCompilerTest(
                "def f():\n    global gvar1\n    del gvar1\n    return gvar1"
        );
        CHECK(t.raises() == PyExc_NameError);
    }
}


TEST_CASE("Test nonlocals") {
    SECTION("test load and set nonlocal") {
        auto t = ScopedCompilerTest(
                "def f():\n"
                "  x = 'bob'\n"
                "  def f2():\n"
                "    nonlocal x\n"
                "    x = 'sally'\n"
                "  f2() \n"
                "  return x"
        );
        CHECK(t.returns() == "'sally'");
    }

    SECTION("test delete nonlocal") {
        auto t = ScopedCompilerTest(
                "def f():\n"
                "  x = 'bob'\n"
                "  def f2():\n"
                "    nonlocal x\n"
                "    del x\n"
                "  f2() \n"
                "  return x"
        );
        CHECK(t.raises() == PyExc_UnboundLocalError);
    }

    SECTION("test list comprehension cannot unpack iterator") {
        auto t = ScopedCompilerTest(
                "def f():\n"
                "  sep = '-'\n"
                "  letters = 'bingo'\n"
                "  def f2(*letters):\n"
                "    nonlocal sep\n"
                "    return sep.join([letter.upper() for letter in letters])\n"
                "  return f2(iter(letters)) \n"
        );
        CHECK(t.raises() == PyExc_AttributeError);
    }

    SECTION("test list comprehension can unpack iterator with listcomp") {
        auto t = ScopedCompilerTest(
                "def f():\n"
                "  sep = '-'\n"
                "  letters = 'bingo'\n"
                "  def f2(*letters):\n"
                "    nonlocal sep\n"
                "    return sep.join([letter.upper() for letter in letters])\n"
                "  return f2(*letters) \n"
        );
        CHECK(t.returns() == "'B-I-N-G-O'");
    }

    SECTION("test list comprehension can unpack iterator with loop") {
        auto t = ScopedCompilerTest(
                "def f():\n"
                "  sep = '-'\n"
                "  letters = 'bingo'\n"
                "  def f2(*letters):\n"
                "    nonlocal sep\n"
                "    res = []\n"
                "    for letter in letters:"
                "       res.append(letter.upper())\n"
                "    return sep.join(res)\n"
                "  return f2(*letters) \n"
        );
        CHECK(t.returns() == "'B-I-N-G-O'");
    }
}