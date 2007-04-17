/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    item serialization code
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
#include "hbapiitm.h"
#include "hbapicls.h"


/*
TODO: 1. extended number format with size and decimals
      2. extended hash format with default value and hash flags
*/

/*
UCHAR [ 1 ] - item type
   0. NIL               0
   1. TRUE              0
   2. FALSE             0
   3. ZERO              0
   4. INT8              1
   5. INT16             2
   6. INT24             3
   7. INT32             4
   8. INT64             8
   9. DOUBLE IEE754 LE  8
  10. DATE              3
  11. STRING8           1+n
  12. STRING16          2+n
  13. STRING32          4+n
  14. ARRAY8            1+n
  15. ARRAY16           2+n
  16. ARRAY32           4+n
  17. ARRAYREF8         1+n
  18. ARRAYREF16        2+n
  19. ARRAYREF32        4+n
  20. HASH8             1+n
  21. HASH16            2+n
  22. HASH32            4+n
  23. HASHREF8          1+n
  24. HASHREF16         2+n
  25. HASHREF32         4+n
  26. SYMBOL		1+n
  27. CYCLIC REFERENCE  4
  28. OBJECT MARKER     n+1+m+1
*/

#define HB_SERIAL_NIL         0
#define HB_SERIAL_TRUE        1
#define HB_SERIAL_FALSE       2
#define HB_SERIAL_ZERO        3
#define HB_SERIAL_INT8        4
#define HB_SERIAL_INT16       5
#define HB_SERIAL_INT24       6
#define HB_SERIAL_INT32       7
#define HB_SERIAL_INT64       8
#define HB_SERIAL_DOUBLE      9
#define HB_SERIAL_DATE       10
#define HB_SERIAL_STRING8    11
#define HB_SERIAL_STRING16   12
#define HB_SERIAL_STRING32   13
#define HB_SERIAL_ARRAY8     14
#define HB_SERIAL_ARRAY16    15
#define HB_SERIAL_ARRAY32    16
#define HB_SERIAL_ARRAYREF8  17
#define HB_SERIAL_ARRAYREF16 18
#define HB_SERIAL_ARRAYREF32 19
#define HB_SERIAL_HASH8      20
#define HB_SERIAL_HASH16     21
#define HB_SERIAL_HASH32     22
#define HB_SERIAL_HASHREF8   23
#define HB_SERIAL_HASHREF16  24
#define HB_SERIAL_HASHREF32  25
#define HB_SERIAL_SYMBOL     26
#define HB_SERIAL_REF        27
#define HB_SERIAL_OBJ        28

#define HB_SERIAL_DUMMYOFFSET ( ( ULONG ) -1 )

typedef struct _HB_CYCLIC_REF
{
   void *   value;
   ULONG    ulOffset;
   BOOL     fRef;
   struct _HB_CYCLIC_REF * pNext;
} HB_CYCLIC_REF, * PHB_CYCLIC_REF;

static ULONG hb_deserializeItem( PHB_ITEM pItem, UCHAR * pBuffer,
                                 ULONG ulOffset, PHB_CYCLIC_REF pRef );

static BOOL hb_itemSerialValueRef( PHB_CYCLIC_REF * pRefPtr, void * value,
                                   ULONG ulOffset )
{
   while( * pRefPtr )
   {
      if( ( * pRefPtr )->value == value )
      {
         ( * pRefPtr )->fRef = TRUE;
         return TRUE;
      }
      pRefPtr = &( * pRefPtr )->pNext;
   }

   * pRefPtr = ( PHB_CYCLIC_REF ) hb_xgrab( sizeof( HB_CYCLIC_REF ) );
   ( * pRefPtr )->value = value;
   ( * pRefPtr )->ulOffset = ulOffset;
   ( * pRefPtr )->fRef = FALSE;
   ( * pRefPtr )->pNext = NULL;

   return FALSE;
}

static void hb_itemSerialUnRefFree( PHB_CYCLIC_REF * pRefPtr )
{
   PHB_CYCLIC_REF pRef;

   while( * pRefPtr )
   {
      pRef = * pRefPtr;
      if( ! pRef->fRef )
      {
         * pRefPtr =pRef->pNext;
         hb_xfree( pRef );
      }
      else
         pRefPtr = &pRef->pNext;
   }
}

