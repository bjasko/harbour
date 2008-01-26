/*
 * $Id$
 */

/*
 * xHarbour Project source code:
 * Functions to create session id and some utils
 *
 * Copyright 2008 Lorenzo Fiorini <lorenzo.fiorini@gmail.com>
 *
 * code from:
 * TIP Class oriented Internet protocol library
 *
 * Copyright 2003 Giancarlo Niccolai <gian@niccolai.ws>
 *
 * www - http://www.harbour-project.org
 *
 *    CGI Session Manager Class
 *
 * Copyright 2003-2006 Francesco Saverio Giudice <info / at / fsgiudice / dot / com>
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

#include "tip.ch"
#include "common.ch"

#define SID_LENGTH      25
#define BASE_KEY_STRING "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define CRC_KEY_STRING  "Ak3yStR1Ng"  // Max Length must be 10 chars

FUNCTION TIP_GENERATESID( cCRCKey )

   local cSID, nSIDCRC, cSIDCRC, n, cTemp
   local nLenSID     := SID_LENGTH
   local cBaseKeys   := BASE_KEY_STRING
   local nLenKeys    := Len( cBaseKeys )
   local cRet
   local nRand, nKey := 0

   DEFAULT cCRCKey  TO CRC_KEY_STRING

   cCRCKey := Left( cCRCKey, 10 )      // Max Lenght must to be of 10 chars

   /* Let's generate the sequence */
   cSID := Space( nLenSID )
   for n := 1 TO nLenSID
      nRand     := HB_RandomInt( 1, nLenKeys )
      cSID      := Stuff( cSID, n, 1, SubStr( cBaseKeys, nRand, 1 ) )
      nKey      += nRand
   next

   nSIDCRC := nKey * 51 // Max Value is 99603 a 5 chars number
   cTemp   := StrZero( nSIDCRC, 5 )
   cSIDCRC := ""
   for n := 1 to Len( cTemp )
      cSIDCRC += SubStr( cCRCKey, Val( SubStr( cTemp, n, 1 ) ) + 1, 1 )
   next

   cRet := cSID + cSIDCRC

   RETURN cRet

FUNCTION TIP_CHECKSID( cSID, cCRCKey )

   local nSIDCRC, cSIDCRC, n, cTemp
   local nLenSID     := SID_LENGTH
   local cBaseKeys   := BASE_KEY_STRING
   local nLenKeys    := Len( cBaseKeys )
   local nRand, nKey := 0

   DEFAULT cCRCKey  TO CRC_KEY_STRING

   cCRCKey := Left( cCRCKey, 10 )      // Max Lenght must to be of 10 chars

   /* Calculate the key */
   for n := 1 to nLenSID
      nRand := At( SubStr( cSID, n, 1), cBaseKeys )
      nKey  += nRand
   next

   // Recalculate the CRC
   nSIDCRC := nKey * 51 // Max Value is 99603. a 5 chars number
   cTemp   := StrZero( nSIDCRC, 5 )
   cSIDCRC := ""
   for n := 1 to Len( cTemp )
      cSIDCRC += SubStr( cCRCKey, Val( SubStr( cTemp, n, 1 ) ) + 1, 1 )
   next

   RETURN ( Right( cSID, 5 ) == cSIDCRC )

FUNCTION TIP_DATETOGMT( dDate, cTime )
   LOCAL cStr := ""
   LOCAL cOldDateFormat := Set( _SET_DATEFORMAT, "dd-mm-yy" )
   LOCAL nDay, nMonth, nYear, nDoW
   LOCAL aDays   := { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" }
   LOCAL aMonths := { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }
  
   DEFAULT dDate TO DATE()
   DEFAULT cTime TO TIME()
  
   nDay   := Day( dDate )
   nMonth := Month( dDate )
   nYear  := Year( dDate)
   nDoW   := Dow( dDate )
  
   cStr := aDays[ nDow ] + ", " + StrZero( nDay, 2 ) + "-" + aMonths[ nMonth ] + "-" + ;
           Right( StrZero( nYear, 4 ), 2 ) + " " + cTime + " GMT"
  
   Set( _SET_DATEFORMAT, cOldDateFormat )

   RETURN cStr
