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

#include <catch2/catch.hpp>
#include "testing_util.h"
#include <Python.h>
#include <frameobject.h>
#include <util.h>
#include <pyjit.h>

class CompilerTest {
private:
    py_ptr <PyCodeObject> m_code;
    py_ptr <PyjionJittedCode> m_jittedcode;

    PyObject *run() {
        auto sysModule = PyObject_ptr(PyImport_ImportModule("sys"));
        auto globals = PyObject_ptr(PyDict_New());
        auto builtins = PyEval_GetBuiltins();
        PyDict_SetItemString(globals.get(), "__builtins__", builtins);
        PyDict_SetItemString(globals.get(), "sys", sysModule.get());

        // Don't DECREF as frames are recycled.
        auto frame = PyFrame_New(PyThreadState_Get(), m_code.get(), globals.get(), PyObject_ptr(PyDict_New()).get());
        auto res = m_jittedcode->j_evalfunc(m_jittedcode.get(), frame);
        //Py_DECREF(frame);
        size_t collected = PyGC_Collect();
        printf("Collected %zu values\n", collected);
        REQUIRE(!m_jittedcode->j_failed);
        return res;
    }

public:
    explicit CompilerTest(const char *code) {
        PyErr_Clear();
        m_code.reset(CompileCode(code));
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

TEST_CASE("Test exception handling", "[!mayfail]") {
    SECTION("test UnboundLocalError") {
        auto t = CompilerTest(
                "def f():\n  x = y\n  y = 1"
        );
        CHECK(t.raises() == PyExc_UnboundLocalError);
    }

    SECTION("test simply try catch") {
        auto t = CompilerTest(
                "def f():\n  a=0\n  try:\n    a=1\n  except:\n    a=2\n  return a\n"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test simply try catch and handle implicit null branch") {
        auto t = CompilerTest(
                "def f():\n  a=0\n  try:\n    a=1/0\n  except:\n    a=2\n  return a\n"
        );
        CHECK(t.returns() == "2");
    }

    SECTION("test simply try catch and handle explicit raise branch") {
        auto t = CompilerTest(
                "def f():\n  a=0\n  try:\n    a=1\n    raise Exception('bork')\n  except:\n    a=2\n  return a\n"
        );
        CHECK(t.returns() == "2");
    }

    SECTION("test return within try") {
        auto t = CompilerTest(
                "def f():\n  try:\n    return 1\n  except:\n    return 2\n"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test nested try and pass") {
        auto t = CompilerTest(
                "def f():\n  a = 1\n  try:\n    try:\n      1/0\n    except:\n      a += 2\n  except:\n    a += 4\n  return a"
        );
        CHECK(t.returns() == "3");
    }

    SECTION("test reraise exception") {
        auto t = CompilerTest(
                "def f():\n    try:\n         raise TypeError('hi')\n    except Exception as e:\n         pass\n    finally:\n         pass"
        );
        CHECK(t.returns() == "None");
    }

    SECTION("test generic nested exception") {
        auto t = CompilerTest(
                "def f():\n    try:\n        try:\n             raise Exception('borkborkbork')\n        finally:\n             pass\n    finally:\n        pass"
        );
        CHECK(t.raises() == PyExc_Exception);
    }

    SECTION("test generic implicit exception") {
        auto t = CompilerTest(
                "def f():\n    try:\n        try:\n             1/0\n        finally:\n             pass\n    finally:\n        pass"
        );
        CHECK(t.raises() == PyExc_ZeroDivisionError);
    }

    SECTION("test simple exception filters") {
        auto t = CompilerTest(
                "def f():\n  try:\n    raise TypeError('err')\n  except ValueError:\n    pass\n  return 2\n"
        );
        CHECK(t.raises() == PyExc_TypeError);
    }

    SECTION("test exception filters") {
        auto t = CompilerTest(
                "def f():\n    try:\n        try:\n             try:\n                  raise TypeError('err')\n             except BaseException:\n                  raise\n        finally:\n             pass\n    finally:\n        return 42\n"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("Break from nested try/finally needs to use BranchLeave to clear the stack") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            raise Exception()\n        finally:\n            try:\n                break\n            finally:\n                pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("Break from a double nested try/finally needs to unwind all exceptions") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            raise Exception()\n        finally:\n            try:\n                raise Exception()\n            finally:\n                try:\n                     break\n                finally:\n                    pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("return from nested try/finally should use BranchLeave to clear stack when branching to return label") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        try:\n            return 42\n        finally:\n            pass"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("Return from nested try/finally should unwind nested exception handlers") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        try:\n            raise Exception()\n        finally:\n            try:\n                return 42\n            finally:\n                pass\n    return 23"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("Break from a nested exception handler needs to unwind all exception handlers") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n             raise Exception()\n        except:\n             try:\n                  raise TypeError()\n             finally:\n                  break\n    return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("Return from a nested exception handler needs to unwind all exception handlers") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n             raise Exception()\n        except:\n             try:\n                  raise TypeError()\n             finally:\n                  return 23\n    return 42"
        );
        CHECK(t.returns() == "23");
    }

    SECTION("We need to do BranchLeave to clear the stack when doing a break inside of a finally") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            raise Exception()\n        finally:\n            break\n    return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("test exception raise inside finally") {
        auto t = CompilerTest(
                "def f():\n    try:\n         raise Exception()\n    finally:\n        raise Exception()"
        );
        CHECK(t.raises() == PyExc_SystemError);
    }

    SECTION("test raise in finally causes runtime error") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        raise"
        );
        CHECK(t.raises() == PyExc_RuntimeError);
    }

    SECTION("test raise custom exception in finally") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        raise OSError"
        );
        CHECK(t.raises() == PyExc_OSError);
    }

