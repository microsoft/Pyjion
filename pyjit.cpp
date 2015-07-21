#include "pyjit.h"

// TODO: Pull helper/intrinsics into their own files, with nice definition syntax
// TODO: pull jittest.cpp into it's own project

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
#define METHOD_PREPARE_EXCEPTION			0x00000036
#define METHOD_DO_RAISE						0x00000037
#define METHOD_EH_TRACE						0x00000038
#define METHOD_COMPARE_EXCEPTIONS			0x00000039
#define METHOD_UNBOUND_LOCAL				0x0000003A
#define METHOD_DEBUG_TRACE					0x0000003B
#define METHOD_FUNC_SET_DEFAULTS			0x0000003C
#define	METHOD_CALLNKW_TOKEN				0x0000003D
#define	METHOD_DEBUG_DUMP_FRAME				0x0000003E
#define METHOD_PYERR_CLEAR					0x0000003F
#define METHOD_PY_CHECKFUNCTIONRESULT		0x00000040
#define METHOD_PY_PUSHFRAME					0x00000041
#define METHOD_PY_POPFRAME					0x00000042
#define METHOD_PY_IMPORTNAME				0x00000043
#define METHOD_PY_FANCYCALL					0x00000044
#define METHOD_PY_IMPORTFROM				0x00000045

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
#define METHOD_PYERR_RESTORE		0x00020008

// Misc helpers
#define METHOD_LOADGLOBAL_TOKEN		0x00030000
#define METHOD_LOADATTR_TOKEN		0x00030001
#define METHOD_STOREATTR_TOKEN		0x00030002
#define METHOD_DELETEATTR_TOKEN		0x00030003
#define METHOD_STOREGLOBAL_TOKEN	0x00030004
#define METHOD_DELETEGLOBAL_TOKEN	0x00030005

// signatures for calli methods
#define SIG_ITERNEXT_TOKEN			0x00040000

#define NAME_ERROR_MSG \
    "name '%.200s' is not defined"

#define UNBOUNDLOCAL_ERROR_MSG \
    "local variable '%.200s' referenced before assignment"
#define UNBOUNDFREE_ERROR_MSG \
    "free variable '%.200s' referenced before assignment" \
    " in enclosing scope"

static void
format_exc_check_arg(PyObject *exc, const char *format_str, PyObject *obj)
{
	const char *obj_str;

	if (!obj)
		return;

	obj_str = _PyUnicode_AsString(obj);
	if (!obj_str)
		return;

	PyErr_Format(exc, format_str, obj_str);
}

static void
format_exc_unbound(PyCodeObject *co, int oparg)
{
	PyObject *name;
	/* Don't stomp existing exception */
	if (PyErr_Occurred())
		return;
	if (oparg < PyTuple_GET_SIZE(co->co_cellvars)) {
		name = PyTuple_GET_ITEM(co->co_cellvars,
			oparg);
		format_exc_check_arg(
			PyExc_UnboundLocalError,
			UNBOUNDLOCAL_ERROR_MSG,
			name);
	}
	else {
		name = PyTuple_GET_ITEM(co->co_freevars, oparg -
			PyTuple_GET_SIZE(co->co_cellvars));
		format_exc_check_arg(PyExc_NameError,
			UNBOUNDFREE_ERROR_MSG, name);
	}
}

Module *g_module;
extern CorJitInfo g_corJitInfo;
ICorJitCompiler* g_jit;

#define LD_FIELDA(type, field) il.ld_i(offsetof(type, field)); il.add(); 
#define LD_FIELD(type, field) il.ld_i(offsetof(type, field)); il.add(); il.ld_ind_i();
#define ST_FIELD(type, field) il.ld_i(offsetof(type, field)); il.add(); il.st_ind_i();

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
	auto ret = res ? Py_True : Py_False;
	Py_INCREF(ret);
	return ret;
}

PyObject* DoCellGet(PyFrameObject* frame, size_t index) {
	PyObject** cells = frame->f_localsplus + frame->f_code->co_nlocals;
	PyObject *value = PyCell_GET(cells[index]);

	if (value == nullptr) {
		format_exc_unbound(frame->f_code, index);
	}
	else{
		Py_INCREF(value);	
	}
	return value;
}


PyObject* DoNotContains(PyObject *left, PyObject *right, int op) {
	auto res = PySequence_Contains(right, left);
	if (res < 0) {
		return nullptr;
	}
	Py_DECREF(left);
	Py_DECREF(right);
	auto ret = res ? Py_False : Py_True;
	Py_INCREF(ret);
	return ret;
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
	return nullptr;
}

PyObject* DoUnaryInvert(PyObject* value) {
	auto res = PyNumber_Invert(value);
	Py_DECREF(value);
	return res;
}

PyObject* DoListAppend(PyObject* list, PyObject* value) {
	int err = PyList_Append(list, value);
	Py_DECREF(value);
	if (err) {
		return nullptr;
	}
	return list;
}

PyObject* DoSetAdd(PyObject* set, PyObject* value) {
	int err = PySet_Add(set, value);
	Py_DECREF(value);
	if (err) {
		return nullptr;
	}
	return set;
}

PyObject* DoMapAdd(PyObject*map, PyObject*key, PyObject* value) {
	int err = PyDict_SetItem(map, key, value);  /* v[w] = u */
	Py_DECREF(value);
	Py_DECREF(key);
	if (err) {
		return nullptr;
	}
	return map;
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

int DoPrintExpr(PyObject *value) {
	_Py_IDENTIFIER(displayhook);
	PyObject *hook = _PySys_GetObjectId(&PyId_displayhook);
	PyObject *res;
	if (hook == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
			"lost sys.displayhook");
		Py_DECREF(value);
		return 1;
	}
	res = PyObject_CallFunctionObjArgs(hook, value, NULL);
	Py_DECREF(value);
	if (res == NULL) {
		return 1;
	}
	Py_DECREF(res);
	return 0;
}

const char * ObjInfo(PyObject *obj);
void DoPrepareException(PyObject** exc, PyObject**val, PyObject** tb) {
	auto tstate = PyThreadState_GET();

	PyErr_Fetch(exc, val, tb);
	/* Make the raw exception data
	available to the handler,
	so a program can emulate the
	Python main loop. */
	PyErr_NormalizeException(
		exc, val, tb);
	if (tb != NULL)
		PyException_SetTraceback(*val, *tb);
	else
		PyException_SetTraceback(*val, Py_None);
	Py_INCREF(*exc);
	tstate->exc_type = *exc;
	Py_INCREF(*val);
	tstate->exc_value = *val;
	tstate->exc_traceback = *tb;
	if (*tb == NULL)
		*tb = Py_None;
	Py_INCREF(*tb);
}

#define CANNOT_CATCH_MSG "catching classes that do not inherit from "\
                         "BaseException is not allowed"

PyObject* DoCompareExceptions(PyObject*v, PyObject* w) {
	if (PyTuple_Check(w)) {
		Py_ssize_t i, length;
		length = PyTuple_Size(w);
		for (i = 0; i < length; i += 1) {
			PyObject *exc = PyTuple_GET_ITEM(w, i);
			if (!PyExceptionClass_Check(exc)) {
				PyErr_SetString(PyExc_TypeError,
					CANNOT_CATCH_MSG);
				return NULL;
			}
		}
	}
	else {
		if (!PyExceptionClass_Check(w)) {
			PyErr_SetString(PyExc_TypeError,
				CANNOT_CATCH_MSG);
			return NULL;
		}
	}
	bool res = PyErr_GivenExceptionMatches(v, w);
	v = res ? Py_True : Py_False;
	Py_INCREF(v);
	return v;
}

void DoUnboundLocal(PyObject* name) {
	format_exc_check_arg(
		PyExc_UnboundLocalError,
		UNBOUNDLOCAL_ERROR_MSG,
		name
		);
}

void DoDebugTrace(char* msg) {
	puts(msg);
}

const char * ObjInfo(PyObject *obj);

void DoPyErrRestore(PyObject*tb, PyObject*value, PyObject*exception) {
	printf("Restoring exception %s\r\n", ObjInfo(exception));
	printf("Restoring value %s\r\n", ObjInfo(value));
	printf("Restoring tb %s\r\n", ObjInfo(tb));
	PyErr_Restore(exception, value, tb);
}

PyObject* DoCheckFunctionResult(PyObject* value) {
	return _Py_CheckFunctionResult(nullptr, value, "CompiledCode");
}

PyObject* DoImportName(PyObject*level, PyObject*from, PyObject* name, PyFrameObject* f) {
	_Py_IDENTIFIER(__import__);
	PyObject *func = _PyDict_GetItemId(f->f_builtins, &PyId___import__);
	PyObject *args, *res;
	if (func == NULL) {
		PyErr_SetString(PyExc_ImportError,
			"__import__ not found");
		return nullptr;
	}
	Py_INCREF(func);
	if (PyLong_AsLong(level) != -1 || PyErr_Occurred())
		args = PyTuple_Pack(5,
		name,
		f->f_globals,
		f->f_locals == NULL ?
			Py_None : f->f_locals,
					  from,
					  level);
	else
		args = PyTuple_Pack(4,
			name,
			f->f_globals,
			f->f_locals == NULL ?
				Py_None : f->f_locals,
						  from);
	Py_DECREF(level);
	Py_DECREF(from);
	if (args == NULL) {
		Py_DECREF(func);
		//STACKADJ(-1);
		return nullptr;
	}
	//READ_TIMESTAMP(intr0);
	res = PyEval_CallObject(func, args);
	//READ_TIMESTAMP(intr1);
	Py_DECREF(args);
	Py_DECREF(func);
	return res;
}

