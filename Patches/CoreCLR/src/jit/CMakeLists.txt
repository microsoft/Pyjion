diff --git a/src/jit/CMakeLists.txt b/src/jit/CMakeLists.txt
index 8d87336..63006cd 100644
--- a/src/jit/CMakeLists.txt
+++ b/src/jit/CMakeLists.txt
@@ -367,9 +367,10 @@ else()
     )
 endif(CLR_CMAKE_PLATFORM_UNIX)
 
+# Despite the directory being named "dll", it creates a static library "clrjit_static" to link into the VM.
+add_subdirectory(dll)
+
 if (FEATURE_MERGE_JIT_AND_ENGINE)
-  # Despite the directory being named "dll", it creates a static library "clrjit_static" to link into the VM.
-  add_subdirectory(dll)
   add_subdirectory(crossgen)
 endif (FEATURE_MERGE_JIT_AND_ENGINE)
 
