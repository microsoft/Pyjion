diff --git a/src/CMakeLists.txt b/src/CMakeLists.txt
index 4ae4a7c..b76de27 100644
--- a/src/CMakeLists.txt
+++ b/src/CMakeLists.txt
@@ -125,25 +125,12 @@ endif(CLR_CMAKE_PLATFORM_UNIX)
 
 add_subdirectory(utilcode)
 add_subdirectory(gcinfo)
-add_subdirectory(coreclr)
 add_subdirectory(jit)
-add_subdirectory(vm)
-add_subdirectory(md)
-add_subdirectory(debug)
 add_subdirectory(inc)
-add_subdirectory(strongname)
-add_subdirectory(binder)
-add_subdirectory(classlibnative)
-add_subdirectory(dlls)
-add_subdirectory(ToolBox)
-add_subdirectory(tools)
-add_subdirectory(unwinder)
-add_subdirectory(ildasm)
-add_subdirectory(ilasm)
 
-if(WIN32)
+if(FALSE)
   add_subdirectory(ipcman)
-endif(WIN32)
+endif(FALSE)
 
 if(CLR_CMAKE_PLATFORM_UNIX)
     add_subdirectory(palrt)
