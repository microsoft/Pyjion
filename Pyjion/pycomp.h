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

#ifndef PYCOMP_H
#define PYCOMP_H

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

#include <Python.h>

#include <vector>
#include <unordered_map>

#include <Python.h>

#include "ipycomp.h"
#include "jitinfo.h"
#include "codemodel.h"
#include "ilgen.h"
#include "intrins.h"
#include "absint.h"

// binary operator helpers
#define METHOD_ADD_TOKEN						0x00000000
#define METHOD_MULTIPLY_TOKEN					0x00000001
#define METHOD_SUBTRACT_TOKEN					0x00000002
#define METHOD_DIVIDE_TOKEN						0x00000003
#define METHOD_FLOORDIVIDE_TOKEN				0x00000004
#define METHOD_POWER_TOKEN						0x00000005
#define METHOD_MODULO_TOKEN						0x00000006
#define METHOD_SUBSCR_TOKEN						0x00000007
#define METHOD_STOREMAP_TOKEN					0x00000008
#define METHOD_RICHCMP_TOKEN					0x00000009
#define METHOD_CONTAINS_TOKEN					0x0000000A
#define METHOD_NOTCONTAINS_TOKEN				0x0000000B
#define METHOD_STORESUBSCR_TOKEN				0x0000000C
#define METHOD_DELETESUBSCR_TOKEN				0x0000000D
#define METHOD_NEWFUNCTION_TOKEN				0x0000000E
#define METHOD_GETITER_TOKEN					0x0000000F
#define METHOD_DECREF_TOKEN						0x00000010
#define METHOD_GETBUILDCLASS_TOKEN				0x00000011
#define METHOD_LOADNAME_TOKEN					0x00000012
#define METHOD_STORENAME_TOKEN					0x00000013
#define METHOD_UNPACK_SEQUENCE_TOKEN			0x00000014
#define METHOD_UNPACK_SEQUENCEEX_TOKEN			0x00000015
#define METHOD_DELETENAME_TOKEN					0x00000016
#define METHOD_PYCELL_SET_TOKEN					0x00000017
#define METHOD_SET_CLOSURE						0x00000018
#define METHOD_BUILD_SLICE						0x00000019
#define METHOD_UNARY_POSITIVE					0x0000001A
#define METHOD_UNARY_NEGATIVE					0x0000001B
#define METHOD_UNARY_NOT						0x0000001C
#define METHOD_UNARY_INVERT						0x0000001D
#define METHOD_MATRIX_MULTIPLY_TOKEN			0x0000001E
#define METHOD_BINARY_LSHIFT_TOKEN				0x0000001F
#define METHOD_BINARY_RSHIFT_TOKEN				0x00000020
#define METHOD_BINARY_AND_TOKEN					0x00000021
#define METHOD_BINARY_XOR_TOKEN					0x00000022
#define METHOD_BINARY_OR_TOKEN					0x00000023
#define METHOD_LIST_APPEND_TOKEN				0x00000024
#define METHOD_SET_ADD_TOKEN					0x00000025
#define METHOD_INPLACE_POWER_TOKEN				0x00000026
#define METHOD_INPLACE_MULTIPLY_TOKEN			0x00000027
#define METHOD_INPLACE_MATRIX_MULTIPLY_TOKEN	0x00000028
#define METHOD_INPLACE_TRUE_DIVIDE_TOKEN		0x00000029
#define METHOD_INPLACE_FLOOR_DIVIDE_TOKEN		0x0000002A
#define METHOD_INPLACE_MODULO_TOKEN				0x0000002B
#define METHOD_INPLACE_ADD_TOKEN				0x0000002C
#define METHOD_INPLACE_SUBTRACT_TOKEN			0x0000002D
#define METHOD_INPLACE_LSHIFT_TOKEN				0x0000002E
#define METHOD_INPLACE_RSHIFT_TOKEN				0x0000002F
#define METHOD_INPLACE_AND_TOKEN				0x00000030
#define METHOD_INPLACE_XOR_TOKEN				0x00000031
#define METHOD_INPLACE_OR_TOKEN					0x00000032
#define METHOD_MAP_ADD_TOKEN					0x00000033
#define METHOD_PRINT_EXPR_TOKEN					0x00000034
#define METHOD_LOAD_CLASSDEREF_TOKEN			0x00000035
#define METHOD_PREPARE_EXCEPTION				0x00000036
#define METHOD_DO_RAISE							0x00000037
#define METHOD_EH_TRACE							0x00000038
#define METHOD_COMPARE_EXCEPTIONS				0x00000039
#define METHOD_UNBOUND_LOCAL					0x0000003A
#define METHOD_DEBUG_TRACE						0x0000003B
#define METHOD_FUNC_SET_DEFAULTS				0x0000003C
#define	METHOD_CALLNKW_TOKEN					0x0000003D
#define	METHOD_DEBUG_DUMP_FRAME					0x0000003E
#define METHOD_UNWIND_EH						0x0000003F
#define METHOD_PY_PUSHFRAME						0x00000041
#define METHOD_PY_POPFRAME						0x00000042
#define METHOD_PY_IMPORTNAME					0x00000043
#define METHOD_PY_FANCYCALL						0x00000044
#define METHOD_PY_IMPORTFROM					0x00000045
#define METHOD_PY_IMPORTSTAR					0x00000046
#define METHOD_PY_FUNC_SET_ANNOTATIONS			0x00000047
#define METHOD_PY_FUNC_SET_KW_DEFAULTS			0x00000048
#define METHOD_IS								0x00000049
#define METHOD_ISNOT							0x0000004A
#define METHOD_IS_BOOL							0x0000004B
#define METHOD_ISNOT_BOOL						0x0000004C
#define METHOD_GETITER_OPTIMIZED_TOKEN			0x0000004D
#define METHOD_COMPARE_EXCEPTIONS_INT			0x0000004E
#define METHOD_CONTAINS_INT_TOKEN				0x0000004F
#define METHOD_NOTCONTAINS_INT_TOKEN			0x00000050
#define METHOD_UNARY_NOT_INT					0x00000051
#define METHOD_RICHEQUALS_GENERIC_TOKEN			0x00000052
#define METHOD_FLOAT_FROM_DOUBLE				0x00000053
#define METHOD_BOOL_FROM_LONG					0x00000054
#define METHOD_PYERR_SETSTRING					0x00000055
#define METHOD_BOX_TAGGED_PTR					0x00000056

