@echo off
rem
rem $Id$
rem

..\..\..\bin\harbour /n pe       /i..\..\..\include\
..\..\..\bin\harbour /n editorhi /i..\..\..\include\

echo -O2 -I..\..\..\include -L..\..\..\lib > build.tmp
echo -epe.exe >> build.tmp
echo pe.c editorhi.c editorlo.c >> build.tmp
echo hbdebug.lib >> build.tmp
echo hbvm.lib >> build.tmp
echo hbrtl.lib >> build.tmp
echo gtwin.lib >> build.tmp
echo hblang.lib >> build.tmp
echo hbrdd.lib >> build.tmp
echo hbmacro.lib >> build.tmp
echo hbpp.lib >> build.tmp
echo rddntx.lib >> build.tmp
echo rddcdx.lib >> build.tmp
echo rddfpt.lib >> build.tmp
echo hbsix.lib >> build.tmp
echo hbcommon.lib >> build.tmp
bcc32 @build.tmp
del build.tmp

pe