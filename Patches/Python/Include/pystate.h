diff --git a/Include/pystate.h b/Include/pystate.h
index 6000b81..e141ae9 100644
--- a/Include/pystate.h
+++ b/Include/pystate.h
@@ -13,6 +13,10 @@ extern "C" {
 struct _ts; /* Forward */
 struct _is; /* Forward */
 
+typedef void*(__stdcall *CompileFunction)(PyObject*);
+typedef void*(__stdcall *JitFreeFunction)(PyObject*);
+typedef void(__stdcall *JitInitFunction)();
+
 #ifdef Py_LIMITED_API
 typedef struct _is PyInterpreterState;
 #else
@@ -41,6 +45,8 @@ typedef struct _is {
 #endif
 
     PyObject *builtins_copy;
+    CompileFunction jitcompile;
+    JitFreeFunction jitfree;
 } PyInterpreterState;
 #endif
 
