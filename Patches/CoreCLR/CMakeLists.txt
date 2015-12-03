diff --git a/CMakeLists.txt b/CMakeLists.txt
index 80625f9..7d4642d 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -573,7 +573,7 @@ add_definitions(-DFEATURE_ASYNC_IO)
 add_definitions(-DFEATURE_BCL_FORMATTING)
 add_definitions(-DFEATURE_COLLECTIBLE_TYPES)

-if(WIN32)
+if(FALSE)
     add_definitions(-DFEATURE_CLASSIC_COMINTEROP)
     add_definitions(-DFEATURE_APPX)
     add_definitions(-DFEATURE_COMINTEROP)
