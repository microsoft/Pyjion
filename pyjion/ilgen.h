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

#ifndef PYJION_ILGEN_H
#define PYJION_ILGEN_H

#define FEATURE_NO_HOST

#include <cstdint>
#include <windows.h>
#include <cwchar>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <cfloat>
#include <share.h>
#include <cstdlib>
#include <intrin.h>

#include <vector>
#include <unordered_map>

#include <corjit.h>
#include <openum.h>

#include "codemodel.h"
#include "ipycomp.h"
#include "disasm.h"

using namespace std;

#define INC_CEE_STACK(c) \
    m_stackSize += (c);
#define DEC_CEE_STACK(c) \
    m_stackSize -= (c); \

class LabelInfo {
public:
    int m_location;
    vector<int> m_branchOffsets;

    LabelInfo() {
        m_location = -1;
    }
};

struct CorInfoTypeHash {
    std::size_t operator()(CorInfoType e) const {
        return static_cast<std::size_t>(e);
    }
};

class ILGenerator {
    vector<Parameter> m_params, m_locals;
    CorInfoType m_retType;
    Module* m_module;
    unordered_map<CorInfoType, vector<Local>, CorInfoTypeHash> m_freedLocals;

public:
    vector<BYTE> m_il;
    int m_localCount;
    vector<LabelInfo> m_labels;
    int m_stackSize;

public:

    ILGenerator(Module* module, CorInfoType returnType, std::vector<Parameter> params) {
        m_module = module;
        m_retType = returnType;
        m_params = params;
        m_localCount = 0;
        m_stackSize =  2 + params.size() ; // starts with frame on stack
    }

    Local define_local(Parameter param) {
        auto existing = m_freedLocals.find(param.m_type);
        if (existing != m_freedLocals.end() && !existing->second.empty()) {
            auto res = existing->second[existing->second.size() - 1];
            existing->second.pop_back();
            return res;
        }
        return define_local_no_cache(param);
    }

    Local define_local_no_cache(Parameter param) {
        m_locals.push_back(param);
        return Local(m_localCount++);
    }

    void free_local(Local local) {
        auto param = m_locals[local.m_index];
        auto existing = m_freedLocals.find(param.m_type);
        vector<Local>* localList;
        if (existing == m_freedLocals.end()) {
            m_freedLocals[param.m_type] = vector<Local>();
            localList = &(m_freedLocals.find(param.m_type)->second);
        }
        else {
            localList = &(existing->second);
        }
#if DEBUG
        for (auto & i : *localList) {
            if (i.m_index == local.m_index) {
                // locals shouldn't be double freed...
                assert(FALSE);
            }
        }
#endif
        localList->push_back(local);
    }

    Label define_label() {
        m_labels.emplace_back();
        return Label((int)m_labels.size() - 1);
    }

    void mark_label(Label label) {
        auto info = &m_labels[label.m_index];
        info->m_location = (int)m_il.size();
        for (int i = 0; i < info->m_branchOffsets.size(); i++) {
            auto from = info->m_branchOffsets[i];
            auto offset = info->m_location - (from + 4);		// relative to the end of the instruction
            m_il[from] = offset & 0xFF;
            m_il[from + 1] = (offset >> 8) & 0xFF;
            m_il[from + 2] = (offset >> 16) & 0xFF;
            m_il[from + 3] = (offset >> 24) & 0xFF;
        }
    }

    void localloc() {
        push_back(CEE_PREFIX1); // NIL CEE SE
        push_back((BYTE)CEE_LOCALLOC); // PopI + PushI
    }

    void brk(){
        // emit a breakpoint in the IL
        push_back(CEE_BREAK);
    }

    void ret(int size) {
        push_back(CEE_RET); DEC_CEE_STACK(size); // VarPop (size)
    }

    void ld_r8(double i) {
        push_back(CEE_LDC_R8); // Pop0 + PushR8
        auto* value = (unsigned char*)(&i);
        for (int j = 0; j < 8; j++) {
            push_back(value[j]);
        }
    }

