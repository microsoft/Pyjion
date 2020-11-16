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

#ifndef PYJION_JITINFO_H
#define PYJION_JITINFO_H


#define FEATURE_NO_HOST

#include <Python.h>
#include <frameobject.h>
#include <opcode.h>
#include <windows.h>
#include <share.h>
#include <intrin.h>

#include <vector>
#include <unordered_map>
#include <corjit.h>

#include <openum.h>

#include "codemodel.h"
#include "cee.h"
#include "ipycomp.h"

#ifndef WINDOWS
#include <sys/mman.h>
#endif

using namespace std;

#ifdef WINDOWS
extern "C" void JIT_StackProbe(); // Implemented in helpers.asm
#endif

const CORINFO_CLASS_HANDLE PYOBJECT_PTR_TYPE = (CORINFO_CLASS_HANDLE)0x11;

class CorJitInfo : public ICorJitInfo, public JittedCode {
    void* m_codeAddr;
    void* m_dataAddr;
    PyCodeObject *m_code;
    UserModule* m_module;
    uint8_t* m_il;
    unsigned int m_ilLen;
    ULONG m_nativeSize;
#ifdef WINDOWS
    HANDLE m_winHeap;
#endif

public:

    CorJitInfo(PyCodeObject* code, UserModule* module) {
        m_codeAddr = m_dataAddr = nullptr;
        m_code = code;
        m_module = module;
        m_il = nullptr;
        m_ilLen = 0;
#ifdef WINDOWS
        m_winHeap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
#endif
    }

    ~CorJitInfo() override {
        if (m_codeAddr != nullptr) {
            freeMem(m_codeAddr);
        }
        if (m_dataAddr != nullptr) {
            free(m_dataAddr);
        }
#ifdef WINDOWS
        HeapDestroy(m_winHeap);
#endif
        delete m_module;
    }

    /// Empty helper function given to the JIT as the entry-point callback. Never used
    static void helperFtn() {};

    /// Empty breakpoint function, put some bonus code in here if you want to debug anything between
    /// CPython opcodes.
    static void breakpointFtn() {};

    /// Override the default .NET CIL_NEWARR with a custom array allocator. See getHelperFtn
    /// \param size Requested array size
    /// \param arrayMT Array type handle
    /// \return new vector
    static vector<PyObject*> newArrayHelperFtn(INT_PTR size, CORINFO_CLASS_HANDLE arrayMT) {
        return std::vector<PyObject*>(size);
    }

    static void stArrayHelperFtn(std::vector<PyObject*>* array, INT_PTR idx, PyObject* ref) {
        // TODO : Implement vector allocation and assignment logic for CIL_STELEM.x
    }

    void* get_code_addr() override {
        return m_codeAddr;
    }

    uint8_t* get_il() override {
        return m_il;
    }

    unsigned int get_il_len() override {
        return m_ilLen;
    }

    unsigned long get_native_size() override {
        return m_nativeSize;
    }

    void freeMem(PVOID code) {
        // TODO: Validate
        PyMem_Free(code);
    }

    void allocMem(
        ULONG               hotCodeSize,    /* IN */
        ULONG               coldCodeSize,   /* IN */
        ULONG               roDataSize,     /* IN */
        ULONG               xcptnsCount,    /* IN */
        CorJitAllocMemFlag  flag,           /* IN */
        void **             hotCodeBlock,   /* OUT */
        void **             coldCodeBlock,  /* OUT */
        void **             roDataBlock     /* OUT */
        ) override {
        // NB: Not honouring flag alignment requested in <flag>, but it is "optional"
#ifdef WINDOWS
        *hotCodeBlock = m_codeAddr = HeapAlloc(m_winHeap, 0 , hotCodeSize);
        assert(hotCodeBlock != nullptr);
#else
#if defined(__APPLE__) && defined(MAP_JIT)
        const int mode = MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT;
#elif defined(MAP_ANONYMOUS)
        const int mode = MAP_PRIVATE | MAP_ANONYMOUS;
#elif defined(MAP_ANON)
		const int mode = MAP_PRIVATE | MAP_ANON;
#else
		#error "not supported"
#endif
        *hotCodeBlock = m_codeAddr = mmap(
                nullptr,
                hotCodeSize,
                PROT_READ | PROT_WRITE | PROT_EXEC,
                mode,
                -1,
                0);
        assert (hotCodeBlock != MAP_FAILED);
#endif

        if (coldCodeSize>0) // PyMem_Malloc passes with 0 but it confuses the JIT
            *coldCodeBlock = PyMem_Malloc(coldCodeSize);
        if (roDataSize>0) // Same as above
            *roDataBlock = PyMem_Malloc(roDataSize);
    }

    BOOL logMsg(unsigned level, const char* fmt, va_list args) override {
#ifdef DUMP_TRACES
        vprintf(fmt, args);
        return FALSE;
#else
        return TRUE;
#endif

    }

    int doAssert(const char* szFile, int iLine, const char* szExpr) override {
        printf(".NET failed assertion: %s %d\n", szFile, iLine);
        // TODO : Use native warnings when it doesn't cause a recursive error.
        // PyErr_WarnFormat(PyExc_RuntimeWarning, 1, ".NET failed assertion: %s %d", szFile, iLine);
        return 1;
    }

    void reportFatalError(CorJitResult result) override {
#ifdef DEBUG
        printf("Fatal error from .NET JIT %X\r\n", result);
#endif
        // TODO : Enable when successful
        // PyErr_Format(PyExc_ValueError, "Fatal error from .NET JIT %X\r\n", result);
    }

