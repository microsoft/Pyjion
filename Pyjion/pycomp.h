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

#ifndef __PYCOMP_H__
#define __PYCOMP_H__

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
#include "compdata.h"

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
#define METHOD_UNWIND_EH					0x0000003F
#define METHOD_PY_CHECKFUNCTIONRESULT		0x00000040
#define METHOD_PY_PUSHFRAME					0x00000041
#define METHOD_PY_POPFRAME					0x00000042
#define METHOD_PY_IMPORTNAME				0x00000043
#define METHOD_PY_FANCYCALL					0x00000044
#define METHOD_PY_IMPORTFROM				0x00000045
#define METHOD_PY_IMPORTSTAR				0x00000046
#define METHOD_PY_FUNC_SET_ANNOTATIONS		0x00000047
#define METHOD_PY_FUNC_SET_KW_DEFAULTS		0x00000048
#define METHOD_IS							0x00000049
#define METHOD_ISNOT						0x0000004A
#define METHOD_IS_BOOL						0x0000004B
#define METHOD_ISNOT_BOOL					0x0000004C
#define METHOD_GETITER_OPTIMIZED_TOKEN		0x0000004D
#define METHOD_COMPARE_EXCEPTIONS_INT		0x0000004E
#define METHOD_CONTAINS_INT_TOKEN           0x0000004F
#define METHOD_NOTCONTAINS_INT_TOKEN        0x00000050
#define METHOD_UNARY_NOT_INT                0x00000051
#define METHOD_RICHEQUALS_GENERIC_TOKEN     0x00000052
#define METHOD_FLOAT_FROM_DOUBLE            0x00000053
#define METHOD_BOOL_FROM_LONG               0x00000054
#define METHOD_FLOAT_ZERO_DIV               0x00000055

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

#define STACK_KIND_OBJECT true
#define STACK_KIND_VALUE  false

#define LD_FIELDA(type, field) m_il.ld_i(offsetof(type, field)); m_il.add(); 
#define LD_FIELD(type, field) m_il.ld_i(offsetof(type, field)); m_il.add(); m_il.ld_ind_i();
#define ST_FIELD(type, field) m_il.ld_i(offsetof(type, field)); m_il.add(); m_il.st_ind_i();

#define BLOCK_CONTINUES 0x01
#define BLOCK_RETURNS	0x02
#define BLOCK_BREAKS	0x04

#define NEXTARG() *(unsigned short*)&m_byteCode[i + 1]; i+= 2

extern ICorJitCompiler* g_jit;

struct ExceptionVars {
    Local PrevExc, PrevExcVal, PrevTraceback;

    ExceptionVars() {
    }

    ExceptionVars(Local prevExc, Local prevExcVal, Local prevTraceback) {
        PrevExc = prevExc;
        PrevExcVal = prevExcVal;
        PrevTraceback = prevTraceback;
    }
};

struct EhInfo {
    bool IsFinally;
    int Flags;

    EhInfo(bool isFinally) {
        IsFinally = isFinally;
        Flags = 0;
    }
};

struct BlockInfo {
    Label Raise,		// our raise stub label, prepares the exception
        ReRaise,		// our re-raise stub label, prepares the exception w/o traceback update
        ErrorTarget;	// the actual label for the handler
    int EndOffset, Kind, Flags, ContinueOffset;
    size_t BlockId;
    ExceptionVars ExVars;
    Local LoopVar; //, LoopOpt1, LoopOpt2;
    vector<bool> Stack;

    BlockInfo() {
    }

    BlockInfo(vector<bool> stack, size_t blockId, Label raise, Label reraise, Label errorTarget, int endOffset, int kind, int flags = 0, int continueOffset = 0) {
        Stack = stack;
        BlockId = blockId;
        Raise = raise;
        ReRaise = reraise;
        ErrorTarget = errorTarget;
        EndOffset = endOffset;
        Kind = kind;
        Flags = flags;
        ContinueOffset = continueOffset;
    }
};

