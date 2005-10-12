/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The Console API
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
 * Copyright 1999 David G. Holm <dholm@jsd-llc.com>
 *    hb_conOutAlt(), hb_conOutDev(), DEVOUT(), hb_conDevPos(),
 *    DEVPOS(), __EJECT(),
 *    hb_conOut(), hb_conOutErr(), OUTERR(),
 *    hb_conOutStd(), OUTSTD(), PCOL(), PROW(),
 *    SETPRC(), and hb_conInit()
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    hb_conNewLine()
 *    DISPOUTAT()
 *
 * See doc/license.txt for licensing terms.
 *
 */

#define HB_OS_WIN_32_USED
#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapifs.h"
#include "hbapigt.h"
#include "hbset.h"
#include "hb_io.h"

/* length of buffer for CR/LF characters */
#define CRLF_BUFFER_LEN   OS_EOL_LEN + 1

static BOOL   s_bInit = FALSE;
static USHORT s_uiPRow;
static USHORT s_uiPCol;
static SHORT  s_originalMaxRow;
static SHORT  s_originalMaxCol;
static char   s_szCrLf[ CRLF_BUFFER_LEN ];
#if defined(X__WIN32__)
static int    s_iFilenoStdin;
static int    s_iFilenoStdout;
static int    s_iFilenoStderr;
#else
static int    s_iFilenoStdin;
static int    s_iFilenoStdout;
static int    s_iFilenoStderr;
#endif

void hb_conInit( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_conInit()"));

#if defined(OS_UNIX_COMPATIBLE) && !defined(HB_EOL_CRLF)
   s_szCrLf[ 0 ] = HB_CHAR_LF;
   s_szCrLf[ 1 ] = '\0';
#else
   s_szCrLf[ 0 ] = HB_CHAR_CR;
   s_szCrLf[ 1 ] = HB_CHAR_LF;
   s_szCrLf[ 2 ] = '\0';
#endif

   s_uiPRow = s_uiPCol = 0;

#if defined(X__WIN32__)

   s_iFilenoStdin = GetStdHandle( STD_INPUT_HANDLE );
   s_iFilenoStdout = GetStdHandle( STD_OUTPUT_HANDLE );

#else

   s_iFilenoStdin = fileno( stdin );
   s_iFilenoStdout = fileno( stdout );

#endif

#ifdef HB_C52_UNDOC
   {
      int iStderr = hb_cmdargNum( "STDERR" ); /* Undocumented CA-Clipper switch //STDERR:x */

      if( iStderr < 0 )        /* //STDERR not used or invalid */
   #if defined(X__WIN32__)
         s_iFilenoStderr = GetStdHandle( STD_ERROR_HANDLE );
   #else
         s_iFilenoStderr = fileno( stderr );
   #endif
      else if( iStderr == 0 )  /* //STDERR with no parameter or 0 */
         s_iFilenoStderr = s_iFilenoStdout;
      else                     /* //STDERR:x */
         s_iFilenoStderr = iStderr;
   }
#else
   #if defined(X__WIN32__)
   s_iFilenoStderr = GetStdHandle( STD_ERROR_HANDLE );
   #else
   s_iFilenoStderr = fileno( stderr );
   #endif
#endif

   /* Some compilers open stdout and stderr in text mode, but
      Harbour needs them to be open in binary mode. */

#if !defined(X__WIN32__)
   hb_fsSetDevMode( s_iFilenoStdout, FD_BINARY );
   hb_fsSetDevMode( s_iFilenoStderr, FD_BINARY );
#endif

   s_bInit = TRUE;

   hb_gtInit( s_iFilenoStdin, s_iFilenoStdout, s_iFilenoStderr );
   s_originalMaxRow = hb_gtMaxRow(); /* Save the original */
   s_originalMaxCol = hb_gtMaxCol(); /* screen size */
   hb_setkeyInit();  /* April White, May 6, 2000 */
}

void hb_conRelease( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_conRelease()"));

   if( s_originalMaxRow != hb_gtMaxRow() || s_originalMaxCol != hb_gtMaxCol() )
   {
      /* If the program changed the screen size, restore the original */
      hb_gtSetMode( s_originalMaxRow + 1, s_originalMaxCol + 1 );
   }

#if !defined(X__WIN32__)
   hb_fsSetDevMode( s_iFilenoStdout, FD_TEXT );
   hb_fsSetDevMode( s_iFilenoStderr, FD_TEXT );
#endif

   /* The is done by the OS from now on */
   s_szCrLf[ 0 ] = HB_CHAR_LF;
   s_szCrLf[ 1 ] = '\0';

   hb_setkeyExit();  /* April White, May 6, 2000 */
   hb_conXSaveRestRelease();

   hb_gtExit();

   s_bInit = FALSE;
}

