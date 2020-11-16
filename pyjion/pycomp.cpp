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


#include <windows.h>
#include <corjit.h>

#ifdef WINDOWS
#include <libloaderapi.h>
typedef void(__cdecl* JITSTARTUP)(ICorJitHost*);
#endif

#include "pycomp.h"

using namespace std;


CCorJitHost g_jitHost;

void CeeInit() {
#ifdef WINDOWS
    auto clrJitHandle = LoadLibrary(TEXT("clrjit.dll"));
    if (clrJitHandle == nullptr) {
        printf("Failed to load clrjit.dll");
        exit(40);
    }
    auto jitStartup = (JITSTARTUP)GetProcAddress(GetModuleHandle(TEXT("clrjit.dll")), "jitStartup");
    if (jitStartup != nullptr)
        jitStartup(&g_jitHost);
    else {
        printf("Failed to load jitStartup() from clrjit.dll");
        exit(41);
    }
#else
	jitStartup(&g_jitHost);
#endif
}

class InitHolder {
public:
    InitHolder() {
        CeeInit();
    }
};

InitHolder g_initHolder;

Module g_module;
ICorJitCompiler* g_jit;

PythonCompiler::PythonCompiler(PyCodeObject *code) :
    // TODO : This shows f(long, long) -> long as a hardcoded value, but no introspection is done on the types.
    m_il(m_module = new UserModule(g_module),
        CORINFO_TYPE_NATIVEINT,
        std::vector < Parameter > {Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT) }) {
    this->m_code = code;
    m_lasti = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
}

void PythonCompiler::load_frame() {
    m_il.ld_arg(1);
}

void PythonCompiler::emit_push_frame() {
    load_frame();
    m_il.emit_call(METHOD_PY_PUSHFRAME, 1);
}

void PythonCompiler::emit_pop_frame() {
    load_frame();
    m_il.emit_call(METHOD_PY_POPFRAME, 1);
}

void PythonCompiler::emit_eh_trace() {
    load_frame();
    m_il.emit_call(METHOD_EH_TRACE, 1);
}

void PythonCompiler::emit_lasti_init() {
    load_frame();
    m_il.ld_i(offsetof(PyFrameObject, f_lasti));
    m_il.add();
    m_il.st_loc(m_lasti);
}

void PythonCompiler::emit_lasti_update(int index) {
    m_il.ld_loc(m_lasti);
    m_il.ld_i4(index);
    m_il.st_ind_i4();
}

void PythonCompiler::load_local(int oparg) {
    load_frame();
    m_il.ld_i(offsetof(PyFrameObject, f_localsplus) + oparg * sizeof(size_t));
    m_il.add();
    m_il.ld_ind_i();
}

void PythonCompiler::emit_incref(bool maybeTagged) {
    Label tagged, done;
    if (maybeTagged) {
        m_il.dup();
        m_il.ld_i(1);
        m_il.bitwise_and();
        tagged = m_il.define_label();
        done = m_il.define_label();
        m_il.branch(BranchTrue, tagged);
    }

    LD_FIELDA(PyObject, ob_refcnt);
    m_il.dup();
    m_il.ld_ind_i4();
    m_il.ld_i4(1);
    m_il.add();
    m_il.st_ind_i4();

    if (maybeTagged) {
        m_il.branch(BranchAlways, done);

        m_il.mark_label(tagged);
        m_il.pop();

        m_il.mark_label(done);
    }
}

void PythonCompiler::emit_breakpoint(){
    // Emits a breakpoint in the IL. useful for debugging
    m_il.brk();
}

void PythonCompiler::decref() {
    m_il.emit_call(METHOD_DECREF_TOKEN, 1);
}

Local PythonCompiler::emit_allocate_stack_array(size_t bytes) {
    auto sequenceTmp = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    m_il.ld_i(bytes);
    m_il.localloc();
    m_il.st_loc(sequenceTmp);
    return sequenceTmp;
}


/************************************************************************
 * Compiler interface implementation
 */

void PythonCompiler::emit_unbound_local_check() {
    //// TODO: Remove this check for definitely assigned values (e.g. params w/ no dels, 
    //// locals that are provably assigned)
    m_il.emit_call(METHOD_UNBOUND_LOCAL, 1);
}

void PythonCompiler::emit_load_fast(int local) {
    load_local(local);
}

CorInfoType PythonCompiler::to_clr_type(LocalKind kind) {
    switch (kind) {
        case LK_Float: return CORINFO_TYPE_DOUBLE;
        case LK_Int: return CORINFO_TYPE_INT;
        case LK_Bool: return CORINFO_TYPE_BOOL;
        case LK_Pointer: return CORINFO_TYPE_PTR;
    }
    return CORINFO_TYPE_NATIVEINT;
}


void PythonCompiler::emit_store_fast(int local) {
    // TODO: Move locals out of the Python frame object and into real locals

    auto valueTmp = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    m_il.st_loc(valueTmp);

    // load the value onto the IL stack, we'll decref it after we replace the
    // value in the frame object so that we never have a freed object in the
    // frame object.
    load_local(local);

    load_frame();
    m_il.ld_i(offsetof(PyFrameObject, f_localsplus) + local * sizeof(size_t));
    m_il.add();

    m_il.ld_loc(valueTmp);

    m_il.st_ind_i();

    m_il.free_local(valueTmp);

    // now dec ref the old value potentially freeing it.
    decref();
}

void PythonCompiler::emit_rot_two(LocalKind kind) {
    auto top = m_il.define_local(Parameter(to_clr_type(kind)));
    auto second = m_il.define_local(Parameter(to_clr_type(kind)));

    m_il.st_loc(top);
    m_il.st_loc(second);

    m_il.ld_loc(top);
    m_il.ld_loc(second);

    m_il.free_local(top);
    m_il.free_local(second);
}

void PythonCompiler::emit_rot_three(LocalKind kind) {
    auto top = m_il.define_local(Parameter(to_clr_type(kind)));
    auto second = m_il.define_local(Parameter(to_clr_type(kind)));
    auto third = m_il.define_local(Parameter(to_clr_type(kind)));

    m_il.st_loc(top);
    m_il.st_loc(second);
    m_il.st_loc(third);

    m_il.ld_loc(top);
    m_il.ld_loc(third);
    m_il.ld_loc(second);

    m_il.free_local(top);
    m_il.free_local(second);
    m_il.free_local(third);
}