static BOOL hb_itemSerialValueOffset( PHB_CYCLIC_REF pRef, void * value,
                                      ULONG ulOffset, ULONG * pulRef )
{
   while( pRef )
   {
      if( pRef->value == value )
      {
         * pulRef = pRef->ulOffset;
         return pRef->ulOffset < ulOffset;
      }
      pRef = pRef->pNext;
   }

   * pulRef = HB_SERIAL_DUMMYOFFSET;
   return FALSE;
}

static BOOL hb_itemSerialOffsetRef( PHB_CYCLIC_REF * pRefPtr, void * value,
                                    ULONG ulOffset )
{
   while( * pRefPtr )
   {
      if( ( * pRefPtr )->ulOffset == ulOffset )
         return TRUE;
      pRefPtr = &( * pRefPtr )->pNext;
   }

   * pRefPtr = ( PHB_CYCLIC_REF ) hb_xgrab( sizeof( HB_CYCLIC_REF ) );
   ( * pRefPtr )->value = value;
   ( * pRefPtr )->ulOffset = ulOffset;
   ( * pRefPtr )->fRef = FALSE;
   ( * pRefPtr )->pNext = NULL;

   return FALSE;
}

static void hb_itemSerialOffsetSet( PHB_CYCLIC_REF pRef, PHB_ITEM pItem,
                                    ULONG ulOffset )
{
   while( pRef )
   {
      if( pRef->ulOffset == ulOffset )
      {
         pRef->value = ( void * ) pItem;
         return;
         break;
      }
      pRef = pRef->pNext;
   }
}

static void hb_itemSerialOffsetGet( PHB_CYCLIC_REF pRef, PHB_ITEM pItem,
                                    ULONG ulOffset )
{
   while( pRef )
   {
      if( pRef->ulOffset == ulOffset )
      {
         hb_itemCopy( pItem, ( PHB_ITEM ) pRef->value );
         return;
         break;
      }
      pRef = pRef->pNext;
   }
}

static void hb_itemSerialRefFree( PHB_CYCLIC_REF pRef )
{
   while( pRef )
   {
      PHB_CYCLIC_REF pNext = pRef->pNext;
      hb_xfree( pRef );
      pRef = pNext;
   }
}

static ULONG hb_itemSerialSize( PHB_ITEM pItem, PHB_CYCLIC_REF * pRefPtr, ULONG ulOffset )
{
   ULONG ulSize, ulLen, u;
   HB_LONG lVal;
   double d;
   USHORT uiClass;

   switch( hb_itemType( pItem ) )
   {
      case HB_IT_NIL:
      case HB_IT_LOGICAL:
         ulSize = 1;
         break;

      case HB_IT_DATE:
         ulSize = 4;
         break;

      case HB_IT_INTEGER:
      case HB_IT_LONG:
         lVal = hb_itemGetNInt( pItem );
         if( lVal == 0 )
            ulSize = 1;
         else if( HB_LIM_INT8( lVal ) )
            ulSize = 2;
         else if( HB_LIM_INT16( lVal ) )
            ulSize = 3;
         else if( HB_LIM_INT24( lVal ) )
            ulSize = 4;
         else if( HB_LIM_INT32( lVal ) )
            ulSize = 5;
         else
            ulSize = 9;
         break;

      case HB_IT_DOUBLE:
         d = hb_itemGetND( pItem );
         if( d == 0.0 )
            ulSize = 1;
         else
            ulSize = 9;
         break;

      case HB_IT_SYMBOL:
         ulSize = 2 + strlen( hb_itemGetSymbol( pItem )->szName );
         break;

      case HB_IT_STRING:
      case HB_IT_MEMO:
         ulLen = hb_itemGetCLen( pItem );
         if( ulLen <= 255 )
            ulSize = ulLen + 2;
         else if( ulLen <= UINT16_MAX )
            ulSize = ulLen + 3;
         else
            ulSize = ulLen + 5;
         break;

      case HB_IT_ARRAY:
         ulSize = 0;
         uiClass = hb_objGetClass( pItem );
         if( uiClass )
         {
            char * szClass = hb_clsName( uiClass ),
                 * szFunc = hb_clsFuncName( uiClass );
            if( szClass && szFunc )
               ulSize += strlen( szClass ) + strlen( szFunc ) + 3;
         }
         if( hb_itemSerialValueRef( pRefPtr, hb_arrayId( pItem ), ulOffset + ulSize ) )
         {
            ulSize = 5;
         }
         else
         {
            ulLen = hb_arrayLen( pItem );
            if( ulLen <= 255 )
               ulSize += 2;
            else if( ulLen <= UINT16_MAX )
               ulSize += 3;
            else
               ulSize += 5;
            for( u = 1; u <= ulLen; u++ )
               ulSize += hb_itemSerialSize( hb_arrayGetItemPtr( pItem, u ),
                                            pRefPtr, ulOffset + ulSize );
         }
         break;

      case HB_IT_HASH:
         if( hb_itemSerialValueRef( pRefPtr, hb_hashId( pItem ), ulOffset ) )
         {
            ulSize = 5;
         }
         else
         {
            ulLen = hb_hashLen( pItem );
            if( ulLen <= 255 )
               ulSize = 2;
            else if( ulLen <= UINT16_MAX )
               ulSize = 3;
            else
               ulSize = 5;
            for( u = 1; u <= ulLen; u++ )
            {
               ulSize += hb_itemSerialSize( hb_hashGetKeyAt( pItem, u ),
                                            pRefPtr, ulOffset + ulSize );
               ulSize += hb_itemSerialSize( hb_hashGetValueAt( pItem, u ),
                                            pRefPtr, ulOffset + ulSize );
            }
         }
         break;

      default:
         /* map to NIL */
         ulSize = 1;
   }

   return ulSize;
}

