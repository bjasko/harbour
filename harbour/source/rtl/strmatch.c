/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * String matching functions
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
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

#include <ctype.h>

#include "hbapi.h"

static BOOL hb_strMatchDOS( const char * pszString, const char * pszMask )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_strMatchDOS(%s, %s)", pszString, pszMask));

   while( *pszMask != '\0' && *pszString != '\0' )
   {
      if( *pszMask == '*' )
      {
         while( *pszMask == '*' )
            pszMask++;

         if( *pszMask == '\0' )
            return TRUE;
         else if( *pszMask == '?' )
            pszString++;
         else
         {
            while( toupper( *pszString ) != toupper( *pszMask ) )
            {
               if( *( ++pszString ) == '\0' )
                  return FALSE;
            }
            while( toupper( *pszString ) == toupper( *pszMask ) )
            {
               if( *( ++pszString ) == '\0' )
                  break;
            }
            pszMask++;
         }
      }
      else if( toupper( *pszMask ) != toupper( *pszString ) && *pszMask != '?' )
         return FALSE;
      else
      {
         pszMask++;
         pszString++;
      }
   }

   return ! ( ( *pszMask != '\0' && *pszString == '\0' && *pszMask != '*') ||
              ( *pszMask == '\0' && *pszString != '\0' ) );
}

#define HB_MAX_WILDPATTERN     256

BOOL HB_EXPORT hb_strMatchWild( const char *szString, const char *szPattern )
{
   BOOL fMatch = TRUE, fAny = FALSE;
   ULONG ulAnyPosP[HB_MAX_WILDPATTERN], ulAnyPosV[HB_MAX_WILDPATTERN],
         ulSize, ulLen, ulAny, i, j;

   i = j = ulAny = 0;
   ulLen = strlen( szString );
   ulSize = strlen( szPattern );
   while ( i < ulSize )
   {
      if ( szPattern[i] == '*' )
      {
         fAny = TRUE;
         i++;
      }
      else if ( j < ulLen && ( szPattern[i] == '?' || szPattern[i] == szString[j] ) )
      {
         if ( fAny )
         {
            if ( ulAny < HB_MAX_WILDPATTERN )
            {
               ulAnyPosP[ulAny] = i;
               ulAnyPosV[ulAny] = j;
               ulAny++;
            }
            fAny = FALSE;
         }
         j++;
         i++;
      }
      else if ( fAny && j < ulLen )
      {
         j++;
      }
      else if ( ulAny > 0 )
      {
         ulAny--;
         i = ulAnyPosP[ulAny];
         j = ulAnyPosV[ulAny] + 1;
         fAny = TRUE;
      }
      else
      {
         fMatch = FALSE;
         break;
      }
   }
   return fMatch;
}


/* TODO: Replace it with a code that supports real regular expressions
 *
 */
BOOL hb_strMatchRegExp( const char * szString, const char * szMask )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_strMatchRegExp(%s, %s)", szString, szMask));

   return hb_strMatchDOS( szString, szMask );
}


/*
 * WildMatch( cPattern, cValue ) compares cValue ith cPattern, cPattern
 * may contain wildcard characters (?*)
 */
HB_FUNC( WILDMATCH )
{
   hb_retl( ( ! ISCHAR( 1 ) || ! ISCHAR( 2 ) ) ? FALSE :
            hb_strMatchWild( hb_parc( 2 ), hb_parc( 1 ) ) );
}
