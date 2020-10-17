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

#ifndef ILGEN_H
#define ILGEN_H

#define FEATURE_NO_HOST

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

#include <vector>
#include <unordered_map>

#include <corjit.h>
#include <openum.h>

#include "codemodel.h"
#include "ipycomp.h"
#include "disasm.h"

using namespace std;

#define INC_CEE_STACK(c) \
    m_stackSize += c;
#define DEC_CEE_STACK(c) \
    m_stackSize -= c; \

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
        if (existing != m_freedLocals.end() && existing->second.size() != 0) {
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
        for (int i = 0; i < localList->size(); i++) {
            if ((*localList)[i].m_index == local.m_index) {
                // locals shouldn't be double freed...
                assert(FALSE);
            }
        }
#endif
        localList->push_back(local);
    }

    Label define_label() {
        m_labels.push_back(LabelInfo());
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
        unsigned char* value = (unsigned char*)(&i);
        for (int i = 0; i < 8; i++) {
            push_back(value[i]);
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
        size_t value = (size_t)ptr;
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
        _ASSERTE(index != -1);
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
        push_back(CEE_SUB); DEC_CEE_STACK(2); INC_CEE_STACK(1); // Pop1+Pop1, Push1
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
        CORINFO_METHOD_INFO methodInfo;
        methodInfo.ftn = (CORINFO_METHOD_HANDLE)addr;
        methodInfo.scope = (CORINFO_MODULE_HANDLE)m_module;
        methodInfo.ILCode = &m_il[0];
        methodInfo.ILCodeSize = (unsigned int)m_il.size();
        methodInfo.maxStack = stackSize;
        methodInfo.EHcount = 0;
        methodInfo.options = CORINFO_OPT_INIT_LOCALS;
        methodInfo.regionKind = CORINFO_REGION_JIT;
        methodInfo.args = CORINFO_SIG_INFO{ CORINFO_CALLCONV_DEFAULT };
        methodInfo.args.args = (CORINFO_ARG_LIST_HANDLE)(m_params.size() == 0 ? nullptr : &m_params[0]);
        methodInfo.args.numArgs = m_params.size();
        methodInfo.args.retType = m_retType;
        methodInfo.args.retTypeClass = nullptr;
        methodInfo.locals = CORINFO_SIG_INFO{ CORINFO_CALLCONV_DEFAULT };
        methodInfo.locals.args = (CORINFO_ARG_LIST_HANDLE)(m_locals.size() == 0 ? nullptr : &m_locals[0]);
        methodInfo.locals.numArgs = m_locals.size();
        return methodInfo;
    }

    Method compile(ICorJitInfo* jitInfo, ICorJitCompiler* jit, int stackSize) {
        BYTE* nativeEntry;
        ULONG nativeSizeOfCode;
        auto res = Method(m_module, m_retType, m_params, nullptr);
        CORINFO_METHOD_INFO methodInfo = to_method(&res, stackSize);

        CorJitResult result = jit->compileMethod(
                jitInfo,
                &methodInfo,
                CORJIT_FLAGS::CORJIT_FLAG_CALL_GETJITFLAGS,
                &nativeEntry,
                &nativeSizeOfCode
        );

        switch (result){
            case CORJIT_OK:
                res.m_addr = nativeEntry;
                break;
            case CORJIT_BADCODE:
#ifdef DEBUG
                printf("JIT failed to compile the submitted method.\n");
                dump();
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

    void dump () {
        printf("Stack Size %d\n", m_stackSize);
        printf("Labels         \n");
        printf("---------------\n");
        for (auto & m_label : m_labels) {
            printf("%d : \n", m_label.m_location);
        }

        printf("Locals         \n");
        printf("---------------\n");
        for (auto & m_local : m_locals) {
            printf("%d : \n", m_local.m_type);
        }

        printf("IL          \n");
        printf("---------------\n");
        for (int i =0 ; i < m_il.size(); i ++) {
           printf("%s\n", opcodename((OPCODE)m_il[i]));
        }
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