@echo off
rem
rem $Id$
rem

rem ---------------------------------------------------------------
rem This is a generic template file, if it doesn't fit your own needs
rem please DON'T MODIFY IT.
rem
rem Instead, make a local copy and modify that one, or make a call to
rem this batch file from your customized one. [vszakats]
rem
rem Set any of the below settings to customize your build process:
rem    set HB_BUILD_MODE=C
rem    set HB_BUILD_DLL=yes
rem    set HB_BUILD_DEBUG=yes
rem    set HB_BUILD_VERBOSE=yes
rem    set HB_REBUILD_PARSER=yes
rem    set HB_MAKE_PROGRAM=
rem    set HB_MAKE_FLAGS=
rem ---------------------------------------------------------------

if "%HB_GT_LIB%"  == "" set HB_GT_LIB=gtwin
if "%HB_CC_NAME%" == "" set HB_CC_NAME=vc
if "%HB_MAKE_PROGRAM%" == "" set HB_MAKE_PROGRAM=nmake.exe
set HB_MAKEFILE=make_%HB_CC_NAME%.mak

rem ---------------------------------------------------------------

if "%1" == "clean" goto CLEAN
if "%1" == "CLEAN" goto CLEAN

if "%1" == "install" goto INSTALL
if "%1" == "INSTALL" goto INSTALL

:BUILD

   %HB_MAKE_PROGRAM% %HB_MAKE_FLAGS% -f %HB_MAKEFILE% %1 %2 %3 > make_%HB_CC_NAME%.log
   if errorlevel 1 notepad make_%HB_CC_NAME%.log
   goto EXIT

:CLEAN

   %HB_MAKE_PROGRAM% %HB_MAKE_FLAGS% -f %HB_MAKEFILE% CLEAN > make_%HB_CC_NAME%.log
   if exist make_%HB_CC_NAME%.log del make_%HB_CC_NAME%.log > nul
   if exist inst_%HB_CC_NAME%.log del inst_%HB_CC_NAME%.log > nul
   goto EXIT

:INSTALL

   if "%HB_INSTALL_PREFIX%" == "" set HB_INSTALL_PREFIX=.

   if "%HB_BIN_INSTALL%"    == "" set HB_BIN_INSTALL=%HB_INSTALL_PREFIX%\bin
   if "%HB_INC_INSTALL%"    == "" set HB_INC_INSTALL=%HB_INSTALL_PREFIX%\include
   if "%HB_LIB_INSTALL%"    == "" set HB_LIB_INSTALL=%HB_INSTALL_PREFIX%\lib

   %HB_MAKE_PROGRAM% %HB_MAKE_FLAGS% -f %HB_MAKEFILE% INSTALL > nul
   goto EXIT

:EXIT
