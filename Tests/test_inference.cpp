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

/**
 Test type inferencing.

 Many tests use a style of test ported over from an old test runner. Any new
 set of tests should write their own testing code that would be easier to work
 with.
*/
#include "stdafx.h"
#include "catch.hpp"
#include "testing_util.h"
#include <Python.h>
#include <absint.h>
#include <util.h>
#include <memory>
#include <vector>

class InferenceTest {
private:
    std::unique_ptr<AbstractInterpreter> m_absint;

public:
    InferenceTest(const char* code) {
        auto pyCode = CompileCode(code);
        m_absint = std::make_unique<AbstractInterpreter>(pyCode, nullptr);
        auto success = m_absint->interpret();
        if (!success) {
            Py_DECREF(pyCode);
            FAIL("Failed to interpret code");
        }
    }

    AbstractValueKind kind(size_t byteCodeIndex, size_t localIndex) {
        auto local = m_absint->get_local_info(byteCodeIndex, localIndex);
        return local.ValueInfo.Value->kind();
    }
};


/* Old test code; do not use in new tests! ========================= */
class AIVerifier {
public:
    virtual void verify(AbstractInterpreter& interpreter) = 0;
};

/* Verify the inferred type stored in the locals array before a specified bytecode executes. */
class VariableVerifier : public AIVerifier {
private:
    // The bytecode whose locals state we are checking *before* execution.
    size_t m_byteCodeIndex;
    // The locals index whose type we are checking.
    size_t m_localIndex;
    // The inferred type.
    AbstractValueKind m_kind;
    // Has the value been defined yet?
    bool m_undefined;
public:
    VariableVerifier(size_t byteCodeIndex, size_t localIndex, AbstractValueKind kind, bool undefined = false) {
        m_byteCodeIndex = byteCodeIndex;
        m_localIndex = localIndex;
        m_undefined = undefined;
        m_kind = kind;
    }

    virtual void verify(AbstractInterpreter& interpreter) {
        auto local = interpreter.get_local_info(m_byteCodeIndex, m_localIndex);
        REQUIRE(local.IsMaybeUndefined == m_undefined);
        REQUIRE(local.ValueInfo.Value->kind() == m_kind);
    };
};

class AITestCase {
private:
public:
    const char* m_code;
    vector<AIVerifier*> m_verifiers;

    AITestCase(const char *code, AIVerifier* verifier) {
        m_code = code;
        m_verifiers.push_back(verifier);
    }

    AITestCase(const char *code, vector<AIVerifier*> verifiers) {
        m_code = code;
        m_verifiers = verifiers;
    }

    AITestCase(const char *code, std::initializer_list<AIVerifier*> list) {
        m_code = code;
        m_verifiers = list;
    }

    ~AITestCase() {
        for (auto verifier : m_verifiers) {
            delete verifier;
        }
    }

    void verify(AbstractInterpreter& interpreter) {
        for (auto cur : m_verifiers) {
            cur->verify(interpreter);
        }
    }
};

void VerifyOldTest(AITestCase testCase) {
    auto codeObj = CompileCode(testCase.m_code);

    AbstractInterpreter interpreter(codeObj, nullptr);
    if (!interpreter.interpret()) {
        FAIL("Failed to interprete code");
    }

    testCase.verify(interpreter);

    Py_DECREF(codeObj);
}
/* ==================================================== */

TEST_CASE("float binary op type inference", "[float][binary op][inference]") {
    SECTION("float + float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float // float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float % float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float * float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float ** float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float - float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float / float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float += float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float //= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float %= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float *= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float **= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float -= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float /= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3.14\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float + int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float // int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float % int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float * int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float ** int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float - int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float / int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float += int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float //= int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float %= int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float *= int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float **= int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float -= int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float /= int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 42\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float + bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float // bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float % bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float * bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float ** bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float - bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float / bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("float += bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float //= bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float %= bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float *= bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float **= bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float -= bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float /= bool  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = True\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("float + complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("float * complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("float ** complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("float - complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("float / complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("float += complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("float *= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("float **=complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("float -= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("float /= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = 3j\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }
}

TEST_CASE("float unary op type inference", "[float][unary op][inference]") {

    SECTION("not float  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ));
    }

    SECTION("- float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = -x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Float)              // y assigned
            }
        ));
    }

    SECTION("+ float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3.14\n    y = +x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Float),              // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Float)              // y assigned
            }
        ));
    }
}

