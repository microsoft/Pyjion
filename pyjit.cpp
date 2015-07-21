#include "pyjit.h"

// binary operator helpers
#define METHOD_ADD_TOKEN					0x00000000
#define METHOD_MULTIPLY_TOKEN				0x00000001
#define METHOD_SUBTRACT_TOKEN				0x00000002
#define METHOD_DIVIDE_TOKEN					0x00000003
#define METHOD_FLOORDIVIDE_TOKEN			0x00000004
#define METHOD_POWER_TOKEN					0x00000005
#define METHOD_MODULO_TOKEN					0x00000006
#define METHOD_SUBSCR_TOKEN					0x00000007
#define METHOD_STOREMAP_TOKEN				0x00000008
#define METHOD_RICHCMP_TOKEN				0x00000009
#define METHOD_CONTAINS_TOKEN				0x0000000A
#define METHOD_NOTCONTAINS_TOKEN			0x0000000B
#define METHOD_STORESUBSCR_TOKEN			0x0000000C
#define METHOD_DELETESUBSCR_TOKEN			0x0000000D
#define METHOD_NEWFUNCTION_TOKEN			0x0000000E
#define METHOD_GETITER_TOKEN				0x0000000F
#define METHOD_DECREF_TOKEN					0x00000010
#define METHOD_GETBUILDCLASS_TOKEN			0x00000011
#define METHOD_LOADNAME_TOKEN				0x00000012
#define METHOD_STORENAME_TOKEN				0x00000013
#define METHOD_UNPACK_SEQUENCE_TOKEN		0x00000014
#define METHOD_UNPACK_SEQUENCEEX_TOKEN		0x00000015
#define METHOD_DELETENAME_TOKEN				0x00000016
#define METHOD_PYCELL_SET_TOKEN				0x00000017
#define METHOD_SET_CLOSURE					0x00000018
#define METHOD_BUILD_SLICE					0x00000019
#define METHOD_UNARY_POSITIVE				0x0000001A
#define METHOD_UNARY_NEGATIVE				0x0000001B
#define METHOD_UNARY_NOT					0x0000001C
#define METHOD_UNARY_INVERT					0x0000001D
#define METHOD_MATRIX_MULTIPLY_TOKEN		0x0000001E
#define METHOD_BINARY_LSHIFT_TOKEN			0x0000001F
#define METHOD_BINARY_RSHIFT_TOKEN			0x00000020
#define METHOD_BINARY_AND_TOKEN				0x00000021
#define METHOD_BINARY_XOR_TOKEN				0x00000022
#define METHOD_BINARY_OR_TOKEN				0x00000023
#define METHOD_LIST_APPEND_TOKEN			0x00000024
#define METHOD_SET_ADD_TOKEN				0x00000025
#define METHOD_INPLACE_POWER_TOKEN			0x00000026
#define METHOD_INPLACE_MULTIPLY_TOKEN		0x00000027
#define METHOD_INPLACE_MATRIX_MULTIPLY_TOKEN 0x00000028
#define METHOD_INPLACE_TRUE_DIVIDE_TOKEN	0x00000029
#define METHOD_INPLACE_FLOOR_DIVIDE_TOKEN	0x0000002A
#define METHOD_INPLACE_MODULO_TOKEN			0x0000002B
#define METHOD_INPLACE_ADD_TOKEN			0x0000002C
#define METHOD_INPLACE_SUBTRACT_TOKEN		0x0000002D
#define METHOD_INPLACE_LSHIFT_TOKEN			0x0000002E
#define METHOD_INPLACE_RSHIFT_TOKEN			0x0000002F
#define METHOD_INPLACE_AND_TOKEN			0x00000030
#define METHOD_INPLACE_XOR_TOKEN			0x00000031
#define METHOD_INPLACE_OR_TOKEN				0x00000032
#define METHOD_MAP_ADD_TOKEN				0x00000033
#define METHOD_PRINT_EXPR_TOKEN				0x00000034
#define METHOD_LOAD_CLASSDEREF_TOKEN		0x00000035

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
#define METHOD_PYITER_NEXT			0x00020006
#define METHOD_PYCELL_GET			0x00020007

// Misc helpers
#define METHOD_LOADGLOBAL_TOKEN		0x00030000
#define METHOD_LOADATTR_TOKEN		0x00030001
#define METHOD_STOREATTR_TOKEN		0x00030002
#define METHOD_DELETEATTR_TOKEN		0x00030003
#define METHOD_STOREGLOBAL_TOKEN	0x00030004
#define METHOD_DELETEGLOBAL_TOKEN	0x00030005

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
		PyUnicode_Append(&left, right);
		sum = left;
	}
	else {
		sum = PyNumber_Add(left, right);
		Py_DECREF(left);
	}
	Py_DECREF(right);
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

