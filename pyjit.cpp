#include "pyjit.h"

// binary operator helpers
#define METHOD_ADD_TOKEN			0x00000000
#define METHOD_MULTIPLY_TOKEN		0x00000001
#define METHOD_SUBTRACT_TOKEN		0x00000002
#define METHOD_DIVIDE_TOKEN			0x00000003
#define METHOD_FLOORDIVIDE_TOKEN	0x00000004
#define METHOD_POWER_TOKEN			0x00000005
#define METHOD_MODULO_TOKEN			0x00000006
#define METHOD_SUBSCR_TOKEN			0x00000007
#define METHOD_STOREMAP_TOKEN		0x00000008
#define METHOD_RICHCMP_TOKEN		0x00000009
#define METHOD_CONTAINS_TOKEN		0x0000000A
#define METHOD_NOTCONTAINS_TOKEN	0x0000000B
#define METHOD_STORESUBSCR_TOKEN	0x0000000C
#define METHOD_DELETESUBSCR_TOKEN	0x0000000D
#define METHOD_NEWFUNCTION_TOKEN	0x0000000E
#define METHOD_GETITER_TOKEN		0x0000000F
#define METHOD_DECREF_TOKEN			0x00000010
#define METHOD_GETBUILDCLASS_TOKEN	0x00000011
#define METHOD_LOADNAME_TOKEN		0x00000012
#define METHOD_STORENAME_TOKEN		0x00000013

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

// signatures for calli methods
#define SIG_ITERNEXT_TOKEN			0x00040000

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
	il.emit_call(METHOD_DECREF_TOKEN);
	//LD_FIELDA(PyObject, ob_refcnt);
	//il.push_back(CEE_DUP);
	//il.push_back(CEE_LDIND_I4);
	//il.push_back(CEE_LDC_I4_1);
	//il.push_back(CEE_SUB);
	////il.push_back(CEE_DUP);
	//// _Py_Dealloc(_py_decref_tmp)

	//il.push_back(CEE_STIND_I4);
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

