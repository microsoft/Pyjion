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

#include "cee.h"
#include "jitinfo.h"

HRESULT __stdcall GetCORSystemDirectoryInternal(__out_ecount_part_opt(cchBuffer, *pdwLength) LPWSTR pBuffer,
	DWORD  cchBuffer,
	__out_opt DWORD* pdwLength) {
	printf("get cor system\n");
	return S_OK;
}

void* __stdcall GetCLRFunction(LPCSTR functionName) {
	if (strcmp(functionName, "EEHeapAllocInProcessHeap") == 0) {
		return ::GlobalAlloc;
	}
	else if (strcmp(functionName, "EEHeapFreeInProcessHeap") == 0) {
		return ::GlobalFree;
	}
	printf("get clr function %s\n", functionName);
	return NULL;
}

CExecutionEngine g_execEngine;
CorJitInfo g_corJitInfo(g_execEngine);

IExecutionEngine* __stdcall IEE() {
	//printf("iee\n");
	return &g_execEngine;
}

void CeeInit() {
	CoreClrCallbacks cccallbacks;
	cccallbacks.m_hmodCoreCLR = (HINSTANCE)GetModuleHandleW(NULL);
	cccallbacks.m_pfnIEE = IEE;
	cccallbacks.m_pfnGetCORSystemDirectory = GetCORSystemDirectoryInternal;
	cccallbacks.m_pfnGetCLRFunction = GetCLRFunction;

	InitUtilcode(cccallbacks);
}