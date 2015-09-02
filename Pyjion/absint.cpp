#include "absint.h"
#include <opcode.h>
#include <deque>
#include <unordered_map>

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

AbstractInterpreter::AbstractInterpreter(PyCodeObject *code) : m_code(code) {
    m_byteCode = (unsigned char *)((PyBytesObject*)code->co_code)->ob_sval;
    m_size = PyBytes_Size(code->co_code);
    m_returnValue = &Undefined;
}

#define NEXTARG() *(unsigned short*)&m_byteCode[curByte + 1]; curByte+= 2


bool AbstractInterpreter::interpret() {
    InterpreterState lastState;
    deque<int> queue;
    for (int i = 0; i < m_code->co_argcount + m_code->co_kwonlyargcount; i++) {
        // all parameters are initially definitely assigned
        // TODO: Populate this with type information from profiling...
        lastState.m_locals.push_back(&Any);
    }
    int localCount = m_code->co_nlocals - (m_code->co_argcount + m_code->co_kwonlyargcount);
    if (m_code->co_flags & CO_VARARGS) {
        localCount--;
        lastState.m_locals.push_back(&Tuple);
    }
    if (m_code->co_flags & CO_VARKEYWORDS) {
        localCount--;
        lastState.m_locals.push_back(&Dict);
    }

    for (int i = 0; i < localCount; i++) {
        // All locals are initially undefined
        lastState.m_locals.push_back(&Undefined);
    }
    
    m_startStates[0] = lastState;
    queue.push_back(0);
    do {
        int oparg;
        auto cur = queue.front();
        queue.pop_front();
        for (size_t curByte = cur; curByte < m_size; curByte++) {
            // merge any previous state in...
            InterpreterState lastState = m_startStates.find(curByte)->second;

            auto opcodeIndex = curByte;

            auto opcode = m_byteCode[curByte];
            if (HAS_ARG(opcode)){
                oparg = NEXTARG();
            }

            switch (opcode) {
                case NOP: break;
                case ROT_TWO:
                    {
                        auto tmp = lastState.m_stack[lastState.m_stack.size() - 1];
                        lastState.m_stack[lastState.m_stack.size() - 1] = lastState.m_stack[lastState.m_stack.size() - 2];
                        lastState.m_stack[lastState.m_stack.size() - 2] = tmp;
                        break;
                    }
                case ROT_THREE:
                    {
                        auto top = lastState.pop();
                        auto second = lastState.pop();
                        auto third = lastState.pop();

                        lastState.m_stack.push_back(top);
                        lastState.m_stack.push_back(third);
                        lastState.m_stack.push_back(second);
                        break;
                    }
                case POP_TOP:
                    lastState.m_stack.pop_back();
                    break;
                case DUP_TOP:
                    lastState.m_stack.push_back(lastState.m_stack[lastState.m_stack.size() - 1]);
                    break;
                case DUP_TOP_TWO:{
                        auto top = lastState.m_stack[lastState.m_stack.size() - 1];
                        auto second = lastState.m_stack[lastState.m_stack.size() - 2];
                        lastState.m_stack.push_back(second);
                        lastState.m_stack.push_back(top);
                        break;
                    }
                case LOAD_FAST:
                    lastState.m_stack.push_back(lastState.m_locals[oparg]);
                    break;
                case STORE_FAST:
                    lastState.m_locals.data()[oparg] = lastState.m_stack.back();
                    lastState.m_stack.pop_back();
                    break;
                case DELETE_FAST:
                    lastState.m_locals.data()[oparg] = &Undefined;
                    break;
                case BINARY_TRUE_DIVIDE: 
                case BINARY_FLOOR_DIVIDE: 
                case BINARY_POWER: 
                case BINARY_MODULO: 
                case BINARY_MATRIX_MULTIPLY: 
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
                case INPLACE_MATRIX_MULTIPLY: 
                case INPLACE_TRUE_DIVIDE: 
                case INPLACE_FLOOR_DIVIDE: 
                case INPLACE_MODULO: 
                case INPLACE_ADD:
                case INPLACE_SUBTRACT: 
                case INPLACE_LSHIFT: 
                case INPLACE_RSHIFT:
                case INPLACE_AND: 
                case INPLACE_XOR:
                case INPLACE_OR: 
                    {
                        auto two = lastState.pop();
                        auto one = lastState.pop();
                        lastState.m_stack.push_back(one->binary(opcode, two));
                    }
                    break;

                case LOAD_CONST:
                    lastState.m_stack.push_back(to_abstract(PyTuple_GetItem(m_code->co_consts, oparg)));
                    break;
                case POP_JUMP_IF_FALSE:
                    {
                        auto value = lastState.pop();

                        // merge our current state into the branched to location...
                        if (update_start_state(lastState, oparg)) {
                            queue.push_back(oparg);
                        }

                        if (value->is_always_false()) {
                            // We're always jumping, we don't need to process the following opcodes...
                            goto next;
                        }

                        // we'll continue processing after the jump with our new state...
                        break;
                    }
                case POP_JUMP_IF_TRUE:
                    {
                        auto value = lastState.pop();

                        // merge our current state into the branched to location...
                        if (update_start_state(lastState, oparg)) {
                            queue.push_back(oparg);
                        }

                        if (value->is_always_true()) {
                            // We're always jumping, we don't need to process the following opcodes...
                            goto next;
                        }

                        // we'll continue processing after the jump with our new state...
                        break;
                    }
                case JUMP_ABSOLUTE:
                    if (update_start_state(lastState, oparg)) {
                        queue.push_back(oparg);
                    }
                    // Done processing this basic block, we'll need to see a branch
                    // to the following opcodes before we'll process them.
                    goto next;
                case JUMP_FORWARD:
                     if (update_start_state(lastState, oparg + curByte + 1)) {
                        queue.push_back(oparg + curByte + 1);
                     }
                     // Done processing this basic block, we'll need to see a branch
                     // to the following opcodes before we'll process them.
                     goto next;
                case RETURN_VALUE:
                    {
                        // TODO: We need to 
                        m_returnValue = m_returnValue->merge_with(lastState.pop());
                    }
                    break;
                case LOAD_NAME:
                    // Used to load __name__ for a class def
                    lastState.m_stack.push_back(&Any);
                    break;
                case STORE_NAME:
                    // Stores __module__, __doc__, __qualname__, as well as class/function defs sometimes
                    lastState.m_stack.pop_back();
                    break;
                case LOAD_GLOBAL:
                    // TODO: Look in globals, then builtins, and see if we can resolve
                    // this to anything concrete.
                    lastState.m_stack.push_back(&Any);
                    break;
                case STORE_GLOBAL:
                    lastState.m_stack.pop_back();
                    break;
                case LOAD_ATTR:
                    // TODO: Add support for resolving known members of known types
                    lastState.m_stack.pop_back();
                    lastState.m_stack.push_back(&Any);
                    break;
                case STORE_ATTR:
                    lastState.m_stack.pop_back();
                    lastState.m_stack.pop_back();
                    break;
                case DELETE_ATTR:
                    lastState.m_stack.pop_back();
                    break;
                case BUILD_LIST:
                    for (int i = 0; i < oparg; i++) {
                        lastState.m_stack.pop_back();
                    }
                    lastState.m_stack.push_back(&List);
                    break;
                case BUILD_TUPLE:
                    for (int i = 0; i < oparg; i++) {
                        lastState.m_stack.pop_back();
                    }
                    lastState.m_stack.push_back(&Tuple);
                    break;
                case BUILD_MAP:
                    for (int i = 0; i < oparg; i++) {
                        lastState.m_stack.pop_back();
                        lastState.m_stack.pop_back();
                    }
                    lastState.m_stack.push_back(&Dict);
                    break;
                case COMPARE_OP:
                    switch (oparg) {
                        case PyCmp_IS:
                        case PyCmp_IS_NOT:
                        case PyCmp_IN:
                        case PyCmp_NOT_IN:
                            lastState.m_stack.pop_back();
                            lastState.m_stack.pop_back();
                            lastState.m_stack.push_back(&Bool);
                            break;
                        case PyCmp_EXC_MATCH:
                            // TODO: Produces an error or a bool, but no way to represent that so we're conservative
                            lastState.m_stack.pop_back();
                            lastState.m_stack.pop_back();
                            lastState.m_stack.push_back(&Any);
                            break;
                        default:
                            // TODO: Comparisons of known types produce bools
                            lastState.m_stack.pop_back();
                            lastState.m_stack.pop_back();
                            lastState.m_stack.push_back(&Any);
                            break;
                    }
                    break;
                case IMPORT_NAME:
                    lastState.m_stack.pop_back();
                    lastState.m_stack.pop_back();
                    lastState.m_stack.push_back(&Any);
                    break;
                case LOAD_CLOSURE:
                    lastState.m_stack.push_back(&Any);
                    break;
                case CALL_FUNCTION:
                    {
                        // TODO: Known functions could return known return types.
                        int argCnt = oparg & 0xff;
                        int kwArgCnt = (oparg >> 8) & 0xff;

                        lastState.m_stack.pop_back();
                        for (int i = 0; i < argCnt; i++) {
                            lastState.m_stack.pop_back();
                        }
                        for (int i = 0; i < kwArgCnt; i++) {
                            lastState.m_stack.pop_back();
                            lastState.m_stack.pop_back();
                        }
                        lastState.m_stack.push_back(&Any);
                        break;
                    }

                case CALL_FUNCTION_VAR:
                case CALL_FUNCTION_KW:
                case CALL_FUNCTION_VAR_KW:
                    {
                        int na = oparg & 0xff;
                        int nk = (oparg >> 8) & 0xff;
                        int flags = (opcode - CALL_FUNCTION) & 3;
                        int n = na + 2 * nk;
#define CALL_FLAG_VAR 1
#define CALL_FLAG_KW 2
                        if (flags & CALL_FLAG_KW) {
                            lastState.pop();
                        }
                        for (int i = 0; i < nk; i++) {
                            lastState.pop();
                            lastState.pop();
                        }
                        if (flags & CALL_FLAG_VAR) {
                            lastState.pop();
                        }
                        for (int i = 0; i < na; i++) {
                            lastState.pop();
                        }
                        lastState.m_stack.push_back(&Any);
                        break;
                    }
                case MAKE_CLOSURE:
                case MAKE_FUNCTION:
                    {
                        int posdefaults = oparg & 0xff;
                        int kwdefaults = (oparg >> 8) & 0xff;
                        int num_annotations = (oparg >> 16) & 0x7fff;

                        lastState.m_stack.pop_back(); // name
                        lastState.m_stack.pop_back(); // code

                        if (opcode == MAKE_CLOSURE) {
                            lastState.m_stack.pop_back(); // closure object
                        }

                        if (num_annotations > 0) {
                            lastState.m_stack.pop_back();
                            for (int i = 0; i < num_annotations - 1; i++) {
                                lastState.m_stack.pop_back();
                            }
                        }

                        for (int i = 0; i < kwdefaults; i++) {
                            lastState.m_stack.pop_back();
                            lastState.m_stack.pop_back();
                        }
                        for (int i = 0; i < posdefaults; i++) {
                            lastState.m_stack.pop_back();
                            
                        }

                        if (num_annotations == 0) {
                            lastState.m_stack.push_back(&Function);
                        }
                        else{
                            // we're not sure of the type with an annotation present
                            lastState.m_stack.push_back(&Any);
                        }
                        break;
                    }
                case BUILD_SLICE:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                    }
                    lastState.m_stack.push_back(&Slice);
                    break;
                case UNARY_NOT:
                    // TODO: Always returns a bool or an error code...
                    // TODO: Known types can always return a bool
                    lastState.pop();
                    lastState.m_stack.push_back(&Any);
                    break;
                case UNARY_NEGATIVE:
                    // TODO: Support known types
                    lastState.pop();
                    lastState.m_stack.push_back(&Any);
                    break;
                case UNARY_INVERT:
                    // TODO: Support known types
                    lastState.pop();
                    lastState.m_stack.push_back(&Any);
                    break;
                case UNPACK_SEQUENCE:
                    // TODO: If the sequence is a known type we could know what types we're pushing here.
                    lastState.pop();
                    for (int i = 0; i < oparg; i++) {
                        lastState.m_stack.push_back(&Any);
                    }
                    break;
                case RAISE_VARARGS:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                    }
                    goto next;
                case BINARY_SUBSCR:
                    // TODO: Deal with known subscription
                    lastState.pop();
                    lastState.pop();
                    lastState.m_stack.push_back(&Any);
                    break;
                case STORE_SUBSCR:
                    // TODO: Do we want to track types on store for lists?
                    lastState.pop();
                    lastState.pop();
                    lastState.pop();
                    break;
                case BUILD_SET:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                    }
                    lastState.m_stack.push_back(&Set);
                    break;
                case STORE_DEREF:
                    lastState.pop();
                    break;
                case LOAD_DEREF:
                    lastState.m_stack.push_back(&Any);
                    break;
                case GET_ITER:
                    // TODO: Known iterable types
                    lastState.pop();
                    lastState.m_stack.push_back(&Any);
                    break;
                case FOR_ITER:
                    {
                        // For branches out with the value consumed
                        auto leaveState = lastState;
                        leaveState.pop();
                        if (update_start_state(leaveState, oparg + curByte + 1)) {
                            queue.push_back(oparg + curByte + 1);
                        }

                        // When we compile this we don't actually leave the value on the stack,
                        // but the sequence of opcodes assumes that happens.  to keep our stack
                        // properly balanced we match what's really going on.
                        lastState.m_stack.push_back(&Any);
                    
                        break;
                    }
                case SETUP_LOOP:
                case POP_BLOCK:
                    break;

                case SETUP_EXCEPT:
                default:
                    printf("Unsupported opcode: %s", opcode_name(opcode));
                    return false;
            }
            update_start_state(lastState, curByte + 1);
        }
    next:;
    } while (queue.size() != 0);
    return true;
}