static ULONG hb_serializeItem( PHB_ITEM pItem, UCHAR * pBuffer,
                               ULONG ulOffset, PHB_CYCLIC_REF pRef )
{
   HB_LONG lVal;
   ULONG ulRef, ulLen, u;
   LONG l;
   double d;
   char * szVal;

   switch( hb_itemType( pItem ) )
   {
      case HB_IT_NIL:
         pBuffer[ ulOffset++ ] = HB_SERIAL_NIL;
         break;

      case HB_IT_LOGICAL:
         pBuffer[ ulOffset++ ] = hb_itemGetL( pItem ) ? HB_SERIAL_TRUE : HB_SERIAL_FALSE;
         break;

      case HB_IT_DATE:
         pBuffer[ ulOffset++ ] = HB_SERIAL_DATE;
         l = hb_itemGetDL( pItem );
         HB_PUT_LE_UINT24( &pBuffer[ ulOffset ], l );
         ulOffset += 3;
         break;

      case HB_IT_INTEGER:
      case HB_IT_LONG:
         lVal = hb_itemGetNInt( pItem );
         if( lVal == 0 )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_ZERO;
         }
         else if( HB_LIM_INT8( lVal ) )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_INT8;
            pBuffer[ ulOffset++ ] = ( UCHAR ) lVal;
         }
         else if( HB_LIM_INT16( lVal ) )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_INT16;
            HB_PUT_LE_UINT16( &pBuffer[ ulOffset ], lVal );
            ulOffset += 2;
         }
         else if( HB_LIM_INT24( lVal ) )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_INT24;
            HB_PUT_LE_UINT24( &pBuffer[ ulOffset ], lVal );
            ulOffset += 3;
         }
         else if( HB_LIM_INT32( lVal ) )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_INT32;
            HB_PUT_LE_UINT32( &pBuffer[ ulOffset ], lVal );
            ulOffset += 4;
         }
         else
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_INT64;
            HB_PUT_LE_UINT64( &pBuffer[ ulOffset ], lVal );
            ulOffset += 8;
         }
         break;

      case HB_IT_DOUBLE:
         d = hb_itemGetND( pItem );
         if( d == 0.0 )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_ZERO;
         }
         else
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_DOUBLE;
            HB_PUT_LE_DOUBLE( &pBuffer[ ulOffset ], d );
            ulOffset += 8;
         }
         break;

      case HB_IT_SYMBOL:
         szVal = hb_itemGetSymbol( pItem )->szName;
         ulLen = strlen( szVal );
         if( ulLen > 0xFF )
            ulLen = 0xFF;
         pBuffer[ ulOffset++ ] = HB_SERIAL_SYMBOL;
         pBuffer[ ulOffset++ ] = ( UCHAR ) ulLen;
         memcpy( &pBuffer[ ulOffset ], szVal, ulLen );
         ulOffset += ulLen;
         break;

      case HB_IT_STRING:
      case HB_IT_MEMO:
         ulLen = hb_itemGetCLen( pItem );
         if( ulLen <= 255 )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_STRING8;
            pBuffer[ ulOffset++ ] = ( UCHAR ) ulLen;
         }
         else if( ulLen <= UINT16_MAX )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_STRING16;
            HB_PUT_LE_UINT16( &pBuffer[ ulOffset ], ulLen );
            ulOffset += 2;
         }
         else
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_STRING32;
            HB_PUT_LE_UINT32( &pBuffer[ ulOffset ], ulLen );
            ulOffset += 4;
         }
         memcpy( &pBuffer[ ulOffset ], hb_itemGetCPtr( pItem ), ulLen );
         ulOffset += ulLen;
         break;

      case HB_IT_ARRAY:
         if( hb_itemSerialValueOffset( pRef, hb_arrayId( pItem ), ulOffset, &ulRef ) )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_REF;
            HB_PUT_LE_UINT32( &pBuffer[ ulOffset ], ulRef );
            ulOffset += 4;
         }
         else
         {
            USHORT uiClass = hb_objGetClass( pItem );
            if( uiClass )
            {
               char * szClass = hb_clsName( uiClass ),
                    * szFunc = hb_clsFuncName( uiClass );
               if( szClass && szFunc )
               {
                  pBuffer[ ulOffset++ ] = HB_SERIAL_OBJ;
                  ulLen = strlen( szClass ) + 1;
                  memcpy( &pBuffer[ ulOffset ], szClass, ulLen );
                  ulOffset += ulLen;
                  ulLen = strlen( szFunc ) + 1;
                  memcpy( &pBuffer[ ulOffset ], szFunc, ulLen );
                  ulOffset += ulLen;
               }
            }
            ulLen = hb_arrayLen( pItem );
            if( ulLen <= 255 )
            {
               pBuffer[ ulOffset++ ] = ulRef == HB_SERIAL_DUMMYOFFSET ?
                                       HB_SERIAL_ARRAY8 : HB_SERIAL_ARRAYREF8;
               pBuffer[ ulOffset++ ] = ( UCHAR ) ulLen;
            }
            else if( ulLen <= UINT16_MAX )
            {
               pBuffer[ ulOffset++ ] = ulRef == HB_SERIAL_DUMMYOFFSET ?
                                       HB_SERIAL_ARRAY16 : HB_SERIAL_ARRAYREF16;
               HB_PUT_LE_UINT16( &pBuffer[ ulOffset ], ulLen );
               ulOffset += 2;
            }
            else
            {
               pBuffer[ ulOffset++ ] = ulRef == HB_SERIAL_DUMMYOFFSET ?
                                       HB_SERIAL_ARRAY32 : HB_SERIAL_ARRAYREF32;
               HB_PUT_LE_UINT32( &pBuffer[ ulOffset ], ulLen );
               ulOffset += 4;
            }
            for( u = 1; u <= ulLen; u++ )
               ulOffset = hb_serializeItem( hb_arrayGetItemPtr( pItem, u ), pBuffer, ulOffset, pRef );
         }
         break;

      case HB_IT_HASH:
         if( hb_itemSerialValueOffset( pRef, hb_hashId( pItem ), ulOffset, &ulRef ) )
         {
            pBuffer[ ulOffset++ ] = HB_SERIAL_REF;
            HB_PUT_LE_UINT32( &pBuffer[ ulOffset ], ulRef );
            ulOffset += 4;
         }
         else
         {
            ulLen = hb_hashLen( pItem );
            if( ulLen <= 255 )
            {
               pBuffer[ ulOffset++ ] = ulRef == HB_SERIAL_DUMMYOFFSET ?
                                       HB_SERIAL_HASH8 : HB_SERIAL_HASHREF8;
               pBuffer[ ulOffset++ ] = ( UCHAR ) ulLen;
            }
            else if( ulLen <= UINT16_MAX )
            {
               pBuffer[ ulOffset++ ] = ulRef == HB_SERIAL_DUMMYOFFSET ?
                                       HB_SERIAL_HASH16 : HB_SERIAL_HASHREF16;
               HB_PUT_LE_UINT16( &pBuffer[ ulOffset ], ulLen );
               ulOffset += 2;
            }
            else
            {
               pBuffer[ ulOffset++ ] = ulRef == HB_SERIAL_DUMMYOFFSET ?
                                       HB_SERIAL_HASH32 : HB_SERIAL_HASHREF32;
               HB_PUT_LE_UINT32( &pBuffer[ ulOffset ], ulLen );
               ulOffset += 4;
            }
            for( u = 1; u <= ulLen; u++ )
            {
               ulOffset = hb_serializeItem( hb_hashGetKeyAt( pItem, u ), pBuffer, ulOffset, pRef );
               ulOffset = hb_serializeItem( hb_hashGetValueAt( pItem, u ), pBuffer, ulOffset, pRef );
            }
         }
         break;

      default:
         /* map to NIL */
         pBuffer[ ulOffset++ ] = HB_SERIAL_NIL;
         break;
   }

   return ulOffset;
}

