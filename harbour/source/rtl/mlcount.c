/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * MLCOUNT() function
 *
 * Copyright 1999 Ignacio Ortiz de Z�niga <ignacio@fivetech.com>
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

HB_FUNC( MLCOUNT )
{
   char * pszString    = hb_parc( 1 );
   ULONG  ulLineLength = ISNUM( 2 ) ? hb_parni( 2 ) : 79;
   ULONG  ulTabLength  = ISNUM( 3 ) ? hb_parni( 3 ) : 4;
   ULONG  ulLastSpace  = 0;
   ULONG  ulCurLength  = 0;
   BOOL   bWordWrap    = ISLOG( 4 ) ? hb_parl( 4 ) : TRUE;
   ULONG  ulLen        = hb_parclen( 1 );
   ULONG  ulLines      = 0;
   ULONG  ulPos;

   if( ulLineLength < 4 )
      ulLineLength = 79;

#if defined( HB_C52_STRICT )
   if( ulLineLength > 254 )
      ulLineLength = 79;
#endif

   if( ulTabLength > ulLineLength )
      ulTabLength = ulLineLength - 1;

   for( ulPos = 0; ulPos < ulLen; ulPos++ )
   {
      switch( pszString[ ulPos ] )
      {
         case HB_CHAR_HT:
            ulCurLength = ( ( ULONG ) ( ulCurLength / ulTabLength ) * ulTabLength ) + ulTabLength;
            ulLastSpace = ulCurLength;
            break;

         case HB_CHAR_LF:
            ulCurLength = 0;
            ulLastSpace = 0;
            ulLines++;
            break;

         case HB_CHAR_CR:
            break;

         case ' ':
            ulCurLength++;
            ulLastSpace = ulCurLength;
            break;

         default:
            ulCurLength++;
      }

      if( ulCurLength > ulLineLength )
      {
         if( bWordWrap )
         {
            if( ulLastSpace == 0 )
               ulCurLength = 1;
            else
               ulCurLength = ulCurLength - ulLastSpace;
         }
         else
            ulCurLength = 1;

         ulLines++;
         ulLastSpace = 0;
      }
   }

   if( ulCurLength > 0 )
      ulLines++;

   hb_retnl( ulLines );
}