TEST_CASE("int binary op type inference", "[int][binary op][inference]") {
    SECTION("int / bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("int + bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int & bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x & y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int // bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int << bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x << y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int % bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int * bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int | bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x | y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int ** bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int >> bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x >> y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int - bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int ^ bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    z = x ^ y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int /= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("int += bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int &= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x &= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int //= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int <<= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x <<= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int %= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int *= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int |= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x |= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int *= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int >>= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x >>= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int -= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int ^= bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = True\n    x ^= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int * bytes  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = b'a'\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ));
    }

    SECTION("int *= bytes  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = b'a'\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)              // x assigned in-place
            }
        ));
    }

    SECTION("int + complext  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("int * complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("int ** complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("int - complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("int / complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("int += complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("int *= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("int **= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("int -= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("int /= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3j\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("int + float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("int // float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
            new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("int % float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("int * float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("int ** float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("int - float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("int / float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("int += float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("int //= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("int %= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("int *= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("int **= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("int -= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("int /= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3.14\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("int / int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("int /= int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("int + int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int & int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x & y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int // int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int << int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x << y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int % int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int * int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int | int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x | y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int ** int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int ** int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int >> int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x >> y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int - int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int ^ int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    z = x ^ y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("int += int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int &= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x &= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int //= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int <<= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x <<= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int %= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int *= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int |= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x |= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int **= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int >>= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x >>= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int -= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int ^= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = 3\n    x ^= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("int * list  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = []\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_List),              // z assigned
            }
        ));
    }

    SECTION("int *= list  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = []\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ));
    }

    SECTION("int * str  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = ''\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("int *= str  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = ''\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("int * tuple  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = ()\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Tuple),             // z assigned
            }
        ));
    }

    SECTION("int *= tuple  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = ()\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(22, 0, AVK_Tuple)              // x assigned in-place
            }
        ));
    }
}

TEST_CASE("int unary op type inference", "[int][unary op][inference]") {
    SECTION("not int  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ));
    }

    SECTION("~ int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = ~x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Integer)            // y assigned
            }
        ));
    }

    SECTION("- int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 42\n    y = -x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Integer),            // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Integer)            // y assigned
            }
        ));
    }
}

TEST_CASE("bool binary op type inference", "[bool][binary op][inference]") {
    SECTION("bool & bool  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = False\n    z = x & y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bool),              // z assigned
            }
        ));
    }

    SECTION("bool | bool  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = False\n    z = x | y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bool),              // z assigned
            }
        ));
    }

    SECTION("bool ^ bool  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = False\n    z = x ^ y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bool),              // z assigned
            }
        ));
    }

    SECTION("bool &= bool  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = False\n    x &= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Bool)               // x assigned in-place
            }
        ));
    }

    SECTION("bool |= bool  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = False\n    x |= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Bool)               // x assigned in-place
            }
        ));
    }

    SECTION("bool ^= bool  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = False\n    x ^= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Bool)               // x assigned in-place
            }
        ));
    }

    SECTION("bool * bytes  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = b'a'\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ));
    }

    SECTION("bool *= bytes  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = b'a'\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)              // x assigned in-place
            }
        ));
    }

    SECTION("bool + complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("bool * complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("bool ** complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("bool - complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("bool / complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("bool += complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("bool *= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("bool **= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("bool -= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("bool /= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3j\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("bool + float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("bool // float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("bool % float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("bool * float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("bool ** float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("bool - float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("bool / float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("bool += float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("bool //= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("bool %= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("bool *= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("bool **= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("bool -= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("bool /= float  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 3.14\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("bool % int  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bool),              // z assigned
            }
        ));
    }

    SECTION("bool %= int  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Bool)               // x assigned in-place
            }
        ));
    }

    SECTION("bool / int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Float),             // z assigned
            }
        ));
    }

    SECTION("bool /= int  # type: float") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Float)              // x assigned in-place
            }
        ));
    }

    SECTION("bool + int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool & int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x & y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool // int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x // y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool << int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x << y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool * int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool | int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x | y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool ** int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool >> int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x >> y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool - int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool ^ int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    z = x ^ y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bool += int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool &= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x &= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool //= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x //= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool <<= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x <<= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool *= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool |= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x |= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool **= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool >>= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x >>= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool -= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool ^= int  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = 42\n    x ^= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Integer)            // x assigned in-place
            }
        ));
    }

    SECTION("bool * list  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = []\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_List),              // z assigned
            }
        ));
    }

    SECTION("bool *= list  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = []\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ));
    }

    SECTION("bool * str  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = ''\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("bool *= str  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = ''\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("bool * tuple  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = ()\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Tuple),             // z assigned
            }
        ));
    }

    SECTION("bool *= tuple  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = ()\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(22, 0, AVK_Tuple)              // x assigned in-place
            }
        ));
    }
}

