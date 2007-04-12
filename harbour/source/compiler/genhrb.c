/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Compiler Harbour Portable Object (.HRB) generation
 *
 * Copyright 1999 Eddie Runia <eddie@runia.com>
 * www - http://www.harbour-project.org
 *
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
 *    rewritten to work on memory buffers and with new compiler code
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

#define SYM_NOLINK  0              /* Symbol does not have to be linked */
#define SYM_FUNC    1              /* Defined function                  */
#define SYM_EXTERN  2              /* Previously defined function       */

static PFUNCTION hb_compFirstFunc( HB_COMP_DECL )
{
   PFUNCTION pFunc = HB_COMP_PARAM->functions.pFirst;
   if( ! HB_COMP_PARAM->fStartProc )
      pFunc = pFunc->pNext;
   return pFunc;
}

static ULONG hb_compHrbSize( HB_COMP_DECL, ULONG * pulSymbols, ULONG * pulFunctions )
{
   PFUNCTION pFunc;
   PCOMSYMBOL pSym;
   ULONG ulSize;

   * pulSymbols = * pulFunctions = 0;

   /* count total size */
   ulSize = 10;  /* signature[4] + version[2] + symbols_number[4] */
   pSym = HB_COMP_PARAM->symbols.pFirst;
   while( pSym )
   {
      ( * pulSymbols )++;
      ulSize += strlen( pSym->szName ) + 3; /* \0 + symscope[1] + symtype[1] */
      pSym = pSym->pNext;
   }
   ulSize += 4; /* functions_number[4] */
   /* Generate functions data */
   pFunc = hb_compFirstFunc( HB_COMP_PARAM );
   while( pFunc )
   {
      ( * pulFunctions )++;
      ulSize += strlen( pFunc->szName ) + 5 + pFunc->lPCodePos; /* \0 + func_size[4] + function_body */
      pFunc = pFunc->pNext;
   }

   return ulSize;
}

void hb_compGenBufPortObj( HB_COMP_DECL, BYTE ** pBufPtr, ULONG * pulSize )
{
   PFUNCTION pFunc;
   PCOMSYMBOL pSym;
   ULONG ulSymbols, ulFunctions, ulLen;
   BYTE * ptr;

   * pulSize = hb_compHrbSize( HB_COMP_PARAM, &ulSymbols, &ulFunctions );
   /* additional 0 byte is for passing buffer directly as string item */
   ptr = * pBufPtr = ( BYTE * ) hb_xgrab( * pulSize + 1 );

   memcpy( ptr, "\300HRB", 4 );  /* signature */
   ptr += 4;
   HB_PUT_LE_UINT16( ptr, 2 );   /* version number */
   ptr += 2;

   HB_PUT_LE_UINT32( ptr, ulSymbols ); /* number of symbols */
   ptr += 4;
   /* generate the symbol table */
   pSym = HB_COMP_PARAM->symbols.pFirst;
   while( pSym )
   {
      ulLen = strlen( pSym->szName ) + 1;
      memcpy( ptr, pSym->szName, ulLen );
      ptr += ulLen;
      *ptr++ = pSym->cScope;
      /* symbol type */
      /* if( hb_compFunctionFind( HB_COMP_PARAM, pSym->szName ) ) */
      if( pSym->cScope & HB_FS_LOCAL )
         *ptr++ = SYM_FUNC;   /* function defined in this module */
      else if( hb_compFunCallFind( HB_COMP_PARAM, pSym->szName ) )
         *ptr++ = SYM_EXTERN; /* external function */
      else
         *ptr++ = SYM_NOLINK; /* other symbol */
      pSym = pSym->pNext;
   }

   HB_PUT_LE_UINT32( ptr, ulFunctions );  /* number of functions */
   ptr += 4;
   /* generate functions data */
   pFunc = hb_compFirstFunc( HB_COMP_PARAM );
   while( pFunc )
   {
      ulLen = strlen( pFunc->szName ) + 1;
      memcpy( ptr, pFunc->szName, ulLen );
      ptr += ulLen;
      HB_PUT_LE_UINT32( ptr, pFunc->lPCodePos );      /* function size */
      ptr += 4;
      memcpy( ptr, pFunc->pCode, pFunc->lPCodePos );  /* function body */
      ptr += pFunc->lPCodePos;
      pFunc = pFunc->pNext;
   }
}

void hb_compGenPortObj( HB_COMP_DECL, PHB_FNAME pFileName )
{
   char szFileName[ _POSIX_PATH_MAX + 1 ];
   ULONG ulSize;
   BYTE * pHrbBody;
   FILE * yyc;

   if( ! pFileName->szExtension )
      pFileName->szExtension = ".hrb";
   hb_fsFNameMerge( szFileName, pFileName );

   yyc = fopen( szFileName, "wb" );
   if( ! yyc )
   {
      hb_compGenError( HB_COMP_PARAM, hb_comp_szErrors, 'E', HB_COMP_ERR_CREATE_OUTPUT, szFileName, NULL );
      return;
   }

   if( ! HB_COMP_PARAM->fQuiet )
   {
      printf( "Generating Harbour Portable Object output to \'%s\'... ", szFileName );
      fflush( stdout );
   }

   hb_compGenBufPortObj( HB_COMP_PARAM, &pHrbBody, &ulSize );
   fwrite( pHrbBody, ulSize, 1, yyc );
   hb_xfree( pHrbBody );

   fclose( yyc );

   if( ! HB_COMP_PARAM->fQuiet )
      printf( "Done.\n" );
}
