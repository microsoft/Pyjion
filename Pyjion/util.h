#pragma once
#ifndef UTIL_H
#define UTIL_H 1

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
    py_ptr(T* ptr) : m_ptr(ptr) {}
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

    T* get() {
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
    PyObject_ptr(PyObject *ptr) : py_ptr(ptr) {}
};

#endif // !UTIL_H
