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

#include "absint.h"
#include <opcode.h>
#include <deque>
#include <unordered_map>

AbstractInterpreter::AbstractInterpreter(PyCodeObject *code) : m_code(code) {
    m_byteCode = (unsigned char *)((PyBytesObject*)code->co_code)->ob_sval;
    m_size = PyBytes_Size(code->co_code);
    m_returnValue = &Undefined;
}

#define NEXTARG() *(unsigned short*)&m_byteCode[curByte + 1]; curByte+= 2

void AbstractInterpreter::preprocess() {
    int oparg;
    vector<bool> ehKind;
    vector<AbsIntBlockInfo> blockStarts;
    for (size_t curByte = 0; curByte < m_size; curByte++) {
        auto opcodeIndex = curByte;
        auto byte = m_byteCode[curByte];
        if (HAS_ARG(byte)){
            oparg = NEXTARG();
        }

        switch (byte) {
            case SETUP_WITH: 
                // not supported...
                return;
            case SETUP_LOOP:
                blockStarts.push_back(AbsIntBlockInfo(opcodeIndex, oparg + curByte + 1, true));
                break;
            case POP_BLOCK:
                {
                    auto blockStart = blockStarts.back();
                    blockStarts.pop_back();
                    m_blockStarts[opcodeIndex] = blockStart.BlockStart;
                    break;
                }
            case SETUP_EXCEPT: 
                blockStarts.push_back(AbsIntBlockInfo(opcodeIndex, oparg + curByte + 1, false));
                ehKind.push_back(false); 
                break;
            case SETUP_FINALLY: 
                blockStarts.push_back(AbsIntBlockInfo(opcodeIndex, oparg + curByte + 1, false));
                ehKind.push_back(true); 
                break;
            case END_FINALLY:
                m_endFinallyIsFinally[opcodeIndex] = ehKind.back();
                ehKind.pop_back();
                break;
            case BREAK_LOOP:
                for (auto iter = blockStarts.rbegin(); iter != blockStarts.rend(); ++iter) {
                    if (iter->IsLoop) {
                        m_breakTo[opcodeIndex] = *iter;
                        break;
                    }
                }
                break;
        }
    }

}

