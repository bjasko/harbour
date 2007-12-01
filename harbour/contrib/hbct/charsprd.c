/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *   CT3 string function:  CHARSPREAD()
 *
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
 *
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

#include "hbapi.h"
#include "hbapiitm.h"

HB_FUNC( CHARSPREAD )
{
   ULONG ulLen = hb_parclen( 1 );

   if( ulLen == 0 )
      hb_retc( NULL );
   else
   {
      long lSize = hb_parnl( 2 );

      if( lSize < 0 || ( ULONG ) lSize <= ulLen )
         hb_itemReturn( hb_param( 1, HB_IT_ANY ) );
      else
      {
         char * szText = hb_parc( 1 ), * szDest, cDelim = ' ';
         int iTokens = 0, iRepl, iRest, iFirst, i;
         ULONG ul, ulDst, ulRest;

         if( ISCHAR( 3 ) )
            cDelim = hb_parc( 3 )[0];
         else if( ISNUM( 3 ) )
            cDelim = ( char ) hb_parni( 3 );

         for( ul = 0; ul < ulLen; ++ul )
         {
            if( szText[ul] == cDelim )
            {
               iTokens++;
               while( ul + 1 < ulLen && szText[ul + 1] == cDelim )
                  ++ul;
            }
         }
         if( iTokens == 0 )
         {
            hb_itemReturn( hb_param( 1, HB_IT_ANY ) );
         }
         else
         {
            ulRest = ( ULONG ) lSize - ulLen;
            iRepl = ulRest / iTokens;
            iRest = ulRest % iTokens;
            iFirst = ( iRest + 1 ) >> 1;
            iRest >>= 1;
            szDest = ( char * ) hb_xgrab( lSize + 1 );
            for( ulDst = ul = 0; ul < ulLen; ++ul )
            {
               szDest[ulDst++] = szText[ul];
               if( szText[ul] == cDelim )
               {
                  while( ul + 1 < ulLen && szText[ul + 1] == cDelim )
                     szDest[ulDst++] = szText[++ul];
                  i = iRepl;
                  if( iFirst )
                  {
                     --iFirst;
                     ++i;
                  }
                  else if( iTokens <= iRest )
                     ++i;
                  while( --i >= 0 )
                     szDest[ulDst++] = cDelim;
                  iTokens--;
               }
            }
            hb_retclen_buffer( szDest, lSize );
         }
      }
   }
}
