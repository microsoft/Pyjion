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

class Method {
    Module* m_module;
public:
    vector<BYTE> m_il;
    vector<Parameter> m_params, m_locals;
    CorInfoType m_retType;
    void* m_addr;
    VTableInfo* m_vtableInfo;
public:
    Method() {
    }

    Method(Module* module, CorInfoType returnType, std::vector<Parameter> params, void* addr, VTableInfo* vtableInfo = nullptr) {
        m_retType = returnType;
        m_params = params;
        m_module = module;
        m_addr = addr;
        m_vtableInfo = vtableInfo;
    }
};

#endif