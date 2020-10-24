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

#ifndef PYJION_CODEMODEL_H
#define PYJION_CODEMODEL_H

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

using namespace std;

class Method;
class BaseMethod;
class CorClass;

class Module {
public:
    unordered_map<int, BaseMethod*> m_methods;
    Module() = default;

    virtual BaseMethod* ResolveMethod(int tokenId) {
        return m_methods[tokenId];
    }
};

class UserModule : public Module {
    Module& m_parent;
public:
    explicit UserModule(Module& parent) : m_parent(parent) {

    }

    BaseMethod* ResolveMethod(int tokenId) override {
        auto res = m_methods.find(tokenId);
        if (res == m_methods.end()) {
            return m_parent.ResolveMethod(tokenId);
        }

        return res->second;
    }
};

class Parameter {
public:
    CorInfoType m_type;
    explicit Parameter(CorInfoType type) {
        m_type = type;
    }
};


struct VTableInfo {
    WORD indirections;
    SIZE_T                  offsets[CORINFO_MAXINDIRECTIONS];
};

class BaseMethod {
public:

    virtual void get_call_info(CORINFO_CALL_INFO *pResult) = 0;
    virtual DWORD get_method_attrs() {
        return CORINFO_FLG_STATIC | CORINFO_FLG_NATIVE ;
    }
    virtual void findSig(CORINFO_SIG_INFO  *sig) = 0;
    virtual void* get_addr() = 0;
    virtual void getFunctionEntryPoint(CORINFO_CONST_LOOKUP *  pResult) = 0;
    virtual CORINFO_CLASS_HANDLE getClass() = 0;
};

class Method : public BaseMethod {
    Module* m_module;
public:
    vector<Parameter> m_params;
    CorInfoType m_retType;
    void* m_addr;

    Method(Module* module, CorInfoType returnType, std::vector<Parameter> params, void* addr) {
        m_retType = returnType;
        m_params = params;
        m_module = module;
        m_addr = addr;
    }

    void* get_addr() override {
        return m_addr;
    }

    void get_call_info(CORINFO_CALL_INFO *pResult) override {
        pResult->codePointerLookup.lookupKind.needsRuntimeLookup = false;
        // TODO: If we use IAT_VALUE we need to generate a jump stub
        pResult->codePointerLookup.constLookup.accessType = IAT_PVALUE;
        pResult->codePointerLookup.constLookup.addr = &m_addr;
        pResult->verMethodFlags = pResult->methodFlags = CORINFO_FLG_STATIC;
        pResult->kind = CORINFO_CALL;
        pResult->sig.args = (CORINFO_ARG_LIST_HANDLE)(m_params.size() == 0 ? nullptr : &m_params[0]);
        pResult->sig.retType = m_retType;
        pResult->sig.numArgs = m_params.size();
    }
    void findSig(CORINFO_SIG_INFO  *sig) override {
        sig->retType = m_retType;
        sig->callConv = CORINFO_CALLCONV_STDCALL;
        sig->retTypeClass = nullptr;
        sig->args = (CORINFO_ARG_LIST_HANDLE)(m_params.size() != 0 ? &m_params[0] : nullptr);
        sig->numArgs = m_params.size();
    }
    void getFunctionEntryPoint(CORINFO_CONST_LOOKUP *  pResult) override {
        // TODO: If we use IAT_VALUE we need to generate a jump stub
        pResult->accessType = IAT_PVALUE;
        pResult->addr = &m_addr;
    }

    CORINFO_CLASS_HANDLE getClass() override {
        // TODO: Implement
        return nullptr;
    }
};

class IndirectDispatchMethod : public BaseMethod {
    BaseMethod* m_coreMethod;
public:
    void* m_addr;

    IndirectDispatchMethod(BaseMethod* coreMethod) : m_coreMethod(coreMethod) {
        m_addr = m_coreMethod->get_addr();
    }

public:
    void* get_addr() override {
        return m_addr;
    }

    void get_call_info(CORINFO_CALL_INFO *pResult) override {
        m_coreMethod->get_call_info(pResult);
        pResult->codePointerLookup.constLookup.addr = &m_addr;
    }

    void findSig(CORINFO_SIG_INFO  *sig) override {
        m_coreMethod->findSig(sig);
    }
    void getFunctionEntryPoint(CORINFO_CONST_LOOKUP *  pResult) override {
        pResult->accessType = IAT_PVALUE;
        pResult->addr = &m_addr;
    }
    CORINFO_CLASS_HANDLE getClass() override {
        CORINFO_CLASS_HANDLE result = nullptr;
        // TODO : Infer type from method, currently it uses NATIVEINT for everything so there is no class
//        MethodDesc* method = GetMethod(methodHnd);
//        TypeHandle th = TypeHandle(method->GetMethodTable());
//        result = CORINFO_CLASS_HANDLE(th.AsPtr());
        return result;
    }
};

// Not currently used...
class VirtualMethod : public Method {
    VTableInfo* m_vtableInfo;
    VirtualMethod(Module* module, CorInfoType returnType, std::vector<Parameter> params, void* addr, VTableInfo* vtableInfo) :
        Method(module, returnType, params, addr),
        m_vtableInfo(vtableInfo) {
    }

    void get_call_info(CORINFO_CALL_INFO *pResult) override {
        pResult->codePointerLookup.lookupKind.needsRuntimeLookup = true;
        pResult->codePointerLookup.lookupKind.runtimeLookupKind = CORINFO_LOOKUP_THISOBJ;
        pResult->codePointerLookup.runtimeLookup.testForNull = false;
        pResult->codePointerLookup.runtimeLookup.testForFixup = false;
        //pResult->codePointerLookup.runtimeLookup.helper = CORINFO_HELP_USER_BREAKPOINT;
        pResult->verMethodFlags = pResult->methodFlags = CORINFO_FLG_VIRTUAL;
        pResult->kind = CORINFO_VIRTUALCALL_VTABLE;
        pResult->methodFlags = CORINFO_FLG_VIRTUAL;
    }

    DWORD get_method_attrs() override {
        return CORINFO_FLG_VIRTUAL | CORINFO_FLG_NATIVE;
    }
};

#endif