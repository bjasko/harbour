/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The Date API (C level)
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
 *    hb_dateDOW()
 *
 * Copyright 1999 David G. Holm <dholm@jsd-llc.com>
 *    hb_dateFormat()
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    hb_dateEncStr()
 *    hb_dateDecStr()
 *    hb_dateStrPut()
 *    hb_dateStrGet()
 *
 * See doc/license.txt for licensing terms.
 *
 */

#define HB_OS_WIN_32_USED

#include <ctype.h>
#include <time.h>

#include "hbapi.h"
#include "hbdate.h"

long HB_EXPORT hb_dateEncode( long lYear, long lMonth, long lDay )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dateEncode(%ld, %ld, %ld)", lYear, lMonth, lDay));

   /* Perform date validation */
   if( lYear >= 1 && lYear <= 2999 &&
       lMonth >= 1 && lMonth <= 12 &&
       lDay >= 1 )
   {
      /* Month, year, and lower day limits are simple,
         but upper day limit is dependent upon month and leap year */
      USHORT auiDayLimit[ 12 ] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

      if( ( ( lYear % 4 == 0 && lYear % 100 != 0 ) || lYear % 400 == 0 ) )
         auiDayLimit[ 1 ] = 29;

      if( lDay <= ( long ) auiDayLimit[ ( int ) lMonth - 1 ] )
      {
         long lFactor = ( lMonth < 3 ) ? -1 : 0;

         return ( 1461 * ( lFactor + 4800 + lYear ) / 4 ) +
                ( ( lMonth - 2 - ( lFactor * 12 ) ) * 367 ) / 12 -
                ( 3 * ( ( lFactor + 4900 + lYear ) / 100 ) / 4 ) +
                lDay - 32075;
      }
   }

   return 0;
}

void HB_EXPORT hb_dateDecode( long lJulian, long * plYear, long * plMonth, long * plDay )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dateDecode(%ld, %p, %p, %p)", lJulian, plYear, plMonth, plDay));

   if( lJulian > 0 )
   {
      long U, V, W, X;

      lJulian += 68569;
      W = ( lJulian * 4 ) / 146097;
      lJulian -= ( ( 146097 * W ) + 3 ) / 4;
      X = 4000 * ( lJulian + 1 ) / 1461001;
      lJulian -= ( ( 1461 * X ) / 4 ) - 31;
      V = 80 * lJulian / 2447;
      U = V / 11;

      *plYear  = X + U + ( W - 49 ) * 100;
      *plMonth = V + 2 - ( U * 12 );
      *plDay   = lJulian - ( 2447 * V / 80 );
   }
   else
   {
      *plYear  =
      *plMonth =
      *plDay   = 0;
   }
}

void HB_EXPORT hb_dateStrPut( char * szDate, long lYear, long lMonth, long lDay )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dateStrPut(%p, %ld, %ld, %ld)", szDate, lYear, lMonth, lDay));

   if( lYear >= 0 && lMonth && lDay )
   {
      szDate[ 0 ] = ( ( lYear % 10000 ) / 1000 ) + '0';
      szDate[ 1 ] = ( ( lYear % 1000 ) / 100 ) + '0';
      szDate[ 2 ] = ( ( lYear % 100 ) / 10 ) + '0';
      szDate[ 3 ] = ( lYear % 10 ) + '0';

      szDate[ 4 ] = ( lMonth / 10 ) + '0';
      szDate[ 5 ] = ( lMonth % 10 ) + '0';

      szDate[ 6 ] = ( lDay / 10 ) + '0';
      szDate[ 7 ] = ( lDay % 10 ) + '0';
   }
   else if ( lYear || lMonth || lDay )
      memset( szDate, '0', 8 );
   else
      memset( szDate, ' ', 8 );
}

