@echo off
rem
rem $Id$
rem

if not "%LIBHARU_DIR%" == "" goto DIR_OK

echo ---------------------------------------------------------------
echo IMPORTANT: You'll need Haru Free PDF Library (libharu) DLL package 
echo            from www.libharu.org and this envvar to be set to 
echo            successfully build this library:
echo            set LIBHARU_DIR=-IC:\libharu
echo ---------------------------------------------------------------
goto POST_EXIT

:DIR_OK

set CFLAGS=-I%LIBHARU_DIR%\include
set HB_DLL_NAME=libhpdf
set HB_DLL_DIR=%LIBHARU_DIR%

rem ---------------------------------------------------------------

call ..\mtpl_vc.bat %1 %2 %3 %4 %5 %6 %7 %8 %9

rem ---------------------------------------------------------------

set _HB_INSTALL_PREFIX=%HB_INSTALL_PREFIX%
if "%_HB_INSTALL_PREFIX%" == "" set _HB_INSTALL_PREFIX=..\..
set _HB_LIB_INSTALL=%HB_LIB_INSTALL%
if "%_HB_LIB_INSTALL%" == "" set _HB_LIB_INSTALL=%_HB_INSTALL_PREFIX%\lib

if "%1" == "clean" goto POST_CLEAN
if "%1" == "Clean" goto POST_CLEAN
if "%1" == "CLEAN" goto POST_CLEAN
if "%1" == "install" goto POST_INSTALL
if "%1" == "Install" goto POST_INSTALL
if "%1" == "INSTALL" goto POST_INSTALL

:POST_BUILD

   rem Use supplied .lib file.
   if not exist ..\..\lib\%_HB_CC_NAME%\%HB_DLL_NAME%.lib copy %LIBHARU_DIR%\%HB_DLL_NAME%.lib ..\..\lib\%_HB_CC_NAME%\%HB_DLL_NAME%.lib > nul
   goto POST_EXIT

:POST_CLEAN

   if exist ..\..\lib\%_HB_CC_NAME%\%HB_DLL_NAME%.lib del ..\..\lib\%_HB_CC_NAME%\%HB_DLL_NAME%.lib > nul
   if exist ..\..\lib\%_HB_CC_NAME%\%HB_DLL_NAME%.exp del ..\..\lib\%_HB_CC_NAME%\%HB_DLL_NAME%.exp > nul
   if exist %_HB_LIB_INSTALL%\%HB_DLL_NAME%.lib       del %_HB_LIB_INSTALL%\%HB_DLL_NAME%.lib       > nul
   goto POST_EXIT

:POST_INSTALL

   if exist %_HB_LIB_INSTALL%\%HB_DLL_NAME%.lib del %_HB_LIB_INSTALL%\%HB_DLL_NAME%.lib
   if exist ..\..\lib\%_HB_CC_NAME%\%HB_DLL_NAME%.lib copy ..\..\lib\%_HB_CC_NAME%\%HB_DLL_NAME%.lib %_HB_LIB_INSTALL%
   goto POST_EXIT

:POST_EXIT

set CFLAGS=
set HB_DLL_NAME=
set HB_DLL_DIR=