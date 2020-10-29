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
TEST_CASE("test imports"){
    SECTION("import this") {
        auto t = CompilerTest(
                "def f():\n  import this"
        );
        CHECK(t.returns() == "None");
    };
}
TEST_CASE("Test ITER", "[float][binary op][inference]") {
    SECTION("test1") {
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
    }
}
TEST_CASE("Test math errors") {
    SECTION("test2") {
        auto t = CompilerTest(
                "def f(): 1.0 / 0"
        );
        CHECK(t.raises() == PyExc_ZeroDivisionError);
    }
}
TEST_CASE("Test f-strings") {
    SECTION("test3") {
        auto t = CompilerTest(
                "def f(): print(f'x {42}')"
        );
        CHECK(t.returns() == "None");
    }SECTION("test4") {
        auto t = CompilerTest(
                "def f(): return f'abc {42}'"
        );
        CHECK(t.returns() == "'abc 42'");
    }

    SECTION("test5") {
        auto t = CompilerTest(
                "def f(): return f'abc {42:3}'"
        );
        CHECK(t.returns() == "'abc  42'");
    }SECTION("test6") {
        auto t = CompilerTest(
                "def f(): return f'abc {\"abc\"!a}'"
        );
        CHECK(t.returns() == "\"abc 'abc'\"");
    }

    SECTION("test f-strings") {
        auto t = CompilerTest(
                "def f(): return f'abc {\"abc\"!a:6}'"
        );
        CHECK(t.returns() == "\"abc 'abc' \"");
    }SECTION("test f-strings 1") {
        auto t = CompilerTest(
                "def f(): return f'abc {\"abc\"!r:6}'"
        );
        CHECK(t.returns() == "\"abc 'abc' \"");
    }SECTION("test f-strings 2") {
        auto t = CompilerTest(
                "def f(): return f'abc {\"abc\"!s}'"
        );
        CHECK(t.returns() == "'abc abc'");
    }
}
TEST_CASE("Test ranges") {
    SECTION("test in range") {
        auto t = CompilerTest(
                "def f():\n    for b in range(1):\n        x = b & 1 and -1.0 or 1.0\n    return x"
        );
        CHECK(t.returns() == "1.0");
    }
}

TEST_CASE("Test method loads and calls") {
    SECTION("Test method call"){
        auto t = CompilerTest(
                "def f():\n  a = [1,2,3]\n  a.append(4)\n  return a"
        );
        CHECK(t.returns() == "[1, 2, 3, 4]");
    }
}