void PythonCompiler::emit_rot_four(LocalKind kind) {
    auto top = m_il.define_local(Parameter(to_clr_type(kind)));
    auto second = m_il.define_local(Parameter(to_clr_type(kind)));
    auto third = m_il.define_local(Parameter(to_clr_type(kind)));
    auto fourth = m_il.define_local(Parameter(to_clr_type(kind)));

    m_il.st_loc(top);
    m_il.st_loc(second);
    m_il.st_loc(third);
    m_il.st_loc(fourth);

    m_il.ld_loc(top);
    m_il.ld_loc(fourth);
    m_il.ld_loc(third);
    m_il.ld_loc(second);

    m_il.free_local(top);
    m_il.free_local(second);
    m_il.free_local(third);
    m_il.free_local(fourth);
}

void PythonCompiler::lift_n_to_second(int pos){
    if (pos == 1)
        return ; // already is second
    vector<Local> tmpLocals (pos-1);

    auto top = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
    m_il.st_loc(top);

    // dump stack up to n
    for (int i = 0 ; i < pos - 1 ; i ++){
        auto loc = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
        tmpLocals[i] = loc;
        m_il.st_loc(loc);
    }

    // pop n
    auto n = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
    m_il.st_loc(n);

    // recover stack
    for (auto& loc: tmpLocals){
        m_il.ld_loc(loc);
        m_il.free_local(loc);
    }

    // push n (so its second)
    m_il.ld_loc(n);
    m_il.free_local(n);

    // push top
    m_il.ld_loc(top);
    m_il.free_local(top);
}

void PythonCompiler::lift_n_to_third(int pos){
    if (pos == 1)
        return ; // already is third
    vector<Local> tmpLocals (pos-2);

    auto top = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
    m_il.st_loc(top);

    auto second = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
    m_il.st_loc(second);

    // dump stack up to n
    for (int i = 0 ; i < pos - 2 ; i ++){
        auto loc = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
        tmpLocals[i] = loc;
        m_il.st_loc(loc);
    }

    // pop n
    auto n = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
    m_il.st_loc(n);

    // recover stack
    for (auto& loc: tmpLocals){
        m_il.ld_loc(loc);
        m_il.free_local(loc);
    }

    // push n (so its third)
    m_il.ld_loc(n);
    m_il.free_local(n);

    // push second
    m_il.ld_loc(second);
    m_il.free_local(second);

    // push top
    m_il.ld_loc(top);
    m_il.free_local(top);
}

void PythonCompiler::sink_top_to_n(int pos){
    if (pos == 0)
        return ; // already is at the correct position
    vector<Local> tmpLocals (pos);

    auto top = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
    m_il.st_loc(top);

    // dump stack up to n
    for (int i = 0 ; i < pos ; i ++){
        auto loc = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
        tmpLocals[i] = loc;
        m_il.st_loc(loc);
    }

    // push n
    m_il.ld_loc(top);
    m_il.free_local(top);

    // recover stack
    for (auto& loc: tmpLocals){
        m_il.ld_loc(loc);
        m_il.free_local(loc);
    }
}

void PythonCompiler::lift_n_to_top(int pos){
    vector<Local> tmpLocals (pos);

    // dump stack up to n
    for (int i = 0 ; i < pos ; i ++){
        auto loc = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
        tmpLocals[i] = loc;
        m_il.st_loc(loc);
    }

    // pop n
    auto n = m_il.define_local(Parameter(to_clr_type(LK_Pointer)));
    m_il.st_loc(n);


    // recover stack
    for (auto& loc: tmpLocals){
        m_il.ld_loc(loc);
        m_il.free_local(loc);
    }

    // push n (so its at the top)
    m_il.ld_loc(n);
    m_il.free_local(n);
}

void PythonCompiler::emit_pop_top() {
    decref();
}
// emit_pop_top is for the POP_TOP opcode, which should pop the stack AND decref. pop_top is just for pop'ing the value.
void PythonCompiler::pop_top() {
    m_il.pop();
}

void PythonCompiler::emit_dup_top() {
    // Dup top and incref
    m_il.dup();
    m_il.dup();
    emit_incref(true);
}

void PythonCompiler::emit_dup_top_two() {
    auto top = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    auto second = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));

    m_il.st_loc(top);
    m_il.st_loc(second);

    m_il.ld_loc(second);
    m_il.ld_loc(top);
    m_il.ld_loc(second);
    m_il.ld_loc(top);

    m_il.ld_loc(top);
    emit_incref(true);
    m_il.ld_loc(second);
    emit_incref(true);

    m_il.free_local(top);
    m_il.free_local(second);
}

void PythonCompiler::emit_dict_build_from_map() {
    m_il.emit_call(METHOD_BUILD_DICT_FROM_TUPLES, 1);
}

void PythonCompiler::emit_new_list(size_t argCnt) {
    m_il.ld_i(argCnt);
    m_il.emit_call(METHOD_PYLIST_NEW, 1);
}

void PythonCompiler::emit_load_assertion_error() {
    m_il.emit_call(METHOD_LOAD_ASSERTION_ERROR, 0);
}

void PythonCompiler::emit_list_store(size_t argCnt) {
    auto valueTmp = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    auto listTmp = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    auto listItems = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));

    m_il.dup();
    m_il.st_loc(listTmp);

    // load the address of the list item...
    m_il.ld_i(offsetof(PyListObject, ob_item));
    m_il.add();
    m_il.ld_ind_i();

    m_il.st_loc(listItems);

    for (size_t i = 0, arg = argCnt - 1; i < argCnt; i++, arg--) {
        // save the argument into a temporary...
        m_il.st_loc(valueTmp);

        // load the address of the list item...
        m_il.ld_loc(listItems);
        m_il.ld_i(arg * sizeof(size_t));
        m_il.add();

        // reload the value
        m_il.ld_loc(valueTmp);

        // store into the array
        m_il.st_ind_i();
    }

    // update the size of the list...
    m_il.ld_loc(listTmp);
    m_il.dup();
    m_il.ld_i(offsetof(PyVarObject, ob_size));
    m_il.add();
    m_il.ld_i(argCnt);
    m_il.st_ind_i();

    m_il.free_local(valueTmp);
    m_il.free_local(listTmp);
    m_il.free_local(listItems);
}

