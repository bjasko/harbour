/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Language Support Module (PL ISO-8859-2)
 *
 * Copyright 1999 {list of individual authors and e-mail addresses}
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version, with one exception:
 *
 * The exception is that if you link the Harbour Runtime Library (HRL)
 * and/or the Harbour Virtual Machine (HVM) with other files to produce
 * an executable, this does not by itself cause the resulting executable
 * to be covered by the GNU General Public License. Your use of that
 * executable is in no way restricted on account of linking the HRL
 * and/or HVM code into it.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA (or visit
 * their web site at http://www.gnu.org/).
 *
 */

/* Language name: Polish */
/* ISO language code (2 chars): PL */
/* Codepage: ISO-8859-2 */

#include "hbapilng.h"

static HB_LANG s_lang =
{
   {
      /* Identification */

      "PLISO",                     /* ID */
      "Polish",                    /* Name (in English) */
      "Polski",                    /* Name (in native language) */
      "PL",                        /* RFC ID */
      "ISO-8859-2",                /* Codepage */
      "$Revision$ $Date$",         /* Version */

      /* Month names */

       "Stycze�",
       "Luty",
       "Marzec",
       "Kwiecie�",
       "Maj",
       "Czerwiec",
       "Lipiec",
       "Sierpie�",
       "Wrzesie�",
       "Pa�dziernik",
       "Listopad",
       "Grudzie�",

      /* Day names */

       "Niedziela",
       "Poniedzia�ek",
       "Wtorek",
       "�roda",
       "Czwartek",
       "Pi�tek",
       "Sobota",

      /* CA-Cl*pper compatible natmsg items */

      "Baza danych       #Rekord�w    Uaktualniona Rozmiar",
      "Wi�cej przyk�ad�w?",
      "Strona",
      "** Subtotal **",
      "* Subsubtotal *",
      "*** Total ***",
      "Wst",    /* wstaw */
      "Zas",    /* zastap */
      "Nieprawid�owa data",
      "Zakres:",
      " - ",
      "T/N",
      "B��dne wyra�enie",

      /* Error description names */

       "B��d bez opisu",
       "Nieprawid�owy argument",
       "B��d zakresu tablicy",
       "Za du�y string",
       "Przepe�nienie numeryczne",
       "Dzielenie przez zero",
       "B��d numeryczny",
       "Nieprawid�owa sk�adnia",
       "Operacja zbyt z�o�ona",
      "",
      "",
       "Za ma�o pami�ci",
       "Niezdefiniowana funkcja",
       "Metoda jest niedost�pna",
       "Zmienna nie istnieje",
       "Alias bazy nie istnieje",
       "Zmienna jest niedost�pna",
       "Nieprawid�owy alias bazy",
       "Podany alias ju� istnieje",
      "",
       "B��d podczas tworzenia zbioru",
       "B��d podczas otwarcia zbioru",
       "B��d podczas zamkni�cia zbioru",
       "B��d podczas odczytu ze zbioru",
       "B��d podczas zapisu do zbioru",
       "B��d wydruku",
      "",
      "",
      "",
      "",
       "Nieprawid�owa operacja",
       "Przekroczony limit",
       "Uszkodzony indeks bazy",
       "Niezgodny typ danych",
       "Warto�� poza zakresem",
       "Baza jest nie otwarta",
       "Baza nie ma indeksu",
       "Wymagany jest wy��czny dost�p do bazy",
       "Wymagana blokada dost�pu",
       "Zapis niedozwolony",
       "Brak blokady dost�pu podczas dodawania rekordu",
       "Nie uda�o si� zablokowa� dost�pu",
      "",
      "",
      "",
      "",
       "Nieprawid�owa liczba argument�w",
       "pobranie elementu tablicy",
       "zmiana warto�ci elementu tablicy",
       "wymagana jest tablica",
       "wymagany typ: logiczny",

      /* Internal error names */

      "Nienaprawialny b��d nr %lu: ",
      "Nieudana pr�ba naprawy b��du",
      "Brak kodu obs�ugi ERRORBLOCK()",
      "Zbyt wiele zagnie�d�onych b��d�w",
      "Nieza�adowany lub z�y RDD",
      "Z�y typ metody wo�anej z %s",
      "hb_xgrab nie mo�e zarezerwowa� pami�ci",
      "hb_xrealloc wywo�any ze wska�nikiem NULL",
      "hb_xrealloc wywo�any ze z�ym wska�nikiem",
      "hb_xrealloc nie mo�e powiekszy� bloku pami�ci",
      "hb_xfree wywo�any ze z�ym wska�nikiem",
      "hb_xfree wywo�any ze wska�nikiem NULL",
      "Brak definicja procedury startowej: \'%s\'",
      "Brak procedury startowej",
      "Nieprawid�owa warto�� VM opcode",
      "W %s wymagany jest item typu \'Symbol\'",
      "W %s podano z�y item dla SELF",
      "W %s oczekiwany jest item typu \'Codeblock\'",
      "Funkcja %s wymaga innego typu na stosie",
      "Stos poni�ej dna",
      "Item nie mo�e by� skopiowany w %s",
      "W %s podano z�y item jako memvar",
      "Zapis poza przydzielonym obszarem",

      /* Texts */

      "YYYY.MM.DD",
      "T",
      "N"
   }
};

HB_LANG_ANNOUNCE( PLISO );

HB_CALL_ON_STARTUP_BEGIN( hb_lang_Init_PLISO )
   hb_langRegister( &s_lang );
HB_CALL_ON_STARTUP_END( hb_lang_Init_PLISO )
#if ! defined(__GNUC__) && ! defined(_MSC_VER)
   #pragma startup hb_lang_Init_PLISO
#endif