PyObject* DoImportFrom(PyObject*v, PyObject* name) {
	PyObject *x;
	_Py_IDENTIFIER(__name__);
	PyObject *fullmodname, *pkgname;

	x = PyObject_GetAttr(v, name);
	if (x != NULL || !PyErr_ExceptionMatches(PyExc_AttributeError))
		return x;
	/* Issue #17636: in case this failed because of a circular relative
	import, try to fallback on reading the module directly from
	sys.modules. */
	PyErr_Clear();
	pkgname = _PyObject_GetAttrId(v, &PyId___name__);
	if (pkgname == NULL)
		return NULL;
	fullmodname = PyUnicode_FromFormat("%U.%U", pkgname, name);
	Py_DECREF(pkgname);
	if (fullmodname == NULL)
		return NULL;
	x = PyDict_GetItem(PyImport_GetModuleDict(), fullmodname);
	if (x == NULL)
		PyErr_Format(PyExc_ImportError, "cannot import name %R", name);
	else
		Py_INCREF(x);
	Py_DECREF(fullmodname);
	return x;
}

PyObject* DoFancyCall(PyObject* func, PyObject*args, PyObject*kwargs, PyObject* stararg, PyObject* kwdict) {
	// any of these arguments can be null...
	PyObject *finalArgs, *result = nullptr;
	int nstar;
	// Convert stararg to tuple if necessary...
	if (stararg != nullptr) {
		if (!PyTuple_Check(stararg)) {
			auto newStarArg = PySequence_Tuple(stararg);
			if (newStarArg == NULL) {
				if (PyErr_ExceptionMatches(PyExc_TypeError)) {
					PyErr_Format(PyExc_TypeError,
						"%.200s%.200s argument after * "
						"must be a sequence, not %.200s",
						PyEval_GetFuncName(func),
						PyEval_GetFuncDesc(func),
						stararg->ob_type->tp_name);
				}
				goto ext_call_fail;
			}
			Py_DECREF(stararg);
			stararg = newStarArg;
		}
		nstar = PyTuple_GET_SIZE(stararg);
	}

	if (kwdict != nullptr) {
		if (kwargs == nullptr) {
			// we haven't allocated the dict for the call yet, do so now
			kwargs = PyDict_New();
			if (kwargs == nullptr) {
				goto ext_call_fail;
			}
		} // else we allocated it in the generated code

		if (PyDict_Update(kwargs, kwdict) != 0) {
			/* PyDict_Update raises attribute
			* error (percolated from an attempt
			* to get 'keys' attribute) instead of
			* a type error if its second argument
			* is not a mapping.
			*/
			if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
				PyErr_Format(PyExc_TypeError,
					"%.200s%.200s argument after ** "
					"must be a mapping, not %.200s",
					PyEval_GetFuncName(func),
					PyEval_GetFuncDesc(func),
					kwdict->ob_type->tp_name);
			}
			goto ext_call_fail;
		}
	}

	// Compute the final arg list...
	if (args == nullptr) {
		if (stararg != nullptr) {
			args = stararg;
			stararg = nullptr;
		}
		else{
			args = PyTuple_New(0);
		}
	} else if (stararg != nullptr) {
		// need to concat values together...
		auto finalArgs = PyTuple_New(PyTuple_GET_SIZE(args) + nstar);
		for (int i = 0; i < PyTuple_GET_SIZE(args); i++) {
			auto item = PyTuple_GET_ITEM(args, i);
			Py_INCREF(item);
			PyTuple_SET_ITEM(finalArgs, i, item);
		}
		for (int i = 0; i < PyTuple_GET_SIZE(stararg); i++) {
			auto item = PyTuple_GET_ITEM(stararg, i);
			Py_INCREF(item);
			PyTuple_SET_ITEM(
				finalArgs, 
				i + PyTuple_GET_SIZE(args), 
				item
			);
		}
		Py_DECREF(args);
		args = finalArgs;
	}

	result = PyObject_Call(func, args , kwargs);

ext_call_fail:
	Py_XDECREF(func);
	Py_XDECREF(args);
	Py_XDECREF(kwargs);
	Py_XDECREF(stararg);
	Py_XDECREF(kwdict);
	return result;
}

void DoDebugDumpFrame(PyFrameObject* frame) {
	static bool _dumping = false;
	if (_dumping) {
		return;
	}
	_dumping = true;

	int argCount = frame->f_code->co_argcount;
	for (int i = 0; i < argCount; i++) {
		auto local = frame->f_localsplus[i];
		if (local != nullptr) {
			if (local->ob_type != nullptr) {
				printf("Arg %d %s\r\n", i, local->ob_type->tp_name);
			}
			else{
				printf("Null local type?");
			}
		}
		else{
			printf("Null local?");
		}
	}
	_dumping = false;
}

void DoPushFrame(PyFrameObject* frame) {
	PyThreadState_Get()->frame = frame;
}

void DoPopFrame(PyFrameObject* frame) {
	PyThreadState_Get()->frame = frame->f_back;
}

int DoFunctionSetDefaults(PyObject* defs, PyObject* func) {
	if (PyFunction_SetDefaults(func, defs) != 0) {
		/* Can't happen unless
		PyFunction_SetDefaults changes. */
		Py_DECREF(defs);
		Py_DECREF(func);
		return 1;
	}
	Py_DECREF(defs);
	return 0;
}

void DoEhTrace(PyFrameObject *f) {
	PyTraceBack_Here(f);

	//auto tstate = PyThreadState_GET();

	//if (tstate->c_tracefunc != NULL) {
	//	call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj,
	//		tstate, f);
	//}
}

int DoRaise(PyObject *exc, PyObject *cause)
{
	PyObject *type = NULL, *value = NULL;

	if (exc == NULL) {
		/* Reraise */
		PyThreadState *tstate = PyThreadState_GET();
		PyObject *tb;
		type = tstate->exc_type;
		value = tstate->exc_value;
		tb = tstate->exc_traceback;
		if (type == Py_None) {
			PyErr_SetString(PyExc_RuntimeError,
				"No active exception to reraise");
			return 0;
		}
		Py_XINCREF(type);
		Py_XINCREF(value);
		Py_XINCREF(tb);
		PyErr_Restore(type, value, tb);
		return 1;
	}

	/* We support the following forms of raise:
	raise
	raise <instance>
	raise <type> */

	if (PyExceptionClass_Check(exc)) {
		type = exc;
		value = PyObject_CallObject(exc, NULL);
		if (value == NULL)
			goto raise_error;
		if (!PyExceptionInstance_Check(value)) {
			PyErr_Format(PyExc_TypeError,
				"calling %R should have returned an instance of "
				"BaseException, not %R",
				type, Py_TYPE(value));
			goto raise_error;
		}
	}
	else if (PyExceptionInstance_Check(exc)) {
		value = exc;
		type = PyExceptionInstance_Class(exc);
		Py_INCREF(type);
	}
	else {
		/* Not something you can raise.  You get an exception
		anyway, just not what you specified :-) */
		Py_DECREF(exc);
		PyErr_SetString(PyExc_TypeError,
			"exceptions must derive from BaseException");
		goto raise_error;
	}

	if (cause) {
		PyObject *fixed_cause;
		if (PyExceptionClass_Check(cause)) {
			fixed_cause = PyObject_CallObject(cause, NULL);
			if (fixed_cause == NULL)
				goto raise_error;
			Py_DECREF(cause);
		}
		else if (PyExceptionInstance_Check(cause)) {
			fixed_cause = cause;
		}
		else if (cause == Py_None) {
			Py_DECREF(cause);
			fixed_cause = NULL;
		}
		else {
			PyErr_SetString(PyExc_TypeError,
				"exception causes must derive from "
				"BaseException");
			goto raise_error;
		}
		PyException_SetCause(value, fixed_cause);
	}

	PyErr_SetObject(type, value);
	/* PyErr_SetObject incref's its arguments */
	Py_XDECREF(value);
	Py_XDECREF(type);
	return 0;

raise_error:
	Py_XDECREF(value);
	Py_XDECREF(type);
	Py_XDECREF(cause);
	return 0;
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
				return nullptr;
			}
			PyErr_Clear();
		}
	}
	if (!value) {
		auto freevars = frame->f_localsplus + co->co_nlocals;
		PyObject *cell = freevars[oparg];
		value = PyCell_GET(cell);
		if (value == NULL) {
			format_exc_unbound(co, oparg);
			return nullptr;
		}
		Py_INCREF(value);
	}

	return value;
}

int DoStoreMap(PyObject *key, PyObject *value, PyObject* map) {
	assert(PyDict_CheckExact(map));
	auto res = PyDict_SetItem(map, key, value);
	Py_DECREF(key);
	Py_DECREF(value);
	return res;
}

int DoStoreSubscr(PyObject* value, PyObject *container, PyObject *index) {
	auto res = PyObject_SetItem(container, index, value);
	Py_DECREF(index);
	Py_DECREF(value);
	Py_DECREF(container);
	return res;
}

