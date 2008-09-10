/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * ASORT() function
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *                     Jose Lalin <dezac@corevia.com>
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

/* TOFIX: The sorting engine requires signed indexes to work, this means
          that arrays larger than 2^31 elements cannot be sorted. [vszakats] */

/* NOTE: Based on PD code found in
         SORTING AND SEARCHING ALGORITHMS: A COOKBOOK, BY THOMAS NIEMANN
         http://members.xoom.com/_XMCM/thomasn/s_man.htm */

#include "hbvmopt.h"
#include "hbapiitm.h"
#include "hbvm.h"
#include "hbstack.h"

static BOOL hb_itemIsLess( PHB_ITEM pItem1, PHB_ITEM pItem2, PHB_ITEM pBlock, PHB_BASEARRAY pBaseArray, ULONG ulLast )
{
   if( pBlock )
   {
      hb_vmPushSymbol( &hb_symEval );
      hb_vmPush( pBlock );
      hb_vmPush( pItem1 );
      hb_vmPush( pItem2 );
      hb_vmSend( 2 );

      if( pBaseArray->ulLen <= ulLast )
         return FALSE;
      else if( HB_IS_LOGICAL( hb_stackReturnItem() ) )
         return hb_itemGetL( hb_stackReturnItem() );
   }

   /* Do native compare when no codeblock is supplied */

   if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
      return hb_itemStrCmp( pItem1, pItem2, FALSE ) < 0;
   else if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
      return hb_itemGetNInt( pItem1 ) < hb_itemGetNInt( pItem2 );
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
      return hb_itemGetND( pItem1 ) < hb_itemGetND( pItem2 );
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
      return hb_itemGetDL( pItem1 ) < hb_itemGetDL( pItem2 );
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
      return hb_itemGetL( pItem1 ) < hb_itemGetL( pItem2 );
   else
   {
      /* NOTE: For non-matching types CA-Cl*pper sorts always like this:
               Array/Object Block String Logical Date Numeric NIL [jlalin] */

      int iWeight1;
      int iWeight2;

      if( HB_IS_ARRAY( pItem1 ) ) iWeight1 = 1;
      else if( HB_IS_BLOCK( pItem1 ) ) iWeight1 = 2;
      else if( HB_IS_STRING( pItem1 ) ) iWeight1 = 3;
      else if( HB_IS_LOGICAL( pItem1 ) ) iWeight1 = 4;
      else if( HB_IS_DATE( pItem1 ) ) iWeight1 = 5;
      else if( HB_IS_NUMERIC( pItem1 ) ) iWeight1 = 6;
      else iWeight1 = 7;

      if( HB_IS_ARRAY( pItem2 ) ) iWeight2 = 1;
      else if( HB_IS_BLOCK( pItem2 ) ) iWeight2 = 2;
      else if( HB_IS_STRING( pItem2 ) ) iWeight2 = 3;
      else if( HB_IS_LOGICAL( pItem2 ) ) iWeight2 = 4;
      else if( HB_IS_DATE( pItem2 ) ) iWeight2 = 5;
      else if( HB_IS_NUMERIC( pItem2 ) ) iWeight2 = 6;
      else iWeight2 = 7;

      return iWeight1 < iWeight2;
   }
}

/* partition array pItems[lb..ub] */

static LONG hb_arraySortQuickPartition( PHB_BASEARRAY pBaseArray, LONG lb, LONG ub, PHB_ITEM pBlock )
{
   LONG i, j;

   /* select pivot and exchange with 1st element */
   i = lb + ( ( ub - lb ) >> 1 );
   if( i != lb )
      hb_itemSwap( pBaseArray->pItems + lb, pBaseArray->pItems + i );

   /* sort lb+1..ub based on pivot */
   i = lb + 1;
   j = ub;

   for( ;; )
   {
      while( j >= i && !hb_itemIsLess( pBaseArray->pItems + j, pBaseArray->pItems + lb, pBlock, pBaseArray, j ) )
      {
         j--;
      }

      while( i < j && !hb_itemIsLess( pBaseArray->pItems + lb, pBaseArray->pItems + i, pBlock, pBaseArray, i ) )
      {
         i++;
      }

      if( i >= j )
      {
         break;
      }

      /* Swap the items */
      hb_itemSwap( pBaseArray->pItems + i, pBaseArray->pItems + j );
      j--;
      i++;
   }

   /* pivot belongs in pBaseArray->pItems[j] */
   if( j > lb && pBaseArray->ulLen > ( ULONG ) j )
      hb_itemSwap( pBaseArray->pItems + lb, pBaseArray->pItems + j );

   return j;
}

/* sort array pBaseArray->pItems[lb..ub] */

static void hb_arraySortQuick( PHB_BASEARRAY pBaseArray, LONG lb, LONG ub, PHB_ITEM pBlock )
{
   LONG m;

   while( lb < ub )
   {
      if( ( ULONG ) ub >= pBaseArray->ulLen )
      {
         ub = pBaseArray->ulLen - 1;
         if( lb >= ub )
            break;
      }

      /* partition into two segments */
      m = hb_arraySortQuickPartition( pBaseArray, lb, ub, pBlock );

      /* sort the smallest partition to minimize stack requirements */
      if( m - lb <= ub - m )
      {
         hb_arraySortQuick( pBaseArray, lb, m - 1, pBlock );
         lb = m + 1;
      }
      else
      {
         hb_arraySortQuick( pBaseArray, m + 1, ub, pBlock );
         ub = m - 1;
      }
   }
}

BOOL hb_arraySort( PHB_ITEM pArray, ULONG * pulStart, ULONG * pulCount, PHB_ITEM pBlock )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_arraySort(%p, %p, %p, %p)", pArray, pulStart, pulCount, pBlock));

   if( HB_IS_ARRAY( pArray ) )
   {
      PHB_BASEARRAY pBaseArray = pArray->item.asArray.value;
      ULONG ulLen = pBaseArray->ulLen;
      ULONG ulStart;
      ULONG ulCount;
      ULONG ulEnd;

      if( pulStart && ( *pulStart >= 1 ) )
         ulStart = *pulStart;
      else
         ulStart = 1;

      if( ulStart <= ulLen )
      {
         if( pulCount && *pulCount >= 1 && ( *pulCount <= ulLen - ulStart ) )
            ulCount = *pulCount;
         else
            ulCount = ulLen - ulStart + 1;

         if( ulStart + ulCount > ulLen )             /* check range */
            ulCount = ulLen - ulStart + 1;

         ulEnd = ulCount + ulStart - 2;

         /* Optimize when only one or no element is to be sorted */
         if( ulCount > 1 )
            hb_arraySortQuick( pBaseArray, ulStart - 1, ulEnd, pBlock );
      }

      return TRUE;
   }
   else
      return FALSE;
}

HB_FUNC( ASORT )
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );

   if( pArray && ! hb_arrayIsObject( pArray ) )
   {
      ULONG ulStart = hb_parnl( 2 );
      ULONG ulCount = hb_parnl( 3 );

      hb_arraySort( pArray,
                    ISNUM( 2 ) ? &ulStart : NULL,
                    ISNUM( 3 ) ? &ulCount : NULL,
                    hb_param( 4, HB_IT_BLOCK ) );

      hb_itemReturn( pArray ); /* ASort() returns the array itself */
   }
}
