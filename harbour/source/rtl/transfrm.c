/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * TRANSFORM() function
 *
 * Copyright 1999 Eddie Runia <eddie@runia.com>
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

/*
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 * Copyright 1999 Matthew Hamilton <mhamilton@bunge.com.au>
 *    String handling
 *
 * See doc/license.txt for licensing terms.
 *
 */

#include <ctype.h>

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbdate.h"
#include "hbset.h"
#include "hbapicdp.h"

/* Picture function flags */
#define PF_LEFT       0x0001   /* @B */
#define PF_CREDIT     0x0002   /* @C */
#define PF_DEBIT      0x0004   /* @X */
#define PF_PADL       0x0008   /* @L */ /* NOTE: This is a FoxPro/XPP extension [vszakats] */
#define PF_PARNEG     0x0010   /* @( */
#define PF_REMAIN     0x0020   /* @R */
#define PF_UPPER      0x0040   /* @! */
#define PF_DATE       0x0080   /* @D */
#define PF_BRITISH    0x0100   /* @E */
#define PF_EXCHANG    0x0100   /* @E. Also means exchange . and , */
#define PF_EMPTY      0x0200   /* @Z */
#define PF_WIDTH      0x0400   /* @S */
#define PF_PARNEGWOS  0x0800   /* @) Similar to PF_PARNEG but without leading spaces */