void hb_dateStrGet( const char * szDate, long * plYear, long * plMonth, long * plDay )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dateStrGet(%s, %p, %p, %p)", szDate, plYear, plMonth, plDay));

   if( szDate && szDate[ 8 ] == '\0' )
   {
      /* Date string has correct length, so attempt to convert */
      *plYear  = ( ( USHORT ) ( szDate[ 0 ] - '0' ) * 1000 ) +
                 ( ( USHORT ) ( szDate[ 1 ] - '0' ) * 100 ) +
                 ( ( USHORT ) ( szDate[ 2 ] - '0' ) * 10 ) +
                   ( USHORT ) ( szDate[ 3 ] - '0' );
      *plMonth = ( ( szDate[ 4 ] - '0' ) * 10 ) + ( szDate[ 5 ] - '0' );
      *plDay   = ( ( szDate[ 6 ] - '0' ) * 10 ) + ( szDate[ 7 ] - '0' );
   }
   else
   {
      /* Date string missing or bad length, so force an empty date */
      *plYear  =
      *plMonth =
      *plDay   = 0;
   }
}

/* This function always closes the date with a zero byte, so it needs a
   9 character long buffer. */

char * hb_dateDecStr( char * szDate, long lJulian )
{
   long lYear, lMonth, lDay;

   HB_TRACE(HB_TR_DEBUG, ("hb_dateDecStr(%p, %ld)", szDate, lJulian));

   hb_dateDecode( lJulian, &lYear, &lMonth, &lDay );
   hb_dateStrPut( szDate, lYear, lMonth, lDay );
   szDate[ 8 ] = '\0';

   return szDate;
}

long hb_dateEncStr( char * szDate )
{
   long lYear, lMonth, lDay;

   HB_TRACE(HB_TR_DEBUG, ("hb_dateEncStr(%s)", szDate));

   hb_dateStrGet( szDate, &lYear, &lMonth, &lDay );

   return hb_dateEncode( lYear, lMonth, lDay );
}

/* NOTE: szFormattedDate must be an at least 11 chars wide buffer */