    void ld_i4(int i) {
        switch (i) {
            case -1:push_back(CEE_LDC_I4_M1); break;
            case 0: push_back(CEE_LDC_I4_0); break;
            case 1: push_back(CEE_LDC_I4_1); break;
            case 2: push_back(CEE_LDC_I4_2); break;
            case 3: push_back(CEE_LDC_I4_3); break;
            case 4: push_back(CEE_LDC_I4_4); break;
            case 5: push_back(CEE_LDC_I4_5); break;
            case 6: push_back(CEE_LDC_I4_6); break;
            case 7: push_back(CEE_LDC_I4_7); break;
            default:
                if (i < 256) {
                    push_back(CEE_LDC_I4_S);
                    m_il.push_back(i);

                }
                else {
                    m_il.push_back(CEE_LDC_I4);
                    m_il.push_back((BYTE)CEE_STLOC); DEC_CEE_STACK(1); // Pop1 + Push0
                    emit_int(i);
                }
        }
    }

    void new_array(size_t len){
        /*Stack Transition:

        …, numElems → …, array

        Description:

        The newarr instruction pushes a reference to a new zero-based, one-dimensional array whose elements are of type etype, a metadata token (a typeref, typedef or typespec; see Partition II). numElems (of type native int or int32) specifies the number of elements in the array. Valid array indexes are 0 ≤ index < numElems. The elements of an array can be any type, including value types.

                Zero-based, one-dimensional arrays of numbers are created using a metadata token referencing the appropriate value type (System.Int32, etc.). Elements of the array are initialized to 0 of the appropriate type.

                One-dimensional arrays that aren't zero-based and multidimensional arrays are created using newobj rather than newarr. More commonly, they are created using the methods of System.Array class in the Base Framework.

        Correctness:

        Correct CIL ensures that etype is a valid typeref, typedef or typespec token.

                Verifiability:

        numElems shall be of type native int or int32.
         */
        ld_i4(len);
        m_il.push_back(CEE_NEWARR);
        INC_CEE_STACK(1);
    }

    void st_elem(Local array, int index, Local value){
        /* Stack Transition:

        …, array, index, value, → …

        Description:

        The stelem instruction replaces the value of the element with zero-based index index (of type native int or int32) in the one-dimensional array array, with value.
         Arrays are objects and hence are represented by a value of type O. The type of value must be array-element-compatible-with typeTok in the instruction.

                Storing into arrays that hold values smaller than 4 bytes whose intermediate type is int32 truncates the value as it moves from the stack to the array.
                Floating-point values are rounded from their native size (type F) to the size associated with the array. (See §III.1.1.1, Numeric data types.)

        [Note: For one-dimensional arrays that aren't zero-based and for multidimensional arrays, the array class provides a StoreElement method. end note]

        Correctness:

        typeTok shall be a valid typedef, typeref, or typespec metadata token.

                array shall be null or a single dimensional array.

                Verifiability:

        Verification requires that:

        the tracked type of array is T[], for some T;

        the tracked type of value is array-element-compatible-with (§I.8.7.1) typeTok;

        typeTok is array-element-compatible-with T; and

        the type of index is int32 or native int.*/
        ld_loc(array);
        ld_i4(index);
        ld_loc(value);
        m_il.push_back(CEE_STELEM); DEC_CEE_STACK(1);
        //ld_i(0x11); // PYOBJECT_PTR type token.
    }

    void st_elem_i(int index){
        emit_int(index);
        m_il.push_back(CEE_STELEM); DEC_CEE_STACK(1);

    }

    void st_elem_i4(int index){
        emit_int(index);
        m_il.push_back(CEE_STELEM_I4); DEC_CEE_STACK(1);
    }

    void ld_elem(int index){
        emit_int(index);
        m_il.push_back(CEE_LDELEM); INC_CEE_STACK(1);
    }

