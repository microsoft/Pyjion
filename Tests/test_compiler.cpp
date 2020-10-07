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

#include "stdafx.h"
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

        return res;
    }

public:
    CompilerTest(const char *code) {
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
        PyErr_Clear();
        return excType;
    }
};

TEST_CASE("Legacy JIT Tests", "[float][binary op][inference]") {
    SECTION("test") {
        // EXTENDED_ARG FOR_ITER:
        auto t = CompilerTest(
                "def f():\n"
                "        x = 1\n"
                "        for w in 1, 2, 3, 4:\n"
                "            x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2;\n"
                "            x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2;\n"
                "        return x\n"
        );
        CHECK(t.returns() == "369");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f(): 1.0 / 0"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f(): print(f'x {42}')"
        );
        CHECK(t.returns() == "None");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f(): return f'abc {42}'"
        );
        CHECK(t.returns() == "'abc 42'");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f(): return f'abc {42:3}'"
        );
        CHECK(t.returns() == "'abc  42'");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f(): return f'abc {\"abc\"!a}'"
        );
        CHECK(t.returns() == "\"abc 'abc'\"");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f(): return f'abc {\"abc\"!a:6}'"
        );
        CHECK(t.returns() == "\"abc 'abc' \"");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f(): return f'abc {\"abc\"!r:6}'"
        );
        CHECK(t.returns() == "\"abc 'abc' \"");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f(): return f'abc {\"abc\"!s}'"
        );
        CHECK(t.returns() == "'abc abc'");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for b in range(1):\n        x = b & 1 and -1.0 or 1.0\n    return x"
        );
        CHECK(t.returns() == "1.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = y\n    y = 1"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n         raise TypeError('hi')\n    except Exception as e:\n         pass\n    finally:\n         pass"
        );
        CHECK(t.returns() == "None");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        try:\n             raise Exception('hi')\n        finally:\n             pass\n    finally:\n        pass"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        try:\n             try:\n                  raise TypeError('err')\n             except BaseException:\n                  raise\n        finally:\n             pass\n    finally:\n        return 42\n"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def f(self) -> 42 : pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, q, r):\n    if not (a == q and r == 0):\n        return 42\n    return 23"
