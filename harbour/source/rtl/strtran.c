/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * STRTRAN function
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

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"

/* TOFIX: Check for string overflow, Clipper can crash if the resulting
          string is too large. Example:
          StrTran( "...", ".", Replicate( "A", 32000 ) ) [vszakats] */

/* replaces lots of characters in a string */
/* TOFIX: Will not work with a search string of > 64 KB on some platforms */
HB_FUNC( STRTRAN )
{
   PHB_ITEM pText = hb_param( 1, HB_IT_STRING );
   PHB_ITEM pSeek = hb_param( 2, HB_IT_STRING );

   if( pText && pSeek )
   {
      char * szText = hb_itemGetCPtr( pText );
      ULONG ulText = hb_itemGetCLen( pText );
      ULONG ulSeek = hb_itemGetCLen( pSeek );

      if( ulSeek && ulSeek <= ulText )
      {
         char * szSeek = hb_itemGetCPtr( pSeek );
         char * szReplace;
         ULONG ulStart;

         ulStart = ( ISNUM( 4 ) ? hb_parnl( 4 ) : 1 );

         if( !ulStart )
         {
            /* Clipper seems to work this way */
            hb_retc( NULL );
         }
         else if( ulStart > 0 )
         {
            PHB_ITEM pReplace = hb_param( 3, HB_IT_STRING );
            ULONG ulReplace;
            ULONG ulCount;
            BOOL bAll;

            if( pReplace )
            {
               szReplace = hb_itemGetCPtr( pReplace );
               ulReplace = hb_itemGetCLen( pReplace );
            }
            else
            {
               szReplace = ""; /* shouldn't matter that we don't allocate */
               ulReplace = 0;
            }

            if( ISNUM( 5 ) )
            {
               ulCount = hb_parnl( 5 );
               bAll = FALSE;
            }
            else
            {
               ulCount = 0;
               bAll = TRUE;
            }

            if( bAll || ulCount > 0 )
            {
               ULONG ulFound = 0;
               long lReplaced = 0;
               ULONG i = 0;
               ULONG ulLength = ulText;
               ULONG ulStop = ulText - ulSeek + 1;

               while( i < ulStop )
               {
                  if( ( bAll || lReplaced < ( long ) ulCount ) &&
                      ! memcmp( szText + i, szSeek, ulSeek ) )
                  {
                     ulFound++;
                     if( ulFound >= ulStart )
                     {
                        lReplaced++;
                        ulLength = ulLength - ulSeek + ulReplace;
                        i += ulSeek;
                     }
                     else
                        i++;
                  }
                  else
                     i++;
               }

               if( ulFound )
               {
                  char * szResult = ( char * ) hb_xgrab( ulLength + 1 );
                  char * szPtr = szResult;

                  ulFound = 0;
                  i = 0;
                  while( i < ulText )
                  {
                     if( lReplaced && ! memcmp( szText + i, szSeek, ulSeek ) )
                     {
                        ulFound++;
                        if( ulFound >= ulStart )
                        {
                           lReplaced--;
                           memcpy( szPtr, szReplace, ulReplace );
                           szPtr += ulReplace;
                           i += ulSeek;
                        }
                        else
                        {
                           *szPtr = szText[ i ];
                           szPtr++;
                           i++;
                        }
                     }
                     else
                     {
                        *szPtr = szText[ i ];
                        szPtr++;
                        i++;
                     }
                  }
                  hb_retclen_buffer( szResult, ulLength );
               }
               else
                  hb_itemReturn( pText );
            }
            else
               hb_itemReturn( pText );
         }
         else
            hb_itemReturn( pText );
      }
      else
         hb_itemReturn( pText );
   }
   else
   {
      /* NOTE: Undocumented but existing Clipper Run-time error [vszakats] */
#ifdef HB_C52_STRICT
      hb_errRT_BASE_SubstR( EG_ARG, 1126, NULL, HB_ERR_FUNCNAME, 0 );
#else
      hb_errRT_BASE_SubstR( EG_ARG, 1126, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
#endif
   }
}
