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

#ifndef PYJION_PYCOMP_H
#define PYJION_PYCOMP_H


#include <windows.h>

#include <share.h>
#include <intrin.h>
#include <climits>
#include <cfloat>

#include "ipycomp.h"
#include "jitinfo.h"
#include "codemodel.h"
#include "ilgen.h"
#include "intrins.h"
#include "absint.h"
#include "disasm.h"

// binary operator helpers
#define METHOD_ADD_TOKEN                         0x00000000
#define METHOD_MULTIPLY_TOKEN                    0x00000001
#define METHOD_SUBTRACT_TOKEN                    0x00000002
#define METHOD_DIVIDE_TOKEN                      0x00000003
#define METHOD_FLOORDIVIDE_TOKEN                 0x00000004
#define METHOD_POWER_TOKEN                       0x00000005
#define METHOD_MODULO_TOKEN                      0x00000006
#define METHOD_SUBSCR_TOKEN                      0x00000007
#define METHOD_STOREMAP_TOKEN                    0x00000008
#define METHOD_RICHCMP_TOKEN                     0x00000009
#define METHOD_CONTAINS_TOKEN                    0x0000000A
#define METHOD_NOTCONTAINS_TOKEN                 0x0000000B
#define METHOD_STORESUBSCR_TOKEN                 0x0000000C
#define METHOD_DELETESUBSCR_TOKEN                0x0000000D
#define METHOD_NEWFUNCTION_TOKEN                 0x0000000E
#define METHOD_GETITER_TOKEN                     0x0000000F
#define METHOD_DECREF_TOKEN                      0x00000010
#define METHOD_GETBUILDCLASS_TOKEN               0x00000011
#define METHOD_LOADNAME_TOKEN                    0x00000012
#define METHOD_STORENAME_TOKEN                   0x00000013
#define METHOD_UNPACK_SEQUENCE_TOKEN             0x00000014
#define METHOD_UNPACK_SEQUENCEEX_TOKEN           0x00000015
#define METHOD_DELETENAME_TOKEN                  0x00000016
#define METHOD_PYCELL_SET_TOKEN                  0x00000017
#define METHOD_SET_CLOSURE                       0x00000018
#define METHOD_BUILD_SLICE                       0x00000019
#define METHOD_UNARY_POSITIVE                    0x0000001A
#define METHOD_UNARY_NEGATIVE                    0x0000001B
#define METHOD_UNARY_NOT                         0x0000001C
#define METHOD_UNARY_INVERT                      0x0000001D
#define METHOD_MATRIX_MULTIPLY_TOKEN             0x0000001E
#define METHOD_BINARY_LSHIFT_TOKEN               0x0000001F
#define METHOD_BINARY_RSHIFT_TOKEN               0x00000020
#define METHOD_BINARY_AND_TOKEN                  0x00000021
#define METHOD_BINARY_XOR_TOKEN                  0x00000022
#define METHOD_BINARY_OR_TOKEN                   0x00000023
#define METHOD_LIST_APPEND_TOKEN                 0x00000024
#define METHOD_SET_ADD_TOKEN                     0x00000025
#define METHOD_INPLACE_POWER_TOKEN               0x00000026
#define METHOD_INPLACE_MULTIPLY_TOKEN            0x00000027
#define METHOD_INPLACE_MATRIX_MULTIPLY_TOKEN     0x00000028
#define METHOD_INPLACE_TRUE_DIVIDE_TOKEN         0x00000029
#define METHOD_INPLACE_FLOOR_DIVIDE_TOKEN        0x0000002A
#define METHOD_INPLACE_MODULO_TOKEN              0x0000002B
#define METHOD_INPLACE_ADD_TOKEN                 0x0000002C
#define METHOD_INPLACE_SUBTRACT_TOKEN            0x0000002D
#define METHOD_INPLACE_LSHIFT_TOKEN              0x0000002E
#define METHOD_INPLACE_RSHIFT_TOKEN              0x0000002F
#define METHOD_INPLACE_AND_TOKEN                 0x00000030
#define METHOD_INPLACE_XOR_TOKEN                 0x00000031
#define METHOD_INPLACE_OR_TOKEN                  0x00000032
#define METHOD_MAP_ADD_TOKEN                     0x00000033
#define METHOD_PRINT_EXPR_TOKEN                  0x00000034
#define METHOD_LOAD_CLASSDEREF_TOKEN             0x00000035
#define METHOD_PREPARE_EXCEPTION                 0x00000036
#define METHOD_DO_RAISE                          0x00000037
#define METHOD_EH_TRACE                          0x00000038
#define METHOD_COMPARE_EXCEPTIONS                0x00000039
#define METHOD_UNBOUND_LOCAL                     0x0000003A
#define METHOD_DEBUG_TRACE                       0x0000003B
#define METHOD_UNWIND_EH                         0x0000003F
#define METHOD_PY_PUSHFRAME                      0x00000041
#define METHOD_PY_POPFRAME                       0x00000042
#define METHOD_PY_IMPORTNAME                     0x00000043