//                        );
//CHECK(t.returns() == "42",
//                                  vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(4), PyLong_FromLong(7)}))
//                }
        // Break from nested try/finally needs to use BranchLeave to clear the stack
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            raise Exception()\n        finally:\n            try:\n                break\n            finally:\n                pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }
        // Break from a double nested try/finally needs to unwind all exceptions
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            raise Exception()\n        finally:\n            try:\n                raise Exception()\n            finally:\n                try:\n                     break\n                finally:\n                    pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }
        // return from nested try/finally should use BranchLeave to clear stack when branching to return label
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        try:\n            return 42\n        finally:\n            pass"
        );
        CHECK(t.returns() == "42");
    }
        // Return from nested try/finally should unwind nested exception handlers
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        try:\n            raise Exception()\n        finally:\n            try:\n                return 42\n            finally:\n                pass\n    return 23"
        );
        CHECK(t.returns() == "42");
    }
        // Break from a nested exception handler needs to unwind all exception handlers
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n             raise Exception()\n        except:\n             try:\n                  raise TypeError()\n             finally:\n                  break\n    return 42"
        );
        CHECK(t.returns() == "42");
    }
        // Return from a nested exception handler needs to unwind all exception handlers
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n             raise Exception()\n        except:\n             try:\n                  raise TypeError()\n             finally:\n                  return 23\n    return 42"
        );
        CHECK(t.returns() == "23");
    }
        // We need to do BranchLeave to clear the stack when doing a break inside of a finally
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            raise Exception()\n        finally:\n            break\n    return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n         raise Exception()\n    finally:\n        raise Exception()"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = b'abc'*3\n    return x"
        );
        CHECK(t.returns() == "b'abcabcabc'");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    unbound += 1"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    5 % 0"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    5.0 % 0.0"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    5.0 // 0.0"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    5.0 / 0.0"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 'abc'*3\n    return x"
        );
        CHECK(t.returns() == "'abcabcabc'");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    if 0.0 < 1.0 <= 1.0 == 1.0 >= 1.0 > 0.0 != 1.0:  return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        try:\n            pass\n        finally:\n            raise OSError\n    except OSError as e:\n        return 1\n    return 0\n"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise\n    except RuntimeError:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        while True:\n            try:\n                raise Exception()\n            except Exception:\n                break\n    finally:\n        pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        raise"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        raise OSError"
        );
        CHECK(t.returns() == "<NULL>");
    }
        // partial should be boxed because it's consumed by print after being assigned in the break loop
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    partial = 0\n    while 1:\n        partial = 1\n        break\n    if not partial:\n        print(partial)\n        return True\n    return False\n"
        );
        CHECK(t.returns() == "False");
    }

        // UNARY_NOT/POP_JUMP_IF_FALSE with optimized value on stack should be compiled correctly w/o crashing
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    abc = 1.0\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23"
        );
        CHECK(t.returns() == "23");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    abc = 1\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23"
        );
        CHECK(t.returns() == "23");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    abc = 0.0\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    abc = 0\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23"
        );
        CHECK(t.returns() == "42");
    }
        // Too many items to unpack from list/tuple shouldn't crash
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = [1,2,3]\n    a, b = x"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = (1,2,3)\n    a, b = x"
        );
        CHECK(t.returns() == "<NULL>");
    }
        // failure to unpack shouldn't crash, should raise Python exception
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = [1]\n    a, b, *c = x"
        );
        CHECK(t.returns() == "<NULL>");
    }
        // unpacking non-iterable shouldn't crash
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, b, c = len"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def x():\n     try:\n         b\n     except:\n         c\n\ndef f():\n    try:\n        x()\n    except:\n        pass\n    return sys.exc_info()[0]\n\n"
        );
        CHECK(t.returns() == "None");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    cs = [('CATEGORY', 'CATEGORY_SPACE')]\n    for op, av in cs:\n        while True:\n            break\n        print(op, av)"
        );
        CHECK(t.returns() == "None");
    }

        // +=, -= checks are to avoid constant folding
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 0\n    x += 1\n    x -= 1\n    return x or 1"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 0\n    x += 1\n    x -= 1\n    return x and 1"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    x += 1\n    x -= 1\n    return x or 2"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    x += 1\n    x -= 1\n    return x and 2"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    return x or 1"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    return x and 1"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    return x or 1"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    return x and 1"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    return -x"
        );
        CHECK(t.returns() == "-4611686018427387903");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    return -x"
        );
        CHECK(t.returns() == "-4611686018427387904");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = -4611686018427387904\n    x += 1\n    x -= 1\n    return -x"
        );
        CHECK(t.returns() == "4611686018427387904");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    y = not x\n    return y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    if x:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    if x:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    if not x:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    if not x:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    y = not x\n    return y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x <= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x >= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x != y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x == y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x == y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x != y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x != y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x != y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x != y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x != y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x != y"
        );
        CHECK(t.returns() == "False");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x >= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x >= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x >= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x >= y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x <= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x <= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x <= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x <= y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x > y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775808\n    y = 9223372036854775807\n    return x > y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775808\n    return x > y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x > y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x > y"
        );
        CHECK(t.returns() == "False");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x < y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775808\n    y = 9223372036854775807\n    return x < y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775808\n    return x < y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x < y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x < y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x % y"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x / y"
        );
        CHECK(t.returns() == "0.5");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x / y"
        );
        CHECK(t.returns() == "2.168404344971009e-19");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x / y"
        );
        CHECK(t.returns() == "1.0842021724855044e-19");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x / y"
        );
        CHECK(t.returns() == "4.611686018427388e+18");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x / y"
        );
        CHECK(t.returns() == "9.223372036854776e+18");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x / y"
        );
        CHECK(t.returns() == "1.0");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x >> y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x >> y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x >> y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x >> y"
        );
        CHECK(t.returns() == "2305843009213693951");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x >> y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x >> y"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x << y"
        );
        CHECK(t.returns() == "4");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 32\n    return x << y"
        );
        CHECK(t.returns() == "4294967296");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 62\n    return x << y"
        );
        CHECK(t.returns() == "4611686018427387904");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 63\n    return x << y"
        );
        CHECK(t.returns() == "9223372036854775808");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 64\n    return x << y"
        );
        CHECK(t.returns() == "18446744073709551616");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x << y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x << y"
        );
        CHECK(t.returns() == "18446744073709551614");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x << y"
        );
        CHECK(t.returns() == "<NULL>");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x ** y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 32\n    return x ** y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x ** y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x ** y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x ** y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x ** y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x // y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x // y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x // y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x // y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 4611686018427387903\n    return x // y"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = -4611686018427387903\n    return x // y"
        );
        CHECK(t.returns() == "-3");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x // y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = -1\n    return x // y"
        );
        CHECK(t.returns() == "-9223372036854775807");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x // y"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x % y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x % y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x % y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 4611686018427387903\n    return x % y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = -4611686018427387903\n    return x % y"
        );
        CHECK(t.returns() == "-4611686018427387902");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x % y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = -1\n    return x % y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x % y"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x | y"
        );
        CHECK(t.returns() == "3");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x | y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x | y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x | y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x | y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x | y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x & y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 3\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x & y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x ^ y"
        );
        CHECK(t.returns() == "3");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 3\n    return x ^ y"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x ^ y"
        );
        CHECK(t.returns() == "4611686018427387902");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x ^ y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x ^ y"
        );
        CHECK(t.returns() == "4611686018427387902");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x ^ y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x ^ y"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = -9223372036854775808\n    y = 1\n    return x - y"
        );
        CHECK(t.returns() == "-9223372036854775809");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = -1\n    y = 4611686018427387904\n    return x - y"
        );
        CHECK(t.returns() == "-4611686018427387905");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = -1\n    y = 9223372036854775808\n    return x - y"
        );
        CHECK(t.returns() == "-9223372036854775809");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x =  -4611686018427387904\n    y = 1\n    return x - y"
        );
        CHECK(t.returns() == "-4611686018427387905");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x + y"
        );
        CHECK(t.returns() == "4611686018427387904");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x + y"
        );
        CHECK(t.returns() == "9223372036854775808");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x + y"
        );
        CHECK(t.returns() == "4611686018427387904");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x + y"
        );
        CHECK(t.returns() == "9223372036854775808");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x + y"
        );
        CHECK(t.returns() == "18446744073709551614");
    }

    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2\n    y = 4611686018427387903\n    return x * y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2\n    y = 9223372036854775807\n    return x * y"
        );
        CHECK(t.returns() == "18446744073709551614");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 2\n    return x * y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 2\n    return x * y"
        );
        CHECK(t.returns() == "18446744073709551614");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x * y"
        );
        CHECK(t.returns() == "85070591730234615847396907784232501249");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        except EnvironmentError:\n            pass\n    return 1"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        finally:\n            pass\n    return 1"
        );
        CHECK(t.returns() == "1");
    }
        // Simple optimized code test cases...
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = +x\n    return y"
        );
        CHECK(t.returns() == "1.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    if not x:\n        return 1\n    return 2"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 0.0\n    if not x:\n        return 1\n    return 2"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = -x\n    return y"
        );
        CHECK(t.returns() == "-1.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = not x\n    return y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 0.0\n    y = not x\n    return y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    return x"
        );
        CHECK(t.returns() == "1.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x + y\n    return z"
        );
        CHECK(t.returns() == "3.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x - y\n    return z"
        );
        CHECK(t.returns() == "-1.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x / y\n    return z"
        );
        CHECK(t.returns() == "0.5");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x // y\n    return z"
        );
        CHECK(t.returns() == "0.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x % y\n    return z"
        );
        CHECK(t.returns() == "1.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    z = x * y\n    return z"
        );
        CHECK(t.returns() == "6.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    z = x ** y\n    return z"
        );
        CHECK(t.returns() == "8.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 3.0\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 'a'\n    y = 'b'\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 'a'\n    y = 'a'\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    class Foo(str): pass\n    x = Foo(1)\n    y = Foo(2)\n    if x == y:        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    class Foo(str): pass\n    x = Foo(1)\n    y = Foo(1)\n    if x == y:        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    if x != y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 3.0\n    if x != y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    if x >= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 3.0\n    if x >= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 4.0\n    y = 3.0\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 2.0\n    if x <= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 3.0\n    if x <= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 2.0\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 4.0\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x += y\n    return x"
        );
        CHECK(t.returns() == "3.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x -= y\n    return x"
        );
        CHECK(t.returns() == "-1.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x /= y\n    return x"
        );
        CHECK(t.returns() == "0.5");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x //= y\n    return x"
        );
        CHECK(t.returns() == "0.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x %= y\n    return x"
        );
        CHECK(t.returns() == "1.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    x *= y\n    return x"
        );
        CHECK(t.returns() == "6.0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    x **= y\n    return x"
        );
        CHECK(t.returns() == "8.0");
    }
        // fully optimized complex code
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    pi = 0.\n    k = 0.\n    while k < 256.:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi"
        );
        CHECK(t.returns() == "3.141592653589793");
    }
        // division error handling code gen with value on the stack
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = 3.0\n    return x + y / z"
        );
        CHECK(t.returns() == "1.6666666666666665");
    }
        // division by zero error case
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 0\n    try:\n        return x / y\n    except:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 0\n    try:\n        return x // y\n    except:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(x):\n    if not x:\n        return True\n    return False",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "False", vector<PyObject *>({PyLong_FromLong(1)})),
//                                                  );
//CHECK(t.returns() == "True", vector<PyObject *>({PyLong_FromLong(0)}))
//                                          })}
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    if a in b:\n        return True\n    return False",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "True", vector<PyObject *>(
//                                                          {PyLong_FromLong(42), Incremented((PyObject *) tupleOfOne)})),
//                                                  );
//CHECK(t.returns() == "False", vector<PyObject *>(
//                                                          {PyLong_FromLong(1), Incremented((PyObject *) tupleOfOne)}))
//                                          })}
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    if a not in b:\n        return True\n    return False",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "False", vector<PyObject *>(
//                                                          {PyLong_FromLong(42), Incremented((PyObject *) tupleOfOne)})),
//                                                  );
//CHECK(t.returns() == "True", vector<PyObject *>(
//                                                          {PyLong_FromLong(1), Incremented((PyObject *) tupleOfOne)}))
//                                          })
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    if a is b:\n        return True",
//                        );
//CHECK(t.returns() == "True", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(1)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    if a is not b:\n        return True",
//                        );
//CHECK(t.returns() == "True", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    assert a is b\n    return True",
//                        );
//CHECK(t.returns() == "True", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(1)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    assert a is b\n    return True",
//                        );
//CHECK(t.returns() == "<NULL>", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
//                }
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a = RefCountCheck()\n    del a\n    return finalized"
        );
        CHECK(t.returns() == "True");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in {2:3}:\n        pass\n    return i"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            pass"
        );
        CHECK(t.returns() == "None");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            pass\n        finally:\n            return i"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            return i"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception(2)\n    except Exception as e:\n        return e.args[0]"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(b:1, *, a = 2):\n     return a\n    return g.__annotations__['b']"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(b:1, *, a = 2):\n     return a\n    return g(3)"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(*, a = 2):\n     return a\n    return g()"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(a:1, b:2): pass\n    return g.__annotations__['a']"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    from sys import winver, version_info\n    return winver[0]"
        );
        CHECK(t.returns() == "'3'");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    from sys import winver\n    return winver[0]"
        );
        CHECK(t.returns() == "'3'");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(*a): return a\n    return g(1, 2, 3, **{})"
        );
        CHECK(t.returns() == "(1, 2, 3)");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(**a): return a\n    return g(y = 3, **{})"
        );
        CHECK(t.returns() == "{'y': 3}");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(**a): return a\n    return g(**{'x':2})"
        );
        CHECK(t.returns() == "{'x': 2}");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(**a): return a\n    return g(x = 2, *())"
        );
        CHECK(t.returns() == "{'x': 2}");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(*a): return a\n    return g(*(1, 2, 3))"
        );
        CHECK(t.returns() == "(1, 2, 3)");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(*a): return a\n    return g(1, *(2, 3))"
        );
        CHECK(t.returns() == "(1, 2, 3)");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(): pass\n    g.abc = {fn.lower() for fn in ['A']}\n    return g.abc"
        );
        CHECK(t.returns() == "{'a'}");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for abc in [1,2,3]:\n        try:\n            break\n        except ImportError:\n            continue\n    return abc"
        );
        CHECK(t.returns() == "1");
    }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f():\n    a = 1\n    b = 2\n    def x():\n        return a, b\n    return x()",
