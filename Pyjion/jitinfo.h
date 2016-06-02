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

#ifndef JITINFO_H
#define JITINFO_H


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

#include <Python.h>
#include <utilcode.h>
#include <frameobject.h>
#include <opcode.h>

#include <vector>
#include <unordered_map>

#include <corjit.h>
#include <utilcode.h>
#include <openum.h>

#include "codemodel.h"
#include "cee.h"
#include "ipycomp.h"

using namespace std;

class CorJitInfo : public ICorJitInfo, public JittedCode {
    CExecutionEngine& m_executionEngine;
    void* m_codeAddr;
    void* m_dataAddr;
    PyCodeObject *m_code;
    UserModule* m_module;

public:

    CorJitInfo(CExecutionEngine& executionEngine, PyCodeObject* code, UserModule* module) : m_executionEngine(executionEngine) {
        m_codeAddr = m_dataAddr = nullptr;
        m_code = code;
        m_module = module;
    }

    ~CorJitInfo() {
        if (m_codeAddr != nullptr) {
            freeMem(m_codeAddr);
        }
        if (m_dataAddr != nullptr) {
            ::GlobalFree(m_dataAddr);
        }
        delete m_module;
    }

    void* get_code_addr() {
        return m_codeAddr;
    }

    /* ICorJitInfo */
    IEEMemoryManager* getMemoryManager() {
        return &m_executionEngine;
    }

    void freeMem(PVOID code) {
        HeapFree(m_executionEngine.m_codeHeap, 0, code);
    }

    virtual void allocMem(
        ULONG               hotCodeSize,    /* IN */
        ULONG               coldCodeSize,   /* IN */
        ULONG               roDataSize,     /* IN */
        ULONG               xcptnsCount,    /* IN */
        CorJitAllocMemFlag  flag,           /* IN */
        void **             hotCodeBlock,   /* OUT */
        void **             coldCodeBlock,  /* OUT */
        void **             roDataBlock     /* OUT */
        ) {
        //printf("allocMem\r\n");
        // TODO: Alignment?
        //printf("Code size: %d\r\n", hotCodeSize);
        auto code = HeapAlloc(m_executionEngine.m_codeHeap, 0, hotCodeSize);
        *hotCodeBlock = m_codeAddr = code;
        if (roDataSize != 0) {
            // TODO: This mem needs to be freed...
            *roDataBlock = m_dataAddr = GlobalAlloc(0, roDataSize);
        }
    }

    virtual void reserveUnwindInfo(
        BOOL                isFunclet,             /* IN */
        BOOL                isColdCode,            /* IN */
        ULONG               unwindSize             /* IN */
        ) {
        //printf("reserveUnwindInfo\r\n");
    }

    virtual void allocUnwindInfo(
        BYTE *              pHotCode,              /* IN */
        BYTE *              pColdCode,             /* IN */
        ULONG               startOffset,           /* IN */
        ULONG               endOffset,             /* IN */
        ULONG               unwindSize,            /* IN */
        BYTE *              pUnwindBlock,          /* IN */
        CorJitFuncKind      funcKind               /* IN */
        ) {
        //printf("allocUnwindInfo\r\n");
    }

    virtual void * allocGCInfo(
        size_t                  size        /* IN */
        ) {
        //printf("allocGCInfo\r\n");
        return malloc(size);
    }

    virtual void yieldExecution() {
    }

    virtual void setEHcount(
        unsigned                cEH          /* IN */
        ) {
        printf("setEHcount\r\n");
    }

    virtual void setEHinfo(
        unsigned                 EHnumber,   /* IN  */
        const CORINFO_EH_CLAUSE *clause      /* IN */
        ) {
        printf("setEHinfo\r\n");
    }

    virtual BOOL logMsg(unsigned level, const char* fmt, va_list args) {
        if (level < 7) {
            vprintf(fmt, args);
        }
        return TRUE;
    }

    int doAssert(const char* szFile, int iLine, const char* szExpr) {
        printf("Assert: %s %d", szFile, iLine);
        return 0;
    }

    virtual void reportFatalError(CorJitResult result) {
        printf("Fatal error %X\r\n", result);
    }

    virtual HRESULT allocBBProfileBuffer(
        ULONG                 count,           // The number of basic blocks that we have
        ProfileBuffer **      profileBuffer
        ) {
        printf("Alloc bb profile buffer\r\n");
        return E_FAIL;
    }

    virtual HRESULT getBBProfileData(
        CORINFO_METHOD_HANDLE ftnHnd,
        ULONG *               count,           // The number of basic blocks that we have
        ProfileBuffer **      profileBuffer,
        ULONG *               numRuns
        ) {
        printf("getBBProfileData\r\n");
        return E_FAIL;
    }

#if !defined(RYUJIT_CTPBUILD)
    // Associates a native call site, identified by its offset in the native code stream, with
    // the signature information and method handle the JIT used to lay out the call site. If
    // the call site has no signature information (e.g. a helper call) or has no method handle
    // (e.g. a CALLI P/Invoke), then null should be passed instead.
    virtual void recordCallSite(
        ULONG                 instrOffset,  /* IN */
        CORINFO_SIG_INFO *    callSig,      /* IN */
        CORINFO_METHOD_HANDLE methodHandle  /* IN */
        ) {
        //printf("recordCallSite\r\n");
    }

#endif // !defined(RYUJIT_CTPBUILD)

    virtual void recordRelocation(
        void *                 location,   /* IN  */
        void *                 target,     /* IN  */
        WORD                   fRelocType, /* IN  */
        WORD                   slotNum = 0,  /* IN  */
        INT32                  addlDelta = 0 /* IN  */
        ) {
        //printf("recordRelocation\r\n");
        switch (fRelocType) {
            case IMAGE_REL_BASED_DIR64:
                *((UINT64 *)((BYTE *)location + slotNum)) = (UINT64)target;
                break;
#ifdef _TARGET_AMD64_
            case IMAGE_REL_BASED_REL32:
            {
                target = (BYTE *)target + addlDelta;

                INT32 * fixupLocation = (INT32 *)((BYTE *)location + slotNum);
                BYTE * baseAddr = (BYTE *)fixupLocation + sizeof(INT32);

                INT64 delta = (INT64)((BYTE *)target - baseAddr);

                //
                // Do we need to insert a jump stub to make the source reach the target?
                //
                // Note that we cannot stress insertion of jump stub by inserting it unconditionally. JIT records the relocations 
                // for intra-module jumps and calls. It does not expect the register used by the jump stub to be trashed.
                //
                //if (!FitsInI4(delta))
                //{
                //	if (m_fAllowRel32)
                //	{
                //		//
                //		// When m_fAllowRel32 == TRUE, the JIT will use REL32s for both data addresses and direct code targets.
                //		// Since we cannot tell what the relocation is for, we have to defensively retry.
                //		//
                //		m_fRel32Overflow = TRUE;
                //		delta = 0;
                //	}
                //	else
                //	{
                //		//
                //		// When m_fAllowRel32 == FALSE, the JIT will use a REL32s for direct code targets only.
                //		// Use jump stub.
                //		// 
                //		delta = rel32UsingJumpStub(fixupLocation, (PCODE)target, m_pMethodBeingCompiled);
                //	}
                //}

                // Write the 32-bits pc-relative delta into location
                *fixupLocation = (INT32)delta;
            }
            break;
#endif // _TARGET_AMD64_

            default:
                printf("!!!!!!!!!!!!!! unsupported reloc type\r\n");
        }
    }

    virtual WORD getRelocTypeHint(void * target) {
        return -1;
    }

    virtual void getModuleNativeEntryPointRange(
        void ** pStart, /* OUT */
        void ** pEnd    /* OUT */
        ) {
        printf("getModuleNativeEntryPointRange\r\n");
    }

    // For what machine does the VM expect the JIT to generate code? The VM
    // returns one of the IMAGE_FILE_MACHINE_* values. Note that if the VM
    // is cross-compiling (such as the case for crossgen), it will return a
    // different value than if it was compiling for the host architecture.
    // 
    virtual DWORD getExpectedTargetArchitecture() {
        //printf("getExpectedTargetArchitecture\r\n");
#ifdef _TARGET_AMD64_
        return IMAGE_FILE_MACHINE_AMD64;
#elif defined(_TARGET_X86_)
        return IMAGE_FILE_MACHINE_I386;
#elif defined(_TARGET_ARM_)
        return IMAGE_FILE_MACHINE_ARM;
#endif
    }

    /* ICorDynamicInfo */

