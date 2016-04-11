diff --git a/Python/pystate.c b/Python/pystate.c
index 7e0267a..31dd664 100644
--- a/Python/pystate.c
+++ b/Python/pystate.c
@@ -80,6 +80,7 @@ PyInterpreterState_New(void)
         interp->codecs_initialized = 0;
         interp->fscodec_initialized = 0;
         interp->importlib = NULL;
+        interp->eval_frame = NULL;
 #ifdef HAVE_DLOPEN
 #ifdef RTLD_NOW
         interp->dlopenflags = RTLD_NOW;
