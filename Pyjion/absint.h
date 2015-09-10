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

#ifndef __ABSINT_H__
#define __ABSINT_H__

#include <Python.h>
#include <vector>
#include <unordered_map>

#include "absvalue.h"
#include "cowvector.h"

using namespace std;

struct AbstractLocalInfo;
struct AbsIntBlockInfo;
class InterpreterState;

// The abstract interpreter implementation.  The abstract interpreter performs 
// static analysis of the Python byte code to determine what types are known.
// Ultimately this information will feedback into code generation allowing
// more efficient code to be produced.
//
// The abstract interpreter ultimately produces a set of states for each opcode
// before it has been executed.  It also produces an abstract value for the type
// that the function returns.
//
// The abstract interpreter walks the byte code updating the stack of the stack
// and locals based upon the opcode being performed and the existing state of the
// stack.  When it encounters a branch it will merge the current state in with the
// state for where we're branching to.  If the merge results in a new starting state
// that we haven't analyzed it will then queue the target opcode as the next starting 
// point to be analyzed. 
//
// If the branch is unconditional, or definitively taken based upon analysis, then 
// we'll go onto the next starting opcode to be analyzed.
//
// Once we've processed all of the blocks of code in this manner the analysis
// is complete.

class __declspec(dllexport) AbstractInterpreter {
#pragma warning (disable:4251)
    // ** Results produced:
    // Tracks the interpreter state before each opcode
    unordered_map<size_t, InterpreterState> m_startStates;
    AbstractValue* m_returnValue;

    // ** Inputs:
    PyCodeObject* m_code;
    unsigned char *m_byteCode;
    size_t m_size;

    // ** Data consumed during analysis:
    // Tracks whether an END_FINALLY is being consumed by a finally block (true) or exception block (false)
    unordered_map<size_t, bool> m_endFinallyIsFinally;
    // Tracks the entry point for each POP_BLOCK opcode, so we can restore our
    // stack state back after the POP_BLOCK
    unordered_map<size_t, size_t> m_blockStarts;
    // Tracks the location where each BREAK_LOOP will break to, so we can merge
    // state with the current state to the breaked location.
    unordered_map<size_t, AbsIntBlockInfo> m_breakTo;
    unordered_map<size_t, AbstractSource*> m_opcodeSources;
    // all values produced during abstract interpretation, need to be freed
    vector<AbstractValue*> m_values;
    vector<AbstractSource*> m_sources;
#pragma warning (default:4251)

public:
    AbstractInterpreter(PyCodeObject *code);
    ~AbstractInterpreter();

    bool interpret();
    void dump();

    // Returns information about the specified local variable at a specific 
    // byte code index.
    AbstractLocalInfo get_local_info(size_t byteCodeIndex, size_t localIndex);

    // Returns information about the stack at the specific byte code index.
    vector<AbstractValueWithSources>& get_stack_info(size_t byteCodeIndex);

    // Returns true if the result of the opcode should be boxed, false if it
    // can be maintained on the stack.
    bool should_box(size_t opcodeIndex);

    bool can_skip_lasti_update(size_t opcodeIndex);

    AbstractValue* get_return_info();

    bool has_info(size_t byteCodeIndex);

private:

    AbstractValue* to_abstract(PyObject* obj);
    bool merge_states(InterpreterState& newState, int index);
    bool merge_states(InterpreterState& newState, InterpreterState& mergeTo);
    bool update_start_state(InterpreterState& newState, size_t index);
    void init_starting_state();
    char* opcode_name(int opcode);
    void preprocess();
    void dump_sources(AbstractSource* sources);
    AbstractSource* new_source(AbstractSource* source) {
        m_sources.push_back(source);
        return source;
    }

    AbstractSource* add_local_source(size_t opcodeIndex, size_t localIndex);
    AbstractSource* add_const_source(size_t opcodeIndex, size_t constIndex);
    AbstractSource* add_intermediate_source(size_t opcodeIndex);
};