bool AbstractInterpreter::update_start_state(InterpreterState& newState, int index) {
    auto initialState = m_startStates.find(index);
    if (initialState != m_startStates.end()) {
        return merge_states(newState, initialState->second);
    }
    else{
        m_startStates[index] = newState;
        return true;
    }
}

bool AbstractInterpreter::merge_states(InterpreterState& newState, InterpreterState& mergeTo) {
    bool changed = false;
    if (mergeTo.m_locals.size() == 0) {
        // first time we assigned, or no locals...
        mergeTo.m_locals = newState.m_locals;
        changed |= newState.m_locals.size() != 0;
    }
    else{
        // need to merge locals...
        _ASSERT(mergeTo.m_locals.size() == newState.m_locals.size());
        for (int i = 0; i < newState.m_locals.size(); i++) {
            auto newType = mergeTo.m_locals[i]->merge_with(newState.m_locals[i]);
            if (newType != mergeTo.m_locals[i]) {
                mergeTo.m_locals[i] = newType;
                changed = true;
            }
            
        }
    }

    if (mergeTo.m_stack.size() == 0) {
        // first time we assigned, or empty stack...
        mergeTo.m_stack = newState.m_stack;
        changed |= newState.m_stack.size() != 0;
    }
    else{
        // need to merge the stacks...
        _ASSERT(mergeTo.m_stack.size() == newState.m_stack.size());
        for (int i = 0; i < newState.m_stack.size(); i++) {
            auto newType = mergeTo.m_stack[i]->merge_with(newState.m_stack[i]);
            if (mergeTo.m_stack[i] != newType) {
                mergeTo.m_stack[i] = newType;
                changed = true;
            }
        }
    }
    return changed;
}

