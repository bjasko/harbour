/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Native compiler object module generation from Harbour C output.
 *
 * Copyright 2001 Jos� Lal�n <dezac@corevia.com>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA (or visit
 * their web site at http://www.gnu.org/).
 *
 */

#include "hbcomp.h"
#include "hb_io.h"

#define HB_CFG_FILENAME    "harbour.cfg"

/* QUESTION: Allocate buffer dynamically ? */
#define HB_CFG_LINE_LEN    ( _POSIX_PATH_MAX << 1 )

#if defined( OS_UNIX_COMPATIBLE )
   #define HB_NULL_STR " > /dev/null"
#elif defined( OS_DOS_COMPATIBLE )
   #define HB_NULL_STR " >nul"      
#endif

/*--------------------------------------------------------------------------*/

static char * hb_searchpath( const char * pszFile, char * pszEnv, char * pszCfg )
{
   char * pszPath;
   BOOL bFound = FALSE;

   /* Check current dir first  */
   if( hb_fsFileExists( ( const char * ) pszFile ) )
   {
      snprintf( pszCfg, _POSIX_PATH_MAX + 1, "%s", pszFile );
      return ( char * ) pszFile;
   }
   else
   {
      /* Check if pszFile exists somewhere in the path */
      while( * pszEnv )
      {
         pszPath = pszEnv;
         while( *pszEnv )
         {
            if( *pszEnv == OS_PATH_LIST_SEPARATOR )
            {
               *pszEnv++ = '\0';
               break;
            }
            pszEnv++;
         }
         if( *pszPath )
         {
            snprintf( pszCfg, _POSIX_PATH_MAX + 1, "%s%c%s", pszPath, OS_PATH_DELIMITER, pszFile );
            if( hb_fsFileExists( ( const char * ) pszCfg ) )
            {
               bFound = TRUE;
               break;
            }
         }
      }
   }

   /* If not found, make sure to return a NULL string */
   if( ! bFound )
      *pszCfg = '\0';

   return ( char * ) pszCfg;
}

static void hb_substenvvar( char * szLine )
{
   char szBuf[ HB_CFG_LINE_LEN + 1 ], * szVar, * ptr;

   ptr = szLine;
   while( *ptr )
   {
      if( *ptr == '$' && ptr[ 1 ] == '(' )
      {
         int i = 2, j;
         while( ( ptr[ i ] >= 'A' && ptr[ i ] <= 'Z' ) ||
                ( ptr[ i ] >= 'a' && ptr[ i ] <= 'z' ) ||
                ( ptr[ i ] >= '0' && ptr[ i ] <= '0' ) || ptr[ i ] == '_' )
            ++i;
         if( i > 2 && ptr[ i ] == ')' )
         {
            ptr[ 0 ] = 0;
            ptr[ i ] = 0;
            hb_strncpy( szBuf, szLine, HB_CFG_LINE_LEN );
            szVar = hb_getenv( ptr + 2 );
            if( szVar )
            {
               hb_strncat( szBuf, szVar, HB_CFG_LINE_LEN );
               hb_xfree( szVar );
            }
            j = strlen( szBuf );
            hb_strncat( szBuf, &ptr[ i + 1 ], HB_CFG_LINE_LEN );
            hb_strncpy( szLine, szBuf, HB_CFG_LINE_LEN );
            ptr = szLine + j;
         }
      }
      ++ptr;
   }
}

/* Builds platform dependant object module from Harbour C output */
void hb_compGenCObj( HB_COMP_DECL, PHB_FNAME pFileName )
{
   char szFileName[ _POSIX_PATH_MAX + 1 ];
   char szLine[ HB_CFG_LINE_LEN + 1 ];
   char szCompiler[ HB_CFG_LINE_LEN + 1 ] = "";
   char szOptions[ HB_CFG_LINE_LEN + 1 ] = "";
   char szCommandLine[ HB_CFG_LINE_LEN * 2 + 1 ];
   char szOutPath[ _POSIX_PATH_MAX + 1 ] = "\0";
   char pszTemp[ _POSIX_PATH_MAX + 1 ] = "";
   char buffer[ HB_CFG_LINE_LEN * 2 + 1024 ];
#if defined( OS_UNIX_COMPATIBLE )
   char * pszEnv = hb_strdup( "/etc:/usr/local/etc" );
#elif defined( OS_DOS_COMPATIBLE )
   char * pszEnv = hb_getenv( "PATH" );
#else
   char * pszEnv = NULL;
#endif
   char * pszCfgFileName = hb_getenv( "HB_CFG_FILE" );
   FILE * filecfg;
   BOOL bVerbose = FALSE;   /* Don't show C compiler messages (default). */
   BOOL bDelTmp = TRUE;     /* Delete intermediate C file (default). */
   int iSuccess;

   HB_SYMBOL_UNUSED( HB_COMP_PARAM );

   /* First pass: build the C output */

   /* Force file extension to avoid collisions when called from a make utility */
   pFileName->szExtension = ".c";
   hb_fsFNameMerge( szFileName, pFileName );
   hb_compGenCCode( HB_COMP_PARAM, HB_COMP_PARAM->pFileName );

   /* Begin second pass */

   /* Set up things  */
   if( !pszCfgFileName )
      pszCfgFileName = hb_strdup( HB_CFG_FILENAME );

   if( pszEnv && *hb_searchpath( pszCfgFileName, pszEnv, pszTemp ) )
   {
      filecfg = hb_fopen( pszTemp, "rt" );
      if( ! filecfg )
      {
         hb_compGenError( HB_COMP_PARAM, hb_comp_szErrors, 'E', HB_COMP_ERR_OPEN_CFG, szFileName, NULL );
         if( pszEnv )
            hb_xfree( ( void * ) pszEnv );
         hb_xfree( ( void * ) pszCfgFileName );
         return;
      }

      while( fgets( szLine, HB_CFG_LINE_LEN, filecfg ) != NULL )
      {
         ULONG ulLen;
         char * szStr = szLine;
         char * szToken;

         hb_substenvvar( szLine );

         /* Trim left */
         while( HB_ISSPACE( *szStr ) )
            szStr++;

         /* Trim right */
         ulLen = strlen( szStr );
         while( ulLen && HB_ISSPACE( szStr[ ulLen - 1 ] ) )
            ulLen--;

         szStr[ ulLen ] = '\0';
         /* TODO: Check for comments within macros, i.e: CC=bcc32 #comment */

         if( *szStr )
         {
            szToken = strchr( szStr, '=' );
         
            if( szToken )
            {
               *szToken++ = '\0';
               if( *szToken )
               {
                  /* Checks compiler name */
                  if( ! hb_stricmp( szStr, "CC" ) )
                  {
                     snprintf( szCompiler, sizeof( szCompiler ), "%s", szToken );
                  }
                  /* Checks optional switches */
                  else if( ! hb_stricmp( szStr, "CFLAGS" ) )
                  {
                     snprintf( szOptions, sizeof( szCompiler ), "%s", szToken );
                  }
                  /* Wanna see C compiler output ? */
                  else if( ! hb_stricmp( szStr, "VERBOSE" ) )
                  {
                     if( ! hb_stricmp( szToken, "YES" ) )
                        bVerbose = TRUE;
                  }
                  /* Delete intermediate C file ? */
                  else if( ! hb_stricmp( szStr, "DELTMP" ) )
                  {
                     if( ! hb_stricmp( szToken, "NO" ) )
                        bDelTmp = FALSE;
                  }
               }
            }
         }
      }

      fclose( filecfg );

   } else {
      snprintf( buffer, sizeof( buffer ),
                "\nError: Can't find %s file in %s.\n"
                "%s should be a text file that contains:\n"
                "CC=C compiler binary name eg. CC=gcc\n"
                "CFLAGS=C compiler options eg. -c -I<includes>\n"
                "       ( 'compile only' and harbour include dir are mandatory )\n"
                "VERBOSE=NO|YES to show steps messages default is NO\n"
                "DELTMP=NO|YES to delete generated C source default is YES\n"
                "remember also to properly set the C compiler env.\n",
                pszCfgFileName, pszEnv, pszCfgFileName );
      hb_compOutStd( HB_COMP_PARAM, buffer );

      if( pszEnv )
         hb_xfree( ( void * ) pszEnv );
      hb_xfree( ( void * ) pszCfgFileName );
      return;
   }

   if( pszEnv )
      hb_xfree( ( void * ) pszEnv );

   if( ! HB_COMP_PARAM->fQuiet )
   {
      snprintf( buffer, sizeof( buffer ),
                "\nBuilding object module for \'%s\'\nusing C compiler \'%s\' as defined in \'%s\'...\n",
                szFileName, szCompiler, pszCfgFileName );
      hb_compOutStd( HB_COMP_PARAM, buffer );
   }

   /* Check if -o<path> was used */
   if( HB_COMP_PARAM->pOutPath )
   {
      PHB_FNAME pOut = hb_fsFNameSplit( ( char * ) szFileName );

      if( HB_COMP_PARAM->pOutPath->szPath )
         pOut->szPath = HB_COMP_PARAM->pOutPath->szPath;

#if defined(__BORLANDC__) || defined(_MSC_VER) || defined(__WATCOMC__)
      pOut->szExtension = ".obj";
#else
      pOut->szExtension = ".o";  /* Don't know if we can hardcode it for Un*x */
#endif
      hb_fsFNameMerge( pszTemp, pOut );

#if defined(_MSC_VER)
      hb_strncat( szOutPath, "-Fo", sizeof( szOutPath ) - 1 );
#elif defined(__WATCOMC__)
      hb_strncat( szOutPath, "-fo=", sizeof( szOutPath ) - 1 );      
#else
      hb_strncat( szOutPath, "-o", sizeof( szOutPath ) - 1 );
#endif

      hb_strncat( szOutPath, pszTemp, sizeof( szOutPath ) - 1 );

      hb_xfree( pOut );
   }

   if( *szCompiler )
   {
      snprintf( szCommandLine, sizeof( szCommandLine ), "%s %s %s %s", szCompiler, szOptions, szOutPath, szFileName );

      if( bVerbose )
      {
         snprintf( buffer, sizeof( buffer ), "Exec: %s\n", szCommandLine );
         hb_compOutStd( HB_COMP_PARAM, buffer );
      }
      else
         hb_strncat( szCommandLine, HB_NULL_STR, sizeof( szCommandLine ) - 1 );

      /* Compile it! */
      iSuccess = ( system( szCommandLine ) != -1 );

      /* Delete intermediate .c file */
      /* QUESTION: Leave this file if C compiler fails ? */
      if( bDelTmp ) /* && iSuccess ) */
      {
         if( bVerbose )
         {
            snprintf( buffer, sizeof( buffer ), "Deleting: \"%s\"\n", szFileName );
            hb_compOutStd( HB_COMP_PARAM, buffer );
         }
         remove( ( char * ) szFileName );
      }

      if( ! HB_COMP_PARAM->fQuiet )
      {
         if( iSuccess )
            hb_compOutStd( HB_COMP_PARAM, "Done.\n" );
         else
         {
            snprintf( buffer, sizeof( buffer ),
                      "\nFailed to execute: \"%s\"\n", szCommandLine );
            hb_compOutErr( HB_COMP_PARAM, buffer );
         }
      }
   }
   else
   {
      snprintf( buffer, sizeof( buffer ),
                "\nError: No compiler defined in %s\n", pszCfgFileName );
      hb_compOutErr( HB_COMP_PARAM, buffer );
   }

   hb_xfree( ( void * ) pszCfgFileName );
}
