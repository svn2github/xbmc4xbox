@ECHO OFF
cls
COLOR 1B
rem ----PURPOSE----
rem - Create a working XBMC build with a single click
rem ---------------------------------------------
rem Config
rem If you get an error that Visual studio was not found, SET your path for VSNET main executable.
rem ONLY needed if you have a very old bios, SET the path for xbepatch. Not needed otherwise.
rem If Winrar isn't installed under standard programs, SET the path for WinRAR's (freeware) rar.exe
rem and finally set the options for the final rar.
rem ---------------------------------------------
rem Remove 'rem' from 'web / python' below to copy these to the BUILD directory.
rem ---------------------------------------------
TITLE XBMC Build Prepare Script
ECHO Wait while preparing the build.
ECHO ------------------------------
rem	CONFIG START

	set Silent=0
	set SkipCompression=0

:PARAMETERS
	IF "%1"=="noprompt" (
	  set Silent=1
	) ELSE IF "%1"=="nocompress" (
	  set SkipCompression=1
	) ELSE IF "%1"=="" (
	  goto PARAMSDONE
	)
	SHIFT
	goto PARAMETERS
:PARAMSDONE

	IF "%VS71COMNTOOLS%"=="" (
	  set NET="%ProgramFiles%\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv.com"
	) ELSE (
	  set NET="%VS71COMNTOOLS%\..\IDE\devenv.com"
	)
	IF NOT EXIST %NET% (
	  set DIETEXT=Visual Studio .NET 2003 was not found.
	  goto DIE
	) 
	set OPTS=xbmc.sln /build debug
	set CLEAN=xbmc.sln /clean debug
	set XBE=debug\default.xbe
	set XBE_PATCH=tools\xbepatch\xbepatch.exe
	
	SET RAR="?"
	IF EXIST "%ProgramFiles(x86)%\Winrar\rar.exe" SET RAR="%ProgramFiles(x86)%\Winrar\rar.exe"
	IF EXIST "%ProgramFiles%\Winrar\rar.exe"      SET RAR="%ProgramFiles%\Winrar\rar.exe"
	IF EXIST "%ProgramW6432%\Winrar\rar.exe" SET RAR="%ProgramW6432%\Winrar\rar.exe"
	
	set RAROPS=a -r -idp -inul -m5 XBMC.rar BUILD
rem	CONFIG END
rem ---------------------------------------------

rem	check for existing xbe
rem ---------------------------------------------

IF %Silent%==1 GOTO COMPILE
IF NOT EXIST debug\default.xbe GOTO COMPILE

:XBE_EXIST
  ECHO ------------------------------
  ECHO Found a previous Compiled XBE!
  ECHO [Y] a new XBE will be compiled for the BUILD 
  ECHO [N] the existing XBE will be used for the BUILD 
  ECHO ------------------------------
  set /P XBMC_COMPILE_ANSWER=Compile a new XBE? [y/n]
  if /I %XBMC_COMPILE_ANSWER% NEQ y goto MAKE_BUILD
  if /I %XBMC_COMPILE_ANSWER% NEQ n goto COMPILE

:COMPILE
  ECHO Compiling Solution...
  %NET% %CLEAN%
  del debug\xbmc.map 2>NUL
  %NET% %OPTS%
  IF NOT EXIST %XBE% (
  	set DIETEXT=Default.xbe failed to build!  See .\debug\BuildLog.htm for details.
  	goto DIE
  )
  ECHO Done!
  ECHO ------------------------------
  GOTO MAKE_BUILD

:MAKE_BUILD
  ECHO Copying files...
  ECHO - XBE Patching %XBE% 
  %XBE_PATCH% %XBE%
  ECHO - Patching Done!
  
  rmdir BUILD /S /Q
  md BUILD
  
  Echo .svn>exclude.txt
  Echo Thumbs.db>>exclude.txt
  Echo Desktop.ini>>exclude.txt
  Echo dsstdfx.bin>>exclude.txt
  Echo exclude.txt>>exclude.txt

  copy %XBE% BUILD
  xcopy UserData BUILD\UserData /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy *.txt BUILD /EXCLUDE:exclude.txt
  rem xcopy *.xml BUILD\

  cd "skin\Project Mayhem III"
  CALL build.bat
  cd ..\..
  xcopy "skin\Project Mayhem III\BUILD\Project Mayhem III" "BUILD\skin\Project Mayhem III" /E /Q /I /Y /EXCLUDE:exclude.txt

  cd "skin\PM3.HD"
  CALL build.bat
  cd ..\..
  xcopy "skin\PM3.HD\BUILD\PM3.HD" "BUILD\skin\PM3.HD" /E /Q /I /Y /EXCLUDE:exclude.txt

  cd "skin\Confluence"
  CALL build.bat
  cd ..\..
  xcopy "skin\Confluence\BUILD\Confluence" "BUILD\skin\Confluence" /E /Q /I /Y /EXCLUDE:exclude.txt

  cd "skin\Confluence Lite"
  CALL build.bat
  cd ..\..
  xcopy "skin\Confluence Lite\BUILD\Confluence Lite" "BUILD\skin\Confluence Lite" /E /Q /I /Y /EXCLUDE:exclude.txt

  xcopy credits BUILD\credits /Q /I /Y /EXCLUDE:exclude.txt
  xcopy language BUILD\language /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy screensavers BUILD\screensavers /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy visualisations BUILD\visualisations /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy system BUILD\system /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy web\XBMC_Reloaded BUILD\web /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy media   BUILD\media   /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy plugins BUILD\plugins /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy sounds  BUILD\sounds  /E /Q /I /Y /EXCLUDE:exclude.txt
  xcopy scripts BUILD\scripts /E /Q /I /Y /EXCLUDE:exclude.txt

  del exclude.txt
  ECHO ------------------------------
  IF NOT EXIST %RAR% (
  	ECHO WinRAR not installed!  Skipping .rar compression...
  ) ELSE IF %SkipCompression%==1 (
  	ECHO Skipping build compression.
  ) ELSE (
    	ECHO Compressing build to XBMC.rar file...
    	%RAR% %RAROPS%
  )

  ECHO ------------------------------
  ECHO Build Succeeded!

  IF %Silent%==1 GOTO END
  GOTO VIEWLOG
  
:DIE
  ECHO !-!-!-!-!-!-!-!-!-!-!-!-!-!-!-
  set DIETEXT=ERROR: %DIETEXT%
  echo %DIETEXT%

:VIEWLOG
  set /P XBMC_BUILD_ANSWER=View the build log in your HTML browser? [y/n]
  if /I %XBMC_BUILD_ANSWER% NEQ y goto VIEWPAUSE
  start /D"%~dp0debug" BuildLog.htm"

:VIEWPAUSE
  set XBMC_BUILD_ANSWER=
  ECHO Press any key to exit...
  pause > NUL
  
:END