TEST_CASE("bool unary op type inference", "[bool][unary op][inference]") {
    SECTION("not bool  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ));
    }

    SECTION("~ bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = ~x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Integer)            // y assigned
            }
        ));
    }

    SECTION("- bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = -x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Integer)            // y assigned
            }
        ));
    }

    SECTION("+ bool  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = True\n    y = +x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bool),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Integer)            // y assigned
            }
        ));
    }
}

TEST_CASE("bytes binary op type inference", "[bytes][binary op][inference]") {
    SECTION("bytes * bool  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = True\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ));
    }

    SECTION("bytes *= bool  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = True\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)              // x assigned in-place
            }
        ));
    }

    SECTION("bytes + bytes  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = b'a'\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ));
    }

    SECTION("bytes % bytes  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = b'a'\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ));
    }

    SECTION("bytes += bytes  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = b'a'\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)              // x assigned in-place
            }
        ));
    }

    SECTION("bytes %= bytes  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = b'a'\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)              // x assigned in-place
            }
        ));
    }

    SECTION("bytes % dict  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = {}\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Dict),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ));
    }

    SECTION("bytes %= dict  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = {}\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Dict),              // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)              // x assigned in-place
            }
        ));
    }

    SECTION("bytes * int  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = 42\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ));
    }

    SECTION("bytes *= int  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = 42\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)              // x assigned in-place
            }
        ));
    }

    SECTION("bytes[int]  # type: int") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = 42\n    z = x[y]",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Integer),           // z assigned
            }
        ));
    }

    SECTION("bytes % list  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = []\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ));
    }

    SECTION("bytes %= list  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = []\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)              // x assigned in-place
            }
        ));
    }

    SECTION("bytes[:]  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = x[:]",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
            new VariableVerifier(19, 1, AVK_Undefined, true),   // y not assigned yet
            new VariableVerifier(22, 1, AVK_Bytes),             // y assigned
            }
        ));
    }

    SECTION("bytes % tuple  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = ()\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Bytes),             // z assigned
            }
        ));
    }

    SECTION("bytes %= tuple  # type: bytes") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = ()\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(22, 0, AVK_Bytes)              // x assigned in-place
            }
        ));
    }
}

TEST_CASE("bytes unary op type inference", "[bytes][unary op][inference]") {
    SECTION("not bytes  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = b'a'\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Bytes),              // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ));
    }
}

TEST_CASE("complex binary op type inference", "[complex][binary op][inference]") {
    SECTION("complex + bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex * bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex ** bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex - bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex / bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex += bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex *= bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex **= bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex -= bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex /= bool  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = True\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex + complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = x + 3j",
            {
                new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex - complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = x - 3j",
            {
                new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex * complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = x * 3j",
            {
                new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex ** complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = x ** 2j",
            {
                new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex / complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = x / 3j",
            {
                new VariableVerifier(3, 1, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 1, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex += complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    x += 3j",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex -= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    x -= 3j",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex *= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    x *= 3j",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex **= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    x **= 3j",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex /= complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    x /= 3j",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // STORE_FAST 0
                new VariableVerifier(6, 0, AVK_Complex),            // LOAD_FAST 0
                new VariableVerifier(16, 0, AVK_Complex),           // STORE_FAST 1
            }));
    }

    SECTION("complex + float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex * float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex ** float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex - float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex / float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex += float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex *= float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex **= float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex -= float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex /= float  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 3.14\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex + int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex * int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex ** int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x ** y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex - int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x - y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex / int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    z = x / y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 2, AVK_Complex),           // z assigned
            }
        ));
    }

    SECTION("complex += int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex *= int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex **= int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x **= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex -= int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x -= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }

    SECTION("complex /= int  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = 42\n    x /= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),            // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Complex)            // x assigned in-place
            }
        ));
    }
}

