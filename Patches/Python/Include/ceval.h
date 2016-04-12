diff --git a/Include/ceval.h b/Include/ceval.h
index b5373a9..c23b562 100644
--- a/Include/ceval.h
+++ b/Include/ceval.h
@@ -10,6 +10,11 @@ extern "C" {
 PyAPI_FUNC(PyObject *) PyEval_CallObjectWithKeywords(
     PyObject *, PyObject *, PyObject *);
 
+PyAPI_FUNC(PyObject *) _PyEval_EvalCodeWithName(PyObject *_co, PyObject *globals, PyObject *locals,
+PyObject **args, int argcount, PyObject **kws, int kwcount,
+PyObject **defs, int defcount, PyObject *kwdefs, PyObject *closure,
+PyObject *name, PyObject *qualname);
+
 /* Inline this */
 #define PyEval_CallObject(func,arg) \
     PyEval_CallObjectWithKeywords(func, arg, (PyObject *)NULL)
@@ -82,6 +87,11 @@ PyAPI_FUNC(int) Py_GetRecursionLimit(void);
 PyAPI_FUNC(int) _Py_CheckRecursiveCall(const char *where);
 PyAPI_DATA(int) _Py_CheckRecursionLimit;
 
+/* Checks to see if periodic work needs to be done, such as releasing the GIL,
+   make any pending calls, or raising an asychronous exception.  Returns true
+   if an error occurred or false on success. */
+PyAPI_FUNC(int) _PyEval_PeriodicWork(void);
+
 #ifdef USE_STACKCHECK
 /* With USE_STACKCHECK, we artificially decrement the recursion limit in order
    to trigger regular stack checks in _Py_CheckRecursiveCall(), except if
@@ -119,6 +129,7 @@ PyAPI_FUNC(const char *) PyEval_GetFuncDesc(PyObject *);
 PyAPI_FUNC(PyObject *) PyEval_GetCallStats(PyObject *);
 PyAPI_FUNC(PyObject *) PyEval_EvalFrame(struct _frame *);
 PyAPI_FUNC(PyObject *) PyEval_EvalFrameEx(struct _frame *f, int exc);
+PyAPI_FUNC(PyObject *) PyEval_EvalFrameDefault(struct _frame *f, int exc);
 
 /* Interface for threads.
 