void PythonCompiler::emit_build_vector(size_t argCnt){
    vector<Local> values;
    // dump all the values into local tmp variables.
    for (int i = 0 ; i < argCnt; i++){
        auto tmpVar = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
        m_il.st_loc(tmpVar);
        values.push_back(tmpVar);
    }
    m_il.new_array(argCnt);
    auto array = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    m_il.st_loc(array);
    for (int i = argCnt ; i > 0; i -- ){
        m_il.st_elem(array, i, values[i]);
    }
    ///  TODO : free local vars
    for (int i = 0 ; i < argCnt; i++){
        emit_free_local(values[i]);
    }
    emit_load_and_free_local(array);
}

void PythonCompiler::emit_list_extend() {
    m_il.emit_call(METHOD_EXTENDLIST_TOKEN, 2);
}

void PythonCompiler::emit_list_to_tuple() {
    m_il.emit_call(METHOD_LISTTOTUPLE_TOKEN, 1);
}

void PythonCompiler::emit_new_set() {
    m_il.load_null();
    m_il.emit_call(METHOD_PYSET_NEW, 1);
}

void PythonCompiler::emit_pyobject_str() {
	m_il.emit_call(METHOD_PYOBJECT_STR, 1);
}

void PythonCompiler::emit_pyobject_repr() {
	m_il.emit_call(METHOD_PYOBJECT_REPR, 1);
}

void PythonCompiler::emit_pyobject_ascii() {
	m_il.emit_call(METHOD_PYOBJECT_ASCII, 1);
}

void PythonCompiler::emit_pyobject_format() {
	m_il.emit_call(METHOD_FORMAT_OBJECT, 2);
}

void PythonCompiler::emit_unicode_joinarray() {
	m_il.emit_call(METHOD_PYUNICODE_JOINARRAY, 2);
}

void PythonCompiler::emit_format_value() {
	m_il.emit_call(METHOD_FORMAT_VALUE, 1);
}

void PythonCompiler::emit_set_extend() {
    m_il.emit_call(METHOD_SETUPDATE_TOKEN, 2);
}

void PythonCompiler::emit_new_dict(size_t size) {
    m_il.ld_i(size);
    m_il.emit_call(METHOD_PYDICT_NEWPRESIZED, 1);
}

void PythonCompiler::emit_dict_store() {
    m_il.emit_call(METHOD_STOREMAP_TOKEN, 3);
}

void PythonCompiler::emit_dict_store_no_decref() {
	m_il.emit_call(METHOD_STOREMAP_NO_DECREF_TOKEN, 3);
}

void PythonCompiler::emit_map_extend() {
    m_il.emit_call(METHOD_DICTUPDATE_TOKEN, 2);
}

void PythonCompiler::emit_is_true() {
    m_il.emit_call(METHOD_PYOBJECT_ISTRUE, 1);
}

void PythonCompiler::emit_load_name(void* name) {
    load_frame();
    m_il.ld_i(name);
    m_il.emit_call(METHOD_LOADNAME_TOKEN, 2);
}

void PythonCompiler::emit_store_name(void* name) {
    load_frame();
    m_il.ld_i(name);
    m_il.emit_call(METHOD_STORENAME_TOKEN, 3);
}

void PythonCompiler::emit_delete_name(void* name) {
    load_frame();
    m_il.ld_i(name);
    m_il.emit_call(METHOD_DELETENAME_TOKEN, 2);
}

void PythonCompiler::emit_store_attr(void* name) {
    m_il.ld_i(name);
    m_il.emit_call(METHOD_STOREATTR_TOKEN, 3);
}

void PythonCompiler::emit_delete_attr(void* name) {
    m_il.ld_i(name);
    m_il.emit_call(METHOD_DELETEATTR_TOKEN, 2);
}

void PythonCompiler::emit_load_attr(void* name) {
    m_il.ld_i(name);
    m_il.emit_call(METHOD_LOADATTR_TOKEN, 2);
}

void PythonCompiler::emit_store_global(void* name) {
    // value is on the stack
    load_frame();
    m_il.ld_i(name);
    m_il.emit_call(METHOD_STOREGLOBAL_TOKEN, 3);
}

void PythonCompiler::emit_delete_global(void* name) {
    load_frame();
    m_il.ld_i(name);
    m_il.emit_call(METHOD_DELETEGLOBAL_TOKEN, 2);
}

void PythonCompiler::emit_load_global(void* name) {
    load_frame();
    m_il.ld_i(name);
    m_il.emit_call(METHOD_LOADGLOBAL_TOKEN, 2);
}

void PythonCompiler::emit_delete_fast(int index) {
    load_local(index);
    load_frame();
    m_il.ld_i(offsetof(PyFrameObject, f_localsplus) + index * sizeof(size_t));
    m_il.add();
    m_il.load_null();
    m_il.st_ind_i();
    decref();
}

void PythonCompiler::emit_new_tuple(size_t size) {
    if (size == 0) {
        m_il.ld_i(PyTuple_New(0));
        m_il.dup();
        emit_incref(false);
    }
    else {
        m_il.ld_i(size);
        m_il.emit_call(METHOD_PYTUPLE_NEW, 1);
    }
}

// Loads the specified index from a tuple that's already on the stack
void PythonCompiler::emit_tuple_load(size_t index) {
	m_il.ld_i(index * sizeof(size_t) + offsetof(PyTupleObject, ob_item));
	m_il.add();
	m_il.ld_ind_i();
}

void PythonCompiler::emit_tuple_store(size_t argCnt) {
    auto valueTmp = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    auto tupleTmp = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    m_il.st_loc(tupleTmp);

    for (size_t i = 0, arg = argCnt - 1; i < argCnt; i++, arg--) {
        // save the argument into a temporary...
        m_il.st_loc(valueTmp);

        // load the address of the tuple item...
        m_il.ld_loc(tupleTmp);
        m_il.ld_i(arg * sizeof(size_t) + offsetof(PyTupleObject, ob_item));
        m_il.add();

        // reload the value
        m_il.ld_loc(valueTmp);

        // store into the array
        m_il.st_ind_i();
    }
    m_il.ld_loc(tupleTmp);

    m_il.free_local(valueTmp);
    m_il.free_local(tupleTmp);
}

