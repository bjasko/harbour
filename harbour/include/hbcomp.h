/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for the Harbour Compiler
 *
 * Copyright 1999 Antonio Linares <alinares@fivetechsoft.com>
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

#ifndef HB_COMP_H_
#define HB_COMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#if defined(__WATCOMC__)
#include <malloc.h>     /* alloca prototype */
#endif

#include "hbmacro.ch"
#include "hbapi.h"
#include "hberrors.h"
#include "hbpp.h"
#include "hbver.h"
#include "hbmacro.h"
#include "hbexprop.h"
#include "hbpcode.h"
#include "hbhash.h"

HB_EXTERN_BEGIN

/* definitions for hb_compPCodeEval() support */
typedef void * HB_VOID_PTR;
#define HB_PCODE_FUNC( func, type ) ULONG func( PFUNCTION pFunc, ULONG lPCodePos, type cargo )
typedef  HB_PCODE_FUNC( HB_PCODE_FUNC_, HB_VOID_PTR );
typedef  HB_PCODE_FUNC_ * HB_PCODE_FUNC_PTR;

extern LONG hb_compPCodeSize( PFUNCTION, ULONG );
extern void hb_compPCodeEval( PFUNCTION, HB_PCODE_FUNC_PTR *, void * );
extern void hb_compPCodeTrace( PFUNCTION, HB_PCODE_FUNC_PTR *, void * );

extern void hb_compGenLabelTable( PFUNCTION pFunc, PHB_LABEL_INFO label_info );
extern PHB_DEBUGINFO hb_compGetDebugInfo( HB_COMP_DECL );

extern void hb_compInitPP( HB_COMP_DECL, int argc, char * argv[] );

extern int  hb_compparse( HB_COMP_DECL );
extern void hb_compParserStop( HB_COMP_DECL );

#define VS_NONE       0
#define VS_LOCAL      1
#define VS_STATIC     2
#define VS_FIELD      4
#define VS_PARAMETER  8
#define VS_PRIVATE    64
#define VS_PUBLIC     128
#define VS_MEMVAR     ( VS_PUBLIC | VS_PRIVATE )

/* return detailed information about a class of variable  */
extern int hb_compVariableScope( HB_COMP_DECL, char * );
#define HB_VS_UNDECLARED	0
/* variables declared in a current codeblock/function/procedure */
#define HB_VS_CBLOCAL_VAR	1	/* local parameter of a codeblock */
#define HB_VS_LOCAL_VAR		2
#define HB_VS_LOCAL_MEMVAR	4
#define HB_VS_LOCAL_FIELD	8
#define HB_VS_STATIC_VAR	16
/* variables declared outside of a current function/procedure */
#define HB_VS_GLOBAL_MEMVAR	(32 | HB_VS_LOCAL_MEMVAR)
#define HB_VS_GLOBAL_FIELD	(32 | HB_VS_LOCAL_FIELD)
#define HB_VS_GLOBAL_STATIC	(32 | HB_VS_STATIC_VAR)

#define VU_NOT_USED    0
#define VU_INITIALIZED 1
#define VU_USED        2

#define VT_OFFSET_BYREF             60
#define VT_OFFSET_VARIANT           90
#define VT_OFFSET_OPTIONAL          90

/*
 * flags for bFlags member
 */
#define FUN_STATEMENTS        0x01  /* Function have at least one executable statement */
#define FUN_USES_STATICS      0x02  /* Function uses static variables */
#define FUN_PROCEDURE         0x04  /* This is a procedure that shouldn't return value */
#define FUN_BREAK_CODE        0x08  /* last statement breaks execution flow */
#define FUN_USES_LOCAL_PARAMS 0x10  /* parameters are declared using () */
#define FUN_WITH_RETURN       0x20  /* there was RETURN statement in previous line */

extern void      hb_compFunctionAdd( HB_COMP_DECL, char * szFunName, HB_SYMBOLSCOPE cScope, int iType ); /* starts a new Clipper language function definition */
extern PFUNCTION hb_compFunctionFind( HB_COMP_DECL, char * szFunName ); /* locates a previously defined function */
extern PINLINE   hb_compInlineFind( HB_COMP_DECL, char * szFunName );
extern USHORT    hb_compFunctionGetPos( HB_COMP_DECL, char * szSymbolName ); /* returns the index + 1 of a function on the functions defined list */
extern PFUNCTION hb_compFunctionKill( HB_COMP_DECL, PFUNCTION );    /* releases all memory allocated by function and returns the next one */
extern void      hb_compAnnounce( HB_COMP_DECL, char * );
extern PINLINE   hb_compInlineAdd( HB_COMP_DECL, char * szFunName, int iLine );

