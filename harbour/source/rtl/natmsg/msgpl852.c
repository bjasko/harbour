/*
 * $Id$
 */

/* Polish language module - Polskoj�zyczny modu� dla Harbour  */
/* Codepage: Latin II - 852 */

#include "hbdefs.h"

char *hb_monthsname[ 12 ] = {
   "Stycze�", "Luty", "Marzec",
   "Kwiecie�", "Maj", "Czerwiec", "Lipiec",
   "Sierpie�", "Wrzesie�", "Pa�dziernik",
   "Listopad", "Grudzie�" };

char *hb_daysname[ 7 ] = {
   "Niedziela", "Poniedzia�ek", "Wtorek",
   "�roda", "Czwartek", "Pi�tek",
   "Sobota" };

static char *genericErrors[] =
{
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
   "Nieprwid�owa liczba argument�w",
   "pobranie elementu tablicy",
   "zmiana warto�ci elementu tablicy",
   "wymagana jest tablica",
   "wymagany typ: logiczny"
};

char *hb_ErrorNatDescription( ULONG ulGenError )
{
   if( ulGenError < sizeof(genericErrors)/sizeof(char*) )
      return genericErrors[ ulGenError ];
   else
      return genericErrors[ 0 ];
}

