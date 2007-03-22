/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for the Macro compiler
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

#ifndef HB_MACRO_H_
#define HB_MACRO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>

#include "hbcompdf.h"
#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbvm.h"
#include "hbexprop.h"
#include "hbpcode.h"
#include "hbmacro.ch"

HB_EXTERN_BEGIN

/* flags for compilation process
 */
#define HB_MACRO_GEN_PUSH     1   /* generate PUSH pcodes */
#define HB_MACRO_GEN_POP      2   /* generate POP pcodes */
#define HB_MACRO_GEN_ALIASED  4   /* force aliased variable */
#define HB_MACRO_GEN_TYPE     8   /* check the type of expression (from TYPE() function) */
#define HB_MACRO_GEN_PARE     16  /* generate parentesized list */
#define HB_MACRO_GEN_LIST     32  /* generate push operation for every comma separated expressions */
#define HB_MACRO_DEALLOCATE   128 /* macro structure is allocated on the heap */

/* values returned from compilation process
 */
#define HB_MACRO_OK           0   /* macro compiled successfully */
#define HB_MACRO_FAILURE      1   /* syntax error */

/* additional status of compilation
 */
#define HB_MACRO_CONT         1   /* everything is OK so far */
#define HB_MACRO_TOO_COMPLEX  2   /* compiled expression is too complex */
#define HB_MACRO_UDF          4   /* code uses UDF function (info used by TYPE function) */
#define HB_MACRO_UNKN_SYM     8   /* requested symbol was not found in runtime symbol table */
#define HB_MACRO_UNKN_VAR     16  /* requested variable doesn't exist */

/* Global functions
 */
extern void hb_macroError( int iError, HB_COMP_DECL );
extern int hb_macroYYParse( HB_MACRO_PTR pMacro );
extern ULONG hb_macroSetMacro( BOOL bSet, ULONG ulFlag );
extern ULONG hb_macroAutoSetMacro( ULONG ulFlag );
extern BOOL hb_macroLexNew( HB_MACRO_PTR pMacro );
extern void hb_macroLexDelete( HB_MACRO_PTR pMacro );

extern HB_EXPR_PTR hb_macroExprGenPush( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_macroExprGenPop( HB_EXPR_PTR, HB_COMP_DECL );

extern HB_EXPR_PTR hb_macroExprNewArrayAt( HB_EXPR_PTR pArray, HB_EXPR_PTR pIndex, HB_COMP_DECL );
extern HB_EXPR_PTR hb_macroExprNewFunCall( HB_EXPR_PTR pName, HB_EXPR_PTR pParms, HB_COMP_DECL );
extern HB_EXPR_PTR hb_macroExprNewSend( HB_EXPR_PTR pObject, char * szMessage, HB_EXPR_PTR pMessage, HB_COMP_DECL );

/* Size of pcode buffer incrementation
 */
#define HB_PCODE_SIZE  512

/* Declarations for functions macro.c */
#if defined( HB_MACRO_SUPPORT )

extern void hb_macroGenPCode1( BYTE byte, HB_COMP_DECL );
extern void hb_macroGenPCode2( BYTE byte1, BYTE byte2, HB_COMP_DECL );
extern void hb_macroGenPCode3( BYTE byte1, BYTE byte2, BYTE byte3, HB_COMP_DECL );
extern void hb_macroGenPCode4( BYTE byte1, BYTE byte2, BYTE byte3, BYTE byte4, HB_COMP_DECL );
extern void hb_macroGenPCodeN( BYTE * pBuffer, ULONG ulSize, HB_COMP_DECL );

extern ULONG hb_macroGenJump( LONG lOffset, HB_COMP_DECL );
extern ULONG hb_macroGenJumpFalse( LONG lOffset, HB_COMP_DECL );
extern void hb_macroGenJumpThere( ULONG ulFrom, ULONG ulTo, HB_COMP_DECL );
extern void hb_macroGenJumpHere( ULONG ulOffset, HB_COMP_DECL );
extern ULONG hb_macroGenJumpTrue( LONG lOffset, HB_COMP_DECL );

extern void hb_macroGenPushSymbol( char * szSymbolName, BOOL bFunction, HB_COMP_DECL );
extern void hb_macroGenPushLong( HB_LONG lNumber, HB_COMP_DECL );
extern void hb_macroGenPushDate( HB_LONG lNumber, HB_COMP_DECL );
extern void hb_macroGenMessage( char * szMsgName, BOOL bIsObject, HB_COMP_DECL );
extern void hb_macroGenMessageData( char * szMsg, BOOL bIsObject, HB_COMP_DECL );
extern void hb_macroGenPopVar( char * szVarName, HB_COMP_DECL );
extern void hb_macroGenPopAliasedVar( char * szVarName,
                                     BOOL bPushAliasValue,
                                     char * szAlias,
                                     long lWorkarea, HB_COMP_DECL );
extern void hb_macroGenPushVar( char * szVarName, BOOL bMacroVar, HB_COMP_DECL );
extern void hb_macroGenPushVarRef( char * szVarName, HB_COMP_DECL );
extern void hb_macroGenPushMemvarRef( char * szVarName, HB_COMP_DECL );
extern void hb_macroGenPushAliasedVar( char * szVarName,
                                      BOOL bPushAliasValue,
                                      char * szAlias,
                                      long lWorkarea, HB_COMP_DECL );
extern void hb_macroGenPushLogical( int iTrueFalse, HB_COMP_DECL );
extern void hb_macroGenPushDouble( double dNumber, BYTE bWidth, BYTE bDec, HB_COMP_DECL );
extern void hb_macroGenPushFunCall( char * szFunName, HB_COMP_DECL );
extern void hb_macroGenPushFunSym( char * szFunName, HB_COMP_DECL );
extern void hb_macroGenPushFunRef( char * szFunName, HB_COMP_DECL );
extern void hb_macroGenPushString( char * szText, ULONG ulStrLen, HB_COMP_DECL );

extern void hb_macroCodeBlockStart( HB_COMP_DECL );
extern void hb_macroCodeBlockEnd( HB_COMP_DECL );

extern BOOL hb_macroIsValidMacroText( char *, ULONG );

#endif /* HB_MACRO_SUPPORT */

HB_EXTERN_END

#endif /* HB_MACRO_H_ */
