diff --git a/Python/pylifecycle.c b/Python/pylifecycle.c
index 4f5efc9..c3fd65b 100644
--- a/Python/pylifecycle.c
+++ b/Python/pylifecycle.c
@@ -16,6 +16,12 @@
 #include "osdefs.h"
 #include <locale.h>
 
+#ifdef HAVE_DLOPEN
+#ifdef HAVE_DLFCN_H
+#include <dlfcn.h>
+#endif
+#endif
+
 #ifdef HAVE_SIGNAL_H
 #include <signal.h>
 #endif
@@ -287,9 +293,14 @@ _Py_InitializeEx_Private(int install_sigs, int install_importlib)
     PyInterpreterState *interp;
     PyThreadState *tstate;
     PyObject *bimod, *sysmod, *pstderr;
-    char *p;
+#if defined(WIN32) || defined(WIN64) 
+	HMODULE pyjit;
+#elif defined(HAVE_DLOPEN)
+	void *pyjit;
+#endif
+	char *p;
     extern void _Py_ReadyTypes(void);
-
+	
     if (initialized)
         return;
     initialized = 1;
@@ -321,6 +332,26 @@ _Py_InitializeEx_Private(int install_sigs, int install_importlib)
     if (interp == NULL)
         Py_FatalError("Py_Initialize: can't make first interpreter");
 
+#if defined(WIN32) || defined(WIN64) 
+    pyjit = LoadLibrary("pyjit.dll");
+    if (pyjit != NULL) {
+        interp->eval_frame = (EvalFrameFunction)GetProcAddress(pyjit, "EvalFrame");
+        if (interp->eval_frame != NULL) {
+            JitInitFunction jitinit = (JitInitFunction)GetProcAddress(pyjit, "JitInit");
+            jitinit();
+        }
+    }
+#elif defined(HAVE_DLOPEN)
+	pyjit = dlopen("pyjit.so", 0);
+	if (pyjit != NULL) {
+		interp->eval_frame = (EvalFrameFunction)dlsym(pyjit, "EvalFrame");
+		if (interp->eval_frame != NULL) {
+			JitInitFunction jitinit = (JitInitFunction)dlsym(pyjit, "JitInit");
+			jitinit();
+		}
+	}
+#endif
+
     tstate = PyThreadState_New(interp);
     if (tstate == NULL)
         Py_FatalError("Py_Initialize: can't make first thread");
