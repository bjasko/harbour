/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Version detection functions
 *
 * Copyright 1999 {list of individual authors and e-mail addresses}
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
 * Copyright 1999 Luiz Rafael Culik <culik@sl.conex.net>
 *    hb_verPlatform() (support for determining the windows version)
 *
 * Copyright 1999 Jose Lalin <dezac@corevia.com>
 *    hb_verPlatform() (support for determining many windows flavours)
 *    hb_verCompiler() (support for determining some compiler version/revision)
 *
 * Copyright 2000-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    hb_verPlatform() (support for detecting Windows NT on DOS)
 *    hb_verPlatform() (rearrangment and cleanup)
 *
 * See doc/license.txt for licensing terms.
 *
 */

/* NOTE: For OS/2. Must be ahead of any and all #include statements */
#define INCL_DOSMISC

#define HB_OS_WIN_32_USED

#include "hbapi.h"
#include "hbmemory.ch"

#if defined(HB_OS_WIN_32)

   #include <ctype.h>
   #ifndef VER_PLATFORM_WIN32_WINDOWS
      #define VER_PLATFORM_WIN32_WINDOWS 1
   #endif
   #ifndef VER_PLATFORM_WIN32_CE
      #define VER_PLATFORM_WIN32_CE 3
   #endif

#elif defined(HB_OS_UNIX) && !defined(__CEGCC__)

   #include <sys/utsname.h>

#endif

/* NOTE: OS() function, as a primary goal will detect the version number
         of the target platform. As an extra it may also detect the host OS.
         The latter is mainly an issue in DOS, where the host OS can be OS/2
         WinNT/2K, Win3x, Win9x, DOSEMU, Desqview, etc. [vszakats] */

/* NOTE: The caller must free the returned buffer. [vszakats] */