int DoDeleteSubscr(PyObject *container, PyObject *index) {
	auto res = PyObject_DelItem(container, index);
	Py_DECREF(index);
	Py_DECREF(container);
	return res;
}

PyObject* CallN(PyObject *target, PyObject* args) {
	// we stole references for the tuple...
	auto res = PyObject_Call(target, args, nullptr);
	Py_DECREF(target);
	return res;
}

PyObject* CallNKW(PyObject *target, PyObject* args, PyObject* kwargs) {
	// we stole references for the tuple...
#ifdef DEBUG_TRACE
	auto targetRepr = PyObject_Repr(target);
	printf("Target: %s\r\n", PyUnicode_AsUTF8(targetRepr));
	Py_DECREF(targetRepr);

	auto repr = PyObject_Repr(args);
	printf("Tuple: %s\r\n", PyUnicode_AsUTF8(repr));
	Py_DECREF(repr);

	auto repr2 = PyObject_Repr(kwargs);
	printf("KW Args: %s\r\n", PyUnicode_AsUTF8(repr2));
	Py_DECREF(repr2);

	printf("%d\r\n", kwargs->ob_refcnt);
#endif
	auto res = PyObject_Call(target, args, kwargs);
#ifdef DEBUG_TRACE
	printf("%d\r\n", kwargs->ob_refcnt);
#endif
	Py_DECREF(target);
	Py_DECREF(args);
	Py_DECREF(kwargs);
	return res;
}

int StoreGlobal(PyObject* v, PyFrameObject* f, PyObject* name) {
	int err = PyDict_SetItem(f->f_globals, name, v);
	Py_DECREF(v);
	return err;
}

int DeleteGlobal(PyFrameObject* f, PyObject* name) {
	return PyDict_DelItem(f->f_globals, name);
}

PyObject* LoadGlobal(PyFrameObject* f, PyObject* name) {
	PyObject* v;
	if (PyDict_CheckExact(f->f_globals)
		&& PyDict_CheckExact(f->f_builtins)) {
		v = _PyDict_LoadGlobal((PyDictObject *)f->f_globals,
			(PyDictObject *)f->f_builtins,
			name);
		if (v == NULL) {
			if (!_PyErr_OCCURRED())
				format_exc_check_arg(PyExc_NameError, NAME_ERROR_MSG, name);
			return nullptr;
		}
		Py_INCREF(v);
	}
	else 
	{
		/* Slow-path if globals or builtins is not a dict */
		auto asciiName = PyUnicode_AsUTF8(name);
		v = PyObject_GetItem(f->f_globals, name);
		if (v == NULL) {

			v = PyObject_GetItem(f->f_builtins, name);
			if (v == NULL) {
				if (PyErr_ExceptionMatches(PyExc_KeyError))
					format_exc_check_arg(
					PyExc_NameError,
					NAME_ERROR_MSG, name);
				return nullptr;
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

PyObject* DoIterNext(PyObject* iter, int*error) {
	auto res = (*iter->ob_type->tp_iternext)(iter);
	if (res == nullptr) {
		if (PyErr_Occurred()) {
			if (!PyErr_ExceptionMatches(PyExc_StopIteration)) {
				*error = 1;
				return nullptr;
			}
			*error = 0;
			// TODO: Tracing...
			//else if (tstate->c_tracefunc != NULL)
			//	call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, f);
			PyErr_Clear();
		}
	}
	return res;
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
			return nullptr;
		}
		Py_INCREF(bc);
	}
	else {
		PyObject *build_class_str = _PyUnicode_FromId(&PyId___build_class__);
		if (build_class_str == NULL) {
			return nullptr;
		}
		bc = PyObject_GetItem(f->f_builtins, build_class_str);
		if (bc == NULL) {
			if (PyErr_ExceptionMatches(PyExc_KeyError)) {
				PyErr_SetString(PyExc_NameError, "__build_class__ not found");
				return nullptr;
			}
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
				if (!PyErr_Occurred()) {
					PyErr_Format(PyExc_ValueError,
						"need more than %d value%s to unpack",
						i, i == 1 ? "" : "s");
				}
				goto Error;
			}
			tempStorage[i] = w;
		}

		if (listRes == nullptr) {
			/* We better have exhausted the iterator now. */
			auto w = PyIter_Next(it);
			if (w == NULL) {
				if (PyErr_Occurred())
					goto Error;
				Py_DECREF(it);
				return tempStorage;
			}
			Py_DECREF(w);
			PyErr_Format(PyExc_ValueError, "too many values to unpack "
				"(expected %d)", leftSize);
			goto Error;
		}
		else{

			auto l = PySequence_List(it);
			if (l == NULL)
				goto Error;
			*listRes = l;
			i++;

			auto ll = PyList_GET_SIZE(l);
			if (ll < rightSize) {
				PyErr_Format(PyExc_ValueError, "need more than %zd values to unpack",
					leftSize + ll);
				goto Error;
			}

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

	Error:
		for (; i > 0; i--, sp++)
			Py_DECREF(*sp);
		Py_XDECREF(it);
		return nullptr;

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
#if DEBUG_TRACE
	if (res == nullptr) {
		printf("Load attr failed: %s\r\n", PyUnicode_AsUTF8(name));
	}
#endif
	return res;
}

const char * ObjInfo(PyObject *obj) {
	if (obj == nullptr) {
		return "<NULL>";
	}
	if (PyUnicode_Check(obj)) {
		return PyUnicode_AsUTF8(obj);
	}
	else if(obj->ob_type != nullptr) {
		return obj->ob_type->tp_name;
	}
	else{
		return "<null type>";
	}
}

int StoreAttr(PyObject* value, PyObject* owner, PyObject* name) {
	int res = PyObject_SetAttr(owner, name, value);
	Py_DECREF(owner);
	Py_DECREF(value);
	return res;
}

int DeleteAttr(PyObject* owner, PyObject* name) {
	int res = PyObject_DelAttr(owner, name);
	Py_DECREF(owner);
	return res;
}

PyObject* LoadName(PyFrameObject* f, PyObject* name) {
	PyObject *locals = f->f_locals;
	PyObject *v;
	if (locals == NULL) {
		PyErr_Format(PyExc_SystemError,
			"no locals when loading %R", name);
		return nullptr;
	}
	if (PyDict_CheckExact(locals)) {
		v = PyDict_GetItem(locals, name);
		Py_XINCREF(v);
	}
	else {
		v = PyObject_GetItem(locals, name);
		if (v == NULL && _PyErr_OCCURRED()) {
			if (!PyErr_ExceptionMatches(PyExc_KeyError))
				return nullptr;
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
					format_exc_check_arg(
						PyExc_NameError,
						NAME_ERROR_MSG, name);
					return nullptr;
				}
				Py_INCREF(v);
			}
			else {
				v = PyObject_GetItem(f->f_builtins, name);
				if (v == NULL) {
					if (PyErr_ExceptionMatches(PyExc_KeyError))
						format_exc_check_arg(
						PyExc_NameError,
						NAME_ERROR_MSG, name);
					return nullptr;
				}
			}
		}
	}
	return v;
}

int StoreName(PyObject* v, PyFrameObject* f, PyObject* name) {
	PyObject *ns = f->f_locals;
	int err;
	if (ns == NULL) {
		PyErr_Format(PyExc_SystemError,
			"no locals found when storing %R", name);
		Py_DECREF(v);
		return 1;
	}
	if (PyDict_CheckExact(ns))
		err = PyDict_SetItem(ns, name, v);
	else
		err = PyObject_SetItem(ns, name, v);
	Py_DECREF(v);
	return err;
}

int DeleteName(PyFrameObject* f, PyObject* name) {
	PyObject *ns = f->f_locals;
	int err;
	if (ns == NULL) {
		PyErr_Format(PyExc_SystemError,
			"no locals when deleting %R", name);
		return 1;
	}
	err = PyObject_DelItem(ns, name);
	if (err != 0) {
		format_exc_check_arg(PyExc_NameError,
			NAME_ERROR_MSG,
			name);
	}
	return err;
}

PyObject* g_emptyTuple;

PyObject* Call0(PyObject *target) {
	auto res = PyObject_Call(target, g_emptyTuple, nullptr);
	Py_DECREF(target);
	return res;
}

#define BLOCK_CONTINUES 0x01
#define BLOCK_RETURNS	0x02
#define BLOCK_BREAKS	0x04

struct BlockInfo {
	Label Raise,		// our raise stub label, prepares the exception
		ReRaise,		// our re-raise stub label, prepares the exception w/o traceback update
		ErrorTarget;	// the actual label for the handler
	int EndOffset, Kind, Flags, ContinueOffset;

	BlockInfo() {
	}

	BlockInfo(Label raise, Label reraise, Label errorTarget, int endOffset, int kind, int flags = 0, int continueOffset = 0) {
		Raise = raise;
		ReRaise = reraise;
		ErrorTarget = errorTarget;
		EndOffset = endOffset;
		Kind = kind;
		Flags = flags;
		ContinueOffset = continueOffset;
	}
};

bool can_skip_lasti_update(int opcode) {
	switch (opcode) {
	case DUP_TOP:
	case SETUP_EXCEPT:
	case NOP:
	case ROT_TWO:
	case ROT_THREE:
	case POP_BLOCK:
	case POP_JUMP_IF_FALSE:
	case POP_JUMP_IF_TRUE:
	case POP_TOP:
	case DUP_TOP_TWO:
	case BREAK_LOOP:
	case CONTINUE_LOOP:
	case END_FINALLY:
	case LOAD_CONST:
	case JUMP_FORWARD:
		return true;
	}
	return false;
}

#define NEXTARG() oparg = *(unsigned short*)&byteCode[i + 1]; i+= 2

class Jitter {
	PyCodeObject *code;
	// pre-calculate some information...
	ILGenerator il;
	unordered_map<int, Local> sequenceLocals;
	unsigned char *byteCode;
	size_t size;
	vector<BlockInfo> m_blockStack;		// maintains the current label we should branch to when we take an exception
	vector<BlockInfo> allHandlers;
	unordered_map<int, Label> offsetLabels;

public:
	Jitter(PyCodeObject *code) : il(g_module, CORINFO_TYPE_NATIVEINT, std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) }){
		if (g_emptyTuple == nullptr) {
			g_emptyTuple = PyTuple_New(0);
		}

		this->code = code;
		this->byteCode = (unsigned char *)((PyBytesObject*)code->co_code)->ob_sval;
		this->size = PyBytes_Size(code->co_code);
	}

	PVOID Compile() {
		PreProcess();
		return CompileWorker();
	}

