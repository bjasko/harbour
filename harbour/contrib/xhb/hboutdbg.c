/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Debug to separate console string output
 *
 * Copyright 2003 Giancarlo Niccolai <gian@niccolai.ws>
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

/* JC1: Caution. This code is NOT multithreaded as it is meant
   to be managed from the main thread. Every thread may use it,
   but only the main thread should be allowed to activate the
   debug window.
*/

#define HB_OS_WIN_32_USED

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapifs.h"

#if defined(HB_OS_UNIX)
#include <hbmath.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


static int s_iDebugFd = 0;
static char s_szDebugName[128];
static int s_iUseDebugName = 0;
static int s_iXtermPid = 0;

static void debugInit( void )
{
   int iPid, iFifoResult;
   char szDebugTitle[30];
   PHB_FNAME pFileName = NULL;
   char szDebugName[128];

   if( ! s_iUseDebugName )
   {
      int iRand = (int) (hb_random_num()*1000000);
      pFileName = hb_fsFNameSplit( hb_cmdargARGV()[0] );
      snprintf( szDebugName, 127, "/tmp/%s%d_dbg", pFileName->szName, iRand );
   }
   else
   {
      snprintf(szDebugName, 127, "/tmp/%s_dbg", s_szDebugName );
      pFileName = hb_fsFNameSplit( szDebugName );
   }

   iFifoResult = mkfifo( szDebugName, 0666 );
   if ( iFifoResult == -1 )
   {
      iFifoResult = errno;
   }   
   if ( iFifoResult == 0 || iFifoResult == EEXIST )
   {
      if ( strlen( pFileName->szName ) > 20 )
      {
         pFileName->szName[20] = 0;
      }
      snprintf( szDebugTitle, sizeof( szDebugTitle ), "%s - Debug", pFileName->szName );


      iPid = fork();
      if ( iPid != 0 )
      {
         s_iDebugFd = open( szDebugName, O_WRONLY );
         s_iXtermPid = iPid;
      }
      else
      {
         if ( iFifoResult != EEXIST ) {
            s_iXtermPid = execlp( "xterm", "xterm", "-T", szDebugTitle, "-e",
               "cat", szDebugName, NULL );

            if ( s_iXtermPid <= 0 ) {
               int lastresort = open( szDebugName, O_RDONLY );
               if ( lastresort >= 0 )
               {
                  close( lastresort );
               }
            }

         }
      }
   }
   hb_xfree( pFileName );
}

#endif

HB_FUNC( HB_OUTDEBUGNAME )
{
#if defined(HB_OS_UNIX)
   PHB_ITEM pName = hb_param( 1, HB_IT_STRING );

   if ( s_iDebugFd == 0 && pName != NULL)
   {
      strncpy( s_szDebugName, hb_itemGetCPtr( pName ), 127 );
      s_iUseDebugName = 1;

      hb_retl( TRUE );
   }
   else if ( pName == NULL)
   {
      s_iUseDebugName = 0;
      hb_retl( TRUE );
   }
   else
   {
      hb_retl( FALSE );
   }
#endif
}

HB_FUNC( HB_OUTDEBUG )
{
   #if defined(HB_OS_UNIX)
   int iStatus, iPid;

   /* Are we under X? */
   if ( getenv("DISPLAY") != NULL)
   {
      if ( s_iDebugFd <= 0 || s_iXtermPid == 0 )
      {
         debugInit();
      }

      /* On error, drop it */
      if ( s_iDebugFd <= 0 || s_iXtermPid == 0 )
      {
         return;
      }

      /* Chech if display process has terminated in the meanwhile */
      if (! s_iUseDebugName )
      {
         iPid = waitpid( s_iXtermPid, &iStatus,  WNOHANG );
         if ( iPid == s_iXtermPid || iPid == -1 )
         {
            s_iXtermPid = 0;
            /* close( s_iDebugFd ); */
            s_iDebugFd = 0;
            return;
         }
      }

      if ( s_iDebugFd > 0 && ISCHAR(1) )
      {
         fd_set wrds;
         struct timeval tv = { 0, 100000 }; /* wait each time a tenth of second */
         FD_ZERO(&wrds);
         FD_SET(s_iDebugFd, &wrds);

         if( select( s_iDebugFd + 1, NULL, &wrds, NULL, &tv ) > 0 )
         {
            write( s_iDebugFd, hb_parcx(1), hb_parclen(1) );
            write( s_iDebugFd, "\n", 1 );
         }
      }
   }
   #elif defined(__WIN32__)

   if( ISCHAR(1) )
   {
      LPTSTR lpMsg = HB_TCHAR_CONVTO( hb_parcx( 1 ) );
      OutputDebugString( lpMsg );
      HB_TCHAR_FREE( lpMsg );
   }

   #endif
}

