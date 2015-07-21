#ifndef __ILGEN_H__
#define __ILGEN_H__

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

#include <corjit.h>
#include <openum.h>

#include "codemodel.h"

using namespace std;

enum BranchType {
	BranchAlways,
	BranchTrue,
	BranchFalse,
	BranchEqual,
};


class Local {
public:
	int m_index;

	Local(int index) {
		m_index = index;
	}
};

class Label {
public:
	int m_index;
	
	Label(int index = -1) {
		m_index = index;
	}
};

class LabelInfo {
public:
	int m_location;
	vector<int> m_branchOffsets;

	LabelInfo() {
		m_location = -1;
	}
};


class ILGenerator {
	vector<Parameter> m_params, m_locals;
	CorInfoType m_retType;
	Module* m_module;
	unordered_map<CorInfoType, vector<Local>> m_freedLocals;

public:
	vector<byte> m_il;
	int m_localCount;
	vector<LabelInfo> m_labels;

public:

	ILGenerator(Module* module, CorInfoType returnType, std::vector<Parameter> params) {
		m_module = module;
		m_retType = returnType;
		m_params = params;
		m_localCount = 0;
	}

	Local define_local(Parameter param) {
		auto existing = m_freedLocals.find(param.m_type);
		if (existing != m_freedLocals.end() && existing->second.size() != 0) {
			auto res = existing->second[existing->second.size() - 1];
			existing->second.pop_back();
			return res;
		}
		m_locals.push_back(param);
		return Local(m_localCount++);
	}

	void free_local(Local local) {
		auto param = m_locals[local.m_index];
		auto existing = m_freedLocals.find(param.m_type);
		vector<Local>* localList;
		if (existing == m_freedLocals.end()) {
			m_freedLocals[param.m_type] = vector<Local>();
			localList = &(m_freedLocals.find(param.m_type)->second);
		}
		else{
			localList = &(existing->second);
		}
		localList->push_back(local);
	}

	Label define_label() {
		m_labels.push_back(LabelInfo());
		return Label((int)m_labels.size() - 1);
	}

	void mark_label(Label label) {
		auto info = &m_labels[label.m_index];
		info->m_location = (int)m_il.size();
		for (int i = 0; i < info->m_branchOffsets.size(); i++) {
			auto from = info->m_branchOffsets[i] ;	
			auto offset = info->m_location - (from + 4);		// relative to the end of the instruction

			m_il[from] = offset & 0xFF;
			m_il[from + 1] = (offset>>8) & 0xFF;
			m_il[from + 2] = (offset>>16) & 0xFF;
			m_il[from + 3] = (offset>>24) & 0xFF;
		}
	}

	void load_null() {
		m_il.push_back(CEE_LDC_I4_0);
		m_il.push_back(CEE_CONV_I);
	}

	void emit_int(int value) {
		m_il.push_back(value & 0xff);
		m_il.push_back((value >> 8) & 0xff);
		m_il.push_back((value >> 16) & 0xff);
		m_il.push_back((value >> 24) & 0xff);
	}

	void ld_i(int i) {
		m_il.push_back(CEE_LDC_I4);
		emit_int(i);
		m_il.push_back(CEE_CONV_I);
	}

	void push_back(byte b) {
		m_il.push_back(b);
	}
	
	void branch(BranchType branchType, Label label) {
		auto info = &m_labels[label.m_index];
		if (info->m_location == -1) {
			info->m_branchOffsets.push_back((int)m_il.size() + 1);
			branch(branchType, 0xFFFF);	
		}
		else{
			branch(branchType, (int)(info->m_location - m_il.size()));
		}
	}

	void branch(BranchType branchType, int offset) {
		if ((offset - 2) <= 128 && (offset - 2) >= -127) {
			switch (branchType) {
			case BranchAlways:
				m_il.push_back(CEE_BR_S);
				break;
			case BranchTrue:
				m_il.push_back(CEE_BRTRUE_S);
				break;
			case BranchFalse:
				m_il.push_back(CEE_BRFALSE_S);
				break;
			case BranchEqual:
				m_il.push_back(CEE_BEQ_S);
				break;
			}
			m_il.push_back((byte)offset - 2);
		}
		else{
			switch (branchType) {
			case BranchAlways:
				m_il.push_back(CEE_BR);
				break;
			case BranchTrue:
				m_il.push_back(CEE_BRTRUE);
				break;
			case BranchFalse:
				m_il.push_back(CEE_BRFALSE);
				break;
			case BranchEqual:
				m_il.push_back(CEE_BEQ);
				break;
			}
			emit_int(offset - 5);
		}
	}

	void dup() {
		m_il.push_back(CEE_DUP);
	}

	void pop() {
		m_il.push_back(CEE_POP);
	}

	void compare_eq() {
		m_il.push_back(CEE_PREFIX1);
		m_il.push_back((byte)CEE_CEQ);
	}

