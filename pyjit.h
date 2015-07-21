#ifndef __PYJIT_H__
#define __PYJIT_H__

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
#include <frameobject.h>
#include <opcode.h>

#include <vector>
#include <hash_map>

#include <corjit.h>
#include <utilcode.h>
#include <openum.h>

#include <Python.h>
#include <frameobject.h>
#include <opcode.h>

#include "cee.h"
#include "jitinfo.h"
#include "codemodel.h"
#include "ilgen.h"

extern "C" __declspec(dllexport) PVOID JitCompile(PyCodeObject* code);
extern "C" __declspec(dllexport) void JitInit();

extern CorJitInfo g_corJitInfo;
extern ICorJitCompiler* g_jit;

#endif