class PythonCompiler : public IPythonCompiler {
    PyCodeObject *m_code;
    // pre-calculate some information...
    ILGenerator m_il;
    // Stores information for a stack allocated local used for sequence unpacking.  We need to allocate
    // one of these when we enter the method, and we use it if we don't have a sequence we can efficiently
    // unpack.
    unordered_map<int, Local> m_sequenceLocals;
    unsigned char *m_byteCode;
    size_t m_size;
    // m_blockStack is like Python's f_blockstack which lives on the frame object, except we only maintain
    // it at compile time.  Blocks are pushed onto the stack when we enter a loop, the start of a try block,
    // or into a finally or exception handler.  Blocks are popped as we leave those protected regions.
    // When we pop a block associated with a try body we transform it into the correct block for the handler
    vector<BlockInfo> m_blockStack;
    // All of the exception handlers defined in the method.  After generating the method we'll generate helper
    // targets which dispatch to each of the handlers.
    vector<BlockInfo> m_allHandlers;
    // Tracks the state for the handler block, used for END_FINALLY processing.  We push these with a SETUP_EXCEPT/
    // SETUP_FINALLY, update them when we hit the POP_EXCEPT so we have information about the try body, and then
    // finally pop them when we hit the SETUP_FINALLY.  These are independent from the block stack because they only
    // contain information about exceptions, and don't change as we transition from the body of the try to the body
    // of the handler.
    vector<EhInfo> m_ehInfo;
    // Labels that map from a Python byte code offset to an ilgen label.  This allows us to branch to any
    // byte code offset.
    unordered_map<int, Label> m_offsetLabels;
    // Tracks the depth of the Python stack
    size_t m_blockIds;
    // Tracks the current depth of the stack,  as well as if we have an object reference that needs to be freed.
    // True (STACK_KIND_OBJECT) if we have an object, false (STACK_KIND_VALUE) if we don't
    vector<bool> m_stack;
    // Tracks the state of the stack when we perform a branch.  We copy the existing state to the map and
    // reload it when we begin processing at the stack.
    unordered_map<int, vector<bool>> m_offsetStack;
    vector<vector<Label>> m_raiseAndFree;
    UserModule* m_module;
    unordered_map<int, bool> m_assignmentState;
    unordered_map<int, unordered_map<AbstractValueKind, Local>> m_optLocals;
    AbstractInterpreter m_interp;
    Local m_retValue, m_tb, m_ehVal, m_excType;
	Local m_lasti;
    Label m_retLabel;

public:
    PythonCompiler(PyCodeObject *code);
	JittedCode* compile();

    virtual void emit_rot_two();

    virtual void emit_rot_three();

    virtual void emit_pop_top();

    virtual void emit_dup_top();

    virtual void emit_dup_top_two();

    virtual void emit_compare_op(int compare);
    virtual void emit_jump_if_or_pop(bool isTrue, int index);
    virtual void emit_pop_jump_if(bool isTrue, int index);
    virtual void emit_load_name(PyObject* name);

	virtual void emit_push_frame();
	virtual void emit_pop_frame();
	virtual void emit_eh_trace();

	void emit_lasti_init();
	void emit_lasti_update(int index);

	virtual void emit_check_function_result();

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

	virtual void emit_new_set();
	virtual void emit_set_store(size_t argCnt);
	virtual void emit_dict_store();

	virtual void emit_new_dict(size_t size);

    virtual void emit_build_map(size_t size);
    virtual void emit_build_slice();
    virtual void emit_build_set(size_t size);

    virtual void emit_store_subscr();
    virtual void emit_delete_subscr();

    virtual void emit_unary_positive();
    virtual void emit_unary_negative();
    virtual void emit_unary_not_int();
    virtual void emit_unary_not();
    virtual void emit_unary_invert();

    virtual void emit_import_name(PyObject* name);
    virtual void emit_import_from(PyObject* name);
    virtual void emit_import_star();

    void emit_pop_except();
    void emit_load_build_class();

	void emit_unpack_sequence(Local sequence, Local sequenceStorage, Label success, size_t size);
	void emit_load_array(int index);

    void unpack_sequence(size_t size, int opcode);
    void emit_unpack_ex(Local sequence, size_t leftSize, size_t rightSize, Local sequenceStorage, Local list, Local remainder);

    void emit_fancy_call();
	// Emits a call for the specified argument count.  If the compiler
	// can't emit a call with this number of args then it returns false,
	// and emit_call_with_tuple is used to call with a variable sized
	// tuple instead.
	bool emit_call(size_t argCnt);
	void emit_call_with_tuple();
	void emit_call(size_t argCnt, size_t kwArgCnt);

	void emit_new_function();
	void emit_set_closure();
	void emit_set_annotations();
	void emit_set_kw_defaults();
	void emit_set_defaults();

    void emit_load_deref(int index);
    void emit_store_deref(int index);
    void emit_delete_deref(int index);
    void emit_load_closure(int index);

	Local emit_spill();
	void emit_store_local(Local local);
	void emit_load_local(Local local, bool free = true);
	Local emit_define_local(bool cache = true);
	void emit_free_local(Local local);
	Local emit_allocate_stack_array(size_t elements);

    void emit_set_add();
    void emit_map_add();
    void emit_list_append();

