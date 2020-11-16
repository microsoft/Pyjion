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

ExceptionHandler* ExceptionHandlerManager::SetRootHandler(Label handlerLabel, ExceptionVars vars) {
    auto rootHandler = new ExceptionHandler(
            0,
            vars,
            handlerLabel,
            ValueStack(),
            EhfNone,
            nullptr);
    m_exceptionHandlers.push_back(
        rootHandler
    );
    return m_exceptionHandlers[0];
}

ExceptionHandler * ExceptionHandlerManager::AddSetupFinallyHandler(Label handlerLabel, ValueStack stack,
                                                                   ExceptionHandler *currentHandler, ExceptionVars vars,
                                                                   unsigned long handlerIndex) {
    auto newHandler = new ExceptionHandler(
            m_exceptionHandlers.size(),
            vars,
            handlerLabel,
            stack,
            EhfTryFinally,
            currentHandler);
    m_handlerIndexes[handlerIndex] = newHandler;

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

bool ExceptionHandlerManager::IsHandlerAtOffset(int offset){
    return m_handlerIndexes.find(offset) != m_handlerIndexes.end();
}

ExceptionHandler* ExceptionHandlerManager::HandlerAtOffset(int offset){
    return m_handlerIndexes.find(offset)->second;
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
