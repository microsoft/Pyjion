diff --git a/Python/pylifecycle.c b/Python/pylifecycle.c
index 4f5efc9..fe8de7b 100644
--- a/Python/pylifecycle.c
+++ b/Python/pylifecycle.c
@@ -321,6 +321,18 @@ _Py_InitializeEx_Private(int install_sigs, int install_importlib)
     if (interp == NULL)
         Py_FatalError("Py_Initialize: can't make first interpreter");
 
+    HMODULE pyjit = LoadLibrary("pyjit.dll");
+    if (pyjit != NULL) {
+        interp->eval_frame = (EvalFrameFunction)GetProcAddress(pyjit, "EvalFrame");
+        if (interp->eval_frame != NULL) {
+            JitInitFunction jitinit = (JitInitFunction)GetProcAddress(pyjit, "JitInit");
+            jitinit();
+        }
+    }
+    else {
+        interp->eval_frame = NULL;
+    }
+
     tstate = PyThreadState_New(interp);
     if (tstate == NULL)
         Py_FatalError("Py_Initialize: can't make first thread");