static ULONG hb_deserializeHash( PHB_ITEM pItem, UCHAR * pBuffer, ULONG ulOffset,
                                 ULONG ulLen, PHB_CYCLIC_REF pRef )
{
   hb_hashNew( pItem );

   if( ulLen )
   {
      PHB_ITEM pKey = hb_itemNew( NULL );
      PHB_ITEM pVal = hb_itemNew( NULL );

      hb_hashPreallocate( pItem, ulLen );
      while( ulLen-- )
      {
         ulOffset = hb_deserializeItem( pKey, pBuffer, ulOffset, pRef );
         ulOffset = hb_deserializeItem( pVal, pBuffer, ulOffset, pRef );
         hb_hashAdd( pItem, pKey, pVal );
      }
      hb_itemRelease( pKey );
      hb_itemRelease( pVal );
   }

   return ulOffset;
}

static ULONG hb_deserializeArray( PHB_ITEM pItem, UCHAR * pBuffer, ULONG ulOffset,
                                  ULONG ulLen, PHB_CYCLIC_REF pRef )
{
   ULONG u;

   hb_arrayNew( pItem, ulLen );
   for( u = 1; u <= ulLen; u++ )
      ulOffset = hb_deserializeItem( hb_arrayGetItemPtr( pItem, u ),
                                     pBuffer, ulOffset, pRef );

   return ulOffset;
}

