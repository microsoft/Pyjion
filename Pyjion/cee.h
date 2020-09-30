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

#ifndef CEE_H
#define CEE_H

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
#include <pal.h>
#include <map>

#include "openum.h"

using namespace std;

class CCorJitHost : public ICorJitHost {
protected:
    map<const WCHAR*, int> intSettings;
    map<const WCHAR*, WCHAR*> strSettings;

public: CCorJitHost(){
        intSettings = map<const WCHAR*, int>();
        strSettings = map<const WCHAR*, WCHAR*>();

#if DEBUG
        intSettings = {
                "JitLsraStats": 1,
                "DumpJittedMethods": 1,
                "JitDumpToDebugger": 1
        };
#endif
    }

	void * allocateMemory(size_t size) override
	{
        return PyMem_Malloc(size);
	}

	void freeMemory(void * block) override
	{
	    return PyMem_Free(block);
	}

	int getIntConfigValue(const WCHAR* name, int defaultValue) override
	{
        if (intSettings.find(name) != intSettings.end())
            return intSettings[name];
        return defaultValue;
	}

	const WCHAR * getStringConfigValue(const WCHAR* name) override
	{
        if (strSettings.find(name) != strSettings.end())
            return strSettings[name];
        return NULL;
	}

	void freeStringConfigValue(const WCHAR* value) override
	{
        // noop.
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