AbstractValue* AbstractInterpreter::to_abstract(PyObject*value) {
    if (value == Py_None) {
        return &None;
    }
    else if (PyLong_CheckExact(value)) {
        return &Integer;
    }
    else if (PyUnicode_Check(value)) {
        return &String;
    }
    else if (PyList_CheckExact(value)) {
        return &List;
    }
    else if (PyDict_CheckExact(value)) {
        return &Dict;
    }
    else if (PyTuple_CheckExact(value)) {
        return &Tuple;
    }
    else if (PyBool_Check(value)) {
        return &Bool;
    }
    else if (PyFloat_CheckExact(value)) {
        return &Float;
    }
    else if (PyBytes_CheckExact(value)) {
        return &Bytes;
    }
    else if (PySet_Check(value)) {
        return &Set;
    }


    return &Any;
}

void AbstractInterpreter::dump() {
    for (size_t curByte = 0; curByte < m_size; curByte++) {
        auto opcode = m_byteCode[curByte];
        auto byteIndex = curByte;
        
        auto find = m_startStates.find(byteIndex);
        if (find != m_startStates.end()) {
            auto state = find->second;
            for (int i = 0; i < state.m_locals.size(); i++) {
                printf(
                    "          %-20s %s\r\n",
                    PyUnicode_AsUTF8(PyTuple_GetItem(m_code->co_varnames, i)),
                    state.m_locals[i]->describe()
                    );
            }
            for (int i = 0; i < state.m_stack.size(); i++) {
                printf("          %-20d %s\r\n", i, state.m_stack[i]->describe());
            }
        }
        
        if (HAS_ARG(opcode)){
            int oparg = NEXTARG();
            printf("    %-3d %-22s %d\r\n", byteIndex, opcode_name(opcode), oparg);
        }
        else{
            printf("    %-3d %-22s\r\n", byteIndex, opcode_name(opcode));
        }
    }
    printf("Returns %s\r\n", m_returnValue->describe());
}

