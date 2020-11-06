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

#ifndef PYJION_ABSINT_H
#define PYJION_ABSINT_H

#include <Python.h>
#include <vector>
#include <unordered_map>

#include "absvalue.h"
#include "cowvector.h"
#include "ipycomp.h"
#include "block.h"
#include "stack.h"
#include "exceptionhandling.h"

using namespace std;

struct AbstractLocalInfo;

// Tracks block information for analyzing loops, exception blocks, and break opcodes.
struct AbsIntBlockInfo {
    size_t BlockStart, BlockEnd;
    bool IsLoop;

    AbsIntBlockInfo(size_t blockStart, size_t blockEnd, bool isLoop) {
        BlockStart = blockStart;
        BlockEnd = blockEnd;
        IsLoop = isLoop;
    }
};

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

struct AbstractValueKindHash {
    std::size_t operator()(AbstractValueKind e) const {
        return static_cast<std::size_t>(e);
    }
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
    AbstractLocalInfo() = default;

    AbstractValueWithSources ValueInfo;
    bool IsMaybeUndefined;

    AbstractLocalInfo(AbstractValueWithSources valueInfo, bool isUndefined = false) : ValueInfo(valueInfo) {
        IsMaybeUndefined = true;
        assert(valueInfo.Value != nullptr);
        assert(!(valueInfo.Value == &Undefined && !isUndefined));
        IsMaybeUndefined = isUndefined;
    }

    AbstractLocalInfo mergeWith(AbstractLocalInfo other) const {
        return {
            ValueInfo.mergeWith(other.ValueInfo),
            IsMaybeUndefined || other.IsMaybeUndefined
            };
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
    vector<AbstractValueWithSources> mStack;
    CowVector<AbstractLocalInfo> mLocals;

    InterpreterState() = default;

    explicit InterpreterState(int numLocals) {
        mLocals = CowVector<AbstractLocalInfo>(numLocals);
    }

    AbstractLocalInfo getLocal(size_t index) {
        return mLocals[index];
    }

    size_t localCount() {
        return mLocals.size();
    }

    void replaceLocal(size_t index, AbstractLocalInfo value) {
        mLocals.replace(index, value);
    }

    AbstractValue* pop() {
        assert(!mStack.empty());
        auto res = mStack.back();
        res.escapes();
        mStack.pop_back();
        return res.Value;
    }

    AbstractValueWithSources popNoEscape() {
        assert(!mStack.empty());
        auto res = mStack.back();
        mStack.pop_back();
        return res;
    }

    void push(AbstractValueWithSources& value) {
        mStack.push_back(value);
    }

    void push(AbstractValue* value) {
        mStack.emplace_back(value);
    }

    size_t stackSize() const {
        return mStack.size();
    }

    AbstractValueWithSources& operator[](const size_t index) {
        return mStack[index];
    }
};

enum ComprehensionType {
    COMP_NONE,
    COMP_LIST,
    COMP_DICT,
    COMP_SET
};


#ifdef _WIN32
class __declspec(dllexport) AbstractInterpreter {
#pragma warning (disable:4251)
#else
class AbstractInterpreter {
#endif
    // ** Results produced:
    // Tracks the interpreter state before each opcode
    unordered_map<size_t, InterpreterState> mStartStates;
    AbstractValue* mReturnValue;
    // ** Inputs:
    PyCodeObject* mCode;
    _Py_CODEUNIT *mByteCode;
    size_t mSize;
    Local mErrorCheckLocal;
    Local mExcVarsOnStack; // Counter of the number of exception variables on the stack.

    // ** Data consumed during analysis:
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
    vector<Local> m_raiseAndFreeLocals;
    IPythonCompiler* m_comp;
    // m_blockStack is like Python's f_blockstack which lives on the frame object, except we only maintain
    // it at compile time.  Blocks are pushed onto the stack when we enter a loop, the start of a try block,
    // or into a finally or exception handler.  Blocks are popped as we leave those protected regions.
    // When we pop a block associated with a try body we transform it into the correct block for the handler
    BlockStack m_blockStack;