#define METHOD_PY_IMPORTFROM                     0x00000045
#define METHOD_PY_IMPORTSTAR                     0x00000046
#define METHOD_IS                                0x00000049
#define METHOD_ISNOT                             0x0000004A
#define METHOD_IS_BOOL                           0x0000004B
#define METHOD_ISNOT_BOOL                        0x0000004C

#define METHOD_UNARY_NOT_INT                     0x00000051
#define METHOD_FLOAT_FROM_DOUBLE                 0x00000053
#define METHOD_BOOL_FROM_LONG                    0x00000054
#define METHOD_PYERR_SETSTRING                   0x00000055
#define METHOD_BOX_TAGGED_PTR                    0x00000056

#define METHOD_EQUALS_INT_TOKEN                  0x00000065
#define METHOD_LESS_THAN_INT_TOKEN               0x00000066
#define METHOD_LESS_THAN_EQUALS_INT_TOKEN        0x00000067
#define METHOD_NOT_EQUALS_INT_TOKEN              0x00000068
#define METHOD_GREATER_THAN_INT_TOKEN            0x00000069
#define METHOD_GREATER_THAN_EQUALS_INT_TOKEN     0x0000006A
#define METHOD_PERIODIC_WORK                     0x0000006B

#define METHOD_EXTENDLIST_TOKEN                  0x0000006C
#define METHOD_LISTTOTUPLE_TOKEN                 0x0000006D
#define METHOD_SETUPDATE_TOKEN                   0x0000006E
#define METHOD_DICTUPDATE_TOKEN                  0x0000006F

#define METHOD_INT_TO_FLOAT                      0x00000072

#define METHOD_STOREMAP_NO_DECREF_TOKEN          0x00000073
#define METHOD_FORMAT_VALUE                      0x00000074
#define METHOD_FORMAT_OBJECT                     0x00000075
#define METHOD_BUILD_DICT_FROM_TUPLES            0x00000076
#define METHOD_DICT_MERGE                        0x00000077
#define METHOD_SETUP_ANNOTATIONS                 0x00000078

// call helpers
#define METHOD_CALL_0_TOKEN        0x00010000
#define METHOD_CALL_1_TOKEN        0x00010001
#define METHOD_CALL_2_TOKEN        0x00010002
#define METHOD_CALL_3_TOKEN        0x00010003
#define METHOD_CALL_4_TOKEN        0x00010004

#define METHOD_METHCALL_0_TOKEN       0x00011000
#define METHOD_METHCALL_1_TOKEN       0x00011001
#define METHOD_METHCALL_2_TOKEN       0x00011002
#define METHOD_METHCALL_3_TOKEN       0x00011003
#define METHOD_METHCALL_4_TOKEN       0x00011004
#define METHOD_METHCALLN_TOKEN        0x00011005

#define METHOD_CALL_ARGS            0x0001000A
#define METHOD_CALL_KWARGS          0x0001000B
#define METHOD_PYUNICODE_JOINARRAY  0x0002000C

#define METHOD_CALLN_TOKEN          0x000101FF

#define METHOD_KWCALLN_TOKEN        0x000103FF

