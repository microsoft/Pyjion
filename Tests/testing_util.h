#pragma once

#ifndef PYJION_TESTING_UTIL_H
#define PYJION_TESTING_UTIL_H 1

#include <Python.h>
#include <absint.h>

PyCodeObject* CompileCode(const char*);
PyCodeObject* CompileCode(const char* code, vector<const char*> locals, vector<const char*> globals);

class TestInput {
public:
    const char* m_expected;
    vector<PyObject*> m_args;

    explicit TestInput(const char* expected) {
        m_expected = expected;
    }

    TestInput(const char* expected, vector<PyObject*> args) {
        m_expected = expected;
        m_args = args;
    }
};

class TestCase {
public:
    const char* m_code;
    vector<TestInput> m_inputs;

    TestCase(const char *code, const char* expected) {
        m_code = code;
        m_inputs.push_back(TestInput(expected));

    }

    TestCase(const char *code, TestInput input) {
        m_code = code;
        m_inputs.push_back(input);
    }

    TestCase(const char *code, vector<TestInput> inputs) {
        m_code = code;
        m_inputs = inputs;
    }
};

class AIVerifier {
public:
    virtual void verify(AbstractInterpreter& interpreter) = 0;
};

class StackVerifier : public AIVerifier {
    size_t m_byteCodeIndex, m_stackIndex;
    AbstractValueKind m_kind;
public:
    StackVerifier(size_t byteCodeIndex, size_t stackIndex, AbstractValueKind kind);

    void verify(AbstractInterpreter& interpreter) override;
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
    VariableVerifier(size_t byteCodeIndex, size_t localIndex, AbstractValueKind kind, bool undefined = false) ;

    void verify(AbstractInterpreter& interpreter) override;
};

class ReturnVerifier : public AIVerifier {
    AbstractValueKind m_kind;
public:
    explicit ReturnVerifier(AbstractValueKind kind);

    void verify(AbstractInterpreter& interpreter) override;
};

class BoxVerifier : public AIVerifier {
public:
    BoxVerifier(size_t byteCodeIndex, bool shouldBox);

    void verify(AbstractInterpreter& interpreter) override {};
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
        m_verifiers = std::move(verifiers);
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

void VerifyOldTest(AITestCase testCase);

PyObject* Incremented(PyObject*o);

#endif // !PYJION_TESTING_UTIL_H
