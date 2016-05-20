diff --git a/src/CMakeLists.txt b/src/CMakeLists.txt
index 4ae4a7c..076a241 100644
--- a/src/CMakeLists.txt
+++ b/src/CMakeLists.txt
@@ -125,11 +125,7 @@ endif(CLR_CMAKE_PLATFORM_UNIX)
 
 add_subdirectory(utilcode)
 add_subdirectory(gcinfo)
-add_subdirectory(coreclr)
 add_subdirectory(jit)
-add_subdirectory(vm)
-add_subdirectory(md)
-add_subdirectory(debug)
 add_subdirectory(inc)
 add_subdirectory(strongname)
 add_subdirectory(binder)
@@ -141,7 +137,7 @@ add_subdirectory(unwinder)
 add_subdirectory(ildasm)
 add_subdirectory(ilasm)
 
-if(WIN32)
+if(FALSE)
   add_subdirectory(ipcman)
 endif(WIN32)
 