    void ld_elem_i4(int index){
        emit_int(index);
        m_il.push_back(CEE_LDELEM_I4); INC_CEE_STACK(1);
    }

    void ld_elem_r8(int index){
        emit_int(index);
        m_il.push_back(CEE_LDELEM_R8); INC_CEE_STACK(1);
    }

    void load_null() {
        ld_i4(0);
        m_il.push_back(CEE_CONV_I); DEC_CEE_STACK(1); // Pop1 + PushI
    }

    void st_ind_i() {
        push_back(CEE_STIND_I); // PopI + PopI / Push0
    }

    void ld_ind_i() {
        push_back(CEE_LDIND_I); // PopI / PushI
    }

    void st_ind_i4() {
        push_back(CEE_STIND_I4); // PopI + PopI / Push0
    }

    void ld_ind_i4() {
        push_back(CEE_LDIND_I4); // PopI  / PushI
    }

    void ld_ind_r8() {
        push_back(CEE_LDIND_R8); // PopI + PopI / Push0
    }

    void branch(BranchType branchType, Label label) {
        auto info = &m_labels[label.m_index];
        if (info->m_location == -1) {
            info->m_branchOffsets.push_back((int)m_il.size() + 1);
            branch(branchType, 0xFFFF);
        }
        else {
            branch(branchType, (int)(info->m_location - m_il.size()));
        }
    }

    void branch(BranchType branchType, int offset) {
        if ((offset - 2) <= 128 && (offset - 2) >= -127) {
            switch (branchType) {
                case BranchLeave:
                    m_il.push_back(CEE_LEAVE_S); // Pop0 + Push0
                    break;
                case BranchAlways:
                    m_il.push_back(CEE_BR_S);  // Pop0 + Push0
                    break;
                case BranchTrue:
                    m_il.push_back(CEE_BRTRUE_S); // PopI + Push0
                    break;
                case BranchFalse:
                    m_il.push_back(CEE_BRFALSE_S);  // PopI, Push0
                    break;
                case BranchEqual:
                    m_il.push_back(CEE_BEQ_S); DEC_CEE_STACK(2); // Pop1+Pop1, Push0
                    break;
                case BranchNotEqual:
                    m_il.push_back(CEE_BNE_UN_S); DEC_CEE_STACK(2); // Pop1+Pop1, Push0
                    break;
                case BranchLessThanEqual:
                    m_il.push_back(CEE_BLE_S); DEC_CEE_STACK(2); // Pop1+Pop1, Push0
                    break;
            }
            m_il.push_back((BYTE)offset - 2);
        }
        else {
            switch (branchType) {
                case BranchLeave:
                    m_il.push_back(CEE_LEAVE);  // Pop0, Push0
                    break;
                case BranchAlways:
                    m_il.push_back(CEE_BR); // Pop0, Push0
                    break;
                case BranchTrue:
                    m_il.push_back(CEE_BRTRUE); // PopI, Push0
                    break;
                case BranchFalse:
                    m_il.push_back(CEE_BRFALSE); // PopI, Push0
                    break;
                case BranchEqual:
                    m_il.push_back(CEE_BEQ); DEC_CEE_STACK(2); //  Pop1+Pop1, Push0
                    break;
                case BranchNotEqual:
                    m_il.push_back(CEE_BNE_UN); DEC_CEE_STACK(2); //  Pop1+Pop1, Push0
                    break;
                case BranchLessThanEqual:
                    m_il.push_back(CEE_BLE); DEC_CEE_STACK(2); // Pop1+Pop1, Push0
                    break;
            }
            emit_int(offset - 5);
        }
    }

    void neg() {
        m_il.push_back(CEE_NEG); DEC_CEE_STACK(1); INC_CEE_STACK(1) //  Pop1, Push1
    }

    void dup() {
        m_il.push_back(CEE_DUP); DEC_CEE_STACK(1); INC_CEE_STACK(2) //  Pop1, Push1+Push1
    }

