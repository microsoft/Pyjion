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
*
* Portions lifted from CPython under the PSF license.
*/

#include "intrins.h"
#include "taggedptr.h"
#include <cstdint>

#ifdef _MSC_VER

#include <safeint.h>
using namespace msl::utilities;

#endif

//#define DEBUG_TRACE
PyObject* g_emptyTuple;

#define NAME_ERROR_MSG \
    "name '%.200s' is not defined"

#define UNBOUNDLOCAL_ERROR_MSG \
    "local variable '%.200s' referenced before assignment"
#define UNBOUNDFREE_ERROR_MSG \
    "free variable '%.200s' referenced before assignment" \
    " in enclosing scope"

static void
format_exc_check_arg(PyObject *exc, const char *format_str, PyObject *obj) {
    const char *obj_str;

    if (!obj)
        return;

    obj_str = _PyUnicode_AsString(obj);
    if (!obj_str)
        return;

    PyErr_Format(exc, format_str, obj_str);
}

static void
format_exc_unbound(PyCodeObject *co, int oparg) {
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

PyObject* PyJit_Add(PyObject *left, PyObject *right) {
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

PyObject* PyJit_Subscr(PyObject *left, PyObject *right) {
    auto res = PyObject_GetItem(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_RichCompare(PyObject *left, PyObject *right, int op) {
    auto res = PyObject_RichCompare(left, right, op);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

int PyJit_RichEquals_Str(PyObject *left, PyObject *right, void**state) {
    int res;
    if (PyUnicode_CheckExact(left) && PyUnicode_CheckExact(right)) {
        res = PyUnicode_Compare(left, right) == 0;
    }
    else {
        return PyJit_RichEquals_Generic(left, right, state);
    }

    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

// taken from longobject.c:
static int
long_compare(PyLongObject *a, PyLongObject *b) {
    Py_ssize_t sign;

    if (Py_SIZE(a) != Py_SIZE(b)) {
        sign = Py_SIZE(a) - Py_SIZE(b);
    }
    else {
        Py_ssize_t i = Py_ABS(Py_SIZE(a));
        while (--i >= 0 && a->ob_digit[i] == b->ob_digit[i])
            ;
        if (i < 0)
            sign = 0;
        else {
            sign = (sdigit)a->ob_digit[i] - (sdigit)b->ob_digit[i];
            if (Py_SIZE(a) < 0)
                sign = -sign;
        }
    }
    return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}

int PyJit_RichEquals_Long(PyObject *left, PyObject *right, void**state) {
    int res;
    if (PyLong_CheckExact(left) && PyLong_CheckExact(right)) {
        res = long_compare((PyLongObject*)left, (PyLongObject*)right) == 0;
    }
    else {
        return PyJit_RichEquals_Generic(left, right, state);
    }

    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}


int PyJit_RichEquals_Generic(PyObject *left, PyObject *right, void**state) {
    if (left->ob_type == right->ob_type) {
        if (PyUnicode_CheckExact(left)) {
            *state = &PyJit_RichEquals_Str;
            return PyJit_RichEquals_Str(left, right, state);
        }
        else if (PyLong_CheckExact(left)) {
            *state = &PyJit_RichEquals_Long;
            return PyJit_RichEquals_Long(left, right, state);
        }
    }

    // Different types, or no optimization for these types...
    auto res = PyObject_RichCompare(left, right, Py_EQ);
    Py_DECREF(left);
    Py_DECREF(right);
    if (res == nullptr) {
        return -1;
    }
    int isTrue = PyObject_IsTrue(res);
    Py_DECREF(res);
    return isTrue;
}


PyObject* PyJit_Contains(PyObject *left, PyObject *right) {
    auto res = PySequence_Contains(right, left);
    Py_DECREF(left);
    Py_DECREF(right);
    if (res < 0) {
        return nullptr;
    }
    auto ret = res ? Py_True : Py_False;
    Py_INCREF(ret);
    return ret;
}

PyObject* PyJit_NotContains(PyObject *left, PyObject *right) {
    auto res = PySequence_Contains(right, left);
    Py_DECREF(left);
    Py_DECREF(right);
    if (res < 0) {
        return nullptr;
    }
    auto ret = res ? Py_False : Py_True;
    Py_INCREF(ret);
    return ret;
}

int PyJit_Contains_Int(PyObject *left, PyObject *right) {
    auto res = PySequence_Contains(right, left);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}


int PyJit_NotContains_Int(PyObject *left, PyObject *right) {
    auto res = PySequence_Contains(right, left);
    Py_DECREF(left);
    Py_DECREF(right);
    if (res < 0) {
        return -1;
    }
    return res ? 0 : 1;
}


PyObject* PyJit_CellGet(PyFrameObject* frame, size_t index) {
    PyObject** cells = frame->f_localsplus + frame->f_code->co_nlocals;
    PyObject *value = PyCell_GET(cells[index]);

    if (value == nullptr) {
        format_exc_unbound(frame->f_code, (int)index);
    }
    else {
        Py_INCREF(value);
    }
    return value;
}

PyObject* PyJit_NewFunction(PyObject* code, PyObject* qualname, PyFrameObject* frame) {
    auto res = PyFunction_NewWithQualName(code, frame->f_globals, qualname);
    Py_DECREF(code);
    Py_DECREF(qualname);

    return res;
}

PyObject* PyJit_SetClosure(PyObject* closure, PyObject* func) {
    PyFunction_SetClosure(func, closure);
    Py_DECREF(closure);
    return func;
}

PyObject* PyJit_BuildSlice(PyObject* start, PyObject* stop, PyObject* step) {
    auto slice = PySlice_New(start, stop, step);
    Py_DECREF(start);
    Py_DECREF(stop);
    Py_XDECREF(step);
    return slice;
}

PyObject* PyJit_UnaryPositive(PyObject* value) {
    auto res = PyNumber_Positive(value);
    Py_DECREF(value);
    return res;
}

PyObject* PyJit_UnaryNegative(PyObject* value) {
    auto res = PyNumber_Negative(value);
    Py_DECREF(value);
    return res;
}

PyObject* PyJit_UnaryNot(PyObject* value) {
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

int PyJit_UnaryNot_Int(PyObject* value) {
    int err = PyObject_IsTrue(value);
    Py_DECREF(value);
    if (err < 0) {
        return -1;
    }
    return err ? 0 : 1;
}

PyObject* PyJit_UnaryInvert(PyObject* value) {
    auto res = PyNumber_Invert(value);
    Py_DECREF(value);
    return res;
}

PyObject* PyJit_ListAppend(PyObject* list, PyObject* value) {
    int err = PyList_Append(list, value);
    Py_DECREF(value);
    if (err) {
        return nullptr;
    }
    return list;
}

PyObject* PyJit_SetAdd(PyObject* set, PyObject* value) {
    int err = PySet_Add(set, value);
    Py_DECREF(value);
    if (err) {
        return nullptr;
    }
    return set;
}

int PyJit_UpdateSet(PyObject* set, PyObject* value) {
    assert(PyAnySet_CheckExact(set));
    auto res = _PySet_Update(set, value);
    Py_DECREF(value);
    return res;
}

PyObject* PyJit_MapAdd(PyObject*map, PyObject* value, PyObject*key) {
    int err = PyDict_SetItem(map, key, value);  /* v[w] = u */
    Py_DECREF(value);
    Py_DECREF(key);
    if (err) {
        return nullptr;
    }
    return map;
}

PyObject* PyJit_Multiply(PyObject *left, PyObject *right) {
    auto res = PyNumber_Multiply(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_TrueDivide(PyObject *left, PyObject *right) {
    auto res = PyNumber_TrueDivide(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_FloorDivide(PyObject *left, PyObject *right) {
    auto res = PyNumber_FloorDivide(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_Power(PyObject *left, PyObject *right) {
    auto res = PyNumber_Power(left, right, Py_None);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_Modulo(PyObject *left, PyObject *right) {
    auto res = PyUnicode_CheckExact(left) ?
        PyUnicode_Format(left, right) :
        PyNumber_Remainder(left, right);

    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_Subtract(PyObject *left, PyObject *right) {
    auto res = PyNumber_Subtract(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_MatrixMultiply(PyObject *left, PyObject *right) {
    auto res = PyNumber_MatrixMultiply(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_BinaryLShift(PyObject *left, PyObject *right) {
    auto res = PyNumber_Lshift(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_BinaryRShift(PyObject *left, PyObject *right) {
    auto res = PyNumber_Rshift(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_BinaryAnd(PyObject *left, PyObject *right) {
    auto res = PyNumber_And(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_BinaryXor(PyObject *left, PyObject *right) {
    auto res = PyNumber_Xor(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_BinaryOr(PyObject *left, PyObject *right) {
    auto res = PyNumber_Or(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplacePower(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlacePower(left, right, Py_None);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceMultiply(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceMultiply(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceMatrixMultiply(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceMatrixMultiply(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceTrueDivide(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceTrueDivide(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceFloorDivide(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceFloorDivide(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceModulo(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceRemainder(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceAdd(PyObject *left, PyObject *right) {
    PyObject* res;
    if (PyUnicode_CheckExact(left) && PyUnicode_CheckExact(right)) {
        PyUnicode_Append(&left, right);
        res = left;
    }
    else {
        res = PyNumber_InPlaceAdd(left, right);
        Py_DECREF(left);
    }
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceSubtract(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceSubtract(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceLShift(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceLshift(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceRShift(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceRshift(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceAnd(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceAnd(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceXor(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceXor(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

PyObject* PyJit_InplaceOr(PyObject *left, PyObject *right) {
    auto res = PyNumber_InPlaceOr(left, right);
    Py_DECREF(left);
    Py_DECREF(right);
    return res;
}

int PyJit_PrintExpr(PyObject *value) {
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
void PyJit_PrepareException(PyObject** exc, PyObject**val, PyObject** tb, PyObject** oldexc, PyObject**oldVal, PyObject** oldTb) {
    auto tstate = PyThreadState_GET();

    // we take ownership of these into locals...
    if (tstate->exc_type != nullptr) {
        *oldexc = tstate->exc_type;
    }
    else {
        *oldexc = Py_None;
        Py_INCREF(Py_None);
    }
    *oldVal = tstate->exc_value;
    *oldTb = tstate->exc_traceback;

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
    assert(PyExceptionInstance_Check(*val));
    tstate->exc_traceback = *tb;
    if (*tb == NULL)
        *tb = Py_None;
    Py_INCREF(*tb);
}

void PyJit_UnwindEh(PyObject*exc, PyObject*val, PyObject*tb) {
    auto tstate = PyThreadState_GET();
    assert(val == nullptr || PyExceptionInstance_Check(val));
    auto oldtb = tstate->exc_traceback;
    auto oldtype = tstate->exc_type;
    auto oldvalue = tstate->exc_value;
    tstate->exc_traceback = tb;
    tstate->exc_type = exc;
    tstate->exc_value = val;
    Py_XDECREF(oldtb);
    Py_XDECREF(oldtype);
    Py_XDECREF(oldvalue);
}

#define CANNOT_CATCH_MSG "catching classes that do not inherit from "\
                         "BaseException is not allowed"

PyObject* PyJit_CompareExceptions(PyObject*v, PyObject* w) {
    if (PyTuple_Check(w)) {
        Py_ssize_t i, length;
        length = PyTuple_Size(w);
        for (i = 0; i < length; i += 1) {
            PyObject *exc = PyTuple_GET_ITEM(w, i);
            if (!PyExceptionClass_Check(exc)) {
                PyErr_SetString(PyExc_TypeError,
                    CANNOT_CATCH_MSG);
                Py_DECREF(v);
                Py_DECREF(w);
                return NULL;
            }
        }
    }
    else {
        if (!PyExceptionClass_Check(w)) {
            PyErr_SetString(PyExc_TypeError,
                CANNOT_CATCH_MSG);
            Py_DECREF(v);
            Py_DECREF(w);
            return NULL;
        }
    }
    int res = PyErr_GivenExceptionMatches(v, w);
    Py_DECREF(v);
    Py_DECREF(w);
    v = res ? Py_True : Py_False;
    Py_INCREF(v);
    return v;
}

// Returns 2 on an error, 1 if the exceptions match, or 0 if they don't.
int PyJit_CompareExceptions_Int(PyObject*v, PyObject* w) {
    if (PyTuple_Check(w)) {
        Py_ssize_t i, length;
        length = PyTuple_Size(w);
        for (i = 0; i < length; i += 1) {
            PyObject *exc = PyTuple_GET_ITEM(w, i);
            if (!PyExceptionClass_Check(exc)) {
                PyErr_SetString(PyExc_TypeError,
                    CANNOT_CATCH_MSG);
                Py_DECREF(v);
                Py_DECREF(w);
                return -1;
            }
        }
    }
    else {
        if (!PyExceptionClass_Check(w)) {
            PyErr_SetString(PyExc_TypeError,
                CANNOT_CATCH_MSG);
            Py_DECREF(v);
            Py_DECREF(w);
            return -1;
        }
    }
    int res = PyErr_GivenExceptionMatches(v, w);
    Py_DECREF(v);
    Py_DECREF(w);
    return res ? 1 : 0;
}


void PyJit_UnboundLocal(PyObject* name) {
    format_exc_check_arg(
        PyExc_UnboundLocalError,
        UNBOUNDLOCAL_ERROR_MSG,
        name
        );
}

void PyJit_DebugTrace(char* msg) {
    puts(msg);
}

const char * ObjInfo(PyObject *obj);

void PyJit_PyErrRestore(PyObject*tb, PyObject*value, PyObject*exception) {
#ifdef DEBUG_TRACE
    printf("Restoring exception %p %s\r\n", exception, ObjInfo(exception));
    printf("Restoring value %p %s\r\n", value, ObjInfo(value));
    printf("Restoring tb %p %s\r\n", tb, ObjInfo(tb));
#endif
    if (exception == Py_None) {
        exception = nullptr;
    }
    PyErr_Restore(exception, value, tb);
}

PyObject* PyJit_ImportName(PyObject*level, PyObject*from, PyObject* name, PyFrameObject* f) {
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

PyObject* PyJit_ImportFrom(PyObject*v, PyObject* name) {
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

static int
import_all_from(PyObject *locals, PyObject *v) {
    _Py_IDENTIFIER(__all__);
    _Py_IDENTIFIER(__dict__);
    PyObject *all = _PyObject_GetAttrId(v, &PyId___all__);
    PyObject *dict, *name, *value;
    int skip_leading_underscores = 0;
    int pos, err;

    if (all == NULL) {
        if (!PyErr_ExceptionMatches(PyExc_AttributeError))
            return -1; /* Unexpected error */
        PyErr_Clear();
        dict = _PyObject_GetAttrId(v, &PyId___dict__);
        if (dict == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError))
                return -1;
            PyErr_SetString(PyExc_ImportError,
                "from-import-* object has no __dict__ and no __all__");
            return -1;
        }
        all = PyMapping_Keys(dict);
        Py_DECREF(dict);
        if (all == NULL)
            return -1;
        skip_leading_underscores = 1;
    }

    for (pos = 0, err = 0;; pos++) {
        name = PySequence_GetItem(all, pos);
        if (name == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_IndexError))
                err = -1;
            else
                PyErr_Clear();
            break;
        }

        // C++ doesn't like the asserts in these macros...
#undef PyUnicode_READY
#define PyUnicode_READY(op)                        \
     (PyUnicode_IS_READY(op) ?                          \
      0 : _PyUnicode_Ready((PyObject *)(op)))
#undef PyUnicode_KIND
#define PyUnicode_KIND(op) \
     ((PyASCIIObject *)(op))->state.kind

#undef PyUnicode_IS_ASCII
#define PyUnicode_IS_ASCII(op)                   \
     ((PyASCIIObject*)op)->state.ascii

#undef _PyUnicode_NONCOMPACT_DATA
#define _PyUnicode_NONCOMPACT_DATA(op)                  \
     ((((PyUnicodeObject *)(op))->data.any))

#undef PyUnicode_DATA
#define PyUnicode_DATA(op) \
     PyUnicode_IS_COMPACT(op) ? _PyUnicode_COMPACT_DATA(op) :   \
     _PyUnicode_NONCOMPACT_DATA(op)

#undef PyUnicode_READ_CHAR
#define PyUnicode_READ_CHAR(unicode, index) \
    ((Py_UCS4)                                  \
        (PyUnicode_KIND((unicode)) == PyUnicode_1BYTE_KIND ? \
            ((const Py_UCS1 *)(PyUnicode_DATA((unicode))))[(index)] : \
            (PyUnicode_KIND((unicode)) == PyUnicode_2BYTE_KIND ? \
                ((const Py_UCS2 *)(PyUnicode_DATA((unicode))))[(index)] : \
                ((const Py_UCS4 *)(PyUnicode_DATA((unicode))))[(index)] \
            ) \
        ))

        if (skip_leading_underscores &&
            PyUnicode_Check(name) &&
            PyUnicode_READY(name) != -1 &&
            PyUnicode_READ_CHAR(name, 0) == '_') {
            Py_DECREF(name);
            continue;
        }
        value = PyObject_GetAttr(v, name);
        if (value == NULL)
            err = -1;
        else if (PyDict_CheckExact(locals))
            err = PyDict_SetItem(locals, name, value);
        else
            err = PyObject_SetItem(locals, name, value);
        Py_DECREF(name);
        Py_XDECREF(value);
        if (err != 0)
            break;
    }
    Py_DECREF(all);
    return err;
}

int PyJit_ImportStar(PyObject*from, PyFrameObject* f) {
    PyObject *locals;
    int err;
    if (PyFrame_FastToLocalsWithError(f) < 0)
        return 1;

    locals = f->f_locals;
    if (locals == NULL) {
        PyErr_SetString(PyExc_SystemError,
            "no locals found during 'import *'");
        return 1;
    }
    err = import_all_from(locals, from);
    PyFrame_LocalsToFast(f, 0);
    Py_DECREF(from);
    return err;
}

PyObject* PyJit_FancyCall(PyObject* func, PyObject*args, PyObject*kwargs, PyObject* stararg, PyObject* kwdict) {
    // any of these arguments can be null...
    PyObject *result = nullptr;
    size_t nstar;
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
        else {
            args = PyTuple_New(0);
        }
    }
    else if (stararg != nullptr) {
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

    result = PyObject_Call(func, args, kwargs);

ext_call_fail:
    Py_XDECREF(func);
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    Py_XDECREF(stararg);
    Py_XDECREF(kwdict);
    return result;
}

void PyJit_DebugDumpFrame(PyFrameObject* frame) {
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
            else {
                printf("Null local type?");
            }
        }
        else {
            printf("Null local?");
        }
    }
    _dumping = false;
}

void PyJit_PushFrame(PyFrameObject* frame) {
    PyThreadState_Get()->frame = frame;
}

void PyJit_PopFrame(PyFrameObject* frame) {
    PyThreadState_Get()->frame = frame->f_back;
}

int PyJit_FunctionSetDefaults(PyObject* defs, PyObject* func) {
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

int PyJit_FunctionSetAnnotations(PyObject* values, PyObject* names, PyObject* func) {
    PyObject *anns = PyDict_New();
    if (anns == NULL) {
        Py_DECREF(values);
        Py_DECREF(names);
        Py_DECREF(func);
        return 1;
    }

    auto name_ix = PyTuple_Size(names);
    while (name_ix > 0) {
        PyObject *name, *value;
        int err;
        --name_ix;
        name = PyTuple_GET_ITEM(names, name_ix);
        value = PyTuple_GET_ITEM(values, name_ix);
        err = PyDict_SetItem(anns, name, value);
        if (err != 0) {
            Py_DECREF(anns);
            Py_DECREF(func);
            Py_DECREF(values);
            return err;
        }
    }

    if (PyFunction_SetAnnotations(func, anns) != 0) {
        /* Can't happen unless
        PyFunction_SetAnnotations changes. */
        Py_DECREF(anns);
        Py_DECREF(func);
        Py_DECREF(values);
        return 1;
    }
    Py_DECREF(anns);
    Py_DECREF(names);
    Py_DECREF(values);
    return 0;
}

int PyJit_FunctionSetKwDefaults(PyObject* defs, PyObject* func) {
    if (PyFunction_SetKwDefaults(func, defs) != 0) {
        /* Can't happen unless
        PyFunction_SetKwDefaults changes. */
        Py_DECREF(func);
        Py_DECREF(defs);
        return 1;
    }
    Py_DECREF(defs);
    return 0;
}

void PyJit_EhTrace(PyFrameObject *f) {
    PyTraceBack_Here(f);
    
    //auto tstate = PyThreadState_GET();

    //if (tstate->c_tracefunc != NULL) {
    //	call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj,
    //		tstate, f);
    //}
}

int PyJit_Raise(PyObject *exc, PyObject *cause) {
    PyObject *type = NULL, *value = NULL;

    if (exc == NULL) {
        /* Reraise */
        PyThreadState *tstate = PyThreadState_GET();
        PyObject *tb;
        type = tstate->exc_type;
        value = tstate->exc_value;
        tb = tstate->exc_traceback;
        if (type == Py_None || type == nullptr) {
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

PyObject* PyJit_LoadClassDeref(PyFrameObject* frame, size_t oparg) {
    PyObject* value;
    PyCodeObject* co = frame->f_code;
    size_t idx = oparg - PyTuple_GET_SIZE(co->co_cellvars);
    assert(idx >= 0 && idx < ((size_t)PyTuple_GET_SIZE(co->co_freevars)));
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
            format_exc_unbound(co, (int)oparg);
            return nullptr;
        }
        Py_INCREF(value);
    }

    return value;
}

int PyJit_ExtendList(PyObject *list, PyObject *extension) {
    assert(PyList_CheckExact(list));
    auto res = _PyList_Extend((PyListObject*)list, extension);
    Py_DECREF(extension);
    int flag = 1;  // Assume error unless we prove to ourselves otherwise.
    if (res == Py_None) {
        flag = 0;
        Py_DECREF(res);
    }

    return flag;
}

PyObject* PyJit_ListToTuple(PyObject *list) {
    PyObject* res = PyList_AsTuple(list);
    Py_DECREF(list);
    return res;
}

int PyJit_StoreMap(PyObject *key, PyObject *value, PyObject* map) {
    assert(PyDict_CheckExact(map));
    auto res = PyDict_SetItem(map, key, value);
    Py_DECREF(key);
    Py_DECREF(value);
    return res;
}

int PyJit_DictUpdate(PyObject* dict, PyObject* other) {
    assert(PyDict_CheckExact(dict));
    auto res = PyDict_Update(dict, other);
    if (res < 0) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Format(PyExc_TypeError,
                "'%.200s' object is not a mapping",
                other->ob_type->tp_name);
        }
    }
    Py_DECREF(other);
    return res;
}

int PyJit_StoreSubscr(PyObject* value, PyObject *container, PyObject *index) {
    auto res = PyObject_SetItem(container, index, value);
    Py_DECREF(index);
    Py_DECREF(value);
    Py_DECREF(container);
    return res;
}

int PyJit_DeleteSubscr(PyObject *container, PyObject *index) {
    auto res = PyObject_DelItem(container, index);
    Py_DECREF(index);
    Py_DECREF(container);
    return res;
}

PyObject* PyJit_CallN(PyObject *target, PyObject* args) {
    // we stole references for the tuple...
    auto res = PyObject_Call(target, args, nullptr);
    Py_DECREF(target);
    Py_DECREF(args);
    return res;
}

PyObject* PyJit_CallNKW(PyObject *target, PyObject* args, PyObject* kwargs) {
    // we stole references for the tuple...
#ifdef DEBUG_TRACE
    printf("Target: %s\r\n", ObjInfo(target));

    printf("Tuple: %s\r\n", ObjInfo(args));

    printf("KW Args: %s\r\n", ObjInfo(kwargs));

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

int PyJit_StoreGlobal(PyObject* v, PyFrameObject* f, PyObject* name) {
    int err = PyDict_SetItem(f->f_globals, name, v);
    Py_DECREF(v);
    return err;
}

int PyJit_DeleteGlobal(PyFrameObject* f, PyObject* name) {
    return PyDict_DelItem(f->f_globals, name);
}

PyObject* PyJit_LoadGlobal(PyFrameObject* f, PyObject* name) {
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
    else {
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
            else {
                PyErr_Clear();
            }
        }
    }
    return v;
}

PyObject* PyJit_GetIter(PyObject* iterable) {
    auto res = PyObject_GetIter(iterable);
    Py_DECREF(iterable);
    return res;
}

typedef struct {
    PyObject_HEAD
        PyObject *start;
    PyObject *stop;
    PyObject *step;
    PyObject *length;
} rangeobject;

PyObject* PyJit_GetIterOptimized(PyObject* iterable, size_t* iterstate1, size_t* iterstate2) {
    //if (PyRange_Check(iterable)) {
    //	rangeobject* range = (rangeobject*)iterable;
    //	auto step = PyLong_AsSize_t(range->step);
    //	if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
    //		PyErr_Clear();		
    //		goto common;
    //	}
    //	else if (step == 1) {
    //		auto start  = PyLong_AsSize_t(range->start);
    //		if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
    //			PyErr_Clear();
    //			goto common;
    //		}
    //		auto end = PyLong_AsSize_t(range->start);
    //		if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
    //			PyErr_Clear();
    //			goto common;
    //		}

    //		// common iteration case
    //		*iterstate1 = start;
    //		*iterstate2 = end;
    //		return (PyObject*)1;
    //	}
    //}
//common:
    auto res = PyObject_GetIter(iterable);
    Py_DECREF(iterable);
    return res;
}

PyObject* PyJit_IterNext(PyObject* iter, int*error) {
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


PyObject* PyJit_IterNextOptimized(PyObject* iter, int*error, size_t* iterstate1, size_t* iterstate2) {
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

void PyJit_CellSet(PyObject* value, PyObject* cell) {
    PyCell_Set(cell, value);
    Py_DecRef(value);
}

PyObject* PyJit_BuildClass(PyFrameObject *f) {
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
PyObject** PyJit_UnpackSequenceEx(PyObject* seq, size_t leftSize, size_t rightSize, PyObject** tempStorage, PyObject** listRes, PyObject*** remainder) {
    if (PyTuple_CheckExact(seq) && ((size_t)PyTuple_GET_SIZE(seq)) >= (leftSize + rightSize)) {
        auto listSize = PyTuple_GET_SIZE(seq) - (leftSize + rightSize);
        auto list = (PyListObject*)PyList_New(listSize);
        if (list == nullptr) {
            return nullptr;
        }
        for (int i = 0; i < listSize; i++) {
            list->ob_item[i] = ((PyTupleObject *)seq)->ob_item[i + leftSize];

        }
        *remainder = ((PyTupleObject *)seq)->ob_item + leftSize + listSize;
        *listRes = (PyObject*)list;
        auto res = ((PyTupleObject *)seq)->ob_item;
        auto size = PyTuple_GET_SIZE(seq);
        for (int i = 0; i < size; i++) {
            Py_INCREF(res[i]);
        }
        return res;
    }
    else if (PyList_CheckExact(seq) && ((size_t)PyList_GET_SIZE(seq)) >= (leftSize + rightSize)) {
        auto listSize = PyList_GET_SIZE(seq) - (leftSize + rightSize);
        auto list = (PyListObject*)PyList_New(listSize);
        if (list == nullptr) {
            return nullptr;
        }
        for (int i = 0; i < listSize; i++) {
            list->ob_item[i] = ((PyListObject *)seq)->ob_item[i + leftSize];
        }
        *remainder = ((PyListObject *)seq)->ob_item + leftSize + listSize;
        *listRes = (PyObject*)list;

        auto res = ((PyListObject *)seq)->ob_item;
        auto size = PyTuple_GET_SIZE(seq);
        for (int i = 0; i < size; i++) {
            Py_INCREF(res[i]);
        }
        return res;
    }
    else {
        // the function allocated space on the stack for us to
        // store these temporarily.
        auto it = PyObject_GetIter(seq);
        if (it == nullptr) {
            goto Error;
        }

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
        else {

            auto l = PySequence_List(it);
            if (l == NULL)
                goto Error;
            *listRes = l;
            i++;

            size_t ll = PyList_GET_SIZE(l);
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
        for (int i = 0; i < leftSize; i++) {
            Py_XDECREF(tempStorage[i]);
        }
        Py_XDECREF(it);
        return nullptr;
    }
}

void PyJit_UnpackError(size_t expected, size_t got) {
    if (got < expected) {
        PyErr_Format(PyExc_ValueError,
            "need more than %d value%s to unpack",
            got, got == 1 ? "" : "s");
    }
    else if (got > expected) {
        PyErr_Format(PyExc_ValueError, "too many values to unpack "
            "(expected %d)", expected);
    }
}

// Unpacks the given sequence and returns a pointer to where the sequence
// is stored.  If this is a type we can just grab the array from it returns
// the array.  Otherwise we unpack the sequence into tempStorage which was
// allocated on the stack when we entered the generated method body.
PyObject** PyJit_UnpackSequence(PyObject* seq, size_t size, PyObject** tempStorage) {
    if (PyTuple_CheckExact(seq)) {
        if (PyTuple_GET_SIZE(seq) == size) {
            PyObject** res = ((PyTupleObject *)seq)->ob_item;
            for (int i = 0; i < size; i++) {
                Py_INCREF(res[i]);
            }
            return res;
        }

        PyJit_UnpackError(size, PyTuple_GET_SIZE(seq));
        return nullptr;
    }
    else if (PyList_CheckExact(seq)) {
        if (PyList_GET_SIZE(seq) == size) {
            PyObject** res = ((PyListObject *)seq)->ob_item;
            for (int i = 0; i < size; i++) {
                Py_INCREF(res[i]);
            }
            return res;
        }
        PyJit_UnpackError(size, PyList_GET_SIZE(seq));
        return nullptr;
    }

    return PyJit_UnpackSequenceEx(seq, size, 0, tempStorage, nullptr, nullptr);
}

PyObject* PyJit_LoadAttr(PyObject* owner, PyObject* name) {
    PyObject *res = PyObject_GetAttr(owner, name);
    Py_DECREF(owner);
#ifdef DEBUG_TRACE
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
    else if (obj->ob_type != nullptr) {
        return obj->ob_type->tp_name;
    }
    else {
        return "<null type>";
    }
}

int PyJit_StoreAttr(PyObject* value, PyObject* owner, PyObject* name) {
    int res = PyObject_SetAttr(owner, name, value);
    Py_DECREF(owner);
    Py_DECREF(value);
    return res;
}

int PyJit_DeleteAttr(PyObject* owner, PyObject* name) {
    int res = PyObject_DelAttr(owner, name);
    Py_DECREF(owner);
    return res;
}

PyObject* PyJit_LoadName(PyFrameObject* f, PyObject* name) {
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

int PyJit_StoreName(PyObject* v, PyFrameObject* f, PyObject* name) {
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

int PyJit_DeleteName(PyFrameObject* f, PyObject* name) {
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

static PyObject *
fast_function(PyObject *func, PyObject **pp_stack, int n) {
    PyCodeObject *co = (PyCodeObject *)PyFunction_GET_CODE(func);
    PyObject *globals = PyFunction_GET_GLOBALS(func);
    PyObject *argdefs = PyFunction_GET_DEFAULTS(func);

    if (argdefs == NULL && co->co_argcount == n &&
        co->co_kwonlyargcount == 0 &&
        co->co_flags == (CO_OPTIMIZED | CO_NEWLOCALS | CO_NOFREE)) {
        PyFrameObject *f;
        PyObject *retval = NULL;
        PyThreadState *tstate = PyThreadState_GET();
        PyObject **fastlocals, **stack;
        int i;

        assert(globals != NULL);
        /* XXX Perhaps we should create a specialized
        PyFrame_New() that doesn't take locals, but does
        take builtins without sanity checking them.
        */
        assert(tstate != NULL);
        f = PyFrame_New(tstate, co, globals, NULL);
        if (f == NULL)
            return NULL;

        fastlocals = f->f_localsplus;
        stack = pp_stack;

        for (i = 0; i < n; i++) {
            Py_INCREF(*stack);
            fastlocals[i] = *stack++;
        }
        retval = PyEval_EvalFrameEx(f, 0);
        ++tstate->recursion_depth;
        Py_DECREF(f);
        --tstate->recursion_depth;
        return retval;
    }

    auto args = PyTuple_New(n);
    if (args == nullptr) {
        return nullptr;
    }
    for (int i = 0; i < n; i++) {
        Py_INCREF(pp_stack[i]);
        PyTuple_SET_ITEM(args, i, pp_stack[i]);
    }
    auto res = PyObject_Call(func, args, nullptr);
    Py_DECREF(args);
    return res;
#if FALSE
    if (argdefs != NULL) {
        d = &PyTuple_GET_ITEM(argdefs, 0);
        nd = Py_SIZE(argdefs);
    }
    return _PyEval_EvalCodeWithName((PyObject*)co, globals,
        (PyObject *)NULL, (pp_stack)-n, na,
        (pp_stack)-2 * nk, nk, d, nd, kwdefs,
        PyFunction_GET_CLOSURE(func),
        name, qualname);
#endif
}

PyObject* Call0(PyObject *target) {
    PyObject* res;
    if (PyFunction_Check(target)) {
        PyObject* empty[1] = { nullptr };
        res = fast_function(target, empty, 0);
    }
    else if (PyCFunction_Check(target)) {
        res = PyCFunction_Call(target, g_emptyTuple, nullptr);
    }
    else if (PyMethod_Check(target) && PyMethod_GET_SELF(target) != NULL) {
        PyObject *self = PyMethod_GET_SELF(target);
        PyObject* func = PyMethod_GET_FUNCTION(target);
        Py_INCREF(self);
        Py_INCREF(func);
        res = Call1(func, self);
    }
    else {
        res = PyObject_Call(target, g_emptyTuple, nullptr);
    }
    Py_DECREF(target);
    return res;
}

PyObject* Call0_Function(PyObject *target, void** addr) {
    PyObject* res;
    if (PyFunction_Check(target)) {
        PyObject* empty[1] = { nullptr };
        res = fast_function(target, empty, 0);
    }
    else {
        return Call0_Generic(target, addr);
    }

    Py_DECREF(target);
    return res;
}

PyObject* Call0_Method(PyObject *target, void** addr) {
    PyObject* res;
    if (PyMethod_Check(target) && PyMethod_GET_SELF(target) != NULL) {
        PyObject *self = PyMethod_GET_SELF(target);
        PyObject* func = PyMethod_GET_FUNCTION(target);
        Py_INCREF(self);
        Py_INCREF(func);
        res = Call1(func, self);
    }
    else {
        return Call0_Generic(target, addr);
    }

    Py_DECREF(target);
    return res;
}

PyObject* Call0_CFunction(PyObject *target, void** addr) {
    PyObject* res;
    if (PyCFunction_Check(target)) {
        res = PyCFunction_Call(target, g_emptyTuple, nullptr);
    }
    else {
        return Call0_Generic(target, addr);
    }

    Py_DECREF(target);
    return res;
}


PyObject* Call0_Generic(PyObject *target, void** addr) {
    PyObject* res;
    if (PyFunction_Check(target)) {
        *addr = &Call0_Function;
        return Call0_Function(target, addr);
    }
    else if (PyCFunction_Check(target)) {
        *addr = Call0_CFunction;
        return Call0_CFunction(target, addr);
    }
    else if (PyMethod_Check(target) && PyMethod_GET_SELF(target) != NULL) {
        *addr = Call0_Method;
        return Call0_Method(target, addr);
    }
    else {
        res = PyObject_Call(target, g_emptyTuple, nullptr);
    }
    Py_DECREF(target);
    return res;
}


PyObject* Call1(PyObject *target, PyObject* arg0) {
    PyObject* res = nullptr;
    if (PyFunction_Check(target)) {
        PyObject* stack[1] = { arg0 };
        res = fast_function(target, stack, 1);
        Py_DECREF(arg0);
        goto error;
    }
    else if (PyMethod_Check(target) && PyMethod_GET_SELF(target) != NULL) {
        PyObject *self = PyMethod_GET_SELF(target);
        PyObject* func = PyMethod_GET_FUNCTION(target);
        Py_INCREF(self);
        Py_INCREF(func);
        res = Call2(func, self, arg0);
        goto error;
    }


    auto args = PyTuple_New(1);
    if (args == nullptr) {
        Py_DECREF(arg0);
        goto error;
    }
    PyTuple_SET_ITEM(args, 0, arg0);
    if (PyCFunction_Check(target)) {
        res = PyCFunction_Call(target, args, nullptr);
    }
    else {
        res = PyObject_Call(target, args, nullptr);
    }
    Py_DECREF(args);
error:
    Py_DECREF(target);
    return res;
}

PyObject* Call2(PyObject *target, PyObject* arg0, PyObject* arg1) {
    PyObject* res = nullptr;
    if (PyFunction_Check(target)) {
        PyObject* stack[2] = { arg0, arg1 };
        res = fast_function(target, stack, 2);
        Py_DECREF(arg0);
        Py_DECREF(arg1);
        goto error;
    }
    else if (PyMethod_Check(target) && PyMethod_GET_SELF(target) != NULL) {
        PyObject *self = PyMethod_GET_SELF(target);
        PyObject* func = PyMethod_GET_FUNCTION(target);
        Py_INCREF(self);
        Py_INCREF(func);
        res = Call3(func, self, arg0, arg1);
        goto error;
    }


    auto args = PyTuple_New(2);
    if (args == nullptr) {
        Py_DECREF(arg0);
        Py_DECREF(arg1);
        goto error;
    }
    PyTuple_SET_ITEM(args, 0, arg0);
    PyTuple_SET_ITEM(args, 1, arg1);
    if (PyCFunction_Check(target)) {
        res = PyCFunction_Call(target, args, nullptr);
    }
    else {
        res = PyObject_Call(target, args, nullptr);
    }
    Py_DECREF(args);
error:
    Py_DECREF(target);
    return res;
}

PyObject* Call3(PyObject *target, PyObject* arg0, PyObject* arg1, PyObject* arg2) {
    PyObject* res = nullptr;
    if (PyFunction_Check(target)) {
        PyObject* stack[3] = { arg0, arg1, arg2 };
        res = fast_function(target, stack, 3);
        Py_DECREF(arg0);
        Py_DECREF(arg1);
        Py_DECREF(arg2);
        goto error;
    }
    else if (PyMethod_Check(target) && PyMethod_GET_SELF(target) != NULL) {
        PyObject *self = PyMethod_GET_SELF(target);
        PyObject* func = PyMethod_GET_FUNCTION(target);
        Py_INCREF(self);
        Py_INCREF(func);
        res = Call4(func, self, arg0, arg1, arg2);
        goto error;
    }

    auto args = PyTuple_New(3);
    if (args == nullptr) {
        Py_DECREF(arg0);
        Py_DECREF(arg1);
        Py_DECREF(arg2);
        goto error;
    }
    PyTuple_SET_ITEM(args, 0, arg0);
    PyTuple_SET_ITEM(args, 1, arg1);
    PyTuple_SET_ITEM(args, 2, arg2);
    if (PyCFunction_Check(target)) {
        res = PyCFunction_Call(target, args, nullptr);
    }
    else {
        res = PyObject_Call(target, args, nullptr);
    }
    Py_DECREF(args);
error:
    Py_DECREF(target);
    return res;
}

PyObject* Call4(PyObject *target, PyObject* arg0, PyObject* arg1, PyObject* arg2, PyObject* arg3) {
    PyObject* res = nullptr;
    if (PyFunction_Check(target)) {
        PyObject* stack[4] = { arg0, arg1, arg2, arg3 };
        res = fast_function(target, stack, 4);
        Py_DECREF(arg0);
        Py_DECREF(arg1);
        Py_DECREF(arg2);
        Py_DECREF(arg3);
        goto error;
    }

    auto args = PyTuple_New(4);
    if (args == nullptr) {
        Py_DECREF(arg0);
        Py_DECREF(arg1);
        Py_DECREF(arg2);
        Py_DECREF(arg3);
        goto error;
    }
    PyTuple_SET_ITEM(args, 0, arg0);
    PyTuple_SET_ITEM(args, 1, arg1);
    PyTuple_SET_ITEM(args, 2, arg2);
    PyTuple_SET_ITEM(args, 3, arg3);
    if (PyCFunction_Check(target)) {
        res = PyCFunction_Call(target, args, nullptr);
    }
    else {
        res = PyObject_Call(target, args, nullptr);
    }
    Py_DECREF(args);
error:
    Py_DECREF(target);
    return res;
}

PyObject* PyJit_Is(PyObject* lhs, PyObject* rhs) {
    auto res = lhs == rhs ? Py_True : Py_False;
    Py_DECREF(lhs);
    Py_DECREF(rhs);
    Py_INCREF(res);
    return res;
}

PyObject* PyJit_IsNot(PyObject* lhs, PyObject* rhs) {
    auto res = lhs == rhs ? Py_False : Py_True;
    Py_DECREF(lhs);
    Py_DECREF(rhs);
    Py_INCREF(res);
    return res;
}

int PyJit_Is_Bool(PyObject* lhs, PyObject* rhs) {
    Py_DECREF(lhs);
    Py_DECREF(rhs);
    return lhs == rhs;
}

int PyJit_IsNot_Bool(PyObject* lhs, PyObject* rhs) {
    Py_DECREF(lhs);
    Py_DECREF(rhs);
    return lhs != rhs;
}

void PyJit_DecRef(PyObject* value) {
    if (IS_TAGGED((tagged_ptr)value)) {
        // Tagged pointer
        return;
    }
    Py_DecRef(value);
}

void PyJit_FloatDivideByZero() {
    PyErr_SetString(PyExc_ZeroDivisionError, "float division by zero");
}


// Initializes a stack-based number with our digits
inline PyObject* init_number(size_t* data, tagged_ptr number) {
    PyObject* value = (PyObject*)data;
    // Initialize a stack-allocated number that will never leak, and never be freed...
    value->ob_refcnt = 0x1000000;
    value->ob_type = &PyLong_Type;

    // This won't overflow on min int because we never have min int due to the stolen bit
    auto size = 0;
    auto tmpNumber = abs(number);
    for (int i = 0; i < DIGITS_IN_TAGGED_PTR; i++) {
        ((PyLongObject*)value)->ob_digit[i] = tmpNumber & ((1 << PYLONG_BITS_IN_DIGIT) - 1);
        tmpNumber >>= PYLONG_BITS_IN_DIGIT;
        size++;
        if (tmpNumber == 0) {
            break;
        }
    }
    Py_SIZE(value) = number < 0 ? -((int)size) : size;
    return value;
}

// If the functions return one of our stack-based values, we return the original tagged value
inline PyObject* safe_return(PyObject* tmpLeft, tagged_ptr left, PyObject* res) {
    if (res == tmpLeft) {
        return (PyObject*)left;
    }
    return res;
}

inline PyObject* safe_return(PyObject* tmpLeft, tagged_ptr left, PyObject* tmpRight, tagged_ptr right, PyObject* res) {
    if (res == tmpLeft) {
        return (PyObject*)left;
    }
    else if (res == tmpRight) {
        return (PyObject*)right;
    }
    else {
        return res;
    }
}

inline bool LongOverflow(PyObject* obj, tagged_ptr* value) {
    int overflow;
    *value = AS_LONG_AND_OVERFLOW(obj, &overflow);
    return overflow || !can_tag(*value);
}

PyObject* PyJit_UnboxInt_Tagged(PyObject* value) {
    int overflow;
    auto intValue = AS_LONG_AND_OVERFLOW(value, &overflow);
    if (overflow || !can_tag(intValue)) {
        return value;
    }
    return TAG_IT(intValue);
}

inline PyObject* PyJit_Tagged_Add(tagged_ptr left, tagged_ptr right) {
    tagged_ptr res;
#ifdef _MSC_VER
    if (SafeAdd(left, right, res)) {
#elif __clang__ || __GNUC__
    if (!__builtin_add_overflow(left, right, &res)) {
#else
#error No support for this compiler
#endif
    if (can_tag(res)) {
        return TAG_IT(res);
    }
    }

INIT_TMP_NUMBER(tmpLeft, left);
INIT_TMP_NUMBER(tmpRight, right);
// We overflowed big time...
return safe_return(tmpLeft, left, tmpRight, right, PyNumber_Add(tmpLeft, tmpRight));
}

inline PyObject* PyJit_Tagged_Subtract(tagged_ptr left, tagged_ptr right) {
    tagged_ptr res;
#ifdef _MSC_VER
    if (SafeSubtract(left, right, res)) {
#elif __clang__ || __GNUC__
    if (!__builtin_sub_overflow(left, right, &res)) {
#else
#error No support for this compiler
#endif
    if (can_tag(res)) {
        return TAG_IT(res);
    }
    }

INIT_TMP_NUMBER(tmpLeft, left);
INIT_TMP_NUMBER(tmpRight, right);

// We overflowed big time...
return safe_return(tmpLeft, left, tmpRight, right, PyNumber_Subtract(tmpLeft, tmpRight));
}

inline PyObject* PyJit_Tagged_Multiply(tagged_ptr left, tagged_ptr right) {
    tagged_ptr res;
#ifdef _MSC_VER
    if (SafeMultiply(left, right, res)) {
#elif __clang__ || __GNUC__
    if (!__builtin_mul_overflow(left, right, &res)) {
#else
#error No support for this compiler
#endif
    if (can_tag(res)) {
        return TAG_IT(res);
    }
    // We overflowed by a single bit
    return PyLong_FromLongLong(res);
    }

INIT_TMP_NUMBER(tmpLeft, left);
INIT_TMP_NUMBER(tmpRight, right);
// We overflowed big time...
return safe_return(tmpLeft, left, tmpRight, right, PyNumber_Multiply(tmpLeft, tmpRight));
}


inline PyObject* PyJit_Tagged_Modulo(tagged_ptr left, tagged_ptr right) {
    if (right == 0) {
        PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
        return nullptr;
    }

    tagged_ptr res;
#ifdef _MSC_VER
    if (SafeModulus(left, right, res)) {
#else
#error No support for this compiler
#endif
    if (can_tag(res)) {
        return TAG_IT(res);
    }
    // We overflowed by a single bit
    return NEW_LONG(res);
    }

INIT_TMP_NUMBER(tmpLeft, left);
INIT_TMP_NUMBER(tmpRight, right);

return safe_return(tmpLeft, left, tmpRight, right, PyNumber_Remainder(tmpLeft, tmpRight));
}

inline PyObject* PyJit_Tagged_TrueDivide(tagged_ptr left, tagged_ptr right) {
    if (right == 0) {
        PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
        return nullptr;
    }

    double ld = (double)left, rd = (double)right;
    return PyFloat_FromDouble(ld / rd);
}

inline PyObject* PyJit_Tagged_FloorDivide(tagged_ptr left, tagged_ptr right) {
    if (right == 0) {
        PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
        return nullptr;
    }

    tagged_ptr res;
#ifdef _MSC_VER
    if (SafeDivide(left, right, res)) {
#else
#error No support for this compiler
#endif
    if (can_tag(res)) {
        return TAG_IT(res);
    }
    // We overflowed by a single bit
    return NEW_LONG(res);
    }

INIT_TMP_NUMBER(tmpLeft, left);
INIT_TMP_NUMBER(tmpRight, right);
return safe_return(tmpLeft, left, tmpRight, right, PyNumber_FloorDivide(tmpLeft, tmpRight));
}

inline PyObject* PyJit_Tagged_BinaryLShift(tagged_ptr left, tagged_ptr right) {
    if (right < 0) {
        PyErr_SetString(PyExc_ValueError, "negative shift count");
        return nullptr;
    }
    else if (left == 0) {
        // shifting zero is always zero regardless of right
        return TAG_IT(0);
    }

    if (right < MAX_BITS) {
        if (left > 0) {
            // If left doesn't have any high bits set, it's safe to shift
            if ((left & ~((1 << right) - 1)) == 0) {
                auto res = left << right;
                if (can_tag(res)) {
                    return TAG_IT(res);
                }
            }
        }
        else {
            // Shifting a negative value, of course we have high bits set
            // TODO: Implement optimial version
        }
    }

    // we're overflowed
    INIT_TMP_NUMBER(tmpLeft, left);
    INIT_TMP_NUMBER(tmpRight, right);

    return safe_return(tmpLeft, left, tmpRight, right, PyNumber_Lshift(tmpLeft, tmpRight));
}

inline PyObject* PyJit_Tagged_BinaryRShift(tagged_ptr left, tagged_ptr right) {
    if (right < 0) {
        PyErr_SetString(PyExc_ValueError, "negative shift count");
        return nullptr;
    }

    return TAG_IT(left >> right);
}

inline PyObject* PyJit_Tagged_BinaryAnd(tagged_ptr left, tagged_ptr right) {
    return TAG_IT(left & right);
}

inline PyObject* PyJit_Tagged_BinaryOr(tagged_ptr left, tagged_ptr right) {
    return TAG_IT(left | right);
}

inline PyObject* PyJit_Tagged_BinaryXor(tagged_ptr left, tagged_ptr right) {
    return TAG_IT(left ^ right);
}

inline PyObject* PyJit_Tagged_Power(tagged_ptr left, tagged_ptr right) {
    INIT_TMP_NUMBER(tmpLeft, left);
    INIT_TMP_NUMBER(tmpRight, right);

    // TODO: Optimal version
    return safe_return(tmpLeft, left, tmpRight, right, PyNumber_Power(tmpLeft, tmpRight, Py_None));
}

PyObject* PyJit_BoxTaggedPointer(PyObject* value) {
    tagged_ptr tagged = (tagged_ptr)value;
    if (IS_TAGGED(tagged)) {
        return NEW_LONG(UNTAG_IT(tagged));
    }
    return value;
}

#define TAGGED_METHOD(name) \
	PyObject* PyJit_##name##_Int(PyObject *left, PyObject *right) {						\
	tagged_ptr leftI = (tagged_ptr)left;												\
	tagged_ptr rightI = (tagged_ptr)right;												\
	size_t tempNumber[NUMBER_SIZE];														\
	if (IS_TAGGED(leftI)) {																\
		if (IS_TAGGED(rightI)) {														\
			return PyJit_Tagged_##name(UNTAG_IT(leftI), UNTAG_IT(rightI));				\
		}																				\
		else if (!LongOverflow(right, &rightI)) {										\
			Py_DECREF(right);															\
			return PyJit_Tagged_##name(UNTAG_IT(leftI), rightI);						\
		}																				\
		else {																			\
			/* right is a PyObject and too big	*/										\
			left = init_number(tempNumber, UNTAG_IT(leftI));							\
			return safe_return(left, leftI, PyJit_##name(left, right));					\
		}																				\
	}																					\
	else if (IS_TAGGED(rightI)) {														\
		if (!LongOverflow(left, &leftI)) {												\
			Py_DECREF(left);															\
			return PyJit_Tagged_##name(leftI, UNTAG_IT(rightI));						\
		}																				\
		else {																			\
			/* left is a PyObject and too big...	*/									\
			right = init_number(tempNumber, UNTAG_IT(rightI));							\
			return safe_return(right, rightI, PyJit_##name(left, right));				\
		}																				\
	}																					\
																						\
	/* both are PyObjects		*/														\
	return PyJit_##name(left, right);													\
}																						\

#define TAGGED_COMPARE(name, cmp) \
int PyJit_##name##_Int(PyObject *left, PyObject *right) {							\
	tagged_ptr leftI = (tagged_ptr)left;												\
	tagged_ptr rightI = (tagged_ptr)right;												\
	size_t tempNumber[NUMBER_SIZE];														\
	if (IS_TAGGED(leftI)) {																\
		if (IS_TAGGED(rightI)) {														\
			return UNTAG_IT(leftI) cmp UNTAG_IT(rightI);								\
		}																				\
		else if (!LongOverflow(right, &rightI)) {										\
			Py_DECREF(right);															\
			return UNTAG_IT(leftI) cmp rightI;											\
		}																				\
		else {																			\
			/* right is a PyObject and too big	*/										\
			left = init_number(tempNumber, UNTAG_IT(leftI));							\
		}																				\
	}																					\
	else if (IS_TAGGED(rightI)) {														\
		if (!LongOverflow(left, &leftI)) {												\
			Py_DECREF(left);															\
			return leftI cmp UNTAG_IT(rightI);											\
		}																				\
		else {																			\
			/* left is a PyObject and too big...	*/									\
			right = init_number(tempNumber, UNTAG_IT(rightI));							\
		}																				\
	}																					\
																						\
	/* both are PyObjects		*/														\
	return long_compare((PyLongObject*)left, (PyLongObject*)right) cmp 0;				\
}																						\

TAGGED_METHOD(Add)
TAGGED_METHOD(Subtract)
TAGGED_METHOD(BinaryAnd)
TAGGED_METHOD(BinaryOr)
TAGGED_METHOD(BinaryXor)
TAGGED_METHOD(Multiply)
TAGGED_METHOD(Modulo)
TAGGED_METHOD(TrueDivide)
TAGGED_METHOD(FloorDivide)
TAGGED_METHOD(BinaryLShift)
TAGGED_METHOD(BinaryRShift)
TAGGED_METHOD(Power)

TAGGED_COMPARE(Equals, == )
TAGGED_COMPARE(NotEquals, != )
TAGGED_COMPARE(GreaterThan, > )
TAGGED_COMPARE(LessThan, < )
TAGGED_COMPARE(GreaterThanEquals, >= )
TAGGED_COMPARE(LessThanEquals, <= )

PyObject* PyJit_UnaryNegative_Int(PyObject*value) {
    tagged_ptr valueI = (tagged_ptr)value;
    size_t tempNumber[NUMBER_SIZE];

    if (IS_TAGGED(valueI)) {
        auto untagged = UNTAG_IT(valueI);
        if (untagged != MIN_TAGGED_VALUE) {
            return TAG_IT(-untagged);
        }

        // we'll overflow on the conversion from min to max
        value = init_number(tempNumber, untagged);
    }

    return PyNumber_Negative(value);
}

int PyJit_UnaryNot_Int_PushBool(PyObject*value) {
    tagged_ptr valueI = (tagged_ptr)value;
    if (IS_TAGGED(valueI)) {
        auto untagged = UNTAG_IT(valueI);
        return untagged == 0;
    }

    return Py_SIZE(value) == 0;
}

int PyJit_Int_ToFloat(PyObject* in, double*out) {
    tagged_ptr inI = (tagged_ptr)in;
    if (IS_TAGGED(inI)) {
        INIT_TMP_NUMBER(tmpIn, UNTAG_IT(inI));
        *out = PyLong_AsDouble(tmpIn);      
    }
    else {
        *out = PyLong_AsDouble(in);
    }
    return *out == -1.0 && PyErr_Occurred();
}