    //
    // These methods return values to the JIT which are not constant
    // from session to session.
    //
    // These methods take an extra parameter : void **ppIndirection.
    // If a JIT supports generation of prejit code (install-o-jit), it
    // must pass a non-null value for this parameter, and check the
    // resulting value.  If *ppIndirection is NULL, code should be
    // generated normally.  If non-null, then the value of
    // *ppIndirection is an address in the cookie table, and the code
    // generator needs to generate an indirection through the table to
    // get the resulting value.  In this case, the return result of the
    // function must NOT be directly embedded in the generated code.
    //
    // Note that if a JIT does not support prejit code generation, it
    // may ignore the extra parameter & pass the default of NULL - the
    // prejit ICorDynamicInfo implementation will see this & generate
    // an error if the jitter is used in a prejit scenario.
    //

    // Return details about EE internal data structures

    virtual DWORD getThreadTLSIndex(
        void                  **ppIndirection = NULL
        ) {
        printf("getThreadTLSIndex\r\n");
        return 0;
    }

    virtual const void * getInlinedCallFrameVptr(
        void                  **ppIndirection = NULL
        ) {
        printf("getInlinedCallFrameVptr\r\n");
        return NULL;
    }

    virtual LONG * getAddrOfCaptureThreadGlobal(
        void                  **ppIndirection = NULL
        ) {
        printf("getAddrOfCaptureThreadGlobal\r\n");
        return NULL;
    }

    virtual SIZE_T*       getAddrModuleDomainID(CORINFO_MODULE_HANDLE   module) {
        printf("getAddrModuleDomainID\r\n");
        return 0;
    }

    static void ThrowFunc() {
    }

    static void FailFast() {
        ::ExitProcess(1);
    }

    // return the native entry point to an EE helper (see CorInfoHelpFunc)
    virtual void* getHelperFtn(
        CorInfoHelpFunc         ftnNum,
        void                  **ppIndirection = NULL
        ) {
        switch (ftnNum) {
            case CORINFO_HELP_THROW: return &ThrowFunc;
            case CORINFO_HELP_FAIL_FAST: return &FailFast;
            case CORINFO_HELP_DBLREM:
                auto res = (double(*)(double, double))&fmod;
                return res;
        }
        printf("unknown getHelperFtn\r\n");
        return NULL;
    }

    // return a callable address of the function (native code). This function
    // may return a different value (depending on whether the method has
    // been JITed or not.
    virtual void getFunctionEntryPoint(
        CORINFO_METHOD_HANDLE   ftn,                 /* IN  */
        CORINFO_CONST_LOOKUP *  pResult,             /* OUT */
        CORINFO_ACCESS_FLAGS    accessFlags = CORINFO_ACCESS_ANY) {
        BaseMethod* method = (BaseMethod*)ftn;
        method->getFunctionEntryPoint(pResult);
    }

    // return a directly callable address. This can be used similarly to the
    // value returned by getFunctionEntryPoint() except that it is
    // guaranteed to be multi callable entrypoint.
    virtual void getFunctionFixedEntryPoint(
        CORINFO_METHOD_HANDLE   ftn,
        CORINFO_CONST_LOOKUP *  pResult) {
        printf("getFunctionFixedEntryPoint\r\n");
    }

    // get the synchronization handle that is passed to monXstatic function
    virtual void* getMethodSync(
        CORINFO_METHOD_HANDLE               ftn,
        void                  **ppIndirection = NULL
        ) {
        printf("getMethodSync\r\n");
        return nullptr;
    }

#if defined(RYUJIT_CTPBUILD)
    // These entry points must be called if a handle is being embedded in
    // the code to be passed to a JIT helper function. (as opposed to just
    // being passed back into the ICorInfo interface.)

    // a module handle may not always be available. A call to embedModuleHandle should always
    // be preceeded by a call to canEmbedModuleHandleForHelper. A dynamicMethod does not have a module
    virtual bool canEmbedModuleHandleForHelper(
        CORINFO_MODULE_HANDLE   handle
        ) {
        printf("canEmbedModuleHandleForHelper\r\n");
        return FALSE;
    }

#else
    // get slow lazy string literal helper to use (CORINFO_HELP_STRCNS*). 
    // Returns CORINFO_HELP_UNDEF if lazy string literal helper cannot be used.
    virtual CorInfoHelpFunc getLazyStringLiteralHelper(
        CORINFO_MODULE_HANDLE   handle
        ) {
        printf("getLazyStringLiteralHelper\r\n"); return CORINFO_HELP_UNDEF;
    }
#endif
    virtual CORINFO_MODULE_HANDLE embedModuleHandle(
        CORINFO_MODULE_HANDLE   handle,
        void                  **ppIndirection = NULL
        ) {
        printf("embedModuleHandle\r\n"); return nullptr;
    }

    virtual CORINFO_CLASS_HANDLE embedClassHandle(
        CORINFO_CLASS_HANDLE    handle,
        void                  **ppIndirection = NULL
        ) {
        printf("embedClassHandle\r\n"); return nullptr;
    }

    virtual CORINFO_METHOD_HANDLE embedMethodHandle(
        CORINFO_METHOD_HANDLE   handle,
        void                  **ppIndirection = NULL
        ) {
        printf("embedMethodHandle\r\n");
        *ppIndirection = NULL;
        return handle;
    }

    virtual CORINFO_FIELD_HANDLE embedFieldHandle(
        CORINFO_FIELD_HANDLE    handle,
        void                  **ppIndirection = NULL
        ) {
        printf("embedFieldHandle\r\n"); return nullptr;
    }

    // Given a module scope (module), a method handle (context) and
    // a metadata token (metaTOK), fetch the handle
    // (type, field or method) associated with the token.
    // If this is not possible at compile-time (because the current method's
    // code is shared and the token contains generic parameters)
    // then indicate how the handle should be looked up at run-time.
    //
    virtual void embedGenericHandle(
        CORINFO_RESOLVED_TOKEN *        pResolvedToken,
        BOOL                            fEmbedParent, // TRUE - embeds parent type handle of the field/method handle
        CORINFO_GENERICHANDLE_RESULT *  pResult) {
        printf("embedGenericHandle\r\n");
    }

    // Return information used to locate the exact enclosing type of the current method.
    // Used only to invoke .cctor method from code shared across generic instantiations
    //   !needsRuntimeLookup       statically known (enclosing type of method itself)
    //   needsRuntimeLookup:
    //      CORINFO_LOOKUP_THISOBJ     use vtable pointer of 'this' param
    //      CORINFO_LOOKUP_CLASSPARAM  use vtable hidden param
    //      CORINFO_LOOKUP_METHODPARAM use enclosing type of method-desc hidden param
    virtual CORINFO_LOOKUP_KIND getLocationOfThisType(
        CORINFO_METHOD_HANDLE context
        ) {
        printf("getLocationOfThisType\r\n");
        return CORINFO_LOOKUP_KIND{ FALSE };
    }

    // return the unmanaged target *if method has already been prelinked.*
    virtual void* getPInvokeUnmanagedTarget(
        CORINFO_METHOD_HANDLE   method,
        void                  **ppIndirection = NULL
        ) {
        printf("getPInvokeUnmanagedTarget\r\n");
        return nullptr;
    }

    // return address of fixup area for late-bound PInvoke calls.
    virtual void* getAddressOfPInvokeFixup(
        CORINFO_METHOD_HANDLE   method,
        void                  **ppIndirection = NULL
        ) {
        printf("getAddressOfPInvokeFixup\r\n");
        return nullptr;
    }

    // Generate a cookie based on the signature that would needs to be passed
    // to CORINFO_HELP_PINVOKE_CALLI
    virtual LPVOID GetCookieForPInvokeCalliSig(
        CORINFO_SIG_INFO* szMetaSig,
        void           ** ppIndirection = NULL
        ) {
        printf("GetCookieForPInvokeCalliSig\r\n");
        return nullptr;
    }

    // returns true if a VM cookie can be generated for it (might be false due to cross-module
    // inlining, in which case the inlining should be aborted)
    virtual bool canGetCookieForPInvokeCalliSig(
        CORINFO_SIG_INFO* szMetaSig
        ) {
        printf("canGetCookieForPInvokeCalliSig\r\n");
        return true;
    }

    // Gets a handle that is checked to see if the current method is
    // included in "JustMyCode"
    virtual CORINFO_JUST_MY_CODE_HANDLE getJustMyCodeHandle(
        CORINFO_METHOD_HANDLE       method,
        CORINFO_JUST_MY_CODE_HANDLE**ppIndirection = NULL
        ) {
        printf("getJustMyCodeHandle\r\n");
        return nullptr;
    }

    // Gets a method handle that can be used to correlate profiling data.
    // This is the IP of a native method, or the address of the descriptor struct
    // for IL.  Always guaranteed to be unique per process, and not to move. */
    virtual void GetProfilingHandle(
        BOOL                      *pbHookFunction,
        void                     **pProfilerHandle,
        BOOL                      *pbIndirectedHandles
        ) {
        printf("GetProfilingHandle\r\n");
    }