//                        );
//CHECK(t.returns() == "(1, 2)", vector<PyObject *>({NULL, PyCell_New(NULL), PyCell_New(NULL)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f() :\n    def g(x) :\n        return x\n    a = 1\n    def x() :\n        return g(a)\n    return x()",
//                        );
//CHECK(t.returns() == "1", vector<PyObject *>({NULL, PyCell_New(NULL), PyCell_New(NULL)}))
//                }
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n	try:\n		pass\n	except ImportError:\n		pass\n	except Exception as e:\n		pass"
        );
        CHECK(t.returns() == "None");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception('hi')\n    except:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }
        //SECTION("test"){auto t = CompilerTest(
        //	"def f(sys_module, _imp_module):\n    '''Setup importlib by importing needed built - in modules and injecting them\n    into the global namespace.\n\n    As sys is needed for sys.modules access and _imp is needed to load built - in\n    modules, those two modules must be explicitly passed in.\n\n    '''\n    global _imp, sys\n    _imp = _imp_module\n    sys = sys_module\n\n    # Set up the spec for existing builtin/frozen modules.\n    module_type = type(sys)\n    for name, module in sys.modules.items():\n        if isinstance(module, module_type):\n            if name in sys.builtin_module_names:\n                loader = BuiltinImporter\n            elif _imp.is_frozen(name):\n                loader = FrozenImporter\n            else:\n                continue\n            spec = _spec_from_module(module, loader)\n            _init_module_attrs(spec, module)\n\n    # Directly load built-in modules needed during bootstrap.\n    self_module = sys.modules[__name__]\n    for builtin_name in ('_warnings',):\n        if builtin_name not in sys.modules:\n            builtin_module = _builtin_from_name(builtin_name)\n        else:\n            builtin_module = sys.modules[builtin_name]\n        setattr(self_module, builtin_name, builtin_module)\n\n    # Directly load the _thread module (needed during bootstrap).\n    try:\n        thread_module = _builtin_from_name('_thread')\n    except ImportError:\n        # Python was built without threads\n        thread_module = None\n    setattr(self_module, '_thread', thread_module)\n\n    # Directly load the _weakref module (needed during bootstrap).\n    weakref_module = _builtin_from_name('_weakref')\n    setattr(self_module, '_weakref', weakref_module)\n",
        //	);
        // CHECK(t.returns() == "42");
        //}
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = {}\n    try:\n        return x[42]\n    except KeyError:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = {}\n    x.update(y=2)\n    return x"
        );
        CHECK(t.returns() == "{'y': 2}");
    }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a):\n    return 1 if a else 2",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "1", vector<PyObject *>({PyLong_FromLong(42)})),
