diff --git a/src/inc/gcinfoencoder.h b/src/inc/gcinfoencoder.h
index b1236ff..0e13638 100644
--- a/src/inc/gcinfoencoder.h
+++ b/src/inc/gcinfoencoder.h
@@ -103,7 +103,7 @@
 
 // As stated in issue #6008, GcInfoSize should be incorporated into debug builds. 
 #ifdef _DEBUG
-#define MEASURE_GCINFO
+//#define MEASURE_GCINFO
 #endif
 
 #ifdef MEASURE_GCINFO
