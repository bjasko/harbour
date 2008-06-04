/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    HB_BIT*() functions
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

static BOOL hb_numParam( int iParam, HB_LONG * plNum )
{
   if( ISNUM( iParam ) )
   {
      *plNum = hb_parnint( iParam );
      return TRUE;
   }
   hb_errRT_BASE_SubstR( EG_ARG, 1089, NULL, &hb_errFuncName, HB_ERR_ARGS_BASEPARAMS );
   *plNum = 0;
   return FALSE;
}

HB_FUNC( HB_BITAND )
{
   HB_LONG lValue;
   if( hb_numParam( 1, &lValue ) )
   {
      int iPCount = hb_pcount() - 1, i = 1;
      do
      {
         HB_LONG lNext;
         if( !hb_numParam( ++i, &lNext ) )
            return;
         lValue &= lNext;
      }
      while( --iPCount > 0 );
      hb_retnint( lValue );
   }
}

HB_FUNC( HB_BITOR )
{
   HB_LONG lValue;
   if( hb_numParam( 1, &lValue ) )
   {
      int iPCount = hb_pcount() - 1, i = 1;
      do
      {
         HB_LONG lNext;
         if( !hb_numParam( ++i, &lNext ) )
            return;
         lValue |= lNext;
      }
      while( --iPCount > 0 );
      hb_retnint( lValue );
   }
}

HB_FUNC( HB_BITXOR )
{
   HB_LONG lValue;
   if( hb_numParam( 1, &lValue ) )
   {
      int iPCount = hb_pcount() - 1, i = 1;
      do
      {
         HB_LONG lNext;
         if( !hb_numParam( ++i, &lNext ) )
            return;
         lValue ^= lNext;
      }
      while( --iPCount > 0 );
      hb_retnint( lValue );
   }
}

HB_FUNC( HB_BITNOT )
{
   HB_LONG lValue;
   if( hb_numParam( 1, &lValue ) )
      hb_retnint( ~lValue );
}

HB_FUNC( HB_BITTEST )
{
   HB_LONG lValue, lBit;
   if( hb_numParam( 1, &lValue ) && hb_numParam( 2, &lBit ) )
      hb_retl( ( lValue & ( ( HB_LONG ) 1 << lBit ) ) != 0 );
}

HB_FUNC( HB_BITSET )
{
   HB_LONG lValue, lBit;
   if( hb_numParam( 1, &lValue ) && hb_numParam( 2, &lBit ) )
      hb_retnint( lValue | ( ( HB_LONG ) 1 << lBit ) );
}

HB_FUNC( HB_BITRESET )
{
   HB_LONG lValue, lBit;
   if( hb_numParam( 1, &lValue ) && hb_numParam( 2, &lBit ) )
      hb_retnint( lValue & ( ~ ( ( HB_LONG ) 1 << lBit ) ) );
}

HB_FUNC( HB_BITSHIFT )
{
   HB_LONG lValue, lBits;
   if( hb_numParam( 1, &lValue ) && hb_numParam( 2, &lBits ) )
   {
      if( lBits < 0 )
         hb_retnint( lValue >> -lBits );
      else
         hb_retnint( lValue << lBits );
   }
}
