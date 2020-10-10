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


/*
 * TODO : New opcodes since 3.6:
 * Need implementation:
 *  - Implement JUMP_IF_NOT_EXC_MATCH
 *  - Implement DICT_MERGE

 *  - Implement WITH_EXCEPT_START
 *  - Implement RERAISE
 *  - Implement END_ASYNC_FOR
 *  - Implement GET_AITER
 *  Implemented (need unit tests)
 *  - Test LOAD_ASSERTION_ERROR
 *  - Implement SETUP_ANNOTATIONS
 *  - Test LOAD_METHOD (Done)
 *  - Test CALL_METHOD (done)
 *  - Test IS_OP (DONE)
 *  - Test DICT_UPDATE (DONE)
 *  - Test CONTAINS_OP (DONE)
 *  - Test SET_UPDATE (DONE)
 *  - Test LIST_EXTEND (DONE)
 *  - Test LIST_TO_TUPLE
 *  - Test ROT_FOUR
 */
#include <Python.h>
#include <opcode.h>
#include <object.h>
#include <deque>
#include <unordered_map>
#include <algorithm>

#include "absint.h"
#include "taggedptr.h"
#include "disasm.h"

#define NUM_ARGS(n) ((n)&0xFF)
#define NUM_KW_ARGS(n) (((n)>>8) & 0xff)

#define GET_OPARG(index)  _Py_OPARG(m_byteCode[(index)/sizeof(_Py_CODEUNIT)])
#define GET_OPCODE(index) _Py_OPCODE(m_byteCode[(index)/sizeof(_Py_CODEUNIT)])


AbstractInterpreter::AbstractInterpreter(PyCodeObject *code, IPythonCompiler* comp) : m_code(code), m_comp(comp) {
    // TODO  : Initialize m_blockIds
    m_byteCode = (_Py_CODEUNIT *)PyBytes_AS_STRING(code->co_code);
    m_size = PyBytes_Size(code->co_code);
    m_returnValue = &Undefined;
    if (comp != nullptr) {
        m_retLabel = comp->emit_define_label();
        m_retValue = comp->emit_define_local();
        m_errorCheckLocal = comp->emit_define_local();
    }
    init_starting_state();
}

AbstractInterpreter::~AbstractInterpreter() {
    // clean up any dynamically allocated objects...
    for (auto source : m_sources) {
        delete source;
    }
    for (auto absValue : m_values) {
        delete absValue;
    }
}

bool AbstractInterpreter::preprocess() {
    if (m_code->co_flags & (CO_COROUTINE | CO_GENERATOR)) {
        // Don't compile co-routines or generators.  We can't rely on
        // detecting yields because they could be optimized out.
#ifdef DEBUG
        printf("Skipping function because it contains a coroutine/generator.");
#endif
        return false;
    }
    for (int i = 0; i < m_code->co_argcount; i++) {
        // all parameters are initially definitely assigned
        m_assignmentState[i] = true;
    }

    int oparg;
    vector<bool> ehKind;
    vector<AbsIntBlockInfo> blockStarts;
    for (size_t curByte = 0; curByte < m_size; curByte += sizeof(_Py_CODEUNIT)) {
        auto opcodeIndex = curByte;
        auto byte = GET_OPCODE(curByte);
        oparg = GET_OPARG(curByte);
    processOpCode:
        while (!blockStarts.empty() &&
            opcodeIndex >= blockStarts[blockStarts.size() - 1].BlockEnd) {
            auto blockStart = blockStarts.back();
            blockStarts.pop_back();
            m_blockStarts[opcodeIndex] = blockStart.BlockStart;
        }

        switch (byte) {
            case POP_BLOCK:
            {
                if (!blockStarts.empty()) {
                    auto blockStart = blockStarts.back();
                    blockStarts.pop_back();
                    m_blockStarts[opcodeIndex] = blockStart.BlockStart;
                }
                break;
            }
            case EXTENDED_ARG:
            {
                curByte += sizeof(_Py_CODEUNIT);
                oparg = (oparg << 8) | GET_OPARG(curByte);
                byte = GET_OPCODE(curByte);
                goto processOpCode;
            }
            case YIELD_FROM:
            case YIELD_VALUE:
#ifdef DEBUG
                printf("Skipping function because it contains a yield operations.");
#endif
                return false;

            case UNPACK_EX:
                if (m_comp != nullptr) {
                    m_sequenceLocals[curByte] = m_comp->emit_allocate_stack_array(((oparg & 0xFF) + (oparg >> 8)) * sizeof(void*));
                }
                break;
            case BUILD_STRING:
            case UNPACK_SEQUENCE:
                // we need a buffer for the slow case, but we need 
                // to avoid allocating it in loops.
                if (m_comp != nullptr) {
                    m_sequenceLocals[curByte] = m_comp->emit_allocate_stack_array(oparg * sizeof(void*));
                }
                break;
            case DELETE_FAST:
                if (oparg < m_code->co_argcount) {
                    // this local is deleted, so we need to check for assignment
                    m_assignmentState[oparg] = false;
                }
                break;
            case SETUP_WITH:
            case SETUP_ASYNC_WITH:
            case SETUP_FINALLY:
            case FOR_ITER:
                blockStarts.push_back(AbsIntBlockInfo(opcodeIndex, oparg + curByte + sizeof(_Py_CODEUNIT), false));
                ehKind.push_back(true);
                break;
            case LOAD_GLOBAL:
            {
                auto name = PyUnicode_AsUTF8(PyTuple_GetItem(m_code->co_names, oparg));
                if (!strcmp(name, "vars") || 
                    !strcmp(name, "dir") || 
                    !strcmp(name, "locals") || 
                    !strcmp(name, "eval")) {
                    // In the future we might be able to do better, e.g. keep locals in fast locals,
                    // but for now this is a known limitation that if you load vars/dir we won't
                    // optimize your code, and if you alias them you won't get the correct behavior.
                    // Longer term we should patch vars/dir/_getframe and be able to provide the
                    // correct values from generated code.
#ifdef DEBUG
                    printf("Skipping function because it contains frame globals.");
#endif
                    return false;
                }
            }
            break;
            case JUMP_FORWARD:
                m_jumpsTo.insert(oparg + curByte + sizeof(_Py_CODEUNIT));
                break;
            case JUMP_ABSOLUTE:
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
            case JUMP_IF_NOT_EXC_MATCH:
            case POP_JUMP_IF_TRUE:
            case POP_JUMP_IF_FALSE:
                m_jumpsTo.insert(oparg);
                break;
        }
    }
    return true;
}

void AbstractInterpreter::set_local_type(int index, AbstractValueKind kind) {
    auto& lastState = m_startStates[0];
//    if (kind == AVK_Integer || kind == AVK_Float) {
//        // Replace our starting state with a local which has a known source
//        // so that we know it's boxed...
//        auto localInfo = AbstractLocalInfo(to_abstract(kind));
//        localInfo.ValueInfo.Sources = new_source(new LocalSource());
//        lastState.replace_local(index, localInfo);
//    }
}

void AbstractInterpreter::init_starting_state() {
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

    update_start_state(lastState, 0);
}

