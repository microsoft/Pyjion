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
#include <pyjit.h>

PyTupleObject * tupleOfOne ;
PyObject* list ;
PyObject* list2 ;

void VerifyOldJitTest(TestCase curCase) {
    auto sysModule = PyImport_ImportModule("sys");
    tupleOfOne = (PyTupleObject *)PyTuple_New(1);
    list = PyList_New(1);
    list2 = PyList_New(1);
    tupleOfOne->ob_item[0] = PyLong_FromLong(42);
    PyList_SetItem(list, 0, PyLong_FromLong(42));
    PyList_SetItem(list, 0, PyLong_FromLong(42));

    puts(curCase.m_code);
    auto codeObj = CompileCode(curCase.m_code);
    auto addr = PyJit_EnsureExtra((PyObject *) codeObj);
    REQUIRE (addr != nullptr) ;
    // For the purpose of exercising the JIT machinery while testing, the
    // code should be JITed everytime.
    addr->j_specialization_threshold = 0;
    if (!jit_compile(codeObj) || addr->j_evalfunc == nullptr) {
        CHECK(FALSE);
    }

    for (auto curInput = 0; curInput < curCase.m_inputs.size(); curInput++) {
        auto input = curCase.m_inputs[curInput];

        auto globals = PyDict_New();
        auto builtins = PyEval_GetBuiltins();
        PyDict_SetItemString(globals, "__builtins__", builtins);
        PyDict_SetItemString(globals, "sys", sysModule);

        PyRun_String(
                "finalized = False\nclass RefCountCheck:\n    def __del__(self):\n        print('finalizing')\n        global finalized\n        finalized = True\n    def __add__(self, other):\n        return self",
                Py_file_input, globals, globals);
        if (PyErr_Occurred()) {
            PyErr_Print();
            return;
        }

        auto frame = PyFrame_New(PyThreadState_Get(), codeObj, globals, PyDict_New());
        for (size_t arg = 0; arg < input.m_args.size(); arg++) {
            frame->f_localsplus[arg] = input.m_args[arg];
        }

        auto res = addr->j_evalfunc(addr, frame);

        auto repr = PyUnicode_AsUTF8(PyObject_Repr(res));
        CHECK(input.m_expected == repr);
        REQUIRE (res != nullptr) ;
        auto tstate = PyThreadState_GET();
        CHECK(tstate->curexc_value == nullptr);
        CHECK(tstate->curexc_traceback == nullptr);
        CHECK((tstate->curexc_type == nullptr || tstate->curexc_type == Py_None));
//Py_DECREF(frame);
        Py_XDECREF(res);
        Py_DECREF(codeObj);
    }
};

