/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * VAL() function
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

#include <math.h>

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"

/* returns the numeric value of a character string representation of a number */
double hb_strVal( const char * szText, ULONG ulLen )
{
   long double ldValue = 0.0;
   ULONG ulPos;
   ULONG ulDecPos = 0;
   BOOL bNegative = FALSE;
   long double ldScale = 0.1L;

   HB_TRACE(HB_TR_DEBUG, ("hb_strVal(%s, %d)", szText, ulLen));

   /* Look for sign */

   for( ulPos = 0; ulPos < ulLen; ulPos++ )
   {
      if( szText[ ulPos ] == '-' )
      {
         bNegative = TRUE;
         ulPos++;
         break;
      }
      else if( szText[ ulPos ] == '+' )
      {
         ulPos++;
         break;
      }
      else if( ! HB_ISSPACE( szText[ ulPos ] ) )
         break;
   }

   /* Build the number */

   for(; ulPos < ulLen; ulPos++ )
   {
      if( szText[ ulPos ] == '.' && ulDecPos == 0 )
      {
         ulDecPos++;
         ldScale = 0.1L;
      }
      else if( szText[ ulPos ] >= '0' && szText[ ulPos ] <= '9' )
      {
         if( ulDecPos )
         {
            ldValue += ldScale * ( long double )( szText[ ulPos ] - '0' );
            ldScale *= 0.1L;
         }
         else
            ldValue = ( ldValue * 10.0L ) + ( long double )( szText[ ulPos ] - '0' );
      }
      else
         break;
   }


   return ( double )( bNegative && ldValue != 0.0L ? -ldValue : ldValue );
}

/* returns the numeric value of a character string representation of a number  */
HB_FUNC( VAL )
{
   PHB_ITEM pText = hb_param( 1, HB_IT_STRING );

   if( pText )
   {
      char * szText = hb_itemGetCPtr( pText );
      int iWidth = ( int ) hb_itemGetCLen( pText );
      int iDec;
      double dValue = hb_strVal( szText, hb_itemGetCLen( pText ) );

      for( iDec = 0; iDec < iWidth && szText[ iDec ] != '.'; iDec++ );

      if( iDec >= iWidth - 1 )
         hb_retnlen( dValue, iWidth, 0 );
      else
      {
         /* NOTE: Kludge Warning! This condition:
                  "|| ( iDec == 1 && szText[ 0 ] == '-' && dValue != 0.0 )"
                  may not be the generic way to handle the width of this "-.1"
                  string. I could not find a matching case which
                  fails for the same reason, nor a better way to handle it.
                  The problem is that in this case only, the width is
                  calculated upon conditions which can only be discovered by
                  parsing the string, but the parsing is made by a lower level
                  generic function. [vszakats] */

         hb_retnlen( dValue, iDec + ( iDec == 0 || ( iDec == 1 && szText[ 0 ] == '-' && dValue != 0.0 ) ? 1 : 0 ), iWidth - iDec - 1 );
      }
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 1098, NULL, "VAL", 1, hb_paramError( 1 ) );
}

