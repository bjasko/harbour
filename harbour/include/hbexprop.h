/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for the Harbour Compiler
 *
 * Copyright 1999 Ryszard Glab
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

#ifndef HB_EXPROP_H_
#define HB_EXPROP_H_

#include "hbapi.h"

HB_EXTERN_BEGIN

/* Definitions of function templates used in expression's message
 * handling
 */
#define  HB_EXPR_FUNC( proc )  HB_EXPR_PTR proc( HB_EXPR_PTR pSelf, HB_EXPR_MESSAGE iMessage, HB_COMP_DECL )
typedef  HB_EXPR_FUNC( HB_EXPR_FUNC_ );
typedef  HB_EXPR_FUNC_ *HB_EXPR_FUNC_PTR;

typedef  HB_EXPR_PTR HB_EXPR_ACTION( HB_EXPR_PTR pSelf, int iMessage, HB_COMP_DECL );
#define HB_EXPR_PCODE0( action ) action( HB_COMP_PARAM )
#define HB_EXPR_PCODE1( action, p1 ) action( (p1), HB_COMP_PARAM )
#define HB_EXPR_PCODE2( action, p1, p2 ) action( (p1), (p2), HB_COMP_PARAM )
#define HB_EXPR_PCODE3( action, p1, p2, p3 ) action( (p1), (p2), (p3), HB_COMP_PARAM )
#define HB_EXPR_PCODE4( action, p1, p2, p3, p4 ) action( (p1), (p2), (p3), (p4), HB_COMP_PARAM )

#if defined( HB_MACRO_SUPPORT )
#define hb_comp_ExprTable     hb_macro_ExprTable
#endif

#if !defined( HB_COMMON_SUPPORT )
extern const HB_EXPR_FUNC_PTR hb_comp_ExprTable[ HB_EXPR_COUNT ];
#define  HB_EXPR_USE( pSelf, iMessage )  \
         hb_comp_ExprTable[ (pSelf)->ExprType ]( (pSelf), (iMessage), HB_COMP_PARAM )
#endif

extern HB_EXPR_PTR hb_compExprNewEmpty( HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewNil( HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewDouble( double, BYTE, BYTE, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewLong( HB_LONG, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewDate( HB_LONG, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewString( char *, ULONG, BOOL, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewLogical( int, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewSelf( HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewCodeBlock( char *, ULONG, int, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewVar( char *, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewAliasVar( HB_EXPR_PTR, HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewAliasExpr( HB_EXPR_PTR, HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewMacro( HB_EXPR_PTR, unsigned char, char *, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewFunName( char *, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewRTVar( char *, HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewAlias( char *, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewEQ( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewNE( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewLT( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewLE( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewGT( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewGE( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewIN( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewPlus( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewMinus( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewMult( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewDiv( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewMod( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewPower( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewAssign( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewEqual( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewPlusEq( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewMinusEq( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewMultEq( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewDivEq( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewModEq( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewExpEq( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewPostInc( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewPostDec( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewPreInc( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewPreDec( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewAnd( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewOr( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewNot( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewNegate( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewVarRef( char *, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewFunRef( char *, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewFunCall( HB_EXPR_PTR, HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewRef( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewCodeblockExpr( HB_EXPR_PTR, HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compExprNewSend( HB_EXPR_PTR, char *szMessage, HB_EXPR_PTR pMessage, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewMethodCall( HB_EXPR_PTR, HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compExprNewList( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewArgList( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewArgRef( HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewArray( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewHash( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprNewArrayAt( HB_EXPR_PTR, HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprAddListExpr( HB_EXPR_PTR, HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compExprCBVarAdd( HB_EXPR_PTR, char *, BYTE, HB_COMP_DECL );
extern void hb_compExprCBVarDel( HB_CBVAR_PTR );
extern HB_EXPR_PTR hb_compExprAddCodeblockExpr( HB_EXPR_PTR, HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compExprSetCodeblockBody( HB_EXPR_PTR pExpr, BYTE * pCode, ULONG ulLen );
extern HB_EXPR_PTR hb_compExprNewIIF( HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compExprMacroAsAlias( HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compExprAssign( HB_EXPR_PTR, HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprEqual( HB_EXPR_PTR, HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compExprAssignStatic( HB_EXPR_PTR, HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprClone( HB_EXPR_PTR pSrc );
extern ULONG hb_compExprListLen( HB_EXPR_PTR );
extern ULONG hb_compExprParamListLen( HB_EXPR_PTR );
extern ULONG hb_compExprMacroListLen( HB_EXPR_PTR );
extern ULONG hb_compExprParamListCheck( HB_COMP_DECL, HB_EXPR_PTR );

extern const char * hb_compExprDescription( HB_EXPR_PTR );
extern int hb_compExprType( HB_EXPR_PTR );
extern int hb_compExprIsInteger( HB_EXPR_PTR );
extern int hb_compExprIsLong( HB_EXPR_PTR );
extern int hb_compExprAsInteger( HB_EXPR_PTR );
extern int hb_compExprAsNumSign( HB_EXPR_PTR );
extern int hb_compExprIsString( HB_EXPR_PTR );
extern int hb_compExprAsStringLen( HB_EXPR_PTR );
extern char * hb_compExprAsString( HB_EXPR_PTR );
extern char * hb_compExprAsSymbol( HB_EXPR_PTR );

extern HB_EXPR_PTR hb_compExprListStrip( HB_EXPR_PTR, HB_COMP_DECL );
extern BOOL hb_compExprIsValidMacro( char *, ULONG, BOOL *, HB_COMP_DECL );

extern HB_EXPR_PTR hb_compExprSetOperand( HB_EXPR_PTR, HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprSetGetBlock( HB_EXPR_PTR pExpr, HB_COMP_DECL );

extern void hb_compExprDelOperator( HB_EXPR_PTR, HB_COMP_DECL );

extern HB_EXPR_PTR hb_compExprReduceMod( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceDiv( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceMult( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceMinus( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReducePlus( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceNegate( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceIN( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceNE( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceGE( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceLE( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceGT( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceLT( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceEQ( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceAnd( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceOr( HB_EXPR_PTR pSelf, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduceIIF( HB_EXPR_PTR, HB_COMP_DECL );

extern BOOL hb_compExprReduceAT( HB_EXPR_PTR, HB_COMP_DECL );
extern BOOL hb_compExprReduceCHR( HB_EXPR_PTR, HB_COMP_DECL );
extern BOOL hb_compExprReduceLEN( HB_EXPR_PTR, HB_COMP_DECL );
extern BOOL hb_compExprReduceASC( HB_EXPR_PTR, HB_COMP_DECL );
extern BOOL hb_compExprReduceSTOD( HB_EXPR_PTR pSelf, USHORT usCount, HB_COMP_DECL );
extern BOOL hb_compExprReduceCTOD( HB_EXPR_PTR, HB_COMP_DECL );
extern BOOL hb_compExprReduceUPPER( HB_EXPR_PTR, HB_COMP_DECL );

extern void hb_compI18nAdd( HB_COMP_DECL, const char* szText, const char* szContext, UINT uiLine );

HB_EXTERN_END

#endif  /* HB_EXPROP_H_ */
