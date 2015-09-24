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
#include <initializer_list>

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

class AIVerifier {
public:
    virtual void verify(AbstractInterpreter& interpreter) = 0;
};

class StackVerifier : public AIVerifier {
    size_t m_byteCodeIndex, m_stackIndex;
    AbstractValueKind m_kind;
public:
    StackVerifier(size_t byteCodeIndex, size_t stackIndex, AbstractValueKind kind) {
        m_byteCodeIndex = byteCodeIndex;
        m_stackIndex = stackIndex;
        m_kind = kind;
    }

    virtual void verify(AbstractInterpreter& interpreter) {
        auto info = interpreter.get_stack_info(m_byteCodeIndex);
        _ASSERTE(m_kind == info[info.size() - m_stackIndex - 1].Value->kind());
    };
};

/* Verify the inferred type stored in the locals array before a specified bytecode executes. */
class VariableVerifier : public AIVerifier {
private:
    // The bytecode whose locals state we are checking *before* execution.
    size_t m_byteCodeIndex;
    // The locals index whose type we are checking.
    size_t m_localIndex;
    // The inferred type.
    AbstractValueKind m_kind;
    // Has the value been defined yet?
    bool m_undefined;
public:
    VariableVerifier(size_t byteCodeIndex, size_t localIndex, AbstractValueKind kind, bool undefined = false) {
        m_byteCodeIndex = byteCodeIndex;
        m_localIndex = localIndex;
        m_undefined = undefined;
        m_kind = kind;
    }

    virtual void verify(AbstractInterpreter& interpreter) {
        auto local = interpreter.get_local_info(m_byteCodeIndex, m_localIndex);
        _ASSERTE(local.IsMaybeUndefined == m_undefined);
        _ASSERTE(local.ValueInfo.Value->kind() == m_kind);
    };
};

class ReturnVerifier : public AIVerifier {
    AbstractValueKind m_kind;
public:
    ReturnVerifier(AbstractValueKind kind) {
        m_kind = kind;
    }

    virtual void verify(AbstractInterpreter& interpreter) {
        _ASSERTE(m_kind == interpreter.get_return_info()->kind());
    }
};

class BoxVerifier : public AIVerifier {
    size_t m_byteCodeIndex;
    bool m_shouldBox;

public:
    BoxVerifier(size_t byteCodeIndex, bool shouldBox) {
        m_byteCodeIndex = byteCodeIndex;
        m_shouldBox = shouldBox;
    }

    virtual void verify(AbstractInterpreter& interpreter) {
        _ASSERTE(m_shouldBox == interpreter.should_box(m_byteCodeIndex));
    }
};

class AITestCase {
private:
public:
    const char* m_code;
    vector<AIVerifier*> m_verifiers;

    AITestCase(const char *code, AIVerifier* verifier) {
        m_code = code;
        m_verifiers.push_back(verifier);
    }

    AITestCase(const char *code, vector<AIVerifier*> verifiers) {
        m_code = code;
        m_verifiers = verifiers;
    }

    AITestCase(const char *code, std::initializer_list<AIVerifier*> list) {
        m_code = code;
        m_verifiers = list;
    }

    ~AITestCase() {
        for (auto verifier : m_verifiers) {
            delete verifier;
        }
    }

    void verify(AbstractInterpreter& interpreter) {
        for (auto cur : m_verifiers) {
            cur->verify(interpreter);
        }
    }
};



PyObject* Incremented(PyObject*o) {
    Py_INCREF(o);
    return o;
}