TEST_CASE("complex unary op type inference", "[complex][unary op][inference]") {
    SECTION("+ complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = +x",
            {
                new VariableVerifier(10, 1, AVK_Undefined, true),   // 10 STORE_FAST               1 (y)
                new VariableVerifier(13, 1, AVK_Complex)            // 13 LOAD_CONST               0 (None)
            }));
    }
    
    SECTION("- complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = -x",
            {
                new VariableVerifier(10, 1, AVK_Undefined, true),   // 10 STORE_FAST               1 (y)
                new VariableVerifier(13, 1, AVK_Complex)            // 13 LOAD_CONST               0 (None)
            }));
    }

    SECTION("not complex  # type: complex") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = 3j\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),  // x not assigned yet
                new VariableVerifier(6, 0, AVK_Complex),          // x assigned
            new VariableVerifier(10, 1, AVK_Undefined, true),     // y not assigned yet
            new VariableVerifier(13, 1, AVK_Bool)                 // y assigned
            }
        ));
    }
}

TEST_CASE("dict unary op type inference", "[dict][unary op][inference]") {
    SECTION("not dict  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {}\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Dict),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ));
    }
}

TEST_CASE("str binary op type inferene", "[str][binary op][inference]") {
    SECTION("str % bool  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = True\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= bool  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = True\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str * bool  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = True\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str *= bool  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = True\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str % bytes  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = b''\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= bytes  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = b''\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str % complex  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = 3j\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= complex  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = 3j\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Complex),           // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str % dict  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = {}\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Dict),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= dict  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = {}\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Dict),              // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str % float  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = 3.14\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= float  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = 3.14\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Float),             // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str % int  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = 42\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= int  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = 42\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str * int  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = 42\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str *= int  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = 42\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str[int]  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = 42\n    z = x[y]",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String)             // z assigned
            }
        ));
    }

    SECTION("str % list  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = []\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
            new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= list  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = []\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str % None  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = None\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_None),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= None  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = None\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_None),              // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str % set  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = {42}\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(12, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(15, 1, AVK_Set),               // y assigned
                new VariableVerifier(22, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(25, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= set  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = {42}\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(12, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(15, 1, AVK_Set),               // y assigned
                new VariableVerifier(25, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str[slice]  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = x[1:2:3]",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(22, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(25, 1, AVK_String),            // y assigned
            }
        ));
    }

    SECTION("str % str  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = ''\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= str  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = ''\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str + str  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = ''\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str += str  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = ''\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }

    SECTION("str % tuple  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = ()\n    z = x % y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_String),            // z assigned
            }
        ));
    }

    SECTION("str %= tuple  # type: str") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = ()\n    x %= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(22, 0, AVK_String)             // x assigned in-place
            }
        ));
    }
}

TEST_CASE("str unary op type inference", "[str][unary op][inference]") {
    SECTION("not str  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ''\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_String),             // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ));
    }
}

TEST_CASE("list binary op type inference", "[list][binary op][inference]") {
    SECTION("list * bool  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = True\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_List),              // z assigned
            }
        ));
    }

    SECTION("list *= bool  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = True\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ));
    }

    SECTION("list += bytes  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = b'a'\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bytes),             // y assigned
                new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ));
    }

    SECTION("list += dict  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = {}\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
            new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
            new VariableVerifier(12, 1, AVK_Dict),              // y assigned
            new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ));
    }

    SECTION("list * int  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = 42\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_List),              // z assigned
            }
        ));
    }

    SECTION("list *= int  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = 42\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ));
    }

    SECTION("list + list  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = []\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_List),              // z assigned
            }
        ));
    }

    SECTION("list += list  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = []\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_List),              // y assigned
                new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ));
    }

    SECTION("list += set  # type: set") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = {42}\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(12, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(15, 1, AVK_Set),               // y assigned
                new VariableVerifier(25, 0, AVK_List)               // x assigned in-place
            }
        ));
    }

    SECTION("list[slice]  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = x[1:2:3]",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(22, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(25, 1, AVK_List),              // y assigned
            }
        ));
    }

    SECTION("list += str  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = ''\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_String),            // y assigned
                new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ));
    }

    SECTION("list += tuple  # type: list") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = ()\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(22, 0, AVK_List)               // x assigned in-place
            }
        ));
    }
}

TEST_CASE("list unary op type inference", "[list][unary op][inference]") {
    SECTION("not list  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = []\n    y = not x",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_List),               // x assigned
                new VariableVerifier(10, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(13, 1, AVK_Bool)               // y assigned
            }
        ));
    }
}

