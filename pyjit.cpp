#include "pyjit.h"

// binary operator helpers
#define METHOD_ADD_TOKEN		0x00000000
#define METHOD_MULTIPLY_TOKEN	0x00000001
#define METHOD_SUBTRACT_TOKEN	0x00000002
#define METHOD_DIVIDE_TOKEN		0x00000003
#define METHOD_FLOORDIVIDE_TOKEN		0x00000004
#define METHOD_POWER_TOKEN		0x00000005
#define METHOD_MODULO_TOKEN		0x00000006
#define METHOD_SUBSCR_TOKEN		0x00000007
#define METHOD_STOREMAP_TOKEN	0x00000008

// call helpers
#define METHOD_CALL0_TOKEN		0x00010000
#define METHOD_CALL1_TOKEN		0x00010001
#define METHOD_CALL2_TOKEN		0x00010002
#define METHOD_CALL3_TOKEN		0x00010003
#define METHOD_CALL4_TOKEN		0x00010004
#define METHOD_CALL5_TOKEN		0x00010005
#define METHOD_CALL6_TOKEN		0x00010006
#define METHOD_CALL7_TOKEN		0x00010007
#define METHOD_CALL8_TOKEN		0x00010008
#define METHOD_CALL9_TOKEN		0x00010009
#define METHOD_CALLN_TOKEN		0x00010100

// Py* helpers
#define METHOD_PYTUPLE_NEW			0x00020000
#define METHOD_PYLIST_NEW			0x00020001
#define METHOD_PYDICT_NEWPRESIZED	0x00020002
#define METHOD_PYSET_NEW			0x00020003
#define METHOD_PYSET_ADD			0x00020004
#define METHOD_PYOBJECT_ISTRUE		0x00020005


// Misc helpers
#define METHOD_LOADGLOBAL_TOKEN		0x00030000
#define METHOD_LOADATTR_TOKEN		0x00030001

Module *g_module;
CorJitInfo g_corJitInfo;
ICorJitCompiler* g_jit;

void load_frame(ILGenerator &il) {
	il.push_back(CEE_LDARG_0);
}

#define LD_FIELDA(type, field) il.ld_i(offsetof(type, field)); il.push_back(CEE_ADD); 
#define LD_FIELD(type, field) il.ld_i(offsetof(type, field)); il.push_back(CEE_ADD); il.push_back(CEE_LDIND_I);
#define ST_FIELD(type, field) il.ld_i(offsetof(type, field)); il.push_back(CEE_ADD); il.push_back(CEE_STIND_I);

void load_local(ILGenerator& il, int oparg) {
	load_frame(il);
	il.ld_i(offsetof(PyFrameObject, f_localsplus) + oparg * sizeof(size_t));
	il.push_back(CEE_ADD);
	il.push_back(CEE_LDIND_I);
}

void incref(ILGenerator&il) {
	LD_FIELDA(PyObject, ob_refcnt);
	il.push_back(CEE_DUP);
	il.push_back(CEE_LDIND_I4);
	il.push_back(CEE_LDC_I4_1);
	il.push_back(CEE_ADD);
	il.push_back(CEE_STIND_I4);
}

void decref(ILGenerator&il) {
	LD_FIELDA(PyObject, ob_refcnt);
	il.push_back(CEE_DUP);
	il.push_back(CEE_LDIND_I4);
	il.push_back(CEE_LDC_I4_1);
	il.push_back(CEE_SUB);
	//il.push_back(CEE_DUP);
	// _Py_Dealloc(_py_decref_tmp)

	il.push_back(CEE_STIND_I4);

}

PyObject* DoAdd(PyObject *left, PyObject *right) {
	// TODO: Verify ref counting...
	PyObject *sum;
	if (PyUnicode_CheckExact(left) && PyUnicode_CheckExact(right)) {
		sum = PyUnicode_Concat(left, right);
	}
	else {
		sum = PyNumber_Add(left, right);
		Py_DECREF(left);
	}
	return sum;
}

