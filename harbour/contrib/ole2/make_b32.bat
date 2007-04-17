@echo off
rem
rem $Id$
rem

if "%1" == "clean" goto CLEAN
if "%1" == "CLEAN" goto CLEAN

:BUILD

   make -fmakefile.bc %1 %2 %3 > make_b32.log
   if errorlevel 1 goto BUILD_ERR

:BUILD_OK

   copy ..\..\lib\b32\hboleaut.lib ..\..\lib\*.* > nul
   if exist ..\..\lib\b32\hboleaut.bak del ..\..\lib\b32\oleaut.bak
   goto EXIT

:BUILD_ERR

   notepad make_b32.log
   goto EXIT

:CLEAN
   if exist ..\..\lib\b32\hboleaut.lib   del ..\..\lib\b32\hboleaut.lib
   if exist ..\..\lib\b32\hboleaut.bak   del ..\..\lib\b32\hboleaut.bak
   if exist ..\..\obj\b32\win32ole.obj   del ..\..\obj\b32\win32ole.obj
   if exist ..\..\obj\b32\w32ole.obj     del ..\..\obj\b32\w32ole.obj

   goto EXIT

:EXIT