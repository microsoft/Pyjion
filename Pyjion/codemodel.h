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

#ifndef __CODEMODEL_H__
#define __CODEMODEL_H__

#define FEATURE_NO_HOST
#define USE_STL
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
#include <hash_map>

#include <corjit.h>

using namespace std;

class Method;

class Module {
public:
    hash_map<int, Method> m_methods;
    Module() {
    }

    Method* ResolveMethod(int tokenId) {
        return &m_methods[tokenId];
    }
};

class Parameter {
public:
    CorInfoType m_type;
    Parameter(CorInfoType type) {
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
        return CORINFO_FLG_NOSECURITYWRAP | CORINFO_FLG_STATIC | CORINFO_FLG_NATIVE;
    }
};

class Method : public BaseMethod {
    Module* m_module;
public:
    vector<Parameter> m_params;
    CorInfoType m_retType;
    void* m_addr;

    Method() {
    }

    Method(Module* module, CorInfoType returnType, std::vector<Parameter> params, void* addr) {
        m_retType = returnType;
        m_params = params;
        m_module = module;
        m_addr = addr;
    }

    virtual void get_call_info(CORINFO_CALL_INFO *pResult) {
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

};

class IndirectDispatchMethod : BaseMethod {
    Method& m_coreMethod;
public:
    void* m_addr;

    IndirectDispatchMethod(Method& coreMethod) : m_coreMethod(coreMethod) {
        m_addr = m_coreMethod.m_addr;
    }

public:
    virtual void get_call_info(CORINFO_CALL_INFO *pResult) {
        pResult->codePointerLookup.lookupKind.needsRuntimeLookup = false;
        // TODO: If we use IAT_VALUE we need to generate a jump stub
        pResult->codePointerLookup.constLookup.accessType = IAT_PVALUE;
        pResult->codePointerLookup.constLookup.addr = &m_addr;
        pResult->verMethodFlags = pResult->methodFlags = 0;
        pResult->kind = CORINFO_VIRTUALCALL_STUB;
        pResult->sig.args = (CORINFO_ARG_LIST_HANDLE)(m_coreMethod.m_params.size() == 0 ? nullptr : &m_coreMethod.m_params[0]);
        pResult->sig.retType = m_coreMethod.m_retType;
        pResult->sig.numArgs = m_coreMethod.m_params.size();
    }
};

// Not currently used...
class VirtualMethod : public Method {
    VTableInfo* m_vtableInfo;
    VirtualMethod(Module* module, CorInfoType returnType, std::vector<Parameter> params, void* addr, VTableInfo* vtableInfo) :
        Method(module, returnType, params, addr),
        m_vtableInfo(vtableInfo) {
    }

    virtual void get_call_info(CORINFO_CALL_INFO *pResult) {
        pResult->codePointerLookup.lookupKind.needsRuntimeLookup = true;
        pResult->codePointerLookup.lookupKind.runtimeLookupKind = CORINFO_LOOKUP_THISOBJ;
        pResult->codePointerLookup.runtimeLookup.testForNull = false;
        pResult->codePointerLookup.runtimeLookup.testForFixup = false;
        //pResult->codePointerLookup.runtimeLookup.helper = CORINFO_HELP_UNDEF;
        //pResult->codePointerLookup.runtimeLookup.indirections = method->m_vtableInfo->indirections;
        //pResult->codePointerLookup.runtimeLookup.offsets[0] = method->m_vtableInfo->offsets[0];
        //pResult->codePointerLookup.runtimeLookup.offsets[1] = method->m_vtableInfo->offsets[1];
        //pResult->codePointerLookup.runtimeLookup.offsets[2] = method->m_vtableInfo->offsets[2];
        //pResult->codePointerLookup.runtimeLookup.offsets[3] = method->m_vtableInfo->offsets[3];

        pResult->verMethodFlags = pResult->methodFlags = CORINFO_FLG_VIRTUAL;
        pResult->kind = CORINFO_VIRTUALCALL_VTABLE;
        pResult->methodFlags = CORINFO_FLG_VIRTUAL;
    }

    virtual DWORD get_method_attrs() {
        return CORINFO_FLG_NOSECURITYWRAP | CORINFO_FLG_VIRTUAL | CORINFO_FLG_NATIVE;
    }
};

#endif