char* AbstractInterpreter::opcode_name(int opcode) {
#define OP_TO_STR(x)   case x: return #x;
    switch (opcode) {
        OP_TO_STR(POP_TOP)
            OP_TO_STR(ROT_TWO)
            OP_TO_STR(ROT_THREE)
            OP_TO_STR(DUP_TOP)
            OP_TO_STR(DUP_TOP_TWO)
            OP_TO_STR(NOP)
            OP_TO_STR(UNARY_POSITIVE)
            OP_TO_STR(UNARY_NEGATIVE)
            OP_TO_STR(UNARY_NOT)
            OP_TO_STR(UNARY_INVERT)
            OP_TO_STR(BINARY_MATRIX_MULTIPLY)
            OP_TO_STR(INPLACE_MATRIX_MULTIPLY)
            OP_TO_STR(BINARY_POWER)
            OP_TO_STR(BINARY_MULTIPLY)
            OP_TO_STR(BINARY_MODULO)
            OP_TO_STR(BINARY_ADD)
            OP_TO_STR(BINARY_SUBTRACT)
            OP_TO_STR(BINARY_SUBSCR)
            OP_TO_STR(BINARY_FLOOR_DIVIDE)
            OP_TO_STR(BINARY_TRUE_DIVIDE)
            OP_TO_STR(INPLACE_FLOOR_DIVIDE)
            OP_TO_STR(INPLACE_TRUE_DIVIDE)
            OP_TO_STR(GET_AITER)
            OP_TO_STR(GET_ANEXT)
            OP_TO_STR(BEFORE_ASYNC_WITH)
            OP_TO_STR(INPLACE_ADD)
            OP_TO_STR(INPLACE_SUBTRACT)
            OP_TO_STR(INPLACE_MULTIPLY)
            OP_TO_STR(INPLACE_MODULO)
            OP_TO_STR(STORE_SUBSCR)
            OP_TO_STR(DELETE_SUBSCR)
            OP_TO_STR(BINARY_LSHIFT)
            OP_TO_STR(BINARY_RSHIFT)
            OP_TO_STR(BINARY_AND)
            OP_TO_STR(BINARY_XOR)
            OP_TO_STR(BINARY_OR)
            OP_TO_STR(INPLACE_POWER)
            OP_TO_STR(GET_ITER)
            OP_TO_STR(PRINT_EXPR)
            OP_TO_STR(LOAD_BUILD_CLASS)
            OP_TO_STR(YIELD_FROM)
            OP_TO_STR(GET_AWAITABLE)
            OP_TO_STR(INPLACE_LSHIFT)
            OP_TO_STR(INPLACE_RSHIFT)
            OP_TO_STR(INPLACE_AND)
            OP_TO_STR(INPLACE_XOR)
            OP_TO_STR(INPLACE_OR)
            OP_TO_STR(BREAK_LOOP)
            OP_TO_STR(WITH_CLEANUP_START)
            OP_TO_STR(WITH_CLEANUP_FINISH)
            OP_TO_STR(RETURN_VALUE)
            OP_TO_STR(IMPORT_STAR)
            OP_TO_STR(YIELD_VALUE)
            OP_TO_STR(POP_BLOCK)
            OP_TO_STR(END_FINALLY)
            OP_TO_STR(POP_EXCEPT)
            OP_TO_STR(STORE_NAME)
            OP_TO_STR(DELETE_NAME)
            OP_TO_STR(UNPACK_SEQUENCE)
            OP_TO_STR(FOR_ITER)
            OP_TO_STR(UNPACK_EX)
            OP_TO_STR(STORE_ATTR)
            OP_TO_STR(DELETE_ATTR)
            OP_TO_STR(STORE_GLOBAL)
            OP_TO_STR(DELETE_GLOBAL)
            OP_TO_STR(LOAD_CONST)
            OP_TO_STR(LOAD_NAME)
            OP_TO_STR(BUILD_TUPLE)
            OP_TO_STR(BUILD_LIST)
            OP_TO_STR(BUILD_SET)
            OP_TO_STR(BUILD_MAP)
            OP_TO_STR(LOAD_ATTR)
            OP_TO_STR(COMPARE_OP)
            OP_TO_STR(IMPORT_NAME)
            OP_TO_STR(IMPORT_FROM)
            OP_TO_STR(JUMP_FORWARD)
            OP_TO_STR(JUMP_IF_FALSE_OR_POP)
            OP_TO_STR(JUMP_IF_TRUE_OR_POP)
            OP_TO_STR(JUMP_ABSOLUTE)
            OP_TO_STR(POP_JUMP_IF_FALSE)
            OP_TO_STR(POP_JUMP_IF_TRUE)
            OP_TO_STR(LOAD_GLOBAL)
            OP_TO_STR(CONTINUE_LOOP)
            OP_TO_STR(SETUP_LOOP)
            OP_TO_STR(SETUP_EXCEPT)
            OP_TO_STR(SETUP_FINALLY)
            OP_TO_STR(LOAD_FAST)
            OP_TO_STR(STORE_FAST)
            OP_TO_STR(DELETE_FAST)
            OP_TO_STR(RAISE_VARARGS)
            OP_TO_STR(CALL_FUNCTION)
            OP_TO_STR(MAKE_FUNCTION)
            OP_TO_STR(BUILD_SLICE)
            OP_TO_STR(MAKE_CLOSURE)
            OP_TO_STR(LOAD_CLOSURE)
            OP_TO_STR(LOAD_DEREF)
            OP_TO_STR(STORE_DEREF)
            OP_TO_STR(DELETE_DEREF)
            OP_TO_STR(CALL_FUNCTION_VAR)
            OP_TO_STR(CALL_FUNCTION_KW)
            OP_TO_STR(CALL_FUNCTION_VAR_KW)
            OP_TO_STR(SETUP_WITH)
            OP_TO_STR(EXTENDED_ARG)
            OP_TO_STR(LIST_APPEND)
            OP_TO_STR(SET_ADD)
            OP_TO_STR(MAP_ADD)
            OP_TO_STR(LOAD_CLASSDEREF)
            OP_TO_STR(BUILD_LIST_UNPACK)
            OP_TO_STR(BUILD_MAP_UNPACK)
            OP_TO_STR(BUILD_MAP_UNPACK_WITH_CALL)
            OP_TO_STR(BUILD_TUPLE_UNPACK)
            OP_TO_STR(BUILD_SET_UNPACK)
            OP_TO_STR(SETUP_ASYNC_WITH)
    }
    return "unknown";
}


AbstractValue* AbstractValue::binary(int op, AbstractValue* other) {
    return &Any;
}

AbstractValue* AbstractValue::merge_with(AbstractValue*other) {
    if (this == other) {
        return this;
    }
    return &Any;
}