#define METHOD_ADD_INT_TOKEN					0x00000057
#define METHOD_DIVIDE_INT_TOKEN					0x00000058
#define METHOD_FLOORDIVIDE_INT_TOKEN			0x00000059
#define METHOD_POWER_INT_TOKEN					0x0000005A	
#define METHOD_MODULO_INT_TOKEN					0x0000005B
#define METHOD_BINARY_LSHIFT_INT_TOKEN			0x0000005C
#define METHOD_BINARY_RSHIFT_INT_TOKEN			0x0000005D
#define METHOD_BINARY_AND_INT_TOKEN				0x0000005E
#define METHOD_BINARY_XOR_INT_TOKEN				0x0000005F
#define METHOD_BINARY_OR_INT_TOKEN				0x00000060
#define METHOD_MULTIPLY_INT_TOKEN				0x00000061
#define METHOD_SUBTRACT_INT_TOKEN				0x00000062
#define METHOD_UNARY_NEGATIVE_INT				0x00000063
#define METHOD_UNARY_NOT_INT_PUSH_BOOL			0x00000064

#define METHOD_EQUALS_INT_TOKEN					0x00000065
#define METHOD_LESS_THAN_INT_TOKEN				0x00000066
#define METHOD_LESS_THAN_EQUALS_INT_TOKEN		0x00000067
#define METHOD_NOT_EQUALS_INT_TOKEN				0x00000068
#define METHOD_GREATER_THAN_INT_TOKEN			0x00000069
#define METHOD_GREATER_THAN_EQUALS_INT_TOKEN	0x0000006A
#define METHOD_PERIODIC_WORK                    0x0000006B