    // Returns instructions on how to make the call. See code:CORINFO_CALL_INFO for possible return values.
    virtual void getCallInfo(
        // Token info
        CORINFO_RESOLVED_TOKEN * pResolvedToken,

        //Generics info
        CORINFO_RESOLVED_TOKEN * pConstrainedResolvedToken,

        //Security info
        CORINFO_METHOD_HANDLE   callerHandle,

        //Jit info
        CORINFO_CALLINFO_FLAGS  flags,

        //out params
        CORINFO_CALL_INFO       *pResult
        ) {
        auto method = (BaseMethod*)pResolvedToken->hMethod;
        pResult->hMethod = (CORINFO_METHOD_HANDLE)method;

        method->get_call_info(pResult);
        pResult->nullInstanceCheck = false;
        pResult->sig.callConv = CORINFO_CALLCONV_DEFAULT;
        pResult->sig.retTypeClass = nullptr;
        pResult->verSig = pResult->sig;
        pResult->accessAllowed = CORINFO_ACCESS_ALLOWED;
        //printf("getCallInfo\r\n");
    }

    virtual BOOL canAccessFamily(CORINFO_METHOD_HANDLE hCaller,
        CORINFO_CLASS_HANDLE hInstanceType) {
        printf("canAccessFamily\r\n");
        return FALSE;
    }

    // Returns TRUE if the Class Domain ID is the RID of the class (currently true for every class
    // except reflection emitted classes and generics)
    virtual BOOL isRIDClassDomainID(CORINFO_CLASS_HANDLE cls) {
        printf("isRIDClassDomainID\r\n");
        return FALSE;
    }

    // returns the class's domain ID for accessing shared statics
    virtual unsigned getClassDomainID(
        CORINFO_CLASS_HANDLE    cls,
        void                  **ppIndirection = NULL
        ) {
        printf("getClassDomainID\r\n");
        return 0;
    }


    // return the data's address (for static fields only)
    virtual void* getFieldAddress(
        CORINFO_FIELD_HANDLE    field,
        void                  **ppIndirection = NULL
        ) {
        printf("getFieldAddress\r\n");
        return nullptr;
    }

    // registers a vararg sig & returns a VM cookie for it (which can contain other stuff)
    virtual CORINFO_VARARGS_HANDLE getVarArgsHandle(
        CORINFO_SIG_INFO       *pSig,
        void                  **ppIndirection = NULL
        ) {
        printf("getVarArgsHandle\r\n");
        return nullptr;
    }

    // returns true if a VM cookie can be generated for it (might be false due to cross-module
    // inlining, in which case the inlining should be aborted)
    virtual bool canGetVarArgsHandle(
        CORINFO_SIG_INFO       *pSig
        ) {
        printf("canGetVarArgsHandle\r\n");
        return false;
    }

    // Allocate a string literal on the heap and return a handle to it
    virtual InfoAccessType constructStringLiteral(
        CORINFO_MODULE_HANDLE   module,
        mdToken                 metaTok,
        void                  **ppValue
        ) {
        printf("constructStringLiteral\r\n");
        return IAT_VALUE;
    }

    virtual InfoAccessType emptyStringLiteral(
        void                  **ppValue
        ) {
        printf("emptyStringLiteral\r\n");
        return IAT_VALUE;
    }

    // (static fields only) given that 'field' refers to thread local store,
    // return the ID (TLS index), which is used to find the begining of the
    // TLS data area for the particular DLL 'field' is associated with.
    virtual DWORD getFieldThreadLocalStoreID(
        CORINFO_FIELD_HANDLE    field,
        void                  **ppIndirection = NULL
        ) {
        printf("getFieldThreadLocalStoreID\r\n"); return 0;
    }

    // Sets another object to intercept calls to "self" and current method being compiled
    virtual void setOverride(
        ICorDynamicInfo             *pOverride,
        CORINFO_METHOD_HANDLE       currentMethod
        ) {
        printf("setOverride\r\n");
    }

    // Adds an active dependency from the context method's module to the given module
    // This is internal callback for the EE. JIT should not call it directly.
    virtual void addActiveDependency(
        CORINFO_MODULE_HANDLE       moduleFrom,
        CORINFO_MODULE_HANDLE       moduleTo
        ) {
        printf("addActiveDependency\r\n");
    }

    virtual CORINFO_METHOD_HANDLE GetDelegateCtor(
        CORINFO_METHOD_HANDLE  methHnd,
        CORINFO_CLASS_HANDLE   clsHnd,
        CORINFO_METHOD_HANDLE  targetMethodHnd,
        DelegateCtorArgs *     pCtorData
        ) {
        printf("GetDelegateCtor\r\n"); return nullptr;
    }

    virtual void MethodCompileComplete(
        CORINFO_METHOD_HANDLE methHnd
        ) {
        printf("MethodCompileComplete\r\n");
    }

    // return a thunk that will copy the arguments for the given signature.
    virtual void* getTailCallCopyArgsThunk(
        CORINFO_SIG_INFO       *pSig,
        CorInfoHelperTailCallSpecialHandling flags
        ) {
        printf("getTailCallCopyArgsThunk\r\n");
        return nullptr;
    }

    /* ICorStaticInfo */
    virtual bool getSystemVAmd64PassStructInRegisterDescriptor(
        /* IN */    CORINFO_CLASS_HANDLE        structHnd,
        /* OUT */   SYSTEMV_AMD64_CORINFO_STRUCT_REG_PASSING_DESCRIPTOR* structPassInRegDescPtr
        ) {
        assert(false);
        return false;
    }

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    virtual DWORD getMethodAttribs(
        CORINFO_METHOD_HANDLE       ftn         /* IN */
        ) {
        //printf("getMethodAttribs\r\n");
        auto method = (BaseMethod*)ftn;
        return method->get_method_attrs();
    }

    // sets private JIT flags, which can be, retrieved using getAttrib.
    virtual void setMethodAttribs(
        CORINFO_METHOD_HANDLE       ftn,        /* IN */
        CorInfoMethodRuntimeFlags   attribs     /* IN */
        ) {
        //printf("setMethodAttribs\r\n");
    }

    // Given a method descriptor ftnHnd, extract signature information into sigInfo
    //
    // 'memberParent' is typically only set when verifying.  It should be the
    // result of calling getMemberParent.
    virtual void getMethodSig(
        CORINFO_METHOD_HANDLE      ftn,        /* IN  */
        CORINFO_SIG_INFO          *sig,        /* OUT */
        CORINFO_CLASS_HANDLE      memberParent = NULL /* IN */
        ) {
        BaseMethod* m = (BaseMethod*)ftn;
        //printf("getMethodSig %p\r\n", ftn);
        m->findSig(sig);
    }

    /*********************************************************************
    * Note the following methods can only be used on functions known
    * to be IL.  This includes the method being compiled and any method
    * that 'getMethodInfo' returns true for
    *********************************************************************/

    // return information about a method private to the implementation
    //      returns false if method is not IL, or is otherwise unavailable.
    //      This method is used to fetch data needed to inline functions
    virtual bool getMethodInfo(
        CORINFO_METHOD_HANDLE   ftn,            /* IN  */
        CORINFO_METHOD_INFO*    info            /* OUT */
        ) {
        printf("getMethodInfo\r\n"); return false;
    }

    // Decides if you have any limitations for inlining. If everything's OK, it will return
    // INLINE_PASS and will fill out pRestrictions with a mask of restrictions the caller of this
    // function must respect. If caller passes pRestrictions = NULL, if there are any restrictions
    // INLINE_FAIL will be returned
    //
    // The callerHnd must be the immediate caller (i.e. when we have a chain of inlined calls)
    //
    // The inlined method need not be verified

    virtual CorInfoInline canInline(
        CORINFO_METHOD_HANDLE       callerHnd,                  /* IN  */
        CORINFO_METHOD_HANDLE       calleeHnd,                  /* IN  */
        DWORD*                      pRestrictions               /* OUT */
        ) {
        printf("canInline\r\n");
        return INLINE_PASS;
    }

    // Reports whether or not a method can be inlined, and why.  canInline is responsible for reporting all
    // inlining results when it returns INLINE_FAIL and INLINE_NEVER.  All other results are reported by the
    // JIT.
    virtual void reportInliningDecision(CORINFO_METHOD_HANDLE inlinerHnd,
        CORINFO_METHOD_HANDLE inlineeHnd,
        CorInfoInline inlineResult,
        const char * reason) {
        //printf("reportInliningDecision\r\n");
    }


