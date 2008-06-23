#
# $Id$
#
# ���������������������Ŀ
# � general definition  ��
# ������������������������
#  �����������������������
# BIN_DIR is defined in dll_55.bat, please adjust accordingly

CC          = bcc32

# ������������������Ŀ
# � source directory ��
# ���������������������
#  ��������������������
# This is standard per repository

COMMON_DIR        = .

# ���������������������������Ŀ
# � obj file output directory ��
# ������������������������������
#  �����������������������������
# Please adjust according to your environment

OBJ_DIR           = obj\dll\b32

# �������������������Ŀ
# � include directory ��
# ����������������������
#  ���������������������

INCLUDE_DIR  = include;..\..\include;

# ����������������Ŀ
# � compiler flags ��
# �������������������
#  ������������������


C_USR          = -tW -D__WIN32__ -D__EXPORT__
CLIBFLAGS      = $(C_USR) -a8 -OS -O2 -6 -c -I$(INCLUDE_DIR) -d -w-

# ��������Ŀ
# � output ��
# �����������
#  ����������
# Please adjust according to your environment

HARBOUR_DLL = ..\..\lib\b32\hbziparchdll.dll

# �������������������Ŀ
# � HARBOUR_DLL rules ��
# ����������������������
#  ���������������������

HARBOUR_DLL_OBJS = \
   $(OBJ_DIR)\hbziparc.obj\
   $(OBJ_DIR)\hbcomprs.obj\
   $(OBJ_DIR)\ziparchive.obj\
   $(OBJ_DIR)\ZipPlatform_win.obj\
   $(OBJ_DIR)\zipstorage.obj\
   $(OBJ_DIR)\zipstring.obj\
   $(OBJ_DIR)\zipplatformcomm.obj\
   $(OBJ_DIR)\ZipPathComponent_win.obj\
   $(OBJ_DIR)\zipmemfile.obj\
   $(OBJ_DIR)\zipfileheader.obj\
   $(OBJ_DIR)\zipfile.obj\
   $(OBJ_DIR)\zipnew.obj\
   $(OBJ_DIR)\zipcomp.obj\
   $(OBJ_DIR)\zipexception.obj\
   $(OBJ_DIR)\zipcompatibility.obj\
   $(OBJ_DIR)\zipcentraldir.obj\
   $(OBJ_DIR)\zipautobuffer.obj\
   $(OBJ_DIR)\stdafx.obj


# ���������Ŀ
# � project ��
# ������������
#  �����������

all: \
   $(HARBOUR_DLL)

CLEAN:
   -@if exist $(HARBOUR_DLL) del $(HARBOUR_DLL) >nul
   -@if exist ..\..\lib\b32\hbziparchdll.tds del ..\..\lib\b32\hbziparchdll.tds >nul
   -@if exist ..\..\lib\b32\hbziparchdll.map del ..\..\lib\b32\hbziparchdll.map >nul
   -@if exist $(OBJ_DIR)\*.obj del $(OBJ_DIR)\*.obj >nul
   -@if exist $(OBJ_DIR)\*.c del $(OBJ_DIR)\*.c >nul
   -@if exist $(OBJ_DIR)\*.h del $(OBJ_DIR)\*.h >nul

# ��������������������������������������Ŀ
# � Library dependencies and build rules ��
# �����������������������������������������
#  ����������������������������������������

$(HARBOUR_DLL)  : \
   $(HARBOUR_DLL_OBJS)

# ���������������������Ŀ
# � HARBOUR_DLL linking ��
# ������������������������
#  �����������������������
  echo. $(OBJ_DIR)\hbziparc.obj             + > hdll.tmp
  echo. $(OBJ_DIR)\hbcomprs.obj             + >> hdll.tmp
  echo. $(OBJ_DIR)\ziparchive.obj           + >> hdll.tmp
  echo. $(OBJ_DIR)\ZipPlatform_win.obj      + >> hdll.tmp
  echo. $(OBJ_DIR)\zipstorage.obj           + >> hdll.tmp
  echo. $(OBJ_DIR)\zipstring.obj            + >> hdll.tmp
  echo. $(OBJ_DIR)\zipplatformcomm.obj      + >> hdll.tmp
  echo. $(OBJ_DIR)\ZipPathComponent_win.obj + >> hdll.tmp
  echo. $(OBJ_DIR)\zipmemfile.obj           + >> hdll.tmp
  echo. $(OBJ_DIR)\zipfileheader.obj        + >> hdll.tmp
  echo. $(OBJ_DIR)\zipfile.obj              + >> hdll.tmp
  echo. $(OBJ_DIR)\zipnew.obj               + >> hdll.tmp
  echo. $(OBJ_DIR)\zipcomp.obj              + >> hdll.tmp
  echo. $(OBJ_DIR)\zipexception.obj         + >> hdll.tmp
  echo. $(OBJ_DIR)\zipcompatibility.obj     + >> hdll.tmp
  echo. $(OBJ_DIR)\zipcentraldir.obj        + >> hdll.tmp
  echo. $(OBJ_DIR)\zipautobuffer.obj        + >> hdll.tmp
  echo. $(OBJ_DIR)\stdafx.obj               + >> hdll.tmp
  echo. c0d32w.obj,                         + >> hdll.tmp
  echo. $(HARBOUR_DLL),                     + >> hdll.tmp
  echo. ,                                   + >> hdll.tmp
  echo. cw32.lib                            + >> hdll.tmp
  echo. ..\..\lib\harbour.lib               + >> hdll.tmp
  echo. import32.lib                        + >> hdll.tmp
  echo. uuid.lib                              >> hdll.tmp
  ILINK32 -aa -Tpd -Gn @hdll.tmp

# ����������������������Ŀ
# � object file creation ��
# �������������������������
#  ������������������������

$(OBJ_DIR)\hbziparc.obj : $(COMMON_DIR)\hbziparc.c
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\ziparchive.obj : $(COMMON_DIR)\ziparchive.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\ZipPlatform_win.obj : $(COMMON_DIR)\ZipPlatform_win.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipstorage.obj : $(COMMON_DIR)\zipstorage.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipstring.obj : $(COMMON_DIR)\zipstring.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipplatformcomm.obj : $(COMMON_DIR)\zipplatformcomm.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\ZipPathComponent_win.obj : $(COMMON_DIR)\ZipPathComponent_win.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipmemfile.obj : $(COMMON_DIR)\zipmemfile.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipfileheader.obj : $(COMMON_DIR)\zipfileheader.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipfile.obj : $(COMMON_DIR)\zipfile.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipnew.obj : $(COMMON_DIR)\zipnew.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipcomp.obj : $(COMMON_DIR)\zipcomp.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipexception.obj : $(COMMON_DIR)\zipexception.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipcompatibility.obj : $(COMMON_DIR)\zipcompatibility.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipcentraldir.obj : $(COMMON_DIR)\zipcentraldir.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\zipautobuffer.obj : $(COMMON_DIR)\zipautobuffer.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**

$(OBJ_DIR)\stdafx.obj : $(COMMON_DIR)\stdafx.cpp
   $(CC) $(CLIBFLAGS) -o$@ $**
