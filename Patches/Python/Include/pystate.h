diff --git a/Include/pystate.h b/Include/pystate.h
index 6000b81..c575a0b 100644
--- a/Include/pystate.h
+++ b/Include/pystate.h
@@ -12,6 +12,12 @@ extern "C" {
 
 struct _ts; /* Forward */
 struct _is; /* Forward */
+struct _frame; /* Forward declaration for PyFrameObject. */
+
+typedef void*(__stdcall *CompileFunction)(PyObject*);
+typedef void*(__stdcall *JitFreeFunction)(PyObject*);
+typedef void(__stdcall *JitInitFunction)();
+typedef PyObject*(__stdcall *EvalFrameFunction)(struct _frame *);
 
 #ifdef Py_LIMITED_API
 typedef struct _is PyInterpreterState;
@@ -41,6 +47,9 @@ typedef struct _is {
 #endif
 
     PyObject *builtins_copy;
+    CompileFunction jitcompile;
+    JitFreeFunction jitfree;
+    EvalFrameFunction eval_frame;
 } PyInterpreterState;
 #endif
 
