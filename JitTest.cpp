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
	
	TestCase cases[] = {
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
	JitInit();
	PyJitTest();
}