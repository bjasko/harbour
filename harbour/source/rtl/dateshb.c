/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The Date API (Harbour level)
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
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
 * Copyright 1999 Jose Lalin <dezac@corevia.com>
 *    DAY()
 *    MONTH()
 *    YEAR()
 *    DOW()
 *
 * Copyright 1999 David G. Holm <dholm@jsd-llc.com>
 *    CTOD()
 *    DATE()
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    HB_STOD()
 *
 * See doc/license.txt for licensing terms.
 *
 */

#include <ctype.h>

#include "hbapi.h"
#include "hbapierr.h"
#include "hbapiitm.h"
#include "hbset.h"
#include "hbdate.h"

HB_FUNC( CTOD )
{
   if( ISCHAR( 1 ) )
   {
      char * szDate = hb_parc( 1 );
      int d_value = 0, m_value = 0, y_value = 0;

      if( szDate )
      {
         int d_pos = 0, m_pos = 0, y_pos = 0;
         int count, digit, non_digit, size = strlen( hb_set.HB_SET_DATEFORMAT );

         for( count = 0; count < size; count++ )
         {
            switch( hb_set.HB_SET_DATEFORMAT[ count ] )
            {
               case 'D':
               case 'd':
                  if( d_pos == 0 )
                  {
                     if( m_pos == 0 && y_pos == 0 ) d_pos = 1;
                     else if( m_pos == 0 || y_pos == 0 ) d_pos = 2;
                     else d_pos = 3;
                  }
                  break;
               case 'M':
               case 'm':
                  if( m_pos == 0 )
                  {
                     if( d_pos == 0 && y_pos == 0 ) m_pos = 1;
                     else if( d_pos == 0 || y_pos == 0 ) m_pos = 2;
                     else m_pos = 3;
                  }
                  break;
               case 'Y':
               case 'y':
                  if( y_pos == 0 )
                  {
                     if( m_pos == 0 && d_pos == 0 ) y_pos = 1;
                     else if( m_pos == 0 || d_pos == 0 ) y_pos = 2;
                     else y_pos = 3;
                  }
            }
         }

         /* If there are non-digits at the start of the date field,
            they are not to be treated as date field separators */
         non_digit = 1;
         size = strlen( szDate );
         for( count = 0; count < size; count++ )
         {
            digit = szDate[ count ];
            if( isdigit( digit ) )
            {
               /* Process the digit for the current date field */
               if( d_pos == 1 )
                  d_value = ( d_value * 10 ) + digit - '0';
               else if( m_pos == 1 )
                  m_value = ( m_value * 10 ) + digit - '0';
               else if( y_pos == 1 )
                  y_value = ( y_value * 10 ) + digit - '0';
               /* Treat the next non-digit as a date field separator */
               non_digit = 0;
            }
            else if( digit != ' ' )
            {
               /* Process the non-digit */
               if( non_digit++ == 0 )
               {
                  /* Only move to the next date field on the first
                     consecutive non-digit that is encountered */
                  d_pos--;
                  m_pos--;
                  y_pos--;
               }
            }
         }

         if( y_value >= 0 && y_value < 100 )
         {
            count = hb_set.HB_SET_EPOCH % 100;
            digit = hb_set.HB_SET_EPOCH / 100;

            if( y_value >= count )
               y_value += ( digit * 100 );
            else
               y_value += ( ( digit * 100 ) + 100 );
         }
      }

      hb_retd( y_value, m_value, d_value );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 1119, NULL, "CTOD", 1, hb_paramError( 1 ) );
}

HB_FUNC( DTOC )
{
   if( ISDATE( 1 ) )
   {
      char szDate[ 9 ];
      char szFormatted[ 11 ];

      hb_retc( hb_dateFormat( hb_pardsbuff( szDate, 1 ), szFormatted, hb_set.HB_SET_DATEFORMAT ) );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 1118, NULL, "DTOC", 1, hb_paramError( 1 ) );
}

HB_FUNC( DTOS )
{
   if( ISDATE( 1 ) )
   {
      char szDate[ 9 ];

      hb_retc( hb_pardsbuff( szDate, 1 ) );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 1120, NULL, "DTOS", 1, hb_paramError( 1 ) );
}

/* NOTE: Harbour extension, exactly the same as STOD(). */

HB_FUNC( HB_STOD )
{
#ifdef HB_FAST_STOD
   hb_retds( hb_parc( 1 ) );
#else
   hb_retds( ( ISCHAR( 1 ) && hb_parclen( 1 ) == 8 ) ? hb_parc( 1 ) : "        " );
#endif
}

HB_FUNC( YEAR )
{
   PHB_ITEM pDate = hb_param( 1, HB_IT_DATE );

   if( pDate )
   {
      int iYear, iMonth, iDay;

      hb_dateDecode( hb_itemGetDL( pDate ), &iYear, &iMonth, &iDay );

      hb_retnllen( iYear, 5 );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 1112, NULL, "YEAR", 1, hb_paramError( 1 ) );
}

HB_FUNC( MONTH )
{
   PHB_ITEM pDate = hb_param( 1, HB_IT_DATE );

   if( pDate )
   {
      int iYear, iMonth, iDay;

      hb_dateDecode( hb_itemGetDL( pDate ), &iYear, &iMonth, &iDay );

      hb_retnilen( iMonth, 3 );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 1113, NULL, "MONTH", 1, hb_paramError( 1 ) );
}

HB_FUNC( DAY )
{
   PHB_ITEM pDate = hb_param( 1, HB_IT_DATE );

   if( pDate )
   {
      int iYear, iMonth, iDay;

      hb_dateDecode( hb_itemGetDL( pDate ), &iYear, &iMonth, &iDay );

      hb_retnilen( iDay, 3 );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 1114, NULL, "DAY", 1, hb_paramError( 1 ) );
}

HB_FUNC( TIME )
{
   char szResult[ 9 ];
   hb_dateTimeStr( szResult );
   hb_retclen( szResult, 8 );
}

HB_FUNC( DATE )
{
   int iYear, iMonth, iDay;
   hb_dateToday( &iYear, &iMonth, &iDay );
   hb_retd( iYear, iMonth, iDay );
}

HB_FUNC( DOW )
{
   PHB_ITEM pDate = hb_param( 1, HB_IT_DATE );

   if( pDate )
   {
      hb_retnilen( hb_dateJulianDOW( hb_itemGetDL( pDate ) ), 3 );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 1115, NULL, "DOW", 1, hb_paramError( 1 ) );
}