PyObject* DoSetClosure(PyObject* closure, PyObject* func) {
	PyFunction_SetClosure(func, closure);
	Py_DECREF(closure);
	return func;
}

PyObject* DoBuildSlice(PyObject* start, PyObject* stop, PyObject* step) {
	auto slice = PySlice_New(start, stop, step);
	Py_DECREF(start);
	Py_DECREF(stop);
	Py_XDECREF(step);
	return slice;
}

PyObject* DoUnaryPositive(PyObject* value) {
	auto res = PyNumber_Positive(value);
	Py_DECREF(value);
	return res;
}

PyObject* DoUnaryNegative(PyObject* value) {
	auto res = PyNumber_Negative(value);
	Py_DECREF(value);
	return res;
}

PyObject* DoUnaryNot(PyObject* value) {
	int err = PyObject_IsTrue(value);
	Py_DECREF(value);
	if (err == 0) {
		Py_INCREF(Py_True);
		return Py_True;
	}
	else if (err > 0) {
		Py_INCREF(Py_False);
		return Py_False;
	}
	// TODO: Error handling?
	return nullptr;
}

PyObject* DoUnaryInvert(PyObject* value) {
	auto res = PyNumber_Invert(value);
	Py_DECREF(value);
	return res;
}

void DoListAppend(PyObject* list, PyObject* value) {
	int err = PyList_Append(list, value);
	Py_DECREF(value);
	if (err != 0) {
		// TODO: Error handling
	}
}

void DoSetAdd(PyObject* set, PyObject* value) {
	int err = PySet_Add(set, value);
	Py_DECREF(value);
	if (err != 0) {
		// TODO: Error handling
	}
}