    void emit_raise_varargs();

	void emit_null();

    void emit_print_expr();
    void emit_load_classderef(int index);
    void emit_getiter();
    //void emit_getiter_opt();
	void emit_for_next(Label processValue, Local iterValue);
    
    void emit_binary_float(int opcode);
    void emit_binary_object(int opcode);
    
    void emit_in_int();
    void emit_in();
    void emit_not_in_int();
    void emit_not_in();

    void emit_is_int(bool isNot);

    void emit_is(bool isNot);

    void emit_compare_object(int compareType);
    void emit_compare_float(int compareType);
    bool emit_compare_object_ret_bool(int compareType);

	void emit_store_float(int local);
	void emit_store_fast(int local);

	void emit_load_fast(int local, bool checkUnbound);
	void emit_load_float(int local);

	virtual Label emit_define_label();
	virtual void emit_mark_label(Label label);
	virtual void emit_branch(BranchType branchType, Label label);
	virtual void emit_compare_equal();

	virtual void emit_int(int value);
	virtual void emit_float(double value);
	virtual void emit_py_object(PyObject* value);

	virtual void emit_prepare_exception(Local prevExc, Local prevExcVal, Local prevTraceback, bool includeTbAndValue);
	virtual void emit_restore_err();
	virtual void emit_restore_err(Local finallyReason);

	virtual void emit_compare_exceptions();
	virtual void emit_compare_exceptions_int();

	// Pops a value off the stack, performing no operations related to reference counting
	virtual void emit_pop();
	// Dups the current value on the stack, performing no operations related to reference counting
	virtual void emit_dup();

	virtual void emit_box_float();
	virtual void emit_box_bool();

	virtual JittedCode* emit_compile();

	
private:
	void make_function(int posdefaults, int kwdefaults, int num_anotations, bool isClosure);
	void fancy_call(int na, int nk, int flags);
	bool can_skip_lasti_update(int opcodeIndex);
    void load_frame();

    void load_local(int oparg);
    void incref();

    void decref();

    void build_tuple(size_t argCnt);
    void build_list(size_t argCnt);
	void build_set(size_t argCnt);

	void unpack_ex(size_t size, int opcode);

    void build_map(size_t argCnt);

    Label getOffsetLabel(int jumpTo);
	void for_iter(int loopIndex, int opcodeIndex, BlockInfo *loopInfo);

    // Checks to see if we have a null value as the last value on our stack
    // indicating an error, and if so, branches to our current error handler.
    void emit_error_check(int curIndex, const char* reason);
    void emit_error_check();
    void emit_int_error_check();

    vector<Label>& getRaiseAndFreeLabels(size_t blockId);

    void branch_raise();

    void clean_stack_for_reraise();
    // Checks to see if we have a non-zero error code on the stack, and if so,
    // branches to the current error handler.  Consumes the error code in the process
    void emit_int_error_check(int curIndex);

    void unwind_eh(ExceptionVars& exVars);

    BlockInfo get_ehblock();

    void mark_offset_label(int index);

    void preprocess();

    // Frees our iteration temporary variable which gets allocated when we hit
    // a FOR_ITER.  Used when we're breaking from the current loop.
    void free_iter_local();

	void jump_absolute(int index);

    // Frees all of the iteration variables in a range. Used when we're
    // going to branch to a finally through multiple loops.
    void free_all_iter_locals(size_t to = 0);

    // Frees all of our iteration variables.  Used when we're unwinding the function
    // on an exception.
    void free_iter_locals_on_exception();

    void dec_stack(size_t size = 1);

    void inc_stack(size_t size = 1, bool kind = STACK_KIND_OBJECT);

    // Handles POP_JUMP_IF_FALSE/POP_JUMP_IF_TRUE with a possible error value on the stack.
    // If the value on the stack is -1, we branch to the current error handler.
    // Otherwise branches based if the current value is true/false based upon the current opcode 
    void branch_or_error(int& i);

    // Handles POP_JUMP_IF_FALSE/POP_JUMP_IF_TRUE with a bool value known to be on the stack.
    // Branches based if the current value is true/false based upon the current opcode 
    void branch(int& i);

    void call_optimizing_function(int baseFunction);


    CorInfoType to_clr_type(AbstractValueKind kind);

    Local get_optimized_local(int index, AbstractValueKind kind);

    void store_fast(int local, int opcodeIndex);

    void load_const(int constIndex, int opcodeIndex);

    void return_value(int opcodeIndex);

    void compare_op(int compareType, int& i, int opcodeIndex);

    void load_fast(int local, int opcodeIndex);

    JittedCode* compile_worker();
};

#endif