TEST_CASE("Legacy JIT Tests", "[float][binary op][inference]") {
    SECTION("test everything") {
        VerifyOldJitTest(
                // EXTENDED_ARG FOR_ITER:
                TestCase(
                        "def f():\n"
                        "        x = 1\n"
                        "        for w in 1, 2, 3, 4:\n"
                        "            x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2;\n"
                        "            x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2; x += 2;\n"
                        "        return x\n",
                        TestInput("369")
                ));
        VerifyOldJitTest(TestCase(
                        "def f(): 1.0 / 0",
                        TestInput("<NULL>")
                ));
        VerifyOldJitTest(TestCase(
                        "def f(): print(f'x {42}')",
                        TestInput("None")
                ));
        VerifyOldJitTest(TestCase(
                        "def f(): return f'abc {42}'",
                        TestInput("'abc 42'")
                ));

        VerifyOldJitTest(TestCase(
                        "def f(): return f'abc {42:3}'",
                        TestInput("'abc  42'")
                ));
        VerifyOldJitTest(TestCase(
                        "def f(): return f'abc {\"abc\"!a}'",
                        TestInput("\"abc 'abc'\"")
                ));
        
                VerifyOldJitTest(TestCase(
                        "def f(): return f'abc {\"abc\"!a:6}'",
                        TestInput("\"abc 'abc' \"")
                ));
                VerifyOldJitTest(TestCase(
                        "def f(): return f'abc {\"abc\"!r:6}'",
                        TestInput("\"abc 'abc' \"")
                ));
                VerifyOldJitTest(TestCase(
                        "def f(): return f'abc {\"abc\"!s}'",
                        TestInput("'abc abc'")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    for b in range(1):\n        x = b & 1 and -1.0 or 1.0\n    return x",
                        TestInput("1.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = y\n    y = 1",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n         raise TypeError('hi')\n    except Exception as e:\n         pass\n    finally:\n         pass",
                        TestInput("None")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        try:\n             raise Exception('hi')\n        finally:\n             pass\n    finally:\n        pass",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        try:\n             try:\n                  raise TypeError('err')\n             except BaseException:\n                  raise\n        finally:\n             pass\n    finally:\n        return 42\n",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def f(self) -> 42 : pass\n    return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, q, r):\n    if not (a == q and r == 0):\n        return 42\n    return 23",
                        TestInput("42",
                                  vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(4), PyLong_FromLong(7)}))
                ));
                // Break from nested try/finally needs to use BranchLeave to clear the stack
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(5):\n        try:\n            raise Exception()\n        finally:\n            try:\n                break\n            finally:\n                pass\n    return 42",
                        TestInput("42")
                ));
                // Break from a double nested try/finally needs to unwind all exceptions
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(5):\n        try:\n            raise Exception()\n        finally:\n            try:\n                raise Exception()\n            finally:\n                try:\n                     break\n                finally:\n                    pass\n    return 42",
                        TestInput("42")
                ));
                // return from nested try/finally should use BranchLeave to clear stack when branching to return label
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise Exception()\n    finally:\n        try:\n            return 42\n        finally:\n            pass",
                        TestInput("42")
                ));
                // Return from nested try/finally should unwind nested exception handlers
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise Exception()\n    finally:\n        try:\n            raise Exception()\n        finally:\n            try:\n                return 42\n            finally:\n                pass\n    return 23",
                        TestInput("42")
                ));
                // Break from a nested exception handler needs to unwind all exception handlers
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(5):\n        try:\n             raise Exception()\n        except:\n             try:\n                  raise TypeError()\n             finally:\n                  break\n    return 42",
                        TestInput("42")
                ));
                // Return from a nested exception handler needs to unwind all exception handlers
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(5):\n        try:\n             raise Exception()\n        except:\n             try:\n                  raise TypeError()\n             finally:\n                  return 23\n    return 42",
                        TestInput("23")
                ));
                // We need to do BranchLeave to clear the stack when doing a break inside of a finally
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(5):\n        try:\n            raise Exception()\n        finally:\n            break\n    return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n         raise Exception()\n    finally:\n        raise Exception()",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = b'abc'*3\n    return x",
                        TestInput("b'abcabcabc'")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    unbound += 1",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    5 % 0",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    5.0 % 0.0",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    5.0 // 0.0",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    5.0 / 0.0",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 'abc'*3\n    return x",
                        TestInput("'abcabcabc'")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    if 0.0 < 1.0 <= 1.0 == 1.0 >= 1.0 > 0.0 != 1.0:  return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        try:\n            pass\n        finally:\n            raise OSError\n    except OSError as e:\n        return 1\n    return 0\n",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise\n    except RuntimeError:\n        return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        while True:\n            try:\n                raise Exception()\n            except Exception:\n                break\n    finally:\n        pass\n    return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        pass\n    finally:\n        raise",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        pass\n    finally:\n        raise OSError",
                        TestInput("<NULL>")
                ));
                // partial should be boxed because it's consumed by print after being assigned in the break loop
                VerifyOldJitTest(TestCase(
                        "def f():\n    partial = 0\n    while 1:\n        partial = 1\n        break\n    if not partial:\n        print(partial)\n        return True\n    return False\n",
                        TestInput("False")
                ));

                // UNARY_NOT/POP_JUMP_IF_FALSE with optimized value on stack should be compiled correctly w/o crashing
                VerifyOldJitTest(TestCase(
                        "def f():\n    abc = 1.0\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23",
                        TestInput("23")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    abc = 1\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23",
                        TestInput("23")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    abc = 0.0\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    abc = 0\n    i = 0\n    n = 0\n    if i == n and not abc:\n        return 42\n    return 23",
                        TestInput("42")
                ));
                // Too many items to unpack from list/tuple shouldn't crash
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = [1,2,3]\n    a, b = x",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = (1,2,3)\n    a, b = x",
                        TestInput("<NULL>")
                ));
                // failure to unpack shouldn't crash, should raise Python exception
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = [1]\n    a, b, *c = x",
                        TestInput("<NULL>")
                ));
                // unpacking non-iterable shouldn't crash
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, b, c = len",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def x():\n     try:\n         b\n     except:\n         c\n\ndef f():\n    try:\n        x()\n    except:\n        pass\n    return sys.exc_info()[0]\n\n",
                        TestInput("None")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    cs = [('CATEGORY', 'CATEGORY_SPACE')]\n    for op, av in cs:\n        while True:\n            break\n        print(op, av)",
                        TestInput("None")
                ));

                // +=, -= checks are to avoid constant folding
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 0\n    x += 1\n    x -= 1\n    return x or 1",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 0\n    x += 1\n    x -= 1\n    return x and 1",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    x += 1\n    x -= 1\n    return x or 2",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    x += 1\n    x -= 1\n    return x and 2",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    return x or 1",
                        TestInput("4611686018427387903")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    return x and 1",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    return x or 1",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    return x and 1",
                        TestInput("0")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    return -x",
                        TestInput("-4611686018427387903")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    return -x",
                        TestInput("-4611686018427387904")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = -4611686018427387904\n    x += 1\n    x -= 1\n    return -x",
                        TestInput("4611686018427387904")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    y = not x\n    return y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    if x:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    if x:\n        return True\n    return False",
                        TestInput("False")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    if not x:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    if not x:\n        return True\n    return False",
                        TestInput("True")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    x += 1\n    x -= 1\n    x -= 4611686018427387903\n    y = not x\n    return y",
                        TestInput("True")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x == y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x <= y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x >= y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x != y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x < y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    if x > y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    if x < y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    if x > y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    if x < y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    if x > y:\n        return True\n    return False",
                        TestInput("False")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x == y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x == y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x == y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x == y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x == y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 1\n    return x == y",
                        TestInput("True")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x != y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x != y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x != y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x != y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x != y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 1\n    return x != y",
                        TestInput("False")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x >= y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x >= y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x >= y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 1\n    return x >= y",
                        TestInput("True")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x <= y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    x -= 1\n    return x <= y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    y -= 1\n    return x <= y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 1\n    return x <= y",
                        TestInput("True")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x > y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775808\n    y = 9223372036854775807\n    return x > y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775808\n    return x > y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x > y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x > y",
                        TestInput("False")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x < y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775808\n    y = 9223372036854775807\n    return x < y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775808\n    return x < y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    x += 1\n    return x < y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 4611686018427387903\n    y += 1\n    return x < y",
                        TestInput("True")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 1\n    return x == y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 9223372036854775807\n    return x % y",
                        TestInput("1")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 2\n    return x / y",
                        TestInput("0.5")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 4611686018427387903\n    return x / y",
                        TestInput("2.168404344971009e-19")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 9223372036854775807\n    return x / y",
                        TestInput("1.0842021724855044e-19")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x / y",
                        TestInput("4.611686018427388e+18")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x / y",
                        TestInput("9.223372036854776e+18")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x / y",
                        TestInput("1.0")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 2\n    return x >> y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 4611686018427387903\n    return x >> y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 9223372036854775807\n    return x >> y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x >> y",
                        TestInput("2305843009213693951")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x >> y",
                        TestInput("4611686018427387903")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x >> y",
                        TestInput("0")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 2\n    return x << y",
                        TestInput("4")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 32\n    return x << y",
                        TestInput("4294967296")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 62\n    return x << y",
                        TestInput("4611686018427387904")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 63\n    return x << y",
                        TestInput("9223372036854775808")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 64\n    return x << y",
                        TestInput("18446744073709551616")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x << y",
                        TestInput("9223372036854775806")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x << y",
                        TestInput("18446744073709551614")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x << y",
                        TestInput("<NULL>")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 2\n    return x ** y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 32\n    return x ** y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 4611686018427387903\n    return x ** y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 9223372036854775807\n    return x ** y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x ** y",
                        TestInput("4611686018427387903")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x ** y",
                        TestInput("9223372036854775807")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 2\n    return x // y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 4611686018427387903\n    return x // y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 9223372036854775807\n    return x // y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x // y",
                        TestInput("4611686018427387903")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 4611686018427387903\n    return x // y",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = -4611686018427387903\n    return x // y",
                        TestInput("-3")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x // y",
                        TestInput("9223372036854775807")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = -1\n    return x // y",
                        TestInput("-9223372036854775807")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x // y",
                        TestInput("1")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 2\n    return x % y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 4611686018427387903\n    return x % y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x % y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 4611686018427387903\n    return x % y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = -4611686018427387903\n    return x % y",
                        TestInput("-4611686018427387902")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x % y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = -1\n    return x % y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x % y",
                        TestInput("0")
                ));


                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 2\n    return x | y",
                        TestInput("3")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 4611686018427387903\n    return x | y",
                        TestInput("4611686018427387903")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 9223372036854775807\n    return x | y",
                        TestInput("9223372036854775807")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x | y",
                        TestInput("4611686018427387903")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x | y",
                        TestInput("9223372036854775807")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x | y",
                        TestInput("9223372036854775807")
                ));


                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 2\n    return x & y",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 3\n    return x & y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 4611686018427387903\n    return x & y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 9223372036854775807\n    return x & y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x & y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x & y",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x & y",
                        TestInput("9223372036854775807")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 2\n    return x ^ y",
                        TestInput("3")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 3\n    return x ^ y",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 4611686018427387903\n    return x ^ y",
                        TestInput("4611686018427387902")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 9223372036854775807\n    return x ^ y",
                        TestInput("9223372036854775806")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x ^ y",
                        TestInput("4611686018427387902")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x ^ y",
                        TestInput("9223372036854775806")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x ^ y",
                        TestInput("0")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = -9223372036854775808\n    y = 1\n    return x - y",
                        TestInput("-9223372036854775809")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = -1\n    y = 4611686018427387904\n    return x - y",
                        TestInput("-4611686018427387905")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = -1\n    y = 9223372036854775808\n    return x - y",
                        TestInput("-9223372036854775809")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x =  -4611686018427387904\n    y = 1\n    return x - y",
                        TestInput("-4611686018427387905")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 4611686018427387903\n    return x + y",
                        TestInput("4611686018427387904")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 9223372036854775807\n    return x + y",
                        TestInput("9223372036854775808")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 1\n    return x + y",
                        TestInput("4611686018427387904")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 1\n    return x + y",
                        TestInput("9223372036854775808")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x + y",
                        TestInput("18446744073709551614")
                ));

                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2\n    y = 4611686018427387903\n    return x * y",
                        TestInput("9223372036854775806")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2\n    y = 9223372036854775807\n    return x * y",
                        TestInput("18446744073709551614")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4611686018427387903\n    y = 2\n    return x * y",
                        TestInput("9223372036854775806")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 2\n    return x * y",
                        TestInput("18446744073709551614")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 9223372036854775807\n    y = 9223372036854775807\n    return x * y",
                        TestInput("85070591730234615847396907784232501249")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        except EnvironmentError:\n            pass\n    return 1",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        finally:\n            pass\n    return 1",
                        TestInput("1")
                ));
                // Simple optimized code test cases...
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = +x\n    return y",
                        TestInput("1.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    if not x:\n        return 1\n    return 2",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 0.0\n    if not x:\n        return 1\n    return 2",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = -x\n    return y",
                        TestInput("-1.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = not x\n    return y",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 0.0\n    y = not x\n    return y",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    return x",
                        TestInput("1.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    z = x + y\n    return z",
                        TestInput("3.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    z = x - y\n    return z",
                        TestInput("-1.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    z = x / y\n    return z",
                        TestInput("0.5")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    z = x // y\n    return z",
                        TestInput("0.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    z = x % y\n    return z",
                        TestInput("1.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2.0\n    y = 3.0\n    z = x * y\n    return z",
                        TestInput("6.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2.0\n    y = 3.0\n    z = x ** y\n    return z",
                        TestInput("8.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2.0\n    y = 3.0\n    if x == y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 3.0\n    y = 3.0\n    if x == y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 'a'\n    y = 'b'\n    if x == y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 'a'\n    y = 'a'\n    if x == y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    class Foo(str): pass\n    x = Foo(1)\n    y = Foo(2)\n    if x == y:        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    class Foo(str): pass\n    x = Foo(1)\n    y = Foo(1)\n    if x == y:        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2.0\n    y = 3.0\n    if x != y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 3.0\n    y = 3.0\n    if x != y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2.0\n    y = 3.0\n    if x >= y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 3.0\n    y = 3.0\n    if x >= y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2.0\n    y = 3.0\n    if x > y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 4.0\n    y = 3.0\n    if x > y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 3.0\n    y = 2.0\n    if x <= y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 3.0\n    y = 3.0\n    if x <= y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 3.0\n    y = 2.0\n    if x < y:\n        return True\n    return False",
                        TestInput("False")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 3.0\n    y = 4.0\n    if x < y:\n        return True\n    return False",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    x += y\n    return x",
                        TestInput("3.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    x -= y\n    return x",
                        TestInput("-1.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    x /= y\n    return x",
                        TestInput("0.5")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    x //= y\n    return x",
                        TestInput("0.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    x %= y\n    return x",
                        TestInput("1.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2.0\n    y = 3.0\n    x *= y\n    return x",
                        TestInput("6.0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2.0\n    y = 3.0\n    x **= y\n    return x",
                        TestInput("8.0")
                ));
                // fully optimized complex code
                VerifyOldJitTest(TestCase(
                        "def f():\n    pi = 0.\n    k = 0.\n    while k < 256.:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi",
                        TestInput("3.141592653589793")
                ));
                // division error handling code gen with value on the stack
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1.0\n    y = 2.0\n    z = 3.0\n    return x + y / z",
                        TestInput("1.6666666666666665")
                ));
                // division by zero error case
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 0\n    try:\n        return x / y\n    except:\n        return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 1\n    y = 0\n    try:\n        return x // y\n    except:\n        return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f(x):\n    if not x:\n        return True\n    return False",
                        vector<TestInput>({
                                                  TestInput("False", vector<PyObject *>({PyLong_FromLong(1)})),
                                                  TestInput("True", vector<PyObject *>({PyLong_FromLong(0)}))
                                          })));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    if a in b:\n        return True\n    return False",
                        vector<TestInput>({
                                                  TestInput("True", vector<PyObject *>(
                                                          {PyLong_FromLong(42), Incremented((PyObject *) tupleOfOne)})),
                                                  TestInput("False", vector<PyObject *>(
                                                          {PyLong_FromLong(1), Incremented((PyObject *) tupleOfOne)}))
                                          })));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    if a not in b:\n        return True\n    return False",
                        vector<TestInput>({
                                                  TestInput("False", vector<PyObject *>(
                                                          {PyLong_FromLong(42), Incremented((PyObject *) tupleOfOne)})),
                                                  TestInput("True", vector<PyObject *>(
                                                          {PyLong_FromLong(1), Incremented((PyObject *) tupleOfOne)}))
                                          })
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    if a is b:\n        return True",
                        TestInput("True", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    if a is not b:\n        return True",
                        TestInput("True", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    assert a is b\n    return True",
                        TestInput("True", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    assert a is b\n    return True",
                        TestInput("<NULL>", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a = RefCountCheck()\n    del a\n    return finalized",
                        TestInput("True")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in {2:3}:\n        pass\n    return i",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            pass",
                        TestInput("None")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(5):\n        try:\n            pass\n        finally:\n            return i",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            return i",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise Exception(2)\n    except Exception as e:\n        return e.args[0]",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(b:1, *, a = 2):\n     return a\n    return g.__annotations__['b']",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(b:1, *, a = 2):\n     return a\n    return g(3)",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(*, a = 2):\n     return a\n    return g()",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(a:1, b:2): pass\n    return g.__annotations__['a']",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    from sys import winver, version_info\n    return winver[0]",
                        TestInput("'3'")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    from sys import winver\n    return winver[0]",
                        TestInput("'3'")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(*a): return a\n    return g(1, 2, 3, **{})",
                        TestInput("(1, 2, 3)")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(**a): return a\n    return g(y = 3, **{})",
                        TestInput("{'y': 3}")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(**a): return a\n    return g(**{'x':2})",
                        TestInput("{'x': 2}")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(**a): return a\n    return g(x = 2, *())",
                        TestInput("{'x': 2}")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(*a): return a\n    return g(*(1, 2, 3))",
                        TestInput("(1, 2, 3)")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(*a): return a\n    return g(1, *(2, 3))",
                        TestInput("(1, 2, 3)")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(): pass\n    g.abc = {fn.lower() for fn in ['A']}\n    return g.abc",
                        TestInput("{'a'}")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    for abc in [1,2,3]:\n        try:\n            break\n        except ImportError:\n            continue\n    return abc",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a = 1\n    b = 2\n    def x():\n        return a, b\n    return x()",
                        TestInput("(1, 2)", vector<PyObject *>({NULL, PyCell_New(NULL), PyCell_New(NULL)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f() :\n    def g(x) :\n        return x\n    a = 1\n    def x() :\n        return g(a)\n    return x()",
                        TestInput("1", vector<PyObject *>({NULL, PyCell_New(NULL), PyCell_New(NULL)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise Exception()\n    finally:\n        return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n	try:\n		pass\n	except ImportError:\n		pass\n	except Exception as e:\n		pass",
                        TestInput("None")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise Exception('hi')\n    except:\n        return 42",
                        TestInput("42")
                ));
                //VerifyOldJitTest(TestCase(
                //	"def f(sys_module, _imp_module):\n    '''Setup importlib by importing needed built - in modules and injecting them\n    into the global namespace.\n\n    As sys is needed for sys.modules access and _imp is needed to load built - in\n    modules, those two modules must be explicitly passed in.\n\n    '''\n    global _imp, sys\n    _imp = _imp_module\n    sys = sys_module\n\n    # Set up the spec for existing builtin/frozen modules.\n    module_type = type(sys)\n    for name, module in sys.modules.items():\n        if isinstance(module, module_type):\n            if name in sys.builtin_module_names:\n                loader = BuiltinImporter\n            elif _imp.is_frozen(name):\n                loader = FrozenImporter\n            else:\n                continue\n            spec = _spec_from_module(module, loader)\n            _init_module_attrs(spec, module)\n\n    # Directly load built-in modules needed during bootstrap.\n    self_module = sys.modules[__name__]\n    for builtin_name in ('_warnings',):\n        if builtin_name not in sys.modules:\n            builtin_module = _builtin_from_name(builtin_name)\n        else:\n            builtin_module = sys.modules[builtin_name]\n        setattr(self_module, builtin_name, builtin_module)\n\n    # Directly load the _thread module (needed during bootstrap).\n    try:\n        thread_module = _builtin_from_name('_thread')\n    except ImportError:\n        # Python was built without threads\n        thread_module = None\n    setattr(self_module, '_thread', thread_module)\n\n    # Directly load the _weakref module (needed during bootstrap).\n    weakref_module = _builtin_from_name('_weakref')\n    setattr(self_module, '_weakref', weakref_module)\n",
                //	TestInput("42")
                //));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = {}\n    try:\n        return x[42]\n    except KeyError:\n        return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        pass\n    finally:\n        pass\n    return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = {}\n    x.update(y=2)\n    return x",
                        TestInput("{'y': 2}")
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a):\n    return 1 if a else 2",
                        vector<TestInput>({
                                                  TestInput("1", vector<PyObject *>({PyLong_FromLong(42)})),
                                                  TestInput("2", vector<PyObject *>({PyLong_FromLong(0)})),
                                                  TestInput("1", vector<PyObject *>({Incremented(Py_True)})),
                                                  TestInput("2", vector<PyObject *>({Incremented(Py_False)}))
                                          })
                ));
                VerifyOldJitTest(TestCase(
                        "def f(v):\n    try:\n        x = v.real\n    except AttributeError:\n        return 42\n    else:\n        return x\n",
                        vector<TestInput>({
                                                  TestInput("1", vector<PyObject *>({PyLong_FromLong(1)})),
                                                  TestInput("42", vector<PyObject *>({PyUnicode_FromString("hi")}))
                                          })));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a=1\n    def x():\n        return a\n    return a",
                        TestInput("1", vector<PyObject *>({NULL, PyCell_New(NULL)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2\n    def g():    return x\n    \n    str(g)\n    return g()",
                        TestInput("2", vector<PyObject *>({NULL, PyCell_New(NULL)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a=2\n    def g(): return a\n    return g()",
                        TestInput("2", vector<PyObject *>({NULL, PyCell_New(NULL)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(a=2): return a\n    return g()",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(5):\n        try:\n            continue\n        finally:\n            return i",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise Exception()\n    finally:\n        pass",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        pass\n    finally:\n        return 42",
                        TestInput("42")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise Exception()\n    except:\n        return 2",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise Exception()\n    except Exception:\n        return 2",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    try:\n        raise Exception()\n    except AssertionError:\n        return 2\n    return 4",
                        TestInput("<NULL>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    global x\n    x = 2\n    return x",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(    // hits JUMP_FORWARD
                        "def f(a):\n    if a:\n        x = 1\n    else:\n        x = 2\n    return x",
                        vector<TestInput>({
                                                  TestInput("1", vector<PyObject *>({PyLong_FromLong(42)})),
                                                  TestInput("2", vector<PyObject *>({PyLong_FromLong(0)})),
                                                  TestInput("1", vector<PyObject *>({Incremented(Py_True)})),
                                                  TestInput("2", vector<PyObject *>({Incremented(Py_False)}))
                                          })));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    return a and b",
                        vector<TestInput>({
                                                  TestInput("2", vector<PyObject *>(
                                                          {PyLong_FromLong(1), PyLong_FromLong(2)})),
                                                  TestInput("0", vector<PyObject *>(
                                                          {PyLong_FromLong(0), PyLong_FromLong(1)})),
                                                  TestInput("False", vector<PyObject *>({Py_True, Py_False})),
                                                  TestInput("False", vector<PyObject *>({Py_False, Py_True})),
                                                  TestInput("True", vector<PyObject *>({Py_True, Py_True})),
                                                  TestInput("False", vector<PyObject *>({Py_False, Py_False}))
                                          })
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    return a or b",
                        vector<TestInput>({
                                                  TestInput("1", vector<PyObject *>(
                                                          {PyLong_FromLong(1), PyLong_FromLong(2)})),
                                                  TestInput("1", vector<PyObject *>(
                                                          {PyLong_FromLong(0), PyLong_FromLong(1)})),
                                                  TestInput("True", vector<PyObject *>({Py_True, Py_False})),
                                                  TestInput("True", vector<PyObject *>({Py_False, Py_True})),
                                                  TestInput("True", vector<PyObject *>({Py_True, Py_True})),
                                                  TestInput("False", vector<PyObject *>({Py_False, Py_False}))
                                          })));
                VerifyOldJitTest(TestCase(         // hits ROT_TWO, POP_TWO, DUP_TOP, ROT_THREE
                        "def f(a, b, c):\n    return a < b < c",
                        vector<TestInput>({
                                                  TestInput("True", vector<PyObject *>(
                                                          {PyLong_FromLong(2), PyLong_FromLong(3),
                                                           PyLong_FromLong(4)})),
                                                  TestInput("False", vector<PyObject *>(
                                                          {PyLong_FromLong(4), PyLong_FromLong(3), PyLong_FromLong(2)}))
                                          })
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a **= b\n    return a",
                        TestInput("8", vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(3)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a *= b\n    return a",
                        TestInput("6", vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(3)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a /= b\n    return a",
                        TestInput("0.5", vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(4)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a //= b\n    return a",
                        vector<TestInput>({
                                                  TestInput("0", vector<PyObject *>(
                                                          {PyLong_FromLong(2), PyLong_FromLong(4)})),
                                                  TestInput("2", vector<PyObject *>(
                                                          {PyLong_FromLong(4), PyLong_FromLong(2)}))
                                          })));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a %= b\n    return a",
                        TestInput("1", vector<PyObject *>({PyLong_FromLong(3), PyLong_FromLong(2)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a += b\n    return a",
                        vector<TestInput>({
                                                  TestInput("9", vector<PyObject *>(
                                                          {PyLong_FromLong(6), PyLong_FromLong(3)})),
                                                  TestInput("'hibye'", vector<PyObject *>(
                                                          {PyUnicode_FromString("hi"), PyUnicode_FromString("bye")}))
                                          })));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a -= b\n    return a",
                        TestInput("1", vector<PyObject *>({PyLong_FromLong(3), PyLong_FromLong(2)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a <<= b\n    return a",
                        TestInput("4", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a >>= b\n    return a",
                        TestInput("1", vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a &= b\n    return a",
                        TestInput("2", vector<PyObject *>({PyLong_FromLong(6), PyLong_FromLong(3)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a ^= b\n    return a",
                        TestInput("5", vector<PyObject *>({PyLong_FromLong(6), PyLong_FromLong(3)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    a |= b\n    return a",
                        TestInput("7", vector<PyObject *>({PyLong_FromLong(6), PyLong_FromLong(3)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(x):\n    return -x",
                        TestInput("-1", vector<PyObject *>({PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(x):\n    return +x",
                        TestInput("1", vector<PyObject *>({PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(x):\n    return ~x",
                        TestInput("-2", vector<PyObject *>({PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a << b",
                        TestInput("4", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a >> b",
                        TestInput("2", vector<PyObject *>({PyLong_FromLong(4), PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a & b",
                        TestInput("2", vector<PyObject *>({PyLong_FromLong(6), PyLong_FromLong(3)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a | b",
                        TestInput("3", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(2)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a ^ b",
                        TestInput("6", vector<PyObject *>({PyLong_FromLong(3), PyLong_FromLong(5)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(x):\n    return not x",
                        vector<TestInput>({
                                                  TestInput("False", vector<PyObject *>({PyLong_FromLong(1)})),
                                                  TestInput("True", vector<PyObject *>({PyLong_FromLong(0)}))
                                          })));
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(3):\n        if i == 0: continue\n        break\n    return i",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    for i in range(3):\n        if i == 1: break\n    return i",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    return [1,2,3][1:]",
                        TestInput("[2, 3]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    return [1,2,3][:1]",
                        TestInput("[1]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    return [1,2,3][1:2]",
                        TestInput("[2]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    return [1,2,3][0::2]",
                        TestInput("[1, 3]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    x = 2\n    def g():    return x\n    return g()",
                        TestInput("2", vector<PyObject *>({NULL, PyCell_New(NULL)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = range(3)\n    return a",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = range(3)\n    return b",
                        TestInput("[1]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = range(3)\n    return c",
                        TestInput("2")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = 1, 2, 3\n    return a",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = 1, 2, 3\n    return b",
                        TestInput("[2]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = 1, 2, 3\n    return c",
                        TestInput("3")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = 1, 3\n    return c",
                        TestInput("3")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = 1, 3\n    return b",
                        TestInput("[]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = [1, 2, 3]\n    return a",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = [1, 2, 3]\n    return b",
                        TestInput("[2]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = [1, 2, 3]\n    return c",
                        TestInput("3")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = [1, 3]\n    return c",
                        TestInput("3")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, *b, c = [1, 3]\n    return b",
                        TestInput("[]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, b = range(2)\n    return a",
                        TestInput("0")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a, b = 1, 2\n    return a",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    class C:\n        pass\n    return C",
                        TestInput("<class 'C'>")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    a = 0\n    for x in[1]:\n        a = a + 1\n    return a",
                        TestInput("1")
                ));
                VerifyOldJitTest(TestCase(
                        "def f(): return [x for x in range(2)]",
                        TestInput("[0, 1]")
                ));
                VerifyOldJitTest(TestCase(
                        "def f():\n    def g(): pass\n    return g.__name__",
                        TestInput("'g'")
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b, c):\n    a[b] = c\n    return a[b]",
                        TestInput("True", vector<PyObject *>({list, PyLong_FromLong(0), Incremented(Py_True)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    del a[b]\n    return len(a)",
                        TestInput("0", vector<PyObject *>({list2, PyLong_FromLong(0)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    return a in b",
                        vector<TestInput>({
                                                  TestInput("True", vector<PyObject *>(
                                                          {PyLong_FromLong(42), Incremented((PyObject *) tupleOfOne)})),
                                                  TestInput("False", vector<PyObject *>(
                                                          {PyLong_FromLong(1), Incremented((PyObject *) tupleOfOne)}))
                                          })));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    return a not in b",
                        vector<TestInput>({
                                                  TestInput("False", vector<PyObject *>(
                                                          {PyLong_FromLong(42), Incremented((PyObject *) tupleOfOne)})),
                                                  TestInput("True", vector<PyObject *>(
                                                          {PyLong_FromLong(1), Incremented((PyObject *) tupleOfOne)}))
                                          })
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    return a == b",
                        TestInput("True", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    return a != b",
                        TestInput("False", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    return a is b",
                        TestInput("True", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    return a is not b",
                        TestInput("False", vector<PyObject *>({PyLong_FromLong(1), PyLong_FromLong(1)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a):\n    while a:\n        a = a - 1\n    return a",
                        TestInput("0", vector<PyObject *>({PyLong_FromLong(42)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a):\n    if a:\n        return 1\n    else:\n        return 2",
                        vector<TestInput>({
                                                  TestInput("1", vector<PyObject *>({PyLong_FromLong(42)})),
                                                  TestInput("2", vector<PyObject *>({PyLong_FromLong(0)})),
                                                  TestInput("1", vector<PyObject *>({Incremented(Py_True)})),
                                                  TestInput("2", vector<PyObject *>({Incremented(Py_False)}))
                                          })));
                VerifyOldJitTest(TestCase(
                        "def f(a): return a.real",
                        TestInput("42", vector<PyObject *>({PyLong_FromLong(42)}))
                ));
                VerifyOldJitTest(TestCase("def f(): return {1,2}", "{1, 2}"));
                VerifyOldJitTest(TestCase("def f(): return {}", "{}"));
                VerifyOldJitTest(TestCase("def f(): return {1:2}", "{1: 2}"));
                VerifyOldJitTest(TestCase("def f(): return []", "[]"));
                VerifyOldJitTest(TestCase("def f(): return [1]", "[1]"));
                VerifyOldJitTest(TestCase("def f(): return ()", "()"));
                VerifyOldJitTest(TestCase("def f(): return (1, )", "(1,)"));
                VerifyOldJitTest(TestCase("def f(): return (1, 2)", "(1, 2)"));
                VerifyOldJitTest(TestCase("def f(): x = 1; return x", "1"));
                VerifyOldJitTest(TestCase("def f(): return int()", "0"));
                VerifyOldJitTest(TestCase("def f(): return range(5)", "range(0, 5)"));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a[b]",
                        TestInput("42", vector<PyObject *>({(PyObject *) tupleOfOne, PyLong_FromLong(0)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a / b",
                        TestInput("3.0", vector<PyObject *>({PyLong_FromLong(75), PyLong_FromLong(25)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a // b",
                        TestInput("2", vector<PyObject *>({PyLong_FromLong(75), PyLong_FromLong(30)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a % b",
                        vector<TestInput>({
                                                  TestInput("2", vector<PyObject *>(
                                                          {PyLong_FromLong(32), PyLong_FromLong(3)})),
                                                  TestInput("'abcdef'", vector<PyObject *>(
                                                          {PyUnicode_FromString("abc%s"), PyUnicode_FromString("def")}))
                                          })
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a ** b",
                        TestInput("32", vector<PyObject *>({PyLong_FromLong(2), PyLong_FromLong(5)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a - b",
                        TestInput("25", vector<PyObject *>({PyLong_FromLong(75), PyLong_FromLong(50)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a * b",
                        TestInput("4200", vector<PyObject *>({PyLong_FromLong(42), PyLong_FromLong(100)}))
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b): return a + b",
                        vector<TestInput>({
                                                  TestInput("142", vector<PyObject *>(
                                                          {PyLong_FromLong(42), PyLong_FromLong(100)})),
                                                  TestInput("'abcdef'", vector<PyObject *>(
                                                          {PyUnicode_FromString("abc"), PyUnicode_FromString("def")}))
                                          })
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b):\n    return (b, a)",
                        vector<TestInput>({
                                                  TestInput("(2, 1)", vector<PyObject *>(
                                                          {PyLong_FromLong(1), PyLong_FromLong(2)})),
                                                  TestInput("(2.2, 1.1)", vector<PyObject *>(
                                                          {PyFloat_FromDouble(1.1), PyFloat_FromDouble(2.2)})),
                                                  TestInput("('def', 'abc')", vector<PyObject *>(
                                                          {PyUnicode_FromString("abc"), PyUnicode_FromString("def")})),
                                                  TestInput("(2.2, 1)", vector<PyObject *>(
                                                          {PyLong_FromLong(1), PyFloat_FromDouble(2.2)})),
                                          })
                ));
                VerifyOldJitTest(TestCase(
                        "def f(a, b, c):\n    (a, b, c) = (b, c, a)\n    return (a, b, c)",
                        vector<TestInput>({
                                                  TestInput("(2, 3, 1)", vector<PyObject *>(
                                                          {PyLong_FromLong(1), PyLong_FromLong(2),
                                                           PyLong_FromLong(3)})),
                                                  TestInput("(2.2, 3.3, 1.1)", vector<PyObject *>(
                                                          {PyFloat_FromDouble(1.1), PyFloat_FromDouble(2.2),
                                                           PyFloat_FromDouble(3.3)})),
                                                  TestInput("('def', 'ghi', 'abc')", vector<PyObject *>(
                                                          {PyUnicode_FromString("abc"), PyUnicode_FromString("def"),
                                                           PyUnicode_FromString("ghi")})),
                                                  TestInput("(2.2, 3, 1)", vector<PyObject *>(
                                                          {PyLong_FromLong(1), PyFloat_FromDouble(2.2),
                                                           PyLong_FromLong(3)}))
                                          })
                ));
        };
    }
;
