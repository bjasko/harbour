/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Language Support Module (BEWIN)
 *
 * Copyright 2004 Pavel Tsarenko <tpe2@mail.ru>
 * www - http://www.xharbour.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

/* Language name: Belorussian */
/* ISO language code (2 chars): BE */
/* Codepage: Windows-1251 */

#include "hbapilng.h"

static HB_LANG s_lang =
{
   {
      /* Identification */

      "BEWIN",                     /* ID */
      "Belorussian",               /* Name (in English) */
      "���������� ",               /* Name (in native language) */
      "BY",                        /* RFC ID */
      "1251",                      /* Codepage */
      "",                          /* Version */

      /* Month names */

      "��������",
      "����",
      "������",
      "�������",
      "���",
      "�������",
      "˳����",
      "������",
      "��������",
      "���������",
      "˳������",
      "�������",

      /* Day names */

      "�������",
      "����������",
      "�������",
      "������",
      "������",
      "������",
      "������",

      /* CA-Cl*pper compatible natmsg items */

      "����� ��������    # �����     ������� ��.     �����",
      "��������� ���� �������� ?",
      "����.N",
      "** Subtotal **",
      "* Subsubtotal *",
      "*** Total ***",
      "Ins",
      "   ",
      "����������� ����",
      "��������: ",
      " - ",
      "�/�",
      "�����²���� �����",

      /* Error description names */

      "��������� �������",
      "���������� ��������",
      "����������� �����",
      "����������� �����",
      "����������� ���",
      "�������� �� ����",
      "���������� �������",
      "ѳ��������� �������",
      "������� ��������� ��������",
      "",
      "",
      "������ ������",
      "��������� �������",
      "����� �� �����������",
      "���������� �� �����",
      "���� �� �����",
      "���������� �� �����������",
      "������������ ������ � ���� �����",
      "���� ��� ���������������",
      "",
      "������� ���������",
      "������� ��������",
      "������� ���������",
      "������� �������",
      "������� �����",
      "������� �����",
      "",
      "",
      "",
      "",
      "�������� �� ��������������",
      "���� ����������",
      "�������� �����������",
      "������� ���� ��������",
      "������� ������ ��������",
      "���� �� ������",
      "���� �� �����������",
      "����������� ������糢�� ������",
      "����������� �������",
      "���� �� ���������",
      "���� ������ ��� �������",
      "������� �� ����������",
      "",
      "",
      "",
      "",
      "����������� ��������� ���������",
      "������ �� �����",
      "������� ���",
      "�� ���",
      "���������",

      /* Internal error names */

      "������������ ������� %lu: ",
      "������� ��� ��������",
      "�� ��������� ERRORBLOCK() ��� ������",
      "����������� ���� ��������� ������ ����������� �������",
      "�� ������������ �������� RDD",
      "���������� ��� ������ %s",
      "hb_xgrab �� ���� ������������ ������",
      "hb_xrealloc �������� � NULL ������������",
      "hb_xrealloc �������� � ����������� ������������",
      "hb_xrealloc �� ���� ���������������� ������",
      "hb_xfree �������� � ����������� ������������",
      "hb_xfree �������� � NULL ������������",
      "�� ���������� ��������� ���������: \'%s\'",
      "��������� ��������� ���������",
      "VM: �������� ���",
      "%s: ������ �����",
      "%s: ���������� ��� ������ ��� self",
      "%s: ������ ���� ����",
      "%s: ���������� ��� �������� �� ������ �����",
      "������ �� ���� �����",
      "%s: ������ ��������� ������� �� ����",
      "%s: ����������� ��� ����������",
      "����������� ������ ������",
      "hb_xgrab requested to allocate zero bytes",
      "hb_xrealloc requested to resize to zero bytes",
      "hb_xalloc requested to allocate zero bytes",

      /* Texts */

      "����/��/��",
      "�",
      "�"
   }
};

HB_LANG_ANNOUNCE( BEWIN );

HB_CALL_ON_STARTUP_BEGIN( hb_lang_Init_BEWIN )
   hb_langRegister( &s_lang );
HB_CALL_ON_STARTUP_END( hb_lang_Init_BEWIN )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup hb_lang_Init_BEWIN
#elif defined( HB_MSC_STARTUP )
   #if defined( HB_OS_WIN_64 )
      #pragma section( HB_MSC_START_SEGMENT, long, read )
   #endif
   #pragma data_seg( HB_MSC_START_SEGMENT )
   static HB_$INITSYM hb_vm_auto_hb_lang_Init_BEWIN = hb_lang_Init_BEWIN;
   #pragma data_seg()
#endif