// Tracks the state of a local variable at each location in the function.
// Each local has a known type associated with it as well as whether or not
// the value is potentially undefined.  When a variable is definitely assigned
// IsMaybeUndefined is false.  
//
// Initially all locals start out as being marked as IsMaybeUndefined and a special
// typeof Undefined.  The special type is really just for convenience to avoid
// having null types.  Merging with the undefined type will produce the other type.
// Assigning to a variable will cause the undefined marker to be removed, and the
// new type to be specified.  
//
// When we merge locals if the undefined flag is specified from either side we will 
// propagate it to the new state.  This could result in:
//
// State 1: Type != Undefined, IsMaybeUndefined = false
//      The value is definitely assigned and we have valid type information
//
// State 2: Type != Undefined, IsMaybeUndefined = true
//      The value is assigned in one code path, but not in another.
//
// State 3: Type == Undefined, IsMaybeUndefined = true
//      The value is definitely unassigned.
//
// State 4: Type == Undefined, IsMaybeUndefined = false
//      This should never happen as it means the Undefined
//      type has leaked out in an odd way
struct AbstractLocalInfo {
    AbstractValueWithSources ValueInfo;
    bool IsMaybeUndefined;
    /*AbstractValue *Value;
    AbstractSources Loads, Stores;*/

    AbstractLocalInfo() {
        ValueInfo = AbstractValueWithSources();
        IsMaybeUndefined = true;
    }

    AbstractLocalInfo(AbstractValueWithSources valueInfo, bool isUndefined = false) {
        _ASSERTE(valueInfo.Value != nullptr);
        _ASSERTE(!(valueInfo.Value == &Undefined && !isUndefined));
        ValueInfo = valueInfo;
        IsMaybeUndefined = isUndefined;
    }

    AbstractLocalInfo merge_with(AbstractLocalInfo other) {
        return AbstractLocalInfo(
            ValueInfo.merge_with(other.ValueInfo),
            IsMaybeUndefined || other.IsMaybeUndefined
        );
    }

    bool operator== (AbstractLocalInfo other) {
        return other.ValueInfo == ValueInfo &&
            other.IsMaybeUndefined == IsMaybeUndefined;
    }
    bool operator!= (AbstractLocalInfo other) {
        return other.ValueInfo != ValueInfo ||
            other.IsMaybeUndefined != IsMaybeUndefined;
    }
};


// Represents the state of the program at each opcode.  Captures the state of both
// the Python stack and the local variables.  We store the state for each opcode in
// AbstractInterpreter.m_startStates which represents the state before the indexed
// opcode has been executed.
//
// The stack is a unique vector for each interpreter state.  There's currently no
// attempts at sharing because most instructions will alter the value stack.
//
// The locals are shared between InterpreterState's using a shared_ptr because the
// values of locals won't change between most opcodes (via CowVector).  When updating 
// a local we first check if the locals are currently shared, and if not simply update 
// them in place.  If they are shared then we will issue a copy.
class InterpreterState {
public:
    vector<AbstractValueWithSources> m_stack;
    CowVector<AbstractLocalInfo> m_locals;

    InterpreterState() {
    }

    InterpreterState(int numLocals) {
        m_locals = CowVector<AbstractLocalInfo>(numLocals);
    }

    AbstractLocalInfo get_local(size_t index) {
        return m_locals[index];
    }

    size_t local_count() {
        return m_locals.size();
    }

    void replace_local(size_t index, AbstractLocalInfo value) {
        m_locals.replace(index, value);
    }

    AbstractValue* pop() {
        auto res = m_stack.back();
        res.escapes();
        m_stack.pop_back();
        return res.Value;
    }

    AbstractValueWithSources pop_no_escape() {
        auto res = m_stack.back();
        m_stack.pop_back();
        return res;
    }

    void push(AbstractValueWithSources& value) {
        m_stack.push_back(value);
    }

    void push(AbstractValue* value) {
        m_stack.push_back(value);
    }

    size_t stack_size() {
        return m_stack.size();
    }

    AbstractValueWithSources& operator[](const size_t index) {
        return m_stack[index];
    }
};

// Tracks block information for analyzing loops, exception blocks, and break opcodes.
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

#endif