bool AbstractInterpreter::interpret() {
    if (!preprocess()) {
        printf("Failed to preprocess");
        return false;
    }

    // walk all the blocks in the code one by one, analyzing them, and enqueing any
    // new blocks that we encounter from branches.
    deque<size_t> queue;
    queue.push_back(0);
    do {
        int oparg;
        auto cur = queue.front();
        queue.pop_front();
        for (size_t curByte = cur; curByte < m_size; curByte += sizeof(_Py_CODEUNIT)) {
            // get our starting state when we entered this opcode
            InterpreterState lastState = m_startStates.find(curByte)->second;

            auto opcodeIndex = curByte;

            auto opcode = GET_OPCODE(curByte);
            oparg = GET_OPARG(curByte);
#ifdef DUMP_TRACES
            printf("Interpreting %d - OPCODE %s (%d) stack %d\n", opcodeIndex, opcode_name(opcode), oparg, lastState.stack_size());
#endif
        processOpCode:

            int curStackLen = lastState.stack_size();
            int jump = 0;
            switch (opcode) {
                case EXTENDED_ARG: {
                    curByte += sizeof(_Py_CODEUNIT);
                    oparg = (oparg << 8) | GET_OPARG(curByte);
                    opcode = GET_OPCODE(curByte);
                    update_start_state(lastState, curByte);
                    goto processOpCode;
                }
                case NOP:
                    break;
                case ROT_TWO: {
                    auto top = lastState.pop_no_escape();
                    auto second = lastState.pop_no_escape();

                    auto sources = AbstractSource::combine(top.Sources, second.Sources);
                    m_opcodeSources[opcodeIndex] = sources;

                    if (top.Value->kind() != second.Value->kind()) {
                        top.escapes();
                        second.escapes();
                    }

                    lastState.push(top);
                    lastState.push(second);
                    break;
                }
                case ROT_THREE: {
                    auto top = lastState.pop_no_escape();
                    auto second = lastState.pop_no_escape();
                    auto third = lastState.pop_no_escape();

                    auto sources = AbstractSource::combine(
                            top.Sources,
                            AbstractSource::combine(second.Sources, third.Sources));
                    m_opcodeSources[opcodeIndex] = sources;

                    if (top.Value->kind() != second.Value->kind()
                        || top.Value->kind() != third.Value->kind()) {
                        top.escapes();
                        second.escapes();
                        third.escapes();
                    }

                    lastState.push(top);
                    lastState.push(third);
                    lastState.push(second);
                    break;
                }
                case ROT_FOUR: {
                    auto top = lastState.pop_no_escape();
                    auto second = lastState.pop_no_escape();
                    auto third = lastState.pop_no_escape();
                    auto fourth = lastState.pop_no_escape();

                    auto sources = AbstractSource::combine(
                            top.Sources,
                            AbstractSource::combine(second.Sources,
                                                    AbstractSource::combine(third.Sources, fourth.Sources)));
                    m_opcodeSources[opcodeIndex] = sources;

                    if (top.Value->kind() != second.Value->kind()
                        || top.Value->kind() != third.Value->kind()
                        || top.Value->kind() != fourth.Value->kind()) {
                        top.escapes();
                        second.escapes();
                        third.escapes();
                        fourth.escapes();
                    }

                    lastState.push(top);
                    lastState.push(fourth);
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
                case DUP_TOP_TWO: {
                    auto top = lastState[lastState.stack_size() - 1];
                    auto second = lastState[lastState.stack_size() - 2];
                    lastState.push(second);
                    lastState.push(top);
                    break;
                }
                case RERAISE: {
                    // RERAISE not implemented
                    return false;
                    lastState.pop();
                    lastState.pop();
                    lastState.pop();
                    break;
                }
                case LOAD_CONST: {
                    auto constSource = add_const_source(opcodeIndex, oparg);
                    auto value = AbstractValueWithSources(
                            to_abstract(PyTuple_GetItem(m_code->co_consts, oparg)),
                            constSource
                    );
                    lastState.push(value);
                    break;
                }
                case LOAD_FAST: {
                    auto localSource = add_local_source(opcodeIndex, oparg);
                    auto local = lastState.get_local(oparg);

                    local.ValueInfo.Sources = AbstractSource::combine(localSource, local.ValueInfo.Sources);

                    lastState.push(local.ValueInfo);
                    break;
                }
                case STORE_FAST: {
                    auto valueInfo = lastState.pop_no_escape();
                    m_opcodeSources[opcodeIndex] = valueInfo.Sources;
                    lastState.replace_local(oparg, AbstractLocalInfo(valueInfo, valueInfo.Value == &Undefined));
                }
                    break;
                case DELETE_FAST:
                    // We need to box any previous stores so we can delete them...  Otherwise
                    // we won't know if we should raise an unbound local error
                    lastState.get_local(oparg).ValueInfo.escapes();
                    lastState.replace_local(oparg, AbstractLocalInfo(&Undefined, true));
                    break;
                case BINARY_SUBSCR:
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
                case INPLACE_OR: {
                    auto two = lastState.pop();
                    auto one = lastState.pop();
                    lastState.push(&Any);
                }
                    break;
                case POP_JUMP_IF_FALSE: {
                    auto value = lastState.pop_no_escape();

                    // merge our current state into the branched to location...
                    if (update_start_state(lastState, oparg)) {
                        queue.push_back(oparg);
                    }

                    value.Value->truth(value.Sources);
                    if (value.Value->is_always_false()) {
                        // We're always jumping, we don't need to process the following opcodes...
                        goto next;
                    }

                    // we'll continue processing after the jump with our new state...
                    break;
                }
                case POP_JUMP_IF_TRUE: {
                    auto value = lastState.pop_no_escape();

                    // merge our current state into the branched to location...
                    if (update_start_state(lastState, oparg)) {
                        queue.push_back(oparg);
                    }

                    value.Value->truth(value.Sources);
                    if (value.Value->is_always_true()) {
                        // We're always jumping, we don't need to process the following opcodes...
                        goto next;
                    }

                    // we'll continue processing after the jump with our new state...
                    break;
                }
                case JUMP_IF_TRUE_OR_POP: {
                    auto curState = lastState;
                    if (update_start_state(lastState, oparg)) {
                        queue.push_back(oparg);
                    }
                    auto value = lastState.pop_no_escape();
                    value.Value->truth(value.Sources);
                    if (value.Value->is_always_true()) {
                        // we always jump, no need to analyze the following instructions...
                        goto next;
                    }
                }
                    break;
                case JUMP_IF_FALSE_OR_POP: {
                    auto curState = lastState;
                    if (update_start_state(lastState, oparg)) {
                        queue.push_back(oparg);
                    }
                    auto value = lastState.pop_no_escape();
                    value.Value->truth(value.Sources);
                    if (value.Value->is_always_false()) {
                        // we always jump, no need to analyze the following instructions...
                        goto next;
                    }
                }
                    break;
                case JUMP_IF_NOT_EXC_MATCH: {
                    auto curState = lastState;
                    lastState.pop();
                    lastState.pop();

                    if (update_start_state(lastState, oparg)) {
                        queue.push_back(oparg);
                    }
                    goto next;
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
                    if (update_start_state(lastState, (size_t) oparg + curByte + sizeof(_Py_CODEUNIT))) {
                        queue.push_back((size_t) oparg + curByte + sizeof(_Py_CODEUNIT));
                    }
                    // Done processing this basic block, we'll need to see a branch
                    // to the following opcodes before we'll process them.
                    goto next;
                case RETURN_VALUE: {
                    // We don't treat returning as escaping as it would just result in a single
                    // boxing over the lifetime of the function.
                    auto retValue = lastState.pop_no_escape();
                    m_returnValue = m_returnValue->merge_with(retValue.Value);
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
                case DELETE_NAME:
                    break;
                case LOAD_CLASSDEREF:
                    lastState.push(&Any);
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
                case BUILD_TUPLE: {
                    vector<AbstractValueWithSources> sources;
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                        //sources.push_back(lastState.pop_no_escape());
                    }
                    // TODO: Currently we don't mark the consuming portions of the tuple
                    // as escaping until the tuple escapes.  Is that good enough?  That
                    // means that if we have a non-escaping tuple we need to optimize
                    // it away too, otherwise our assumptions about what's in the tuple are
                    // broken.
                    // This optimization is disabled until the above comment sorted out.
                    //auto tuple = new TupleSource(sources);
                    //m_sources.push_back(tuple);
                    //lastState.push(AbstractValueWithSources(&Tuple, tuple));
                    lastState.push(&Tuple);
                    break;
                }
                case BUILD_MAP:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                        lastState.pop();
                    }
                    lastState.push(&Dict);
                    break;
                case COMPARE_OP: {
                    lastState.pop();
                    lastState.pop();
                    lastState.push(&Any);
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
                case CALL_FUNCTION: {
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
                case CALL_FUNCTION_KW: {
                    int na = oparg;

                    // Pop the names tuple
                    auto names = lastState.pop_no_escape();
                    assert(names.Value->kind() == AVK_Tuple);

                    for (int i = 0; i < na; i++) {
                        lastState.pop();
                    }

                    // pop the function
                    lastState.pop();

                    lastState.push(&Any);
                    break;
                }
                case CALL_FUNCTION_EX:
                    if (oparg & 0x01) {
                        // kwargs
                        lastState.pop();
                    }

                    // call args (iterable)
                    lastState.pop();
                    // function
                    lastState.pop();

                    lastState.push(&Any);
                    break;
                case MAKE_FUNCTION: {
                    lastState.pop(); // qual name
                    lastState.pop(); // code

                    if (oparg & 0x08) {
                        // closure object
                        lastState.pop();
                    }
                    if (oparg & 0x04) {
                        // annotations
                        lastState.pop();
                    }
                    if (oparg & 0x02) {
                        // kw defaults
                        lastState.pop();
                    }
                    if (oparg & 0x01) {
                        // defaults
                        lastState.pop();
                    }

                    lastState.push(&Function);
                    break;
                }
                case BUILD_SLICE:
                    for (int i = 0; i < oparg; i++) {
                        lastState.pop();
                    }
                    lastState.push(&Slice);
                    break;
                case UNARY_POSITIVE:
                case UNARY_NEGATIVE:
                case UNARY_INVERT:
                case UNARY_NOT: {
                    lastState.pop();
                    lastState.push(&Any);
                    break;
                }
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
                    switch (oparg) {
                        case 2:
                            lastState.pop_no_escape(); /* cause */
                            /* fall through */
                        case 1:
                            lastState.pop_no_escape(); /* exc */
                            /* fall through */
                        case 0:
                            break;
                    }
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
                    // There is no tracking of cell variables.
                    lastState.pop();
                    break;
                case LOAD_DEREF:
                    // There is no tracking of cell variables.
                    lastState.push(&Any);
                    break;
                case DELETE_DEREF:
                    // Since cell variables are not tracked, no need to worry
                    // about their deletion.
                    break;
                case GET_ITER:
                    // TODO: Known iterable types
                    lastState.pop();
                    lastState.push(&Any);
                    break;
                case FOR_ITER: {
                    // For branches out with the value consumed
                    auto leaveState = lastState;
                    leaveState.pop();
                    if (update_start_state(leaveState, (size_t) oparg + curByte + sizeof(_Py_CODEUNIT))) {
                        queue.push_back((size_t) oparg + curByte + sizeof(_Py_CODEUNIT));
                    }

                    // When we compile this we don't actually leave the value on the stack,
                    // but the sequence of opcodes assumes that happens.  to keep our stack
                    // properly balanced we match what's really going on.
                    lastState.push(&Any);
                    break;
                }
                case POP_BLOCK:
                    // Save the state for when break back into this block
                    merge_states(m_startStates[m_blockStarts[opcodeIndex]], lastState);
                    goto next;
                case POP_EXCEPT:
                    // pop traceback values
                    lastState.pop();
                    lastState.pop();
                    lastState.pop();
                    break;
                case LOAD_BUILD_CLASS:
                    // TODO: if we know this is __builtins__.__build_class__ we can push a special value
                    // to optimize the call.f
                    lastState.push(&Any);
                    break;
                case SET_ADD:
                    lastState.pop();
                    break;
                case LIST_APPEND:
                    // pop the value being stored off, leave list on stack
                    lastState.pop();
                    lastState.pop();
                    lastState.push(&List);
                    break;
                case MAP_ADD:
                    // pop the value and key being stored off, leave list on stack
                    lastState.pop();
                    lastState.pop();
                    break;
                case SETUP_FINALLY: {
                    auto finallyState = lastState;
                    // Finally is entered with value pushed onto stack indicating reason for 
                    // the finally running...
                    finallyState.push(&Any);
                    finallyState.push(&Any);
                    finallyState.push(&Any);
                    finallyState.push(&Any);
                    finallyState.push(&Any);
                    finallyState.push(&Any);
                    if (update_start_state(finallyState, (size_t) oparg + curByte + sizeof(_Py_CODEUNIT))) {
                        queue.push_back((size_t) oparg + curByte + sizeof(_Py_CODEUNIT));
                    }
                    goto next;
                }
                break;
                case FORMAT_VALUE:
                    if ((oparg & FVS_MASK) == FVS_HAVE_SPEC) {
                        // format spec
                        lastState.pop();
                    }
                    lastState.pop();
                    lastState.push(&String);
                    break;
                case BUILD_STRING:
                    for (auto i = 0; i < oparg; i++) {
                        lastState.pop();
                    }
                    lastState.push(&String);
                    break;
                case SETUP_ASYNC_WITH:
                case SETUP_WITH: {
                    auto finallyState = lastState;
                    finallyState.push(&Any);
                    if (update_start_state(finallyState, (size_t) oparg + curByte + sizeof(_Py_CODEUNIT))) {
                        jump = 1;
                        queue.push_back((size_t) oparg + curByte + sizeof(_Py_CODEUNIT));
                    }
                    lastState.push(&Any);
                    goto next;
                }
                case BUILD_CONST_KEY_MAP:
                    lastState.pop(); //keys
                    for (auto i = 0; i < oparg; i++) {
                        lastState.pop(); // values
                    }
                    lastState.push(&Dict);
                    break;
                case LOAD_METHOD:
                    lastState.push(&Any);
                    break;
                case CALL_METHOD:
                {
                    /* LOAD_METHOD could push a NULL value, but (as above)
                     it doesn't, so instead it just considers the same scenario.

                     This is a method call.  Stack layout:

                         ... | method | self | arg1 | ... | argN
                                                            ^- TOP()
                                               ^- (-oparg)
                                        ^- (-oparg-1)
                               ^- (-oparg-2)

                      `self` and `method` will be POPed by call_function.
                      We'll be passing `oparg + 1` to call_function, to
                      make it accept the `self` as a first argument.
                    */
                    lastState.pop();
                    lastState.pop();
                    for (int i = 0 ; i < oparg; i++)
                        lastState.pop();
                    lastState.push(&Any); // push result.
                    break;
                }
                case IS_OP:
                    lastState.pop();
                    lastState.pop();
                    lastState.push(&Bool);
                    break;
                case CONTAINS_OP:
                    lastState.pop();
                    lastState.pop();
                    lastState.push(&Bool);
                    break;
                case WITH_EXCEPT_START: {
                    /* At the top of the stack are 7 values:
                       - (TOP, SECOND, THIRD) = exc_info()
                       - (FOURTH, FIFTH, SIXTH) = previous exception for EXCEPT_HANDLER
                       - SEVENTH: the context.__exit__ bound method
                       We call SEVENTH(TOP, SECOND, THIRD).
                       Then we push again the TOP exception and the __exit__
                       return value.
                    */
                    return false; // not implemented
                    auto top = lastState.pop(); // exc
                    auto second = lastState.pop(); // val
                    auto third = lastState.pop(); // tb
                    auto seventh = lastState[lastState.stack_size() - 7]; // exit_func
                    // TODO : Vectorcall (exit_func, stack+1, 3, ..)
                    assert(false);
                    lastState.push(&Any); // res
                    break;
                }
                case LIST_EXTEND:
                {
                    lastState.pop();
                    lastState.pop();
                    lastState.push(&List);
                    break;
                }
                case DICT_UPDATE:
                {
                    // Calls dict.update(TOS1[-i], TOS). Used to build dicts.
                    lastState.pop(); // value
                    break;
                }
                case SET_UPDATE:
                {
                    lastState.pop(); // value
                    break;
                }
                case PRINT_EXPR:
                {
                    lastState.pop(); // value
                    break;
                }
                case LIST_TO_TUPLE:
                {
                    lastState.pop(); // list
                    lastState.push(&Tuple); // tuple
                    break;
                }
                case LOAD_ASSERTION_ERROR:
                {
                    lastState.push(&Any);
                    break;
                }
                default:
                    PyErr_Format(PyExc_ValueError,
                                 "Unknown unsupported opcode: %s", opcode_name(opcode));
                    return false;
                    break;
            }
            assert(PyCompile_OpcodeStackEffectWithJump(opcode, oparg, jump) == (lastState.stack_size() - curStackLen));
            update_start_state(lastState, curByte + sizeof(_Py_CODEUNIT));
        }

    next:;
    } while (!queue.empty());

    return true;
}

bool AbstractInterpreter::update_start_state(InterpreterState& newState, size_t index) {
    auto initialState = m_startStates.find(index);
    if (initialState != m_startStates.end()) {
        return merge_states(newState, initialState->second);
    }
    else {
        m_startStates[index] = newState;
        return true;
    }
}

bool AbstractInterpreter::merge_states(InterpreterState& newState, InterpreterState& mergeTo) {
    bool changed = false;
    if (mergeTo.m_locals != newState.m_locals) {
        //    if (mergeTo.m_locals.get() != newState.m_locals.get()) {
            // need to merge locals...
        assert(mergeTo.local_count() == newState.local_count());
        for (size_t i = 0; i < newState.local_count(); i++) {
            auto oldType = mergeTo.get_local(i);
            auto newType = oldType.merge_with(newState.get_local(i));
            if (newType != oldType) {
                if (oldType.ValueInfo.needs_boxing()) {
                    newType.ValueInfo.escapes();
                }
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
    else {
        // need to merge the stacks...
        assert(mergeTo.stack_size() >= newState.stack_size());
        for (size_t i = 0; i < newState.stack_size(); i++) {
            auto newType = mergeTo[i].merge_with(newState[i]);
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
    else if (PyComplex_CheckExact(value)) {
        return &Complex;
    }
    else if (PyFunction_Check(value)) {
        return &Function;
    }


    return &Any;
}

AbstractValue* AbstractInterpreter::to_abstract(AbstractValueKind kind) {
    switch (kind) {
        case AVK_None:
            return &None;
        case AVK_Integer:
            return &Integer;
        case AVK_String:
            return &String;
        case AVK_List:
            return &List;
        case AVK_Dict:
            return &Dict;
        case AVK_Tuple:
            return &Tuple;
        case AVK_Bool:
            return &Bool;
        case AVK_Float:
            return &Float;
        case AVK_Bytes:
            return &Bytes;
        case AVK_Set:
            return &Set;
        case AVK_Complex:
            return &Complex;
        case AVK_Function:
            return &Function;
        case AVK_Slice:
            return &Slice;
    }

    return &Any;
}

bool AbstractInterpreter::should_box(size_t opcodeIndex) {
    auto boxInfo = m_opcodeSources.find(opcodeIndex);
    if (boxInfo != m_opcodeSources.end()) {
        if (boxInfo->second == nullptr) {
            return true;
        }
        return boxInfo->second->needs_boxing();
    }
    return true;
}

bool AbstractInterpreter::can_skip_lasti_update(size_t opcodeIndex) {
    switch (GET_OPCODE(opcodeIndex)) {
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
        case COMPARE_OP:
        case LOAD_FAST:
            return !should_box(opcodeIndex);
    }

    return false;

}

void AbstractInterpreter::dump_sources(AbstractSource* sources) {
    if (sources != nullptr) {
        for (auto value : sources->Sources->Sources) {
            printf("              %s (%p)\r\n", value->describe(), value);
        }
    }
}

const char* AbstractInterpreter::opcode_name(int opcode) {
#define OP_TO_STR(x)   case x: return #x;
    switch (opcode) {
        OP_TO_STR(POP_TOP)
        OP_TO_STR(ROT_TWO)
        OP_TO_STR(ROT_THREE)
        OP_TO_STR(DUP_TOP)
        OP_TO_STR(DUP_TOP_TWO)
        OP_TO_STR(ROT_FOUR)
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
        OP_TO_STR(RERAISE)
        OP_TO_STR(WITH_EXCEPT_START)
        OP_TO_STR(GET_AITER)
        OP_TO_STR(GET_ANEXT)
        OP_TO_STR(BEFORE_ASYNC_WITH)
        OP_TO_STR(END_ASYNC_FOR)
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
        OP_TO_STR(GET_YIELD_FROM_ITER)
        OP_TO_STR(PRINT_EXPR)
        OP_TO_STR(LOAD_BUILD_CLASS)
        OP_TO_STR(YIELD_FROM)
        OP_TO_STR(GET_AWAITABLE)
        OP_TO_STR(LOAD_ASSERTION_ERROR)
        OP_TO_STR(INPLACE_LSHIFT)
        OP_TO_STR(INPLACE_RSHIFT)
        OP_TO_STR(INPLACE_AND)
        OP_TO_STR(INPLACE_XOR)
        OP_TO_STR(INPLACE_OR)
        OP_TO_STR(LIST_TO_TUPLE)
        OP_TO_STR(RETURN_VALUE)
        OP_TO_STR(IMPORT_STAR)
        OP_TO_STR(SETUP_ANNOTATIONS)
        OP_TO_STR(YIELD_VALUE)
        OP_TO_STR(POP_BLOCK)
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
        OP_TO_STR(IS_OP)
        OP_TO_STR(CONTAINS_OP)
        OP_TO_STR(JUMP_IF_NOT_EXC_MATCH)
        OP_TO_STR(SETUP_FINALLY)
        OP_TO_STR(LOAD_FAST)
        OP_TO_STR(STORE_FAST)
        OP_TO_STR(DELETE_FAST)
        OP_TO_STR(RAISE_VARARGS)
        OP_TO_STR(CALL_FUNCTION)
        OP_TO_STR(MAKE_FUNCTION)
        OP_TO_STR(BUILD_SLICE)
        OP_TO_STR(LOAD_CLOSURE)
        OP_TO_STR(LOAD_DEREF)
        OP_TO_STR(STORE_DEREF)
        OP_TO_STR(DELETE_DEREF)
        OP_TO_STR(CALL_FUNCTION_EX)
        OP_TO_STR(CALL_FUNCTION_KW)
        OP_TO_STR(SETUP_WITH)
        OP_TO_STR(EXTENDED_ARG)
        OP_TO_STR(LIST_APPEND)
        OP_TO_STR(SET_ADD)
        OP_TO_STR(MAP_ADD)
        OP_TO_STR(LOAD_CLASSDEREF)
        OP_TO_STR(SETUP_ASYNC_WITH)
        OP_TO_STR(FORMAT_VALUE)
        OP_TO_STR(BUILD_CONST_KEY_MAP)
        OP_TO_STR(BUILD_STRING)
        OP_TO_STR(LOAD_METHOD)
        OP_TO_STR(CALL_METHOD)
        OP_TO_STR(LIST_EXTEND)
        OP_TO_STR(SET_UPDATE)
        OP_TO_STR(DICT_MERGE)
        OP_TO_STR(DICT_UPDATE)
    }
    return "unknown";
}


// Returns information about the specified local variable at a specific 
// byte code index.
AbstractLocalInfo AbstractInterpreter::get_local_info(size_t byteCodeIndex, size_t localIndex) {
    return m_startStates[byteCodeIndex].get_local(localIndex);
}

// Returns information about the stack at the specific byte code index.
vector<AbstractValueWithSources>& AbstractInterpreter::get_stack_info(size_t byteCodeIndex) {
    return m_startStates[byteCodeIndex].m_stack;
}

AbstractValue* AbstractInterpreter::get_return_info() {
    return m_returnValue;
}

bool AbstractInterpreter::has_info(size_t byteCodeIndex) {
    return m_startStates.find(byteCodeIndex) != m_startStates.end();
}


AbstractSource* AbstractInterpreter::add_local_source(size_t opcodeIndex, size_t localIndex) {
    auto store = m_opcodeSources.find(opcodeIndex);
    if (store == m_opcodeSources.end()) {
        return m_opcodeSources[opcodeIndex] = new_source(new LocalSource());
    }

    return store->second;
}

AbstractSource* AbstractInterpreter::add_const_source(size_t opcodeIndex, size_t constIndex) {
    auto store = m_opcodeSources.find(opcodeIndex);
    if (store == m_opcodeSources.end()) {
        return m_opcodeSources[opcodeIndex] = new_source(new ConstSource());
    }

    return store->second;
}

AbstractSource* AbstractInterpreter::add_intermediate_source(size_t opcodeIndex) {
    auto store = m_opcodeSources.find(opcodeIndex);
    if (store == m_opcodeSources.end()) {
        return m_opcodeSources[opcodeIndex] = new_source(new IntermediateSource());
    }

    return store->second;
}

/*****************
 * Code generation

 */


 /************************************************************************
 * Compiler driver code...
 */



 // Checks to see if we have a non-zero error code on the stack, and if so,
 // branches to the current error handler.  Consumes the error code in the process
void AbstractInterpreter::int_error_check(const char* reason) {
    auto noErr = m_comp->emit_define_label();
    m_comp->emit_int(0);
    m_comp->emit_branch(BranchEqual, noErr);

    branch_raise(reason);
    m_comp->emit_mark_label(noErr);
}

// Checks to see if we have a null value as the last value on our stack
// indicating an error, and if so, branches to our current error handler.
void AbstractInterpreter::error_check(const char *reason) {
    auto noErr = m_comp->emit_define_label();
    m_comp->emit_dup();
    m_comp->emit_store_local(m_errorCheckLocal);
    m_comp->emit_null();
    m_comp->emit_branch(BranchNotEqual, noErr);

    branch_raise(reason);
    m_comp->emit_mark_label(noErr);
    m_comp->emit_load_local(m_errorCheckLocal);
}

Label AbstractInterpreter::getOffsetLabel(int jumpTo) {
    auto jumpToLabelIter = m_offsetLabels.find(jumpTo);
    Label jumpToLabel;
    if (jumpToLabelIter == m_offsetLabels.end()) {
        m_offsetLabels[jumpTo] = jumpToLabel = m_comp->emit_define_label();
    }
    else {
        jumpToLabel = jumpToLabelIter->second;
    }
    return jumpToLabel;
}

void AbstractInterpreter::ensure_raise_and_free_locals(size_t localCount) {
    while (m_raiseAndFreeLocals.size() <= localCount) {
        m_raiseAndFreeLocals.push_back(m_comp->emit_define_local());
    }
}

void AbstractInterpreter::spill_stack_for_raise(size_t localCount) {
    ensure_raise_and_free_locals(localCount);
    for (size_t i = 0; i < localCount; i++) {
        m_comp->emit_store_local(m_raiseAndFreeLocals[i]);
    }
}

vector<Label>& AbstractInterpreter::get_raise_and_free_labels(size_t blockId) {
    while (m_raiseAndFree.size() <= blockId) {
        m_raiseAndFree.emplace_back();
    }

    return m_raiseAndFree[blockId];
}

vector<Label>& AbstractInterpreter::get_reraise_and_free_labels(size_t blockId) {
    while (m_reraiseAndFree.size() <= blockId) {
        m_reraiseAndFree.emplace_back();
    }

    return m_reraiseAndFree[blockId];
}

size_t AbstractInterpreter::clear_value_stack() {
    auto& ehBlock = get_ehblock();
    auto& entry_stack = ehBlock.EntryStack;

    // clear any non-object values from the stack up
    // to the stack that owned the block when we entered.
    size_t count = m_stack.size() - entry_stack.size();

    for (auto cur = m_stack.rbegin(); cur != m_stack.rend(); cur++) {
        if (*cur == STACK_KIND_VALUE) {
            count--;
            m_comp->emit_pop();
        }
        else {
            break;
        }
    }
    return count;
}

void AbstractInterpreter::ensure_labels(vector<Label>& labels, size_t count) {
    for (auto i = labels.size(); i < count; i++) {
        labels.push_back(m_comp->emit_define_label());
    }
}

void AbstractInterpreter::branch_raise(const char *reason) {
    auto& ehBlock = get_ehblock();
    auto& entry_stack = ehBlock.EntryStack;

#ifdef DEBUG
    if (reason != nullptr) {
        m_comp->emit_debug_msg(reason);
    }
#endif

    // number of stack entries we need to clear...
    size_t count = m_stack.size() - entry_stack.size();    
    
    auto cur = m_stack.rbegin();
    for (; cur != m_stack.rend() && count >= 0; cur++) {
        if (*cur == STACK_KIND_VALUE) {
            count--;
            m_comp->emit_pop();
        }
        else {
            break;
        }
    }

    if (!count) {
        // No values on the stack, we can just branch directly to the raise label
        m_comp->emit_branch(BranchAlways, ehBlock.Raise);
        return;
    }

    vector<Label>& labels = get_raise_and_free_labels(ehBlock.RaiseAndFreeId);
    ensure_labels(labels, count);
    ensure_raise_and_free_locals(count);

    // continue walking our stack iterator
    for (auto i = 0; i < count; cur++, i++) {
        if (*cur == STACK_KIND_VALUE) {
            // pop off the stack value...
            m_comp->emit_pop();

            // and store null into our local that needs to be freed
            m_comp->emit_null();
            m_comp->emit_store_local(m_raiseAndFreeLocals[i]);
        }
        else {
            m_comp->emit_store_local(m_raiseAndFreeLocals[i]);
        }
    }
    m_comp->emit_branch(BranchAlways, labels[count - 1]);
}

void AbstractInterpreter::clean_stack_for_reraise() {
    auto ehBlock = get_ehblock();

    auto& entry_stack = ehBlock.EntryStack;
    size_t count = m_stack.size() - entry_stack.size();

    for (size_t i = m_stack.size(); i-- > entry_stack.size();) {
        m_comp->emit_pop_top();
    }
}

void AbstractInterpreter::build_tuple(size_t argCnt) {
    m_comp->emit_new_tuple(argCnt);
    if (argCnt != 0) {
        error_check("tuple build failed");
        m_comp->emit_tuple_store(argCnt);
        dec_stack(argCnt);
    }
}

void AbstractInterpreter::extend_tuple(size_t argCnt) {
    extend_list(argCnt);
    m_comp->emit_list_to_tuple();
    error_check("extend tuple failed");
}

void AbstractInterpreter::build_list(size_t argCnt) {
    m_comp->emit_new_list(argCnt);
    error_check("build list failed");
    if (argCnt != 0) {
        m_comp->emit_list_store(argCnt);
    }
    dec_stack(argCnt);
}

void AbstractInterpreter::extend_list_recursively(Local listTmp, size_t argCnt) {
    if (argCnt == 0) {
        return;
    }

    auto valueTmp = m_comp->emit_define_local();
    m_comp->emit_store_local(valueTmp);
    dec_stack();

    extend_list_recursively(listTmp, --argCnt);

    m_comp->emit_load_local(valueTmp); // arg 2
    m_comp->emit_load_local(listTmp);  // arg 1

    m_comp->emit_list_extend();
    int_error_check("list extend failed");

    m_comp->emit_free_local(valueTmp);
}

void AbstractInterpreter::extend_list(size_t argCnt) {
    assert(argCnt > 0);
    auto listTmp = m_comp->emit_spill();
    dec_stack();
    extend_list_recursively(listTmp, argCnt);
    m_comp->emit_load_and_free_local(listTmp);
    inc_stack(1, STACK_KIND_OBJECT);
}

void AbstractInterpreter::build_set(size_t argCnt) {
    m_comp->emit_new_set();
    error_check("build set failed");

    if (argCnt != 0) {
        auto setTmp = m_comp->emit_define_local();
        m_comp->emit_store_local(setTmp);
        Local* tmps = new Local[argCnt];
        Label* frees = new Label[argCnt];
        for (auto i = 0; i < argCnt; i++) {
            tmps[argCnt - (i + 1)] = m_comp->emit_spill();
            dec_stack();
        }

        // load all the values into the set...
        auto err = m_comp->emit_define_label();
        for (int i = 0; i < argCnt; i++) {
            m_comp->emit_load_local(setTmp);
            m_comp->emit_load_local(tmps[i]);
            m_comp->emit_set_add();
            frees[i] = m_comp->emit_define_label();
            m_comp->emit_branch(BranchFalse, frees[i]);
        }

        auto noErr = m_comp->emit_define_label();
        m_comp->emit_branch(BranchAlways, noErr);

        m_comp->emit_mark_label(err);
        m_comp->emit_load_local(setTmp);
        m_comp->emit_pop_top();
        
        // In the event of an error we need to free any
        // args that weren't processed.  We'll always process
        // the 1st value and dec ref it in the set add helper.
        // tmps[0] = 'a', tmps[1] = 'b', tmps[2] = 'c'
        // We'll process tmps[0], and if that fails, then we need
        // to free tmps[1] and tmps[2] which correspond with frees[0]
        // and frees[1]
        for (size_t i = 1; i < argCnt; i++) {
            m_comp->emit_mark_label(frees[i - 1]);
            m_comp->emit_load_local(tmps[i]);
            m_comp->emit_pop_top();
        }

        // And if the last one failed, then all of the values have been
        // decref'd
        m_comp->emit_mark_label(frees[argCnt - 1]);
        branch_raise("set add failed");

        m_comp->emit_mark_label(noErr);
        delete[] frees;
        delete[] tmps;

        m_comp->emit_load_local(setTmp);
        m_comp->emit_free_local(setTmp);
    }
    inc_stack();
}

void AbstractInterpreter::build_map(size_t  argCnt) {
    m_comp->emit_new_dict(argCnt);
    error_check("build map failed");

    if (argCnt > 0) {
        auto map = m_comp->emit_spill();
        for (size_t curArg = 0; curArg < argCnt; curArg++) {
            m_comp->emit_load_local(map);

            m_comp->emit_dict_store();

            dec_stack(2);
            int_error_check("dict store failed");
        }
        m_comp->emit_load_and_free_local(map);
    }
}

void AbstractInterpreter::make_function(int oparg) {
    m_comp->emit_new_function();
    dec_stack(2);

    if (oparg & 0x0f) {
        auto func = m_comp->emit_spill();
        if (oparg & 0x08) {
            // closure
            auto tmp = m_comp->emit_spill();
            m_comp->emit_load_local(func);
            m_comp->emit_load_and_free_local(tmp);
            m_comp->emit_set_closure();
            dec_stack();
        }
        if (oparg & 0x04) {
            // annoations
            auto tmp = m_comp->emit_spill();
            m_comp->emit_load_local(func);
            m_comp->emit_load_and_free_local(tmp);

            m_comp->emit_set_annotations();
            dec_stack();
        }
        if (oparg & 0x02) {
            // kw defaults
            auto tmp = m_comp->emit_spill();
            m_comp->emit_load_local(func);
            m_comp->emit_load_and_free_local(tmp);

            m_comp->emit_set_kw_defaults();
            dec_stack();
        }
        if (oparg & 0x01) {
            // defaults
            auto tmp = m_comp->emit_spill();
            m_comp->emit_load_local(func);
            m_comp->emit_load_and_free_local(tmp);
            m_comp->emit_set_defaults();
            dec_stack();
        }
        m_comp->emit_load_and_free_local(func);
    }

    inc_stack();
}

void AbstractInterpreter::dec_stack(size_t size) {
    assert(m_stack.size() >= size);
    for (size_t i = 0; i < size; i++) {
        m_stack.pop_back();
    }
}

void AbstractInterpreter::inc_stack(size_t size, bool kind) {
    for (size_t i = 0; i < size; i++) {
        m_stack.push_back(kind);
    }
}

// Frees our iteration temporary variable which gets allocated when we hit
// a FOR_ITER.  Used when we're breaking from the current loop.
void AbstractInterpreter::free_iter_local() {

}

// Frees all of the iteration variables in a range. Used when we're
// going to branch to a finally through multiple loops.
void AbstractInterpreter::free_all_iter_locals(size_t to) {

}

// Frees all of our iteration variables.  Used when we're unwinding the function
// on an exception.
void AbstractInterpreter::free_iter_locals_on_exception() {

}

void AbstractInterpreter::periodic_work() {
    m_comp->emit_periodic_work();
    int_error_check("periodic work");
}

int AbstractInterpreter::get_extended_opcode(int curByte) {
    auto opcode = GET_OPCODE(curByte);
    while (opcode == EXTENDED_ARG) {
        curByte += 2;
        opcode = GET_OPCODE(curByte);
    }
    return opcode;
}

// Handles POP_JUMP_IF_FALSE/POP_JUMP_IF_TRUE with a possible error value on the stack.
// If the value on the stack is -1, we branch to the current error handler.
// Otherwise branches based if the current value is true/false based upon the current opcode 
void AbstractInterpreter::branch_or_error(int& curByte) {
    curByte += sizeof(_Py_CODEUNIT);
    auto jmpType = GET_OPCODE(curByte);
    auto oparg = GET_OPARG(curByte);
    while (jmpType == EXTENDED_ARG) {
        curByte += sizeof(_Py_CODEUNIT);
        oparg = (oparg << 8) | GET_OPARG(curByte);
        jmpType = GET_OPCODE(curByte);
    }
    mark_offset_label(curByte);

    raise_on_negative_one();

    if (oparg <= curByte) {
        auto tmp = m_comp->emit_spill();
        periodic_work();
        m_comp->emit_load_and_free_local(tmp);
    }

    m_comp->emit_branch(jmpType == POP_JUMP_IF_FALSE ? BranchFalse : BranchTrue, getOffsetLabel(oparg));
    m_offsetStack[oparg] = m_stack;
}

// Checks to see if -1 is the current value on the stack, and if so, falls into
// the logic for raising an exception.  If not execution continues forward progress.
// Used for checking if an API reports an error (typically true/false/-1)
void AbstractInterpreter::raise_on_negative_one() {
    m_comp->emit_dup();
    m_comp->emit_int(-1);

    auto noErr = m_comp->emit_define_label();
    m_comp->emit_branch(BranchNotEqual, noErr);
    // we need to issue a leave to clear the stack as we may have
    // values on the stack...

    m_comp->emit_pop();
    branch_raise();
    m_comp->emit_mark_label(noErr);
}

// Handles POP_JUMP_IF_FALSE/POP_JUMP_IF_TRUE with a bool value known to be on the stack.
// Branches based if the current value is true/false based upon the current opcode 
void AbstractInterpreter::branch(int& curByte) {
    curByte += sizeof(_Py_CODEUNIT);
    auto jmpType = GET_OPCODE(curByte);
    auto oparg = GET_OPARG(curByte);
    while (jmpType == EXTENDED_ARG) {
        curByte += sizeof(_Py_CODEUNIT);
        oparg = (oparg << 8) | GET_OPARG(curByte);
        jmpType = GET_OPCODE(curByte);
    }

    mark_offset_label(curByte);

    if (oparg <= curByte) {
        periodic_work();
    }
    m_comp->emit_branch(jmpType == POP_JUMP_IF_FALSE ? BranchFalse : BranchTrue, getOffsetLabel(oparg));
    dec_stack();
    m_offsetStack[oparg] = m_stack;
}

JittedCode* AbstractInterpreter::compile_worker() {
    Label ok;

    auto raiseNoHandlerLabel = m_comp->emit_define_label();
    auto reraiseNoHandlerLabel = m_comp->emit_define_label();

    m_comp->emit_lasti_init();
    m_comp->emit_push_frame();

    m_blockStack.push_back(BlockInfo(-1, NOP, 0));
    m_allHandlers.push_back(
        ExceptionHandler(
            0,
            ExceptionVars(),
            raiseNoHandlerLabel, 
            reraiseNoHandlerLabel, 
            Label(),
            vector<bool>()
        )
    );

    for (int curByte = 0; curByte < m_size; curByte += sizeof(_Py_CODEUNIT)) {
        assert(curByte % sizeof(_Py_CODEUNIT) == 0);

        auto opcodeIndex = curByte;

        auto byte = GET_OPCODE(curByte);
        auto oparg = GET_OPARG(curByte);

    processOpCode:
        // See FOR_ITER for special handling of the offset label
        if (get_extended_opcode(curByte) != FOR_ITER) {
#ifdef DUMP_TRACES
            printf("Building offset for %d OPCODE %s (%d)\n", opcodeIndex, opcode_name(byte), oparg);
#endif
            mark_offset_label(curByte);
        }

        auto curStackDepth = m_offsetStack.find(curByte);
        if (curStackDepth != m_offsetStack.end()) {
            m_stack = curStackDepth->second;
        }

#ifdef DUMP_TRACES
        printf("Compiling OPCODE %s (%d) stack %d\n", opcode_name(byte), oparg, m_stack.size());
        int ilLen = m_comp->il_length();
#endif

        if (m_blockStack.size() > 1 && 
            curByte >= m_blockStack.back().EndOffset &&
            m_blockStack.back().EndOffset != -1) {
            compile_pop_block();
        }

        // update f_lasti
        if (!can_skip_lasti_update(curByte)) {
            m_comp->emit_lasti_update(curByte);
        }

        int curStackSize = m_stack.size();

        m_comp->emit_breakpoint();

        switch (byte) {
            case NOP: break;
            case ROT_TWO: 
            {
                std::swap(m_stack[m_stack.size() - 1], m_stack[m_stack.size() - 2]);

                if (!should_box(opcodeIndex)) {
                    auto stackInfo = get_stack_info(opcodeIndex);
                    auto top_kind = stackInfo[stackInfo.size() - 1].Value->kind();
                    auto second_kind = stackInfo[stackInfo.size() - 2].Value->kind();

                    if (top_kind == AVK_Float && second_kind == AVK_Float) {
                        m_comp->emit_rot_two(LK_Float);
                        break;
                    } else if (top_kind == AVK_Integer && second_kind == AVK_Integer) {
                        m_comp->emit_rot_two(LK_Pointer);
                        break;
                    }
                }
                m_comp->emit_rot_two();
                break;
            }
            case ROT_THREE: 
            {
                std::swap(m_stack[m_stack.size() - 1], m_stack[m_stack.size() - 2]);
                std::swap(m_stack[m_stack.size() - 2], m_stack[m_stack.size() - 3]);

                if (!should_box(opcodeIndex)) {
                    auto stackInfo = get_stack_info(opcodeIndex);
                    auto top_kind = stackInfo[stackInfo.size() - 1].Value->kind();
                    auto second_kind = stackInfo[stackInfo.size() - 2].Value->kind();
                    auto third_kind = stackInfo[stackInfo.size() - 3].Value->kind();

                    if (top_kind == AVK_Float && second_kind == AVK_Float && third_kind == AVK_Float) {
                        m_comp->emit_rot_three(LK_Float);
                        break;
                    }
                    else if (top_kind == AVK_Integer && second_kind == AVK_Integer && third_kind == AVK_Integer) {
                        m_comp->emit_rot_three(LK_Pointer);
                        break;
                    }
                }
                m_comp->emit_rot_three();
                break;
            }
            case POP_TOP:
                m_comp->emit_pop_top();
                dec_stack();
                break;
            case DUP_TOP:
                m_comp->emit_dup_top();
                m_stack.push_back(m_stack.back());
                break;
            case DUP_TOP_TWO:
                inc_stack(2);
                m_comp->emit_dup_top_two();
                break;
            case COMPARE_OP:
                m_comp->emit_compare_object(oparg);
                dec_stack(2);
                error_check("failed to compare");
                inc_stack(1);
                break;
            case LOAD_BUILD_CLASS:
                m_comp->emit_load_build_class();
                error_check("load build class failed");
                inc_stack();
                break;
            case JUMP_ABSOLUTE: jump_absolute(oparg, opcodeIndex); break;
            case JUMP_FORWARD:  jump_absolute(oparg + curByte + sizeof(_Py_CODEUNIT), opcodeIndex); break;
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP: jump_if_or_pop(byte != JUMP_IF_FALSE_OR_POP, opcodeIndex, oparg); break;
            case POP_JUMP_IF_TRUE:
            case POP_JUMP_IF_FALSE: pop_jump_if(byte != POP_JUMP_IF_FALSE, opcodeIndex, oparg); break;
            case LOAD_NAME:
                m_comp->emit_load_name(PyTuple_GetItem(m_code->co_names, oparg));
                error_check("load name failed");
                inc_stack();
                break;
            case STORE_ATTR:
                m_comp->emit_store_attr(PyTuple_GetItem(m_code->co_names, oparg));
                dec_stack(2);
                int_error_check("store attr failed");
                break;
            case DELETE_ATTR:
                m_comp->emit_delete_attr(PyTuple_GetItem(m_code->co_names, oparg));
                dec_stack();
                int_error_check("delete attr failed");
                break;
            case LOAD_ATTR:
                m_comp->emit_load_attr(PyTuple_GetItem(m_code->co_names, oparg));
                dec_stack();
                error_check("load attr failed");
                inc_stack();
                break;
            case STORE_GLOBAL:
                m_comp->emit_store_global(PyTuple_GetItem(m_code->co_names, oparg));
                dec_stack();
                int_error_check("store global failed");
                break;
            case DELETE_GLOBAL:
                m_comp->emit_delete_global(PyTuple_GetItem(m_code->co_names, oparg));
                int_error_check("delete global failed");
                break;
            case LOAD_GLOBAL:
                m_comp->emit_load_global(PyTuple_GetItem(m_code->co_names, oparg));
                error_check("load global failed");
                inc_stack();
                break;
            case LOAD_CONST: load_const(oparg, opcodeIndex); break;
            case STORE_NAME:
                m_comp->emit_store_name(PyTuple_GetItem(m_code->co_names, oparg));
                dec_stack();
                int_error_check("store name failed");
                break;
            case DELETE_NAME:
                m_comp->emit_delete_name(PyTuple_GetItem(m_code->co_names, oparg));
                int_error_check("delete name failed");
                break;
            case DELETE_FAST:
                load_fast_worker(oparg, true);
                m_comp->emit_pop_top();
                m_comp->emit_delete_fast(oparg);
                break;
            case STORE_FAST: store_fast(oparg, opcodeIndex); break;
            case LOAD_FAST: load_fast(oparg, opcodeIndex); break;
            case UNPACK_SEQUENCE:
                unpack_sequence(oparg, curByte);
                break;
            case UNPACK_EX: unpack_ex(oparg, curByte); break;
            case CALL_FUNCTION_KW:
                // names is a tuple on the stack, should have come from a LOAD_CONST
                if (!m_comp->emit_kwcall(oparg)) {
                    auto names = m_comp->emit_spill();
                    dec_stack();    // names
                    build_tuple(oparg);
                    m_comp->emit_load_and_free_local(names);

                    m_comp->emit_kwcall_with_tuple();
                    dec_stack();// function & names
                }
                else {
                    dec_stack(oparg + 2); // + function & names
                }

                error_check("kwcall failed");
                inc_stack();
                break;
            case CALL_FUNCTION_EX:
                if (oparg & 0x01) {
                    // kwargs, then args, then function
                    m_comp->emit_call_kwargs();
                    dec_stack(3);
                }else{
                    m_comp->emit_call_args();
                    dec_stack(2);
                }

                error_check("call failed");
                inc_stack();
                break;
            case CALL_FUNCTION:
            {
                if (!m_comp->emit_call(oparg)) {
                    build_tuple(oparg);
                    m_comp->emit_call_with_tuple();
                    dec_stack();// function
                }
                else {
                    dec_stack(oparg + 1); // + function
                }
                
                error_check("call function failed");
                inc_stack();
                break;
            }
            case BUILD_TUPLE:
                build_tuple(oparg);
                inc_stack();
                break;
            case BUILD_LIST:
                build_list(oparg);
                inc_stack();
                break;
            case BUILD_MAP:
                build_map(oparg);
                inc_stack();
                break;
            case STORE_SUBSCR:
                dec_stack(3);
                m_comp->emit_store_subscr();
                int_error_check("store subscr failed");
                break;
            case DELETE_SUBSCR:
                dec_stack(2);
                m_comp->emit_delete_subscr();
                int_error_check("delete subscr failed");
                break;
            case BUILD_SLICE:
                dec_stack(oparg);
                if (oparg != 3) {
                    m_comp->emit_null();
                }
                m_comp->emit_build_slice();
                inc_stack();
                break;
            case BUILD_SET:
                build_set(oparg);
                break;
            case UNARY_POSITIVE: unary_positive(opcodeIndex); break;
            case UNARY_NEGATIVE: unary_negative(opcodeIndex); break;
            case UNARY_NOT:
                m_comp->emit_unary_not();
                dec_stack(1);
                error_check("unary not failed");
                inc_stack();
                break;
            case UNARY_INVERT:
                m_comp->emit_unary_invert();
                dec_stack(1);
                error_check("unary invert failed");
                inc_stack();
                break;
            case BINARY_SUBSCR:
            case BINARY_ADD:
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
                m_comp->emit_binary_object(byte);
                dec_stack(2);
                error_check("binary op failed");
                inc_stack();
                break;
            case RETURN_VALUE: return_value(opcodeIndex); break;
            case EXTENDED_ARG:
            {
                curByte += sizeof(_Py_CODEUNIT);
                oparg = (oparg << 8) | GET_OPARG(curByte);
                byte = GET_OPCODE(curByte);
                
                goto processOpCode;
            }
            case MAKE_FUNCTION:
                make_function(oparg);
                break;
            case LOAD_DEREF:
                m_comp->emit_load_deref(oparg);
                error_check("load deref failed");
                inc_stack();
                break;
            case STORE_DEREF:
                dec_stack();
                m_comp->emit_store_deref(oparg);
                break;
            case DELETE_DEREF: m_comp->emit_delete_deref(oparg); break;
            case LOAD_CLOSURE:
                m_comp->emit_load_closure(oparg);
                inc_stack();
                break;
            case GET_ITER:
                // GET_ITER can be followed by FOR_ITER, or a CALL_FUNCTION.                
            {
                /*
                bool optFor = false;
                Local loopOpt1, loopOpt2;
                if (GET_OPCODE(curByte + sizeof(_Py_CODEUNIT)) == FOR_ITER) {
                for (size_t blockIndex = m_blockStack.size() - 1; blockIndex != (-1); blockIndex--) {
                if (m_blockStack[blockIndex].Kind == SETUP_LOOP) {
                // save our iter variable so we can free it on break, continue, return, and
                // when encountering an exception.
                m_blockStack.data()[blockIndex].LoopOpt1 = loopOpt1 = m_comp->emit_define_local(Parameter(CORINFO_TYPE_NATIVEINT));
                m_blockStack.data()[blockIndex].LoopOpt2 = loopOpt2 = m_comp->emit_define_local(Parameter(CORINFO_TYPE_NATIVEINT));
                optFor = true;
                break;
                }
                }
                }
                */
                /*if (optFor) {
                m_comp->emit_getiter_opt();
                }
                else*/ {
                    m_comp->emit_getiter();
                    dec_stack();
                    error_check("get iter failed");
                    inc_stack();
                }
            }
            break;
            case FOR_ITER:
            {
                BlockInfo *loopBlock = nullptr;
                for_iter(
                    curByte + oparg + sizeof(_Py_CODEUNIT), 
                    opcodeIndex, 
                    loopBlock
                );
                inc_stack();
                break;
            }
            case SET_ADD:
                assert(oparg == 1); // TODO : Shift down stack to oparg when != 1
                // Calls set.update(TOS1[-i], TOS). Used to build sets.
                m_comp->emit_set_add();
                dec_stack(2); // set, value
                error_check("set update failed");
                inc_stack(1); // set
                break;
            case MAP_ADD:
                m_comp->emit_map_add();
                dec_stack(3);
                error_check("map add failed");
                inc_stack();
                break;
            case LIST_APPEND: {
                assert(oparg == 1); // TODO : Shift down stack to oparg when != 1
                // Calls list.append(TOS1[-i], TOS).
                m_comp->emit_list_append();
                dec_stack(2); // list, value
                error_check("list append failed");
                inc_stack(1);
                break;
            }
            case PRINT_EXPR:
                m_comp->emit_print_expr();
                dec_stack();
                int_error_check("print expr failed");
                break;
            case LOAD_CLASSDEREF:
                m_comp->emit_load_classderef(oparg);
                error_check("load classderef failed");
                inc_stack();
                break;
            case RAISE_VARARGS:
                // do raise (exception, cause)
                // We can be invoked with no args (bare raise), raise exception, or raise w/ exceptoin and cause
                switch (oparg) {
                    case 0: m_comp->emit_null();
                    case 1: m_comp->emit_null();
                    case 2:
                        dec_stack(oparg);
                        // raise exc
                        m_comp->emit_raise_varargs();
                        // returns 1 if we're doing a re-raise in which case we don't need
                        // to update the traceback.  Otherwise returns 0.
                        auto& curHandler = get_ehblock();
                        auto stackDepth = m_stack.size() - curHandler.EntryStack.size();
                        if (oparg == 0) {
                            bool noStack = true;
                            if (stackDepth != 0) {
                                // This is pretty rare, you need to do a re-raise with something
                                // already on the stack.  One way to get to it is doing a bare raise
                                // inside of finally block.  We spill the result of the raise into a
                                // temporary, then we clear up any values on the stack, and we generate
                                // a set of labels for freeing objects on the stack for the re-raise.
                                auto raiseTmp = m_comp->emit_define_local(LK_Bool);
                                m_comp->emit_store_local(raiseTmp);
                                auto count = clear_value_stack();

                                auto& raiseLabels = get_raise_and_free_labels(curHandler.RaiseAndFreeId);
                                auto& reraiseLabels = get_reraise_and_free_labels(curHandler.RaiseAndFreeId); 

                                m_comp->emit_load_and_free_local(raiseTmp);
                                if (count != 0) {
                                    ensure_labels(raiseLabels, count);
                                    ensure_labels(reraiseLabels, count);

                                    spill_stack_for_raise(count);
                                    m_comp->emit_branch(BranchFalse, raiseLabels[count - 1]);
                                    m_comp->emit_branch(BranchAlways, reraiseLabels[count - 1]);
                                    noStack = false;
                                }
                            }

                            if (noStack) {
                                // The stack actually ended up being empty - either because we didn't
                                // have any values, or the values were all non-objects that we could
                                // spill eagerly.
                                m_comp->emit_branch(BranchFalse, curHandler.Raise);
                                m_comp->emit_branch(BranchAlways, curHandler.ReRaise);
                            }
                        }
                        else {
                            // if we have args we'll always return 0...
                            m_comp->emit_pop();
                            branch_raise();
                        }
                        break;
                }
                break;
            case SETUP_FINALLY:
            {
                auto handlerLabel = getOffsetLabel(oparg + curByte + sizeof(_Py_CODEUNIT));
                auto blockInfo = BlockInfo(oparg + curByte + sizeof(_Py_CODEUNIT), SETUP_FINALLY, m_allHandlers.size());

                m_blockStack.push_back(blockInfo);
                m_allHandlers.push_back(
                    ExceptionHandler(
                        m_allHandlers.size(),
                        ExceptionVars(m_comp, true),
                        m_comp->emit_define_label(),
                        m_comp->emit_define_label(),
                        handlerLabel,
                        m_stack,
                        EHF_TryFinally
                    )
                );

                vector<bool> newStack = m_stack;
                newStack.push_back(STACK_KIND_OBJECT);
                m_offsetStack[oparg + curByte + sizeof(_Py_CODEUNIT)] = newStack;
                inc_stack(6);
            }
            break;
            case POP_EXCEPT: pop_except(); break;
            case POP_BLOCK: compile_pop_block(); break;

            case YIELD_FROM:
            case YIELD_VALUE:
                printf("Unsupported opcode: %d (with related)\r\n", byte);
                return nullptr;

            case IMPORT_NAME:
                m_comp->emit_import_name(PyTuple_GetItem(m_code->co_names, oparg));
                dec_stack(2);
                error_check("import name failed");
                inc_stack();
                break;
            case IMPORT_FROM:
                m_comp->emit_import_from(PyTuple_GetItem(m_code->co_names, oparg));
                error_check("import from failed");
                inc_stack();
                break;
            case IMPORT_STAR:
                m_comp->emit_import_star();
                dec_stack(1);
                int_error_check("import star failed");
                break;
            case SETUP_WITH:
                printf("Unsupported opcode: %d (with related)\r\n", byte);
                return nullptr;
            case FORMAT_VALUE:
            {
                Local fmtSpec;
                if ((oparg & FVS_MASK) == FVS_HAVE_SPEC) {
                    // format spec
                    fmtSpec = m_comp->emit_spill();
                    dec_stack();
                }

                int which_conversion = oparg & FVC_MASK;

                dec_stack();
                if (which_conversion) {
                    // Save the original value so we can decref it...
                    m_comp->emit_dup();
                    auto tmp = m_comp->emit_spill();

                    // Convert it
                    switch (which_conversion) {
                        case FVC_STR:   m_comp->emit_pyobject_str();   break;
                        case FVC_REPR:  m_comp->emit_pyobject_repr();  break;
                        case FVC_ASCII: m_comp->emit_pyobject_ascii(); break;
                    }

                    // Decref the original value
                    m_comp->emit_load_and_free_local(tmp);
                    m_comp->emit_pop_top();

                    // Custom error handling in case we have a spilled spec
                    // we need to free as well.
                    auto noErr = m_comp->emit_define_label();
                    m_comp->emit_dup();
                    m_comp->emit_store_local(m_errorCheckLocal);
                    m_comp->emit_null();
                    m_comp->emit_branch(BranchNotEqual, noErr);

                    if ((oparg & FVS_MASK) == FVS_HAVE_SPEC) {
                        m_comp->emit_load_local(fmtSpec);
                        m_comp->emit_pop_top();
                    }

                    branch_raise("conversion failed");
                    m_comp->emit_mark_label(noErr);
                    m_comp->emit_load_local(m_errorCheckLocal);
                }

                if ((oparg & FVS_MASK) == FVS_HAVE_SPEC) {
                    // format spec
                    m_comp->emit_load_and_free_local(fmtSpec);
                    m_comp->emit_pyobject_format();

                    error_check("format object");
                }
                else if (!which_conversion) {
                    // TODO: This could also be avoided if we knew we had a string on the stack

                    // If we did a conversion we know we have a string...
                    // Otherwise we need to convert
                    m_comp->emit_format_value();
                }

                inc_stack();
                break;
            }
            case BUILD_STRING:
            {
                    Local stackArray = m_sequenceLocals[curByte];
                    Local tmp;
                    for (auto i = 0; i < oparg; i++) {
                        m_comp->emit_store_to_array(stackArray, oparg - i - 1);
                        dec_stack();
                    }

                    // Array
                    m_comp->emit_load_local(stackArray);
                    // Count
                    m_comp->emit_ptr((void*)oparg);

                    m_comp->emit_unicode_joinarray();

                    inc_stack();
                    break;
            }
            case BUILD_CONST_KEY_MAP:
            {
                /*
                 * The version of BUILD_MAP specialized for constant keys.
                 * Pops the top element on the stack which contains a tuple of keys,
                 * then starting from TOS1, pops count values to form values
                 * in the built dictionary.
                 */
                auto keys = m_comp->emit_spill();
                m_comp->emit_new_dict(oparg);
                auto dict = m_comp->emit_spill();

                for (auto i = 0; i < oparg; i++) {

                    auto value = m_comp->emit_spill();
                    // key
                    m_comp->emit_load_local(keys); // arg 2 (key)
                    m_comp->emit_tuple_load(i);
                    m_comp->emit_load_and_free_local(value); // arg 1 (val)
                    m_comp->emit_load_local(dict); // arg 0 (dict/map)
                    m_comp->emit_dict_store_no_decref();
                    dec_stack(1);
                }
                dec_stack(1); // keys
                m_comp->emit_load_local(keys);
                m_comp->emit_pop_top();

                m_comp->emit_free_local(keys);

                m_comp->emit_load_local(dict);

                inc_stack();
                break;
            }
            case LIST_EXTEND:
            {
                assert(oparg == 1); // TODO : Shift down stack to oparg when != 1
                m_comp->emit_list_extend();
                dec_stack(2);
                error_check("list extend failed");
                inc_stack(1);
                // Takes list, value from stack and pushes list back onto stack
                break;
            }
            case DICT_UPDATE:
            {
                assert(oparg == 1); // TODO : Shift down stack to oparg when != 1
                // Calls dict.update(TOS1[-i], TOS). Used to build dicts.
                m_comp->emit_dict_update();
                dec_stack(2); // dict, item
                error_check("dict update failed");
                inc_stack(1);
                break;
            }
            case SET_UPDATE:
            {
                assert(oparg == 1); // TODO : Shift down stack to oparg when != 1
                // Calls set.update(TOS1[-i], TOS). Used to build sets.
                m_comp->emit_set_extend();
                dec_stack(2); // set, iterable
                error_check("set update failed");
                inc_stack(1); // set
                break;
            }
            case LIST_TO_TUPLE:
            {
                m_comp->emit_list_to_tuple();
                dec_stack(1); // list
                error_check("list to tuple failed");
                inc_stack(1); // tuple
                break;
            }
            case IS_OP:
            {
                m_comp->emit_is(oparg);
                dec_stack(2);
                error_check("is check failed");
                inc_stack(1);
                break;
            }
            case CONTAINS_OP:
            {
                // Oparg is 0 if "in", and 1 if "not in"
                if (oparg == 0)
                    m_comp->emit_in();
                else
                    m_comp->emit_not_in();
                dec_stack(2); // in/not in takes two
                inc_stack(); // pushes result
                break;
            }
            case LOAD_METHOD:
            {
                m_comp->emit_dup_top(); // dup self as needs to remain on stack
                m_comp->emit_load_method(PyTuple_GetItem(m_code->co_names, oparg));
                inc_stack(1);
                break;
            }
            case CALL_METHOD:
            {
                if (!m_comp->emit_method_call(oparg)) {
                    // TODO : emit method call with >4 args
                }
                else {
                    dec_stack(oparg + 2); // + method + name + nargs
                }

                inc_stack(); //result
                break;
            }
            case LOAD_ASSERTION_ERROR :
            {
                m_comp->emit_load_assertion_error();
                inc_stack();
                break;
            }
            default:
                printf("Unsupported opcode: %d (with related)\r\n", byte);

                return nullptr;
        }
        assert(PyCompile_OpcodeStackEffect(byte, oparg) == (m_stack.size() - curStackSize));

#ifdef DUMP_TRACES
        m_comp->dump(ilLen);
#endif
    }

    // for each exception handler we need to load the exception
    // information onto the stack, and then branch to the correct
    // handler.  When we take an error we'll branch down to this
    // little stub and then back up to the correct handler.
    if (!m_allHandlers.empty()) {
        // TODO: Unify the first handler with this loop
        for (size_t i = 1; i < m_allHandlers.size(); i++) {
            auto& handler = m_allHandlers[i];

            emit_raise_and_free(i);

            if (handler.ErrorTarget.m_index != -1) {
                m_comp->emit_prepare_exception(
                    handler.ExVars.PrevExc,
                    handler.ExVars.PrevExcVal,
                    handler.ExVars.PrevTraceback
                );
                if (handler.Flags & EHF_TryFinally) {
                    auto tmpEx = m_comp->emit_spill();

                    auto targetHandler = i;
                    while (m_allHandlers[targetHandler].BackHandler != -1) {
                        targetHandler = m_allHandlers[targetHandler].BackHandler;
                    }
                    auto& vars = m_allHandlers[targetHandler].ExVars;

                    m_comp->emit_store_local(vars.FinallyValue);
                    m_comp->emit_store_local(vars.FinallyTb);

                    m_comp->emit_load_and_free_local(tmpEx);
                }
                m_comp->emit_branch(BranchAlways, handler.ErrorTarget);
            }
        }
    }

    // label we branch to for error handling when we have no EH handlers, return NULL.
    emit_raise_and_free(0);

    m_comp->emit_null();
    auto finalRet = m_comp->emit_define_label();
    m_comp->emit_branch(BranchAlways, finalRet);

    m_comp->emit_mark_label(m_retLabel);
    m_comp->emit_load_local(m_retValue);

    m_comp->emit_mark_label(finalRet);
    m_comp->emit_pop_frame();

    m_comp->emit_ret(1);

    return m_comp->emit_compile();
}

void AbstractInterpreter::compile_pop_block() {
    auto curHandler = m_blockStack.back();
    m_blockStack.pop_back();
    if (curHandler.Kind == SETUP_FINALLY) {
        // convert block into an END_FINALLY/POP_EXCEPT BlockInfo
        auto back = m_blockStack.back();

        auto& prevHandler = m_allHandlers[curHandler.CurrentHandler];
        auto& backHandler = m_allHandlers[back.CurrentHandler];

        auto newBlock = BlockInfo(
            back.EndOffset,
            curHandler.Kind == SETUP_FINALLY ? 0 : POP_EXCEPT, // TODO : Find equivalent
            m_allHandlers.size(),
            curHandler.Flags,
            curHandler.ContinueOffset
        );

        // For exceptions in a except/finally block we need to unwind the
        // current exceptions before raising a new exception.  When we emit
        // the unwind code we use the exception vars that we created for the
        // try portion of the block, so we just flow those in here, but when we
        // hit an error we'll branch to any previous handler that was on the
        // stack.
        EhFlags flags = EHF_None;
        ExceptionVars exVars = prevHandler.ExVars;
        if (backHandler.Flags & EHF_TryFinally) {
            flags |= EHF_TryFinally;
        }

        m_allHandlers.emplace_back(
            ExceptionHandler(
                m_allHandlers.size(),
                exVars,
                m_comp->emit_define_label(),
                m_comp->emit_define_label(),
                backHandler.ErrorTarget,
                backHandler.EntryStack,
                flags | EHF_InExceptHandler,
                back.CurrentHandler
            )
        );

        m_blockStack.push_back(newBlock);
    }
}

const char* AbstractInterpreter::op_to_string(int op) {
    switch(op) {
        case BINARY_AND: 
        case INPLACE_AND:
            return "&";
        case  INPLACE_OR:
        case  BINARY_OR:
            return "|";
        case  INPLACE_LSHIFT: 
        case  BINARY_LSHIFT: 
            return "<<";
        case  INPLACE_RSHIFT:
        case  BINARY_RSHIFT:
            return ">>";
        case  INPLACE_XOR:
        case  BINARY_XOR:
            return "^";
    }
    return "?";
}

void AbstractInterpreter::emit_raise_and_free(size_t handlerIndex) {
    auto handler = m_allHandlers[handlerIndex];
    auto reraiseAndFreeLabels = get_reraise_and_free_labels(handler.RaiseAndFreeId);
    for (auto cur = reraiseAndFreeLabels.size() - 1; cur != -1; cur--) {
        m_comp->emit_mark_label(reraiseAndFreeLabels[cur]);
        m_comp->emit_load_local(m_raiseAndFreeLocals[cur]);
        m_comp->emit_pop_top();
    }
    if (reraiseAndFreeLabels.size() != 0) {
        m_comp->emit_branch(BranchAlways, handler.ReRaise);
    }
    auto raiseAndFreeLabels = get_raise_and_free_labels(handler.RaiseAndFreeId);
    for (auto cur = raiseAndFreeLabels.size() - 1; cur != -1; cur--) {
        m_comp->emit_mark_label(raiseAndFreeLabels[cur]);
        m_comp->emit_load_local(m_raiseAndFreeLocals[cur]);
        m_comp->emit_pop_top();
    }

    if (handler.Flags & EHF_InExceptHandler) {
        // We're in an except handler and we're raising a new exception.  First we need
        // to unwind the current exception.  If we're doing a raise we need to update the
        // traceback with our line/frame information.  If we're doing a re-raise we need
        // to skip that.
        auto prepare = m_comp->emit_define_label();

        m_comp->emit_mark_label(handler.Raise);
        unwind_eh(handlerIndex);

        m_comp->emit_eh_trace();     // update the traceback

        if (handler.ErrorTarget.m_index == -1) {
            // We're in an except handler raising an exception with no outer exception
            // handlers.  We'll return NULL from the function indicating an error has
            // occurred
            m_comp->emit_branch(BranchAlways, m_allHandlers[0].Raise);
        }
        else {
            m_comp->emit_branch(BranchAlways, prepare);
        }

        m_comp->emit_mark_label(handler.ReRaise);

        unwind_eh(handlerIndex);

        m_comp->emit_mark_label(prepare);
        if (handler.ErrorTarget.m_index == -1) {
            // We're in an except handler re-raising an exception with no outer exception
            // handlers.  We'll return NULL from the function indicating an error has
            // occurred
            m_comp->emit_branch(BranchAlways, m_allHandlers[0].ReRaise);
        }
    }
    else {
        // We're not in a nested exception handler, we just need to emit our
        // line tracing information if we're doing a raise, and skip it if
        // we're a re-raise.  Then we prepare the exception and branch to 
        // whatever opcode will handle the exception.
        m_comp->emit_mark_label(handler.Raise);

        m_comp->emit_eh_trace();

        m_comp->emit_mark_label(handler.ReRaise);
    }

}

void AbstractInterpreter::unwind_loop(Local finallyReason, EhFlags branchKind, int continueOffset) {
    size_t clearEh = -1;
    for (size_t i = m_blockStack.size() - 1; i != -1; i--) {
        if (m_blockStack[i].Kind == SETUP_FINALLY) {
            // need to dispatch to outer finally...
            if (clearEh != -1) {
                unwind_eh(
                    m_blockStack[clearEh].CurrentHandler, 
                    m_allHandlers[m_blockStack[clearEh].CurrentHandler].BackHandler
                );
            }

            m_comp->emit_load_local(finallyReason);
            m_comp->emit_branch(BranchAlways, m_allHandlers[m_blockStack[i].CurrentHandler].ErrorTarget);
            break;
        }
        else if (m_blockStack[i].Kind == POP_EXCEPT) {
            if (clearEh == -1) {
                clearEh = i;
            }
        }
    }
}

void AbstractInterpreter::test_bool_and_branch(Local value, bool isTrue, Label target) {
    m_comp->emit_load_local(value);
    m_comp->emit_ptr(isTrue ? Py_False : Py_True);
    m_comp->emit_branch(BranchEqual, target);
}

void AbstractInterpreter::jump_if_or_pop(bool isTrue, int opcodeIndex, int jumpTo) {
    auto stackInfo = get_stack_info(opcodeIndex);
    auto one = stackInfo[stackInfo.size() - 1];

    if (jumpTo <= opcodeIndex) {
        periodic_work();
    }

    auto target = getOffsetLabel(jumpTo);
    m_offsetStack[jumpTo] = m_stack;
    dec_stack();
    
    if (!one.needs_boxing()) {
        switch (one.Value->kind()) {
            case AVK_Float:
                m_comp->emit_dup();
                m_comp->emit_float(0);
                m_comp->emit_branch(isTrue ? BranchNotEqual : BranchEqual, target);
                m_comp->emit_pop_top();
                return;
            case AVK_Integer:
                m_comp->emit_dup();
                m_comp->emit_unary_not_tagged_int_push_bool();
                m_comp->emit_branch(isTrue ? BranchFalse : BranchTrue, target);
                m_comp->emit_pop_top();
                return;
        }
    }
    
    auto tmp = m_comp->emit_spill();
    auto noJump = m_comp->emit_define_label();
    auto willJump = m_comp->emit_define_label();

    // fast checks for true/false.
    test_bool_and_branch(tmp, isTrue, noJump);
    test_bool_and_branch(tmp, !isTrue, willJump);

    // Use PyObject_IsTrue
    m_comp->emit_load_local(tmp);
    m_comp->emit_is_true();

    raise_on_negative_one();

    m_comp->emit_branch(isTrue ? BranchFalse : BranchTrue, noJump);

    // Jumping, load the value back and jump
    m_comp->emit_mark_label(willJump);
    m_comp->emit_load_local(tmp);    // load the value back onto the stack
    m_comp->emit_branch(BranchAlways, target);

    // not jumping, load the value and dec ref it
    m_comp->emit_mark_label(noJump);
    m_comp->emit_load_local(tmp);
    m_comp->emit_pop_top();

    m_comp->emit_free_local(tmp);
    inc_stack();
}

void AbstractInterpreter::pop_jump_if(bool isTrue, int opcodeIndex, int jumpTo) {
    auto stackInfo = get_stack_info(opcodeIndex);
    auto one = stackInfo[stackInfo.size() - 1];

    if (jumpTo <= opcodeIndex) {
        periodic_work();
    }
    auto target = getOffsetLabel(jumpTo);

    auto noJump = m_comp->emit_define_label();
    auto willJump = m_comp->emit_define_label();

    // fast checks for true/false...
    m_comp->emit_dup();
    m_comp->emit_ptr(isTrue ? Py_False : Py_True);
    m_comp->emit_branch(BranchEqual, noJump);

    m_comp->emit_dup();
    m_comp->emit_ptr(isTrue ? Py_True : Py_False);
    m_comp->emit_branch(BranchEqual, willJump);

    // Use PyObject_IsTrue
    m_comp->emit_dup();
    m_comp->emit_is_true();

    raise_on_negative_one();

    m_comp->emit_branch(isTrue ? BranchFalse : BranchTrue, noJump);

    // Branching, pop the value and branch
    m_comp->emit_mark_label(willJump);
    m_comp->emit_pop_top();
    m_comp->emit_branch(BranchAlways, target);

    // Not branching, just pop the value and fall through
    m_comp->emit_mark_label(noJump);
    m_comp->emit_pop_top();

    dec_stack();
    m_offsetStack[jumpTo] = m_stack;
}

void AbstractInterpreter::unary_positive(int opcodeIndex) {
    auto stackInfo = get_stack_info(opcodeIndex);
    auto one = stackInfo[stackInfo.size() - 1];

    switch (one.Value->kind()) {
        case AVK_Float:
        case AVK_Integer:
            // nop
            break;
        default:
            dec_stack();
            m_comp->emit_unary_positive();
            error_check("unary positive failed");
            inc_stack();
            break;
    }
}

void AbstractInterpreter::unary_negative(int opcodeIndex) {
    auto stackInfo = get_stack_info(opcodeIndex);
    auto one = stackInfo[stackInfo.size() - 1];

    bool handled = false;
    if (!one.needs_boxing()) {
        switch (one.Value->kind()) {
            case AVK_Float:
                handled = true;
                dec_stack();
                m_comp->emit_unary_negative_float();
                inc_stack(1, STACK_KIND_VALUE);
                break;
            case AVK_Integer:
                handled = true;
                dec_stack();
                m_comp->emit_unary_negative_tagged_int();
                inc_stack();
                break;
        }
    }

    if (!handled) {
        dec_stack();
        m_comp->emit_unary_negative();
        error_check("unary negative failed");
        inc_stack();
    }
}

/* Unused for now */
bool AbstractInterpreter::can_optimize_pop_jump(int opcodeIndex) {
    auto opcode = get_extended_opcode(opcodeIndex + sizeof(_Py_CODEUNIT));
    if (opcode == POP_JUMP_IF_TRUE || opcode == POP_JUMP_IF_FALSE) {
        return m_jumpsTo.find(opcodeIndex + sizeof(_Py_CODEUNIT)) == m_jumpsTo.end();
    }
    return false;
}

void AbstractInterpreter::unary_not(int& opcodeIndex) {
    m_comp->emit_unary_not();
    error_check("unary not failed");
    inc_stack();
}

JittedCode* AbstractInterpreter::compile() {
    bool interpreted = interpret();
    if (!interpreted) {
        printf("Failed to interpret");
        return nullptr;
    }
    
    return compile_worker();
}

bool AbstractInterpreter::can_skip_lasti_update(int opcodeIndex) {
    switch (GET_OPCODE(opcodeIndex)) {
        case DUP_TOP:
        case NOP:
        case ROT_TWO:
        case ROT_THREE:
        case ROT_FOUR:
        case POP_BLOCK:
        case POP_JUMP_IF_FALSE:
        case POP_JUMP_IF_TRUE:
        case POP_TOP:
        case DUP_TOP_TWO:
        case LOAD_CONST:
        case JUMP_FORWARD:
        case JUMP_ABSOLUTE:
            return true;
    }

    return false;
}

void AbstractInterpreter::store_fast(int local, int opcodeIndex) {
    m_comp->emit_store_fast(local);
    dec_stack();
}

void AbstractInterpreter::load_const(int constIndex, int opcodeIndex) {
    auto constValue = PyTuple_GetItem(m_code->co_consts, constIndex);
    m_comp->emit_ptr(constValue);
    m_comp->emit_dup();
    m_comp->emit_incref();
    inc_stack();
}

void AbstractInterpreter::return_value(int opcodeIndex) {
    dec_stack();

    m_comp->emit_store_local(m_retValue);

    size_t clearEh = -1;
    bool inFinally = false;
    for (size_t blockIndex = m_blockStack.size() - 1; blockIndex != (-1); blockIndex--) {
        if (m_blockStack[blockIndex].Kind == SETUP_FINALLY) {
            // we need to run the finally before returning...
            m_blockStack[blockIndex].Flags |= EHF_BlockReturns;

            if (!inFinally) {
                // Only emit the store and branch to the inner most finally, but
                // we need to mark all finallys as being capable of being returned
                // through.
                inFinally = true;
                if (clearEh != -1) {
                    unwind_eh(
                        m_blockStack[clearEh].CurrentHandler, 
                        m_allHandlers[m_blockStack[blockIndex].CurrentHandler].BackHandler
                    );
                }
                free_all_iter_locals(blockIndex);
                m_comp->emit_int(EHF_BlockReturns);
                m_comp->emit_branch(BranchAlways, m_allHandlers[m_blockStack[blockIndex].CurrentHandler].ErrorTarget);
            }
        }
        else if (m_blockStack[blockIndex].Kind == POP_EXCEPT) {
            // we need to restore the previous exception before we return
            if (clearEh == -1) {
                clearEh = blockIndex;
            }
        }
    }

    if (!inFinally) {
        if (clearEh != -1) {
            unwind_eh(m_blockStack[clearEh].CurrentHandler);
        }
        free_all_iter_locals();
        m_comp->emit_branch(BranchLeave, m_retLabel);
    }
}


void AbstractInterpreter::unpack_sequence(size_t size, int opcode) {
    auto valueTmp = m_comp->emit_spill();
    dec_stack();

    auto success = m_comp->emit_define_label();
    m_comp->emit_unpack_sequence(valueTmp, m_sequenceLocals[opcode], success, size);

    branch_raise();

    m_comp->emit_mark_label(success);
    auto fastTmp = m_comp->emit_spill();

    // Equivalent to CPython's:
    //while (oparg--) {
    //    item = items[oparg];
    //    Py_INCREF(item);
    //    PUSH(item);
    //}

    auto tmpOpArg = size;
    while (tmpOpArg--) {
        m_comp->emit_load_local(fastTmp);
        m_comp->emit_load_array(tmpOpArg);
        inc_stack();
    }

    m_comp->emit_load_and_free_local(valueTmp);
    m_comp->emit_pop_top();

    m_comp->emit_free_local(fastTmp);
}

void AbstractInterpreter::for_iter(int loopIndex, int opcodeIndex, BlockInfo *loopInfo) {
    // CPython always generates LOAD_FAST or a GET_ITER before a FOR_ITER.
    // Therefore we know that we always fall into a FOR_ITER when it is
    // initialized, and we branch back to it for the loop condition.  We
    // do this because keeping the value on the stack becomes problematic.
    // At the very least it requires that we spill the value out when we
    // are doing a "continue" in a for loop.

    // oparg is where to jump on break
    auto iterValue = m_comp->emit_spill();
    dec_stack();
    //bool inLoop = false;
    //Local loopOpt1, loopOpt2;
    if (loopInfo != nullptr) {
        loopInfo->LoopVar = iterValue;
    }

    // now that we've saved the value into a temp we can mark the offset
    // label.
    mark_offset_label(opcodeIndex);

    m_comp->emit_load_local(iterValue);

    // TODO: It'd be nice to inline this...
    auto processValue = m_comp->emit_define_label();

    m_comp->emit_for_next(processValue, iterValue);

    int_error_check("for_iter failed");

    jump_absolute(loopIndex, opcodeIndex);

    m_comp->emit_mark_label(processValue);
    inc_stack();
}

void AbstractInterpreter::compare_op(int compareType, int& i, int opcodeIndex) {
    // TODO : Validate against new opcode
    if (get_extended_opcode(i + sizeof(_Py_CODEUNIT)) == POP_JUMP_IF_FALSE) {
        m_comp->emit_compare_exceptions_int();
        dec_stack(2);
        branch_or_error(i);
    }
    else {
        // this will actually not currently be reached due to the way
        // CPython generates code, but is left for completeness.
        m_comp->emit_compare_exceptions();
        dec_stack(2);
        error_check("exc match failed");
        inc_stack();
    }
}

void AbstractInterpreter::load_fast(int local, int opcodeIndex) {
    if (!should_box(opcodeIndex)) {
        // We have an optimized local...
        auto localInfo = get_local_info(opcodeIndex, local);
        auto kind = localInfo.ValueInfo.Value->kind();
        // We only optimize floats so far...
        if (kind == AVK_Float) {
            m_comp->emit_load_local(get_optimized_local(local, AVK_Float));
            inc_stack(1, STACK_KIND_VALUE);
            return;
        }
        else if (kind == AVK_Integer) {
            m_comp->emit_load_local(get_optimized_local(local, AVK_Any));
            m_comp->emit_dup();
            m_comp->emit_incref(true);
            inc_stack();
            return;
        }
    }

    bool checkUnbound = m_assignmentState.find(local) == m_assignmentState.end() || !m_assignmentState.find(local)->second;
    load_fast_worker(local, checkUnbound);
    inc_stack();
}

void AbstractInterpreter::load_fast_worker(int local, bool checkUnbound) {
    m_comp->emit_load_fast(local);

    if (checkUnbound) {
        Label success = m_comp->emit_define_label();

        m_comp->emit_dup();
        m_comp->emit_store_local(m_errorCheckLocal);
        m_comp->emit_null();
        m_comp->emit_branch(BranchNotEqual, success);

        m_comp->emit_ptr(PyTuple_GetItem(m_code->co_varnames, local));

        m_comp->emit_unbound_local_check();
        
        branch_raise();

        m_comp->emit_mark_label(success);
        m_comp->emit_load_local(m_errorCheckLocal);
    }

    m_comp->emit_dup();
    m_comp->emit_incref(false);
}

void AbstractInterpreter::unpack_ex(size_t size, int opcode) {
    auto valueTmp = m_comp->emit_spill();
    auto listTmp = m_comp->emit_define_local();
    auto remainderTmp = m_comp->emit_define_local();

    dec_stack();

    m_comp->emit_unpack_ex(valueTmp, size & 0xff, size >> 8, m_sequenceLocals[opcode], listTmp, remainderTmp);
    // load the iterable, the sizes, and our temporary 
    // storage if we need to iterate over the object, 
    // the list local address, and the remainder address
    // PyObject* seq, size_t leftSize, size_t rightSize, PyObject** tempStorage, PyObject** list, PyObject*** remainder

    error_check("unpack ex failed"); // TODO: We leak the sequence on failure

    auto fastTmp = m_comp->emit_spill();

    // load the right hand side...
    auto tmpOpArg = size >> 8;
    while (tmpOpArg--) {
        m_comp->emit_load_local(remainderTmp);
        m_comp->emit_load_array(tmpOpArg);
        inc_stack();
    }

    // load the list
    m_comp->emit_load_and_free_local(listTmp);
    inc_stack();
    // load the left hand side, Equivalent to CPython's:
    //while (oparg--) {
    //    item = items[oparg];
    //    Py_INCREF(item);
    //    PUSH(item);
    //}

    tmpOpArg = size & 0xff;
    while (tmpOpArg--) {
        m_comp->emit_load_local(fastTmp);
        m_comp->emit_load_array(tmpOpArg);
        inc_stack();
    }

    m_comp->emit_load_and_free_local(valueTmp);
    m_comp->emit_pop_top();

    m_comp->emit_free_local(fastTmp);
    m_comp->emit_free_local(remainderTmp);
}

void AbstractInterpreter::jump_absolute(size_t index, size_t from) { 
    if (index <= from) {
        periodic_work();
    }

    m_offsetStack[index] = m_stack;
    m_comp->emit_branch(BranchAlways, getOffsetLabel(index));
}


// Unwinds exception handling starting at the current handler.  Emits the unwind for all
// of the current handlers until we reach one which will actually handle the current
// exception.
void AbstractInterpreter::unwind_eh(size_t fromHandler, size_t toHandler) {
    auto cur = fromHandler;
    do {
        auto& exVars = m_allHandlers[cur].ExVars;

        if (exVars.PrevExc.is_valid()) {
            m_comp->emit_unwind_eh(exVars.PrevExc, exVars.PrevExcVal, exVars.PrevTraceback);
        }

        cur = m_allHandlers[cur].BackHandler;
    } while (cur != -1 && cur != toHandler && !(m_allHandlers[cur].Flags & (EHF_TryExcept | EHF_TryFinally)));
}

ExceptionHandler& AbstractInterpreter::get_ehblock() {
    return m_allHandlers[m_blockStack.back().CurrentHandler];
}

// We want to maintain a mapping between locations in the Python byte code
// and generated locations in the code.  So for each Python byte code index
// we define a label in the generated code.  If we ever branch to a specific
// opcode then we'll branch to the generated label.
void AbstractInterpreter::mark_offset_label(int index) {
    auto existingLabel = m_offsetLabels.find(index);
    if (existingLabel != m_offsetLabels.end()) {
#ifdef DUMP_TRACES
        printf("Label for %d exists, marking label %d\n", index, existingLabel->second.m_index);
#endif
        m_comp->emit_mark_label(existingLabel->second);
    }
    else {
#ifdef DUMP_TRACES
        printf("Label for %d doesnt exist, defining new\n", index);
#endif
        auto label = m_comp->emit_define_label();
        m_offsetLabels[index] = label;
        m_comp->emit_mark_label(label);
    }
}

LocalKind get_optimized_local_kind(AbstractValueKind kind) {
    // Remove optimizations for now, always use PyObject* /Lk_ptr/ NATIVEINT
    return LK_Pointer;
}

Local AbstractInterpreter::get_optimized_local(int index, AbstractValueKind kind) {
    if (m_optLocals.find(index) == m_optLocals.end()) {
        m_optLocals[index] = unordered_map<AbstractValueKind, Local, AbstractValueKindHash>();
    }
    auto& map = m_optLocals.find(index)->second;
    if (map.find(kind) == map.end()) {
        return map[kind] = m_comp->emit_define_local(get_optimized_local_kind(kind));
    }
    return map.find(kind)->second;
}


void AbstractInterpreter::pop_except() {
    // we made it to the end of an EH block w/o throwing,
    // clear the exception.
    auto block = m_blockStack.back();
    unwind_eh(block.CurrentHandler, m_allHandlers[block.CurrentHandler].BackHandler);
#ifdef DEBUG_TRACE
    m_il.ld_i("Exception cleared");
    m_il.emit_call(METHOD_DEBUG_TRACE);
#endif
}

void AbstractInterpreter::debug_log(const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);
    const int bufferSize = 181;
    char* buffer = new char[bufferSize];
    vsnprintf(buffer, bufferSize, fmt, args);
    va_end(args);
    m_comp->emit_debug_msg(buffer);
}

EhFlags operator | (EhFlags lhs, EhFlags rhs) {
    return (EhFlags)(static_cast<int>(lhs) | static_cast<int>(rhs));
}

EhFlags operator |= (EhFlags& lhs, EhFlags rhs) {
    lhs = (EhFlags)(static_cast<int>(lhs) | static_cast<int>(rhs));
    return lhs;
}