static ULONG hb_deserializeItem( PHB_ITEM pItem, UCHAR * pBuffer,
                                 ULONG ulOffset, PHB_CYCLIC_REF pRef )
{
   ULONG ulLen;
   char * szVal;

   switch( pBuffer[ ulOffset++ ] )
   {
      case HB_SERIAL_NIL:
         hb_itemClear( pItem );
         break;

      case HB_SERIAL_TRUE:
         hb_itemPutL( pItem, TRUE );
         break;

      case HB_SERIAL_FALSE:
         hb_itemPutL( pItem, FALSE );
         break;

      case HB_SERIAL_ZERO:
         hb_itemPutNI( pItem, 0 );
         break;

      case HB_SERIAL_INT8:
         hb_itemPutNI( pItem, pBuffer[ ulOffset++ ] );
         break;

      case HB_SERIAL_INT16:
         hb_itemPutNI( pItem, HB_GET_LE_INT16( &pBuffer[ ulOffset ] ) );
         ulOffset += 2;
         break;

      case HB_SERIAL_INT24:
         hb_itemPutNInt( pItem, HB_GET_LE_INT24( &pBuffer[ ulOffset ] ) );
         ulOffset += 3;
         break;

      case HB_SERIAL_INT32:
         hb_itemPutNInt( pItem, HB_GET_LE_INT32( &pBuffer[ ulOffset ] ) );
         ulOffset += 4;
         break;

      case HB_SERIAL_INT64:
         hb_itemPutNInt( pItem, HB_GET_LE_INT64( &pBuffer[ ulOffset ] ) );
         ulOffset += 8;
         break;

      case HB_SERIAL_DOUBLE:
         hb_itemPutND( pItem, HB_GET_LE_DOUBLE( &pBuffer[ ulOffset ] ) );
         ulOffset += 8;
         break;

      case HB_SERIAL_DATE:
         hb_itemPutDL( pItem, HB_GET_LE_UINT24( &pBuffer[ ulOffset ] ) );
         ulOffset += 3;
         break;

      case HB_SERIAL_SYMBOL:
         ulLen = pBuffer[ ulOffset++ ];
         szVal = hb_strndup( ( char * ) &pBuffer[ ulOffset ], ulLen );
         hb_itemPutSymbol( pItem, hb_dynsymGetSymbol( szVal ) );
         hb_xfree( szVal );
         ulOffset += ulLen;
         break;

      case HB_SERIAL_STRING8:
         ulLen = pBuffer[ ulOffset++ ];
         hb_itemPutCL( pItem, ( char * ) &pBuffer[ ulOffset ], ulLen );
         ulOffset += ulLen;
         break;
      case HB_SERIAL_STRING16:
         ulLen = HB_GET_LE_UINT16( &pBuffer[ ulOffset ] );
         ulOffset += 2;
         hb_itemPutCL( pItem, ( char * ) &pBuffer[ ulOffset ], ulLen );
         ulOffset += ulLen;
         break;
      case HB_SERIAL_STRING32:
         ulLen = HB_GET_LE_UINT32( &pBuffer[ ulOffset ] );
         ulOffset += 4;
         hb_itemPutCL( pItem, ( char * ) &pBuffer[ ulOffset ], ulLen );
         ulOffset += ulLen;
         break;

      case HB_SERIAL_ARRAYREF8:
         hb_itemSerialOffsetSet( pRef, pItem, ulOffset - 1 );
      case HB_SERIAL_ARRAY8:
         ulLen = pBuffer[ ulOffset++ ];
         ulOffset = hb_deserializeArray( pItem, pBuffer, ulOffset, ulLen, pRef );
         break;
      case HB_SERIAL_ARRAYREF16:
         hb_itemSerialOffsetSet( pRef, pItem, ulOffset - 1 );
      case HB_SERIAL_ARRAY16:
         ulLen = HB_GET_LE_UINT16( &pBuffer[ ulOffset ] );
         ulOffset = hb_deserializeArray( pItem, pBuffer, ulOffset + 2, ulLen, pRef );
         break;
      case HB_SERIAL_ARRAYREF32:
         hb_itemSerialOffsetSet( pRef, pItem, ulOffset - 1 );
      case HB_SERIAL_ARRAY32:
         ulLen = HB_GET_LE_UINT32( &pBuffer[ ulOffset ] );
         ulOffset = hb_deserializeArray( pItem, pBuffer, ulOffset + 4, ulLen, pRef );
         break;

      case HB_SERIAL_HASHREF8:
         hb_itemSerialOffsetSet( pRef, pItem, ulOffset - 1 );
      case HB_SERIAL_HASH8:
         ulLen = pBuffer[ ulOffset++ ];
         ulOffset = hb_deserializeHash( pItem, pBuffer, ulOffset, ulLen, pRef );
         break;
      case HB_SERIAL_HASHREF16:
         hb_itemSerialOffsetSet( pRef, pItem, ulOffset - 1 );
      case HB_SERIAL_HASH16:
         ulLen = HB_GET_LE_UINT16( &pBuffer[ ulOffset ] );
         ulOffset = hb_deserializeHash( pItem, pBuffer, ulOffset + 2, ulLen, pRef );
         break;
      case HB_SERIAL_HASHREF32:
         hb_itemSerialOffsetSet( pRef, pItem, ulOffset - 1 );
      case HB_SERIAL_HASH32:
         ulLen = HB_GET_LE_UINT32( &pBuffer[ ulOffset ] );
         ulOffset = hb_deserializeHash( pItem, pBuffer, ulOffset + 4, ulLen, pRef );
         break;

      case HB_SERIAL_REF:
         hb_itemSerialOffsetGet( pRef, pItem,
                                 HB_GET_LE_UINT32( &pBuffer[ ulOffset ] ) );
         ulOffset += 4;
         break;

      case HB_SERIAL_OBJ:
      {
         char * szClass, * szFunc;
         szClass = ( char * ) &pBuffer[ ulOffset ];
         ulLen = strlen( szClass );
         szFunc = szClass + ulLen + 1;
         ulOffset = hb_deserializeItem( pItem, pBuffer,
                              ulOffset + ulLen + strlen( szFunc ) + 2, pRef );
         hb_objSetClass( pItem, szClass, szFunc );
         break;
      }

      default:
         hb_itemClear( pItem );
         break;
   }

   return ulOffset;
}