private:
	void load_frame() {
		il.ld_arg(0);
	}

	void load_local(int oparg) {
		load_frame();
		il.ld_i(offsetof(PyFrameObject, f_localsplus) + oparg * sizeof(size_t));
		il.add();
		il.ld_ind_i();
	}

	void incref() {
		LD_FIELDA(PyObject, ob_refcnt);
		il.dup();
		il.ld_ind_i4();
		il.ld_i4(1);
		il.add();
		il.st_ind_i4();
	}

	void decref() {
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

	void build_tuple(int argCnt) {
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
			il.add();

			// reload the value
			il.ld_loc(valueTmp);

			// store into the array
			il.st_ind_i();
		}
		il.ld_loc(tupleTmp);

		il.free_local(valueTmp);
		il.free_local(tupleTmp);
	}

	void build_list(int argCnt) {

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
			il.add();
			il.ld_ind_i();

			il.st_loc(listItems);

			for (int arg = argCnt - 1; arg >= 0; arg--) {
				// save the argument into a temporary...
				il.st_loc(valueTmp);

				// load the address of the tuple item...
				il.ld_loc(listItems);
				il.ld_i(arg * sizeof(size_t));
				il.add();

				// reload the value
				il.ld_loc(valueTmp);

				// store into the array
				il.st_ind_i();
			}

			// update the size of the list...
			il.ld_loc(listTmp);
			il.dup();
			il.ld_i(offsetof(PyVarObject, ob_size));
			il.add();
			il.push_ptr((void*)argCnt);
			il.st_ind_i();

			il.free_local(valueTmp);
			il.free_local(listTmp);
			il.free_local(listItems);
		}

	}

	void build_set(int argCnt) {
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

	void build_map(int argCnt) {
		il.push_ptr((void*)argCnt);
		il.emit_call(METHOD_PYDICT_NEWPRESIZED);
		{
			// 3.6 doesn't have STORE_OP and instead does it all in BUILD_MAP...
			if (argCnt > 0) {
				auto map = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				il.st_loc(map);
				for (int curArg = 0; curArg < argCnt; curArg++) {
					il.ld_loc(map);
					il.emit_call(METHOD_STOREMAP_TOKEN);
					check_int_error(-1);
				}
				il.ld_loc(map);

				il.free_local(map);
			}
		}
	}

	Label getOffsetLabel(int jumpTo) {
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

	bool can_skip_lasti_update(int opcode) {
		switch (opcode) {
		case DUP_TOP:
		case SETUP_EXCEPT:
		case NOP:
		case ROT_TWO:
		case ROT_THREE:
		case POP_BLOCK:
		case POP_JUMP_IF_FALSE:
		case POP_JUMP_IF_TRUE:
		case POP_TOP:
		case DUP_TOP_TWO:
		case BREAK_LOOP:
		case CONTINUE_LOOP:
		case END_FINALLY:
		case LOAD_CONST:
			return true;
		}
		return false;
	}

	// Checks to see if we have a null value as the last value on our stack
	// indicating an error, and if so, branches to our current error handler.
	void check_error(int curIndex, const char* reason) {
		auto noErr = il.define_label();
		il.dup();
		il.load_null();
		il.branch(BranchNotEqual, noErr);
		// we need to issue a leave to clear the stack as we may have
		// values on the stack...
#if DEBUG_TRACE
		char* tmp = (char*)malloc(100);
		sprintf_s(tmp, 100, "Error at index %d %s %s", curIndex, PyUnicode_AsUTF8(code->co_name), reason);
		il.push_ptr(tmp);
		il.emit_call(METHOD_DEBUG_TRACE);
#endif

		il.branch(BranchLeave, GetEHBlock().Raise);	
		il.mark_label(noErr);
	}

	void check_int_error_leave(int curIndex) {
		auto noErr = il.define_label();
		il.ld_i4(0);
		il.branch(BranchEqual, noErr);
		// we need to issue a leave to clear the stack as we may have
		// values on the stack...
#if DEBUG_TRACE
		char* tmp = (char*)malloc(100);
		sprintf_s(tmp, 100, "Int Error at index %d %s", curIndex, PyUnicode_AsUTF8(code->co_name));
		il.push_ptr(tmp);
		il.emit_call(METHOD_DEBUG_TRACE);
#endif

		il.branch(BranchLeave, GetEHBlock().Raise);
		il.mark_label(noErr);
	}

	// Checks to see if we have a non-zero error code on the stack, and if so,
	// branches to the current error handler.  Consumes the error code in the process
	void check_int_error(int curIndex) {
		check_int_error_leave(curIndex);
		//il.ld_i4(0);
		//il.branch(BranchNotEqual, GetEHBlock().Raise);
	}

	BlockInfo GetEHBlock() {
		for (int i = m_blockStack.size() - 1; i >= 0; i--) {
			if (m_blockStack[i].Kind != SETUP_LOOP) {
				return m_blockStack[i];
			}
		}
		assert(FALSE);
		return BlockInfo();
	}

	void mark_offset_label(int index) {
		auto existingLabel = offsetLabels.find(index);
		if (existingLabel != offsetLabels.end()) {
			il.mark_label(existingLabel->second);
		}
		else{
			auto label = il.define_label();
			offsetLabels[index] = label;
			il.mark_label(label);
		}

	}

	void PreProcess() {
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
				il.localloc();
				il.st_loc(sequenceTmp);

				sequenceLocals[i] = sequenceTmp;
			}
				break;
			case UNPACK_SEQUENCE:
				// we need a buffer for the slow case, but we need 
				// to avoid allocating it in loops.
				auto sequenceTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				il.ld_i(oparg * sizeof(void*));
				il.localloc();
				il.st_loc(sequenceTmp);

				sequenceLocals[i] = sequenceTmp;
				break;
			}
		}
	}

	PVOID CompileWorker() {
		int oparg;
		Label ok;

#ifdef DEBUG_TRACE
		il.push_ptr(PyUnicode_AsUTF8(code->co_name));
		il.emit_call(METHOD_DEBUG_TRACE);

		load_frame();
		il.emit_call(METHOD_DEBUG_DUMP_FRAME);
#endif

		auto raiseLabel = il.define_label();
		auto reraiseLabel = il.define_label();

		auto lasti = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
		load_frame();
		il.ld_i(offsetof(PyFrameObject, f_lasti));
		il.add();
		il.st_loc(lasti);

		load_frame();
		il.emit_call(METHOD_PY_PUSHFRAME);

		m_blockStack.push_back(BlockInfo(raiseLabel, reraiseLabel, Label(), -1, NOP));

		auto tb = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
		auto ehVal = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
		auto excType = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
		auto retValue = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
		auto retLabel = il.define_label();

		for (int i = 0; i < size; i++) {
#if DEBUG_TRACE
			//char * tmp = (char*)malloc(8);
			//sprintf_s(tmp, 8, "%d", i);
			//il.push_ptr(tmp);
			//il.emit_call(METHOD_DEBUG_TRACE);
#endif

			auto byte = byteCode[i];

			// See FOR_ITER for special handling of the offset label
			if (byte != FOR_ITER) {
				mark_offset_label(i);
			}

			// update f_lasti
			if (!can_skip_lasti_update(byteCode[i])) {
				il.ld_loc(lasti);
				il.ld_i(i);
				il.st_ind_i4();
			}

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
				decref();
				break;
			case DUP_TOP:
				il.dup();
				il.dup();
				incref();
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
				incref();
				il.ld_loc(second);
				incref();

				il.free_local(top);
				il.free_local(second);

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
					incref();
				}
					break;
				case PyCmp_IN:
					il.emit_call(METHOD_CONTAINS_TOKEN);
					break;
				case PyCmp_NOT_IN:
					il.emit_call(METHOD_NOTCONTAINS_TOKEN);
					break;
				case PyCmp_EXC_MATCH:
					il.emit_call(METHOD_COMPARE_EXCEPTIONS);
					break;
				default:
					il.ld_i(oparg);
					il.emit_call(METHOD_RICHCMP_TOKEN);
				}
			}
				break;
			case SETUP_LOOP:
				// offset is relative to end of current instruction
				m_blockStack.push_back(
					BlockInfo(
					m_blockStack.back().Raise,
					m_blockStack.back().ReRaise,
					m_blockStack.back().ErrorTarget,
					oparg + i + 1,
					SETUP_LOOP
					)
					);
				break;
			case BREAK_LOOP:
			case CONTINUE_LOOP:
				// if we have finally blocks we need to unwind through them...
				// used in exceptional case...
			{
				bool inFinally = false;
				int loopIndex = -1;
				for (int i = m_blockStack.size() - 1; i >= 0; i--) {
					if (m_blockStack[i].Kind == SETUP_LOOP) {
						// we found our loop, we don't need additional processing...
						loopIndex = i;
						break;
					}
					if (m_blockStack[i].Kind == SETUP_FINALLY) {
						// we need to run the finally before continuing to the loop...
						// That means we need to spill the stack, branch to the finally,
						// run it, and have the finally branch back to our oparg.
						// CPython handles this by pushing the opcode to continue at onto
						// the stack, and then pushing an integer value which indicates END_FINALLY
						// should continue execution.  Our END_FINALLY expects only a single value
						// on the stack, and we also need to preserve any loop variables.
						m_blockStack.data()[i].Flags |= byte == BREAK_LOOP ? BLOCK_BREAKS : BLOCK_CONTINUES;

						if (!inFinally) {
							// only emit the branch to the first finally, subsequent branches
							// to other finallys will be handled by the END_FINALLY code.  But we
							// need to mark those finallys as needing special handling.
							inFinally = true;
							il.ld_i4(byte == BREAK_LOOP ? BLOCK_BREAKS : BLOCK_CONTINUES);
							il.branch(BranchAlways, m_blockStack[i].ErrorTarget);
							if (byte == CONTINUE_LOOP) {
								m_blockStack.data()[i].ContinueOffset = oparg;
							}
						}
					}
				}

				if (!inFinally) {
					if (byte == BREAK_LOOP) {
						assert(loopIndex != -1);
						il.branch(BranchAlways, getOffsetLabel(m_blockStack[loopIndex].EndOffset));
					}
					else{
						il.branch(BranchAlways, getOffsetLabel(oparg));
					}
				}

			}
				break;
			case LOAD_BUILD_CLASS:
				load_frame();
				il.emit_call(METHOD_GETBUILDCLASS_TOKEN);
				check_error(i, "get build class");
				break;
			case JUMP_ABSOLUTE:
			{
				il.branch(BranchAlways, getOffsetLabel(oparg));
				break;
			}
			case JUMP_FORWARD:
				il.branch(BranchAlways, getOffsetLabel(oparg + i + 1));
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
				il.branch(BranchAlways, getOffsetLabel(oparg));

				il.mark_label(noJump);

				// dec ref because we're popping...
				il.ld_loc(tmp);
				decref();

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
				decref();

				il.branch(BranchAlways, getOffsetLabel(oparg));

				il.mark_label(noJump);
				decref();
			}
				break;
			case LOAD_NAME:
				load_frame();
				il.push_ptr(PyTuple_GetItem(code->co_names, oparg));
				il.emit_call(METHOD_LOADNAME_TOKEN);
				check_error(i, "load name");
				break;
			case STORE_ATTR:
			{
				auto globalName = PyTuple_GetItem(code->co_names, oparg);
				il.push_ptr(globalName);
			}
				il.emit_call(METHOD_STOREATTR_TOKEN);
				check_int_error(i);
				break;
			case DELETE_ATTR:
			{
				auto globalName = PyTuple_GetItem(code->co_names, oparg);
				il.push_ptr(globalName);
			}
				il.emit_call(METHOD_DELETEATTR_TOKEN);
				check_int_error(i);
				break;
			case LOAD_ATTR:
			{
				auto globalName = PyTuple_GetItem(code->co_names, oparg);
				il.push_ptr(globalName);
			}
				il.emit_call(METHOD_LOADATTR_TOKEN);
				check_error(i, "load attr");
				break;
			case STORE_GLOBAL:
				// value is on the stack
				load_frame();
				{
					auto globalName = PyTuple_GetItem(code->co_names, oparg);
					il.push_ptr(globalName);
				}
				il.emit_call(METHOD_STOREGLOBAL_TOKEN);
				check_int_error(i);
				break;
			case DELETE_GLOBAL:
				load_frame();
				{
					auto globalName = PyTuple_GetItem(code->co_names, oparg);
					il.push_ptr(globalName);
				}
				il.emit_call(METHOD_DELETEGLOBAL_TOKEN);
				check_int_error(i);
				break;

			case LOAD_GLOBAL:
				load_frame();
				{
					auto globalName = PyTuple_GetItem(code->co_names, oparg);
					il.push_ptr(globalName);
				}
				il.emit_call(METHOD_LOADGLOBAL_TOKEN);
				check_error(i, "load global");
				break;
			case LOAD_CONST:
				il.push_ptr(PyTuple_GetItem(code->co_consts, oparg));
				il.dup();
				incref();
				break;
			case STORE_NAME:
				load_frame();
				il.push_ptr(PyTuple_GetItem(code->co_names, oparg));
				il.emit_call(METHOD_STORENAME_TOKEN);
				//check_int_error(i); // TODO: Enable me
				break;
			case DELETE_NAME:
				load_frame();
				il.push_ptr(PyTuple_GetItem(code->co_names, oparg));
				il.emit_call(METHOD_DELETENAME_TOKEN);
				//check_int_error(i); // TODO: Enable me
				break;
			case DELETE_FAST:
			{
				//load_local(oparg);
				//il.load_null();
				//auto valueSet = il.define_label();

				//il.branch(BranchNotEqual, valueSet);
				//il.push_ptr(PyTuple_GetItem(code->co_varnames, oparg));
				//il.emit_call(METHOD_UNBOUND_LOCAL);
				//il.branch(BranchAlways, GetEHBlock().Raise);

				//il.mark_label(valueSet);
				load_frame();
				il.ld_i(offsetof(PyFrameObject, f_localsplus) + oparg * sizeof(size_t));
				il.add();
				il.load_null();
				il.st_ind_i();
				break;
			}
			case STORE_FAST:
				// TODO: Move locals out of the Python frame object and into real locals
			{
				auto valueTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				il.st_loc(valueTmp);

				load_frame();
				il.ld_i(offsetof(PyFrameObject, f_localsplus) + oparg * sizeof(size_t));
				il.add();

				il.ld_loc(valueTmp);

				il.st_ind_i();

				il.free_local(valueTmp);
			}
				break;
			case LOAD_FAST:
				/* PyObject *value = GETLOCAL(oparg); */
			{
				load_local(oparg);

				//auto valueSet = il.define_label();

				////// TODO: Remove this check for definitely assigned values (e.g. params w/ no dels, 
				////// locals that are provably assigned)
				//il.dup();
				//il.load_null();
				//il.branch(BranchNotEqual, valueSet);

				//il.pop();
				//il.push_ptr(PyTuple_GetItem(code->co_varnames, oparg));
				//il.emit_call(METHOD_UNBOUND_LOCAL);
				//il.branch(BranchAlways, GetEHBlock().Raise);

				//il.mark_label(valueSet);

				il.dup();
				incref();
			}
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
				check_error(i, "unapack sequence");

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
					il.add();
					il.ld_ind_i();
					il.dup();
					incref();
				}

				il.ld_loc(valueTmp);
				decref();

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
				check_error(i, "unpack seq ex");

				auto fastTmp = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				il.st_loc(fastTmp);

				// load the right hand side...
				auto tmpOpArg = oparg >> 8;
				while (tmpOpArg--) {
					il.ld_loc(remainderTmp);
					il.push_ptr((void*)(tmpOpArg * sizeof(size_t)));
					il.add();
					il.ld_ind_i();
					il.dup();
					incref();
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
					il.add();
					il.ld_ind_i();
					il.dup();
					incref();
				}

				il.ld_loc(valueTmp);
				decref();

				il.free_local(valueTmp);
				il.free_local(fastTmp);
				il.free_local(remainderTmp);
				il.free_local(listTmp);
			}
				break;
			case CALL_FUNCTION_VAR:
			case CALL_FUNCTION_KW:
			case CALL_FUNCTION_VAR_KW:
			{
				int na = oparg & 0xff;
				int nk = (oparg >> 8) & 0xff;
				int flags = (byte - CALL_FUNCTION) & 3;
				int n = na + 2 * nk;
				PyObject **pfunc, *func, **sp, *res;
#define CALL_FLAG_VAR 1
#define CALL_FLAG_KW 2
				auto varArgs = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				auto varKwArgs = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				if (flags & CALL_FLAG_KW) {
					// kw args dict is last on the stack, save it....
					il.st_loc(varKwArgs);
				}

				Local map;
				if (nk != 0) {
					// if we have keywords build the map, and then save them...
					build_map(nk);
					map = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
					il.st_loc(map);
				}

				if (flags & CALL_FLAG_VAR) {
					// then save var args...
					il.st_loc(varArgs);
				}

				// now we have the normal args (if any), and then the function
				// Build a tuple of the normal args...
				if (na != 0) {
					build_tuple(na);
				}
				else{
					il.load_null();
				}

				// If we have keywords load them or null
				if (nk != 0) {
					il.ld_loc(map);
				}
				else{
					il.load_null();
				}

				// If we have var args load them or null
				if (flags & CALL_FLAG_VAR) {
					il.ld_loc(varArgs);
				}
				else{
					il.load_null();
				}

				// If we have a kw dict, load it...
				if (flags & CALL_FLAG_KW) {
					il.ld_loc(varKwArgs);
				}
				else{
					il.load_null();
				}

				// finally emit the call to our helper...
				il.emit_call(METHOD_PY_FANCYCALL);
				check_error(i, "fancy call");

				il.free_local(varArgs);
				il.free_local(varKwArgs);
				if (nk != 0) {
					il.free_local(map);
				}
			}
				break;
			case CALL_FUNCTION:
			{
				int argCnt = oparg & 0xff;
				int kwArgCnt = (oparg >> 8) & 0xff;
				// Optimize for # of calls, and various call types...
				// Function is last thing on the stack...

				// target + args popped, result pushed
				if (kwArgCnt == 0) {
					switch (argCnt) {
					case 0: 
						il.emit_call(METHOD_CALL0_TOKEN); 
						check_error(i, "call 0"); break;
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
						build_tuple(argCnt);

						// target is on the stack already...
						il.emit_call(METHOD_CALLN_TOKEN);
						check_error(i, "call n");
						break;
					}
				}
				else{
					build_map(kwArgCnt);
					auto map = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
					il.st_loc(map);

					build_tuple(argCnt);
					il.ld_loc(map);
					il.emit_call(METHOD_CALLNKW_TOKEN);
					check_error(i, "call nkw");
					il.free_local(map);
					break;
				}
			}
				break;
			case BUILD_TUPLE:
				if (oparg == 0) {
					il.push_ptr(PyTuple_New(0));
				}
				else{
					build_tuple(oparg);
				}
				break;
			case BUILD_LIST:
				build_list(oparg);
				break;
			case BUILD_MAP:
				build_map(oparg);
				break;
