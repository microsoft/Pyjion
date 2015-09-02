#ifndef __ABSINT_H__
#define __ABSINT_H__

#include <Python.h>
#include <opcode.h>
#include <vector>
#include <unordered_map>
using namespace std;

class AbstractValue;
class AnyValue;

extern AnyValue Any;

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
};

class AbstractValue {
public:
    virtual AbstractValue* binary(int op, AbstractValue* other);

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

class IntegerValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_Integer;
    }
    virtual AbstractValue* binary(int op, AbstractValue* other) {
        if (other->kind() == AVK_Integer) {
            switch (op) {
                case BINARY_ADD: 
                case BINARY_SUBTRACT: 
                case BINARY_MULTIPLY: 
                case BINARY_MODULO: 
                    return this;
            }
        }
        return &Any;
    }


    virtual const char* describe() {
        return "Int";
    }
};

class StringValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_String;
    }
    virtual AbstractValue* binary(int op, AbstractValue* other) {
        if (other->kind() == AVK_String) {
            switch (op) {
                case BINARY_ADD:
                    return this;
            }
        }
        return &Any;
    }


    virtual const char* describe() {
        return "String";
    }
};

class BytesValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_String;
    }
    virtual AbstractValue* binary(int op, AbstractValue* other) {
        if (other->kind() == AVK_Bytes) {
            switch (op) {
                case BINARY_ADD:
                    return this;
            }
        }
        return &Any;
    }


    virtual const char* describe() {
        return "Bytes";
    }
};

class FloatValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_Float;
    }
    virtual const char* describe() {
        return "float";
    }
};

class TupleValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_Float;
    }
    virtual const char* describe() {
        return "tuple";
    }
};

class ListValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_List;
    }
    virtual const char* describe() {
        return "list";
    }
};

class DictValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_Dict;
    }
    virtual const char* describe() {
        return "dict";
    }
};

class SetValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_Set;
    }
    virtual const char* describe() {
        return "set";
    }
};

class BoolValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_Bool;
    }
    virtual const char* describe() {
        return "Bool";
    }

};

class NoneValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_None;
    }
    virtual const char* describe() {
        return "None";
    }
};

class FunctionValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_Function;
    }
    virtual const char* describe() {
        return "Function";
    }
};

class SliceValue : public AbstractValue {
    virtual AbstractValueKind kind() {
        return AVK_Slice;
    }
    virtual const char* describe() {
        return "Slice";
    }
};

class InterpreterState {
public:
    vector<AbstractValue*> m_stack;
    vector<AbstractValue*> m_locals;

    AbstractValue* pop() {
        auto res = m_stack.back();
        m_stack.pop_back();
        return res;
    }
};

class AbstractInterpreter {
    // Tracks the interpreter state before each opcode
    unordered_map<size_t, InterpreterState> m_startStates;
    PyCodeObject* m_code;
    unsigned char *m_byteCode;
    AbstractValue* m_returnValue;
    size_t m_size;

    vector<AbstractValue*> m_values;    // all values produced during abstract interpretation


public:
    AbstractInterpreter(PyCodeObject *code);

    bool interpret();
    void dump();

private:
    AbstractValue* to_abstract(PyObject* obj);
    bool merge_states(InterpreterState& newState, int index);
    bool merge_states(InterpreterState& newState, InterpreterState& mergeTo);
    bool update_start_state(InterpreterState& newState, int index);
    char* opcode_name(int opcode);
};

#endif