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


AbstractValue* AbstractValue::binary(int op, AbstractValue* other) {
    return &Any;
}

AbstractValue* AbstractValue::unary(int op) {
    return &Any;
}

AbstractValue* AbstractValue::merge_with(AbstractValue*other) {
    if (this == other) {
        return this;
    }
    return &Any;
}
