diff --git a/Include/code.h b/Include/code.h
index 56e6ec1..3389443 100644
--- a/Include/code.h
+++ b/Include/code.h
@@ -7,6 +7,23 @@
 extern "C" {
 #endif
 
+struct _frame;
+
+typedef PyObject* (__stdcall*  Py_EvalFunc)(void*, struct _frame*);
+
+PyAPI_DATA(PyTypeObject) PyJittedCode_Type;
+
+/* Jitted code object.  This object is returned from the JIT implementation.  The JIT can allocate
+   a jitted code object and fill in the state for which is necessary for it to perform an evaluation. */
+typedef struct {
+    PyObject_HEAD
+    Py_EvalFunc j_evalfunc;
+    void* j_evalstate;          /* opaque value, allows the JIT to track any relevant state */
+} PyJittedCode;
+
+/* Creates a new PyJittedCode object which can have the eval function and state populated. */
+PyAPI_FUNC(PyObject*) PyJittedCode_New();
+
 /* Bytecode object */
 typedef struct {
     PyObject_HEAD
@@ -35,6 +52,9 @@ typedef struct {
 				   Objects/lnotab_notes.txt for details. */
     void *co_zombieframe;     /* for optimization only (see frameobject.c) */
     PyObject *co_weakreflist;   /* to support weakrefs to code objects */
+    PyJittedCode* co_jitted;    /* Jitted code object */
+    int co_runcount;            /* The number of times the code object has been invoked */
+    PyObject *co_extra;         /* Extra information/scratch space related to the code object. */
 } PyCodeObject;
 
 /* Masks for co_flags above */