//                                                  );
//CHECK(t.returns() == "2", vector<PyObject *>({PyLong_FromLong(0)})),
//                                                  );
//CHECK(t.returns() == "1", vector<PyObject *>({Incremented(Py_True)})),
//                                                  );
//CHECK(t.returns() == "2", vector<PyObject *>({Incremented(Py_False)}))
//                                          })
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(v):\n    try:\n        x = v.real\n    except AttributeError:\n        return 42\n    else:\n        return x\n",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "1", vector<PyObject *>({PyLong_FromLong(1)})),
//                                                  );
//CHECK(t.returns() == "42", vector<PyObject *>({PyUnicode_FromString("hi")}))
//                                          })}
//                SECTION("test"){auto t = CompilerTest(
//                        "def f():\n    a=1\n    def x():\n        return a\n    return a",
//                        );
//CHECK(t.returns() == "1", vector<PyObject *>({NULL, PyCell_New(NULL)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f():\n    x = 2\n    def g():    return x\n    \n    str(g)\n    return g()",
//                        );
//CHECK(t.returns() == "2", vector<PyObject *>({NULL, PyCell_New(NULL)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f():\n    a=2\n    def g(): return a\n    return g()",
//                        );
//CHECK(t.returns() == "2", vector<PyObject *>({NULL, PyCell_New(NULL)}))
//                }
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(a=2): return a\n    return g()"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            continue\n        finally:\n            return i"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        pass"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    except:\n        return 2"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    except Exception:\n        return 2"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    except AssertionError:\n        return 2\n    return 4"
        );
        CHECK(t.returns() == "<NULL>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    global x\n    x = 2\n    return x"
        );
        CHECK(t.returns() == "2");
    }