void PythonCompiler::emit_store_subscr() {
    // stack is value, container, index
    m_il.emit_call(METHOD_STORESUBSCR_TOKEN, 3);
}

void PythonCompiler::emit_delete_subscr() {
    // stack is container, index
    m_il.emit_call(METHOD_DELETESUBSCR_TOKEN, 2);
}

void PythonCompiler::emit_build_slice() {
    m_il.emit_call(METHOD_BUILD_SLICE, 3);
}

void PythonCompiler::emit_unary_positive() {
    m_il.emit_call(METHOD_UNARY_POSITIVE, 1);
}

void PythonCompiler::emit_unary_negative() {
    m_il.emit_call(METHOD_UNARY_NEGATIVE, 1);
}

void PythonCompiler::emit_unary_not_push_int() {
    m_il.emit_call(METHOD_UNARY_NOT_INT, 1);
}

void PythonCompiler::emit_unary_not() {
    m_il.emit_call(METHOD_UNARY_NOT, 1);
}

void PythonCompiler::emit_unary_negative_float() {
    m_il.neg();
}

void PythonCompiler::emit_unary_invert() {
    m_il.emit_call(METHOD_UNARY_INVERT, 1);
}

void PythonCompiler::emit_import_name(void* name) {
    m_il.ld_i(name);
    load_frame();
    m_il.emit_call(METHOD_PY_IMPORTNAME, 4);
}

void PythonCompiler::emit_import_from(void* name) {
    m_il.dup();
    m_il.ld_i(name);
    m_il.emit_call(METHOD_PY_IMPORTFROM, 2);
}

void PythonCompiler::emit_import_star() {
    load_frame();
    m_il.emit_call(METHOD_PY_IMPORTSTAR, 2);
}

void PythonCompiler::emit_load_build_class() {
    load_frame();
    m_il.emit_call(METHOD_GETBUILDCLASS_TOKEN, 1);
}

void PythonCompiler::emit_unpack_sequence(Local sequence, Local sequenceStorage, Label success, size_t size) {
    // load the iterable, the count, and our temporary 
    // storage if we need to iterate over the object.
    m_il.ld_loc(sequence);
    m_il.ld_i(size);
    m_il.ld_loc(sequenceStorage);
    m_il.emit_call(METHOD_UNPACK_SEQUENCE_TOKEN, 3);

    m_il.dup();
    m_il.load_null();
    m_il.branch(BranchNotEqual, success);
    m_il.pop();
    m_il.ld_loc(sequence);
    decref();
}

