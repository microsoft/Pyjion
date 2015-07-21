#define FEATURE_NO_HOST
#define USE_STL
#include <stdint.h>
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <float.h>
#include <share.h>
#include <cstdlib>
#include <intrin.h>

#include <vector>
#include <hash_map>

#include <corjit.h>
#include <utilcode.h>
#include <openum.h>

#include "cee.h"
#include "jitinfo.h"
#include "codemodel.h"
#include "pyjit.h"

using namespace std;

PyCodeObject* CompileCode(const char* code) {
	auto globals = PyDict_New();
	auto locals = PyDict_New();
	PyRun_String(code, Py_file_input, globals, locals);
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
		TestCase(	// hits JUMP_FORWARD
			"def f(a):\n    if a:\n        x = 1\n    else:\n        x = 2\n    return x",
			vector<TestInput>({
				TestInput("1", vector<PyObject*>({ PyLong_FromLong(42) })),
				TestInput("2", vector<PyObject*>({ PyLong_FromLong(0) })),
				TestInput("1", vector<PyObject*>({ Incremented(Py_True) })),
				TestInput("2", vector<PyObject*>({ Incremented(Py_False) }))
			})
		),
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
			})
		),
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
			})
		),
		TestCase(
			"def f(a, b):\n    a %= b\n    return a",
			TestInput("1", vector<PyObject*>({ PyLong_FromLong(3), PyLong_FromLong(2) }))
		),
		TestCase(
			"def f(a, b):\n    a += b\n    return a",
			vector<TestInput>({
				TestInput("9", vector<PyObject*>({ PyLong_FromLong(6), PyLong_FromLong(3) })),
				TestInput("'hibye'", vector<PyObject*>({ PyUnicode_FromString("hi"), PyUnicode_FromString("bye") }))
			})
		),
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
			})
		),
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
			})	
		),
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
			})
		),
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
			TestInput("25", vector<PyObject*>({PyLong_FromLong(75), PyLong_FromLong(50)}))
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

	for (int i = 0; i < COUNTOF(cases); i++) {
		auto curCase = cases[i];

		auto codeObj = CompileCode(curCase.m_code);
		auto addr = JitCompile(codeObj);

		for (auto curInput = 0; curInput < curCase.m_inputs.size(); curInput++) {
			auto input = curCase.m_inputs[curInput];

			auto globals = PyDict_New();
			auto builtins = PyThreadState_GET()->interp->builtins;
			PyDict_SetItemString(globals, "__builtins__", builtins);

			auto frame = PyFrame_New(PyThreadState_Get(), codeObj, globals, PyDict_New());
			for (size_t arg = 0; arg < input.m_args.size(); arg++) {
				frame->f_localsplus[arg] = input.m_args[arg];
			}
			
			auto res = ((evalFunc)addr)(frame);

			auto repr = PyUnicode_AsUTF8(PyObject_Repr(res));
			if (strcmp(input.m_expected, repr) != 0) {
				printf("Unexpected: %s\r\n", repr);
				_ASSERT(FALSE);
			}

			//Py_DECREF(frame);
			Py_DECREF(res);
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