extern PFUNCALL  hb_compFunCallFind( HB_COMP_DECL, char * szFunName ); /* locates a previously defined called function */
extern BOOL      hb_compFunCallCheck( HB_COMP_DECL, char *, int );

extern void hb_compVariableAdd( HB_COMP_DECL, char * szVarName, BYTE cType ); /* add a new param, local, static variable to a function definition or a public or private */
extern PVAR hb_compVariableFind( PVAR pVars, USHORT wOrder ); /* returns a variable if defined or zero */
extern PVAR hb_compLocalVariableFind( PFUNCTION pFunc, USHORT wVar );
extern USHORT hb_compVariableGetPos( PVAR pVars, char * szVarName ); /* returns the order + 1 of a variable if defined or zero */
extern int hb_compLocalGetPos( HB_COMP_DECL, char * szVarName );   /* returns the order + 1 of a local variable */
extern char * hb_compStaticGetName( HB_COMP_DECL, USHORT wVar );   /* returns the name of static variable */

#define HB_SYM_MEMVAR   FALSE
#define HB_SYM_ALIAS    FALSE
#define HB_SYM_MSGNAME  FALSE
#define HB_SYM_FUNCNAME TRUE
extern PCOMSYMBOL hb_compSymbolAdd( HB_COMP_DECL, char *, USHORT *, BOOL );
extern PCOMSYMBOL hb_compSymbolKill( PCOMSYMBOL );    /* releases all memory allocated by symbol and returns the next one */
extern PCOMSYMBOL hb_compSymbolFind( HB_COMP_DECL, char *, USHORT *, BOOL ); /* returns a symbol pointer from the symbol table */
extern PCOMSYMBOL hb_compSymbolGetPos( HB_COMP_DECL, USHORT );   /* returns a symbol based on its index on the symbol table */

extern PCOMDECLARED hb_compDeclaredAdd( HB_COMP_DECL, char * );
extern PCOMDECLARED hb_compDeclaredFind( HB_COMP_DECL, char * );

extern PCOMCLASS hb_compClassAdd( HB_COMP_DECL, char * );
extern PCOMCLASS hb_compClassFind( HB_COMP_DECL, char * );
extern PCOMDECLARED hb_compMethodAdd( HB_COMP_DECL, PCOMCLASS pClass, char * );
extern PCOMDECLARED hb_compMethodFind( PCOMCLASS pClass, char * );
extern void hb_compDeclaredParameterAdd( HB_COMP_DECL, char * szVarName, BYTE cValueType );

extern void hb_compGenBreak( HB_COMP_DECL );  /* generate code for BREAK statement */

extern void hb_compExternGen( HB_COMP_DECL ); /* generates the symbols for the EXTERN names */
extern void hb_compExternAdd( HB_COMP_DECL, char * szExternName, HB_SYMBOLSCOPE cScope ); /* defines a new extern name */
extern void hb_compAutoOpenAdd( HB_COMP_DECL, char * szName );

extern void hb_compSwitchKill( HB_COMP_DECL );
extern void hb_compLoopKill( HB_COMP_DECL );
extern void hb_compRTVariableKill( HB_COMP_DECL );
extern void hb_compElseIfKill( HB_COMP_DECL );

extern void hb_compGenError( HB_COMP_DECL, const char * szErrors[], char cPrefix, int iError, const char * szError1, const char * szError2 ); /* generic parsing error management function */
extern void hb_compGenWarning( HB_COMP_DECL, const char * szWarnings[], char cPrefix, int iWarning, const char * szWarning1, const char * szWarning2); /* generic parsing warning management function */

extern BOOL hb_compForEachVarError( HB_COMP_DECL, char * );       /* checks if it is FOR EACH enumerator variable and generates a warning */