    // Returns false if the call is across security boundaries thus we cannot tailcall
    //
    // The callerHnd must be the immediate caller (i.e. when we have a chain of inlined calls)
    virtual bool canTailCall(
        CORINFO_METHOD_HANDLE   callerHnd,          /* IN */
        CORINFO_METHOD_HANDLE   declaredCalleeHnd,  /* IN */
        CORINFO_METHOD_HANDLE   exactCalleeHnd,     /* IN */
        bool fIsTailPrefix                          /* IN */
        ) {
        return FALSE;
    }

    // Reports whether or not a method can be tail called, and why.
    // canTailCall is responsible for reporting all results when it returns
    // false.  All other results are reported by the JIT.
    virtual void reportTailCallDecision(CORINFO_METHOD_HANDLE callerHnd,
        CORINFO_METHOD_HANDLE calleeHnd,
        bool fIsTailPrefix,
        CorInfoTailCall tailCallResult,
        const char * reason) {
        //printf("reportTailCallDecision\r\n");
    }

    // get individual exception handler
    virtual void getEHinfo(
        CORINFO_METHOD_HANDLE ftn,              /* IN  */
        unsigned          EHnumber,             /* IN */
        CORINFO_EH_CLAUSE* clause               /* OUT */
        ) {
        printf("getEHinfo\r\n");
    }

    // return class it belongs to
    virtual CORINFO_CLASS_HANDLE getMethodClass(
        CORINFO_METHOD_HANDLE       method
        ) {
        //printf("getMethodClass\r\n"); 
        return nullptr;
    }

    // return module it belongs to
    virtual CORINFO_MODULE_HANDLE getMethodModule(
        CORINFO_METHOD_HANDLE       method
        ) {
        printf("getMethodModule\r\n"); return nullptr;
    }

    // This function returns the offset of the specified method in the
    // vtable of it's owning class or interface.
    virtual void getMethodVTableOffset(
        CORINFO_METHOD_HANDLE       method,                 /* IN */
        unsigned*                   offsetOfIndirection,    /* OUT */
        unsigned*                   offsetAfterIndirection  /* OUT */
        ) {
        *offsetOfIndirection = 0x1234;
        *offsetAfterIndirection = 0x2468;
        printf("getMethodVTableOffset\r\n");
    }

    // If a method's attributes have (getMethodAttribs) CORINFO_FLG_INTRINSIC set,
    // getIntrinsicID() returns the intrinsic ID.
    virtual CorInfoIntrinsics getIntrinsicID(
        CORINFO_METHOD_HANDLE       method,
		bool * pMustExpand = NULL
        ) {
        printf("getIntrinsicID\r\n"); return CORINFO_INTRINSIC_Object_GetType;
    }

#ifndef RYUJIT_CTPBUILD
    // Is the given module the System.Numerics.Vectors module?
    // This defaults to false.
    virtual bool isInSIMDModule(
        CORINFO_CLASS_HANDLE        classHnd
        ) {
        return false;
    }
#endif // RYUJIT_CTPBUILD

    // return the unmanaged calling convention for a PInvoke
    virtual CorInfoUnmanagedCallConv getUnmanagedCallConv(
        CORINFO_METHOD_HANDLE       method
        ) {
        printf("getUnmanagedCallConv\r\n");
        return CORINFO_UNMANAGED_CALLCONV_C;
    }

    // return if any marshaling is required for PInvoke methods.  Note that
    // method == 0 => calli.  The call site sig is only needed for the varargs or calli case
    virtual BOOL pInvokeMarshalingRequired(
        CORINFO_METHOD_HANDLE       method,
        CORINFO_SIG_INFO*           callSiteSig
        ) {
        printf("pInvokeMarshalingRequired\r\n");
        return TRUE;
    }

    // Check constraints on method type arguments (only).
    // The parent class should be checked separately using satisfiesClassConstraints(parent).
    virtual BOOL satisfiesMethodConstraints(
        CORINFO_CLASS_HANDLE        parent, // the exact parent of the method
        CORINFO_METHOD_HANDLE       method
        ) {
        //printf("satisfiesMethodConstraints\r\n"); 
        return TRUE;
    }

    // Given a delegate target class, a target method parent class,  a  target method,
    // a delegate class, check if the method signature is compatible with the Invoke method of the delegate
    // (under the typical instantiation of any free type variables in the memberref signatures).
    virtual BOOL isCompatibleDelegate(
        CORINFO_CLASS_HANDLE        objCls,           /* type of the delegate target, if any */
        CORINFO_CLASS_HANDLE        methodParentCls,  /* exact parent of the target method, if any */
        CORINFO_METHOD_HANDLE       method,           /* (representative) target method, if any */
        CORINFO_CLASS_HANDLE        delegateCls,      /* exact type of the delegate */
        BOOL                        *pfIsOpenDelegate /* is the delegate open */
        ) {
        printf("isCompatibleDelegate\r\n");
        return TRUE;
    }

    // Determines whether the delegate creation obeys security transparency rules
    virtual BOOL isDelegateCreationAllowed(
        CORINFO_CLASS_HANDLE        delegateHnd,
        CORINFO_METHOD_HANDLE       calleeHnd
        ) {
        printf("isDelegateCreationAllowed\r\n"); return FALSE;
    }


    // Indicates if the method is an instance of the generic
    // method that passes (or has passed) verification
    virtual CorInfoInstantiationVerification isInstantiationOfVerifiedGeneric(
        CORINFO_METHOD_HANDLE   method /* IN  */
        ) {
        //printf("isInstantiationOfVerifiedGeneric\r\n"); 
        return  INSTVER_NOT_INSTANTIATION;
    }

    // Loads the constraints on a typical method definition, detecting cycles;
    // for use in verification.
    virtual void initConstraintsForVerification(
        CORINFO_METHOD_HANDLE   method, /* IN */
        BOOL *pfHasCircularClassConstraints, /* OUT */
        BOOL *pfHasCircularMethodConstraint /* OUT */
        ) {
        *pfHasCircularClassConstraints = FALSE;
        *pfHasCircularMethodConstraint = FALSE;
        //printf("initConstraintsForVerification\r\n");
    }

    // Returns enum whether the method does not require verification
    // Also see ICorModuleInfo::canSkipVerification
    virtual CorInfoCanSkipVerificationResult canSkipMethodVerification(
        CORINFO_METHOD_HANDLE       ftnHandle
        ) {
        //printf("canSkipMethodVerification\r\n"); 
        return CORINFO_VERIFICATION_CAN_SKIP;
    }

    // load and restore the method
    virtual void methodMustBeLoadedBeforeCodeIsRun(
        CORINFO_METHOD_HANDLE       method
        ) {
        printf("methodMustBeLoadedBeforeCodeIsRun\r\n");
    }

    virtual CORINFO_METHOD_HANDLE mapMethodDeclToMethodImpl(
        CORINFO_METHOD_HANDLE       method
        ) {
        printf("mapMethodDeclToMethodImpl\r\n"); return nullptr;
    }

    // Returns the global cookie for the /GS unsafe buffer checks
    // The cookie might be a constant value (JIT), or a handle to memory location (Ngen)
    virtual void getGSCookie(
        GSCookie * pCookieVal,                     // OUT
        GSCookie ** ppCookieVal                    // OUT
        ) {
        *pCookieVal = 0x1234; // TODO: Should be a secure value
        *ppCookieVal = nullptr;
        //printf("getGSCookie\r\n");
    }

#ifdef  MDIL
    virtual unsigned getNumTypeParameters(
        CORINFO_METHOD_HANDLE       method
        ) {
        printf("getNumTypeParameters\r\n"); return 0;
    }

    virtual CorElementType getTypeOfTypeParameter(
        CORINFO_METHOD_HANDLE       method,
        unsigned                    index
        ) {
        printf("getTypeOfTypeParameter\r\n");
        return 0;

    }

    virtual CORINFO_CLASS_HANDLE getTypeParameter(
        CORINFO_METHOD_HANDLE       method,
        bool                        classTypeParameter,
        unsigned                    index
        ) {
        printf("getTypeParameter\r\n"); return nullptr;
    }

    virtual unsigned getStructTypeToken(
        InlineContext              *context,
        CORINFO_ARG_LIST_HANDLE     argList
        ) {
        printf("getStructTypeToken\r\n");
        return 0;
    }

    virtual unsigned getEnclosingClassToken(
        InlineContext              *context,
        CORINFO_METHOD_HANDLE       method
        ) {
        printf("getEnclosingClassToken\r\n"); return 0;
    }

    virtual CorInfoType getFieldElementType(
        unsigned                    fieldToken,
        CORINFO_MODULE_HANDLE       scope,
        CORINFO_METHOD_HANDLE       methHnd
        ) {
        printf("getFieldElementType\r\n");
        return 0;
    }