#define METHOD_EXTENDLIST_TOKEN                 0x0000006C
#define METHOD_LISTTOTUPLE_TOKEN                0x0000006D
#define METHOD_SETUPDATE_TOKEN                  0x0000006E
#define METHOD_DICTUPDATE_TOKEN                 0x0000006F
#define METHOD_UNBOX_LONG_TAGGED                0x00000070
#define METHOD_UNBOX_FLOAT                      0x00000071

#define METHOD_INT_TO_FLOAT					    0x00000072

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

#define METHOD_CALL0_OPT_TOKEN		0x00010200

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

#define METHOD_FLOAT_POWER_TOKEN	0x00050000
#define METHOD_FLOAT_FLOOR_TOKEN	0x00050001
#define METHOD_FLOAT_MODULUS_TOKEN  0x00050002

// signatures for calli methods
#define SIG_ITERNEXT_TOKEN			0x00040000
#define SIG_ITERNEXT_OPTIMIZED_TOKEN	0x00040001

#define FIRST_USER_FUNCTION_TOKEN   0x00100000

#define LD_FIELDA(type, field) m_il.ld_i(offsetof(type, field)); m_il.add(); 
#define LD_FIELD(type, field) m_il.ld_i(offsetof(type, field)); m_il.add(); m_il.ld_ind_i();
#define ST_FIELD(type, field) m_il.ld_i(offsetof(type, field)); m_il.add(); m_il.st_ind_i();


#define NEXTARG() *(unsigned short*)&m_byteCode[i + 1]; i+= 2

extern ICorJitCompiler* g_jit;
class PythonCompiler : public IPythonCompiler {
    PyCodeObject *m_code;
    // pre-calculate some information...
    ILGenerator m_il;
    unsigned char *m_byteCode;
    size_t m_size;
    UserModule* m_module;
    Local m_lasti;

public:
    PythonCompiler(PyCodeObject *code);

    virtual void emit_rot_two(LocalKind kind = LK_Pointer);

    virtual void emit_rot_three(LocalKind kind = LK_Pointer);

    virtual void emit_pop_top();

    virtual void emit_dup_top();

    virtual void emit_dup_top_two();

    virtual void emit_load_name(PyObject* name);
    virtual void emit_is_true();

    virtual void emit_push_frame();
    virtual void emit_pop_frame();
    virtual void emit_eh_trace();

    void emit_lasti_init();
    void emit_lasti_update(int index);

    virtual void emit_ret();

    virtual void emit_store_name(PyObject* name);
    virtual void emit_delete_name(PyObject* name);
    virtual void emit_store_attr(PyObject* name);
    virtual void emit_delete_attr(PyObject* name);
    virtual void emit_load_attr(PyObject* name);
    virtual void emit_store_global(PyObject* name);
    virtual void emit_delete_global(PyObject* name);
    virtual void emit_load_global(PyObject* name);
    virtual void emit_delete_fast(int index, PyObject* name);

    virtual void emit_new_tuple(size_t size);
    virtual void emit_tuple_store(size_t size);

    virtual void emit_new_list(size_t argCnt);
    virtual void emit_list_store(size_t argCnt);
    virtual void emit_list_extend();
    virtual void emit_list_to_tuple();

    virtual void emit_new_set();
    virtual void emit_set_extend();
    virtual void emit_dict_store();

    virtual void emit_new_dict(size_t size);
    virtual void emit_map_extend();

    virtual void emit_build_slice();

    virtual void emit_store_subscr();
    virtual void emit_delete_subscr();

    virtual void emit_unary_positive();
    virtual void emit_unary_negative();
    virtual void emit_unary_negative_float();
    virtual void emit_unary_negative_tagged_int();

    virtual void emit_unary_not();

    virtual void emit_unary_not_push_int();
    virtual void emit_unary_not_float_push_bool();
    virtual void emit_unary_not_tagged_int_push_bool();
    virtual void emit_unary_invert();

    virtual void emit_import_name(PyObject* name);
    virtual void emit_import_from(PyObject* name);
    virtual void emit_import_star();

    virtual void emit_load_build_class();

    virtual void emit_unpack_sequence(Local sequence, Local sequenceStorage, Label success, size_t size);
    virtual void emit_load_array(int index);

