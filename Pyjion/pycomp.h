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
#include <winwrap.h>

#include <windef.h>
#include <winnt.h>
#include <stdlib.h>
#include <wchar.h>
#include <objbase.h>
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <assert.h>

#include "ipycomp.h"
#include "jitinfo.h"
#include "ilgen.h"

extern ICorJitCompiler* g_jit;

class PythonCompiler : public IPythonCompiler {
    // pre-calculate some information...
    ILGenerator m_il;
    IMethod* m_method;

public:
    PythonCompiler(IMethod* method);

	virtual void emit_compare_float(CompareType compareType);
	virtual void emit_ret();
    virtual void emit_load_array(int index);
    virtual void emit_store_to_array(Local array, int index);

    virtual Local emit_spill();
    virtual void emit_store_local(Local local);

    virtual void emit_load_local(Local local);
    virtual void emit_load_local_addr(Local local);
    virtual void emit_load_and_free_local(Local local);
    virtual Local emit_define_local(bool cache);
    virtual Local emit_define_local(LocalKind kind = LK_Pointer);
    virtual void emit_free_local(Local local);
    virtual Local emit_allocate_stack_array(size_t elements);

    virtual void emit_null();

    virtual Label emit_define_label();
    virtual void emit_mark_label(Label label);
    virtual void emit_branch(BranchType branchType, Label label);
    virtual void emit_compare_equal();

    virtual void emit_int(int value);
    virtual void emit_float(double value);
    virtual void emit_ptr(void *value);
	virtual void emit_ptr(size_t value);
    virtual void emit_bool(bool value);

	virtual void emit_store_int32();
	virtual void emit_load_arg(int arg);
	virtual void emit_add();

    // Pops a value off the stack, performing no operations related to reference counting
    virtual void emit_pop();
    // Dups the current value on the stack, performing no operations related to reference counting
    virtual void emit_dup();

    virtual JittedCode* emit_compile();

	virtual void emit_call(void* func);
	virtual void emit_call(int token);

	virtual void emit_store_indirect_ptr();
	virtual void emit_load_indirect_ptr();
	virtual void emit_load_indirect_double();
	virtual void emit_negate();
	virtual void emit_bitwise_and();
	virtual void emit_load_indirect_int32();
	virtual void emit_store_indirect_int32();

	virtual void emit_divide();
	virtual void emit_multiply();
	virtual void emit_subtract();
};

#endif