char * hb_conNewLine( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_conNewLine()"));

   return s_szCrLf;
}

HB_FUNC( HB_OSNEWLINE )
{
   hb_retc( s_szCrLf );
}

typedef void hb_out_func_typedef( const char *, ULONG );

/* Format items for output, then call specified output function */
static void hb_conOut( USHORT uiParam, hb_out_func_typedef * pOutFunc )
{
   char * pszString;
   ULONG ulLen;
   BOOL bFreeReq;

   HB_TRACE(HB_TR_DEBUG, ("hb_conOut(%hu, %p)", uiParam, pOutFunc));

   pszString = hb_itemString( hb_param( uiParam, HB_IT_ANY ), &ulLen, &bFreeReq );

   if ( ulLen )
      pOutFunc( pszString, ulLen );

   if( bFreeReq )
      hb_xfree( pszString );
}

/* Output an item to STDOUT */
void hb_conOutStd( const char * pStr, ULONG ulLen )
{
   USHORT uiErrorOld;

   HB_TRACE(HB_TR_DEBUG, ("hb_conOutStd(%s, %lu)", pStr, ulLen));

   if( ulLen == 0 )
      ulLen = strlen( pStr );

   if( s_bInit )
      hb_gtPreExt();

   uiErrorOld = hb_fsError(); /* Save current user file error code */
   hb_fsWriteLarge( ( int ) s_iFilenoStdout, ( BYTE * ) pStr, ulLen );
   hb_fsSetError( uiErrorOld ); /* Restore last user file error code */

   if( s_bInit )
   {
      hb_gtAdjustPos( ( int ) s_iFilenoStdout, pStr, ulLen );
      hb_gtPostExt();
   }
}

/* Output an item to STDERR */
void hb_conOutErr( const char * pStr, ULONG ulLen )
{
   USHORT uiErrorOld;

   HB_TRACE(HB_TR_DEBUG, ("hb_conOutErr(%s, %lu)", pStr, ulLen));

   if( ulLen == 0 )
      ulLen = strlen( pStr );

   if( s_bInit )
      hb_gtPreExt();

   uiErrorOld = hb_fsError(); /* Save current user file error code */
   hb_fsWriteLarge( ( int ) s_iFilenoStderr, ( BYTE * ) pStr, ulLen );
   hb_fsSetError( uiErrorOld ); /* Restore last user file error code */

   if( s_bInit )
   {
      hb_gtAdjustPos( ( int ) s_iFilenoStderr, pStr, ulLen );
      hb_gtPostExt();
   }
}

/* Output an item to the screen and/or printer and/or alternate */
static void hb_conOutAlt( const char * pStr, ULONG ulLen )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_conOutAlt(%s, %lu)", pStr, ulLen));

   if( hb_set.HB_SET_CONSOLE )
      hb_gtWriteCon( ( BYTE * ) pStr, ulLen );

   if( hb_set.HB_SET_ALTERNATE && hb_set.hb_set_althan != FS_ERROR )
   {
      /* Print to alternate file if SET ALTERNATE ON and valid alternate file */
      USHORT uiErrorOld = hb_fsError(); /* Save current user file error code */
      hb_fsWriteLarge( hb_set.hb_set_althan, ( BYTE * ) pStr, ulLen );
      hb_fsSetError( uiErrorOld ); /* Restore last user file error code */
   }

   if( hb_set.hb_set_extrahan != FS_ERROR )
   {
      /* Print to extra file if valid alternate file */
      USHORT uiErrorOld = hb_fsError(); /* Save current user file error code */
      hb_fsWriteLarge( hb_set.hb_set_extrahan, ( BYTE * ) pStr, ulLen );
      hb_fsSetError( uiErrorOld ); /* Restore last user file error code */
   }

   if( hb_set.HB_SET_PRINTER && hb_set.hb_set_printhan != FS_ERROR )
   {
      /* Print to printer if SET PRINTER ON and valid printer file */
      USHORT uiErrorOld = hb_fsError(); /* Save current user file error code */
      hb_fsWriteLarge( hb_set.hb_set_printhan, ( BYTE * ) pStr, ulLen );
      hb_fsSetError( uiErrorOld ); /* Restore last user file error code */
      s_uiPCol += ( USHORT ) ulLen;
   }
}

