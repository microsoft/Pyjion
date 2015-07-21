#include "cee.h"

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

IExecutionEngine* __stdcall IEE() {
	printf("iee\n");
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