void PyJitTest() {
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
            "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        except EnvironmentError:\n            pass\n    return 1",
            TestInput("1")
            ),
        TestCase(
            "def f():\n    try:\n        min(1,2)\n    finally:\n        try:\n            min(1,2)\n        finally:\n            pass\n    return 1",
            TestInput("1")
            ),
        // Simple optimized code test cases...
        TestCase(
            "def f():\n    x = 1.0\n    return x",
            TestInput("1.0")
        ),
        TestCase(
            "def f():\n    x = 1.0\n    y = 2.0\n    z = x + y\n    return z",
            TestInput("3.0")
        ),
        TestCase(
            "def f():\n    x = 1.0\n    y = 2.0\n    z = x - y\n    return z",
            TestInput("-1.0")
        ),
        TestCase(
            "def f():\n    x = 1.0\n    y = 2.0\n    z = x / y\n    return z",
            TestInput("0.5")
        ),
        TestCase(
            "def f():\n    x = 1.0\n    y = 2.0\n    z = x // y\n    return z",
            TestInput("0.0")
        ),
        TestCase(
            "def f():\n    x = 1.0\n    y = 2.0\n    z = x % y\n    return z",
            TestInput("1.0")
        ),
        TestCase(
            "def f():\n    x = 2.0\n    y = 3.0\n    z = x * y\n    return z",
            TestInput("6.0")
        ),
        TestCase(
            "def f():\n    x = 2.0\n    y = 3.0\n    z = x ** y\n    return z",
            TestInput("8.0")
        ),
        TestCase(
            "def f():\n    x = 2.0\n    y = 3.0\n    if x == y:\n        return True\n    return False",
            TestInput("False")
        ),
        TestCase(
            "def f():\n    x = 3.0\n    y = 3.0\n    if x == y:\n        return True\n    return False",
            TestInput("True")
        ),
        TestCase(
        "def f():\n    x = 2.0\n    y = 3.0\n    if x != y:\n        return True\n    return False",
        TestInput("True")
        ),
        TestCase(
        "def f():\n    x = 3.0\n    y = 3.0\n    if x != y:\n        return True\n    return False",
        TestInput("False")
        ),
        TestCase(
        "def f():\n    x = 2.0\n    y = 3.0\n    if x >= y:\n        return True\n    return False",
        TestInput("False")
        ),
        TestCase(
        "def f():\n    x = 3.0\n    y = 3.0\n    if x >= y:\n        return True\n    return False",
        TestInput("True")
        ),
        TestCase(
        "def f():\n    x = 2.0\n    y = 3.0\n    if x > y:\n        return True\n    return False",
        TestInput("False")
        ),
        TestCase(
        "def f():\n    x = 4.0\n    y = 3.0\n    if x > y:\n        return True\n    return False",
        TestInput("True")
        ),
        TestCase(
        "def f():\n    x = 3.0\n    y = 2.0\n    if x <= y:\n        return True\n    return False",
        TestInput("False")
        ),
        TestCase(
        "def f():\n    x = 3.0\n    y = 3.0\n    if x <= y:\n        return True\n    return False",
        TestInput("True")
        ),
        TestCase(
        "def f():\n    x = 3.0\n    y = 2.0\n    if x < y:\n        return True\n    return False",
        TestInput("False")
        ),
        TestCase(
        "def f():\n    x = 3.0\n    y = 4.0\n    if x < y:\n        return True\n    return False",
        TestInput("True")
        ),
        TestCase(
        "def f():\n    x = 1.0\n    y = 2.0\n    x += y\n    return x",
        TestInput("3.0")
        ),
        TestCase(
        "def f():\n    x = 1.0\n    y = 2.0\n    x -= y\n    return x",
        TestInput("-1.0")
        ),
        TestCase(
        "def f():\n    x = 1.0\n    y = 2.0\n    x /= y\n    return x",
        TestInput("0.5")
        ),
        TestCase(
        "def f():\n    x = 1.0\n    y = 2.0\n    x //= y\n    return x",
        TestInput("0.0")
        ),
        TestCase(
        "def f():\n    x = 1.0\n    y = 2.0\n    x %= y\n    return x",
        TestInput("1.0")
        ),
        TestCase(
        "def f():\n    x = 2.0\n    y = 3.0\n    x *= y\n    return x",
        TestInput("6.0")
        ),
        TestCase(
        "def f():\n    x = 2.0\n    y = 3.0\n    x **= y\n    return x",
        TestInput("8.0")
        ),
        // fully optimized complex code
        TestCase(
        "def f():\n    pi = 0.\n    k = 0.\n    while k < 256.:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi",
        TestInput("3.141592653589793")
        ),
        // division error handling code gen with value on the stack
        TestCase(
        "def f():\n    x = 1.0\n    y = 2.0\n    z = 3.0\n    return x + y / z",
        TestInput("1.6666666666666665")
        ),
        // division by zero error case
        TestCase(
        "def f():\n    x = 1\n    y = 0\n    try:\n        return x / y\n    except:\n        return 42",
        TestInput("42")
        ),
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

            auto res = addr->j_evalfunc(nullptr, frame);

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
            else {
                _ASSERT(PyErr_Occurred());
                PyErr_Clear();
            }
            //Py_DECREF(frame);
            Py_XDECREF(res);
            Py_DECREF(codeObj);
        }
    }

    printf("JIT tests passed\r\n");
}