    void bitwise_and() {
        m_il.push_back(CEE_AND); DEC_CEE_STACK(2); INC_CEE_STACK(1) //  Pop1+Pop1, Push1
    }

    void pop() {
        m_il.push_back(CEE_POP); DEC_CEE_STACK(1); //  Pop1, Push0
    }

    void compare_eq() {
        m_il.push_back(CEE_PREFIX1); // NIL SE
        m_il.push_back((BYTE)CEE_CEQ); DEC_CEE_STACK(2); //  Pop1+Pop1, PushI
    }

    void compare_ne() {
        compare_eq();
        ld_i4(0);
        compare_eq();
    }

    void compare_gt() {
        m_il.push_back(CEE_PREFIX1);  // NIL SE
        m_il.push_back((BYTE)CEE_CGT); DEC_CEE_STACK(2); //  Pop1+Pop1, PushI
    }

    void compare_lt() {
        m_il.push_back(CEE_PREFIX1); // NIL
        m_il.push_back((BYTE)CEE_CLT); DEC_CEE_STACK(2); //  Pop1+Pop1, PushI
    }

    void compare_ge() {
        m_il.push_back(CEE_PREFIX1); // NIL
        m_il.push_back((BYTE)CEE_CLT); DEC_CEE_STACK(2); //  Pop1+Pop1, PushI
        ld_i4(0);
        compare_eq();
    }

    void compare_le() {
        m_il.push_back(CEE_PREFIX1); // NIL
        m_il.push_back((BYTE)CEE_CGT); DEC_CEE_STACK(2); //  Pop1+Pop1, PushI
        ld_i4(0);
        compare_eq();
    }

    void compare_ge_float() {
        m_il.push_back(CEE_PREFIX1); // NIL
        m_il.push_back((BYTE)CEE_CLT_UN); DEC_CEE_STACK(2); //  Pop1+Pop1, PushI
        ld_i4(0);
        compare_eq();
    }

    void compare_le_float() {
        m_il.push_back(CEE_PREFIX1); // NIL
        m_il.push_back((BYTE)CEE_CGT_UN); DEC_CEE_STACK(2); //  Pop1+Pop1, PushI
        ld_i4(0);
        compare_eq();
    }

    void ld_i(int i) {
        m_il.push_back(CEE_LDC_I4); // Pop0, PushI
        emit_int(i);
        m_il.push_back(CEE_CONV_I); DEC_CEE_STACK(1); // Pop1, PushI
    }

    void ld_i(size_t i) {
        ld_i((void*)i);
    }

    void ld_i(void* ptr) {
        auto value = (size_t)ptr;
#ifdef _TARGET_AMD64_
        if ((value & 0xFFFFFFFF) == value) {
            ld_i((int)value);
        }
        else {
            m_il.push_back(CEE_LDC_I8); // Pop0, PushI8
            m_il.push_back(value & 0xff);
            m_il.push_back((value >> 8) & 0xff);
            m_il.push_back((value >> 16) & 0xff);
            m_il.push_back((value >> 24) & 0xff);
            m_il.push_back((value >> 32) & 0xff);
            m_il.push_back((value >> 40) & 0xff);
            m_il.push_back((value >> 48) & 0xff);
            m_il.push_back((value >> 56) & 0xff);
            m_il.push_back(CEE_CONV_I); // DEC_CEE_STACK(1); // Pop1, PushI
        }
#else
        ld_i(value);
        m_il.push_back(CEE_CONV_I);
#endif
    }

    void emit_call(int token, int nargs) {
        m_il.push_back(CEE_CALL); DEC_CEE_STACK(nargs); INC_CEE_STACK(1); // VarPop, VarPush
        emit_int(token);
    }

    // Unused
    void emit_calli(int token) {
    	m_il.push_back(CEE_CALLI);
    	emit_int(token);
    }

    // Unused
    void emit_callvirt(int token) {
    	m_il.push_back(CEE_CALLVIRT);
    	emit_int(token);
    }

