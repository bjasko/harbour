/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Preprocessor standalone main module
 *
 * Copyright 1999 Alexander S.Kresin <alex@belacy.belgorod.su>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version, with one exception:
 *
 * The exception is that if you link the Harbour Runtime Library (HRL)
 * and/or the Harbour Virtual Machine (HVM) with other files to produce
 * an executable, this does not by itself cause the resulting executable
 * to be covered by the GNU General Public License. Your use of that
 * executable is in no way restricted on account of linking the HRL
 * and/or HVM code into it.
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

/*
 * Avoid tracing in preprocessor/compiler.
 */
#if ! defined(HB_TRACE_UTILS)
   #if defined(HB_TRACE_LEVEL)
      #undef HB_TRACE_LEVEL
   #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>     /* required for allocating and freeing memory */

#include "hbpp.h"
#include "hbcomp.h"

extern int hb_pp_ParseDefine( char * );

static int hb_pp_Parse( FILE * handl_o );
static void AddSearchPath( char * szPath, PATHNAMES * * pSearchList );
static void OutTable( DEFINES * endDefine, COMMANDS * endCommand );
static BOOL hb_pp_fopen( char * szFileName );

static char s_szLine[ HB_PP_STR_SIZE ];
static char s_szOutLine[ HB_PP_STR_SIZE ];
static int  s_iWarnings = 0;
static char * hb_buffer;

PATHNAMES * hb_comp_pIncludePath = NULL;
PHB_FNAME   hb_comp_pFileName = NULL;
FILES       hb_comp_files;
int         hb_comp_iLine = 1; /* currently parsed file line number */

/* These are need for the PP #pragma support */
BOOL hb_comp_bPPO = FALSE;                      /* flag indicating, is ppo output needed */
BOOL hb_comp_bStartProc = TRUE;                 /* holds if we need to create the starting procedure */
BOOL hb_comp_bLineNumbers = TRUE;               /* holds if we need pcodes with line numbers */
BOOL hb_comp_bShortCuts = TRUE;                 /* .and. & .or. expressions shortcuts */
int  hb_comp_iWarnings = 0;                     /* enable parse warnings */
BOOL hb_comp_bAutoMemvarAssume = FALSE;         /* holds if undeclared variables are automatically assumed MEMVAR (-a)*/
BOOL hb_comp_bForceMemvars = FALSE;             /* holds if memvars are assumed when accesing undeclared variable (-v)*/
BOOL hb_comp_bDebugInfo = FALSE;                /* holds if generate debugger required info */
int  hb_comp_iExitLevel = HB_EXITLEVEL_DEFAULT; /* holds if there was any warning during the compilation process */
FILE *hb_comp_yyppo = NULL;

int  hb_comp_iLinePRG;
int  hb_comp_iLineINLINE = 0;

