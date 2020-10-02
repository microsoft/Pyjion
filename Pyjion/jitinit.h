#ifndef JITINIT_H
#define JITINIT_H


#define FEATURE_NO_HOST

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
#include <intrin.h>

#include <vector>

#include <corjit.h>
#include <openum.h>

ICorJitCompiler* g_jit;

extern "C" __declspec(dllexport) void JitInit()

#endif