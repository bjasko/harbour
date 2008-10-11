/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Compiler Expression Optimizer - reducing expressions
 *
 * Copyright 1999 Ryszard Glab
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

/* NOTE: This must be the first definition
 *    This is a common code shared by macro and standalone compiler
 */
#define  HB_COMMON_SUPPORT

#include <math.h>
#include "hbmacro.h"
#include "hbcomp.h"
#include "hbdate.h"

static HB_EXPR_PTR hb_compExprReducePlusStrings( HB_EXPR_PTR pLeft, HB_EXPR_PTR pRight, HB_COMP_DECL )
{
   if( pLeft->value.asString.dealloc )
   {
      pLeft->value.asString.string = (char *) hb_xrealloc( pLeft->value.asString.string, pLeft->ulLength + pRight->ulLength + 1 );
      memcpy( pLeft->value.asString.string + pLeft->ulLength,
              pRight->value.asString.string, pRight->ulLength );
      pLeft->ulLength += pRight->ulLength;
      pLeft->value.asString.string[ pLeft->ulLength ] = '\0';
   }
   else
   {
      char *szString;
      szString = (char *) hb_xgrab( pLeft->ulLength + pRight->ulLength + 1 );
      memcpy( szString, pLeft->value.asString.string, pLeft->ulLength );
      memcpy( szString + pLeft->ulLength, pRight->value.asString.string, pRight->ulLength );
      pLeft->ulLength += pRight->ulLength;
      szString[ pLeft->ulLength ] = '\0';
      pLeft->value.asString.string = szString;
      pLeft->value.asString.dealloc = TRUE;
   }
   HB_COMP_EXPR_FREE( pRight );
   return pLeft;
}

static HB_EXPR_PTR hb_compExprReduceMinusStrings( HB_EXPR_PTR pLeft, HB_EXPR_PTR pRight, HB_COMP_DECL )
{
   char * szText = pLeft->value.asString.string;
   ULONG ulLen = pLeft->ulLength;

   while( ulLen && szText[ ulLen - 1 ] == ' ' )
      --ulLen;

   if( pLeft->value.asString.dealloc )
   {
      pLeft->value.asString.string = (char *) hb_xrealloc( pLeft->value.asString.string, pLeft->ulLength + pRight->ulLength + 1 );
      memcpy( pLeft->value.asString.string + ulLen,
              pRight->value.asString.string, pRight->ulLength );
      memset( pLeft->value.asString.string + ulLen + pRight->ulLength, ' ',
              pLeft->ulLength - ulLen );
      pLeft->ulLength += pRight->ulLength;
      pLeft->value.asString.string[ pLeft->ulLength ] = '\0';
   }
   else
   {
      char *szString;
      szString = (char *) hb_xgrab( pLeft->ulLength + pRight->ulLength + 1 );
      memcpy( szString, pLeft->value.asString.string, ulLen );
      memcpy( szString + ulLen, pRight->value.asString.string, pRight->ulLength );
      memset( szString + ulLen + pRight->ulLength, ' ', pLeft->ulLength - ulLen );
      pLeft->ulLength += pRight->ulLength;
      szString[ pLeft->ulLength ] = '\0';
      pLeft->value.asString.string = szString;
      pLeft->value.asString.dealloc = TRUE;
   }
   HB_COMP_EXPR_FREE( pRight );
   return pLeft;
}

HB_EXPR_PTR hb_compExprReduceMod( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == HB_ET_NUMERIC && pRight->ExprType == HB_ET_NUMERIC )
   {
      switch( pLeft->value.asNum.NumType & pRight->value.asNum.NumType )
      {
         case HB_ET_LONG:
            if( pRight->value.asNum.val.l )
            {
               pSelf->value.asNum.val.l = pLeft->value.asNum.val.l % pRight->value.asNum.val.l;
               pSelf->value.asNum.bDec = 0;
               pSelf->value.asNum.NumType = HB_ET_LONG;
               pSelf->ExprType = HB_ET_NUMERIC;
               pSelf->ValType  = HB_EV_NUMERIC;
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
            }
            break;

         default:
            if( HB_SUPPORT_HARBOUR )
            {
               double dValue, dDivisor;

               dDivisor = pRight->value.asNum.NumType == HB_ET_LONG ?
                          pRight->value.asNum.val.l :
                          pRight->value.asNum.val.d;
               if( dDivisor )
               {
                  dValue = pLeft->value.asNum.NumType == HB_ET_LONG ?
                           pLeft->value.asNum.val.l :
                           pLeft->value.asNum.val.d;
                  pSelf->value.asNum.val.d = fmod( dValue, dDivisor );
                  pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
                  pSelf->value.asNum.bDec = HB_DEFAULT_DECIMALS;
                  pSelf->value.asNum.NumType = HB_ET_DOUBLE;
                  pSelf->ExprType = HB_ET_NUMERIC;
                  pSelf->ValType  = HB_EV_NUMERIC;
                  HB_COMP_EXPR_FREE( pLeft );
                  HB_COMP_EXPR_FREE( pRight );
               }
            }
      }
   }
   else
   {
      /* TODO: Check for incompatible types e.g.  3 % "txt"
      */
   }
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceDiv( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == HB_ET_NUMERIC && pRight->ExprType == HB_ET_NUMERIC )
   {
      BYTE bType = ( pLeft->value.asNum.NumType & pRight->value.asNum.NumType );

      switch( bType )
      {
         case HB_ET_LONG:

            if( pRight->value.asNum.val.l )
            {
               if( pLeft->value.asNum.val.l % pRight->value.asNum.val.l == 0 )
               {
                  /* Return integer results as long */
                  pSelf->value.asNum.val.l = pLeft->value.asNum.val.l / pRight->value.asNum.val.l;
                  pSelf->value.asNum.bDec = 0;
                  pSelf->value.asNum.NumType = HB_ET_LONG;
               }
               else
               {
                  /* Return non-integer results as double */
                  pSelf->value.asNum.val.d = ( double ) pLeft->value.asNum.val.l / ( double ) pRight->value.asNum.val.l;
                  pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
                  pSelf->value.asNum.bDec = HB_DEFAULT_DECIMALS;
                  pSelf->value.asNum.NumType = HB_ET_DOUBLE;
               }
               pSelf->ExprType = HB_ET_NUMERIC;
            }
            break;

         case HB_ET_DOUBLE:

            if( pRight->value.asNum.val.d != 0.0 )
            {
               pSelf->value.asNum.val.d = pLeft->value.asNum.val.d / pRight->value.asNum.val.d;
               pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
               pSelf->value.asNum.bDec = HB_DEFAULT_DECIMALS;
               pSelf->value.asNum.NumType = HB_ET_DOUBLE;
               pSelf->ExprType = HB_ET_NUMERIC;
            }
            break;

         default:

            if( pLeft->value.asNum.NumType == HB_ET_DOUBLE )
            {
               if( pRight->value.asNum.val.l )
               {
                  pSelf->value.asNum.val.d = pLeft->value.asNum.val.d / ( double ) pRight->value.asNum.val.l;
                  pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
                  pSelf->value.asNum.bDec = HB_DEFAULT_DECIMALS;
               }
            }
            else
            {
               if( pRight->value.asNum.val.d != 0.0 )
               {
                  pSelf->value.asNum.val.d = ( double ) pLeft->value.asNum.val.l / pRight->value.asNum.val.d;
                  pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
                  pSelf->value.asNum.bDec = HB_DEFAULT_DECIMALS;
               }
            }

            pSelf->value.asNum.NumType = HB_ET_DOUBLE;
            pSelf->ExprType = HB_ET_NUMERIC;

      } /* switch bType */

      if( pSelf->ExprType == HB_ET_NUMERIC )
      {
         /* The expression was reduced - delete old components */
         pSelf->ValType = HB_EV_NUMERIC;
         HB_COMP_EXPR_FREE( pLeft );
         HB_COMP_EXPR_FREE( pRight );
      }
   }
   else
   {
      /* TODO: Check for incompatible types e.g.  3 / "txt"
      */
   }
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceMult( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == HB_ET_NUMERIC && pRight->ExprType == HB_ET_NUMERIC )
   {
      BYTE bType = ( pLeft->value.asNum.NumType & pRight->value.asNum.NumType );

      switch( bType )
      {
         case HB_ET_LONG:
         {
            HB_MAXDBL dVal = ( HB_MAXDBL ) pLeft->value.asNum.val.l * ( HB_MAXDBL ) pRight->value.asNum.val.l;

            if( HB_DBL_LIM_LONG( dVal ) )
            {
               pSelf->value.asNum.val.l = pLeft->value.asNum.val.l * pRight->value.asNum.val.l;
               pSelf->value.asNum.bDec = 0;
               pSelf->value.asNum.NumType = HB_ET_LONG;
            }
            else
            {
               pSelf->value.asNum.val.d = ( double ) dVal;
               pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
               pSelf->value.asNum.bDec = 0;
               pSelf->value.asNum.NumType = HB_ET_DOUBLE;
            }

            break;
         }

         case HB_ET_DOUBLE:
         {
            pSelf->value.asNum.val.d = pLeft->value.asNum.val.d * pRight->value.asNum.val.d;
            pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
            pSelf->value.asNum.bDec = ( UCHAR ) ( pLeft->value.asNum.bDec + pRight->value.asNum.bDec );
            pSelf->value.asNum.NumType = HB_ET_DOUBLE;

            break;
         }

         default:
         {
            if( pLeft->value.asNum.NumType == HB_ET_DOUBLE )
            {
               pSelf->value.asNum.val.d = pLeft->value.asNum.val.d * ( double ) pRight->value.asNum.val.l;
               pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
               pSelf->value.asNum.bDec = pLeft->value.asNum.bDec;
            }
            else
            {
               pSelf->value.asNum.val.d = ( double ) pLeft->value.asNum.val.l * pRight->value.asNum.val.d;
               pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
               pSelf->value.asNum.bDec = pRight->value.asNum.bDec;
            }
            pSelf->value.asNum.NumType = HB_ET_DOUBLE;
         }
      }
      pSelf->ExprType = HB_ET_NUMERIC;
      pSelf->ValType  = HB_EV_NUMERIC;
      HB_COMP_EXPR_FREE( pLeft );
      HB_COMP_EXPR_FREE( pRight );
   }
   else
   {
      /* TODO: Check for incompatible types e.g. 3 * "txt"
      */
   }
   return pSelf;
}