void DoMapAdd(PyObject*map, PyObject*key, PyObject* value) {
	PyDict_SetItem(map, key, value);  /* v[w] = u */
	Py_DECREF(value);
	Py_DECREF(key);
	// TODO: Error handling
	//if (err != 0)
	//	goto error;
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

PyObject* DoMatrixMultiply(PyObject *left, PyObject *right) {
	auto res = PyNumber_MatrixMultiply(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoBinaryLShift(PyObject *left, PyObject *right) {
	auto res = PyNumber_Lshift(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoBinaryRShift(PyObject *left, PyObject *right) {
	auto res = PyNumber_Rshift(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoBinaryAnd(PyObject *left, PyObject *right) {
	auto res = PyNumber_And(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoBinaryXor(PyObject *left, PyObject *right) {
	auto res = PyNumber_Xor(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

PyObject* DoBinaryOr(PyObject *left, PyObject *right) {
	auto res = PyNumber_Or(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}


PyObject* DoInplacePower(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlacePower(left, right, Py_None);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceMultiply(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceMultiply(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceMatrixMultiply(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceMatrixMultiply(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceTrueDivide(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceTrueDivide(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceFloorDivide(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceFloorDivide(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceModulo(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceRemainder(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceAdd(PyObject *left, PyObject *right) {
	PyObject* res;
	if (PyUnicode_CheckExact(left) && PyUnicode_CheckExact(right)) {
		PyUnicode_Append(&left, right);
		res = left;
	}
	else{
		res = PyNumber_InPlaceAdd(left, right);
		Py_DECREF(left);
	}
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceSubtract(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceSubtract(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceLShift(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceLshift(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceRShift(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceRshift(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceAnd(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceAnd(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceXor(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceXor(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}
PyObject* DoInplaceOr(PyObject *left, PyObject *right) {
	auto res = PyNumber_InPlaceOr(left, right);
	Py_DECREF(left);
	Py_DECREF(right);
	return res;
}

void DoPrintExpr(PyObject *value) {
	PyObject *hook = _PySys_GetObjectId(&PyId_displayhook);
	PyObject *res;
	if (hook == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
			"lost sys.displayhook");
		Py_DECREF(value);
		/*goto error;*/ // TODO: Error
	}
	res = PyObject_CallFunctionObjArgs(hook, value, NULL);
	Py_DECREF(value);
	//if (res == NULL)	// TODO: Error
	//	goto error;
	Py_DECREF(res);
}

PyObject* DoLoadClassDeref(PyFrameObject* frame, size_t oparg) {
	PyObject* value;
	PyCodeObject* co = frame->f_code;
	auto idx = oparg - PyTuple_GET_SIZE(co->co_cellvars);
	assert(idx >= 0 && idx < PyTuple_GET_SIZE(co->co_freevars));
	auto name = PyTuple_GET_ITEM(co->co_freevars, idx);
	auto locals = frame->f_locals;
	if (PyDict_CheckExact(locals)) {
		value = PyDict_GetItem(locals, name);
		Py_XINCREF(value);
	}
	else {
		value = PyObject_GetItem(locals, name);
		if (value == NULL && PyErr_Occurred()) {
			if (!PyErr_ExceptionMatches(PyExc_KeyError)) {
				//goto error; // TODO: Error handling
			}
			PyErr_Clear();
		}
	}
	if (!value) {
		auto freevars = frame->f_localsplus + co->co_nlocals;
		PyObject *cell = freevars[oparg];
		value = PyCell_GET(cell);
		if (value == NULL) {
			// TODO: Error handling
			//format_exc_unbound(co, oparg);
			//goto error;
		}
		Py_INCREF(value);
	}

	return value;
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

void StoreGlobal(PyObject* v, PyFrameObject* f, PyObject* name) {
	int err;
	err = PyDict_SetItem(f->f_globals, name, v);
	Py_DECREF(v);
	// TODO: Error handling
}

void DeleteGlobal(PyFrameObject* f, PyObject* name) {
	int err;
	err = PyDict_DelItem(f->f_globals, name);
	// TODO: Error handling
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

PyObject* DoIterNext(PyObject* iter) {
	return (*iter->ob_type->tp_iternext)(iter);
}

void DoCellSet(PyObject* value, PyObject* cell) {
	PyCell_Set(cell, value);
	Py_DecRef(value);
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

// Returns: the address for the 1st set of items, the constructed list, and the
// address where the remainder live.
PyObject** DoUnpackSequenceEx(PyObject* seq, size_t leftSize, size_t rightSize, PyObject** tempStorage, PyObject** listRes, PyObject*** remainder){
	if (PyTuple_CheckExact(seq) && PyTuple_GET_SIZE(seq) >= (leftSize + rightSize)) {
		auto listSize = PyTuple_GET_SIZE(seq) - (leftSize + rightSize);
		auto list = (PyListObject*)PyList_New(listSize);
		for (int i = 0; i < listSize; i++) {
			list->ob_item[i] = ((PyTupleObject *)seq)->ob_item[i + leftSize];
		}
		*remainder = ((PyTupleObject *)seq)->ob_item + leftSize + listSize;
		*listRes = (PyObject*)list;
		return ((PyTupleObject *)seq)->ob_item;
	}
	else if (PyList_CheckExact(seq) && PyList_GET_SIZE(seq) >= (leftSize + rightSize)) {
		auto listSize = PyList_GET_SIZE(seq) - (leftSize + rightSize);
		auto list = (PyListObject*)PyList_New(listSize);
		for (int i = 0; i < listSize; i++) {
			list->ob_item[i] = ((PyListObject *)seq)->ob_item[i + leftSize];
		}
		*remainder = ((PyListObject *)seq)->ob_item + leftSize + listSize;
		*listRes = (PyObject*)list;
		return ((PyListObject *)seq)->ob_item;
	}
	else {
		// the function allocated space on the stack for us to
		// store these temporarily.
		auto it = PyObject_GetIter(seq);
		auto sp = tempStorage + leftSize + rightSize;
		int i = 0;
		for (; i < leftSize; i++) {
			auto w = PyIter_Next(it);
			if (w == NULL) {
				/* Iterator done, via error or exhaustion. */
				// TODO: Error handling
				/*if (!PyErr_Occurred()) {
				PyErr_Format(PyExc_ValueError,
				"need more than %d value%s to unpack",
				i, i == 1 ? "" : "s");
				}
				goto Error;*/
			}
			tempStorage[i] = w;
		}

		if (listRes == nullptr) {
			// TODO: Error handling
			//	/* We better have exhausted the iterator now. */
			//	w = PyIter_Next(it);
			//	if (w == NULL) {
			//		if (PyErr_Occurred())
			//			goto Error;
			//		Py_DECREF(it);
			//		return 1;
			//	}
			//	Py_DECREF(w);
			//	PyErr_Format(PyExc_ValueError, "too many values to unpack "
			//		"(expected %d)", argcnt);
			//	goto Error;
		}
		else{

			auto l = PySequence_List(it);
			//if (l == NULL)
			//	goto Error;
			*listRes = l;
			i++;

			auto ll = PyList_GET_SIZE(l);
			//if (ll < argcntafter) {
			//	PyErr_Format(PyExc_ValueError, "need more than %zd values to unpack",
			//		argcnt + ll);
			//	goto Error;
			//}

			/* Pop the "after-variable" args off the list. */
			for (auto j = rightSize; j > 0; j--, i++) {
				*--sp = PyList_GET_ITEM(l, ll - j);
			}
			/* Resize the list. */
			Py_SIZE(l) = ll - rightSize;
		}
		Py_DECREF(it);
		if (remainder != nullptr) {
			*remainder = tempStorage + leftSize;
		}
		return tempStorage;
	}
}

// Unpacks the given sequence and returns a pointer to where the sequence
// is stored.  If this is a type we can just grab the array from it returns
// the array.  Otherwise we unpack the sequence into tempStorage which was
// allocated on the stack when we entered the generated method body.
PyObject** DoUnpackSequence(PyObject* seq, size_t size, PyObject** tempStorage){
	if (PyTuple_CheckExact(seq) && PyTuple_GET_SIZE(seq) == size) {
		return ((PyTupleObject *)seq)->ob_item;
	}
	else if (PyList_CheckExact(seq) && PyList_GET_SIZE(seq) == size) {
		return ((PyListObject *)seq)->ob_item;
	}
	else{
		return DoUnpackSequenceEx(seq, size, 0, tempStorage, nullptr, nullptr);
	}
}

PyObject* LoadAttr(PyObject* owner, PyObject* name) {
	PyObject *res = PyObject_GetAttr(owner, name);
	Py_DECREF(owner);
	return res;
}

void StoreAttr(PyObject* owner, PyObject* name, PyObject* value) {
	int res = PyObject_SetAttr(owner, name, value);
	// TODO: Error handling
	Py_DECREF(owner);
	Py_DECREF(value);
}

void DeleteAttr(PyObject* owner, PyObject* name) {
	int res = PyObject_DelAttr(owner, name); // TODO: Error handling
	Py_DECREF(owner);
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

void DeleteName(PyObject* v, PyFrameObject* f, PyObject* name) {
	PyObject *ns = f->f_locals;
	int err;
	//if (ns == NULL) {
	//	PyErr_Format(PyExc_SystemError,
	//		"no locals when deleting %R", name);
	//	Py_DECREF(v);
	//	goto error;
	//}
	err = PyObject_DelItem(ns, name);
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

	// pre-calculate some information...
	ILGenerator il(g_module, CORINFO_TYPE_NATIVEINT, std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) });

	unordered_map<int, Local> sequenceLocals;
	unsigned char *byteCode = (unsigned char *)((PyBytesObject*)code->co_code)->ob_sval;
	auto size = PyBytes_Size(code->co_code);
	int oparg;
	for (int i = 0; i < size; i++) {
		auto byte = byteCode[i];
		if (HAS_ARG(byte)){
			oparg = NEXTARG();
		}

		switch (byte) {
		case UNPACK_EX:
		{
			auto sequenceTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.ld_i(((oparg & 0xFF) + (oparg >> 8)) * sizeof(void*));
			il.push_back(CEE_PREFIX1);
			il.push_back(CEE_LOCALLOC);
			il.st_loc(sequenceTmp);

			sequenceLocals[i] = sequenceTmp;
		}
			break;
		case UNPACK_SEQUENCE:
			// we need a buffer for the slow case, but we need 
			// to avoid allocating it in loops.
			auto sequenceTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.ld_i(oparg * sizeof(void*));
			il.push_back(CEE_PREFIX1);
			il.push_back(CEE_LOCALLOC);
			il.st_loc(sequenceTmp);

			sequenceLocals[i] = sequenceTmp;
			break;
		}
	}

	Label ok;
	unordered_map<int, Label> offsetLabels;
	vector<size_t> loopEnd;
	int stackDepth = 0;
	for (int i = 0; i < size; i++) {

		while (loopEnd.size() != 0 && loopEnd.back() == i) {
			loopEnd.pop_back();
		}

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
		case NOP:
			break;
		case ROT_TWO:
		{
			auto top = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			auto second = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));

			il.st_loc(top);
			il.st_loc(second);

			il.ld_loc(top);
			il.ld_loc(second);

			il.free_local(top);
			il.free_local(second);
		}
			break;
		case ROT_THREE:
		{
			auto top = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			auto second = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			auto third = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));

			il.st_loc(top);
			il.st_loc(second);
			il.st_loc(third);

			il.ld_loc(top);
			il.ld_loc(third);
			il.ld_loc(second);

			il.free_local(top);
			il.free_local(second);
			il.free_local(third);
		}
			break;
		case POP_TOP:
			decref(il);
			stackDepth--;
			break;
		case DUP_TOP:
			il.dup();
			il.dup();
			incref(il);
			stackDepth++;
			break;
		case DUP_TOP_TWO:
		{
			auto top = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			auto second = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));

			il.st_loc(top);
			il.st_loc(second);

			il.ld_loc(second);
			il.ld_loc(top);
			il.ld_loc(second);
			il.ld_loc(top);

			il.ld_loc(top);
			incref(il);
			il.ld_loc(second);
			incref(il);

			il.free_local(top);
			il.free_local(second);

			stackDepth += 2;
		}
			break;
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
			// offset is relative to end of current instruction
			loopEnd.push_back(oparg + i + 1);
			break;
		case BREAK_LOOP:
			il.branch(BranchLeave, getOffsetLabel(il, offsetLabels, loopEnd.back()));
			break;
		case CONTINUE_LOOP:
			// used in exceptional case...
			il.branch(BranchLeave, getOffsetLabel(il, offsetLabels, oparg));
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
		case JUMP_FORWARD:
			il.branch(BranchAlways, getOffsetLabel(il, offsetLabels, oparg + i + 1));
			break;
		case JUMP_IF_FALSE_OR_POP:
		case JUMP_IF_TRUE_OR_POP:
		{
			auto tmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(tmp);
			

			auto noJump = il.define_label();
			auto willJump = il.define_label();
			// fast checks for true/false...
			il.ld_loc(tmp);
			il.push_ptr(byte == JUMP_IF_FALSE_OR_POP ? Py_True : Py_False);
			il.compare_eq();
			il.branch(BranchTrue, noJump);

			il.ld_loc(tmp);
			il.push_ptr(byte == JUMP_IF_FALSE_OR_POP ? Py_False : Py_True);
			il.compare_eq();
			il.branch(BranchTrue, willJump);

			// Use PyObject_IsTrue
			il.ld_loc(tmp);
			il.emit_call(METHOD_PYOBJECT_ISTRUE);
			il.ld_i(0);
			il.compare_eq();
			il.branch(byte == JUMP_IF_FALSE_OR_POP ? BranchFalse : BranchTrue, noJump);

			il.mark_label(willJump);			

			il.ld_loc(tmp);	// load the value back onto the stack
			il.branch(BranchAlways, getOffsetLabel(il, offsetLabels, oparg));

			il.mark_label(noJump);
			
			// dec ref because we're popping...
			il.ld_loc(tmp);
			decref(il);
			stackDepth--;

			il.free_local(tmp);
		}
			break;
		case POP_JUMP_IF_TRUE:
		case POP_JUMP_IF_FALSE:
		{
			auto noJump = il.define_label();
			auto willJump = il.define_label();
			// fast checks for true/false...
			il.dup();
			il.push_ptr(byte == POP_JUMP_IF_FALSE ? Py_True : Py_False);
			il.compare_eq();
			il.branch(BranchTrue, noJump);

			il.dup();
			il.push_ptr(byte == POP_JUMP_IF_FALSE ? Py_False : Py_True);
			il.compare_eq();
			il.branch(BranchTrue, willJump);

			// Use PyObject_IsTrue
			il.dup();
			il.emit_call(METHOD_PYOBJECT_ISTRUE);
			il.ld_i(0);
			il.compare_eq();
			il.branch(byte == POP_JUMP_IF_FALSE ? BranchFalse : BranchTrue, noJump);

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
		case STORE_ATTR:
		{
			auto globalName = PyTuple_GetItem(code->co_names, oparg);
			il.push_ptr(globalName);
		}
			il.emit_call(METHOD_STOREATTR_TOKEN);
			break;
		case DELETE_ATTR:
		{
			auto globalName = PyTuple_GetItem(code->co_names, oparg);
			il.push_ptr(globalName);
		}
			il.emit_call(METHOD_DELETEATTR_TOKEN);
			break;
		case LOAD_ATTR:
		{
			auto globalName = PyTuple_GetItem(code->co_names, oparg);
			il.push_ptr(globalName);
		}
			il.emit_call(METHOD_LOADATTR_TOKEN);
			break;
		case STORE_GLOBAL:
			// value is on the stack
			stackDepth--;
			load_frame(il);
			{
				auto globalName = PyTuple_GetItem(code->co_names, oparg);
				il.push_ptr(globalName);
			}
			il.emit_call(METHOD_STOREGLOBAL_TOKEN);
			break;
		case DELETE_GLOBAL:
			load_frame(il);
			{
				auto globalName = PyTuple_GetItem(code->co_names, oparg);
				il.push_ptr(globalName);
			}
			il.emit_call(METHOD_DELETEGLOBAL_TOKEN);
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
		case DELETE_NAME:
			load_frame(il);
			il.push_ptr(PyTuple_GetItem(code->co_names, oparg));
			il.emit_call(METHOD_DELETENAME_TOKEN);
			break;
		case DELETE_FAST:
			// TODO: Check if value is null and report error
			load_frame(il);
			il.ld_i(offsetof(PyFrameObject, f_localsplus) + oparg * sizeof(size_t));
			il.push_back(CEE_ADD);
			il.load_null();
			il.push_back(CEE_STIND_I);
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
		case UNPACK_SEQUENCE:
		{
			auto valueTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(valueTmp); // save the sequence

			// load the iterable, the count, and our temporary 
			// storage if we need to iterate over the object.
			il.ld_loc(valueTmp);
			il.push_ptr((void*)oparg);
			il.ld_loc(sequenceLocals[i]);
			il.emit_call(METHOD_UNPACK_SEQUENCE_TOKEN);

			auto fastTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(fastTmp);

			//while (oparg--) {
			//	item = items[oparg];
			//	Py_INCREF(item);
			//	PUSH(item);
			//}

			auto tmpOpArg = oparg;
			while (tmpOpArg--) {
				il.ld_loc(fastTmp);
				il.push_ptr((void*)(tmpOpArg * sizeof(size_t)));
				il.push_back(CEE_ADD);
				il.push_back(CEE_LDIND_I);
				il.dup();
				incref(il);
			}

			il.ld_loc(valueTmp);
			decref(il);

			il.free_local(valueTmp);
			il.free_local(fastTmp);
		}
			break;
		case UNPACK_EX:
		{
			auto valueTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			auto listTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			auto remainderTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(valueTmp); // save the sequence

			// load the iterable, the sizes, and our temporary 
			// storage if we need to iterate over the object, 
			// the list local address, and the remainder address
			// PyObject* seq, size_t leftSize, size_t rightSize, PyObject** tempStorage, PyObject** list, PyObject*** remainder

			il.ld_loc(valueTmp);
			il.push_ptr((void*)(oparg & 0xFF));
			il.push_ptr((void*)(oparg >> 8));
			il.ld_loc(sequenceLocals[i]);
			il.ld_loca(listTmp);
			il.ld_loca(remainderTmp);
			il.emit_call(METHOD_UNPACK_SEQUENCEEX_TOKEN);

			auto fastTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(fastTmp);

			// load the right hand side...
			auto tmpOpArg = oparg >> 8;
			while (tmpOpArg--) {
				il.ld_loc(remainderTmp);
				il.push_ptr((void*)(tmpOpArg * sizeof(size_t)));
				il.push_back(CEE_ADD);
				il.push_back(CEE_LDIND_I);
				il.dup();
				incref(il);
			}

			// load the list
			il.ld_loc(listTmp);

			// load the left hand side
			//while (oparg--) {
			//	item = items[oparg];
			//	Py_INCREF(item);
			//	PUSH(item);
			//}

			tmpOpArg = oparg & 0xff;
			while (tmpOpArg--) {
				il.ld_loc(fastTmp);
				il.push_ptr((void*)(tmpOpArg * sizeof(size_t)));
				il.push_back(CEE_ADD);
				il.push_back(CEE_LDIND_I);
				il.dup();
				incref(il);
			}

			il.ld_loc(valueTmp);
			decref(il);

			il.free_local(valueTmp);
			il.free_local(fastTmp);
			il.free_local(remainderTmp);
			il.free_local(listTmp);
		}
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
		case BUILD_SLICE:
			if (oparg != 3) {
				il.load_null();
			}
			il.emit_call(METHOD_BUILD_SLICE);
			break;
		case BUILD_SET:
		{
			build_set(il, oparg);
			stackDepth -= oparg;
			stackDepth++;
		}
			break;
		case UNARY_POSITIVE:
			il.emit_call(METHOD_UNARY_POSITIVE);
			break;
		case UNARY_NEGATIVE:
			il.emit_call(METHOD_UNARY_NEGATIVE);
			break;
		case UNARY_NOT:
			il.emit_call(METHOD_UNARY_NOT);
			break;
		case UNARY_INVERT:
			il.emit_call(METHOD_UNARY_INVERT);
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
		case BINARY_MATRIX_MULTIPLY:
			stackDepth--;
			il.emit_call(METHOD_MATRIX_MULTIPLY_TOKEN);
			break;
		case BINARY_LSHIFT:
			stackDepth--;
			il.emit_call(METHOD_BINARY_LSHIFT_TOKEN);
			break;
		case BINARY_RSHIFT:
			stackDepth--;
			il.emit_call(METHOD_BINARY_RSHIFT_TOKEN);
			break;
		case BINARY_AND:
			stackDepth--;
			il.emit_call(METHOD_BINARY_AND_TOKEN);
			break;
		case BINARY_XOR:
			stackDepth--;
			il.emit_call(METHOD_BINARY_XOR_TOKEN);
			break;
		case BINARY_OR:
			stackDepth--;
			il.emit_call(METHOD_BINARY_OR_TOKEN);
			break;
		case BINARY_MULTIPLY:
			stackDepth--;
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
		case MAKE_CLOSURE:
		case MAKE_FUNCTION:
		{
			int posdefaults = oparg & 0xff;
			int kwdefaults = (oparg >> 8) & 0xff;
			int num_annotations = (oparg >> 16) & 0x7fff;

			load_frame(il);
			il.emit_call(METHOD_NEWFUNCTION_TOKEN);

			if (byte == MAKE_CLOSURE) {
				il.emit_call(METHOD_SET_CLOSURE);
			}

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
		case LOAD_DEREF:
			load_frame(il);
			il.ld_i(offsetof(PyFrameObject, f_localsplus) + code->co_nlocals * sizeof(size_t));
			il.push_back(CEE_ADD);
			il.push_back(CEE_LDIND_I);
			il.emit_call(METHOD_PYCELL_GET);
			// TODO:
			//if (value == NULL) {
			//	format_exc_unbound(co, oparg);
			//	goto error;
			//}
			incref(il);
			break;
		case STORE_DEREF:
			stackDepth--;
			load_frame(il);
			il.ld_i(offsetof(PyFrameObject, f_localsplus) + code->co_nlocals * sizeof(size_t));
			il.push_back(CEE_ADD);
			il.push_back(CEE_LDIND_I);
			il.emit_call(METHOD_PYCELL_SET_TOKEN);
			break;
		case DELETE_DEREF:
			il.load_null();
			load_frame(il);
			il.ld_i(offsetof(PyFrameObject, f_localsplus) + code->co_nlocals * sizeof(size_t));
			il.push_back(CEE_ADD);
			il.push_back(CEE_LDIND_I);
			il.emit_call(METHOD_PYCELL_SET_TOKEN);
			break;
		case LOAD_CLOSURE:
			load_frame(il);
			il.ld_i(offsetof(PyFrameObject, f_localsplus) + code->co_nlocals * sizeof(size_t));
			il.push_back(CEE_ADD);
			il.push_back(CEE_LDIND_I);
			il.push_back(CEE_DUP);
			incref(il);
			break;
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
		case SET_ADD:
		{
			vector<Local> tmps;
			Local tmpValue = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(tmpValue);

			for (int i = 0; i < oparg - 1; i++) {
				auto loc = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				il.st_loc(loc);
				tmps.push_back(loc);
			}

			il.ld_loc(tmpValue);
			il.emit_call(METHOD_SET_ADD_TOKEN);

			il.free_local(tmpValue);
			for (int i = tmps.size() - 1; i >= 0; i--) {
				il.ld_loc(tmps[i]);
				il.free_local(tmps[i]);
			}
		}
			break;
		case MAP_ADD:
		{
			vector<Local> tmps;
			Local keyValue = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			Local valueValue = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(keyValue);
			il.st_loc(valueValue);

			for (int i = 0; i < oparg - 1; i++) {
				auto loc = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				il.st_loc(loc);
				tmps.push_back(loc);
			}

			il.ld_loc(keyValue);
			il.ld_loc(valueValue);
			il.emit_call(METHOD_MAP_ADD_TOKEN);

			il.free_local(keyValue);
			il.free_local(valueValue);
			for (int i = tmps.size() - 1; i >= 0; i--) {
				il.ld_loc(tmps[i]);
				il.free_local(tmps[i]);
			}
		}
			break;
		case LIST_APPEND:
			// stack on entry is: list, <temps>, value
			// we need to spill the values in the middle...
		{
			vector<Local> tmps;
			Local tmpValue = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			il.st_loc(tmpValue);

			for (int i = 0; i < oparg - 1; i++) {
				auto loc = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				il.st_loc(loc);
				tmps.push_back(loc);
			}

			il.ld_loc(tmpValue);
			il.emit_call(METHOD_LIST_APPEND_TOKEN);

			il.free_local(tmpValue);
			for (int i = tmps.size() - 1; i >= 0; i--) {
				il.ld_loc(tmps[i]);
				il.free_local(tmps[i]);
			}
		}
			break;
		case INPLACE_POWER:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_POWER_TOKEN);
			break;
		case INPLACE_MULTIPLY:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_MULTIPLY_TOKEN);
			break;
		case INPLACE_MATRIX_MULTIPLY:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_MATRIX_MULTIPLY_TOKEN);
			break;
		case INPLACE_TRUE_DIVIDE:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_TRUE_DIVIDE_TOKEN);
			break;
		case INPLACE_FLOOR_DIVIDE:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_FLOOR_DIVIDE_TOKEN);
			break;
		case INPLACE_MODULO:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_MODULO_TOKEN);
			break;
		case INPLACE_ADD:
			// TODO: We should do the unicode_concatenate ref count optimization
			stackDepth--;
			il.emit_call(METHOD_INPLACE_ADD_TOKEN);
			break;
		case INPLACE_SUBTRACT:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_SUBTRACT_TOKEN);
			break;
		case INPLACE_LSHIFT:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_LSHIFT_TOKEN);
			break;
		case INPLACE_RSHIFT:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_RSHIFT_TOKEN);
			break;
		case INPLACE_AND:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_AND_TOKEN);
			break;
		case INPLACE_XOR:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_XOR_TOKEN);
			break;
		case INPLACE_OR:
			stackDepth--;
			il.emit_call(METHOD_INPLACE_OR_TOKEN);
			break;
		case PRINT_EXPR:
			stackDepth--;
			il.emit_call(METHOD_PRINT_EXPR_TOKEN);
			break;
		case LOAD_CLASSDEREF:
			load_frame(il);
			il.ld_i(oparg);
			il.emit_call(METHOD_LOAD_CLASSDEREF_TOKEN);
			break;
		case CALL_FUNCTION_VAR_KW:
		
		case YIELD_FROM:
		case YIELD_VALUE:

		case IMPORT_NAME:
		case IMPORT_STAR:
		case IMPORT_FROM:

		case RAISE_VARARGS:
		case POP_EXCEPT:
		case END_FINALLY:
		case SETUP_EXCEPT:
		case SETUP_WITH:
		case WITH_CLEANUP:
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


	g_module->m_methods[METHOD_MATRIX_MULTIPLY_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoMatrixMultiply
		);
	g_module->m_methods[METHOD_BINARY_LSHIFT_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoBinaryLShift
		);
	g_module->m_methods[METHOD_BINARY_RSHIFT_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoBinaryRShift
		);
	g_module->m_methods[METHOD_BINARY_AND_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoBinaryAnd
		);
	g_module->m_methods[METHOD_BINARY_XOR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoBinaryXor
		);
	g_module->m_methods[METHOD_BINARY_OR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoBinaryOr
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

	g_module->m_methods[METHOD_PYITER_NEXT] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&PyIter_Next
		);

	g_module->m_methods[METHOD_PYCELL_GET] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&PyCell_Get
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

	g_module->m_methods[METHOD_UNPACK_SEQUENCE_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoUnpackSequence
		);

	g_module->m_methods[METHOD_UNPACK_SEQUENCEEX_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT),
		Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT)},
		&DoUnpackSequenceEx
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
	g_module->m_methods[METHOD_STOREGLOBAL_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&StoreGlobal
		);
	g_module->m_methods[METHOD_DELETEGLOBAL_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DeleteGlobal
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
	g_module->m_methods[METHOD_STOREATTR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&StoreAttr
		);
	g_module->m_methods[METHOD_DELETEATTR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DeleteAttr
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
	g_module->m_methods[METHOD_DELETENAME_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT)  },
		&DeleteName
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
	g_module->m_methods[METHOD_PYCELL_SET_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoCellSet
		);
	g_module->m_methods[METHOD_SET_CLOSURE] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoSetClosure
		);
	g_module->m_methods[METHOD_BUILD_SLICE] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoBuildSlice
		);
	g_module->m_methods[METHOD_UNARY_POSITIVE] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoUnaryPositive
		);
	g_module->m_methods[METHOD_UNARY_NEGATIVE] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoUnaryNegative
		);
	g_module->m_methods[METHOD_UNARY_NOT] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoUnaryNot
		);

	g_module->m_methods[METHOD_UNARY_INVERT] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoUnaryInvert
		);

	g_module->m_methods[METHOD_LIST_APPEND_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoListAppend
		);
	g_module->m_methods[METHOD_SET_ADD_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoSetAdd
		);
	
	g_module->m_methods[METHOD_MAP_ADD_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoMapAdd
		);

	g_module->m_methods[METHOD_INPLACE_POWER_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplacePower
		);

	g_module->m_methods[METHOD_INPLACE_MULTIPLY_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceMultiply
		);

	g_module->m_methods[METHOD_INPLACE_MATRIX_MULTIPLY_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceMatrixMultiply
		);

	g_module->m_methods[METHOD_INPLACE_TRUE_DIVIDE_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceTrueDivide
		);

	g_module->m_methods[METHOD_INPLACE_FLOOR_DIVIDE_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceFloorDivide
		);

	g_module->m_methods[METHOD_INPLACE_MODULO_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceModulo
		);

	g_module->m_methods[METHOD_INPLACE_ADD_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceAdd
		);

	g_module->m_methods[METHOD_INPLACE_SUBTRACT_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceSubtract
		);

	g_module->m_methods[METHOD_INPLACE_LSHIFT_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceLShift
		);

	g_module->m_methods[METHOD_INPLACE_RSHIFT_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceRShift
		);

	g_module->m_methods[METHOD_INPLACE_AND_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceAnd
		);

	g_module->m_methods[METHOD_INPLACE_XOR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceXor
		);

	g_module->m_methods[METHOD_INPLACE_OR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoInplaceOr
		);

	g_module->m_methods[METHOD_PRINT_EXPR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoPrintExpr
		);
	
	g_module->m_methods[METHOD_LOAD_CLASSDEREF_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoLoadClassDeref
		);
	
	//g_module->m_methods[SIG_ITERNEXT_TOKEN] = Method(
	//	nullptr,
	//	CORINFO_TYPE_NATIVEINT,
	//	std::vector < Parameter > { },
	//	nullptr,
	//	&g_iterNextVtable
	//);
}

