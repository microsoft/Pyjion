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

#include "absvalue.h"

AnyValue Any;
UndefinedValue Undefined;
IntegerValue Integer;
FloatValue Float;
BoolValue Bool;
ListValue List;
TupleValue Tuple;
SetValue Set;
StringValue String;
BytesValue Bytes;
DictValue Dict;
NoneValue None;
FunctionValue Function;
SliceValue Slice;
ComplexValue Complex;


AbstractValue* AbstractValue::binary(AbstractSources& selfSources, int op, AbstractValueWithSources& other) {
    selfSources.escapes();
    other.escapes();

    return &Any;
}

AbstractValue* AbstractValue::unary(AbstractSources& selfSources, int op) {
    selfSources.escapes();
    return &Any;
}

AbstractValue* AbstractValue::compare(AbstractSources& selfSources, int op, AbstractValueWithSources& other) {
    if (is_known_type(kind()) && is_known_type(other.Value->kind())) {
        // We know all of the known types don't have fancy rich comparison
        // operations and will return true/false.  This is in contrast to
        // user defined types which can override the rich comparison methods
        // and return values which are not bools.
        return &Bool;
    }

    selfSources.escapes();
    other.escapes();
    return &Any;
}

AbstractValue* AbstractValue::merge_with(AbstractValue*other) {
    if (this == other) {
        return this;
    }
    return &Any;
}

/*
void TupleSource::escapes() {
    for (auto cur = m_sources.begin(); cur != m_sources.end(); cur++) {
        (*cur).escapes();
    }
    m_escapes = true;
}*/

void AbstractSources::escapes() {
    for (auto value : Sources) {
        value->escapes();
    }
}

bool AbstractSources::needs_boxing() {
    for (auto value : Sources) {
        if (value->needs_boxing()) {
            return true;
        }
    }
    return false;
}

bool AbstractSources::contains(AbstractSource* source) {
    for (auto cur : Sources) {
        if (cur->contains(source)) {
            return true;
        }
    }
    return false;
}

void AbstractSources::insert(AbstractSource* source) {
    // Once we're unknown we don't need to insert...
    if (Sources != *UnknownSource::set()) {
        
        Sources.insert(source);
    }
    else{
        // but we do need to mark this source as escaping...
        source->escapes();
    }
}

AbstractSources AbstractSources::combine(AbstractSources other) {
    if (this->Sources == *UnknownSource::set()) {
        other.escapes();
        return *this;
    }
    else if (other.Sources == *UnknownSource::set()) {
        this->escapes();
        return other;
    }

    if (needs_boxing()) {
        // we escape and are operating against other, so we'll need boxing too
        other.escapes();
        return AbstractSources(*UnknownSource::set());
    }
    else if (other.needs_boxing()) {
        // other escapes, and it's operating against us, so we'll need boxing too
        escapes();
        return AbstractSources(*UnknownSource::set());
    }
    return AbstractSources(Sources.combine(other.Sources));
}

bool AbstractSources::operator== (AbstractSources& other) {
    // quick identity check...
    if (Sources == other.Sources) {
        return true;
    }
    // quick size check...
    if (Sources.size() != other.Sources.size()) {
        return false;
    }

    // expensive set comparison...
    for (auto cur = other.Sources.begin(); cur != other.Sources.end(); cur++) {
        if (Sources.find(*cur) == Sources.end()) {
            return false;
        }
    }
    return true;
}

bool AbstractSources::operator != (AbstractSources& other) {
    return !(*this == other);
}


UnknownSource UnknownSource::g_unknownSource;
CowSet<AbstractSource*>* UnknownSource::g_unknownSourceSet;
