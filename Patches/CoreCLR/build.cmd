diff --git a/build.cmd b/build.cmd
index 82fa0a5..2f4f0d8 100644
--- a/build.cmd
+++ b/build.cmd
@@ -338,7 +338,7 @@ set __msbuildLogArgs=^
 /consoleloggerparameters:Summary ^
 /verbosity:minimal
 
-set __msbuildArgs="%__IntermediatesDir%\install.vcxproj" %__msbuildCommonArgs% %__msbuildLogArgs% /p:Configuration=%__BuildType%
+set __msbuildArgs="%__IntermediatesDir%\install.vcxproj" %__msbuildCommonArgs% %__msbuildLogArgs% /p:Configuration=%__BuildType% /p:FeatureCominterop=false
 
 if /i "%__BuildArch%" == "arm64" (  
     REM TODO, remove once we have msbuild support for this platform.