char * hb_verPlatform( void )
{
   char * pszPlatform;

   HB_TRACE(HB_TR_DEBUG, ("hb_verPlatform()"));

   /* NOTE: Must be larger than 128, which is the maximum size of
            osVer.szCSDVersion (Win32). [vszakats] */
   pszPlatform = ( char * ) hb_xgrab( 256 );

#if defined(HB_OS_DOS)

   {
      union REGS regs;

      regs.h.ah = 0x30;
      HB_DOS_INT86( 0x21, &regs, &regs );

      snprintf( pszPlatform, 256, "DOS %d.%02d", regs.h.al, regs.h.ah );

      /* Host OS detection: Windows 2.x, 3.x, 95/98 */

      {
         regs.HB_XREGS.ax = 0x1600;
         HB_DOS_INT86( 0x2F, &regs, &regs );

         if( regs.h.al != 0x00 && regs.h.al != 0x80 )
         {
            char szHost[ 128 ];

            if( regs.h.al == 0x01 || regs.h.al == 0xFF )
               snprintf( szHost, sizeof( szHost ), " (Windows 2.x)" );
            else
               snprintf( szHost, sizeof( szHost ), " (Windows %d.%02d)", regs.h.al, regs.h.ah );

            hb_strncat( pszPlatform, szHost, 255 );
         }
      }

      /* Host OS detection: Windows NT/2000 */

      {
         regs.HB_XREGS.ax = 0x3306;
         HB_DOS_INT86( 0x21, &regs, &regs );

         if( regs.HB_XREGS.bx == 0x3205 )
            hb_strncat( pszPlatform, " (Windows NT/2000)", 255 );
      }

      /* Host OS detection: OS/2 */

      {
         regs.h.ah = 0x30;
         HB_DOS_INT86( 0x21, &regs, &regs );

         if( regs.h.al >= 10 )
         {
            char szHost[ 128 ];

            if( regs.h.al == 20 && regs.h.ah > 20 )
               snprintf( szHost, sizeof( szHost ), " (OS/2 %d.%02d)", regs.h.ah / 10, regs.h.ah % 10 );
            else
               snprintf( szHost, sizeof( szHost ), " (OS/2 %d.%02d)", regs.h.al / 10, regs.h.ah );

            hb_strncat( pszPlatform, szHost, 255 );
         }
      }
   }

#elif defined(HB_OS_OS2)

   {
      unsigned long aulQSV[ QSV_MAX ] = { 0 };
      APIRET rc;

      rc = DosQuerySysInfo( 1L, QSV_MAX, ( void * ) aulQSV, sizeof( ULONG ) * QSV_MAX );

      if( rc == 0 )
      {
         /* is this OS/2 2.x ? */
         if( aulQSV[ QSV_VERSION_MINOR - 1 ] < 30 )
         {
            snprintf( pszPlatform, 256, "OS/2 %ld.%02ld",
                      aulQSV[ QSV_VERSION_MAJOR - 1 ] / 10,
                      aulQSV[ QSV_VERSION_MINOR - 1 ] );
         }
         else
            snprintf( pszPlatform, 256, "OS/2 %2.2f",
                      ( float ) aulQSV[ QSV_VERSION_MINOR - 1 ] / 10 );
      }
      else
         snprintf( pszPlatform, 256, "OS/2" );
   }

#elif defined(HB_OS_WIN_32)

   {
      OSVERSIONINFOA osVer;

      osVer.dwOSVersionInfoSize = sizeof( osVer );

      if( GetVersionExA( &osVer ) )
      {
         char * pszName = "Windows";

         switch( osVer.dwPlatformId )
         {
            case VER_PLATFORM_WIN32_WINDOWS:

               if( osVer.dwMajorVersion == 4 && osVer.dwMinorVersion < 10 )
                  pszName = "Windows 95";
               else if( osVer.dwMajorVersion == 4 && osVer.dwMinorVersion == 10 )
                  pszName = "Windows 98";
               else
                  pszName = "Windows ME";

               break;

            case VER_PLATFORM_WIN32_NT:

               if( osVer.dwMajorVersion == 6 )
               {
#if !defined(HB_WINCE)
 #if defined(_MSC_VER)
  #if (_MSC_VER >= 1400)

                  OSVERSIONINFOEXA osVerEx;

                  osVerEx.dwOSVersionInfoSize = sizeof( osVerEx );

                  if( GetVersionExA( ( OSVERSIONINFOA * ) &osVerEx ) )
                  {
                     if( osVerEx.wProductType == VER_NT_WORKSTATION )
                        pszName = "Windows Vista";
                     else
                        pszName = "Windows Server 2008";
                  }
                  else
  #endif
 #endif
#endif
                     pszName = "Windows";
               }
               else if( osVer.dwMajorVersion == 5 && osVer.dwMinorVersion >= 2 )
                  pszName = "Windows Server 2003";
               else if( osVer.dwMajorVersion == 5 && osVer.dwMinorVersion == 1 )
                  pszName = "Windows XP";
               else if( osVer.dwMajorVersion == 5 )
                  pszName = "Windows 2000";
               else
                  pszName = "Windows NT";

               break;

            case VER_PLATFORM_WIN32s:
               pszName = "Windows 32s";
               break;

            case VER_PLATFORM_WIN32_CE:
               pszName = "Windows CE";
               break;
         }

         snprintf( pszPlatform, 256, "%s %lu.%lu.%04d",
                   pszName,
                   ( ULONG ) osVer.dwMajorVersion,
                   ( ULONG ) osVer.dwMinorVersion,
                   ( USHORT ) LOWORD( osVer.dwBuildNumber ) );

         /* Add service pack/other info */

         if( osVer.szCSDVersion )
         {
            int i;

            /* Skip the leading spaces (Win95B, Win98) */
            for( i = 0; osVer.szCSDVersion[ i ] != '\0' && isspace( ( int ) osVer.szCSDVersion[ i ] ); i++ );

            if( osVer.szCSDVersion[ i ] != '\0' )
            {
               hb_strncat( pszPlatform, " ", 255 );
               hb_strncat( pszPlatform, osVer.szCSDVersion + i, 255 );
            }
         }
      }
      else
         snprintf( pszPlatform, 256, "Windows" );
   }

#elif defined(__CEGCC__)
   {
      snprintf( pszPlatform, 256, "Windows" );
   }
#elif defined(HB_OS_UNIX)

   {
      struct utsname un;

      uname( &un );
      snprintf( pszPlatform, 256, "%s %s %s", un.sysname, un.release, un.machine );
   }

#elif defined(HB_OS_MAC)

   {
      hb_strncpy( pszPlatform, "MacOS compatible", 255 );
   }

#else

   {
      hb_strncpy( pszPlatform, "(unknown)", 255 );
   }

#endif

   return pszPlatform;
}