    void st_loc(Local param) {
        st_loc(param.m_index);
    }

    void ld_loc(Local param) {
        ld_loc(param.m_index);
    }

    void ld_loca(Local param) {
        assert(param.is_valid());
        ld_loca(param.m_index);
    }

    void st_loc(int index) {
        assert(index != -1);
        switch (index) {
            case 0: m_il.push_back(CEE_STLOC_0); DEC_CEE_STACK(1); break;
            case 1: m_il.push_back(CEE_STLOC_1); DEC_CEE_STACK(1); break;
            case 2: m_il.push_back(CEE_STLOC_2); DEC_CEE_STACK(1); break;
            case 3: m_il.push_back(CEE_STLOC_3); DEC_CEE_STACK(1); break;
            default:
                if (index < 256) {
                    m_il.push_back(CEE_STLOC_S); DEC_CEE_STACK(1);
                    m_il.push_back(index);
                }
                else {
                    m_il.push_back(CEE_PREFIX1); // NIL
                    m_il.push_back((BYTE)CEE_STLOC); DEC_CEE_STACK(1);
                    m_il.push_back(index & 0xff);
                    m_il.push_back((index >> 8) & 0xff);
                }
        }
    }

    void ld_loc(int index) {
        assert(index != -1);
        switch (index) {
            case 0: m_il.push_back(CEE_LDLOC_0); INC_CEE_STACK(1); break;
            case 1: m_il.push_back(CEE_LDLOC_1); INC_CEE_STACK(1); break;
            case 2: m_il.push_back(CEE_LDLOC_2); INC_CEE_STACK(1); break;
            case 3: m_il.push_back(CEE_LDLOC_3); INC_CEE_STACK(1); break;
            default:
                if (index < 256) {
                    m_il.push_back(CEE_LDLOC_S); INC_CEE_STACK(1);
                    m_il.push_back(index);
                }
                else {
                    m_il.push_back(CEE_PREFIX1); // NIL
                    m_il.push_back((BYTE)CEE_LDLOC); INC_CEE_STACK(1);
                    m_il.push_back(index & 0xff);
                    m_il.push_back((index >> 8) & 0xff);
                }
        }
    }

    void ld_loca(int index) {
        assert(index != -1);
        if (index < 256) {
            m_il.push_back(CEE_LDLOCA_S); // Pop0, PushI
            m_il.push_back(index);
        }
        else {
            m_il.push_back(CEE_PREFIX1); // NIL
            m_il.push_back((BYTE)CEE_LDLOCA); // Pop0, PushI
            m_il.push_back(index & 0xff);
            m_il.push_back((index >> 8) & 0xff);
        }
    }

    void add() {
        push_back(CEE_ADD); DEC_CEE_STACK(2); INC_CEE_STACK(1); // Pop1+Pop1, Push1
    }

    void sub() {
        push_back(CEE_SUB_OVF); DEC_CEE_STACK(2); INC_CEE_STACK(1); // Pop1+Pop1, Push1
    }

    void div() {
        push_back(CEE_DIV); DEC_CEE_STACK(2); INC_CEE_STACK(1); // Pop1+Pop1, Push1
    }

    void mod() {
        push_back(CEE_REM); DEC_CEE_STACK(2); INC_CEE_STACK(1); // Pop1+Pop1, Push1
    }

    void mul() {
        push_back(CEE_MUL); DEC_CEE_STACK(2); INC_CEE_STACK(1); // Pop1+Pop1, Push1
    }

