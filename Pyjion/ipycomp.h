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

#ifndef IPYCOMP_H
#define IPYCOMP_H

class Local {
public:
    int m_index;

    Local(int index = -1) {
        m_index = index;
    }

    bool is_valid() {
        return m_index != -1;
    }
};

class Label {
public:
    int m_index;

    Label(int index = -1) {
        m_index = index;
    }
};

enum LocalKind {
    LK_Pointer,
    LK_Float,
    LK_Int,
    LK_Bool
};

enum BranchType {
    BranchAlways,
    BranchTrue,
    BranchFalse,
    BranchEqual,
    BranchNotEqual,
    BranchLeave,
};

class JittedCode {
public:
    virtual ~JittedCode() {
    }
    virtual void* get_code_addr() = 0;

};

// Defines the interface between the abstract compiler and code generator
//
// The compiler is stack based, various operations can push and pop values from the stack.
// The compiler supports defining locals, labels, performing branches, etc...  Ultimately it
// will support a wide array of primitive operations, but also support higher level Python
// operations.
class IPythonCompiler {
public:
    /*****************************************************
     * Basic Python stack manipulations */
    virtual void emit_rot_two(LocalKind kind = LK_Pointer) = 0;
    virtual void emit_rot_three(LocalKind kind = LK_Pointer) = 0;
    // Pops the top value from the stack and decrements its refcount
    virtual void emit_pop_top() = 0;
    // Dups the top value on the stack (and bumps its ref count)
    virtual void emit_dup_top() = 0;
    // Dups the top two values on the stack
    virtual void emit_dup_top_two() = 0;

    /*****************************************************
     * Primitives */

     // Defines a label that can be branched to and marked at some point
    virtual Label emit_define_label() = 0;
    // Marks the location of a label at the current code offset
    virtual void emit_mark_label(Label label) = 0;
    // Emits a branch to the specified label 
    virtual void emit_branch(BranchType branchType, Label label) = 0;
    // Compares if the last two values pushed onto the stack are equal
    virtual void emit_compare_equal() = 0;

    // Emits an unboxed integer value onto the stack
    virtual void emit_int(int value) = 0;
    // Emits an unboxed floating point value onto the stack
    virtual void emit_float(double value) = 0;
    // Emits a unboxed tagged integer value onto the stack
    virtual void emit_tagged_int(ssize_t value) = 0;
    // Takes a Python long off the stack and converts it to a tagged int, or leaves
    // it as an object if the long doesn't fix in a tagged int.
    virtual void emit_unbox_int_tagged() = 0;
    // Takes a Python float off the stack and converts it to a native double
    virtual void emit_unbox_float() = 0;

    // Emits an unboxed bool onto the stack
    virtual void emit_bool(bool value) = 0;
    // Emits a pointer value onto the stack
    virtual void emit_ptr(void* value) = 0;
    // Emits the address of a Python object on the stack, bumping its ref count
    virtual void emit_py_object(PyObject* value) = 0;
    // Emits a null pointer onto the stack
    virtual void emit_null() = 0;

    // Pops a value off the stack, performing no operations related to reference counting
    virtual void emit_pop() = 0;
    // Dups the current value on the stack, performing no operations related to reference counting
    virtual void emit_dup() = 0;

    /*****************************************************
     * Boxing */

     // Boxes a raw floating point value into a Python object
    virtual void emit_box_float() = 0;
    // Boxes a raw bool into a Python object
    virtual void emit_box_bool() = 0;
    // Boxes a tagged int into a Python object
    virtual void emit_box_tagged_ptr() = 0;

    /*****************************************************
     * Stack based locals */
     // Stores the top stack value into a local (only supports pointer types)
    virtual Local emit_spill() = 0;
    // Stores the top value into a local
    virtual void emit_store_local(Local local) = 0;
    // Loads the local onto the top of the stack
    virtual void emit_load_local(Local local) = 0;
    // Loads the address of a local onto the top of the stack
    virtual void emit_load_local_addr(Local local) = 0;
    // Loads a local onto the stack and makes the local available for re-use
    virtual void emit_load_and_free_local(Local local) = 0;
    // Defines a pointer local, optionally not pulling it from the cache of available locals
    virtual Local emit_define_local(bool cache) = 0;
    // Defines a local of a specific type
    virtual Local emit_define_local(LocalKind kind = LK_Pointer) = 0;
    // Frees a local making it available for re-use
    virtual void emit_free_local(Local local) = 0;
    // Creates an array on the stack for temporary storage
    virtual Local emit_allocate_stack_array(size_t elements) = 0;

    /*****************************************************
     * Frames, basic function semantics */