TEST_CASE("Test general errors") {
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
}
TEST_CASE("Annotation tests") {
    SECTION("test annotations") {
        auto t = CompilerTest(
                "def f():\n    def f(self) -> 42 : pass\n    return 42"
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
}
TEST_CASE("Test math operations") {
    SECTION("test binary multiply") {
        auto t = CompilerTest(
                "def f():\n    x = b'abc'*3\n    return x"
        );
        CHECK(t.returns() == "b'abcabcabc'");
    }
    SECTION("test increment unbound ") {
        auto t = CompilerTest(
                "def f():\n    unbound += 1"
        );
        CHECK(t.raises() == PyExc_UnboundLocalError);
    }
    SECTION("test modulus by zero") {
        auto t = CompilerTest(
                "def f():\n    5 % 0"
        );
        CHECK(t.raises() == PyExc_ZeroDivisionError);
    }
    SECTION("test modulus floats by zero") {
        auto t = CompilerTest(
                "def f():\n    5.0 % 0.0"
        );
        CHECK(t.raises() == PyExc_ZeroDivisionError);
    }
    SECTION("test floor divide by zero") {
        auto t = CompilerTest(
                "def f():\n    5.0 // 0.0"
        );
        CHECK(t.raises() == PyExc_ZeroDivisionError);
    }
    SECTION("test divide by zero") {
        auto t = CompilerTest(
                "def f():\n    5.0 / 0.0"
        );
        CHECK(t.raises() == PyExc_ZeroDivisionError);
    }
    SECTION("test string multiply") {
        auto t = CompilerTest(
                "def f():\n    x = 'abc'*3\n    return x"
        );
        CHECK(t.returns() == "'abcabcabc'");
    }
    SECTION("test boundary ranging") {
        auto t = CompilerTest(
                "def f():\n    if 0.0 < 1.0 <= 1.0 == 1.0 >= 1.0 > 0.0 != 1.0:  return 42"
        );
        CHECK(t.returns() == "42");
    }
}
TEST_CASE("Test try") {
    SECTION("test1") {
        auto t = CompilerTest(
                "def f():\n    try:\n        try:\n            pass\n        finally:\n            raise OSError\n    except OSError as e:\n        return 1\n    return 0\n"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test2") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise\n    except RuntimeError:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("test3") {
        auto t = CompilerTest(
                "def f():\n    try:\n        while True:\n            try:\n                raise Exception()\n            except Exception:\n                break\n    finally:\n        pass\n    return 42"
        );
        CHECK(t.returns() == "42");
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
}
TEST_CASE("X Test boxing") {
    SECTION("partial should be boxed because it's consumed by print after being assigned in the break loop") {
        auto t = CompilerTest(
                "def f():\n    partial = 0\n    while 1:\n        partial = 1\n        break\n    if not partial:\n        print(partial)\n        return True\n    return False\n"
        );
        CHECK(t.returns() == "False");
    }

    SECTION("UNARY_NOT/POP_JUMP_IF_FALSE with optimized value on stack should be compiled correctly w/o crashing") {
        auto t = CompilerTest(
                "def f():\n    abc = 1.0\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23"
        );
        CHECK(t.returns() == "23");
    }

    SECTION("test3") {
        auto t = CompilerTest(
                "def f():\n    abc = 1\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23"
        );
        CHECK(t.returns() == "23");
    }

    SECTION("test4") {
        auto t = CompilerTest(
                "def f():\n    abc = 0.0\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("test5") {
        auto t = CompilerTest(
                "def f():\n    abc = 0\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23"
        );
        CHECK(t.returns() == "42");
    }

    SECTION("Too many items to unpack from list raises valueerror") {
        auto t = CompilerTest(
                "def f():\n    x = [1,2,3]\n    a, b = x"
        );
        CHECK(t.raises() == PyExc_ValueError);
    }

    SECTION("Too many items to unpack from tuple raises valueerror") {
        auto t = CompilerTest(
                "def f():\n    x = (1,2,3)\n    a, b = x"
        );
        CHECK(t.raises() == PyExc_ValueError);
    }

    SECTION("failure to unpack shouldn't crash, should raise Python exception") {
        auto t = CompilerTest(
                "def f():\n    x = [1]\n    a, b, *c = x"
        );
        CHECK(t.raises() == PyExc_ValueError);
    }

    SECTION("unpacking non-iterable shouldn't crash") {
        auto t = CompilerTest(
                "def f():\n    a, b, c = len"
        );
        CHECK(t.raises() == PyExc_TypeError);
    }

    SECTION("test stack dump") {
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

}
TEST_CASE("X Conditional returns") {
    // +=, -= checks are to avoid constant folding
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 0\n    x += 1\n    x -= 1\n    return x or 1"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test2") {
        auto t = CompilerTest(
                "def f():\n    x = 0\n    x += 1\n    x -= 1\n    return x and 1"
        );
        CHECK(t.returns() == "0");
    }
    SECTION("test3") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    x += 1\n    x -= 1\n    return x or 2"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test4") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    x += 1\n    x -= 1\n    return x and 2"
        );
        CHECK(t.returns() == "2");
    }
    SECTION("test5") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    return x or 1"
        );
        CHECK(t.returns() == "4611686018427387903");
    }
    SECTION("test6") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    return x and 1"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test7") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    return x or 1"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test8") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    return x and 1"
        );
        CHECK(t.returns() == "0");
    }
    SECTION("test9") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    return -x"
        );
        CHECK(t.returns() == "-4611686018427387903");
    }
    SECTION("test10") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    return -x"
        );
        CHECK(t.returns() == "-4611686018427387904");
    }
    SECTION("test11") {
        auto t = CompilerTest(
                "def f():\n    x = -4611686018427387904\n    x += 1\n    x -= 1\n    return -x"
        );
        CHECK(t.returns() == "4611686018427387904");
    }
}
TEST_CASE("Unary tests") {
    SECTION("most basic unary not") {
        auto t = CompilerTest(
                "def f():\n  x=True\n  return not x\n"
        );
        CHECK(t.returns() == "False");
    }SECTION("in place add") {
        auto t = CompilerTest(
                "def f():\n  x=1\n  x+=1\n  return x"
        );
        CHECK(t.returns() == "2");
    }SECTION("test1") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    y = not x\n    return y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test2") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    if x:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test3") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    if x:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }

    SECTION("test4") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    if not x:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test5") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    if not x:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test6") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    y = not x\n    return y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test7") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test8") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x <= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test9") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x >= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test10") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x != y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test11") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test12") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test13") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test14") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test15") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test16") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }

    SECTION("test17") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test18") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test19") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test20") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x == y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test21") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x == y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test22") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test23") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x != y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test24") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x != y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test25") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x != y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test26") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x != y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test27") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x != y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test28") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x != y"
        );
        CHECK(t.returns() == "False");
    }

    SECTION("test29") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x >= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test30") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x >= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test31") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x >= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test32") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x >= y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test33") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x <= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test34") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x <= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test35") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x <= y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test36") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x <= y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test37") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x > y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test38") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775808\n    y = 9223372036854775807\n    return x > y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test39") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775808\n    return x > y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test40") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x > y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test41") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x > y"
        );
        CHECK(t.returns() == "False");
    }

    SECTION("test42") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x < y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test43") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775808\n    y = 9223372036854775807\n    return x < y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test44") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775808\n    return x < y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test45") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x < y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test46") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x < y"
        );
        CHECK(t.returns() == "True");
    }

    SECTION("test47") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 1\n    return x == y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test48") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x % y"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test49") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x / y"
        );
        CHECK(t.returns() == "0.5");
    }SECTION("test50") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x / y"
        );
        CHECK(t.returns() == "2.168404344971009e-19");
    }SECTION("test51") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x / y"
        );
        CHECK(t.returns() == "1.0842021724855044e-19");
    }SECTION("test52") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x / y"
        );
        CHECK(t.returns() == "4.611686018427388e+18");
    }SECTION("test53") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x / y"
        );
        CHECK(t.returns() == "9.223372036854776e+18");
    }SECTION("test54") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x / y"
        );
        CHECK(t.returns() == "1.0");
    }

    SECTION("test55") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x >> y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test56") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x >> y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test57") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x >> y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test58") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x >> y"
        );
        CHECK(t.returns() == "2305843009213693951");
    }SECTION("test59") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x >> y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test60") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x >> y"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test61") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x << y"
        );
        CHECK(t.returns() == "4");
    }SECTION("test62") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 32\n    return x << y"
        );
        CHECK(t.returns() == "4294967296");
    }SECTION("test63") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 62\n    return x << y"
        );
        CHECK(t.returns() == "4611686018427387904");
    }SECTION("test64") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 63\n    return x << y"
        );
        CHECK(t.returns() == "9223372036854775808");
    }SECTION("test65") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 64\n    return x << y"
        );
        CHECK(t.returns() == "18446744073709551616");
    }SECTION("test66") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x << y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test67") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x << y"
        );
        CHECK(t.returns() == "18446744073709551614");
    }SECTION("test68") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x << y"
        );
        CHECK(t.raises() == PyExc_MemoryError);
    }

    SECTION("test69") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x ** y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test70") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 32\n    return x ** y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test71") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x ** y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test72") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x ** y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test73") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x ** y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test74") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x ** y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }

    SECTION("test75") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x // y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test76") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x // y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test77") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x // y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test78") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x // y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test79") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 4611686018427387903\n    return x // y"
        );
        CHECK(t.returns() == "2");
    }SECTION("test80") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = -4611686018427387903\n    return x // y"
        );
        CHECK(t.returns() == "-3");
    }SECTION("test81") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x // y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }SECTION("test82") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = -1\n    return x // y"
        );
        CHECK(t.returns() == "-9223372036854775807");
    }SECTION("test83") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x // y"
        );
        CHECK(t.returns() == "1");
    }

    SECTION("test84") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x % y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test85") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x % y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test86") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x % y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test87") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 4611686018427387903\n    return x % y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test88") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = -4611686018427387903\n    return x % y"
        );
        CHECK(t.returns() == "-4611686018427387902");
    }SECTION("test89") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x % y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test90") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = -1\n    return x % y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test91") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x % y"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test92") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x | y"
        );
        CHECK(t.returns() == "3");
    }SECTION("test93") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x | y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test94") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x | y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }SECTION("test95") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x | y"
        );
        CHECK(t.returns() == "4611686018427387903");
    }SECTION("test96") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x | y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }SECTION("test97") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x | y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }SECTION("test98") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x & y"
        );
        CHECK(t.returns() == "0");
    }SECTION("test99") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 3\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test100") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test101") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test102") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test103") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x & y"
        );
        CHECK(t.returns() == "1");
    }SECTION("test104") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x & y"
        );
        CHECK(t.returns() == "9223372036854775807");
    }

    SECTION("test105") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 2\n    return x ^ y"
        );
        CHECK(t.returns() == "3");
    }SECTION("test106") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 3\n    return x ^ y"
        );
        CHECK(t.returns() == "2");
    }SECTION("test107") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x ^ y"
        );
        CHECK(t.returns() == "4611686018427387902");
    }SECTION("test108") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x ^ y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test109") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x ^ y"
        );
        CHECK(t.returns() == "4611686018427387902");
    }SECTION("test110") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x ^ y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test111") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x ^ y"
        );
        CHECK(t.returns() == "0");
    }

    SECTION("test112") {
        auto t = CompilerTest(
                "def f():\n    x = -9223372036854775808\n    y = 1\n    return x - y"
        );
        CHECK(t.returns() == "-9223372036854775809");
    }SECTION("test113") {
        auto t = CompilerTest(
                "def f():\n    x = -1\n    y = 4611686018427387904\n    return x - y"
        );
        CHECK(t.returns() == "-4611686018427387905");
    }SECTION("test114") {
        auto t = CompilerTest(
                "def f():\n    x = -1\n    y = 9223372036854775808\n    return x - y"
        );
        CHECK(t.returns() == "-9223372036854775809");
    }SECTION("test115") {
        auto t = CompilerTest(
                "def f():\n    x =  -4611686018427387904\n    y = 1\n    return x - y"
        );
        CHECK(t.returns() == "-4611686018427387905");
    }

    SECTION("test116") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 4611686018427387903\n    return x + y"
        );
        CHECK(t.returns() == "4611686018427387904");
    }SECTION("test117") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 9223372036854775807\n    return x + y"
        );
        CHECK(t.returns() == "9223372036854775808");
    }SECTION("test118") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 1\n    return x + y"
        );
        CHECK(t.returns() == "4611686018427387904");
    }SECTION("test119") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 1\n    return x + y"
        );
        CHECK(t.returns() == "9223372036854775808");
    }SECTION("test120") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x + y"
        );
        CHECK(t.returns() == "18446744073709551614");
    }

    SECTION("test121") {
        auto t = CompilerTest(
                "def f():\n    x = 2\n    y = 4611686018427387903\n    return x * y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test122") {
        auto t = CompilerTest(
                "def f():\n    x = 2\n    y = 9223372036854775807\n    return x * y"
        );
        CHECK(t.returns() == "18446744073709551614");
    }SECTION("test123") {
        auto t = CompilerTest(
                "def f():\n    x = 4611686018427387903\n    y = 2\n    return x * y"
        );
        CHECK(t.returns() == "9223372036854775806");
    }SECTION("test124") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 2\n    return x * y"
        );
        CHECK(t.returns() == "18446744073709551614");
    }SECTION("test125") {
        auto t = CompilerTest(
                "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x * y"
        );
        CHECK(t.returns() == "85070591730234615847396907784232501249");
    }SECTION("test126") {
        auto t = CompilerTest(
                "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        except EnvironmentError:\n            pass\n    return 1"
        );
        CHECK(t.returns() == "1");
    }SECTION("test127") {
        auto t = CompilerTest(
                "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        finally:\n            pass\n    return 1"
        );
        CHECK(t.returns() == "1");
    }
}
TEST_CASE("optimized cases") {
        // Simple optimized code test cases...
    SECTION("test") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = +x\n    return y"
        );
        CHECK(t.returns() == "1.0");
    }SECTION("test2") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    if not x:\n        return 1\n    return 2"
        );
        CHECK(t.returns() == "2");
    }SECTION("test3") {
        auto t = CompilerTest(
                "def f():\n    x = 0.0\n    if not x:\n        return 1\n    return 2"
        );
        CHECK(t.returns() == "1");
    }SECTION("test4") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = -x\n    return y"
        );
        CHECK(t.returns() == "-1.0");
    }SECTION("test5") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = not x\n    return y"
        );
        CHECK(t.returns() == "False");
    }SECTION("test6") {
        auto t = CompilerTest(
                "def f():\n    x = 0.0\n    y = not x\n    return y"
        );
        CHECK(t.returns() == "True");
    }SECTION("test7") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    return x"
        );
        CHECK(t.returns() == "1.0");
    }SECTION("test8") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x + y\n    return z"
        );
        CHECK(t.returns() == "3.0");
    }SECTION("test9") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x - y\n    return z"
        );
        CHECK(t.returns() == "-1.0");
    }SECTION("test10") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x / y\n    return z"
        );
        CHECK(t.returns() == "0.5");
    }SECTION("test11") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x // y\n    return z"
        );
        CHECK(t.returns() == "0.0");
    }SECTION("test12") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = x % y\n    return z"
        );
        CHECK(t.returns() == "1.0");
    }SECTION("test13") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    z = x * y\n    return z"
        );
        CHECK(t.returns() == "6.0");
    }SECTION("test14") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    z = x ** y\n    return z"
        );
        CHECK(t.returns() == "8.0");
    }SECTION("test15") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test16") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 3.0\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test17") {
        auto t = CompilerTest(
                "def f():\n    x = 'a'\n    y = 'b'\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test18") {
        auto t = CompilerTest(
                "def f():\n    x = 'a'\n    y = 'a'\n    if x == y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test19") {
        auto t = CompilerTest(
                "def f():\n    class Foo(str): pass\n    x = Foo(1)\n    y = Foo(2)\n    if x == y:        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test20") {
        auto t = CompilerTest(
                "def f():\n    class Foo(str): pass\n    x = Foo(1)\n    y = Foo(1)\n    if x == y:        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test21") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    if x != y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test22") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 3.0\n    if x != y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test23") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    if x >= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test24") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 3.0\n    if x >= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test25") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test26") {
        auto t = CompilerTest(
                "def f():\n    x = 4.0\n    y = 3.0\n    if x > y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test27") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 2.0\n    if x <= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test28") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 3.0\n    if x <= y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test29") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 2.0\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "False");
    }SECTION("test30") {
        auto t = CompilerTest(
                "def f():\n    x = 3.0\n    y = 4.0\n    if x < y:\n        return True\n    return False"
        );
        CHECK(t.returns() == "True");
    }SECTION("test31") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x += y\n    return x"
        );
        CHECK(t.returns() == "3.0");
    }SECTION("test32") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x -= y\n    return x"
        );
        CHECK(t.returns() == "-1.0");
    }SECTION("test33") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x /= y\n    return x"
        );
        CHECK(t.returns() == "0.5");
    }SECTION("test34") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x //= y\n    return x"
        );
        CHECK(t.returns() == "0.0");
    }SECTION("test35") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    x %= y\n    return x"
        );
        CHECK(t.returns() == "1.0");
    }SECTION("test36") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    x *= y\n    return x"
        );
        CHECK(t.returns() == "6.0");
    }SECTION("test37") {
        auto t = CompilerTest(
                "def f():\n    x = 2.0\n    y = 3.0\n    x **= y\n    return x"
        );
        CHECK(t.returns() == "8.0");
    }
        // fully optimized complex code
    SECTION("test38") {
        auto t = CompilerTest(
                "def f():\n    pi = 0.\n    k = 0.\n    while k < 256.:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi"
        );
        CHECK(t.returns() == "3.141592653589793");
    }
        // division error handling code gen with value on the stack
    SECTION("test39") {
        auto t = CompilerTest(
                "def f():\n    x = 1.0\n    y = 2.0\n    z = 3.0\n    return x + y / z"
        );
        CHECK(t.returns() == "1.6666666666666665");
    }
        // division by zero error case
    SECTION("test40") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 0\n    try:\n        return x / y\n    except:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }SECTION("test41") {
        auto t = CompilerTest(
                "def f():\n    x = 1\n    y = 0\n    try:\n        return x // y\n    except:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }
    SECTION("test43") {
        auto t = CompilerTest(
                "def f():\n    a = RefCountCheck()\n    del a\n    return finalized"
        );
        CHECK(t.raises() == PyExc_NameError);
    }SECTION("test44") {
        auto t = CompilerTest(
                "def f():\n    for i in {2:3}:\n        pass\n    return i"
        );
        CHECK(t.returns() == "2");
    }SECTION("test45") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            pass"
        );
        CHECK(t.returns() == "None");
    }SECTION("test46") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            pass\n        finally:\n            return i"
        );
        CHECK(t.returns() == "0");
    }SECTION("test47") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            return i"
        );
        CHECK(t.returns() == "0");
    }SECTION("test48") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception(2)\n    except Exception as e:\n        return e.args[0]"
        );
        CHECK(t.returns() == "2");
    }SECTION("test49") {
        auto t = CompilerTest(
                "def f():\n    def g(b:1, *, a = 2):\n     return a\n    return g.__annotations__['b']"
        );
        CHECK(t.returns() == "1");
    }SECTION("test50") {
        auto t = CompilerTest(
                "def f():\n    def g(b:1, *, a = 2):\n     return a\n    return g(3)"
        );
        CHECK(t.returns() == "2");
    }
    SECTION("test51") {
        auto t = CompilerTest(
                "def f():\n    def g(*, a = 2):\n     return a\n    return g()"
        );
        CHECK(t.returns() == "2");
    }SECTION("test52") {
        auto t = CompilerTest(
                "def f():\n    def g(a:1, b:2): pass\n    return g.__annotations__['a']"
        );
        CHECK(t.returns() == "1");
    }SECTION("test55") {
        auto t = CompilerTest(
                "def f():\n    def g(*a): return a\n    return g(1, 2, 3, **{})"
        );
        CHECK(t.returns() == "(1, 2, 3)");
    }SECTION("test56") {
        auto t = CompilerTest(
                "def f():\n    def g(**a): return a\n    return g(y = 3, **{})"
        );
        CHECK(t.returns() == "{'y': 3}");
    }SECTION("test57") {
        auto t = CompilerTest(
                "def f():\n    def g(**a): return a\n    return g(**{'x':2})"
        );
        CHECK(t.returns() == "{'x': 2}");
    }SECTION("test58") {
        auto t = CompilerTest(
                "def f():\n    def g(**a): return a\n    return g(x = 2, *())"
        );
        CHECK(t.returns() == "{'x': 2}");
    }SECTION("test59") {
        auto t = CompilerTest(
                "def f():\n    def g(*a): return a\n    return g(*(1, 2, 3))"
        );
        CHECK(t.returns() == "(1, 2, 3)");
    }
    SECTION("test60") {
        auto t = CompilerTest(
                "def f():\n    def g(*a): return a\n    return g(1, *(2, 3))"
        );
        CHECK(t.returns() == "(1, 2, 3)");
    }
    SECTION("test61") {
        auto t = CompilerTest(
                "def f():\n    def g(): pass\n    g.abc = {fn.lower() for fn in ['A']}\n    return g.abc"
        );
        CHECK(t.returns() == "{'a'}");
    }
    SECTION("test62") {
        auto t = CompilerTest(
                "def f():\n    for abc in [1,2,3]:\n        try:\n            break\n        except ImportError:\n            continue\n    return abc"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test63") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }
    SECTION("test64") {
        auto t = CompilerTest(
                "def f():\n	try:\n		pass\n	except ImportError:\n		pass\n	except Exception as e:\n		pass"
        );
        CHECK(t.returns() == "None");
    }
    SECTION("test65") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception('hi')\n    except:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }
    SECTION("test66") {
        auto t = CompilerTest(
                "def f():\n    x = {}\n    try:\n        return x[42]\n    except KeyError:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }
    SECTION("test67") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        pass\n    return 42"
        );
        CHECK(t.returns() == "42");
    }
    SECTION("test68") {
        auto t = CompilerTest(
                "def f():\n    x = {}\n    x.update(y=2)\n    return x"
        );
        CHECK(t.returns() == "{'y': 2}");
    }
    SECTION("test69") {
        auto t = CompilerTest(
                "def f():\n    def g(a=2): return a\n    return g()"
        );
        CHECK(t.returns() == "2");
    }
    SECTION("test70") {
        auto t = CompilerTest(
                "def f():\n    for i in range(5):\n        try:\n            continue\n        finally:\n            return i"
        );
        CHECK(t.returns() == "0");
    }
    SECTION("test71") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    finally:\n        pass"
        );
        CHECK(t.raises() == PyExc_Exception);
    }
    SECTION("test72") {
        auto t = CompilerTest(
                "def f():\n    try:\n        pass\n    finally:\n        return 42"
        );
        CHECK(t.returns() == "42");
    }
    SECTION("test73") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    except:\n        return 2"
        );
        CHECK(t.returns() == "2");
    }
    SECTION("test74") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    except Exception:\n        return 2"
        );
        CHECK(t.returns() == "2");
    }
    SECTION("test75") {
        auto t = CompilerTest(
                "def f():\n    try:\n        raise Exception()\n    except AssertionError:\n        return 2\n    return 4"
        );
        CHECK(t.raises() == PyExc_Exception);
    }
    SECTION("test76") {
        auto t = CompilerTest(
                "def f():\n    global x\n    x = 2\n    return x"
        );
        CHECK(t.returns() == "2");
    }
    SECTION("test77") {
        auto t = CompilerTest(
                "def f():\n    for i in range(3):\n        if i == 0: continue\n        break\n    return i"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test78") {
        auto t = CompilerTest(
                "def f():\n    for i in range(3):\n        if i == 1: break\n    return i"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test79") {
        auto t = CompilerTest(
                "def f():\n    return [1,2,3][1:]"
        );
        CHECK(t.returns() == "[2, 3]");
    }
    SECTION("test80") {
        auto t = CompilerTest(
                "def f():\n    return [1,2,3][:1]"
        );
        CHECK(t.returns() == "[1]");
    }
    SECTION("test81") {
        auto t = CompilerTest(
                "def f():\n    return [1,2,3][1:2]"
        );
        CHECK(t.returns() == "[2]");
    }
    SECTION("test82") {
        auto t = CompilerTest(
                "def f():\n    return [1,2,3][0::2]"
        );
        CHECK(t.returns() == "[1, 3]");
    }
    SECTION("test83") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = range(3)\n    return a"
        );
        CHECK(t.returns() == "0");
    }
    SECTION("test84") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = range(3)\n    return b"
        );
        CHECK(t.returns() == "[1]");
    }
    SECTION("test85") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = range(3)\n    return c"
        );
        CHECK(t.returns() == "2");
    }
    SECTION("test86") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 2, 3\n    return a"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test87") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 2, 3\n    return b"
        );
        CHECK(t.returns() == "[2]");
    }
    SECTION("test88") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 2, 3\n    return c"
        );
        CHECK(t.returns() == "3");
    }
    SECTION("test89") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 3\n    return c"
        );
        CHECK(t.returns() == "3");
    }
    SECTION("test90") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = 1, 3\n    return b"
        );
        CHECK(t.returns() == "[]");
    }
    SECTION("test91") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 2, 3]\n    return a"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test92") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 2, 3]\n    return b"
        );
        CHECK(t.returns() == "[2]");
    }
    SECTION("test92") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 2, 3]\n    return c"
        );
        CHECK(t.returns() == "3");
    }
    SECTION("test93") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 3]\n    return c"
        );
        CHECK(t.returns() == "3");
    }
    SECTION("test94") {
        auto t = CompilerTest(
                "def f():\n    a, *b, c = [1, 3]\n    return b"
        );
        CHECK(t.returns() == "[]");
    }
    SECTION("test95") {
        auto t = CompilerTest(
                "def f():\n    a, b = range(2)\n    return a"
        );
        CHECK(t.returns() == "0");
    }
    SECTION("test96") {
        auto t = CompilerTest(
                "def f():\n    a, b = 1, 2\n    return a"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test97") {
        auto t = CompilerTest(
                "def f():\n    class C:\n        pass\n    return C"
        );
        CHECK(t.returns() == "<class 'C'>");
    }
    SECTION("test98") {
        auto t = CompilerTest(
                "def f():\n    a = 0\n    for x in[1]:\n        a = a + 1\n    return a"
        );
        CHECK(t.returns() == "1");
    }
    SECTION("test99") {
        auto t = CompilerTest(
                "def f(): return [x for x in range(2)]"
        );
        CHECK(t.returns() == "[0, 1]");
    }
    SECTION("test100") {
        auto t = CompilerTest(
                "def f():\n    def g(): pass\n    return g.__name__"
        );
        CHECK(t.returns() == "'g'");
    };
}
