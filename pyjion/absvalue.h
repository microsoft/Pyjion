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

#ifndef PYJION_ABSVALUE_H
#define PYJION_ABSVALUE_H

#include <Python.h>
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

static bool isKnownType(AbstractValueKind kind) {
    // TODO : AVK_Any and AVK_Undefined are missing from here, dont know if this was intentional?
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

    void escapes() const;

    bool needsBoxing() const;

    virtual const char* describe() {
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

    bool needsBoxing() const {
        return m_escapes;
    }

};

class ConstSource : public AbstractSource {
public:
    const char* describe() override {
        if (needsBoxing()) {
            return "Source: Const (escapes)";
        }
        else {
            return "Source: Const";
        }
    }
};

class LocalSource : public AbstractSource {
public:
    const char* describe() override {
        if (needsBoxing()) {
            return "Source: Local (escapes)";
        }
        else {
            return "Source: Local";
        }
    }
};

class IntermediateSource : public AbstractSource {
public:
    const char* describe() override {
        if (needsBoxing()) {
            return "Source: Intermediate (escapes)";
        }
        else {
            return "Source: Intermediate";
        }
    }
};


class AbstractValue {
public:
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* compare(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual void truth(AbstractSource* selfSources);

    virtual bool isAlwaysTrue() {
        return false;
    }
    virtual bool isAlwaysFalse() {
        return false;
    }
    virtual AbstractValue* mergeWith(AbstractValue*other);
    virtual AbstractValueKind kind() = 0;
    virtual const char* describe() {
        return "";
    }

};

struct AbstractValueWithSources {
    AbstractValue* Value;
    AbstractSource* Sources;

    AbstractValueWithSources(AbstractValue *type = nullptr) { // NOLINT(google-explicit-constructor)
        Value = type;
        Sources = nullptr;
    }

    AbstractValueWithSources(AbstractValue *type, AbstractSource* source) {
        Value = type;
        Sources = source;
    }

    void escapes() const {
        if (Sources != nullptr) {
            Sources->escapes();
        }
    }

    bool needsBoxing() const {
        if (Sources != nullptr) {
            return Sources->needsBoxing();
        }
        return true;
    }

    AbstractValueWithSources mergeWith(AbstractValueWithSources other) const {
        // TODO: Is defining a new source at the merge point better?

        auto newValue = Value->mergeWith(other.Value);
        if ((newValue->kind() != Value->kind() && Value->kind() != AVK_Undefined) ||
            (newValue->kind() != other.Value->kind() && other.Value->kind() != AVK_Undefined)) {
            if (Sources != nullptr) {
                Sources->escapes();
            }
            if (other.Sources != nullptr) {
                other.Sources->escapes();
            }
        }
        return {
                Value->mergeWith(other.Value),
            AbstractSource::combine(Sources, other.Sources)
            };
    }

    bool operator== (AbstractValueWithSources& other) const {
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

    bool operator!= (AbstractValueWithSources& other) const {
        return !(*this == other);
    }
};

class AnyValue : public AbstractValue {
    AbstractValueKind kind() override {
        return AVK_Any;
    }
    const char* describe() override {
        return "Any";
    }
};

class UndefinedValue : public AbstractValue {
    AbstractValue* mergeWith(AbstractValue*other) override {
        return other;
    }
    AbstractValueKind kind() override {
        return AVK_Undefined;
    }
    const char* describe() override {
        return "Undefined";
    }
};

class BoolValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
    void truth(AbstractSource* selfSources) override;
};

class BytesValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
};

class ComplexValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
};

class IntegerValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* binary(AbstractSource*selfSources, int op, AbstractValueWithSources& other) override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
    void truth(AbstractSource* sources) override;
};

class StringValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
};

class FloatValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    void truth(AbstractSource* selfSources) override;
    const char* describe() override;
};

class TupleValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
};

class ListValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
};

class DictValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
};

class SetValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
};

class NoneValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
};

class FunctionValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
};

class SliceValue : public AbstractValue {
    AbstractValueKind kind() override;
    AbstractValue* unary(AbstractSource* selfSources, int op) override;
    const char* describe() override;
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

