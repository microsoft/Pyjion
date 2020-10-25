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

#ifndef TAGGED_PTR_H
#define TAGGED_PTR_H

#include <Python.h>

typedef ssize_t tagged_ptr;

#ifdef _TARGET_AMD64_
#define NEW_LONG PyLong_FromLongLong
#define AS_LONG_AND_OVERFLOW PyLong_AsLongLongAndOverflow
#else
#define NEW_LONG PyLong_FromLong
#define AS_LONG_AND_OVERFLOW PyLong_AsLongAndOverflow
#endif

#define MAX_BITS ((sizeof(tagged_ptr) * 8) - 1)

#define MAX_TAGGED_VALUE (((tagged_ptr)1<<(MAX_BITS-1)) - 1)
#define MIN_TAGGED_VALUE (- ((tagged_ptr)1 << (MAX_BITS-1)))

inline bool can_tag(tagged_ptr value) {
    return value >= MIN_TAGGED_VALUE && value <= MAX_TAGGED_VALUE;
}

#define TAG_IT(x) ((PyObject*) (((x) << 1) | 0x01))
#define UNTAG_IT(x) ((x) >> 1)
#define IS_TAGGED(x) ((x) & 0x01)

// Gets the number of "digits" (as defined by the Python long object) that fit into our tagged pointers
#define DIGITS_IN_TAGGED_PTR ((MAX_BITS + PYLONG_BITS_IN_DIGIT - 1) / PYLONG_BITS_IN_DIGIT)
// Gets the size of a PyNumber object to be allocated on the stack that can hold our tagged pointer
#define NUMBER_SIZE (((sizeof(PyVarObject) + sizeof(PY_UINT32_T) * DIGITS_IN_TAGGED_PTR) / sizeof(size_t)) + sizeof(size_t))
#define INIT_TMP_NUMBER(name, value) \
	size_t tmp_##name[NUMBER_SIZE];  \
	PyObject* name = init_number(tmp_##name, value);
#endif