    // tokens in inlined methods may need to be translated,
    // for example if they are in a generic method we need to fill in type parameters,
    // or in one from another module we need to translate tokens so they are valid
    // in module
    // tokens in dynamic methods (IL stubs) are always translated because
    // as generated they are not backed by any metadata

    // this is called at the start of an inline expansion
    virtual InlineContext *computeInlineContext(
        InlineContext              *outerContext,
        unsigned                    inlinedMethodToken,
        unsigned                    constraintTypeRef,
        CORINFO_METHOD_HANDLE       methHnd
        ) {
        printf("computeInlineContext\r\n");
        return nullptr;

    }

    // this does the actual translation
    virtual unsigned translateToken(
        InlineContext              *inlineContext,
        CORINFO_MODULE_HANDLE       scopeHnd,
        unsigned                    token
        ) {
        printf("translateToken\r\n");
        return 0;
    }

    virtual unsigned getCurrentMethodToken(
        InlineContext              *inlineContext,
        CORINFO_METHOD_HANDLE       method
        ) {
        printf("getCurrentMethodToken\r\n");
        return 0;
    }

    // computes flags for an IL stub method
    virtual unsigned getStubMethodFlags(
        CORINFO_METHOD_HANDLE method
        ) {
        printf("getStubMethodFlags\r\n");
        return 0;
    }
#endif


    /**********************************************************************************/
    //
    // ICorModuleInfo
    //
    /**********************************************************************************/

    // Resolve metadata token into runtime method handles.
    virtual void resolveToken(/* IN, OUT */ CORINFO_RESOLVED_TOKEN * pResolvedToken) {
        Module* mod = (Module*)pResolvedToken->tokenScope;
        BaseMethod* method = mod->ResolveMethod(pResolvedToken->token);
        pResolvedToken->hMethod = (CORINFO_METHOD_HANDLE)method;
        pResolvedToken->hClass = (CORINFO_CLASS_HANDLE)1; // this just suppresses a JIT assert
        //printf("resolveToken %d\r\n", pResolvedToken->token);
    }

#ifdef MDIL
    // Given a field or method token metaTOK return its parent token
    // we still need this in MDIL, for example for static field access we need the 
    // token of the enclosing type
    virtual unsigned getMemberParent(CORINFO_MODULE_HANDLE  scopeHnd, unsigned metaTOK) { printf("getMemberParent\r\n"); return 0; }

    // given a token representing an MD array of structs, get the element type token
    virtual unsigned getArrayElementToken(CORINFO_MODULE_HANDLE  scopeHnd, unsigned metaTOK) { printf("getArrayElementToken\r\n"); return 0; }
#endif // MDIL

    // Signature information about the call sig
    virtual void findSig(
        CORINFO_MODULE_HANDLE       module,     /* IN */
        unsigned                    sigTOK,     /* IN */
        CORINFO_CONTEXT_HANDLE      context,    /* IN */
        CORINFO_SIG_INFO           *sig         /* OUT */
        ) {
        printf("findSig %d\r\n", sigTOK);
        auto mod = (Module*)module;
        auto method = mod->ResolveMethod(sigTOK);
        method->findSig(sig);
    }

    // for Varargs, the signature at the call site may differ from
    // the signature at the definition.  Thus we need a way of
    // fetching the call site information
    virtual void findCallSiteSig(
        CORINFO_MODULE_HANDLE       module,     /* IN */
        unsigned                    methTOK,    /* IN */
        CORINFO_CONTEXT_HANDLE      context,    /* IN */
        CORINFO_SIG_INFO           *sig         /* OUT */
        ) {
        //printf("findCallSiteSig\r\n");
    }

    virtual CORINFO_CLASS_HANDLE getTokenTypeAsHandle(
        CORINFO_RESOLVED_TOKEN *    pResolvedToken /* IN  */) {
        printf("getTokenTypeAsHandle\r\n");
        return nullptr;
    }

    // Returns true if the module does not require verification
    //
    // If fQuickCheckOnlyWithoutCommit=TRUE, the function only checks that the
    // module does not currently require verification in the current AppDomain.
    // This decision could change in the future, and so should not be cached.
    // If it is cached, it should only be used as a hint.
    // This is only used by ngen for calculating certain hints.
    //

    // Returns enum whether the module does not require verification
    // Also see ICorMethodInfo::canSkipMethodVerification();
    virtual CorInfoCanSkipVerificationResult canSkipVerification(
        CORINFO_MODULE_HANDLE       module     /* IN  */
        ) {
        printf("canSkipVerification\r\n");
        return CORINFO_VERIFICATION_CAN_SKIP;
    }

    // Checks if the given metadata token is valid
    virtual BOOL isValidToken(
        CORINFO_MODULE_HANDLE       module,     /* IN  */
        unsigned                    metaTOK     /* IN  */
        ) {
        printf("isValidToken\r\n"); return TRUE;
    }

    // Checks if the given metadata token is valid StringRef
    virtual BOOL isValidStringRef(
        CORINFO_MODULE_HANDLE       module,     /* IN  */
        unsigned                    metaTOK     /* IN  */
        ) {
        printf("isValidStringRef\r\n");
        return TRUE;
    }

    virtual BOOL shouldEnforceCallvirtRestriction(
        CORINFO_MODULE_HANDLE   scope
        ) {
        printf("shouldEnforceCallvirtRestriction\r\n"); return FALSE;
    }
#ifdef  MDIL
    virtual unsigned getTypeTokenForFieldOrMethod(
        unsigned                fieldOrMethodToken
        ) {
        printf("getTypeTokenForFieldOrMethod\r\n"); return 0;
    }

    virtual unsigned getTokenForType(CORINFO_CLASS_HANDLE  cls) { printf("getTokenForType\r\n"); return 0; }
#endif

    /**********************************************************************************/
    //
    // ICorClassInfo
    //
    /**********************************************************************************/

    // If the value class 'cls' is isomorphic to a primitive type it will
    // return that type, otherwise it will return CORINFO_TYPE_VALUECLASS
    virtual CorInfoType asCorInfoType(
        CORINFO_CLASS_HANDLE    cls
        ) {
        printf("asCorInfoType\r\n");
        return CORINFO_TYPE_UNDEF;
    }

    // for completeness
    virtual const char* getClassName(
        CORINFO_CLASS_HANDLE    cls
        ) {
        //printf("getClassName\r\n");
        return NULL;

    }


    // Append a (possibly truncated) representation of the type cls to the preallocated buffer ppBuf of length pnBufLen
    // If fNamespace=TRUE, include the namespace/enclosing classes
    // If fFullInst=TRUE (regardless of fNamespace and fAssembly), include namespace and assembly for any type parameters
    // If fAssembly=TRUE, suffix with a comma and the full assembly qualification
    // return size of representation
    virtual int appendClassName(
        __deref_inout_ecount(*pnBufLen) WCHAR** ppBuf,
        int* pnBufLen,
        CORINFO_CLASS_HANDLE    cls,
        BOOL fNamespace,
        BOOL fFullInst,
        BOOL fAssembly
        ) {
        printf("appendClassName\r\n"); return 0;
    }

    // Quick check whether the type is a value class. Returns the same value as getClassAttribs(cls) & CORINFO_FLG_VALUECLASS, except faster.
    virtual BOOL isValueClass(CORINFO_CLASS_HANDLE cls) { printf("isValueClass\r\n"); return FALSE; }

    // If this method returns true, JIT will do optimization to inline the check for
    //     GetTypeFromHandle(handle) == obj.GetType()
    virtual BOOL canInlineTypeCheckWithObjectVTable(CORINFO_CLASS_HANDLE cls) {
        printf("canInlineTypeCheckWithObjectVTable\r\n");
        return FALSE;
    }

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    virtual DWORD getClassAttribs(
        CORINFO_CLASS_HANDLE    cls
        ) {
        //printf("getClassAttribs\r\n");
        return 0;
    }

    // Returns "TRUE" iff "cls" is a struct type such that return buffers used for returning a value
    // of this type must be stack-allocated.  This will generally be true only if the struct 
    // contains GC pointers, and does not exceed some size limit.  Maintaining this as an invariant allows
    // an optimization: the JIT may assume that return buffer pointers for return types for which this predicate
    // returns TRUE are always stack allocated, and thus, that stores to the GC-pointer fields of such return
    // buffers do not require GC write barriers.
    virtual BOOL isStructRequiringStackAllocRetBuf(CORINFO_CLASS_HANDLE cls) {
        printf("isStructRequiringStackAllocRetBuf\r\n");
        return FALSE;
    }