#if PY_MINOR_VERSION == 5
			case STORE_MAP:
				return nullptr;
				// stack is map, key, value
				il.emit_call(METHOD_STOREMAP_TOKEN);
				break;
#endif
			case STORE_SUBSCR:
				// stack is value, container, index
				il.emit_call(METHOD_STORESUBSCR_TOKEN);
				check_int_error(i);
				break;
			case DELETE_SUBSCR:
				// stack is container, index
				il.emit_call(METHOD_DELETESUBSCR_TOKEN);
				check_int_error(i);
				break;
			case BUILD_SLICE:
				if (oparg != 3) {
					il.load_null();
				}
				il.emit_call(METHOD_BUILD_SLICE);
				break;
			case BUILD_SET:
				build_set(oparg);
				break;
			case UNARY_POSITIVE:
				il.emit_call(METHOD_UNARY_POSITIVE);
				break;
			case UNARY_NEGATIVE:
				il.emit_call(METHOD_UNARY_NEGATIVE);
				break;
			case UNARY_NOT: il.emit_call(METHOD_UNARY_NOT); check_error(i, "not"); break;
			case UNARY_INVERT: il.emit_call(METHOD_UNARY_INVERT); check_error(i, "invert");  break;
			case BINARY_SUBSCR: il.emit_call(METHOD_SUBSCR_TOKEN); check_error(i, "subscr"); break;
			case BINARY_ADD: il.emit_call(METHOD_ADD_TOKEN); check_error(i, "add"); break;
			case BINARY_TRUE_DIVIDE: il.emit_call(METHOD_DIVIDE_TOKEN); check_error(i, "true divide"); break;
			case BINARY_FLOOR_DIVIDE: il.emit_call(METHOD_FLOORDIVIDE_TOKEN); check_error(i, "floor divide"); break;
			case BINARY_POWER: il.emit_call(METHOD_POWER_TOKEN); check_error(i, "power"); break;
			case BINARY_MODULO: il.emit_call(METHOD_MODULO_TOKEN); check_error(i, "modulo"); break;
			case BINARY_MATRIX_MULTIPLY: il.emit_call(METHOD_MATRIX_MULTIPLY_TOKEN); check_error(i, "matrix multi"); break;
			case BINARY_LSHIFT: il.emit_call(METHOD_BINARY_LSHIFT_TOKEN); check_error(i, "lshift"); break;
			case BINARY_RSHIFT: il.emit_call(METHOD_BINARY_RSHIFT_TOKEN); check_error(i, "rshift"); break;
			case BINARY_AND: il.emit_call(METHOD_BINARY_AND_TOKEN); check_error(i, "and"); break;
			case BINARY_XOR: il.emit_call(METHOD_BINARY_XOR_TOKEN); check_error(i, "xor"); break;
			case BINARY_OR: il.emit_call(METHOD_BINARY_OR_TOKEN); check_error(i, "or"); break;
			case BINARY_MULTIPLY: il.emit_call(METHOD_MULTIPLY_TOKEN); check_error(i, "multiply"); break;
			case BINARY_SUBTRACT: il.emit_call(METHOD_SUBTRACT_TOKEN); check_error(i, "subtract"); break;
			case RETURN_VALUE:
			{
				il.st_loc(retValue);

				bool inFinally = false;
				for (int i = m_blockStack.size() - 1; i >= 0; i--) {
					if (m_blockStack[i].Kind == SETUP_FINALLY) {
						// we need to run the finally before returning...
						m_blockStack.data()[i].Flags |= BLOCK_RETURNS;

						if (!inFinally) {
							// Only emit the store and branch to the inner most finally, but
							// we need to mark all finallys as being capable of being returned
							// through.
							inFinally = true;
							il.ld_i4(BLOCK_RETURNS);
							il.branch(BranchAlways, m_blockStack[i].ErrorTarget);
						}
					}
				}

				if (!inFinally) {
#if DEBUG_TRACE
					char* tmp = (char*)malloc(100);
					sprintf_s(tmp, 100, "Returning %s...", PyUnicode_AsUTF8(code->co_name));
					il.push_ptr(tmp);
					il.emit_call(METHOD_DEBUG_TRACE);
#endif

					il.branch(BranchLeave, retLabel);
				}
			}
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

				load_frame();
				il.emit_call(METHOD_NEWFUNCTION_TOKEN);

				if (byte == MAKE_CLOSURE) {
					il.emit_call(METHOD_SET_CLOSURE);
				}
				if (num_annotations > 0 || kwdefaults > 0 || posdefaults > 0) {
					auto func = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
					il.st_loc(func);
					if (num_annotations > 0) {
						printf("Make function, annotations not supported\r\n");
						return nullptr;
					}
					if (kwdefaults > 0) {
						printf("Make function, kwdefaults not supported\r\n");
						return nullptr;
					}
					if (posdefaults > 0) {
						build_tuple(posdefaults);
						il.ld_loc(func);
						il.emit_call(METHOD_FUNC_SET_DEFAULTS);
						//check_int_error_leave();// TODO: enable me
					}
					il.ld_loc(func);
				}
				break;
			}
			case LOAD_DEREF:
				load_frame();
				il.ld_i4(oparg);
				//il.ld_i(offsetof(PyFrameObject, f_localsplus) + (code->co_nlocals + oparg) * sizeof(size_t));
				//il.add();
				//il.ld_ind_i();
				il.emit_call(METHOD_PYCELL_GET);
				check_error(i, "pycell get");
				break;
			case STORE_DEREF:
				load_frame();
				il.ld_i(offsetof(PyFrameObject, f_localsplus) + (code->co_nlocals + oparg) * sizeof(size_t));
				il.add();
				il.ld_ind_i();
				il.emit_call(METHOD_PYCELL_SET_TOKEN);
				break;
			case DELETE_DEREF:
				il.load_null();
				load_frame();
				il.ld_i(offsetof(PyFrameObject, f_localsplus) + (code->co_nlocals + oparg) * sizeof(size_t));
				il.add();
				il.ld_ind_i();
				il.emit_call(METHOD_PYCELL_SET_TOKEN);
				break;
			case LOAD_CLOSURE:
				load_frame();
				il.ld_i(offsetof(PyFrameObject, f_localsplus) + (code->co_nlocals + oparg) * sizeof(size_t));
				il.add();
				il.ld_ind_i();
				il.dup();
				incref();
				break;
			case GET_ITER:
				il.emit_call(METHOD_GETITER_TOKEN);
				break;
			case FOR_ITER:
			{
				// CPython always generates LOAD_FAST or a GET_ITER before a FOR_ITER.
				// Therefore we know that we always fall into a FOR_ITER when it is
				// initialized, and we branch back to it for the loop condition.  We
				// do this becaues keeping the value on the stack becomes problematic.
				// At the very least it requires that we spill the value out when we
				// are doing a "continue" in a for loop.

				// oparg is where to jump on break
				auto iterValue = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
				il.st_loc(iterValue);		// store the value...

				// now that we've saved the value into a temp we can mark the offset
				// label.
				mark_offset_label(i - 2);	// minus 2 removes our oparg

				il.ld_loc(iterValue);

				/*
				// TODO: It'd be nice to inline this...
				il.dup();
				LD_FIELD(PyObject, ob_type);
				LD_FIELD(PyTypeObject, tp_iternext);*/
				//il.push_ptr((void*)offsetof(PyObject, ob_type));
				//il.push_back(CEE_ADD);
				auto error = il.define_local(Parameter(CORINFO_TYPE_INT));
				il.ld_loca(error);

				il.emit_call(SIG_ITERNEXT_TOKEN);
				auto processValue = il.define_label();
				il.dup();
				il.push_ptr(nullptr);
				il.compare_eq();
				il.branch(BranchFalse, processValue);

				// iteration has ended, or an exception was raised...
				il.pop();
				il.ld_loc(error);
				check_int_error(i);

				il.branch(BranchAlways, getOffsetLabel(i + oparg + 1));

				// leave iter and value on stack
				il.mark_label(processValue);
				il.free_local(error);
			}
				break;
			case SET_ADD:
			{
				// due to FOR_ITER magic we store the
				// iterable off the stack, and oparg here is based upon the stacking
				// of the generator indexes, so we don't need to spill anything...
				il.emit_call(METHOD_SET_ADD_TOKEN);
				check_error(i, "set add");
			}
				break;
			case MAP_ADD:
			{
				il.emit_call(METHOD_MAP_ADD_TOKEN);
				check_error(i, "map add");
			}
				break;
			case LIST_APPEND:
			{
				il.emit_call(METHOD_LIST_APPEND_TOKEN);
				check_error(i, "list append");
			}
				break;
			case INPLACE_POWER: il.emit_call(METHOD_INPLACE_POWER_TOKEN); check_error(i, "inplace power"); break;
			case INPLACE_MULTIPLY: il.emit_call(METHOD_INPLACE_MULTIPLY_TOKEN); check_error(i, "inplace multiply"); break;
			case INPLACE_MATRIX_MULTIPLY: il.emit_call(METHOD_INPLACE_MATRIX_MULTIPLY_TOKEN); check_error(i, "inplace matrix"); break;
			case INPLACE_TRUE_DIVIDE: il.emit_call(METHOD_INPLACE_TRUE_DIVIDE_TOKEN); check_error(i, "inplace true div"); break;
			case INPLACE_FLOOR_DIVIDE: il.emit_call(METHOD_INPLACE_FLOOR_DIVIDE_TOKEN); check_error(i, "inplace floor div"); break;
			case INPLACE_MODULO: il.emit_call(METHOD_INPLACE_MODULO_TOKEN); check_error(i, "inplace modulo"); break;
			case INPLACE_ADD:
				// TODO: We should do the unicode_concatenate ref count optimization
				il.emit_call(METHOD_INPLACE_ADD_TOKEN);
				check_error(i, "inplace add");
				break;
			case INPLACE_SUBTRACT: il.emit_call(METHOD_INPLACE_SUBTRACT_TOKEN); check_error(i, "inplace subtract"); break;
			case INPLACE_LSHIFT: il.emit_call(METHOD_INPLACE_LSHIFT_TOKEN); check_error(i, "inplace lshift"); break;
			case INPLACE_RSHIFT:il.emit_call(METHOD_INPLACE_RSHIFT_TOKEN); check_error(i, "inplace rshift"); break;
			case INPLACE_AND: il.emit_call(METHOD_INPLACE_AND_TOKEN); check_error(i, "inplace and"); break;
			case INPLACE_XOR:il.emit_call(METHOD_INPLACE_XOR_TOKEN); check_error(i, "inplace xor"); break;
			case INPLACE_OR: il.emit_call(METHOD_INPLACE_OR_TOKEN); check_error(i, "inplace or"); break;
			case PRINT_EXPR:
				il.emit_call(METHOD_PRINT_EXPR_TOKEN);
				check_int_error(i);
				break;
			case LOAD_CLASSDEREF:
				load_frame();
				il.ld_i(oparg);
				il.emit_call(METHOD_LOAD_CLASSDEREF_TOKEN);
				check_error(i, "class deref");
				break;
			case RAISE_VARARGS:
				// do raise (exception, cause)
				// We can be invoked with no args (bare raise), raise exception, or raise w/ exceptoin and cause
				switch (oparg) {
				case 0: il.load_null();
				case 1: il.load_null();
				case 2:
#if DEBUG_TRACE
					char* tmp = (char*)malloc(100);
					sprintf_s(tmp, 100, "Exception explicitly raised in %s", PyUnicode_AsUTF8(code->co_name));
					il.push_ptr(tmp);
					il.emit_call(METHOD_DEBUG_TRACE);
#endif

					// raise exc
					il.emit_call(METHOD_DO_RAISE);
					// returns 1 if we're doing a re-raise in which case we don't need
					// to update the traceback.  Otherwise returns 0.
					auto curHandler = GetEHBlock();
					if (oparg == 0) {
						il.branch(BranchFalse, curHandler.Raise);
						il.branch(BranchAlways, curHandler.ReRaise);
					}
					else {
						// if we have args we'll always return 0...
						il.pop();
						il.branch(BranchAlways, curHandler.Raise);
					}
					break;
				}
				break;
			case SETUP_EXCEPT:
			{
				auto handlerLabel = getOffsetLabel(oparg + i + 1);
				auto blockInfo = BlockInfo(il.define_label(), il.define_label(), handlerLabel, oparg + i + 1, SETUP_EXCEPT);
				m_blockStack.push_back(blockInfo);
				allHandlers.push_back(blockInfo);
			}
				break;
			case SETUP_FINALLY: {
				auto handlerLabel = getOffsetLabel(oparg + i + 1);
				auto blockInfo = BlockInfo(il.define_label(), il.define_label(), handlerLabel, oparg + i + 1, SETUP_FINALLY);
				m_blockStack.push_back(blockInfo);
				allHandlers.push_back(blockInfo);
			}
				break;
			case POP_EXCEPT:
				// we made it to the end of an EH block w/o throwing,
				// clear the exception.
				il.load_null();
				il.st_loc(ehVal);
				il.emit_call(METHOD_PYERR_CLEAR);