static BOOL hb_deserializeTest( UCHAR ** pBufferPtr, ULONG * pulSize,
                                ULONG ulOffset, PHB_CYCLIC_REF * pRefPtr )
{
   UCHAR * pBuffer = * pBufferPtr;
   ULONG ulSize = * pulSize, ulLen = 0;

   if( ulSize == 0 )
      return FALSE;

   switch( *pBuffer++ )
   {
      case HB_SERIAL_NIL:
      case HB_SERIAL_TRUE:
      case HB_SERIAL_FALSE:
      case HB_SERIAL_ZERO:
         ulSize = 1;
         break;
      case HB_SERIAL_INT8:
         ulSize = 2;
         break;
      case HB_SERIAL_INT16:
         ulSize = 3;
         break;
      case HB_SERIAL_INT24:
      case HB_SERIAL_DATE:
         ulSize = 4;
         break;
      case HB_SERIAL_INT32:
         ulSize = 5;
         break;
      case HB_SERIAL_INT64:
      case HB_SERIAL_DOUBLE:
         ulSize = 9;
         break;
      case HB_SERIAL_SYMBOL:
      case HB_SERIAL_STRING8:
         ulSize = 2 + ( ulSize >= 2 ? *pBuffer : ulSize );
         break;
      case HB_SERIAL_STRING16:
         ulSize = 3 + ( ulSize >= 3 ? HB_GET_LE_UINT16( pBuffer ) : ulSize );
         break;
      case HB_SERIAL_STRING32:
         ulSize = 5 + ( ulSize >= 5 ? HB_GET_LE_UINT32( pBuffer ) : ulSize );
         break;
      case HB_SERIAL_ARRAYREF8:
         if( hb_itemSerialOffsetRef( pRefPtr, NULL, ulOffset ) )
            return FALSE;
      case HB_SERIAL_ARRAY8:
         if( ulSize >= 2 )
         {
            ulSize = 2;
            ulLen = *pBuffer;
         }
         else
            ulSize++;
         break;
      case HB_SERIAL_ARRAYREF16:
         if( hb_itemSerialOffsetRef( pRefPtr, NULL, ulOffset ) )
            return FALSE;
      case HB_SERIAL_ARRAY16:
         if( ulSize >= 3 )
         {
            ulSize = 3;
            ulLen = HB_GET_LE_UINT16( pBuffer );
         }
         else
            ulSize++;
         break;
      case HB_SERIAL_ARRAYREF32:
         if( hb_itemSerialOffsetRef( pRefPtr, NULL, ulOffset ) )
            return FALSE;
      case HB_SERIAL_ARRAY32:
         if( ulSize >= 5 )
         {
            ulSize = 5;
            ulLen = HB_GET_LE_UINT32( pBuffer );
         }
         else
            ulSize++;
         break;
      case HB_SERIAL_HASHREF8:
         if( hb_itemSerialOffsetRef( pRefPtr, NULL, ulOffset ) )
            return FALSE;
      case HB_SERIAL_HASH8:
         if( ulSize >= 2 )
         {
            ulSize = 2;
            ulLen = *pBuffer << 1;
         }
         else
            ulSize++;
         break;
      case HB_SERIAL_HASHREF16:
         if( hb_itemSerialOffsetRef( pRefPtr, NULL, ulOffset ) )
            return FALSE;
      case HB_SERIAL_HASH16:
         if( ulSize >= 3 )
         {
            ulSize = 3;
            ulLen = HB_GET_LE_UINT16( pBuffer ) << 1;
         }
         else
            ulSize++;
         break;
      case HB_SERIAL_HASHREF32:
         if( hb_itemSerialOffsetRef( pRefPtr, NULL, ulOffset ) )
            return FALSE;
      case HB_SERIAL_HASH32:
         if( ulSize >= 5 )
         {
            ulSize = 5;
            ulLen = HB_GET_LE_UINT32( pBuffer ) << 1;
         }
         else
            ulSize++;
         break;
      case HB_SERIAL_REF:
         if( !hb_itemSerialOffsetRef( pRefPtr, NULL, HB_GET_LE_UINT32( pBuffer ) ) )
            return FALSE;
         ulSize = 5;
         break;
      case HB_SERIAL_OBJ:
         ulLen = hb_strnlen( ( char * ) pBuffer, ulSize - 1 ) + 1;
         if( ulLen >= ulSize )
            ulSize++;
         else
         {
            ulLen += hb_strnlen( ( char * ) pBuffer + ulLen, ulSize - ulLen - 1 ) + 2;
            if( ulLen >= ulSize )
               ulSize++;
            else
               ulSize = ulLen;
         }
         ulLen = 1;
         break;
      default:
         ulSize = 1;
         break;
   }

   if( ulSize > * pulSize )
      return FALSE;

   * pulSize -= ulSize;
   * pBufferPtr += ulSize;

   while( ulLen )
   {
      ulOffset += ulSize;
      ulSize = * pulSize;
      if( !hb_deserializeTest( pBufferPtr, pulSize, ulOffset, pRefPtr ) )
         return FALSE;
      ulSize -= * pulSize;
      --ulLen;
   }

   return TRUE;
}

