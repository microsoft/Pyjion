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

#include <catch2/catch.hpp>
#include <Python.h>
#include "intrins.h"

TEST_CASE("Test BuildDictFromTuples"){
    SECTION("Test happy path") {
        PyObject* keys_and_values = PyTuple_New(3);
        PyObject* keys = PyTuple_New(2);
        PyTuple_SetItem(keys, 0, PyUnicode_FromString("key1"));
        PyTuple_SetItem(keys, 1, PyUnicode_FromString("key2"));

        PyTuple_SetItem(keys_and_values, 0, PyUnicode_FromString("value1"));
        PyTuple_SetItem(keys_and_values, 1, PyUnicode_FromString("value2"));
        PyTuple_SetItem(keys_and_values, 2, keys);
        auto res = PyJit_BuildDictFromTuples(keys_and_values);
        CHECK(PyDict_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(PyObject_Repr(res)), Catch::Equals("{'key1': 'value1', 'key2': 'value2'}"));
    }
}