extern ULONG hb_compGenJump( LONG, HB_COMP_DECL );                /* generates the pcode to jump to a specific offset */
extern ULONG hb_compGenJumpFalse( LONG, HB_COMP_DECL );           /* generates the pcode to jump if false */
extern ULONG hb_compGenJumpTrue( LONG, HB_COMP_DECL );            /* generates the pcode to jump if true */
extern void  hb_compGenJumpHere( ULONG, HB_COMP_DECL );           /* returns the pcode pos where to set a jump offset */
extern void  hb_compGenJumpThere( ULONG, ULONG, HB_COMP_DECL );   /* sets a jump offset */

extern void hb_compGenModuleName( HB_COMP_DECL, char * szFunName );  /* generates the pcode with the currently compiled module and function name */
extern void hb_compLinePush( HB_COMP_DECL );             /* generates the pcode with the currently compiled source code line */
extern void hb_compLinePushIfDebugger( HB_COMP_DECL );   /* generates the pcode with the currently compiled source code line */
extern void hb_compLinePushIfInside( HB_COMP_DECL );     /* generates the pcode with the currently compiled source code line */
extern void hb_compStatmentStart( HB_COMP_DECL );        /* Check if we can start statement (without line pushing) */

extern void hb_compGenMessage( char * szMsgName, BOOL bIsObject, HB_COMP_DECL );       /* sends a message to an object */
extern void hb_compGenMessageData( char * szMsg, BOOL bIsObject, HB_COMP_DECL );     /* generates an underscore-symbol name for a data assignment */
extern void hb_compGenPopVar( char * szVarName, HB_COMP_DECL );         /* generates the pcode to pop a value from the virtual machine stack onto a variable */
extern void hb_compGenPushDouble( double dNumber, BYTE bWidth, BYTE bDec, HB_COMP_DECL ); /* Pushes a number on the virtual machine stack */
extern void hb_compGenPushFunCall( char *, HB_COMP_DECL );             /* generates the pcode to push function's call */
extern void hb_compGenPushFunSym( char *, HB_COMP_DECL );              /* generates the pcode to push function's symbol */
extern void hb_compGenPushFunRef( char *, HB_COMP_DECL );              /* generates the pcode to push function's reference symbol */
extern void hb_compGenPushVar( char * szVarName, BOOL bMacroVar, HB_COMP_DECL );       /* generates the pcode to push a variable value to the virtual machine stack */
extern void hb_compGenPushVarRef( char * szVarName, HB_COMP_DECL );    /* generates the pcode to push a variable by reference to the virtual machine stack */
extern void hb_compGenPushMemvarRef( char * szVarName, HB_COMP_DECL ); /* generates the pcode to push memvar variable by reference to the virtual machine stack */
extern void hb_compGenPushInteger( int iNumber, HB_COMP_DECL );        /* Pushes a integer number on the virtual machine stack */
extern void hb_compGenPushLogical( int iTrueFalse, HB_COMP_DECL );     /* pushes a logical value on the virtual machine stack */
extern void hb_compGenPushLong( HB_LONG lNumber, HB_COMP_DECL );       /* Pushes a long number on the virtual machine stack */
extern void hb_compGenPushDate( HB_LONG lNumber, HB_COMP_DECL );       /* Pushes a date constant on the virtual machine stack */
extern void hb_compGenPushNil( HB_COMP_DECL );                   /* Pushes nil on the virtual machine stack */
extern void hb_compGenPushString( char * szText, ULONG ulLen, HB_COMP_DECL );       /* Pushes a string on the virtual machine stack */
extern void hb_compGenPushSymbol( char * szSymbolName, BOOL bFunction, HB_COMP_DECL ); /* Pushes a symbol on to the Virtual machine stack */
extern void hb_compGenPushAliasedVar( char *, BOOL, char *, long, HB_COMP_DECL );
extern void hb_compGenPopAliasedVar( char *, BOOL, char *, long, HB_COMP_DECL );
extern void hb_compGenPushFunRef( char *, HB_COMP_DECL );
extern void hb_compGenPCode1( BYTE, HB_COMP_DECL ); /* generates 1 byte of pcode */
extern void hb_compGenPCode2( BYTE, BYTE, HB_COMP_DECL ); /* generates 2 bytes of pcode + flag for optional StrongType(). */
extern void hb_compGenPCode3( BYTE, BYTE, BYTE, HB_COMP_DECL ); /* generates 3 bytes of pcode + flag for optional StrongType() */
extern void hb_compGenPCode4( BYTE, BYTE, BYTE, BYTE, HB_COMP_DECL ); /* generates 4 bytes of pcode + flag for optional StrongType() */
extern void hb_compGenPCodeN( BYTE * pBuffer, ULONG ulSize, HB_COMP_DECL ); /* copy bytes to a pcode buffer + flag for optional StrongType() */