    virtual void emit_unpack_ex(Local sequence, size_t leftSize, size_t rightSize, Local sequenceStorage, Local list, Local remainder);

    virtual void emit_fancy_call();
    // Emits a call for the specified argument count.  If the compiler
    // can't emit a call with this number of args then it returns false,
    // and emit_call_with_tuple is used to call with a variable sized
    // tuple instead.
    virtual bool emit_call(size_t argCnt);
    virtual void emit_call_with_tuple();
    virtual void emit_call_with_kws();

    virtual void emit_new_function();
    virtual void emit_set_closure();
    virtual void emit_set_annotations();
    virtual void emit_set_kw_defaults();
    virtual void emit_set_defaults();

    virtual void emit_load_deref(int index);
    virtual void emit_store_deref(int index);
    virtual void emit_delete_deref(int index);
    virtual void emit_load_closure(int index);

    virtual Local emit_spill();
    virtual void emit_store_local(Local local);

    virtual void emit_load_local(Local local);
    virtual void emit_load_local_addr(Local local);
    virtual void emit_load_and_free_local(Local local);
    virtual Local emit_define_local(bool cache);
    virtual Local emit_define_local(LocalKind kind = LK_Pointer);
    virtual void emit_free_local(Local local);
    virtual Local emit_allocate_stack_array(size_t elements);

    virtual void emit_set_add();
    virtual void emit_map_add();
    virtual void emit_list_append();

    virtual void emit_raise_varargs();

    virtual void emit_null();

    virtual void emit_print_expr();
    virtual void emit_load_classderef(int index);
    virtual void emit_getiter();
    //void emit_getiter_opt();
    virtual void emit_for_next(Label processValue, Local iterValue);

    virtual void emit_binary_float(int opcode);
    virtual void emit_binary_tagged_int(int opcode);
    virtual void emit_binary_object(int opcode);
    virtual void emit_tagged_int_to_float();

    virtual void emit_in_push_int();
    virtual void emit_in();
    virtual void emit_not_in_push_int();
    virtual void emit_not_in();

    virtual void emit_is_push_int(bool isNot);

    virtual void emit_is(bool isNot);

    virtual void emit_compare_object(int compareType);
    virtual void emit_compare_float(int compareType);
    virtual void emit_compare_tagged_int(int compareType);
    virtual bool emit_compare_object_push_int(int compareType);

    virtual void emit_store_fast(int local);

    virtual void emit_unbound_local_check();
    virtual void emit_load_fast(int local);

    virtual Label emit_define_label();
    virtual void emit_mark_label(Label label);
    virtual void emit_branch(BranchType branchType, Label label);
    virtual void emit_compare_equal();

    virtual void emit_int(int value);
    virtual void emit_float(double value);
    virtual void emit_tagged_int(ssize_t value);
    virtual void emit_unbox_int_tagged();
    virtual void emit_unbox_float();
    virtual void emit_ptr(void *value);
    virtual void emit_bool(bool value);
    virtual void emit_py_object(PyObject* value);

    virtual void emit_unwind_eh(Local prevExc, Local prevExcVal, Local prevTraceback);
    virtual void emit_prepare_exception(Local prevExc, Local prevExcVal, Local prevTraceback);
    virtual void emit_restore_err();
    virtual void emit_pyerr_setstring(PyObject* exception, const char*msg);

    virtual void emit_compare_exceptions();
    virtual void emit_compare_exceptions_int();

    // Pops a value off the stack, performing no operations related to reference counting
    virtual void emit_pop();
    // Dups the current value on the stack, performing no operations related to reference counting
    virtual void emit_dup();

    virtual void emit_box_float();
    virtual void emit_box_bool();
    virtual void emit_box_tagged_ptr();
    virtual void emit_incref(bool maybeTagged = false);

    virtual void emit_debug_msg(const char* msg);

    virtual void emit_periodic_work();

    virtual JittedCode* emit_compile();

private:
    void load_frame();

    void load_local(int oparg);
    void decref();

    void call_optimizing_function(int baseFunction);

    CorInfoType to_clr_type(LocalKind kind);
};

#endif