/*
 * These function will be public in the future [druzus]
 */
static char * hb_itemSerial( PHB_ITEM pItem, ULONG *pulSize )
{
   PHB_CYCLIC_REF pRef = NULL;
   ULONG ulSize = hb_itemSerialSize( pItem, &pRef, 0 );
   UCHAR * pBuffer = ( UCHAR * ) hb_xgrab( ulSize + 1 );

   hb_itemSerialUnRefFree( &pRef );
   hb_serializeItem( pItem, pBuffer, 0, pRef );
   pBuffer[ ulSize ] = '\0';
   if( pulSize )
      *pulSize = ulSize;

   hb_itemSerialRefFree( pRef );

   return ( char * ) pBuffer;
}

/*
 * These function will be public in the future [druzus]
 */
static PHB_ITEM hb_itemDeSerial( char ** pBufferPtr, ULONG * pulSize )
{
   PHB_CYCLIC_REF pRef = NULL;
   UCHAR * pBuffer = ( UCHAR * ) *pBufferPtr;
   PHB_ITEM pItem = NULL;

   if( !pulSize || hb_deserializeTest( ( UCHAR ** ) pBufferPtr, pulSize, 0, &pRef ) )
   {
      pItem = hb_itemNew( NULL );
      hb_deserializeItem( pItem, pBuffer, 0, pRef );
   }
   hb_itemSerialRefFree( pRef );

   return pItem;
}