HB_FUNC( TRANSFORM )
{
   PHB_ITEM pValue = hb_param( 1, HB_IT_ANY ); /* Input parameter */
   PHB_ITEM pPic = hb_param( 2, HB_IT_STRING ); /* Picture string */

   BOOL bError = FALSE;

   if( pPic && hb_itemGetCLen( pPic ) > 0 )
   {
      char szPicDate[ 11 ];
      char * szPic = hb_itemGetCPtr( pPic );
      ULONG  ulPicLen = hb_itemGetCLen( pPic );
      USHORT uiPicFlags; /* Function flags */

      ULONG  ulParamS = 0; /* To avoid GCC -O2 warning */
      BYTE   byParamL = '\0'; /* To avoid GCC -O2 warning */

      char * szResult;
      ULONG  ulResultPos;

      ULONG  ulOffset = 0;

      /* ======================================================= */
      /* Analyze picture functions                               */
      /* ======================================================= */

      uiPicFlags = 0;

      /* If an "@" char is at the first pos, we have picture function */

      if( *szPic == '@' )
      {
         BOOL bDone = FALSE;

         /* Skip the "@" char */

         szPic++;
         ulPicLen--;

         /* Go through all function chars, until the end of the picture string
            or any whitespace found. */

         while( ulPicLen && ! bDone )
         {
            switch( hb_charUpper( *szPic ) )
            {
               case HB_CHAR_HT:
               case ' ':
                  bDone = TRUE;     /* End of function string */
                  break;
               case '!':
                  uiPicFlags |= PF_UPPER;
                  break;
               case '(':
                  uiPicFlags |= PF_PARNEG;
                  break;
               case ')':
                  uiPicFlags |= PF_PARNEGWOS;
                  break;
               case 'L':
               case '0':
                  uiPicFlags |= PF_PADL;  /* FoxPro/XPP extension */
                  byParamL = '0';
                  break;
               case 'B':
                  uiPicFlags |= PF_LEFT;
                  break;
               case 'C':
                  uiPicFlags |= PF_CREDIT;
                  break;
               case 'D':
                  uiPicFlags |= PF_DATE;
                  break;
               case 'E':
                  uiPicFlags |= PF_BRITISH;
                  break;
               case 'R':
                  uiPicFlags |= PF_REMAIN;
                  break;
               case 'S':
                  uiPicFlags |= PF_WIDTH;
                  ulParamS = 0;
                  while( ulPicLen > 1 && *( szPic + 1 ) >= '0' && *( szPic + 1 ) <= '9' )
                  {
                     szPic++;
                     ulPicLen--;
                     ulParamS = ( ulParamS * 10 ) + ( ( ULONG ) ( *szPic - '0' ) );
                  }
                  break;
               case 'X':
                  uiPicFlags |= PF_DEBIT;
                  break;
               case 'Z':
                  uiPicFlags |= PF_EMPTY;
                  break;
            }

            szPic++;
            ulPicLen--;
         }
      }

      /* ======================================================= */
      /* Handle STRING values                                    */
      /* ======================================================= */

      if( HB_IS_STRING( pValue ) )
      {
         char * szExp = hb_itemGetCPtr( pValue );
         ULONG  ulExpLen = hb_itemGetCLen( pValue );
         ULONG  ulExpPos = 0;
         BOOL bAnyPic = FALSE;
         BOOL bFound  = FALSE;

         /* Grab enough */

         /* Support date function for strings */
         if( uiPicFlags & ( PF_DATE | PF_BRITISH ) )
         {
            hb_dateFormat( "XXXXXXXX", szPicDate, hb_set.HB_SET_DATEFORMAT );
            szPic = szPicDate;
            ulPicLen = strlen( szPicDate );
         }

         szResult = ( char * ) hb_xgrab( ulExpLen + ulPicLen + 1 );
         ulResultPos = 0;

         /* Template string */
         if( ulPicLen )
         {
            while( ulPicLen )                        /* Analyze picture mask */
            {
               if( ulExpPos < ulExpLen )
               {
                  switch( *szPic )
                  {
                     /* Upper */
                     case '!':
                     {
                        szResult[ ulResultPos++ ] = hb_charUpper( szExp[ ulExpPos ] );
                        ulExpPos++;
                        bAnyPic = TRUE;
                        break;
                     }

                     /* Out the character */
                     case '#':
                     case '9':
                     case 'a':
                     case 'A':
                     case 'l':
                     case 'L':
                     case 'n':
                     case 'N':
                     case 'x':
                     case 'X':
                     {
                        szResult[ ulResultPos++ ] = ( uiPicFlags & PF_UPPER ) ? hb_charUpper( szExp[ ulExpPos ] ) : szExp[ ulExpPos ];
                        ulExpPos++;
                        bAnyPic = TRUE;
                        break;
                     }

                     /* Logical */
                     case 'y':
                     case 'Y':
                     {
                        szResult[ ulResultPos++ ] = ( szExp[ ulExpPos ] == 't' ||
                                                      szExp[ ulExpPos ] == 'T' ||
                                                      szExp[ ulExpPos ] == 'y' ||
                                                      szExp[ ulExpPos ] == 'Y' ) ? ( char ) 'Y' : ( char ) 'N';
                        ulExpPos++;
                        bAnyPic = TRUE;
                        break;
                     }

                     /* Other choices */
                     default:
                     {
                        szResult[ ulResultPos++ ] = *szPic;

                        if( !( uiPicFlags & PF_REMAIN ) )
                           ulExpPos++;
                     }
                  }
               }
               else if( !( uiPicFlags & PF_REMAIN ) )
                  break;

               else
               {
                  switch( *szPic )
                  {
                     case '!':
                     case '#':
                     case '9':
                     case 'a':
                     case 'A':
                     case 'l':
                     case 'L':
                     case 'n':
                     case 'N':
                     case 'x':
                     case 'X':
                     case 'y':
                     case 'Y':
                     {
                        szResult[ ulResultPos++ ] = ' ';
                        break;
                     }

                     default:
                        szResult[ ulResultPos++ ] = *szPic;
                  }
               }

               szPic++;
               ulPicLen--;
            }
         }
         else
         {
            while( ulExpPos++ < ulExpLen )
            {
               if( uiPicFlags & PF_EXCHANG )
               {
                  switch( *szExp )
                  {
                     case ',':
                     {
                        szResult[ ulResultPos++ ] = '.';
                        break;
                     }
                     case '.':
                     {
                        if( !bFound && ulResultPos )
                        {
                           szResult[ ulResultPos++ ] = ',';
                           bFound = TRUE;
                        }

                        break;
                     }
                     default:
                        szResult[ ulResultPos++ ] = ( uiPicFlags & PF_UPPER ) ? hb_charUpper( *szExp ) : *szExp;
                  }
               }
               else
               {
                  szResult[ ulResultPos++ ] = ( uiPicFlags & PF_UPPER ) ? hb_charUpper( *szExp ) : *szExp;
               }
               szExp++;
            }
         }

         if( ( uiPicFlags & PF_REMAIN ) && ! bAnyPic )
         {
            while( ulExpPos++ < ulExpLen )
            {
               szResult[ ulResultPos++ ] = ( uiPicFlags & PF_UPPER ) ? hb_charUpper( *szExp ) : *szExp;
               szExp++;
            }
         }

         /* Any chars left ? */
         if( ( uiPicFlags & PF_REMAIN ) && ulPicLen )
         {
            /* Export remainder */
            while( ulPicLen-- )
               szResult[ ulResultPos++ ] = ' ';
         }

         if( uiPicFlags & PF_BRITISH )
         {
            /* CA-Cl*pper do not check result size and always exchanges
             * bytes 1-2 with bytes 4-5. It's buffer overflow bug and I do
             * not want to replicate it. It also causes that the results of
             * @E conversion used for strings smaller then 5 bytes behaves
             * randomly.
             * In fact precise tests can show that it's not random behavior
             * but CA-Cl*pper uses static buffer for result and when current
             * one is smaller then 5 bytes then first two bytes are exchanged
             * with 4-5 bytes from previous result which was length enough,
             * f.e.:
             *          ? transform( "0123456789", "" )
             *          ? transform( "AB", "@E" )
             *          ? transform( "ab", "@E" )
             * [druzus]
             */
            if( ulResultPos >= 5 )
            {
               szPicDate[ 0 ] = szResult[ 0 ];
               szPicDate[ 1 ] = szResult[ 1 ];
               szResult[ 0 ] = szResult[ 3 ];
               szResult[ 1 ] = szResult[ 4 ];
               szResult[ 3 ] = szPicDate[ 0 ];
               szResult[ 4 ] = szPicDate[ 1 ];
            }
         }
      }

      /* ======================================================= */
      /* Handle NUMERIC values                                   */
      /* ======================================================= */

      else if( HB_IS_NUMERIC( pValue ) )
      {
         int      iWidth;                             /* Width of string          */
         int      iDec;                               /* Number of decimals       */
         int      iCount;
         ULONG    i;
         char     cPic;
         PHB_ITEM pNumber = NULL;

         double dValue = hb_itemGetND( pValue );

         /* Support date function for numbers */
         if( uiPicFlags & PF_DATE )
         {
            hb_dateFormat( "99999999", szPicDate, hb_set.HB_SET_DATEFORMAT );
            szPic = szPicDate;
            ulPicLen = strlen( szPicDate );
         }

         for( i = iWidth = iDec = 0; i < ulPicLen; i++ )
         {
            if( szPic[ i ] == '.' )
            {
               while( ++i < ulPicLen )
               {
                  if( szPic[ i ] == '9' || szPic[ i ] == '#' ||
                      szPic[ i ] == '$' || szPic[ i ] == '*' )
                  {
                     iWidth++;
                     iDec++;
                  }
               }
               if( iDec )
                  iWidth++;
               break;
            }
            else if( szPic[ i ] == '9' || szPic[ i ] == '#' ||
                     szPic[ i ] == '$' || szPic[ i ] == '*' )
               iWidth++;
         }

         iCount = 0;
         if( iWidth == 0 )                             /* Width calculated ??      */
         {
            hb_itemGetNLen( pValue, &iWidth, &iDec );
            if( hb_set.HB_SET_FIXED )
            {
               if( HB_IS_NUMINT( pValue ) )
                  iWidth += 2 + ( hb_set.HB_SET_DECIMALS << 1 );
               else
                  iDec = hb_set.HB_SET_DECIMALS;
            }
            if( iDec )
               iWidth += iDec + 1;
         }
         else if( iDec > 0 && iWidth - iDec == 1 )
         {
            iCount = 1;
            iWidth++;
         }

         if( ( uiPicFlags & ( PF_DEBIT | PF_PARNEG | PF_PARNEGWOS ) ) && dValue < 0 )
         {
            /* Always convert absolute val */
            if( HB_IS_NUMINT( pValue ) ) /* workaround for 64bit integer conversion */
               pNumber = hb_itemPutNInt( NULL, - hb_itemGetNInt( pValue ) );
            else
               pNumber = hb_itemPutND( NULL, -dValue );
            pValue = pNumber;
         }

         if( dValue != 0 )
            /* Don't empty the result if the number is not zero */
            uiPicFlags &= ~PF_EMPTY;

         /* allocate 4 additional bytes for possible ") CR" or ") DB" suffix */
         szResult = ( char * ) hb_xgrab( iWidth + 5 );
         hb_itemStrBuf( szResult, pValue, iWidth, iDec );
         if( pNumber )
            hb_itemRelease( pNumber );

         if( iCount )
         {
            iWidth--;
            if( *szResult != '0' )
            {
               memset( szResult + 1, '*', iWidth );
               *szResult = '.';
            }
            else
               memmove( szResult, szResult + 1, iWidth );
            szResult[ iWidth ] = '\0';
         }

         /* Pad with padding char */
         if( uiPicFlags & PF_PADL )
         {
            for( i = 0; szResult[ i ] == ' '; i++ )
               szResult[ i ] = byParamL;

            /* please test it with FoxPro and xbase++ to check
             * if they made the same [druzus]
             */
            if( i && szResult[ i ] == '-' )
            {
               szResult[ 0 ] = '-';
               szResult[ i ] = byParamL;
            }
         }

         if( ulPicLen == 0 )
         {
            if( uiPicFlags & PF_EXCHANG )
            {
               for( i = 0; i < ( ULONG ) iWidth; ++i )
               {
                  if( szResult[ i ] == '.' )
                  {
                     szResult[ i ] = ',';
                     break;
                  }
               }
            }
            i = iWidth;
         }
         else
         {
            char * szStr = szResult;

            /* allocate 4 additional bytes for possible ") CR" or ") DB" suffix */
            szResult = ( char * ) hb_xgrab( ulPicLen + 5 );

            for( i = iCount = 0; i < ulPicLen; i++ )
            {
               cPic = szPic[ i ];
               if( cPic == '9' || cPic == '#' )
               {
                  szResult[ i ] = iCount < iWidth ? szStr[ iCount++ ] : ' ';
               }
               else if( cPic == '$' || cPic == '*' )
               {
                  if( iCount < iWidth )
                  {
                     szResult[ i ] = szStr[ iCount ] == ' ' ? cPic : szStr[ iCount ];
                     iCount++;
                  }
                  else
                     szResult[ i ] = ' ';
               }
               else if( cPic == '.' && iCount < iWidth )
               {
                  szResult[ i ] = ( uiPicFlags & PF_EXCHANG ) ? ',' : '.';
                  iCount++;
               }
               else if( cPic == ',' && i && iCount < iWidth )
               {
                  if( isdigit( ( UCHAR ) szResult[ i - 1 ] ) )
                     szResult[ i ] = ( uiPicFlags & PF_EXCHANG ) ? '.' : ',';
                  else
                  {
                     szResult[ i ] = szResult[ i - 1 ];
                     if( szResult[ i - 1 ] == '-' )
                     {
                        szResult[ i - 1 ] = i > 1 && szResult[ i - 2 ] != '$' ?
                                            szResult[ i - 2 ] : ' ';
                     }
                  }
               }
               else
                  szResult[ i ] = cPic;
            }
            hb_xfree( szStr );
         }

         if( dValue < 0 )
         {
            /* PF_PARNEGWOS has higher priority then PF_PARNEG */
            if( ( uiPicFlags & PF_PARNEGWOS ) )
            {
               iCount = 0;
               if( ulPicLen && i > 1 )
               {
                  if( *szPic == *szResult && ( *szPic == '*' || *szPic == '$' ) &&
                      szResult[ 1 ] == ' ' )
                     ++iCount;
               }
               while( ( ULONG ) iCount + 1 < i && szResult[ iCount + 1 ] == ' ' )
                  ++iCount;

#ifndef HB_C52_STRICT
               /* This is not Clipper compatible */
               if( szResult[ iCount ] >= '1' && szResult[ iCount ] <= '9' &&
                   ( ulPicLen == 0 || szPic[ iCount ] == '9' ||
                     szPic[ iCount ] != szResult[ iCount ] ) )
               {
                  szResult[ iCount ] = '(';
                  for( ++iCount; ( ULONG ) iCount < i; iCount++ )
                  {
                     if( szResult[ iCount ] >= '0' && szResult[ iCount ] <= '9' &&
                         ( ulPicLen == 0 || szPic[ iCount ] == '9' ||
                           szPic[ iCount ] != szResult[ iCount ] ) )
                        szResult[ iCount ] = '*';
                  }
               }
               else
#endif
                  szResult[ iCount ] = '(';
               szResult[ i++ ] = ')';
            }
            else if( ( uiPicFlags & PF_PARNEG ) )
            {
#ifndef HB_C52_STRICT
               /* This is not Clipper compatible */
               if( *szResult >= '1' && *szResult <= '9' &&
                   ( ulPicLen == 0 || *szPic == '9' || *szPic != *szResult ) )
               {
                  for( iCount = 1; ( ULONG ) iCount < i; iCount++ )
                  {
                     if( szResult[ iCount ] >= '0' && szResult[ iCount ] <= '9' &&
                         ( ulPicLen == 0 || szPic[ iCount ] == '9' ||
                           szPic[ iCount ] != szResult[ iCount ] ) )
                        szResult[ iCount ] = '*';
                  }
               }
#endif
               *szResult       = '(';
               szResult[ i++ ] = ')';
               ulOffset = 1;
            }

            if( ( uiPicFlags & PF_DEBIT ) )
            {
               szResult[ i++ ] = ' ';
               szResult[ i++ ] = 'D';
               szResult[ i++ ] = 'B';
            }
         }
         else if( ( uiPicFlags & PF_CREDIT ) && dValue > 0 )
         {
            szResult[ i++ ] = ' ';
            szResult[ i++ ] = 'C';
            szResult[ i++ ] = 'R';
         }

         ulResultPos = i;
         szResult[ i ] = '\0';
      }

      /* ======================================================= */
      /* Handle DATE values                                      */
      /* ======================================================= */

      else if( HB_IS_DATE( pValue ) )
      {
         char szDate[ 9 ];
         char * szDateFormat;
         char szNewFormat[ 11 ];
         UINT nFor;

         szResult = ( char * ) hb_xgrab( 13 );
         szDateFormat = hb_set.HB_SET_DATEFORMAT;

#ifndef HB_C52_STRICT
         if( uiPicFlags & PF_BRITISH )
         {
            /* When @E is used CA-Cl*pper do not update date format
             * pattern but wrongly moves 4-th and 5-th bytes of
             * formatted date to the beginning (see below). It causes
             * that date formats formats different then MM?DD?YY[YY]
             * are wrongly translated. The code below is not CA-Cl*pper
             * compatible but it tries to respect user date format
             * [druzus]
             */
            const char * szBritish = hb_set.hb_set_century ?
                                     "DDMMYYYY" : "DDMMYY";
            char cLast = 'x';

            for( nFor = 0; nFor < 10; nFor++ )
            {
               if( *szBritish == cLast )
               {
                  szNewFormat[ nFor ] = cLast;
                  szBritish++;
               }
               else if( ! *szDateFormat )
                  break;
               else if( *szBritish &&
                        ( *szDateFormat == 'Y' || *szDateFormat == 'y' ||
                          *szDateFormat == 'D' || *szDateFormat == 'd' ||
                          *szDateFormat == 'M' || *szDateFormat == 'm' ) )
               {
                  szNewFormat[ nFor ] = cLast = *szBritish++;
                  do
                     szDateFormat++;
                  while( szDateFormat[ -1 ] == szDateFormat[ 0 ] );
               }
               else
                  szNewFormat[ nFor ] = *szDateFormat++;
            }
            szNewFormat[ nFor ] = '\0';
            szDateFormat = szNewFormat;
         }
#endif

         hb_dateFormat( hb_itemGetDS( pValue, szDate ), szResult, szDateFormat );
         ulResultPos = strlen( szResult );

#ifdef HB_C52_STRICT
         if( uiPicFlags & PF_BRITISH )
         {
            /* replicated wrong Clipper behavior, see note above.
             * It's not exact CA-Cl*pper behavior because it does
             * not check for size of results and can extract data
             * from static memory buffer used in previous conversions
             * (see my note for @E in string conversion above)
             * but this is buffer overflow and I do not plan to
             * replicated it too [druzus]
             */
            if( ulResultPos >= 5 )
            {
               szNewFormat[ 0 ] = szResult[ 0 ];
               szNewFormat[ 1 ] = szResult[ 1 ];
               szResult[ 0 ] = szResult[ 3 ];
               szResult[ 1 ] = szResult[ 4 ];
               szResult[ 3 ] = szNewFormat[ 0 ];
               szResult[ 4 ] = szNewFormat[ 1 ];
            }
         }
#endif
         if( uiPicFlags & PF_REMAIN )
         {
            /* Here we also respect the date format modified for @E [druzus]
             */
            hb_dateFormat( "99999999", szPicDate, szDateFormat );
            ulPicLen = strlen( szPicDate );

            for( nFor = 0; nFor < ulPicLen; nFor++ )
            {
               if( szPicDate[ nFor ] != '9' )
               {
                  memmove( szResult + nFor + 1, szResult + nFor, 12 - nFor );
                  szResult[ nFor ] = szPicDate[ nFor ];
                  ulResultPos++;
               }
            }
         }
      }

      /* ======================================================= */
      /* Handle LOGICAL values                                   */
      /* ======================================================= */

      else if( HB_IS_LOGICAL( pValue ) )
      {
         BOOL bDone = FALSE;
         BOOL bExit = FALSE;
         char cPic;

         if( uiPicFlags & ( PF_DATE | PF_BRITISH ) )
         {
            hb_dateFormat( "99999999", szPicDate, hb_set.HB_SET_DATEFORMAT );
            szPic = szPicDate;
            ulPicLen = strlen( szPicDate );
         }

         ulResultPos = 0;
         szResult = ( char * ) hb_xgrab( ulPicLen + 2 );

         for( ; ( ulPicLen || !bDone ) && !bExit ; ulResultPos++, szPic++, ulPicLen-- )
         {
            if( ulPicLen )
               cPic = *szPic;
            else
            {
               cPic  = 'L';
               bExit = TRUE;
            }

            switch( cPic )
            {
               case 'y':                     /* Yes/No                   */
               case 'Y':                     /* Yes/No                   */
               {
                  if( !bDone )
                  {
                     szResult[ ulResultPos ] = hb_itemGetL( pValue ) ? 'Y' : 'N';
                     bDone = TRUE;           /* Logical written          */
                  }
                  else
                     szResult[ ulResultPos ] = ' ';

                  break;
               }

               case '#':
               case 'l':                     /* True/False               */
               case 'L':                     /* True/False               */
               {
                  if( !bDone )
                  {
                     szResult[ ulResultPos ] = hb_itemGetL( pValue ) ? 'T' : 'F';
                     bDone = TRUE;
                  }
                  else
                     szResult[ ulResultPos ] = ' ';

                  break;
               }

               default:
                  szResult[ ulResultPos ] = cPic;
            }

            if( !( uiPicFlags & PF_REMAIN ) )
               bExit = TRUE;
         }
      }

      /* ======================================================= */

      else
      {
         szResult = NULL; /* To avoid GCC -O2 warning */
         ulResultPos = 0; /* To avoid GCC -O2 warning */
         bError = TRUE;
      }

      if( ! bError )
      {
         if( uiPicFlags & PF_EMPTY )
            memset( szResult, ' ', ulResultPos );
         else if( uiPicFlags & PF_LEFT )
         {
            /* Trim left and pad with spaces */
            ULONG ulFirstChar = ulOffset;

            while( ulFirstChar < ulResultPos && szResult[ ulFirstChar ] == ' ' )
               ulFirstChar++;

            if( ulFirstChar > ulOffset && ulFirstChar < ulResultPos )
            {
               memmove( szResult + ulOffset, szResult + ulFirstChar, ulResultPos - ulFirstChar );
               memset( szResult + ulOffset + ulResultPos - ulFirstChar, ' ', ulFirstChar - ulOffset );
            }
         }

         hb_retclen_buffer( szResult, ( ulParamS && ulResultPos > ulParamS ) ? ulParamS : ulResultPos );
      }
   }
   else if( pPic || ISNIL( 2 ) ) /* Picture is an empty string or NIL */
   {
      if( HB_IS_STRING( pValue ) )
      {
         hb_itemReturn( pValue );
      }
      else if( HB_IS_NUMERIC( pValue ) )
      {
         char * szStr;

         if( HB_IS_NUMINT( pValue ) && hb_set.HB_SET_FIXED )
         {
            int iWidth, iDec;
            hb_itemGetNLen( pValue, &iWidth, &iDec );
            iWidth += 2 + ( hb_set.HB_SET_DECIMALS << 1 );
            szStr = ( char * ) hb_xgrab( iWidth + 1 );
            hb_itemStrBuf( szStr, pValue, iWidth, iDec );
            hb_retclen_buffer( szStr, iWidth );
         }
         else
         {
            ULONG ulLen;
            BOOL bFreeReq;

            szStr = hb_itemString( pValue, &ulLen, &bFreeReq );
            if( bFreeReq )
               hb_retclen_buffer( szStr, ulLen );
            else
               hb_retclen( szStr, ulLen );
         }
      }
      else if( HB_IS_DATE( pValue ) )
      {
         char szDate[ 9 ];
         char szResult[ 11 ];

         hb_retc( hb_dateFormat( hb_itemGetDS( pValue, szDate ), szResult, hb_set.HB_SET_DATEFORMAT ) );
      }
      else if( HB_IS_LOGICAL( pValue ) )
      {
         hb_retc( hb_itemGetL( pValue ) ? "T" : "F" );
      }
      else
         bError = TRUE;
   }
   else
      bError = TRUE;

   /* If there was any parameter error, launch a runtime error */

   if( bError )
      hb_errRT_BASE_SubstR( EG_ARG, 1122, NULL, "TRANSFORM", HB_ERR_ARGS_BASEPARAMS );
}