HB_EXPORT BOOL hb_iswinnt( void )
{
#if defined(HB_OS_WIN_32)
   static BOOL s_fWinNT = FALSE;
   static BOOL s_fInited = FALSE;

   if( ! s_fInited )
   {
      OSVERSIONINFO osvi;
      osvi.dwOSVersionInfoSize = sizeof( osvi );
      if( GetVersionEx( &osvi ) )
         s_fWinNT = osvi.dwPlatformId == VER_PLATFORM_WIN32_NT; /* && osvi.dwMajorVersion >= 4); */
      s_fInited = TRUE;
   }
   return s_fWinNT;
#else
   return FALSE;
#endif
}

HB_EXPORT BOOL hb_iswince( void )
{
#if defined(HB_WINCE)
   return TRUE;
#else
   return FALSE;
#endif
}

/* NOTE: The caller must free the returned buffer. [vszakats] */

char * hb_verCompiler( void )
{
   char * pszCompiler;
   char * pszName;
   char szSub[ 32 ];
   int iVerMajor;
   int iVerMinor;
   int iVerPatch;

   HB_TRACE(HB_TR_DEBUG, ("hb_verCompiler()"));

   pszCompiler = ( char * ) hb_xgrab( 80 );
   szSub[ 0 ] = '\0';

#if defined(__IBMC__) || defined(__IBMCPP__)

   #if defined(__IBMC__)
      iVerMajor = __IBMC__;
   #else
      iVerMajor = __IBMCPP__;
   #endif

   if( iVerMajor >= 300 )
      pszName = "IBM Visual Age C++";
   else
      pszName = "IBM C++";

   iVerMajor /= 100;
   iVerMinor = iVerMajor % 100;
   iVerPatch = 0;

#elif defined(__POCC__)

   pszName = "Pelles ISO C Compiler";
   iVerMajor = __POCC__ / 100;
   iVerMinor = __POCC__ % 100;
   iVerPatch = 0;

#elif defined(__XCC__)

   pszName = "Pelles ISO C Compiler";
   iVerMajor = __XCC__ / 100;
   iVerMinor = __XCC__ % 100;
   iVerPatch = 0;

#elif defined(__LCC__)

   pszName = "Logiciels/Informatique lcc-win32";
   iVerMajor = 0 /* __LCC__ / 100 */;
   iVerMinor = 0 /* __LCC__ % 100 */;
   iVerPatch = 0;

#elif defined(__DMC__)

   pszName = __DMC_VERSION_STRING__;
   iVerMajor = 0;
   iVerMinor = 0;
   iVerPatch = 0;

#elif defined(_MSC_VER)

   #if (_MSC_VER >= 800)
      pszName = "Microsoft Visual C";
   #else
      pszName = "Microsoft C";
   #endif

   #if defined(__cplusplus)
      hb_strncpy( szSub, "++", sizeof( szSub ) - 1 );
   #endif

   iVerMajor = _MSC_VER / 100;
   iVerMinor = _MSC_VER % 100;

   #if defined(_MSC_FULL_VER)
      iVerPatch = _MSC_FULL_VER - ( _MSC_VER * 10000 );
   #else
      iVerPatch = 0;
   #endif

#elif defined(__BORLANDC__)

   pszName = "Borland C++";
   #if (__BORLANDC__ == 1040) /* Version 3.1 */
      iVerMajor = 3;
      iVerMinor = 1;
      iVerPatch = 0;
   #elif (__BORLANDC__ >= 1280) /* Version 5.x */
      iVerMajor = __BORLANDC__ >> 8;
      iVerMinor = ( __BORLANDC__ & 0xFF ) >> 4;
      iVerPatch = __BORLANDC__ & 0xF;
   #else /* Version 4.x */
      iVerMajor = __BORLANDC__ >> 8;
      iVerMinor = ( __BORLANDC__ - 1 & 0xFF ) >> 4;
      iVerPatch = 0;
   #endif

#elif defined(__TURBOC__)

   pszName = "Borland Turbo C";
   iVerMajor = __TURBOC__ >> 8;
   iVerMinor = __TURBOC__ & 0xFF;
   iVerPatch = 0;

#elif defined(__MPW__)

   pszName = "MPW C";
   iVerMajor = __MPW__ / 100;
   iVerMinor = __MPW__ % 100;
   iVerPatch = 0;

#elif defined(__WATCOMC__)

   #if __WATCOMC__ < 1200
      pszName = "Watcom C";
   #else
      pszName = "Open Watcom C";
   #endif

   #if defined(__cplusplus)
      hb_strncpy( szSub, "++", sizeof( szSub ) - 1 );
   #endif

   iVerMajor = __WATCOMC__ / 100;
   iVerMinor = __WATCOMC__ % 100;

   #if defined( __WATCOM_REVISION__ )
      iVerPatch = __WATCOM_REVISION__;
   #else
      iVerPatch = 0;
   #endif

#elif defined(__GNUC__)

   #if defined(__DJGPP__)
      pszName = "Delorie GNU C";
   #elif defined(__CYGWIN__)
      pszName = "Cygwin GNU C";
   #elif defined(__MINGW32__)
      pszName = "MinGW GNU C";
   #elif defined(__RSX32__)
      pszName = "EMX/RSXNT/DOS GNU C";
   #elif defined(__RSXNT__)
      pszName = "EMX/RSXNT/Win32 GNU C";
   #elif defined(__EMX__)
      pszName = "EMX GNU C";
   #else
      pszName = "GNU C";
   #endif

   #if defined(__cplusplus)
      hb_strncpy( szSub, "++", sizeof( szSub ) - 1 );
   #endif

   iVerMajor = __GNUC__;
   iVerMinor = __GNUC_MINOR__;
   #if defined(__GNUC_PATCHLEVEL__)
      iVerPatch = __GNUC_PATCHLEVEL__;
   #else
      iVerPatch = 0;
   #endif
#else

   pszName = ( char * ) NULL;
   iVerMajor = iVerMinor = iVerPatch = 0;

#endif

   if( pszName )
   {
      if( iVerPatch != 0 )
         snprintf( pszCompiler, 80, "%s%s %hd.%hd.%hd", pszName, szSub, iVerMajor, iVerMinor, iVerPatch );
      else if( iVerMajor != 0 || iVerMinor != 0 )
         snprintf( pszCompiler, 80, "%s%s %hd.%hd", pszName, szSub, iVerMajor, iVerMinor );
      else
         snprintf( pszCompiler, 80, "%s%s", pszName, szSub );
   }
   else
      hb_strncpy( pszCompiler, "(unknown)", 79 );

#if defined(__DJGPP__)

   snprintf( szSub, sizeof( szSub ), " (DJGPP %i.%02i)", ( int ) __DJGPP__, ( int ) __DJGPP_MINOR__ );
   hb_strncat( pszCompiler, szSub, 79 );

#elif defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__GNUC__)

   #if defined( HB_ARCH_16BIT )
      hb_strncat( pszCompiler, " (16 bit)", 79 );
   #elif defined( HB_ARCH_32BIT )
      hb_strncat( pszCompiler, " (32 bit)", 79 );
   #elif defined( HB_ARCH_64BIT )
      hb_strncat( pszCompiler, " (64 bit)", 79 );
   #endif

#endif

   return pszCompiler;
}