    SECTION("test stack dump") {
        auto t = CompilerTest(
                "def x():\n     try:\n         b\n     except:\n         c\n\ndef f():\n    try:\n        x()\n    except:\n        pass\n    return sys.exc_info()[0]\n\n"
        );
        CHECK(t.returns() == "None");
    }

    SECTION("test handle in finally") {
        auto t = CompilerTest(
                "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        finally:\n            pass\n    return 1"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test exception in for loop") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            pass"
        );
        CHECK(t.returns() == "None");
    }

    SECTION("test exception in for loop 2") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            pass\n        finally:\n            return i"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test exception in for loop 3") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            return i"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test return constant within finally") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("test really simple try except with explicit errors") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception('hi')\n    except:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("test empty try finally") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("test continue in try block") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            continue\n        finally:\n            return i"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test no handler") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        pass"
        );
        CHECK(t.raises() == PyExc_Exception);
    }

    SECTION("test empty try and return constant in finally") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("test simple handle explicit builtin exception") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    except:\n        return 2"
        );
        CHECK(t.returns() == "2");
    }
}
TEST_CASE("Exception Filters", "[!mayfail]"){
    SECTION("test filters positive case") {
        auto t = CompilerTest(
                "def f():\n    a = 1\n    try:\n        raise Exception()\n    except Exception:\n        a = 2\n    return a"
        );
        CHECK(t.returns() == "2");
    }

    SECTION("test filters negative case") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    except AssertionError:\n        return 2\n    return 4"
        );
        CHECK(t.raises() == PyExc_Exception);
    }

    SECTION("test key error") {
        auto t = CompilerTest(
                "def f():\n    x = {}\n    try:\n        return x[42]\n    except KeyError:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("test pass in try block") {
        auto t = CompilerTest(
                "def f():\n	try:\n		pass\n	except ImportError:\n		pass\n	except Exception as e:\n		pass"
        );
        CHECK(t.returns() == "None");
    }

    SECTION("test handle and return exception arguments") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception(2)\n    except Exception as e:\n        return e.args[0]"
        );
        CHECK(t.returns() == "2");
    }

    SECTION("test raising exception keeps locals in scope") {
        auto t = CompilerTest(
                "def f():\n    for abc in [1,2,3]:\n        try:\n            break\n        except ImportError:\n            continue\n    return abc"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test handle error in except") {
        auto t = CompilerTest(
                "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        except EnvironmentError:\n            pass\n    return 1"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test nested exception matching fallthrough") {
        auto t = CompilerTest(
                "def f():\n    try:\n        try:\n            pass\n        finally:\n            raise OSError\n    except OSError as e:\n        return 1\n    return 0\n"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test match exception fallthrough") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise\n    except RuntimeError:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("test raising exceptions in a loop") {
        auto t = CompilerTest(
                "def f():\n    try:\n        while True:\n            try:\n                raise Exception()\n            except Exception:\n                break\n    finally:\n        pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }
}

TEST_CASE("try else", "[!mayfail]"){
    SECTION("test try except keep scope to else") {
        auto t = CompilerTest(
                "def f():\n  try:\n    a = 1\n  except:\n    a = 2\n  else:\n    a += 4\n  return a"
        );
        CHECK(t.returns() == "5");
    }

    SECTION("test try except keep scope to else with raise") {
        auto t = CompilerTest(
                "def f():\n  try:\n    a = 1/0\n  except:\n    a = 2\n  else:\n    a += 4\n  return a"
        );
        CHECK(t.returns() == "2");
    }

    SECTION("test try except keep scope to else with raise and filter") {
        auto t = CompilerTest(
                "def f():\n  try:\n    a = 1/0\n  except OSError:\n    a = 2\n  else:\n    a += 4\n  return a"
        );
        CHECK(t.returns() == "5");
    }
}