//                SECTION("test"){auto t = CompilerTest(    // hits JUMP_FORWARD
//                        "def f(a):\n    if a:\n        x = 1\n    else:\n        x = 2\n    return x",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "1", vector<PyObject *>({PyLong_FromLong(42)})),
//                                                  );
//CHECK(t.returns() == "2", vector<PyObject *>({PyLong_FromLong(0)})),
//                                                  );
//CHECK(t.returns() == "1", vector<PyObject *>({Incremented(Py_True)})),
//                                                  );
//CHECK(t.returns() == "2", vector<PyObject *>({Incremented(Py_False)}))
//                                          })}
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    return a and b",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "2", vector<PyObject *>(
//                                                          {PyLong_FromLong(1), PyLong_FromLong(2)})),
//                                                  );
//CHECK(t.returns() == "0", vector<PyObject *>(
//                                                          {PyLong_FromLong(0), PyLong_FromLong(1)})),
//                                                  );
//CHECK(t.returns() == "False", vector<PyObject *>({Py_True, Py_False})),
//                                                  );
//CHECK(t.returns() == "False", vector<PyObject *>({Py_False, Py_True})),
//                                                  );
//CHECK(t.returns() == "True", vector<PyObject *>({Py_True, Py_True})),
//                                                  );
//CHECK(t.returns() == "False", vector<PyObject *>({Py_False, Py_False}))
//                                          })
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    return a or b",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "1", vector<PyObject *>(
//                                                          {PyLong_FromLong(1), PyLong_FromLong(2)})),
//                                                  );
//CHECK(t.returns() == "1", vector<PyObject *>(
//                                                          {PyLong_FromLong(0), PyLong_FromLong(1)})),
//                                                  );
//CHECK(t.returns() == "True", vector<PyObject *>({Py_True, Py_False})),
//                                                  );
//CHECK(t.returns() == "True", vector<PyObject *>({Py_False, Py_True})),
//                                                  );
//CHECK(t.returns() == "True", vector<PyObject *>({Py_True, Py_True})),
//                                                  );
//CHECK(t.returns() == "False", vector<PyObject *>({Py_False, Py_False}))
//                                          })}
//                SECTION("test"){auto t = CompilerTest(         // hits ROT_TWO, POP_TWO, DUP_TOP, ROT_THREE
//                        "def f(a, b, c):\n    return a < b < c",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "True", vector<PyObject *>(
//                                                          {PyLong_FromLong(2), PyLong_FromLong(3),
//                                                           PyLong_FromLong(4)})),
//                                                  );
//CHECK(t.returns() == "False", vector<PyObject *>(
//                                                          {PyLong_FromLong(4), PyLong_FromLong(3), PyLong_FromLong(2)}))
//                                          })
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a **= b\n    return a",
//                        );
//CHECK(t.returns() == "8", vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(3)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a *= b\n    return a",
//                        );
//CHECK(t.returns() == "6", vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(3)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a /= b\n    return a",
//                        );
//CHECK(t.returns() == "0.5", vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(4)}))
//                }
//
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a %= b\n    return a",
//                        );
//CHECK(t.returns() == "1", vector<PyObject *>({PyLong_FromLong(3), PyLong_FromLong(2)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a -= b\n    return a",
//                        );
//CHECK(t.returns() == "1", vector<PyObject *>({PyLong_FromLong(3), PyLong_FromLong(2)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a <<= b\n    return a",
//                        );
//CHECK(t.returns() == "4", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a >>= b\n    return a",
//                        );
//CHECK(t.returns() == "1", vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(1)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a &= b\n    return a",
//                        );
//CHECK(t.returns() == "2", vector<PyObject *>({PyLong_FromLong(6), PyLong_FromLong(3)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a ^= b\n    return a",
//                        );
//CHECK(t.returns() == "5", vector<PyObject *>({PyLong_FromLong(6), PyLong_FromLong(3)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b):\n    a |= b\n    return a",
//                        );
//CHECK(t.returns() == "7", vector<PyObject *>({PyLong_FromLong(6), PyLong_FromLong(3)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(x):\n    return -x",
//                        );
//CHECK(t.returns() == "-1", vector<PyObject *>({PyLong_FromLong(1)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(x):\n    return +x",
//                        );
//CHECK(t.returns() == "1", vector<PyObject *>({PyLong_FromLong(1)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(x):\n    return ~x",
//                        );
//CHECK(t.returns() == "-2", vector<PyObject *>({PyLong_FromLong(1)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b): return a << b",
//                        );
//CHECK(t.returns() == "4", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b): return a >> b",
//                        );
//CHECK(t.returns() == "2", vector<PyObject *>({PyLong_FromLong(4), PyLong_FromLong(1)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b): return a & b",
//                        );
//CHECK(t.returns() == "2", vector<PyObject *>({PyLong_FromLong(6), PyLong_FromLong(3)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b): return a | b",
//                        );
//CHECK(t.returns() == "3", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(a, b): return a ^ b",
//                        );
//CHECK(t.returns() == "6", vector<PyObject *>({PyLong_FromLong(3), PyLong_FromLong(5)}))
//                }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f(x):\n    return not x",
//                        vector<TestInput>({
//                                                  );
//CHECK(t.returns() == "False", vector<PyObject *>({PyLong_FromLong(1)}));
//CHECK(t.returns() == "True", vector<PyObject *>({PyLong_FromLong(0)}));
//                                          })}
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(3):\n        if i == 0: continue\n        break\n    return i"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    for i in range(3):\n        if i == 1: break\n    return i"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    return [1,2,3][1:]"
        );
        CHECK(t.returns() == "[2, 3]");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    return [1,2,3][:1]"
        );
        CHECK(t.returns() == "[1]");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    return [1,2,3][1:2]"
        );
        CHECK(t.returns() == "[2]");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    return [1,2,3][0::2]"
        );
        CHECK(t.returns() == "[1, 3]");
    }