HB_EXPR_PTR hb_compExprReducePower( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == HB_ET_NUMERIC && pRight->ExprType == HB_ET_NUMERIC )
   {
      BYTE bType = ( pLeft->value.asNum.NumType & pRight->value.asNum.NumType );

      switch( bType )
      {
         case HB_ET_LONG:
            pSelf->value.asNum.val.d = pow( ( double ) pLeft->value.asNum.val.l,
                                            ( double ) pRight->value.asNum.val.l );
            break;

         case HB_ET_DOUBLE:
            pSelf->value.asNum.val.d = pow( pLeft->value.asNum.val.d,
                                            pRight->value.asNum.val.d );
            break;

         default:
            if( pLeft->value.asNum.NumType == HB_ET_DOUBLE )
               pSelf->value.asNum.val.d = pow( pLeft->value.asNum.val.d,
                                               ( double ) pRight->value.asNum.val.l );
            else
               pSelf->value.asNum.val.d = pow( ( double ) pLeft->value.asNum.val.l,
                                               pRight->value.asNum.val.d );
            break;
      }
      pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
      pSelf->value.asNum.bDec = HB_DEFAULT_DECIMALS;
      pSelf->value.asNum.NumType = HB_ET_DOUBLE;
      pSelf->ExprType = HB_ET_NUMERIC;
      pSelf->ValType  = HB_EV_NUMERIC;
      HB_COMP_EXPR_FREE( pLeft );
      HB_COMP_EXPR_FREE( pRight );
   }
   else
   {
      /* TODO: Check for incompatible types e.g. 3 * "txt"
      */
   }
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceMinus( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == HB_ET_NUMERIC && pRight->ExprType == HB_ET_NUMERIC )
   {
      BYTE bType = ( pLeft->value.asNum.NumType & pRight->value.asNum.NumType );

      switch( bType )
      {
         case HB_ET_LONG:
         {
            HB_MAXDBL dVal = ( HB_MAXDBL ) pLeft->value.asNum.val.l - ( HB_MAXDBL ) pRight->value.asNum.val.l;

            if( HB_DBL_LIM_LONG( dVal ) )
            {
               pSelf->value.asNum.val.l = pLeft->value.asNum.val.l - pRight->value.asNum.val.l;
               pSelf->value.asNum.bDec = 0;
               pSelf->value.asNum.NumType = HB_ET_LONG;
            }
            else
            {
               pSelf->value.asNum.val.d = ( double ) dVal;
               pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
               pSelf->value.asNum.bDec = 0;
               pSelf->value.asNum.NumType = HB_ET_DOUBLE;
            }

            break;
         }

         case HB_ET_DOUBLE:
         {
            pSelf->value.asNum.val.d = pLeft->value.asNum.val.d - pRight->value.asNum.val.d;
            pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
            if( pLeft->value.asNum.bDec < pRight->value.asNum.bDec )
               pSelf->value.asNum.bDec = pRight->value.asNum.bDec;
            else
               pSelf->value.asNum.bDec = pLeft->value.asNum.bDec;
            pSelf->value.asNum.NumType = HB_ET_DOUBLE;

            break;
         }

         default:
         {
            if( pLeft->value.asNum.NumType == HB_ET_DOUBLE )
            {
               pSelf->value.asNum.val.d = pLeft->value.asNum.val.d - ( double ) pRight->value.asNum.val.l;
               pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
               pSelf->value.asNum.bDec = pLeft->value.asNum.bDec;
            }
            else
            {
               pSelf->value.asNum.val.d = ( double ) pLeft->value.asNum.val.l - pRight->value.asNum.val.d;
               pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
               pSelf->value.asNum.bDec = pRight->value.asNum.bDec;
            }
            pSelf->value.asNum.NumType = HB_ET_DOUBLE;
         }
      }
      pSelf->ExprType = HB_ET_NUMERIC;
      pSelf->ValType  = HB_EV_NUMERIC;
      HB_COMP_EXPR_FREE( pLeft );
      HB_COMP_EXPR_FREE( pRight );
   }
   else if( pLeft->ExprType == HB_ET_DATE && pRight->ExprType == HB_ET_DATE )
   {
      pSelf->value.asNum.val.l = pLeft->value.asNum.val.l - pRight->value.asNum.val.l;
      pSelf->value.asNum.bDec = 0;
      pSelf->value.asNum.NumType = HB_ET_LONG;
      pSelf->ExprType = HB_ET_NUMERIC;
      pSelf->ValType  = HB_EV_NUMERIC;
      HB_COMP_EXPR_FREE( pLeft );
      HB_COMP_EXPR_FREE( pRight );
   }
   else if( pLeft->ExprType == HB_ET_DATE && pRight->ExprType == HB_ET_NUMERIC )
   {
      if( pRight->value.asNum.NumType == HB_ET_LONG )
      {
         pSelf->value.asNum.val.l = pLeft->value.asNum.val.l - pRight->value.asNum.val.l;
      }
      else
      {
         pSelf->value.asNum.val.l = pLeft->value.asNum.val.l - ( HB_LONG ) pRight->value.asNum.val.d;
      }
      pSelf->ExprType = HB_ET_DATE;
      pSelf->ValType  = HB_EV_DATE;
      HB_COMP_EXPR_FREE( pLeft );
      HB_COMP_EXPR_FREE( pRight );
   }
   else if( pLeft->ExprType == HB_ET_STRING && pRight->ExprType == HB_ET_STRING )
   {
      if( pRight->ulLength == 0 )
      {
         pSelf->ExprType = HB_ET_NONE; /* suppress deletion of operator components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pLeft;
         HB_COMP_EXPR_FREE( pRight );
      }
      else if( pLeft->ulLength == 0 )
      {
         pSelf->ExprType = HB_ET_NONE; /* suppress deletion of operator components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pRight;
         HB_COMP_EXPR_FREE( pLeft );
      }
      else
      {
         /* Do not reduce strings with the macro operator '&'
         */
         char * szText = pLeft->value.asString.string;
         ULONG ulLen = pLeft->ulLength;
         BOOL fReduce = TRUE;

         while( ulLen && szText[ ulLen - 1 ] == ' ' )
            --ulLen;

         while( ulLen-- )
         {
            if( *szText++ == '&' )
            {
               char ch = ulLen ? *szText : *pRight->value.asString.string;
               if( ( ch >= 'A' && ch <= 'Z' ) ||
                   ( ch >= 'a' && ch <= 'z' ) || ch == '_' ||
                   ! HB_SUPPORT_HARBOUR )
               {
                  fReduce = FALSE;
                  break;
               }
            }
         }

         if( fReduce )
         {
            pSelf->ExprType = HB_ET_NONE; /* suppress deletion of operator components */
            HB_COMP_EXPR_FREE( pSelf );
            pSelf = hb_compExprReduceMinusStrings( pLeft, pRight, HB_COMP_PARAM );
         }
      }
   }
   else
   {
      /* TODO: Check for incompatible types e.g. "txt" - 3
      */
   }
   return pSelf;
}

