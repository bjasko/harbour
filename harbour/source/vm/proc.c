/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * PROCNAME(), PROCLINE() and PROCFILE() functions
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
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    PROCFILE()
 *
 * Copyright 2001 JFL (Mafact) <jfl@mafact.com>
 *    Adding the MethodName() just calling Procname()
 *    Special treatment in case of Object and Eval (only for methodname)
 *    skipping block and adding (b) before the method name
 *
 * See doc/license.txt for licensing terms.
 *
 */

#include "hbvmopt.h"
#include "hbapi.h"
#include "hbapicls.h"
#include "hbapiitm.h"
#include "hbstack.h"
#include "hbvm.h"

#ifdef HB_EXTENSION

HB_FUNC( HB_METHODNAME )
{
   char szName[ HB_SYMBOL_NAME_LEN + HB_SYMBOL_NAME_LEN + 5 ];

   hb_retc( hb_procname( hb_parni( 1 ) + 1, szName, TRUE ) );
}

#endif

HB_FUNC( PROCNAME )
{
   char szName[ HB_SYMBOL_NAME_LEN + HB_SYMBOL_NAME_LEN + 5 ];

   hb_retc( hb_procname( hb_parni( 1 ) + 1, szName, FALSE ) );
}

HB_FUNC( PROCLINE )
{
   long lOffset = hb_stackBaseProcOffset( hb_parni( 1 ) + 1 );

   if( lOffset > 0 )
      hb_retni( hb_stackItem( lOffset )->item.asSymbol.stackstate->uiLineNo );
   else
      hb_retni( 0 );
}

#ifdef HB_C52_UNDOC

/* NOTE: Clipper undocumented function, which always returns an empty
         string. [vszakats] */

HB_FUNC( PROCFILE )
{
   PHB_SYMB pSym = NULL;

   if( ISSYMBOL( 1 ) )
   {
      pSym = hb_itemGetSymbol( hb_param( 1, HB_IT_SYMBOL ) );
   }
   else if( ISCHAR( 1 ) )
   {
      PHB_DYNS pDynSym = hb_dynsymFindName( hb_parc( 1 ) );

      if( pDynSym )
         pSym = pDynSym->pSymbol;
   }
   else
   {
      long lOffset = hb_stackBaseProcOffset( hb_parni( 1 ) + 1 );

      if( lOffset > 0 )
      {
         pSym = hb_stackItem( lOffset )->item.asSymbol.value;

         if( pSym == &hb_symEval || pSym->pDynSym == hb_symEval.pDynSym )
         {
            PHB_ITEM pSelf = hb_stackItem( lOffset + 1 );

            if( HB_IS_BLOCK( pSelf ) )
               pSym = pSelf->item.asBlock.value->pDefSymb;
         }
      }
   }

   hb_retc( hb_vmFindModuleSymbolName( hb_vmGetRealFuncSym( pSym ) ) );
}

#endif

/* NOTE: szName size must be an at least:
         HB_SYMBOL_NAME_LEN + HB_SYMBOL_NAME_LEN + 5 [vszakats] */