HB_FUNC( HB_SERIALIZE )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ANY );

   if( pItem )
   {
      ULONG ulSize;
      char * pBuffer = hb_itemSerial( pItem, &ulSize );
      hb_retclen_buffer( pBuffer, ulSize );
   }
}

HB_FUNC( HB_DESERIALIZE )
{
   PHB_ITEM pItem, pParam = hb_param( 1, HB_IT_BYREF );
   ULONG ulSize = hb_parclen( 1 );

   if( ulSize )
   {
      char * pBuffer = hb_parc( 1 );

      pItem = hb_itemDeSerial( &pBuffer, &ulSize );
      if( pItem )
      {
         hb_itemReturn( pItem );
         if( pParam )
         {
            hb_itemPutCL( pItem, pBuffer, ulSize );
            hb_itemMove( pParam, pItem );
         }
         hb_itemRelease( pItem );
      }
      else if( pParam )
         hb_itemClear( pParam );
   }
   else if( pParam )
      hb_itemClear( pParam );
}

#ifdef HB_COMPAT_XHB
HB_FUNC( HB_DESERIALBEGIN )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_STRING );
   if( pItem )
      hb_itemReturn( pItem );
}

HB_FUNC( HB_DESERIALNEXT )
{
   HB_FUNC_EXEC( HB_DESERIALIZE );
}
#endif
