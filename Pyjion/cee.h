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
#include <unordered_map>

#include "corjit.h"
#include "utilcode.h"
#include "openum.h"

using namespace std;

class CExecutionEngine : public IExecutionEngine, public IEEMemoryManager {
public:
    HANDLE m_codeHeap, m_heap;
    DWORD m_tlsIndex;
    PTLS_CALLBACK_FUNCTION* m_callbacks;


    CExecutionEngine() {
        m_codeHeap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
        m_heap = HeapCreate(0, 0, 0);
        m_tlsIndex = TlsAlloc();
        // We can't use new[] here because utilcode isn't spun up yet...
        m_callbacks = (PTLS_CALLBACK_FUNCTION*)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PTLS_CALLBACK_FUNCTION) * MAX_PREDEFINED_TLS_SLOT);
    }

    ~CExecutionEngine() {
        ::HeapDestroy(m_codeHeap);
        ::HeapDestroy(m_heap);
        TlsFree(m_tlsIndex);
        ::HeapFree(GetProcessHeap(), 0, m_callbacks);
    }

    // Thread Local Storage is based on logical threads.  The underlying
    // implementation could be threads, fibers, or something more exotic.
    // Slot numbers are predefined.  This is not a general extensibility
    // mechanism.

    // Associate a callback function for releasing TLS on thread/fiber death.
    // This can be NULL.
    void TLS_AssociateCallback(DWORD slot, PTLS_CALLBACK_FUNCTION callback) {
        m_callbacks[slot] = callback;
    }

    // Get the TLS block for fast Get/Set operations
    PVOID* TLS_GetDataBlock() {
        //printf("get data block\r\n");
        PVOID* block = (PVOID*)TlsGetValue(m_tlsIndex);
        if (block == nullptr) {
            block = new PVOID[MAX_PREDEFINED_TLS_SLOT];
            memset(block, 0, sizeof(PVOID) * MAX_PREDEFINED_TLS_SLOT);
            if (block != nullptr) {
                TlsSetValue(m_tlsIndex, block);
            }
        }
        return block;
    }

    // Get the value at a slot
    PVOID TLS_GetValue(DWORD slot) {
        auto block = TLS_GetDataBlock();
        if (block != nullptr) {
            return block[slot];
        }
        return nullptr;
    }

    // Get the value at a slot, return FALSE if TLS info block doesn't exist
    BOOL TLS_CheckValue(DWORD slot, PVOID * pValue) {
        auto block = TLS_GetDataBlock();
        if (block != nullptr) {
            *pValue = block[slot];
            return TRUE;
        }
        return FALSE;
    }

    // Set the value at a slot
    void TLS_SetValue(DWORD slot, PVOID pData) {
        auto block = TLS_GetDataBlock();
        if (block != nullptr) {
            block[slot] = pData;
        }
    }

    // Free TLS memory block and make callback
    void TLS_ThreadDetaching() {
        auto block = TLS_GetDataBlock();
        if (block != nullptr) {
            for (int i = 0; i < MAX_PREDEFINED_TLS_SLOT; i++) {
                if (m_callbacks[i] != nullptr) {
                    m_callbacks[i](block[i]);
                }
            }
            delete[] block;
        }
    }

    // Critical Sections are sometimes exposed to the host and therefore need to be
    // reflected from all CLR DLLs to the EE.
    //
    // In addition, we always monitor interactions between the lock & the GC, based
    // on the GC mode in which the lock is acquired and we restrict what operations
    // are permitted while holding the lock based on this.
    //
    // Finally, we we rank all our locks to prevent deadlock across all the DLLs of
    // the CLR.  This is the level argument to CreateLock.
    //
    // All usage of these locks must be exception-safe.  To achieve this, we suggest
    // using Holders (see holder.h & crst.h).  In fact, within the EE code cannot
    // hold locks except by using exception-safe holders.

    CRITSEC_COOKIE CreateLock(LPCSTR szTag, LPCSTR level, CrstFlags flags) {
        LPCRITICAL_SECTION critSec = new CRITICAL_SECTION();
        InitializeCriticalSection(critSec);
        return (CRITSEC_COOKIE)critSec;
    }

    void DestroyLock(CRITSEC_COOKIE lock) {
        DeleteCriticalSection((LPCRITICAL_SECTION)lock);
        delete (CRITICAL_SECTION*)lock;
    }

    void AcquireLock(CRITSEC_COOKIE lock) {
        EnterCriticalSection((LPCRITICAL_SECTION)lock);
    }

    void ReleaseLock(CRITSEC_COOKIE lock) {
        LeaveCriticalSection((LPCRITICAL_SECTION)lock);
    }

    EVENT_COOKIE CreateAutoEvent(BOOL bInitialState) {
        return (EVENT_COOKIE)::CreateEventW(NULL, FALSE, bInitialState, NULL);
    }

    EVENT_COOKIE CreateManualEvent(BOOL bInitialState) {
        return (EVENT_COOKIE)::CreateEventW(NULL, TRUE, bInitialState, NULL);
    }

    void CloseEvent(EVENT_COOKIE event) {
        CloseHandle((HANDLE)event);
    }

    BOOL ClrSetEvent(EVENT_COOKIE event) {
        return SetEvent((HANDLE)event);
    }

    BOOL ClrResetEvent(EVENT_COOKIE event) {
        return ResetEvent((HANDLE)event);
    }

    DWORD WaitForEvent(EVENT_COOKIE event, DWORD dwMilliseconds, BOOL bAlertable) {
        return ::WaitForSingleObjectEx((HANDLE)event, dwMilliseconds, bAlertable);
    }

    DWORD WaitForSingleObject(HANDLE handle, DWORD dwMilliseconds) {
        return ::WaitForSingleObject(handle, dwMilliseconds);
    }

    // OS header file defines CreateSemaphore.
    SEMAPHORE_COOKIE ClrCreateSemaphore(DWORD dwInitial, DWORD dwMax) {
        return (SEMAPHORE_COOKIE)CreateSemaphoreW(NULL, dwInitial, dwMax, NULL);
    }

    void ClrCloseSemaphore(SEMAPHORE_COOKIE semaphore) {
        CloseHandle((HANDLE)semaphore);
    }

    DWORD ClrWaitForSemaphore(SEMAPHORE_COOKIE semaphore, DWORD dwMilliseconds, BOOL bAlertable) {
        return ::WaitForSingleObjectEx((HANDLE)semaphore, dwMilliseconds, bAlertable);
    }

    BOOL ClrReleaseSemaphore(SEMAPHORE_COOKIE semaphore, LONG lReleaseCount, LONG *lpPreviousCount) {
        return ::ReleaseSemaphore(semaphore, lReleaseCount, lpPreviousCount);
    }

    MUTEX_COOKIE ClrCreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName) {
        printf("create mutex not impl\r\n");
        return NULL;
    }

    DWORD ClrWaitForMutex(MUTEX_COOKIE mutex, DWORD dwMilliseconds, BOOL bAlertable) {
        printf("wait for mutex impl\r\n");
        return 0;
    }

    BOOL ClrReleaseMutex(MUTEX_COOKIE mutex) {
        printf("release mutex\r\n");
        return FALSE;
    }

    void ClrCloseMutex(MUTEX_COOKIE mutex) {
        printf("close mutex\r\n");
    }

    DWORD ClrSleepEx(DWORD dwMilliseconds, BOOL bAlertable) {
        return ::SleepEx(dwMilliseconds, bAlertable);
    }

    BOOL ClrAllocationDisallowed() {
        return FALSE;
    }

    void GetLastThrownObjectExceptionFromThread(void **ppvException) {
        ppvException = nullptr;
    }

    ULONG AddRef(void) {
        return InterlockedIncrement(&m_refCount);
    }

    HRESULT QueryInterface(REFIID iid, void** ppvObject) {
        if (iid == IID_IUnknown) {
            *ppvObject = (void*)static_cast<IExecutionEngine*>(this);
            return S_OK;
        }
        else if (iid == IID_IExecutionEngine) {
            *ppvObject = (void*)static_cast<IExecutionEngine*>(this);
            return S_OK;
        }
        else if (iid == IID_IEEMemoryManager) {
            *ppvObject = (void*)static_cast<IEEMemoryManager*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG Release(void) {
        return InterlockedDecrement(&m_refCount);
    }

    LPVOID ClrVirtualAlloc(
        LPVOID lpAddress,        // region to reserve or commit
        SIZE_T dwSize,           // size of region
        DWORD flAllocationType,  // type of allocation
        DWORD flProtect          // type of access protection
        ) {
        return ::VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
    }

    BOOL ClrVirtualFree(
        LPVOID lpAddress,   // address of region
        SIZE_T dwSize,      // size of region
        DWORD dwFreeType    // operation type
        ) {
        return ::VirtualFree(lpAddress, dwSize, dwFreeType);
    }

    SIZE_T ClrVirtualQuery(
        const void* lpAddress,                    // address of region
        PMEMORY_BASIC_INFORMATION lpBuffer,  // information buffer
        SIZE_T dwLength                      // size of buffer
        ) {
        return ::VirtualQuery(lpAddress, lpBuffer, dwLength);
    }

    BOOL ClrVirtualProtect(
        LPVOID lpAddress,       // region of committed pages
        SIZE_T dwSize,          // size of the region
        DWORD flNewProtect,     // desired access protection
        DWORD* lpflOldProtect   // old protection
        ) {
        return ::VirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);
    }

    HANDLE ClrGetProcessHeap() {
        return m_heap;
    }

    HANDLE ClrHeapCreate(
        DWORD flOptions,       // heap allocation attributes
        SIZE_T dwInitialSize,  // initial heap size
        SIZE_T dwMaximumSize   // maximum heap size
        ) {
        return ::HeapCreate(flOptions, dwInitialSize, dwMaximumSize);
    }

    BOOL ClrHeapDestroy(
        HANDLE hHeap   // handle to heap
        ) {
        return ::HeapDestroy(hHeap);
    }

    LPVOID ClrHeapAlloc(
        HANDLE hHeap,   // handle to private heap block
        DWORD dwFlags,  // heap allocation control
        SIZE_T dwBytes  // number of bytes to allocate
        ) {
        return ::HeapAlloc(hHeap, dwFlags, dwBytes);
    }

    BOOL ClrHeapFree(
        HANDLE hHeap,  // handle to heap
        DWORD dwFlags, // heap free options
        LPVOID lpMem   // pointer to memory
        ) {
        return ::HeapFree(hHeap, dwFlags, lpMem);
    }

    BOOL ClrHeapValidate(
        HANDLE hHeap,  // handle to heap
        DWORD dwFlags, // heap access options
        const void* lpMem   // optional pointer to memory block
        ) {
        return ::HeapValidate(hHeap, dwFlags, lpMem);
    }

    HANDLE ClrGetProcessExecutableHeap() {
        if (m_executableHeap == NULL) {
            m_executableHeap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
        }
        return m_executableHeap;
    }


private:
    ULONG m_refCount;
    HANDLE m_executableHeap;
};  // interface IExecutionEngine

class CCorJitHost : public ICorJitHost {
	void * allocateMemory(size_t size, bool usePageAllocator = false)
	{
		return nullptr;
	}

	void freeMemory(void * block, bool usePageAllocator = false)
	{
	}

	int getIntConfigValue(const wchar_t * name, int defaultValue)
	{
		return defaultValue;
	}

	const wchar_t * getStringConfigValue(const wchar_t * name)
	{
		return nullptr;
	}

	void freeStringConfigValue(const wchar_t * value)
	{
	}
};

void CeeInit();

#endif
