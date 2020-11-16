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

#ifndef PYJION_STACK_H
#define PYJION_STACK_H

#include <vector>
#include <exception>

#include "block.h"


enum StackEntryKind {
    STACK_KIND_VALUE = 0, // A non-boxed value, currently just floating point
    STACK_KIND_OBJECT = 1 // A Python object, or a tagged int which might be an object
};

class StackUnderflowException: public std::exception {
public:
    StackUnderflowException() : std::exception() {};
    const char * what () const noexcept override
    {
        return "Stack underflow";
    }
};

 class ValueStack : std::vector<StackEntryKind> {

 public:
     ValueStack() = default;

    ValueStack(ValueStack const &old)  {
        for (int i=0; i< old.size(); i++)
            push_back(old[i]);
    }

    void inc(size_t by, StackEntryKind kind);
    void dec(size_t by);

    size_t size() const {
        return std::vector<StackEntryKind>::size();
    }

    void dup_top(){
        if (empty())
            throw StackUnderflowException();
        push_back(back()); // does inc_stack
    }

    reverse_iterator rbegin(){
        return std::vector<StackEntryKind>::rbegin();
    }

    reverse_iterator rend(){
     return std::vector<StackEntryKind>::rend();
    }
};

 class BlockStack : std::vector<BlockInfo> {

 public:
     BlockStack() = default;

     BlockStack(BlockStack const &old)  {
         for (int i=0; i< old.size(); i++)
             push_back(old[i]);
     }

     bool empty(){
         return vector<BlockInfo>::empty();
     }

     void pop_back() {
         if (empty())
             throw StackUnderflowException();
         return vector<BlockInfo>::pop_back();
     }

     void push_back(BlockInfo block){
         return vector<BlockInfo>::push_back(block);
     }

     bool beyond(int curByte){
         return (size() > 1 &&
             curByte >= back().EndOffset &&
             back().EndOffset != -1);
     }

     size_t size() const{
         return vector<BlockInfo>::size();
     }

     BlockInfo back(){
         if (empty())
             throw StackUnderflowException();
         return vector<BlockInfo>::back();
     }

     reverse_iterator rbegin(){
         return std::vector<BlockInfo>::rbegin();
     }

     reverse_iterator rend(){
         return std::vector<BlockInfo>::rend();
     }

     BlockInfo get(size_t i) {
        return std::vector<BlockInfo>::at(i);
     }
 };


#endif //PYJION_STACK_H