#if DEBUG_TRACE
				{
					char* tmp = (char*)malloc(100);
					sprintf_s(tmp, 100, "Exception cleared %d", i);
					il.push_ptr(tmp);
					il.emit_call(METHOD_DEBUG_TRACE);
				}
#endif
				break;
			case POP_BLOCK:
			{
				auto curHandler = m_blockStack.back();
				m_blockStack.pop_back();
				if (curHandler.Kind == SETUP_FINALLY) {
					// convert block into an END_FINALLY BlockInfo which will
					// dispatch to all of the previous locations...
					auto back = m_blockStack.back();
					m_blockStack.push_back(
						BlockInfo(
						back.Raise,		// if we take a nested exception this is where we go to...
						back.ReRaise,
						back.ErrorTarget,
						back.EndOffset,
						END_FINALLY,
						curHandler.Flags,
						curHandler.ContinueOffset
						)
						);
				}
			}
				break;
			case END_FINALLY:
				// CPython excepts END_FINALLY can be entered in 1 of 3 ways:
				//	1) With a status code for why the finally is unwinding, indicating a RETURN
				//			or a continue.  In this case there is an extra retval on the stack
				//	2) With an excpetion class which is being raised.  In this case there are 2 extra
				//			values on the stack, the exception value, and the traceback.
				//	3) After the try block has completed normally.  In this case None is on the stack.
				//
				//	That means in CPython this opcode can be branched to with 1 of 3 different stack
				//		depths, and the CLR doesn't like that.  Worse still the rest of the generated
				//		byte code assumes this is true.  For case 2 an except handler includes tests
				//		and pops which remove the 3 values from the class.  For case 3 the byte code
				//		at the end of the finally range includes the opcode to load None.
				//
				//  END_FINALLY can also be encountered w/o a SETUP_FINALLY, as happens when it's used
				//	solely for re-throwing exceptions.

				if (m_blockStack.back().Kind == END_FINALLY) {
					int flags = m_blockStack.back().Flags;
					int continues = m_blockStack.back().ContinueOffset;

					m_blockStack.pop_back();

					// We're actually ending a finally.  If we're in an exceptional case we
					// need to re-throw, otherwise we need to just continue execution.  Our
					// exception handling code will only push the exception type on in this case.
					auto finallyReason = il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
					auto noException = il.define_label();
					il.st_loc(finallyReason);
					il.ld_loc(finallyReason);
					il.push_ptr(Py_None);
					il.branch(BranchEqual, noException);

					if (flags & BLOCK_BREAKS) {
						for (int i = m_blockStack.size() - 1; i >= 0; i--) {
							if (m_blockStack[i].Kind == SETUP_LOOP) {
								il.ld_loc(finallyReason);
								il.ld_i(BLOCK_BREAKS);
								il.branch(BranchEqual, getOffsetLabel(m_blockStack[i].EndOffset));
								break;
							}
							else if (m_blockStack[i].Kind == SETUP_FINALLY) {
								// need to dispatch to outer finally...
								il.ld_loc(finallyReason);
								il.branch(BranchAlways, m_blockStack[i].ErrorTarget);
								break;
							}
						}
					}

					if (flags & BLOCK_CONTINUES) {
						for (int i = m_blockStack.size() - 1; i >= 0; i--) {
							if (m_blockStack[i].Kind == SETUP_LOOP) {
								il.ld_loc(finallyReason);
								il.ld_i(BLOCK_CONTINUES);
								il.branch(BranchEqual, getOffsetLabel(m_blockStack[i].ContinueOffset));
								break;
							}
							else if (m_blockStack[i].Kind == SETUP_FINALLY) {
								// need to dispatch to outer finally...
								il.ld_loc(finallyReason);
								il.branch(BranchAlways, m_blockStack[i].ErrorTarget);
								break;
							}
						}
					}

					if (flags & BLOCK_RETURNS) {
						auto exceptional = il.define_label();
						il.ld_loc(finallyReason);
						il.ld_i(BLOCK_RETURNS);
						il.compare_eq();
						il.branch(BranchFalse, exceptional);

						bool hasOuterFinally = false;
						for (int i = m_blockStack.size() - 1; i >= 0; i--) {
							if (m_blockStack[i].Kind == SETUP_FINALLY) {
								// need to dispatch to outer finally...
								il.ld_loc(finallyReason);
								il.branch(BranchAlways, m_blockStack[i].ErrorTarget);
								hasOuterFinally = true;
								break;
							}
						}
						if (!hasOuterFinally) {
							il.branch(BranchLeave, retLabel);
						}

						il.mark_label(exceptional);
					}

					// re-raise the exception...
					il.ld_loc(tb);
					il.ld_loc(ehVal);
					il.ld_loc(finallyReason);
					il.emit_call(METHOD_PYERR_RESTORE);
					il.branch(BranchAlways, GetEHBlock().ReRaise);

					il.mark_label(noException);
#ifdef DEBUG_TRACE
					il.push_ptr("finally exited normally...");
					il.emit_call(METHOD_DEBUG_TRACE);
#endif

					il.free_local(finallyReason);
				}
				else{
					// END_FINALLY is marking the EH rethrow
					il.setStackDepth(3);
					il.emit_call(METHOD_PYERR_RESTORE);
					il.branch(BranchAlways, GetEHBlock().ReRaise);
				}
				break;

			case YIELD_FROM:
			case YIELD_VALUE:
				printf("Unsupported opcode: %d (yield related)\r\n", byte);
				//_ASSERT(FALSE);
				return nullptr;

			case IMPORT_NAME:
				il.push_ptr(PyTuple_GetItem(code->co_names, oparg));
				load_frame();
				il.emit_call(METHOD_PY_IMPORTNAME);
				check_error(i, "import name");
				break;
			case IMPORT_FROM:
				il.dup();
				il.push_ptr(PyTuple_GetItem(code->co_names, oparg));
				il.emit_call(METHOD_PY_IMPORTFROM);
				check_error(i, "import from");
				break;
			case IMPORT_STAR:
				printf("Unsupported opcode: %d (import related)\r\n", byte);
				//_ASSERT(FALSE);
				return nullptr;

			case SETUP_WITH:
			case WITH_CLEANUP_START:
			case WITH_CLEANUP_FINISH:
			default:
				printf("Unsupported opcode: %d (with related)\r\n", byte);
				//_ASSERT(FALSE);
				return nullptr;
			}


		}


		// for each exception handler we need to load the exception
		// information onto the stack, and then branch to the correct
		// handler.  When we take an error we'll branch down to this
		// little stub and then back up to the correct handler.
		if (allHandlers.size() != 0) {
			for (int i = 0; i < allHandlers.size(); i++) {
				il.mark_label(allHandlers[i].Raise);
#ifdef DEBUG_TRACE
				il.push_ptr("Exception raised");
				il.emit_call(METHOD_DEBUG_TRACE);
#endif

				load_frame();
				il.emit_call(METHOD_EH_TRACE);

				il.mark_label(allHandlers[i].ReRaise);
#ifdef DEBUG_TRACE
				il.push_ptr("Exception reraised");
				il.emit_call(METHOD_DEBUG_TRACE);
#endif
				
				il.ld_loca(excType);
				il.ld_loca(ehVal);
				il.ld_loca(tb);
				il.emit_call(METHOD_PREPARE_EXCEPTION);
				if (allHandlers[i].Kind != SETUP_FINALLY) {
					il.ld_loc(tb);
					il.ld_loc(ehVal);
				}
				il.ld_loc(excType);
				il.branch(BranchAlways, allHandlers[i].ErrorTarget);
			}
		}

		// label we branch to for error handling when we have no EH handlers, return NULL.
		il.mark_label(raiseLabel);
