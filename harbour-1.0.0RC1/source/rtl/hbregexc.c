/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * 
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

/* #define HB_PCRE_REGEX */

#define _HB_REGEX_INTERNAL_
#include "hbregex.h"
#include "hbapiitm.h"
#include "hbapierr.h"

static void hb_regfree( PHB_REGEX pRegEx )
{
   HB_SYMBOL_UNUSED( pRegEx );
}

static int hb_regcomp( PHB_REGEX pRegEx, const char * szRegEx )
{
   HB_SYMBOL_UNUSED( pRegEx );
   HB_SYMBOL_UNUSED( szRegEx );
   return -1;
}

static int hb_regexec( PHB_REGEX pRegEx, const char * szString, ULONG ulLen,
                       int iMatches, HB_REGMATCH * aMatches )
{
   HB_SYMBOL_UNUSED( pRegEx );
   HB_SYMBOL_UNUSED( szString );
   HB_SYMBOL_UNUSED( ulLen );
   HB_SYMBOL_UNUSED( iMatches );
   HB_SYMBOL_UNUSED( aMatches );
   return -1;
}

static HB_REG_FREE s_reg_free = hb_regfree;
static HB_REG_COMP s_reg_comp = hb_regcomp;
static HB_REG_EXEC s_reg_exec = hb_regexec;

void hb_regexInit( HB_REG_FREE pFree, HB_REG_COMP pComp, HB_REG_EXEC pExec )
{
   s_reg_free = pFree;
   s_reg_comp = pComp;
   s_reg_exec = pExec;
}

/* This releases regex when called from the garbage collector */
HB_GARBAGE_FUNC( hb_regexRelease )
{
   ( s_reg_free )( ( PHB_REGEX ) Cargo );
}

PHB_REGEX hb_regexCompile( const char *szRegEx, ULONG ulLen, int iFlags )
{
   PHB_REGEX pRegEx;

   HB_SYMBOL_UNUSED( ulLen );

   pRegEx = ( PHB_REGEX ) hb_gcAlloc( sizeof( HB_REGEX ), hb_regexRelease );
   hb_gcLock( pRegEx );
   memset( pRegEx, 0, sizeof( HB_REGEX ) );
   pRegEx->fFree = TRUE;
   pRegEx->iFlags = iFlags;

   if( ( s_reg_comp )( pRegEx, szRegEx ) != 0 )
   {
      hb_gcFree( pRegEx );
      pRegEx = NULL;
   }

   return pRegEx;
}

PHB_REGEX hb_regexGet( PHB_ITEM pRegExItm, int iFlags )
{
   PHB_REGEX pRegEx = NULL;

   if( pRegExItm )
   {
      if( HB_IS_POINTER( pRegExItm ) )
      {
         pRegEx = ( PHB_REGEX ) hb_itemGetPtrGC( pRegExItm, hb_regexRelease );
      }
      else if( HB_IS_STRING( pRegExItm ) )
      {
         ULONG ulLen = hb_itemGetCLen( pRegExItm );
         char * szRegEx = hb_itemGetCPtr( pRegExItm );
         if( ulLen > 0 )
            pRegEx = hb_regexCompile( szRegEx, ulLen, iFlags );
      }
   }

   if( !pRegEx )
      hb_errRT_BASE_SubstR( EG_ARG, 3012, "Invalid Regular expression", &hb_errFuncName, 1, pRegExItm );

   return pRegEx;
}

void      hb_regexFree( PHB_REGEX pRegEx )
{
   if( pRegEx && pRegEx->fFree )
   {
      ( s_reg_free )( pRegEx );
      hb_gcFree( pRegEx );
   }
}

BOOL      hb_regexMatch( PHB_REGEX pRegEx, const char *szString, ULONG ulLen, BOOL fFull )
{
   HB_REGMATCH aMatches[ HB_REGMATCH_SIZE( 1 ) ];
   BOOL fMatch;

   fMatch = ( s_reg_exec )( pRegEx, szString, ulLen, 1, aMatches ) > 0;
   return fMatch && ( !fFull ||
            ( HB_REGMATCH_SO( aMatches, 0 ) == 0 &&
              HB_REGMATCH_EO( aMatches, 0 ) == ( int ) ulLen ) );
}
