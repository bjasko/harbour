@echo off
rem
rem $Id$
rem

..\..\bin\harbour -w3 -q -n -km -l -i..\..\include hbmake

cl -D_CRT_SECURE_NO_DEPRECATE -nologo -O2 -W3 -I..\..\include hbmake.c hbmfrdln.c hbmgauge.c hbmlang.c /link /subsystem:CONSOLE /LIBPATH:..\..\lib hbvm.lib hbrtl.lib gtwin.lib hbnulrdd.lib hbmacro.lib hbcommon.lib user32.lib

del *.obj
del hbmake.c