    void ld_arg(int index) {
        assert(index != -1);
        switch (index) {
            case 0:
                push_back(CEE_LDARG_0); INC_CEE_STACK(1); // Pop0, Push1
                break;
            case 1:
                push_back(CEE_LDARG_1); INC_CEE_STACK(1); // Pop0, Push1
                break;
            case 2:
                push_back(CEE_LDARG_2); INC_CEE_STACK(1); // Pop0, Push1
                break;
            case 3:
                push_back(CEE_LDARG_3); INC_CEE_STACK(1); // Pop0, Push1
                break;
            default:
                if (index < 256) {
                    push_back(CEE_LDARG_3); INC_CEE_STACK(1); // Pop0, Push1
                    m_il.push_back(index);
                } else {
                    m_il.push_back(CEE_PREFIX1); // NIL
                    m_il.push_back((BYTE) CEE_LDARG); INC_CEE_STACK(1); // Pop0, Push1
                    m_il.push_back(index & 0xff);
                    m_il.push_back((index >> 8) & 0xff);
                }

                break;
        }
    }

    CORINFO_METHOD_INFO to_method(Method* addr, int stackSize) {
        CORINFO_METHOD_INFO methodInfo{};
        methodInfo.ftn = (CORINFO_METHOD_HANDLE)addr;
        methodInfo.scope = (CORINFO_MODULE_HANDLE)m_module;
        methodInfo.ILCode = &m_il[0];
        methodInfo.ILCodeSize = (unsigned int)m_il.size();
        methodInfo.maxStack = stackSize;
        methodInfo.EHcount = 0;
        methodInfo.options = CORINFO_OPT_INIT_LOCALS;
        methodInfo.regionKind = CORINFO_REGION_JIT;
        methodInfo.args = CORINFO_SIG_INFO{ CORINFO_CALLCONV_DEFAULT };
        methodInfo.args.args = (CORINFO_ARG_LIST_HANDLE)(m_params.empty() ? nullptr : &m_params[0]);
        methodInfo.args.numArgs = m_params.size();
        methodInfo.args.retType = m_retType;
        methodInfo.args.retTypeClass = nullptr;
        methodInfo.locals = CORINFO_SIG_INFO{ CORINFO_CALLCONV_DEFAULT };
        methodInfo.locals.args = (CORINFO_ARG_LIST_HANDLE)(m_locals.empty() ? nullptr : &m_locals[0]);
        methodInfo.locals.numArgs = m_locals.size();
        return methodInfo;
    }

    Method compile(CorJitInfo* jitInfo, ICorJitCompiler* jit, int stackSize) {
        BYTE* nativeEntry;
        ULONG nativeSizeOfCode;
        auto res = Method(m_module, m_retType, m_params, nullptr);

        CORINFO_METHOD_INFO methodInfo = to_method(&res, stackSize);
        jitInfo->assignIL(methodInfo.ILCode, methodInfo.ILCodeSize);
        CorJitResult result = jit->compileMethod(
                jitInfo,
                &methodInfo,
                CORJIT_FLAGS::CORJIT_FLAG_CALL_GETJITFLAGS,
                &nativeEntry,
                &nativeSizeOfCode
        );
        jitInfo->setNativeSize(nativeSizeOfCode);
        switch (result){
            case CORJIT_OK:
                res.m_addr = nativeEntry;
                break;
            case CORJIT_BADCODE:
#ifdef DEBUG
                printf("JIT failed to compile the submitted method.\n");
#endif
                break;
            case CORJIT_OUTOFMEM:
                printf("out of memory.\n");
                break;
            case CORJIT_INTERNALERROR:
#ifdef DEBUG
                printf("internal error code.\n");
#endif
                break;
            case CORJIT_SKIPPED:
#ifdef DEBUG
                printf("skipped code.\n");
#endif
                break;
            case CORJIT_RECOVERABLEERROR:
#ifdef DEBUG
                printf("recoverable error code.\n");
#endif
                break;
        }
        return res;
    }

private:
    void emit_int(int value) {
        m_il.push_back(value & 0xff);
        m_il.push_back((value >> 8) & 0xff);
        m_il.push_back((value >> 16) & 0xff);
        m_il.push_back((value >> 24) & 0xff);
    }

    void push_back(BYTE b) {
        m_il.push_back(b);
    }
};

#endif