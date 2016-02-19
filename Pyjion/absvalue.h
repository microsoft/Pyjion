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

#ifndef ABSVALUE_H
#define ABSVALUE_H

#include <python.h>
#include <opcode.h>
#include "cowvector.h"

class AbstractValue;
struct AbstractValueWithSources;
class LocalSource;
struct AbstractSources;
class AbstractSource;

enum AbstractValueKind {
    AVK_Any,
    AVK_Undefined,
    AVK_Integer,
    AVK_Float,
    AVK_Bool,
    AVK_List,
    AVK_Dict,
    AVK_Tuple,
    AVK_Set,
    AVK_String,
    AVK_Bytes,
    AVK_None,
    AVK_Function,
    AVK_Slice,
    AVK_Complex
};

static bool is_known_type(AbstractValueKind kind) {
    switch (kind) {
        case AVK_Integer:
        case AVK_Float:
        case AVK_Bool:
        case AVK_List:
        case AVK_Dict:
        case AVK_Tuple:
        case AVK_Set:
        case AVK_String:
        case AVK_Bytes:
        case AVK_None:
        case AVK_Function:
        case AVK_Slice:
        case AVK_Complex:
            return true;
    }
    return false;
}


class AbstractSource {
public:
    shared_ptr<AbstractSources> Sources;

    AbstractSource();

    void escapes();

    bool needs_boxing();

    virtual char* describe() {
        return "unknown source";
    }

    static AbstractSource* combine(AbstractSource* one, AbstractSource*two);
};

struct AbstractSources {
    unordered_set<AbstractSource*> Sources;
    bool m_escapes;

    AbstractSources() {
        Sources = unordered_set<AbstractSource*>();
        m_escapes = false;
    }
    void escapes() {
        m_escapes = true;
    }

    bool needs_boxing() {
        return m_escapes;
    }

};

class ConstSource : public AbstractSource {
public:
    virtual char* describe() {
        if (needs_boxing()) {
            return "Source: Const (escapes)";
        }
        else {
            return "Source: Const";
        }
    }
};

class LocalSource : public AbstractSource {
public:
    virtual char* describe() {
        if (needs_boxing()) {
            return "Source: Local (escapes)";
        }
        else {
            return "Source: Local";
        }
    }
};

class IntermediateSource : public AbstractSource {
public:
    virtual char* describe() {
        if (needs_boxing()) {
            return "Source: Intermediate (escapes)";
        }
        else {
            return "Source: Intermediate";
        }
    }
};

/*
class TupleSource : public AbstractSource {
    vector<AbstractValueWithSources> m_sources;
    bool m_escapes;
public:
    TupleSource(vector<AbstractValueWithSources> sources) {
        m_sources = sources;
    }

    virtual void escapes();

    virtual char* describe() {
        if (m_escapes) {
            return "Source: Tuple (escapes)";
        }
        else{
            return "Source: Tuple";
        }
    }
};*/


class AbstractValue {
public:
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* compare(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual void truth(AbstractSource* selfSources);

    virtual bool is_always_true() {
        return false;
    }
    virtual bool is_always_false() {
        return false;
    }
    virtual AbstractValue* merge_with(AbstractValue*other);
    virtual AbstractValueKind kind() = 0;
    virtual const char* describe() {
        return "";
    }

};

struct AbstractValueWithSources {
    AbstractValue* Value;
    AbstractSource* Sources;

    AbstractValueWithSources(AbstractValue *type = nullptr) {
        Value = type;
        Sources = nullptr;
    }

    AbstractValueWithSources(AbstractValue *type, AbstractSource* source) {
        Value = type;
        Sources = source;
    }

    void escapes() {
        if (Sources != nullptr) {
            Sources->escapes();
        }
    }

    bool needs_boxing() {
        if (Sources != nullptr) {
            return Sources->needs_boxing();
        }
        return true;
    }

    AbstractValueWithSources merge_with(AbstractValueWithSources other) {
        // TODO: Is defining a new source at the merge point better?

        auto newValue = Value->merge_with(other.Value);
        if ((newValue->kind() != Value->kind() && Value->kind() != AVK_Undefined) ||
            (newValue->kind() != other.Value->kind() && other.Value->kind() != AVK_Undefined)) {
            if (Sources != nullptr) {
                Sources->escapes();
            }
            if (other.Sources != nullptr) {
                other.Sources->escapes();
            }
        }
        return AbstractValueWithSources(
            Value->merge_with(other.Value),
            AbstractSource::combine(Sources, other.Sources)
            );
    }

    bool operator== (AbstractValueWithSources& other) {
        if (Value != other.Value) {
            return false;
        }

        if (Sources == nullptr) {
            return other.Sources == nullptr;
        }
        else if (other.Sources == nullptr) {
            return false;
        }

        return Sources->Sources.get() == other.Sources->Sources.get();
    }

    bool operator!= (AbstractValueWithSources& other) {
        return !(*this == other);
    }
};

class AnyValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_Any;
    }
    virtual const char* describe() {
        return "Any";
    }
};

class UndefinedValue : public AbstractValue {
    virtual AbstractValue* merge_with(AbstractValue*other) {
        return other;
    }
    virtual AbstractValueKind kind() {
        return AVK_Undefined;
    }
    virtual const char* describe() {
        return "Undefined";
    }
};

class BoolValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
    virtual void truth(AbstractSource* selfSources);
};

class BytesValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};

class ComplexValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};

class IntegerValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource*selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
    virtual void truth(AbstractSource* sources);
};

class StringValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};

class FloatValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual void truth(AbstractSource* selfSources);

    virtual const char* describe();
};

class TupleValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};

class ListValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};

class DictValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};

class SetValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};

class NoneValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};

class FunctionValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};

class SliceValue : public AbstractValue {
    virtual AbstractValueKind kind();
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual const char* describe();
};


extern UndefinedValue Undefined;
extern AnyValue Any;
extern BoolValue Bool;
extern IntegerValue Integer;
extern FloatValue Float;
extern ListValue List;
extern TupleValue Tuple;
extern SetValue Set;
extern StringValue String;
extern BytesValue Bytes;
extern DictValue Dict;
extern NoneValue None;
extern FunctionValue Function;
extern SliceValue Slice;
extern ComplexValue Complex;

#endif