    ExceptionHandlerManager m_exceptionHandler;
    // Labels that map from a Python byte code offset to an ilgen label.  This allows us to branch to any
    // byte code offset.
    unordered_map<int, Label> m_offsetLabels;
    // Tracks the current depth of the stack,  as well as if we have an object reference that needs to be freed.
    // True (STACK_KIND_OBJECT) if we have an object, false (STACK_KIND_VALUE) if we don't
    ValueStack m_stack;
    // Tracks the state of the stack when we perform a branch.  We copy the existing state to the map and
    // reload it when we begin processing at the stack.
    unordered_map<int, ValueStack> m_offsetStack;
    // Set of labels used for when we need to raise an error but have values on the stack
    // that need to be freed.  We have one set of labels which fall through to each other
    // before doing the raise:
    //      free2: <decref>/<pop>
    //      free1: <decref>/<pop>
    //      raise logic.
    //  This was so we don't need to have decref/frees spread all over the code
    vector<vector<Label>> m_raiseAndFree, m_reraiseAndFree;
    unordered_set<size_t> m_jumpsTo;
    Label m_retLabel;
    Local m_retValue;
    // Stores information for a stack allocated local used for sequence unpacking.  We need to allocate
    // one of these when we enter the method, and we use it if we don't have a sequence we can efficiently
    // unpack.
    unordered_map<int, Local> m_sequenceLocals;
    unordered_map<int, bool> m_assignmentState;
    unordered_map<int, unordered_map<AbstractValueKind, Local, AbstractValueKindHash>> m_optLocals;

#pragma warning (default:4251)

public:
    AbstractInterpreter(PyCodeObject *code, IPythonCompiler* compiler);
    ~AbstractInterpreter();

    JittedCode* compile();
    bool interpret();

    void setLocalType(int index, AbstractValueKind kind);
    // Returns information about the specified local variable at a specific
    // byte code index.
    AbstractLocalInfo getLocalInfo(size_t byteCodeIndex, size_t localIndex);

    // Returns information about the stack at the specific byte code index.
    vector<AbstractValueWithSources>& getStackInfo(size_t byteCodeIndex);

    // Returns true if the result of the opcode should be boxed, false if it
    // can be maintained on the stack.
    bool shouldBox(size_t opcodeIndex);

    bool canSkipLastiUpdate(size_t opcodeIndex);

    AbstractValue* getReturnInfo();

private:
    AbstractValue* toAbstract(PyObject* obj);
    AbstractValue* toAbstract(AbstractValueKind kind);
    static bool mergeStates(InterpreterState& newState, InterpreterState& mergeTo);
    bool updateStartState(InterpreterState& newState, size_t index);
    void initStartingState();
    const char* opcodeName(int opcode);
    bool preprocess();
    void dumpSources(AbstractSource* sources);
    AbstractSource* newSource(AbstractSource* source) {
        m_sources.push_back(source);
        return source;
    }

    AbstractSource* addLocalSource(size_t opcodeIndex, size_t localIndex);
    AbstractSource* addConstSource(size_t opcodeIndex, size_t constIndex);
    AbstractSource* addIntermediateSource(size_t opcodeIndex);

    void makeFunction(int oparg);
    bool canSkipLastiUpdate(int opcodeIndex);
    void buildTuple(size_t argCnt);
    void extendTuple(size_t argCnt);
    void buildList(size_t argCnt);
    void extendListRecursively(Local list, size_t argCnt);
    void extendList(size_t argCnt);
    void buildSet(size_t argCnt);
    void unpackEx(size_t size, int opcode);

    void buildMap(size_t argCnt);

    Label getOffsetLabel(int jumpTo);
    void forIter(int loopIndex);

    // Checks to see if we have a null value as the last value on our stack
    // indicating an error, and if so, branches to our current error handler.
    void errorCheck(const char* reason = nullptr);
    void intErrorCheck(const char* reason = nullptr);

    vector<Label>& getRaiseAndFreeLabels(size_t blockId);
    vector<Label>& getReraiseAndFreeLabels(size_t blockId);
    void ensureRaiseAndFreeLocals(size_t localCount);
    void emitRaiseAndFree(ExceptionHandler* handler);

    void ensureLabels(vector<Label>& labels, size_t count);

    void branchRaise(const char* reason = nullptr);
    size_t clearValueStack();
    void raiseOnNegativeOne();

    void unwindEh(ExceptionHandler* fromHandler, ExceptionHandler* toHandler = nullptr);

    ExceptionHandler * currentHandler();

    void markOffsetLabel(int index);

    void jumpAbsolute(size_t index, size_t from);

    void decStack(size_t size = 1);

    void incStack(size_t size = 1, StackEntryKind kind = STACK_KIND_OBJECT);

    JittedCode* compileWorker();

    void periodicWork();
    void storeFast(int local, int opcodeIndex);

    void loadConst(int constIndex, int opcodeIndex);

    void returnValue(int opcodeIndex);

    void loadFast(int local, int opcodeIndex);
    void loadFastWorker(int local, bool checkUnbound);
    void unpackSequence(size_t size, int opcode);

    void popExcept();

    void unaryPositive(int opcodeIndex);
    void unaryNegative(int opcodeIndex);
    void unaryNot(int& opcodeIndex);

    void jumpIfOrPop(bool isTrue, int opcodeIndex, int offset);
    void popJumpIf(bool isTrue, int opcodeIndex, int offset);
    void jumpIfNotExact(int opcodeIndex, int jumpTo);
    void testBoolAndBranch(Local value, bool isTrue, Label target);

    void unwindHandlers();

    void emitRaise(ExceptionHandler *handler);
    void popExcVars();
    void decExcVars(int count);
    void incExcVars(int count);
};


#endif
