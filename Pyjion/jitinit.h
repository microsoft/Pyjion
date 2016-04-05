#ifndef JITINIT_H
#define JITINIT_H


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
#include <utilcode.h>
#include <openum.h>

CorJitInfo g_corJitInfo;
ICorJitCompiler* g_jit;

extern "C" __declspec(dllexport) void JitInit()

#endif