/* NOTE: The caller must free the returned buffer. [vszakats] */

char * hb_verHarbour( void )
{
   char * pszVersion;

   HB_TRACE(HB_TR_DEBUG, ("hb_verHarbour()"));

   pszVersion = ( char * ) hb_xgrab( 80 );

   /* NOTE: 
      CA-Cl*pper 5.2e returns: "Clipper (R) 5.2e Intl. (x216)  (1995.02.07)"
      CA-Cl*pper 5.3b returns: "Clipper (R) 5.3b Intl. (Rev. 338) (1997.04.25)"
   */

   snprintf( pszVersion, 80, "Harbour %d.%d.%d%s Intl. (Rev. %d)",
             HB_VER_MAJOR, HB_VER_MINOR, HB_VER_REVISION, HB_VER_STATUS,
             hb_verSvnID() );

   return pszVersion;
}

char * hb_verPCode( void )
{
   char * pszPCode;

   HB_TRACE(HB_TR_DEBUG, ("hb_verPCode()"));

   pszPCode = ( char * ) hb_xgrab( 24 );

   snprintf( pszPCode, 24, "PCode version: %d.%d",
             HB_PCODE_VER >> 8, HB_PCODE_VER & 0xff );

   return pszPCode;
}

char * hb_verBuildDate( void )
{
   char * pszDate;

   HB_TRACE(HB_TR_DEBUG, ("hb_verBuildDate()"));

   pszDate = ( char * ) hb_xgrab( 64 );
   snprintf( pszDate, 64, "%s %s", __DATE__, __TIME__ );

   return  pszDate;
}