void PythonCompiler::emit_load_array(int index) {
    m_il.ld_i((index * sizeof(size_t)));
    m_il.add();
    m_il.ld_ind_i();
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

Local PythonCompiler::emit_define_local(LocalKind kind) {
    return m_il.define_local(Parameter(to_clr_type(kind)));
}

Local PythonCompiler::emit_define_local(bool cache) {
    if (cache) {
        return m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    }
    else {
        return m_il.define_local_no_cache(Parameter(CORINFO_TYPE_NATIVEINT));
    }
}

void PythonCompiler::emit_unpack_ex(Local sequence, size_t leftSize, size_t rightSize, Local sequenceStorage, Local list, Local remainder) {
    m_il.ld_loc(sequence);
    m_il.ld_i(leftSize);
    m_il.ld_i(rightSize);
    m_il.ld_loc(sequenceStorage);
    m_il.ld_loca(list);
    m_il.ld_loca(remainder);
    m_il.emit_call(METHOD_UNPACK_SEQUENCEEX_TOKEN, 6);
}

void PythonCompiler::emit_call_args() {
	m_il.emit_call(METHOD_CALL_ARGS, 2);
}

void PythonCompiler::emit_call_kwargs() {
	m_il.emit_call(METHOD_CALL_KWARGS, 3);
}

bool PythonCompiler::emit_call(size_t argCnt) {
    switch (argCnt) {
        case 0: m_il.emit_call(METHOD_CALL_0_TOKEN, 1); return true;
        case 1: m_il.emit_call(METHOD_CALL_1_TOKEN, 2); return true;
        case 2: m_il.emit_call(METHOD_CALL_2_TOKEN, 3); return true;
        case 3: m_il.emit_call(METHOD_CALL_3_TOKEN, 4); return true;
        case 4: m_il.emit_call(METHOD_CALL_4_TOKEN, 5); return true;
    }
    return false;
}

bool PythonCompiler::emit_method_call(size_t argCnt) {
    switch (argCnt) {
        case 0: m_il.emit_call(METHOD_METHCALL_0_TOKEN, 2); return true;
        case 1: m_il.emit_call(METHOD_METHCALL_1_TOKEN, 3); return true;
        case 2: m_il.emit_call(METHOD_METHCALL_2_TOKEN, 4); return true;
        case 3: m_il.emit_call(METHOD_METHCALL_3_TOKEN, 5); return true;
        case 4: m_il.emit_call(METHOD_METHCALL_4_TOKEN, 6); return true;
    }
    return false;
}

void PythonCompiler::emit_method_call_n(){
    m_il.emit_call(METHOD_METHCALLN_TOKEN, 3);
}

void PythonCompiler::emit_call_with_tuple() {
    m_il.emit_call(METHOD_CALLN_TOKEN, 2);
}

void PythonCompiler::emit_kwcall_with_tuple() {
	m_il.emit_call(METHOD_KWCALLN_TOKEN, 3);
}

void PythonCompiler::emit_store_local(Local local) {
    m_il.st_loc(local);
}

Local PythonCompiler::emit_spill() {
    auto tmp = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
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

void PythonCompiler::emit_restore_err() {
    m_il.emit_call(METHOD_PYERR_RESTORE, 3);
}

void PythonCompiler::emit_compare_exceptions() {
    m_il.emit_call(METHOD_COMPARE_EXCEPTIONS, 2);
}

void PythonCompiler::emit_pyerr_setstring(void* exception, const char*msg) {
    emit_ptr(exception);
    emit_ptr((void*)msg);
    m_il.emit_call(METHOD_PYERR_SETSTRING, 2);
}

void PythonCompiler::emit_unwind_eh(Local prevExc, Local prevExcVal, Local prevTraceback) {
    m_il.ld_loc(prevExc);
    m_il.ld_loc(prevExcVal);
    m_il.ld_loc(prevTraceback);
    m_il.emit_call(METHOD_UNWIND_EH, 3);
}

void PythonCompiler::emit_prepare_exception(Local prevExc, Local prevExcVal, Local prevTraceback) {
    auto excType = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    auto ehVal = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    auto tb = m_il.define_local(Parameter(CORINFO_TYPE_NATIVEINT));
    m_il.ld_loca(excType);
    m_il.ld_loca(ehVal);
    m_il.ld_loca(tb);

    m_il.ld_loca(prevExc);
    m_il.ld_loca(prevExcVal);
    m_il.ld_loca(prevTraceback);

    m_il.emit_call(METHOD_PREPARE_EXCEPTION, 6);
    m_il.ld_loc(tb);
    m_il.ld_loc(ehVal);
    m_il.ld_loc(excType);

    m_il.free_local(excType);
    m_il.free_local(ehVal);
    m_il.free_local(tb);
}

void PythonCompiler::emit_int(int value) {
    m_il.ld_i4(value);
}

void PythonCompiler::emit_reraise() {
    m_il.emit_call(METHOD_UNWIND_EH, 3);
}

void PythonCompiler::emit_float(double value) {
    m_il.ld_r8(value);
}

void PythonCompiler::emit_ptr(void* value) {
    m_il.ld_i(value);
}

void PythonCompiler::emit_bool(bool value) {
    m_il.ld_i4(value);
}

// Emits a call to create a new function, consuming the code object and
// the qualified name.
void PythonCompiler::emit_new_function() {
    load_frame();
    m_il.emit_call(METHOD_NEWFUNCTION_TOKEN, 3);
}

void PythonCompiler::emit_setup_annotations() {
    load_frame();
    m_il.emit_call(METHOD_SETUP_ANNOTATIONS, 1);
}

void PythonCompiler::emit_set_closure() {
	auto func = emit_spill();
	m_il.ld_i(offsetof(PyFunctionObject, func_closure));
	m_il.add();
	emit_load_and_free_local(func);
	m_il.st_ind_i();
}

void PythonCompiler::emit_set_annotations() {
	auto tmp = emit_spill();
	m_il.ld_i(offsetof(PyFunctionObject, func_annotations));
	m_il.add();
	emit_load_and_free_local(tmp);
    m_il.st_ind_i();
}

void PythonCompiler::emit_set_kw_defaults() {
	auto tmp = emit_spill();
	m_il.ld_i(offsetof(PyFunctionObject, func_kwdefaults));
	m_il.add();
	emit_load_and_free_local(tmp);
	m_il.st_ind_i();
}

void PythonCompiler::emit_set_defaults() {
	auto tmp = emit_spill();
	m_il.ld_i(offsetof(PyFunctionObject, func_defaults));
	m_il.add();
	emit_load_and_free_local(tmp);
	m_il.st_ind_i(); 
}

void PythonCompiler::emit_load_deref(int index) {
    load_frame();
    m_il.ld_i4(index);
    m_il.emit_call(METHOD_PYCELL_GET, 2);
}

void PythonCompiler::emit_store_deref(int index) {
    load_frame();
    m_il.ld_i4(index);
    m_il.emit_call(METHOD_PYCELL_SET_TOKEN, 3);
}

void PythonCompiler::emit_delete_deref(int index) {
    m_il.load_null();
    load_frame();
    m_il.ld_i4(index);
    m_il.emit_call(METHOD_PYCELL_SET_TOKEN, 3);
}

void PythonCompiler::emit_load_closure(int index) {
    load_frame();
    m_il.ld_i(offsetof(PyFrameObject, f_localsplus) + (m_code->co_nlocals + index) * sizeof(size_t));
    m_il.add();
    m_il.ld_ind_i();
    m_il.dup();
    emit_incref();
}

void PythonCompiler::emit_set_add() {
    // due to FOR_ITER magic we store the
    // iterable off the stack, and oparg here is based upon the stacking
    // of the generator indexes, so we don't need to spill anything...
    m_il.emit_call(METHOD_SET_ADD_TOKEN, 2);
}

void PythonCompiler::emit_set_update() {
    m_il.emit_call(METHOD_SETUPDATE_TOKEN, 2);
}

void PythonCompiler::emit_dict_merge() {
    m_il.emit_call(METHOD_DICT_MERGE, 2);
}

void PythonCompiler::emit_map_add() {
    m_il.emit_call(METHOD_MAP_ADD_TOKEN, 3);
}

void PythonCompiler::emit_list_append() {
    m_il.emit_call(METHOD_LIST_APPEND_TOKEN, 2);
}

void PythonCompiler::emit_null() {
    m_il.load_null();
}

void PythonCompiler::emit_raise_varargs() {
    // raise exc
    m_il.emit_call(METHOD_DO_RAISE, 2);
}

void PythonCompiler::emit_print_expr() {
    m_il.emit_call(METHOD_PRINT_EXPR_TOKEN, 1);
}

void PythonCompiler::emit_dict_update() {
    m_il.emit_call(METHOD_DICTUPDATE_TOKEN, 2);
}

void PythonCompiler::emit_load_classderef(int index) {
    load_frame();
    m_il.ld_i(index);
    m_il.emit_call(METHOD_LOAD_CLASSDEREF_TOKEN, 2);
}

void PythonCompiler::emit_getiter() {
    m_il.emit_call(METHOD_GETITER_TOKEN, 1);
}

Label PythonCompiler::emit_define_label() {
    return m_il.define_label();
}

void PythonCompiler::emit_inc_local(Local local, int value) {
    emit_int(value);
    emit_load_local(local);
    m_il.add();
    emit_store_local(local);
}

void PythonCompiler::emit_dec_local(Local local, int value) {
    emit_load_local(local);
    emit_int(value);
    m_il.sub();
    emit_store_local(local);
}

void PythonCompiler::emit_ret(int size) {
    m_il.ret(size);
}

void PythonCompiler::emit_mark_label(Label label) {
    m_il.mark_label(label);
}

void PythonCompiler::emit_for_next() {
    m_il.emit_call(METHOD_ITERNEXT_TOKEN, 1);
}

void PythonCompiler::emit_debug_msg(const char* msg) {
    m_il.ld_i((void*)msg);
    m_il.emit_call(METHOD_DEBUG_TRACE, 1);
}

void PythonCompiler::emit_binary_float(int opcode) {
    switch (opcode) {
        case BINARY_ADD:
        case INPLACE_ADD: m_il.add(); break;
        case INPLACE_TRUE_DIVIDE:
        case BINARY_TRUE_DIVIDE: m_il.div(); break;
        case INPLACE_MODULO:
        case BINARY_MODULO:
            // TODO: We should be able to generate a mod and provide the JIT
            // with a helper call method (CORINFO_HELP_DBLREM), but that's 
            // currently crashing for some reason...
            m_il.emit_call(METHOD_FLOAT_MODULUS_TOKEN, 2);
            //m_il.mod(); 
            break;
        case INPLACE_MULTIPLY:
        case BINARY_MULTIPLY:  m_il.mul(); break;
        case INPLACE_SUBTRACT:
        case BINARY_SUBTRACT:  m_il.sub(); break;

        case BINARY_POWER:
        case INPLACE_POWER: m_il.emit_call(METHOD_FLOAT_POWER_TOKEN, 2); break;
        case BINARY_FLOOR_DIVIDE:
        case INPLACE_FLOOR_DIVIDE: m_il.div(); m_il.emit_call(METHOD_FLOAT_FLOOR_TOKEN, 1); break;

    }
}

void PythonCompiler::emit_binary_object(int opcode) {
    switch (opcode) {
        case BINARY_SUBSCR: m_il.emit_call(METHOD_SUBSCR_TOKEN, 2); break;
        case BINARY_ADD: m_il.emit_call(METHOD_ADD_TOKEN, 2); break;
        case BINARY_TRUE_DIVIDE: m_il.emit_call(METHOD_DIVIDE_TOKEN, 2); break;
        case BINARY_FLOOR_DIVIDE: m_il.emit_call(METHOD_FLOORDIVIDE_TOKEN, 2); break;
        case BINARY_POWER: m_il.emit_call(METHOD_POWER_TOKEN, 2); break;
        case BINARY_MODULO: m_il.emit_call(METHOD_MODULO_TOKEN, 2); break;
        case BINARY_MATRIX_MULTIPLY: m_il.emit_call(METHOD_MATRIX_MULTIPLY_TOKEN, 2); break;
        case BINARY_LSHIFT: m_il.emit_call(METHOD_BINARY_LSHIFT_TOKEN, 2); break;
        case BINARY_RSHIFT: m_il.emit_call(METHOD_BINARY_RSHIFT_TOKEN, 2); break;
        case BINARY_AND: m_il.emit_call(METHOD_BINARY_AND_TOKEN, 2); break;
        case BINARY_XOR: m_il.emit_call(METHOD_BINARY_XOR_TOKEN, 2); break;
        case BINARY_OR: m_il.emit_call(METHOD_BINARY_OR_TOKEN, 2); break;
        case BINARY_MULTIPLY: m_il.emit_call(METHOD_MULTIPLY_TOKEN, 2); break;
        case BINARY_SUBTRACT: m_il.emit_call(METHOD_SUBTRACT_TOKEN, 2); break;
        case INPLACE_POWER: m_il.emit_call(METHOD_INPLACE_POWER_TOKEN, 2); break;
        case INPLACE_MULTIPLY: m_il.emit_call(METHOD_INPLACE_MULTIPLY_TOKEN, 2); break;
        case INPLACE_MATRIX_MULTIPLY: m_il.emit_call(METHOD_INPLACE_MATRIX_MULTIPLY_TOKEN, 2); break;
        case INPLACE_TRUE_DIVIDE: m_il.emit_call(METHOD_INPLACE_TRUE_DIVIDE_TOKEN, 2); break;
        case INPLACE_FLOOR_DIVIDE: m_il.emit_call(METHOD_INPLACE_FLOOR_DIVIDE_TOKEN, 2); break;
        case INPLACE_MODULO: m_il.emit_call(METHOD_INPLACE_MODULO_TOKEN, 2); break;
        case INPLACE_ADD:
            // TODO: We should do the unicode_concatenate ref count optimization
            m_il.emit_call(METHOD_INPLACE_ADD_TOKEN, 2);
            break;
        case INPLACE_SUBTRACT: m_il.emit_call(METHOD_INPLACE_SUBTRACT_TOKEN, 2); break;
        case INPLACE_LSHIFT: m_il.emit_call(METHOD_INPLACE_LSHIFT_TOKEN, 2); break;
        case INPLACE_RSHIFT:m_il.emit_call(METHOD_INPLACE_RSHIFT_TOKEN, 2); break;
        case INPLACE_AND: m_il.emit_call(METHOD_INPLACE_AND_TOKEN, 2); break;
        case INPLACE_XOR:m_il.emit_call(METHOD_INPLACE_XOR_TOKEN, 2); break;
        case INPLACE_OR: m_il.emit_call(METHOD_INPLACE_OR_TOKEN, 2); break;
    }
}

void PythonCompiler::emit_is(bool isNot) {
    m_il.emit_call(isNot ? METHOD_ISNOT : METHOD_IS, 2);
}


void PythonCompiler::emit_in() {
    m_il.emit_call(METHOD_CONTAINS_TOKEN, 2);
}


void PythonCompiler::emit_not_in() {
    m_il.emit_call(METHOD_NOTCONTAINS_TOKEN, 2);
}

void PythonCompiler::emit_compare_float(int compareType) {
    // TODO: If we know we're followed by the pop jump we could combine
    // and do a single branch comparison.
    switch (compareType) {
        case Py_EQ:  m_il.compare_eq(); break;
        case Py_LT: m_il.compare_lt(); break;
        case Py_LE: m_il.compare_le_float(); break;
        case Py_NE: m_il.compare_ne(); break;
        case Py_GT: m_il.compare_gt(); break;
        case Py_GE: m_il.compare_ge_float(); break;
    }
}

void PythonCompiler::emit_compare_tagged_int(int compareType) {
    switch (compareType) {
        case Py_EQ:  m_il.emit_call(METHOD_EQUALS_INT_TOKEN, 2); break;
        case Py_LT: m_il.emit_call(METHOD_LESS_THAN_INT_TOKEN, 2); break;
        case Py_LE: m_il.emit_call(METHOD_LESS_THAN_EQUALS_INT_TOKEN, 2); break;
        case Py_NE: m_il.emit_call(METHOD_NOT_EQUALS_INT_TOKEN, 2); break;
        case Py_GT: m_il.emit_call(METHOD_GREATER_THAN_INT_TOKEN, 2); break;
        case Py_GE: m_il.emit_call(METHOD_GREATER_THAN_EQUALS_INT_TOKEN, 2); break;
    }
}

void PythonCompiler::emit_compare_object(int compareType) {
    m_il.ld_i4(compareType);
    m_il.emit_call(METHOD_RICHCMP_TOKEN, 3);
}

void PythonCompiler::emit_load_method(void* name) {
    m_il.ld_i(name);
    m_il.emit_call(METHOD_LOAD_METHOD, 2);
}

void PythonCompiler::emit_periodic_work() {
    m_il.emit_call(METHOD_PERIODIC_WORK, 0);
}

JittedCode* PythonCompiler::emit_compile() {
    auto* jitInfo = new CorJitInfo(m_code, m_module);
    auto addr = m_il.compile(jitInfo, g_jit, m_code->co_stacksize + 100).m_addr;
    if (addr == nullptr) {
#ifdef DEBUG
        printf("Compiling failed %s from %s line %d\r\n",
            PyUnicode_AsUTF8(m_code->co_name),
            PyUnicode_AsUTF8(m_code->co_filename),
            m_code->co_firstlineno
            );
#endif
        delete jitInfo;
        return nullptr;
    }
    return jitInfo;

}

void PythonCompiler::emit_tagged_int_to_float() {
    m_il.emit_call(METHOD_INT_TO_FLOAT, 2);
}



/************************************************************************
* End Compiler interface implementation
*/

class GlobalMethod {
    Method m_method;
public:
    GlobalMethod(int token, Method method) : m_method(method) {
        m_method = method;
        g_module.m_methods[token] = &m_method;
    }
};

#define GLOBAL_METHOD(token, addr, returnType, ...) \
    GlobalMethod g ## token(token, Method(&g_module, returnType, std::vector<Parameter>{__VA_ARGS__}, (void*)addr));

GLOBAL_METHOD(METHOD_ADD_TOKEN, &PyJit_Add, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_SUBSCR_TOKEN, &PyJit_Subscr, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_MULTIPLY_TOKEN, &PyJit_Multiply, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_DIVIDE_TOKEN, &PyJit_TrueDivide, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_FLOORDIVIDE_TOKEN, &PyJit_FloorDivide, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_POWER_TOKEN, &PyJit_Power, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_SUBTRACT_TOKEN, &PyJit_Subtract, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_MODULO_TOKEN, &PyJit_Modulo, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_MATRIX_MULTIPLY_TOKEN, &PyJit_MatrixMultiply, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_BINARY_LSHIFT_TOKEN, &PyJit_BinaryLShift, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_BINARY_RSHIFT_TOKEN, &PyJit_BinaryRShift, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_BINARY_AND_TOKEN, &PyJit_BinaryAnd, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_BINARY_XOR_TOKEN, &PyJit_BinaryXor, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_BINARY_OR_TOKEN, &PyJit_BinaryOr, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PYLIST_NEW, &PyJit_NewList, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_EXTENDLIST_TOKEN, &PyJit_ExtendList, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_LISTTOTUPLE_TOKEN, &PyJit_ListToTuple, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_STOREMAP_TOKEN, &PyJit_StoreMap, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_STOREMAP_NO_DECREF_TOKEN, &PyJit_StoreMapNoDecRef, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_DICTUPDATE_TOKEN, &PyJit_DictUpdate, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_STORESUBSCR_TOKEN, &PyJit_StoreSubscr, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_DELETESUBSCR_TOKEN, &PyJit_DeleteSubscr, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_BUILD_DICT_FROM_TUPLES, &PyJit_BuildDictFromTuples, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_DICT_MERGE, &PyJit_DictMerge, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PYDICT_NEWPRESIZED, &_PyDict_NewPresized, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_PYTUPLE_NEW, &PyJit_PyTuple_New, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_PYSET_NEW, &PySet_New, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PYOBJECT_STR, &PyObject_Str, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_PYOBJECT_REPR, &PyObject_Repr, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_PYOBJECT_ASCII, &PyObject_ASCII, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PYOBJECT_ISTRUE, &PyObject_IsTrue, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_PYITER_NEXT, &PyIter_Next, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PYCELL_GET, &PyJit_CellGet, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_INT));
GLOBAL_METHOD(METHOD_PYCELL_SET_TOKEN, &PyJit_CellSet, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_INT));