void AbsIntTest() {
    AITestCase cases[] = {
        // Basic typing tests...
        AITestCase(
        "def f(): x = 1",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer, false),     // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = None",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_None, false),     // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = 'abc'",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_String, false),      // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = []",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // BUILD_LIST 0
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_List, false),      // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = {}",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // BUILD_MAP 0
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Dict, false),      // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = ()",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // BUILD_TUPLE 0
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Tuple, false),      // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = True",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bool, false),      // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = False",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bool, false),      // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = 1.0",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float, false),      // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = b'abc'",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bytes, false),      // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = {1,}",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // BUILD_SET 1
            new VariableVerifier(6, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(9, 0, AVK_Set, false),         // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): x = 3j",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex, false),     // LOAD_CONST None
            new ReturnVerifier(AVK_None)
        }),
        AITestCase(
        "def f(): return 42",
        {
            new ReturnVerifier(AVK_Integer),    // VC 2013 ICE's with only a single value here...
            new ReturnVerifier(AVK_Integer)
        }),
        AITestCase(
        "def f(x):\n    if x:    return 42\n    return 'abc'",
        {
            new ReturnVerifier(AVK_Any),    // VC 2013 ICE's with only a single value here...
            new ReturnVerifier(AVK_Any)
        }),
        // We won't follow blocks following a return statement...
        AITestCase(
        "def f(x):\n    return 42\n    if 'abc': return 'abc'",
        {
            new ReturnVerifier(AVK_Integer),    // VC 2013 ICE's with only a single value here...
            new ReturnVerifier(AVK_Integer)
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = +x",
        {
            new VariableVerifier(10, 1, AVK_Undefined, true),   // 10 STORE_FAST               1 (y)
            new VariableVerifier(13, 1, AVK_Integer)            // 13 LOAD_CONST               0 (None)
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    y = +x",
        {
            new VariableVerifier(10, 1, AVK_Undefined, true),   // 10 STORE_FAST               1 (y)
            new VariableVerifier(13, 1, AVK_Float)            // 13 LOAD_CONST               0 (None)
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = -x",
        {
            new VariableVerifier(10, 1, AVK_Undefined, true),   // 10 STORE_FAST               1 (y)
            new VariableVerifier(13, 1, AVK_Integer)            // 13 LOAD_CONST               0 (None)
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    y = -x",
        {
            new VariableVerifier(10, 1, AVK_Undefined, true),   // 10 STORE_FAST               1 (y)
            new VariableVerifier(13, 1, AVK_Float)            // 13 LOAD_CONST               0 (None)
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = ~x",
        {
            new VariableVerifier(10, 1, AVK_Undefined, true),   // 10 STORE_FAST               1 (y)
            new VariableVerifier(13, 1, AVK_Integer)            // 13 LOAD_CONST               0 (None)
        }),
        // Basic delete
        AITestCase(
        "def f():\n    x = 1\n    del x",
        {
            new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST
            new VariableVerifier(6, 0, AVK_Integer, false),     // DELETE_FAST
            new VariableVerifier(9, 0, AVK_Undefined, true),    // LOAD_CONST None
        }),
        // Delete / Undefined merging...
        AITestCase(
        "def f():\n    x = 1\n    if abc:\n        del x\n        y = 1",
        {
            new VariableVerifier(0,  0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(3,  0, AVK_Undefined, true),    // STORE_FAST
            new VariableVerifier(6,  0, AVK_Integer, false),     // LOAD_GLOBAL abc
            new VariableVerifier(12, 0, AVK_Integer, false),     // DELETE_FAST
            new VariableVerifier(15, 0, AVK_Undefined, true),    // LOAD_CONST 1
            new VariableVerifier(21, 0, AVK_Any, true),          // LOAD_CONST None
        }),
        // Type tracking / merging...
        AITestCase(
        "def f():\n    if abc:\n        x = 1\n    else:\n        x = 'abc'\n        y = 1",
        {
            new VariableVerifier(9, 0, AVK_Undefined, true),    // STORE_FAST x
            new VariableVerifier(12, 0, AVK_Integer),           // JUMP_FORWARD
            new VariableVerifier(18, 0, AVK_Undefined, true),   // STORE_FAST x
            new VariableVerifier(21, 0, AVK_String),            // LOAD_CONST
            new VariableVerifier(27, 0, AVK_Any),               // LOAD_CONST None
        }),
        // Binary integer operations
        AITestCase(
        "def f():\n    x = 1\n    y = x + 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x - 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x * 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x % 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x << 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x >> 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x & 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x | 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x ^ 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x ** 2",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = x // 1",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Integer),           // STORE_FAST 1
        }),
        // Integer in place binary operations
        AITestCase(
        "def f():\n    x = 1\n    x += 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x -= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x *= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x %= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x <<= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x >>= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x &= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x |= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x ^= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x **= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1\n    x //= 1",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Integer),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Integer),           // STORE_FAST 1
        }),
        // Float binary operations
        AITestCase(
        "def f():\n    x = 1.0\n    y = x + 1.0",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    y = x - 1.0",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    y = x * 1.0",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    y = x % 1.0",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    y = x ** 2.0",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    y = x / 1.0",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    y = x // 1.0",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Float),           // STORE_FAST 1
        }),
        // Float in place binary operations
        AITestCase(
        "def f():\n    x = 1.0\n    x += 1.0",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    x -= 1.0",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    x *= 1.0",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    x %= 1.0",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    x **= 1.0",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    x /= 1.0",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Float),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 1.0\n    x //= 1.0",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Float),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Float),           // STORE_FAST 1
        }),

        // Boolean binary operations
        // Bool/bool
        AITestCase(
            "def f():\n    x = True\n    y = False\n    z = x & y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bool),              // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = False\n    z = x | y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bool),              // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = False\n    z = x ^ y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bool),              // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = False\n    x &= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Bool)               // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = False\n    x |= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Bool)               // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = False\n    x ^= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Bool)               // x assigned in-place
            }
        ),
        // Bool/bytes
        AITestCase(
            "def f():\n    x = True\n    y = b'a'\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = b'a'\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),              // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)               // x assigned in-place
            }
        ),
        // Bool/complex
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        // Bool/float
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ),
        // Bool/int
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bool),              // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Bool)               // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x & y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x << y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x | y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x >> y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x ^ y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x &= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x <<= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x |= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x >>= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = 42\n    x ^= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ),
        // Bool/list
        AITestCase(
            "def f():\n    x = True\n    y = []\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_List),              // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = []\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ),
        // Bool/str
        AITestCase(
            "def f():\n    x = True\n    y = ''\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = ''\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ),
        // Bool/tuple
        AITestCase(
            "def f():\n    x = True\n    y = ()\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Tuple),             // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = ()\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(22, 0, AVK_Tuple)              // x assigned in-place
            }
        ),
        // Bool unary operations
        AITestCase(
            "def f():\n    x = True\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = ~ x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Integer)            // y assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = - x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Integer)            // y assigned
            }
        ),
        AITestCase(
            "def f():\n    x = True\n    y = + x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Integer)            // y assigned
            }
        ),

        // Complex binary operations
        // Complex/bool
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        // Complex/complex
        AITestCase(
        "def f():\n    x = 3j\n    y = x + 3j",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 3j\n    y = x - 3j",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 3j\n    y = x * 3j",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 3j\n    y = x ** 2j",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 3j\n    y = x / 3j",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 3j\n    x += 3j",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 3j\n    x -= 3j",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 3j\n    x *= 3j",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 3j\n    x **= 3j",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 3j\n    x /= 3j",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
        }),
        // Complex/float
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        // Complex/int
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ),
        // Complex unary operations
        AITestCase(
        "def f():\n    x = 3j\n    y = +x",
        {
            new VariableVerifier(10, 1, AVK_Undefined, true),   // 10 STORE_FAST               1 (y)
            new VariableVerifier(13, 1, AVK_Complex)            // 13 LOAD_CONST               0 (None)
        }),
        AITestCase(
        "def f():\n    x = 3j\n    y = -x",
        {
            new VariableVerifier(10, 1, AVK_Undefined, true),   // 10 STORE_FAST               1 (y)
            new VariableVerifier(13, 1, AVK_Complex)            // 13 LOAD_CONST               0 (None)
        }),
        AITestCase(
            "def f():\n    x = 3j\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ),

        // Dict unary operations
        AITestCase(
            "def f():\n    x = {}\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Dict),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ),

        // Binary String operations
        AITestCase(
        "def f():\n    x = 'abc'\n    y = x + 'def'",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_String),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_String),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 'abc'\n    x += 'def'",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_String),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_String),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 'abc'\n    y = x % 'def'",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_String),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_String),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 'abc'\n    y = x % 42",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_String),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_String),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 'abc'\n    x %= 'def'",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_String),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_String),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 'abc'\n    y = x * 3",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_String),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_String),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = 'abc'\n    x *= 3",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_String),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_String),           // STORE_FAST 1
        }),

        // Binary bytes operations
        AITestCase(
        "def f():\n    x = b'abc'\n    y = x + b'def'",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bytes),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Bytes),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = b'abc'\n    x += b'def'",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bytes),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Bytes),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = b'abc'\n    y = x % b'def'",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bytes),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Bytes),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = b'abc'\n    y = x % 42",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bytes),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Bytes),           // STORE_FAST 1
        }), AITestCase(
        "def f():\n    x = b'abc'\n    x %= b'def'",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bytes),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Bytes),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = b'abc'\n    y = x * 3",
        {
            new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bytes),            // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Bytes),           // STORE_FAST 1
        }),
        AITestCase(
        "def f():\n    x = b'abc'\n    x *= 3",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Bytes),            // LOAD_FAST 0
            new VariableVerifier(16, 0, AVK_Bytes),           // STORE_FAST 1
        }),

        // Tuple binary operations
        AITestCase(
        "def f():\n    x = (1,2)\n    y = x + x",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Tuple),              // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Tuple),             // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = (1,2)\n    y = x * 3",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Tuple),              // LOAD_FAST 0
            new VariableVerifier(16, 1, AVK_Tuple),             // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = (1,2)\n    y = x[0:1]",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(6, 0, AVK_Tuple),              // LOAD_FAST 0
            new VariableVerifier(22, 1, AVK_Tuple),             // LOAD_CONST None
        }),

        // List binary operations
        AITestCase(
        "def f():\n    x = [1,2]\n    y = x + x",
        {
            new VariableVerifier(9, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(12, 0, AVK_List),              // LOAD_FAST 0
            new VariableVerifier(22, 1, AVK_List),             // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = [1,2]\n    y = x * 3",
        {
            new VariableVerifier(9, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(12, 0, AVK_List),              // LOAD_FAST 0
            new VariableVerifier(22, 1, AVK_List),             // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = [1,2]\n    y = x[0:1]",
        {
            new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(12, 0, AVK_List),              // LOAD_FAST 0
            new VariableVerifier(28, 1, AVK_List),             // LOAD_CONST None
        }),

        // Unary not
        AITestCase(
        "def f():\n    x = not 1",
        {
            new VariableVerifier(4, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(7, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not 1.0",
        {
            new VariableVerifier(4, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(7, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not True",
        {
            new VariableVerifier(4, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(7, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not 3j",
        {
            new VariableVerifier(4, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(7, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not ()",
        {
            new VariableVerifier(4, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(7, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not []",
        {
            new VariableVerifier(4, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(7, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not {3,}",
        {
            new VariableVerifier(7, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(10, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not None",
        {
            new VariableVerifier(4, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(7, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not 'abc'",
        {
            new VariableVerifier(4, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(7, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not b'abc'",
        {
            new VariableVerifier(4, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(7, 0, AVK_Bool),               // LOAD_CONST None
        }),
        AITestCase(
        "def f():\n    x = not (lambda:42)",
        {
            new VariableVerifier(10, 0, AVK_Undefined, true),    // STORE_FAST 0
            new VariableVerifier(13, 0, AVK_Bool),               // LOAD_CONST None
        }),

        // Escape tests
        AITestCase(
        "def f():\n    pi = 0.\n    k = 0.\n    while k < 256.:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi",
        {
            new ReturnVerifier(AVK_Float),
            new BoxVerifier(0, false),  // LOAD_CONST 0
            new BoxVerifier(6, false),  // LOAD_CONST 0.0
            new BoxVerifier(15, false),  // LOAD_FAST k
            new BoxVerifier(18, false),  // LOAD_CONST 256
            new BoxVerifier(27, false),  // LOAD_FAST pi
        }),
        // Can't optimize due to float int comparison:
        AITestCase(
        "def f():\n    pi = 0.\n    k = 0.\n    while k < 256:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi",
        {
            new ReturnVerifier(AVK_Float),
            new BoxVerifier(0, true),  // LOAD_CONST 0
            new BoxVerifier(6, true),  // LOAD_CONST 0.0
            new BoxVerifier(15, true),  // LOAD_FAST k
            new BoxVerifier(18, true),  // LOAD_CONST 256
            new BoxVerifier(27, true),  // LOAD_FAST pi
        }),
        // Can't optimize because pi is assigned an int initially
        AITestCase(
        "def f():\n    pi = 0\n    k = 0.\n    while k < 256.:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi",
        {
            new ReturnVerifier(AVK_Any),
            new BoxVerifier(0, true),  // LOAD_CONST 0
            new BoxVerifier(6, true),  // LOAD_CONST 0.0
            new BoxVerifier(15, true),  // LOAD_FAST k
            new BoxVerifier(18, true),  // LOAD_CONST 256
            new BoxVerifier(27, true),  // LOAD_FAST pi
        }),
        AITestCase(
        "def f():\n    x = 1\n    y = 2\n    return min(x, y)",
        {
            new BoxVerifier(0, true),  // LOAD_CONST 1
            new BoxVerifier(6, true),  // LOAD_CONST 2
            new BoxVerifier(15, true),  // LOAD_FAST x
            new BoxVerifier(18, true),  // LOAD_FAST y
            new ReturnVerifier(AVK_Any)
        }),
        AITestCase(
        "def f():\n    x = 1\n    return x",
        {
            new BoxVerifier(0, false),  // LOAD_CONST 1
            new BoxVerifier(9, false)
        }),
        AITestCase(
        "def f():\n    x = 1\n    return abs",
        {
            new BoxVerifier(0, false),  // LOAD_CONST 1
            new BoxVerifier(9, true)
        }),

        // Locations are tracked independently...
        AITestCase(
        "def f():\n    if abc:\n         x = 1\n         while x < 100:\n              x += 1\n    else:\n         x = 1\n         y = 2\n         return min(x, y)",
        {
            new BoxVerifier(6, false),  // LOAD_CONST 1
            new BoxVerifier(15, false),  // LOAD_FAST x
            new BoxVerifier(18, false),  // LOAD_CONST 100
            new BoxVerifier(21, false),  // COMPARE_OP <
            new BoxVerifier(27, false),  // LOAD_FAST x
            new BoxVerifier(30, false),  // LOAD_CONST 1
            new BoxVerifier(44, true),  // LOAD_CONST 1
            new BoxVerifier(50, true),  // LOAD_CONST 3
            new BoxVerifier(59, true),  // LOAD_FAST x
            new BoxVerifier(62, true),  // LOAD_FAST y
        }),
        AITestCase(
        "def f():\n    if abc:\n         x = 1\n         while x < 100:\n              x += 1\n    else:\n         x = 1\n         y = 2\n         while x < y:\n            return min(x, y)",
        {
            new BoxVerifier(6, true),  // LOAD_CONST 1
            new BoxVerifier(15, true),  // LOAD_FAST x
            new BoxVerifier(18, true),  // LOAD_CONST 100
            new BoxVerifier(21, true),  // COMPARE_OP <
            new BoxVerifier(27, true),  // LOAD_FAST x
            new BoxVerifier(30, true),  // LOAD_CONST 1
            new BoxVerifier(44, true),  // LOAD_CONST 1
            new BoxVerifier(50, true),  // LOAD_CONST 3
            new BoxVerifier(59, true),  // LOAD_FAST x
            new BoxVerifier(62, true),  // LOAD_FAST y
            new BoxVerifier(65, true),  // COMPARE_OP <
        }),
        // Values flowing across a branch...
        AITestCase(
            "def f():\n    x = 1 if abc else 2",
            {
                new BoxVerifier(6, false),  // LOAD_CONST 1
                new BoxVerifier(12, false),  // LOAD_CONST 2
            }
        ),
        AITestCase(
        "def f():\n    x = 1 if abc else 2.0",
        {
            new BoxVerifier(6, true),  // LOAD_CONST 1
            new BoxVerifier(12, true),  // LOAD_CONST 2
        }),
        // Merging w/ unknown value
        AITestCase(
        "def f():\n    x = g()\n    x = 1",
        {
            new BoxVerifier(6, true),  // STORE_FAST x
            new BoxVerifier(9, false),  // LOAD_CONST 1
            new BoxVerifier(12, false),  // STORE_FAST x
        }),
        // Merging w/ deleted value...
        AITestCase(
        "def f():\n    x = 1\n    del x\n    x = 2",
        {
            new BoxVerifier(0, true),  // LOAD_CONST 1
            new BoxVerifier(9, false),  // LOAD_CONST 2
        }),
        // Distinct assignments of different types...
        AITestCase(
        "def f():\n    x = 1\n    x += 2\n    x = 1.0\n    x += 2.0",
        {
            new BoxVerifier(0, false),  // LOAD_CONST 1
            new BoxVerifier(3, false),  // STORE_FAST x
            new BoxVerifier(6, false),  // LOAD_FAST 2
            new BoxVerifier(9, false),  // LOAD_CONST 2
            new BoxVerifier(12, false),  // INPLACE_ADD
            new BoxVerifier(13, false),  // STORE_FAST x
            new BoxVerifier(16, false),  // LOAD_CONST 1
            new BoxVerifier(19, false),  // STORE_FAST x
            new BoxVerifier(22, false),  // LOAD_FAST 2
            new BoxVerifier(25, false),  // LOAD_CONST 2
            new BoxVerifier(28, false),  // INPLACE_ADD
            new BoxVerifier(29, false),  // STORE_FAST x
        }
        )
    };

    for (auto& testCase : cases) {
        auto codeObj = CompileCode(testCase.m_code);
        printf("Testing %s\r\n", testCase.m_code);

        AbstractInterpreter interpreter(codeObj);
        if (!interpreter.interpret()) {
            _ASSERTE(FALSE && "Failed to interprete code");
        }
        interpreter.dump();

        testCase.verify(interpreter);

        Py_DECREF(codeObj);
    }
}

int main() {
    Py_Initialize();
    JitInit();

    AbsIntTest();

    PyJitTest();

    Py_Finalize();
}