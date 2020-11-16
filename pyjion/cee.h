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

#ifndef PYJION_CEE_H
#define PYJION_CEE_H

#define FEATURE_NO_HOST

#include <Python.h>
#include <stdint.h>
#include <windows.h>
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
#include <map>
#include <cwchar>
#include <string>

#include "openum.h"

using namespace std;

class CCorJitHost : public ICorJitHost {
protected:
#ifdef WINDOWS
    map<const WCHAR *, int> intSettings;
    map<const WCHAR *, const WCHAR *> strSettings;
    typedef pair <const WCHAR*, int> Str_Int_Pair;
    typedef pair <const WCHAR*, const WCHAR*> Str_Str_Pair;
#else
    map<std::u16string, int> intSettings;
    map<std::u16string, const char16_t*> strSettings;
#endif

public: CCorJitHost(){
    
#ifdef DUMP_JIT_TRACES
#ifndef WINDOWS
        intSettings[u"DumpJittedMethods"] = 1;
        intSettings[u"JitDumpIR"] = 1;
        strSettings[u"JitDump"] = u"*";
#else
        intSettings.insert(Str_Int_Pair(L"DumpJittedMethods", 1));
        intSettings.insert(Str_Int_Pair(L"JitDumpIR", 1));
        strSettings.insert(Str_Str_Pair(L"JitDump", L"*"));
#endif
#endif
    }

	void * allocateMemory(size_t size) override
	{
        // Use CPython's memory allocator (alignment 16)
        return PyMem_Malloc(size);
	}

	void freeMemory(void * block) override
	{
	    return PyMem_Free(block);
	}

	int getIntConfigValue(const WCHAR* name, int defaultValue) override
	{
        if (intSettings.find(name) != intSettings.end())
            return intSettings.at(name);
        return defaultValue;
	}

	const WCHAR * getStringConfigValue(const WCHAR* name) override
	{
        if (strSettings.find(name) != strSettings.end())
            return strSettings.at(name);
        return nullptr;
	}

	void freeStringConfigValue(const WCHAR* value) override
	{
        // TODO : Figure out what this is for? Delete key?
	}

	void* allocateSlab(size_t size, size_t* pActualSize) override
    {
        *pActualSize = size;
        return allocateMemory(size);
    }

    void freeSlab(void* slab, size_t actualSize) override
    {
        freeMemory(slab);
    }
};

void CeeInit();

#endif
