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

#ifndef PYJION_INTRINS_H
#define PYJION_INTRINS_H

#include <Python.h>
#include <frameobject.h>
#include <vector>

#define NAME_ERROR_MSG \
    "name '%.200s' is not defined"

#define UNBOUNDLOCAL_ERROR_MSG \
    "local variable '%.200s' referenced before assignment"
#define UNBOUNDFREE_ERROR_MSG \
    "free variable '%.200s' referenced before assignment" \
    " in enclosing scope"

struct PyMethodLocation {
    PyObject* object;
    PyObject* method;
};

static void
format_exc_check_arg(PyObject *exc, const char *format_str, PyObject *obj);

static void
format_exc_unbound(PyCodeObject *co, int oparg);

PyObject* PyJit_Add(PyObject *left, PyObject *right);

PyObject* PyJit_Subscr(PyObject *left, PyObject *right);

PyObject* PyJit_RichCompare(PyObject *left, PyObject *right, size_t op);

PyObject* PyJit_CellGet(PyFrameObject* frame, size_t index);
void PyJit_CellSet(PyObject* value, PyFrameObject* frame, size_t index);

PyObject* PyJit_Contains(PyObject *left, PyObject *right);
PyObject* PyJit_NotContains(PyObject *left, PyObject *right);

PyObject* PyJit_NewFunction(PyObject* code, PyObject* qualname, PyFrameObject* frame);

PyObject* PyJit_SetClosure(PyObject* closure, PyObject* func);

PyObject* PyJit_BuildSlice(PyObject* start, PyObject* stop, PyObject* step);

PyObject* PyJit_UnaryPositive(PyObject* value);

PyObject* PyJit_UnaryNegative(PyObject* value);

PyObject* PyJit_UnaryNot(PyObject* value);
int PyJit_UnaryNot_Int(PyObject* value);

PyObject* PyJit_UnaryInvert(PyObject* value);

PyObject* PyJit_NewList(size_t size);
PyObject* PyJit_ListAppend(PyObject* value, PyObject* list);

PyObject* PyJit_SetAdd(PyObject* value, PyObject* set);
PyObject* PyJit_UpdateSet(PyObject* iterable, PyObject* set);

PyObject* PyJit_MapAdd(PyObject*key, PyObject*map, PyObject* value);

PyObject* PyJit_Multiply(PyObject *left, PyObject *right);

PyObject* PyJit_TrueDivide(PyObject *left, PyObject *right);

PyObject* PyJit_FloorDivide(PyObject *left, PyObject *right);

PyObject* PyJit_Power(PyObject *left, PyObject *right);

PyObject* PyJit_Modulo(PyObject *left, PyObject *right);

PyObject* PyJit_Subtract(PyObject *left, PyObject *right);

PyObject* PyJit_MatrixMultiply(PyObject *left, PyObject *right);

PyObject* PyJit_BinaryLShift(PyObject *left, PyObject *right);
PyObject* PyJit_BinaryRShift(PyObject *left, PyObject *right);

PyObject* PyJit_BinaryAnd(PyObject *left, PyObject *right);
PyObject* PyJit_BinaryXor(PyObject *left, PyObject *right);

PyObject* PyJit_BinaryOr(PyObject *left, PyObject *right);

