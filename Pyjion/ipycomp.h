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

#ifndef IPYCOMP_H
#define IPYCOMP_H


class Local {
public:
    int m_index;

    Local(int index = -1) {
        m_index = index;
    }

    bool is_valid() {
        return m_index != -1;
    }
};

class Label {
public:
    int m_index;

    Label(int index = -1) {
        m_index = index;
    }
};

enum BranchType {
    BranchAlways,
    BranchTrue,
    BranchFalse,
    BranchEqual,
    BranchNotEqual,
    BranchLeave,
};

enum CompareType {
	CT_Equal,
	CT_LessThan,
	CT_LessThanEqual,
	CT_NotEqual,
	CT_GreaterThan,
	CT_GreaterThanEqual,
};

class Method;
class IMethod;

enum LocalKind {
	LK_Pointer,
	LK_Float,
	LK_Int,
	LK_Bool,
	LK_Void
};

class IModule {
public:
	virtual IMethod* ResolveMethod(int tokenId) = 0;

	virtual int ResolveMethodToken(void* addr) = 0;

	virtual ~IModule() {
	};
};

class Parameter {
public:
	LocalKind m_type;
	Parameter(LocalKind type) {
		m_type = type;
	}
};

class IMethod {
public:
	virtual unsigned int get_param_count() = 0;
	virtual Parameter* get_params() = 0;
	virtual LocalKind get_return_type() = 0;
	virtual IModule* get_module() = 0;
	
	virtual const char* get_name() {
		return nullptr;
	}

	virtual void* get_addr() {
		return nullptr;
	}

	virtual void* get_indirect_addr() {
		return nullptr;
	}

	virtual ~IMethod() {
	};
	//    virtual void findSig(CORINFO_SIG_INFO  *sig) = 0;
};

// Not currently used...
/*
struct VTableInfo {
int indirections;
SIZE_T                  offsets[256];
};

class VirtualMethod : public Method {
VTableInfo* m_vtableInfo;
VirtualMethod(Module* module, CorInfoType returnType, std::vector<Parameter> params, void* addr, VTableInfo* vtableInfo) :
Method(module, returnType, params, addr),
m_vtableInfo(vtableInfo) {
}

virtual void get_call_info(CORINFO_CALL_INFO *pResult) {
pResult->codePointerLookup.lookupKind.needsRuntimeLookup = true;
pResult->codePointerLookup.lookupKind.runtimeLookupKind = CORINFO_LOOKUP_THISOBJ;
pResult->codePointerLookup.runtimeLookup.testForNull = false;
pResult->codePointerLookup.runtimeLookup.testForFixup = false;
//pResult->codePointerLookup.runtimeLookup.helper = CORINFO_HELP_UNDEF;
//pResult->codePointerLookup.runtimeLookup.indirections = method->m_vtableInfo->indirections;
//pResult->codePointerLookup.runtimeLookup.offsets[0] = method->m_vtableInfo->offsets[0];
//pResult->codePointerLookup.runtimeLookup.offsets[1] = method->m_vtableInfo->offsets[1];
//pResult->codePointerLookup.runtimeLookup.offsets[2] = method->m_vtableInfo->offsets[2];
//pResult->codePointerLookup.runtimeLookup.offsets[3] = method->m_vtableInfo->offsets[3];

pResult->verMethodFlags = pResult->methodFlags = CORINFO_FLG_VIRTUAL;
pResult->kind = CORINFO_VIRTUALCALL_VTABLE;
pResult->methodFlags = CORINFO_FLG_VIRTUAL;
}

virtual DWORD get_method_attrs() {
return CORINFO_FLG_NOSECURITYWRAP | CORINFO_FLG_VIRTUAL | CORINFO_FLG_NATIVE;
}
};
*/

class JittedCode {
public:
    virtual ~JittedCode() {
    }
    virtual void* get_code_addr() = 0;

};

// Defines the interface between the abstract compiler and code generator
//
// The compiler is stack based, various operations can push and pop values from the stack.
// The compiler supports defining locals, labels, performing branches, etc...  Ultimately it
// will support a wide array of primitive operations, but also support higher level Python
// operations.
class IPythonCompiler {
public:

    /*****************************************************
     * Primitives */

     // Defines a label that can be branched to and marked at some point
    virtual Label emit_define_label() = 0;
    // Marks the location of a label at the current code offset
    virtual void emit_mark_label(Label label) = 0;
    // Emits a branch to the specified label 
    virtual void emit_branch(BranchType branchType, Label label) = 0;
    // Compares if the last two values pushed onto the stack are equal
    virtual void emit_compare_equal() = 0;

    // Emits an unboxed bool onto the stack
    virtual void emit_bool(bool value) = 0;
    // Emits a pointer value onto the stack
    virtual void emit_ptr(void* value) = 0;
	virtual void emit_ptr(size_t value) = 0;
    // Emits a null pointer onto the stack
    virtual void emit_null() = 0;

    // Pops a value off the stack, performing no operations related to reference counting
    virtual void emit_pop() = 0;
    // Dups the current value on the stack, performing no operations related to reference counting
    virtual void emit_dup() = 0;

    /*****************************************************
     * Stack based locals */
     // Stores the top stack value into a local (only supports pointer types)
    virtual Local emit_spill() = 0;
    // Stores the top value into a local
    virtual void emit_store_local(Local local) = 0;
    // Loads the local onto the top of the stack
    virtual void emit_load_local(Local local) = 0;
    // Loads the address of a local onto the top of the stack
    virtual void emit_load_local_addr(Local local) = 0;
    // Loads a local onto the stack and makes the local available for re-use
    virtual void emit_load_and_free_local(Local local) = 0;
    // Defines a pointer local, optionally not pulling it from the cache of available locals
    virtual Local emit_define_local(bool cache) = 0;
    // Defines a local of a specific type
    virtual Local emit_define_local(LocalKind kind = LK_Pointer) = 0;
    // Frees a local making it available for re-use
    virtual void emit_free_local(Local local) = 0;
    // Creates an array on the stack for temporary storage
    virtual Local emit_allocate_stack_array(size_t elements) = 0;

	virtual void emit_float(double value) = 0;
	virtual void emit_int(int value) = 0;
	virtual void emit_add() = 0;
	virtual void emit_load_array(int index) = 0;
	virtual void emit_load_indirect_ptr() = 0;
	virtual void emit_store_indirect_ptr() = 0;
	virtual void emit_load_indirect_double() = 0;
	virtual void emit_load_indirect_int32() = 0;
	virtual void emit_store_indirect_int32() = 0;

	virtual void emit_negate() = 0;
	virtual void emit_divide() = 0;
	virtual void emit_multiply() = 0;
	virtual void emit_subtract() = 0;

	virtual void emit_store_to_array(Local array, int index) = 0;
    // Returns from the current function
    virtual void emit_ret() = 0;
       
    // Performs a comparison of two unboxed floating point values on the stack
    virtual void emit_compare_float(CompareType compareType) = 0;

	virtual void emit_store_int32() = 0;
	virtual void emit_load_arg(int arg) = 0;
	virtual void emit_bitwise_and() = 0;
	/* Compiles the generated code */
    virtual JittedCode* emit_compile() = 0;

	// Allows passing any function delegate w/o implicit conversion
	template<typename T> inline void emit_call(T* func) {
		emit_call((void*)func);
	}
	virtual void emit_call(void* func) = 0;
	virtual void emit_call(int token) = 0;
};

typedef IPythonCompiler* (CompilerFactory)(IMethod* method);


#endif