diff --git a/src/utilcode/longfilepathwrappers.cpp b/src/utilcode/longfilepathwrappers.cpp
index 2b5a8b4..54c8e09 100644
--- a/src/utilcode/longfilepathwrappers.cpp
+++ b/src/utilcode/longfilepathwrappers.cpp
@@ -1190,10 +1190,6 @@ FindFirstFileExWrapper(
 
 #ifndef FEATURE_PAL
 
-#if ! defined(DACCESS_COMPILE) && !defined(SELF_NO_HOST)
-extern HINSTANCE            g_pMSCorEE;
-#endif// ! defined(DACCESS_COMPILE) && !defined(SELF_NO_HOST)
-
 BOOL PAL_GetPALDirectoryWrapper(SString& pbuffer)
 {
 
@@ -1203,10 +1199,6 @@ BOOL PAL_GetPALDirectoryWrapper(SString& pbuffer)
     DWORD dwPath; 
     HINSTANCE hinst = NULL;
 
-#if ! defined(DACCESS_COMPILE) && !defined(SELF_NO_HOST)
-    hinst = g_pMSCorEE;
-#endif// ! defined(DACCESS_COMPILE) && !defined(SELF_NO_HOST)
-
 #ifndef CROSSGEN_COMPILE
     _ASSERTE(hinst != NULL);
 #endif
