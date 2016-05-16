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

#ifndef INTRINS_H
#define INTRINS_H

#include <Python.h>
#include <frameobject.h>

#define NAME_ERROR_MSG \
    "name '%.200s' is not defined"

#define UNBOUNDLOCAL_ERROR_MSG \
    "local variable '%.200s' referenced before assignment"
#define UNBOUNDFREE_ERROR_MSG \
    "free variable '%.200s' referenced before assignment" \
    " in enclosing scope"

static void
format_exc_check_arg(PyObject *exc, const char *format_str, PyObject *obj);

static void
format_exc_unbound(PyCodeObject *co, int oparg);


PyObject* PyJit_Add(PyObject *left, PyObject *right);

PyObject* PyJit_Subscr(PyObject *left, PyObject *right);

PyObject* PyJit_RichCompare(PyObject *left, PyObject *right, int op);

int PyJit_RichEquals_Generic(PyObject *left, PyObject *right, void** addr);

PyObject* PyJit_CellGet(PyFrameObject* frame, size_t index);


PyObject* PyJit_Contains(PyObject *left, PyObject *right);
PyObject* PyJit_NotContains(PyObject *left, PyObject *right);

int PyJit_Contains_Int(PyObject *left, PyObject *right);
int PyJit_NotContains_Int(PyObject *left, PyObject *right);

PyObject* PyJit_NewFunction(PyObject* code, PyObject* qualname, PyFrameObject* frame);

PyObject* PyJit_SetClosure(PyObject* closure, PyObject* func);

PyObject* PyJit_BuildSlice(PyObject* start, PyObject* stop, PyObject* step);

PyObject* PyJit_UnaryPositive(PyObject* value);

PyObject* PyJit_UnaryNegative(PyObject* value);

PyObject* PyJit_UnaryNot(PyObject* value);
int PyJit_UnaryNot_Int(PyObject* value);

PyObject* PyJit_UnaryInvert(PyObject* value);

PyObject* PyJit_ListAppend(PyObject* list, PyObject* value);

PyObject* PyJit_SetAdd(PyObject* set, PyObject* value);

int PyJit_UpdateSet(PyObject* set, PyObject* value);

PyObject* PyJit_MapAdd(PyObject*map, PyObject* value, PyObject*key);

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
const char * ObjInfo(PyObject *obj);

void PyJit_PyErrRestore(PyObject*tb, PyObject*value, PyObject*exception);

PyObject* PyJit_CheckFunctionResult(PyObject* value);

PyObject* PyJit_ImportName(PyObject*level, PyObject*from, PyObject* name, PyFrameObject* f);

PyObject* PyJit_ImportFrom(PyObject*v, PyObject* name);

static int
import_all_from(PyObject *locals, PyObject *v);
int PyJit_ImportStar(PyObject*from, PyFrameObject* f);

PyObject* PyJit_FancyCall(PyObject* func, PyObject*args, PyObject*kwargs, PyObject* stararg, PyObject* kwdict);

void PyJit_DebugDumpFrame(PyFrameObject* frame);

void PyJit_PushFrame(PyFrameObject* frame);
void PyJit_PopFrame(PyFrameObject* frame);

int PyJit_FunctionSetDefaults(PyObject* defs, PyObject* func);

int PyJit_FunctionSetAnnotations(PyObject* values, PyObject* names, PyObject* func);

int PyJit_FunctionSetKwDefaults(PyObject* defs, PyObject* func);

void PyJit_EhTrace(PyFrameObject *f);

int PyJit_Raise(PyObject *exc, PyObject *cause);

PyObject* PyJit_LoadClassDeref(PyFrameObject* frame, size_t oparg);

int PyJit_ExtendList(PyObject *list, PyObject *extension);

PyObject* PyJit_ListToTuple(PyObject *list);

int PyJit_StoreMap(PyObject *key, PyObject *value, PyObject* map);

int PyJit_DictUpdate(PyObject *dict, PyObject* other);

int PyJit_StoreSubscr(PyObject* value, PyObject *container, PyObject *index);

int PyJit_DeleteSubscr(PyObject *container, PyObject *index);

PyObject* PyJit_CallN(PyObject *target, PyObject* args);

PyObject* PyJit_CallNKW(PyObject *target, PyObject* args, PyObject* kwargs);

int PyJit_StoreGlobal(PyObject* v, PyFrameObject* f, PyObject* name);

int PyJit_DeleteGlobal(PyFrameObject* f, PyObject* name);

PyObject* PyJit_LoadGlobal(PyFrameObject* f, PyObject* name);

PyObject* PyJit_GetIter(PyObject* iterable);
PyObject* PyJit_GetIterOptimized(PyObject* iterable, size_t* iterstate1, size_t* iterstate2);
PyObject* PyJit_IterNextOptimized(PyObject* iter, int*error, size_t* iterstate1, size_t* iterstate2);

PyObject* PyJit_IterNext(PyObject* iter, int*error);

void PyJit_CellSet(PyObject* value, PyObject* cell);

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

const char * ObjInfo(PyObject *obj);

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

PyObject* Call0_Generic(PyObject *target, void**addr);

extern PyObject* g_emptyTuple;


void PyJit_DecRef(PyObject* value);

void PyJit_FloatDivideByZero();

PyObject* PyJit_UnboxInt_Tagged(PyObject* value);
PyObject* PyJit_Add_Int(PyObject *left, PyObject *right);
PyObject* PyJit_Subtract_Int(PyObject *left, PyObject *right);
PyObject* PyJit_BinaryAnd_Int(PyObject *left, PyObject *right);
PyObject* PyJit_BinaryOr_Int(PyObject *left, PyObject *right);
PyObject* PyJit_BinaryXor_Int(PyObject *left, PyObject *right);
PyObject* PyJit_Multiply_Int(PyObject *left, PyObject *right);
PyObject* PyJit_Modulo_Int(PyObject *left, PyObject *right);
PyObject* PyJit_TrueDivide_Int(PyObject *left, PyObject *right);
PyObject* PyJit_FloorDivide_Int(PyObject *left, PyObject *right);
PyObject* PyJit_BinaryLShift_Int(PyObject *left, PyObject *right);
PyObject* PyJit_BinaryRShift_Int(PyObject *left, PyObject *right);
PyObject* PyJit_Power_Int(PyObject *left, PyObject *right);

PyObject* PyJit_BoxTaggedPointer(PyObject* value);
PyObject* PyJit_UnaryNegative_Int(PyObject*value);
int PyJit_UnaryNot_Int_PushBool(PyObject*value);

int PyJit_Equals_Int(PyObject *left, PyObject *right);
int PyJit_LessThan_Int(PyObject *left, PyObject *right);
int PyJit_LessThanEquals_Int(PyObject *left, PyObject *right);
int PyJit_NotEquals_Int(PyObject *left, PyObject *right);
int PyJit_GreaterThan_Int(PyObject *left, PyObject *right);
int PyJit_GreaterThanEquals_Int(PyObject *left, PyObject *right);

int PyJit_Int_ToFloat(PyObject* in, double*out);
#endif