/* Output an item to the screen and/or printer */
static void hb_conOutDev( const char * pStr, ULONG ulLen )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_conOutDev(%s, %lu)", pStr, ulLen));

   if( hb_set.hb_set_printhan != FS_ERROR && hb_stricmp( hb_set.HB_SET_DEVICE, "PRINTER" ) == 0 )
   {
      /* Display to printer if SET DEVICE TO PRINTER and valid printer file */
      USHORT uiErrorOld = hb_fsError(); /* Save current user file error code */
      hb_fsWriteLarge( hb_set.hb_set_printhan, ( BYTE * ) pStr, ulLen );
      hb_fsSetError( uiErrorOld ); /* Restore last user file error code */
      s_uiPCol += ( USHORT ) ulLen;
   }
   else
      /* Otherwise, display to console */
      hb_gtWrite( ( BYTE * ) pStr, ulLen );
}

HB_FUNC( OUTSTD ) /* writes a list of values to the standard output device */
{
   USHORT uiPCount = hb_pcount();
   USHORT uiParam;

   for( uiParam = 1; uiParam <= uiPCount; uiParam++ )
   {
      hb_conOut( uiParam, hb_conOutStd );
      if( uiParam < uiPCount )
         hb_conOutStd( " ", 1 );
   }
}

HB_FUNC( OUTERR ) /* writes a list of values to the standard error device */
{
   USHORT uiPCount = hb_pcount();
   USHORT uiParam;

   for( uiParam = 1; uiParam <= uiPCount; uiParam++ )
   {
      hb_conOut( uiParam, hb_conOutErr );
      if( uiParam < uiPCount )
         hb_conOutErr( " ", 1 );
   }
}

HB_FUNC( QQOUT ) /* writes a list of values to the current device (screen or printer) and is affected by SET ALTERNATE */
{
   USHORT uiPCount = hb_pcount();
   USHORT uiParam;

   for( uiParam = 1; uiParam <= uiPCount; uiParam++ )
   {
      hb_conOut( uiParam, hb_conOutAlt );
      if( uiParam < uiPCount )
         hb_conOutAlt( " ", 1 );
   }
}

HB_FUNC( QOUT )
{
   hb_conOutAlt( s_szCrLf, CRLF_BUFFER_LEN - 1 );

   if( hb_set.HB_SET_PRINTER && hb_set.hb_set_printhan != FS_ERROR )
   {
      USHORT uiErrorOld = hb_fsError(); /* Save current user file error code */
      USHORT uiCount;

      s_uiPRow++;

      uiCount = s_uiPCol = hb_set.HB_SET_MARGIN;
      while( uiCount-- > 0 )
         hb_fsWrite( hb_set.hb_set_printhan, ( BYTE * ) " ", 1 );

      hb_fsSetError( uiErrorOld ); /* Restore last user file error code */
   }

   HB_FUNCNAME( QQOUT )();
}

HB_FUNC( __EJECT ) /* Ejects the current page from the printer */
{
   if( hb_stricmp( hb_set.HB_SET_DEVICE, "PRINTER" ) == 0 && hb_set.hb_set_printhan != FS_ERROR )
   {
      USHORT uiErrorOld = hb_fsError(); /* Save current user file error code */
      hb_fsWrite( hb_set.hb_set_printhan, ( BYTE * ) "\x0C\x0D", 2 );
      hb_fsSetError( uiErrorOld ); /* Restore last user file error code */
   }

   s_uiPRow = s_uiPCol = 0;
}

HB_FUNC( PROW ) /* Returns the current printer row position */
{
   hb_retni( ( int ) s_uiPRow );
}

HB_FUNC( PCOL ) /* Returns the current printer row position */
{
   hb_retni( ( int ) s_uiPCol );
}

static void hb_conDevPos( SHORT iRow, SHORT iCol )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_conDevPos(%hd, %hd)", iRow, iCol));

   /* Position printer if SET DEVICE TO PRINTER and valid printer file
      otherwise position console */

   if( hb_set.hb_set_printhan != FS_ERROR && hb_stricmp( hb_set.HB_SET_DEVICE, "PRINTER" ) == 0 )
   {
      USHORT uiCount;
      USHORT uiProw = ( USHORT ) iRow;
      USHORT uiPcol = ( USHORT ) iCol;
      USHORT uiErrorOld = hb_fsError(); /* Save current user file error code */

      if( uiProw < s_uiPRow )
      {
         hb_fsWrite( hb_set.hb_set_printhan, ( BYTE * ) "\x0C\x0D", 2 );
         s_uiPRow = s_uiPCol = 0;
      }

      for( uiCount = s_uiPRow; uiCount < uiProw; uiCount++ )
         hb_fsWrite( hb_set.hb_set_printhan, ( BYTE * ) s_szCrLf, CRLF_BUFFER_LEN - 1 );

      if( uiProw > s_uiPRow )
         s_uiPCol = 0;

      uiPcol += hb_set.HB_SET_MARGIN;

      if( ( uiProw == s_uiPRow ) && ( uiPcol < s_uiPCol ) )
      {
         hb_fsWrite( hb_set.hb_set_printhan, ( BYTE * ) "\x0D", 1 );
         s_uiPCol = 0;
      }

      for( uiCount = s_uiPCol; uiCount < uiPcol; uiCount++ )
         hb_fsWrite( hb_set.hb_set_printhan, ( BYTE * ) " ", 1 );

      s_uiPRow = uiProw;
      s_uiPCol = uiPcol;

      hb_fsSetError( uiErrorOld ); /* Restore last user file error code */
   }
   else
      hb_gtSetPos( iRow, iCol );
}