PyObject* PyJit_InplacePower(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceMultiply(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceMatrixMultiply(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceTrueDivide(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceFloorDivide(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceModulo(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceAdd(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceSubtract(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceLShift(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceRShift(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceAnd(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceXor(PyObject *left, PyObject *right);

PyObject* PyJit_InplaceOr(PyObject *left, PyObject *right);
int PyJit_PrintExpr(PyObject *value);

const char * ObjInfo(PyObject *obj);
void PyJit_PrepareException(PyObject** exc, PyObject**val, PyObject** tb, PyObject** oldexc, PyObject**oldVal, PyObject** oldTb);
void PyJit_UnwindEh(PyObject*exc, PyObject*val, PyObject*tb);

#define CANNOT_CATCH_MSG "catching classes that do not inherit from "\
                         "BaseException is not allowed"

PyObject* PyJit_CompareExceptions(PyObject*v, PyObject* w);
int PyJit_CompareExceptions_Int(PyObject*v, PyObject* w);

void PyJit_UnboundLocal(PyObject* name);

void PyJit_DebugTrace(char* msg);

void PyJit_PyErrRestore(PyObject*tb, PyObject*value, PyObject*exception);

PyObject* PyJit_CheckFunctionResult(PyObject* value);

PyObject* PyJit_ImportName(PyObject*level, PyObject*from, PyObject* name, PyFrameObject* f);

PyObject* PyJit_ImportFrom(PyObject*v, PyObject* name);

static int
import_all_from(PyObject *locals, PyObject *v);
int PyJit_ImportStar(PyObject*from, PyFrameObject* f);

PyObject* PyJit_CallArgs(PyObject* func, PyObject*callargs);
PyObject* PyJit_CallKwArgs(PyObject* func, PyObject*callargs, PyObject*kwargs);

PyObject* PyJit_KwCallN(PyObject *target, PyObject* args, PyObject* names);

void PyJit_PushFrame(PyFrameObject* frame);
void PyJit_PopFrame(PyFrameObject* frame);

void PyJit_EhTrace(PyFrameObject *f);

int PyJit_Raise(PyObject *exc, PyObject *cause);

PyObject* PyJit_LoadClassDeref(PyFrameObject* frame, size_t oparg);

PyObject* PyJit_ExtendList(PyObject *iterable, PyObject *list);

PyObject* PyJit_LoadAssertionError();

PyObject* PyJit_ListToTuple(PyObject *list);

int PyJit_StoreMap(PyObject *key, PyObject *value, PyObject* map);
int PyJit_StoreMapNoDecRef(PyObject *key, PyObject *value, PyObject* map);

PyObject* PyJit_DictUpdate(PyObject* other, PyObject* dict);
PyObject * PyJit_BuildDictFromTuples(PyObject *keys_and_values);
PyObject* PyJit_DictMerge(PyObject* other, PyObject* dict);

int PyJit_StoreSubscr(PyObject* value, PyObject *container, PyObject *index);

int PyJit_DeleteSubscr(PyObject *container, PyObject *index);

PyObject* PyJit_CallN(PyObject *target, PyObject* args);

PyObject* PyJit_CallNKW(PyObject *target, PyObject* args, PyObject* kwargs);

int PyJit_StoreGlobal(PyObject* v, PyFrameObject* f, PyObject* name);

int PyJit_DeleteGlobal(PyFrameObject* f, PyObject* name);

PyObject* PyJit_LoadGlobal(PyFrameObject* f, PyObject* name);

PyObject* PyJit_GetIter(PyObject* iterable);

PyObject* PyJit_IterNext(PyObject* iter);

PyObject* PyJit_PyTuple_New(ssize_t len);

PyObject* PyJit_BuildClass(PyFrameObject *f);

// Returns: the address for the 1st set of items, the constructed list, and the
// address where the remainder live.
PyObject** PyJit_UnpackSequenceEx(PyObject* seq, size_t leftSize, size_t rightSize, PyObject** tempStorage, PyObject** listRes, PyObject*** remainder);

// Unpacks the given sequence and returns a pointer to where the sequence
// is stored.  If this is a type we can just grab the array from it returns
// the array.  Otherwise we unpack the sequence into tempStorage which was
// allocated on the stack when we entered the generated method body.
PyObject** PyJit_UnpackSequence(PyObject* seq, size_t size, PyObject** tempStorage);

PyObject* PyJit_LoadAttr(PyObject* owner, PyObject* name);

int PyJit_StoreAttr(PyObject* value, PyObject* owner, PyObject* name);

int PyJit_DeleteAttr(PyObject* owner, PyObject* name);

PyObject* PyJit_LoadName(PyFrameObject* f, PyObject* name);

int PyJit_StoreName(PyObject* v, PyFrameObject* f, PyObject* name);
int PyJit_DeleteName(PyFrameObject* f, PyObject* name);

PyObject* PyJit_Is(PyObject* lhs, PyObject* rhs);
PyObject* PyJit_IsNot(PyObject* lhs, PyObject* rhs);

int PyJit_Is_Bool(PyObject* lhs, PyObject* rhs);
int PyJit_IsNot_Bool(PyObject* lhs, PyObject* rhs);

PyObject* Call0(PyObject *target);
PyObject* Call1(PyObject *target, PyObject* arg0);
PyObject* Call2(PyObject *target, PyObject* arg0, PyObject* arg1);
PyObject* Call3(PyObject *target, PyObject* arg0, PyObject* arg1, PyObject* arg2);
PyObject* Call4(PyObject *target, PyObject* arg0, PyObject* arg1, PyObject* arg2, PyObject* arg3);

extern PyObject* g_emptyTuple;

void PyJit_DecRef(PyObject* value);

PyObject* PyJit_BinaryLShift_Int(PyObject *left, PyObject *right);
PyObject* PyJit_BinaryRShift_Int(PyObject *left, PyObject *right);
PyObject* PyJit_Power_Int(PyObject *left, PyObject *right);

int PyJit_PeriodicWork();

PyObject* PyJit_UnicodeJoinArray(PyObject** items, Py_ssize_t count);
PyObject* PyJit_FormatObject(PyObject* item, PyObject*fmtSpec);
PyObject* PyJit_FormatValue(PyObject* item);

PyMethodLocation * PyJit_LoadMethod(PyObject* object, PyObject* name);

PyObject* MethCall0(PyObject* self, PyMethodLocation* method_info);
PyObject* MethCall1(PyObject* self, PyMethodLocation* method_info, PyObject* arg1);
PyObject* MethCall2(PyObject* self, PyMethodLocation* method_info, PyObject* arg1, PyObject* arg2);
PyObject* MethCall3(PyObject* self, PyMethodLocation* method_info, PyObject* arg1, PyObject* arg2, PyObject* arg3);
PyObject* MethCall4(PyObject* self, PyMethodLocation* method_info, PyObject* arg1, PyObject* arg2, PyObject* arg3, PyObject* arg4);
PyObject* MethCallN(PyObject* self, PyMethodLocation* method_info, PyObject* args);

int PyJit_SetupAnnotations(PyFrameObject* frame);

#endif