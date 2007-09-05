/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * hb_fsFile() function
 *
 * Copyright 1999-2002 Viktor Szakats <viktor.szakats@syenar.hu>
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
#include "hbapifs.h"

HB_EXPORT BOOL hb_fsFile( BYTE * pFilename )
{
   PHB_FFIND ffind;
   BOOL fFree;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsFile(%s)", ( char * ) pFilename));

   pFilename = hb_fsNameConv( pFilename, &fFree );

   if( ( ffind = hb_fsFindFirst( ( char * ) pFilename, HB_FA_ALL ) ) != NULL )
   {
      hb_fsFindClose( ffind );
      hb_xfree(pFilename);
      return TRUE;
   }

   if( fFree )
      hb_xfree( pFilename );

   return FALSE;
}

HB_EXPORT BOOL hb_fsIsDirectory( BYTE * pFilename )
{
   BOOL bResult = FALSE, fFree;
   PHB_FFIND ffind;
   int iLen;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsIsDirectory(%s)", ( char * ) pFilename));

   pFilename = hb_fsNameConv( pFilename, &fFree );

   iLen = strlen( ( char * ) pFilename );
   while( iLen && strchr( OS_PATH_DELIMITER_LIST, pFilename[ iLen - 1 ] ) )
      --iLen;

   if( pFilename[ iLen ] )
   {
      if( fFree )
         pFilename[ iLen ] = '\0';
      else
      {
         pFilename = ( BYTE * ) hb_strndup( ( char * ) pFilename, iLen );
         fFree = TRUE;
      }
   }

   if( iLen && iLen <= _POSIX_PATH_MAX )
   {
      if( ( ffind = hb_fsFindFirst( ( char * ) pFilename, HB_FA_DIRECTORY ) ) != NULL )
      {
         if( ( ffind->attr & HB_FA_DIRECTORY ) == HB_FA_DIRECTORY )
            bResult = TRUE;
         hb_fsFindClose( ffind );
      }
   }

   if( fFree )
      hb_xfree( pFilename );

   return bResult;
}
