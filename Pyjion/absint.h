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


struct AbsIntBlockInfo {
    size_t BlockStart, BlockEnd;
    bool IsLoop;

    AbsIntBlockInfo() {
    }

    AbsIntBlockInfo(size_t blockStart, size_t blockEnd, bool isLoop) {
        BlockStart = blockStart;
        BlockEnd = blockEnd;
        IsLoop = isLoop;
    }
};

#ifdef _DEBUG
extern int g_cowArrayCount;
#endif
template<typename T> class COWArray {
    T* m_data;
    size_t m_refCount, m_size;

    COWArray(T* data, size_t size) {
        m_refCount = 1;
        m_data = data;
        m_size = size;
#ifdef _DEBUG
        g_cowArrayCount++;
#endif

    }

    ~COWArray() {
        delete[] m_data;
#ifdef _DEBUG
        g_cowArrayCount--;
#endif
    }
public:
    COWArray(size_t size) {
        m_size = size;
        m_refCount = 1;
        m_data = new T[size];
#ifdef _DEBUG
        g_cowArrayCount++;
#endif
    }

    T& operator[] (const size_t n) {
        return m_data[n];
    }

    COWArray* copy(){ 
        m_refCount++;
        return this;
    }

    void free() {
        m_refCount--;
        if (m_refCount == 0) {
            delete this;
        }
    }

    int size() {
        return m_size;
    }

    // Called to replace an existing array with a new one by replacing
    // a single value.  If there are no other references to this array
    // we re-use the existing one.  If there are other references returns
    // a new array and dec refs the existing one.
    COWArray* replace(size_t index, T value) {
        if (m_refCount == 1) {
            m_data[index] = value;
            return this;
        }

        m_refCount--;
        T* newData = new T[m_size];
        memcpy_s(newData, m_size * sizeof(T), m_data, m_size * sizeof(T));
        return new COWArray(newData, m_size);
    }
};


class InterpreterState {
public:
    vector<AbstractValue*> m_stack;
    COWArray<AbstractValue*>* m_locals;

    AbstractValue* pop() {
        auto res = m_stack.back();
        m_stack.pop_back();
        return res;
    }
};


class AbstractInterpreter {
    // Tracks the interpreter state before each opcode
    unordered_map<size_t, InterpreterState> m_startStates;
    unordered_map<size_t, bool> m_endFinallyIsFinally;
    unordered_map<size_t, size_t> m_blockStarts;
    unordered_map<size_t, AbsIntBlockInfo> m_breakTo;
    PyCodeObject* m_code;
    unsigned char *m_byteCode;
    AbstractValue* m_returnValue;
    size_t m_size;

    vector<AbstractValue*> m_values;    // all values produced during abstract interpretation


public:
    AbstractInterpreter(PyCodeObject *code);
    ~AbstractInterpreter() {
        for (auto cur = m_startStates.begin(); cur != m_startStates.end(); cur++){
            cur->second.m_locals->free();
        }
        _ASSERTE(g_cowArrayCount == 0);
    }

    bool interpret();
    void dump();

private:

    AbstractValue* to_abstract(PyObject* obj);
    bool merge_states(InterpreterState& newState, int index);
    bool merge_states(InterpreterState& newState, InterpreterState& mergeTo);
    bool update_start_state(InterpreterState& newState, int index);
    char* opcode_name(int opcode);
    void preprocess();
};

#endif