TEST_CASE("set binary op type inference", "[set][binary op][inference]") {
    SECTION("set & set  # type: set") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {42}\n    y = {-13}\n    z = x & y",
            {
                new VariableVerifier(6, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(9, 0, AVK_Set),                // x assigned
                new VariableVerifier(15, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(18, 1, AVK_Set),               // y assigned
                new VariableVerifier(25, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(28, 2, AVK_Set),               // z assigned
            }
        ));
    }
    SECTION("set | set  # type: set") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {42}\n    y = {-13}\n    z = x | y",
            {
                new VariableVerifier(6, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(9, 0, AVK_Set),                // x assigned
                new VariableVerifier(15, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(18, 1, AVK_Set),               // y assigned
                new VariableVerifier(25, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(28, 2, AVK_Set),               // z assigned
            }
        ));
    }

    SECTION("set - set  # type: set") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {42}\n    y = {-13}\n    z = x - y",
            {
                new VariableVerifier(6, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(9, 0, AVK_Set),                // x assigned
                new VariableVerifier(15, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(18, 1, AVK_Set),               // y assigned
                new VariableVerifier(25, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(28, 2, AVK_Set),               // z assigned
            }
        ));
    }

    SECTION("set ^ set  # type: set") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {42}\n    y = {-13}\n    z = x ^ y",
            {
                new VariableVerifier(6, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(9, 0, AVK_Set),                // x assigned
                new VariableVerifier(15, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(18, 1, AVK_Set),               // y assigned
                new VariableVerifier(25, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(28, 2, AVK_Set),               // z assigned
            }
        ));
    }


    SECTION("set &= set  # type: set") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {42}\n    y = {-13}\n    x &= y",
            {
                new VariableVerifier(6, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(9, 0, AVK_Set),                // x assigned
                new VariableVerifier(15, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(18, 1, AVK_Set),               // y assigned
                new VariableVerifier(28, 0, AVK_Set)                // x assigned in-place
            }
        ));
    }

    SECTION("set |= set  # type: set") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {42}\n    y = {-13}\n    x |= y",
            {
                new VariableVerifier(6, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(9, 0, AVK_Set),                // x assigned
                new VariableVerifier(15, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(18, 1, AVK_Set),               // y assigned
                new VariableVerifier(28, 0, AVK_Set)                // x assigned in-place
            }
        ));
    }

    SECTION("set -= set  # type: set") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {42}\n    y = {-13}\n    x -= y",
            {
                new VariableVerifier(6, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(9, 0, AVK_Set),                // x assigned
                new VariableVerifier(15, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(18, 1, AVK_Set),               // y assigned
                new VariableVerifier(28, 0, AVK_Set)                // x assigned in-place
            }
        ));
    }

    SECTION("set ^= set  # type: set") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {42}\n    y = {-13}\n    x ^= y",
            {
                new VariableVerifier(6, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(9, 0, AVK_Set),                // x assigned
                new VariableVerifier(15, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(18, 1, AVK_Set),               // y assigned
                new VariableVerifier(28, 0, AVK_Set)                // x assigned in-place
            }
        ));
    }
}

TEST_CASE("set unary op type inference", "[set][unary op][inference]") {
    SECTION("not set  # type: bool") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = {42}\n    y = not x",
            {
                new VariableVerifier(6, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(9, 0, AVK_Set),                // x assigned
                new VariableVerifier(13, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(16, 1, AVK_Bool)               // y assigned
            }
        ));
    }
}

TEST_CASE("tuple binary op type inference", "[tuple][binary op][inference]") {
    SECTION("tuple * bool  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ()\n    y = True\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Tuple),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Tuple),             // z assigned
            }
        ));
    }

    SECTION("tuple *= bool  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ()\n    y = True\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Tuple),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Bool),              // y assigned
                new VariableVerifier(22, 0, AVK_Tuple)              // x assigned in-place
            }
        ));
    }

    SECTION("tuple * int  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ()\n    y = 42\n    z = x * y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Tuple),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
                new VariableVerifier(22, 2, AVK_Tuple),             // z assigned
            }
        ));
    }

    SECTION("tuple *= int  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ()\n    y = 42\n    x *= y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Tuple),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Integer),           // y assigned
                new VariableVerifier(22, 0, AVK_Tuple)              // x assigned in-place
            }
        ));
    }

    SECTION("tuple[slice]  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ()\n    y = x[1:2:3]",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Tuple),              // x assigned
                new VariableVerifier(22, 1, AVK_Undefined, true),   // y not assigned yet
                new VariableVerifier(25, 1, AVK_Tuple),             // y assigned
            }
        ));
    }

    SECTION("tuple + tuple  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ()\n    y = ()\n    z = x + y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Tuple),              // x assigned
            new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
            new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
            new VariableVerifier(19, 2, AVK_Undefined, true),   // z not assigned yet
            new VariableVerifier(22, 2, AVK_Tuple),             // z assigned
            }
        ));
    }

    SECTION("tuple += tuple  # type: tuple") {
        VerifyOldTest(AITestCase(
            "def f():\n    x = ()\n    y = ()\n    x += y",
            {
                new VariableVerifier(3, 0, AVK_Undefined, true),    // x not assigned yet
                new VariableVerifier(6, 0, AVK_Tuple),              // x assigned
                new VariableVerifier(9, 1, AVK_Undefined, true),    // y not assigned yet
                new VariableVerifier(12, 1, AVK_Tuple),             // y assigned
                new VariableVerifier(22, 0, AVK_Tuple)              // x assigned in-place
            }
        ));
    }
}

