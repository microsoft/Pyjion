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

#ifndef PYJION_COWVECTOR_H
#define PYJION_COWVECTOR_H

#include <memory>
#include <vector>
#include <unordered_set>

using namespace std;

// Copy on write data implementation
template<typename T> class CowData {
    shared_ptr<T> m_data;
public:
    CowData() = default;

    explicit CowData(T *data) : m_data(data) {
    }

    explicit CowData(shared_ptr<T> data) : m_data(data) {
    }

    bool operator ==(CowData<T> other) {
        return m_data.get() == other.m_data.get();
    }

    bool operator !=(CowData<T> other) {
        return m_data.get() != other.m_data.get();
    }

protected:
    // Returns an instance of the data which isn't shared and is safe to mutate.
    T & get_mutable() {
        if (m_data.use_count() != 1) {
            m_data = shared_ptr<T>(new T(*m_data));
        }
        return *m_data;
    }

    // Returns an instance of the data which may be shared and is not safe to
    // mutate.
    T & get_current() {
        return *m_data;
    }
};

// Copy on write vector implementation
template<typename T> class CowVector : public CowData<vector<T>> {
public:
    CowVector() = default;

    explicit CowVector(size_t size) : CowData<vector<T>>(new vector<T>(size)) {
    }

    T operator[](size_t index) {
        return this->get_current()[index];
    }

    size_t size() {
        return this->get_current().size();
    }

    void replace(size_t index, T value) {
        this->get_mutable()[index] = value;
    }

    T& back() {
        return this->get_current().back();
    }

    void push_back(T value) {
        this->get_mutable().push_back(value);
    }

    void pop_back() {
        this->get_mutable().pop_back();
    }
};

template<typename T> class CowSet : public CowData<unordered_set<T>> {
    typedef typename unordered_set<T>::value_type value_type;
    typedef typename unordered_set<T>::key_type key_type;
    typedef typename unordered_set<T>::iterator iterator;
    static shared_ptr<unordered_set<T>> s_empty;
public:
    CowSet() : CowData<unordered_set<T>>(get_empty()) { } ;

    iterator find(const key_type& k) {
        return this->get_current().find(k);
    }

    size_t size() {
        return this->get_current().size();
    }

    void insert(const value_type& val) {
        this->get_mutable().insert(val);
    }

    iterator begin() {
        return this->get_current().begin();
    }
    iterator end() {
        return this->get_current().end();
    }

    CowSet<T> combine(CowSet<T> other) {
        if (other.size() == 0) {
            return *this;
        }
        else if (size() == 0) {
            return other;
        }

        CowSet<T> res = CowSet<T>();
        for (auto cur = other.begin(); cur != other.end(); cur++) {
            res.insert(*cur);
        }
        for (auto cur = begin(); cur != end(); cur++) {
            res.insert(*cur);
        }
        return res;
    }

private:
    static shared_ptr<unordered_set<T>> get_empty() {
        if (s_empty.get() == nullptr) {
            s_empty = shared_ptr<unordered_set<T>>(new unordered_set<T>());
        }
        return s_empty;
    }
};

template<typename T> shared_ptr<unordered_set<T>> CowSet<T>::s_empty;

#endif