int main( int argc, char * argv[] )
{
  FILE * handl_o;
  char szFileName[ _POSIX_PATH_MAX ];
  char szPpoName[ _POSIX_PATH_MAX ];
  char * szDefText;
  int iArg = 1;
  unsigned int i;
  BOOL bOutTable = FALSE;
  BOOL bOutNew = FALSE;
  DEFINES * stdef;
  COMMANDS * stcmd;

  HB_TRACE(HB_TR_DEBUG, ("main(%d, %p)", argc, argv));

  printf( "Harbour Preprocessor %d.%d%s (Build %d) (%04d.%02d.%02d)\n",
    HB_VER_MAJOR, HB_VER_MINOR, HB_VER_REVISION, HB_VER_BUILD, HB_VER_YEAR, HB_VER_MONTH, HB_VER_DAY );
  printf( "Copyright 1999-2000, http://www.harbour-project.org\n" );

  hb_pp_Table();
  stdef = hb_pp_topDefine;
  stcmd = hb_pp_topCommand;
  hb_pp_Init();

  while( iArg < argc )
    {
      if( HB_ISOPTSEP(argv[ iArg ][ 0 ]))
        {
          switch( argv[ iArg ][ 1 ] )
            {
            case 'd':
            case 'D':   /* defines a #define from the command line */
              {
                 char *szDefText = hb_strdup( argv[iArg] + 2 ), *pAssign, *sDefLine;
                 unsigned int i = 0;

                 while( i < strlen( szDefText ) && ! HB_ISOPTSEP( szDefText[ i ] ) )
                    i++;

                 szDefText[ i ] = '\0';
                 if( szDefText )
                 {
                    if( ( pAssign = strchr( szDefText, '=' ) ) == NULL )
                    {
                       hb_pp_AddDefine( szDefText, 0 );
                    }
                    else
                    {
                       szDefText[ pAssign - szDefText ] = '\0';

                       //hb_pp_AddDefine( szDefText,  pAssign + 1 );
                       sDefLine = (char*) hb_xgrab( strlen( szDefText ) + 1 + strlen( pAssign + 1 ) + 1 );
                       sprintf( sDefLine, "%s %s", szDefText, pAssign + 1 );
                       hb_pp_ParseDefine( sDefLine );
                       hb_xfree( sDefLine );
                    }
                 }

                 hb_xfree( szDefText );
              }
              break;
            case 'i':
            case 'I':
              AddSearchPath( argv[ iArg ]+2, &hb_comp_pIncludePath );
              break;
            case 'o':
            case 'O':
              bOutTable = TRUE;
              break;
            case 'n':
            case 'N':
              bOutNew = TRUE;
              break;
            case 'w':
            case 'W':
              s_iWarnings = 1;
              if( argv[ iArg ][ 2 ] )
                {  /*there is -w<0,1,2,3> probably */
                  s_iWarnings = argv[ iArg ][ 2 ] - '0';
                  if( s_iWarnings < 0 || s_iWarnings > 3 )
                    printf( "\nInvalid command line option: %s\n", argv[ iArg ] );
                }
              break;
            default:
              printf( "\nInvalid command line option: %s\n", &argv[ iArg ][ 1 ] );
              break;
            }
        }
      else  hb_comp_pFileName = hb_fsFNameSplit( argv[ iArg ] );
      iArg++;
    }

  if( hb_comp_pFileName )
    {
      if( ! hb_comp_pFileName->szExtension )
        hb_comp_pFileName->szExtension =".prg";

      hb_fsFNameMerge( szFileName, hb_comp_pFileName );

      if( !hb_pp_fopen( szFileName ) )
        {
          printf("\nCan't open %s\n", szFileName );
          return 1;
        }

      printf( "\nParsing file %s\n", szFileName );
    }
  else
    {
      printf( "\nSyntax:  %s <file[.prg]> [options]"
              "\n"
              "\nOptions:  /d<id>[=<val>]   #define <id>"
              "\n          /i<path>         add #include file search path"
              "\n          /o               creates hbpp.out with all tables"
              "\n          /n               with those only, which defined in your file"
              "\n          /w               enable warnings"
              "\n"
              , argv[ 0 ] );

      if( bOutTable )
        OutTable( NULL, NULL );

      return 1;
    }

  hb_comp_pFileName->szExtension = ".ppo";
  hb_fsFNameMerge( szPpoName, hb_comp_pFileName );

  if( ( handl_o = fopen( szPpoName, "wt" ) ) == NULL )
    {
      printf("\nCan't open %s\n", szPpoName );
      return 1;
    }

  {
    char * szInclude = getenv( "INCLUDE" );

    if( szInclude )
      {
        char * pPath;
        char * pDelim;

        pPath = szInclude = hb_strdup( szInclude );
        while( ( pDelim = strchr( pPath, OS_PATH_LIST_SEPARATOR ) ) != NULL )
          {
            *pDelim = '\0';
            AddSearchPath( pPath, &hb_comp_pIncludePath );
            pPath = pDelim + 1;
          }
        AddSearchPath( pPath, &hb_comp_pIncludePath );
      }
  }

  hb_buffer = ( char* ) hb_xgrab( HB_PP_STR_SIZE );
  while( hb_pp_Internal( handl_o,hb_buffer ) > 0 );
  fclose( hb_comp_files.pLast->handle );
  hb_xfree( hb_comp_files.pLast->pBuffer );
  hb_xfree( hb_comp_files.pLast );
  hb_xfree( hb_buffer );
  fclose( handl_o );

  if( bOutTable )
    OutTable( NULL, NULL );
  else if( bOutNew )
    OutTable( stdef, stcmd );

  printf( "\nOk" );

  return 0;
}