PyObject* DoRichCompare(PyObject *left, PyObject *right, int op) {
	auto res = PyObject_RichCompare(left, right, op);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoContains(PyObject *left, PyObject *right) {
	auto res = PySequence_Contains(right, left);
	if (res < 0) {
		return nullptr;
	}
	Py_DECREF(left);
	Py_DECREF(right);
	return res ? Py_True : Py_False;
}

PyObject* DoNotContains(PyObject *left, PyObject *right, int op) {
	auto res = PySequence_Contains(right, left);
	if (res < 0) {
		return nullptr;
	}
	Py_DECREF(left);
	Py_DECREF(right);
	return res ? Py_False : Py_True;
}

PyObject* DoNewFunction(PyObject* code, PyObject* qualname, PyFrameObject* frame) {
	auto res = PyFunction_NewWithQualName(code, frame->f_globals, qualname);
	Py_DECREF(code);
	Py_DECREF(qualname);

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
	assert(PyDict_CheckExact(map));
	auto res = PyDict_SetItem(map, key, value);
	Py_DECREF(key);
	Py_DECREF(value);
	return map;
}

void DoStoreSubscr(PyObject* value, PyObject *container, PyObject *index) {
	auto res = PyObject_SetItem(container, index, value);
	Py_DECREF(index);
	Py_DECREF(value);
	Py_DECREF(container);
}

void DoDeleteSubscr(PyObject *container, PyObject *index) {
	auto res = PyObject_DelItem(container, index);
	Py_DECREF(index);
	Py_DECREF(container);
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
	if (v != nullptr) {
		Py_INCREF(v);
	}
	return v;
}

PyObject* DoGetIter(PyObject* iterable) {
	auto res = PyObject_GetIter(iterable);
	Py_DECREF(iterable);
	return res;
}

PyObject* DoIterNext(PyObject* iter ) {
	return (*iter->ob_type->tp_iternext)(iter);
}

PyObject* DoBuildClass(PyFrameObject *f) {
	_Py_IDENTIFIER(__build_class__);

	PyObject *bc;
	if (PyDict_CheckExact(f->f_builtins)) {
		bc = _PyDict_GetItemId(f->f_builtins, &PyId___build_class__);
		if (bc == NULL) {
			PyErr_SetString(PyExc_NameError,
				"__build_class__ not found");
			// TODO: Error handling
			//goto error;
		}
		Py_INCREF(bc);
	}
	else {
		PyObject *build_class_str = _PyUnicode_FromId(&PyId___build_class__);
		// TODO: Error handling
		/*if (build_class_str == NULL)
			break;*/
		bc = PyObject_GetItem(f->f_builtins, build_class_str);
		if (bc == NULL) {
			// TODO Error handling
			//if (PyErr_ExceptionMatches(PyExc_KeyError))
			//	PyErr_SetString(PyExc_NameError,
			//	"__build_class__ not found");
			//goto error;
		}
	}
	return bc;
}

PyObject* LoadAttr(PyObject* owner, PyObject* name) {
	PyObject *res = PyObject_GetAttr(owner, name);
	Py_DECREF(owner);
	return res;
}

PyObject* LoadName(PyFrameObject* f, PyObject* name) {
	PyObject *locals = f->f_locals;
	PyObject *v;
	if (locals == NULL) {
		//PyErr_Format(PyExc_SystemError,
		//	"no locals when loading %R", name);
		//goto error;
	}
	if (PyDict_CheckExact(locals)) {
		v = PyDict_GetItem(locals, name);
		Py_XINCREF(v);
	}
	else {
		v = PyObject_GetItem(locals, name);
		if (v == NULL && _PyErr_OCCURRED()) {
			//if (!PyErr_ExceptionMatches(PyExc_KeyError))
			//	goto error;
			PyErr_Clear();
		}
	}
	if (v == NULL) {
		v = PyDict_GetItem(f->f_globals, name);
		Py_XINCREF(v);
		if (v == NULL) {
			if (PyDict_CheckExact(f->f_builtins)) {
				v = PyDict_GetItem(f->f_builtins, name);
				if (v == NULL) {
					//format_exc_check_arg(
					//	PyExc_NameError,
					//	NAME_ERROR_MSG, name);
					//goto error;
				}
				Py_INCREF(v);
			}
			else {
				v = PyObject_GetItem(f->f_builtins, name);
				if (v == NULL) {
					//if (PyErr_ExceptionMatches(PyExc_KeyError))
					//	format_exc_check_arg(
					//	PyExc_NameError,
					//	NAME_ERROR_MSG, name);
					//goto error;
				}
			}
		}
	}
	return v;
}

void StoreName(PyObject* v, PyFrameObject* f, PyObject* name) {
	PyObject *ns = f->f_locals;
	int err;
	//if (ns == NULL) {
	//	PyErr_Format(PyExc_SystemError,
	//		"no locals found when storing %R", name);
	//	Py_DECREF(v);
	//	goto error;
	//}
	if (PyDict_CheckExact(ns))
		err = PyDict_SetItem(ns, name, v);
	else
		err = PyObject_SetItem(ns, name, v);
	Py_DECREF(v);
	//if (err != 0)
	//	goto error;
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

Label getOffsetLabel(ILGenerator&il, unordered_map<int, Label>& offsetLabels, int jumpTo) {
	auto jumpToLabelIter = offsetLabels.find(jumpTo);
	Label jumpToLabel;
	if (jumpToLabelIter == offsetLabels.end()) {
		offsetLabels[jumpTo] = jumpToLabel = il.define_label();
	}
	else{
		jumpToLabel = jumpToLabelIter->second;
	}
	return jumpToLabel;
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
		else{
			auto label = il.define_label();
			offsetLabels[i] = label;
			il.mark_label(label);
		}

		auto byte = byteCode[i];
		if (HAS_ARG(byte)){
			oparg = NEXTARG();
		}
processOpCode:
		switch (byte) {
		case COMPARE_OP:
		{
			auto compareType = oparg;
			switch (compareType) {
			case PyCmp_IS:
			case PyCmp_IS_NOT:
				// TODO: Missing dec refs here...
			{
				Label same = il.define_label();
				Label done = il.define_label();
				il.branch(BranchEqual, same);
				il.push_ptr(compareType == PyCmp_IS ? Py_False : Py_True);
				il.branch(BranchAlways, done);
				il.mark_label(same);
				il.push_ptr(compareType == PyCmp_IS ? Py_True : Py_False);
				il.mark_label(done);
				il.dup();
				incref(il);
			}
				break;
			case PyCmp_IN:
				il.emit_call(METHOD_CONTAINS_TOKEN);
				break;
			case PyCmp_NOT_IN:
				il.emit_call(METHOD_NOTCONTAINS_TOKEN);
				break;
			case PyCmp_EXC_MATCH:
				_ASSERTE(FALSE);
				break;
			//	if (PyTuple_Check(w)) {
			//		Py_ssize_t i, length;
			//		length = PyTuple_Size(w);
			//		for (i = 0; i < length; i += 1) {
			//			PyObject *exc = PyTuple_GET_ITEM(w, i);
			//			if (!PyExceptionClass_Check(exc)) {
			//				PyErr_SetString(PyExc_TypeError,
			//					CANNOT_CATCH_MSG);
			//				return NULL;
			//			}
			//		}
			//	}
			//	else {
			//		if (!PyExceptionClass_Check(w)) {
			//			PyErr_SetString(PyExc_TypeError,
			//				CANNOT_CATCH_MSG);
			//			return NULL;
			//		}
			//	}
			//	res = PyErr_GivenExceptionMatches(v, w);
			//	break;
			default:
				il.ld_i(oparg);
				il.emit_call(METHOD_RICHCMP_TOKEN);
			}
		}
			break;
		case SETUP_LOOP:
			break;
		case POP_BLOCK:
			break;
		case LOAD_BUILD_CLASS:
			load_frame(il);
			il.emit_call(METHOD_GETBUILDCLASS_TOKEN);
			break;
		case JUMP_ABSOLUTE:
		{
			il.branch(BranchAlways, getOffsetLabel(il, offsetLabels, oparg));
			break;
		}
		case POP_JUMP_IF_FALSE:
		{
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

			il.branch(BranchAlways, getOffsetLabel(il, offsetLabels, oparg));

			il.mark_label(noJump);
			decref(il);
			stackDepth--;
		}
			break;
		case LOAD_NAME:
			load_frame(il);
			il.push_ptr(PyTuple_GetItem(code->co_names, oparg));
			il.emit_call(METHOD_LOADNAME_TOKEN);
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
			load_frame(il);
			{
				auto globalName = PyTuple_GetItem(code->co_names, oparg);
				il.push_ptr(globalName);
			}
			il.emit_call(METHOD_LOADGLOBAL_TOKEN);
			break;
		case LOAD_CONST:
			stackDepth++;
			il.push_ptr(PyTuple_GetItem(code->co_consts, oparg));
			il.dup();
			incref(il);
			break;
		case STORE_NAME:
			load_frame(il);
			il.push_ptr(PyTuple_GetItem(code->co_names, oparg));
			il.emit_call(METHOD_STORENAME_TOKEN);
			break;
		case STORE_FAST:
			// TODO: Move locals out of the Python frame object and into real locals
		{
			stackDepth--;
			auto valueTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(valueTmp);

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
			load_local(il, oparg);

			//// TODO: Remove this check for definitely assigned values (e.g. params w/ no dels, 
			//// locals that are provably assigned)
			///*if (value == NULL) {*/
			//il.dup();
			//il.load_null();
			//il.push_back(CEE_PREFIX1);
			//il.push_back((unsigned char)CEE_CEQ);
			//ok = il.define_label();			
			//il.branch(BranchFalse, ok);
			//il.pop();
			//for (int cnt = 0; cnt < stackDepth; cnt++) {
			//	il.pop();
			//}
			//il.push_back(CEE_LDNULL);
			//il.push_back(CEE_THROW);
			//il.mark_label(ok);

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
			int argCnt = oparg & 0xff;
			int kwArgCnt = (oparg >> 8) & 0xff;
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
					build_tuple(il, argCnt);

					// target is on the stack already...
					il.emit_call(METHOD_CALLN_TOKEN);

					break;
				}
			}
		}
			break;
		case BUILD_TUPLE:
		{
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
			build_list(il, oparg);
			stackDepth -= oparg;
			stackDepth++;
		}
			break;
		case BUILD_MAP:
			build_map(il, oparg);
			stackDepth++;
			break;
		case STORE_MAP:
			// stack is map, key, value
			il.emit_call(METHOD_STOREMAP_TOKEN);
			stackDepth -= 2;	// map stays on the stack
			break;	
		case STORE_SUBSCR:
			// stack is value, container, index
			// TODO: Error check
			il.emit_call(METHOD_STORESUBSCR_TOKEN);
			stackDepth -= 3;	
			break;			
		case DELETE_SUBSCR:
			// stack is container, index
			// TODO: Error check
			il.emit_call(METHOD_DELETESUBSCR_TOKEN);
			stackDepth -= 2;	
			break;
		case BUILD_SET:
		{
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
		case EXTENDED_ARG:
		{
			byte = byteCode[i + 1];
			auto bottomArg = NEXTARG();
			oparg = (oparg << 16) | bottomArg;
			goto processOpCode;
		}
		case MAKE_FUNCTION:
		{
			int posdefaults = oparg & 0xff;
			int kwdefaults = (oparg >> 8) & 0xff;
			int num_annotations = (oparg >> 16) & 0x7fff;

			load_frame(il);
			il.emit_call(METHOD_NEWFUNCTION_TOKEN);
			if (num_annotations > 0) {
				_ASSERTE(FALSE);
			}
			if (kwdefaults > 0) {
				_ASSERTE(FALSE);
			}
			if (posdefaults > 0) {
				_ASSERTE(FALSE);
			}
			break;
		}
		case GET_ITER:
			il.emit_call(METHOD_GETITER_TOKEN);
			break;
		case FOR_ITER:
		{
			// oparg is where to jump on break
			il.dup();	// keep value on stack in non-error/exit case
			/*il.dup();	
			LD_FIELD(PyObject, ob_type);
			LD_FIELD(PyTypeObject, tp_iternext);*/
			//il.push_ptr((void*)offsetof(PyObject, ob_type));
			//il.push_back(CEE_ADD);
			il.emit_call(SIG_ITERNEXT_TOKEN);
			auto processValue = il.define_label();
			il.dup();
			il.push_ptr(nullptr);
			il.compare_eq();
			il.branch(BranchFalse, processValue);
			// iteration has ended, or an exception was raised...
			// TODO: Error check besides stop iteration...

			il.pop();
			decref(il);
			il.branch(BranchAlways, getOffsetLabel(il, offsetLabels, i + oparg + 1));

			// leave iter and value on stack
			il.mark_label(processValue);
			stackDepth++;
		}
			break;
		case LIST_APPEND:
		default:
			_ASSERT(FALSE);
			break;
		}
	}

	return il.compile(&g_corJitInfo, g_jit).m_addr;
}