#ifdef DEBUG_TRACE
		il.push_ptr("End raise exception ");
		il.emit_call(METHOD_DEBUG_TRACE);

		load_frame();
		il.emit_call(METHOD_EH_TRACE);
#endif
		il.mark_label(reraiseLabel);

#if DEBUG_TRACE
		char* tmp = (char*)malloc(100);
		sprintf_s(tmp, 100, "Re-raising exception %s", PyUnicode_AsUTF8(code->co_name));
		il.push_ptr(tmp);
		il.emit_call(METHOD_DEBUG_TRACE);
#endif

		il.load_null();
		auto finalRet = il.define_label();
		il.branch(BranchAlways, finalRet);

		il.mark_label(retLabel);
		il.ld_loc(retValue);

		il.mark_label(finalRet);
		load_frame();
		il.emit_call(METHOD_PY_POPFRAME);
		il.emit_call(METHOD_PY_CHECKFUNCTIONRESULT);
		il.ret();
		
		return il.compile(&g_corJitInfo, g_jit, code->co_stacksize + 100).m_addr;
	}

	void debugLog(const char* fmt, va_list args) {
	}

};

extern "C" __declspec(dllexport) PVOID JitCompile(PyCodeObject* code) {
	if (code->co_stacksize > 200) {
		// TODO: Remove me, currently we can't compile encodings\aliases.py.
		return nullptr;
	}
#ifdef DEBUG_TRACE
	static int compileCount = 0, failCount = 0;
	printf("Compiling %s from %s line %d #%d (%d failures so far)\r\n", 
		PyUnicode_AsUTF8(code->co_name), 
		PyUnicode_AsUTF8(code->co_filename), 
		code->co_firstlineno,
		++compileCount,
		failCount);
#endif
	Jitter jitter(code);
	auto res = jitter.Compile();
#ifdef DEBUG_TRACE
	if (res == nullptr) {
		printf("Compilation failure #%d\r\n", ++failCount);
	}
#endif
	return res;
}