/* NOTE: This should be placed after the hb_conDevPos() definition. */

HB_FUNC( DEVPOS ) /* Sets the screen and/or printer position */
{
   if( ISNUM( 1 ) && ISNUM( 2 ) )
      hb_conDevPos( hb_parni( 1 ), hb_parni( 2 ) );
}

HB_FUNC( SETPRC ) /* Sets the current printer row and column positions */
{
   if( hb_pcount() == 2 && ISNUM( 1 ) && ISNUM( 2 ) )
   {
      s_uiPRow = ( USHORT ) hb_parni( 1 );
      s_uiPCol = ( USHORT ) hb_parni( 2 );
   }
}

HB_FUNC( DEVOUT ) /* writes a single value to the current device (screen or printer), but is not affected by SET ALTERNATE */
{
   if( ISCHAR( 2 ) )
   {
      char szOldColor[ CLR_STRLEN ];

      hb_gtGetColorStr( szOldColor );
      hb_gtSetColorStr( hb_parc( 2 ) );

      hb_conOut( 1, hb_conOutDev );

      hb_gtSetColorStr( szOldColor );
   }
   else if( hb_pcount() >= 1 )
      hb_conOut( 1, hb_conOutDev );
}

HB_FUNC( DISPOUT ) /* writes a single value to the screen, but is not affected by SET ALTERNATE */
{
   char * pszString;
   ULONG ulLen;
   BOOL bFreeReq;

   if( ISCHAR( 2 ) )
   {
      char szOldColor[ CLR_STRLEN ];

      hb_gtGetColorStr( szOldColor );
      hb_gtSetColorStr( hb_parc( 2 ) );

      pszString = hb_itemString( hb_param( 1, HB_IT_ANY ), &ulLen, &bFreeReq );

      hb_gtWrite( ( BYTE * ) pszString, ulLen );

      if( bFreeReq )
         hb_xfree( pszString );

      hb_gtSetColorStr( szOldColor );
   }
   else if( hb_pcount() >= 1 )
   {
      pszString = hb_itemString( hb_param( 1, HB_IT_ANY ), &ulLen, &bFreeReq );

      hb_gtWrite( ( BYTE * ) pszString, ulLen );

      if( bFreeReq )
         hb_xfree( pszString );
   }
}

/* Undocumented Clipper function */

/* NOTE: Clipper does no checks about the screen positions. [vszakats] */

HB_FUNC( DISPOUTAT ) /* writes a single value to the screen at speficic position, but is not affected by SET ALTERNATE */
{
   char * pszString;
   ULONG ulLen;
   BOOL bFreeReq;

   if( ISCHAR( 4 ) )
   {
      char szOldColor[ CLR_STRLEN ];

      hb_gtGetColorStr( szOldColor );
      hb_gtSetColorStr( hb_parc( 4 ) );

      pszString = hb_itemString( hb_param( 3, HB_IT_ANY ), &ulLen, &bFreeReq );

      hb_gtWriteAt( hb_parni( 1 ), hb_parni( 2 ), ( BYTE * ) pszString, ulLen );

      if( bFreeReq )
         hb_xfree( pszString );

      hb_gtSetColorStr( szOldColor );
   }
   else if( hb_pcount() >= 3 )
   {
      pszString = hb_itemString( hb_param( 3, HB_IT_ANY ), &ulLen, &bFreeReq );

      hb_gtWriteAt( hb_parni( 1 ), hb_parni( 2 ), ( BYTE * ) pszString, ulLen );

      if( bFreeReq )
         hb_xfree( pszString );
   }
}


HB_FUNC( HB_GETSTDIN ) /* Return Handel for STDIN */
{
   hb_retni( s_iFilenoStdin );
}

HB_FUNC( HB_GETSTDOUT ) /* Return Handel for STDOUT */
{
   hb_retni( s_iFilenoStdout );
}