char * hb_dateFormat( const char * szDate, char * szFormattedDate, const char * szDateFormat )
{
   /*
    * NOTE: szFormattedDate must point to a buffer of at least 11 bytes.
    *       szDateFormat must point to a buffer holding the date format to use.
    */
   int format_count, digit_count, size;

   HB_TRACE(HB_TR_DEBUG, ("hb_dateFormat(%s, %p, %s)", szDate, szFormattedDate, szDateFormat));

   /*
    * Determine the maximum size of the formatted date string
    */
   size = strlen( szDateFormat );
   if( size > 10 ) size = 10;

   if( szDate && szFormattedDate && strlen( szDate ) == 8 ) /* A valid date is always 8 characters */
   {
      const char * szPtr;
      int digit;
      BOOL used_d, used_m, used_y;

      format_count = 0;
      used_d = used_m = used_y = FALSE;
      szPtr = szDateFormat;

      while( format_count < size )
      {
         digit = toupper( *szPtr );
         szPtr++;
         digit_count = 1;
         while( toupper( *szPtr ) == digit && format_count < size )
         {
            szPtr++;
            if( format_count + digit_count < size ) digit_count++;
         }
         switch( digit )
         {
            case 'D':
               switch( digit_count )
               {
                  case 4:
                     if( ! used_d && format_count < size )
                     {
/*                        szFormattedDate[ format_count++ ] = '0'; */
                        szFormattedDate[ format_count++ ] = szDate[ 6 ];
                        digit_count--;
                     }
                  case 3:
                     if( ! used_d && format_count < size )
                     {
/*                        szFormattedDate[ format_count++ ] = '0'; */
                        szFormattedDate[ format_count++ ] = szDate[ 6 ];
                        digit_count--;
                     }
                  case 2:
                     if( ! used_d && format_count < size )
                     {
                        szFormattedDate[ format_count++ ] = szDate[ 6 ];
                        digit_count--;
                     }
                  default:
                     if( ! used_d && format_count < size )
                     {
                        szFormattedDate[ format_count++ ] = szDate[ 7 ];
                        digit_count--;
                     }
                     while( digit_count-- > 0 && format_count < size ) szFormattedDate[ format_count++ ] = digit;
               }
               used_d = TRUE;
               break;

            case 'M':
               switch ( digit_count )
               {
                  case 4:
                     if( ! used_m && format_count < size )
                     {
/*                        szFormattedDate[ format_count++ ] = '0'; */
                        szFormattedDate[ format_count++ ] = szDate[ 4 ];
                        digit_count--;
                     }
                  case 3:
                     if( ! used_m && format_count < size )
                     {
/*                        szFormattedDate[ format_count++ ] = '0'; */
                        szFormattedDate[ format_count++ ] = szDate[ 4 ];
                        digit_count--;
                     }
                  case 2:
                     if( ! used_m && format_count < size )
                     {
                        szFormattedDate[ format_count++ ] = szDate[ 4 ];
                        digit_count--;
                     }
                  default:
                     if( ! used_m && format_count < size )
                     {
                        szFormattedDate[ format_count++ ] = szDate[ 5 ];
                        digit_count--;
                     }
                     while( digit_count-- > 0 && format_count < size ) szFormattedDate[ format_count++ ] = digit;
               }
               used_m = TRUE;
               break;

            case 'Y':
               switch( digit_count )
               {
                  case 4:
                     if( ! used_y && format_count < size )
                     {
                        szFormattedDate[ format_count++ ] = szDate[ 0 ];
                        digit_count--;
                     }

                  case 3:
                     if( ! used_y && format_count < size )
                     {
                        szFormattedDate[ format_count++ ] = szDate[ 1 ];
                        digit_count--;
                     }

                  case 2:
                     if( ! used_y && format_count < size )
                     {
                        szFormattedDate[ format_count++ ] = szDate[ 2 ];
                        digit_count--;
                     }

                  default:
                     if( ! used_y && format_count < size )
                     {
                        szFormattedDate[ format_count++ ] = szDate[ 3 ];
                        digit_count--;
                     }
                     while( digit_count-- > 0 && format_count < size ) szFormattedDate[ format_count++ ] = digit;
               }
               used_y = TRUE;
               break;

            default:
               while( digit_count-- > 0 && format_count < size ) szFormattedDate[ format_count++ ] = digit;
         }
      }
   }
   else
   {
      /* Not a valid date string, so return a blank date with separators */
      format_count = size; /* size is either 8 or 10 */
      strncpy( szFormattedDate, szDateFormat, size );

      for( digit_count = 0; digit_count < size; digit_count++ )
      {
         switch( szFormattedDate[ digit_count ] )
         {
            case 'D':
            case 'd':
            case 'M':
            case 'm':
            case 'Y':
            case 'y':
               szFormattedDate[ digit_count ] = ' ';
         }
      }
   }

   szFormattedDate[ format_count ] = '\0';

   return szFormattedDate;
}

long hb_dateDOW( long lYear, long lMonth, long lDay )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dateDOW(%ld, %ld, %ld)", lYear, lMonth, lDay));

   if( lMonth < 3 )
   {
      lMonth += 13;
      lYear--;
   }
   else
      lMonth++;

   return ( lDay + 26 * lMonth / 10 +
            lYear + lYear / 4 - lYear / 100 + lYear / 400 + 6 ) % 7 + 1;
}

void hb_dateToday( long * plYear, long * plMonth, long * plDay )
{
#if defined(HB_OS_WIN_32)
   {
      SYSTEMTIME st;
      GetLocalTime( &st );

      *plYear  = st.wYear;
      *plMonth = st.wMonth;
      *plDay   = st.wDay;
   }
#else
   {
      time_t t;
      struct tm * oTime;

      time( &t );
      oTime = localtime( &t );

      *plYear  = oTime->tm_year + 1900;
      *plMonth = oTime->tm_mon + 1;
      *plDay   = oTime->tm_mday;
   }
#endif
}

/* NOTE: The passed buffer must be at least 9 chars long */

void hb_dateTimeStr( char * pszTime )
{
#if defined(HB_OS_WIN_32)
   {
      SYSTEMTIME st;
      GetLocalTime( &st );
      sprintf( pszTime, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond );
   }
#else
   {
      time_t t;
      struct tm * oTime;

      time( &t );
      oTime = localtime( &t );
      sprintf( pszTime, "%02d:%02d:%02d", oTime->tm_hour, oTime->tm_min, oTime->tm_sec );
   }
#endif
}