extern ULONG hb_compSequenceBegin( HB_COMP_DECL );
extern ULONG hb_compSequenceEnd( HB_COMP_DECL );
extern ULONG hb_compSequenceAlways( HB_COMP_DECL );
extern void hb_compSequenceFinish( HB_COMP_DECL, ULONG, ULONG, ULONG, BOOL, BOOL );

/* support for FIELD declaration */
extern void hb_compFieldSetAlias( HB_COMP_DECL, char *, int );
extern int  hb_compFieldsCount( HB_COMP_DECL );

/* Static variables */
extern void hb_compStaticDefStart( HB_COMP_DECL );
extern void hb_compStaticDefEnd( HB_COMP_DECL );
extern void hb_compGenStaticName( char *, HB_COMP_DECL );

extern BOOL hb_compCheckUnclosedStru( HB_COMP_DECL );

#define HB_COMP_ERROR_TYPE( x )     HB_COMP_PARAM->funcs->ErrorType( HB_COMP_PARAM, x )
#define HB_COMP_ERROR_SYNTAX( x )   HB_COMP_PARAM->funcs->ErrorSyntax( HB_COMP_PARAM, x )
#define HB_COMP_ERROR_DUPLVAR( s )  HB_COMP_PARAM->funcs->ErrorDuplVar( HB_COMP_PARAM, s )

#define HB_COMP_EXPR_NEW( i )       HB_COMP_PARAM->funcs->ExprNew( HB_COMP_PARAM, i )
#define HB_COMP_EXPR_FREE( x )      HB_COMP_PARAM->funcs->ExprFree( HB_COMP_PARAM, x )
#define HB_COMP_EXPR_CLEAR( x )     HB_COMP_PARAM->funcs->ExprClear( HB_COMP_PARAM, x )
#define HB_COMP_EXPR_DELETE( x )    HB_COMP_PARAM->funcs->ExprDelete( HB_COMP_PARAM, x )

#if defined( HB_MACRO_SUPPORT )

#define HB_GEN_FUNC1( func, p1 )          hb_macroGen##func( p1, HB_COMP_PARAM )
#define HB_GEN_FUNC2( func, p1,p2 )       hb_macroGen##func( p1, p2, HB_COMP_PARAM )
#define HB_GEN_FUNC3( func, p1,p2,p3 )    hb_macroGen##func( p1, p2, p3, HB_COMP_PARAM )
#define HB_GEN_FUNC4( func, p1,p2,p3,p4 ) hb_macroGen##func( p1, p2, p3, p4, HB_COMP_PARAM )

#define hb_compErrorIndex( p, x )         hb_macroError( EG_BOUND, ( p ) )
#define hb_compErrorLValue( p, x )        hb_macroError( EG_SYNTAX, ( p ) )
#define hb_compErrorBound( p, x )         hb_macroError( EG_BOUND, ( p ) )
#define hb_compErrorAlias( p, x )         hb_macroError( EG_NOALIAS, ( p ) )
#define hb_compErrorRefer( p, x, c )      hb_macroError( EG_SYNTAX, ( p ) )
#define hb_compErrorVParams( p, x )       hb_macroError( EG_SYNTAX, ( p ) )
#define hb_compWarnMeaningless( p, x )
#define hb_compErrorMacro( p, x )

#elif !defined( HB_COMMON_SUPPORT )

#define HB_GEN_FUNC1( func, p1 )          hb_compGen##func( p1, HB_COMP_PARAM )
#define HB_GEN_FUNC2( func, p1,p2 )       hb_compGen##func( p1, p2, HB_COMP_PARAM )
#define HB_GEN_FUNC3( func, p1,p2,p3 )    hb_compGen##func( p1, p2, p3, HB_COMP_PARAM )
#define HB_GEN_FUNC4( func, p1,p2,p3,p4 ) hb_compGen##func( p1, p2, p3, p4, HB_COMP_PARAM )

