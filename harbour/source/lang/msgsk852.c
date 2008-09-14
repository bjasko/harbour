/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Language Support Module (SK852) SK852
 *
 * Copyright 2008 Gyula Bartal <bartal@tauris-danubius.sk> (from msgskwin.c)
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

/* Language name: Slovak */
/* ISO language code (2 chars): SK */
/* Codepage: 852 */

#include "hbapilng.h"

static HB_LANG s_lang =
{
   {
      /* Identification */

      "SK852",                     /* ID */
      "Slovak",                    /* Name (in English) */
      "Slovensky",                 /* Name (in native language) */
      "SK",                        /* RFC ID */
      "852",                       /* Codepage */
      "",                          /* Version */

      /* Month names */

      "janu�r",
      "febru�r",
      "marec",
      "apr�l",
      "m�j",
      "j�n",
      "j�l",
      "august",
      "september",
      "okt�ber",
      "november",
      "december",

      /* Day names */

      "nede�a",
      "pondelok",
      "utorok",
      "streda",
      "�tvrtok",
      "piatok",
      "sobota",

      /* CA-Cl*pper compatible natmsg items */

      "D�tab�ze          #  Vety      Aktualiz�cia Ve�kos�",
      "Chcete viac pr�kladov?",
      "Strana",
      "** Medzis��et **",
      "* Medzimedzis��et *",
      "*** S��et ***",
      "Ins",
      "   ",
      "Chybn� d�tum",
      "Rozsah: ",
      " - ",
      "A/N",
      "CHYBN� V�RAZ",

      /* Error description names */

      "Nezn�m� chyba",
      "Chyba argumentu",
      "Chyba medz�",
      "Preplnenie re�azca",
      "Preplnenie ��sla",
      "Delenie nulou",
      "Numerick� chyba",
      "Chyba syntaxe",
      "Oper�cia pr�li� komplexn�",
      "",
      "",
      "Nedostatok pam�te",
      "Nedefinovan� funkcia",
      "Nezn�ma met�da",
      "Premenn� neexistuje",
      "Oblas� neexistuje",
      "Nezn�ma premenn�",
      "Nepovolen� znaky v oblasti",
      "Oblas� u� pou�it�",
      "",
      "Chyba vytvorenia",
      "Chyba otvorenia",
      "Chyba zatvorenia",
      "Chyba ��tania",
      "Chyba z�pisu",
      "Chyba tla�e",
      "",
      "",
      "",
      "",
      "Nepodporov�na oper�cia",
      "Prekro�en� limit",
      "Index po�koden�",
      "Chyba typu d�t",
      "Chyba d��ky d�t",
      "Nepou�it� pracovn� oblas� ",
      "Nezoraden� pracovn� oblas�",
      "Nutn� v�hradn� pr�stup",
      "Uzamknutie nutn�",
      "Zlyhanie z�mka pri prid�v�nie",
      "Zlyhanie z�mka",
      "",
      "",
      "",
      "",
      "pr�stup k polom",
      "priradenie k pole",
      "zmena dimenze pole",
      "nie je polom",
      "podmienka",

       /* Internal error names */

      "Unrecoverable error %lu: ",
      "Error recovery failure",
      "No ERRORBLOCK() for error",
      "Too many recursive error handler calls",
      "RDD invalid or failed to load",
      "Invalid method type from %s",
      "hb_xgrab can't allocate memory",
      "hb_xrealloc called with a NULL pointer",
      "hb_xrealloc called with an invalid pointer",
      "hb_xrealloc can't reallocate memory",
      "hb_xfree called with an invalid pointer",
      "hb_xfree called with a NULL pointer",
      "Can\'t locate the starting procedure: \'%s\'",
      "No starting procedure",
      "Unsupported VM opcode",
      "Symbol item expected from %s",
      "Invalid symbol type for self from %s",
      "Codeblock expected from %s",
      "Incorrect item type on the stack trying to pop from %s",
      "Stack underflow",
      "An item was going to be copied to itself from %s",
      "Invalid symbol item passed as memvar %s",
      "Memory buffer overflow",
      "hb_xgrab requested to allocate zero bytes",
      "hb_xrealloc requested to resize to zero bytes",
      "hb_xalloc requested to allocate zero bytes",

      /* Texts */

      "DD.MM.YYYY",
      "A",
      "N"
   }
};

HB_LANG_ANNOUNCE( SK852 );

HB_CALL_ON_STARTUP_BEGIN( hb_lang_Init_SK852 )
   hb_langRegister( &s_lang );
HB_CALL_ON_STARTUP_END( hb_lang_Init_SK852 )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup hb_lang_Init_SK852
#elif defined( HB_MSC_STARTUP )
   #if defined( HB_OS_WIN_64 )
      #pragma section( HB_MSC_START_SEGMENT, long, read )
   #endif
   #pragma data_seg( HB_MSC_START_SEGMENT )
   static HB_$INITSYM hb_vm_auto_hb_lang_Init_SK852 = hb_lang_Init_SK852;
   #pragma data_seg()
#endif
