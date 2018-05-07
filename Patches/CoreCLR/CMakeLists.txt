diff --git a/CMakeLists.txt b/CMakeLists.txt
index ba328f4..2837e0a 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -762,13 +762,13 @@ add_definitions(-DFEATURE_ASYNC_IO)
 add_definitions(-DFEATURE_BCL_FORMATTING)
 add_definitions(-DFEATURE_COLLECTIBLE_TYPES)
 
-if(WIN32)
+if(FALSE)
     add_definitions(-DFEATURE_CLASSIC_COMINTEROP)
     add_definitions(-DFEATURE_COMINTEROP)
     add_definitions(-DFEATURE_COMINTEROP_APARTMENT_SUPPORT)
     add_definitions(-DFEATURE_COMINTEROP_UNMANAGED_ACTIVATION)
     add_definitions(-DFEATURE_COMINTEROP_WINRT_MANAGED_ACTIVATION)
-endif(WIN32)
+endif(FALSE)
 
 add_definitions(-DFEATURE_CORECLR)
 if (CLR_CMAKE_PLATFORM_UNIX)