HB_EXPR_PTR hb_compExprReducePlus( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == HB_ET_NUMERIC )
   {
      if( pRight->ExprType == HB_ET_NUMERIC )
      {
         BYTE bType = ( pLeft->value.asNum.NumType & pRight->value.asNum.NumType );

         switch( bType )
         {
            case HB_ET_LONG:
            {
               HB_MAXDBL dVal = ( HB_MAXDBL ) pLeft->value.asNum.val.l + ( HB_MAXDBL ) pRight->value.asNum.val.l;

               if( HB_DBL_LIM_LONG( dVal ) )
               {
                  pSelf->value.asNum.val.l = pLeft->value.asNum.val.l + pRight->value.asNum.val.l;
                  pSelf->value.asNum.bDec = 0;
                  pSelf->value.asNum.NumType = HB_ET_LONG;
               }
               else
               {
                  pSelf->value.asNum.val.d = ( double ) dVal;
                  pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
                  pSelf->value.asNum.bDec = 0;
                  pSelf->value.asNum.NumType = HB_ET_DOUBLE;
               }
               break;
            }

            case HB_ET_DOUBLE:
               pSelf->value.asNum.val.d = pLeft->value.asNum.val.d + pRight->value.asNum.val.d;
               pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
               if( pLeft->value.asNum.bDec < pRight->value.asNum.bDec )
                  pSelf->value.asNum.bDec = pRight->value.asNum.bDec;
               else
                  pSelf->value.asNum.bDec = pLeft->value.asNum.bDec;
               pSelf->value.asNum.NumType = HB_ET_DOUBLE;
               break;

            default:
               if( pLeft->value.asNum.NumType == HB_ET_DOUBLE )
               {
                  pSelf->value.asNum.val.d = pLeft->value.asNum.val.d + ( double ) pRight->value.asNum.val.l;
                  pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
                  pSelf->value.asNum.bDec = pLeft->value.asNum.bDec;
               }
               else
               {
                  pSelf->value.asNum.val.d = ( double ) pLeft->value.asNum.val.l + pRight->value.asNum.val.d;
                  pSelf->value.asNum.bWidth = HB_DEFAULT_WIDTH;
                  pSelf->value.asNum.bDec = pRight->value.asNum.bDec;
               }
               pSelf->value.asNum.NumType = HB_ET_DOUBLE;
         }
         pSelf->ExprType = HB_ET_NUMERIC;
         pSelf->ValType  = HB_EV_NUMERIC;
         HB_COMP_EXPR_FREE( pLeft );
         HB_COMP_EXPR_FREE( pRight );
      }
      else if( pRight->ExprType == HB_ET_DATE )
      {
         if( pLeft->value.asNum.NumType == HB_ET_LONG )
            pSelf->value.asNum.val.l = pRight->value.asNum.val.l + pLeft->value.asNum.val.l;
         else
            pSelf->value.asNum.val.l = pRight->value.asNum.val.l + ( HB_LONG ) pLeft->value.asNum.val.d;
         pSelf->ExprType = HB_ET_DATE;
         pSelf->ValType  = HB_EV_DATE;
         HB_COMP_EXPR_FREE( pLeft );
         HB_COMP_EXPR_FREE( pRight );
      }
      else if( HB_SUPPORT_HARBOUR &&
               ( pLeft->value.asNum.NumType == HB_ET_LONG ?
                 pLeft->value.asNum.val.l == 0 :
                 pLeft->value.asNum.val.d == 0 ) )
      {
         /* NOTE: This will not generate a runtime error if incompatible
          * data type is used
          */
         pSelf->ExprType = HB_ET_NONE; /* suppress deletion of operator components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pRight;
         HB_COMP_EXPR_FREE( pLeft );
      }
      else
      {
         /* TODO: Check for incompatible types e.g. "txt" + 3
         */
      }
   }
   else if( pRight->ExprType == HB_ET_NUMERIC )
   {
      if( pLeft->ExprType == HB_ET_DATE )
      {
         if( pRight->value.asNum.NumType == HB_ET_LONG )
            pSelf->value.asNum.val.l = pLeft->value.asNum.val.l + pRight->value.asNum.val.l;
         else
            pSelf->value.asNum.val.l = pLeft->value.asNum.val.l + ( HB_LONG ) pRight->value.asNum.val.d;
         pSelf->ExprType = HB_ET_DATE;
         pSelf->ValType  = HB_EV_DATE;
         HB_COMP_EXPR_FREE( pLeft );
         HB_COMP_EXPR_FREE( pRight );
      }
      else if( HB_SUPPORT_HARBOUR &&
               ( pRight->value.asNum.NumType == HB_ET_LONG ?
                 pRight->value.asNum.val.l == 0 :
                 pRight->value.asNum.val.d == 0 ) )
      {
         /* NOTE: This will not generate a runtime error if incompatible
          * data type is used
          */
         pSelf->ExprType = HB_ET_NONE; /* suppress deletion of operator components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pLeft;
         HB_COMP_EXPR_FREE( pRight );
      }
      else
      {
         /* TODO: Check for incompatible types e.g. "txt" + 3
         */
      }
   }
   else if( pLeft->ExprType == HB_ET_STRING && pRight->ExprType == HB_ET_STRING )
   {
      if( pRight->ulLength == 0 )
      {
         pSelf->ExprType = HB_ET_NONE; /* suppress deletion of operator components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pLeft;
         HB_COMP_EXPR_FREE( pRight );
      }
      else if( pLeft->ulLength == 0 )
      {
         pSelf->ExprType = HB_ET_NONE; /* suppress deletion of operator components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pRight;
         HB_COMP_EXPR_FREE( pLeft );
      }
      else
      {
         /* Do not reduce strings with the macro operator '&'
         */
         char * szText = pLeft->value.asString.string;
         ULONG ulLen = pLeft->ulLength;
         BOOL fReduce = TRUE;

         while( ulLen-- )
         {
            if( *szText++ == '&' )
            {
               char ch = ulLen ? *szText : *pRight->value.asString.string;
               if( ( ch >= 'A' && ch <= 'Z' ) ||
                   ( ch >= 'a' && ch <= 'z' ) || ch == '_' ||
                   ! HB_SUPPORT_HARBOUR )
               {
                  fReduce = FALSE;
                  break;
               }
            }
         }

         if( fReduce )
         {
            pSelf->ExprType = HB_ET_NONE; /* suppress deletion of operator components */
            HB_COMP_EXPR_FREE( pSelf );
            pSelf = hb_compExprReducePlusStrings( pLeft, pRight, HB_COMP_PARAM );
         }
      }
   }
   else
   {
      /* TODO: Check for incompatible types e.g. "txt" + 3
      */
   }
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceNegate( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pExpr;

   pExpr = pSelf->value.asOperator.pLeft;

   if( pExpr->ExprType == HB_ET_NUMERIC )
   {
      if( pExpr->value.asNum.NumType == HB_ET_DOUBLE )
      {
         pExpr->value.asNum.val.d = - pExpr->value.asNum.val.d;
         pExpr->value.asNum.bWidth = HB_DEFAULT_WIDTH;
      }
      else
      {
#if -HB_LONG_MAX > HB_LONG_MIN
         if( pExpr->value.asNum.val.l < -HB_LONG_MAX )
         {
            pExpr->value.asNum.NumType = HB_ET_DOUBLE;
            pExpr->value.asNum.val.d = - ( double ) pExpr->value.asNum.val.l;
            pExpr->value.asNum.bWidth = HB_DEFAULT_WIDTH;
            pExpr->value.asNum.bDec = 0;
         }
         else
#endif
         {
            pExpr->value.asNum.val.l = - pExpr->value.asNum.val.l;
            pExpr->value.asNum.bWidth = HB_DEFAULT_WIDTH;
         }
      }
      pSelf->ExprType = HB_ET_NONE; /* suppress deletion of operator components */
      HB_COMP_EXPR_FREE( pSelf );
      pSelf = pExpr;
   }

   return pSelf;
}