TEST_CASE("tuple unary op type inference", "[tuple][unary op][inference]") {
    auto t = InferenceTest("def f():\n    x = ()\n    y = not x");
    REQUIRE(t.kind(3, 0) == AVK_Undefined);   // x not assigned yet
    REQUIRE(t.kind(6, 0) == AVK_Tuple);       // x assigned
    REQUIRE(t.kind(10, 1) == AVK_Undefined);  // y not assigned yet
    REQUIRE(t.kind(13, 1) == AVK_Bool);       // y assigned
}

TEST_CASE("None unary op type inference", "[None][unary op][inference]") {
    SECTION("not None  # type: None") {
        auto t = InferenceTest("def f():\n    x = None\n    y = not None");
        REQUIRE(t.kind(3, 0) == AVK_Undefined);   // STORE_FAST 0
        REQUIRE(t.kind(6, 0) == AVK_None);        // LOAD_CONST 0
        REQUIRE(t.kind(10, 1) == AVK_Undefined);  // STORE_FAST 1
        REQUIRE(t.kind(13, 1) == AVK_Bool);       // LOAD_CONST 0
    }
}

TEST_CASE("Function unary op type inference", "[function][unary op][inference]") {
    SECTION("not function  # type: function") {
        auto t = InferenceTest("def f():\n    def g(): pass\n    x = not g");
        REQUIRE(t.kind(9, 0) == AVK_Undefined);   // STORE_FAST 0
        REQUIRE(t.kind(12, 0) == AVK_Function);   // LOAD_FAST 0
        REQUIRE(t.kind(16, 1) == AVK_Undefined);  // STORE_FAST 1
        REQUIRE(t.kind(19, 1) == AVK_Bool);       // LOAD_CONST 0
    }
}

// `not slice` untested as no way to syntactically create a slice object in isolation.

TEST_CASE("Generalized unpacking within a list", "[list][BUILD_LIST_UNPACK][inference]") {
    SECTION("[1, *[2], 3]  # type: list") {
        auto t = InferenceTest("def f():\n  z = [1, *[2], 3]");
        REQUIRE(t.kind(15, 0) == AVK_Undefined);  // STORE_FAST 0
        REQUIRE(t.kind(18, 0) == AVK_List);       // LOAD_CONST 0
    }
}

TEST_CASE("Generalized unpacking within a tuple", "[tuple][BUILD_TUPLE_UNPACK][inference]") {
    SECTION("(1, *(2,) 3)  # type: tuple") {
        auto t = InferenceTest("def f():\n  z = (1, *(2,), 3)");
        REQUIRE(t.kind(12, 0) == AVK_Undefined);  // STORE_FAST 0
        REQUIRE(t.kind(15, 0) == AVK_Tuple);      // LOAD_CONST 0
    }
}

TEST_CASE("Generalize unpacking within a set", "[set][BUILD_SET_UNPACK][inference]") {
    SECTION("{1, *{2}, 3}  # type: set") {
        auto t = InferenceTest("def f():\n  z = {1, *{2}, 3}");
        REQUIRE(t.kind(21, 0) == AVK_Undefined);  // STORE_FAST 0
        REQUIRE(t.kind(24, 0) == AVK_Set);        // LOAD_CONST 0
    }
}

TEST_CASE("Generalize unpacking within a dict", "[dict][BUILD_MAP_UNPACK][inference]") {
    SECTION("{1:1, **{2:2}, 3:3}  # type: dict") {
        auto t = InferenceTest("def f():\n  x = {1:1, **{2:2}, 3:3}");
        REQUIRE(t.kind(30, 0) == AVK_Undefined);  // STORE_FAST 0
        REQUIRE(t.kind(33, 0) == AVK_Dict);       // LOAD_CONST 0
    }
}