static void OutTable( DEFINES * endDefine, COMMANDS * endCommand )
{
  FILE *handl_o;
  int ipos, len_mpatt = 0, len_value;
  int num;
  DEFINES * stdef1 = hb_pp_topDefine, * stdef2 = NULL, * stdef3;
  COMMANDS * stcmd1 = hb_pp_topCommand, * stcmd2 = NULL, * stcmd3;

  HB_TRACE(HB_TR_DEBUG, ("OutTable(%p, %p)", endDefine, endCommand));

  while( stdef1 != endDefine )
    {
      stdef3 = stdef1->last;
      stdef1->last = stdef2;
      stdef2 = stdef1;
      stdef1 = stdef3;
    }
  while( stcmd1 != endCommand )
    {
      stcmd3 = stcmd1->last;
      stcmd1->last = stcmd2;
      stcmd2 = stcmd1;
      stcmd1 = stcmd3;
    }

  if( ( handl_o = fopen( "hbpp.out", "wt" ) ) == NULL )
    {
      printf( "\nCan't open hbpp.out\n" );
      return;
    }

  num = 1;
  while( stdef2 != NULL )
    {
      fprintf( handl_o, "\n   static DEFINES sD___%i = ", num );
      fprintf( handl_o, "{\"%s\",", stdef2->name );
      if( stdef2->pars )
        fprintf( handl_o, "\"%s\",", stdef2->pars );
      else
        fprintf( handl_o, "NULL," );
      fprintf( handl_o, "%d,", stdef2->npars );
      if( stdef2->value )
        fprintf( handl_o, "\"%s\"", stdef2->value );
      else
        fprintf( handl_o, "NULL" );
      if( num == 1 )
        fprintf( handl_o, ", NULL };" );
      else
        fprintf( handl_o, ", &sD___%i };", num - 1 );
      stdef2 = stdef2->last;
      num++;
    }
  fprintf( handl_o, "\n   DEFINES * hb_pp_topDefine = " );
  if( num == 1 )
    fprintf( handl_o, "NULL;" );
  else
    fprintf( handl_o, " = &sD___%i;\n", num - 1 );

  num = 1;
  while( stcmd2 != NULL )
    {
      fprintf( handl_o, "\n   static COMMANDS sC___%i = ", num );
      fprintf( handl_o, "{%d,\"%s\",", stcmd2->com_or_xcom, stcmd2->name );
      if( stcmd2->mpatt != NULL )
        {
          len_mpatt = hb_pp_strocpy( s_szLine, stcmd2->mpatt );
          while( ( ipos = hb_strAt( "\1", 1, s_szLine, len_mpatt ) ) > 0 )
            {
              hb_pp_Stuff( "\\1", s_szLine + ipos - 1, 2, 1, len_mpatt );
              len_mpatt++;
            }
          fprintf( handl_o, "\"%s\",", s_szLine );
        }
      else
        fprintf( handl_o, "NULL," );
      if( stcmd2->value != NULL )
        {
          len_value = hb_pp_strocpy( s_szLine, stcmd2->value );
          while( ( ipos = hb_strAt( "\1", 1, s_szLine, len_value ) ) > 0 )
            {
              hb_pp_Stuff( "\\1", s_szLine + ipos - 1, 2, 1, len_value );
              len_value++;
            }
          if( len_mpatt + len_value > 80 )
            fprintf( handl_o, "\n       " );
          fprintf( handl_o, "\"%s\"", s_szLine );
        }
      else fprintf( handl_o, "NULL" );
      if( num == 1 )
        fprintf( handl_o, ",NULL };" );
      else
        fprintf( handl_o, ",&sC___%i };", num - 1 );
      stcmd2 = stcmd2->last;
      num++;
    }
  fprintf( handl_o, "\n   COMMANDS * hb_pp_topCommand = " );
  if( num == 1 )
    fprintf( handl_o, "NULL;" );
  else
    fprintf( handl_o, " = &sC___%i;\n", num - 1 );

  stcmd1 = hb_pp_topTranslate;
  stcmd2 = NULL;
  while( stcmd1 != NULL )
    {
      stcmd3 = stcmd1->last;
      stcmd1->last = stcmd2;
      stcmd2 = stcmd1;
      stcmd1 = stcmd3;
    }
  num = 1;
  while( stcmd2 != NULL )
    {
      fprintf( handl_o, "\n   static COMMANDS sC___%i = ", num );
      fprintf( handl_o, "{%d,\"%s\",", stcmd2->com_or_xcom, stcmd2->name );
      if( stcmd2->mpatt != NULL )
        {
          len_mpatt = hb_pp_strocpy( s_szLine, stcmd2->mpatt );
          while( ( ipos = hb_strAt( "\1", 1, s_szLine, len_mpatt ) ) > 0 )
            {
              hb_pp_Stuff( "\\1", s_szLine + ipos - 1, 2, 1, len_mpatt );
              len_mpatt++;
            }
          fprintf( handl_o, "\"%s\",", s_szLine );
        }
      else
        fprintf( handl_o, "NULL," );
      if( stcmd2->value != NULL )
        {
          len_value = hb_pp_strocpy( s_szLine, stcmd2->value );
          while( ( ipos = hb_strAt( "\1", 1, s_szLine, len_value ) ) > 0 )
            {
              hb_pp_Stuff( "\\1", s_szLine + ipos - 1, 2, 1, len_value );
              len_value++;
            }
          if( len_mpatt + len_value > 80 )
            fprintf( handl_o, "\n       " );
          fprintf( handl_o, "\"%s\"", s_szLine );
        }
      else fprintf( handl_o, "NULL" );
      if( num == 1 )
        fprintf( handl_o, ",NULL };" );
      else
        fprintf( handl_o, ",&sC___%i };", num - 1 );
      stcmd2 = stcmd2->last;
      num++;
    }
  fprintf( handl_o, "\n   COMMANDS * hb_pp_topTranslate = " );
  if( num == 1 )
    fprintf( handl_o, "NULL;" );
  else
    fprintf( handl_o, " = &sT___%i;", num );

  fclose( handl_o );
}

