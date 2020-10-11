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

#include "stdafx.h"
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
TEST_CASE("Type tracking tests", "[float][binary op][inference]") {
    SECTION("test 1") {
        VerifyOldTest(
                // Type tracking / merging...
                AITestCase(
                        "def f():\n    if abc:\n        x = 1\n    else:\n        x = 'abc'\n        y = 1",
                        {
                                new VariableVerifier(6, 0, AVK_Undefined, true),    // STORE_FAST x
                                new VariableVerifier(8, 0, AVK_Integer),           // JUMP_FORWARD
                                new VariableVerifier(12, 0, AVK_Undefined, true),   // STORE_FAST x
                                new VariableVerifier(14, 0, AVK_String),            // LOAD_CONST
                                new VariableVerifier(18, 0, AVK_Any),               // LOAD_CONST None
                        }));
    }
}
TEST_CASE("Escape tests", "[float][binary op][inference]") {
    SECTION("escape test") {
        VerifyOldTest(
                // Escape tests
                AITestCase(
                        "def f():\n    pi = 0.\n    k = 0.\n    while k < 256.:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi",
                        {
                                new ReturnVerifier(AVK_Any),
                                new BoxVerifier(0, false),  // LOAD_CONST 0
                                new BoxVerifier(4, false),  // LOAD_CONST 0.0
                                new BoxVerifier(10, false),  // LOAD_FAST k
                                new BoxVerifier(12, false),  // LOAD_CONST 256
                                new BoxVerifier(18, false),  // LOAD_FAST pi
                        }));
    }SECTION("float comparison non-opt") {
        VerifyOldTest(
                // Can't optimize due to float int comparison:
                AITestCase(
                        "def f():\n    pi = 0.\n    k = 0.\n    while k < 256:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi",
                        {
                                new ReturnVerifier(AVK_Any),
                                new BoxVerifier(0, true),  // LOAD_CONST 0
                                new BoxVerifier(4, true),  // LOAD_CONST 0.0
                                new BoxVerifier(10, true),  // LOAD_FAST k
                                new BoxVerifier(12, true),  // LOAD_CONST 256
                                new BoxVerifier(18, true),  // LOAD_FAST pi
                        }));
    }SECTION("pi non-opt ") {
        VerifyOldTest(
                // Can't optimize because pi is assigned an int initially
                AITestCase(
                        "def f():\n    pi = 0\n    k = 0.\n    while k < 256.:\n        pi += (4. / (8.*k + 1.) - 2. / (8.*k + 4.) - 1. / (8.*k + 5.) - 1. / (8.*k + 6.)) / 16.**k\n        k += 1.\n    return pi",
                        {
                                new ReturnVerifier(AVK_Any),
                                new BoxVerifier(0, true),  // LOAD_CONST 0
                                new BoxVerifier(4, true),  // LOAD_CONST 0.0
                                new BoxVerifier(10, true),  // LOAD_FAST k
                                new BoxVerifier(12, true),  // LOAD_CONST 256
                                new BoxVerifier(18, true),  // LOAD_FAST pi
                        }));
    }SECTION("min verify ") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1\n    y = 2\n    return min(x, y)",
                        {
                                new BoxVerifier(0, true),  // LOAD_CONST 1
                                new BoxVerifier(4, true),  // LOAD_CONST 2
                                new BoxVerifier(10, true),  // LOAD_FAST x
                                new BoxVerifier(12, true),  // LOAD_FAST y
                                new ReturnVerifier(AVK_Any)
                        }));
    }

    SECTION("const return") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1\n    return x",
                        {
                                new BoxVerifier(0, false),  // LOAD_CONST 1
                                new BoxVerifier(6, false)
                        }));
    }SECTION("const load") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1\n    return abs",
                        {
                                new BoxVerifier(0, false),  // LOAD_CONST 1
                                new BoxVerifier(6, true)
                        }));
    }
}
TEST_CASE("Location tracking tests", "[float][binary op][inference]") {
    SECTION("location track test 1 ") {
        VerifyOldTest(
                // Locations are tracked independently...
                AITestCase(
                        "def f():\n    if abc:\n         x = 1\n         while x < 100:\n              x += 1\n    else:\n         x = 1\n         y = 2\n         return min(x, y)",
                        {
                                new BoxVerifier(4, false),  // LOAD_CONST 1
                                new BoxVerifier(10, false),  // LOAD_FAST x
                                new BoxVerifier(12, false),  // LOAD_CONST 100
                                new BoxVerifier(14, false),  // COMPARE_OP <
                                new BoxVerifier(18, false),  // LOAD_FAST x
                                new BoxVerifier(20, false),  // LOAD_CONST 1
                                new BoxVerifier(32, true),  // LOAD_CONST 1
                                new BoxVerifier(36, true),  // LOAD_CONST 3
                                new BoxVerifier(42, true),  // LOAD_FAST x
                                new BoxVerifier(44, true),  // LOAD_FAST y
                        }));
    }SECTION("location tracking test 2") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    if abc:\n         x = 1\n         while x < 100:\n              x += 1\n    else:\n         x = 1\n         y = 2\n         while x < y:\n            return min(x, y)",
                        {
                                new BoxVerifier(4, true),  // LOAD_CONST 1
                                new BoxVerifier(10, true),  // LOAD_FAST x
                                new BoxVerifier(12, true),  // LOAD_CONST 100
                                new BoxVerifier(14, true),  // COMPARE_OP <
                                new BoxVerifier(18, true),  // LOAD_FAST x
                                new BoxVerifier(20, true),  // LOAD_CONST 1
                                new BoxVerifier(32, true),  // LOAD_CONST 1
                                new BoxVerifier(36, true),  // LOAD_CONST 3
                                new BoxVerifier(42, true),  // LOAD_FAST x
                                new BoxVerifier(44, true),  // LOAD_FAST y
                                new BoxVerifier(46, true),  // COMPARE_OP <
                        }));
    }
}
TEST_CASE("Branching tests", "[float][binary op][inference]"){
    SECTION("basic branch ") {
        VerifyOldTest(
                // Values flowing across a branch...
                AITestCase(
                        "def f():\n    x = 1 if abc else 2",
                        {
                                new BoxVerifier(4, false),  // LOAD_CONST 1
                                new BoxVerifier(8, false),  // LOAD_CONST 2
                        }
                ));
    }
    SECTION("basic branch 2 ") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1 if abc else 2.0",
                        {
                                new BoxVerifier(4, true),  // LOAD_CONST 1
                                new BoxVerifier(8, true),  // LOAD_CONST 2
                        }));
    }

}
TEST_CASE("Basic merging tests", "[float][binary op][inference]"){
    SECTION("Merging w/ unknown value") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = g()\n    x = 1",
                        {
                                new BoxVerifier(4, true),  // STORE_FAST x
                                new BoxVerifier(6, false),  // LOAD_CONST 1
                                new BoxVerifier(8, false),  // STORE_FAST x
                        }));
    }SECTION("Merging w/ deleted value") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1\n    del x\n    x = 2",
                        {
                                new BoxVerifier(0, true),  // LOAD_CONST 1
                                new BoxVerifier(6, false),  // LOAD_CONST 2
                        }));
    }SECTION("Distinct assignments of different types") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1\n    x += 2\n    x = 1.0\n    x += 2.0",
                        {
                                new BoxVerifier(0, false),  // LOAD_CONST 1
                                new BoxVerifier(2, false),  // STORE_FAST x
                                new BoxVerifier(4, false),  // LOAD_FAST 2
                                new BoxVerifier(6, false),  // LOAD_CONST 2
                                new BoxVerifier(8, false),  // INPLACE_ADD
                                new BoxVerifier(10, false),  // STORE_FAST x
                                new BoxVerifier(12, false),  // LOAD_CONST 1
                                new BoxVerifier(14, false),  // STORE_FAST x
                                new BoxVerifier(16, false),  // LOAD_FAST 2
                                new BoxVerifier(18, false),  // LOAD_CONST 2
                                new BoxVerifier(20, false),  // INPLACE_ADD
                                new BoxVerifier(22, false),  // STORE_FAST x
                        }
                ));
    }SECTION("Swap two ints") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1\n    y = 2\n    (x, y) = (y, x)",
                        {
                                new BoxVerifier(0, false),  // LOAD_CONST  1 (1)
                                new BoxVerifier(2, false),  // STORE_FAST  0 (x)
                                new BoxVerifier(4, false),  // LOAD_CONST  2 (2)
                                new BoxVerifier(6, false),  // STORE_FAST  1 (y)
                                new BoxVerifier(8, false), // LOAD_FAST   1 (y)
                                new BoxVerifier(10, false), // LOAD_FAST   0 (x)
                                new BoxVerifier(12, false), // ROT_TWO
                                new BoxVerifier(14, false), // STORE_FAST  0 (x)
                                new BoxVerifier(16, false), // STORE_FAST  1 (y)
                        }));
    }SECTION("Swap two floats") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1.1\n    y = 2.2\n    (x, y) = (y, x)",
                        {
                                new BoxVerifier(0, false),  // LOAD_CONST  1 (1.1)
                                new BoxVerifier(2, false),  // STORE_FAST  0 (x)
                                new BoxVerifier(4, false),  // LOAD_CONST  2 (2.2)
                                new BoxVerifier(6, false),  // STORE_FAST  1 (y)
                                new BoxVerifier(8, false), // LOAD_FAST   1 (y)
                                new BoxVerifier(10, false), // LOAD_FAST   0 (x)
                                new BoxVerifier(12, false), // ROT_TWO
                                new BoxVerifier(14, false), // STORE_FAST  0 (x)
                                new BoxVerifier(16, false), // STORE_FAST  1 (y)
                        }));
    }SECTION("Swap two objects of unknown type") {
        VerifyOldTest(
                AITestCase(
                        "def f(x, y):\n    (x, y) = (y, x)",
                        {
                                new BoxVerifier(0, true),  // LOAD_FAST   1 (y)
                                new BoxVerifier(2, true),  // LOAD_FAST   0 (x)
                                new BoxVerifier(4, true),  // ROT_TWO
                                new BoxVerifier(6, true),  // STORE_FAST  0 (x)
                                new BoxVerifier(8, true), // STORE_FAST  1 (y)
                        }));
    }SECTION("Mix floats and ints") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1\n    y = 2.2\n    (x, y) = (y, x)",
                        {
                                new BoxVerifier(0, true),  // LOAD_CONST  1 (1)
                                new BoxVerifier(2, true),  // STORE_FAST  0 (x)
                                new BoxVerifier(4, true),  // LOAD_CONST  2 (2.2)
                                new BoxVerifier(6, true),  // STORE_FAST  1 (y)
                                new BoxVerifier(8, true), // LOAD_FAST   1 (y)
                                new BoxVerifier(10, true), // LOAD_FAST   0 (x)
                                new BoxVerifier(12, true), // ROT_TWO
                                new BoxVerifier(14, true), // STORE_FAST  0 (x)
                                new BoxVerifier(16, true), // STORE_FAST  1 (y)
                        }));
    }SECTION("Swap three ints") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1\n    y = 2\n    z = 3\n    (x, y, z) = (y, z, x)",
                        {
                                new BoxVerifier(0, false),  // LOAD_CONST  1 (1)
                                new BoxVerifier(2, false),  // STORE_FAST  0 (x)
                                new BoxVerifier(4, false),  // LOAD_CONST  2 (2)
                                new BoxVerifier(6, false),  // STORE_FAST  1 (y)
                                new BoxVerifier(8, false), // LOAD_CONST  3 (3)
                                new BoxVerifier(10, false), // STORE_FAST  2 (z)
                                new BoxVerifier(12, false), // LOAD_FAST   1 (y)
                                new BoxVerifier(14, false), // LOAD_FAST   2 (z)
                                new BoxVerifier(16, false), // LOAD_FAST   0 (x)
                                new BoxVerifier(18, false), // ROT_THREE
                                new BoxVerifier(20, false), // ROT_TWO
                                new BoxVerifier(22, false), // STORE_FAST  0 (x)
                                new BoxVerifier(24, false), // STORE_FAST  1 (y)
                                new BoxVerifier(26, false), // STORE_FAST  2 (z)
                        }));
    }SECTION("Swap three floats") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1.1\n    y = 2.2\n    z = 3.3\n    (x, y, z) = (y, z, x)",
                        {
                                new BoxVerifier(0, false),  // LOAD_CONST  1 (1.1)
                                new BoxVerifier(2, false),  // STORE_FAST  0 (x)
                                new BoxVerifier(4, false),  // LOAD_CONST  2 (2.2)
                                new BoxVerifier(6, false),  // STORE_FAST  1 (y)
                                new BoxVerifier(8, false), // LOAD_CONST  3 (3.3)
                                new BoxVerifier(10, false), // STORE_FAST  2 (z)
                                new BoxVerifier(12, false), // LOAD_FAST   1 (y)
                                new BoxVerifier(14, false), // LOAD_FAST   2 (z)
                                new BoxVerifier(16, false), // LOAD_FAST   0 (x)
                                new BoxVerifier(18, false), // ROT_THREE
                                new BoxVerifier(20, false), // ROT_TWO
                                new BoxVerifier(22, false), // STORE_FAST  0 (x)
                                new BoxVerifier(24, false), // STORE_FAST  1 (y)
                                new BoxVerifier(26, false), // STORE_FAST  2 (z)
                        }));
    }SECTION("Swap three objects of unknown type") {
        VerifyOldTest(
                AITestCase(
                        "def f(x, y, z):\n    (x, y, z) = (y, z, x)",
                        {
                                new BoxVerifier(0, true),  // LOAD_FAST   1 (y)
                                new BoxVerifier(2, true),  // LOAD_FAST   2 (z)
                                new BoxVerifier(4, true),  // LOAD_FAST   0 (x)
                                new BoxVerifier(6, true),  // ROT_THREE
                                new BoxVerifier(8, true), // ROT_TWO
                                new BoxVerifier(10, true), // STORE_FAST  0 (x)
                                new BoxVerifier(12, true), // STORE_FAST  1 (y)
                                new BoxVerifier(14, true), // STORE_FAST  2 (z)
                        }));
    }SECTION("Mix floats and ints") {
        VerifyOldTest(
                AITestCase(
                        "def f():\n    x = 1.1\n    y = 2\n    z = 3.3\n    (x, y, z) = (y, z, x)",
                        {
                                new BoxVerifier(0, true),  // LOAD_CONST  1 (1.1)
                                new BoxVerifier(2, true),  // STORE_FAST  0 (x)
                                new BoxVerifier(4, true),  // LOAD_CONST  2 (2)
                                new BoxVerifier(6, true),  // STORE_FAST  1 (y)
                                new BoxVerifier(8, true), // LOAD_CONST  3 (3.3)
                                new BoxVerifier(10, true), // STORE_FAST  2 (z)
                                new BoxVerifier(12, true), // LOAD_FAST   1 (y)
                                new BoxVerifier(14, true), // LOAD_FAST   2 (z)
                                new BoxVerifier(16, true), // LOAD_FAST   0 (x)
                                new BoxVerifier(18, true), // ROT_THREE
                                new BoxVerifier(20, true), // ROT_TWO
                                new BoxVerifier(22, true), // STORE_FAST  0 (x)
                                new BoxVerifier(24, true), // STORE_FAST  1 (y)
                                new BoxVerifier(26, true), // STORE_FAST  2 (z)
                        }));
    }
};
