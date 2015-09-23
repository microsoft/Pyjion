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
			else {
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
		else {
			// merging with an unknown source...
			one->escapes();
			return one;
		}
	}
	else if (two != nullptr) {
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

// BoolValue methods
AbstractValueKind BoolValue::kind() {
    return AVK_Bool;
}

AbstractValue* BoolValue::compare(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {
    if (other.Value->kind() == AVK_Bool) {
        return &Bool;
    }
    return AbstractValue::compare(selfSources, op, other);
}

AbstractValue* BoolValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* BoolValue::describe() {
    return "Bool";
}

// IntegerValue methods
AbstractValueKind IntegerValue::kind() {
    return AVK_Integer;
}

AbstractValue* IntegerValue::binary(AbstractSource*selfSources, int op, AbstractValueWithSources& other) {
    if (other.Value->kind() == AVK_Integer) {
        switch (op) {
        case BINARY_FLOOR_DIVIDE:
        case BINARY_POWER:
        case BINARY_MODULO:
        case BINARY_LSHIFT:
        case BINARY_RSHIFT:
        case BINARY_AND:
        case BINARY_XOR:
        case BINARY_OR:
        case BINARY_MULTIPLY:
        case BINARY_SUBTRACT:
        case BINARY_ADD:
        case INPLACE_POWER:
        case INPLACE_MULTIPLY:
        case INPLACE_FLOOR_DIVIDE:
        case INPLACE_MODULO:
        case INPLACE_ADD:
        case INPLACE_SUBTRACT:
        case INPLACE_LSHIFT:
        case INPLACE_RSHIFT:
        case INPLACE_AND:
        case INPLACE_XOR:
        case INPLACE_OR:
            return this;
        }
    }

    return AbstractValue::binary(selfSources, op, other);
}

AbstractValue* IntegerValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_POSITIVE:
    case UNARY_NEGATIVE:
    case UNARY_INVERT:
        return this;
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* IntegerValue::describe() {
    return "Int";
}

// StringValue methods
AbstractValueKind StringValue::kind() {
    return AVK_String;
}

AbstractValue* StringValue::binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {
    if (other.Value->kind() == AVK_String) {
        switch (op) {
        case INPLACE_ADD:
        case BINARY_ADD:
            return this;
        }
    }
    if (op == BINARY_MODULO || op == INPLACE_MODULO) {
        // Or could be an error, but that seems ok...
        return this;
    }
    else if ((op == BINARY_MULTIPLY || op == INPLACE_MULTIPLY) && other.Value->kind() == AVK_Integer) {
        return this;
    }
    return AbstractValue::binary(selfSources, op, other);
}

AbstractValue* StringValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* StringValue::describe() {
    return "String";
}

// BytesValue methods
AbstractValueKind BytesValue::kind() {
    return AVK_Bytes;
}

AbstractValue* BytesValue::binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {
    if (other.Value->kind() == AVK_Bytes) {
        switch (op) {
        case INPLACE_ADD:
        case BINARY_ADD:
            return this;
        }
    }
    if (op == BINARY_MODULO || op == INPLACE_MODULO) {
        // Or could be an error, but that seems ok...
        return this;
    }
    else if ((op == BINARY_MULTIPLY || op == INPLACE_MULTIPLY) && other.Value->kind() == AVK_Integer) {
        return this;
    }
    return AbstractValue::binary(selfSources, op, other);
}

AbstractValue* BytesValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* BytesValue::describe() {
    return "Bytes";
}

// FloatValue methods
AbstractValueKind FloatValue::kind() {
    return AVK_Float;
}
AbstractValue* FloatValue::binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {
    if (other.Value->kind() == AVK_Float) {
        switch (op) {
        case BINARY_TRUE_DIVIDE:
        case BINARY_FLOOR_DIVIDE:
        case BINARY_POWER:
        case BINARY_MODULO:
        case BINARY_LSHIFT:
        case BINARY_RSHIFT:
        case BINARY_AND:
        case BINARY_XOR:
        case BINARY_OR:
        case BINARY_MULTIPLY:
        case BINARY_SUBTRACT:
        case BINARY_ADD:
        case INPLACE_POWER:
        case INPLACE_MULTIPLY:
        case INPLACE_TRUE_DIVIDE:
        case INPLACE_FLOOR_DIVIDE:
        case INPLACE_MODULO:
        case INPLACE_ADD:
        case INPLACE_SUBTRACT:
            return this;
        }
    }
    return AbstractValue::binary(selfSources, op, other);
}

AbstractValue* FloatValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_POSITIVE:
    case UNARY_NEGATIVE:
        return this;
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* FloatValue::describe() {
    return "float";
}

// TupleValue methods
AbstractValueKind TupleValue::kind() {
    return AVK_Tuple;
}

AbstractValue* TupleValue::binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {
    if (op == BINARY_ADD && other.Value->kind() == AVK_Tuple) {
        return this;
    }
    else if (op == BINARY_MULTIPLY && other.Value->kind() == AVK_Integer) {
        return this;
    }
    else if (op == BINARY_SUBSCR && other.Value->kind() == AVK_Slice) {
        return this;
    }
    return AbstractValue::binary(selfSources, op, other);
}

AbstractValue* TupleValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* TupleValue::describe() {
    return "tuple";
}

// ListValue methods
AbstractValueKind ListValue::kind() {
    return AVK_List;
}

AbstractValue* ListValue::binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {
    if (op == BINARY_ADD && other.Value->kind() == AVK_List) {
        return this;
    }
    else if (op == BINARY_MULTIPLY && other.Value->kind() == AVK_Integer) {
        return this;
    }
    else if (op == BINARY_SUBSCR && other.Value->kind() == AVK_Slice) {
        return this;
    }
    return AbstractValue::binary(selfSources, op, other);
}

AbstractValue* ListValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* ListValue::describe() {
    return "list";
}

// DictValue methods
AbstractValueKind DictValue::kind() {
    return AVK_Dict;
}

AbstractValue* DictValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* DictValue::describe() {
    return "dict";
}

// SetValue methods
AbstractValueKind SetValue::kind() {
    return AVK_Set;
}

AbstractValue* SetValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* SetValue::describe() {
    return "set";
}

// NoneValue methods
AbstractValueKind NoneValue::kind() {
    return AVK_None;
}

AbstractValue* NoneValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* NoneValue::describe() {
    return "None";
}

// FunctionValue methods
AbstractValueKind FunctionValue::kind() {
    return AVK_Function;
}

AbstractValue* FunctionValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* FunctionValue::describe() {
    return "Function";
}

// SliceValue methods
AbstractValueKind SliceValue::kind() {
    return AVK_Slice;
}
AbstractValue* SliceValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    }
    return AbstractValue::unary(selfSources, op);
}
const char* SliceValue::describe() {
    return "Slice";
}