GLOBAL_METHOD(METHOD_RICHCMP_TOKEN, &PyJit_RichCompare, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_INT));

GLOBAL_METHOD(METHOD_CONTAINS_TOKEN, &PyJit_Contains, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_NOTCONTAINS_TOKEN, &PyJit_NotContains, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_NEWFUNCTION_TOKEN, &PyJit_NewFunction, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_GETBUILDCLASS_TOKEN, &PyJit_BuildClass, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_UNPACK_SEQUENCE_TOKEN, &PyJit_UnpackSequence, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_UNPACK_SEQUENCEEX_TOKEN, &PyJit_UnpackSequenceEx, CORINFO_TYPE_NATIVEINT,
    Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PYSET_ADD, &PySet_Add, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_CALL_0_TOKEN, &Call0, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_CALL_1_TOKEN, &Call1, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_CALL_2_TOKEN, &Call2, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_CALL_3_TOKEN, &Call3, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_CALL_4_TOKEN, &Call4, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_CALLN_TOKEN, &PyJit_CallN, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_KWCALLN_TOKEN, &PyJit_KwCallN, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_STOREGLOBAL_TOKEN, &PyJit_StoreGlobal, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_DELETEGLOBAL_TOKEN, &PyJit_DeleteGlobal, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_LOADGLOBAL_TOKEN, &PyJit_LoadGlobal, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_LOADATTR_TOKEN, &PyJit_LoadAttr, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_STOREATTR_TOKEN, &PyJit_StoreAttr, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_DELETEATTR_TOKEN, &PyJit_DeleteAttr, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_LOADNAME_TOKEN, &PyJit_LoadName, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_STORENAME_TOKEN, &PyJit_StoreName, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_DELETENAME_TOKEN, &PyJit_DeleteName, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_GETITER_TOKEN, &PyJit_GetIter, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_ITERNEXT_TOKEN, &PyJit_IterNext, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_DECREF_TOKEN, &PyJit_DecRef, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_SET_CLOSURE, &PyJit_SetClosure, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_BUILD_SLICE, &PyJit_BuildSlice, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_UNARY_POSITIVE, &PyJit_UnaryPositive, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_UNARY_NEGATIVE, &PyJit_UnaryNegative, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_UNARY_NOT, &PyJit_UnaryNot, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_UNARY_NOT_INT, &PyJit_UnaryNot_Int, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_UNARY_INVERT, &PyJit_UnaryInvert, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_LIST_APPEND_TOKEN, &PyJit_ListAppend, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_SET_ADD_TOKEN, &PyJit_SetAdd, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_SETUPDATE_TOKEN, &PyJit_UpdateSet, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_MAP_ADD_TOKEN, &PyJit_MapAdd, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_INPLACE_POWER_TOKEN, &PyJit_InplacePower, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_INPLACE_MULTIPLY_TOKEN, &PyJit_InplaceMultiply, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_MATRIX_MULTIPLY_TOKEN, &PyJit_InplaceMatrixMultiply, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_TRUE_DIVIDE_TOKEN, &PyJit_InplaceTrueDivide, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_FLOOR_DIVIDE_TOKEN, &PyJit_InplaceFloorDivide, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_MODULO_TOKEN, &PyJit_InplaceModulo, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_ADD_TOKEN, &PyJit_InplaceAdd, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_SUBTRACT_TOKEN, &PyJit_InplaceSubtract, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_LSHIFT_TOKEN, &PyJit_InplaceLShift, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_RSHIFT_TOKEN, &PyJit_InplaceRShift, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_AND_TOKEN, &PyJit_InplaceAnd, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_XOR_TOKEN, &PyJit_InplaceXor, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_INPLACE_OR_TOKEN, &PyJit_InplaceOr, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PRINT_EXPR_TOKEN, &PyJit_PrintExpr, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_LOAD_CLASSDEREF_TOKEN, &PyJit_LoadClassDeref, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PREPARE_EXCEPTION, &PyJit_PrepareException, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT),
    Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_DO_RAISE, &PyJit_Raise, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_EH_TRACE, &PyJit_EhTrace, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_COMPARE_EXCEPTIONS, &PyJit_CompareExceptions, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_UNBOUND_LOCAL, &PyJit_UnboundLocal, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_PYERR_RESTORE, &PyJit_PyErrRestore, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_DEBUG_TRACE, &PyJit_DebugTrace, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PY_POPFRAME, &PyJit_PopFrame, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_PY_PUSHFRAME, &PyJit_PushFrame, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_UNWIND_EH, &PyJit_UnwindEh, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_PY_IMPORTNAME, &PyJit_ImportName, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_CALL_ARGS, &PyJit_CallArgs, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_CALL_KWARGS, &PyJit_CallKwArgs, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PY_IMPORTFROM, &PyJit_ImportFrom, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_PY_IMPORTSTAR, &PyJit_ImportStar, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_IS, &PyJit_Is, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_ISNOT, &PyJit_IsNot, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_IS_BOOL, &PyJit_Is_Bool, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_ISNOT_BOOL, &PyJit_IsNot_Bool, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_FLOAT_POWER_TOKEN, static_cast<double(*)(double, double)>(pow), CORINFO_TYPE_DOUBLE, Parameter(CORINFO_TYPE_DOUBLE), Parameter(CORINFO_TYPE_DOUBLE));