VTableInfo g_iterNextVtable{ 2, { offsetof(PyObject, ob_type), offsetof(PyTypeObject, tp_iternext) } };

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
	g_module->m_methods[METHOD_STORESUBSCR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoStoreSubscr
		);
	g_module->m_methods[METHOD_DELETESUBSCR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoDeleteSubscr
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
	
	g_module->m_methods[METHOD_RICHCMP_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_INT) },
		&DoRichCompare
	);
	g_module->m_methods[METHOD_CONTAINS_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoContains
		);
	g_module->m_methods[METHOD_NOTCONTAINS_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoNotContains
	);

	g_module->m_methods[METHOD_NEWFUNCTION_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoNewFunction
		);

	g_module->m_methods[METHOD_GETBUILDCLASS_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoBuildClass
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
	g_module->m_methods[METHOD_LOADNAME_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&LoadName
	);
	g_module->m_methods[METHOD_STORENAME_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT)  },
		&StoreName
	);
	g_module->m_methods[METHOD_GETITER_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoGetIter
		);
	g_module->m_methods[SIG_ITERNEXT_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoIterNext
		);
	g_module->m_methods[METHOD_DECREF_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT) },
		&Py_DecRef
	);
	//g_module->m_methods[SIG_ITERNEXT_TOKEN] = Method(
	//	nullptr,
	//	CORINFO_TYPE_NATIVEINT,
	//	std::vector < Parameter > { },
	//	nullptr,
	//	&g_iterNextVtable
	//);
}

