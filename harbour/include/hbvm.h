/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for the Virtual Machine API
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

#ifndef HB_VM_H_
#define HB_VM_H_

#include "hbapi.h"

HB_EXTERN_BEGIN

/* Harbour virtual machine init/exit functions */
extern HB_EXPORT void     hb_vmInit( BOOL bStartMainProc );
extern HB_EXPORT int      hb_vmQuit( void ); /* Immediately quits the virtual machine, return ERRORLEVEL code */

/* registration AtInit and AtExit functions - they are executed
 * just before (after) .prg INIT (EXIT) procedures.
 */
extern HB_EXPORT void     hb_vmAtInit( HB_INIT_FUNC pFunc, void * cargo );
extern HB_EXPORT void     hb_vmAtExit( HB_INIT_FUNC pFunc, void * cargo );

/* Harbour virtual machine functions */
extern HB_EXPORT void     hb_vmExecute( const BYTE * pCode, PHB_SYMB pSymbols );  /* invokes the virtual machine */
extern HB_EXPORT PHB_SYMB hb_vmProcessSymbols( PHB_SYMB pSymbols, USHORT uiSymbols ); /* old module symbols initialization */
extern HB_EXPORT PHB_SYMB hb_vmProcessSymbolsEx( PHB_SYMB pSymbols, USHORT uiSymbols, char * szModuleName, ULONG ulID, USHORT uiPcodeVer ); /* module symbols initialization with extended information */

#ifdef _HB_API_INTERNAL_
   typedef struct _HB_SYMBOLS
   {
      PHB_SYMB pModuleSymbols;      /* pointer to module symbol table */
      USHORT   uiModuleSymbols;     /* number of symbols on that table */
      struct _HB_SYMBOLS * pNext;   /* pointer to the next SYMBOLS structure */
      HB_SYMBOLSCOPE hScope;        /* scope collected from all symbols in module used to speed initialization code */
      void *   hDynLib;             /* handler to dynamic library */
      BOOL     fAllocated;          /* the symbol table is dynamically allocated and should be freed on HVM exit */
      BOOL     fActive;             /* the symbol table is currently active */
      BOOL     fInitStatics;        /* static initialization should be executed */
      char *   szModuleName;        /* module name */
      ULONG    ulID;                /* module unique identifier */
   } HB_SYMBOLS, * PHB_SYMBOLS;     /* structure to keep track of all modules symbol tables */

   extern PHB_SYMBOLS hb_vmRegisterSymbols( PHB_SYMB pModuleSymbols, USHORT uiSymbols, char * szModuleName, ULONG ulID, BOOL fDynLib, BOOL fClone );
   extern void        hb_vmFreeSymbols( PHB_SYMBOLS pSymbols );
   extern void        hb_vmBeginSymbolGroup( void * hDynLib, BOOL fClone );
   extern void        hb_vmInitSymbolGroup( void * hNewDynLib, int argc, char * argv[] );
   extern void        hb_vmExitSymbolGroup( void * hDynLib );
   extern char *      hb_vmFindModuleSymbolName( PHB_SYMB pSym );

   extern void        hb_vmEnumRelease( PHB_ITEM pBase, PHB_ITEM pValue );
#endif

extern HB_EXPORT void     hb_vmSymbolInit_RT( void );   /* initialization of runtime support symbols */

/* Harbour virtual machine escaping API */
extern HB_EXPORT void     hb_vmRequestDebug( void );
extern HB_EXPORT void     hb_vmRequestBreak( PHB_ITEM pItem );
extern HB_EXPORT void     hb_vmRequestCancel( void );
extern HB_EXPORT void     hb_vmRequestQuit( void );
extern HB_EXPORT void     hb_vmRequestEndProc( void );
extern HB_EXPORT USHORT   hb_vmRequestQuery( void );

/* Return values of hb_vmRequestQuery() */
#define HB_QUIT_REQUESTED       1   /* immediately quit the application */
#define HB_BREAK_REQUESTED      2   /* break to nearest RECOVER/END sequence */
#define HB_ENDPROC_REQUESTED    4   /* immediately return from procedure (error handler in macro evaluation) */

/* Public PCode functions */

/* Execution */
extern HB_EXPORT void     hb_vmDo( USHORT uiParams );      /* invoke the virtual machine */
extern HB_EXPORT void     hb_vmFunction( USHORT uiParams ); /* executes a function */
extern HB_EXPORT void     hb_vmSend( USHORT uiParams ); /* sends a message to an object */
extern HB_EXPORT PHB_ITEM hb_vmEvalBlock( PHB_ITEM pBlockItem ); /* executes passed codeblock with no arguments */
/* executes passed codeblock with variable number of arguments */
extern HB_EXPORT PHB_ITEM hb_vmEvalBlockV( PHB_ITEM pBlockItem, ULONG ulArgCount, ... );
extern HB_EXPORT PHB_ITEM hb_vmEvalBlockOrMacro( PHB_ITEM pItem ); /* executes codeblock or macro pointed by given item */
extern HB_EXPORT void     hb_vmDestroyBlockOrMacro( PHB_ITEM pItem ); /* destroy codeblock or macro in given item */

/* Push */
extern HB_EXPORT void     hb_vmPush( PHB_ITEM pItem );     /* pushes a generic item onto the stack */
extern HB_EXPORT void     hb_vmPushNil( void );            /* in this case it places nil at self */
extern HB_EXPORT void     hb_vmPushNumber( double dNumber, int iDec ); /* pushes a number on to the stack and decides if it is integer, long or double */
extern HB_EXPORT void     hb_vmPushInteger( int iNumber ); /* pushes a integer number onto the stack */
extern HB_EXPORT void     hb_vmPushLong( long lNumber ); /* pushes a long number onto the stack */
extern HB_EXPORT void     hb_vmPushDouble( double lNumber, int iDec ); /* pushes a double number onto the stack */
extern HB_EXPORT void     hb_vmPushLogical( BOOL bValue );    /* pushes a logical value onto the stack */
extern HB_EXPORT void     hb_vmPushString( char * szText, ULONG length );  /* pushes a string on to the stack */
extern HB_EXPORT void     hb_vmPushStringPcode( char * szText, ULONG length );  /* pushes a string from pcode on to the stack */
extern HB_EXPORT void     hb_vmPushDate( long lDate );   /* pushes a long date onto the stack */
extern HB_EXPORT void     hb_vmPushSymbol( PHB_SYMB pSym ); /* pushes a function pointer onto the stack */
extern HB_EXPORT void     hb_vmPushPointer( void * ); /* push an item of HB_IT_POINTER type */
extern HB_EXPORT void     hb_vmPushState( void ); /* push current VM state on stack */
extern HB_EXPORT void     hb_vmPopState( void ); /* pop current VM state from stack */

/* various flags for supported features */
#define  HB_VMFLAG_HARBOUR    1     /* enable Harbour extension */
#define  HB_VMFLAG_ARRSTR     2     /* support for string as array of bytes -ks */
extern HB_EXPORT ULONG    hb_vmFlagEnabled( ULONG flag );

HB_EXTERN_END

#endif /* HB_VM_H_ */