PyObject* DoSubscr(PyObject *left, PyObject *right) {
	auto res = PyObject_GetItem(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoMultiply(PyObject *left, PyObject *right) {
	auto res = PyNumber_Multiply(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoTrueDivide(PyObject *left, PyObject *right) {
	auto res = PyNumber_TrueDivide(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoFloorDivide(PyObject *left, PyObject *right) {
	auto res = PyNumber_FloorDivide(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoPower(PyObject *left, PyObject *right) {
	auto res = PyNumber_Power(left, right, Py_None);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoModulo(PyObject *left, PyObject *right) {
	auto res = PyUnicode_CheckExact(left) ?
		PyUnicode_Format(left, right) :
		PyNumber_Remainder(left, right); 
	
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoSubtract(PyObject *left, PyObject *right) {
	auto res = PyNumber_Subtract(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoStoreMap(PyObject* map, PyObject *value, PyObject *key) {
	auto res = PyObject_SetItem(map, key, value);
	Py_DECREF(key);
	Py_DECREF(value);
	return map;
}

PyObject* CallN(PyObject *target, PyObject* args) {
	// we stole references for the tuple...
	auto res = PyObject_Call(target, args, nullptr);
	Py_DECREF(target);
	return res;
}

PyObject* LoadGlobal(PyFrameObject* f, PyObject* name) {
	PyObject* v;
	//if (PyDict_CheckExact(f->f_globals)
	//	&& PyDict_CheckExact(f->f_builtins)) {
	//	v = _PyDict_LoadGlobal((PyDictObject *)f->f_globals,
	//		(PyDictObject *)f->f_builtins,
	//		name);
	//	if (v == NULL) {
	//		//if (!_PyErr_OCCURRED())
	//		//	format_exc_check_arg(PyExc_NameError, NAME_ERROR_MSG, name);
	//		//goto error;
	//	}
	//	Py_INCREF(v);
	//}
	//else 
	{
		/* Slow-path if globals or builtins is not a dict */
		auto asciiName = PyUnicode_AsUTF8(name);
		v = PyObject_GetItem(f->f_globals, name);
		if (v == NULL) {

			v = PyObject_GetItem(f->f_builtins, name);
			if (v == NULL) {
				//if (PyErr_ExceptionMatches(PyExc_KeyError))
				//	format_exc_check_arg(
				//	PyExc_NameError,
				//	NAME_ERROR_MSG, name);
				//goto error;
			}
			else{
				PyErr_Clear();
			}
		}
	}
	return v;
}

PyObject* LoadAttr(PyObject* owner, PyObject* name) {
	PyObject *res = PyObject_GetAttr(owner, name);
	Py_DECREF(owner);
	return res;
}

PyObject* g_emptyTuple;

PyObject* Call0(PyObject *target) {	
	auto res = PyObject_Call(target, g_emptyTuple, nullptr);
	Py_DECREF(target);
	return res;
}

PyObject* Call1(PyObject *target, PyObject* arg1) {
	return nullptr;
}

void build_tuple(ILGenerator& il, int argCnt) {
	auto valueTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
	auto tupleTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));

	il.push_ptr((void*)argCnt);
	il.emit_call(METHOD_PYTUPLE_NEW);
	il.st_loc(tupleTmp);

	for (int arg = argCnt - 1; arg >= 0; arg--) {
		// save the argument into a temporary...
		il.st_loc(valueTmp);

		// load the address of the tuple item...
		il.ld_loc(tupleTmp);
		il.ld_i(arg * sizeof(size_t) + offsetof(PyTupleObject, ob_item));
		il.push_back(CEE_ADD);

		// reload the value
		il.ld_loc(valueTmp);

		// store into the array
		il.push_back(CEE_STIND_I);
	}
	il.ld_loc(tupleTmp);

	il.free_local(valueTmp);	
	il.free_local(tupleTmp);
}

void build_list(ILGenerator& il, int argCnt) {

	il.push_ptr((void*)argCnt);
	il.emit_call(METHOD_PYLIST_NEW);

	if (argCnt != 0) {
		auto valueTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
		auto listTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
		auto listItems = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));

		il.dup();
		il.st_loc(listTmp);

		// load the address of the tuple item...
		il.ld_i(offsetof(PyListObject, ob_item));
		il.push_back(CEE_ADD);
		il.push_back(CEE_LDIND_I);

		il.st_loc(listItems);

		for (int arg = argCnt - 1; arg >= 0; arg--) {
			// save the argument into a temporary...
			il.st_loc(valueTmp);

			// load the address of the tuple item...
			il.ld_loc(listItems);
			il.ld_i(arg * sizeof(size_t));
			il.push_back(CEE_ADD);

			// reload the value
			il.ld_loc(valueTmp);

			// store into the array
			il.push_back(CEE_STIND_I);
		}

		// update the size of the list...
		il.ld_loc(listTmp);
		il.dup();
		il.ld_i(offsetof(PyVarObject, ob_size));
		il.push_back(CEE_ADD);
		il.push_ptr((void*)argCnt);
		il.push_back(CEE_STIND_I);

		il.free_local(valueTmp);
		il.free_local(listTmp);
		il.free_local(listItems);
	}

}

void build_set(ILGenerator& il, int argCnt) {	
	il.load_null();
	il.emit_call(METHOD_PYSET_NEW);

	if (argCnt != 0) {
		auto valueTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
		auto setTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
		
		il.st_loc(setTmp);

		for (int arg = argCnt - 1; arg >= 0; arg--) {
			// save the argument into a temporary...
			il.st_loc(valueTmp);

			// load the address of the tuple item...
			il.ld_loc(setTmp);
			il.ld_loc(valueTmp);
			il.emit_call(METHOD_PYSET_ADD);
			il.pop();
		}

		il.ld_loc(setTmp);
		il.free_local(valueTmp);
		il.free_local(setTmp);
	}
}

void build_map(ILGenerator& il, int argCnt) {
	il.push_ptr((void*)argCnt);
	il.emit_call(METHOD_PYDICT_NEWPRESIZED);
}

#define NEXTARG() oparg = *(unsigned short*)&byteCode[i + 1]; i+= 2
extern "C" __declspec(dllexport) PVOID JitCompile(PyCodeObject* code) {
	if (g_emptyTuple == nullptr) {
		g_emptyTuple = PyTuple_New(0);
	}

	ILGenerator il(g_module, CORINFO_TYPE_NATIVEINT, std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) });
	unsigned char *byteCode = (unsigned char *)((PyBytesObject*)code->co_code)->ob_sval;
	auto size = PyBytes_Size(code->co_code);
	Label ok;
	unordered_map<int, Label> offsetLabels;
	int oparg;
	int stackDepth = 0;
	for (int i = 0; i < size; i++) {
		auto existingLabel = offsetLabels.find(i);
		if (existingLabel != offsetLabels.end()) {
			il.mark_label(existingLabel->second);
		}

		auto byte = byteCode[i];
		switch (byte) {
		case POP_JUMP_IF_FALSE:
		{
			auto jumpTo = NEXTARG();
			auto noJump = il.define_label();
			auto willJump = il.define_label();
			// fast checks for true/false...
			il.dup();
			il.push_ptr(Py_True);
			il.compare_eq();			
			il.branch(BranchTrue, noJump);

			il.dup();
			il.push_ptr(Py_False);
			il.compare_eq();
			il.branch(BranchTrue, willJump);

			// Use PyObject_IsTrue
			il.dup();
			il.emit_call(METHOD_PYOBJECT_ISTRUE);
			il.ld_i(0);
			il.compare_eq();
			il.branch(BranchFalse, noJump);

			il.mark_label(willJump);
			decref(il);

			auto jumpToLabelIter = offsetLabels.find(jumpTo);
			Label jumpToLabel;
			if (jumpToLabelIter == offsetLabels.end()) {
				offsetLabels[jumpTo] = jumpToLabel = il.define_label();
			}
			else{
				jumpToLabel = jumpToLabelIter->second;
			}
			il.branch(BranchAlways, jumpToLabel);

			il.mark_label(noJump);
			decref(il);
		}
			break;
		case LOAD_ATTR:
			{
				auto globalName = PyTuple_GetItem(code->co_names, oparg);
				il.push_ptr(globalName);
			}
			il.emit_call(METHOD_LOADATTR_TOKEN);
			break;
		case LOAD_GLOBAL:
			stackDepth++;
			oparg = NEXTARG();
			load_frame(il);
			{
				auto globalName = PyTuple_GetItem(code->co_names, oparg);
				il.push_ptr(globalName);
			}
			il.emit_call(METHOD_LOADGLOBAL_TOKEN);
			break;
		case LOAD_CONST:
			stackDepth++;
			oparg = NEXTARG();
			il.push_ptr(PyTuple_GetItem(code->co_consts, oparg));
			break;
		case STORE_FAST:
			// TODO: Move locals out of the Python frame object and into real locals
		{
			stackDepth--;
			auto valueTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(valueTmp);

			oparg = NEXTARG();
			load_frame(il);
			il.ld_i(offsetof(PyFrameObject, f_localsplus) + oparg * sizeof(size_t));
			il.push_back(CEE_ADD);

			il.ld_loc(valueTmp);

			il.push_back(CEE_STIND_I);

			il.free_local(valueTmp);
		}
			break;
		case LOAD_FAST:
			/* PyObject *value = GETLOCAL(oparg); */
			oparg = NEXTARG();
			load_local(il, oparg);
			il.dup();

			/*if (value == NULL) {*/
			il.load_null();
			il.push_back(CEE_PREFIX1);
			il.push_back((unsigned char)CEE_CEQ);
			ok = il.define_label();			
			il.branch(BranchFalse, ok);
			il.pop();
			for (int cnt = 0; cnt < stackDepth; cnt++) {
				il.pop();
			}
			il.push_back(CEE_LDNULL);
			il.push_back(CEE_THROW);
			il.mark_label(ok);

			/* TODO: Implement this, update branch above...
			format_exc_check_arg(PyExc_UnboundLocalError,
			UNBOUNDLOCAL_ERROR_MSG,
			PyTuple_GetItem(co->co_varnames, oparg));
			goto error;
			}*/

			il.dup();
			incref(il);
			stackDepth++;
			break;
		case CALL_FUNCTION:
		{
			int argCnt = byteCode[i + 1];
			int kwArgCnt = byteCode[i + 2];
			i += 2;
			// Optimize for # of calls, and various call types...
			// Function is last thing on the stack...
			
			// target + args popped, result pushed
			stackDepth -= argCnt;

			if (kwArgCnt == 0) {
				switch (argCnt) {
				case 0: il.emit_call(METHOD_CALL0_TOKEN); break;
					/*
					case 1: emit_call(il, METHOD_CALL1_TOKEN); break;
					case 2: emit_call(il, METHOD_CALL2_TOKEN); break;
					case 3: emit_call(il, METHOD_CALL3_TOKEN); break;
					case 4: emit_call(il, METHOD_CALL4_TOKEN); break;
					case 5: emit_call(il, METHOD_CALL5_TOKEN); break;
					case 6: emit_call(il, METHOD_CALL6_TOKEN); break;
					case 7: emit_call(il, METHOD_CALL7_TOKEN); break;
					case 8: emit_call(il, METHOD_CALL8_TOKEN); break;
					case 9: emit_call(il, METHOD_CALL9_TOKEN); break;*/
				default:
					// generic call, build a tuple for the call...
					build_tuple(il, oparg);

					// target is on the stack already...
					il.emit_call(METHOD_CALLN_TOKEN);

					break;
				}
			}
		}
			break;
		case BUILD_TUPLE:
		{
			oparg = NEXTARG();
			if (oparg == 0) {
				il.push_ptr(PyTuple_New(0));
				stackDepth++;
			}
			else{
				build_tuple(il, oparg);
			}
			stackDepth -= oparg;
			stackDepth++;
		}
			break;
		case BUILD_LIST:
		{
			oparg = NEXTARG();
			build_list(il, oparg);
			stackDepth -= oparg;
			stackDepth++;
		}
			break;
		case BUILD_MAP:
			oparg = NEXTARG();
			build_map(il, oparg);
			stackDepth++;
			break;
		case STORE_MAP:
			// stack is map, key, value
			il.emit_call(METHOD_STOREMAP_TOKEN);
			stackDepth -= 2;	// map stays on the stack
			break;	
		case BUILD_SET:
		{
			oparg = NEXTARG();
			build_set(il, oparg);
			stackDepth -= oparg;
			stackDepth++;
		}
			break;
		case BINARY_SUBSCR:
			stackDepth -= 2;
			il.emit_call(METHOD_SUBSCR_TOKEN);
			break;
		case BINARY_ADD:
			stackDepth -= 2;
			il.emit_call(METHOD_ADD_TOKEN);
			break;
		case BINARY_TRUE_DIVIDE:
			stackDepth -= 2;
			il.emit_call(METHOD_DIVIDE_TOKEN);
			break;
		case BINARY_FLOOR_DIVIDE:
			stackDepth -= 2;
			il.emit_call(METHOD_FLOORDIVIDE_TOKEN);
			break;
		case BINARY_POWER:
			stackDepth -= 2;
			il.emit_call(METHOD_POWER_TOKEN);
			break;
		case BINARY_MODULO:
			stackDepth -= 2;
			il.emit_call(METHOD_MODULO_TOKEN);
			break;			
		case BINARY_MULTIPLY:
			/*
			PyObject *right = POP();
			PyObject *left = TOP();
			PyObject *res = PyNumber_Multiply(left, right);
			Py_DECREF(left);
			Py_DECREF(right);
			SET_TOP(res);
			if (res == NULL)
			goto error;
			break;
			*/
			stackDepth -= 2;
			il.emit_call(METHOD_MULTIPLY_TOKEN);
			break;
		case BINARY_SUBTRACT:
			/*
			PyObject *right = POP();
			PyObject *left = TOP();
			PyObject *res = PyNumber_Subtract(left, right);
			Py_DECREF(left);
			Py_DECREF(right);
			SET_TOP(res);
			if (res == NULL)
			goto error;
			break;
			*/
			stackDepth -= 2;
			il.emit_call(METHOD_SUBTRACT_TOKEN);
			break;
		case RETURN_VALUE:
			stackDepth -= 1;
			il.push_back(CEE_RET);
			break;
		}
	}

	return il.compile(&g_corJitInfo, g_jit).m_addr;
}

extern "C" __declspec(dllexport) void JitInit() {
	CeeInit();

	g_jit = getJit();

	g_module = new Module();
	g_module->m_methods[METHOD_ADD_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoAdd
	);
	g_module->m_methods[METHOD_SUBSCR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoSubscr
	);
	g_module->m_methods[METHOD_MULTIPLY_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoMultiply
		);
	g_module->m_methods[METHOD_DIVIDE_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoTrueDivide
		);
	g_module->m_methods[METHOD_FLOORDIVIDE_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoFloorDivide
		);
	g_module->m_methods[METHOD_POWER_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoPower
		);
	g_module->m_methods[METHOD_MODULO_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoModulo
		);
	g_module->m_methods[METHOD_SUBTRACT_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoSubtract
		);
	
	g_module->m_methods[METHOD_PYLIST_NEW] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&PyList_New
		);
	
	g_module->m_methods[METHOD_STOREMAP_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoStoreMap
	);
	g_module->m_methods[METHOD_PYDICT_NEWPRESIZED] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&_PyDict_NewPresized
		);
	g_module->m_methods[METHOD_PYTUPLE_NEW] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&PyTuple_New
		);
	g_module->m_methods[METHOD_PYSET_NEW] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&PySet_New
	);
	g_module->m_methods[METHOD_PYOBJECT_ISTRUE] = Method(
		nullptr,
		CORINFO_TYPE_INT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&PyObject_IsTrue
	);
	g_module->m_methods[METHOD_PYSET_ADD] = Method(
		nullptr,
		CORINFO_TYPE_INT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&PySet_Add
		);
	g_module->m_methods[METHOD_CALL0_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&Call0
		);
	g_module->m_methods[METHOD_CALLN_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&CallN
		);

	g_module->m_methods[METHOD_LOADGLOBAL_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&LoadGlobal
	);
	g_module->m_methods[METHOD_LOADATTR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&LoadAttr
	);
}

