/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Language Support Module (BG866)
 *
 * Copyright 1999-2005 Viktor Szakats <viktor.szakats@syenar.hu>
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

/* Language name: Bulgarian */
/* ISO language code (2 chars): (please look it up in /doc/lang_id.txt) */
/* Codepage: CP-866 */

#include "hbapilng.h"

static HB_LANG s_lang =
{
   {
      /* Identification */

      "BG866",                     /* ID */
      "Bulgarian",                 /* Name (in English) */
      "�ꫣ��᪨",                 /* Name (in native language) */
      "BG",                        /* RFC ID */
      "CP-866",                    /* Codepage */
      "$Revision$ $Date$",         /* Version */

      /* Month names */

      "����",
      "������",
      "����",
      "��ਫ",
      "���",
      "���",
      "���",
      "������",
      "���⥬��",
      "��⮬��",
      "������",
      "�������",

      /* Day names */

      "������",
      "����������",
      "��୨�",
      "��鸞",
      "�������",
      "����",
      "�ꡮ�",

      /* CA-Cl*pper compatible natmsg items */

      "�������          # �����  ��᫥��� �������  ��������",
      "�᪠� �� �� �ਬ��?",
      "���. No.",
      "** ����. �㬠 **",
      "* ����. �㬠 *",
      "*** ��� ***",
      "Ins",
      "   ",
      "����४⭠ ���",
      "��������: ",
      " - ",
      "Y/N",
      "���������� �����",

      /* Error description names */

      "�������� ��誠",
      "��誠 � ��ࠬ����",
      "��誠 � �࠭�筨 �⮩����",
      "�९ꫢ��� �� ��ਭ�",
      "��᫮�� �९ꫢ���",
      "������� �� �㫠",
      "��᫮�� ��誠",
      "���⠪�筠 ��誠",
      "����� ᫮��� ������",
      "",
      "",
      "�����⨣ �� �����",
      "����䨭�࠭� �㭪��",
      "�ﬠ ��ᯮ��࠭ ��⮤",
      "�஬������� �� �����㢠",
      "�ᥢ������ �� �����㢠",
      "�ﬠ ��ᯮ��࠭� �஬������",
      "����������� ᨬ���� � �ᥢ�����",
      "�ᥢ������ ��� � ��������",
      "",
      "��誠 �� �ꧤ�����",
      "��誠 �� �⢠�ﭥ",
      "��誠 �� ��⢠�ﭥ",
      "��誠 �� �⥭�",
      "��誠 �� �����",
      "��誠 �� ����",
      "",
      "",
      "",
      "",
      "������� �� � ����ঠ",
      "�����૥�� ��࠭�祭��",
      "���� ���।�",
      "��誠 � ⨯ �����",
      "��誠 � ࠧ��� �� �����",
      "����⭠� ������ �� � � 㯮�ॡ�",
      "����⭠� ������ �� � ������࠭�",
      "���᪢� � Exclusive",
      "���᪢� � �����碠��",
      "����� �� � ��������",
      "Append lock � �஢���",
      "���ᯥ譮 �����碠��",
      "",
      "",
      "",
      "",
      "����� �� ��ᨢ",
      "��᢮�� �� ��ᨢ",
      "ࠧ��୮�� �� ��ᨢ",
      "�� � ��ᨢ",
      "�᫮���",

      /* Internal error names */

      "�����ࠢ��� ��誠 %lu: ",
      "���ᯥ譮 ���ࠢﭥ �� ��誠",
      "�ﬠ ERRORBLOCK() �� ��誠",
      "�४����� ����� ४��ᨢ�� ���������� �� ��墠�� �� ��誨",
      "RDD ��������� ��� ���ᯥ譮 ��०����",
      "��������� ⨯ ��⮤ �� %s",
      "hb_xgrab �� ���� �� ������ �����",
      "hb_xrealloc �������� � NULL 㪠��⥫",
      "hb_xrealloc �������� � ��������� 㪠��⥫",
      "hb_xrealloc �� ���� �� ������ �����",
      "hb_xfree �������� � ��������� 㪠��⥫",
      "hb_xfree �������� � NULL 㪠��⥫",
      "�� ���� �� ��।��� ��砫��� ��楤��: \'%s\'",
      "�ﬠ ��砫�� ��楤��",
      "������ঠ� VM opcode",
      "�������� ��⨪� �砪��� �� %s",
      "��������� ᨬ����� ⨯ �� self �� %s",
      "Codeblock �砪��� �� %s",
      "����४⥭ ⨯ � �⥪� ���� �� �����砭� �� %s",
      "�ࠧ�� �⥪",
      "���� �� ����࠭� �� ����� � ᥡ� � �� %s",
      "��������� ᨬ����� ����� ����⥭ ��� memvar %s",
      "�९ꫢ��� �� ���� � ������",
      "hb_xgrab ���� �� ������ �㫠 ����",
      "hb_xrealloc ���� �� �஬��� ��������� �� �㫠 ����",
      "hb_xalloc ���� �� ������ �㫠 ����",

      /* Texts */

      "YYYY/MM/DD",
      "Y",
      "N"
   }
};

HB_LANG_ANNOUNCE( BG866 );

HB_CALL_ON_STARTUP_BEGIN( hb_lang_Init_BG866 )
   hb_langRegister( &s_lang );
HB_CALL_ON_STARTUP_END( hb_lang_Init_BG866 )

#if defined(HB_PRAGMA_STARTUP)
   #pragma startup hb_lang_Init_BG866
#elif defined(HB_MSC_STARTUP)
   #pragma data_seg( HB_MSC_START_SEGMENT )
   static HB_$INITSYM hb_vm_auto_hb_lang_Init_BG866 = hb_lang_Init_BG866;
   #pragma data_seg()
#endif