//VTableInfo g_iterNextVtable{ 2, { offsetof(PyObject, ob_type), offsetof(PyTypeObject, tp_iternext) } };

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
		CORINFO_TYPE_INT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoStoreMap
		);
	g_module->m_methods[METHOD_STORESUBSCR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_INT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoStoreSubscr
		);
	g_module->m_methods[METHOD_DELETESUBSCR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_INT,
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
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoCellGet
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
	g_module->m_methods[METHOD_CALLNKW_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&CallNKW
		);
	g_module->m_methods[METHOD_STOREGLOBAL_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_INT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&StoreGlobal
		);
	g_module->m_methods[METHOD_DELETEGLOBAL_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_INT,
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
		CORINFO_TYPE_INT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&StoreAttr
		);
	g_module->m_methods[METHOD_DELETEATTR_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_INT,
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
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT)  },
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
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
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
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoListAppend
		);
	g_module->m_methods[METHOD_SET_ADD_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > { Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoSetAdd
		);

	g_module->m_methods[METHOD_MAP_ADD_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
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
		CORINFO_TYPE_INT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoPrintExpr
		);

	g_module->m_methods[METHOD_LOAD_CLASSDEREF_TOKEN] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoLoadClassDeref
		);

	g_module->m_methods[METHOD_PREPARE_EXCEPTION] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoPrepareException
		);

	g_module->m_methods[METHOD_DO_RAISE] = Method(
		nullptr,
		CORINFO_TYPE_INT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoRaise
		);

	g_module->m_methods[METHOD_EH_TRACE] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoEhTrace
		);

	g_module->m_methods[METHOD_COMPARE_EXCEPTIONS] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoCompareExceptions
		);

	g_module->m_methods[METHOD_UNBOUND_LOCAL] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoUnboundLocal
		);

	g_module->m_methods[METHOD_PYERR_RESTORE] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoPyErrRestore
		);


	g_module->m_methods[METHOD_DEBUG_TRACE] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoDebugTrace
		);
	
	g_module->m_methods[METHOD_FUNC_SET_DEFAULTS] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoFunctionSetDefaults
		);

	g_module->m_methods[METHOD_DEBUG_DUMP_FRAME] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoDebugDumpFrame
		);

	g_module->m_methods[METHOD_PY_POPFRAME] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoPopFrame
		);

	g_module->m_methods[METHOD_PY_PUSHFRAME] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT) },
		&DoPushFrame
		);

	g_module->m_methods[METHOD_PYERR_CLEAR] = Method(
		nullptr,
		CORINFO_TYPE_VOID,
		std::vector < Parameter > {},
		&PyErr_Clear
		);

	g_module->m_methods[METHOD_PY_CHECKFUNCTIONRESULT] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT)},
		&DoCheckFunctionResult
		);
	
	g_module->m_methods[METHOD_PY_IMPORTNAME] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT)},
		&DoImportName
		);

	
	g_module->m_methods[METHOD_PY_FANCYCALL] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT),
								Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT)},
		&DoFancyCall
	);

	g_module->m_methods[METHOD_PY_IMPORTFROM] = Method(
		nullptr,
		CORINFO_TYPE_NATIVEINT,
		std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT)},
		&DoImportFrom
		);


	//g_module->m_methods[SIG_ITERNEXT_TOKEN] = Method(
	//	nullptr,
	//	CORINFO_TYPE_NATIVEINT,
	//	std::vector < Parameter > { },
	//	nullptr,
	//	&g_iterNextVtable
	//);
}

