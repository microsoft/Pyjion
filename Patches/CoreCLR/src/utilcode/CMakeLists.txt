diff --git a/src/utilcode/CMakeLists.txt b/src/utilcode/CMakeLists.txt
index 52a017f..5a37a4b 100644
--- a/src/utilcode/CMakeLists.txt
+++ b/src/utilcode/CMakeLists.txt
@@ -71,8 +71,7 @@ set(UTILCODE_SOURCES
 # They should be moved out from here into the declaration
 # of UTILCODE_SOURCES above after fixing compiler errors.
 if(WIN32)
-  list(APPEND UTILCODE_SOURCES 
-    appxutil.cpp
+  list(APPEND UTILCODE_SOURCES
     dlwrap.cpp
     downlevel.cpp
     loadrc.cpp