     // Pushes the current Python frame into the list of frames
    virtual void emit_push_frame() = 0;
    // Pops the current Python frame from the list of frames
    virtual void emit_pop_frame() = 0;
    // Returns from the current function
    virtual void emit_ret() = 0;
    // Initializes state associated with updating the frames lasti value
    virtual void emit_lasti_init() = 0;
    // Updates the current value of last
    virtual void emit_lasti_update(int index) = 0;

    /*****************************************************
     * Loads/Stores to/from various places */

     // Loads/stores/deletes from the frame objects fast local variables
    virtual void emit_load_fast(int local) = 0;
    virtual void emit_store_fast(int local) = 0;
    virtual void emit_delete_fast(int index, PyObject* name) = 0;
    virtual void emit_unbound_local_check() = 0;

    // Loads/stores/deletes by name for values not known to be in fast locals
    virtual void emit_load_name(PyObject* name) = 0;
    virtual void emit_store_name(PyObject* name) = 0;
    virtual void emit_delete_name(PyObject* name) = 0;

    // Loads/stores/deletes an attribute on an object
    virtual void emit_load_attr(PyObject* name) = 0;
    virtual void emit_store_attr(PyObject* name) = 0;
    virtual void emit_delete_attr(PyObject* name) = 0;

    // Loads/stores/deletes a global variable
    virtual void emit_load_global(PyObject* name) = 0;
    virtual void emit_store_global(PyObject* name) = 0;
    virtual void emit_delete_global(PyObject* name) = 0;

    // Loads/stores/deletes a cell variable for closures.
    virtual void emit_load_deref(int index) = 0;
    virtual void emit_store_deref(int index) = 0;
    virtual void emit_delete_deref(int index) = 0;
    // Loads the cell object for a variable
    virtual void emit_load_closure(int index) = 0;

    // Sets/deletes a subscript value
    virtual void emit_store_subscr() = 0;
    virtual void emit_delete_subscr() = 0;
    virtual void emit_periodic_work() = 0;

    /*****************************************************
     * Collection operations */

     // Creates a new tuple of the specified size
    virtual void emit_new_tuple(size_t size) = 0;
    // Stores all of the values on the stack into a tuple
    virtual void emit_tuple_store(size_t size) = 0;
    // Convert a list to a tuple
    virtual void emit_list_to_tuple() = 0;

    // Creates a new list of the specified size
    virtual void emit_new_list(size_t argCnt) = 0;
    // Stores all of the values on the stack into a list
    virtual void emit_list_store(size_t argCnt) = 0;
    // Appends a single value to a list
    virtual void emit_list_append() = 0;
    // Extends a list with a single iterable
    virtual void emit_list_extend() = 0;

    // Creates a new set
    virtual void emit_new_set() = 0;
    // Extends a set with a single iterable
    virtual void emit_set_extend() = 0;
    // Adds a single item to a set
    virtual void emit_set_add() = 0;

    // Creates a new dictionary
    virtual void emit_new_dict(size_t size) = 0;
    // Stores a key/value pair into a set
    virtual void emit_dict_store() = 0;
    // Adds a single key/value pair to a dict
    virtual void emit_map_add() = 0;
    // Extends a map by another mapping
    virtual void emit_map_extend() = 0;

    // Creates a slice object from values on the stack
    virtual void emit_build_slice() = 0;

    // Pushes an unboxed bool onto the stack indicating the truthiness of the top value on the stack
    virtual void emit_is_true() = 0;

    // Imports the specified name
    virtual void emit_import_name(PyObject* name) = 0;
    // Imports the specified name from a module
    virtual void emit_import_from(PyObject* name) = 0;
    // Does ... import *
    virtual void emit_import_star() = 0;

    virtual void emit_load_build_class() = 0;

    // Unpacks the sequence onto the stack
    virtual void emit_unpack_sequence(Local sequence, Local sequenceStorage, Label success, size_t size) = 0;
    // Unpacks the sequence onto the stack, supporting a remainder list
    virtual void emit_unpack_ex(Local sequence, size_t leftSize, size_t rightSize, Local sequenceStorage, Local list, Local remainder) = 0;
    // Loads an element from the array on the stack
    virtual void emit_load_array(int index) = 0;

    // Emits a call for the specified argument count.  If the compiler
    // can't emit a call with this number of args then it returns false,
    // and emit_call_with_tuple is used to call with a variable sized
    // tuple instead.
    virtual bool emit_call(size_t argCnt) = 0;
    // Emits a call with the arguments to be invoked in a tuple object
    virtual void emit_call_with_tuple() = 0;
    // Emits a call with the arguments in a tuple and kw arguments in a dict
    virtual void emit_call_with_kws() = 0;
    // Emits a call which includes *args or **args
    virtual void emit_fancy_call() = 0;

    /*****************************************************
     * Function creation */

