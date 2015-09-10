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

AbstractSource::AbstractSource() {
    Sources = shared_ptr<AbstractSources>(new AbstractSources());
    Sources->Sources.insert(this);
}

AbstractValue* AbstractValue::binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {
    if (selfSources != nullptr) {
        selfSources->escapes();
    }
    other.escapes();

    return &Any;
}

AbstractValue* AbstractValue::unary(AbstractSource* selfSources, int op) {
    if (selfSources != nullptr) {
        selfSources->escapes();
    }
    return &Any;
}

AbstractValue* AbstractValue::compare(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {
    if (is_known_type(kind()) && is_known_type(other.Value->kind()) && kind() == other.Value->kind()) {
        // We know all of the known types don't have fancy rich comparison
        // operations and will return true/false.  This is in contrast to
        // user defined types which can override the rich comparison methods
        // and return values which are not bools.
        return &Bool;
    }

    if (selfSources != nullptr) {
        selfSources->escapes();
    }
    other.escapes();
    return &Any;
}

AbstractValue* AbstractValue::merge_with(AbstractValue*other) {
    if (this == other) {
        return this;
    }
    return &Any;
}

void AbstractSource::escapes() {
    if (Sources) {
        Sources->m_escapes = true;
    }
}

bool AbstractSource::needs_boxing() {
    if (Sources) {
        return Sources->m_escapes;
    }
    return true;
}

AbstractSource* AbstractSource::combine(AbstractSource* one, AbstractSource* two) {
    if (one == two) {
        return one;
    }
    if (one != nullptr) {
        if (two != nullptr) {
            if (one->Sources.get() == two->Sources.get()) {
                return one;
            }

            // link the sources...
            if (one->Sources->Sources.size() > two->Sources->Sources.size()) {
                for (auto source : two->Sources->Sources) {
                    one->Sources->Sources.insert(source);
                    if (source != two) {
                        source->Sources = one->Sources;
                    }
                }
                two->Sources = one->Sources;
                return one;
            }
            else{
                for (auto source : one->Sources->Sources) {
                    two->Sources->Sources.insert(source);
                    if (source != one) {
                        source->Sources = two->Sources;
                    }
                }
                one->Sources = two->Sources;
                return two;
            }
        }
        else{
            // merging with an unknown source...
            one->escapes();
            return one;
        }
    }
    else if(two != nullptr) {
        // merging with an unknown source...
        two->escapes();
        return two;
    }
    return nullptr;
}
/*
void TupleSource::escapes() {
    for (auto cur = m_sources.begin(); cur != m_sources.end(); cur++) {
        (*cur).escapes();
    }
    m_escapes = true;
}*/
