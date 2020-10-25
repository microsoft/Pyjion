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

#pragma once
#ifndef PYJION_UTIL_H
#define PYJION_UTIL_H 1

#include <Python.h>

/**
Use RAII to Py_XDECREF a pointer.

Inspired by std::unique_ptr.
*/
template <class T> class py_ptr {
private:
    T* m_ptr;

public:
    py_ptr() : m_ptr(nullptr) {}
    explicit py_ptr(T* ptr) : m_ptr(ptr) {}
    py_ptr(const py_ptr& copy) {
        m_ptr = copy.get();
        Py_INCREF(m_ptr);
    }

    ~py_ptr() {
        Py_XDECREF(m_ptr);
    }

    void reset(T* ptr) {
        Py_XDECREF(m_ptr);
        m_ptr = ptr;
    }

    T* get() const {
        return m_ptr;
    }

    T* operator->() {
        return m_ptr;
    }

    T* operator*() {
        return m_ptr;
    }
};

/**
A concrete PyObject instance of py_ptr.
*/
class PyObject_ptr : public py_ptr<PyObject> {
public:
    explicit PyObject_ptr(PyObject *ptr) : py_ptr(ptr) {}
};

#endif // !UTIL_H
