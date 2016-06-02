diff --git a/Include/code.h b/Include/code.h
index 56e6ec1..018aa0d 100644
--- a/Include/code.h
+++ b/Include/code.h
@@ -7,6 +7,10 @@
 extern "C" {
 #endif
 
+struct _frame;
+
+typedef PyObject* (*Py_EvalFunc)(void*, struct _frame*);
+
 /* Bytecode object */
 typedef struct {
     PyObject_HEAD
@@ -35,6 +39,7 @@ typedef struct {
 				   Objects/lnotab_notes.txt for details. */
     void *co_zombieframe;     /* for optimization only (see frameobject.c) */
     PyObject *co_weakreflist;   /* to support weakrefs to code objects */
+    PyObject *co_extra;         /* Extra information/scratch space related to the code object. */
 } PyCodeObject;
 
 /* Masks for co_flags above */
