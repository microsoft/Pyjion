diff --git a/CMakeLists.txt b/CMakeLists.txt
index ba328f4..23585c8 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -762,7 +762,7 @@ add_definitions(-DFEATURE_ASYNC_IO)
 add_definitions(-DFEATURE_BCL_FORMATTING)
 add_definitions(-DFEATURE_COLLECTIBLE_TYPES)
 
-if(WIN32)
+if(FALSE)
     add_definitions(-DFEATURE_CLASSIC_COMINTEROP)
     add_definitions(-DFEATURE_COMINTEROP)
     add_definitions(-DFEATURE_COMINTEROP_APARTMENT_SUPPORT)
