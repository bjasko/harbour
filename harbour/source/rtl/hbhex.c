/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    HB_NUMTOHEX()
 *    HB_HEXTONUM()
 *
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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
#include "hbapierr.h"

HB_FUNC( HB_HEXTONUM )
{
   char * szHex = hb_parc( 1 );

   if( szHex )
   {
      HB_ULONG ulNum = 0;

      while( *szHex == ' ' ) szHex++;
      while( *szHex )
      {
         int iDigit;
         char c = *szHex++;
         if ( c >= '0' && c <= '9' )
            iDigit = c - '0';
         else if ( c >= 'A' && c <= 'F' )
            iDigit = c - ( 'A' - 10 );
         else if ( c >= 'a' && c <= 'f' )
            iDigit = c - ( 'a' - 10 );
         else
         {
            ulNum = 0;
            break;
         }
         ulNum = ( ulNum << 4 ) + iDigit;
      }
      hb_retnint( ulNum );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, &hb_errFuncName, HB_ERR_ARGS_BASEPARAMS );
}

HB_FUNC( HB_NUMTOHEX )
{
   HB_ULONG ulNum;
   int      iLen;
   BOOL     fDefaultLen;
   char     ret[ 33 ];

   if( ISNUM( 2 ) )
   {
      iLen = hb_parni( 2 );
      iLen = ( iLen < 1 ) ? 1 : ( ( iLen > 32 ) ? 32 : iLen );
      fDefaultLen = 0;
   }
   else
   {
      iLen = 32;
      fDefaultLen = 1;
   }
                    
   if( ISNUM( 1 ) )
      ulNum = hb_parnint( 1 );
   else if( ISPOINTER( 1 ) )
      ulNum = (HB_PTRDIFF) hb_parptr( 1 );
   else
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, &hb_errFuncName, HB_ERR_ARGS_BASEPARAMS );
      return;
   }

   ret[ iLen ] = '\0';
   do
   {
      int iDigit = ( int ) ( ulNum & 0x0F );
      ret[ --iLen ] = iDigit + ( iDigit < 10 ? '0' : 'A' - 10 ); 
      ulNum >>= 4;
   }
   while( fDefaultLen ? ulNum != 0 : iLen != 0 );

   hb_retc( &ret[ iLen ] );
}

HB_FUNC( HB_STRTOHEX )
{
   char * szStr = hb_parc( 1 ), * szSep = "";
   ULONG ulStr, ulSep = 0;

   if( hb_pcount() > 1 )
   {
      szSep = hb_parc( 2 );
      ulSep = hb_parclen( 2 );
   }

   if( !szStr || !szSep )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, &hb_errFuncName, HB_ERR_ARGS_BASEPARAMS );
      return;
   }

   ulStr = hb_parclen( 1 );
   if( ulStr )
   {
      ULONG ulDest = ( ulStr << 1 ) + ( ulStr - 1 ) * ulSep;
      char * szDest, * szPtr;

      szPtr = szDest = ( char * ) hb_xgrab( ulDest + 1 );
      do
      {
         UCHAR uc = ( UCHAR ) *szStr++, ud;
         ud = uc >> 4;
         *szPtr++ = ud + ( ud < 10 ? '0' : 'A' - 10 );
         ud = uc & 0x0F;
         *szPtr++ = ud + ( ud < 10 ? '0' : 'A' - 10 );
         if( --ulStr && ulSep )
         {
            memcpy( szPtr, szSep, ulSep );
            ulStr += ulSep;
         }
      }
      while( ulStr );
      hb_retclen_buffer( szDest, ulDest );
   }
   else
      hb_retc( NULL );
}

HB_FUNC( HB_HEXTOSTR )
{
   char * szStr = hb_parc( 1 );
   ULONG ulStr;

   if( !szStr )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, &hb_errFuncName, HB_ERR_ARGS_BASEPARAMS );
      return;
   }

   ulStr = hb_parclen( 1 );
   if( ulStr > 1 )
   {
      ULONG ulDest, ul;
      char * szDest, * szPtr;
      UCHAR uc;

      szDest = szStr;
      ul = ulStr;
      ulDest = 0;
      do
      {
         char c = *szDest++;
         if( ( c >= '0' && c <= '9' ) ||
             ( c >= 'A' && c <= 'F' ) ||
             ( c >= 'a' && c <= 'f' ) )
            ++ulDest;
      }
      while( --ul );

      ulDest >>= 1;
      if( ulDest )
      {
         szPtr = szDest = ( char * ) hb_xgrab( ulDest + 1 );
         uc = 0;
         for( ul = 0; ul < ulStr; ++ul )
         {
            char c = szStr[ ul ];
            if( c >= '0' && c <= '9' )
               uc += c - '0';
            else if( c >= 'A' && c <= 'F' )
               uc += c - ( 'A' - 10 );
            else if( c >= 'a' && c <= 'f' )
               uc += c - ( 'a' - 10 );
            if( ul & 1 )
               *szPtr++ = ( char ) uc;
            else
               uc <<= 4;
         }
         hb_retclen_buffer( szDest, ulDest );
         return;
      }
   }

   hb_retc( NULL );
}
