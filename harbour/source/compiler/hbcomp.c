/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    allocate/free new compiler context
 *
 * Copyright 2006 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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


#include "hbcomp.h"

static HB_EXPR_PTR hb_compExprAlloc( HB_COMP_DECL )
{
   PHB_EXPRLST pExpItm = ( PHB_EXPRLST ) hb_xgrab( sizeof( HB_EXPRLST ) );

   pExpItm->pNext = HB_COMP_PARAM->pExprLst;
   HB_COMP_PARAM->pExprLst = pExpItm;
   if( pExpItm->pNext )
   {
      pExpItm->pPrev = pExpItm->pNext->pPrev;
      pExpItm->pNext->pPrev = pExpItm;
      pExpItm->pPrev->pNext = pExpItm;
   }
   else
      pExpItm->pPrev = pExpItm->pNext = pExpItm;

   return &pExpItm->Expression;
}

static void hb_compExprDealloc( HB_COMP_DECL, HB_EXPR_PTR pExpr )
{
   if( HB_COMP_PARAM->pExprLst )
   {
      PHB_EXPRLST pExpItm = ( PHB_EXPRLST ) pExpr;

      pExpItm->pNext->pPrev = pExpItm->pPrev;
      pExpItm->pPrev->pNext = pExpItm->pNext;
      if( pExpItm == HB_COMP_PARAM->pExprLst )
      {
         if( pExpItm->pNext == pExpItm )
            HB_COMP_PARAM->pExprLst = NULL;
         else
            HB_COMP_PARAM->pExprLst = pExpItm->pNext;
      }
      hb_xfree( pExpItm );
   }
}

static HB_EXPR_PTR hb_compExprNew( HB_COMP_DECL, HB_EXPRTYPE iType )
{
   HB_EXPR_PTR pExpr;

   HB_TRACE(HB_TR_DEBUG, ("hb_compExprNew(%p,%i)", HB_COMP_PARAM, iType));

   pExpr = hb_compExprAlloc( HB_COMP_PARAM );
   pExpr->ExprType = iType;
   pExpr->pNext    = NULL;
   pExpr->ValType  = HB_EV_UNKNOWN;
   pExpr->Counter  = 1;

   return pExpr;
}

/* Delete self - all components will be deleted somewhere else
 */
static void hb_compExprClear( HB_COMP_DECL, HB_EXPR_PTR pExpr )
{
   if( --pExpr->Counter == 0 )
      hb_compExprDealloc( HB_COMP_PARAM, pExpr );
}

/* Delete all components and delete self
 */
static void hb_compExprDelete( HB_COMP_DECL, HB_EXPR_PTR pExpr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_compExprDelete(%p,%p)", HB_COMP_PARAM, pExpr));
   if( pExpr && --pExpr->Counter == 0 )
   {
      HB_EXPR_USE( pExpr, HB_EA_DELETE );
      hb_compExprDealloc( HB_COMP_PARAM, pExpr );
   }
}

/* Delete all components and delete self
 */
static void hb_compExprFree( HB_COMP_DECL, HB_EXPR_PTR pExpr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_compExprFree()"));
   if( --pExpr->Counter == 0 )
   {
      HB_EXPR_USE( pExpr, HB_EA_DELETE );
      hb_compExprDealloc( HB_COMP_PARAM, pExpr );
   }
}

void hb_compExprLstDealloc( HB_COMP_DECL )
{
   if( HB_COMP_PARAM->pExprLst )
   {
      PHB_EXPRLST pExpItm, pExp;
      pExpItm = pExp = HB_COMP_PARAM->pExprLst;
      HB_COMP_PARAM->pExprLst = NULL;
      do
      {
         hb_compExprDelete( HB_COMP_PARAM, &pExp->Expression );
         pExp = pExp->pNext;
      }
      while( pExp != pExpItm );
      do
      {
         PHB_EXPRLST pFree = pExp;
         pExp = pExp->pNext;
         hb_xfree( pFree );
      }
      while( pExp != pExpItm );
   }
}

static HB_EXPR_PTR hb_compErrorType( HB_COMP_DECL, HB_EXPR_PTR pExpr )
{
   const char * szDesc = hb_compExprDescription( pExpr );
   hb_compGenError( HB_COMP_PARAM, hb_comp_szErrors, 'E', HB_COMP_ERR_INVALID_TYPE, szDesc, NULL );
   return pExpr;
}

static HB_EXPR_PTR hb_compErrorSyntax( HB_COMP_DECL, HB_EXPR_PTR pExpr )
{
   const char * szDesc = hb_compExprDescription( pExpr );
   hb_compGenError( HB_COMP_PARAM, hb_comp_szErrors, 'E', HB_COMP_ERR_SYNTAX, szDesc, NULL );
   return pExpr;
}

static void hb_compErrorDuplVar( HB_COMP_DECL, const char * szVarName )
{
   hb_compGenError( HB_COMP_PARAM, hb_comp_szErrors, 'E', HB_COMP_ERR_VAR_DUPL, szVarName, NULL );
}

static const HB_COMP_FUNCS s_comp_funcs =
{
   hb_compExprNew,
   hb_compExprClear,
   hb_compExprFree,
   hb_compExprDelete,

   hb_compErrorType,
   hb_compErrorSyntax,
   hb_compErrorDuplVar,
};