GLOBAL_METHOD(METHOD_FLOAT_FLOOR_TOKEN, static_cast<double(*)(double)>(floor), CORINFO_TYPE_DOUBLE, Parameter(CORINFO_TYPE_DOUBLE));
GLOBAL_METHOD(METHOD_FLOAT_MODULUS_TOKEN, static_cast<double(*)(double, double)>(fmod), CORINFO_TYPE_DOUBLE, Parameter(CORINFO_TYPE_DOUBLE), Parameter(CORINFO_TYPE_DOUBLE));
GLOBAL_METHOD(METHOD_FLOAT_FROM_DOUBLE, PyFloat_FromDouble, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_DOUBLE));
GLOBAL_METHOD(METHOD_BOOL_FROM_LONG, PyBool_FromLong, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_INT));

GLOBAL_METHOD(METHOD_PYERR_SETSTRING, PyErr_SetString, CORINFO_TYPE_VOID, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_PERIODIC_WORK, PyJit_PeriodicWork, CORINFO_TYPE_INT)

GLOBAL_METHOD(METHOD_PYUNICODE_JOINARRAY, &PyJit_UnicodeJoinArray, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_FORMAT_VALUE, &PyJit_FormatValue, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_FORMAT_OBJECT, &PyJit_FormatObject, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_LOAD_METHOD, &PyJit_LoadMethod, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_METHCALL_0_TOKEN, &MethCall0, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_METHCALL_1_TOKEN, &MethCall1, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_METHCALL_2_TOKEN, &MethCall2, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_METHCALL_3_TOKEN, &MethCall3, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_METHCALL_4_TOKEN, &MethCall4, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));
GLOBAL_METHOD(METHOD_METHCALLN_TOKEN, &MethCallN, CORINFO_TYPE_NATIVEINT, Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT), Parameter(CORINFO_TYPE_NATIVEINT));

GLOBAL_METHOD(METHOD_SETUP_ANNOTATIONS, &PyJit_SetupAnnotations, CORINFO_TYPE_INT, Parameter(CORINFO_TYPE_NATIVEINT),);

GLOBAL_METHOD(METHOD_LOAD_ASSERTION_ERROR, &PyJit_LoadAssertionError, CORINFO_TYPE_NATIVEINT);