/*
 * Function that adds specified path to the list of pathnames to search
 */
static void AddSearchPath( char * szPath, PATHNAMES * * pSearchList )
{
  PATHNAMES * pPath = *pSearchList;

  HB_TRACE(HB_TR_DEBUG, ("AddSearchPath(%s, %p)", szPath, pSearchList));

  if( pPath )
    {
      while( pPath->pNext )
        pPath = pPath->pNext;
      pPath->pNext = ( PATHNAMES * ) hb_xgrab( sizeof( PATHNAMES ) );
      pPath = pPath->pNext;
    }
  else
    {
      *pSearchList = pPath = ( PATHNAMES * ) hb_xgrab( sizeof( PATHNAMES ) );
    }
  pPath->pNext  = NULL;
  pPath->szPath = szPath;
}

void hb_compGenError( char * _szErrors[], char cPrefix, int iError, const char * szError1, const char * szError2 )
{
  HB_TRACE(HB_TR_DEBUG, ("hb_compGenError(%p, %c, %d, %s, %s)", _szErrors, cPrefix, iError, szError1, szError2));

  printf( "\r(%i) ", hb_comp_iLine );
  printf( "Error %c%04i  ", cPrefix, iError );
  printf( _szErrors[ iError - 1 ], szError1, szError2 );
  printf( "\n\n" );

  exit( EXIT_FAILURE );
}

void hb_compGenWarning( char* _szWarnings[], char cPrefix, int iWarning, const char * szWarning1, const char * szWarning2)
{
  HB_TRACE(HB_TR_DEBUG, ("hb_compGenWarning(%p, %c, %d, %s, %s)", _szWarnings, cPrefix, iWarning, szWarning1, szWarning2));

  if( s_iWarnings )
    {
      char *szText = _szWarnings[ iWarning - 1 ];

      if( (szText[ 0 ] - '0') <= s_iWarnings )
        {
          printf( "\r(%i) ", hb_comp_iLine );
          printf( "Warning %c%04i  ", cPrefix, iWarning );
          printf( szText + 1, szWarning1, szWarning2 );
          printf( "\n" );
        }
    }
}

void * hb_xgrab( ULONG ulSize )         /* allocates fixed memory, exits on failure */
{
  void * pMem = malloc( ulSize );

  HB_TRACE(HB_TR_DEBUG, ("hb_xgrab(%lu)", ulSize));

  if( ! pMem )
    hb_compGenError( hb_pp_szErrors, 'P', HB_PP_ERR_MEMALLOC, NULL, NULL );

  return pMem;
}

void * hb_xrealloc( void * pMem, ULONG ulSize )       /* reallocates memory */
{
  void * pResult = realloc( pMem, ulSize );

  HB_TRACE(HB_TR_DEBUG, ("hb_xrealloc(%p, %lu)", pMem, ulSize));

  if( ! pResult )
    hb_compGenError( hb_pp_szErrors, 'P', HB_PP_ERR_MEMREALLOC, NULL, NULL );

  return pResult;
}

void hb_xfree( void * pMem )            /* frees fixed memory */
{
  HB_TRACE(HB_TR_DEBUG, ("hb_xfree(%p)", pMem));

  if( pMem )
    free( pMem );
  else
    hb_compGenError( hb_pp_szErrors, 'P', HB_PP_ERR_MEMFREE, NULL, NULL );
}

BOOL hb_pp_fopen( char * szFileName )
{
   PFILE pFile;
   FILE * handl_i = fopen( szFileName, "r" );

   if( !handl_i )
      return FALSE;

   pFile = ( PFILE ) hb_xgrab( sizeof( _FILE ) );
   pFile->handle = handl_i;
   pFile->pBuffer = hb_xgrab( HB_PP_BUFF_SIZE );
   pFile->iBuffer = pFile->lenBuffer = 10;
   pFile->szFileName = szFileName;
   pFile->pPrev = NULL;

   hb_comp_files.pLast = pFile;
   hb_comp_files.iFiles = 1;

   return TRUE;
}

PINLINE   hb_compInlineAdd( char * szFunName )
{
   HB_SYMBOL_UNUSED( szFunName );
   return NULL;
}
