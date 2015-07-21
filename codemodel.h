#ifndef __CODEMODEL_H__
#define __CODEMODEL_H__

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

#include <corjit.h>

using namespace std;

class Method;

class Module {
public:
	hash_map<int, Method> m_methods;
	Module() {
	}

	Method* ResolveMethod(int tokenId) {
		return &m_methods[tokenId];
	}
};

class Parameter {
public:
	CorInfoType m_type;
	Parameter(CorInfoType type) {
		m_type = type;
	}
};


class Method {
	vector<BYTE> m_il;
	Module* m_module;
public:
	vector<Parameter> m_params, m_locals;
	CorInfoType m_retType;
	void* m_addr;
public:
	Method() {
	}

	Method(Module* module, CorInfoType returnType, std::vector<Parameter> params, void* addr) {
		m_retType = returnType;
		m_params = params;
		m_module = module;
		m_addr = addr;
	}
};

#endif