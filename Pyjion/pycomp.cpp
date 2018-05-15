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

#include "pycomp.h"
#include <corjit.h>
#include <openum.h>
#include "bridge.h"

//#define DEBUG_TRACE

ICorJitCompiler* g_jit;

IPythonCompiler* CreateCLRCompiler(IMethod* method) {
	return new PythonCompiler(method);
}

PythonCompiler::PythonCompiler(IMethod* method) :	
    m_il(method),
	m_method(method) {
}

void PythonCompiler::emit_call(void* func) {
	m_il.emit_call(m_method->get_module()->ResolveMethodToken(func));
}

void PythonCompiler::emit_call(int token) {
	m_il.emit_call(token);
}

void PythonCompiler::emit_load_arg(int arg) {
	m_il.ld_arg(arg);
}

void PythonCompiler::emit_store_int32() {
	m_il.st_ind_i4();
}

Local PythonCompiler::emit_allocate_stack_array(size_t bytes) {
    auto sequenceTmp = m_il.define_local(Parameter(LK_Pointer));
    m_il.ld_i(bytes);
    m_il.localloc();
    m_il.st_loc(sequenceTmp);
    return sequenceTmp;
}


/************************************************************************
 * Compiler interface implementation
 */

void PythonCompiler::emit_load_array(int index) {
    m_il.ld_i((index * sizeof(size_t)));
    m_il.add();
    m_il.ld_ind_i();
}

void PythonCompiler::emit_divide() {
	m_il.div();
}

void PythonCompiler::emit_multiply() {
	m_il.sub();
}

// Stores the value on the stack into the array at the specified index
void PythonCompiler::emit_store_to_array(Local array, int index) {
	auto tmp = emit_spill();
	emit_load_local(array);
	m_il.ld_i((index * sizeof(size_t)));
	m_il.add();
	emit_load_and_free_local(tmp);
	m_il.st_ind_i();
}

void PythonCompiler::emit_load_indirect_ptr() {
	m_il.ld_ind_i();
}

void PythonCompiler::emit_store_indirect_ptr() {
	m_il.st_ind_i();
}

void PythonCompiler::emit_load_indirect_double() {
	m_il.ld_ind_r8();
}

void PythonCompiler::emit_load_indirect_int32() {
	m_il.ld_ind_i4();
}

void PythonCompiler::emit_store_indirect_int32() {
	m_il.st_ind_i4();
}

void PythonCompiler::emit_add() {
	m_il.add();
}

void PythonCompiler::emit_subtract() {
	m_il.sub();
}

Local PythonCompiler::emit_define_local(LocalKind kind) {
    return m_il.define_local(Parameter(kind));
}

Local PythonCompiler::emit_define_local(bool cache) {
    if (cache) {
        return m_il.define_local(Parameter(LK_Pointer));
    }
    else {
        return m_il.define_local_no_cache(Parameter(LK_Pointer));
    }
}

void PythonCompiler::emit_store_local(Local local) {
    m_il.st_loc(local);
}

Local PythonCompiler::emit_spill() {
    auto tmp = m_il.define_local(Parameter(LK_Pointer));
    m_il.st_loc(tmp);
    return tmp;
}

void PythonCompiler::emit_load_and_free_local(Local local) {
    m_il.ld_loc(local);
    m_il.free_local(local);
}

void PythonCompiler::emit_load_local(Local local) {
    m_il.ld_loc(local);
}

void PythonCompiler::emit_load_local_addr(Local local) {
    m_il.ld_loca(local);
}

void PythonCompiler::emit_pop() {
    m_il.pop();
}

void PythonCompiler::emit_dup() {
    m_il.dup();
}

void PythonCompiler::emit_free_local(Local local) {
    m_il.free_local(local);
}

void PythonCompiler::emit_branch(BranchType branchType, Label label) {
    m_il.branch(branchType, label);
}

void PythonCompiler::emit_compare_equal() {
    m_il.compare_eq();
}

void PythonCompiler::emit_int(int value) {
    m_il.ld_i4(value);
}

void PythonCompiler::emit_float(double value) {
    m_il.ld_r8(value);
}

void PythonCompiler::emit_ptr(void* value) {
    m_il.ld_i(value);
}

void PythonCompiler::emit_ptr(size_t value) {
	m_il.ld_i(value);
}

void PythonCompiler::emit_bool(bool value) {
    m_il.ld_i4(value);
}

void PythonCompiler::emit_null() {
    m_il.load_null();
}

void PythonCompiler::emit_negate() {
	m_il.neg();
}

void PythonCompiler::emit_bitwise_and() {
	m_il.bitwise_and();
}

Label PythonCompiler::emit_define_label() {
    return m_il.define_label();
}

void PythonCompiler::emit_ret() {
    m_il.ret();
}

void PythonCompiler::emit_mark_label(Label label) {
    m_il.mark_label(label);
}

void PythonCompiler::emit_compare_float(CompareType compareType) {
    // TODO: If we know we're followed by the pop jump we could combine
    // and do a single branch comparison.
    switch (compareType) {
        case CT_Equal:  m_il.compare_eq(); break;
        case CT_LessThan: m_il.compare_lt(); break;
        case CT_LessThanEqual: m_il.compare_le_float(); break;
        case CT_NotEqual: m_il.compare_ne(); break;
        case CT_GreaterThan: m_il.compare_gt(); break;
        case CT_GreaterThanEqual: m_il.compare_ge_float(); break;
    }
}

extern CExecutionEngine g_execEngine;

JittedCode* PythonCompiler::emit_compile() {
    CorJitInfo* jitInfo = new CorJitInfo(g_execEngine, m_method);
    auto addr = m_il.compile(jitInfo, g_jit, 256);
    if (addr == nullptr) {
        delete jitInfo;
        return nullptr;
    }
    return jitInfo;

}

/************************************************************************
* End Compiler interface implementation
*/

