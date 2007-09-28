/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    xHarbour compatible messages used in overloaded scalar classes
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

#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbapilng.h"
#include "hbstack.h"

HB_FUNC( XHB_HASHERROR )
{
   const char * szMessage = hb_itemGetSymbol( hb_stackBaseItem() )->szName;
   int iPCount = hb_pcount();

   if( iPCount == 1 )
   {
      if( szMessage[ 0 ] == '_' )
      {  /* ASSIGN */
         PHB_ITEM pIndex = hb_itemPutCConst( hb_stackAllocItem(), szMessage + 1 );
         PHB_ITEM pDest = hb_hashGetItemPtr( hb_stackSelfItem(), pIndex, HB_HASH_AUTOADD_ASSIGN );
         hb_stackPop();
         if( pDest )
         {
            PHB_ITEM pValue = hb_param( 1, HB_IT_ANY );
            hb_itemCopyFromRef( pDest, pValue );
            hb_itemReturn( pValue );
            return;
         }
      }
   }
   else if( iPCount == 0 )
   {  /* ACCESS */
      PHB_ITEM pIndex = hb_itemPutCConst( hb_stackAllocItem(), szMessage );
      PHB_ITEM pValue = hb_hashGetItemPtr( hb_stackSelfItem(), pIndex, HB_HASH_AUTOADD_ACCESS );
      hb_stackPop();
      if( pValue )
      {
         hb_itemReturn( pValue );
         return;
      }
   }

   if( szMessage[0] == '_' )
      hb_errRT_BASE_SubstR( EG_NOVARMETHOD, 1005, NULL, szMessage + 1, HB_ERR_ARGS_SELFPARAMS );
   else
      hb_errRT_BASE_SubstR( EG_NOMETHOD, 1004, NULL, szMessage, HB_ERR_ARGS_SELFPARAMS );
}

HB_FUNC( XHB_INCLUDE )
{
   PHB_ITEM pSelf = hb_stackSelfItem();
   PHB_ITEM pKey = hb_param( 1, HB_IT_ANY );

   if( HB_IS_ARRAY( pSelf ) )
   {
      hb_retl( hb_arrayScan( pSelf, pKey, NULL, NULL, TRUE ) != 0 );
   }
   else if( HB_IS_HASH( pSelf ) && ( HB_IS_HASHKEY( pKey ) || hb_hashLen( pKey ) == 1 ) )
   {
      hb_retl( hb_hashScan( pSelf, pKey, NULL ) );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1109, NULL, "$", 2, pKey, pSelf );
      if( pResult )
         hb_itemReturnRelease( pResult );
   }
}

#undef HB_IS_VALID_INDEX
#define HB_IS_VALID_INDEX( idx, max )  ( ( ( LONG ) (idx) < 0 ? (idx) += (max) + 1 : (idx) ) > 0 && ( ULONG ) (idx) <= (max) )

HB_FUNC( XHB_INDEX )
{
   PHB_ITEM pSelf = hb_stackSelfItem();
   PHB_ITEM pIndex = hb_param( 1, HB_IT_ANY );

   if( hb_pcount() == 2 )
   {  /* ASSIGN */
      PHB_ITEM pValue = hb_param( 2, HB_IT_ANY );
      if( HB_IS_NUMERIC( pIndex ) )
      {
         ULONG ulIndex = hb_itemGetNL( pIndex );
         if( HB_IS_ARRAY( pSelf ) )
         {
            ULONG ulLen = hb_arrayLen( pSelf );
            if( HB_IS_VALID_INDEX( ulIndex, ulLen ) )
            {
               hb_itemMoveRef( hb_arrayGetItemPtr( pSelf, ulIndex ), pValue );
            }
            else
               hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 1, pIndex );
         }
         else if( HB_IS_STRING( pSelf ) )
         {
            ULONG ulLen = hb_itemGetCLen( pSelf );
            if( HB_IS_VALID_INDEX( ulIndex, ulLen ) )
            {
               char cValue = HB_IS_STRING( pValue ) ? hb_itemGetCPtr( pValue )[0] :
                             hb_itemGetNI( pValue );
               if( ulLen == 1 )
                  hb_itemPutCL( pSelf, &cValue, 1 );
               else
               {
                  hb_itemUnShareString( pSelf );
                  hb_itemGetCPtr( pSelf )[ ulIndex - 1 ] = cValue;
               }
            }
            else
               hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 1, pIndex );
         }
         else
            hb_errRT_BASE( EG_ARG, 1069, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 1, pIndex );
      }
      else
      {
         hb_errRT_BASE( EG_ARG, 1069, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 1, pIndex );
      }
      hb_itemReturn( pSelf );
   }
   else
   {  /* ACCESS */
      if( HB_IS_NUMERIC( pIndex ) )
      {
         ULONG ulIndex = hb_itemGetNL( pIndex );
         if( HB_IS_ARRAY( pSelf ) )
         {
            ULONG ulLen = hb_arrayLen( pSelf );
            if( HB_IS_VALID_INDEX( ulIndex, ulLen ) )
               hb_itemReturn( hb_arrayGetItemPtr( pSelf, ulIndex ) );
            else
               hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pSelf, pIndex );
         }
         else if( HB_IS_STRING( pSelf ) )
         {
            ULONG ulLen = hb_itemGetCLen( pSelf );
            if( HB_IS_VALID_INDEX( ulIndex, ulLen ) )
               hb_retclen( hb_itemGetCPtr( pSelf ) + ulIndex - 1, 1 );
            else
               hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pSelf, pIndex );
         }
         else
            hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pSelf, pIndex );
      }
      else
      {
         PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pSelf, pIndex );
         if( pResult )
            hb_itemReturnRelease( pResult );
      }
   }
}