bool AbstractInterpreter::interpret() {
    preprocess();
    
    InterpreterState lastState = InterpreterState(m_code->co_nlocals);

    int localIndex = 0;
    for (int i = 0; i < m_code->co_argcount + m_code->co_kwonlyargcount; i++) {
        // all parameters are initially definitely assigned
        // TODO: Populate this with type information from profiling...
        lastState.replace_local(localIndex++, AbstractLocalInfo(&Any));
    }

    if (m_code->co_flags & CO_VARARGS) {
        lastState.replace_local(localIndex++, AbstractLocalInfo(&Tuple));
    }
    if (m_code->co_flags & CO_VARKEYWORDS) {
        lastState.replace_local(localIndex++, AbstractLocalInfo(&Dict));
    }

    for (; localIndex < m_code->co_nlocals; localIndex++) {
        // All locals are initially undefined
        lastState.replace_local(localIndex, AbstractLocalInfo(&Undefined, true));
    }
    
    //dump();

    update_start_state(lastState, 0);

    // walk all the blocks in the code one by one, analyzing them, and enqueing any
    // new blocks that we encounter from branches.
    deque<size_t> queue;
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
            processOpCode:
            switch (opcode) {
                case EXTENDED_ARG:
                    {
                        opcode = m_byteCode[++curByte];
                        int bottomArg = NEXTARG();
                        oparg = (oparg << 16) | bottomArg;
                        goto processOpCode;
                    }
                case NOP: break;
                case ROT_TWO:
                    {
                        auto tmp = lastState[lastState.stack_size() - 1];
                        lastState[lastState.stack_size() - 1] = lastState[lastState.stack_size() - 2];
                        lastState[lastState.stack_size() - 2] = tmp;
                        break;
                    }
                case ROT_THREE:
                    {
                        auto top = lastState.pop();
                        auto second = lastState.pop();
                        auto third = lastState.pop();

                        lastState.push(top);
                        lastState.push(third);
                        lastState.push(second);
                        break;
                    }
                case POP_TOP:
                    lastState.pop();
                    break;
                case DUP_TOP:
                    lastState.push(lastState[lastState.stack_size() - 1]);
                    break;
                case DUP_TOP_TWO:{
                        auto top = lastState[lastState.stack_size() - 1];
                        auto second = lastState[lastState.stack_size() - 2];
                        lastState.push(second);
                        lastState.push(top);
                        break;
                    }
                case LOAD_FAST:
                    lastState.push(lastState.get_local(oparg).Type);
                    break;
                case STORE_FAST:
                    {
                        auto value = lastState.pop();
                        lastState.replace_local(oparg, value);
                    }
                    break;
                case DELETE_FAST:
                    lastState.replace_local(oparg, AbstractLocalInfo(&Undefined, true));
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
                        lastState.push(one->binary(opcode, two));
                    }
                    break;

                case LOAD_CONST:
                    lastState.push(to_abstract(PyTuple_GetItem(m_code->co_consts, oparg)));
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
                case JUMP_IF_TRUE_OR_POP:
                    {
                        auto curState = lastState;
                        if (update_start_state(lastState, oparg)) {
                            queue.push_back(oparg);
                        }
                        auto value = lastState.pop();
                        if (value->is_always_true()) {
                            // we always jump, no need to analyze the following instructions...
                            goto next;
                        }
                    }
                    break;
                case JUMP_IF_FALSE_OR_POP:
                    {
                        auto curState = lastState;
                        if (update_start_state(lastState, oparg)) {
                            queue.push_back(oparg);
                        }
                        auto value = lastState.pop();
                        if (value->is_always_false()) {
                            // we always jump, no need to analyze the following instructions...
                            goto next;
                        }
                    }
                    break;
                case JUMP_ABSOLUTE:
                    if (update_start_state(lastState, oparg)) {
                        queue.push_back(oparg);
                    }
                    // Done processing this basic block, we'll need to see a branch
                    // to the following opcodes before we'll process them.
                    goto next;
                case JUMP_FORWARD:
                     if (update_start_state(lastState, (size_t)oparg + curByte + 1)) {
                         queue.push_back((size_t)oparg + curByte + 1);
                     }
                     // Done processing this basic block, we'll need to see a branch
                     // to the following opcodes before we'll process them.
                     goto next;
                case RETURN_VALUE:
                    {
                        m_returnValue = m_returnValue->merge_with(lastState.pop());
                    }
                    goto next;
                case LOAD_NAME:
                    // Used to load __name__ for a class def
                    lastState.push(&Any);
                    break;
                case STORE_NAME:
                    // Stores __module__, __doc__, __qualname__, as well as class/function defs sometimes
                    lastState.pop();
                    break;
                case LOAD_GLOBAL:
                    // TODO: Look in globals, then builtins, and see if we can resolve
                    // this to anything concrete.
                    lastState.push(&Any);
                    break;
                case STORE_GLOBAL:
                    lastState.pop();
                    break;
                case LOAD_ATTR:
                    // TODO: Add support for resolving known members of known types
                    lastState.pop();
                    lastState.push(&Any);
                    break;
                case STORE_ATTR:
                    lastState.pop();
                    lastState.pop();
                    break;
                case DELETE_ATTR:
                    lastState.pop();
                    break;
                case BUILD_LIST:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                    }
                    lastState.push(&List);
                    break;
                case BUILD_TUPLE:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                    }
                    lastState.push(&Tuple);
                    break;
                case BUILD_MAP:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                        lastState.pop();
                    }
                    lastState.push(&Dict);
                    break;
                case COMPARE_OP:
                    switch (oparg) {
                        case PyCmp_IS:
                        case PyCmp_IS_NOT:
                        case PyCmp_IN:
                        case PyCmp_NOT_IN:
                            lastState.pop();
                            lastState.pop();
                            lastState.push(&Bool);
                            break;
                        case PyCmp_EXC_MATCH:
                            // TODO: Produces an error or a bool, but no way to represent that so we're conservative
                            lastState.pop();
                            lastState.pop();
                            lastState.push(&Any);
                            break;
                        default:
                            // TODO: Comparisons of known types produce bools
                            lastState.pop();
                            lastState.pop();
                            lastState.push(&Any);
                            break;
                    }
                    break;
                case IMPORT_NAME:
                    lastState.pop();
                    lastState.pop();
                    lastState.push(&Any);
                    break;
                case IMPORT_FROM:
                    // leave the module on the stack, and push the new value.
                    lastState.push(&Any);
                    break;
                case LOAD_CLOSURE:
                    lastState.push(&Any);
                    break;
                case CALL_FUNCTION:
                    {
                        // TODO: Known functions could return known return types.
                        int argCnt = oparg & 0xff;
                        int kwArgCnt = (oparg >> 8) & 0xff;

                        for (int i = 0; i < argCnt; i++) {
                            lastState.pop();
                        }
                        for (int i = 0; i < kwArgCnt; i++) {
                            lastState.pop();
                            lastState.pop();
                        }

                        // TODO: If the function was a known quantity we could
                        // abstract interpret it here with the known args, and
                        // potentially consider inlining it.

                        // pop the function...
                        lastState.pop();

                        lastState.push(&Any);
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

                        // pop the function
                        lastState.pop();

                        lastState.push(&Any);
                        break;
                    }
                case MAKE_CLOSURE:
                case MAKE_FUNCTION:
                    {
                        int posdefaults = oparg & 0xff;
                        int kwdefaults = (oparg >> 8) & 0xff;
                        int num_annotations = (oparg >> 16) & 0x7fff;

                        lastState.pop(); // name
                        lastState.pop(); // code

                        if (opcode == MAKE_CLOSURE) {
                            lastState.pop(); // closure object
                        }

                        if (num_annotations > 0) {
                            lastState.pop();
                            for (int i = 0; i < num_annotations - 1; i++) {
                                lastState.pop();
                            }
                        }

                        for (int i = 0; i < kwdefaults; i++) {
                            lastState.pop();
                            lastState.pop();
                        }
                        for (int i = 0; i < posdefaults; i++) {
                            lastState.pop();
                            
                        }

                        if (num_annotations == 0) {
                            lastState.push(&Function);
                        }
                        else{
                            // we're not sure of the type with an annotation present
                            lastState.push(&Any);
                        }
                        break;
                    }
                case BUILD_SLICE:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                    }
                    lastState.push(&Slice);
                    break;
                case UNARY_POSITIVE:
                    // TODO: Support known types
                    lastState.pop();
                    lastState.push(&Any);
                    break;
                case UNARY_NOT:
                    // TODO: Always returns a bool or an error code...
                    // TODO: Known types can always return a bool
                    lastState.pop();
                    lastState.push(&Any);
                    break;
                case UNARY_NEGATIVE:
                    // TODO: Support known types
                    lastState.pop();
                    lastState.push(&Any);
                    break;
                case UNARY_INVERT:
                    // TODO: Support known types
                    lastState.pop();
                    lastState.push(&Any);
                    break;
                case UNPACK_EX:
                    lastState.pop();
                    for (int i = 0; i < oparg >> 8; i++) {
                        lastState.push(&Any);
                    }
                    lastState.push(&List);
                    for (int i = 0; i < (oparg & 0xff); i++) {
                        lastState.push(&Any);
                    }
                    break;
                case UNPACK_SEQUENCE:
                    // TODO: If the sequence is a known type we could know what types we're pushing here.
                    lastState.pop();
                    for (int i = 0; i < oparg; i++) {
                        lastState.push(&Any);
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
                    lastState.push(&Any);
                    break;
                case STORE_SUBSCR:
                    // TODO: Do we want to track types on store for lists?
                    lastState.pop();
                    lastState.pop();
                    lastState.pop();
                    break;
                case DELETE_SUBSCR:
                    lastState.pop();
                    lastState.pop();
                    break;
                case BUILD_SET:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                    }
                    lastState.push(&Set);
                    break;
                case STORE_DEREF:
                    lastState.pop();
                    break;
                case LOAD_DEREF:
                    lastState.push(&Any);
                    break;
                case GET_ITER:
                    // TODO: Known iterable types
                    lastState.pop();
                    lastState.push(&Any);
                    break;
                case FOR_ITER:
                    {
                        // For branches out with the value consumed
                        auto leaveState = lastState;
                        leaveState.pop();
                        if (update_start_state(leaveState, (size_t)oparg + curByte + 1)) {
                            queue.push_back((size_t)oparg + curByte + 1);
                        }

                        // When we compile this we don't actually leave the value on the stack,
                        // but the sequence of opcodes assumes that happens.  to keep our stack
                        // properly balanced we match what's really going on.
                        lastState.push(&Any);
                    
                        break;
                    }
                case SETUP_LOOP:
                    break;
                case POP_BLOCK:
                    // Restore the stack state to what we had on entry, merge the locals.
                    lastState.m_stack = m_startStates[m_blockStarts[opcodeIndex]].m_stack;
                    merge_states(m_startStates[m_blockStarts[opcodeIndex]], lastState);
                    break;
                case POP_EXCEPT:
                    break;
                case LOAD_BUILD_CLASS:
                    // TODO: if we know this is __builtins__.__build_class__ we can push a special value
                    // to optimize the call.f
                    lastState.push(&Any);
                    break;
                case SET_ADD:
                    // pop the value being stored off, leave set on stack
                    lastState.pop(); 
                    break;
                case LIST_APPEND:
                    // pop the value being stored off, leave list on stack
                    lastState.pop();
                    break;
                case MAP_ADD:
                    // pop the value and key being stored off, leave list on stack
                    lastState.pop();
                    lastState.pop();
                    break;
                case CONTINUE_LOOP:
                    if (update_start_state(lastState, oparg)) {
                        queue.push_back(oparg);
                    }
                    // Done processing this basic block, we'll need to see a branch
                    // to the following opcodes before we'll process them.
                    goto next;
                case BREAK_LOOP:
                    {
                        auto breakTo = m_breakTo[opcodeIndex];

                        // BREAK_LOOP does an unwind block, which restores the
                        // stack state to what it was when we entered the loop.  So
                        // we get the start state for where the SETUP_LOOP happened
                        // here and propagate it to where we're breaking to.
                        if (update_start_state(m_startStates[breakTo.BlockStart], breakTo.BlockEnd)) {
                            queue.push_back(breakTo.BlockEnd);
                        }

                        goto next;
                    }
                case SETUP_FINALLY:
                    {
                        auto finallyState = lastState;
                        // Finally is entered with value pushed onto stack indicating reason for 
                        // the finally running...
                        finallyState.push(&Any);
                        if (update_start_state(finallyState, (size_t)oparg + curByte + 1)) {
                            queue.push_back((size_t)oparg + curByte + 1);
                        }
                    }
                    break;
                case SETUP_EXCEPT:
                    {
                        auto ehState = lastState;
                        // Except is entered with the exception object, traceback, and exception
                        // type.  TODO: We could type these stronger then they currently are typed
                        ehState.push(&Any);
                        ehState.push(&Any);
                        ehState.push(&Any);
                        if (update_start_state(ehState, (size_t)oparg + curByte + 1)) {
                            queue.push_back((size_t)oparg + curByte + 1);
                        }
                    }
                    break;
                case END_FINALLY:
                    {
                        bool isFinally = m_endFinallyIsFinally[opcodeIndex];
                        if (isFinally) {
                            // single value indicating whether we're unwinding or
                            // completed normally.
                            lastState.pop();
                        }
                        else{
                            lastState.pop();
                            lastState.pop();
                            lastState.pop();
                        }
                        break;
                    }
                case SETUP_WITH:
                case YIELD_VALUE:
                    return false;
                default:
                    printf("Unknown unsupported opcode: %s", opcode_name(opcode));
                    return false;
            }
            update_start_state(lastState, curByte + 1);
            
        }
    next:;
    } while (queue.size() != 0);
    return true;
}