// ComplexValue methods
AbstractValueKind ComplexValue::kind() {
    return AVK_Complex;
}

AbstractValue* ComplexValue::binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {
    auto other_kind = other.Value->kind();
    if (other_kind == AVK_Bool) {
        switch (op) {
        case BINARY_ADD:
        case BINARY_MULTIPLY:
        case BINARY_POWER:
        case BINARY_SUBTRACT:
        case BINARY_TRUE_DIVIDE:
        case INPLACE_ADD:
        case INPLACE_MULTIPLY:
        case INPLACE_POWER:
        case INPLACE_SUBTRACT:
        case INPLACE_TRUE_DIVIDE:
            return this;
        }
    }
    else if (other_kind == AVK_Complex) {
        switch (op) {
        case BINARY_ADD:
        case BINARY_MULTIPLY:
        case BINARY_POWER:
        case BINARY_SUBTRACT:
        case BINARY_TRUE_DIVIDE:
        case INPLACE_ADD:
        case INPLACE_MULTIPLY:
        case INPLACE_POWER:
        case INPLACE_SUBTRACT:
        case INPLACE_TRUE_DIVIDE:
            return this;
        }
    }
    else if (other_kind == AVK_Float) {
        switch (op) {
        case BINARY_ADD:
        case BINARY_MULTIPLY:
        case BINARY_POWER:
        case BINARY_SUBTRACT:
        case BINARY_TRUE_DIVIDE:
        case INPLACE_ADD:
        case INPLACE_MULTIPLY:
        case INPLACE_POWER:
        case INPLACE_SUBTRACT:
        case INPLACE_TRUE_DIVIDE:
            return this;
        }
    }
    else if (other_kind == AVK_Integer) {
        switch (op) {
        case BINARY_ADD:
        case BINARY_MULTIPLY:
        case BINARY_POWER:
        case BINARY_SUBTRACT:
        case BINARY_TRUE_DIVIDE:
        case INPLACE_ADD:
        case INPLACE_MULTIPLY:
        case INPLACE_POWER:
        case INPLACE_SUBTRACT:
        case INPLACE_TRUE_DIVIDE:
            return this;
        }
    }
    return AbstractValue::binary(selfSources, op, other);
}

AbstractValue* ComplexValue::unary(AbstractSource* selfSources, int op) {
    switch (op) {
    case UNARY_NOT:
        return &Bool;
    case UNARY_NEGATIVE:
    case UNARY_POSITIVE:
        return this;
    }
    return AbstractValue::unary(selfSources, op);
}

const char* ComplexValue::describe() {
    return "complex";
}
