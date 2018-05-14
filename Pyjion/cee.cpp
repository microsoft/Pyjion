#include "cee.h"

#include <clrhost.h>
#include <corjit.h>
#include <openum.h>
#include <utilcode.h>
#include <sstring.h>
CExecutionEngine g_execEngine;
extern ICorJitCompiler* g_jit;

BOOL EEHeapFreeInProcessHeap(DWORD dwFlags, LPVOID lpMem) {
	return ::HeapFree(g_execEngine.ClrGetProcessHeap(), dwFlags, lpMem);
}

LPVOID EEHeapAllocInProcessHeap(DWORD dwFlags, SIZE_T dwBytes) {
	return ::HeapAlloc(g_execEngine.ClrGetProcessHeap(), dwFlags, dwBytes);
}

void* __stdcall GetCLRFunction(LPCSTR functionName) {
	if (strcmp(functionName, "EEHeapAllocInProcessHeap") == 0) {
		return ::EEHeapAllocInProcessHeap;
	}
	else if (strcmp(functionName, "EEHeapFreeInProcessHeap") == 0) {
		return ::EEHeapFreeInProcessHeap;
	}
	printf("get clr function %s\n", functionName);
	return NULL;
}

IExecutionEngine* __stdcall IEE() {
	return &g_execEngine;
}

#ifndef PLATFORM_UNIX
HRESULT __stdcall GetCORSystemDirectoryInternal(SString& pbuffer) {
	printf("get cor system\n");
	return S_OK;
}
#endif

CCorJitHost g_pyjionjitHost;

void CeeInit() {
	CoreClrCallbacks cccallbacks;
#ifndef PLATFORM_UNIX
	cccallbacks.m_hmodCoreCLR = (HINSTANCE)GetModuleHandleW(NULL);
	cccallbacks.m_pfnGetCORSystemDirectory = GetCORSystemDirectoryInternal;
#endif
	cccallbacks.m_pfnIEE = IEE;
	cccallbacks.m_pfnGetCLRFunction = GetCLRFunction;

	InitUtilcode(cccallbacks);
	// TODO: We should re-enable contracts and handle exceptions from OOM
	// and just fail the whole compilation if we hit that.  Right now we
	// just leak an exception out across the JIT boundary.

	jitStartup(&g_pyjionjitHost);

	g_jit = getJit();

#if _DEBUG
	DisableThrowCheck();
#endif
}
#ifndef PLATFORM_UNIX
extern "C" __declspec(dllexport) BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH: break;
	case DLL_PROCESS_DETACH: break;
	case DLL_THREAD_ATTACH: break;
	case DLL_THREAD_DETACH:
		g_execEngine.TLS_ThreadDetaching();
		break;
	}
	return TRUE;
}
#endif

class InitHolder {
public:
	InitHolder() {
		CeeInit();
	}
};

InitHolder g_initHolder;

#ifdef PLATFORM_UNIX
class Compiler;
struct ThreadLocalInfo
{
	Compiler* m_pCompiler;
};

#ifndef __llvm__
__declspec(thread) ThreadLocalInfo gCurrentThreadInfo;
#else // !__llvm__
__thread ThreadLocalInfo gCurrentThreadInfo;
#endif // !__llvm__

Compiler* GetTlsCompiler()
{
	return gCurrentThreadInfo.m_pCompiler;
}
void SetTlsCompiler(Compiler* c)
{
	gCurrentThreadInfo.m_pCompiler = c;
}

#endif