    virtual CORINFO_MODULE_HANDLE getClassModule(
        CORINFO_CLASS_HANDLE    cls
        ) {
        printf("getClassModule\r\n");
        return nullptr;
    }

    // Returns the assembly that contains the module "mod".
    virtual CORINFO_ASSEMBLY_HANDLE getModuleAssembly(
        CORINFO_MODULE_HANDLE   mod
        ) {
        printf("getModuleAssembly\r\n");
        return nullptr;
    }

    // Returns the name of the assembly "assem".
    virtual const char* getAssemblyName(
        CORINFO_ASSEMBLY_HANDLE assem
        ) {
        printf("getAssemblyName\r\n");
        return nullptr;
    }

    // Allocate and delete process-lifetime objects.  Should only be
    // referred to from static fields, lest a leak occur.
    // Note that "LongLifetimeFree" does not execute destructors, if "obj"
    // is an array of a struct type with a destructor.
    virtual void* LongLifetimeMalloc(size_t sz) { printf("LongLifetimeMalloc\r\n"); return nullptr; }
    virtual void LongLifetimeFree(void* obj) {
        printf("LongLifetimeFree\r\n");
    }

    virtual size_t getClassModuleIdForStatics(
        CORINFO_CLASS_HANDLE    cls,
        CORINFO_MODULE_HANDLE *pModule,
        void **ppIndirection
        ) {
        printf("getClassModuleIdForStatics\r\n");
        return 0;
    }

    // return the number of bytes needed by an instance of the class
    virtual unsigned getClassSize(
        CORINFO_CLASS_HANDLE        cls
        ) {
        printf("getClassSize\r\n");
        return 0;
    }

    virtual unsigned getClassAlignmentRequirement(
        CORINFO_CLASS_HANDLE        cls,
        BOOL                        fDoubleAlignHint = FALSE
        ) {
        printf("getClassAlignmentRequirement\r\n");
        return 0;
    }

    // This is only called for Value classes.  It returns a boolean array
    // in representing of 'cls' from a GC perspective.  The class is
    // assumed to be an array of machine words
    // (of length // getClassSize(cls) / sizeof(void*)),
    // 'gcPtrs' is a poitner to an array of BYTEs of this length.
    // getClassGClayout fills in this array so that gcPtrs[i] is set
    // to one of the CorInfoGCType values which is the GC type of
    // the i-th machine word of an object of type 'cls'
    // returns the number of GC pointers in the array
    virtual unsigned getClassGClayout(
        CORINFO_CLASS_HANDLE        cls,        /* IN */
        BYTE                       *gcPtrs      /* OUT */
        ) {
        printf("getClassGClayout\r\n");
        return 0;
    }

    // returns the number of instance fields in a class
    virtual unsigned getClassNumInstanceFields(
        CORINFO_CLASS_HANDLE        cls        /* IN */
        ) {
        printf("getClassNumInstanceFields\r\n");
        return 0;
    }

    virtual CORINFO_FIELD_HANDLE getFieldInClass(
        CORINFO_CLASS_HANDLE clsHnd,
        INT num
        ) {
        printf("getFieldInClass\r\n");
        return nullptr;
    }

    virtual BOOL checkMethodModifier(
        CORINFO_METHOD_HANDLE hMethod,
        LPCSTR modifier,
        BOOL fOptional
        ) {
        printf("checkMethodModifier\r\n");
        return FALSE;
    }

