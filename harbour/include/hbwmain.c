/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    WinMain() to main() wrapper
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

#include <windows.h>

#define HB_MAX_ARGS     128

static int    s_argc = 0;
static char * s_argv[ HB_MAX_ARGS ];
static char   s_szAppName[ MAX_PATH ];
static TCHAR  s_lpAppName[ MAX_PATH ];

#if defined( HB_WINCE )
#  define HB_LPSTR      LPWSTR
#else
#  define HB_LPSTR      LPSTR
#endif

int WINAPI WinMain( HINSTANCE hInstance,      /* handle to current instance */
                    HINSTANCE hPrevInstance,  /* handle to previous instance */
                    HB_LPSTR  lpCmdLine,      /* pointer to command line */
                    int iCmdShow )            /* show state of window */
{
   LPSTR pArgs, pArg, pDst, pSrc, pFree;
   BOOL fQuoted;
   int iErrorCode;

   HB_SYMBOL_UNUSED( hPrevInstance );
   HB_SYMBOL_UNUSED( iCmdShow );

   GetModuleFileName( hInstance, s_lpAppName, MAX_PATH );
   HB_TCHAR_GETFROM( s_szAppName, s_lpAppName, MAX_PATH );
   s_argv[ s_argc++ ] = s_szAppName;

   pArg = NULL;

#if defined( HB_WINCE )
   pSrc = pFree = HB_TCHAR_CONVFROM( lpCmdLine );
#else
   pSrc = pFree = lpCmdLine;
#endif
   pDst = pArgs = ( LPSTR ) LocalAlloc( LMEM_FIXED, strlen( pFree ) + 1 );
   fQuoted = FALSE;

   while( *pSrc != 0 && s_argc < HB_MAX_ARGS )
   {
      if( *pSrc == '"' )
      {
         if( pArg == NULL )
            pArg = pDst;
         fQuoted = !fQuoted;
      }
      else if( fQuoted || !HB_ISSPACE( *pSrc ) )
      {
         if( pArg == NULL )
            pArg = pDst;
         *pDst++ = *pSrc;
      }
      else
      {
         if( pArg )
         {
            *pDst++ = '\0';
            s_argv[ s_argc++ ] = pArg;
            pArg = NULL;
         }
      }
      ++pSrc;
   }
   if( pArg )
   {
      *pDst = '\0';
      s_argv[ s_argc++ ] = pArg;
   }

#if defined( HB_WINCE )
   HB_TCHAR_FREE( pFree );
#endif

   iErrorCode = main( s_argc, s_argv );

   LocalFree( pArgs );

   return iErrorCode;
}