	void push_ptr(void* ptr) {
		size_t value = (size_t)ptr;
#ifdef _TARGET_AMD64_
		if ((value & 0xFFFFFFFF) == value) {
			ld_i((int)value);
		}
		else{
			m_il.push_back(CEE_LDC_I8);
			m_il.push_back(value & 0xff);
			m_il.push_back((value >> 8) & 0xff);
			m_il.push_back((value >> 16) & 0xff);
			m_il.push_back((value >> 24) & 0xff);
			m_il.push_back((value >> 32) & 0xff);
			m_il.push_back((value >> 40) & 0xff);
			m_il.push_back((value >> 48) & 0xff);
			m_il.push_back((value >> 56) & 0xff);
			m_il.push_back(CEE_CONV_I);
		}
#else
		ld_i(value);
		m_il.push_back(CEE_CONV_I);
#endif
	}

	void emit_call(int token) {
		m_il.push_back(CEE_CALL);
		emit_int(token);
	}

	void emit_calli(int token) {
		m_il.push_back(CEE_CALLI);
		emit_int(token);
	}

	void emit_callvirt(int token) {
		m_il.push_back(CEE_CALLVIRT);
		emit_int(token);
	}

	void st_loc(Local param) {
		st_loc(param.m_index);
	}

	void ld_loc(Local param) {
		ld_loc(param.m_index);
	}

	void st_loc(int index) {
		switch (index) {
		case 0: m_il.push_back(CEE_STLOC_0); break;
		case 1: m_il.push_back(CEE_STLOC_1); break;
		case 2: m_il.push_back(CEE_STLOC_2); break;
		case 3: m_il.push_back(CEE_STLOC_3); break;
		default:
			if (index < 256) {
				m_il.push_back(CEE_STLOC_S);
				m_il.push_back(index);
			}
			else{
				m_il.push_back(CEE_PREFIX1);
				m_il.push_back((byte)CEE_STLOC);
				m_il.push_back(index & 0xff);
				m_il.push_back((index >> 8) & 0xff);
			}
		}
	}

	void ld_loc(int index) {
		switch (index) {
		case 0: m_il.push_back(CEE_LDLOC_0); break;
		case 1: m_il.push_back(CEE_LDLOC_1); break;
		case 2: m_il.push_back(CEE_LDLOC_2); break;
		case 3: m_il.push_back(CEE_LDLOC_3); break;
		default:
			if (index < 256) {
				m_il.push_back(CEE_LDLOC_S);
				m_il.push_back(index);
			}
			else{
				m_il.push_back(CEE_PREFIX1);
				m_il.push_back((byte)CEE_LDLOC);
				m_il.push_back(index & 0xff);
				m_il.push_back((index >> 8) & 0xff);
			}
		}
	}

	CORINFO_METHOD_INFO to_method(Method* addr) {
		CORINFO_METHOD_INFO methodInfo;
		methodInfo.ftn = (CORINFO_METHOD_HANDLE)addr;
		methodInfo.scope = (CORINFO_MODULE_HANDLE)m_module;
		methodInfo.ILCode = &m_il[0];
		methodInfo.ILCodeSize = (unsigned int)m_il.size();
		methodInfo.maxStack = 100;
		methodInfo.EHcount = 0;
		methodInfo.options = CORINFO_OPT_INIT_LOCALS;
		methodInfo.regionKind = CORINFO_REGION_JIT;
		methodInfo.args = CORINFO_SIG_INFO{ CORINFO_CALLCONV_DEFAULT };
		methodInfo.args.args = (CORINFO_ARG_LIST_HANDLE)(m_params.size() == 0 ? nullptr : &m_params[0]);
		methodInfo.args.numArgs = m_params.size();
		methodInfo.args.retType = m_retType;
		methodInfo.args.retTypeClass = nullptr;
		methodInfo.locals = CORINFO_SIG_INFO{ CORINFO_CALLCONV_DEFAULT };
		methodInfo.locals.args = (CORINFO_ARG_LIST_HANDLE)(m_locals.size() == 0 ? nullptr : &m_locals[0]);
		methodInfo.locals.numArgs = m_locals.size();
		return methodInfo;
	}

	Method compile(ICorJitInfo* jitInfo, ICorJitCompiler* jit) {
		BYTE* nativeEntry;
		ULONG nativeSizeOfCode;
		auto res = Method(m_module, m_retType, m_params, nullptr);
		CORINFO_METHOD_INFO methodInfo = to_method(&res);
		CorJitResult result = jit->compileMethod(
			/*ICorJitInfo*/jitInfo,
			/*CORINFO_METHOD_INFO */&methodInfo,
			/*flags*/0,
			&nativeEntry,
			&nativeSizeOfCode
		);
		res.m_addr = nativeEntry;
		return res;
	}
};

#endif