//                SECTION("test"){auto t = CompilerTest(
//                        "def f():\n    x = 2\n    def g():    return x\n    return g()",
//                        );
//CHECK(t.returns() == "2", vector<PyObject *>({NULL, PyCell_New(NULL)}))
//                }
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = range(3)\n    return a"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = range(3)\n    return b"
        );
        CHECK(t.returns() == "[1]");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = range(3)\n    return c"
        );
        CHECK(t.returns() == "2");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 2, 3\n    return a"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 2, 3\n    return b"
        );
        CHECK(t.returns() == "[2]");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 2, 3\n    return c"
        );
        CHECK(t.returns() == "3");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 3\n    return c"
        );
        CHECK(t.returns() == "3");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 3\n    return b"
        );
        CHECK(t.returns() == "[]");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 2, 3]\n    return a"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 2, 3]\n    return b"
        );
        CHECK(t.returns() == "[2]");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 2, 3]\n    return c"
        );
        CHECK(t.returns() == "3");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 3]\n    return c"
        );
        CHECK(t.returns() == "3");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 3]\n    return b"
        );
        CHECK(t.returns() == "[]");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, b = range(2)\n    return a"
        );
        CHECK(t.returns() == "0");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a, b = 1, 2\n    return a"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    class C:\n        pass\n    return C"
        );
        CHECK(t.returns() == "<class 'C'>");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    a = 0\n    for x in[1]:\n        a = a + 1\n    return a"
        );
        CHECK(t.returns() == "1");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f(): return [x for x in range(2)]"
        );
        CHECK(t.returns() == "[0, 1]");
    }SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    def g(): pass\n    return g.__name__"
        );
        CHECK(t.returns() == "'g'");
    };
}