HB_EXPR_PTR hb_compExprReduceIN( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   if( pSelf->value.asOperator.pLeft->ExprType == pSelf->value.asOperator.pRight->ExprType &&
       pSelf->value.asOperator.pLeft->ExprType == HB_ET_STRING )
   {
      /* Both arguments are literal strings
       */
      BOOL bResult;

      /* NOTE: CA-Cl*pper has a bug where the $ operator returns .T.
       *       when an empty string is searched [vszakats]
       *
       *       But this bug exist only in compiler and CA-Cl*pper macro
       *       compiler does not have optimizer. This bug is replicated
       *       by us only when Harbour extensions in compiler (-kh) are
       *       not enabled f.e. in strict Clipper cmpatible mode (-kc)
       *       [druzus]
       */
      if( pSelf->value.asOperator.pLeft->ulLength == 0 )
         bResult = HB_COMP_PARAM->mode == HB_MODE_COMPILER &&
                   ! HB_SUPPORT_HARBOUR;
      else
         bResult = ( hb_strAt( pSelf->value.asOperator.pLeft->value.asString.string, pSelf->value.asOperator.pLeft->ulLength,
                     pSelf->value.asOperator.pRight->value.asString.string, pSelf->value.asOperator.pRight->ulLength ) != 0 );

      /* NOTE:
       * "" $ "XXX" = .T.
       * "" $ "" = .T.
       */
      HB_COMP_EXPR_FREE( pSelf->value.asOperator.pLeft );
      HB_COMP_EXPR_FREE( pSelf->value.asOperator.pRight );
      pSelf->ExprType = HB_ET_LOGICAL;
      pSelf->ValType  = HB_EV_LOGICAL;
      pSelf->value.asLogical = bResult;
   }
   /* TODO: add checking for incompatible types
    */
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceNE( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == pRight->ExprType )
   {
      switch( pLeft->ExprType )
      {
         case HB_ET_LOGICAL:
            {
               /* .F. != .T.  = .T.
               * .T. != .T.  = .F.
               * .F. != .F.  = .F.
               * .T. != .F.  = .T.
               */
               BOOL bResult = ( pLeft->value.asLogical != pRight->value.asLogical );
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         case HB_ET_STRING:
            /* NOTE: the result depends on SET EXACT setting then it
            * cannot be optimized except the case when NULL string are
            * compared - "" != "" is always FALSE regardless of EXACT
            * setting
            */
            if( ( pLeft->ulLength | pRight->ulLength ) == 0 )
            {
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = FALSE;

               /* NOTE: COMPATIBILITY: Clipper doesn't optimize this */
            }
            break;

         case HB_ET_NUMERIC:
            {
               BOOL bResult;

               switch( pLeft->value.asNum.NumType & pRight->value.asNum.NumType )
               {
                  case HB_ET_LONG:
                     bResult = ( pLeft->value.asNum.val.l != pRight->value.asNum.val.l );
                     break;
                  case HB_ET_DOUBLE:
                     bResult = ( pLeft->value.asNum.val.d != pRight->value.asNum.val.d );
                     break;
                  default:
                     {
                        if( pLeft->value.asNum.NumType == HB_ET_LONG )
                           bResult = ( pLeft->value.asNum.val.l != pRight->value.asNum.val.d );
                        else
                           bResult = ( pLeft->value.asNum.val.d != pRight->value.asNum.val.l );
                     }
                     break;
               }
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         case HB_ET_NIL:
            HB_COMP_EXPR_FREE( pLeft );
            HB_COMP_EXPR_FREE( pRight );
            pSelf->ExprType = HB_ET_LOGICAL;
            pSelf->ValType  = HB_EV_LOGICAL;
            pSelf->value.asLogical = FALSE;
            break;
      }
   }
   else if( ( pLeft->ExprType == HB_ET_NIL &&
              ( pRight->ExprType == HB_ET_NUMERIC ||
                pRight->ExprType == HB_ET_LOGICAL ||
                pRight->ExprType == HB_ET_DATE ||
                pRight->ExprType == HB_ET_STRING ||
                pRight->ExprType == HB_ET_CODEBLOCK ||
                pRight->ExprType == HB_ET_ARRAY ||
                pRight->ExprType == HB_ET_FUNREF ) ) ||
            ( pRight->ExprType == HB_ET_NIL &&
              ( pLeft->ExprType == HB_ET_NUMERIC ||
                pLeft->ExprType == HB_ET_LOGICAL ||
                pLeft->ExprType == HB_ET_DATE ||
                pLeft->ExprType == HB_ET_STRING ||
                pLeft->ExprType == HB_ET_CODEBLOCK ||
                pLeft->ExprType == HB_ET_ARRAY ||
                pLeft->ExprType == HB_ET_FUNREF ) ) )
   {
      HB_COMP_EXPR_FREE( pLeft );
      HB_COMP_EXPR_FREE( pRight );
      pSelf->ExprType = HB_ET_LOGICAL;
      pSelf->ValType  = HB_EV_LOGICAL;
      pSelf->value.asLogical = TRUE;
   }
   /* TODO: add checking of incompatible types
   else
   {
   }
   */
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceGE( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == pRight->ExprType )
      switch( pLeft->ExprType )
      {
         case HB_ET_LOGICAL:
            {
               /* .T. >= .F.  = .T.
                * .T. >= .T.  = .T.
                * .F. >= .F.  = .T.
                * .F. >= .T.  = .f.
                */
               BOOL bResult = ! ( ! pLeft->value.asLogical && pRight->value.asLogical );
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         case HB_ET_NUMERIC:
            {
               BOOL bResult;

               switch( pLeft->value.asNum.NumType & pRight->value.asNum.NumType )
               {
                  case HB_ET_LONG:
                     bResult = ( pLeft->value.asNum.val.l >= pRight->value.asNum.val.l );
                     break;
                  case HB_ET_DOUBLE:
                     bResult = ( pLeft->value.asNum.val.d >= pRight->value.asNum.val.d );
                     break;
                  default:
                     {
                        if( pLeft->value.asNum.NumType == HB_ET_LONG )
                           bResult = ( pLeft->value.asNum.val.l >= pRight->value.asNum.val.d );
                        else
                           bResult = ( pLeft->value.asNum.val.d >= pRight->value.asNum.val.l );
                     }
                     break;
               }
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

      }
   /* TODO: add checking of incompatible types
   else
   {
   }
   */
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceLE( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == pRight->ExprType )
      switch( pLeft->ExprType )
      {
         case HB_ET_LOGICAL:
            {
               /* .T. <= .F.  = .F.
                * .T. <= .T.  = .T.
                * .F. <= .F.  = .T.
                * .F. <= .T.  = .T.
                */
               BOOL bResult = ! ( pLeft->value.asLogical && ! pRight->value.asLogical );
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         case HB_ET_NUMERIC:
            {
               BOOL bResult;

               switch( pLeft->value.asNum.NumType & pRight->value.asNum.NumType )
               {
                  case HB_ET_LONG:
                     bResult = ( pLeft->value.asNum.val.l <= pRight->value.asNum.val.l );
                     break;
                  case HB_ET_DOUBLE:
                     bResult = ( pLeft->value.asNum.val.d <= pRight->value.asNum.val.d );
                     break;
                  default:
                     {
                        if( pLeft->value.asNum.NumType == HB_ET_LONG )
                           bResult = ( pLeft->value.asNum.val.l <= pRight->value.asNum.val.d );
                        else
                           bResult = ( pLeft->value.asNum.val.d <= pRight->value.asNum.val.l );
                     }
                     break;
               }
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

      }
   /* TODO: add checking of incompatible types
   else
   {
   }
   */
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceGT( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == pRight->ExprType )
      switch( pLeft->ExprType )
      {
         case HB_ET_LOGICAL:
            {
               /* .T. > .F.  = .T.
                * .T. > .T.  = .F.
                * .F. > .F.  = .F.
                * .F. > .T.  = .F.
                */
               BOOL bResult = ( pLeft->value.asLogical && ! pRight->value.asLogical );
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         case HB_ET_NUMERIC:
            {
               BOOL bResult;

               switch( pLeft->value.asNum.NumType & pRight->value.asNum.NumType )
               {
                  case HB_ET_LONG:
                     bResult = ( pLeft->value.asNum.val.l > pRight->value.asNum.val.l );
                     break;
                  case HB_ET_DOUBLE:
                     bResult = ( pLeft->value.asNum.val.d > pRight->value.asNum.val.d );
                     break;
                  default:
                     {
                        if( pLeft->value.asNum.NumType == HB_ET_LONG )
                           bResult = ( pLeft->value.asNum.val.l > pRight->value.asNum.val.d );
                        else
                           bResult = ( pLeft->value.asNum.val.d > pRight->value.asNum.val.l );
                     }
                     break;
               }
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

      }
   /* TODO: add checking of incompatible types
   else
   {
   }
   */
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceLT( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == pRight->ExprType )
      switch( pLeft->ExprType )
      {
         case HB_ET_LOGICAL:
            {
               /* .F. < .T.  = .T.
                * .T. < .T.  = .F.
                * .F. < .F.  = .F.
                * .T. < .F.  = .F.
                */
               BOOL bResult = ( ! pLeft->value.asLogical && pRight->value.asLogical );
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         case HB_ET_NUMERIC:
            {
               BOOL bResult;

               switch( pLeft->value.asNum.NumType & pRight->value.asNum.NumType )
               {
                  case HB_ET_LONG:
                     bResult = ( pLeft->value.asNum.val.l < pRight->value.asNum.val.l );
                     break;
                  case HB_ET_DOUBLE:
                     bResult = ( pLeft->value.asNum.val.d < pRight->value.asNum.val.d );
                     break;
                  default:
                     {
                        if( pLeft->value.asNum.NumType == HB_ET_LONG )
                           bResult = ( pLeft->value.asNum.val.l < pRight->value.asNum.val.d );
                        else
                           bResult = ( pLeft->value.asNum.val.d < pRight->value.asNum.val.l );
                     }
                     break;
               }
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         default:
            break;
      }
   /* TODO: add checking of incompatible types
   else
   {
   }
   */
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceEQ( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == pRight->ExprType )
   {
      switch( pLeft->ExprType )
      {
         case HB_ET_LOGICAL:
            {
               BOOL bResult = ( pLeft->value.asLogical == pRight->value.asLogical );
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         case HB_ET_STRING:
            /* NOTE: when not exact comparison (==) is used 
             * the result depends on SET EXACT setting then it
             * cannot be optimized except the case when NULL string are
             * compared - "" = "" is always TRUE regardless of EXACT
             * setting
             */
            if( pSelf->ExprType == HB_EO_EQ ||
                ( pLeft->ulLength | pRight->ulLength ) == 0 )
            {
               BOOL bResult = pLeft->ulLength == pRight->ulLength &&
                              memcmp( pLeft->value.asString.string,
                                      pRight->value.asString.string,
                                      pLeft->ulLength ) == 0;
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         case HB_ET_NUMERIC:
            {
               BOOL bResult;

               switch( pLeft->value.asNum.NumType & pRight->value.asNum.NumType )
               {
                  case HB_ET_LONG:
                     bResult = ( pLeft->value.asNum.val.l == pRight->value.asNum.val.l );
                     break;
                  case HB_ET_DOUBLE:
                     bResult = ( pLeft->value.asNum.val.d == pRight->value.asNum.val.d );
                     break;
                  default:
                     if( pLeft->value.asNum.NumType == HB_ET_LONG )
                        bResult = ( pLeft->value.asNum.val.l == pRight->value.asNum.val.d );
                     else
                        bResult = ( pLeft->value.asNum.val.d == pRight->value.asNum.val.l );
                     break;
               }
               HB_COMP_EXPR_FREE( pLeft );
               HB_COMP_EXPR_FREE( pRight );
               pSelf->ExprType = HB_ET_LOGICAL;
               pSelf->ValType  = HB_EV_LOGICAL;
               pSelf->value.asLogical = bResult;
            }
            break;

         case HB_ET_NIL:
            HB_COMP_EXPR_FREE( pLeft );
            HB_COMP_EXPR_FREE( pRight );
            pSelf->ExprType = HB_ET_LOGICAL;
            pSelf->ValType  = HB_EV_LOGICAL;
            pSelf->value.asLogical = TRUE;
            break;
      }
   }
   else if( ( pLeft->ExprType == HB_ET_NIL &&
              ( pRight->ExprType == HB_ET_NUMERIC ||
                pRight->ExprType == HB_ET_LOGICAL ||
                pRight->ExprType == HB_ET_DATE ||
                pRight->ExprType == HB_ET_STRING ||
                pRight->ExprType == HB_ET_CODEBLOCK ||
                pRight->ExprType == HB_ET_ARRAY ||
                pRight->ExprType == HB_ET_FUNREF ) ) ||
            ( pRight->ExprType == HB_ET_NIL &&
              ( pLeft->ExprType == HB_ET_NUMERIC ||
                pLeft->ExprType == HB_ET_LOGICAL ||
                pLeft->ExprType == HB_ET_DATE ||
                pLeft->ExprType == HB_ET_STRING ||
                pLeft->ExprType == HB_ET_CODEBLOCK ||
                pLeft->ExprType == HB_ET_ARRAY ||
                pLeft->ExprType == HB_ET_FUNREF ) ) )
   {
      HB_COMP_EXPR_FREE( pLeft );
      HB_COMP_EXPR_FREE( pRight );
      pSelf->ExprType = HB_ET_LOGICAL;
      pSelf->ValType  = HB_EV_LOGICAL;
      pSelf->value.asLogical = FALSE;
   }
   /* TODO: add checking of incompatible types
   else
   {
   }
   */
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceAnd( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == HB_ET_LOGICAL && pRight->ExprType == HB_ET_LOGICAL )
   {
      BOOL bResult;

      bResult = pLeft->value.asLogical && pRight->value.asLogical;
      HB_COMP_EXPR_FREE( pLeft );
      HB_COMP_EXPR_FREE( pRight );
      pSelf->ExprType = HB_ET_LOGICAL;
      pSelf->ValType  = HB_EV_LOGICAL;
      pSelf->value.asLogical = bResult;
   }
   else if( pLeft->ExprType == HB_ET_LOGICAL &&
            HB_COMP_ISSUPPORTED( HB_COMPFLAG_SHORTCUTS ) )
   {
      if( pLeft->value.asLogical )
      {
         /* .T. .AND. expr => expr
          */
         HB_COMP_EXPR_FREE( pLeft );
         pSelf->ExprType = HB_ET_NONE;    /* don't delete expression components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pRight;
      }
      else
      {
         /* .F. .AND. expr => .F.
          */
         HB_COMP_EXPR_FREE( pLeft );
         HB_COMP_EXPR_FREE( pRight );         /* discard expression */
         pSelf->ExprType = HB_ET_LOGICAL;
         pSelf->ValType  = HB_EV_LOGICAL;
         pSelf->value.asLogical = FALSE;
      }
   }
   else if( pRight->ExprType == HB_ET_LOGICAL &&
            HB_COMP_ISSUPPORTED( HB_COMPFLAG_SHORTCUTS ) )
   {
      if( pRight->value.asLogical )
      {
         /* expr .AND. .T. => expr
          */
         HB_COMP_EXPR_FREE( pRight );
         pSelf->ExprType = HB_ET_NONE;    /* don't delete expression components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pLeft;
      }
      else
      {
         /* expr .AND. .F. => .F.
          */
         HB_COMP_EXPR_FREE( pLeft );      /* discard expression */
         HB_COMP_EXPR_FREE( pRight );
         pSelf->ExprType = HB_ET_LOGICAL;
         pSelf->ValType  = HB_EV_LOGICAL;
         pSelf->value.asLogical = FALSE;
      }
   }
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceOr( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pLeft, pRight;

   pLeft  = pSelf->value.asOperator.pLeft;
   pRight = pSelf->value.asOperator.pRight;

   if( pLeft->ExprType == HB_ET_LOGICAL && pRight->ExprType == HB_ET_LOGICAL )
   {
      BOOL bResult;

      bResult = pLeft->value.asLogical || pRight->value.asLogical;
      HB_COMP_EXPR_FREE( pLeft );
      HB_COMP_EXPR_FREE( pRight );
      pSelf->ExprType = HB_ET_LOGICAL;
      pSelf->ValType  = HB_EV_LOGICAL;
      pSelf->value.asLogical = bResult;
   }
   else if( pLeft->ExprType == HB_ET_LOGICAL &&
            HB_COMP_ISSUPPORTED( HB_COMPFLAG_SHORTCUTS ) )
   {
      if( pLeft->value.asLogical )
      {
         /* .T. .OR. expr => .T.
          */
         HB_COMP_EXPR_FREE( pLeft );
         HB_COMP_EXPR_FREE( pRight );     /* discard expression */
         pSelf->ExprType = HB_ET_LOGICAL;
         pSelf->ValType  = HB_EV_LOGICAL;
         pSelf->value.asLogical = TRUE;
      }
      else
      {
         /* .F. .OR. expr => expr
          */
         HB_COMP_EXPR_FREE( pLeft );
         pSelf->ExprType = HB_ET_NONE;    /* don't delete expression components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pRight;
      }
   }
   else if( pRight->ExprType == HB_ET_LOGICAL &&
            HB_COMP_ISSUPPORTED( HB_COMPFLAG_SHORTCUTS ) )
   {
      if( pRight->value.asLogical )
      {
         /* expr .OR. .T. => .T.
          */
         HB_COMP_EXPR_FREE( pLeft );      /* discard expression */
         HB_COMP_EXPR_FREE( pRight );
         pSelf->ExprType = HB_ET_LOGICAL;
         pSelf->ValType  = HB_EV_LOGICAL;
         pSelf->value.asLogical = TRUE;
      }
      else
      {
         /* expr .OR. .F. => expr
          */
         HB_COMP_EXPR_FREE( pRight );
         pSelf->ExprType = HB_ET_NONE;    /* don't delete expression components */
         HB_COMP_EXPR_FREE( pSelf );
         pSelf = pLeft;
      }
   }
   return pSelf;
}

HB_EXPR_PTR hb_compExprReduceIIF( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pExpr;

   /* get conditional expression */
   pExpr = pSelf->value.asList.pExprList;

   if( pExpr->ExprType == HB_ET_LOGICAL )
   {
      /* the condition was reduced to a logical value: .T. or .F.
      */
      if( pExpr->value.asLogical )
      {
         /* .T. was specified
         */
         pExpr = pExpr->pNext;   /* skip to TRUE expression */
         /* delete condition  - it is no longer needed
            */
         HB_COMP_EXPR_FREE( pSelf->value.asList.pExprList );
         /* assign NULL to a start of expressions list to suppress
          * deletion of expression's components - we are deleting them
          * here
          */
         pSelf->value.asList.pExprList = NULL;
         HB_COMP_EXPR_FREE( pSelf );
         /* store the TRUE expression as a result of reduction
          */
         pSelf = pExpr;
         pExpr = pExpr->pNext;     /* skip to FALSE expression */
         HB_COMP_EXPR_FREE( pExpr );      /* delete FALSE expr */
         pSelf->pNext = NULL;
      }
      else
      {
         /* .F. was specified
         */
         pExpr = pExpr->pNext;   /* skip to TRUE expression */
         /* delete condition  - it is no longer needed
          */
         HB_COMP_EXPR_FREE( pSelf->value.asList.pExprList );
         /* assign NULL to a start of expressions list to suppress
          * deletion of expression's components - we are deleting them
          * here
          */
         pSelf->value.asList.pExprList = NULL;
         HB_COMP_EXPR_FREE( pSelf );
         /* store the FALSE expression as a result of reduction
            */
         pSelf = pExpr->pNext;
         HB_COMP_EXPR_FREE( pExpr );      /* delete TRUE expr */
         pSelf->pNext = NULL;
      }

      /* this will cause warning when IIF is used as statement */
      /*
      if( pSelf->ExprType == HB_ET_NONE )
      {
         pSelf->ExprType = HB_ET_NIL;
         pSelf->ValType = HB_EV_NIL;
      }
      */
   }
   /* check if valid expression is passed
   */
   else if( pExpr->ExprType == HB_ET_NIL ||
            pExpr->ExprType == HB_ET_NUMERIC ||
            pExpr->ExprType == HB_ET_DATE ||
            pExpr->ExprType == HB_ET_STRING ||
            pExpr->ExprType == HB_ET_CODEBLOCK ||
            pExpr->ExprType == HB_ET_ARRAY ||
            pExpr->ExprType == HB_ET_VARREF ||
            pExpr->ExprType == HB_ET_REFERENCE ||
            pExpr->ExprType == HB_ET_FUNREF )
   {
      HB_COMP_ERROR_TYPE( pExpr );
   }
   return pSelf;
}

/* replace the list containing a single expression with a simple expression
 * - strips parenthesis
 *  ( EXPR ) -> EXPR
 */
HB_EXPR_PTR hb_compExprListStrip( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   while( pSelf->ExprType == HB_ET_LIST &&
          pSelf->value.asList.pExprList->ExprType <= HB_ET_VARIABLE &&
          hb_compExprListLen( pSelf ) == 1 )
   {
      /* replace the list with a simple expression
       *  ( EXPR ) -> EXPR
       */
      HB_EXPR_PTR pExpr = pSelf;

      pSelf = pSelf->value.asList.pExprList;
      pExpr->value.asList.pExprList = NULL;
      HB_COMP_EXPR_FREE( pExpr );
   }

   return pSelf;
}

BOOL hb_compExprReduceAT( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pSub  = pParms->value.asList.pExprList;
   HB_EXPR_PTR pText = pSub->pNext;
   HB_EXPR_PTR pReduced;

   if( pSub->ExprType == HB_ET_STRING && pText->ExprType == HB_ET_STRING )
   {
      /* NOTE: CA-Cl*pper has a bug in AT("",cText) compile time
       *       optimization and always set 1 as result in such cses.
       *       This bug exist only in compiler and CA-Cl*pper macro
       *       compiler does not have optimizer. This bug is replicated
       *       by us only when Harbour extensions in compiler (-kh) are
       *       not enabled f.e. in strict Clipper cmpatible mode (-kc)
       *       [druzus]
       */
      if( pSub->ulLength == 0 )
      {
         pReduced = hb_compExprNewLong( ( HB_COMP_PARAM->mode == HB_MODE_COMPILER &&
                                          ! HB_SUPPORT_HARBOUR ) ? 1 : 0, HB_COMP_PARAM );
      }
      else
      {
         pReduced = hb_compExprNewLong( hb_strAt( pSub->value.asString.string,
                               pSub->ulLength, pText->value.asString.string,
                               pText->ulLength ), HB_COMP_PARAM );
      }

      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pParms );

      memcpy( pSelf, pReduced, sizeof( HB_EXPR ) );
      HB_COMP_EXPR_CLEAR( pReduced );
      return TRUE;
   }
   else
      return FALSE;
}

BOOL hb_compExprReduceCHR( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pArg = pParms->value.asList.pExprList;

   /* try to change it into a string */
   if( pArg->ExprType == HB_ET_NUMERIC )
   {
      /* NOTE: CA-Cl*pper's compiler optimizer will be wrong for those
       *       CHR() cases where the passed parameter is a constant which
       *       can be divided by 256 but it's not zero, in this case it
       *       will return an empty string instead of a Chr(0). [vszakats]
       *
       *       But this bug exist only in compiler and CA-Cl*pper macro
       *       compiler does not have optimizer. This bug is replicated
       *       by us only when Harbour extensions in compiler (-kh) are
       *       not enabled f.e. in strict Clipper cmpatible mode (-kc)
       *       [druzus]
       */

      HB_EXPR_PTR pExpr = HB_COMP_EXPR_NEW( HB_ET_STRING );

      pExpr->ValType = HB_EV_STRING;
      if( pArg->value.asNum.NumType == HB_ET_LONG )
      {
         if( HB_COMP_PARAM->mode == HB_MODE_COMPILER &&
             ! HB_SUPPORT_HARBOUR &&
             ( pArg->value.asNum.val.l & 0xff ) == 0 &&
               pArg->value.asNum.val.l != 0 )
         {
            pExpr->value.asString.string = ( char * ) "";
            pExpr->value.asString.dealloc = FALSE;
            pExpr->ulLength = 0;
         }
         else
         {
            pExpr->value.asString.string = ( char * ) hb_szAscii[ ( int ) pArg->value.asNum.val.l & 0xff ];
            pExpr->value.asString.dealloc = FALSE;
            pExpr->ulLength = 1;
         }
      }
      else
      {
         pExpr->value.asString.string = ( char * ) hb_szAscii[ ( unsigned int ) pArg->value.asNum.val.d & 0xff ];
         pExpr->value.asString.dealloc = FALSE;
         pExpr->ulLength = 1;
      }

      HB_COMP_EXPR_FREE( pParms );
      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
      memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
      HB_COMP_EXPR_CLEAR( pExpr );
      return TRUE;
   }

   return FALSE;
}

BOOL hb_compExprReduceLEN( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pArg = pParms->value.asList.pExprList;

   /* TOFIX: do not optimize when array/hash args have user expressions */
   if( pArg->ExprType == HB_ET_STRING || pArg->ExprType == HB_ET_ARRAY ||
       pArg->ExprType == HB_ET_HASH )
   {
      HB_EXPR_PTR pExpr = hb_compExprNewLong( pArg->ExprType == HB_ET_HASH ?
                        pArg->ulLength >> 1 : pArg->ulLength, HB_COMP_PARAM );

      HB_COMP_EXPR_FREE( pParms );
      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
      memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
      HB_COMP_EXPR_CLEAR( pExpr );
      return TRUE;
   }
   return FALSE;
}

BOOL hb_compExprReduceEMPTY( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pArg = pParms->value.asList.pExprList;
   BOOL fReduced = TRUE, fResult = FALSE;

   switch( pArg->ExprType )
   {
      case HB_ET_STRING:
         fResult = hb_strEmpty( pArg->value.asString.string, pArg->ulLength );
         break;

      case HB_ET_ARRAY:
      case HB_ET_HASH:
         /* TOFIX: do not optimize when array/hash args have user expressions */
         fResult = pArg->ulLength == 0;
         break;

      case HB_ET_NUMERIC:
         if( pArg->value.asNum.NumType == HB_ET_DOUBLE )
            fResult = pArg->value.asNum.val.d == 0.0;
         else
            fResult = pArg->value.asNum.val.l == 0;
         break;

      case HB_ET_LOGICAL:
         fResult = !pArg->value.asLogical;
         break;

      case HB_ET_NIL:
         fResult = TRUE;
         break;

      case HB_ET_DATE:
         fResult = pArg->value.asNum.val.l == 0;
         break;

      case HB_ET_CODEBLOCK:
         break;

      /* case HB_ET_FUNREF: */
      default:
         fReduced = FALSE;
   }

   if( fReduced )
   {
      HB_EXPR_PTR pExpr = hb_compExprNewLogical( fResult, HB_COMP_PARAM );

      HB_COMP_EXPR_FREE( pParms );
      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
      memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
      HB_COMP_EXPR_CLEAR( pExpr );
      return TRUE;
   }
   return FALSE;
}

BOOL hb_compExprReduceASC( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pArg = pParms->value.asList.pExprList;

   if( pArg->ExprType == HB_ET_STRING )
   {
      HB_EXPR_PTR pExpr = hb_compExprNewLong(
                ( UCHAR ) pArg->value.asString.string[0], HB_COMP_PARAM );

      HB_COMP_EXPR_FREE( pParms );
      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
      memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
      HB_COMP_EXPR_CLEAR( pExpr );
      return TRUE;
   }
   return FALSE;
}

BOOL hb_compExprReduceINT( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pArg = pParms->value.asList.pExprList;

   if( pArg->ExprType == HB_ET_NUMERIC )
   {
      HB_EXPR_PTR pExpr;

      if( pArg->value.asNum.NumType == HB_ET_LONG )
         pExpr = hb_compExprNewLong( pArg->value.asNum.val.l, HB_COMP_PARAM );
      else
      {
         HB_MAXDBL dVal = ( HB_MAXDBL ) pArg->value.asNum.val.d;
         if( HB_DBL_LIM_LONG( dVal ) )
            pExpr = hb_compExprNewLong( ( HB_LONG ) pArg->value.asNum.val.d, HB_COMP_PARAM );
         else
            pExpr = hb_compExprNewDouble( pArg->value.asNum.val.d,
                                          pArg->value.asNum.bWidth, 0,
                                          HB_COMP_PARAM );
      }
      HB_COMP_EXPR_FREE( pParms );
      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
      memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
      HB_COMP_EXPR_CLEAR( pExpr );
      return TRUE;
   }
   return FALSE;
}

BOOL hb_compExprReduceDTOS( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pArg = pParms->value.asList.pExprList;

   if( pArg->ExprType == HB_ET_DATE )
   {
      char szBuffer[ 9 ];
      char * szDate = ( char * ) memcpy( hb_xgrab( 9 ),
                      hb_dateDecStr( szBuffer, ( long ) pArg->value.asNum.val.l ), 9 );
      HB_EXPR_PTR pExpr = hb_compExprNewString( szDate, 8, TRUE, HB_COMP_PARAM );

      HB_COMP_EXPR_FREE( pParms );
      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
      memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
      HB_COMP_EXPR_CLEAR( pExpr );
      return TRUE;
   }

   return FALSE;
}

BOOL hb_compExprReduceSTOD( HB_EXPR_PTR pSelf, USHORT usCount, HB_COMP_DECL )
{
   if( usCount == 1 )
   {
      HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
      HB_EXPR_PTR pArg = pParms->value.asList.pExprList;

      if( pArg->ExprType == HB_ET_STRING && ( pArg->ulLength == 8 || pArg->ulLength == 0 ) )
      {
         HB_EXPR_PTR pExpr = hb_compExprNewDate( pArg->ulLength == 0 ? 0 :
                                  hb_dateEncStr( pArg->value.asString.string ),
                                  HB_COMP_PARAM );

         HB_COMP_EXPR_FREE( pParms );
         HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
         memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
         HB_COMP_EXPR_CLEAR( pExpr );
         return TRUE;
      }
   }
   else
   {
      HB_EXPR_PTR pExpr = hb_compExprNewDate( 0, HB_COMP_PARAM );

      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pParms );
      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
      memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
      HB_COMP_EXPR_CLEAR( pExpr );
      return TRUE;
   }

   return FALSE;
}

BOOL hb_compExprReduceCTOD( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pArg = pParms->value.asList.pExprList;

   if( pArg->ExprType == HB_ET_STRING && pArg->ulLength == 0 )
   {
      HB_EXPR_PTR pExpr = hb_compExprNewDate( 0, HB_COMP_PARAM );

      HB_COMP_EXPR_FREE( pParms );
      HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
      memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
      HB_COMP_EXPR_CLEAR( pExpr );
      return TRUE;
   }

   return FALSE;
}

BOOL hb_compExprReduceUPPER( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pArg = pParms->value.asList.pExprList;

   if( pArg->ExprType == HB_ET_STRING )
   {
      ULONG ulLen = pArg->ulLength;
      BOOL fLower = FALSE;

      if( ulLen )
      {
         char * szValue = pArg->value.asString.string;
         do
         {
            char c = * szValue++;
            if( c >= 'a' && c <= 'z' )
               fLower = TRUE;
            else if( !( ( c >= 'A' && c <= 'Z' ) ||
                        ( c >= '0' && c <= '9' ) || c == ' ' ) )
               break;
         }
         while( --ulLen );
      }

      if( ulLen == 0 )
      {
         HB_EXPR_PTR pExpr;
         char * szValue;
         BOOL fDealloc;

         if( fLower )
         {
            if( pArg->ulLength == 1 )
            {
               szValue = ( char * ) hb_szAscii[ toupper( (unsigned char)
                                          pArg->value.asString.string[ 0 ] ) ];
               fDealloc = FALSE;
            }
            else
            {
               if( pArg->value.asString.dealloc )
               {
                  szValue = pArg->value.asString.string;
                  pArg->value.asString.dealloc = FALSE;
                  fDealloc = TRUE;
               }
               else
               {
                  szValue = ( char * ) hb_xgrab( pArg->ulLength + 1 );
                  memcpy( szValue, pArg->value.asString.string, pArg->ulLength + 1 );
                  fDealloc = TRUE;
               }
               do
                  szValue[ ulLen ] = ( char ) toupper( ( unsigned char ) szValue[ ulLen ] );
               while( ++ulLen < pArg->ulLength );
            }
         }
         else
         {
            szValue = pArg->value.asString.string;
            fDealloc = pArg->value.asString.dealloc;
            pArg->value.asString.dealloc = FALSE;
         }

         pExpr = HB_COMP_EXPR_NEW( HB_ET_STRING );
         pExpr->ValType = HB_EV_STRING;
         pExpr->value.asString.string = szValue;
         pExpr->value.asString.dealloc = fDealloc;
         pExpr->ulLength = pArg->ulLength;

         HB_COMP_EXPR_FREE( pParms );
         HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
         memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
         HB_COMP_EXPR_CLEAR( pExpr );

         return TRUE;
      }
   }

   return FALSE;
}

BOOL hb_compExprReduceMIN( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pFirst = pParms->value.asList.pExprList;
   HB_EXPR_PTR pNext = pFirst->pNext;

   if( pFirst->ExprType == pNext->ExprType )
   {
      HB_EXPR_PTR pExpr = NULL;

      if( pFirst->ExprType == HB_ET_NUMERIC )
      {
         BYTE bType = ( pFirst->value.asNum.NumType & pNext->value.asNum.NumType );

         switch( bType )
         {
            case HB_ET_LONG:
               pExpr = pFirst->value.asNum.val.l <= pNext->value.asNum.val.l ?
                       pFirst : pNext;
               break;

            case HB_ET_DOUBLE:
               pExpr = pFirst->value.asNum.val.d <= pNext->value.asNum.val.d ?
                       pFirst : pNext;
               break;

            default:
               if( pFirst->value.asNum.NumType == HB_ET_DOUBLE )
                  pExpr = ( pFirst->value.asNum.val.d <= ( double ) pNext->value.asNum.val.l ) ?
                          pFirst : pNext;
               else
                  pExpr = ( ( double ) pFirst->value.asNum.val.l <= pNext->value.asNum.val.d ) ?
                          pFirst : pNext;
         }
      }
      else if( pFirst->ExprType == HB_ET_DATE )
      {
         pExpr = pFirst->value.asNum.val.l <= pNext->value.asNum.val.l ?
                 pFirst : pNext;
      }
      else if( pFirst->ExprType == HB_ET_LOGICAL )
      {
         pExpr = !pFirst->value.asLogical ? pFirst : pNext;
      }

      if( pExpr )
      {
         HB_EXPR_PTR * pExprPtr = &pParms->value.asList.pExprList;

         while( *pExprPtr )
         {
            if( *pExprPtr == pExpr )
            {
               *pExprPtr = pExpr->pNext;
               break;
            }
            pExprPtr = &( *pExprPtr )->pNext;
         }
         HB_COMP_EXPR_FREE( pParms );
         HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
         memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
         HB_COMP_EXPR_CLEAR( pExpr );
         return TRUE;
      }
   }

   return FALSE;
}

BOOL hb_compExprReduceMAX( HB_EXPR_PTR pSelf, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pFirst = pParms->value.asList.pExprList;
   HB_EXPR_PTR pNext = pFirst->pNext;

   if( pFirst->ExprType == pNext->ExprType )
   {
      HB_EXPR_PTR pExpr = NULL;

      if( pFirst->ExprType == HB_ET_NUMERIC )
      {
         BYTE bType = ( pFirst->value.asNum.NumType & pNext->value.asNum.NumType );

         switch( bType )
         {
            case HB_ET_LONG:
               pExpr = pFirst->value.asNum.val.l >= pNext->value.asNum.val.l ?
                       pFirst : pNext;
               break;

            case HB_ET_DOUBLE:
               pExpr = pFirst->value.asNum.val.d >= pNext->value.asNum.val.d ?
                       pFirst : pNext;
               break;

            default:
               if( pFirst->value.asNum.NumType == HB_ET_DOUBLE )
                  pExpr = ( pFirst->value.asNum.val.d >= ( double ) pNext->value.asNum.val.l ) ?
                          pFirst : pNext;
               else
                  pExpr = ( ( double ) pFirst->value.asNum.val.l >= pNext->value.asNum.val.d ) ?
                          pFirst : pNext;
         }
      }
      else if( pFirst->ExprType == HB_ET_DATE )
      {
         pExpr = pFirst->value.asNum.val.l >= pNext->value.asNum.val.l ?
                 pFirst : pNext;
      }
      else if( pFirst->ExprType == HB_ET_LOGICAL )
      {
         pExpr = pFirst->value.asLogical ? pFirst : pNext;
      }

      if( pExpr )
      {
         HB_EXPR_PTR * pExprPtr = &pParms->value.asList.pExprList;

         while( * pExprPtr )
         {
            if( * pExprPtr == pExpr )
            {
               * pExprPtr = pExpr->pNext;
               break;
            }
            pExprPtr = &( *pExprPtr )->pNext;
         }
         HB_COMP_EXPR_FREE( pParms );
         HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
         memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
         HB_COMP_EXPR_CLEAR( pExpr );
         return TRUE;
      }
   }

   return FALSE;
}

BOOL hb_compExprReduceBitFunc( HB_EXPR_PTR pSelf, HB_LONG lResult, BOOL fBool, HB_COMP_DECL )
{
   HB_EXPR_PTR pParms = pSelf->value.asFunCall.pParms;
   HB_EXPR_PTR pExpr = fBool ? hb_compExprNewLogical( lResult != 0, HB_COMP_PARAM ) :
                               hb_compExprNewLong( lResult, HB_COMP_PARAM );

   HB_COMP_EXPR_FREE( pParms );
   HB_COMP_EXPR_FREE( pSelf->value.asFunCall.pFunName );
   memcpy( pSelf, pExpr, sizeof( HB_EXPR ) );
   HB_COMP_EXPR_CLEAR( pExpr );
   return TRUE;
}