extern int  hb_compMain( int argc, char * argv[], BYTE ** pBufPtr, ULONG * pulSize );
extern void hb_compExprLstDealloc( HB_COMP_DECL );

extern HB_EXPR_PTR hb_compExprGenStatement( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprGenPush( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprGenPop( HB_EXPR_PTR, HB_COMP_DECL );
extern HB_EXPR_PTR hb_compExprReduce( HB_EXPR_PTR, HB_COMP_DECL );

extern HB_EXPR_PTR hb_compErrorIndex( HB_COMP_DECL, HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compErrorLValue( HB_COMP_DECL, HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compErrorBound( HB_COMP_DECL, HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compErrorAlias( HB_COMP_DECL, HB_EXPR_PTR );
extern HB_EXPR_PTR hb_compErrorRefer( HB_COMP_DECL, HB_EXPR_PTR, const char * );
extern HB_EXPR_PTR hb_compWarnMeaningless( HB_COMP_DECL, HB_EXPR_PTR );
extern void        hb_compErrorMacro( HB_COMP_DECL, const char * );
extern void        hb_compErrorVParams( HB_COMP_DECL, const char * );

extern HB_EXPR_PTR hb_compErrorStatic( HB_COMP_DECL, const char *, HB_EXPR_PTR );
extern void        hb_compErrorCodeblock( HB_COMP_DECL, const char * );

extern BOOL        hb_compIsValidMacroText( HB_COMP_DECL, char *, ULONG );

/* Codeblocks */
extern void hb_compCodeBlockStart( HB_COMP_DECL, BOOL ); /* starts a codeblock creation */
extern void hb_compCodeBlockEnd( HB_COMP_DECL );         /* end of codeblock creation */
extern void hb_compCodeBlockStop( HB_COMP_DECL );        /* end of fake codeblock */
extern void hb_compCodeBlockRewind( HB_COMP_DECL );      /* restart of fake codeblock */

#endif    /* HB_MACRO_SUPPORT */


extern ULONG hb_compExprListEval( HB_COMP_DECL, HB_EXPR_PTR pExpr, HB_CARGO_FUNC_PTR pEval );
extern ULONG hb_compExprListEval2( HB_COMP_DECL, HB_EXPR_PTR pExpr1, HB_EXPR_PTR pExpr2, HB_CARGO2_FUNC_PTR pEval );

extern void hb_compChkCompilerSwitch( HB_COMP_DECL, int, char * Args[] );
extern void hb_compChkPaths( HB_COMP_DECL );
extern void hb_compChkDefines( HB_COMP_DECL, int iArg, char * Args[] );

extern void hb_compPrintUsage( char * );
extern void hb_compPrintCredits( void );
extern void hb_compFileInfo( void );
extern void hb_compPrintLogo( void );
extern void hb_compPrintModes( void );


/* Misc functions defined in harbour.c */
extern void hb_compFinalizeFunction( HB_COMP_DECL ); /* fixes all last defined function returns jumps offsets */
extern void hb_compNOOPfill( PFUNCTION pFunc, ULONG ulFrom, int iCount, BOOL fPop, BOOL fCheck );
extern BOOL hb_compIsJump( HB_COMP_DECL, PFUNCTION pFunc, ULONG ulPos );

/* Misc functions defined in hbfix.c */
extern void hb_compFixFuncPCode( HB_COMP_DECL, PFUNCTION pFunc );
/* Misc functions defined in hbdead.c */
extern void hb_compCodeTraceMarkDead( HB_COMP_DECL, PFUNCTION pFunc );
/* Misc functions defined in hbopt.c */
extern void hb_compOptimizePCode( HB_COMP_DECL, PFUNCTION pFunc );
/* Misc functions defined in hbstripl.c */
extern void hb_compStripFuncLines( PFUNCTION pFunc );

/* output related functions defined in gen*.c */
extern void hb_compGenCCode( HB_COMP_DECL, PHB_FNAME );      /* generates the C language output */
extern void hb_compGenPortObj( HB_COMP_DECL, PHB_FNAME );    /* generates the portable objects */
extern void hb_compGenILCode( HB_COMP_DECL, PHB_FNAME );     /* generates the .NET IL language output */
extern void hb_compGenJava( HB_COMP_DECL, PHB_FNAME );       /* generates the Java language output */
extern void hb_compGenObj32( HB_COMP_DECL, PHB_FNAME );      /* generates OBJ 32 bits */
extern void hb_compGenCObj( HB_COMP_DECL, PHB_FNAME );       /* generates platform dependant object module */

extern void hb_compGenBufPortObj( HB_COMP_DECL, BYTE ** pBufPtr, ULONG * pulSize ); /* generates the portable objects to memory buffer */

extern void hb_compGenCRealCode( HB_COMP_DECL, PFUNCTION pFunc, FILE * yyc );
extern void hb_compGenCString( FILE * yyc, BYTE * pText, ULONG ulLen );

/* hbident.c   */
extern char * hb_compIdentifierNew( HB_COMP_DECL, char * szName, int iType ); /* create the reusable identifier */
extern void hb_compIdentifierOpen( HB_COMP_DECL ); /* prepare the table of identifiers */
extern void hb_compIdentifierClose( HB_COMP_DECL ); /* release the table of identifiers */

/* global readonly variables used by compiler
 */

extern const char *  hb_comp_szErrors[];
extern const char *  hb_comp_szWarnings[];

/* table with PCODEs' length */
extern const BYTE    hb_comp_pcode_len[];

/* file handle for error messages */
extern FILE          * hb_comp_errFile;

/* identifier types for hb_compIdentifierNew() */
#define HB_IDENT_STATIC       0
#define HB_IDENT_FREE         1
#define HB_IDENT_COPY         2

/* /GC command line setting types */
#define HB_COMPGENC_COMPACT     0
#define HB_COMPGENC_NORMAL      1
#define HB_COMPGENC_VERBOSE     2
#define HB_COMPGENC_REALCODE    3

/* /ES command line setting types */
#define HB_EXITLEVEL_DEFAULT    0
#define HB_EXITLEVEL_SETEXIT    1
#define HB_EXITLEVEL_DELTARGET  2

/* /kx command line setting types - compatibility modes
 * (turn on a bit in ULONG word)
*/
#define HB_COMPFLAG_HARBOUR      HB_SM_HARBOUR     /* 1 -kh */
#define HB_COMPFLAG_XBASE        HB_SM_XBASE       /* 2 -kx */
#define HB_COMPFLAG_SHORTCUTS    HB_SM_SHORTCUTS   /* 8 -z enable sortcuts for logical operators */
#define HB_COMPFLAG_ARRSTR       HB_SM_ARRSTR      /* 16 -ks strings as array of bytes */
#define HB_COMPFLAG_RT_MACRO     HB_SM_RT_MACRO    /* 64 -kr */
#define HB_COMPFLAG_OPTJUMP      256               /* -kj turn off jump optimalization */
#define HB_COMPFLAG_HB_INLINE    512               /* -ki hb_inLine(...) { ... } support */
#define HB_COMPFLAG_MACROTEXT    1024              /* -kM turn off macrotext substitution */

#define HB_COMP_ISSUPPORTED(flag)   ( HB_COMP_PARAM->supported & (flag) )

#define HB_SUPPORT_XBASE            ( HB_COMP_ISSUPPORTED(HB_COMPFLAG_XBASE) )
#define HB_SUPPORT_HARBOUR          ( HB_COMP_ISSUPPORTED(HB_COMPFLAG_HARBOUR) )
#define HB_SUPPORT_ARRSTR           ( HB_COMP_ISSUPPORTED(HB_COMPFLAG_ARRSTR) )

#if defined( HB_MACRO_SUPPORT )
#  define HB_MACRO_GENFLAGS   HB_COMPFLAG_RT_MACRO
#elif ! defined( HB_COMMON_SUPPORT )
#  define HB_MACRO_GENFLAGS   ( HB_COMP_PARAM->supported & \
                                ( HB_COMPFLAG_HARBOUR | \
                                  HB_COMPFLAG_XBASE | \
                                  HB_COMPFLAG_SHORTCUTS | \
                                  HB_COMPFLAG_RT_MACRO ) )
#endif

HB_EXTERN_END

#endif /* HB_COMP_H_ */