    void recordRelocation(
        void *                 location,   /* IN  */
        void *                 target,     /* IN  */
        WORD                   fRelocType, /* IN  */
        WORD                   slotNum,  /* IN  */
        INT32                  addlDelta /* IN  */
        ) override {
        switch (fRelocType) {
            case IMAGE_REL_BASED_DIR64:
                *((UINT64 *)((BYTE *)location + slotNum)) = (UINT64)target;
                break;
#ifdef _TARGET_AMD64_
            case IMAGE_REL_BASED_REL32:
            {
                target = (BYTE *)target + addlDelta;

                auto * fixupLocation = (INT32 *)((BYTE *)location + slotNum);
                BYTE * baseAddr = (BYTE *)fixupLocation + sizeof(INT32);

                auto delta = (INT64)((BYTE *)target - baseAddr);

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

    WORD getRelocTypeHint(void * target) override {
        return -1;
    }

    // For what machine does the VM expect the JIT to generate code? The VM
    // returns one of the IMAGE_FILE_MACHINE_* values. Note that if the VM
    // is cross-compiling (such as the case for crossgen), it will return a
    // different value than if it was compiling for the host architecture.
    // 
    DWORD getExpectedTargetArchitecture() override {
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

    DWORD getThreadTLSIndex(
        void                  **ppIndirection
        ) override {
        printf("getThreadTLSIndex  not implemented\r\n");
        return 0;
    }

    const void * getInlinedCallFrameVptr(
        void                  **ppIndirection
        ) override {
        printf("getInlinedCallFrameVptr  not implemented\r\n");
        return nullptr;
    }

    LONG * getAddrOfCaptureThreadGlobal(
        void                  **ppIndirection
        ) override {
        printf("getAddrOfCaptureThreadGlobal  not implemented\r\n");
        return nullptr;
    }

    virtual SIZE_T*       getAddrModuleDomainID(CORINFO_MODULE_HANDLE   module) {
        printf("getAddrModuleDomainID not implemented\r\n");
        return nullptr;
    }

    // return a callable address of the function (native code). This function
    // may return a different value (depending on whether the method has
    // been JITed or not.
    void getFunctionEntryPoint(
        CORINFO_METHOD_HANDLE   ftn,                 /* IN  */
        CORINFO_CONST_LOOKUP *  pResult,             /* OUT */
        CORINFO_ACCESS_FLAGS    accessFlags) override {
        auto* method = (BaseMethod*)ftn;
        method->getFunctionEntryPoint(pResult);
    }

    // return a directly callable address. This can be used similarly to the
    // value returned by getFunctionEntryPoint() except that it is
    // guaranteed to be multi callable entrypoint.
    void getFunctionFixedEntryPoint(
        CORINFO_METHOD_HANDLE   ftn,
        CORINFO_CONST_LOOKUP *  pResult) override {
        printf("getFunctionFixedEntryPoint not implemented\r\n");
    }

    // get the synchronization handle that is passed to monXstatic function
    void* getMethodSync(
        CORINFO_METHOD_HANDLE               ftn,
        void                  **ppIndirection
        ) override {
        printf("getMethodSync  not implemented\r\n");
        return nullptr;
    }

    // get slow lazy string literal helper to use (CORINFO_HELP_STRCNS*). 
    // Returns CORINFO_HELP_UNDEF if lazy string literal helper cannot be used.
    CorInfoHelpFunc getLazyStringLiteralHelper(
        CORINFO_MODULE_HANDLE   handle
        ) override {
        printf("getLazyStringLiteralHelper\r\n"); return CORINFO_HELP_UNDEF;
    }

    CORINFO_MODULE_HANDLE embedModuleHandle(
        CORINFO_MODULE_HANDLE   handle,
        void                  **ppIndirection
        ) override {
        printf("embedModuleHandle  not implemented\r\n"); return nullptr;
    }

    CORINFO_CLASS_HANDLE embedClassHandle(
        CORINFO_CLASS_HANDLE    handle,
        void                  **ppIndirection
        ) override {
        printf("embedClassHandle  not implemented\r\n"); return nullptr;
    }

    CORINFO_METHOD_HANDLE embedMethodHandle(
        CORINFO_METHOD_HANDLE   handle,
        void                  **ppIndirection
        ) override {
        printf("embedMethodHandle  not implemented\r\n");
        *ppIndirection = nullptr;
        return handle;
    }

    CORINFO_FIELD_HANDLE embedFieldHandle(
        CORINFO_FIELD_HANDLE    handle,
        void                  **ppIndirection
        ) override {
        printf("embedFieldHandle  not implemented\r\n"); return nullptr;
    }

    // Given a module scope (module), a method handle (context) and
    // a metadata token (metaTOK), fetch the handle
    // (type, field or method) associated with the token.
    // If this is not possible at compile-time (because the current method's
    // code is shared and the token contains generic parameters)
    // then indicate how the handle should be looked up at run-time.
    //
    void embedGenericHandle(
        CORINFO_RESOLVED_TOKEN *        pResolvedToken,
        BOOL                            fEmbedParent, // TRUE - embeds parent type handle of the field/method handle
        CORINFO_GENERICHANDLE_RESULT *  pResult) override {
        if (pResolvedToken->tokenType == CORINFO_TOKENKIND_Newarr) {
            // Emitted from ILGenerator::new_array()
            pResult->lookup.lookupKind.needsRuntimeLookup = false;
            pResult->lookup.constLookup.handle = pResult->compileTimeHandle;
            pResult->lookup.constLookup.accessType = IAT_VALUE;
        }
    }

    // Generate a cookie based on the signature that would needs to be passed
    // to CORINFO_HELP_PINVOKE_CALLI
    LPVOID GetCookieForPInvokeCalliSig(
        CORINFO_SIG_INFO* szMetaSig,
        void           ** ppIndirection
        ) override {
        printf("GetCookieForPInvokeCalliSig  not implemented\r\n");
        return nullptr;
    }

    // returns true if a VM cookie can be generated for it (might be false due to cross-module
    // inlining, in which case the inlining should be aborted)
    bool canGetCookieForPInvokeCalliSig(
        CORINFO_SIG_INFO* szMetaSig
        ) override {
        printf("canGetCookieForPInvokeCalliSig\r\n");
        return true;
    }

    // Gets a handle that is checked to see if the current method is
    // included in "JustMyCode"
    CORINFO_JUST_MY_CODE_HANDLE getJustMyCodeHandle(
        CORINFO_METHOD_HANDLE       method,
        CORINFO_JUST_MY_CODE_HANDLE**ppIndirection
        ) override {
        CORINFO_JUST_MY_CODE_HANDLE result;
        if (ppIndirection)
            *ppIndirection = nullptr;
        DWORD * pFlagAddr = nullptr;
        result = (CORINFO_JUST_MY_CODE_HANDLE) pFlagAddr;
        return result;
    }

    // Gets a method handle that can be used to correlate profiling data.
    // This is the IP of a native method, or the address of the descriptor struct
    // for IL.  Always guaranteed to be unique per process, and not to move. */
    void GetProfilingHandle(
        BOOL                      *pbHookFunction,
        void                     **pProfilerHandle,
        BOOL                      *pbIndirectedHandles
        ) override {
        printf("GetProfilingHandle\r\n");
    }

    // Returns instructions on how to make the call. See code:CORINFO_CALL_INFO for possible return values.
    void getCallInfo(
        // Token info
        CORINFO_RESOLVED_TOKEN * pResolvedToken,

        //Generics info
        CORINFO_RESOLVED_TOKEN * pConstrainedResolvedToken,

        //Security info
        CORINFO_METHOD_HANDLE   callerHandle,

        //Jit info
        CORINFO_CALLINFO_FLAGS  flags,

        //out params (OUT)
        CORINFO_CALL_INFO       *pResult
        ) override {
        auto method = (BaseMethod*)pResolvedToken->hMethod;
        pResult->hMethod = (CORINFO_METHOD_HANDLE)method;

        method->get_call_info(pResult);
        pResult->nullInstanceCheck = false;
        pResult->sig.callConv = CORINFO_CALLCONV_DEFAULT;
        pResult->sig.retTypeClass = nullptr;
        pResult->verSig = pResult->sig;
        pResult->accessAllowed = CORINFO_ACCESS_ALLOWED;
    }

    BOOL canAccessFamily(CORINFO_METHOD_HANDLE hCaller,
        CORINFO_CLASS_HANDLE hInstanceType) override {
        printf("canAccessFamily \r\n");
        return FALSE;
    }

    // Returns TRUE if the Class Domain ID is the RID of the class (currently true for every class
    // except reflection emitted classes and generics)
    BOOL isRIDClassDomainID(CORINFO_CLASS_HANDLE cls) override {
        printf("isRIDClassDomainID  \r\n");
        return FALSE;
    }

    // returns the class's domain ID for accessing shared statics
    unsigned getClassDomainID(
        CORINFO_CLASS_HANDLE    cls,
        void                  **ppIndirection
        ) override {
        printf("getClassDomainID not implemented\r\n");
        return 0;
    }

    // return the data's address (for static fields only)
    void* getFieldAddress(
        CORINFO_FIELD_HANDLE    field,
        void                  **ppIndirection
        ) override {
        printf("getFieldAddress  not implemented\r\n");
        return nullptr;
    }

    // registers a vararg sig & returns a VM cookie for it (which can contain other stuff)
    CORINFO_VARARGS_HANDLE getVarArgsHandle(
        CORINFO_SIG_INFO       *pSig,
        void                  **ppIndirection
        ) override {
        printf("getVarArgsHandle  not implemented\r\n");
        return nullptr;
    }

    // returns true if a VM cookie can be generated for it (might be false due to cross-module
    // inlining, in which case the inlining should be aborted)
    bool canGetVarArgsHandle(
        CORINFO_SIG_INFO       *pSig
        ) override {
        printf("canGetVarArgsHandle\r\n");
        return false;
    }

    // Allocate a string literal on the heap and return a handle to it
    InfoAccessType constructStringLiteral(
        CORINFO_MODULE_HANDLE   module,
        mdToken                 metaTok,
        void                  **ppValue
        ) override {
        printf("constructStringLiteral\r\n");
        return IAT_VALUE;
    }

    InfoAccessType emptyStringLiteral(
        void                  **ppValue
        ) override {
        printf("emptyStringLiteral\r\n");
        return IAT_VALUE;
    }

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    DWORD getMethodAttribs(
        CORINFO_METHOD_HANDLE       ftn         /* IN */
        ) override {
        auto method = (BaseMethod*)ftn;
        return method->get_method_attrs();
    }

    // sets private JIT flags, which can be, retrieved using getAttrib.
    void setMethodAttribs(
        CORINFO_METHOD_HANDLE       ftn,        /* IN */
        CorInfoMethodRuntimeFlags   attribs     /* IN */
        ) override {
        //printf("setMethodAttribs  not implemented\r\n");
    }

    // Given a method descriptor ftnHnd, extract signature information into sigInfo
    //
    // 'memberParent' is typically only set when verifying.  It should be the
    // result of calling getMemberParent.
    void getMethodSig(
        CORINFO_METHOD_HANDLE      ftn,        /* IN  */
        CORINFO_SIG_INFO          *sig,        /* OUT */
        CORINFO_CLASS_HANDLE      memberParent /* IN */
        ) override {
        auto* m = (BaseMethod*)ftn;
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
    bool getMethodInfo(
        CORINFO_METHOD_HANDLE   ftn,            /* IN  */
        CORINFO_METHOD_INFO*    info            /* OUT */
        ) override {
        printf("getMethodInfo  not implemented\r\n"); return false;
    }

    // Decides if you have any limitations for inlining. If everything's OK, it will return
    // INLINE_PASS and will fill out pRestrictions with a mask of restrictions the caller of this
    // function must respect. If caller passes pRestrictions = NULL, if there are any restrictions
    // INLINE_FAIL will be returned
    //
    // The callerHnd must be the immediate caller (i.e. when we have a chain of inlined calls)
    //
    // The inlined method need not be verified

    CorInfoInline canInline(
        CORINFO_METHOD_HANDLE       callerHnd,                  /* IN  */
        CORINFO_METHOD_HANDLE       calleeHnd,                  /* IN  */
        DWORD*                      pRestrictions               /* OUT */
        ) override {
        printf("canInline\r\n");
        return INLINE_PASS;
    }

    // Reports whether or not a method can be inlined, and why.  canInline is responsible for reporting all
    // inlining results when it returns INLINE_FAIL and INLINE_NEVER.  All other results are reported by the
    // JIT.
    void reportInliningDecision(CORINFO_METHOD_HANDLE inlinerHnd,
        CORINFO_METHOD_HANDLE inlineeHnd,
        CorInfoInline inlineResult,
        const char * reason) override {
        //printf("reportInliningDecision\r\n");
    }


    // Returns false if the call is across security boundaries thus we cannot tailcall
    //
    // The callerHnd must be the immediate caller (i.e. when we have a chain of inlined calls)
    bool canTailCall(
        CORINFO_METHOD_HANDLE   callerHnd,          /* IN */
        CORINFO_METHOD_HANDLE   declaredCalleeHnd,  /* IN */
        CORINFO_METHOD_HANDLE   exactCalleeHnd,     /* IN */
        bool fIsTailPrefix                          /* IN */
        ) override {
        return FALSE;
    }

    // Reports whether or not a method can be tail called, and why.
    // canTailCall is responsible for reporting all results when it returns
    // false.  All other results are reported by the JIT.
    void reportTailCallDecision(CORINFO_METHOD_HANDLE callerHnd,
        CORINFO_METHOD_HANDLE calleeHnd,
        bool fIsTailPrefix,
        CorInfoTailCall tailCallResult,
        const char * reason) override {
        //printf("reportTailCallDecision\r\n");
    }

    // get individual exception handler
    void getEHinfo(
        CORINFO_METHOD_HANDLE ftn,              /* IN  */
        unsigned          EHnumber,             /* IN */
        CORINFO_EH_CLAUSE* clause               /* OUT */
        ) override {
        printf("getEHinfo not implemented\r\n");
    }

    // return class it belongs to
    CORINFO_CLASS_HANDLE getMethodClass(
        CORINFO_METHOD_HANDLE       method
        ) override {
        auto meth = (BaseMethod*)method;
        return meth->getClass();
    }

    // return module it belongs to
    CORINFO_MODULE_HANDLE getMethodModule(
        CORINFO_METHOD_HANDLE       method
        ) override {
        printf("getMethodModule  not implemented\r\n"); return nullptr;
    }

    // If a method's attributes have (getMethodAttribs) CORINFO_FLG_INTRINSIC set,
    // getIntrinsicID() returns the intrinsic ID.
    CorInfoIntrinsics getIntrinsicID(
        CORINFO_METHOD_HANDLE       method,
		bool * pMustExpand
        ) override {
        printf("getIntrinsicID\r\n"); return CORINFO_INTRINSIC_Object_GetType;
    }

    // return the unmanaged calling convention for a PInvoke
    CorInfoUnmanagedCallConv getUnmanagedCallConv(
        CORINFO_METHOD_HANDLE       method
        ) override {
        printf("getUnmanagedCallConv\r\n");
        return CORINFO_UNMANAGED_CALLCONV_C;
    }

    // return if any marshaling is required for PInvoke methods.  Note that
    // method == 0 => calli.  The call site sig is only needed for the varargs or calli case
    BOOL pInvokeMarshalingRequired(
        CORINFO_METHOD_HANDLE       method,
        CORINFO_SIG_INFO*           callSiteSig
        ) override {
        printf("pInvokeMarshalingRequired\r\n");
        return TRUE;
    }

    // Check constraints on method type arguments (only).
    // The parent class should be checked separately using satisfiesClassConstraints(parent).
    BOOL satisfiesMethodConstraints(
        CORINFO_CLASS_HANDLE        parent, // the exact parent of the method
        CORINFO_METHOD_HANDLE       method
        ) override {
        //printf("satisfiesMethodConstraints\r\n"); 
        return TRUE;
    }

    // Given a delegate target class, a target method parent class,  a  target method,
    // a delegate class, check if the method signature is compatible with the Invoke method of the delegate
    // (under the typical instantiation of any free type variables in the memberref signatures).
    BOOL isCompatibleDelegate(
        CORINFO_CLASS_HANDLE        objCls,           /* type of the delegate target, if any */
        CORINFO_CLASS_HANDLE        methodParentCls,  /* exact parent of the target method, if any */
        CORINFO_METHOD_HANDLE       method,           /* (representative) target method, if any */
        CORINFO_CLASS_HANDLE        delegateCls,      /* exact type of the delegate */
        BOOL                        *pfIsOpenDelegate /* is the delegate open */
        ) override {
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

    // load and restore the method
    void methodMustBeLoadedBeforeCodeIsRun(
        CORINFO_METHOD_HANDLE       method
        ) override {
        printf("methodMustBeLoadedBeforeCodeIsRun\r\n");
    }

    CORINFO_METHOD_HANDLE mapMethodDeclToMethodImpl(
        CORINFO_METHOD_HANDLE       method
        ) override {
        printf("mapMethodDeclToMethodImpl\r\n"); return nullptr;
    }

    // Returns the global cookie for the /GS unsafe buffer checks
    // The cookie might be a constant value (JIT), or a handle to memory location (Ngen)
    void getGSCookie(
        GSCookie * pCookieVal,                     // OUT
        GSCookie ** ppCookieVal                    // OUT
        ) override {
        *pCookieVal = 0x1234; // TODO: Should be a secure value
        *ppCookieVal = nullptr;
        //printf("getGSCookie\r\n");
    }

    /**********************************************************************************/
    //
    // ICorModuleInfo
    //
    /**********************************************************************************/

    // Resolve metadata token into runtime method handles.
    void resolveToken(/* IN, OUT */ CORINFO_RESOLVED_TOKEN * pResolvedToken) override {
        auto* mod = (Module*)pResolvedToken->tokenScope;
        BaseMethod* method = mod->ResolveMethod(pResolvedToken->token);
        pResolvedToken->hMethod = (CORINFO_METHOD_HANDLE)method;
        pResolvedToken->hClass = PYOBJECT_PTR_TYPE; // Internal reference for Pyobject ptr
        //printf("resolveToken %d\r\n", pResolvedToken->token);
    }

    // Signature information about the call sig
    void findSig(
        CORINFO_MODULE_HANDLE       module,     /* IN */
        unsigned                    sigTOK,     /* IN */
        CORINFO_CONTEXT_HANDLE      context,    /* IN */
        CORINFO_SIG_INFO           *sig         /* OUT */
        ) override {
        printf("findSig %d\r\n", sigTOK);
        auto mod = (Module*)module;
        // TODO : ResolveMethod is signed, sigTok is unsigned.
        auto method = mod->ResolveMethod(sigTOK);
        method->findSig(sig);
    }

    // for Varargs, the signature at the call site may differ from
    // the signature at the definition.  Thus we need a way of
    // fetching the call site information
    void findCallSiteSig(
        CORINFO_MODULE_HANDLE       module,     /* IN */
        unsigned                    methTOK,    /* IN */
        CORINFO_CONTEXT_HANDLE      context,    /* IN */
        CORINFO_SIG_INFO           *sig         /* OUT */
        ) override {
#ifdef DUMP_TRACES
        printf("findCallSiteSig\r\n");
#endif
    }

    CORINFO_CLASS_HANDLE getTokenTypeAsHandle(
        CORINFO_RESOLVED_TOKEN *    pResolvedToken /* IN  */) override {
        printf("getTokenTypeAsHandle  not implemented\r\n");
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

    // Checks if the given metadata token is valid
    BOOL isValidToken(
        CORINFO_MODULE_HANDLE       module,     /* IN  */
        unsigned                    metaTOK     /* IN  */
        ) override {
        printf("isValidToken\r\n"); return TRUE;
    }

    // Checks if the given metadata token is valid StringRef
    BOOL isValidStringRef(
        CORINFO_MODULE_HANDLE       module,     /* IN  */
        unsigned                    metaTOK     /* IN  */
        ) override {
        printf("isValidStringRef\r\n");
        return TRUE;
    }

    /**********************************************************************************/
    //
    // ICorClassInfo
    //
    /**********************************************************************************/

    // If the value class 'cls' is isomorphic to a primitive type it will
    // return that type, otherwise it will return CORINFO_TYPE_VALUECLASS
    CorInfoType asCorInfoType(
        CORINFO_CLASS_HANDLE    cls
        ) override {
        if (cls == PYOBJECT_PTR_TYPE){
            return CORINFO_TYPE_PTR;
        }
        printf("unimplemented asCorInfoType\r\n");
        return CORINFO_TYPE_UNDEF;
    }

    // for completeness
    const char* getClassName(
        CORINFO_CLASS_HANDLE    cls
        ) override {
        if (cls == PYOBJECT_PTR_TYPE)
            return "PyObject";
        return "classname";
    }

    // Append a (possibly truncated) representation of the type cls to the preallocated buffer ppBuf of length pnBufLen
    // If fNamespace=TRUE, include the namespace/enclosing classes
    // If fFullInst=TRUE (regardless of fNamespace and fAssembly), include namespace and assembly for any type parameters
    // If fAssembly=TRUE, suffix with a comma and the full assembly qualification
    // return size of representation
    int appendClassName(
        __deref_inout_ecount(*pnBufLen) WCHAR** ppBuf,
        int* pnBufLen,
        CORINFO_CLASS_HANDLE    cls,
        BOOL fNamespace,
        BOOL fFullInst,
        BOOL fAssembly
        ) override {
        printf("appendClassName\r\n"); return 0;
    }

    // Quick check whether the type is a value class. Returns the same value as getClassAttribs(cls) & CORINFO_FLG_VALUECLASS, except faster.
    BOOL isValueClass(CORINFO_CLASS_HANDLE cls) override {
        if (cls == PYOBJECT_PTR_TYPE)
            return FALSE;
        return FALSE;
    }

    // return flags (defined above, CORINFO_FLG_PUBLIC ...)
    DWORD getClassAttribs(
        CORINFO_CLASS_HANDLE    cls
        ) override {
        // TODO : Load from a base class and establish correct attribs.
        if (cls == PYOBJECT_PTR_TYPE)
            return CORINFO_FLG_NATIVE;
        return CORINFO_FLG_VALUECLASS;
    }

    // Returns "TRUE" iff "cls" is a struct type such that return buffers used for returning a value
    // of this type must be stack-allocated.  This will generally be true only if the struct 
    // contains GC pointers, and does not exceed some size limit.  Maintaining this as an invariant allows
    // an optimization: the JIT may assume that return buffer pointers for return types for which this predicate
    // returns TRUE are always stack allocated, and thus, that stores to the GC-pointer fields of such return
    // buffers do not require GC write barriers.
    BOOL isStructRequiringStackAllocRetBuf(CORINFO_CLASS_HANDLE cls) override {
        printf("isStructRequiringStackAllocRetBuf\r\n");
        return FALSE;
    }

    CORINFO_MODULE_HANDLE getClassModule(
        CORINFO_CLASS_HANDLE    cls
        ) override {
        printf("getClassModule  not implemented\r\n");
        return nullptr;
    }

    // Returns the assembly that contains the module "mod".
    CORINFO_ASSEMBLY_HANDLE getModuleAssembly(
        CORINFO_MODULE_HANDLE   mod
        ) override {
        printf("getModuleAssembly  not implemented\r\n");
        return nullptr;
    }

    // Returns the name of the assembly "assem".
    const char* getAssemblyName(
        CORINFO_ASSEMBLY_HANDLE assem
        ) override {
        printf("getAssemblyName  not implemented\r\n");
        return "assem";
    }

    // Allocate and delete process-lifetime objects.  Should only be
    // referred to from static fields, lest a leak occur.
    // Note that "LongLifetimeFree" does not execute destructors, if "obj"
    // is an array of a struct type with a destructor.
    void* LongLifetimeMalloc(size_t sz) override {
        printf("LongLifetimeMalloc\r\n");
        return nullptr;
    }

    void LongLifetimeFree(void* obj) override {
        printf("LongLifetimeFree\r\n");
    }

    size_t getClassModuleIdForStatics(
        CORINFO_CLASS_HANDLE    cls,
        CORINFO_MODULE_HANDLE *pModule,
        void **ppIndirection
        ) override {
        printf("getClassModuleIdForStatics  not implemented\r\n");
        return 0;
    }

    // return the number of bytes needed by an instance of the class
    unsigned getClassSize(
        CORINFO_CLASS_HANDLE        cls
        ) override {
        printf("getClassSize  not implemented\r\n");
        return 0;
    }

    unsigned getClassAlignmentRequirement(
        CORINFO_CLASS_HANDLE        cls,
        BOOL                        fDoubleAlignHint
        ) override {
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
    unsigned getClassGClayout(
        CORINFO_CLASS_HANDLE        cls,        /* IN */
        BYTE                       *gcPtrs      /* OUT */
        ) override {
        printf("getClassGClayout\r\n");
        return 0;
    }

    // returns the number of instance fields in a class
    unsigned getClassNumInstanceFields(
        CORINFO_CLASS_HANDLE        cls        /* IN */
        ) override {
        printf("getClassNumInstanceFields\r\n");
        return 0;
    }

    CORINFO_FIELD_HANDLE getFieldInClass(
        CORINFO_CLASS_HANDLE clsHnd,
        INT num
        ) override {
        printf("getFieldInClass\r\n");
        return nullptr;
    }

    BOOL checkMethodModifier(
        CORINFO_METHOD_HANDLE hMethod,
        LPCSTR modifier,
        BOOL fOptional
        ) override {
        printf("checkMethodModifier\r\n");
        return FALSE;
    }

    // returns the newArr (1-Dim array) helper optimized for "arrayCls."
    CorInfoHelpFunc getNewArrHelper(
        CORINFO_CLASS_HANDLE        arrayCls
        ) override {
        if (arrayCls == PYOBJECT_PTR_TYPE){
            return CORINFO_HELP_NEWARR_1_VC;
        }
        printf("getNewArrHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

    // returns the optimized "IsInstanceOf" or "ChkCast" helper
    CorInfoHelpFunc getCastingHelper(
        CORINFO_RESOLVED_TOKEN * pResolvedToken,
        bool fThrowing
        ) override {
        printf("getCastingHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

    // returns helper to trigger static constructor
    CorInfoHelpFunc getSharedCCtorHelper(
        CORINFO_CLASS_HANDLE clsHnd
        ) override {
        printf("getSharedCCtorHelper\r\n");
        return CORINFO_HELP_UNDEF;
    }

    // This is not pretty.  Boxing nullable<T> actually returns
    // a boxed<T> not a boxed Nullable<T>.  This call allows the verifier
    // to call back to the EE on the 'box' instruction and get the transformed
    // type to use for verification.
    CORINFO_CLASS_HANDLE  getTypeForBox(
        CORINFO_CLASS_HANDLE        cls
        ) override {
        printf("getTypeForBox  not implemented\r\n");
        return nullptr;
    }

    // returns the correct box helper for a particular class.  Note
    // that if this returns CORINFO_HELP_BOX, the JIT can assume 
    // 'standard' boxing (allocate object and copy), and optimize
    CorInfoHelpFunc getBoxHelper(
        CORINFO_CLASS_HANDLE        cls
        ) override {
        printf("getBoxHelper\r\n");
        return CORINFO_HELP_BOX;
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

    CorInfoHelpFunc getUnBoxHelper(
        CORINFO_CLASS_HANDLE        cls
        ) override {
        printf("getUnBoxHelper\r\n");
        return CORINFO_HELP_UNBOX;
    }

    const char* getHelperName(
        CorInfoHelpFunc
        ) override {
        return "AnyJITHelper";
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
    void classMustBeLoadedBeforeCodeIsRun(
        CORINFO_CLASS_HANDLE        cls
        ) override {
        //printf("classMustBeLoadedBeforeCodeIsRun\r\n");
    }

    // returns the class handle for the special builtin classes
    CORINFO_CLASS_HANDLE getBuiltinClass(
        CorInfoClassId              classId
        ) override {
        printf("getBuiltinClass\r\n");
        return nullptr;
    }

    // "System.Int32" ==> CORINFO_TYPE_INT..
    CorInfoType getTypeForPrimitiveValueClass(
        CORINFO_CLASS_HANDLE        cls
        ) override {
        if (cls == PYOBJECT_PTR_TYPE)
            return CORINFO_TYPE_NATIVEINT;
        printf("getTypeForPrimitiveValueClass\r\n");
        return CORINFO_TYPE_UNDEF;
    }

    // TRUE if child is a subtype of parent
    // if parent is an interface, then does child implement / extend parent
    BOOL canCast(
        CORINFO_CLASS_HANDLE        child,  // subtype (extends parent)
        CORINFO_CLASS_HANDLE        parent  // base type
        ) override {
        printf("canCast\r\n");
        return TRUE;
    }

    // TRUE if cls1 and cls2 are considered equivalent types.
    BOOL areTypesEquivalent(
        CORINFO_CLASS_HANDLE        cls1,
        CORINFO_CLASS_HANDLE        cls2
        ) override {
        printf("areTypesEquivalent\r\n");
        return FALSE;
    }

    // returns is the intersection of cls1 and cls2.
    CORINFO_CLASS_HANDLE mergeClasses(
        CORINFO_CLASS_HANDLE        cls1,
        CORINFO_CLASS_HANDLE        cls2
        ) override {
        printf("mergeClasses  not implemented\r\n");
        return nullptr;
    }

    // Given a class handle, returns the Parent type.
    // For COMObjectType, it returns Class Handle of System.Object.
    // Returns 0 if System.Object is passed in.
    CORINFO_CLASS_HANDLE getParentType(
        CORINFO_CLASS_HANDLE        cls
        ) override {
        printf("getParentType  not implemented\r\n");
        return nullptr;
    }

    // Returns the CorInfoType of the "child type". If the child type is
    // not a primitive type, *clsRet will be set.
    // Given an Array of Type Foo, returns Foo.
    // Given BYREF Foo, returns Foo
    CorInfoType getChildType(
        CORINFO_CLASS_HANDLE       clsHnd,
        CORINFO_CLASS_HANDLE       *clsRet
        ) override {
        printf("getChildType  not implemented\r\n");
        return CORINFO_TYPE_UNDEF;
    }

    // Check constraints on type arguments of this class and parent classes
    BOOL satisfiesClassConstraints(
        CORINFO_CLASS_HANDLE cls
        ) override {
        //printf("satisfiesClassConstraints\r\n");
        return TRUE;
    }

    // Check if this is a single dimensional array type
    BOOL isSDArray(
        CORINFO_CLASS_HANDLE        cls
        ) override {
        printf("isSDArray\r\n");
        return TRUE;
    }

    // Get the numbmer of dimensions in an array 
    unsigned getArrayRank(
        CORINFO_CLASS_HANDLE        cls
        ) override {
        printf("getArrayRank\r\n");
        return 0;
    }

    // Get static field data for an array
    void * getArrayInitializationData(
        CORINFO_FIELD_HANDLE        field,
        DWORD                       size
        ) override {
        printf("getArrayInitializationData\r\n");
        return nullptr;
    }

    // Check Visibility rules.
    CorInfoIsAccessAllowedResult canAccessClass(
        CORINFO_RESOLVED_TOKEN * pResolvedToken,
        CORINFO_METHOD_HANDLE   callerHandle,
        CORINFO_HELPER_DESC    *pAccessHelper /* If canAccessMethod returns something other
                                              than ALLOWED, then this is filled in. */
        ) override {
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
    const char* getFieldName(
        CORINFO_FIELD_HANDLE        ftn,        /* IN */
        const char                **moduleName  /* OUT */
        ) override {
        printf("getFieldName  not implemented\r\n");
        return "field";
    }

    // return class it belongs to
    CORINFO_CLASS_HANDLE getFieldClass(
        CORINFO_FIELD_HANDLE    field
        ) override {
        printf("getFieldClass not implemented\r\n");
        return nullptr;
    }

    // Return the field's type, if it is CORINFO_TYPE_VALUECLASS 'structType' is set
    // the field's value class (if 'structType' == 0, then don't bother
    // the structure info).
    //
    // 'memberParent' is typically only set when verifying.  It should be the
    // result of calling getMemberParent.
    CorInfoType getFieldType(
        CORINFO_FIELD_HANDLE    field,
        CORINFO_CLASS_HANDLE   *structType,
        CORINFO_CLASS_HANDLE    memberParent /* IN */
        ) override {
        printf("getFieldType\r\n");
        return CORINFO_TYPE_UNDEF;
    }

    // return the data member's instance offset
    unsigned getFieldOffset(
        CORINFO_FIELD_HANDLE    field
        ) override {
        printf("getFieldOffset\r\n");
        return 0;
    }

    void getFieldInfo(CORINFO_RESOLVED_TOKEN * pResolvedToken,
        CORINFO_METHOD_HANDLE  callerHandle,
        CORINFO_ACCESS_FLAGS   flags,
        CORINFO_FIELD_INFO    *pResult
        ) override {
        printf("\r\n");
    }

    // Returns true iff "fldHnd" represents a static field.
    bool isFieldStatic(CORINFO_FIELD_HANDLE fldHnd) override {
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
    void getBoundaries(
        CORINFO_METHOD_HANDLE   ftn,                // [IN] method of interest
        unsigned int           *cILOffsets,         // [OUT] size of pILOffsets
        DWORD                 **pILOffsets,         // [OUT] IL offsets of interest
        //       jit MUST free with freeArray!
        ICorDebugInfo::BoundaryTypes *implictBoundaries // [OUT] tell jit, all boundries of this type
        ) override {
        printf("getBoundaries\r\n");
    }

    // Report back the mapping from IL to native code,
    // this map should include all boundaries that 'getBoundaries'
    // reported as interesting to the debugger.

    // Note that debugger (and profiler) is assuming that all of the
    // offsets form a contiguous block of memory, and that the
    // OffsetMapping is sorted in order of increasing native offset.
    void setBoundaries(
        CORINFO_METHOD_HANDLE   ftn,            // [IN] method of interest
        ULONG32                 cMap,           // [IN] size of pMap
        ICorDebugInfo::OffsetMapping *pMap      // [IN] map including all points of interest.
        //      jit allocated with allocateArray, EE frees
        ) override {
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
    void getVars(
        CORINFO_METHOD_HANDLE           ftn,            // [IN]  method of interest
        ULONG32                        *cVars,          // [OUT] size of 'vars'
        ICorDebugInfo::ILVarInfo       **vars,          // [OUT] scopes of variables of interest
        //       jit MUST free with freeArray!
        bool                           *extendOthers    // [OUT] it TRUE, then assume the scope
        //       of unmentioned vars is entire method
        ) override {
        printf("getVars\r\n");
    }

    // Report back to the EE the location of every variable.
    // note that the JIT might split lifetimes into different
    // locations etc.

    void setVars(
        CORINFO_METHOD_HANDLE           ftn,            // [IN] method of interest
        ULONG32                         cVars,          // [IN] size of 'vars'
        ICorDebugInfo::NativeVarInfo   *vars            // [IN] map telling where local vars are stored at what points
        //      jit allocated with allocateArray, EE frees
        ) override {
        printf("setVars\r\n");
    }

    /*-------------------------- Misc ---------------------------------------*/

    // JitCompiler will free arrays passed by the EE using this
    // For eg, The EE returns memory in getVars() and getBoundaries()
    // to the JitCompiler, which the JitCompiler should release using
    // freeArray()
    void freeArray(
        void               *array
        ) override {
        printf("freeArray\r\n");
    }

    /*********************************************************************************/
    //
    // ICorArgInfo
    //
    /*********************************************************************************/

    // advance the pointer to the argument list.
    // a ptr of 0, is special and always means the first argument
    CORINFO_ARG_LIST_HANDLE getArgNext(
        CORINFO_ARG_LIST_HANDLE     args            /* IN */
        ) override {
        return (CORINFO_ARG_LIST_HANDLE)(((Parameter*)args) + 1);
    }

    // If the Arg is a CORINFO_TYPE_CLASS fetch the class handle associated with it
    CORINFO_CLASS_HANDLE getArgClass(
        CORINFO_SIG_INFO*           sig,            /* IN */
        CORINFO_ARG_LIST_HANDLE     args            /* IN */
        ) override {
        // TODO: Work out correct return type
        //return PYOBJECT_PTR_TYPE;
        return nullptr;
    }

    /*****************************************************************************
    * ICorErrorInfo contains methods to deal with SEH exceptions being thrown
    * from the corinfo interface.  These methods may be called when an exception
    * with code EXCEPTION_COMPLUS is caught.
    *****************************************************************************/

    // Returns the HRESULT of the current exception
    HRESULT GetErrorHRESULT(
    struct _EXCEPTION_POINTERS *pExceptionPointers
        ) override {
        printf("GetErrorHRESULT\r\n");
        return E_FAIL;
    }

    // Fetches the message of the current exception
    // Returns the size of the message (including terminating null). This can be
    // greater than bufferLength if the buffer is insufficient.
    ULONG GetErrorMessage(
        __inout_ecount(bufferLength) LPWSTR buffer,
        ULONG bufferLength
        ) override {
        printf("GetErrorMessage\r\n");
        return 0;
    }

    // returns EXCEPTION_EXECUTE_HANDLER if it is OK for the compile to handle the
    //                        exception, abort some work (like the inlining) and continue compilation
    // returns EXCEPTION_CONTINUE_SEARCH if exception must always be handled by the EE
    //                    things like ThreadStoppedException ...
    // returns EXCEPTION_CONTINUE_EXECUTION if exception is fixed up by the EE

    int FilterException(
    struct _EXCEPTION_POINTERS *pExceptionPointers
        ) override {
        printf("FilterException\r\n"); return 0;
    }

    // Cleans up internal EE tracking when an exception is caught.
    void HandleException(
    struct _EXCEPTION_POINTERS *pExceptionPointers
        ) override {
        printf("HandleException\r\n");
    }

    void ThrowExceptionForJitResult(
        HRESULT result) override {
        printf("ThrowExceptionForJitResult\r\n");
    }

    //Throws an exception defined by the given throw helper.
    void ThrowExceptionForHelper(
        const CORINFO_HELPER_DESC * throwHelper) override {
        printf("ThrowExceptionForHelper\r\n");
    }

    /*****************************************************************************
    * ICorStaticInfo contains EE interface methods which return values that are
    * constant from invocation to invocation.  Thus they may be embedded in
    * persisted information like statically generated code. (This is of course
    * assuming that all code versions are identical each time.)
    *****************************************************************************/

    // Return details about EE internal data structures
    void getEEInfo(
        CORINFO_EE_INFO            *pEEInfoOut
        ) override {
        memset(pEEInfoOut, 0, sizeof(CORINFO_EE_INFO));
        pEEInfoOut->inlinedCallFrameInfo.size = 4;
#ifdef WINDOWS
        pEEInfoOut->osPageSize = 0x1000; // Set to the windows default
        pEEInfoOut->osType = CORINFO_WINNT;
#else
        pEEInfoOut->osType = CORINFO_UNIX;
#endif
    }

    // Returns name of the JIT timer log
    LPCWSTR getJitTimeLogFilename() override {
        return reinterpret_cast<LPCWSTR>("pyjion.log");
    }

    /*********************************************************************************/
    //
    // Diagnostic methods
    //
    /*********************************************************************************/

    // this function is for debugging only. Returns method token.
    // Returns mdMethodDefNil for dynamic methods.
    mdMethodDef getMethodDefFromMethod(
        CORINFO_METHOD_HANDLE hMethod
        ) override {
        printf("getMethodDefFromMethod\r\n");
        return 0;
    }

    // this function is for debugging only.  It returns the method name
    // and if 'moduleName' is non-null, it sets it to something that will
    // says which method (a class name, or a module name)
    const char* getMethodName(
        CORINFO_METHOD_HANDLE       ftn,        /* IN */
        const char                **moduleName  /* OUT */
        ) override {
        *moduleName = PyUnicode_AsUTF8(m_code->co_filename);
        return PyUnicode_AsUTF8(m_code->co_name);
    }

    // this function is for debugging only.  It returns a value that
    // is will always be the same for a given method.  It is used
    // to implement the 'jitRange' functionality
    unsigned getMethodHash(
        CORINFO_METHOD_HANDLE       ftn         /* IN */
        ) override {
        return 0;
    }

    // this function is for debugging only.
    size_t findNameOfToken(
        CORINFO_MODULE_HANDLE       module,     /* IN  */
        mdToken                     metaTOK,     /* IN  */
        __out_ecount(FQNameCapacity) char * szFQName, /* OUT */
        size_t FQNameCapacity  /* IN */
        ) override {
        printf("findNameOfToken\r\n");
        return 0;

    }

	void getAddressOfPInvokeTarget(CORINFO_METHOD_HANDLE method, CORINFO_CONST_LOOKUP * pLookup) override
	{
        printf("getAddressOfPInvokeTarget\r\n");
	}

	DWORD getJitFlags(CORJIT_FLAGS * flags, DWORD sizeInBytes) override
	{
		flags->Add(flags->CORJIT_FLAG_SKIP_VERIFICATION);
#ifdef DEBUG
        flags->Add(flags->CORJIT_FLAG_DEBUG_CODE);
#endif
        // Extra flags available
        //flags->Add(flags->CORJIT_FLAG_NO_INLINING);
        //flags->Add(flags->CORJIT_FLAG_MIN_OPT);
		return sizeof(CORJIT_FLAGS);
	}

    void getMethodVTableOffset(CORINFO_METHOD_HANDLE method, unsigned int *offsetOfIndirection,
                               unsigned int *offsetAfterIndirection, bool *isRelative) override {
        // TODO : API added isRelative flag, this doesn't inspect that.
        *offsetOfIndirection = 0x1234;
        *offsetAfterIndirection = 0x2468;
        printf("getMethodVTableOffset\r\n");
    }

    CORINFO_METHOD_HANDLE
    resolveVirtualMethod(CORINFO_METHOD_HANDLE virtualMethod, CORINFO_CLASS_HANDLE implementingClass,
                         CORINFO_CONTEXT_HANDLE ownerType) override {
        printf("resolveVirtualMethod not defined\r\n");
        return nullptr;
    }

    CORINFO_METHOD_HANDLE getUnboxedEntry(CORINFO_METHOD_HANDLE ftn, bool *requiresInstMethodTableArg) override {
        printf("getUnboxedEntry not defined\r\n");
        return nullptr;
    }

    CORINFO_CLASS_HANDLE getDefaultEqualityComparerClass(CORINFO_CLASS_HANDLE elemType) override {
        printf("getDefaultEqualityComparerClass not defined\r\n");
        return nullptr;
    }

    void
    expandRawHandleIntrinsic(CORINFO_RESOLVED_TOKEN *pResolvedToken, CORINFO_GENERICHANDLE_RESULT *pResult) override {

    }

    void setPatchpointInfo(PatchpointInfo *patchpointInfo) override {

    }

    PatchpointInfo *getOSRInfo(unsigned int *ilOffset) override {
        printf("getOSRInfo not defined\r\n");
        return nullptr;
    }

    bool tryResolveToken(CORINFO_RESOLVED_TOKEN *pResolvedToken) override {
        return false;
    }

    LPCWSTR getStringLiteral(CORINFO_MODULE_HANDLE module, unsigned int metaTOK, int *length) override {
        printf("getStringLiteral not defined\r\n");
        return nullptr;
    }

    const char *getClassNameFromMetadata(CORINFO_CLASS_HANDLE cls, const char **namespaceName) override {
        printf("getClassNameFromMetadata not defined\r\n");
        return nullptr;
    }

    CORINFO_CLASS_HANDLE getTypeInstantiationArgument(CORINFO_CLASS_HANDLE cls, unsigned int index) override {
        printf("getTypeInstantiationArgument not defined\r\n");
        return nullptr;
    }

    CorInfoInlineTypeCheck canInlineTypeCheck(CORINFO_CLASS_HANDLE cls, CorInfoInlineTypeCheckSource source) override {
        return CORINFO_INLINE_TYPECHECK_USE_HELPER;
    }

    unsigned int getHeapClassSize(CORINFO_CLASS_HANDLE cls) override {
        return 0;
    }

    BOOL canAllocateOnStack(CORINFO_CLASS_HANDLE cls) override {
        return false;
    }

    CorInfoHelpFunc getNewHelper(CORINFO_RESOLVED_TOKEN *pResolvedToken, CORINFO_METHOD_HANDLE callerHandle,
                                 bool *pHasSideEffects) override {
        return CORINFO_HELP_GETREFANY;
    }

    bool getReadyToRunHelper(CORINFO_RESOLVED_TOKEN *pResolvedToken, CORINFO_LOOKUP_KIND *pGenericLookupKind,
                             CorInfoHelpFunc id, CORINFO_CONST_LOOKUP *pLookup) override {
        return false;
    }

    void getReadyToRunDelegateCtorHelper(CORINFO_RESOLVED_TOKEN *pTargetMethod, CORINFO_CLASS_HANDLE delegateType,
                                         CORINFO_LOOKUP *pLookup) override {

    }

    CorInfoInitClassResult
    initClass(CORINFO_FIELD_HANDLE field, CORINFO_METHOD_HANDLE method, CORINFO_CONTEXT_HANDLE context) override {
        return CORINFO_INITCLASS_INITIALIZED;
    }

    CorInfoType getTypeForPrimitiveNumericClass(CORINFO_CLASS_HANDLE cls) override {
        return CORINFO_TYPE_BYREF;
    }

    TypeCompareState compareTypesForCast(CORINFO_CLASS_HANDLE fromClass, CORINFO_CLASS_HANDLE toClass) override {
        return TypeCompareState::May;
    }

    TypeCompareState compareTypesForEquality(CORINFO_CLASS_HANDLE cls1, CORINFO_CLASS_HANDLE cls2) override {
        return TypeCompareState::May;
    }

    BOOL isMoreSpecificType(CORINFO_CLASS_HANDLE cls1, CORINFO_CLASS_HANDLE cls2) override {
        return false;
    }

    void *allocateArray(size_t cBytes) override {
        printf("allocateArray not defined\r\n");
        return nullptr;
    }

    CorInfoHFAElemType getHFAType(CORINFO_CLASS_HANDLE hClass) override {
        return CORINFO_HFA_ELEM_DOUBLE;
    }

    bool runWithErrorTrap(void (*function)(void *), void *parameter) override {
        return false;
    }

    const char *getMethodNameFromMetadata(CORINFO_METHOD_HANDLE ftn, const char **className, const char **namespaceName,
                                          const char **enclosingClassName) override {
        printf("getMethodNameFromMetadata not defined\r\n");
        return nullptr;
    }

    bool getSystemVAmd64PassStructInRegisterDescriptor(CORINFO_CLASS_HANDLE structHnd,
                                                       SYSTEMV_AMD64_CORINFO_STRUCT_REG_PASSING_DESCRIPTOR *structPassInRegDescPtr) override {
        return false;
    }


#ifdef WINDOWS
    void *getHelperFtn(CorInfoHelpFunc ftnNum, void **ppIndirection) override {
        static void* helperAddr = (void*)0xfefefe;
        assert(ftnNum < CORINFO_HELP_COUNT);
        switch (ftnNum){
            case CORINFO_HELP_USER_BREAKPOINT:
                helperAddr = &breakpointFtn;
                break;
            case CORINFO_HELP_NEWARR_1_VC:
                helperAddr = &newArrayHelperFtn;
                break;
            case CORINFO_HELP_ARRADDR_ST:
                helperAddr = &stArrayHelperFtn;
                break;
            case CORINFO_HELP_STACK_PROBE: 
                helperAddr = &JIT_StackProbe;
                break;
            default:
                helperAddr = &helperFtn;
                break;
        }
        *ppIndirection = &helperAddr;
        return nullptr;
    }
#else
    void* getHelperFtn(CorInfoHelpFunc ftnNum, void** ppIndirection) override {
        if (ppIndirection != nullptr)
            ppIndirection = nullptr;
        assert(ftnNum < CORINFO_HELP_COUNT);
        switch (ftnNum) {
        case CORINFO_HELP_USER_BREAKPOINT:
            return (void*)breakpointFtn;
        case CORINFO_HELP_NEWARR_1_VC:
            return (void*)newArrayHelperFtn;
        case CORINFO_HELP_ARRADDR_ST:
            return (void*)stArrayHelperFtn;
        }
        return (void*)helperFtn;
    }
#endif

    void getLocationOfThisType(CORINFO_METHOD_HANDLE context, CORINFO_LOOKUP_KIND *pLookupKind) override {

    }

    CORINFO_CLASS_HANDLE getStaticFieldCurrentClass(CORINFO_FIELD_HANDLE field, bool *pIsSpeculative) override {
        printf("getStaticFieldCurrentClass not defined\r\n");
        return nullptr;
    }

    DWORD getFieldThreadLocalStoreID(CORINFO_FIELD_HANDLE field, void **ppIndirection) override {
        return 0;
    }

    void setOverride(ICorDynamicInfo *pOverride, CORINFO_METHOD_HANDLE currentMethod) override {

    }

    void addActiveDependency(CORINFO_MODULE_HANDLE moduleFrom, CORINFO_MODULE_HANDLE moduleTo) override {

    }

    CORINFO_METHOD_HANDLE
    GetDelegateCtor(CORINFO_METHOD_HANDLE methHnd, CORINFO_CLASS_HANDLE clsHnd, CORINFO_METHOD_HANDLE targetMethodHnd,
                    DelegateCtorArgs *pCtorData) override {
        printf("GetDelegateCtor not defined\r\n");
        return nullptr;
    }

    void MethodCompileComplete(CORINFO_METHOD_HANDLE methHnd) override {

    }

    bool getTailCallHelpers(CORINFO_RESOLVED_TOKEN *callToken, CORINFO_SIG_INFO *sig,
                            CORINFO_GET_TAILCALL_HELPERS_FLAGS flags, CORINFO_TAILCALL_HELPERS *pResult) override {
        return false;
    }

    bool convertPInvokeCalliToCall(CORINFO_RESOLVED_TOKEN *pResolvedToken, bool fMustConvert) override {
        return false;
    }

    void notifyInstructionSetUsage(CORINFO_InstructionSet instructionSet, bool supportEnabled) override {

    }

    void reserveUnwindInfo(BOOL isFunclet, BOOL isColdCode, ULONG unwindSize) override {

    }

    void allocUnwindInfo(BYTE *pHotCode, BYTE *pColdCode, ULONG startOffset, ULONG endOffset, ULONG unwindSize,
                         BYTE *pUnwindBlock, CorJitFuncKind funcKind) override {
        //printf("allocUnwindInfo not implemented \r\n");
    }

    void *allocGCInfo(size_t size) override {
        return PyMem_Malloc(size);
    }

    void setEHcount(unsigned int cEH) override {
        printf("setEHcount not implemented \r\n");
    }

    void setEHinfo(unsigned int EHnumber, const CORINFO_EH_CLAUSE *clause) override {
        printf("setEHinfo not implemented \r\n");
    }

    HRESULT allocMethodBlockCounts(UINT32 count, BlockCounts **pBlockCounts) override {
        printf("allocMethodBlockCounts not implemented \r\n");
        return 0;
    }

    HRESULT getMethodBlockCounts(CORINFO_METHOD_HANDLE ftnHnd, UINT32 *pCount, BlockCounts **pBlockCounts,
                                 UINT32 *pNumRuns) override {
        printf("getMethodBlockCounts not implemented \r\n");
        return 0;
    }

    CorInfoTypeWithMod
    getArgType(CORINFO_SIG_INFO *sig, CORINFO_ARG_LIST_HANDLE args, CORINFO_CLASS_HANDLE *vcTypeRet) override {
        *vcTypeRet = nullptr;
        return (CorInfoTypeWithMod)((Parameter*)args)->m_type;
    }

    void recordCallSite(ULONG instrOffset,
                        CORINFO_SIG_INFO *callSig,
                        CORINFO_METHOD_HANDLE methodHandle) override {
    }

    void assignIL(uint8_t *il, unsigned int ilLen) {
        m_il = il;
        m_ilLen = ilLen;
    }

    void setNativeSize(ULONG i) {
        m_nativeSize = i;
    }
};

#endif