     // Creates a new function object
    virtual void emit_new_function() = 0;
    // Creates a new closure object
    virtual void emit_set_closure() = 0;
    // Sets the annotations on a function object
    virtual void emit_set_annotations() = 0;
    // Sets the KW defaults on a function object
    virtual void emit_set_kw_defaults() = 0;
    // Sets the defaults on a function object
    virtual void emit_set_defaults() = 0;

    // Prints the current value on the stack
    virtual void emit_print_expr() = 0;
    virtual void emit_load_classderef(int index) = 0;

    /*****************************************************
     * Iteration */
    virtual void emit_getiter() = 0;
    //void emit_getiter_opt() = 0;
    virtual void emit_for_next(Label processValue, Local iterValue) = 0;

    /*****************************************************
     * Operators */
     // Performs a unary positive, pushing the result onto the stack
    virtual void emit_unary_positive() = 0;
    // Performs a unary negative, pushing the result onto the stack
    virtual void emit_unary_negative() = 0;
    // Performs a unary not, pushing the Python object result onto the stack, or NULL if an error occurred
    virtual void emit_unary_not() = 0;
    // Perform a unary not, pushing an unboxed int onto the stack indicating true (1), false (0), or error
    virtual void emit_unary_not_push_int() = 0;

    // Does a unary not on an unboxed floating value, pushing an unboxed bool onto the stack
    virtual void emit_unary_not_float_push_bool() = 0;
    // Performs a unary negative on a tagged integer
    virtual void emit_unary_not_tagged_int_push_bool() = 0;
    // Performs a unary invert on the top value on the stack, pushing the result onto the stack or NULL if an error occurred
    virtual void emit_unary_invert() = 0;

    // Peforms a unary negative on a unboxed floating value on the stack, pushing the unboxed result back to the stack
    virtual void emit_unary_negative_float() = 0;
    // Performs a unary negative on a tagged integer
    virtual void emit_unary_negative_tagged_int() = 0;

    // Performans a binary operation for values on the stack which are unboxed floating points
    virtual void emit_binary_float(int opcode) = 0;
    // Performs a binary operation for values on the stack which are boxed objects
    virtual void emit_binary_object(int opcode) = 0;

    virtual void emit_binary_tagged_int(int opcode) = 0;

    virtual void emit_tagged_int_to_float() = 0;

    // Does an in/contains check and pushes a Python object onto the stack as the result, or NULL if there was an error
    virtual void emit_in() = 0;
    // Does an in/contains check and pushes an unboxed int onto the stack indicating true (1)/false (0)/error (-1)
    virtual void emit_in_push_int() = 0;
    // Does an not in check and pushes a Python object onto the stack as the result, or NULL if there was an error
    virtual void emit_not_in() = 0;
    // Does an not in check and pushes an unboxed int onto the stack indicating true (1)/false (0)/error (-1)
    virtual void emit_not_in_push_int() = 0;

    // Does an is check and pushes a boxed Python bool on the stack as the result
    virtual void emit_is(bool isNot) = 0;
    // Does an is check and pushes an unboxed bool onto the stack
    virtual void emit_is_push_int(bool isNot) = 0;

    // Performs a comparison for values on the stack which are objects, keeping a boxed Python object as the result.
    virtual void emit_compare_object(int compareType) = 0;
    // Performs a comparison of two Python objects on the stack keeping an unboxed bool on the stack as the result
    virtual bool emit_compare_object_push_int(int compareType) = 0;
    // Performs a comparison of two unboxed floating point values on the stack
    virtual void emit_compare_float(int compareType) = 0;
    // Performs a comparison of two tagged integers
    virtual void emit_compare_tagged_int(int compareType) = 0;

    /*****************************************************
     * Exception handling */
     // Raises an exception taking the exception, type, and cause
    virtual void emit_raise_varargs() = 0;
    // Clears the current exception
    // Updates the trace back as it propagtes through a function
    virtual void emit_eh_trace() = 0;
    // Performs exception handling unwind as we go through loops
    virtual void emit_unwind_eh(Local prevExc, Local prevExcVal, Local prevTraceback) = 0;
    // Prepares to raise an exception, storing the existing exceptions
    virtual void emit_prepare_exception(Local prevExc, Local prevExcVal, Local prevTraceback) = 0;
    // Restores the previous exception for nested exception handling
    virtual void emit_restore_err() = 0;
    // Compares to see if an exception is handled, pushing a Python bool onto the stack
    virtual void emit_compare_exceptions() = 0;
    // Compares to see if an exception is handled, pushing an int on the stack indicating true (1), false (0), or error (-1)
    virtual void emit_compare_exceptions_int() = 0;
    // Sets the current exception type and text
    virtual void emit_pyerr_setstring(PyObject* exception, const char*msg) = 0;

    virtual void emit_incref(bool maybeTagged = false) = 0; 

    virtual void emit_debug_msg(const char* msg) = 0;
    /* Compiles the generated code */
    virtual JittedCode* emit_compile() = 0;
};

#endif