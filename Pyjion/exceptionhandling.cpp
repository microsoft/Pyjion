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

#include "exceptionhandling.h"
#include "stack.h"

ExceptionHandler* ExceptionHandlerManager::SetRootHandler(Label raiseNoHandlerLabel, Label reraiseNoHandlerLabel, ExceptionVars vars) {
    assert(m_exceptionHandlers.empty());
    auto rootHandler = new ExceptionHandler(
            0,
            vars,
            raiseNoHandlerLabel,
            reraiseNoHandlerLabel,
            Label(),
            ValueStack(),
            EhfNone,
            nullptr);
    m_exceptionHandlers.push_back(
        rootHandler
    );
    return m_exceptionHandlers[0];
}

ExceptionHandler* ExceptionHandlerManager::AddSetupFinallyHandler(Label raiseLabel,
                                                                  Label reraiseLabel,
                                                                  Label handlerLabel,
                                                                  ValueStack stack,
                                                                  ExceptionHandler* currentHandler,
                                                                  ExceptionVars vars
                                                                  ) {
    auto newHandler = new ExceptionHandler(
            m_exceptionHandlers.size(),
            vars,
            raiseLabel,
            reraiseLabel,
            handlerLabel,
            stack,
            EhfTryFinally,
            currentHandler);
    m_exceptionHandlers.push_back(
        newHandler
    );
    return newHandler;
}

ExceptionHandler* ExceptionHandlerManager::AddInTryHandler(Label raiseLabel,
                                                              Label reraiseLabel,
                                                              Label handlerLabel,
                                                              ValueStack stack,
                                                              ExceptionHandler* currentHandler,
                                                              ExceptionVars vars,
                                                              bool inTryFinally
) {
    auto newHandler = new ExceptionHandler(
            m_exceptionHandlers.size(),
            vars,
            raiseLabel,
            reraiseLabel,
            handlerLabel,
            stack,
            inTryFinally ? EhfTryFinally | EhfInExceptHandler : EhfInExceptHandler,
            currentHandler);
    m_exceptionHandlers.push_back(
            newHandler
    );
    return newHandler;
}

ExceptionHandler* ExceptionHandlerManager::GetRootHandler() {
    return m_exceptionHandlers[0];
}

bool ExceptionHandlerManager::Empty() {
    return m_exceptionHandlers.empty();
}

vector<ExceptionHandler*> ExceptionHandlerManager::GetHandlers() {
    return m_exceptionHandlers;
}

ehFlags operator | (ehFlags lhs, ehFlags rhs) {
    return (ehFlags)(static_cast<int>(lhs) | static_cast<int>(rhs));
}

ehFlags operator |= (ehFlags& lhs, ehFlags rhs) {
    lhs = (ehFlags)(static_cast<int>(lhs) | static_cast<int>(rhs));
    return lhs;
}