HB_COMP_PTR hb_comp_new( void )
{
   HB_COMP_PTR pComp = NULL;
   PHB_PP_STATE pPP = hb_pp_new();

   if( pPP )
   {
      pComp = ( HB_COMP_PTR ) hb_xgrab( sizeof( HB_COMP ) );
      memset( pComp, 0, sizeof( HB_COMP ) );
      pComp->pLex = ( PHB_COMP_LEX ) hb_xgrab( sizeof( HB_COMP_LEX ) );
      memset( pComp->pLex, 0, sizeof( HB_COMP_LEX ) );

      /* initialize default settings */
      pComp->mode = HB_MODE_COMPILER;
      pComp->funcs = &s_comp_funcs;

      pComp->pLex->pPP = pPP;

      /* various compatibility flags (-k switch)
         activate Harbour extensions by default. */
      pComp->supported = HB_COMPFLAG_HARBOUR   |
                         HB_COMPFLAG_XBASE     |
                         HB_COMPFLAG_HB_INLINE |
                         HB_COMPFLAG_OPTJUMP   |
                         HB_COMPFLAG_MACROTEXT |
                         HB_COMPFLAG_SHORTCUTS;

      pComp->fTextSubst       = ( pComp->supported & HB_COMPFLAG_MACROTEXT ) != 0;
      pComp->fLongOptimize    = TRUE;
      pComp->fPPO             = FALSE;    /* flag indicating, is ppo output needed */
      pComp->fStartProc       = TRUE;     /* holds if we need to create the starting procedure */
      pComp->fLineNumbers     = TRUE;     /* holds if we need pcodes with line numbers */
      pComp->fAnyWarning      = FALSE;    /* holds if there was any warning during the compilation process */
      pComp->fAutoMemvarAssume= FALSE;    /* holds if undeclared variables are automatically assumed MEMVAR (-a)*/
      pComp->fForceMemvars    = FALSE;    /* holds if memvars are assumed when accesing undeclared variable (-v)*/
      pComp->fDebugInfo       = FALSE;    /* holds if generate debugger required info */
      pComp->fNoStartUp       = FALSE;    /* C code generation embed HB_FS_FIRST or not */
      pComp->fCredits         = FALSE;    /* print credits */
      pComp->fBuildInfo       = FALSE;    /* print build info */
      pComp->fLogo            = TRUE;     /* print logo */
      pComp->fSyntaxCheckOnly = FALSE;    /* syntax check only */
      pComp->fAutoOpen        = TRUE;
      pComp->fError           = FALSE;

      pComp->iWarnings  = 0;                    /* enable parse warnings */
      pComp->iGenCOutput= HB_COMPGENC_VERBOSE;  /* C code generation should be verbose (use comments) or not */
      pComp->iExitLevel = HB_EXITLEVEL_DEFAULT; /* holds if there was any warning during the compilation process */
      pComp->iLanguage  = LANG_C;               /* default Harbour generated output language */
   }

   return pComp;
}

void hb_comp_free( HB_COMP_PTR pComp )
{
   hb_compCompileEnd( pComp );
   hb_compParserStop( pComp );

   /* free allocated expressions only when errors appear - in all
    * other cases expressions should be always cleanly freed so
    * executing hb_compExprLstDealloc() may only hides some real
    * memory leaks
    */
   if( pComp->iErrorCount != 0 )
      hb_compExprLstDealloc( pComp );

   hb_compIdentifierClose( pComp );

   if( pComp->pOutPath )
      hb_xfree( pComp->pOutPath );

   if( pComp->pPpoPath )
      hb_xfree( pComp->pPpoPath );

   while( pComp->autoopen )
   {
      PAUTOOPEN pAutoOpen = pComp->autoopen;

      pComp->autoopen = pComp->autoopen->pNext;
      hb_xfree( pAutoOpen );
   }

   if( pComp->pOutBuf )
      hb_xfree( pComp->pOutBuf );

   if( pComp->pLex )
   {
      if( pComp->pLex->pPP )
         hb_pp_free( pComp->pLex->pPP );
      hb_xfree( pComp->pLex );
   }
   if( pComp->szStdCh )
      hb_xfree( pComp->szStdCh );
   hb_xfree( pComp );
}

void hb_compOutStd( HB_COMP_DECL, const char * szMessage )
{
   if( ! HB_COMP_PARAM->fFullQuiet )
   {
      if( HB_COMP_PARAM->outStdFunc )
         HB_COMP_PARAM->outStdFunc( HB_COMP_PARAM->cargo, szMessage );
      else
      {
         fprintf( stderr, "%s", szMessage ); fflush( stdout );
      }
   }
}

void hb_compOutErr( HB_COMP_DECL, const char * szMessage )
{
   if( ! HB_COMP_PARAM->fFullQuiet )
   {
      if( HB_COMP_PARAM->outErrFunc )
         HB_COMP_PARAM->outErrFunc( HB_COMP_PARAM->cargo, szMessage );
      else
      {
#if defined( HOST_OS_UNIX_COMPATIBLE )
         fprintf( stderr, "%s", szMessage ); fflush( stderr );
#else
         fprintf( stdout, "%s", szMessage ); fflush( stdout );
#endif
      }
   }
}
