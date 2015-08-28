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
#include <frameobject.h>

using namespace std;

PyCodeObject* CompileCode(const char* code) {
    auto globals = PyDict_New();
    auto builtins = PyThreadState_GET()->interp->builtins;
    PyDict_SetItemString(globals, "__builtins__", builtins);

    auto locals = PyDict_New();
    PyRun_String(code, Py_file_input, globals, locals);
    if (PyErr_Occurred()) {
        PyErr_Print();
        return nullptr;
    }
    auto func = PyObject_GetItem(locals, PyUnicode_FromString("f"));
    auto codeObj = (PyCodeObject*)PyObject_GetAttrString(func, "__code__");

    Py_DECREF(globals);
    Py_DECREF(locals);
    Py_DECREF(func);

    return codeObj;
}

class TestInput {
public:
    const char* m_expected;
    vector<PyObject*> m_args;

    TestInput(const char* expected) {
        m_expected = expected;
    }

    TestInput(const char* expected, vector<PyObject*> args) {
        m_expected = expected;
        m_args = args;
    }
};

class TestCase {
public:
    const char* m_code;
    vector<TestInput> m_inputs;

    TestCase(const char *code, const char* expected) {
        m_code = code;
        m_inputs.push_back(TestInput(expected));

    }

    TestCase(const char *code, TestInput input) {
        m_code = code;
        m_inputs.push_back(input);
    }

    TestCase(const char *code, vector<TestInput> inputs) {
        m_code = code;
        m_inputs = inputs;
    }
};

PyObject* Incremented(PyObject*o){
    Py_INCREF(o);
    return o;
}

