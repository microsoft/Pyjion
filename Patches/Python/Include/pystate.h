diff --git a/Include/pystate.h b/Include/pystate.h
index 6000b81..c89e3df 100644
--- a/Include/pystate.h
+++ b/Include/pystate.h
@@ -12,6 +12,10 @@ extern "C" {
 
 struct _ts; /* Forward */
 struct _is; /* Forward */
+struct _frame; /* Forward declaration for PyFrameObject. */
+
+typedef void(*JitInitFunction)(void);
+typedef PyObject*(*EvalFrameFunction)(struct _frame *, int);
 
 #ifdef Py_LIMITED_API
 typedef struct _is PyInterpreterState;
@@ -41,6 +45,7 @@ typedef struct _is {
 #endif
 
     PyObject *builtins_copy;
+    EvalFrameFunction eval_frame;
 } PyInterpreterState;
 #endif
 
