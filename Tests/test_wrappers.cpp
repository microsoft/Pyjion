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

TEST_CASE("Test Add"){
    SECTION("Test add two numbers") {
        PyObject* left = PyLong_FromLong(200);
        PyObject* right = PyLong_FromLong(304);

        auto res = PyJit_Add(left, right);
        CHECK(PyNumber_Check(res));
        CHECK(PyLong_AsDouble(res) == 504);
    }

     SECTION("Test add two strings") {
        PyObject* left = PyUnicode_FromString("horse");
        PyObject* right = PyUnicode_FromString("staple");

        auto res = PyJit_Add(left, right);
        CHECK(PyUnicode_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(res), Catch::Equals("horsestaple"));
    }
}

TEST_CASE("Test Subscr"){
    SECTION("Test subscr list") {
        PyObject* left = PyList_New(0);
        PyList_Append(left, PyUnicode_FromString("horse"));
        PyList_Append(left, PyUnicode_FromString("battery"));
        PyList_Append(left, PyUnicode_FromString("staple"));
        PyObject* index = PyLong_FromLong(2);

        auto res = PyJit_Subscr(left, index);
        CHECK(PyUnicode_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(res), Catch::Equals("staple"));
    }

    SECTION("Test subscr dict") {
        PyObject* left = PyDict_New();
        PyDict_SetItem(left, PyLong_FromLong(0), PyUnicode_FromString("horse"));
        PyDict_SetItem(left, PyLong_FromLong(1), PyUnicode_FromString("battery"));
        PyDict_SetItem(left, PyLong_FromLong(2), PyUnicode_FromString("staple"));
        PyObject* right = PyLong_FromLong(2);

        auto res = PyJit_Subscr(left, right);
        CHECK(PyUnicode_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(res), Catch::Equals("staple"));
    }
}


TEST_CASE("Test RichCompare"){
    SECTION("Test string does not equal") {
        PyObject* left = PyUnicode_FromString("horse");
        PyObject* right = PyUnicode_FromString("battery");

        auto res = PyJit_RichCompare(left, right, Py_EQ);
        CHECK(PyBool_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(PyObject_Repr(res)), Catch::Equals("False"));
    }
}

TEST_CASE("Test Contains"){
    SECTION("Test word contains letter") {
        PyObject* left = PyUnicode_FromString("o");
        PyObject* right = PyUnicode_FromString("horse");

        auto res = PyJit_Contains(left, right);
        CHECK(PyBool_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(PyObject_Repr(res)), Catch::Equals("True"));
    }

    SECTION("Test word does not contain other word") {
        PyObject* left = PyUnicode_FromString("pig");
        PyObject* right = PyUnicode_FromString("horse");

        auto res = PyJit_Contains(left, right);
        CHECK(PyBool_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(PyObject_Repr(res)), Catch::Equals("False"));
    }
}

TEST_CASE("Test NotContains"){
    SECTION("Test word does not contain letter") {
        PyObject* left = PyUnicode_FromString("pig");
        PyObject* right = PyUnicode_FromString("horse");

        auto res = PyJit_NotContains(left, right);
        CHECK(PyBool_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(PyObject_Repr(res)), Catch::Equals("True"));
    }

    SECTION("Test word does not contain letter") {
        PyObject* left = PyUnicode_FromString("horse");
        PyObject* right = PyUnicode_FromString("horseback");

        auto res = PyJit_NotContains(left, right);
        CHECK(PyBool_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(PyObject_Repr(res)), Catch::Equals("False"));
    }
}

TEST_CASE("Test BuildDictFromTuples"){
    SECTION("Test happy path") {
        PyObject* keysAndValues = PyTuple_New(3);
        PyObject* keys = PyTuple_New(2);
        PyTuple_SetItem(keys, 0, PyUnicode_FromString("key1"));
        PyTuple_SetItem(keys, 1, PyUnicode_FromString("key2"));

        PyTuple_SetItem(keysAndValues, 0, PyUnicode_FromString("value1"));
        PyTuple_SetItem(keysAndValues, 1, PyUnicode_FromString("value2"));
        PyTuple_SetItem(keysAndValues, 2, keys);
        auto res = PyJit_BuildDictFromTuples(keysAndValues);
        CHECK(PyDict_Check(res));
        CHECK_THAT(PyUnicode_AsUTF8(PyObject_Repr(res)), Catch::Equals("{'key1': 'value1', 'key2': 'value2'}"));
    }
}