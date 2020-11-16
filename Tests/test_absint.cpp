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

#include <catch2/catch.hpp>
#include "testing_util.h"


TEST_CASE("Basic typing tests", "[float][binary op][inference]") {
    SECTION("const int") {
        VerifyOldTest(
                //
                AITestCase(
                        "def f(): return 42",
                        {
                                new ReturnVerifier(AVK_Integer),    // VC 2013 ICE's with only a single value here...
                                new ReturnVerifier(AVK_Integer)
                        }));
    }SECTION("conditional const int") {
        VerifyOldTest(AITestCase(
                "def f(x):\n    if x:    return 42\n    return 'abc'",
                {
                        new ReturnVerifier(AVK_Any),    // VC 2013 ICE's with only a single value here...
                        new ReturnVerifier(AVK_Any)
                }));
    }
}
TEST_CASE("Follow block tests", "[float][binary op][inference]") {
    SECTION("conditional return ") {
        VerifyOldTest(
                // We won't follow blocks following a return statement...
                AITestCase(
                        "def f(x):\n    return 42\n    if 'abc': return 'abc'",
                        {
                                new ReturnVerifier(AVK_Integer),    // VC 2013 ICE's with only a single value here...
                                new ReturnVerifier(AVK_Integer)
                        }));
    }};
TEST_CASE("Basic delete tests", "[float][binary op][inference]") {
    SECTION("delete int") {
        VerifyOldTest(
                // Basic delete
                AITestCase(
                        "def f():\n    x = 1\n    del x",
                        {
                                new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
                                new VariableVerifier(2, 0, AVK_Undefined, true),    // STORE_FAST
                                new VariableVerifier(4, 0, AVK_Integer, false),     // DELETE_FAST
                                new VariableVerifier(8, 0, AVK_Undefined, true),    // LOAD_CONST None
                        }));
    }SECTION("delete/undefined merging ") {
        VerifyOldTest(
                // Delete / Undefined merging...
                AITestCase(
                        "def f():\n    x = 1\n    if abc:\n        del x\n        y = 1",
                        {
                                new VariableVerifier(0, 0, AVK_Undefined, true),    // LOAD_CONST 1
                                new VariableVerifier(2, 0, AVK_Undefined, true),    // STORE_FAST
                                new VariableVerifier(4, 0, AVK_Integer, false),     // LOAD_GLOBAL abc
                                new VariableVerifier(8, 0, AVK_Integer, false),     // DELETE_FAST
                                new VariableVerifier(10, 0, AVK_Undefined, true),    // LOAD_CONST 1
                                new VariableVerifier(14, 0, AVK_Any, true),          // LOAD_CONST None
                        }));
    }
}
