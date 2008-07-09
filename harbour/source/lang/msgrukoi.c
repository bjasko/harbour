/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Language Support Module (RUKOI-8)
 *
 * Copyright 2002 Alexander S.Kresin <alex@belacy.belgorod.su>
 * www - http://www.harbour-project.org
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

/* Language name: Russian */
/* ISO language code (2 chars): RU */
/* Codepage: KOI-8 */

#include "hbapilng.h"

static HB_LANG s_lang =
{
   {
      /* Identification */

      "RUKOI8",                    /* ID */
      "Russian",                   /* Name (in English) */
      "�������",                   /* Name (in native language) */
      "RU",                        /* RFC ID */
      "KOI8",                       /* Codepage */
      "$Revision$ $Date$",         /* Version */

      /* Month names */

      "������",
      "�������",
      "����",
      "������",
      "���",
      "����",
      "����",
      "������",
      "��������",
      "�������",
      "������",
      "�������",

      /* Day names */

      "�����������",
      "�����������",
      "�������",
      "�����",
      "�������",
      "�������",
      "�������",

      /* CA-Cl*pper compatible natmsg items */

      "����� ������      # ������     ��������� ���.  ������",
      "����� ��� ������� ?",
      "���.N",
      "** Subtotal **",
      "* Subsubtotal *",
      "*** Total ***",
      "Ins",
      "   ",
      "�������� ����",
      "��������: ",
      " - ",
      "�/�",
      "�������� ���������",

      /* Error description names */

      "����������� ������",
      "�������� ��������",
      "������������ �������",
      "������������ ������",
      "������������ �����",
      "������� �� ����",
      "��������� ������",
      "�������������� ������",
      "������� ������� ��������",
      "",
      "",
      "�� ������� ������",
      "����������� �������",
      "����� �� �������������",
      "���������� �� ����������",
      "����� �� ����������",
      "���������� �� ��������������",
      "������������ ������� � ����� ������",
      "����� ��� ������������",
      "",
      "������ ��������",
      "������ ��������",
      "������ ��������",
      "������ ������",
      "������ ������",
      "������ ������",
      "",
      "",
      "",
      "",
      "�������� �� ��������������",
      "����� ��������",
      "���������� �����������",
      "������ ���� ������",
      "������ ������� ������",
      "���� �� ������",
      "���� �� ������������",
      "��������� ������������ ������",
      "��������� ����������",
      "������ �� ���������",
      "���� ���������� ��� ����������",
      "���������� �� �������",
      "",
      "",
      "",
      "",
      "�������� ���������� ����������",
      "������ � �������",
      "���������� �������",
      "�� ������",
      "���������",

      /* Internal error names */

      "������������ ������ %lu: ",
      "������ ��� ��������������",
      "�� ��������� ERRORBLOCK() ��� ������",
      "�������� ������ ����������� ������� ����������� ������",
      "�� ������� ��������� RDD",
      "�������� ��� ������ %s",
      "hb_xgrab �� ����� ������������ ������",
      "hb_xrealloc ������ � NULL ����������",
      "hb_xrealloc ������ � �������� ����������",
      "hb_xrealloc ��� ����� ���������������� ������",
      "hb_xfree ������ � �������� ����������",
      "hb_xfree ������ � NULL ����������",
      "�� ������� ��������� ���������: \'%s\'",
      "����������� ��������� ���������",
      "VM: ����������� ���",
      "%s: �������� ������",
      "%s: �������� ��� ������� ��� self",
      "%s: �������� ��������",
      "%s: �������� ��� �������� �� ������� �����",
      "����� �� ������� �����",
      "%s: ������� ���������� ������� �� ����",
      "%s: �������� ��� ����������",
      "������������ ������ ������",
      "hb_xgrab requested to allocate zero bytes",
      "hb_xrealloc requested to resize to zero bytes",
      "hb_xalloc requested to allocate zero bytes",

      /* Texts */

      "����/��/��",
      "�",
      "�"
   }
};

HB_LANG_ANNOUNCE( RUKOI8 );

HB_CALL_ON_STARTUP_BEGIN( hb_lang_Init_RUKOI8 )
   hb_langRegister( &s_lang );
HB_CALL_ON_STARTUP_END( hb_lang_Init_RUKOI8 )

#if defined(HB_PRAGMA_STARTUP)                                         
   #pragma startup hb_lang_Init_RUKOI8                                     
#elif defined(HB_MSC_STARTUP)                                          
   #pragma data_seg( HB_MSC_START_SEGMENT )
   static HB_$INITSYM hb_vm_auto_hb_lang_Init_RUKOI8 = hb_lang_Init_RUKOI8;    
   #pragma data_seg()
#endif                                                                 

