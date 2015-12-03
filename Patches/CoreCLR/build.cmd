diff --git a/build.cmd b/build.cmd
index 046970e..960cfb7 100644
--- a/build.cmd
+++ b/build.cmd
@@ -123,7 +123,7 @@ exit /b 1
 :: Note: We've disabled node reuse because it causes file locking issues.
 ::       The issue is that we extend the build with our own targets which
 ::       means that that rebuilding cannot successfully delete the task
-::       assembly. 
+::       assembly.
 if /i "%__VSVersion%" =="vs2015" goto MSBuild14
 set _msbuildexe="%ProgramFiles(x86)%\MSBuild\12.0\Bin\MSBuild.exe"
 if not exist %_msbuildexe% set _msbuildexe="%ProgramFiles%\MSBuild\12.0\Bin\MSBuild.exe"
@@ -174,8 +174,8 @@ exit /b 1
 REM Build CoreCLR
 :BuildCoreCLR
 set "__CoreCLRBuildLog=%__LogsDir%\CoreCLR_%__BuildOS%__%__BuildArch%__%__BuildType%.log"
-%_msbuildexe% "%__IntermediatesDir%\install.vcxproj" %__MSBCleanBuildArgs% /nologo /maxcpucount /nodeReuse:false /p:Configuration=%__BuildType% /p:Platform=%__BuildArch% /fileloggerparameters:Verbosity=normal;LogFile="%__CoreCLRBuildLog%"
-IF NOT ERRORLEVEL 1 goto PerformMScorlibBuild
+%_msbuildexe% "%__IntermediatesDir%\install.vcxproj" %__MSBCleanBuildArgs% /nologo /maxcpucount /nodeReuse:false /p:Configuration=%__BuildType% /p:Platform=%__BuildArch% /fileloggerparameters:Verbosity=normal;LogFile="%__CoreCLRBuildLog%" /p:FeatureCominterop=false
+IF NOT ERRORLEVEL 1 goto SuccessfulBuild
 echo Native component build failed. Refer !__CoreCLRBuildLog! for details.
 exit /b 1
 
