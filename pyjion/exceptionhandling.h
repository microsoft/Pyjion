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

#ifndef PYJION_EXCEPTIONHANDLING_H
#define PYJION_EXCEPTIONHANDLING_H

#include <utility>
#include <vector>
#include <unordered_map>
#include "ipycomp.h"
#include "flags.h"
#include "stack.h"

using namespace std;


ehFlags operator | (ehFlags lhs, ehFlags rhs);
ehFlags operator |= (ehFlags& lhs, ehFlags rhs);

struct ExceptionVars {
    // The previous exception value before we took the exception we're currently
    // handling.  These correspond with the values in tstate->exc_* and will be
    // restored back to their current values if the exception is handled.
    // When we're generating the try portion of the block these are new locals, when
    // we're generating the finally/except portion of the block these hold the values
    // for the handler so we can unwind from the correct variables.
    Local PrevExc, PrevExcVal, PrevTraceback;
    // The previous traceback and exception values if we're handling a finally block.
    // We store these in locals and keep only the exception type on the stack so that
    // we don't enter the finally handler with multiple stack depths.
    Local FinallyExc, FinallyTb, FinallyValue;

    explicit ExceptionVars(IPythonCompiler* comp, bool isFinally = false) {
        PrevExc = comp->emit_define_local(false);
        PrevExcVal = comp->emit_define_local(false);
        PrevTraceback = comp->emit_define_local(false);
        FinallyExc = comp->emit_define_local(false);
        FinallyTb = comp->emit_define_local(false);
        FinallyValue = comp->emit_define_local(false);
    }
};

// Exception Handling information
struct ExceptionHandler {
    size_t RaiseAndFreeId;
    ehFlags Flags;
    Label ErrorTarget;    // The place to branch to for handling errors
    ExceptionVars ExVars;
    ValueStack EntryStack;
    ExceptionHandler* BackHandler;

    ExceptionHandler(size_t raiseAndFreeId,
                     ExceptionVars exceptionVars,
                     Label errorTarget,
                     ValueStack entryStack,
                     ehFlags flags = EhfNone,
                     ExceptionHandler *backHandler = nullptr) : ExVars(exceptionVars) {
        RaiseAndFreeId = raiseAndFreeId;
        Flags = flags;
        EntryStack = entryStack;
        ErrorTarget = errorTarget;
        BackHandler = backHandler;
    }

    bool HasErrorTarget() const {
        return ErrorTarget.m_index != -1;
    }

    bool IsTryFinally() {
        return Flags & EhfTryFinally && ExVars.FinallyValue.is_valid();
    }

    ExceptionHandler* GetRootOf(){
        auto cur = this;
        while (cur->BackHandler != nullptr) {
            cur = cur->BackHandler;
        }
        return cur;
    }

    bool IsRootHandler() const{
        return BackHandler == nullptr;
    }

    bool IsTryExceptOrFinally() const {
        return Flags & (EhfTryExcept | EhfTryFinally);
    }
};

class ExceptionHandlerManager {
    vector<ExceptionHandler*> m_exceptionHandlers;
    unordered_map<unsigned long, ExceptionHandler*> m_handlerIndexes;
public:
    ExceptionHandlerManager() = default;

    ~ExceptionHandlerManager(){
        for (auto &handler: m_exceptionHandlers){
            delete handler;
        }
    }

    bool Empty();
    ExceptionHandler* SetRootHandler(Label handlerLabel, ExceptionVars vars);
    ExceptionHandler* GetRootHandler();
    ExceptionHandler *AddSetupFinallyHandler(Label handlerLabel, ValueStack stack,
                                             ExceptionHandler *currentHandler, ExceptionVars vars, unsigned long handlerIndex);

    vector<ExceptionHandler*> GetHandlers();

    bool IsHandlerAtOffset(int offset);
    ExceptionHandler* HandlerAtOffset(int offset);
};

#endif //PYJION_EXCEPTIONHANDLING_H
