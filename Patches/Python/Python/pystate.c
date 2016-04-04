diff --git a/Python/pystate.c b/Python/pystate.c
index 7e0267a..0def12b 100644
--- a/Python/pystate.c
+++ b/Python/pystate.c
@@ -80,6 +80,9 @@ PyInterpreterState_New(void)
         interp->codecs_initialized = 0;
         interp->fscodec_initialized = 0;
         interp->importlib = NULL;
+        interp->jitcompile = NULL;
+        interp->jitfree = NULL;
+        interp->eval_frame = NULL;
 #ifdef HAVE_DLOPEN
 #ifdef RTLD_NOW
         interp->dlopenflags = RTLD_NOW;