bool AbstractInterpreter::update_start_state(InterpreterState& newState, size_t index) {
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
    if (mergeTo.m_locals.get() != newState.m_locals.get()) {
        // need to merge locals...
        _ASSERT(mergeTo.local_count() == newState.local_count());
        for (int i = 0; i < newState.local_count(); i++) {
            auto oldType = mergeTo.get_local(i);
            auto newType = oldType.merge_with(newState.get_local(i));
            if (newType != oldType) {
                mergeTo.replace_local(i, newType);
                changed = true;
            }
            
        }
    }

    if (mergeTo.stack_size() == 0) {
        // first time we assigned, or empty stack...
        mergeTo.m_stack = newState.m_stack;
        changed |= newState.stack_size() != 0;
    }
    else{
        // need to merge the stacks...
        _ASSERT(mergeTo.stack_size() == newState.stack_size());
        for (int i = 0; i < newState.stack_size(); i++) {
            auto newType = mergeTo[i]->merge_with(newState[i]);
            if (mergeTo[i] != newType) {
                mergeTo[i] = newType;
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
    printf("Dumping %s from %s line %d\r\n",
        PyUnicode_AsUTF8(m_code->co_name),
        PyUnicode_AsUTF8(m_code->co_filename),
        m_code->co_firstlineno
    );
    for (size_t curByte = 0; curByte < m_size; curByte++) {
        auto opcode = m_byteCode[curByte];
        auto byteIndex = curByte;
        
        auto find = m_startStates.find(byteIndex);
        if (find != m_startStates.end()) {
            auto state = find->second;
            for (int i = 0; i < state.local_count(); i++) {
                auto local = state.get_local(i);
                if (local.IsMaybeUndefined) {
                    if (local.Type == &Undefined) {
                        printf(
                            "          %-20s UNDEFINED\r\n",
                            PyUnicode_AsUTF8(PyTuple_GetItem(m_code->co_varnames, i))
                            );
                    }
                    else{
                        printf(
                            "          %-20s %s (maybe UNDEFINED)\r\n",
                            PyUnicode_AsUTF8(PyTuple_GetItem(m_code->co_varnames, i)),
                            local.Type->describe()
                        );
                    }
                }
                else{
                    printf(
                        "          %-20s %s\r\n",
                        PyUnicode_AsUTF8(PyTuple_GetItem(m_code->co_varnames, i)),
                        local.Type->describe()
                    );
                }
            }
            for (int i = 0; i < state.stack_size(); i++) {
                printf("          %-20d %s\r\n", i, state[i]->describe());
            }
        }
        
        if (HAS_ARG(opcode)){
            int oparg = NEXTARG();
            switch (opcode) {
                case SETUP_LOOP:
                case SETUP_EXCEPT:
                case SETUP_FINALLY:
                case JUMP_FORWARD:
                case FOR_ITER:
                    printf("    %-3d %-22s %d (to %d)\r\n", 
                        byteIndex, 
                        opcode_name(opcode), 
                        oparg, 
                        oparg + curByte + 1
                    );
                    break;
                case LOAD_FAST:
                case STORE_FAST:
                case DELETE_FAST:
                    printf("    %-3d %-22s %d (%s)\r\n", 
                        byteIndex, 
                        opcode_name(opcode), 
                        oparg, 
                        PyUnicode_AsUTF8(PyTuple_GetItem(m_code->co_varnames, oparg))
                    );
                    break;
                case LOAD_ATTR:
                case STORE_ATTR:
                case DELETE_ATTR:
                case STORE_NAME:
                case LOAD_NAME:
                case DELETE_NAME:
                case LOAD_GLOBAL:
                case STORE_GLOBAL:
                case DELETE_GLOBAL:
                    printf("    %-3d %-22s %d (%s)\r\n",
                        byteIndex,
                        opcode_name(opcode),
                        oparg,
                        PyUnicode_AsUTF8(PyTuple_GetItem(m_code->co_names, oparg))
                        );
                    break;
                default:
                    printf("    %-3d %-22s %d\r\n", byteIndex, opcode_name(opcode), oparg);
                    break;
            }
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
