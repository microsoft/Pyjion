#ifndef __CEE_H__
#define __CEE_H__

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

#include "corjit.h"
#include "utilcode.h"
#include "openum.h"

using namespace std;

class CExecutionEngine : public IExecutionEngine
{
public:
	// Thread Local Storage is based on logical threads.  The underlying
	// implementation could be threads, fibers, or something more exotic.
	// Slot numbers are predefined.  This is not a general extensibility
	// mechanism.

	// Associate a callback function for releasing TLS on thread/fiber death.
	// This can be NULL.
	void TLS_AssociateCallback(DWORD slot, PTLS_CALLBACK_FUNCTION callback) {
		printf("associate callback\r\n");
	}

	// Get the TLS block for fast Get/Set operations
	PVOID* TLS_GetDataBlock() {
		//printf("get data block\r\n");
		return NULL;
	}

	// Get the value at a slot
	PVOID TLS_GetValue(DWORD slot) {
		return TlsGetValue(slot);
	}

	// Get the value at a slot, return FALSE if TLS info block doesn't exist
	BOOL TLS_CheckValue(DWORD slot, PVOID * pValue) {
		printf("check value not impl\r\n");
		return FALSE;
	}

	// Set the value at a slot
	void TLS_SetValue(DWORD slot, PVOID pData) {
		TlsSetValue(slot, pData);
	}

	// Free TLS memory block and make callback
	void TLS_ThreadDetaching() {
		printf("thread detaching...\r\n");
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
			*ppvObject = (void*)static_cast<IUnknown*>(this);
			return S_OK;
		}
		else if (iid == IID_IExecutionEngine) {
			*ppvObject = (void*)static_cast<IExecutionEngine*>(this);
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG Release(void) {
		return InterlockedDecrement(&m_refCount);
	}

private:
	ULONG m_refCount;
};  // interface IExecutionEngine

void CeeInit();

#endif