// method helpers

#define METHOD_LOAD_METHOD      0x00010400

// Py* helpers
#define METHOD_PYTUPLE_NEW           0x00020000
#define METHOD_PYLIST_NEW            0x00020001
#define METHOD_PYDICT_NEWPRESIZED    0x00020002
#define METHOD_PYSET_NEW             0x00020003
#define METHOD_PYSET_ADD             0x00020004
#define METHOD_PYOBJECT_ISTRUE       0x00020005
#define METHOD_PYITER_NEXT           0x00020006
#define METHOD_PYCELL_GET            0x00020007
#define METHOD_PYERR_RESTORE         0x00020008
#define METHOD_PYOBJECT_STR          0x00020009
#define METHOD_PYOBJECT_REPR         0x0002000A
#define METHOD_PYOBJECT_ASCII        0x0002000B

// Misc helpers
#define METHOD_LOADGLOBAL_TOKEN      0x00030000
#define METHOD_LOADATTR_TOKEN        0x00030001
#define METHOD_STOREATTR_TOKEN       0x00030002
#define METHOD_DELETEATTR_TOKEN      0x00030003
#define METHOD_STOREGLOBAL_TOKEN     0x00030004
#define METHOD_DELETEGLOBAL_TOKEN    0x00030005
#define METHOD_LOAD_ASSERTION_ERROR  0x00030006

#define METHOD_FLOAT_POWER_TOKEN    0x00050000
#define METHOD_FLOAT_FLOOR_TOKEN    0x00050001
#define METHOD_FLOAT_MODULUS_TOKEN  0x00050002

#define METHOD_ITERNEXT_TOKEN         0x00040000

#define LD_FIELDA(type, field) m_il.ld_i(offsetof(type, field)); m_il.add(); 
#define LD_FIELD(type, field) m_il.ld_i(offsetof(type, field)); m_il.add(); m_il.ld_ind_i();
#define ST_FIELD(type, field) m_il.ld_i(offsetof(type, field)); m_il.add(); m_il.st_ind_i();

extern ICorJitCompiler* g_jit;
class PythonCompiler : public IPythonCompiler {
    PyCodeObject *m_code;
    // pre-calculate some information...
    ILGenerator m_il;
    UserModule* m_module;
    Local m_lasti;

public:
    explicit PythonCompiler(PyCodeObject *code);

    int il_length() override {
        return m_il.m_il.size();
    };

    virtual void emit_rot_two(LocalKind kind = LK_Pointer);

    virtual void emit_rot_three(LocalKind kind = LK_Pointer);

    virtual void emit_rot_four(LocalKind kind = LK_Pointer);

    virtual void emit_pop_top();

    virtual void emit_dup_top();

    virtual void emit_dup_top_two();

    virtual void emit_load_name(void* name);
    virtual void emit_is_true();

    virtual void emit_push_frame();
    virtual void emit_pop_frame();
    virtual void emit_eh_trace();

    void emit_lasti_init();
    void emit_lasti_update(int index);

    virtual void emit_ret(int size);

    virtual void emit_store_name(void* name);
    virtual void emit_delete_name(void* name);
    virtual void emit_store_attr(void* name);
    virtual void emit_delete_attr(void* name);
    virtual void emit_load_attr(void* name);
    virtual void emit_store_global(void* name);
    virtual void emit_delete_global(void* name);
    virtual void emit_load_global(void* name);
    virtual void emit_delete_fast(int index);

    virtual void emit_new_tuple(size_t size);
    virtual void emit_tuple_store(size_t size);
    virtual void emit_tuple_load(size_t index);

    virtual void emit_new_list(size_t argCnt);
    virtual void emit_list_store(size_t argCnt);
    virtual void emit_list_extend();
    virtual void emit_list_to_tuple();

    virtual void emit_new_set();
    virtual void emit_set_extend();
    virtual void emit_set_update();
    virtual void emit_dict_store();
    virtual void emit_dict_store_no_decref();
    virtual void emit_dict_update();
    virtual void emit_dict_build_from_map();

