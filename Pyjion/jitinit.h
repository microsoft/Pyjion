#ifndef PYJION_JITINIT_H
#define PYJION_JITINIT_H


#define FEATURE_NO_HOST

#include <cstdint>
#include <windows.h>
#include <cwchar>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <cfloat>
#include <share.h>
#include <intrin.h>

#include <vector>

#include <corjit.h>
#include <openum.h>

ICorJitCompiler* g_jit;

extern "C" __declspec(dllexport) void JitInit()

#endif