#define HB_PROCBUF_LEN  ( HB_SYMBOL_NAME_LEN + HB_SYMBOL_NAME_LEN + 4 )
char * hb_procname( int iLevel, char * szName, BOOL fMethodName )
{
   long lOffset = hb_stackBaseProcOffset( iLevel );

   szName[ 0 ] = '\0';
   if( lOffset > 0 )
   {
      PHB_ITEM pBase, pSelf;

      pBase = hb_stackItem( lOffset );
      pSelf = hb_stackItem( lOffset + 1 );

      if( fMethodName && lOffset > 0 &&
          pBase->item.asSymbol.value == &hb_symEval &&
          pBase->item.asSymbol.stackstate->uiClass )
      {
         long lPrevOffset = hb_stackItem( lOffset )->item.asSymbol.stackstate->lBaseItem;

         if( hb_stackItem( lPrevOffset )->item.asSymbol.stackstate->uiClass ==
             pBase->item.asSymbol.stackstate->uiClass &&
             hb_stackItem( lPrevOffset )->item.asSymbol.stackstate->uiMethod ==
             pBase->item.asSymbol.stackstate->uiMethod )
         {
            pBase = hb_stackItem( lPrevOffset );
            pSelf = hb_stackItem( lPrevOffset + 1 );
         }
      }

      if( pBase->item.asSymbol.value == &hb_symEval ||
          pBase->item.asSymbol.value->pDynSym == hb_symEval.pDynSym )
      {
         hb_strncat( szName, "(b)", HB_PROCBUF_LEN );

         if( HB_IS_BLOCK( pSelf ) )
            hb_strncat( szName, pSelf->item.asBlock.value->pDefSymb->szName,
                        HB_PROCBUF_LEN );
         else
            hb_strncat( szName, pBase->item.asSymbol.value->szName, HB_PROCBUF_LEN );
      }
      else
      {
         /* it is a method name? */
         if( pBase->item.asSymbol.stackstate->uiClass )
         {
            hb_strncat( szName, hb_clsName( pBase->item.asSymbol.stackstate->uiClass ),
                        HB_PROCBUF_LEN );
            hb_strncat( szName, ":", HB_PROCBUF_LEN );
         }
         hb_strncat( szName, pBase->item.asSymbol.value->szName, HB_PROCBUF_LEN );
      }
   }

   return szName;
}

/* NOTE: szName size must be an at least:
 *          HB_SYMBOL_NAME_LEN + HB_SYMBOL_NAME_LEN + 5
 *       szFile szie must be an at least:
 *          _POSIX_PATH_MAX + 1
 */
BOOL hb_procinfo( int iLevel, char * szName, USHORT * puiLine, char * szFile )
{
   long lOffset = hb_stackBaseProcOffset( iLevel );

   if( lOffset > 0 )
   {
      PHB_ITEM pBase, pSelf;
      PHB_SYMB pSym;

      pBase = hb_stackItem( lOffset );
      pSelf = hb_stackItem( lOffset + 1 );

      pSym = pBase->item.asSymbol.value;

      if( szName )
      {
         szName[ 0 ] = '\0';
         if( pSym == &hb_symEval || pSym->pDynSym == hb_symEval.pDynSym )
         {
            hb_strncat( szName, "(b)", HB_PROCBUF_LEN );

            if( HB_IS_BLOCK( pSelf ) )
               hb_strncat( szName, pSelf->item.asBlock.value->pDefSymb->szName,
                           HB_PROCBUF_LEN );
            else
               hb_strncat( szName, pSym->szName, HB_PROCBUF_LEN );
         }
         else
         {
            if( pBase->item.asSymbol.stackstate->uiClass ) /* it is a method name */
            {
               hb_strncat( szName, hb_clsName( pBase->item.asSymbol.stackstate->uiClass ),
                           HB_PROCBUF_LEN );
               hb_strncat( szName, ":", HB_PROCBUF_LEN );
            }
            hb_strncat( szName, pSym->szName, HB_PROCBUF_LEN );
         }
      }

      if( puiLine )
         * puiLine = pBase->item.asSymbol.stackstate->uiLineNo;

      if( szFile )
      {
         char * szModule;

         if( HB_IS_BLOCK( pSelf ) &&
             ( pSym == &hb_symEval || pSym->pDynSym == hb_symEval.pDynSym ) )
            pSym = pSelf->item.asBlock.value->pDefSymb;

         szModule = hb_vmFindModuleSymbolName( hb_vmGetRealFuncSym( pSym ) );

         if( szModule )
            hb_strncpy( szFile, szModule, _POSIX_PATH_MAX );
         else
            szFile[ 0 ] = '\0';
      }

      return TRUE;
   }

   if( szName )
      szName[ 0 ] = '\0';
   if( puiLine )
      * puiLine = 0;
   if( szFile )
      szFile[ 0 ] = '\0';

   return FALSE;
}