    virtual void emit_unicode_joinarray();
    virtual void emit_format_value();
    virtual void emit_pyobject_str();
    virtual void emit_pyobject_repr();
    virtual void emit_pyobject_ascii();
    virtual void emit_pyobject_format();

    virtual void emit_new_dict(size_t size);
    virtual void emit_map_extend();

    virtual void emit_build_slice();

    virtual void emit_store_subscr();
    virtual void emit_delete_subscr();

    virtual void emit_unary_positive();
    virtual void emit_unary_negative();
    virtual void emit_unary_negative_float();

    virtual void emit_unary_not();

    virtual void emit_unary_not_push_int();
    virtual void emit_unary_invert();

    virtual void emit_import_name(void* name);
    virtual void emit_import_from(void* name);
    virtual void emit_import_star();

    virtual void emit_load_build_class();

    virtual void emit_unpack_sequence(Local sequence, Local sequenceStorage, Label success, size_t size);
    virtual void emit_load_array(int index);
    virtual void emit_store_to_array(Local array, int index);

    virtual void emit_unpack_ex(Local sequence, size_t leftSize, size_t rightSize, Local sequenceStorage, Local list, Local remainder);

    virtual void emit_build_vector(size_t argCnt);

    // Emits a call for the specified argument count.  If the compiler
    // can't emit a call with this number of args then it returns false,
    // and emit_call_with_tuple is used to call with a variable sized
    // tuple instead.
    virtual bool emit_call(size_t argCnt);
    virtual void emit_call_with_tuple();

    virtual void emit_kwcall_with_tuple();

    virtual void emit_call_args();
    virtual void emit_call_kwargs();
    
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
    virtual void emit_for_next();

    virtual void emit_binary_float(int opcode);
    virtual void emit_binary_object(int opcode);
    virtual void emit_tagged_int_to_float();

    virtual void emit_in();
    virtual void emit_not_in();

    virtual void emit_is(bool isNot);

    virtual void emit_compare_object(int compareType);
    virtual void emit_compare_float(int compareType);
    virtual void emit_compare_tagged_int(int compareType);

    virtual void emit_store_fast(int local);

    virtual void emit_unbound_local_check();
    virtual void emit_load_fast(int local);

    virtual Label emit_define_label();
    virtual void emit_mark_label(Label label);
    virtual void emit_branch(BranchType branchType, Label label);
    virtual void emit_compare_equal();

    virtual void emit_int(int value);
    virtual void emit_float(double value);
    virtual void emit_ptr(void *value);
    virtual void emit_bool(bool value);

    virtual void emit_unwind_eh(Local prevExc, Local prevExcVal, Local prevTraceback);
    virtual void emit_prepare_exception(Local prevExc, Local prevExcVal, Local prevTraceback);
    virtual void emit_reraise();
    virtual void emit_restore_err();
    virtual void emit_pyerr_setstring(void* exception, const char*msg);

    virtual void emit_compare_exceptions();

    // Pops a value off the stack, performing no operations related to reference counting
    virtual void emit_pop();
    // Dups the current value on the stack, performing no operations related to reference counting
    virtual void emit_dup();

    virtual void emit_incref(bool maybeTagged = false);

    virtual void emit_debug_msg(const char* msg);

    virtual void emit_load_method(void* name);
    virtual bool emit_method_call(size_t argCnt);
    virtual void emit_method_call_n();

    virtual void emit_dict_merge();

    virtual void emit_load_assertion_error();

    virtual void emit_periodic_work();

    virtual void emit_setup_annotations();

    virtual void emit_breakpoint();

    virtual void emit_inc_local(Local local, int value);
    virtual void emit_dec_local(Local local, int value);

    virtual JittedCode* emit_compile();
    virtual void lift_n_to_top(int pos);
    virtual void lift_n_to_second(int pos);
    virtual void lift_n_to_third(int pos);
    virtual void sink_top_to_n(int pos);
private:
    void load_frame();

    void load_local(int oparg);
    void decref();
    CorInfoType to_clr_type(LocalKind kind);
    void pop_top();
};

#endif