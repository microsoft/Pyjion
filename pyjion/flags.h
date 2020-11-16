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

#ifndef PYJION_FLAGS_H
#define PYJION_FLAGS_H

enum ehFlags {
    EhfNone = 0,
    // The exception handling block includes a continue statement
    EhfBlockContinues = 0x01,
    // The exception handling block includes a return statement
    EhfBlockReturns = 0x02,
    // The exception handling block includes a break statement
    EhfBlockBreaks = 0x04,
    // The exception handling block is in the try portion of a try/finally
    EhfTryFinally = 0x08,
    // The exception handling block is in the try portion of a try/except
    EhfTryExcept = 0x10,
    // The exception handling block is in the finally or except portion of a try/finally or try/except
    EhfInExceptHandler = 0x20,
};

#endif //PYJION_FLAGS_H
