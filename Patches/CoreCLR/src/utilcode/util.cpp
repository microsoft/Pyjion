diff --git a/src/utilcode/util.cpp b/src/utilcode/util.cpp
index d7d3a9f..972edd7 100644
--- a/src/utilcode/util.cpp
+++ b/src/utilcode/util.cpp
@@ -212,6 +212,7 @@ typedef HRESULT __stdcall DLLGETCLASSOBJECT(REFCLSID rclsid,
 EXTERN_C const IID _IID_IClassFactory = 
     {0x00000001, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
 
+#if FEATURE_COMINTEROP
 // ----------------------------------------------------------------------------
 // FakeCoCreateInstanceEx
 // 
@@ -374,6 +375,7 @@ HRESULT FakeCoCallDllGetClassObject(REFCLSID       rclsid,
 
     return hr;
 }
+#endif
 
 #if USE_UPPER_ADDRESS
 static BYTE * s_CodeMinAddr;        // Preferred region to allocate the code in.
