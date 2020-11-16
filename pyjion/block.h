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


#ifndef PYJION_BLOCK_H
#define PYJION_BLOCK_H

#include <cstdio>
#include <climits>

#include "ipycomp.h"
#include "flags.h"

// forward dec exception handlers.
class ExceptionHandler;

struct BlockInfo {
    int EndOffset, Kind, ContinueOffset;
    ehFlags Flags;
    ExceptionHandler* CurrentHandler;  // the current exception handler

    BlockInfo(int endOffset, int kind, ExceptionHandler* currentHandler, ehFlags flags = EhfNone, int continueOffset = 0) {
        EndOffset = endOffset;
        Kind = kind;
        Flags = flags;
        CurrentHandler = currentHandler;
        ContinueOffset = continueOffset;
    }
};

#endif //PYJION_BLOCK_H