    // returns the "NEW" helper optimized for "newCls."
    virtual CorInfoHelpFunc getNewHelper(
        CORINFO_RESOLVED_TOKEN * pResolvedToken,
        CORINFO_METHOD_HANDLE    callerHandle
        ) {
        printf("getNewHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

    // returns the newArr (1-Dim array) helper optimized for "arrayCls."
    virtual CorInfoHelpFunc getNewArrHelper(
        CORINFO_CLASS_HANDLE        arrayCls
        ) {
        printf("getNewArrHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

    // returns the optimized "IsInstanceOf" or "ChkCast" helper
    virtual CorInfoHelpFunc getCastingHelper(
        CORINFO_RESOLVED_TOKEN * pResolvedToken,
        bool fThrowing
        ) {
        printf("getCastingHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

    // returns helper to trigger static constructor
    virtual CorInfoHelpFunc getSharedCCtorHelper(
        CORINFO_CLASS_HANDLE clsHnd
        ) {
        printf("getSharedCCtorHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

    virtual CorInfoHelpFunc getSecurityPrologHelper(
        CORINFO_METHOD_HANDLE   ftn
        ) {
        printf("getSecurityPrologHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

    // This is not pretty.  Boxing nullable<T> actually returns
    // a boxed<T> not a boxed Nullable<T>.  This call allows the verifier
    // to call back to the EE on the 'box' instruction and get the transformed
    // type to use for verification.
    virtual CORINFO_CLASS_HANDLE  getTypeForBox(
        CORINFO_CLASS_HANDLE        cls
        ) {
        printf("getTypeForBox\r\n");
        return nullptr;
    }

    // returns the correct box helper for a particular class.  Note
    // that if this returns CORINFO_HELP_BOX, the JIT can assume 
    // 'standard' boxing (allocate object and copy), and optimize
    virtual CorInfoHelpFunc getBoxHelper(
        CORINFO_CLASS_HANDLE        cls
        ) {
        printf("getBoxHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

    // returns the unbox helper.  If 'helperCopies' points to a true 
    // value it means the JIT is requesting a helper that unboxes the
    // value into a particular location and thus has the signature
    //     void unboxHelper(void* dest, CORINFO_CLASS_HANDLE cls, Object* obj)
    // Otherwise (it is null or points at a FALSE value) it is requesting 
    // a helper that returns a poitner to the unboxed data 
    //     void* unboxHelper(CORINFO_CLASS_HANDLE cls, Object* obj)
    // The EE has the option of NOT returning the copy style helper
    // (But must be able to always honor the non-copy style helper)
    // The EE set 'helperCopies' on return to indicate what kind of
    // helper has been created.  

    virtual CorInfoHelpFunc getUnBoxHelper(
        CORINFO_CLASS_HANDLE        cls
        ) {
        printf("getUnBoxHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

#ifndef RYUJIT_CTPBUILD
    virtual void getReadyToRunHelper(
        CORINFO_RESOLVED_TOKEN * pResolvedToken,
        CorInfoHelpFunc          id,
        CORINFO_CONST_LOOKUP *   pLookup
        ) {
        printf("getReadyToRunHelper\r\n");
    }
#endif

    virtual const char* getHelperName(
        CorInfoHelpFunc
        ) {
        printf("getHelperName\r\n");
        return nullptr;
    }

    // This function tries to initialize the class (run the class constructor).
    // this function returns whether the JIT must insert helper calls before 
    // accessing static field or method.
    //
    // See code:ICorClassInfo#ClassConstruction.
    virtual CorInfoInitClassResult initClass(
        CORINFO_FIELD_HANDLE    field,          // Non-NULL - inquire about cctor trigger before static field access
        // NULL - inquire about cctor trigger in method prolog
        CORINFO_METHOD_HANDLE   method,         // Method referencing the field or prolog
        CORINFO_CONTEXT_HANDLE  context,        // Exact context of method
        BOOL                    speculative = FALSE     // TRUE means don't actually run it
        ) {
        //printf("initClass\r\n");
        return CORINFO_INITCLASS_NOT_REQUIRED;
    }

    // This used to be called "loadClass".  This records the fact
    // that the class must be loaded (including restored if necessary) before we execute the
    // code that we are currently generating.  When jitting code
    // the function loads the class immediately.  When zapping code
    // the zapper will if necessary use the call to record the fact that we have
    // to do a fixup/restore before running the method currently being generated.
    //
    // This is typically used to ensure value types are loaded before zapped
    // code that manipulates them is executed, so that the GC can access information
    // about those value types.
    virtual void classMustBeLoadedBeforeCodeIsRun(
        CORINFO_CLASS_HANDLE        cls
        ) {
        //printf("classMustBeLoadedBeforeCodeIsRun\r\n");
    }

    // returns the class handle for the special builtin classes
    virtual CORINFO_CLASS_HANDLE getBuiltinClass(
        CorInfoClassId              classId
        ) {
        printf("getBuiltinClass\r\n");
        return NULL;
    }

    // "System.Int32" ==> CORINFO_TYPE_INT..
    virtual CorInfoType getTypeForPrimitiveValueClass(
        CORINFO_CLASS_HANDLE        cls
        ) {
        printf("getTypeForPrimitiveValueClass\r\n");
        return CORINFO_TYPE_UNDEF;
    }

    // TRUE if child is a subtype of parent
    // if parent is an interface, then does child implement / extend parent
    virtual BOOL canCast(
        CORINFO_CLASS_HANDLE        child,  // subtype (extends parent)
        CORINFO_CLASS_HANDLE        parent  // base type
        ) {
        printf("canCast\r\n");
        return TRUE;
    }

    // TRUE if cls1 and cls2 are considered equivalent types.
    virtual BOOL areTypesEquivalent(
        CORINFO_CLASS_HANDLE        cls1,
        CORINFO_CLASS_HANDLE        cls2
        ) {
        printf("areTypesEquivalent\r\n");
        return FALSE;
    }

    // returns is the intersection of cls1 and cls2.
    virtual CORINFO_CLASS_HANDLE mergeClasses(
        CORINFO_CLASS_HANDLE        cls1,
        CORINFO_CLASS_HANDLE        cls2
        ) {
        printf("mergeClasses\r\n");
        return nullptr;
    }

    // Given a class handle, returns the Parent type.
    // For COMObjectType, it returns Class Handle of System.Object.
    // Returns 0 if System.Object is passed in.
    virtual CORINFO_CLASS_HANDLE getParentType(
        CORINFO_CLASS_HANDLE        cls
        ) {
        printf("getParentType\r\n");
        return NULL;
    }

    // Returns the CorInfoType of the "child type". If the child type is
    // not a primitive type, *clsRet will be set.
    // Given an Array of Type Foo, returns Foo.
    // Given BYREF Foo, returns Foo
    virtual CorInfoType getChildType(
        CORINFO_CLASS_HANDLE       clsHnd,
        CORINFO_CLASS_HANDLE       *clsRet
        ) {
        printf("getChildType\r\n");
        return CORINFO_TYPE_UNDEF;
    }

    // Check constraints on type arguments of this class and parent classes
    virtual BOOL satisfiesClassConstraints(
        CORINFO_CLASS_HANDLE cls
        ) {
        //printf("satisfiesClassConstraints\r\n");
        return TRUE;
    }

    // Check if this is a single dimensional array type
    virtual BOOL isSDArray(
        CORINFO_CLASS_HANDLE        cls
        ) {
        printf("isSDArray\r\n");
        return TRUE;
    }

    // Get the numbmer of dimensions in an array 
    virtual unsigned getArrayRank(
        CORINFO_CLASS_HANDLE        cls
        ) {
        printf("getArrayRank\r\n");
        return 0;
    }

    // Get static field data for an array
    virtual void * getArrayInitializationData(
        CORINFO_FIELD_HANDLE        field,
        DWORD                       size
        ) {
        printf("getArrayInitializationData\r\n");
        return NULL;
    }

    // Check Visibility rules.
    virtual CorInfoIsAccessAllowedResult canAccessClass(
        CORINFO_RESOLVED_TOKEN * pResolvedToken,
        CORINFO_METHOD_HANDLE   callerHandle,
        CORINFO_HELPER_DESC    *pAccessHelper /* If canAccessMethod returns something other
                                              than ALLOWED, then this is filled in. */
        ) {
        printf("canAccessClass\r\n");
        return CORINFO_ACCESS_ALLOWED;
    }

    /**********************************************************************************/
    //
    // ICorFieldInfo
    //
    /**********************************************************************************/

    // this function is for debugging only.  It returns the field name
    // and if 'moduleName' is non-null, it sets it to something that will
    // says which method (a class name, or a module name)
    virtual const char* getFieldName(
        CORINFO_FIELD_HANDLE        ftn,        /* IN */
        const char                **moduleName  /* OUT */
        ) {
        printf("getFieldName\r\n");
        return NULL;
    }

    // return class it belongs to
    virtual CORINFO_CLASS_HANDLE getFieldClass(
        CORINFO_FIELD_HANDLE    field
        ) {
        printf("getFieldClass\r\n");
        return 0;
    }

    // Return the field's type, if it is CORINFO_TYPE_VALUECLASS 'structType' is set
    // the field's value class (if 'structType' == 0, then don't bother
    // the structure info).
    //
    // 'memberParent' is typically only set when verifying.  It should be the
    // result of calling getMemberParent.
    virtual CorInfoType getFieldType(
        CORINFO_FIELD_HANDLE    field,
        CORINFO_CLASS_HANDLE   *structType,
        CORINFO_CLASS_HANDLE    memberParent = NULL /* IN */
        ) {
        printf("getFieldType\r\n");
        return CORINFO_TYPE_UNDEF;
    }

    // return the data member's instance offset
    virtual unsigned getFieldOffset(
        CORINFO_FIELD_HANDLE    field
        ) {
        printf("getFieldOffset\r\n");
        return 0;
    }

    // TODO: jit64 should be switched to the same plan as the i386 jits - use
    // getClassGClayout to figure out the need for writebarrier helper, and inline the copying.
    // The interpretted value class copy is slow. Once this happens, USE_WRITE_BARRIER_HELPERS
    virtual bool isWriteBarrierHelperRequired(
        CORINFO_FIELD_HANDLE    field) {
        printf("isWriteBarrierHelperRequired\r\n");
        return false;
    }

    virtual void getFieldInfo(CORINFO_RESOLVED_TOKEN * pResolvedToken,
        CORINFO_METHOD_HANDLE  callerHandle,
        CORINFO_ACCESS_FLAGS   flags,
        CORINFO_FIELD_INFO    *pResult
        ) {
        printf("\r\n");
    }
#ifdef MDIL
    virtual DWORD getFieldOrdinal(CORINFO_MODULE_HANDLE  tokenScope,
        unsigned               fieldToken) {
        printf("getFieldInfo\r\n");
    }
#endif

    // Returns true iff "fldHnd" represents a static field.
    virtual bool isFieldStatic(CORINFO_FIELD_HANDLE fldHnd) {
        printf("isFieldStatic\r\n");
        return FALSE;
    }

    /*********************************************************************************/
    //
    // ICorDebugInfo
    //
    /*********************************************************************************/

    // Query the EE to find out where interesting break points
    // in the code are.  The native compiler will ensure that these places
    // have a corresponding break point in native code.
    //
    // Note that unless CORJIT_FLG_DEBUG_CODE is specified, this function will
    // be used only as a hint and the native compiler should not change its
    // code generation.
    virtual void getBoundaries(
        CORINFO_METHOD_HANDLE   ftn,                // [IN] method of interest
        unsigned int           *cILOffsets,         // [OUT] size of pILOffsets
        DWORD                 **pILOffsets,         // [OUT] IL offsets of interest
        //       jit MUST free with freeArray!
        ICorDebugInfo::BoundaryTypes *implictBoundaries // [OUT] tell jit, all boundries of this type
        ) {
        printf("getBoundaries\r\n");
    }

    // Report back the mapping from IL to native code,
    // this map should include all boundaries that 'getBoundaries'
    // reported as interesting to the debugger.

    // Note that debugger (and profiler) is assuming that all of the
    // offsets form a contiguous block of memory, and that the
    // OffsetMapping is sorted in order of increasing native offset.
    virtual void setBoundaries(
        CORINFO_METHOD_HANDLE   ftn,            // [IN] method of interest
        ULONG32                 cMap,           // [IN] size of pMap
        ICorDebugInfo::OffsetMapping *pMap      // [IN] map including all points of interest.
        //      jit allocated with allocateArray, EE frees
        ) {
        printf("setBoundaries\r\n");
    }

    // Query the EE to find out the scope of local varables.
    // normally the JIT would trash variables after last use, but
    // under debugging, the JIT needs to keep them live over their
    // entire scope so that they can be inspected.
    //
    // Note that unless CORJIT_FLG_DEBUG_CODE is specified, this function will
    // be used only as a hint and the native compiler should not change its
    // code generation.
    virtual void getVars(
        CORINFO_METHOD_HANDLE           ftn,            // [IN]  method of interest
        ULONG32                        *cVars,          // [OUT] size of 'vars'
        ICorDebugInfo::ILVarInfo       **vars,          // [OUT] scopes of variables of interest
        //       jit MUST free with freeArray!
        bool                           *extendOthers    // [OUT] it TRUE, then assume the scope
        //       of unmentioned vars is entire method
        ) {
        printf("getVars\r\n");
    }

    // Report back to the EE the location of every variable.
    // note that the JIT might split lifetimes into different
    // locations etc.

    virtual void setVars(
        CORINFO_METHOD_HANDLE           ftn,            // [IN] method of interest
        ULONG32                         cVars,          // [IN] size of 'vars'
        ICorDebugInfo::NativeVarInfo   *vars            // [IN] map telling where local vars are stored at what points
        //      jit allocated with allocateArray, EE frees
        ) {
        printf("setVars\r\n");
    }

    /*-------------------------- Misc ---------------------------------------*/

    // Used to allocate memory that needs to handed to the EE.
    // For eg, use this to allocated memory for reporting debug info,
    // which will be handed to the EE by setVars() and setBoundaries()
    virtual void * allocateArray(
        ULONG              cBytes
        ) {
        printf("\r\n");
        return nullptr;
    }

    // JitCompiler will free arrays passed by the EE using this
    // For eg, The EE returns memory in getVars() and getBoundaries()
    // to the JitCompiler, which the JitCompiler should release using
    // freeArray()
    virtual void freeArray(
        void               *array
        ) {
        printf("freeArray\r\n");
    }

    /*********************************************************************************/
    //
    // ICorArgInfo
    //
    /*********************************************************************************/

    // advance the pointer to the argument list.
    // a ptr of 0, is special and always means the first argument
    virtual CORINFO_ARG_LIST_HANDLE getArgNext(
        CORINFO_ARG_LIST_HANDLE     args            /* IN */
        ) {
        //printf("getArgNext %p\r\n", args);
        return (CORINFO_ARG_LIST_HANDLE)(((Parameter*)args) + 1);
    }

    // Get the type of a particular argument
    // CORINFO_TYPE_UNDEF is returned when there are no more arguments
    // If the type returned is a primitive type (or an enum) *vcTypeRet set to NULL
    // otherwise it is set to the TypeHandle associted with the type
    // Enumerations will always look their underlying type (probably should fix this)
    // Otherwise vcTypeRet is the type as would be seen by the IL,
    // The return value is the type that is used for calling convention purposes
    // (Thus if the EE wants a value class to be passed like an int, then it will
    // return CORINFO_TYPE_INT
    virtual CorInfoTypeWithMod getArgType(
        CORINFO_SIG_INFO*           sig,            /* IN */
        CORINFO_ARG_LIST_HANDLE     args,           /* IN */
        CORINFO_CLASS_HANDLE       *vcTypeRet       /* OUT */
        ) {
        //printf("getArgType %p\r\n", args);
        *vcTypeRet = nullptr;
        return (CorInfoTypeWithMod)((Parameter*)args)->m_type;
    }

    // If the Arg is a CORINFO_TYPE_CLASS fetch the class handle associated with it
    virtual CORINFO_CLASS_HANDLE getArgClass(
        CORINFO_SIG_INFO*           sig,            /* IN */
        CORINFO_ARG_LIST_HANDLE     args            /* IN */
        ) {
        //printf("getArgClass\r\n");
        return NULL;
    }

    // Returns type of HFA for valuetype
    virtual CorInfoType getHFAType(
        CORINFO_CLASS_HANDLE hClass
        ) {
        printf("getHFAType\r\n");
        return CORINFO_TYPE_UNDEF;
    }

    /*****************************************************************************
    * ICorErrorInfo contains methods to deal with SEH exceptions being thrown
    * from the corinfo interface.  These methods may be called when an exception
    * with code EXCEPTION_COMPLUS is caught.
    *****************************************************************************/

    // Returns the HRESULT of the current exception
    virtual HRESULT GetErrorHRESULT(
    struct _EXCEPTION_POINTERS *pExceptionPointers
        ) {
        printf("GetErrorHRESULT\r\n");
        return E_FAIL;

    }

    // Fetches the message of the current exception
    // Returns the size of the message (including terminating null). This can be
    // greater than bufferLength if the buffer is insufficient.
    virtual ULONG GetErrorMessage(
        __inout_ecount(bufferLength) LPWSTR buffer,
        ULONG bufferLength
        ) {
        printf("GetErrorMessage\r\n");
        return 0;
    }

    // returns EXCEPTION_EXECUTE_HANDLER if it is OK for the compile to handle the
    //                        exception, abort some work (like the inlining) and continue compilation
    // returns EXCEPTION_CONTINUE_SEARCH if exception must always be handled by the EE
    //                    things like ThreadStoppedException ...
    // returns EXCEPTION_CONTINUE_EXECUTION if exception is fixed up by the EE

    virtual int FilterException(
    struct _EXCEPTION_POINTERS *pExceptionPointers
        ) {
        printf("FilterException\r\n"); return 0;
    }

    // Cleans up internal EE tracking when an exception is caught.
    virtual void HandleException(
    struct _EXCEPTION_POINTERS *pExceptionPointers
        ) {
        printf("HandleException\r\n");
    }

    virtual void ThrowExceptionForJitResult(
        HRESULT result) {
        printf("ThrowExceptionForJitResult\r\n");
    }

    //Throws an exception defined by the given throw helper.
    virtual void ThrowExceptionForHelper(
        const CORINFO_HELPER_DESC * throwHelper) {
        printf("ThrowExceptionForHelper\r\n");
    }

    /*****************************************************************************
    * ICorStaticInfo contains EE interface methods which return values that are
    * constant from invocation to invocation.  Thus they may be embedded in
    * persisted information like statically generated code. (This is of course
    * assuming that all code versions are identical each time.)
    *****************************************************************************/

    // Return details about EE internal data structures
    virtual void getEEInfo(
        CORINFO_EE_INFO            *pEEInfoOut
        ) {
        printf("getEEInfo\r\n");
        memset(pEEInfoOut, 0, sizeof(CORINFO_EE_INFO));
        pEEInfoOut->inlinedCallFrameInfo.size = 4;

    }

    // Returns name of the JIT timer log
    virtual LPCWSTR getJitTimeLogFilename() {
        printf("getJitTimeLogFilename\r\n");
        return NULL;

    }

#ifdef RYUJIT_CTPBUILD
    // Logs a SQM event for a JITting a very large method.
    virtual void logSQMLongJitEvent(unsigned mcycles, unsigned msec, unsigned ilSize, unsigned numBasicBlocks, bool minOpts,
        CORINFO_METHOD_HANDLE methodHnd) {
        printf("logSQMLongJitEvent\r\n");
    }
#endif // RYUJIT_CTPBUILD

    /*********************************************************************************/
    //
    // Diagnostic methods
    //
    /*********************************************************************************/

    // this function is for debugging only. Returns method token.
    // Returns mdMethodDefNil for dynamic methods.
    virtual mdMethodDef getMethodDefFromMethod(
        CORINFO_METHOD_HANDLE hMethod
        ) {
        printf("getMethodDefFromMethod\r\n");
        return 0;
    }

    // this function is for debugging only.  It returns the method name
    // and if 'moduleName' is non-null, it sets it to something that will
    // says which method (a class name, or a module name)
    virtual const char* getMethodName(
        CORINFO_METHOD_HANDLE       ftn,        /* IN */
        const char                **moduleName  /* OUT */
        ) {
        *moduleName = "foo";
        //printf("getMethodName %p\r\n", ftn);
        return "bar";
    }

    // this function is for debugging only.  It returns a value that
    // is will always be the same for a given method.  It is used
    // to implement the 'jitRange' functionality
    virtual unsigned getMethodHash(
        CORINFO_METHOD_HANDLE       ftn         /* IN */
        ) {
        return 0;
    }

    // this function is for debugging only.
    virtual size_t findNameOfToken(
        CORINFO_MODULE_HANDLE       module,     /* IN  */
        mdToken                     metaTOK,     /* IN  */
        __out_ecount(FQNameCapacity) char * szFQName, /* OUT */
        size_t FQNameCapacity  /* IN */
        ) {
        printf("findNameOfToken\r\n");
        return 0;

    }

#if !defined(RYUJIT_CTPBUILD)
    /*************************************************************************/
    //
    // Configuration values - Allows querying of the CLR configuration.
    //
    /*************************************************************************/

    //  Return an integer ConfigValue if any.
    //
    virtual int getIntConfigValue(
        const wchar_t *name,
        int defaultValue
        ) {
        printf("getIntConfigValue\r\n");
        return 0;
    }

    //  Return a string ConfigValue if any.
    //
    virtual wchar_t *getStringConfigValue(
        const wchar_t *name
        ) {
        printf("getStringConfigValue\r\n");
        return NULL;
    }

    // Free a string ConfigValue returned by the runtime.
    // JITs using the getStringConfigValue query are required
    // to return the string values to the runtime for deletion.
    // this avoid leaking the memory in the JIT.
    virtual void freeStringConfigValue(
        wchar_t *value
        ) {
        printf("freeStringConfigValue\r\n");
    }
#endif // RYUJIT_CTPBUILD


	void CorJitInfo::getAddressOfPInvokeTarget(CORINFO_METHOD_HANDLE method, CORINFO_CONST_LOOKUP * pLookup)
	{
	}

	DWORD CorJitInfo::getJitFlags(CORJIT_FLAGS * flags, DWORD sizeInBytes)
	{
		return 0;
	}

};

#endif