void PyJitTest() {
    Py_Initialize();

    auto tupleOfOne = (PyTupleObject*)PyTuple_New(1);
    tupleOfOne->ob_item[0] = PyLong_FromLong(42);

    auto list = PyList_New(1);
    PyList_SetItem(list, 0, PyLong_FromLong(42));

    auto list2 = PyList_New(1);
    PyList_SetItem(list, 0, PyLong_FromLong(42));

    TestCase cases[] = {
        //TestCase(
        //    "def f():\n    try:\n        a = RefCountCheck() + undefined\n        return 'noerr'\n    except:\n        return finalized",
        //    TestInput("True")
        //),
        TestCase(
            "def f(x):\n    if not x:\n        return True\n    return False",
            vector<TestInput>({
                TestInput("False", vector<PyObject*>({ PyLong_FromLong(1) })),
                TestInput("True", vector<PyObject*>({ PyLong_FromLong(0) }))
        })),
        TestCase(
            "def f(a, b):\n    if a in b:\n        return True\n    return False",
            vector<TestInput>({
                TestInput("True", vector<PyObject*>({ PyLong_FromLong(42), Incremented((PyObject*)tupleOfOne) })),
                TestInput("False", vector<PyObject*>({ PyLong_FromLong(1), Incremented((PyObject*)tupleOfOne) }))
        })),
        TestCase(
            "def f(a, b):\n    if a not in b:\n        return True\n    return False",
            vector<TestInput>({
                    TestInput("False", vector<PyObject*>({ PyLong_FromLong(42), Incremented((PyObject*)tupleOfOne) })),
                    TestInput("True", vector<PyObject*>({ PyLong_FromLong(1), Incremented((PyObject*)tupleOfOne) }))
                })
        ),
        TestCase(
            "def f(a, b):\n    if a is b:\n        return True",
            TestInput("True", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(a, b):\n    if a is not b:\n        return True",
            TestInput("True", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(2) }))
        ),
        TestCase(
            "def f(a, b):\n    assert a is b\n    return True",
            TestInput("True", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(a, b):\n    assert a is b\n    return True",
            TestInput("<NULL>", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(2) }))
        ),
        TestCase(
            "def f():\n    a = RefCountCheck()\n    del a\n    return finalized",
            TestInput("True")
        ),
        TestCase(
            "def f():\n    for i in {2:3}:\n        pass\n    return i",
            TestInput("2")
        ),
        TestCase(
            "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            pass",
            TestInput("None")
        ),
        TestCase(
            "def f():\n    for i in range(5):\n        try:\n            pass\n        finally:\n            return i",
            TestInput("0")
        ),
        TestCase(
            "def f():\n    for i in range(5):\n        try:\n            break\n        finally:\n            return i",
            TestInput("0")
        ),
        TestCase(
            "def f():\n    try:\n        raise Exception(2)\n    except Exception as e:\n        return e.args[0]",
            TestInput("2")
        ),
        TestCase(
            "def f():\n    def g(b:1, *, a = 2):\n     return a\n    return g.__annotations__['b']",
            TestInput("1")
        ),
        TestCase(
            "def f():\n    def g(b:1, *, a = 2):\n     return a\n    return g(3)",
            TestInput("2")
        ),
        TestCase(
            "def f():\n    def g(*, a = 2):\n     return a\n    return g()",
            TestInput("2")
        ),
        TestCase(
            "def f():\n    def g(a:1, b:2): pass\n    return g.__annotations__['a']",
            TestInput("1")
        ),
        TestCase(
            "def f():\n    from sys import winver, version_info\n    return winver[0]",
            TestInput("'3'")
        ),
        TestCase(
            "def f():\n    from sys import winver\n    return winver[0]",
            TestInput("'3'")
        ),
        TestCase(
            "def f():\n    def g(*a): return a\n    return g(1, 2, 3, **{})",
            TestInput("(1, 2, 3)")
        ),
        TestCase(
            "def f():\n    def g(**a): return a\n    return g(y = 3, **{})",
            TestInput("{'y': 3}")
        ),
        TestCase(
            "def f():\n    def g(**a): return a\n    return g(**{'x':2})",
            TestInput("{'x': 2}")
        ),
        TestCase(
            "def f():\n    def g(**a): return a\n    return g(x = 2, *())",
            TestInput("{'x': 2}")
        ),
        TestCase(
            "def f():\n    def g(*a): return a\n    return g(*(1, 2, 3))",
            TestInput("(1, 2, 3)")
        ),
        TestCase(
            "def f():\n    def g(*a): return a\n    return g(1, *(2, 3))",
            TestInput("(1, 2, 3)")
        ),
        TestCase(
            "def f():\n    def g(): pass\n    g.abc = {fn.lower() for fn in ['A']}\n    return g.abc",
            TestInput("{'a'}")
        ),
        TestCase(
            "def f():\n    for abc in [1,2,3]:\n        try:\n            break\n        except ImportError:\n            continue\n    return abc",
            TestInput("1")
        ),
        TestCase(
            "def f():\n    a = 1\n    b = 2\n    def x():\n        return a, b\n    return x()",
            TestInput("(1, 2)", vector<PyObject*>({ NULL, PyCell_New(NULL), PyCell_New(NULL) }))
        ),
        TestCase(
            "def f() :\n    def g(x) :\n        return x\n    a = 1\n    def x() :\n        return g(a)\n    return x()",
            TestInput("1", vector<PyObject*>({ NULL, PyCell_New(NULL), PyCell_New(NULL) }))
        ),
        TestCase(
            "def f():\n    try:\n        raise Exception()\n    finally:\n        return 42",
            TestInput("42")
        ),
        TestCase(
            "def f():\n	try:\n		pass\n	except ImportError:\n		pass\n	except Exception as e:\n		pass",
            TestInput("None")
        ),
        TestCase(
            "def f():\n    try:\n        raise Exception('hi')\n    except:\n        return 42",
            TestInput("42")
        ),
        //TestCase(
        //	"def f(sys_module, _imp_module):\n    '''Setup importlib by importing needed built - in modules and injecting them\n    into the global namespace.\n\n    As sys is needed for sys.modules access and _imp is needed to load built - in\n    modules, those two modules must be explicitly passed in.\n\n    '''\n    global _imp, sys\n    _imp = _imp_module\n    sys = sys_module\n\n    # Set up the spec for existing builtin/frozen modules.\n    module_type = type(sys)\n    for name, module in sys.modules.items():\n        if isinstance(module, module_type):\n            if name in sys.builtin_module_names:\n                loader = BuiltinImporter\n            elif _imp.is_frozen(name):\n                loader = FrozenImporter\n            else:\n                continue\n            spec = _spec_from_module(module, loader)\n            _init_module_attrs(spec, module)\n\n    # Directly load built-in modules needed during bootstrap.\n    self_module = sys.modules[__name__]\n    for builtin_name in ('_warnings',):\n        if builtin_name not in sys.modules:\n            builtin_module = _builtin_from_name(builtin_name)\n        else:\n            builtin_module = sys.modules[builtin_name]\n        setattr(self_module, builtin_name, builtin_module)\n\n    # Directly load the _thread module (needed during bootstrap).\n    try:\n        thread_module = _builtin_from_name('_thread')\n    except ImportError:\n        # Python was built without threads\n        thread_module = None\n    setattr(self_module, '_thread', thread_module)\n\n    # Directly load the _weakref module (needed during bootstrap).\n    weakref_module = _builtin_from_name('_weakref')\n    setattr(self_module, '_weakref', weakref_module)\n",
        //	TestInput("42")
        //),
        TestCase(
            "def f():\n    x = {}\n    try:\n        return x[42]\n    except KeyError:\n        return 42",
            TestInput("42")
        ),
        TestCase(
            "def f():\n    try:\n        pass\n    finally:\n        pass\n    return 42",
            TestInput("42")
        ),
        TestCase(
            "def f():\n    x = {}\n    x.update(y=2)\n    return x",
            TestInput("{'y': 2}")
        ),
        TestCase(
            "def f(a):\n    return 1 if a else 2",
            vector<TestInput>({
                TestInput("1", vector<PyObject*>({ PyLong_FromLong(42) })),
                TestInput("2", vector<PyObject*>({ PyLong_FromLong(0) })),
                TestInput("1", vector<PyObject*>({ Incremented(Py_True) })),
                TestInput("2", vector<PyObject*>({ Incremented(Py_False) }))
            })
        ),
        TestCase(
            "def f(v):\n    try:\n        x = v.real\n    except AttributeError:\n        return 42\n    else:\n        return x\n",
            vector<TestInput>({
                TestInput("1", vector<PyObject*>({ PyLong_FromLong(1) })),
                TestInput("42", vector<PyObject*>({ PyUnicode_FromString("hi") }))
        })),
        TestCase(
            "def f():\n    a=1\n    def x():\n        return a\n    return a",
            TestInput("1", vector<PyObject*>({ NULL, PyCell_New(NULL) }))
        ),
        TestCase(
            "def f():\n    x = 2\n    def g():    return x\n    \n    str(g)\n    return g()",
            TestInput("2", vector<PyObject*>({ NULL, PyCell_New(NULL) }))
        ),
        TestCase(
            "def f():\n    a=2\n    def g(): return a\n    return g()",
            TestInput("2", vector<PyObject*>({ NULL, PyCell_New(NULL) }))
        ),
        TestCase(
            "def f():\n    def g(a=2): return a\n    return g()",
            TestInput("2")
        ),
        TestCase(
            "def f():\n    for i in range(5):\n        try:\n            continue\n        finally:\n            return i",
            TestInput("0")
        ),
        TestCase(
            "def f():\n    try:\n        raise Exception()\n    finally:\n        pass",
            TestInput("<NULL>")
        ),
        TestCase(
            "def f():\n    try:\n        pass\n    finally:\n        return 42",
            TestInput("42")
        ),
        TestCase(
            "def f():\n    try:\n        raise Exception()\n    except:\n        return 2",
            TestInput("2")
        ),
        TestCase(
            "def f():\n    try:\n        raise Exception()\n    except Exception:\n        return 2",
            TestInput("2")
        ),
        TestCase(
            "def f():\n    try:\n        raise Exception()\n    except AssertionError:\n        return 2\n    return 4",
            TestInput("<NULL>")
        ),
        TestCase(
            "def f():\n    global x\n    x = 2\n    return x",
            TestInput("2")
        ),
        TestCase(	// hits JUMP_FORWARD
            "def f(a):\n    if a:\n        x = 1\n    else:\n        x = 2\n    return x",
            vector<TestInput>({
                TestInput("1", vector<PyObject*>({ PyLong_FromLong(42) })),
                TestInput("2", vector<PyObject*>({ PyLong_FromLong(0) })),
                TestInput("1", vector<PyObject*>({ Incremented(Py_True) })),
                TestInput("2", vector<PyObject*>({ Incremented(Py_False) }))
        })),
        TestCase(
            "def f(a, b):\n    return a and b",
            vector<TestInput>({
                TestInput("2", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(2) })),
                TestInput("0", vector<PyObject*>({ PyLong_FromLong(0), PyLong_FromLong(1) })),
                TestInput("False", vector<PyObject*>({ Py_True, Py_False })),
                TestInput("False", vector<PyObject*>({ Py_False, Py_True })),
                TestInput("True", vector<PyObject*>({ Py_True, Py_True })),
                TestInput("False", vector<PyObject*>({ Py_False, Py_False })),
            })
        ),
        TestCase(
            "def f(a, b):\n    return a or b",
            vector<TestInput>({
                TestInput("1", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(2) })),
                TestInput("1", vector<PyObject*>({ PyLong_FromLong(0), PyLong_FromLong(1) })),
                TestInput("True", vector<PyObject*>({ Py_True, Py_False })),
                TestInput("True", vector<PyObject*>({ Py_False, Py_True })),
                TestInput("True", vector<PyObject*>({ Py_True, Py_True })),
                TestInput("False", vector<PyObject*>({ Py_False, Py_False })),
        })),
        TestCase(		 // hits ROT_TWO, POP_TWO, DUP_TOP, ROT_THREE
            "def f(a, b, c):\n    return a < b < c",
            vector<TestInput>({
            TestInput("True", vector<PyObject*>({ PyLong_FromLong(2), PyLong_FromLong(3), PyLong_FromLong(4) })),
            TestInput("False", vector<PyObject*>({ PyLong_FromLong(4), PyLong_FromLong(3), PyLong_FromLong(2) }))
        })
        ),
        TestCase(
            "def f(a, b):\n    a **= b\n    return a",
            TestInput("8", vector<PyObject*>({ PyLong_FromLong(2), PyLong_FromLong(3) }))
        ),
        TestCase(
            "def f(a, b):\n    a *= b\n    return a",
            TestInput("6", vector<PyObject*>({ PyLong_FromLong(2), PyLong_FromLong(3) }))
        ),
        TestCase(
            "def f(a, b):\n    a /= b\n    return a",
            TestInput("0.5", vector<PyObject*>({ PyLong_FromLong(2), PyLong_FromLong(4) }))
        ),
        TestCase(
            "def f(a, b):\n    a //= b\n    return a",
            vector<TestInput>({
                    TestInput("0", vector<PyObject*>({ PyLong_FromLong(2), PyLong_FromLong(4) })),
                    TestInput("2", vector<PyObject*>({ PyLong_FromLong(4), PyLong_FromLong(2) }))
        })),
        TestCase(
            "def f(a, b):\n    a %= b\n    return a",
            TestInput("1", vector<PyObject*>({ PyLong_FromLong(3), PyLong_FromLong(2) }))
        ),
        TestCase(
            "def f(a, b):\n    a += b\n    return a",
            vector<TestInput>({
                    TestInput("9", vector<PyObject*>({ PyLong_FromLong(6), PyLong_FromLong(3) })),
                    TestInput("'hibye'", vector<PyObject*>({ PyUnicode_FromString("hi"), PyUnicode_FromString("bye") }))
        })),
        TestCase(
            "def f(a, b):\n    a -= b\n    return a",
            TestInput("1", vector<PyObject*>({ PyLong_FromLong(3), PyLong_FromLong(2) }))
        ),
        TestCase(
        "def f(a, b):\n    a <<= b\n    return a",
        TestInput("4", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(2) }))
        ),
        TestCase(
            "def f(a, b):\n    a >>= b\n    return a",
            TestInput("1", vector<PyObject*>({ PyLong_FromLong(2), PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(a, b):\n    a &= b\n    return a",
            TestInput("2", vector<PyObject*>({ PyLong_FromLong(6), PyLong_FromLong(3) }))
        ),
        TestCase(
            "def f(a, b):\n    a ^= b\n    return a",
            TestInput("5", vector<PyObject*>({ PyLong_FromLong(6), PyLong_FromLong(3) }))
        ),
        TestCase(
            "def f(a, b):\n    a |= b\n    return a",
            TestInput("7", vector<PyObject*>({ PyLong_FromLong(6), PyLong_FromLong(3) }))
        ),
        TestCase(
            "def f(x):\n    return -x",
            TestInput("-1", vector<PyObject*>({ PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(x):\n    return +x",
            TestInput("1", vector<PyObject*>({ PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(x):\n    return ~x",
            TestInput("-2", vector<PyObject*>({ PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(a, b): return a << b",
            TestInput("4", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(2) }))
        ),
        TestCase(
            "def f(a, b): return a >> b",
            TestInput("2", vector<PyObject*>({ PyLong_FromLong(4), PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(a, b): return a & b",
            TestInput("2", vector<PyObject*>({ PyLong_FromLong(6), PyLong_FromLong(3) }))
        ),
        TestCase(
            "def f(a, b): return a | b",
            TestInput("3", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(2) }))
        ),
        TestCase(
            "def f(a, b): return a ^ b",
            TestInput("6", vector<PyObject*>({ PyLong_FromLong(3), PyLong_FromLong(5) }))
        ),
        TestCase(
            "def f(x):\n    return not x",
            vector<TestInput>({
                TestInput("False", vector<PyObject*>({ PyLong_FromLong(1) })),
                TestInput("True", vector<PyObject*>({ PyLong_FromLong(0) }))
        })),
        TestCase(
            "def f():\n    for i in range(3):\n        if i == 0: continue\n        break\n    return i",
            TestInput("1")
        ),
        TestCase(
            "def f():\n    for i in range(3):\n        if i == 1: break\n    return i",
            TestInput("1")
        ),
        TestCase(
            "def f():\n    return [1,2,3][1:]",
            TestInput("[2, 3]")
        ),
        TestCase(
            "def f():\n    return [1,2,3][:1]",
            TestInput("[1]")
        ),
        TestCase(
            "def f():\n    return [1,2,3][1:2]",
            TestInput("[2]")
        ),
        TestCase(
            "def f():\n    return [1,2,3][0::2]",
            TestInput("[1, 3]")
        ),
        TestCase(
            "def f():\n    x = 2\n    def g():    return x\n    return g()",
            TestInput("2", vector<PyObject*>({ NULL, PyCell_New(NULL) }))
        ),
        TestCase(
            "def f():\n    a, *b, c = range(3)\n    return a",
            TestInput("0")
        ),
        TestCase(
            "def f():\n    a, *b, c = range(3)\n    return b",
            TestInput("[1]")
        ),
        TestCase(
            "def f():\n    a, *b, c = range(3)\n    return c",
            TestInput("2")
        ),
        TestCase(
            "def f():\n    a, *b, c = 1, 2, 3\n    return a",
            TestInput("1")
        ),
        TestCase(
            "def f():\n    a, *b, c = 1, 2, 3\n    return b",
            TestInput("[2]")
        ),
        TestCase(
            "def f():\n    a, *b, c = 1, 2, 3\n    return c",
            TestInput("3")
        ),
        TestCase(
            "def f():\n    a, *b, c = 1, 3\n    return c",
            TestInput("3")
        ),
        TestCase(
            "def f():\n    a, *b, c = 1, 3\n    return b",
            TestInput("[]")
        ),
        TestCase(
            "def f():\n    a, *b, c = [1, 2, 3]\n    return a",
            TestInput("1")
        ),
        TestCase(
            "def f():\n    a, *b, c = [1, 2, 3]\n    return b",
            TestInput("[2]")
        ),
        TestCase(
            "def f():\n    a, *b, c = [1, 2, 3]\n    return c",
            TestInput("3")
        ),
        TestCase(
            "def f():\n    a, *b, c = [1, 3]\n    return c",
            TestInput("3")
        ),
        TestCase(
            "def f():\n    a, *b, c = [1, 3]\n    return b",
            TestInput("[]")
        ),
        TestCase(
            "def f():\n    a, b = range(2)\n    return a",
            TestInput("0")
        ),
        TestCase(
            "def f():\n    a, b = 1, 2\n    return a",
            TestInput("1")
        ),
        TestCase(
            "def f():\n    class C:\n        pass\n    return C",
            TestInput("<class 'C'>")
        ),
        TestCase(
            "def f():\n    a = 0\n    for x in[1]:\n        a = a + 1\n    return a",
            TestInput("1")
        ),
        TestCase(
            "def f(): return [x for x in range(2)]",
            TestInput("[0, 1]")
        ),
        TestCase(
            "def f():\n    def g(): pass\n    return g.__name__",
            TestInput("'g'")
        ),
        TestCase(
            "def f(a, b, c):\n    a[b] = c\n    return a[b]",
            TestInput("True", vector<PyObject*>({ list, PyLong_FromLong(0), Incremented(Py_True) }))
        ),
        TestCase(
            "def f(a, b):\n    del a[b]\n    return len(a)",
            TestInput("0", vector<PyObject*>({ list2, PyLong_FromLong(0) }))
        ),
        TestCase(
            "def f(a, b):\n    return a in b",
            vector<TestInput>({
                    TestInput("True", vector<PyObject*>({ PyLong_FromLong(42), Incremented((PyObject*)tupleOfOne) })),
                    TestInput("False", vector<PyObject*>({ PyLong_FromLong(1), Incremented((PyObject*)tupleOfOne) }))
                })),
        TestCase(
            "def f(a, b):\n    return a not in b",
            vector<TestInput>({
                TestInput("False", vector<PyObject*>({ PyLong_FromLong(42), Incremented((PyObject*)tupleOfOne) })),
                TestInput("True", vector<PyObject*>({ PyLong_FromLong(1), Incremented((PyObject*)tupleOfOne) }))
            })
        ),
        TestCase(
            "def f(a, b):\n    return a == b",
            TestInput("True", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(a, b):\n    return a != b",
            TestInput("False", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(a, b):\n    return a is b",
            TestInput("True", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(a, b):\n    return a is not b",
            TestInput("False", vector<PyObject*>({ PyLong_FromLong(1), PyLong_FromLong(1) }))
        ),
        TestCase(
            "def f(a):\n    while a:\n        a = a - 1\n    return a",
            TestInput("0", vector<PyObject*>({ PyLong_FromLong(42) }))
        ),
        TestCase(
            "def f(a):\n    if a:\n        return 1\n    else:\n        return 2",
            vector<TestInput>({
                TestInput("1", vector<PyObject*>({ PyLong_FromLong(42) })),
                TestInput("2", vector<PyObject*>({ PyLong_FromLong(0) })),
                TestInput("1", vector<PyObject*>({ Incremented(Py_True) })),
                TestInput("2", vector<PyObject*>({ Incremented(Py_False) }))
        })),
        TestCase(
            "def f(a): return a.real",
            TestInput("42", vector<PyObject*>({ PyLong_FromLong(42) }))
        ),
        TestCase("def f(): return {1,2}", "{1, 2}"),
        TestCase("def f(): return {}", "{}"),
        TestCase("def f(): return {1:2}", "{1: 2}"),
        TestCase("def f(): return []", "[]"),
        TestCase("def f(): return [1]", "[1]"),
        TestCase("def f(): return ()", "()"),
        TestCase("def f(): return (1, )", "(1,)"),
        TestCase("def f(): return (1, 2)", "(1, 2)"),
        TestCase("def f(): x = 1; return x", "1"),
        TestCase("def f(): return int()", "0"),
        TestCase("def f(): return range(5)", "range(0, 5)"),
        TestCase(
            "def f(a, b): return a[b]",
            TestInput("42", vector<PyObject*>({ (PyObject*)tupleOfOne, PyLong_FromLong(0) }))
        ),
        TestCase(
            "def f(a, b): return a / b",
            TestInput("3.0", vector<PyObject*>({ PyLong_FromLong(75), PyLong_FromLong(25) }))
        ),
        TestCase(
            "def f(a, b): return a // b",
            TestInput("2", vector<PyObject*>({ PyLong_FromLong(75), PyLong_FromLong(30) }))
        ),
        TestCase(
            "def f(a, b): return a % b",
            vector<TestInput>({
                    TestInput("2", vector<PyObject*>({ PyLong_FromLong(32), PyLong_FromLong(3) })),
                    TestInput("'abcdef'", vector<PyObject*>({ PyUnicode_FromString("abc%s"), PyUnicode_FromString("def") }))
                })
        ),
        TestCase(
            "def f(a, b): return a ** b",
            TestInput("32", vector<PyObject*>({ PyLong_FromLong(2), PyLong_FromLong(5) }))
        ),
        TestCase(
            "def f(a, b): return a - b",
            TestInput("25", vector<PyObject*>({ PyLong_FromLong(75), PyLong_FromLong(50) }))
        ),
        TestCase(
            "def f(a, b): return a * b",
            TestInput("4200", vector<PyObject*>({ PyLong_FromLong(42), PyLong_FromLong(100) }))
        ),
        TestCase(
            "def f(a, b): return a + b",
            vector<TestInput>({
                    TestInput("142", vector<PyObject*>({ PyLong_FromLong(42), PyLong_FromLong(100) })),
                    TestInput("'abcdef'", vector<PyObject*>({ PyUnicode_FromString("abc"), PyUnicode_FromString("def") }))
                })
        )
    };

    typedef PyObject*(__stdcall * evalFunc)(PyFrameObject*);


    for (int i = 0; i < _countof(cases); i++) {
        auto curCase = cases[i];
        printf("---\r\n");
        puts(curCase.m_code);
        printf("\r\n");
        auto codeObj = CompileCode(curCase.m_code);
        auto addr = JitCompile(codeObj);

        for (auto curInput = 0; curInput < curCase.m_inputs.size(); curInput++) {
            auto input = curCase.m_inputs[curInput];

            auto globals = PyDict_New();
            auto builtins = PyThreadState_GET()->interp->builtins;
            PyDict_SetItemString(globals, "__builtins__", builtins);
            PyRun_String("finalized = False\nclass RefCountCheck:\n    def __del__(self):\n        print('finalizing')\n        global finalized\n        finalized = True\n    def __add__(self, other):\n        return self", Py_file_input, globals, globals);
            if (PyErr_Occurred()) {
                PyErr_Print();
                return;
            }

            auto frame = PyFrame_New(PyThreadState_Get(), codeObj, globals, PyDict_New());
            for (size_t arg = 0; arg < input.m_args.size(); arg++) {
                frame->f_localsplus[arg] = input.m_args[arg];
            }

            auto res = ((evalFunc)addr)(frame);

            auto repr = PyUnicode_AsUTF8(PyObject_Repr(res));
            if (strcmp(input.m_expected, repr) != 0) {
                printf("Unexpected: %s\r\n", repr);
                if (strcmp(repr, "<NULL>") == 0) {
                    PyErr_Print();
                }
                _ASSERT(FALSE);
            }
            if (res != nullptr) {
                _ASSERT(!PyErr_Occurred());
            }
            else{
                _ASSERT(PyErr_Occurred());
                PyErr_Clear();
            }
            //Py_DECREF(frame);
            Py_XDECREF(res);
            Py_DECREF(codeObj);
        }
    }

    Py_Finalize();
    printf("Done\r\n");
}

int main() {
    CoInitialize(NULL);
    JitInit();
    PyJitTest();
}