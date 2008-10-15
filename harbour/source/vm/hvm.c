/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The Virtual Machine
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
 *    hb_vmPushLongConst()
 *    hb_vmPushDoubleConst()
 *
 * Copyright 1999 Eddie Runia <eddie@runia.com>
 *    __DBGVMVARSGET()
 *    __DBGVMVARSLIST()
 *
 * See doc/license.txt for licensing terms.
 *
 */

#define HB_STACK_PRELOAD

#include <math.h>
#include <time.h>

#include "hbvmopt.h"
#include "hbapi.h"
#include "hbstack.h"
#include "hbapierr.h"
#include "hbapicls.h"
#include "hbapidbg.h"
#include "hbapiitm.h"
#include "hbapilng.h"
#include "hbapirdd.h"
#include "hbapigt.h"
#include "hbapicdp.h"
#include "hbvm.h"
#include "hbxvm.h"
#include "hbpcode.h"
#include "hbset.h"
#include "hbinkey.ch"
#include "inkey.ch"
#include "hbdebug.ch"
#if defined( HB_MT_VM )
#  include "hbthread.h"
#endif
#include "hbmemory.ch"


HB_FUNC_EXTERN( SYSINIT );

/* PCode functions */

/* Operators (mathematical / character / misc) */
static void    hb_vmNegate( void );          /* negates (-) the latest value on the stack */
static void    hb_vmInc( HB_ITEM_PTR pItem );   /* increment the latest numeric value on the stack */
static void    hb_vmDec( HB_ITEM_PTR pItem );   /* decrements the latest numeric value on the stack */
static void    hb_vmFuncPtr( void );         /* pushes a function address pointer. Removes the symbol from the satck */
static void    hb_vmAddInt( HB_ITEM_PTR pResult, LONG lAdd );      /* add integer to given item */
static void    hb_vmPlus( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 );        /* sums given values */
static void    hb_vmMinus( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 );       /* substracts given values */
static void    hb_vmMult( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 );        /* multiplies given values */
static void    hb_vmDivide( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 );      /* divides the given values */
static void    hb_vmModulus( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 );     /* calculates modulus given values */
static void    hb_vmPower( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 );       /* power given values */

/* Operators (relational) */
static void    hb_vmEqual( void );           /* checks if the two latest values on the stack are equal, removes both and leaves result */
static void    hb_vmExactlyEqual( void );    /* checks if the two latest values on the stack are exactly equal, removes both and leaves result */
static void    hb_vmNotEqual( void );        /* checks if the two latest values on the stack are not equal, removes both and leaves result */
static void    hb_vmLess( void );            /* checks if the latest - 1 value is less than the latest, removes both and leaves result */
static void    hb_vmLessEqual( void );       /* checks if the latest - 1 value is less than or equal the latest, removes both and leaves result */
static void    hb_vmGreater( void );         /* checks if the latest - 1 value is greater than the latest, removes both and leaves result */
static void    hb_vmGreaterEqual( void );    /* checks if the latest - 1 value is greater than or equal the latest, removes both and leaves result */
static void    hb_vmInstring( void );        /* check whether string 1 is contained in string 2 */
static void    hb_vmForTest( void );         /* test for end condition of for */
static void    hb_vmSeqBlock( void );        /* set begin sequence WITH codeblock */
static void    hb_vmWithObjectStart( void ); /* prepare WITH OBJECT block */
static void    hb_vmEnumStart( BYTE, BYTE ); /* prepare FOR EACH loop */
static void    hb_vmEnumNext( void );        /* increment FOR EACH loop counter */
static void    hb_vmEnumPrev( void );        /* decrement FOR EACH loop counter */
static void    hb_vmEnumEnd( void );         /* rewind the stack after FOR EACH loop counter */
static LONG    hb_vmSwitch( const BYTE * pCode, LONG, USHORT );  /* make a SWITCH statement */

/* Operators (logical) */
static void    hb_vmNot( void );             /* changes the latest logical value on the stack */
static void    hb_vmAnd( void );             /* performs the logical AND on the latest two values, removes them and leaves result on the stack */
static void    hb_vmOr( void );              /* performs the logical OR on the latest two values, removes them and leaves result on the stack */

/* Array */
static void    hb_vmArrayPush( void );       /* pushes an array element to the stack, removing the array and the index from the stack */
static void    hb_vmArrayPushRef( void );    /* pushes a reference to an array element to the stack, removing the array and the index from the stack */
static void    hb_vmArrayPop( void );        /* pops a value from the stack */
static void    hb_vmArrayDim( USHORT uiDimensions ); /* generates an uiDimensions Array and initialize those dimensions from the stack values */
static void    hb_vmArrayGen( ULONG ulElements ); /* generates an ulElements Array and fills it from the stack values */
static void    hb_vmHashGen( ULONG ulElements ); /* generates an ulElements Hash and fills it from the stack values */

/* macros */
static void    hb_vmMacroDo( USHORT uiArgSets );         /* execute function passing arguments set(s) on HVM stack func( &var ) */
static void    hb_vmMacroFunc( USHORT uiArgSets );       /* execute procedure passing arguments set(s) on HVM stack func( &var ) */
static void    hb_vmMacroSend( USHORT uiArgSets );       /* execute procedure passing arguments set(s) on HVM stack func( &var ) */
static void    hb_vmMacroArrayGen( USHORT uiArgSets );   /* generate array from arguments set(s) on HVM stack { &var } */
static void    hb_vmMacroPushIndex( void );              /* push macro array index {...}[ &var ] */

/* Database */
static ERRCODE hb_vmSelectWorkarea( PHB_ITEM, PHB_SYMB );  /* select the workarea using a given item or a substituted value */
static void    hb_vmSwapAlias( void );           /* swaps items on the eval stack and pops the workarea number */

/* Execution */
static HARBOUR hb_vmDoBlock( void );             /* executes a codeblock */
static void    hb_vmFrame( USHORT usLocals, BYTE bParams ); /* increases the stack pointer for the amount of locals and params suplied */
static void    hb_vmVFrame( USHORT usLocals, BYTE bParams ); /* increases the stack pointer for the amount of locals and variable number of params suplied */
static void    hb_vmSFrame( PHB_SYMB pSym );     /* sets the statics frame for a function */
static void    hb_vmStatics( PHB_SYMB pSym, USHORT uiStatics ); /* increases the global statics array to hold a PRG statics */
static void    hb_vmInitThreadStatics( USHORT uiCount, const BYTE * pCode ); /* mark thread static variables */
static void    hb_vmEndBlock( void );            /* copies the last codeblock pushed value into the return value */
static void    hb_vmRetValue( void );            /* pops the latest stack value into stack.Return */
/* Push */
static void    hb_vmPushAlias( void );            /* pushes the current workarea number */
static void    hb_vmPushAliasedField( PHB_SYMB ); /* pushes an aliased field on the eval stack */
static void    hb_vmPushAliasedVar( PHB_SYMB );   /* pushes an aliased variable on the eval stack */
static void    hb_vmPushBlock( const BYTE * pCode, PHB_SYMB pSymbols, ULONG ulLen ); /* creates a codeblock */
static void    hb_vmPushBlockShort( const BYTE * pCode, PHB_SYMB pSymbols, ULONG ulLen ); /* creates a codeblock */
static void    hb_vmPushMacroBlock( const BYTE * pCode, ULONG ulSize, USHORT usParams ); /* creates a macro-compiled codeblock */
static void    hb_vmPushDoubleConst( double dNumber, int iWidth, int iDec ); /* Pushes a double constant (pcode) */
static void    hb_vmPushLocal( int iLocal );       /* pushes the containts of a local onto the stack */
static void    hb_vmPushLocalByRef( int iLocal );  /* pushes a local by refrence onto the stack */
static void    hb_vmPushHBLong( HB_LONG lNumber ); /* pushes a HB_LONG number onto the stack */
#if !defined( HB_LONG_LONG_OFF )
   static void hb_vmPushLongLongConst( LONGLONG lNumber );  /* Pushes a long long constant (pcode) */
#endif
#if HB_INT_MAX >= INT32_MAX
static void    hb_vmPushIntegerConst( int iNumber );  /* Pushes a int constant (pcode) */
#else
static void    hb_vmPushLongConst( long lNumber );    /* Pushes a long constant (pcode) */
#endif
static void    hb_vmPushStatic( USHORT uiStatic );     /* pushes the containts of a static onto the stack */
static void    hb_vmPushStaticByRef( USHORT uiStatic ); /* pushes a static by refrence onto the stack */
static void    hb_vmPushVariable( PHB_SYMB pVarSymb ); /* pushes undeclared variable */
static void    hb_vmPushObjectVarRef( void );   /* pushes reference to object variable */
static void    hb_vmPushVParams( void );        /* pusges variable parameters */
static void    hb_vmPushUnRef( void );          /* push the unreferenced latest value on the stack */
static void    hb_vmDuplicate( void );          /* duplicates the latest value on the stack */
static void    hb_vmDuplTwo( void );            /* duplicates the latest two value on the stack */
static void    hb_vmDuplUnRef( void );          /* duplicates the latest value on the stack and unref the source one */
static void    hb_vmSwap( BYTE bCount );        /* swap bCount+1 time two items on HVM stack starting from the most top one */

/* Pop */
static BOOL    hb_vmPopLogical( void );           /* pops the stack latest value and returns its logical value */
static double  hb_vmPopNumber( void );            /* pops the stack latest value and returns its numeric value */
static void    hb_vmPopAlias( void );             /* pops the workarea number form the eval stack */
static void    hb_vmPopAliasedField( PHB_SYMB );  /* pops an aliased field from the eval stack*/
static void    hb_vmPopAliasedVar( PHB_SYMB );    /* pops an aliased variable from the eval stack*/
static void    hb_vmPopLocal( int iLocal );       /* pops the stack latest value onto a local */
static void    hb_vmPopStatic( USHORT uiStatic ); /* pops the stack latest value onto a static */

/* misc */
static void    hb_vmDoInitStatics( void );        /* executes all _INITSTATICS functions */
static void    hb_vmDoInitFunctions( void );      /* executes all defined PRGs INIT functions */
static void    hb_vmDoExitFunctions( void );      /* executes all defined PRGs EXIT functions */
static void    hb_vmReleaseLocalSymbols( void );  /* releases the memory of the local symbols linked list */

static void    hb_vmMsgIndexReference( PHB_ITEM pRefer, PHB_ITEM pObject, PHB_ITEM pIndex ); /* create object index reference */

#ifndef HB_NO_DEBUG
static void    hb_vmLocalName( USHORT uiLocal, char * szLocalName ); /* locals and parameters index and name information for the debugger */
static void    hb_vmStaticName( BYTE bIsGlobal, USHORT uiStatic, char * szStaticName ); /* statics vars information for the debugger */
static void    hb_vmModuleName( char * szModuleName ); /* PRG and function name information for the debugger */

static void    hb_vmDebugEntry( int nMode, int nLine, char *szName, int nIndex, int nFrame );
static void    hb_vmDebuggerExit( BOOL fRemove );      /* shuts down the debugger */
static void    hb_vmDebuggerShowLine( USHORT uiLine ); /* makes the debugger shows a specific source code line */
static void    hb_vmDebuggerEndProc( void );     /* notifies the debugger for an endproc */

static PHB_DYNS s_pDynsDbgEntry = NULL;   /* Cached __DBGENTRY symbol */
static HB_DBGENTRY_FUNC s_pFunDbgEntry;   /* C level debugger entry */
#endif

#if defined( HB_MT_VM )
static int volatile hb_vmThreadRequest = 0;
static void hb_vmRequestTest( void );

static PHB_ITEM s_pSymbolsMtx = NULL;

static HB_CRITICAL_NEW( s_atInitMtx );
#  define HB_ATINIT_LOCK      hb_threadEnterCriticalSection( &s_atInitMtx );
#  define HB_ATINIT_UNLOCK    hb_threadLeaveCriticalSection( &s_atInitMtx );
#else
#  define HB_ATINIT_LOCK
#  define HB_ATINIT_UNLOCK
#endif

#ifndef HB_NO_PROFILER
static ULONG hb_ulOpcodesCalls[ HB_P_LAST_PCODE ];/* array to profile opcodes calls */
static ULONG hb_ulOpcodesTime[ HB_P_LAST_PCODE ]; /* array to profile opcodes consumed time */
static BOOL  hb_bProfiler = FALSE;                        /* profiler status is off */
#endif

#if defined( HB_PRG_TRACE )
static BOOL hb_bTracePrgCalls = FALSE; /* prg tracing is off */
#  define HB_TRACE_PRG( _TRMSG_ ) if( hb_bTracePrgCalls ) HB_TRACE( HB_TR_ALWAYS, _TRMSG_ )
#else
#  define HB_TRACE_PRG( _TRMSG_ )
#endif

HB_EXPORT const char * hb_vm_pszLinkedMain = NULL; /* name of startup function set by linker */

/* virtual machine state */

HB_SYMB  hb_symEval      = { "EVAL",      {HB_FS_PUBLIC},  {hb_vmDoBlock}, NULL }; /* symbol to evaluate codeblocks */

static HB_ITEM  s_aStatics;            /* Harbour array to hold all application statics variables */

static BOOL     s_fHVMActive = FALSE;  /* is HVM ready for PCODE executing */
static BOOL     s_fDoExitProc = TRUE;  /* execute EXIT procedures */
static int      s_nErrorLevel = 0;     /* application exit errorlevel */
static PHB_SYMB s_pSymStart = NULL;    /* start symbol of the application. MAIN() is not required */

static PHB_SYMBOLS s_pSymbols = NULL;  /* to hold a linked list of all different modules symbol tables */
static ULONG       s_ulFreeSymbols = 0;/* number of free module symbols */
static void *      s_hDynLibID = NULL; /* unique identifer to mark symbol tables loaded from dynamic libraries */
static BOOL        s_fCloneSym = FALSE;/* clone registered symbol tables */

/* main VM thread stack ID */
static void * s_main_thread = NULL;

/* Various compatibility flags
*/
static ULONG      s_VMFlags = HB_VMFLAG_HARBOUR;
#undef hb_vmFlagEnabled
#define hb_vmFlagEnabled(flag)      (s_VMFlags & (flag))

/* Keycodes to stop virtual machine
*/
static int        s_VMCancelKey = K_ALT_C;
static int        s_VMCancelKeyEx = HB_K_ALT_C;

/* SEQUENCE envelope items position from stack top active
 */
#define HB_RECOVER_STATE     -1
#define HB_RECOVER_VALUE     -2

#define HB_SEQ_CANRECOVER    64
#define HB_SEQ_DOALWAYS     128

static PHB_FUNC_LIST s_InitFunctions = NULL;
static PHB_FUNC_LIST s_ExitFunctions = NULL;

HB_EXPORT void hb_vmAtInit( HB_INIT_FUNC pFunc, void * cargo )
{
   PHB_FUNC_LIST pLst = ( PHB_FUNC_LIST ) hb_xgrab( sizeof( HB_FUNC_LIST ) );

   pLst->pFunc = pFunc;
   pLst->cargo = cargo;
   HB_ATINIT_LOCK
   pLst->pNext = s_InitFunctions;
   s_InitFunctions = pLst;
   HB_ATINIT_UNLOCK
}

HB_EXPORT void hb_vmAtExit( HB_INIT_FUNC pFunc, void * cargo )
{
   PHB_FUNC_LIST pLst = ( PHB_FUNC_LIST ) hb_xgrab( sizeof( HB_FUNC_LIST ) );

   pLst->pFunc = pFunc;
   pLst->cargo = cargo;
   HB_ATINIT_LOCK
   pLst->pNext = s_ExitFunctions;
   s_ExitFunctions = pLst;
   HB_ATINIT_UNLOCK
}

static void hb_vmCleanModuleFunctions( void )
{
   PHB_FUNC_LIST pLst;

   while( s_InitFunctions )
   {
      pLst = s_InitFunctions;
      s_InitFunctions = pLst->pNext;
      hb_xfree( pLst );
   }
   while( s_ExitFunctions )
   {
      pLst = s_ExitFunctions;
      s_ExitFunctions = pLst->pNext;
      hb_xfree( pLst );
   }
}

static void hb_vmDoModuleInitFunctions( void )
{
   PHB_FUNC_LIST pLst = s_InitFunctions;

   while( pLst )
   {
      pLst->pFunc( pLst->cargo );
      pLst = pLst->pNext;
   }
}

static void hb_vmDoModuleExitFunctions( void )
{
   PHB_FUNC_LIST pLst = s_ExitFunctions;

   while( pLst )
   {
      pLst->pFunc( pLst->cargo );
      pLst = pLst->pNext;
   }
}


/* call CLIPINIT function to initialize ErrorBlock() and __SetHelpK() */
static void hb_vmDoInitClip( void )
{
   PHB_DYNS pDynSym = hb_dynsymFind( "CLIPINIT" );

   if( pDynSym && pDynSym->pSymbol->value.pFunPtr )
   {
      hb_vmPushSymbol( pDynSym->pSymbol );
      hb_vmPushNil();
      hb_vmDo( 0 );
   }
}

#if !defined( HB_MT_VM )

BOOL hb_vmIsMt( void ) { return FALSE; }
void hb_vmLock( void ) {}
void hb_vmUnlock( void ) {}
BOOL hb_vmSuspendThreads( BOOL fWait ) { HB_SYMBOL_UNUSED( fWait ); return TRUE; }
void hb_vmResumeThreads( void ) {}

#else

static HB_CRITICAL_NEW( s_vmMtx );
static HB_COND_NEW( s_vmCond );

/* number of allocated HVM stacks */
static int volatile s_iStackCount = 0;
/* number of running HVM threads */
static int volatile s_iRunningCount = 0;
/* active HVM stacks list */
static PHB_THREADSTATE s_vmStackLst = NULL;
/* thread number */
static HB_THREAD_NO s_threadNo = 0;

#  define HB_THREQUEST_STOP   1
#  define HB_THREQUEST_QUIT   2

#  define HB_VM_LOCK      hb_threadEnterCriticalSection( &s_vmMtx );
#  define HB_VM_UNLOCK    hb_threadLeaveCriticalSection( &s_vmMtx );

BOOL hb_vmIsMt( void ) { return TRUE; }

static void hb_vmRequestTest( void )
{
   HB_VM_LOCK

   s_iRunningCount--;
   while( TRUE )
   {
      if( hb_vmThreadRequest & HB_THREQUEST_QUIT )
      {
         HB_STACK_TLS_PRELOAD
         if( !hb_stackQuitState() )
         {
            hb_stackSetQuitState( TRUE );
            hb_stackSetActionRequest( HB_QUIT_REQUESTED );
         }
      }
      if( hb_vmThreadRequest & HB_THREQUEST_STOP )
      {
         hb_threadCondBroadcast( &s_vmCond );
         hb_threadCondWait( &s_vmCond, &s_vmMtx );
      }
      else
         break;
   }
   s_iRunningCount++;

   HB_VM_UNLOCK
}

/* unlock VM, allow GC and other exclusive single task code execution */
void hb_vmUnlock( void )
{
   HB_STACK_TLS_PRELOAD

   if( hb_stackId() )   /* check if thread has associated HVM stack */
   {
      if( hb_stackUnlock() == 1 )
      {
         HB_VM_LOCK
         s_iRunningCount--;
         if( hb_vmThreadRequest )
         {
            if( hb_vmThreadRequest & HB_THREQUEST_QUIT )
            {
               if( !hb_stackQuitState() )
               {
                  hb_stackSetQuitState( TRUE );
                  hb_stackSetActionRequest( HB_QUIT_REQUESTED );
               }
            }
            hb_threadCondBroadcast( &s_vmCond );
         }
         HB_VM_UNLOCK
      }
   }
}

/* lock VM blocking GC and other exclusive single task code execution */
void hb_vmLock( void )
{
   HB_STACK_TLS_PRELOAD

   if( hb_stackId() )   /* check if thread has associated HVM stack */
   {
      if( hb_stackLock() == 0 )
      {
         HB_VM_LOCK
         while( TRUE )
         {
            if( hb_vmThreadRequest & HB_THREQUEST_QUIT )
            {
               if( !hb_stackQuitState() )
               {
                  hb_stackSetQuitState( TRUE );
                  hb_stackSetActionRequest( HB_QUIT_REQUESTED );
               }
            }
            if( hb_vmThreadRequest & HB_THREQUEST_STOP )
               hb_threadCondWait( &s_vmCond, &s_vmMtx );
            else
               break;
         }
         s_iRunningCount++;
         HB_VM_UNLOCK
      }
   }
}

/* (try to) stop all threads except current one */
BOOL hb_vmSuspendThreads( BOOL fWait )
{
   HB_VM_LOCK

   if( ( hb_vmThreadRequest & ( HB_THREQUEST_STOP | HB_THREQUEST_QUIT ) ) == 0 )
   {
      hb_vmThreadRequest |= HB_THREQUEST_STOP;
      --s_iRunningCount;
      while( TRUE )
      {
         if( s_iRunningCount <= 0 )
         {
            hb_vmThreadRequest &= ~HB_THREQUEST_STOP;
            ++s_iRunningCount;
            return TRUE;
         }
         if( !fWait )
            break;
         hb_threadCondWait( &s_vmCond, &s_vmMtx );
         if( hb_vmThreadRequest & HB_THREQUEST_QUIT )
            break;
      }
      hb_vmThreadRequest &= ~HB_THREQUEST_STOP;
      ++s_iRunningCount;
      hb_threadCondBroadcast( &s_vmCond );
   }

   HB_VM_UNLOCK

   return FALSE;
}

/* unblock execution of threads stopped by hb_vmSuspendThreads() */
void hb_vmResumeThreads( void )
{
   hb_vmThreadRequest &= ~HB_THREQUEST_STOP;
   hb_threadCondBroadcast( &s_vmCond );
   HB_VM_UNLOCK
}

/* send QUIT request to all threads except current one
 * and wait for their termination,
 * should be called only by main HVM thread
 */
void hb_vmTerminateThreads( void )
{
   HB_STACK_TLS_PRELOAD

   if( s_main_thread == hb_stackId() )
   {
      HB_VM_LOCK

      hb_vmThreadRequest |= HB_THREQUEST_QUIT;
      --s_iRunningCount;

      hb_threadCondBroadcast( &s_vmCond );

      while( s_iStackCount > 1 )
         hb_threadCondWait( &s_vmCond, &s_vmMtx );

      ++s_iRunningCount;
      /* hb_vmThreadRequest &= ~HB_THREQUEST_QUIT; */
      hb_vmThreadRequest = 0;

      HB_VM_UNLOCK
   }
}

/* wait for all threads to terminate
 * should be called only by main HVM thread
 */
void hb_vmWaitForThreads( void )
{
   HB_STACK_TLS_PRELOAD

   if( s_main_thread == hb_stackId() )
   {
      HB_VM_LOCK

      --s_iRunningCount;
      if( hb_vmThreadRequest )
         hb_threadCondBroadcast( &s_vmCond );

      while( s_iStackCount > 1 )
         hb_threadCondWait( &s_vmCond, &s_vmMtx );

      ++s_iRunningCount;

      HB_VM_UNLOCK
   }
}

void * hb_vmThreadState( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmThreadState()"));

   return hb_stackList();
}

static void hb_vmStackAdd( PHB_THREADSTATE pState )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmStackAdd(%p)", pState));

   if( !pState->pPrev )
   {
      if( s_vmStackLst )
      {
         pState->pNext = s_vmStackLst;
         pState->pPrev = s_vmStackLst->pPrev;
         pState->pPrev->pNext = pState;
         s_vmStackLst->pPrev = pState;
      }
      else
      {
         s_vmStackLst = pState->pNext = pState->pPrev = pState;
      }
      s_iStackCount++;
   }
   if( pState->th_no == 0 )
      pState->th_no = ++s_threadNo;
}

static PHB_ITEM hb_vmStackDel( PHB_THREADSTATE pState )
{
   PHB_ITEM pThItm;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmStackDel(%p)", pState));

   pState->fActive = FALSE;
   pState->pStackId = NULL;
   pState->fFinished = TRUE;

   if( pState->pPrev )
   {
      pState->pPrev->pNext = pState->pNext;
      pState->pNext->pPrev = pState->pPrev;
      if( s_vmStackLst == pState )
      {
         s_vmStackLst = pState->pNext;
         if( s_vmStackLst == pState )
            s_vmStackLst = NULL;
      }
      pState->pPrev = pState->pNext = NULL;
   }

   /* NOTE: releasing pThItm may force pState freeing if parent
    *       thread does not keep thread pointer item. So it's
    *       important to not access it later. [druzus]
    */
   pThItm = pState->pThItm;
   pState->pThItm = NULL;

   return pThItm;
}

static void hb_vmStackInit( PHB_THREADSTATE pState )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmStackInit(%p)", pState));

   hb_stackInit();      /* initialize HVM thread stack */

   HB_VM_LOCK
   {
      HB_STACK_TLS_PRELOAD

      hb_stackUnlock();
      pState->pStackId = hb_stackId();
      hb_stackListSet( ( void * ) ( pState ) );
      pState->fActive = TRUE;
      hb_vmStackAdd( pState );
   }
   HB_VM_UNLOCK

   hb_vmLock();
}

static void hb_vmStackRelease( void )
{
   HB_STACK_TLS_PRELOAD
   BOOL fLocked;
   PHB_ITEM pThItm;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmStackRelease()"));

   HB_VM_LOCK

   fLocked = hb_stackUnlock() == 1;
   pThItm = hb_vmStackDel( ( PHB_THREADSTATE ) hb_stackList() );

   HB_VM_UNLOCK

   if( pThItm )
      hb_itemRelease( pThItm );

   hb_setRelease( hb_stackSetStruct() );
   hb_stackFree();

   hb_threadMutexUnlockAll();

   HB_VM_LOCK

   if( fLocked )
      s_iRunningCount--;

   s_iStackCount--;
   hb_threadCondBroadcast( &s_vmCond );

   HB_VM_UNLOCK
}

HB_EXPORT BOOL hb_vmThreadRegister( void * Cargo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmThreadRegister(%p)", Cargo));

   HB_VM_LOCK

   hb_vmStackAdd( ( PHB_THREADSTATE ) Cargo );

   HB_VM_UNLOCK

   return TRUE;
}

HB_EXPORT void hb_vmThreadRelease( void * Cargo )
{
   PHB_ITEM pThItm;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmThreadRelease(%p)", Cargo));

   HB_VM_LOCK

   pThItm = hb_vmStackDel( ( PHB_THREADSTATE ) Cargo );
   s_iStackCount--;
   hb_threadCondBroadcast( &s_vmCond );

   HB_VM_UNLOCK

   if( pThItm )
      hb_itemRelease( pThItm );
}

/* thread entry point */
HB_EXPORT void hb_vmThreadInit( void * Cargo )
{
   PHB_THREADSTATE pState;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmThreadInit(%p)", Cargo));

   pState = ( PHB_THREADSTATE ) Cargo;
   if( !pState )
      pState = hb_threadStateNew();

   hb_vmStackInit( pState );  /* initialize HVM thread stack */
   {
      HB_STACK_TLS_PRELOAD

      hb_cdpSelectID( pState->pszCDP );
      hb_langSelectID( pState->pszLang );

      if( pState && pState->pSet )
      {
         /* TODO: add set sharing */
         memcpy( hb_stackSetStruct(), pState->pSet, sizeof( HB_SET_STRUCT ) );
         hb_xfree( pState->pSet );
         pState->pSet = NULL;
      }
      else
         hb_setInitialize( hb_stackSetStruct() );

      if( pState->pszDefRDD )
         hb_stackRDD()->szDefaultRDD = pState->pszDefRDD;

      if( s_fHVMActive )
      {
         /* call CLIPINIT function to initialize GetList PUBLIC variables
          * ErrorBlock() and __SetHelpK()
          */
         hb_vmDoInitClip();
      }

      if( pState->pMemvars )
      {
         hb_memvarRestoreFromArray( pState->pMemvars );
         hb_itemRelease( pState->pMemvars );
         pState->pMemvars = NULL;
      }
   }
}

/* thread leave point */
HB_EXPORT void hb_vmThreadQuit( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_THREADSTATE pState;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmThreadQuit()"));

   hb_stackSetQuitState( TRUE );
   hb_stackSetActionRequest( 0 );

   pState = ( PHB_THREADSTATE ) hb_stackList();
   {
      PHB_ITEM pReturn = hb_stackReturnItem();

      if( HB_IS_BYREF( pReturn ) )
         pReturn = hb_itemUnRef( pReturn );

      if( !pState->pResult )
         pState->pResult = hb_itemNew( pReturn );
      else
         hb_itemCopy( pState->pResult, pReturn );
   }
   hb_itemClear( hb_stackReturnItem() );

   hb_stackSetActionRequest( 0 );
   hb_rddCloseAll();             /* close all workareas */
   hb_stackRemove( 1 );          /* clear stack items, leave only initial symbol item */
   hb_memvarsClear();            /* clear all PUBLIC (and PRIVATE if any) variables */
#ifndef HB_NO_DEBUG
   hb_vmDebuggerExit( FALSE );   /* deactivate debugger */
#endif
   hb_vmStackRelease();          /* release HVM stack and remove it from linked HVM stacks list */
}

/* send QUIT request to given thread */
HB_EXPORT void hb_vmThreadQuitRequest( void * Cargo )
{
   PHB_THREADSTATE pState;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmThreadQuitRequest(%p)", Cargo));

   pState = ( PHB_THREADSTATE ) Cargo;

   HB_VM_LOCK

   if( pState->pStackId && pState->fActive )
      hb_stackIdSetActionRequest( pState->pStackId, HB_QUIT_REQUESTED );

   HB_VM_UNLOCK
}

#endif

/* application entry point */

HB_EXPORT void hb_vmInit( BOOL bStartMainProc )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmInit()"));

   /* initialize internal data structures */
   s_aStatics.type = HB_IT_NIL;

   hb_vmSymbolInit_RT();      /* initialize symbol table with runtime support functions */

   hb_xinit();

   hb_vmSetExceptionHandler();

#if defined( HB_MT_VM )
   hb_threadInit();
   hb_vmStackInit( hb_threadStateNew() ); /* initialize HVM thread stack */
   s_pSymbolsMtx = hb_threadMutexCreate( FALSE );
#else
   hb_stackInit();                        /* initialize HVM stack */
#endif
   /* Set the language and codepage to the default */
   /* This trick is needed to stringify the macro value */
   hb_langSelectID( HB_MACRO2STRING( HB_LANG_DEFAULT ) );
   hb_cdpSelectID( HB_MACRO2STRING( HB_CODEPAGE_DEFAULT ) );
   {
      HB_STACK_TLS_PRELOAD
      s_main_thread = hb_stackId();
      /* _SET_* initialization */
      hb_setInitialize( hb_stackSetStruct() );
   }

   hb_clsInit();              /* initialize Classy/OO system */
   hb_errInit();

   /* initialize dynamic symbol for evaluating codeblocks */
   hb_symEval.pDynSym = hb_dynsymGetCase( hb_symEval.szName );

   hb_conInit();

   /* Check for some internal switches */
   s_VMFlags = hb_cmdargProcessVM( &s_VMCancelKey, &s_VMCancelKeyEx );
   hb_inkeySetCancelKeys( s_VMCancelKey, s_VMCancelKeyEx );

#ifndef HB_NO_PROFILER
   /* Initialize opcodes profiler support arrays */
   {
      ULONG ul;

      for( ul = 0; ul < HB_P_LAST_PCODE; ul++ )
      {
         hb_ulOpcodesCalls[ ul ] = 0;
         hb_ulOpcodesTime[ ul ] = 0;
      }
   }
#endif

   /* enable executing PCODE (HVM reenter request) */
   s_fHVMActive = TRUE;

#ifndef HB_NO_DEBUG
   s_pDynsDbgEntry = hb_dynsymFind( "__DBGENTRY" );
   if( s_pDynsDbgEntry )
   {
      /* Try to get C dbgEntry() function pointer */
      if( !s_pFunDbgEntry )
         hb_vmDebugEntry( HB_DBG_GETENTRY, 0, NULL, 0, 0 );
      if( !s_pFunDbgEntry )
         s_pFunDbgEntry = hb_vmDebugEntry;
   }
#endif

   /* Call functions that initializes static variables
    * Static variables have to be initialized before any INIT functions
    * because INIT function can use static variables
    */
   hb_vmDoInitStatics();
   /* call CLIPINIT function to initialize GetList PUBLIC variables
    * ErrorBlock() and __SetHelpK()
    * Because on some platform the execution order of init functions
    * is out of Harbour control then this function has to be called
    * explicitly in VM initialization process before hb_vmDoInitFunctions()
    * and not depends on INIT clause.
    */
   hb_vmDoInitClip();
   hb_clsDoInit();                  /* initialize Classy .prg functions */

   hb_vmDoModuleInitFunctions();    /* process AtInit registered functions */
   hb_vmDoInitFunctions();          /* process defined INIT functions */

   /* This is undocumented CA-Cl*pper, if there's a function called _APPMAIN
      it will be executed first. [vszakats] */
   {
      PHB_DYNS pDynSym = hb_dynsymFind( "_APPMAIN" );

      if( pDynSym && pDynSym->pSymbol->value.pFunPtr )
         s_pSymStart = pDynSym->pSymbol;
#ifdef HB_START_PROCEDURE
      else
      {
         /* if first char is '@' then start procedure were set by
            programmer explicitly and should have the highest priority
            in other case it's the name of first public function in
            first linked moudule which is used if there is no
            HB_START_PROCEDURE in code */
         if( hb_vm_pszLinkedMain && *hb_vm_pszLinkedMain == '@' )
            pDynSym = hb_dynsymFind( hb_vm_pszLinkedMain + 1 );
         else
         {
            pDynSym = hb_dynsymFind( HB_START_PROCEDURE );

            if( ! ( pDynSym && pDynSym->pSymbol->value.pFunPtr ) && hb_vm_pszLinkedMain )
               pDynSym = hb_dynsymFind( hb_vm_pszLinkedMain );
         }

         if( pDynSym && pDynSym->pSymbol->value.pFunPtr )
            s_pSymStart = pDynSym->pSymbol;
         else
            hb_errInternal( HB_EI_VMBADSTARTUP, NULL, HB_START_PROCEDURE, NULL );
      }
#else
      else if( hb_vm_pszLinkedMain )
      {
         pDynSym = hb_dynsymFind( hb_vm_pszLinkedMain + ( *hb_vm_pszLinkedMain == '@' ? 1 : 0 ) );
         if( pDynSym && pDynSym->pSymbol->value.pFunPtr )
            s_pSymStart = pDynSym->pSymbol;
      }
#ifndef HB_C52_STRICT
      if( bStartMainProc && ! s_pSymStart )
         hb_errInternal( HB_EI_VMNOSTARTUP, NULL, NULL, NULL );
#endif
#endif
   }

   if( bStartMainProc && s_pSymStart )
   {
      int i;
      int iArgCount;

      hb_vmPushSymbol( s_pSymStart ); /* pushes first HB_FS_PUBLIC defined symbol to the stack */
      hb_vmPushNil();                 /* places NIL at self */

      iArgCount = 0;
      for( i = 1; i < hb_cmdargARGC(); i++ )     /* places application parameters on the stack */
      {
         char ** argv = hb_cmdargARGV();

         /* Filter out any parameters beginning with //, like //INFO */
         if( ! hb_cmdargIsInternal( argv[ i ], NULL ) )
         {
            hb_vmPushString( argv[ i ], strlen( argv[ i ] ) );
            iArgCount++;
         }
      }

      hb_vmDo( ( USHORT ) iArgCount ); /* invoke it with number of supplied parameters */

   }
}

HB_EXPORT int hb_vmQuit( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmQuit()"));

#if defined( HB_MT_VM )
   hb_vmTerminateThreads();
#endif

   hb_vmDoExitFunctions(); /* process defined EXIT functions */

   /* process AtExit registered functions */
   hb_vmDoModuleExitFunctions();
   hb_vmCleanModuleFunctions();

   /* release all known items stored in subsystems */
   hb_itemClear( hb_stackReturnItem() );
   hb_stackRemove( 1 );          /* clear stack items, leave only initial symbol item */

   hb_memvarsClear();            /* clear all PUBLIC (and PRIVATE if any) variables */

   /* intentionally here to allow executing object destructors for all
    * cross referenced items before we release classy subsystem
    */
   hb_gcCollectAll( TRUE );

   /* Clear any pending actions so RDD shutdown process
    * can be cleanly executed
    */
   hb_stackSetActionRequest( 0 );
   hb_rddCloseAll();             /* close all workareas */
   hb_rddShutDown();             /* remove all registered RDD drivers */

#ifndef HB_NO_DEBUG
   /* deactivate debugger */
   hb_vmDebuggerExit( TRUE );
#endif

   /* release thread specific data */
   hb_stackDestroyTSD();

   /* stop executing PCODE (HVM reenter request) */
   s_fHVMActive = FALSE;

   hb_errExit();
   hb_clsReleaseAll();

   hb_itemClear( &s_aStatics );

   /* release all remaining items */

   hb_conRelease();                       /* releases Console */
   hb_vmReleaseLocalSymbols();            /* releases the local modules linked list */
   hb_dynsymRelease();                    /* releases the dynamic symbol table */
#ifndef HB_CDP_SUPPORT_OFF
   hb_cdpReleaseAll();                    /* releases codepages */
#endif
   hb_itemClear( hb_stackReturnItem() );
   hb_gcCollectAll( TRUE );

#if defined( HB_MT_VM )
   hb_vmStackRelease();       /* release HVM stack and remove it from linked HVM stacks list */
   if( s_pSymbolsMtx )
   {
      hb_itemRelease( s_pSymbolsMtx );
      s_pSymbolsMtx = NULL;
   }
   hb_threadExit();
#else
   hb_setRelease( hb_stackSetStruct() );  /* releases Sets */
   hb_stackFree();
#endif

   /* release all known garbage */
   if( hb_xquery( HB_MEM_USEDMAX ) == 0 ) /* check if fmstat is ON */
      hb_gcReleaseAll();

   hb_vmUnsetExceptionHandler();

   hb_xexit();

   return s_nErrorLevel;
}

HB_EXPORT void hb_vmExecute( const BYTE * pCode, PHB_SYMB pSymbols )
{
   HB_STACK_TLS_PRELOAD
   LONG w = 0;
   BOOL bCanRecover = FALSE;
   BOOL bDynCode = pSymbols == NULL || ( pSymbols->scope.value & HB_FS_DYNCODE ) != 0;
#ifndef HB_NO_PROFILER
   ULONG ulLastOpcode = 0; /* opcodes profiler support */
   ULONG ulPastClock = 0;  /* opcodes profiler support */
#endif
#if !defined( HB_GUI )
   int * piKeyPolls = hb_stackKeyPolls();
#endif

   HB_TRACE(HB_TR_DEBUG, ("hb_vmExecute(%p, %p)", pCode, pSymbols));

#ifndef HB_NO_PROFILER
   if( hb_bProfiler )
      ulPastClock = ( ULONG ) clock();
#endif

   for( ;; )
   {
#ifndef HB_NO_PROFILER
      if( hb_bProfiler )
      {
         ULONG ulActualClock = ( ULONG ) clock();

         hb_ulOpcodesTime[ ulLastOpcode ] += ( ulActualClock - ulPastClock );
         ulPastClock = ulActualClock;
         ulLastOpcode = pCode[ w ];
         hb_ulOpcodesCalls[ ulLastOpcode ]++;
      }
#endif

#if !defined( HB_GUI )
      if( ! --( *piKeyPolls ) )
      {
         hb_inkeyPoll();
         *piKeyPolls = 65536;

         /* IMHO we should have a _SET_ controlled by user
          * sth like:

         if( hb_stackSetStruct()->HB_SET_KEYPOLL )
         {
            hb_inkeyPoll();
            *piKeyPolls = hb_stackSetStruct()->HB_SET_KEYPOLL;
         }

         for some GTs which can work in assynchrous mode user may
         set it to 0 (or if he doesn't need any inkey poll) and
         when ALT+C/ALT+D is pressed (or any other platform dependent
         key combination) they should set proper flags in
         ActionRequest so we can serve it in main VM loop without
         performance decrease or ignore depending on
         hb_stackSetStruct()->HB_SET_CANCEL,
         hb_stackSetStruct()->HB_SET_DEBUG flags
         */
      }
#endif
#if defined( HB_MT_VM )
      if( hb_vmThreadRequest )
         hb_vmRequestTest();
#endif

      switch( pCode[ w ] )
      {
         /* Operators ( mathematical / character / misc ) */

         case HB_P_NEGATE:
            hb_vmNegate();
            w++;
            break;

         case HB_P_PLUS:
            hb_vmPlus( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            w++;
            break;

         case HB_P_PLUSEQ:
            {
               HB_ITEM_PTR pResult, pValue;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               pValue = hb_stackItemFromTop( -1 );
               hb_vmPlus( pResult, pResult, pValue );
               hb_itemCopy( pValue, pResult );
               hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
               hb_stackDec();
            }
            w++;
            break;

         case HB_P_PLUSEQPOP:
            {
               HB_ITEM_PTR pResult;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               hb_vmPlus( pResult, pResult, hb_stackItemFromTop( -1 ) );
               hb_stackPop();
               hb_stackPop();
            }
            w++;
            break;

         case HB_P_MINUS:
            hb_vmMinus( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            w++;
            break;

         case HB_P_MINUSEQ:
            {
               HB_ITEM_PTR pResult, pValue;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               pValue = hb_stackItemFromTop( -1 );
               hb_vmMinus( pResult, pResult, pValue );
               hb_itemCopy( pValue, pResult );
               hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
               hb_stackDec();
            }
            w++;
            break;

         case HB_P_MINUSEQPOP:
            {
               HB_ITEM_PTR pResult;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               hb_vmMinus( pResult, pResult, hb_stackItemFromTop( -1 ) );
               hb_stackPop();
               hb_stackPop();
            }
            w++;
            break;

         case HB_P_MULT:
            hb_vmMult( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            w++;
            break;

         case HB_P_MULTEQ:
            {
               HB_ITEM_PTR pResult, pValue;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               pValue = hb_stackItemFromTop( -1 );
               hb_vmMult( pResult, pResult, pValue );
               hb_itemCopy( pValue, pResult );
               hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
               hb_stackDec();
            }
            w++;
            break;

         case HB_P_MULTEQPOP:
            {
               HB_ITEM_PTR pResult;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               hb_vmMult( pResult, pResult, hb_stackItemFromTop( -1 ) );
               hb_stackPop();
               hb_stackPop();
            }
            w++;
            break;

         case HB_P_DIVIDE:
            hb_vmDivide( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            w++;
            break;

         case HB_P_DIVEQ:
            {
               HB_ITEM_PTR pResult, pValue;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               pValue = hb_stackItemFromTop( -1 );
               hb_vmDivide( pResult, pResult, pValue );
               hb_itemCopy( pValue, pResult );
               hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
               hb_stackDec();
            }
            w++;
            break;

         case HB_P_DIVEQPOP:
            {
               HB_ITEM_PTR pResult;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               hb_vmDivide( pResult, pResult, hb_stackItemFromTop( -1 ) );
               hb_stackPop();
               hb_stackPop();
            }
            w++;
            break;

         case HB_P_MODULUS:
            hb_vmModulus( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            w++;
            break;

         case HB_P_MODEQ:
            {
               HB_ITEM_PTR pResult, pValue;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               pValue = hb_stackItemFromTop( -1 );
               hb_vmModulus( pResult, pResult, pValue );
               hb_itemCopy( pValue, pResult );
               hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
               hb_stackDec();
            }
            w++;
            break;

         case HB_P_MODEQPOP:
            {
               HB_ITEM_PTR pResult;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               hb_vmModulus( pResult, pResult, hb_stackItemFromTop( -1 ) );
               hb_stackPop();
               hb_stackPop();
            }
            w++;
            break;

         case HB_P_POWER:
            hb_vmPower( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            w++;
            break;

         case HB_P_EXPEQ:
            {
               HB_ITEM_PTR pResult, pValue;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               pValue = hb_stackItemFromTop( -1 );
               hb_vmPower( pResult, pResult, pValue );
               hb_itemCopy( pValue, pResult );
               hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
               hb_stackDec();
            }
            w++;
            break;

         case HB_P_EXPEQPOP:
            {
               HB_ITEM_PTR pResult;
               pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
               hb_vmPower( pResult, pResult, hb_stackItemFromTop( -1 ) );
               hb_stackPop();
               hb_stackPop();
            }
            w++;
            break;

         case HB_P_INC:
            hb_vmInc( hb_stackItemFromTop( -1 ) );
            w++;
            break;

         case HB_P_INCEQ:
            {
               HB_ITEM_PTR pResult, pValue, pTemp;
               pResult = hb_stackItemFromTop( -1 );
               pValue = hb_itemUnRef( pResult );
               hb_vmInc( pValue );
               pTemp = hb_stackAllocItem();
               hb_itemCopy( pTemp, pValue );
               hb_itemMove( pResult, pTemp );
               hb_stackDec();
               w++;
            }
            break;

         case HB_P_INCEQPOP:
            hb_vmInc( hb_itemUnRef( hb_stackItemFromTop( -1 ) ) );
            hb_stackPop();
            w++;
            break;

         case HB_P_DEC:
            hb_vmDec( hb_stackItemFromTop( -1 ) );
            w++;
            break;

         case HB_P_DECEQ:
            {
               HB_ITEM_PTR pResult, pValue, pTemp;
               pResult = hb_stackItemFromTop( -1 );
               pValue = hb_itemUnRef( pResult );
               hb_vmDec( pValue );
               pTemp = hb_stackAllocItem();
               hb_itemCopy( pTemp, pValue );
               hb_itemMove( pResult, pTemp );
               hb_stackDec();
               w++;
            }
            break;

         case HB_P_DECEQPOP:
            hb_vmDec( hb_itemUnRef( hb_stackItemFromTop( -1 ) ) );
            hb_stackPop();
            w++;
            break;

         case HB_P_FUNCPTR:
            hb_vmFuncPtr();
            w++;
            break;

         /* Operators (relational) */

         case HB_P_EQUAL:
            hb_vmEqual();
            w++;
            break;

         case HB_P_EXACTLYEQUAL:
            hb_vmExactlyEqual();
            w++;
            break;

         case HB_P_NOTEQUAL:
            hb_vmNotEqual();
            w++;
            break;

         case HB_P_LESS:
            hb_vmLess();
            w++;
            break;

         case HB_P_LESSEQUAL:
            hb_vmLessEqual();
            w++;
            break;

         case HB_P_GREATER:
            hb_vmGreater();
            w++;
            break;

         case HB_P_GREATEREQUAL:
            hb_vmGreaterEqual();
            w++;
            break;

         case HB_P_INSTRING:
            hb_vmInstring();
            w++;
            break;

         case HB_P_FORTEST:
            hb_vmForTest();
            w++;
            break;

         case HB_P_ENUMSTART:
            hb_vmEnumStart( pCode[ w + 1 ], pCode[ w + 2 ] );
            w += 3;
            break;

         case HB_P_ENUMNEXT:
            hb_vmEnumNext();
            w++;
            break;

         case HB_P_ENUMPREV:
            hb_vmEnumPrev();
            w++;
            break;

         case HB_P_ENUMEND:
            hb_vmEnumEnd();
            w++;
            break;

         case HB_P_SWITCH:
            w = hb_vmSwitch( pCode, w+3, HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            break;

         /* Operators (logical) */

         case HB_P_NOT:
            hb_vmNot();
            w++;
            break;

         case HB_P_AND:
            hb_vmAnd();
            w++;
            break;

         case HB_P_OR:
            hb_vmOr();
            w++;
            break;

         /* Array */

         case HB_P_ARRAYPUSH:
            hb_vmArrayPush();
            w++;
            break;

         case HB_P_ARRAYPUSHREF:
            hb_vmArrayPushRef();
            w++;
            break;

         case HB_P_ARRAYPOP:
            hb_vmArrayPop();
            w++;
            break;

         case HB_P_ARRAYDIM:
            hb_vmArrayDim( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_ARRAYGEN:
            hb_vmArrayGen( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_HASHGEN:
            hb_vmHashGen( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         /* Object */

         case HB_P_MESSAGE:
            hb_vmPushSymbol( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         /* Database */

         case HB_P_SWAPALIAS:
            hb_vmSwapAlias();
            w++;
            break;

         /* Execution */

         case HB_P_DO:
            hb_vmDo( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_DOSHORT:
            hb_vmDo( pCode[ w + 1 ] );
            w += 2;
            break;

         case HB_P_FUNCTION:
            hb_itemSetNil( hb_stackReturnItem() );
            hb_vmDo( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            hb_stackPushReturn();
            w += 3;
            break;

         case HB_P_FUNCTIONSHORT:
            hb_itemSetNil( hb_stackReturnItem() );
            hb_vmDo( pCode[ w + 1 ] );
            hb_stackPushReturn();
            w += 2;
            break;

         case HB_P_SEND:
            hb_itemSetNil( hb_stackReturnItem() );
            hb_vmSend( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;

            /* Is This OK??? */
            if( pCode[ w ] == HB_P_POP )
               w++;
            else
               hb_stackPushReturn();
            break;

         case HB_P_SENDSHORT:
            hb_itemSetNil( hb_stackReturnItem() );
            hb_vmSend( pCode[ w + 1 ] );
            w += 2;

            if( pCode[ w ] == HB_P_POP )
               w++;
            else
               hb_stackPushReturn();
            break;

         case HB_P_PUSHOVARREF:
            hb_vmPushObjectVarRef();
            w++;
            break;

         case HB_P_LINE:
            HB_TRACE(HB_TR_INFO, ("Opcode: HB_P_LINE: %s (%i)",
                     hb_stackBaseItem()->item.asSymbol.value->szName,
                     hb_stackBaseItem()->item.asSymbol.stackstate->uiLineNo));

            hb_stackBaseItem()->item.asSymbol.stackstate->uiLineNo = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
#ifndef HB_NO_DEBUG
            if( hb_stackBaseItem()->item.asSymbol.stackstate->fDebugging )
               hb_vmDebuggerShowLine( hb_stackBaseItem()->item.asSymbol.stackstate->uiLineNo );
#endif
            w += 3;
            break;

         case HB_P_PARAMETER:
            hb_memvarNewParameter( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ), hb_stackItemFromBase( pCode[ w + 3 ] ) );
            HB_TRACE(HB_TR_INFO, ("(hb_vmPopParameter)"));
            w += 4;
            break;

         case HB_P_FRAME:
            hb_vmFrame( pCode[ w + 1 ], pCode[ w + 2 ] );
            w += 3;
            break;

         case HB_P_VFRAME:
            hb_vmVFrame( pCode[ w + 1 ], pCode[ w + 2 ] );
            w += 3;
            break;

         case HB_P_LARGEFRAME:
            hb_vmFrame( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ), pCode[ w + 3 ] );
            w += 4;
            break;

         case HB_P_LARGEVFRAME:
            hb_vmVFrame( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ), pCode[ w + 3 ] );
            w += 4;
            break;

         case HB_P_SFRAME:
            hb_vmSFrame( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_STATICS:
            hb_vmStatics( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ), HB_PCODE_MKUSHORT( &pCode[ w + 3 ] ) );
            w += 5;
            break;

         case HB_P_THREADSTATICS:
         {
            USHORT uiCount = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
            hb_vmInitThreadStatics( uiCount, &pCode[ w + 3 ] );
            w += 3 + ( ( ULONG ) uiCount << 1 );
            break;
         }

         case HB_P_RETVALUE:
            hb_vmRetValue();
            w++;
            break;

         case HB_P_LOCALNAME:
#ifndef HB_NO_DEBUG
            hb_vmLocalName( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ),
                            ( char * ) pCode + w + 3 );
#endif
            w += 3;
            while( pCode[ w++ ] ) {};
            break;

         case HB_P_STATICNAME:
#ifndef HB_NO_DEBUG
            hb_vmStaticName( pCode[ w + 1 ], HB_PCODE_MKUSHORT( &pCode[ w + 2 ] ),
                             ( char * ) pCode + w + 4 );
#endif
            w += 4;
            while( pCode[ w++ ] ) {};
            break;

         case HB_P_MODULENAME:
#ifndef HB_NO_DEBUG
            hb_vmModuleName( ( char * ) pCode + w + 1 );
#endif
            w++;
            while( pCode[ w++ ] ) {};
            break;

         case HB_P_ENDBLOCK:
            HB_TRACE(HB_TR_INFO, ("HB_P_ENDBLOCK"));
            hb_vmEndBlock();
            /* manually inlined hb_vmRequestEndProc() for some C compilers
             * which does not make such optimisation
             */
            hb_stackSetActionRequest( HB_ENDPROC_REQUESTED );
            break;

         case HB_P_ENDPROC:
            HB_TRACE(HB_TR_INFO, ("HB_P_ENDPROC"));
            /* manually inlined hb_vmRequestEndProc() for some C compilers
             * which does not make such optimisation
             */
            hb_stackSetActionRequest( HB_ENDPROC_REQUESTED );
            break;

         /* BEGIN SEQUENCE/RECOVER/ALWAYS/END SEQUENCE */

         case HB_P_SEQBLOCK:
            hb_vmSeqBlock();
            w++;
            break;

         case HB_P_SEQALWAYS:
         {
            /*
             * Create the SEQUENCE envelope
             * [ break return value ]  -2
             * [ recover envelope   ]  -1
             * [                    ] <- new recover base
             */

            PHB_ITEM pItem;

            /*
             * 1) clear the storage for value returned by BREAK statement
             */
            hb_stackAllocItem()->type = HB_IT_NIL;

            /*
             * 2) recover data
             */
            pItem = hb_stackAllocItem();
            /* mark type as NIL - it's not real item */
            pItem->type = HB_IT_NIL;
            /* store the address of RECOVER or END opcode */
            pItem->item.asRecover.recover = w + HB_PCODE_MKINT24( &pCode[ w + 1 ] );
            /* store current RECOVER base */
            pItem->item.asRecover.base = hb_stackGetRecoverBase();
            /* store current bCanRecover flag - in a case of nested sequences */
            pItem->item.asRecover.flags = HB_SEQ_DOALWAYS | ( bCanRecover ? HB_SEQ_CANRECOVER : 0 );
            /* clear new recovery state */
            pItem->item.asRecover.request = 0;

            /*
             * set new recover base
             */
            hb_stackSetRecoverBase( hb_stackTopOffset() );
            /*
             * we are now inside a valid SEQUENCE envelope
             */
            bCanRecover = TRUE;
            w += 4;
            break;
         }

         case HB_P_ALWAYSBEGIN:
            /* change the recover address to ALWAYSEND opcode */
            hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.recover =
               w + HB_PCODE_MKINT24( &pCode[ w + 1 ] );
            /* store and reset action */
            hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags |=
               hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.request;
            hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.request = 0;
            /* store RETURN value */
            if( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags & HB_ENDPROC_REQUESTED )
               hb_itemMove( hb_stackItemFromTop( HB_RECOVER_VALUE ), hb_stackReturnItem() );
            w += 4;
            break;

         case HB_P_ALWAYSEND:
         {
            USHORT uiPrevAction, uiCurrAction;

            uiPrevAction = hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags;
            uiCurrAction = hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.request;

            /* restore previous recovery base */
            bCanRecover = ( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags & HB_SEQ_CANRECOVER ) != 0;
            hb_stackSetRecoverBase( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.base );

            /* restore requested action */
            if( ( uiCurrAction | uiPrevAction ) & HB_QUIT_REQUESTED )
               hb_stackSetActionRequest( HB_QUIT_REQUESTED );
            else if( ( uiCurrAction | uiPrevAction ) & HB_BREAK_REQUESTED )
               hb_stackSetActionRequest( HB_BREAK_REQUESTED );
            else if( ( uiCurrAction | uiPrevAction ) & HB_ENDPROC_REQUESTED )
               hb_stackSetActionRequest( HB_ENDPROC_REQUESTED );
            else
               hb_stackSetActionRequest( 0 );

            /* Remove the ALWAYS envelope */
            hb_stackDec();

            /* restore RETURN value if not overloaded inside ALWAYS code */
            if( !( uiCurrAction & HB_ENDPROC_REQUESTED ) &&
                 ( uiPrevAction & HB_ENDPROC_REQUESTED ) )
               hb_stackPopReturn();
            else
               hb_stackPop();
            w++;
            break;
         }

         case HB_P_SEQBEGIN:
         {
            /*
             * Create the SEQUENCE envelope
             * [ break return value ]  -2
             * [ recover envelope   ]  -1
             * [                    ] <- new recover base
             */

            PHB_ITEM pItem;

            /*
             * 1) clear the storage for value returned by BREAK statement
             */
            hb_stackAllocItem()->type = HB_IT_NIL;

            /*
             * 2) recover data
             */
            pItem = hb_stackAllocItem();
            /* mark type as NIL - it's not real item */
            pItem->type = HB_IT_NIL;
            /* store the address of RECOVER or END opcode */
            pItem->item.asRecover.recover = w + HB_PCODE_MKINT24( &pCode[ w + 1 ] );
            /* store current RECOVER base */
            pItem->item.asRecover.base = hb_stackGetRecoverBase();
            /* store current bCanRecover flag - in a case of nested sequences */
            pItem->item.asRecover.flags = bCanRecover ? HB_SEQ_CANRECOVER : 0;
            /* clear new recovery state */
            pItem->item.asRecover.request = 0;

            /*
             * set new recover base
             */
            hb_stackSetRecoverBase( hb_stackTopOffset() );
            /*
             * we are now inside a valid SEQUENCE envelope
             */
            bCanRecover = TRUE;
            w += 4;
            break;
         }

         case HB_P_SEQEND:
            /*
             * Remove the SEQUENCE envelope
             * This is executed either at the end of sequence or as the
             * response to the break statement if there is no RECOVER clause
             */

            /*
             * 2) Restore previous recovery state
             */
            bCanRecover = ( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags & HB_SEQ_CANRECOVER ) != 0;
            hb_stackSetRecoverBase( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.base );
            hb_stackDec();

            /*
             * 1) Discard the value returned by BREAK statement - there
             * was no RECOVER clause or there was no BREAK statement
             */
            hb_stackPop();
            /*
             * skip outside of SEQUENCE structure
             */
            w += HB_PCODE_MKINT24( &pCode[ w + 1 ] );
            break;

         case HB_P_SEQRECOVER:
            /*
             * Execute the RECOVER code
             */

            /*
             * 2) Restore previous recovery state
             */
            bCanRecover = ( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags & HB_SEQ_CANRECOVER ) != 0;
            hb_stackSetRecoverBase( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.base );
            hb_stackDec();
            /*
             * 1) Leave the value returned from BREAK  - it will be popped
             * in next executed opcode
             */
            w++;
            break;

         /* Jumps */

         case HB_P_JUMPNEAR:
            w += (signed char) pCode[ w + 1 ];
            break;

         case HB_P_JUMP:
            w += HB_PCODE_MKSHORT( &pCode[ w + 1 ] );
            break;

         case HB_P_JUMPFAR:
            w += HB_PCODE_MKINT24( &pCode[ w + 1 ] );
            break;

         case HB_P_JUMPFALSENEAR:
            if( ! hb_vmPopLogical() )
               w += (signed char) pCode[ w + 1 ];
            else
               w += 2;
            break;

         case HB_P_JUMPFALSE:
            if( ! hb_vmPopLogical() )
               w += HB_PCODE_MKSHORT( &pCode[ w + 1 ] );
            else
               w += 3;
            break;

         case HB_P_JUMPFALSEFAR:
            if( ! hb_vmPopLogical() )
               w += HB_PCODE_MKINT24( &pCode[ w + 1 ] );
            else
               w += 4;
            break;

         case HB_P_JUMPTRUENEAR:
            if( hb_vmPopLogical() )
               w += (signed char) pCode[ w + 1 ];
            else
               w += 2;
            break;

         case HB_P_JUMPTRUE:
            if( hb_vmPopLogical() )
               w += HB_PCODE_MKSHORT( &pCode[ w + 1 ] );
            else
               w += 3;
            break;

         case HB_P_JUMPTRUEFAR:
            if( hb_vmPopLogical() )
               w += HB_PCODE_MKINT24( &pCode[ w + 1 ] );
            else
               w += 4;
            break;

         /* Push */

         case HB_P_TRUE:
            {
               PHB_ITEM pItem = hb_stackAllocItem();

               pItem->type = HB_IT_LOGICAL;
               pItem->item.asLogical.value = TRUE;
               w++;
            }
            break;

         case HB_P_FALSE:
            {
               PHB_ITEM pItem = hb_stackAllocItem();

               pItem->type = HB_IT_LOGICAL;
               pItem->item.asLogical.value = FALSE;
               w++;
            }
            break;

         case HB_P_ONE:
            {
               PHB_ITEM pItem = hb_stackAllocItem();

               pItem->type = HB_IT_INTEGER;
               pItem->item.asInteger.value = 1;
               pItem->item.asInteger.length = 10;
               HB_TRACE(HB_TR_INFO, ("(HB_P_ONE)"));
               w++;
            }
            break;

         case HB_P_ZERO:
            {
               PHB_ITEM pItem = hb_stackAllocItem();

               pItem->type = HB_IT_INTEGER;
               pItem->item.asInteger.value = 0;
               pItem->item.asInteger.length = 10;
               HB_TRACE(HB_TR_INFO, ("(HB_P_ZERO)"));
               w++;
            }
            break;

         case HB_P_PUSHNIL:
            hb_stackAllocItem()->type = HB_IT_NIL;
            HB_TRACE(HB_TR_INFO, ("(HB_P_PUSHNIL)"));
            w++;
            break;

         case HB_P_PUSHBYTE:
            {
               PHB_ITEM pItem = hb_stackAllocItem();

               pItem->type = HB_IT_INTEGER;
               pItem->item.asInteger.value = ( signed char ) pCode[ w + 1 ];
               pItem->item.asInteger.length = 10;
               HB_TRACE(HB_TR_INFO, ("(HB_P_PUSHBYTE)"));
               w += 2;
            }
            break;

         case HB_P_PUSHINT:
            HB_TRACE(HB_TR_INFO, ("(HB_P_PUSHINT)"));
            hb_vmPushInteger( HB_PCODE_MKSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_PUSHLONG:
            HB_TRACE( HB_TR_DEBUG, ("(HB_P_PUSHLONG)") );
#if HB_INT_MAX >= INT32_MAX
            hb_vmPushIntegerConst( ( int ) HB_PCODE_MKLONG( &pCode[ w + 1 ] ) );
#else
            hb_vmPushLongConst( ( long ) HB_PCODE_MKLONG( &pCode[ w + 1 ] ) );
#endif
            w += 5;
            break;

         case HB_P_PUSHLONGLONG:
            HB_TRACE( HB_TR_DEBUG, ("(HB_P_PUSHLONGLONG)") );
#if !defined( HB_LONG_LONG_OFF )
            hb_vmPushLongLongConst( HB_PCODE_MKLONGLONG( &pCode[ w + 1 ] ) );
#else
            hb_vmPushDoubleConst( HB_PCODE_MKLONGLONG( &pCode[ w + 1 ] ),
                                  HB_DEFAULT_WIDTH, HB_DEFAULT_DECIMALS );
#endif
            w += 9;

            break;

         case HB_P_PUSHDOUBLE:
            hb_vmPushDoubleConst( HB_PCODE_MKDOUBLE( &pCode[ w + 1 ] ),
                                  ( int ) * ( BYTE * ) &pCode[ w + 1 + sizeof( double ) ],
                                  ( int ) * ( BYTE * ) &pCode[ w + 1 + sizeof( double ) + sizeof( BYTE ) ] );
            w += 1 + sizeof( double ) + sizeof( BYTE ) + sizeof( BYTE );
            break;

         case HB_P_PUSHSTRSHORT:
            if( bDynCode )
               hb_vmPushString( ( char * ) pCode + w + 2, ( ULONG ) pCode[ w + 1 ] - 1 );
            else
               hb_vmPushStringPcode( ( char * ) pCode + w + 2, ( ULONG ) pCode[ w + 1 ] - 1 );
            w += 2 + pCode[ w + 1 ];
            break;

         case HB_P_PUSHSTR:
         {
            USHORT uiSize = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
            if( bDynCode )
               hb_vmPushString( ( char * ) pCode + w + 3, uiSize - 1 );
            else
               hb_vmPushStringPcode( ( char * ) pCode + w + 3, uiSize - 1 );
            w += 3 + uiSize;
            break;
         }

         case HB_P_PUSHSTRLARGE:
         {
            ULONG ulSize = HB_PCODE_MKUINT24( &pCode[ w + 1 ] );
            if( bDynCode )
               hb_vmPushString( ( char * ) pCode + w + 4, ulSize - 1 );
            else
               hb_vmPushStringPcode( ( char * ) pCode + w + 4, ulSize - 1 );
            w += 4 + ulSize;
            break;
         }

         case HB_P_PUSHSTRHIDDEN:
         {
            ULONG ulSize = ( ULONG ) HB_PCODE_MKUSHORT( &pCode[ w + 2 ] );
            char * szText = hb_compDecodeString( pCode[ w + 1 ], ( char * ) pCode + w + 4, &ulSize );
            hb_itemPutCLPtr( hb_stackAllocItem(), szText, ulSize );
            w += ( 4 + ulSize );
            break;
         }

         case HB_P_PUSHDATE:
            HB_TRACE( HB_TR_DEBUG, ("(HB_P_PUSHDATE)") );
            hb_vmPushDate( ( long ) HB_PCODE_MKLONG( &pCode[ w + 1 ] ) );
            w += 5;
            break;

         case HB_P_PUSHBLOCK:
         {
            /* +0    -> _pushblock
             * +1 +2 -> size of codeblock
             * +3 +4 -> number of expected parameters
             * +5 +6 -> number of referenced local variables
             * +7    -> start of table with referenced local variables
             */
            ULONG ulSize = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
            hb_vmPushBlock( ( const BYTE * ) ( pCode + w + 3 ), pSymbols, bDynCode ? ulSize - 7 : 0 );
            w += ulSize;
            break;
         }
         case HB_P_PUSHBLOCKLARGE:
         {
            /* +0       -> _pushblock
             * +1 +2 +3 -> size of codeblock
             * +4 +5    -> number of expected parameters
             * +6 +7    -> number of referenced local variables
             * +8       -> start of table with referenced local variables
             */
            ULONG ulSize = HB_PCODE_MKUINT24( &pCode[ w + 1 ] );
            hb_vmPushBlock( ( const BYTE * ) ( pCode + w + 4 ), pSymbols, bDynCode ? ulSize - 8 : 0 );
            w += ulSize;
            break;
         }
         case HB_P_PUSHBLOCKSHORT:
         {
            /* +0    -> _pushblock
             * +1    -> size of codeblock
             */
            ULONG ulSize = pCode[ w + 1 ];
            hb_vmPushBlockShort( ( const BYTE * ) ( pCode + w + 2 ), pSymbols, bDynCode ? ulSize - 2 : 0 );
            w += ulSize;
            break;
         }

         case HB_P_PUSHSELF:
            hb_vmPush( hb_stackSelfItem() );
            w++;
            break;

         case HB_P_PUSHSYM:
            hb_vmPushSymbol( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_PUSHSYMNEAR:
            hb_vmPushSymbol( pSymbols + pCode[ w + 1 ] );
            w += 2;
            break;

         case HB_P_PUSHFUNCSYM:
            hb_vmPushSymbol( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            hb_stackAllocItem()->type = HB_IT_NIL;
            w += 3;
            break;

         case HB_P_PUSHALIAS:
            hb_vmPushAlias();
            w++;
            break;

         case HB_P_PUSHALIASEDFIELD:
            hb_vmPushAliasedField( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_PUSHALIASEDFIELDNEAR:
            hb_vmPushAliasedField( pSymbols + pCode[ w + 1 ] );
            w += 2;
            break;

         case HB_P_PUSHALIASEDVAR:
            hb_vmPushAliasedVar( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_PUSHFIELD:
            /* It pushes the current value of the given field onto the eval stack
             */
            hb_rddGetFieldValue( hb_stackAllocItem(), pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            HB_TRACE(HB_TR_INFO, ("(hb_vmPushField)"));
            w += 3;
            break;

         case HB_P_PUSHLOCAL:
            hb_vmPushLocal( HB_PCODE_MKSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_PUSHLOCALNEAR:
            hb_vmPushLocal( ( signed char ) pCode[ w + 1 ] );
            w += 2;  /* only first two bytes are used */
            break;

         case HB_P_PUSHLOCALREF:
            hb_vmPushLocalByRef( HB_PCODE_MKSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_PUSHSTATIC:
            hb_vmPushStatic( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_PUSHSTATICREF:
            hb_vmPushStaticByRef( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_PUSHMEMVAR:
            hb_memvarGetValue( hb_stackAllocItem(), pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            HB_TRACE(HB_TR_INFO, ("(hb_vmPushMemvar)"));
            w += 3;
            break;

         case HB_P_PUSHMEMVARREF:
            hb_memvarGetRefer( hb_stackAllocItem(), pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            HB_TRACE(HB_TR_INFO, ("(hb_vmPushMemvarRef)"));
            w += 3;
            break;

         case HB_P_PUSHVARIABLE:
            /* Push a value of variable of unknown type onto the eval stack
             */
            hb_vmPushVariable( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_DUPLICATE:
            hb_vmDuplicate();
            w++;
            break;

         case HB_P_DUPLTWO:
            hb_vmDuplTwo();
            w++;
            break;

         case HB_P_DUPLUNREF:
            hb_vmDuplUnRef();
            w++;
            break;

         case HB_P_PUSHUNREF:
            hb_vmPushUnRef();
            w++;
            break;

         case HB_P_PUSHVPARAMS:
            hb_vmPushVParams();
            w++;
            break;

         case HB_P_SWAP:
            hb_vmSwap( pCode[ w + 1 ] );
            w+=2;
            break;

         /* Pop */

         case HB_P_POP:
            hb_stackPop();
            w++;
            break;

         case HB_P_POPALIAS:
            hb_vmPopAlias();
            w++;
            break;

         case HB_P_POPALIASEDFIELD:
            hb_vmPopAliasedField( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_POPALIASEDFIELDNEAR:
            hb_vmPopAliasedField( pSymbols + pCode[ w + 1 ] );
            w += 2;
            break;

         case HB_P_POPALIASEDVAR:
            hb_vmPopAliasedVar( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_POPFIELD:
            /* Pops a value from the eval stack and uses it to set
             * a new value of the given field
             */
            hb_rddPutFieldValue( hb_stackItemFromTop(-1), pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            hb_stackPop();
            HB_TRACE(HB_TR_INFO, ("(hb_vmPopField)"));
            w += 3;
            break;

         case HB_P_POPLOCAL:
            hb_vmPopLocal( HB_PCODE_MKSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_POPLOCALNEAR:
            hb_vmPopLocal( ( signed char ) pCode[ w + 1 ] );
            w += 2;  /* only first two bytes are used */
            break;

         case HB_P_POPSTATIC:
            hb_vmPopStatic( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_POPMEMVAR:
            hb_memvarSetValue( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ),
                               hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            HB_TRACE(HB_TR_INFO, ("(hb_vmPopMemvar)"));
            w += 3;
            break;

         case HB_P_POPVARIABLE:
         {
            /*
               2004-03-19 Ron Pinkas
               Test with Clipper shows that for assignment, MEMVAR context
               is always used even if MEMVAR does NOT exists, and a FIELD
               with this name exists!!!
               Here is the Test Ueed - Clipper produced NO R/T Error -
               indicating MEMVAR was created.
                 PROCEDURE Main()
                    USE Test
                    First := First
                    CLOSE
                    ? First
                 RETURN
            */
#if 0
            /* Pops a value from the eval stack and uses it to set
             * a new value of a variable of unknown type.
             */
            PHB_SYMB pSymbol = pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );

            if( pSymbol->pDynSym && hb_dynsymGetMemvar( pSymbol->pDynSym ) )
               /* If exist a memory symbol with this name use it */
               hb_memvarSetValue( pSymbol, hb_stackItemFromTop(-1) );
            else if( hb_rddFieldPut( hb_stackItemFromTop(-1), pSymbol ) == FAILURE )
               /* Try with a field and after create a memvar */
               hb_memvarSetValue( pSymbol, hb_stackItemFromTop(-1) );
#else
            hb_memvarSetValue( pSymbols + HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ),
                               hb_stackItemFromTop( -1 ) );
#endif
            hb_stackPop();
            HB_TRACE(HB_TR_INFO, ("(hb_vmPopVariable)"));
            w += 3;
            break;
         }

         /* macro creation */

         case HB_P_MACROPOP:
            /* compile and run - pop a value from the stack */
            hb_macroSetValue( hb_stackItemFromTop( -1 ), pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPOPALIASED:
            /* compile and run - pop an aliased variable from the stack */
            hb_macroPopAliasedValue( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPUSH:
            /* compile and run - leave the result on the stack */
            /* the topmost element on the stack contains a macro
             * string for compilation
             */
            hb_macroGetValue( hb_stackItemFromTop( -1 ), 0, pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPUSHLIST:
            /* compile and run - leave the result on the stack */
            /* the topmost element on the stack contains a macro
             * string for compilation
             */
            hb_macroGetValue( hb_stackItemFromTop( -1 ), HB_P_MACROPUSHLIST, pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPUSHINDEX:
            hb_vmMacroPushIndex();
            w++;
            break;

         case HB_P_MACROARRAYGEN:
            hb_vmMacroArrayGen( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_MACRODO:
            hb_vmMacroDo( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_MACROFUNC:
            hb_vmMacroFunc( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_MACROSEND:
            hb_vmMacroSend( HB_PCODE_MKUSHORT( &pCode[ w + 1 ] ) );
            w += 3;
            break;

         case HB_P_MACROPUSHPARE:
            /* compile and run - leave the result on the stack */
            /* the topmost element on the stack contains a macro
             * string for compilation
             */
            hb_macroGetValue( hb_stackItemFromTop( -1 ), HB_P_MACROPUSHPARE, pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPUSHALIASED:
            /* compile and run - leave an aliased variable on the stack */
            hb_macroPushAliasedValue( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPUSHREF:
            {
               PHB_ITEM pMacro = hb_stackItemFromTop( -1 );
               PHB_SYMB pSym;
               /* compile into a symbol name (used in function calls) */
               hb_macroPushSymbol( pMacro );
               /* NOTE: pMacro string is replaced with a symbol.
                * Symbol is created if it doesn't exist.
                */
               if( hb_stackGetActionRequest() == 0 )
               {
                  pSym = pMacro->item.asSymbol.value;
                  /* NOTE: pMacro item of symbol type is replaced with
                   *  the reference
                   */
                  hb_memvarGetRefer( pMacro, pSym );
               }
               w++;
            }
            break;

         case HB_P_MACROSYMBOL:
            /* compile into a symbol name (used in function calls) */
            hb_macroPushSymbol( hb_stackItemFromTop( -1 ) );
            w++;
            break;

         case HB_P_MACROTEXT:
            /* macro text substitution
             * "text &macro.other text"
             */
            hb_macroTextValue( hb_stackItemFromTop( -1 ) );
            w++;
            break;

         /* macro compiled opcodes - we are using symbol address here */

         case HB_P_MMESSAGE:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_vmPushSymbol( pDynSym->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPOPALIASEDFIELD:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_vmPopAliasedField( pDynSym->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPOPALIASEDVAR:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_vmPopAliasedVar( pDynSym->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPOPFIELD:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            /* Pops a value from the eval stack and uses it to set
            * a new value of the given field
            */
            hb_rddPutFieldValue( ( hb_stackItemFromTop(-1) ), pDynSym->pSymbol );
            hb_stackPop();
            HB_TRACE(HB_TR_INFO, ("(hb_vmMPopField)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPOPMEMVAR:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_memvarSetValue( pDynSym->pSymbol, hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            HB_TRACE(HB_TR_INFO, ("(hb_vmMPopMemvar)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHALIASEDFIELD:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_vmPushAliasedField( pDynSym->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHALIASEDVAR:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_vmPushAliasedVar( pDynSym->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHBLOCK:
         {
            /*NOTE: the pcode is stored in dynamically allocated memory
            * We need to handle it with more care than compile-time
            * codeblocks
            */
            /* +0    -> _pushblock
             * +1 +2 -> size of codeblock
             * +3 +4 -> number of expected parameters
             * +5    -> pcode bytes
             */
            ULONG ulSize = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
            hb_vmPushMacroBlock( ( BYTE * ) ( pCode + w + 5 ), ulSize - 5,
                                 HB_PCODE_MKUSHORT( &pCode[ w + 3 ] ) );
            w += ulSize;
            break;
         }

         case HB_P_MPUSHBLOCKLARGE:
         {
            /*NOTE: the pcode is stored in dynamically allocated memory
            * We need to handle it with more care than compile-time
            * codeblocks
            */
            /* +0       -> _pushblock
             * +1 +2 +3 -> size of codeblock
             * +4 +5    -> number of expected parameters
             * +6       -> pcode bytes
             */
            ULONG ulSize = HB_PCODE_MKUINT24( &pCode[ w + 1 ] );
            hb_vmPushMacroBlock( ( BYTE * ) ( pCode + w + 6 ), ulSize - 6,
                                 HB_PCODE_MKUSHORT( &pCode[ w + 4 ] ) );
            w += ulSize;
            break;
         }

         case HB_P_MPUSHFIELD:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            /* It pushes the current value of the given field onto the eval stack
            */
            hb_rddGetFieldValue( hb_stackAllocItem(), pDynSym->pSymbol );
            HB_TRACE(HB_TR_INFO, ("(hb_vmMPushField)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHMEMVAR:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_memvarGetValue( hb_stackAllocItem(), pDynSym->pSymbol );
            HB_TRACE(HB_TR_INFO, ("(hb_vmMPushMemvar)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHMEMVARREF:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_memvarGetRefer( hb_stackAllocItem(), pDynSym->pSymbol );
            HB_TRACE(HB_TR_INFO, ("(hb_vmMPushMemvarRef)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHSYM:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_vmPushSymbol( pDynSym->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHVARIABLE:
         {
            HB_DYNS_PTR pDynSym = ( HB_DYNS_PTR ) HB_GET_PTR( pCode + w + 1 );
            hb_vmPushVariable( pDynSym->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHSTR:
         {
            USHORT uiSize = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );

            hb_vmPushString( ( char * ) ( pCode + w + 3 ), uiSize - 1 );
            w += 3 + uiSize;
            break;
         }

         case HB_P_MPUSHSTRLARGE:
         {
            ULONG ulSize = HB_PCODE_MKUINT24( &pCode[ w + 1 ] );

            hb_vmPushString( ( char * ) ( pCode + w + 3 ), ulSize - 1 );
            w += 4 + ulSize;
            break;
         }

         case HB_P_LOCALNEARADDINT:
         {
            int iLocal = pCode[ w + 1 ];
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LOCALNEARADDINT") );

            hb_vmAddInt( hb_stackLocalVariable( &iLocal ),
                         HB_PCODE_MKSHORT( &pCode[ w + 2 ] ) );
            w += 4;
            break;
         }

         case HB_P_LOCALADDINT:
         {
            int iLocal = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LOCALADDINT") );

            hb_vmAddInt( hb_stackLocalVariable( &iLocal ),
                         HB_PCODE_MKSHORT( &pCode[ w + 3 ] ) );
            w += 5;
            break;
         }

         case HB_P_LOCALINC:
         {
            int iLocal = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
            PHB_ITEM pLocal = hb_stackLocalVariable( &iLocal );
            hb_vmInc( HB_IS_BYREF( pLocal ) ? hb_itemUnRef( pLocal ) : pLocal );
            w += 3;
            break;
         }

         case HB_P_LOCALDEC:
         {
            int iLocal = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
            PHB_ITEM pLocal = hb_stackLocalVariable( &iLocal );
            hb_vmDec( HB_IS_BYREF( pLocal ) ? hb_itemUnRef( pLocal ) : pLocal );
            w += 3;
            break;
         }

         case HB_P_LOCALINCPUSH:
         {
            int iLocal = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
            PHB_ITEM pLocal = hb_stackLocalVariable( &iLocal );
            if( HB_IS_BYREF( pLocal ) )
               pLocal = hb_itemUnRef( pLocal );
            hb_vmInc( pLocal );
            hb_itemCopy( hb_stackAllocItem(), pLocal );
            w += 3;
            break;
         }

         /* WITH OBJECT */

         case HB_P_WITHOBJECTMESSAGE:
         {
            USHORT wSymPos = HB_PCODE_MKUSHORT( &pCode[ w + 1 ] );
            if( wSymPos != 0xFFFF )
            {
               /* NOTE: 0xFFFF is passed when ':&varmacro' syntax is used.
                * In this case symbol is already pushed on the stack
                * using HB_P_MACROSYMBOL.
               */
               hb_vmPushSymbol( pSymbols + wSymPos );
            }
            hb_vmPush( hb_stackWithObjectItem() );
            w += 3;
            break;
         }

         case HB_P_WITHOBJECTSTART:
            hb_vmWithObjectStart();
            w++;
            break;

         case HB_P_WITHOBJECTEND:
            hb_stackPop();    /* remove with object envelope */
            hb_stackPop();    /* remove implicit object */
            w += 1;
            break;

         /* misc */

         case HB_P_NOOP:
            /* Intentionally do nothing */
            w++;
            break;

         default:
            /* TODO: Include to failing pcode in the error message */
            hb_errInternal( HB_EI_VMBADOPCODE, NULL, NULL, NULL );
            break;
      }

      if( hb_stackGetActionRequest() )
      {
         if( hb_stackGetActionRequest() & HB_ENDPROC_REQUESTED )
         {
            /* request to stop current procedure was issued
             * (from macro evaluation)
             */

            /* This code allow to use RETURN inside BEGIN/END sequence
             * or in RECOVER code when ALWAYS clause is used
             */
            if( bCanRecover )
            {
               do
               {
                  hb_stackRemove( hb_stackGetRecoverBase() );
                  if( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags & HB_SEQ_DOALWAYS )
                     break;
                  /* Restore previous recovery state */
                  bCanRecover = ( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags & HB_SEQ_CANRECOVER ) != 0;
                  hb_stackSetRecoverBase( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.base );
               }
               while( bCanRecover );

               /* ALWAYS found? */
               if( bCanRecover )
               {
                  /* reload the address of ALWAYS code */
                  w = hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.recover;
                  /* store and reset action */
                  hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.request = hb_stackGetActionRequest();
                  hb_stackSetActionRequest( 0 );
                  continue;
               }
            }
            hb_stackSetActionRequest( 0 );
            break;
         }
         else if( hb_stackGetActionRequest() & HB_BREAK_REQUESTED )
         {
            if( bCanRecover )
            {
               /*
                * There is the BEGIN/END sequence defined in current
                * procedure/function - use it to continue opcodes execution
                */
               /*
                * remove all items placed on the stack after BEGIN code
                */
               hb_stackRemove( hb_stackGetRecoverBase() );
               /*
                * reload the address of recovery code
                */
               w = hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.recover;
               /*
                * leave the SEQUENCE envelope on the stack - it will
                * be popped either in RECOVER or END opcode
                */

               /* store and reset action */
               hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.request = hb_stackGetActionRequest();
               hb_stackSetActionRequest( 0 );
            }
            else
               break;
         }
         else if( hb_stackGetActionRequest() & HB_QUIT_REQUESTED )
         {
            if( bCanRecover )
            {
               do
               {
                  hb_stackRemove( hb_stackGetRecoverBase() );
                  if( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags & HB_SEQ_DOALWAYS )
                     break;
                  /* Restore previous recovery state */
                  bCanRecover = ( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.flags & HB_SEQ_CANRECOVER ) != 0;
                  hb_stackSetRecoverBase( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.base );
                  /* skip other steps */
               }
               while( bCanRecover );

               /* ALWAYS found? */
               if( bCanRecover )
               {
                  /* reload the address of ALWAYS code */
                  w = hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.recover;
                  /* store and reset action */
                  hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.request = hb_stackGetActionRequest();
                  hb_stackSetActionRequest( 0 );
                  continue;
               }
            }
            break;
         }
      }
   }
}

/* ------------------------------- */
/* Operators ( mathematical        */
/*             character / misc )  */
/* ------------------------------- */

static void hb_vmAddInt( HB_ITEM_PTR pResult, LONG lAdd )
{
   double dNewVal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmAddInt(%p,%ld)", pResult, lAdd));

   if( HB_IS_BYREF( pResult ) )
   {
      pResult = hb_itemUnRef( pResult );
   }

   if( HB_IS_NUMINT( pResult ) )
   {
      HB_LONG lVal = HB_ITEM_GET_NUMINTRAW( pResult ), lResult;

      lResult = lVal + lAdd;

      if( lAdd >= 0 ? lResult >= lVal : lResult <  lVal )
      {
         HB_ITEM_PUT_NUMINTRAW( pResult, lResult );
         return;
      }
      else
      {
         dNewVal = ( double ) lVal + lAdd;
      }
   }
   else if( HB_IS_DATE( pResult ) )
   {
      pResult->item.asDate.value += lAdd;
      return;
   }
   else if( HB_IS_DOUBLE( pResult ) )
   {
      dNewVal = pResult->item.asDouble.value + lAdd;
   }
   else if( hb_objHasOperator( pResult, HB_OO_OP_PLUS ) )
   {
      HB_STACK_TLS_PRELOAD
      hb_vmPushLong( lAdd );
      hb_objOperatorCall( HB_OO_OP_PLUS, pResult, pResult, hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
      return;
   }
   else
   {
      HB_STACK_TLS_PRELOAD
      PHB_ITEM pSubst;

      hb_vmPushLong( lAdd );
      pSubst = hb_errRT_BASE_Subst( EG_ARG, 1081, NULL, "+", 2, pResult, hb_stackItemFromTop( -1 ) );
      if( pSubst )
      {
         hb_stackPop();
         hb_itemMove( pResult, pSubst );
         hb_itemRelease( pSubst );
      }
      return;
   }

   if( !HB_IS_DOUBLE( pResult ) )
   {
      pResult->type = HB_IT_DOUBLE;
      pResult->item.asDouble.decimal = 0;
   }

   pResult->item.asDouble.value = dNewVal;
   pResult->item.asDouble.length = HB_DBL_LENGTH( dNewVal );
}

/* NOTE: Clipper is resetting the number width on a negate. */

static void hb_vmNegate( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmNegate()"));

   pItem = hb_stackItemFromTop( -1 );

   if( HB_IS_INTEGER( pItem ) )
   {
#if -HB_INT_MAX > HB_INT_MIN
      if( pItem->item.asInteger.value < -HB_INT_MAX )
      {
#if HB_LONG_MAX > HB_INT_MAX
         HB_LONG lValue = ( HB_LONG ) pItem->item.asInteger.value;
         pItem->type = HB_IT_LONG;
         pItem->item.asLong.value = -lValue;
         pItem->item.asLong.length = HB_LONG_EXPLENGTH( -lValue );
#else
         double dValue = ( double ) pItem->item.asInteger.value;
         pItem->type = HB_IT_DOUBLE;
         pItem->item.asDouble.value = -dValue;
         pItem->item.asDouble.length = HB_DBL_LENGTH( -dValue );
#endif
      }
      else
#endif
      {
         pItem->type = HB_IT_INTEGER;
         pItem->item.asInteger.value = -pItem->item.asInteger.value;
         pItem->item.asInteger.length = HB_INT_EXPLENGTH( pItem->item.asInteger.value );
      }
   }
   else if( HB_IS_LONG( pItem ) )
   {
#if -HB_LONG_MAX > HB_LONG_MIN
      if( pItem->item.asLong.value < -HB_LONG_MAX )
      {
         double dValue = ( double ) pItem->item.asLong.value;
         pItem->type = HB_IT_DOUBLE;
         pItem->item.asDouble.value = -dValue;
         pItem->item.asDouble.length = HB_DBL_LENGTH( -dValue );
      }
      else
#endif
      {
         pItem->type = HB_IT_LONG;
         pItem->item.asLong.value = -pItem->item.asLong.value;
         pItem->item.asLong.length = HB_LONG_EXPLENGTH( pItem->item.asLong.value );
      }
   }
   else if( HB_IS_DOUBLE( pItem ) )
   {
      pItem->type = HB_IT_DOUBLE;
      pItem->item.asDouble.value = -pItem->item.asDouble.value;
      pItem->item.asDouble.length = HB_DBL_LENGTH( pItem->item.asDouble.value );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1080, NULL, "-", 1, pItem );

      if( pResult )
      {
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmPlus( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPlus(%p,%p,%p)", pResult, pItem1, pItem2));

   if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      HB_LONG lNumber1 = HB_ITEM_GET_NUMINTRAW( pItem1 );
      HB_LONG lNumber2 = HB_ITEM_GET_NUMINTRAW( pItem2 );
      HB_LONG lResult = lNumber1 + lNumber2;

      if( lNumber2 >= 0 ? lResult >= lNumber1 : lResult < lNumber1 )
      {
         HB_ITEM_PUT_NUMINTRAW( pResult, lResult );
      }
      else
      {
         hb_itemPutND( pResult, ( double ) lNumber1 + ( double ) lNumber2 );
      }
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      int iDec1, iDec2;
      double dNumber1 = hb_itemGetNDDec( pItem1, &iDec1 );
      double dNumber2 = hb_itemGetNDDec( pItem2, &iDec2 );

      hb_itemPutNumType( pResult, dNumber1 + dNumber2, HB_MAX( iDec1, iDec2 ),
                         HB_ITEM_TYPERAW( pItem1 ), HB_ITEM_TYPERAW( pItem2 ) );
   }
   else if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      ULONG ulLen1 = pItem1->item.asString.length;
      ULONG ulLen2 = pItem2->item.asString.length;

      if( ulLen2 )
      {
         if( ulLen1 )
         {
            if( ulLen1 < ULONG_MAX - ulLen2 )
            {
               if( pResult != pItem1 )
               {
                  hb_itemMove( pResult, pItem1 );
                  pItem1 = pResult;
               }
               hb_itemReSizeString( pItem1, ulLen1 + ulLen2 );
               hb_xmemcpy( pItem1->item.asString.value + ulLen1,
                           pItem2->item.asString.value, ulLen2 );
            }
            else
               hb_errRT_BASE( EG_STROVERFLOW, 1209, NULL, "+", 2, pItem1, pItem2 );
         }
         else
            hb_itemCopy( pResult, pItem2 );
      }
      else if( pResult != pItem1 )
         hb_itemCopy( pResult, pItem1 );
      pResult->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );
   }
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      /* NOTE: This is not a bug. CA-Cl*pper does exactly that. */
      hb_itemPutDL( pResult, hb_itemGetNL( pItem1 ) + hb_itemGetNL( pItem2 ) );
   }
   else if( HB_IS_DATE( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      hb_itemPutDL( pResult, hb_itemGetDL( pItem1 ) + hb_itemGetNL( pItem2 ) );
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      hb_itemPutDL( pResult, hb_itemGetNL( pItem1 ) + hb_itemGetDL( pItem2 ) );
   }
   else if( ! hb_objOperatorCall( HB_OO_OP_PLUS, pResult, pItem1, pItem2, NULL ) )
   {
      PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ARG, 1081, NULL, "+", 2, pItem1, pItem2 );

      if( pSubst )
      {
         hb_itemMove( pResult, pSubst );
         hb_itemRelease( pSubst );
      }
   }
}

static void hb_vmMinus( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmMinus(%p,%p,%p)", pResult, pItem1, pItem2));

   if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      HB_LONG lNumber1 = HB_ITEM_GET_NUMINTRAW( pItem1 );
      HB_LONG lNumber2 = HB_ITEM_GET_NUMINTRAW( pItem2 );
      HB_LONG lResult = lNumber1 - lNumber2;

      if( lNumber2 <= 0 ? lResult >= lNumber1 : lResult < lNumber1 )
      {
         HB_ITEM_PUT_NUMINTRAW( pResult, lResult );
      }
      else
      {
         hb_itemPutND( pResult, ( double ) lNumber1 - ( double ) lNumber2 );
      }
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      int iDec1, iDec2;
      double dNumber1 = hb_itemGetNDDec( pItem1, &iDec1 );
      double dNumber2 = hb_itemGetNDDec( pItem2, &iDec2 );

      hb_itemPutNumType( pResult, dNumber1 - dNumber2, HB_MAX( iDec1, iDec2 ),
                         HB_ITEM_TYPERAW( pItem1 ), HB_ITEM_TYPERAW( pItem2 ) );
   }
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      HB_LONG lResult = hb_itemGetDL( pItem1 ) - hb_itemGetDL( pItem2 );

      HB_ITEM_PUT_NUMINTRAW( pResult, lResult );
   }
   else if( HB_IS_DATE( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      hb_itemPutDL( pResult, hb_itemGetDL( pItem1 ) - hb_itemGetNL( pItem2 ) );
   }
   else if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      ULONG ulLen1 = pItem1->item.asString.length;
      ULONG ulLen2 = pItem2->item.asString.length;

      if( ulLen1 == 0 )
      {
         hb_itemCopy( pResult, pItem2 );
         pResult->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );
      }
      else if( ulLen2 == 0 )
      {
         if( pResult != pItem1 )
            hb_itemCopy( pResult, pItem1 );
         pResult->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );
      }
      else if( ulLen1 < ULONG_MAX - ulLen2 )
      {
         if( pResult != pItem1 )
         {
            hb_itemMove( pResult, pItem1 );
            pItem1 = pResult;
         }
         hb_itemReSizeString( pItem1, ulLen1 + ulLen2 );
         while( ulLen1 && pItem1->item.asString.value[ ulLen1 - 1 ] == ' ' )
            ulLen1--;
         hb_xmemcpy( pItem1->item.asString.value + ulLen1,
                     pItem2->item.asString.value, ulLen2 );
         hb_xmemset( pItem1->item.asString.value + ulLen1 + ulLen2, ' ',
                     pItem1->item.asString.length - ulLen1 - ulLen2 );
      }
      else
         hb_errRT_BASE( EG_STROVERFLOW, 1210, NULL, "-", 2, pItem1, pItem2 );
   }
   else if( ! hb_objOperatorCall( HB_OO_OP_MINUS, pResult, pItem1, pItem2, NULL ) )
   {
      PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ARG, 1082, NULL, "-", 2, pItem1, pItem2 );

      if( pSubst )
      {
         hb_itemMove( pResult, pSubst );
         hb_itemRelease( pSubst );
      }
   }
}

static void hb_vmMult( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmMult(%p,%p,%p)", pResult, pItem1, pItem2));

   /* if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      HB_LONG lNumber1 = HB_ITEM_GET_NUMINTRAW( pItem1 )
      HB_LONG lNumber2 = HB_ITEM_GET_NUMINTRAW( pItem2 );
      HB_LONG lResult = lNumber1 * lNumber2;
      if( lNumber2 == 0 || lResult / lNumber2 == lNumber1 )
         HB_ITEM_PUT_NUMINTRAW( pResult, lResult );
      else
         hb_itemPutNLen( pResult, ( double ) lNumber1 * lNumber2, 0, 0 );
   }
   else */ if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      int iDec1, iDec2;
      double dNumber1 = hb_itemGetNDDec( pItem1, &iDec1 );
      double dNumber2 = hb_itemGetNDDec( pItem2, &iDec2 );

      hb_itemPutNumType( pResult, dNumber1 * dNumber2, iDec1 + iDec2,
                         HB_ITEM_TYPERAW( pItem1 ), HB_ITEM_TYPERAW( pItem2 ) );
   }

   else if( ! hb_objOperatorCall( HB_OO_OP_MULT, pResult, pItem1, pItem2, NULL ) )
   {
      PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ARG, 1083, NULL, "*", 2, pItem1, pItem2 );

      if( pSubst )
      {
         hb_itemMove( pResult, pSubst );
         hb_itemRelease( pSubst );
      }
   }
}

static void hb_vmDivide( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmDivide(%p,%p,%p)", pResult, pItem1, pItem2));

   if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      HB_LONG lDivisor = HB_ITEM_GET_NUMINTRAW( pItem2 );

      if( lDivisor == 0 )
      {
         PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ZERODIV, 1340, NULL, "/", 2, pItem1, pItem2 );

         if( pSubst )
         {
            hb_itemMove( pResult, pSubst );
            hb_itemRelease( pSubst );
         }
      }
      else
      {
         HB_LONG lNumber1 = HB_ITEM_GET_NUMINTRAW( pItem1 );
         hb_itemPutND( pResult, ( double ) lNumber1 / ( double ) lDivisor );
      }
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      double dDivisor = hb_itemGetND( pItem2 );

      if( dDivisor == 0.0 )
      {
         PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ZERODIV, 1340, NULL, "/", 2, pItem1, pItem2 );

         if( pSubst )
         {
            hb_itemMove( pResult, pSubst );
            hb_itemRelease( pSubst );
         }
      }
      else
      {
         /* If all both operand was integer and the result is an integer, too,
            push the number without decimals. Clipper compatible. Actually,
            this is not Clipper compatible. The only time Clipper returns 0
            decimal places is for compiler optimized integer division with an
            integer result. Therefore this code is not needed and has been
            removed - David G. Holm <dholm@jsd-llc.com>
         */
         hb_itemPutND( pResult, hb_itemGetND( pItem1 ) / dDivisor );
      }
   }
   else if( ! hb_objOperatorCall( HB_OO_OP_DIVIDE, pResult, pItem1, pItem2, NULL ) )
   {
      PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ARG, 1084, NULL, "/", 2, pItem1, pItem2 );

      if( pSubst )
      {
         hb_itemMove( pResult, pSubst );
         hb_itemRelease( pSubst );
      }
   }
}

static void hb_vmModulus( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmModulus(%p,%p,%p)", pResult, pItem1, pItem2));

   if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      HB_LONG lDivisor = HB_ITEM_GET_NUMINTRAW( pItem2 );

      if( lDivisor == 0 )
      {
         PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ZERODIV, 1341, NULL, "%", 2, pItem1, pItem2 );

         if( pSubst )
         {
            hb_itemMove( pResult, pSubst );
            hb_itemRelease( pSubst );
         }
      }
      else
      {
         /* NOTE: Clipper always returns the result of modulus
                  with the SET number of decimal places. */
         hb_itemPutND( pResult, ( double ) ( HB_ITEM_GET_NUMINTRAW( pItem1 ) % lDivisor ) );
      }
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      double dDivisor = hb_itemGetND( pItem2 );

      if( dDivisor == 0.0 )
      {
         PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ZERODIV, 1341, NULL, "%", 2, pItem1, pItem2 );

         if( pSubst )
         {
            hb_itemMove( pResult, pSubst );
            hb_itemRelease( pSubst );
         }
      }
      else
      {
         /* NOTE: Clipper always returns the result of modulus
                  with the SET number of decimal places. */
         hb_itemPutND( pResult, fmod( hb_itemGetND( pItem1 ), dDivisor ) );
      }
   }
   else if( ! hb_objOperatorCall( HB_OO_OP_MOD, pResult, pItem1, pItem2, NULL ) )
   {
      PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ARG, 1085, NULL, "%", 2, pItem1, pItem2 );

      if( pSubst )
      {
         hb_itemMove( pResult, pSubst );
         hb_itemRelease( pSubst );
      }
   }
}

static void hb_vmPower( HB_ITEM_PTR pResult, HB_ITEM_PTR pItem1, HB_ITEM_PTR pItem2 )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPower(%p,%p,%p)", pResult, pItem1, pItem2));

   if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      /* NOTE: Clipper always returns the result of power
               with the SET number of decimal places. */
      hb_itemPutND( pResult, pow( hb_itemGetND( pItem1 ), hb_itemGetND( pItem2 ) ) );
   }
   else if( ! hb_objOperatorCall( HB_OO_OP_POWER, pResult, pItem1, pItem2, NULL ) )
   {
      PHB_ITEM pSubst = hb_errRT_BASE_Subst( EG_ARG, 1088, NULL, "^", 2, pItem1, pItem2 );

      if( pSubst )
      {
         hb_itemMove( pResult, pSubst );
         hb_itemRelease( pSubst );
      }
   }
}

static void hb_vmInc( PHB_ITEM pItem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmInc(%p)", pItem));

   if( HB_IS_NUMINT( pItem ) )
   {
      if( HB_IS_INTEGER( pItem ) )
      {
         if( pItem->item.asInteger.value < HB_INT_MAX )
         {
            pItem->type = HB_IT_INTEGER;
            pItem->item.asInteger.value++;
            pItem->item.asInteger.length = HB_INT_EXPLENGTH( pItem->item.asInteger.value );
         }
         else
         {
#if HB_INT_MAX < HB_LONG_MAX
            pItem->type = HB_IT_LONG;
            pItem->item.asLong.value = ( HB_LONG ) pItem->item.asInteger.value + 1;
            pItem->item.asLong.length = HB_LONG_EXPLENGTH( pItem->item.asLong.value );
#else
            pItem->type = HB_IT_DOUBLE;
            pItem->item.asDouble.value = ( double ) pItem->item.asInteger.value + 1;
            pItem->item.asDouble.length = HB_DBL_LENGTH( pItem->item.asDouble.value );
            pItem->item.asDouble.decimal = 0;
#endif
         }
      }
      else if( pItem->item.asLong.value < HB_LONG_MAX )
      {
         pItem->type = HB_IT_LONG;
         pItem->item.asLong.value++;
         pItem->item.asLong.length = HB_LONG_EXPLENGTH( pItem->item.asLong.value );
      }
      else
      {
         pItem->type = HB_IT_DOUBLE;
         pItem->item.asDouble.value = ( double ) pItem->item.asLong.value + 1;
         pItem->item.asDouble.length = HB_DBL_LENGTH( pItem->item.asDouble.value );
         pItem->item.asDouble.decimal = 0;
      }
   }
   else if( HB_IS_DOUBLE( pItem ) )
   {
      pItem->type = HB_IT_DOUBLE;
      pItem->item.asDouble.value++;
      pItem->item.asDouble.length = HB_DBL_LENGTH( pItem->item.asDouble.value );
   }
   else if( HB_IS_DATE( pItem ) )
   {
      pItem->type = HB_IT_DATE;
      pItem->item.asDate.value++;
   }
   else if( ! hb_objOperatorCall( HB_OO_OP_INC, pItem, pItem, NULL, NULL ) )
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1086, NULL, "++", 1, pItem );

      if( pResult )
      {
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmDec( PHB_ITEM pItem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmDec(%p)", pItem));

   if( HB_IS_NUMINT( pItem ) )
   {
      if( HB_IS_INTEGER( pItem ) )
      {
         if( pItem->item.asInteger.value > HB_INT_MIN )
         {
            pItem->type = HB_IT_INTEGER;
            pItem->item.asInteger.value--;
            pItem->item.asInteger.length = HB_INT_EXPLENGTH( pItem->item.asInteger.value );
         }
         else
         {
#if HB_INT_MIN > HB_LONG_MIN
            pItem->type = HB_IT_LONG;
            pItem->item.asLong.value = ( HB_LONG ) pItem->item.asInteger.value - 1;
            pItem->item.asLong.length = HB_LONG_EXPLENGTH( pItem->item.asLong.value );
#else
            pItem->type = HB_IT_DOUBLE;
            pItem->item.asDouble.value = ( double ) pItem->item.asInteger.value - 1;
            pItem->item.asDouble.length = HB_DBL_LENGTH( pItem->item.asDouble.value );
            pItem->item.asDouble.decimal = 0;
#endif
         }
      }
      else if( pItem->item.asLong.value > HB_LONG_MIN )
      {
         pItem->type = HB_IT_LONG;
         pItem->item.asLong.value--;
         pItem->item.asLong.length = HB_LONG_EXPLENGTH( pItem->item.asLong.value );
      }
      else
      {
         pItem->type = HB_IT_DOUBLE;
         pItem->item.asDouble.value = ( double ) pItem->item.asLong.value - 1;
         pItem->item.asDouble.length = HB_DBL_LENGTH( pItem->item.asDouble.value );
         pItem->item.asDouble.decimal = 0;
      }
   }
   else if( HB_IS_DOUBLE( pItem ) )
   {
      pItem->type = HB_IT_DOUBLE;
      pItem->item.asDouble.value--;
      pItem->item.asDouble.length = HB_DBL_LENGTH( pItem->item.asDouble.value );
   }
   else if( HB_IS_DATE( pItem ) )
   {
      pItem->type = HB_IT_DATE;
      pItem->item.asDate.value--;
   }
   else if( ! hb_objOperatorCall( HB_OO_OP_DEC, pItem, pItem, NULL, NULL ) )
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1087, NULL, "--", 1, pItem );

      if( pResult )
      {
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmFuncPtr( void )  /* pushes a function address pointer. Removes the symbol from the satck */
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmFuncPtr()"));

   pItem = hb_stackItemFromTop( -1 );

   if( HB_IS_SYMBOL( pItem ) )
   {
      /* do nothing - now we are using HB_IT_SYMBOL */
#if 0
      hb_stackPop();
      hb_vmPushPointer( ( void* ) pItem->item.asSymbol.value->value.pFunPtr );
#endif
   }
   else
      hb_errInternal( HB_EI_VMNOTSYMBOL, NULL, "hb_vmFuncPtr()", NULL );
}

/* ------------------------------- */
/* Operators (relational)          */
/* ------------------------------- */

static void hb_vmExactlyEqual( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmExactlyEqual()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_NIL( pItem1 ) )
   {
      /* pItem1 is NIL so this is safe */
      pItem1->type = HB_IT_LOGICAL;
      pItem1->item.asLogical.value = HB_IS_NIL( pItem2 );
      hb_stackPop();    /* clear the pItem2 */
   }
   else if( HB_IS_NIL( pItem2 ) )
   {
      hb_stackDec();    /* pItem2 is already NIL */
      hb_stackPop();    /* clear the pItem1 */
      hb_vmPushLogical( FALSE );
   }
   else if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      BOOL fResult = pItem1->item.asString.length == pItem2->item.asString.length &&
                     memcmp( pItem1->item.asString.value,
                             pItem2->item.asString.value,
                             pItem1->item.asString.length ) == 0;
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( HB_ITEM_GET_NUMINTRAW( pItem1 ) ==
                                       HB_ITEM_GET_NUMINTRAW( pItem2 ) );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
      hb_vmPushLogical( hb_vmPopNumber() == hb_vmPopNumber() );
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( pItem1->item.asDate.value ==
                                       pItem2->item.asDate.value );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
      hb_vmPushLogical( hb_vmPopLogical() == hb_vmPopLogical() );
   else if( HB_IS_POINTER( pItem1 ) && HB_IS_POINTER( pItem2 ) )
   {
      BOOL fResult = pItem1->item.asPointer.value == pItem2->item.asPointer.value;

      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( HB_IS_HASH( pItem1 ) && HB_IS_HASH( pItem2 ) )
   {
      BOOL fResult = pItem1->item.asHash.value == pItem2->item.asHash.value;

      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( HB_IS_ARRAY( pItem1 ) && HB_IS_ARRAY( pItem2 ) &&
            ! hb_objHasOperator( pItem1, HB_OO_OP_EXACTEQUAL ) )
   {
      BOOL fResult = pItem1->item.asArray.value == pItem2->item.asArray.value;

      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( hb_objOperatorCall( HB_OO_OP_EXACTEQUAL, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1070, NULL, "==", 2, pItem1, pItem2 );
      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmEqual( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmEqual()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_NIL( pItem1 ) )
   {
      /* pItem1 is NIL so this is safe */
      pItem1->type = HB_IT_LOGICAL;
      pItem1->item.asLogical.value = HB_IS_NIL( pItem2 );
      hb_stackPop();    /* clear the pItem2 */
   }
   else if( HB_IS_NIL( pItem2 ) )
   {
      hb_stackDec();    /* pItem2 is already NIL */
      hb_stackPop();    /* clear the pItem1 */
      hb_vmPushLogical( FALSE );
   }
   else if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      BOOL fResult = hb_itemStrCmp( pItem1, pItem2, FALSE ) == 0;
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( HB_ITEM_GET_NUMINTRAW( pItem1 ) ==
                                       HB_ITEM_GET_NUMINTRAW( pItem2 ) );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
      hb_vmPushLogical( hb_vmPopNumber() == hb_vmPopNumber() );
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( pItem1->item.asDate.value ==
                                       pItem2->item.asDate.value );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
      hb_vmPushLogical( hb_vmPopLogical() == hb_vmPopLogical() );
   else if( HB_IS_POINTER( pItem1 ) && HB_IS_POINTER( pItem2 ) )
   {
      BOOL fResult = pItem1->item.asPointer.value == pItem2->item.asPointer.value;
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( HB_IS_HASH( pItem1 ) && HB_IS_HASH( pItem2 ) )
   {
      BOOL fResult = pItem1->item.asHash.value == pItem2->item.asHash.value;
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( hb_objOperatorCall( HB_OO_OP_EQUAL, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1071, NULL, "=", 2, pItem1, pItem2 );
      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmNotEqual( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmNotEqual()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_NIL( pItem1 ) )
   {
      /* pItem1 is NIL so this is safe */
      pItem1->type = HB_IT_LOGICAL;
      pItem1->item.asLogical.value = ! HB_IS_NIL( pItem2 );
      hb_stackPop();    /* clear the pItem2 */
   }
   else if( HB_IS_NIL( pItem2 ) )
   {
      hb_stackDec();    /* pItem2 is already NIL */
      hb_stackPop();    /* clear the pItem1 */
      hb_vmPushLogical( TRUE );
   }
   else if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      int i = hb_itemStrCmp( pItem1, pItem2, FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i != 0 );
   }
   else if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( HB_ITEM_GET_NUMINTRAW( pItem1 ) !=
                                       HB_ITEM_GET_NUMINTRAW( pItem2 ) );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
      hb_vmPushLogical( hb_vmPopNumber() != hb_vmPopNumber() );
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( pItem1->item.asDate.value !=
                                       pItem2->item.asDate.value );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
      hb_vmPushLogical( hb_vmPopLogical() != hb_vmPopLogical() );
   else if( HB_IS_POINTER( pItem1 ) && HB_IS_POINTER( pItem2 ) )
   {
      BOOL fValue = pItem1->item.asPointer.value !=
                    pItem2->item.asPointer.value;
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fValue );
   }
   else if( HB_IS_HASH( pItem1 ) && HB_IS_HASH( pItem2 ) )
   {
      BOOL fResult = pItem1->item.asHash.value != pItem2->item.asHash.value;
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( hb_objOperatorCall( HB_OO_OP_NOTEQUAL, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1072, NULL, "<>", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmLess( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmLess()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      int i = hb_itemStrCmp( pItem1, pItem2, FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i < 0 );
   }
   else if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( HB_ITEM_GET_NUMINTRAW( pItem1 ) <
                                       HB_ITEM_GET_NUMINTRAW( pItem2 ) );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      double dNumber2 = hb_vmPopNumber();
      double dNumber1 = hb_vmPopNumber();
      hb_vmPushLogical( dNumber1 < dNumber2 );
   }
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( pItem1->item.asDate.value <
                                       pItem2->item.asDate.value );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
   {
      BOOL bLogical2 = hb_vmPopLogical();
      BOOL bLogical1 = hb_vmPopLogical();
      hb_vmPushLogical( bLogical1 < bLogical2 );
   }
   else if( hb_objOperatorCall( HB_OO_OP_LESS, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();

   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1073, NULL, "<", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmLessEqual( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmLessEqual()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      int i = hb_itemStrCmp( pItem1, pItem2, FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i <= 0 );
   }
   else if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( HB_ITEM_GET_NUMINTRAW( pItem1 ) <=
                                       HB_ITEM_GET_NUMINTRAW( pItem2 ) );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      double dNumber2 = hb_vmPopNumber();
      double dNumber1 = hb_vmPopNumber();
      hb_vmPushLogical( dNumber1 <= dNumber2 );
   }
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( pItem1->item.asDate.value <=
                                       pItem2->item.asDate.value );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
   {
      BOOL bLogical2 = hb_vmPopLogical();
      BOOL bLogical1 = hb_vmPopLogical();
      hb_vmPushLogical( bLogical1 <= bLogical2 );
   }
   else if( hb_objOperatorCall( HB_OO_OP_LESSEQUAL, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();

   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1074, NULL, "<=", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmGreater( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmGreater()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      int i = hb_itemStrCmp( pItem1, pItem2, FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i > 0 );
   }
   else if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( HB_ITEM_GET_NUMINTRAW( pItem1 ) >
                                       HB_ITEM_GET_NUMINTRAW( pItem2 ) );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      double dNumber2 = hb_vmPopNumber();
      double dNumber1 = hb_vmPopNumber();
      hb_vmPushLogical( dNumber1 > dNumber2 );
   }
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( pItem1->item.asDate.value >
                                       pItem2->item.asDate.value );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
   {
      BOOL bLogical2 = hb_vmPopLogical();
      BOOL bLogical1 = hb_vmPopLogical();
      hb_vmPushLogical( bLogical1 > bLogical2 );
   }
   else if( hb_objOperatorCall( HB_OO_OP_GREATER, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();

   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1075, NULL, ">", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmGreaterEqual( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmGreaterEqual()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      int i = hb_itemStrCmp( pItem1, pItem2, FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i >= 0 );
   }
   else if( HB_IS_NUMINT( pItem1 ) && HB_IS_NUMINT( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( HB_ITEM_GET_NUMINTRAW( pItem1 ) >=
                                       HB_ITEM_GET_NUMINTRAW( pItem2 ) );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      double dNumber2 = hb_vmPopNumber();
      double dNumber1 = hb_vmPopNumber();
      hb_vmPushLogical( dNumber1 >= dNumber2 );
   }
   else if( HB_IS_DATE( pItem1 ) && HB_IS_DATE( pItem2 ) )
   {
      pItem1->item.asLogical.value = ( pItem1->item.asDate.value >=
                                       pItem2->item.asDate.value );
      pItem1->type = HB_IT_LOGICAL;
      hb_stackDec();
   }
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
   {
      BOOL bLogical2 = hb_vmPopLogical();
      BOOL bLogical1 = hb_vmPopLogical();
      hb_vmPushLogical( bLogical1 >= bLogical2 );
   }
   else if( hb_objOperatorCall( HB_OO_OP_GREATEREQUAL, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();

   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1076, NULL, ">=", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmInstring( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem1;
   PHB_ITEM pItem2;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmInstring()"));

   pItem1 = hb_stackItemFromTop( -2 );
   pItem2 = hb_stackItemFromTop( -1 );

   if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      BOOL fResult = ( hb_strAt( pItem1->item.asString.value, pItem1->item.asString.length,
                                 pItem2->item.asString.value, pItem2->item.asString.length ) != 0 );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( HB_IS_HASH( pItem2 ) &&
            ( HB_IS_HASHKEY( pItem1 ) || hb_hashLen( pItem1 ) == 1 ) )
   {
      BOOL fResult = hb_hashScan( pItem2, pItem1, NULL );

      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( fResult );
   }
   else if( hb_objOperatorCall( HB_OO_OP_INCLUDE, pItem1, pItem2, pItem1, NULL ) )
      hb_stackPop();

   else if( hb_objOperatorCall( HB_OO_OP_INSTRING, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();

   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1109, NULL, "$", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

/* At this moment the eval stack should store:
 * -3 -> <current counter value>
 * -2 -> <end value>
 * -1 -> <step value>
 */
static void hb_vmForTest( void )        /* Test to check the end point of the FOR */
{
   HB_STACK_TLS_PRELOAD
   BOOL fBack;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmForTest()"));

   if( HB_IS_NUMERIC( hb_stackItemFromTop( -1 ) ) )
   {
      fBack = hb_vmPopNumber() < 0.0;
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushInteger( 0 );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1073, NULL, "<", 2, hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         if( HB_IS_LOGICAL( pResult ) )
         {
            fBack = pResult->item.asLogical.value;
            hb_itemRelease( pResult );
            hb_stackPop();
            hb_stackPop();
         }
         else
         {
            hb_itemMove( hb_stackItemFromTop( -1 ), pResult );
            hb_itemRelease( pResult );
            hb_errRT_BASE( EG_ARG, 1066, NULL, hb_langDGetErrorDesc( EG_CONDITION ), 1, hb_stackItemFromTop( -1 ) );
            return;
         }
      }
      else
         return;
   }

   if( fBack )
      hb_vmLess();
   else
      hb_vmGreater();
}

/* Begin Sequence WITH block auto destructor */
static HB_GARBAGE_FUNC( hb_SeqBlockDestructor )
{
   PHB_ITEM * pBlockPtr = ( PHB_ITEM * ) Cargo;

   hb_itemMove( hb_errorBlock(), * pBlockPtr );
   hb_itemRelease( * pBlockPtr );
}

static void hb_vmSeqBlock( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmSeqBlock()"));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_BLOCK( pItem ) )
   {
      PHB_ITEM * pBlockPtr, pBlock, pHolder;

      pBlock = hb_errorBlock();
      pHolder = hb_itemNew( pBlock );
      hb_itemMove( pBlock, pItem );
      pBlockPtr = ( PHB_ITEM * ) hb_gcAlloc( sizeof( PHB_ITEM ),
                                             hb_SeqBlockDestructor );
      * pBlockPtr = pHolder;
      pItem->type = HB_IT_POINTER;
      pItem->item.asPointer.value = pBlockPtr;
      pItem->item.asPointer.collect = pItem->item.asPointer.single = TRUE;
   }
}

/* With object auto destructor */
static HB_GARBAGE_FUNC( hb_withObjectDestructor )
{
   HB_STACK_TLS_PRELOAD
   LONG * plWithObjectBase = ( LONG * ) Cargo;
   hb_stackWithObjectSetOffset( * plWithObjectBase );
}

static void hb_vmWithObjectStart( void )
{
   HB_STACK_TLS_PRELOAD
   LONG * plWithObjectBase;
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmWithObjectStart()"));

   pItem = hb_stackAllocItem();
   plWithObjectBase = ( LONG * ) hb_gcAlloc( sizeof( LONG ),
                                             hb_withObjectDestructor );
   * plWithObjectBase = hb_stackWithObjectOffset();
   pItem->type = HB_IT_POINTER;
   pItem->item.asPointer.value = plWithObjectBase;
   pItem->item.asPointer.collect = pItem->item.asPointer.single = TRUE;
   /* The object is pushed directly before this pcode */
   /* store position of current WITH OBJECT frame */
   hb_stackWithObjectSetOffset( hb_stackTopOffset() - 2 );
}

/*
 * Relase enumerator items - called from hb_itemClear()
 */
void hb_vmEnumRelease( PHB_ITEM pBase, PHB_ITEM pValue )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmEnumRelease(%p,%p)", pBase, pValue));

   if( pValue )
      hb_itemRelease( pValue );

   if( HB_IS_OBJECT( pBase ) && hb_vmRequestQuery() == 0 &&
       hb_objHasOperator( pBase, HB_OO_OP_ENUMSTOP ) )
   {
      hb_stackPushReturn();
      hb_vmPushNil();
      hb_objOperatorCall( HB_OO_OP_ENUMSTOP, hb_stackItemFromTop( -1 ),
                          pBase, NULL, NULL );
      hb_stackPop();
      hb_stackPopReturn();
   }
}

/*
 * extended reference used as enumerator destructor
 */
typedef struct
{
   HB_ITEM  basevalue;
   HB_ITEM  oldvalue;
   HB_ITEM  enumref;
} HB_ENUMREF, * PHB_ENUMREF;

static PHB_ITEM hb_vmEnumRefRead( PHB_ITEM pRefer )
{
   return &( ( PHB_ENUMREF ) pRefer->item.asExtRef.value )->oldvalue;
}

static PHB_ITEM hb_vmEnumRefWrite( PHB_ITEM pRefer, PHB_ITEM pSource )
{
   HB_SYMBOL_UNUSED( pRefer );
   HB_SYMBOL_UNUSED( pSource );
   return NULL;
}

static void hb_vmEnumRefCopy( PHB_ITEM pDest )
{
   pDest->type = HB_IT_NIL;
}

static void hb_vmEnumRefClear( void * value )
{
   hb_itemMove( hb_itemUnRefOnce( &( ( PHB_ENUMREF ) value )->enumref ),
                &( ( PHB_ENUMREF ) value )->oldvalue );
   if( HB_IS_COMPLEX( &( ( PHB_ENUMREF ) value )->basevalue ) )
      hb_itemClear( &( ( PHB_ENUMREF ) value )->basevalue );
   if( HB_IS_COMPLEX( &( ( PHB_ENUMREF ) value )->enumref ) )
      hb_itemClear( &( ( PHB_ENUMREF ) value )->enumref );

   hb_xfree( value );
}

static void hb_vmEnumRefMark( void * value )
{
   if( HB_IS_GCITEM( &( ( PHB_ENUMREF ) value )->basevalue ) )
      hb_gcItemRef( &( ( PHB_ENUMREF ) value )->basevalue );
   if( HB_IS_GCITEM( &( ( PHB_ENUMREF ) value )->oldvalue ) )
      hb_gcItemRef( &( ( PHB_ENUMREF ) value )->oldvalue );
   if( HB_IS_GCITEM( &( ( PHB_ENUMREF ) value )->enumref ) )
      hb_gcItemRef( &( ( PHB_ENUMREF ) value )->enumref );
}

/*
 * create extended reference for enumerator destructor
 */
static void hb_vmEnumReference( PHB_ITEM pBase )
{
   static const HB_EXTREF s_EnumExtRef = {
             hb_vmEnumRefRead,
             hb_vmEnumRefWrite,
             hb_vmEnumRefCopy,
             hb_vmEnumRefClear,
             hb_vmEnumRefMark };

   PHB_ENUMREF pEnumExtRef;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmEnumReference(%p)", pBase));

   pEnumExtRef = ( PHB_ENUMREF ) hb_xgrab( sizeof( HB_ENUMREF ) );
   pEnumExtRef->basevalue.type = HB_IT_NIL;
   pEnumExtRef->oldvalue.type = HB_IT_NIL;
   pEnumExtRef->enumref.type = HB_IT_NIL;
   hb_itemMove( &pEnumExtRef->basevalue, pBase );
   pBase->type = HB_IT_BYREF | HB_IT_EXTREF;
   pBase->item.asExtRef.value = ( void * ) pEnumExtRef;
   pBase->item.asExtRef.func = &s_EnumExtRef;
}

/* At this moment the eval stack should store:
 * -2 -> <array for traverse>
 * -1 -> <the reference to enumerate variable>
 */
 /* Test to check the start point of the FOR EACH loop */
static void hb_vmEnumStart( BYTE nVars, BYTE nDescend )
{
   HB_STACK_TLS_PRELOAD
   BOOL fStart = TRUE;
   int i;

/*
   pItem = hb_itemUnRef( hb_stackItemFromTop( -( ( int ) nVars << 1 ) ) );
   if( ( pItem->type & ( HB_IT_ARRAY | HB_IT_HASH | HB_IT_STRING ) ) == 0 )
   {
      hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 1, pItem );
      return;
   }
*/

   for( i = ( int ) nVars << 1; i > 0 && fStart; i -= 2 )
   {
      HB_ITEM_PTR pBase, pValue, pEnumRef, pEnum;

      pValue = hb_stackItemFromTop( -i );
      /* create extended reference for enumerator destructor */
      hb_vmEnumReference( pValue );
      pBase = &( ( PHB_ENUMREF ) pValue->item.asExtRef.value )->basevalue;
      /* store the reference to control variable */
      pEnumRef = hb_stackItemFromTop( -i + 1 );
      hb_itemCopy( &( ( PHB_ENUMREF ) pValue->item.asExtRef.value )->enumref,
                   pEnumRef );
      /* the control variable */
      pEnum = hb_itemUnRefOnce( pEnumRef );
      /* store the old value of control variable and clear it */
      hb_itemMove( &( ( PHB_ENUMREF ) pValue->item.asExtRef.value )->oldvalue,
                   pEnum );
      /* set the iterator value */
      pEnum->type = HB_IT_BYREF | HB_IT_ENUM;
      pEnum->item.asEnum.basePtr = pBase;
      pEnum->item.asEnum.valuePtr = NULL;

      if( HB_IS_BYREF( pBase ) )
         pBase = hb_itemUnRef( pBase );

      if( HB_IS_OBJECT( pBase ) && hb_objHasOperator( pBase, HB_OO_OP_ENUMSTART ) )
      {
         pEnum->item.asEnum.offset = 0;
         pEnum->item.asEnum.valuePtr = hb_itemNew( NULL );
         hb_vmPushNil();
         hb_vmPushLogical( nDescend == 0 );
         hb_objOperatorCall( HB_OO_OP_ENUMSTART, hb_stackItemFromTop( -2 ),
                             pBase, pEnumRef, hb_stackItemFromTop( -1 ) );
         hb_stackPop();
         if( hb_vmRequestQuery() != 0 || ! hb_vmPopLogical() )
         {
            fStart = FALSE;
            break;
         }
         else if( hb_objHasOperator( pBase, HB_OO_OP_ENUMSKIP ) )
            continue;
         hb_itemRelease( pEnum->item.asEnum.valuePtr );
         pEnum->item.asEnum.valuePtr = NULL;
      }

      if( HB_IS_ARRAY( pBase ) )
      {
         /* the index into an array */
         pEnum->item.asEnum.offset = ( nDescend > 0 ) ? 1 :
                                       pBase->item.asArray.value->ulLen;
         if( pBase->item.asArray.value->ulLen == 0 )
            fStart = FALSE;
      }
      else if( HB_IS_HASH( pBase ) )
      {
         ULONG ulLen = hb_hashLen( pBase );
         /* the index into a hash */
         pEnum->item.asEnum.offset = ( nDescend > 0 ) ? 1 : ulLen;
         if( ulLen == 0 )
            fStart = FALSE;
      }
      else if( HB_IS_STRING( pBase ) )
      {
         /* storage item for single characters */
         pEnum->item.asEnum.offset = ( nDescend > 0 ) ? 1 :
                                       pBase->item.asString.length;
         if( pBase->item.asString.length )
            pEnum->item.asEnum.valuePtr =
                        hb_itemPutCL( NULL, pBase->item.asString.value +
                                            pEnum->item.asEnum.offset - 1, 1 );
         else
            fStart = FALSE;
      }
      else
      {
         hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 1, pBase );
         return;
      }
   }

   hb_vmPushInteger( nVars );    /* number of iterators */
   /* empty array/string - do not start enumerations loop */
   hb_vmPushLogical( fStart );
}


/* Enumeration in ascending order
 * At this moment the eval stack should store:
 * -3 -> <old value of enumerator variable>
 * -2 -> <the reference to enumerate variable>
 * -1 -> <number of iterators>
 */
static void hb_vmEnumNext( void )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR pEnumRef, pEnum, pBase;
   int i;

   for( i = ( int ) hb_stackItemFromTop( -1 )->item.asInteger.value; i > 0; --i )
   {
      pEnumRef = hb_stackItemFromTop( -( i << 1 ) );
      pEnum = hb_itemUnRefOnce( pEnumRef );
      pBase = pEnum->item.asEnum.basePtr;
      if( HB_IS_BYREF( pBase ) )
         pBase = hb_itemUnRef( pBase );
      if( HB_IS_ARRAY( pBase ) )
      {
         if( HB_IS_OBJECT( pBase ) &&
             hb_objHasOperator( pBase, HB_OO_OP_ENUMSKIP ) )
         {
            ++pEnum->item.asEnum.offset;
            hb_vmPushNil();
            hb_vmPushLogical( FALSE );
            hb_objOperatorCall( HB_OO_OP_ENUMSKIP, hb_stackItemFromTop( -2 ),
                                pBase, pEnumRef, hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            if( hb_vmRequestQuery() != 0 || ! hb_vmPopLogical() )
               break;
         }
         else
         {
            /* Clear the item value which can be set with RT error
               when enumerator was out of array size during unreferencing
             */
            if( pEnum->item.asEnum.valuePtr )
            {
               hb_itemRelease( pEnum->item.asEnum.valuePtr );
               pEnum->item.asEnum.valuePtr = NULL;
            }
            if( ( ULONG ) ++pEnum->item.asEnum.offset >
                pBase->item.asArray.value->ulLen )
               break;
         }
      }
      else if( HB_IS_HASH( pBase ) )
      {
         /* Clear the item value which can be set with RT error
            when enumerator was out of array size during unreferencing
          */
         if( pEnum->item.asEnum.valuePtr )
         {
            hb_itemRelease( pEnum->item.asEnum.valuePtr );
            pEnum->item.asEnum.valuePtr = NULL;
         }
         if( ( ULONG ) ++pEnum->item.asEnum.offset > hb_hashLen( pBase ) )
            break;
      }
      else if( HB_IS_STRING( pBase ) )
      {
         if( ( ULONG ) ++pEnum->item.asEnum.offset >
             pBase->item.asString.length )
            break;
         hb_itemPutCL( pEnum->item.asEnum.valuePtr,
                       pBase->item.asString.value +
                       pEnum->item.asEnum.offset - 1, 1 );
      }
      else
      {
         hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 1, pBase );
         return;
      }
   }
   hb_vmPushLogical( i == 0 );
}

/* Enumeration in descending order
 * At this moment the eval stack should store:
 * -3 -> <old value of enumerator variable>
 * -2 -> <the reference to enumerate variable>
 * -1 -> <number of iterators>
 */
static void hb_vmEnumPrev( void )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR pEnumRef, pEnum, pBase;
   int i;

   for( i = hb_stackItemFromTop( -1 )->item.asInteger.value; i > 0; --i )
   {
      pEnumRef = hb_stackItemFromTop( -( i << 1 ) );
      pEnum = hb_itemUnRefOnce( pEnumRef );
      pBase = pEnum->item.asEnum.basePtr;
      if( HB_IS_BYREF( pBase ) )
         pBase = hb_itemUnRef( pBase );
      if( HB_IS_ARRAY( pBase ) )
      {
         if( HB_IS_OBJECT( pBase ) &&
             hb_objHasOperator( pBase, HB_OO_OP_ENUMSKIP ) )
         {
            --pEnum->item.asEnum.offset;
            hb_vmPushNil();
            hb_vmPushLogical( TRUE );
            hb_objOperatorCall( HB_OO_OP_ENUMSKIP, hb_stackItemFromTop( -2 ),
                                pBase, pEnumRef, hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            if( hb_vmRequestQuery() != 0 || ! hb_vmPopLogical() )
               break;
         }
         else
         {
            /* Clear the item value which can be set with RT error
               when enumerator was out of array size during unreferencing
             */
            if( pEnum->item.asEnum.valuePtr )
            {
               hb_itemRelease( pEnum->item.asEnum.valuePtr );
               pEnum->item.asEnum.valuePtr = NULL;
            }
            if( --pEnum->item.asEnum.offset == 0 )
               break;
         }
      }
      else if( HB_IS_HASH( pBase ) )
      {
         /* Clear the item value which can be set with RT error
            when enumerator was out of array size during unreferencing
          */
         if( pEnum->item.asEnum.valuePtr )
         {
            hb_itemRelease( pEnum->item.asEnum.valuePtr );
            pEnum->item.asEnum.valuePtr = NULL;
         }
         if( --pEnum->item.asEnum.offset == 0 )
            break;
      }
      else if( HB_IS_STRING( pBase ) )
      {
         if( --pEnum->item.asEnum.offset == 0 )
            break;
         hb_itemPutCL( pEnum->item.asEnum.valuePtr,
                       pBase->item.asString.value +
                       pEnum->item.asEnum.offset - 1, 1 );
      }
      else
      {
         hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 1, pBase );
         return;
      }
   }
   hb_vmPushLogical( i == 0 );
}

/* Enumeration in descending order
 * At this moment the eval stack should store:
 * -3 -> <old value of enumerator variable>
 * -2 -> <the reference to enumerate variable>
 * -1 -> <number of iterators>
 */
static void hb_vmEnumEnd( void )
{
   HB_STACK_TLS_PRELOAD
   int iVars;

   /* remove number of iterators */
   iVars = hb_stackItemFromTop( -1 )->item.asInteger.value;
   hb_stackDec();

   while( --iVars >= 0 )
   {
      hb_stackPop();
      hb_stackPop();
   }
}

static LONG hb_vmSwitch( const BYTE * pCode, LONG offset, USHORT casesCnt )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR pSwitch = hb_stackItemFromTop( -1 );
   BOOL fFound = FALSE;

   if( !( HB_IS_NUMINT( pSwitch ) || HB_IS_STRING( pSwitch ) ) )
   {
      HB_ITEM_PTR pResult = hb_errRT_BASE_Subst( EG_ARG, 3104, NULL, "SWITCH", 1, pSwitch );

      if( !pResult )
         return offset;

      hb_itemMove( pSwitch, pResult );
      hb_itemRelease( pResult );
   }

   while( !fFound && casesCnt-- )
   {
      switch( pCode[ offset ] )
      {
         case HB_P_PUSHLONG:
            if( HB_IS_NUMINT( pSwitch ) )
            {
               fFound = HB_ITEM_GET_NUMINTRAW( pSwitch ) == HB_PCODE_MKLONG( &pCode[ offset + 1 ] );
            }
            offset += 5;
            break;

         case HB_P_PUSHSTRSHORT:
            if( HB_IS_STRING( pSwitch ) )
            {
               /*fFound = hb_itemStrCmp( pItem1, pItem2, bExact );*/
               fFound = ( ULONG ) pCode[ offset + 1 ] - 1 == pSwitch->item.asString.length &&
                        memcmp( pSwitch->item.asString.value,
                                ( char * ) &pCode[ offset + 2 ],
                                pSwitch->item.asString.length ) == 0;
            }
            offset += 2 + pCode[ offset + 1 ];
            break;

         case HB_P_PUSHNIL:
            /* default clause */
            fFound = TRUE;
            ++offset;
            break;
      }

      switch( pCode[ offset ] )
      {
         case HB_P_JUMPNEAR:
            if( fFound )
               offset += ( signed char ) pCode[ offset + 1 ];
            else
               offset += 2;
            break;
         case HB_P_JUMP:
            if( fFound )
               offset += HB_PCODE_MKSHORT( &pCode[ offset + 1 ] );
            else
               offset += 3;
            break;
         case HB_P_JUMPFAR:
            if( fFound )
               offset += HB_PCODE_MKINT24( &pCode[ offset + 1 ] );
            else
               offset += 4;
            break;
      }
   }
   hb_stackPop();
   return offset;
}

/* ------------------------------- */
/* Operators (logical)             */
/* ------------------------------- */

static void hb_vmNot( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmNot()"));

   pItem = hb_stackItemFromTop( -1 );

   if( HB_IS_LOGICAL( pItem ) )
   {
      pItem->type = HB_IT_LOGICAL;
      pItem->item.asLogical.value = ! pItem->item.asLogical.value;
   }
   else if( ! hb_objOperatorCall( HB_OO_OP_NOT, pItem, pItem, NULL, NULL ) )
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1077, NULL, ".NOT.", 1, pItem );

      if( pResult )
      {
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmAnd( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmAnd()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
   {
      pItem1->type = HB_IT_LOGICAL;
      pItem1->item.asLogical.value = pItem1->item.asLogical.value && pItem2->item.asLogical.value;
      hb_stackDec();
   }
   else if( hb_objOperatorCall( HB_OO_OP_AND, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();

   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1078, NULL, ".AND.", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmOr( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmOr()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
   {
      pItem1->type = HB_IT_LOGICAL;
      pItem1->item.asLogical.value = pItem1->item.asLogical.value || pItem2->item.asLogical.value;
      hb_stackDec();
   }
   else if( hb_objOperatorCall( HB_OO_OP_OR, pItem1, pItem1, pItem2, NULL ) )
      hb_stackPop();

   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1079, NULL, ".OR.", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem1, pResult );
         hb_itemRelease( pResult );
      }
   }
}

/* ------------------------------- */
/* Array                           */
/* ------------------------------- */

static void hb_vmArrayPush( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pIndex;
   PHB_ITEM pArray;
   ULONG ulIndex;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayPush()"));

   pIndex = hb_stackItemFromTop( -1 );
   pArray = hb_stackItemFromTop( -2 );

   if( HB_IS_HASH( pArray ) && HB_IS_HASHKEY( pIndex ) )
   {
      PHB_ITEM pValue = hb_hashGetItemPtr( pArray, pIndex, HB_HASH_AUTOADD_ACCESS );
      if( pValue )
      {
         hb_itemCopy( pIndex, pValue );
         hb_itemMove( pArray, pIndex );
         hb_stackDec();
      }
      else if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, NULL ) )
         hb_stackPop();
      else
         hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
      return;
   }
   else if( HB_IS_INTEGER( pIndex ) )
      ulIndex = ( ULONG ) pIndex->item.asInteger.value;
   else if( HB_IS_LONG( pIndex ) )
      ulIndex = ( ULONG ) pIndex->item.asLong.value;
   else if( HB_IS_DOUBLE( pIndex ) )
      ulIndex = ( ULONG ) pIndex->item.asDouble.value;
   else
   {
      if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, NULL ) )
         hb_stackPop();
      else
      {
         PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
         if( pResult )
         {
            hb_stackPop();
            hb_itemMove( pArray, pResult );
            hb_itemRelease( pResult );
         }
      }
      return;
   }

   if( HB_IS_ARRAY( pArray ) )
   {
      if( HB_IS_OBJECT( pArray ) &&
          hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, NULL ) )
      {
         hb_stackPop();
         return;
      }

      if( HB_IS_VALID_INDEX( ulIndex, pArray->item.asArray.value->ulLen ) )
      {
         hb_itemCopy( pIndex, pArray->item.asArray.value->pItems + ulIndex - 1 );
         hb_itemMove( pArray, pIndex );
         hb_stackDec();
      }
      else if( !HB_IS_OBJECT( pArray ) &&
               hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, NULL ) )
         hb_stackPop();
      else
#ifdef HB_C52_STRICT
         hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 0 );
#else
         hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
#endif
   }
   else if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, NULL ) )
      hb_stackPop();

   else
      hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
}

static void hb_vmArrayPushRef( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pIndex;
   PHB_ITEM pArray;
   PHB_ITEM pRefer;
   ULONG ulIndex;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayPushRef()"));

   pIndex = hb_stackItemFromTop( -1 );
   pRefer = hb_stackItemFromTop( -2 );
   pArray = HB_IS_BYREF( pRefer ) ? hb_itemUnRef( pRefer ) : pRefer;

   if( HB_IS_HASH( pArray ) && HB_IS_HASHKEY( pIndex ) )
   {
      PHB_ITEM pValue = hb_hashGetItemRefPtr( pArray, pIndex );
      if( pValue )
      {
         hb_itemCopy( pIndex, pValue );
         hb_itemMove( pRefer, pIndex );
         hb_stackDec();
      }
      else if( hb_objHasOperator( pArray, HB_OO_OP_ARRAYINDEX ) )
      {
         /* create extended object index reference */
         hb_vmMsgIndexReference( pRefer, pArray, pIndex );
         hb_stackPop();
         return;
      }
      else
         hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
      return;
   }
   else if( HB_IS_INTEGER( pIndex ) )
      ulIndex = ( ULONG ) pIndex->item.asInteger.value;
   else if( HB_IS_LONG( pIndex ) )
      ulIndex = ( ULONG ) pIndex->item.asLong.value;
   else if( HB_IS_DOUBLE( pIndex ) )
      ulIndex = ( ULONG ) pIndex->item.asDouble.value;
   else if( hb_objHasOperator( pArray, HB_OO_OP_ARRAYINDEX ) )
   {
      /* create extended object index reference */
      hb_vmMsgIndexReference( pRefer, pArray, pIndex );
      hb_stackPop();
      return;
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pRefer, pResult );
         hb_itemRelease( pResult );
      }
      return;
   }

   if( HB_IS_ARRAY( pArray ) )
   {
      if( HB_IS_OBJECT( pArray ) && hb_objHasOperator( pArray, HB_OO_OP_ARRAYINDEX ) )
      {
         /* create extended object index reference */
         hb_vmMsgIndexReference( pRefer, pArray, pIndex );
         hb_stackPop();
         return;
      }
      else if( HB_IS_VALID_INDEX( ulIndex, pArray->item.asArray.value->ulLen ) )
      {
         /* This function is safe for overwriting passed array, [druzus] */
         hb_arrayGetItemRef( pArray, ulIndex, pRefer );
         hb_stackDec();
      }
      else if( !HB_IS_OBJECT( pArray ) && hb_objHasOperator( pArray, HB_OO_OP_ARRAYINDEX ) )
      {
         /* create extended object index reference */
         hb_vmMsgIndexReference( pRefer, pArray, pIndex );
         hb_stackPop();
         return;
      }
      else
#ifdef HB_C52_STRICT
         hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 0 );
#else
         hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
#endif
   }
   else if( hb_objHasOperator( pArray, HB_OO_OP_ARRAYINDEX ) )
   {
      /* create extended object index reference */
      hb_vmMsgIndexReference( pRefer, pArray, pIndex );
      hb_stackPop();
      return;
   }
   else
      hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
}

static void hb_vmArrayPop( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pValue;
   PHB_ITEM pIndex;
   PHB_ITEM pArray;
   ULONG ulIndex;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayPop()"));

   pValue = hb_stackItemFromTop( -3 );
   pArray = hb_stackItemFromTop( -2 );
   pIndex = hb_stackItemFromTop( -1 );

   if( HB_IS_BYREF( pArray ) )
      pArray = hb_itemUnRef( pArray );

   if( HB_IS_HASH( pArray ) && HB_IS_HASHKEY( pIndex ) )
   {
      PHB_ITEM pDest = hb_hashGetItemPtr( pArray, pIndex, HB_HASH_AUTOADD_ASSIGN );
      if( pDest )
      {
         pValue->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );
         hb_itemMoveFromRef( pDest, pValue );
         hb_stackPop();
         hb_stackPop();
         hb_stackDec();    /* value was moved above hb_stackDec() is enough */
      }
      else if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, pValue ) )
      {
         hb_stackPop();
         hb_stackPop();
         hb_stackPop();
      }
      else
         hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 3, pArray, pIndex, pValue );
      return;
   }
   else if( HB_IS_INTEGER( pIndex ) )
      ulIndex = ( ULONG ) pIndex->item.asInteger.value;
   else if( HB_IS_LONG( pIndex ) )
      ulIndex = ( ULONG ) pIndex->item.asLong.value;
   else if( HB_IS_DOUBLE( pIndex ) )
      ulIndex = ( ULONG ) pIndex->item.asDouble.value;
   else
   {
      if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, pValue ) )
      {
         hb_stackPop();
         hb_stackPop();
         hb_stackPop();
      }
      else
         hb_errRT_BASE( EG_ARG, 1069, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 1, pIndex );
      return;
   }

   if( HB_IS_ARRAY( pArray ) )
   {
      if( HB_IS_OBJECT( pArray ) &&
          hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, pValue ) )
      {
         hb_stackPop();
         hb_stackPop();
         hb_stackPop();
         return;
      }

      if( HB_IS_VALID_INDEX( ulIndex, pArray->item.asArray.value->ulLen ) )
      {
         pValue->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );
         hb_itemMoveRef( pArray->item.asArray.value->pItems + ulIndex - 1, pValue );
         hb_stackPop();
         hb_stackPop();
         hb_stackDec();    /* value was moved above hb_stackDec() is enough */
      }
      else if( !HB_IS_OBJECT( pArray ) &&
               hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, pValue ) )
      {
         hb_stackPop();
         hb_stackPop();
         hb_stackPop();
      }
      else
#ifdef HB_C52_STRICT
         hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 0 );
#else
         hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 1, pIndex );
#endif
   }
   else if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray, pIndex, pValue ) )
   {
      hb_stackPop();
      hb_stackPop();
      hb_stackPop();
      return;
   }
   else
      hb_errRT_BASE( EG_ARG, 1069, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 1, pIndex );
}

static void hb_vmArrayGen( ULONG ulElements ) /* generates an ulElements Array and fills it from the stack values */
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pArray;
   ULONG ulPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayGen(%lu)", ulElements));

   /* create new array on HVM stack */
   pArray = hb_stackAllocItem();
   hb_arrayNew( pArray, ulElements );

   if( ulElements )
   {
      /* move items from HVM stack to created array */
      for( ulPos = 0; ulPos < ulElements; ulPos++ )
      {
         PHB_ITEM pValue = hb_stackItemFromTop( ( int ) ( ulPos - ulElements - 1 ) );
         pValue->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );
         hb_itemMove( pArray->item.asArray.value->pItems + ulPos, pValue );
      }
      /* move the new array to position of first parameter */
      hb_itemMove( hb_stackItemFromTop( ( int ) ( -1 - ulElements ) ), pArray );

      /* decrease the stack counter - all items are NIL */
      hb_stackDecrease( ulElements );
   }
}

/* This function creates an array item using 'uiDimension' as an index
 * to retrieve the number of elements from the stack
 */
static void hb_vmArrayNew( HB_ITEM_PTR pArray, USHORT uiDimension )
{
   HB_STACK_TLS_PRELOAD
   ULONG ulElements;
   HB_ITEM_PTR pDim;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayNew(%p, %hu)", pArray, uiDimension));

   pDim = hb_stackItemFromTop( ( int ) ( -1 - uiDimension ) );

   /* use the proper type of number of elements */
   if( HB_IS_INTEGER( pDim ) )
      ulElements = ( ULONG ) pDim->item.asInteger.value;
   else if( HB_IS_LONG( pDim ) )
      ulElements = ( ULONG ) pDim->item.asLong.value;
   else if( HB_IS_DOUBLE( pDim ) )
      ulElements = ( ULONG ) pDim->item.asDouble.value;
   else
      /* NOTE: Clipper creates empty array if non-numeric value is
       * specified as dimension and stops further processing.
       * There is no runtime error generated.
       */
      ulElements = 0;

   /* create an array */
   hb_arrayNew( pArray, ulElements );

   if( --uiDimension )
   {
      /* call self recursively to create next dimensions
       */
      while( ulElements-- )
         hb_vmArrayNew( pArray->item.asArray.value->pItems + ulElements, uiDimension );
   }
}

static void hb_vmArrayDim( USHORT uiDimensions ) /* generates an uiDimensions Array and initialize those dimensions from the stack values */
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayDim(%hu)", uiDimensions));

   hb_vmArrayNew( hb_stackAllocItem(), uiDimensions );

   hb_itemMove( hb_stackItemFromTop( ( int ) ( -1 - uiDimensions ) ),
                hb_stackItemFromTop( -1 ) );
   do
   {
      hb_stackPop();
   }
   while( --uiDimensions );
}

static void hb_vmHashGen( ULONG ulElements ) /* generates an ulElements Hash and fills it from the stack values */
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pHash, pKey, pVal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmHashGen(%lu)", ulElements));

   /* create new hash item */
   pHash = hb_hashNew( NULL );
   hb_hashPreallocate( pHash, ulElements );
   while( ulElements-- )
   {
      pKey = hb_stackItemFromTop( -2 );
      pVal = hb_stackItemFromTop( -1 );
      if( HB_IS_HASHKEY( pKey ) )
      {
         hb_hashAddNew( pHash, pKey, pVal );
         hb_stackPop();
         hb_stackPop();
      }
      else
      {
         hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 3, pHash, pKey, pVal );
         break;
      }
   }
   hb_itemMove( hb_stackAllocItem(), pHash );
   hb_itemRelease( pHash );
}


/* ------------------------------- */
/* Macros                          */
/* ------------------------------- */

static void hb_vmMacroPushIndex( void )
{
   HB_STACK_TLS_PRELOAD
   ULONG ulIndexes;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMacroPushIndex()"));

   /*
    * Now the top most element on the stack points to number of
    * additional indexes to generated array
    */
   ulIndexes = hb_itemGetNL( hb_stackItemFromTop( -1 ) );
   hb_stackDec();

   if( ulIndexes > 1 )
   {
      PHB_ITEM pIndexArray;
      ULONG ul = 1;

      hb_vmArrayGen( ulIndexes - 1 );
      pIndexArray = hb_itemNew( hb_stackItemFromTop( -1 ) );
      hb_stackPop();

      /* First index is still on stack.*/
      do
      {
         hb_vmArrayPush();
         /* RT error? */
         if( hb_stackGetActionRequest() != 0 )
            break;
         hb_vmPush( hb_arrayGetItemPtr( pIndexArray, ul ) );
      }
      while( ++ul < ulIndexes );

      hb_itemRelease( pIndexArray );
   }
   else if( ulIndexes == 0 )
      hb_vmPushNil();   /* It will force RT error later in array push or pop */
}

/*
 * On HVM stack we have sets with arguments
 *    offset   value
 *    (-9)     6
 *    (-8)     7
 *    (-7)     2 // number of arguments
 *    (-6)     1
 *    (-5)     2
 *    (-4)     2 // number of arguments
 *    (-3)     1
 *    (-2)     2
 *    (-1)     2 // number of arguments
 * we should join them into one continuous list
 */
static LONG hb_vmArgsJoin( LONG lLevel, USHORT uiArgSets )
{
   HB_STACK_TLS_PRELOAD
   LONG lArgs, lRestArgs, lOffset;
   PHB_ITEM pArgs = hb_stackItemFromTop( lLevel ) ;


   lArgs = hb_itemGetNL( pArgs );
   if( HB_IS_COMPLEX( pArgs ) )
      hb_itemClear( pArgs );

   if( --uiArgSets )
   {
      lRestArgs = lArgs;
      lArgs += hb_vmArgsJoin( lLevel - lArgs - 1, uiArgSets );
      lOffset = lLevel - lRestArgs - uiArgSets;
      while( lRestArgs-- )
      {
         hb_itemMove( hb_stackItemFromTop( lOffset ),
                      hb_stackItemFromTop( lOffset + uiArgSets ) );
         ++lOffset;
      }
   }

   return lArgs;
}

static void hb_vmMacroDo( USHORT uiArgSets )
{
   HB_STACK_TLS_PRELOAD
   LONG lArgs;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMacroDo(%hu)", uiArgSets));

   lArgs = hb_vmArgsJoin( -1, uiArgSets );
   hb_stackDecrease( uiArgSets );
   hb_vmDo( ( USHORT ) lArgs );
}

static void hb_vmMacroFunc( USHORT uiArgSets )
{
   HB_STACK_TLS_PRELOAD
   LONG lArgs;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMacroFunc(%hu)", uiArgSets));

   lArgs = hb_vmArgsJoin( -1, uiArgSets );
   hb_stackDecrease( uiArgSets );
   hb_vmDo( ( USHORT ) lArgs );
   hb_stackPushReturn();
}

static void hb_vmMacroSend( USHORT uiArgSets )
{
   HB_STACK_TLS_PRELOAD
   LONG lArgs;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMacroSend(%hu)", uiArgSets));

   lArgs = hb_vmArgsJoin( -1, uiArgSets );
   hb_stackDecrease( uiArgSets );
   hb_vmSend( ( USHORT ) lArgs );
   hb_stackPushReturn();
}

static void hb_vmMacroArrayGen( USHORT uiArgSets )
{
   HB_STACK_TLS_PRELOAD
   LONG lArgs;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMacroArrayGen(%hu)", uiArgSets));

   lArgs = hb_vmArgsJoin( -1, uiArgSets );
   hb_stackDecrease( uiArgSets );
   hb_vmArrayGen( lArgs );
}

static void hb_vmPushVParams( void )
{
   HB_STACK_TLS_PRELOAD
   int iPCount, iFirst, i = 0;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushVParams()"));

   iFirst = hb_stackBaseItem()->item.asSymbol.paramdeclcnt;
   iPCount = hb_pcount();
   while( ++iFirst <= iPCount )
   {
      hb_vmPush( hb_stackItemFromBase( iFirst ) );
      i++;
   }
   hb_vmPushInteger( i );
}

/* ------------------------------- */
/* Database                        */
/* ------------------------------- */

static ERRCODE hb_vmSelectWorkarea( PHB_ITEM pAlias, PHB_SYMB pField )
{
   HB_STACK_TLS_PRELOAD
   ERRCODE errCode;
   BOOL fRepeat;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmSelectWorkArea(%p,%p)", pAlias, pField));

   /* NOTE: Clipper doesn't generate an error if an workarea specified
    * as numeric value cannot be selected
    */
   do
   {
      fRepeat = FALSE;
      errCode = SUCCESS;

      switch( HB_ITEM_TYPE( pAlias ) )
      {
         case HB_IT_INTEGER:
            /* Alias was used as integer value, for example: 4->field
             * or it was saved on the stack using hb_vmPushAlias()
             * or was evaluated from an expression, (nWorkArea)->field
             */
            hb_rddSelectWorkAreaNumber( pAlias->item.asInteger.value );
            pAlias->type = HB_IT_NIL;
            break;

         case HB_IT_LONG:
            /* Alias was evaluated from an expression, (nWorkArea)->field
             */
            hb_rddSelectWorkAreaNumber( ( int ) pAlias->item.asLong.value );
            pAlias->type = HB_IT_NIL;
            break;

         case HB_IT_DOUBLE:
            /* Alias was evaluated from an expression, (nWorkArea)->field
             */
            hb_rddSelectWorkAreaNumber( ( int ) pAlias->item.asDouble.value );
            pAlias->type = HB_IT_NIL;
            break;

         case HB_IT_SYMBOL:
            /* Alias was specified using alias identifier, for example: al->field
             */
            errCode = hb_rddSelectWorkAreaSymbol( pAlias->item.asSymbol.value );
            pAlias->type = HB_IT_NIL;
            break;

         case HB_IT_STRING:
         {
            /* Alias was evaluated from an expression, for example: (cVar)->field
             */
            /* expand '&' operator if exists */
            char * szAlias;
            BOOL bNewString;

            szAlias = hb_macroExpandString( pAlias->item.asString.value, pAlias->item.asString.length, &bNewString );
            if( pField )
            {
               errCode = hb_rddSelectWorkAreaAlias( szAlias );
            }
            else
            {
               int iArea;
               hb_rddGetAliasNumber( szAlias, &iArea );
               hb_rddSelectWorkAreaNumber( iArea );
            }

            if( bNewString )
               hb_xfree( szAlias );
            hb_itemClear( pAlias );
            break;
         }

         default:
            if( pField )
            {
               PHB_ITEM pSubstVal;

               hb_vmPushString( pField->szName, strlen( pField->szName ) );
               pSubstVal = hb_errRT_BASE_Subst( EG_ARG, 1065, NULL, "&",
                                       2, pAlias, hb_stackItemFromTop( -1 ) );
               hb_stackPop();
               if( pSubstVal )
               {
                  hb_itemMove( pAlias, pSubstVal );
                  hb_itemRelease( pSubstVal );
                  fRepeat = TRUE;
               }
               else
               {
                  hb_itemSetNil( pAlias );
                  errCode = FAILURE;
               }
            }
            else
            {
               hb_rddSelectWorkAreaNumber( -1 );
               hb_itemSetNil( pAlias );
            }
            break;
      }
   }
   while( fRepeat );

   return errCode;
}

/* Swaps two last items on the eval stack - the last item after swaping
 * is popped as current workarea number
 */
static void hb_vmSwapAlias( void )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR pItem;
   HB_ITEM_PTR pWorkArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmSwapAlias()"));

   pItem = hb_stackItemFromTop( -1 );
   pWorkArea = hb_stackItemFromTop( -2 );

   hb_vmSelectWorkarea( pWorkArea, NULL );

   hb_itemMove( pWorkArea, pItem );
   hb_stackDec();
}

/* ------------------------------- */
/* Execution                       */
/* ------------------------------- */

HB_EXPORT void hb_vmDo( USHORT uiParams )
{
   HB_STACK_TLS_PRELOAD
   HB_STACK_STATE sStackState;
   PHB_SYMB pSym;
   PHB_ITEM pSelf;
#ifndef HB_NO_PROFILER
   ULONG ulClock = 0;
   BOOL bProfiler = hb_bProfiler; /* because profiler state may change */
#endif

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDo(%hu)", uiParams));

#ifndef HB_NO_PROFILER
   if( bProfiler )
      ulClock = ( ULONG ) clock();
#endif

   /* Poll the console keyboard
   #if !defined( HB_GUI )
      hb_inkeyPoll();
   #endif
   */

   pSym = hb_stackNewFrame( &sStackState, uiParams )->item.asSymbol.value;
   pSelf = hb_stackSelfItem();   /* NIL, OBJECT or BLOCK */

   if( ! HB_IS_NIL( pSelf ) ) /* are we sending a message ? */
   {
      PHB_SYMB pExecSym;

      pExecSym = hb_objGetMethod( pSelf, pSym, &sStackState );
      if( pExecSym && ( pExecSym->scope.value & HB_FS_DEFERRED ) && pExecSym->pDynSym )
         pExecSym = pExecSym->pDynSym->pSymbol;
      if( pExecSym && pExecSym->value.pFunPtr )
      {
         HB_TRACE_PRG(("Calling: %s:%s", hb_objGetClsName( pSelf ), pSym->szName));
         if( pExecSym->scope.value & HB_FS_PCODEFUNC )
            /* Running pCode dynamic function from .hrb */
            hb_vmExecute( pExecSym->value.pCodeFunc->pCode,
                          pExecSym->value.pCodeFunc->pSymbols );
         else
            pExecSym->value.pFunPtr();

#ifndef HB_NO_PROFILER
         if( bProfiler )
            hb_mthAddTime( clock() - ulClock );
#endif
      }
      else if( pSym->szName[ 0 ] == '_' )
         hb_errRT_BASE_SubstR( EG_NOVARMETHOD, 1005, NULL, pSym->szName + 1, HB_ERR_ARGS_SELFPARAMS );
      else
         hb_errRT_BASE_SubstR( EG_NOMETHOD, 1004, NULL, pSym->szName, HB_ERR_ARGS_SELFPARAMS );
   }
   else /* it is a function */
   {
      if( ( pSym->scope.value & HB_FS_DEFERRED ) && pSym->pDynSym )
         pSym = pSym->pDynSym->pSymbol;
      if( pSym->value.pFunPtr )
      {
         HB_TRACE_PRG(("Calling: %s", pSym->szName));
#ifndef HB_NO_PROFILER
         if( bProfiler && pSym->pDynSym )
            pSym->pDynSym->ulRecurse++;
#endif
         /* Running pCode dynamic function from .hrb? */
         if( pSym->scope.value & HB_FS_PCODEFUNC )
            hb_vmExecute( pSym->value.pCodeFunc->pCode,
                          pSym->value.pCodeFunc->pSymbols );
         else
            pSym->value.pFunPtr();

#ifndef HB_NO_PROFILER
         if( bProfiler && pSym->pDynSym )
         {
            pSym->pDynSym->ulCalls++;                   /* profiler support */
            /* Time spent has to be added only inside topmost call of a recursive function */
            if( --pSym->pDynSym->ulRecurse == 0 )
               pSym->pDynSym->ulTime += clock() - ulClock; /* profiler support */
         }
#endif
      }
      else
         hb_errRT_BASE_SubstR( EG_NOFUNC, 1001, NULL, pSym->szName, HB_ERR_ARGS_BASEPARAMS );
   }

#ifndef HB_NO_DEBUG
   if( sStackState.fDebugging )
      hb_vmDebuggerEndProc();
#endif

   hb_stackOldFrame( &sStackState );
}

HB_EXPORT void hb_vmSend( USHORT uiParams )
{
   HB_STACK_TLS_PRELOAD
   HB_STACK_STATE sStackState;
   PHB_SYMB pSym;
   PHB_SYMB pExecSym;
   PHB_ITEM pSelf;
#ifndef HB_NO_PROFILER
   ULONG ulClock = 0;
   BOOL bProfiler = hb_bProfiler; /* because profiler state may change */
#endif

   HB_TRACE(HB_TR_DEBUG, ("hb_vmSend(%hu)", uiParams));

#ifndef HB_NO_PROFILER
   if( bProfiler )
      ulClock = ( ULONG ) clock();
#endif

   /* Poll the console keyboard
   #if !defined( HB_GUI )
      hb_inkeyPoll();
   #endif
   */

   pSym = hb_stackNewFrame( &sStackState, uiParams )->item.asSymbol.value;
   pSelf = hb_stackSelfItem();   /* NIL, OBJECT or BLOCK */

   pExecSym = hb_objGetMethod( pSelf, pSym, &sStackState );
   if( pExecSym && ( pExecSym->scope.value & HB_FS_DEFERRED ) && pExecSym->pDynSym )
      pExecSym = pExecSym->pDynSym->pSymbol;
   if( pExecSym && pExecSym->value.pFunPtr )
   {
      HB_TRACE_PRG(("Calling: %s:%s", hb_objGetClsName( pSelf ), pSym->szName));

      if( pExecSym->scope.value & HB_FS_PCODEFUNC )
         /* Running pCode dynamic function from .hrb */
         hb_vmExecute( pExecSym->value.pCodeFunc->pCode,
                    pExecSym->value.pCodeFunc->pSymbols );
      else
         pExecSym->value.pFunPtr();

#ifndef HB_NO_PROFILER
      if( bProfiler )
         hb_mthAddTime( clock() - ulClock );
#endif
   }
   else if( pSym->szName[ 0 ] == '_' )
      hb_errRT_BASE_SubstR( EG_NOVARMETHOD, 1005, NULL, pSym->szName + 1, HB_ERR_ARGS_SELFPARAMS );
   else
      hb_errRT_BASE_SubstR( EG_NOMETHOD, 1004, NULL, pSym->szName, HB_ERR_ARGS_SELFPARAMS );

#ifndef HB_NO_DEBUG
   if( sStackState.fDebugging )
      hb_vmDebuggerEndProc();
#endif

   hb_stackOldFrame( &sStackState );
}

static void hb_vmPushObjectVarRef( void )
{
   HB_STACK_TLS_PRELOAD
   HB_STACK_STATE sStackState;
   PHB_ITEM pItem;
   PHB_SYMB pSym;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushObjectVarRef()"));

   pItem = hb_stackNewFrame( &sStackState, 0 );   /* procedure name */
   pSym = pItem->item.asSymbol.value;

   if( !hb_objGetVarRef( hb_stackSelfItem(), pSym, &sStackState ) &&
       hb_vmRequestQuery() == 0 )
      hb_errRT_BASE_SubstR( EG_NOVARMETHOD, 1005, NULL, pSym->szName + ( pSym->szName[ 0 ] == '_' ? 1 : 0 ), 1, hb_stackSelfItem() );

   hb_stackOldFrame( &sStackState );

   hb_stackPushReturn();
}

static HARBOUR hb_vmDoBlock( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pBlock;
   int iParam;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDoBlock()"));

   pBlock = hb_stackSelfItem();

   if( ! HB_IS_BLOCK( pBlock ) )
      hb_errInternal( HB_EI_VMNOTCBLOCK, NULL, "hb_vmDoBlock()", NULL );

   /* Check for valid count of parameters */
   iParam = pBlock->item.asBlock.paramcnt - hb_pcount();
   hb_stackBaseItem()->item.asSymbol.paramdeclcnt =
               pBlock->item.asBlock.paramcnt;

   /* add missing parameters */
   while( --iParam >= 0 )
      hb_vmPushNil();

   /* set the current line number to a line where the codeblock was defined
    */
   hb_stackBaseItem()->item.asSymbol.stackstate->uiLineNo = pBlock->item.asBlock.lineno;
   hb_stackBaseItem()->item.asSymbol.stackstate->uiClass  = pBlock->item.asBlock.hclass;
   hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod = pBlock->item.asBlock.method;

   hb_codeblockEvaluate( pBlock );
}

/* Evaluates a passed codeblock item with no arguments passed to a codeblock
*/
HB_ITEM_PTR hb_vmEvalBlock( HB_ITEM_PTR pBlock )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmEvalBlock(%p)", pBlock));

   hb_vmPushSymbol( &hb_symEval );
   hb_vmPush( pBlock );
   hb_vmSend( 0 );
   return hb_stackReturnItem();
}

/* Evaluates a codeblock item using passed additional arguments
 * pBlock = an item of codeblock type to evaluate
 * ulArgCount = number of arguments passed to a codeblock
 * ... = the list of arguments of type PHB_ITEM
 *
 * for example:
 *  retVal = hb_vmEvalBlockV( pBlock, 2, pParam1, pParam2 );
*/
HB_ITEM_PTR hb_vmEvalBlockV( HB_ITEM_PTR pBlock, ULONG ulArgCount, ... )
{
   HB_STACK_TLS_PRELOAD
   va_list va;
   ULONG i;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmEvalBlockV(%p, %hu, ...)", pBlock, ulArgCount));

   hb_vmPushSymbol( &hb_symEval );
   hb_vmPush( pBlock );

   va_start( va, ulArgCount );
   for( i = 1; i <= ulArgCount; i++ )
      hb_vmPush( va_arg( va, PHB_ITEM ) );
   va_end( va );

   /* take care here, possible loss of data long to short ... */
   /* added an explicit casting here for VC++ JFL */
   hb_vmSend( ( USHORT ) ulArgCount );

   return hb_stackReturnItem();
}

/* Evaluates a passed codeblock item or macro pointer item
 */
HB_EXPORT PHB_ITEM hb_vmEvalBlockOrMacro( PHB_ITEM pItem )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmEvalBlockOrMacro(%p)", pItem));

   if( HB_IS_BLOCK( pItem ) )
   {
      hb_vmPushSymbol( &hb_symEval );
      hb_vmPush( pItem );
      hb_vmSend( 0 );
   }
   else
   {
      HB_MACRO_PTR pMacro = ( HB_MACRO_PTR ) hb_itemGetPtr( pItem );
      if( pMacro )
      {
         hb_macroRun( pMacro );
         hb_stackPopReturn();
      }
      else
      {
         hb_itemSetNil( hb_stackReturnItem() );
      }
   }
   return hb_stackReturnItem();
}

/*
 * destroy codeblock or macro in given item
 */
HB_EXPORT void hb_vmDestroyBlockOrMacro( PHB_ITEM pItem )
{
   if( HB_IS_POINTER( pItem ) )
   {
      HB_MACRO_PTR pMacro = ( HB_MACRO_PTR ) hb_itemGetPtr( pItem );
      if( pMacro )
      {
         hb_macroDelete( pMacro );
      }
   }
   hb_itemRelease( pItem );
}



void hb_vmFunction( USHORT uiParams )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmFunction(%hu)", uiParams));

   hb_itemSetNil( hb_stackReturnItem() );
   hb_vmDo( uiParams );
}


#ifndef HB_NO_DEBUG
static void hb_vmDebugEntry( int nMode, int nLine, char *szName, int nIndex, int nFrame )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDebugEntry"));

   HB_SYMBOL_UNUSED( nFrame );

   switch( nMode )
   {
      case HB_DBG_MODULENAME:
         hb_vmPushDynSym( s_pDynsDbgEntry );
         hb_vmPushNil();
         hb_vmPushInteger( HB_DBG_MODULENAME );
         hb_vmPushString( szName, strlen( szName ) );
         hb_vmDo( 2 );
         break;

      case HB_DBG_LOCALNAME:
         hb_vmPushDynSym( s_pDynsDbgEntry );
         hb_vmPushNil();
         hb_vmPushInteger( HB_DBG_LOCALNAME );
         hb_vmPushInteger( nIndex );
         hb_vmPushString( szName, strlen( szName ) );
         hb_vmDo( 3 );
         break;

      case HB_DBG_STATICNAME:
         hb_vmPushDynSym( s_pDynsDbgEntry );
         hb_vmPushNil();
         hb_vmPushInteger( HB_DBG_STATICNAME );
         hb_vmPushLong( hb_stackGetStaticsBase() );   /* current static frame */
         hb_vmPushInteger( nIndex );                  /* variable index */
         hb_vmPushString( szName, strlen( szName ) );
         hb_vmDo( 4 );
         break;

      case HB_DBG_ENDPROC:
         hb_stackPushReturn();      /* saves the previous returned value */
         hb_vmPushDynSym( s_pDynsDbgEntry );
         hb_vmPushNil();
         hb_vmPushInteger( HB_DBG_ENDPROC );
         hb_vmDo( 1 );
         hb_stackPopReturn();       /* restores the previous returned value */
         break;

      case HB_DBG_SHOWLINE:
         hb_vmPushDynSym( s_pDynsDbgEntry );
         hb_vmPushNil();
         hb_vmPushInteger( HB_DBG_SHOWLINE );
         hb_vmPushInteger( nLine );
         hb_vmDo( 2 );
         break;

      case HB_DBG_GETENTRY:
         /* Try to get C dbgEntry() function pointer */
         hb_vmPushDynSym( s_pDynsDbgEntry );
         hb_vmPushNil();
         hb_vmPushInteger( HB_DBG_GETENTRY );
         hb_vmDo( 1 );
         break;

      case HB_DBG_VMQUIT:
         hb_vmPushDynSym( s_pDynsDbgEntry );
         hb_vmPushNil();
         hb_vmPushInteger( HB_DBG_VMQUIT );
         hb_vmDo( 1 );
         break;
   }
}

static void hb_vmDummyDebugEntry( int nMode, int nLine, char *szName, int nIndex, int nFrame )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmDummyDebugEntry"));

   HB_SYMBOL_UNUSED( nMode );
   HB_SYMBOL_UNUSED( nLine );
   HB_SYMBOL_UNUSED( szName );
   HB_SYMBOL_UNUSED( nIndex );
   HB_SYMBOL_UNUSED( nFrame );
}

static void hb_vmDebuggerExit( BOOL fRemove )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmDebuggerExit(%d)", fRemove));

   /* is debugger linked ? */
   if( s_pFunDbgEntry )
   {
      /* inform debugger that we are quitting now */
      s_pFunDbgEntry( HB_DBG_VMQUIT, 0, NULL, 0, 0 );
      /* set dummy debugger function to avoid debugger activation in .prg
       *       destructors if any */
      if( fRemove )
         s_pFunDbgEntry = hb_vmDummyDebugEntry;
   }
}

static void hb_vmDebuggerEndProc( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmDebuggerEndProc()"));

   s_pFunDbgEntry( HB_DBG_ENDPROC, 0, NULL, 0, 0 );
}

static void hb_vmDebuggerShowLine( USHORT uiLine ) /* makes the debugger shows a specific source code line */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmDebuggerShowLine(%hu)", uiLine));

   s_pFunDbgEntry( HB_DBG_SHOWLINE, uiLine, NULL, 0, 0 );
}

static void hb_vmLocalName( USHORT uiLocal, char * szLocalName ) /* locals and parameters index and name information for the debugger */
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmLocalName(%hu, %s)", uiLocal, szLocalName));

   if( hb_stackBaseItem()->item.asSymbol.stackstate->fDebugging )
      s_pFunDbgEntry( HB_DBG_LOCALNAME, 0, szLocalName, uiLocal, 0 );
}

static void hb_vmStaticName( BYTE bIsGlobal, USHORT uiStatic, char * szStaticName ) /* statics vars information for the debugger */
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmStaticName(%hu, %s)", uiStatic, szStaticName));

   HB_SYMBOL_UNUSED( bIsGlobal );

   if( hb_stackBaseItem()->item.asSymbol.stackstate->fDebugging )
      s_pFunDbgEntry( HB_DBG_STATICNAME, 0, szStaticName, uiStatic, hb_stackGetStaticsBase() );
}

static void hb_vmModuleName( char * szModuleName ) /* PRG and function name information for the debugger */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmModuleName(%s)", szModuleName));

   if( s_pFunDbgEntry )
   {
      HB_STACK_TLS_PRELOAD
      s_pFunDbgEntry( HB_DBG_MODULENAME, 0, szModuleName, 0, 0 );
      hb_stackBaseItem()->item.asSymbol.stackstate->fDebugging = TRUE;
   }
}
#endif

static void hb_vmFrame( USHORT usLocals, BYTE bParams )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pBase;
   int iTotal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmFrame(%d, %d)", (int) usLocals, (int) bParams));

   pBase = hb_stackBaseItem();

#if 0
   /* This old code which clears additional parameters to make space for
    * local variables without updating pBase->item.asSymbol.paramdeclcnt
    */
   iTotal = pBase->item.asSymbol.paramcnt - bParams;
   if( iTotal > 0 )
   {
      pBase->item.asSymbol.paramcnt = bParams;
      do
         hb_itemClear( hb_stackItemFromTop( -iTotal ) );
      while( --iTotal > 0 );
   }

   iTotal = usLocals + bParams;
   if( iTotal )
   {
      iTotal -= pBase->item.asSymbol.paramcnt;
      while( --iTotal >= 0 )
         hb_vmPushNil();
   }
#else
   pBase->item.asSymbol.paramdeclcnt = bParams;

   iTotal = bParams - pBase->item.asSymbol.paramcnt;
   if( iTotal < 0 )
      iTotal = 0;
   iTotal += usLocals;

   if( iTotal )
   {
      do
         hb_vmPushNil();
      while( --iTotal > 0 );
   }
#endif
}

static void hb_vmVFrame( USHORT usLocals, BYTE bParams )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pBase;
   int iTotal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmVFrame(%d, %d)", (int) usLocals, (int) bParams));

   pBase = hb_stackBaseItem();

   pBase->item.asSymbol.paramdeclcnt = bParams;

   iTotal = bParams - pBase->item.asSymbol.paramcnt;
   if( iTotal < 0 )
      iTotal = 0;
   iTotal += usLocals;

   if( iTotal )
   {
      do
         hb_vmPushNil();
      while( --iTotal > 0 );
   }
}

static void hb_vmSFrame( PHB_SYMB pSym )      /* sets the statics frame for a function */
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmSFrame(%p)", pSym));

   /* _INITSTATICS is now the statics frame. Statics() changed it! */
   hb_stackSetStaticsBase( pSym->value.lStaticsBase );
}

static void hb_vmStatics( PHB_SYMB pSym, USHORT uiStatics ) /* initializes the global aStatics array or redimensionates it */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmStatics(%p, %hu)", pSym, uiStatics));

   if( HB_IS_NIL( &s_aStatics ) )
   {
      pSym->value.lStaticsBase = 0;         /* statics frame for this PRG */
      hb_arrayNew( &s_aStatics, uiStatics );
   }
   else
   {
      pSym->value.lStaticsBase = hb_arrayLen( &s_aStatics );
      hb_arraySize( &s_aStatics, ( ULONG ) pSym->value.lStaticsBase + uiStatics );
   }
}

#if defined( HB_MT_VM )
/*
 * extended thread static variable reference structure
 */
typedef struct
{
   HB_ITEM  source;
   HB_TSD   threadData;
} HB_TSVREF, * PHB_TSVREF;

/*
 * extended thread static variable reference functions
 */
static PHB_ITEM hb_vmTSVRefRead( PHB_ITEM pRefer )
{
   PHB_TSVREF pTSVRef = ( PHB_TSVREF ) pRefer->item.asExtRef.value;
   PHB_ITEM pItem = ( PHB_ITEM ) hb_stackTestTSD( &pTSVRef->threadData );
   if( !pItem )
   {
      pItem = ( PHB_ITEM ) hb_stackGetTSD( &pTSVRef->threadData );
      hb_itemCopy( pItem, &pTSVRef->source );
   }
   return pItem;
}

static PHB_ITEM hb_vmTSVRefWrite( PHB_ITEM pRefer, PHB_ITEM pSource )
{
   PHB_TSVREF pTSVRef = ( PHB_TSVREF ) pRefer->item.asExtRef.value;
   HB_SYMBOL_UNUSED( pSource );
   return ( PHB_ITEM ) hb_stackGetTSD( &pTSVRef->threadData );
}

static void hb_vmTSVRefCopy( PHB_ITEM pDest )
{
   hb_xRefInc( pDest->item.asExtRef.value );
}

static void hb_vmTSVRefClear( void * value )
{
   if( hb_xRefDec( value ) )
   {
      PHB_ITEM pItem;

      if( HB_IS_COMPLEX( &( ( PHB_TSVREF ) value )->source ) )
         hb_itemClear( &( ( PHB_TSVREF ) value )->source );

      pItem = ( PHB_ITEM ) hb_stackTestTSD( &( ( PHB_TSVREF ) value )->threadData );
      if( pItem && HB_IS_COMPLEX( pItem ) )
         hb_itemClear( pItem );

      hb_xfree( value );
   }
}

static void hb_vmTSVRefMark( void * value )
{
   PHB_ITEM pItem;

   if( HB_IS_GCITEM( &( ( PHB_TSVREF ) value )->source ) )
      hb_gcItemRef( &( ( PHB_TSVREF ) value )->source );

   pItem = ( PHB_ITEM ) hb_stackTestTSD( &( ( PHB_TSVREF ) value )->threadData );
   if( pItem && HB_IS_GCITEM( pItem ) )
      hb_gcItemRef( pItem );
}

/* destructor for terminated threads */
void hb_vmTSVarClean( void * pThreadItem )
{
   if( HB_IS_COMPLEX( ( PHB_ITEM ) pThreadItem ) )
      hb_itemClear( ( PHB_ITEM ) pThreadItem );
}

/*
 * create extended thread static variable reference
 */
static void hb_vmTSVReference( PHB_ITEM pStatic )
{
   static const HB_EXTREF s_TSVExtRef = {
             hb_vmTSVRefRead,
             hb_vmTSVRefWrite,
             hb_vmTSVRefCopy,
             hb_vmTSVRefClear,
             hb_vmTSVRefMark };

   HB_STACK_TLS_PRELOAD
   PHB_TSVREF pTSVRef;
   PHB_ITEM pRefer;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmTSVReference(%p)", pStatic));

   pTSVRef = ( PHB_TSVREF ) hb_xgrab( sizeof( HB_TSVREF ) );

   pTSVRef->source.type = HB_IT_NIL;
   HB_TSD_INIT( &pTSVRef->threadData, sizeof( HB_ITEM ), NULL, hb_vmTSVarClean );

   /* Use hb_stackReturnItem() as temporary item holder */
   pRefer = hb_stackReturnItem();
   if( HB_IS_COMPLEX( pRefer ) )
      hb_itemClear( pRefer );
   pRefer->type = HB_IT_BYREF | HB_IT_EXTREF;
   pRefer->item.asExtRef.value = ( void * ) pTSVRef;
   pRefer->item.asExtRef.func = &s_TSVExtRef;

   hb_itemMove( &pTSVRef->source, pStatic );
   hb_itemMove( pStatic, pRefer );
}

static void hb_vmInitThreadStatics( USHORT uiCount, const BYTE * pCode )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmInitThreadStatics(%p,%hu,%p)", uiCount, pCode));

   while( uiCount-- )
   {
      USHORT uiStatic = HB_PCODE_MKUSHORT( pCode );
      PHB_ITEM pStatic = s_aStatics.item.asArray.value->pItems + hb_stackGetStaticsBase() + uiStatic - 1;
      hb_vmTSVReference( pStatic );
      pCode += 2;
   }
}
#else
static void hb_vmInitThreadStatics( USHORT uiCount, const BYTE * pCode )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmInitThreadStatics(%p,%hu,%p)", uiCount, pCode));

   /* single thread VM - do nothing, use normal static variables */

   HB_SYMBOL_UNUSED( uiCount );
   HB_SYMBOL_UNUSED( pCode );
}
#endif

static void hb_vmEndBlock( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmEndBlock()"));

   hb_stackPopReturn();
}

static void hb_vmRetValue( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmRetValue()"));

   hb_stackPopReturn();
   hb_stackReturnItem()->type &= ~HB_IT_MEMOFLAG;
}

/* ------------------------------- */
/* Push                            */
/* ------------------------------- */

HB_EXPORT void hb_vmPush( PHB_ITEM pItem )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPush(%p)", pItem));

   hb_itemCopy( hb_stackAllocItem(), pItem );
}

HB_EXPORT void hb_vmPushState( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushState()"));

   hb_stackPushReturn();
}

HB_EXPORT void hb_vmPushNil( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushNil()"));

   hb_stackAllocItem()->type = HB_IT_NIL;
}

HB_EXPORT void hb_vmPushLogical( BOOL bValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLogical(%d)", (int) bValue));

   pItem->type = HB_IT_LOGICAL;
   pItem->item.asLogical.value = bValue;
}

/* not used by HVM code */
HB_EXPORT void hb_vmPushNumber( double dNumber, int iDec )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushNumber(%lf, %d)", dNumber, iDec));

   if( iDec )
      hb_vmPushDouble( dNumber, iDec );

   else if( HB_DBL_LIM_INT( dNumber ) )
      hb_vmPushInteger( ( int ) dNumber );

   else if( HB_DBL_LIM_LONG( dNumber ) )
      hb_vmPushHBLong( ( HB_LONG ) dNumber );

   else
      hb_vmPushDouble( dNumber, hb_stackSetStruct()->HB_SET_DECIMALS );
}

static int hb_vmCalcIntWidth( HB_LONG lNumber )
{
   int iWidth;

   if( lNumber <= -1000000000L )
   {
      iWidth = 20;
   }
   else
   {
      iWidth = 10;
      while( lNumber >= 1000000000L )
      {
         iWidth++;
         lNumber /= 10;
      }
   }
   return iWidth;
}

HB_EXPORT void hb_vmPushInteger( int iNumber )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushInteger(%d)", iNumber));

   pItem->type = HB_IT_INTEGER;
   pItem->item.asInteger.value = iNumber;
   pItem->item.asInteger.length = HB_INT_LENGTH( iNumber );
}

#if HB_INT_MAX >= INT32_MAX
static void hb_vmPushIntegerConst( int iNumber )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushIntegerConst(%d)", iNumber));

   pItem->type = HB_IT_INTEGER;
   pItem->item.asInteger.value = iNumber;
   pItem->item.asInteger.length = hb_vmCalcIntWidth( iNumber );
}
#else
static void hb_vmPushLongConst( long lNumber )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLongConst(%ld)", lNumber));

   pItem->type = HB_IT_LONG;
   pItem->item.asLong.value = ( HB_LONG ) lNumber;
   pItem->item.asLong.length = hb_vmCalcIntWidth( lNumber );
}
#endif


HB_EXPORT void hb_vmPushLong( long lNumber )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLong(%ld)", lNumber));

#if HB_INT_MAX >= LONG_MAX
   pItem->type = HB_IT_INTEGER;
   pItem->item.asInteger.value = ( int ) lNumber;
   pItem->item.asInteger.length = HB_INT_LENGTH( lNumber );
#else
   pItem->type = HB_IT_LONG;
   pItem->item.asLong.value = ( HB_LONG ) lNumber;
   pItem->item.asLong.length = HB_LONG_LENGTH( lNumber );
#endif
}

static void hb_vmPushHBLong( HB_LONG lNumber )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushHBLong(%" PFHL "d)", lNumber));

   pItem->type = HB_IT_LONG;
   pItem->item.asLong.value = lNumber;
   pItem->item.asLong.length = HB_LONG_LENGTH( lNumber );
}

#if !defined( HB_LONG_LONG_OFF )
static void hb_vmPushLongLongConst( LONGLONG llNumber )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLongLongConst(%" PFLL "d)", llNumber));

   pItem->type = HB_IT_LONG;
   pItem->item.asLong.value = ( HB_LONG ) llNumber;
   pItem->item.asLong.length = hb_vmCalcIntWidth( llNumber );
}
#endif

HB_EXPORT void hb_vmPushNumInt( HB_LONG lNumber )
{
   if( HB_LIM_INT( lNumber ) )
      hb_vmPushInteger( ( int ) lNumber );
   else
      hb_vmPushHBLong( lNumber );
}

HB_EXPORT void hb_vmPushDouble( double dNumber, int iDec )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushDouble(%lf, %d)", dNumber, iDec));

   pItem->type = HB_IT_DOUBLE;
   pItem->item.asDouble.value = dNumber;
   pItem->item.asDouble.length = HB_DBL_LENGTH( dNumber );
   if( iDec == HB_DEFAULT_DECIMALS )
      pItem->item.asDouble.decimal = hb_stackSetStruct()->HB_SET_DECIMALS;
   else
      pItem->item.asDouble.decimal = iDec;
}

static void hb_vmPushDoubleConst( double dNumber, int iWidth, int iDec )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushDoubleConst(%lf, %d, %d)", dNumber, iWidth, iDec));

   pItem->type = HB_IT_DOUBLE;
   pItem->item.asDouble.value = dNumber;

   if( iDec == HB_DEFAULT_DECIMALS )
      pItem->item.asDouble.decimal = hb_stackSetStruct()->HB_SET_DECIMALS;
   else
      pItem->item.asDouble.decimal = iDec;

   if( iWidth == HB_DEFAULT_WIDTH )
      pItem->item.asDouble.length = HB_DBL_LENGTH( dNumber );
   else
      pItem->item.asDouble.length = iWidth;
}

HB_EXPORT void hb_vmPushDate( long lDate )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushDate(%ld)", lDate));

   pItem->type = HB_IT_DATE;
   pItem->item.asDate.value = lDate;
}

HB_EXPORT void hb_vmPushPointer( void * pPointer )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushPointer(%ld)", pPointer));

   pItem->type = HB_IT_POINTER;
   pItem->item.asPointer.value = pPointer;
   pItem->item.asPointer.collect =
   pItem->item.asPointer.single = FALSE;
}

HB_EXPORT void hb_vmPushString( const char * szText, ULONG length )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushString(%s, %lu)", szText, length));

   hb_itemPutCL( hb_stackAllocItem(), szText, length );
}

HB_EXPORT void hb_vmPushStringPcode( const char * szText, ULONG length )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushStringPcode(%s, %lu)", szText, length));

   pItem->type = HB_IT_STRING;
   pItem->item.asString.length    = length;
   pItem->item.asString.allocated = 0;
   pItem->item.asString.value     = ( char * ) szText;
}

HB_EXPORT void hb_vmPushSymbol( PHB_SYMB pSym )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushSymbol(%p)", pSym));

   pItem->type = HB_IT_SYMBOL;
   pItem->item.asSymbol.value = pSym;
   pItem->item.asSymbol.stackstate = NULL;
}

HB_EXPORT void hb_vmPushDynSym( PHB_DYNS pDynSym )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushDynSym(%p)", pDynSym));

   pItem->type = HB_IT_SYMBOL;
   pItem->item.asSymbol.value = pDynSym->pSymbol;
   pItem->item.asSymbol.stackstate = NULL;
}

HB_EXPORT void hb_vmPushEvalSym( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushEvalSym()"));

   pItem->type = HB_IT_SYMBOL;
   pItem->item.asSymbol.value = &hb_symEval;
   pItem->item.asSymbol.stackstate = NULL;
}

/* -3    -> HB_P_PUSHBLOCK
 * -2 -1 -> size of codeblock
 *  0 +1 -> number of expected parameters
 * +2 +3 -> number of referenced local variables
 * +4    -> start of table with referenced local variables
 *
 * NOTE: pCode points to static memory
 */
static void hb_vmPushBlock( const BYTE * pCode, PHB_SYMB pSymbols, ULONG ulLen )
{
   HB_STACK_TLS_PRELOAD
   USHORT uiLocals;
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushBlock(%p,%p,%lu)", pCode, pSymbols, ulLen));

   uiLocals = HB_PCODE_MKUSHORT( &pCode[ 2 ] );

   if( ulLen )
      ulLen -= uiLocals << 1;

   pItem->item.asBlock.value =
         hb_codeblockNew( pCode + 4 + ( uiLocals << 1 ),/* pcode buffer         */
         uiLocals,                                      /* number of referenced local variables */
         pCode + 4,                                     /* table with referenced local variables */
         pSymbols,
         ulLen );

   pItem->type = HB_IT_BLOCK;
   /* store the number of expected parameters
    */
   pItem->item.asBlock.paramcnt = HB_PCODE_MKUSHORT( pCode );
   /* store the line number where the codeblock was defined
    */
   pItem->item.asBlock.lineno = hb_stackBaseItem()->item.asSymbol.stackstate->uiLineNo;
   pItem->item.asBlock.hclass = hb_stackBaseItem()->item.asSymbol.stackstate->uiClass;
   pItem->item.asBlock.method = hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;
}

/* -2    -> HB_P_PUSHBLOCKSHORT
 * -1    -> size of codeblock
 *  0    -> start of table with referenced local variables
 *
 * NOTE: pCode points to static memory
 */
static void hb_vmPushBlockShort( const BYTE * pCode, PHB_SYMB pSymbols, ULONG ulLen )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushBlockShort(%p,%p,%hu)", pCode, pSymbols, ulLen));

   pItem->item.asBlock.value =
         hb_codeblockNew( pCode,                    /* pcode buffer         */
         0,                                         /* number of referenced local variables */
         NULL,                                      /* table with referenced local variables */
         pSymbols,
         ulLen );

   pItem->type = HB_IT_BLOCK;

   /* store the number of expected parameters
    */
   pItem->item.asBlock.paramcnt = 0;
   /* store the line number where the codeblock was defined
    */
   pItem->item.asBlock.lineno = hb_stackBaseItem()->item.asSymbol.stackstate->uiLineNo;
   pItem->item.asBlock.hclass = hb_stackBaseItem()->item.asSymbol.stackstate->uiClass;
   pItem->item.asBlock.method = hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;
}

/* -(5|6)     -> HB_P_MPUSHBLOCK[LARGE]
 * [-5] -4 -3 -> size of codeblock
 * -2 -1      -> number of expected parameters
 * +0         -> start of pcode
 *
 * NOTE: pCode points to dynamically allocated memory
 */
static void hb_vmPushMacroBlock( const BYTE * pCode, ULONG ulSize, USHORT usParams )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushMacroBlock(%p,%lu,%hu)", pCode, ulSize, usParams));

   pItem->item.asBlock.value = hb_codeblockMacroNew( pCode, ulSize );
   pItem->type = HB_IT_BLOCK;

   /* store the number of expected parameters
    */
   pItem->item.asBlock.paramcnt = usParams;
   /* store the line number where the codeblock was defined
    */
   pItem->item.asBlock.lineno = hb_stackBaseItem()->item.asSymbol.stackstate->uiLineNo;
   pItem->item.asBlock.hclass = hb_stackBaseItem()->item.asSymbol.stackstate->uiClass;
   pItem->item.asBlock.method = hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;
}

/* pushes current workarea number on the eval stack
 */
static void hb_vmPushAlias( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushAlias()"));

   pItem->type = HB_IT_INTEGER;
   pItem->item.asInteger.value = hb_rddGetCurrentWorkAreaNumber();
   pItem->item.asInteger.length = 10;
}

/* It pops the last item from the stack to use it to select a workarea
 * and next pushes the value of a given field
 * (for performance reason it replaces alias value with field value)
 */
static void hb_vmPushAliasedField( PHB_SYMB pSym )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pAlias;
   int iCurrArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushAliasedField(%p)", pSym));

   iCurrArea = hb_rddGetCurrentWorkAreaNumber();
   pAlias = hb_stackItemFromTop( -1 );

   /*
    * NOTE: hb_vmSelecWorkarea clears passed item
    */
   if( hb_vmSelectWorkarea( pAlias, pSym ) == SUCCESS )
      hb_rddGetFieldValue( pAlias, pSym );

   hb_rddSelectWorkAreaNumber( iCurrArea );
}

/* It pops the last item from the stack to use it to select a workarea
 * and next pushes the value of either a field or a memvar based on alias value
 * (for performance reason it replaces alias value with field value)
 * This is used in the following context:
 * ( any_alias )->variable
 */
static void hb_vmPushAliasedVar( PHB_SYMB pSym )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pAlias = hb_stackItemFromTop( -1 );

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushAliasedVar(%p)", pSym));

   if( HB_IS_STRING( pAlias ) )
   {
      char * szAlias = pAlias->item.asString.value;

      if( szAlias[ 0 ] == 'M' || szAlias[ 0 ] == 'm' )
      {
         if( pAlias->item.asString.length == 1 || /* M->variable */
             ( pAlias->item.asString.length >= 4 &&
               hb_strnicmp( szAlias, "MEMVAR", /* MEMVAR-> or MEMVA-> or MEMV-> */
                                     pAlias->item.asString.length ) == 0 ) )
         {
            hb_memvarGetValue( pAlias, pSym );
            return;
         }
      }
      else if( pAlias->item.asString.length >= 4 &&
               ( hb_strnicmp( szAlias, "FIELD", /* FIELD-> or FIEL-> */
                                       pAlias->item.asString.length ) == 0 ||
                 hb_strnicmp( szAlias, "_FIELD", /* _FIELD-> or _FIE-> */
                                       pAlias->item.asString.length ) == 0 ) )
      {
         hb_rddGetFieldValue( pAlias, pSym );
         return;
      }
   }
   hb_vmPushAliasedField( pSym );
}

static void hb_vmPushLocal( int iLocal )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pLocal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLocal(%d)", iLocal));

   if( iLocal >= 0 )
   {
      /* local variable or local parameter */
      pLocal = hb_stackLocalVariable( &iLocal );
   }
   else
   {
      /* local variable referenced in a codeblock
       * hb_stackSelfItem() points to a codeblock that is currently evaluated
       */
      pLocal = hb_codeblockGetRef( hb_stackSelfItem()->item.asBlock.value, ( LONG ) iLocal );
   }

   hb_itemCopy( hb_stackAllocItem(),
                HB_IS_BYREF( pLocal ) ? hb_itemUnRef( pLocal ) : pLocal );
}

static void hb_vmPushLocalByRef( int iLocal )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR pTop = hb_stackAllocItem();

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLocalByRef(%d)", iLocal));

   /* we store its stack offset instead of a pointer to support a dynamic stack */
   if( iLocal >= 0 )
   {
      PHB_ITEM pLocal = hb_stackLocalVariable( &iLocal );
      if( HB_IS_BYREF( pLocal ) && !HB_IS_ENUM( pLocal ) )
      {
         hb_itemCopy( pTop, pLocal );
         return;
      }
      pTop->item.asRefer.BasePtr.itemsbasePtr = hb_stackItemBasePtr();
   }
   else
   {
      /* store direct codeblock address because an item where a codeblock
       * is stored can be no longer placed on the eval stack at the time
       * of a codeblock evaluation or variable access
       */
      pTop->item.asRefer.BasePtr.block = hb_stackSelfItem()->item.asBlock.value;
   }
   pTop->type = HB_IT_BYREF;
   pTop->item.asRefer.value = iLocal;
   pTop->item.asRefer.offset = hb_stackBaseOffset();
}

static void hb_vmPushStatic( USHORT uiStatic )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pStatic;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushStatic(%hu)", uiStatic));

   pStatic = s_aStatics.item.asArray.value->pItems + hb_stackGetStaticsBase() + uiStatic - 1;
   hb_itemCopy( hb_stackAllocItem(),
                HB_IS_BYREF( pStatic ) ? hb_itemUnRef( pStatic ) : pStatic );
}

static void hb_vmPushStaticByRef( USHORT uiStatic )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR pTop = hb_stackAllocItem();
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushStaticByRef(%hu)", uiStatic));

   pTop->type = HB_IT_BYREF;
   /* we store the offset instead of a pointer to support a dynamic stack */
   pTop->item.asRefer.value = hb_stackGetStaticsBase() + uiStatic - 1;
   pTop->item.asRefer.offset = 0;    /* 0 for static variables */
   pTop->item.asRefer.BasePtr.array = s_aStatics.item.asArray.value;
   hb_gcRefInc( s_aStatics.item.asArray.value );
}

static void hb_vmPushVariable( PHB_SYMB pVarSymb )
{
   HB_STACK_TLS_PRELOAD
   USHORT uiAction = E_DEFAULT;
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_INFO, ("(hb_vmPushVariable)"));

   pItem = hb_stackAllocItem();

   do
   {
      /* First try if passed symbol is a name of field
         * in a current workarea - if it is not a field (FAILURE)
         * then try the memvar variable
         */
      if( hb_rddFieldGet( pItem, pVarSymb ) != SUCCESS )
      {
         if( hb_memvarGet( pItem, pVarSymb ) != SUCCESS )
         {
            HB_ITEM_PTR pError;

            pError = hb_errRT_New( ES_ERROR, NULL, EG_NOVAR, 1003,
                                    NULL, pVarSymb->szName,
                                    0, EF_CANRETRY );

            uiAction = hb_errLaunch( pError );
            hb_errRelease( pError );
         }
      }
   }
   while( uiAction == E_RETRY );
}


static void hb_vmDuplicate( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDuplicate()"));

   pItem = hb_stackItemFromTop( -1 );
   hb_itemCopy( hb_stackAllocItem(), pItem );
}

static void hb_vmDuplUnRef( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDuplUnRef()"));

   pItem = hb_stackItemFromTop( -1 );
   hb_itemCopy( hb_stackAllocItem(), pItem );
   if( HB_IS_BYREF( pItem ) )
      hb_itemCopy( pItem, hb_itemUnRef( pItem ) );
}

static void hb_vmDuplTwo( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDuplTwo()"));

   pItem = hb_stackItemFromTop( -2 );
   hb_itemCopy( hb_stackAllocItem(), pItem );
   pItem = hb_stackItemFromTop( -2 );
   hb_itemCopy( hb_stackAllocItem(), pItem );
}

static void hb_vmPushUnRef( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushUnRef()"));

   pItem = hb_stackItemFromTop( -1 );
   hb_itemCopy( hb_stackAllocItem(),
                HB_IS_BYREF( pItem ) ? hb_itemUnRef( pItem ) : pItem );
}

static void hb_vmSwap( BYTE bCount )
{
   HB_STACK_TLS_PRELOAD
   int i = -1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmSwap(%d)", bCount));

   do
   {
      hb_itemSwap( hb_stackItemFromTop( i ), hb_stackItemFromTop( i - 1 ) );
      --i;
   }
   while( bCount-- );
}

/* ------------------------------- */
/* Pop                             */
/* ------------------------------- */

HB_EXPORT void hb_vmPopState( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE_STEALTH( HB_TR_DEBUG, ( "hb_vmPopState()" ) );

   hb_stackPopReturn();
}

static BOOL hb_vmPopLogical( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopLogical()"));

   if( HB_IS_LOGICAL( hb_stackItemFromTop( -1 ) ) )
   {
      BOOL fValue = hb_stackItemFromTop( -1 )->item.asLogical.value;

      hb_stackDec();
      return fValue;
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 1066, NULL, hb_langDGetErrorDesc( EG_CONDITION ), 1, hb_stackItemFromTop( -1 ) );
      return FALSE;
   }
}

/* NOTE: Type checking should be done by the caller. */

static double hb_vmPopNumber( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;
   double dNumber;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopNumber()"));

   pItem = hb_stackItemFromTop( -1 );

   if( HB_IS_INTEGER( pItem ) )
      dNumber = ( double ) pItem->item.asInteger.value;

   else if( HB_IS_LONG( pItem ) )
      dNumber = ( double ) pItem->item.asLong.value;

   else if( HB_IS_DOUBLE( pItem ) )
      dNumber = pItem->item.asDouble.value;

   else
   {
      hb_errInternal( HB_EI_VMPOPINVITEM, NULL, "hb_vmPopNumber()", NULL );
      dNumber = 0.0; /* To avoid GCC -O2 warning */
   }

   hb_stackDec();

   return dNumber;
}

/* Pops the item from the eval stack and uses it to select the current
 * workarea
 */
static void hb_vmPopAlias( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopAlias()"));

   hb_vmSelectWorkarea( hb_stackItemFromTop( -1 ), NULL ); /* it clears the passed item */
   hb_stackDec();
}

/* Pops the alias to use it to select a workarea and next pops a value
 * into a given field
 */
static void hb_vmPopAliasedField( PHB_SYMB pSym )
{
   HB_STACK_TLS_PRELOAD
   int iCurrArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopAliasedField(%p)", pSym));

   iCurrArea = hb_rddGetCurrentWorkAreaNumber();
   if( hb_vmSelectWorkarea( hb_stackItemFromTop( -1 ), pSym ) == SUCCESS )
      hb_rddPutFieldValue( hb_stackItemFromTop( -2 ), pSym );

   hb_rddSelectWorkAreaNumber( iCurrArea );
   hb_stackDec();    /* alias - it was cleared in hb_vmSelectWorkarea */
   hb_stackPop();    /* field value */
}

/* Pops the alias to use it to select a workarea and next pops a value
 * into either a field or a memvar based on the alias value
 * This is used in the following context:
 * ( any_alias )->variable
 */
static void hb_vmPopAliasedVar( PHB_SYMB pSym )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR pAlias = hb_stackItemFromTop( -1 );

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopAliasedVar(%p)", pSym));

   /*
    * "M", "MEMV" - "MEMVAR" and "FIEL" - "FIELD" are reserved aliases
    */
   if( HB_IS_STRING( pAlias ) )
   {
      char * szAlias = pAlias->item.asString.value;

      if( szAlias[ 0 ] == 'M' || szAlias[ 0 ] == 'm' )
      {
         if( pAlias->item.asString.length == 1 || /* M->variable */
             ( pAlias->item.asString.length >= 4 &&
               hb_strnicmp( szAlias, "MEMVAR", /* MEMVAR-> or MEMVA-> or MEMV-> */
                                     pAlias->item.asString.length ) == 0 ) )
         {
            hb_memvarSetValue( pSym, hb_stackItemFromTop( -2 ) );
            hb_stackPop();    /* alias */
            hb_stackPop();    /* value */
            return;
         }
      }
      else if( pAlias->item.asString.length >= 4 &&
               ( hb_strnicmp( szAlias, "FIELD", /* FIELD-> or FIEL-> */
                                     pAlias->item.asString.length ) == 0 ||
                 hb_strnicmp( szAlias, "_FIELD", /* _FIELD-> or _FIE-> */
                                       pAlias->item.asString.length ) == 0 ) )
      {
         hb_rddPutFieldValue( hb_stackItemFromTop( -2 ), pSym );
         hb_stackPop();    /* alias */
         hb_stackPop();    /* value */
         return;
      }
   }
   hb_vmPopAliasedField( pSym );
}

static void hb_vmPopLocal( int iLocal )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pLocal, pVal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopLocal(%d)", iLocal));

   pVal = hb_stackItemFromTop( -1 );

   /* Remove MEMOFLAG if exists (assignment from field). */
   pVal->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );

   if( iLocal >= 0 )
   {
      /* local variable or local parameter */
      pLocal = hb_stackLocalVariable( &iLocal );
   }
   else
   {
      /* local variable referenced in a codeblock
       * hb_stackSelfItem() points to a codeblock that is currently evaluated
       */
      pLocal = hb_codeblockGetRef( hb_stackSelfItem()->item.asBlock.value, iLocal );
   }

   hb_itemMoveToRef( pLocal, pVal );

   hb_stackDec();
}

static void hb_vmPopStatic( USHORT uiStatic )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pStatic, pVal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopStatic(%hu)", uiStatic));

   pVal = hb_stackItemFromTop( -1 );

   /* Remove MEMOFLAG if exists (assignment from field). */
   pVal->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );
   pStatic = s_aStatics.item.asArray.value->pItems + hb_stackGetStaticsBase() + uiStatic - 1;

   hb_itemMoveToRef( pStatic, pVal );
   hb_stackDec();
}

/* ----------------------------------------------- */
/*
 * Functions to mange module symbols
 */

PHB_SYMB hb_vmGetRealFuncSym( PHB_SYMB pSym )
{
   if( pSym && !( pSym->scope.value & HB_FS_LOCAL ) )
   {
      pSym = pSym->pDynSym &&
           ( pSym->pDynSym->pSymbol->scope.value & HB_FS_LOCAL ) ?
             pSym->pDynSym->pSymbol : NULL;
   }

   return pSym;
}

BOOL hb_vmLockModuleSymbols( void )
{
#if defined( HB_MT_VM )
   return !s_pSymbolsMtx || hb_threadMutexLock( s_pSymbolsMtx );
#else
   return TRUE;
#endif
}

void hb_vmUnlockModuleSymbols( void )
{
#if defined( HB_MT_VM )
   if( s_pSymbolsMtx )
      hb_threadMutexUnlock( s_pSymbolsMtx );
#endif
}

char * hb_vmFindModuleSymbolName( PHB_SYMB pSym )
{
   if( pSym )
   {
      PHB_SYMBOLS pLastSymbols = s_pSymbols;

      while( pLastSymbols )
      {
         if( pLastSymbols->fActive &&
             pSym >= pLastSymbols->pModuleSymbols &&
             pSym < pLastSymbols->pModuleSymbols + pLastSymbols->uiModuleSymbols )
         {
            return pLastSymbols->szModuleName;
         }
         pLastSymbols = pLastSymbols->pNext;
      }
   }
   return NULL;
}

BOOL hb_vmFindModuleSymbols( PHB_SYMB pSym, PHB_SYMB * pSymbols,
                             USHORT * puiSymbols )
{
   if( pSym )
   {
      PHB_SYMBOLS pLastSymbols = s_pSymbols;

/*
      if( pSym->scope.value & HB_FS_PCODEFUNC )
         * pSymbols = pSym->value.pCodeFunc->pSymbols;
*/

      while( pLastSymbols )
      {
         if( pLastSymbols->fActive &&
             pSym >= pLastSymbols->pModuleSymbols &&
             pSym < pLastSymbols->pModuleSymbols + pLastSymbols->uiModuleSymbols )
         {
            * pSymbols = pLastSymbols->pModuleSymbols;
            * puiSymbols = pLastSymbols->uiModuleSymbols;
            return TRUE;
         }
         pLastSymbols = pLastSymbols->pNext;
      }
   }

   * pSymbols = NULL;
   * puiSymbols = 0;
   return FALSE;
}

static PHB_SYMBOLS hb_vmFindFreeModule( PHB_SYMB pSymbols, USHORT uiSymbols,
                                        const char * szModuleName, ULONG ulID )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmFindFreeModule(%p,%hu,%s,%lu)", pSymbols, uiSymbols, szModuleName, ulID));

   if( s_ulFreeSymbols )
   {
      PHB_SYMBOLS pLastSymbols = s_pSymbols;

      while( pLastSymbols )
      {
         if( !pLastSymbols->fActive &&
             pLastSymbols->ulID == ulID &&
             pLastSymbols->uiModuleSymbols == uiSymbols &&
             pLastSymbols->szModuleName != NULL &&
             strcmp( pLastSymbols->szModuleName, szModuleName ) == 0 )
         {
            PHB_SYMB pModuleSymbols = pLastSymbols->pModuleSymbols;
            USHORT ui;

            for( ui = 0; ui < uiSymbols; ++ui )
            {
               if( ( pSymbols[ ui ].scope.value & ~( HB_FS_PCODEFUNC | HB_FS_DYNCODE ) ) !=
                   ( pModuleSymbols[ ui ].scope.value & ~HB_FS_DEFERRED ) ||
                   strcmp( pSymbols[ ui ].szName, pModuleSymbols[ ui ].szName ) != 0 )
               {
                  break;
               }
            }
            if( ui == uiSymbols )
            {
               --s_ulFreeSymbols;
               return pLastSymbols;
            }
         }
         pLastSymbols = pLastSymbols->pNext;
      }
   }

   return NULL;
}

void hb_vmFreeSymbols( PHB_SYMBOLS pSymbols )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmFreeSymbols(%p)", pSymbols));

   if( pSymbols->fActive && hb_vmLockModuleSymbols() )
   {
      if( pSymbols->fActive )
      {
         USHORT ui;

         for( ui = 0; ui < pSymbols->uiModuleSymbols; ++ui )
         {
            HB_SYMBOLSCOPE scope = pSymbols->pModuleSymbols[ ui ].scope.value & HB_FS_INITEXIT;

            /* do not overwrite already initialized statics' frame */
            if( scope != HB_FS_INITEXIT )
            {
               PHB_SYMB pSymbol = &pSymbols->pModuleSymbols[ ui ];
               pSymbol->value.pFunPtr = NULL;
               if( pSymbol->pDynSym && pSymbol->pDynSym->pSymbol != pSymbol &&
                   ( pSymbol->scope.value & HB_FS_LOCAL ) == 0 )
                  pSymbol->scope.value |= HB_FS_DEFERRED;
            }
            pSymbols->pModuleSymbols[ ui ].scope.value &= ~( HB_FS_PCODEFUNC | HB_FS_DYNCODE );
         }
         pSymbols->hDynLib = NULL;
         pSymbols->fActive = FALSE;
         ++s_ulFreeSymbols;
      }
      hb_vmUnlockModuleSymbols();
   }
}

void hb_vmBeginSymbolGroup( void * hDynLib, BOOL fClone )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmBeginSymbolGroup(%p,%d)", hDynLib, (int)fClone));

   s_hDynLibID = hDynLib;
   s_fCloneSym = fClone;
}

void hb_vmInitSymbolGroup( void * hNewDynLib, int argc, char * argv[] )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmInitSymbolGroup(%p,%d,%p)", hNewDynLib, argc, argv));

   s_fCloneSym = FALSE;

   if( s_hDynLibID )
   {
      PHB_SYMBOLS pLastSymbols = s_pSymbols;
      void * hDynLib = s_hDynLibID;
      BOOL fFound = FALSE;
      USHORT ui;

      s_hDynLibID = NULL;

      while( pLastSymbols )
      {
         if( pLastSymbols->hDynLib == hDynLib )
         {
            fFound = TRUE;

            if( pLastSymbols->fInitStatics && pLastSymbols->fActive )
            {
               for( ui = 0; ui < pLastSymbols->uiModuleSymbols; ui++ )
               {
                  HB_SYMBOLSCOPE scope = ( pLastSymbols->pModuleSymbols + ui )->scope.value & HB_FS_INITEXIT;

                  if( scope == HB_FS_INITEXIT )
                  {
                     hb_vmPushSymbol( pLastSymbols->pModuleSymbols + ui );
                     hb_vmPushNil();
                     hb_vmDo( 0 );
                  }
               }
               pLastSymbols->fInitStatics = FALSE;
            }

            pLastSymbols->hDynLib = hNewDynLib;
         }
         pLastSymbols = pLastSymbols->pNext;
      }

      if( fFound )
      {
         pLastSymbols = s_pSymbols;
         while( pLastSymbols )
         {
            if( pLastSymbols->hDynLib == hNewDynLib )
            {
               if( pLastSymbols->fActive && ( pLastSymbols->hScope & HB_FS_INIT ) != 0 )
               {
                  for( ui = 0; ui < pLastSymbols->uiModuleSymbols; ui++ )
                  {
                     HB_SYMBOLSCOPE scope = ( pLastSymbols->pModuleSymbols + ui )->scope.value & HB_FS_INITEXIT;

                     if( scope == HB_FS_INIT )
                     {
                        int i;
                        hb_vmPushSymbol( pLastSymbols->pModuleSymbols + ui );
                        hb_vmPushNil();
                        for( i = 0; i < argc; ++i )
                        {
                           hb_vmPushString( argv[i], strlen( argv[i] ) );
                        }
                        hb_vmDo( ( USHORT ) argc );
                     }
                  }
               }
            }
            pLastSymbols = pLastSymbols->pNext;
         }
      }
   }
}

void hb_vmExitSymbolGroup( void * hDynLib )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmExitSymbolGroup(%p)", hDynLib));

   if( hDynLib )
   {
      PHB_SYMBOLS pLastSymbols = s_pSymbols;
      BOOL fFound = FALSE;

      while( pLastSymbols )
      {
         if( pLastSymbols->hDynLib == hDynLib )
         {
            fFound = TRUE;
            if( pLastSymbols->fActive && ( pLastSymbols->hScope & HB_FS_EXIT ) != 0 )
            {
               USHORT ui;
               for( ui = 0; ui < pLastSymbols->uiModuleSymbols; ui++ )
               {
                  HB_SYMBOLSCOPE scope = ( pLastSymbols->pModuleSymbols + ui )->scope.value & HB_FS_INITEXIT;

                  if( scope == HB_FS_EXIT )
                  {
                     hb_vmPushSymbol( pLastSymbols->pModuleSymbols + ui );
                     hb_vmPushNil();
                     hb_vmDo( 0 );
                  }
               }
            }
         }
         pLastSymbols = pLastSymbols->pNext;
      }

      if( fFound )
      {
         pLastSymbols = s_pSymbols;
         while( pLastSymbols )
         {
            if( pLastSymbols->hDynLib == hDynLib )
            {
               hb_vmFreeSymbols( pLastSymbols );
            }
            pLastSymbols = pLastSymbols->pNext;
         }
      }
   }
}

PHB_SYMBOLS hb_vmRegisterSymbols( PHB_SYMB pModuleSymbols, USHORT uiSymbols,
                                  const char * szModuleName, ULONG ulID,
                                  BOOL fDynLib, BOOL fClone )
{
   PHB_SYMBOLS pNewSymbols;
   BOOL fRecycled, fInitStatics = FALSE;
   USHORT ui;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmRegisterSymbols(%p,%hu,%s,%lu,%d,%d)", pModuleSymbols, uiSymbols, szModuleName, ulID, (int)fDynLib, (int)fClone));

   pNewSymbols = s_ulFreeSymbols == 0 ? NULL :
               hb_vmFindFreeModule( pModuleSymbols, uiSymbols, szModuleName, ulID );

   if( pNewSymbols )
   {
      pNewSymbols->fActive = fRecycled = TRUE;
      pNewSymbols->hDynLib = s_hDynLibID;
      pNewSymbols->hScope = 0;
   }
   else
   {
      fRecycled = FALSE;

      if( fClone )
      {
         PHB_SYMB pSymbols = ( PHB_SYMB ) hb_xgrab( uiSymbols * sizeof( HB_SYMB ) );
         memcpy( pSymbols, pModuleSymbols, uiSymbols * sizeof( HB_SYMB ) );
         for( ui = 0; ui < uiSymbols; ui++ )
         {
            pSymbols[ ui ].szName = hb_strdup( pSymbols[ ui ].szName );
         }
         pModuleSymbols = pSymbols;
      }

      pNewSymbols = ( PHB_SYMBOLS ) hb_xgrab( sizeof( HB_SYMBOLS ) );
      pNewSymbols->pModuleSymbols = pModuleSymbols;
      pNewSymbols->uiModuleSymbols = uiSymbols;
      pNewSymbols->szModuleName = hb_strdup( szModuleName );
      pNewSymbols->ulID = ulID;
      pNewSymbols->fAllocated = fClone;
      pNewSymbols->fActive = TRUE;
      pNewSymbols->fInitStatics = FALSE;
      pNewSymbols->hDynLib = s_hDynLibID;
      pNewSymbols->hScope = 0;
      pNewSymbols->pNext = NULL;

      if( s_pSymbols == NULL )
      {
         s_pSymbols = pNewSymbols;
      }
      else
      {
         PHB_SYMBOLS pLastSymbols = s_pSymbols;

         while( pLastSymbols->pNext ) /* locates the latest processed group of symbols */
            pLastSymbols = pLastSymbols->pNext;
         pLastSymbols->pNext = pNewSymbols;
      }
   }

   for( ui = 0; ui < uiSymbols; ui++ ) /* register each public symbol on the dynamic symbol table */
   {
      PHB_SYMB pSymbol = pNewSymbols->pModuleSymbols + ui;
      HB_SYMBOLSCOPE hSymScope;
      BOOL fPublic, fStatics;

      fStatics = ( pSymbol->scope.value & HB_FS_INITEXIT ) == HB_FS_INITEXIT;

      if( fRecycled && !fStatics )
      {
         pSymbol->value.pFunPtr = ( pModuleSymbols + ui )->value.pFunPtr;
         pSymbol->scope.value = ( pModuleSymbols + ui )->scope.value;
      }
      if( fDynLib )
      {
         pSymbol->scope.value |= HB_FS_DYNCODE;
      }

      hSymScope = pSymbol->scope.value;
      pNewSymbols->hScope |= hSymScope;
      /* fPublic = ( hSymScope & ( HB_FS_PUBLIC | HB_FS_MESSAGE | HB_FS_MEMVAR ) ) != 0; */
      fPublic = ( hSymScope & ( HB_FS_INITEXIT | HB_FS_STATIC ) ) == 0;
      if( fStatics )
      {
         fInitStatics = TRUE;
      }

      if( ( hSymScope & HB_FS_PCODEFUNC ) && ( hSymScope & HB_FS_LOCAL ) &&
          ( fRecycled || fClone ) )
      {
         pSymbol->value.pCodeFunc->pSymbols = pNewSymbols->pModuleSymbols;
      }

      if( !s_pSymStart && !fDynLib &&
          ( hSymScope & HB_FS_FIRST ) != 0 &&
          ( hSymScope & HB_FS_INITEXIT ) == 0 )
      {
         /* first public defined symbol to start execution */
         s_pSymStart = pSymbol;
      }

      /* Enable this code to see static functions which are registered in global dynsym table */
#if 0
      if( fPublic && ( hSymScope & ( HB_FS_INITEXIT | HB_FS_STATIC ) ) != 0 )
      {
         printf("Registring: %s:%s scope %04x\r\n", szModuleName, pSymbol->szName, hSymScope ); fflush(stdout);
      }
#endif

      if( fPublic )
      {
         if( fDynLib && pSymbol->value.pFunPtr )
         {
            PHB_DYNS pDynSym;

            pDynSym = hb_dynsymFind( pSymbol->szName );

            if( pDynSym )
            {
               pSymbol->pDynSym = pDynSym;
               if( pDynSym->pSymbol->value.pFunPtr )
               {
                  pSymbol->scope.value =
                     ( pSymbol->scope.value & ~( HB_FS_PCODEFUNC | HB_FS_LOCAL ) ) |
                     ( pDynSym->pSymbol->scope.value & HB_FS_PCODEFUNC );
                  pSymbol->value.pFunPtr = pDynSym->pSymbol->value.pFunPtr;
               }
               else
               {
                  pDynSym->pSymbol = pSymbol;
               }
               continue;
            }
         }

         hb_dynsymNew( pSymbol );
      }
   }

   if( !fRecycled )
   {
      pNewSymbols->fInitStatics = fInitStatics;
   }

   return pNewSymbols;
}

static void hb_vmVerifyPCodeVersion( const char * szModuleName, USHORT uiPCodeVer )
{
   if( uiPCodeVer != 0 )
   {
      if( uiPCodeVer > HB_PCODE_VER || /* the module is compiled with newer compiler version then HVM */
          uiPCodeVer < HB_PCODE_VER_MIN )  /* the module is compiled with old not longer supported by HVM compiler version */
      {
         char szPCode[ 10 ];
         snprintf( szPCode, sizeof( szPCode ), "%i.%i", uiPCodeVer>>8, uiPCodeVer &0xff );

         hb_errInternal( HB_EI_ERRUNRECOV, "Module '%s'\n"
                         "was compiled with unsupported PCODE version %s.\n"
                         "Please recompile.", szModuleName, szPCode );
      }
   }

}

/*
 * module symbols initialization with extended information
 */
HB_EXPORT PHB_SYMB hb_vmProcessSymbolsEx( PHB_SYMB pSymbols, USHORT uiModuleSymbols,
                                          const char * szModuleName, ULONG ulID,
                                          USHORT uiPCodeVer )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmProcessSymbolsEx(%p,%hu,%s,%lu,%hu)", pSymbols, uiModuleSymbols, szModuleName, ulID, uiPCodeVer));

   hb_vmVerifyPCodeVersion( szModuleName, uiPCodeVer );
   return hb_vmRegisterSymbols( pSymbols, uiModuleSymbols, szModuleName, ulID,
                                s_fCloneSym, s_fCloneSym )->pModuleSymbols;
}

HB_EXPORT PHB_SYMB hb_vmProcessDynLibSymbols( PHB_SYMB pSymbols, USHORT uiModuleSymbols,
                                              const char * szModuleName, ULONG ulID,
                                              USHORT uiPCodeVer )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmProcessDynLibSymbols(%p,%hu,%s,%lu,%hu)", pSymbols, uiModuleSymbols, szModuleName, ulID, uiPCodeVer));

   hb_vmVerifyPCodeVersion( szModuleName, uiPCodeVer );
   return hb_vmRegisterSymbols( pSymbols, uiModuleSymbols, szModuleName, ulID,
                                TRUE, TRUE )->pModuleSymbols;
}

/*
 * old module symbols initialization - do not use it.
 */
HB_EXPORT PHB_SYMB hb_vmProcessSymbols( PHB_SYMB pSymbols, USHORT uiSymbols )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmProcessSymbols(%p,%hu)", pSymbols, uiSymbols));

   return hb_vmRegisterSymbols( pSymbols, uiSymbols, "", 0L,
                                s_fCloneSym, s_fCloneSym )->pModuleSymbols;
}

/*
 * old function - do not use it.
 */
HB_EXPORT PHB_SYMB hb_vmProcessDllSymbols( PHB_SYMB pSymbols, USHORT uiModuleSymbols )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmProcessDllSymbols(%p,%hu)", pSymbols, uiModuleSymbols));

   return hb_vmRegisterSymbols( pSymbols, uiModuleSymbols, "", 0L,
                                TRUE, TRUE )->pModuleSymbols;
}

static void hb_vmReleaseLocalSymbols( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmReleaseLocalSymbols()"));

   while( s_pSymbols )
   {
      PHB_SYMBOLS pDestroy;

      pDestroy = s_pSymbols;
      s_pSymbols = s_pSymbols->pNext;

      if( pDestroy->szModuleName )
      {
         hb_xfree( pDestroy->szModuleName );
      }
      if( pDestroy->fAllocated )
      {
         USHORT ui;
         for( ui = 0; ui < pDestroy->uiModuleSymbols; ++ui )
         {
            PHB_SYMB pSymbol = pDestroy->pModuleSymbols + ui;
            if( pSymbol->pDynSym && pSymbol->pDynSym->pSymbol == pSymbol )
            {
               pSymbol->pDynSym->pSymbol = NULL;
            }
            hb_xfree( ( void * ) pSymbol->szName );
         }
         hb_xfree( pDestroy->pModuleSymbols );
      }
      hb_xfree( pDestroy );
   }
}

/* This calls all _INITSTATICS functions defined in the application.
 * We are using a special symbol's scope HB_FS_INITEXIT to mark
 * this function. These two bits cannot be marked at the same
 * time for normal user defined functions.
 */
static void hb_vmDoInitStatics( void )
{
   PHB_SYMBOLS pLastSymbols = s_pSymbols;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDoInitStatics()"));

   while( pLastSymbols )
   {
      if( pLastSymbols->fInitStatics )
      {
         USHORT ui;

         for( ui = 0; ui < pLastSymbols->uiModuleSymbols; ui++ )
         {
            HB_SYMBOLSCOPE scope = ( pLastSymbols->pModuleSymbols + ui )->scope.value & ( HB_FS_EXIT | HB_FS_INIT );

            if( scope == HB_FS_INITEXIT )
            {
               hb_vmPushSymbol( pLastSymbols->pModuleSymbols + ui );
               hb_vmPushNil();
               hb_vmDo( 0 );
            }
         }
         pLastSymbols->fInitStatics = FALSE;
      }
      pLastSymbols = pLastSymbols->pNext;
   }
}

static void hb_vmDoInitFunctions( void )
{
   PHB_SYMBOLS pLastSymbols = s_pSymbols;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDoInitFunctions()"));

   while( pLastSymbols )
   {
      /* only if module contains some INIT functions */
      if( pLastSymbols->fActive && pLastSymbols->hScope & HB_FS_INIT )
      {
         USHORT ui;

         for( ui = 0; ui < pLastSymbols->uiModuleSymbols; ui++ )
         {
            HB_SYMBOLSCOPE scope = ( pLastSymbols->pModuleSymbols + ui )->scope.value & ( HB_FS_EXIT | HB_FS_INIT );

            if( scope == HB_FS_INIT )
            {
               int argc = hb_cmdargARGC();
               char ** argv = hb_cmdargARGV();

               int i;
               int iArgCount;

               hb_vmPushSymbol( pLastSymbols->pModuleSymbols + ui );
               hb_vmPushNil();

               iArgCount = 0;
               for( i = 1; i < argc; i++ ) /* places application parameters on the stack */
               {
                  /* Filter out any parameters beginning with //, like //INFO */
                  if( ! hb_cmdargIsInternal( argv[ i ], NULL ) )
                  {
                     hb_vmPushString( argv[ i ], strlen( argv[ i ] ) );
                     iArgCount++;
                  }
               }

               hb_vmDo( ( USHORT ) iArgCount );
            }
         }
      }
      pLastSymbols = pLastSymbols->pNext;
   }
}

static void hb_vmDoExitFunctions( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_SYMBOLS pLastSymbols = s_pSymbols;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDoExitFunctions()"));

   /* EXIT procedures should be processed? */
   if( s_fDoExitProc )
   {
      s_fDoExitProc = FALSE;
      hb_stackSetActionRequest( 0 );

      while( pLastSymbols )
      {
         /* only if module contains some EXIT functions */
         if( pLastSymbols->fActive && pLastSymbols->hScope & HB_FS_EXIT )
         {
            USHORT ui;

            for( ui = 0; ui < pLastSymbols->uiModuleSymbols; ui++ )
            {
               HB_SYMBOLSCOPE scope = ( pLastSymbols->pModuleSymbols + ui )->scope.value & ( HB_FS_EXIT | HB_FS_INIT );

               if( scope == HB_FS_EXIT )
               {
                  hb_vmPushSymbol( pLastSymbols->pModuleSymbols + ui );
                  hb_vmPushNil();
                  hb_vmDo( 0 );
                  if( hb_stackGetActionRequest() )
                     /* QUIT or BREAK was issued - stop processing
                     */
                     return;
               }
            }
         }
         pLastSymbols = pLastSymbols->pNext;
      }
   }
}

/* ------------------------------- */
/* Extended references             */
/* ------------------------------- */

/*
 * extended item reference functions
 */
static PHB_ITEM hb_vmItemRefRead( PHB_ITEM pRefer )
{
   return ( PHB_ITEM ) pRefer->item.asExtRef.value;
}

static PHB_ITEM hb_vmItemRefWrite( PHB_ITEM pRefer, PHB_ITEM pSource )
{
   HB_SYMBOL_UNUSED( pSource );
   return ( PHB_ITEM ) pRefer->item.asExtRef.value;
}

static void hb_vmItemRefCopy( PHB_ITEM pDest )
{
   pDest->type = HB_IT_NIL;
   hb_itemCopy( pDest, ( PHB_ITEM ) pDest->item.asExtRef.value );
}

static void hb_vmItemRefDummy( void * value )
{
   HB_SYMBOL_UNUSED( value );
}

/*
 * push extended item reference
 */
HB_EXPORT void hb_vmPushItemRef( PHB_ITEM pItem )
{
   static const HB_EXTREF s_ItmExtRef = {
             hb_vmItemRefRead,
             hb_vmItemRefWrite,
             hb_vmItemRefCopy,
             hb_vmItemRefDummy,
             hb_vmItemRefDummy };

   HB_STACK_TLS_PRELOAD
   PHB_ITEM pRefer;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushItemRef(%p)", pItem));

   pRefer = hb_stackAllocItem();
   pRefer->type = HB_IT_BYREF | HB_IT_EXTREF;
   pRefer->item.asExtRef.value = ( void * ) pItem;
   pRefer->item.asExtRef.func = &s_ItmExtRef;
}

/* ------------------------------- */

/*
 * extended message reference structure
 */
typedef struct
{
   PHB_DYNS access;
   PHB_DYNS assign;
   HB_ITEM  object;
   HB_ITEM  value;
} HB_MSGREF, * PHB_MSGREF;

/*
 * extended message reference functions
 */
static PHB_ITEM hb_vmMsgRefRead( PHB_ITEM pRefer )
{
   PHB_MSGREF pMsgRef = ( PHB_MSGREF ) pRefer->item.asExtRef.value;

   if( hb_vmRequestQuery() == 0 )
   {
      HB_STACK_TLS_PRELOAD

      hb_stackPushReturn();
      if( !pMsgRef->access )
         pMsgRef->access = hb_dynsymGetCase( pMsgRef->assign->pSymbol->szName + 1 );
      hb_vmPushDynSym( pMsgRef->access );
      hb_vmPush( &pMsgRef->object );
      hb_vmSend( 0 );
      hb_itemMove( &pMsgRef->value, hb_stackReturnItem() );
      pMsgRef->value.type |= HB_IT_DEFAULT;
      hb_stackPopReturn();
   }
   return &pMsgRef->value;
}

static PHB_ITEM hb_vmMsgRefWrite( PHB_ITEM pRefer, PHB_ITEM pSource )
{
   PHB_MSGREF pMsgRef = ( PHB_MSGREF ) pRefer->item.asExtRef.value;

   if( hb_vmRequestQuery() == 0 )
   {
      HB_STACK_TLS_PRELOAD

      hb_stackPushReturn();
      hb_vmPushDynSym( pMsgRef->assign );
      hb_vmPush( &pMsgRef->object );
      hb_vmPush( pSource );
      hb_vmSend( 1 );
      hb_itemCopy( &pMsgRef->value, pSource );
      pMsgRef->value.type |= HB_IT_DEFAULT;
      hb_stackPopReturn();
   }
   return NULL; /*&pMsgIdxRef->value;*/
}

static void hb_vmMsgRefCopy( PHB_ITEM pDest )
{
   hb_xRefInc( pDest->item.asExtRef.value );
}

static void hb_vmMsgRefClear( void * value )
{
   if( hb_xRefDec( value ) )
   {
      PHB_MSGREF pMsgRef = ( PHB_MSGREF ) value;
      /* value were change by C code without calling RefWrite(),
       *  f.e. hb_stor*() function
       */
      if( ( pMsgRef->value.type & HB_IT_DEFAULT ) == 0 )
      {
         if( hb_vmRequestReenter() )
         {
            hb_vmPushDynSym( pMsgRef->assign );
            hb_vmPush( &pMsgRef->object );
            hb_vmPush( &pMsgRef->value );
            hb_vmSend( 1 );
            hb_vmRequestRestore();
         }
      }
      if( HB_IS_COMPLEX( &pMsgRef->value ) )
         hb_itemClear( &pMsgRef->value );
      if( HB_IS_COMPLEX( &pMsgRef->object ) )
         hb_itemClear( &pMsgRef->object );
      hb_xfree( value );
   }
}

static void hb_vmMsgRefMark( void * value )
{
   if( HB_IS_GCITEM( &( ( PHB_MSGREF ) value )->object ) )
      hb_gcItemRef( &( ( PHB_MSGREF ) value )->object );
   if( HB_IS_GCITEM( &( ( PHB_MSGREF ) value )->value ) )
      hb_gcItemRef( &( ( PHB_MSGREF ) value )->value );
}

/*
 * create extended message reference
 */
BOOL hb_vmMsgReference( PHB_ITEM pObject, PHB_DYNS pMessage, PHB_DYNS pAccMsg )
{
   static const HB_EXTREF s_MsgExtRef = {
             hb_vmMsgRefRead,
             hb_vmMsgRefWrite,
             hb_vmMsgRefCopy,
             hb_vmMsgRefClear,
             hb_vmMsgRefMark };

   HB_STACK_TLS_PRELOAD
   PHB_MSGREF pMsgRef;
   PHB_ITEM pRefer;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMsgReference(%p,%p,%p)", pObject, pMessage, pAccMsg));

   pMsgRef = ( PHB_MSGREF ) hb_xgrab( sizeof( HB_MSGREF ) );
   pMsgRef->access = pAccMsg;
   pMsgRef->assign = pMessage;
   pMsgRef->value.type = HB_IT_NIL | HB_IT_DEFAULT;
   pMsgRef->object.type = HB_IT_NIL;
   hb_itemMove( &pMsgRef->object, pObject );

   pRefer = hb_stackReturnItem();
   if( HB_IS_COMPLEX( pRefer ) )
      hb_itemClear( pRefer );
   pRefer->type = HB_IT_BYREF | HB_IT_EXTREF;
   pRefer->item.asExtRef.value = ( void * ) pMsgRef;
   pRefer->item.asExtRef.func = &s_MsgExtRef;

   return TRUE;
}

/* ------------------------------- */

/*
 * extended object index reference structure
 */
typedef struct
{
   HB_ITEM  object;
   HB_ITEM  value;
   HB_ITEM  index;
} HB_MSGIDXREF, * PHB_MSGIDXREF;

/*
 * extended object index reference functions
 */
static PHB_ITEM hb_vmMsgIdxRefRead( PHB_ITEM pRefer )
{
   PHB_MSGIDXREF pMsgIdxRef = ( PHB_MSGIDXREF ) pRefer->item.asExtRef.value;

   if( hb_vmRequestQuery() == 0 )
   {
      HB_STACK_TLS_PRELOAD

      hb_stackPushReturn();
      hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, &pMsgIdxRef->value,
                          HB_IS_BYREF( &pMsgIdxRef->object ) ?
                          hb_itemUnRef( &pMsgIdxRef->object ) :
                          &pMsgIdxRef->object, &pMsgIdxRef->index, NULL );
      hb_stackPopReturn();
      pMsgIdxRef->value.type |= HB_IT_DEFAULT;
   }
   return &pMsgIdxRef->value;
}

static PHB_ITEM hb_vmMsgIdxRefWrite( PHB_ITEM pRefer, PHB_ITEM pSource )
{
   PHB_MSGIDXREF pMsgIdxRef = ( PHB_MSGIDXREF ) pRefer->item.asExtRef.value;

   if( hb_vmRequestQuery() == 0 )
   {
      HB_STACK_TLS_PRELOAD
      PHB_ITEM pObject = HB_IS_BYREF( &pMsgIdxRef->object ) ?
                         hb_itemUnRef( &pMsgIdxRef->object ) :
                         &pMsgIdxRef->object;
      hb_stackPushReturn();
      hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pObject, pObject,
                          &pMsgIdxRef->index, pSource );
      hb_stackPopReturn();
      pMsgIdxRef->value.type |= HB_IT_DEFAULT;
   }

   return NULL; /*&pMsgIdxRef->value;*/
}

static void hb_vmMsgIdxRefCopy( PHB_ITEM pDest )
{
   hb_xRefInc( pDest->item.asExtRef.value );
}

static void hb_vmMsgIdxRefClear( void * value )
{
   if( hb_xRefDec( value ) )
   {
      PHB_MSGIDXREF pMsgIdxRef = ( PHB_MSGIDXREF ) value;
      /* value were change by C code without calling RefWrite(),
       *  f.e. hb_stor*() function
       */
      if( ( pMsgIdxRef->value.type & HB_IT_DEFAULT ) == 0 )
      {
         if( hb_vmRequestReenter() )
         {
            PHB_ITEM pObject = HB_IS_BYREF( &pMsgIdxRef->object ) ?
                               hb_itemUnRef( &pMsgIdxRef->object ) :
                               &pMsgIdxRef->object;
            hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pObject, pObject,
                                &pMsgIdxRef->index, &pMsgIdxRef->value );
            hb_vmRequestRestore();
         }
      }
      if( HB_IS_COMPLEX( &pMsgIdxRef->value ) )
         hb_itemClear( &pMsgIdxRef->value );
      if( HB_IS_COMPLEX( &pMsgIdxRef->object ) )
         hb_itemClear( &pMsgIdxRef->object );
      if( HB_IS_COMPLEX( &pMsgIdxRef->index ) )
         hb_itemClear( &pMsgIdxRef->index );
      hb_xfree( value );
   }
}

static void hb_vmMsgIdxRefMark( void * value )
{
   if( HB_IS_GCITEM( &( ( PHB_MSGIDXREF ) value )->object ) )
      hb_gcItemRef( &( ( PHB_MSGIDXREF ) value )->object );
   if( HB_IS_GCITEM( &( ( PHB_MSGIDXREF ) value )->index ) )
      hb_gcItemRef( &( ( PHB_MSGIDXREF ) value )->index );
   if( HB_IS_GCITEM( &( ( PHB_MSGIDXREF ) value )->value ) )
      hb_gcItemRef( &( ( PHB_MSGIDXREF ) value )->value );
}

/*
 * create extended message reference
 */
static void hb_vmMsgIndexReference( PHB_ITEM pRefer, PHB_ITEM pObject, PHB_ITEM pIndex )
{
   static const HB_EXTREF s_MsgIdxExtRef = {
             hb_vmMsgIdxRefRead,
             hb_vmMsgIdxRefWrite,
             hb_vmMsgIdxRefCopy,
             hb_vmMsgIdxRefClear,
             hb_vmMsgIdxRefMark };

   PHB_MSGIDXREF pMsgIdxRef;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMsgIndexReference(%p,%p,%p)", pRefer, pObject, pIndex));

   pMsgIdxRef = ( PHB_MSGIDXREF ) hb_xgrab( sizeof( HB_MSGIDXREF ) );
   pMsgIdxRef->value.type = HB_IT_NIL | HB_IT_DEFAULT;
   pMsgIdxRef->object.type = HB_IT_NIL;
   pMsgIdxRef->index.type = HB_IT_NIL;
   hb_itemCopy( &pMsgIdxRef->object, HB_IS_STRING( pObject ) ? pRefer : pObject );
   hb_itemMove( &pMsgIdxRef->index, pIndex );

   pIndex->type = HB_IT_BYREF | HB_IT_EXTREF;
   pIndex->item.asExtRef.value = ( void * ) pMsgIdxRef;
   pIndex->item.asExtRef.func = &s_MsgIdxExtRef;
   hb_itemMove( pRefer, pIndex );
}

/* ------------------------------- */
/* VM exceptions                   */
/* ------------------------------- */

void hb_vmRequestQuit( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestQuit()"));

   /* In MT mode EXIT functions are executed only from hb_vmQuit()
    * when all other threads have terminated
    */
#if !defined( HB_MT_VM )
   hb_vmDoExitFunctions(); /* process defined EXIT functions */
#endif
   hb_stackSetActionRequest( HB_QUIT_REQUESTED );
}

void hb_vmRequestEndProc( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestEndProc()"));

   hb_stackSetActionRequest( HB_ENDPROC_REQUESTED );
}

void hb_vmRequestBreak( PHB_ITEM pItem )
{
   HB_STACK_TLS_PRELOAD
   ULONG ulRecoverBase;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestBreak(%p)", pItem));

   ulRecoverBase = hb_stackGetRecoverBase();
   while( ulRecoverBase && ( hb_stackItem( ulRecoverBase +
               HB_RECOVER_STATE )->item.asRecover.flags & HB_SEQ_DOALWAYS ) )
   {
      ulRecoverBase = hb_stackItem( ulRecoverBase +
                                    HB_RECOVER_STATE )->item.asRecover.base;
   }

   if( ulRecoverBase )
   {
      if( pItem )
         hb_itemCopy( hb_stackItem( ulRecoverBase + HB_RECOVER_VALUE ), pItem );

      hb_stackSetActionRequest( HB_BREAK_REQUESTED );
   }
   else
   {
#ifdef HB_C52_STRICT
      /*
       * do not execute EXIT procedures to be as close as possible
       * buggy Clipper behavior. [druzus]
       */
      s_fDoExitProc = FALSE;
      hb_stackSetActionRequest( HB_QUIT_REQUESTED );
#else
      /*
       * Clipper has a bug here. Tests shows that it set exception flag
       * and then tries to execute EXIT procedures so the first one is
       * immediately interrupted. Because Clipper does not check the
       * exception flag often enough then it's possible to execute one
       * function from first EXIT PROC. Using small trick with
       * QOUT( TYPE( cPrivateVar ) ) in the EXIT procedure (TYPE() is
       * not normal function) we can also check that it tries to execute
       * EXIT procedures exactly here before leave current function.
       * So to be as close as possible the Clipper intentional behavior
       * we execute hb_vmRequestQuit() here. [druzus]
       */
      hb_vmRequestQuit();
#endif
   }
}

void hb_vmRequestCancel( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestCancel()"));

   if( hb_stackSetStruct()->HB_SET_CANCEL )
   {
      char buffer[ HB_SYMBOL_NAME_LEN + HB_SYMBOL_NAME_LEN + 5 + 10 ]; /* additional 10 bytes for line info (%hu) overhead */
      char file[ _POSIX_PATH_MAX + 1 ];
      USHORT uiLine;
      int iLevel = 0, l;

      hb_conOutErr( hb_conNewLine(), 0 );
      hb_conOutErr( "Cancelled at: ", 0 );

      while( hb_procinfo( iLevel++, buffer, &uiLine, file ) )
      {
         l = strlen( buffer );
         snprintf( buffer + l, sizeof( buffer ) - l, " (%hu)%s%s", uiLine, *file ? HB_I_(" in ") : "", file );

         hb_conOutErr( buffer, 0 );
         hb_conOutErr( hb_conNewLine(), 0 );
      }

      /*
       * Clipper does not execute EXIT procedures when quiting using break key
       */
      s_fDoExitProc = FALSE;
      hb_stackSetActionRequest( HB_QUIT_REQUESTED );
   }
}

USHORT hb_vmRequestQuery( void )
{
   HB_STACK_TLS_PRELOAD
   return hb_stackGetActionRequest();
}

BOOL hb_vmRequestReenter( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestReenter()"));

   if( !s_fHVMActive )
      return FALSE;

   hb_stackPushReturn();

   hb_vmPushInteger( hb_stackGetActionRequest() );
   hb_stackSetActionRequest( 0 );

   return TRUE;
}

void hb_vmRequestRestore( void )
{
   HB_STACK_TLS_PRELOAD
   USHORT uiAction;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestRestore()"));

   uiAction = ( USHORT ) hb_stackItemFromTop( -1 )->item.asInteger.value |
              hb_stackGetActionRequest();
   if( uiAction & HB_QUIT_REQUESTED )
      hb_stackSetActionRequest( HB_QUIT_REQUESTED );
   else if( uiAction & HB_BREAK_REQUESTED )
      hb_stackSetActionRequest( HB_BREAK_REQUESTED );
   else if( uiAction & HB_ENDPROC_REQUESTED )
      hb_stackSetActionRequest( HB_ENDPROC_REQUESTED );
   else
      hb_stackSetActionRequest( 0 );

   hb_stackDec();
   hb_stackPopReturn();
}

PHB_CODEPAGE hb_vmCDP( void )
{
   HB_STACK_TLS_PRELOAD

   return ( PHB_CODEPAGE ) hb_stackGetCDP();
}

void hb_vmSetCDP( PHB_CODEPAGE pCDP )
{
   HB_STACK_TLS_PRELOAD

   hb_stackSetCDP( ( void * ) pCDP );
}

PHB_LANG hb_vmLang( void )
{
   HB_STACK_TLS_PRELOAD

   return ( PHB_LANG ) hb_stackGetLang();
}

void hb_vmSetLang( PHB_LANG pLang )
{
   HB_STACK_TLS_PRELOAD

   hb_stackSetLang( ( void * ) pLang );
}

#if defined( HB_MT_VM )
#  define HB_XVM_RETURN \
      if( hb_vmThreadRequest ) \
         hb_vmRequestTest(); \
      return ( hb_stackGetActionRequest() & \
               ( HB_ENDPROC_REQUESTED | HB_BREAK_REQUESTED | HB_QUIT_REQUESTED ) ) != 0;
#else
#  define HB_XVM_RETURN \
      return ( hb_stackGetActionRequest() & \
               ( HB_ENDPROC_REQUESTED | HB_BREAK_REQUESTED | HB_QUIT_REQUESTED ) ) != 0;
#endif

HB_EXPORT void hb_xvmExitProc( void )
{
   HB_STACK_TLS_PRELOAD

   if( hb_stackGetActionRequest() & HB_ENDPROC_REQUESTED )
      hb_stackSetActionRequest( 0 );
}

HB_EXPORT void hb_xvmEndProc( void )
{
   HB_STACK_TLS_PRELOAD

   if( !( hb_stackGetActionRequest() & ( HB_QUIT_REQUESTED | HB_BREAK_REQUESTED ) ) )
      hb_stackSetActionRequest( HB_ENDPROC_REQUESTED );
}

HB_EXPORT void hb_xvmSeqBegin( void )
{
   HB_STACK_TLS_PRELOAD

   PHB_ITEM pItem;
   /*
    * Create the SEQUENCE envelope
    * To keep compatibility with pure PCODE evaluation we have
    * use exactly the same SEQUENCE envelope or hb_vmRequestBreak()
    * will not work as expected.
    *
    * [ break return value ]  -2
    * [ recover envelope   ]  -1
    * [                    ] <- new recover base
    */

   /* 1) clear the storage for value returned by BREAK statement */
   hb_stackAllocItem()->type = HB_IT_NIL;
   /* 2) recovery state */
   pItem = hb_stackAllocItem();
   /* mark type as NIL - it's not real item */
   pItem->type = HB_IT_NIL;
   /* address of RECOVER or END opcode - not used in C code */
   pItem->item.asRecover.recover = 0;
   /* store current RECOVER base */
   pItem->item.asRecover.base = hb_stackGetRecoverBase();
   /* store current bCanRecover flag - not used in C code */
   pItem->item.asRecover.flags = 0;
   /* clear new recovery state */
   pItem->item.asRecover.request = 0;

   /* set new recover base */
   hb_stackSetRecoverBase( hb_stackTopOffset() );
}

HB_EXPORT BOOL hb_xvmSeqEnd( void )
{
   HB_STACK_TLS_PRELOAD

   /*
    * remove all items placed on the stack after BEGIN code
    */
   hb_stackRemove( hb_stackGetRecoverBase() );

   /*
    * Remove the SEQUENCE envelope
    * This is executed either at the end of sequence or as the
    * response to the break statement if there is no RECOVER clause
    */

   /* 2) Restore previous recovery base address */
   hb_stackSetRecoverBase( hb_stackItemFromTop( -1 )->item.asRecover.base );
   hb_stackDec();
   /* 1) Discard the value returned by BREAK statement */
   hb_stackPop();

#if defined( HB_MT_VM )
   if( hb_vmThreadRequest )
      hb_vmRequestTest();
#endif
   if( hb_stackGetActionRequest() & ( HB_ENDPROC_REQUESTED | HB_QUIT_REQUESTED ) )
      return TRUE;
   else if( hb_stackGetActionRequest() & HB_BREAK_REQUESTED )
      hb_stackSetActionRequest( 0 );
   return FALSE;
}

HB_EXPORT BOOL hb_xvmSeqEndTest( void )
{
   HB_STACK_TLS_PRELOAD

#if defined( HB_MT_VM )
   if( hb_vmThreadRequest )
      hb_vmRequestTest();
#endif
   if( ( hb_stackGetActionRequest() &
       ( HB_ENDPROC_REQUESTED | HB_BREAK_REQUESTED | HB_QUIT_REQUESTED ) ) != 0 )
      return TRUE;

   /*
    * remove all items placed on the stack after BEGIN code
    */
   hb_stackRemove( hb_stackGetRecoverBase() );

   /*
    * Remove the SEQUENCE envelope
    * This is executed either at the end of sequence or as the
    * response to the break statement if there is no RECOVER clause
    */

   /* 2) Restore previous recovery base address */
   hb_stackSetRecoverBase( hb_stackItemFromTop( -1 )->item.asRecover.base );
   hb_stackDec();
   /* 1) Discard the value returned by BREAK statement */
   hb_stackPop();
   return FALSE;
}

HB_EXPORT BOOL hb_xvmSeqRecover( void )
{
   HB_STACK_TLS_PRELOAD

   /*
    * Execute the RECOVER code
    */

   /*
    * remove all items placed on the stack after BEGIN code
    */
   hb_stackRemove( hb_stackGetRecoverBase() );

   /* 2) Restore previous recovery base address */
   hb_stackSetRecoverBase( hb_stackItemFromTop( -1 )->item.asRecover.base );
   hb_stackDec();
   /* 1) Leave the value returned from BREAK */

#if defined( HB_MT_VM )
   if( hb_vmThreadRequest )
      hb_vmRequestTest();
#endif
   if( hb_stackGetActionRequest() & ( HB_ENDPROC_REQUESTED | HB_QUIT_REQUESTED ) )
      return TRUE;
   else if( hb_stackGetActionRequest() & HB_BREAK_REQUESTED )
      hb_stackSetActionRequest( 0 );
   return FALSE;
}

HB_EXPORT void hb_xvmSeqAlways( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmSeqAlways()"));

   /* Create the SEQUENCE ALWAYS envelope */
   /* 1) clear the storage for RETURN value */
   hb_stackAllocItem()->type = HB_IT_NIL;
   /* 2) recovery state */
   pItem = hb_stackAllocItem();
   /* mark type as NIL - it's not real item */
   pItem->type = HB_IT_NIL;
   /* address of RECOVER or END opcode - not used in C code */
   pItem->item.asRecover.recover = 0;
   /* store current RECOVER base */
   pItem->item.asRecover.base = hb_stackGetRecoverBase();
   /* store current bCanRecover flag - not used in C code */
   pItem->item.asRecover.flags = 0;
   /* clear new recovery state */
   pItem->item.asRecover.request = 0;
   /* set sequence type */
   pItem->item.asRecover.flags = HB_SEQ_DOALWAYS;
   /* set new recover base */
   hb_stackSetRecoverBase( hb_stackTopOffset() );
}

HB_EXPORT BOOL hb_xvmAlwaysBegin( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmAlwaysBegin()"));

   /* remove all items placed on the stack after BEGIN code */
   hb_stackRemove( hb_stackGetRecoverBase() );
   /* store and reset action */
   hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.request = hb_stackGetActionRequest();
   hb_stackSetActionRequest( 0 );
   /* store RETURN value */
   if( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.request & HB_ENDPROC_REQUESTED )
      hb_itemMove( hb_stackItemFromTop( HB_RECOVER_VALUE ), hb_stackReturnItem() );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmAlwaysEnd( void )
{
   HB_STACK_TLS_PRELOAD
   USHORT uiPrevAction, uiCurrAction;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmAlwaysEnd()"));

   /* remove all items placed on the stack after ALWAYSBEGIN code */
   hb_stackRemove( hb_stackGetRecoverBase() );
   /* restore previous recovery base address */
   hb_stackSetRecoverBase( hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.base );
   uiCurrAction = hb_stackGetActionRequest();
   uiPrevAction = hb_stackItemFromTop( HB_RECOVER_STATE )->item.asRecover.request;
   /* restore requested action */
   if( ( uiCurrAction | uiPrevAction ) & HB_QUIT_REQUESTED )
      hb_stackSetActionRequest( HB_QUIT_REQUESTED );
   else if( ( uiCurrAction | uiPrevAction ) & HB_BREAK_REQUESTED )
      hb_stackSetActionRequest( HB_BREAK_REQUESTED );
   else if( ( uiCurrAction | uiPrevAction ) & HB_ENDPROC_REQUESTED )
      hb_stackSetActionRequest( HB_ENDPROC_REQUESTED );
   else
      hb_stackSetActionRequest( 0 );
   /* remove the ALWAYS envelope */
   hb_stackDec();
   /* restore RETURN value if not overloaded inside ALWAYS code */
   if( !( uiCurrAction & HB_ENDPROC_REQUESTED ) &&
        ( uiPrevAction & HB_ENDPROC_REQUESTED ) )
      hb_stackPopReturn();
   else
      hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmSeqBlock( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmSeqBlock()"));

   hb_vmSeqBlock();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmEnumStart( BYTE nVars, BYTE nDescend )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmEnumStart(%d,%d)", nVars, nDescend));

   hb_vmEnumStart( nVars, nDescend );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmEnumNext( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmEnumNext()"));

   hb_vmEnumNext();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmEnumPrev( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmEnumPrev()"));

   hb_vmEnumPrev();

   HB_XVM_RETURN
}

HB_EXPORT void hb_xvmEnumEnd( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmEnumEnd()"));

   hb_vmEnumEnd();
}

HB_EXPORT void hb_xvmSetLine( USHORT uiLine )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmSetLine(%hu)", uiLine));

   hb_stackBaseItem()->item.asSymbol.stackstate->uiLineNo = uiLine;
#ifndef HB_NO_DEBUG
   if( hb_stackBaseItem()->item.asSymbol.stackstate->fDebugging )
      hb_vmDebuggerShowLine( uiLine );
#endif
}

HB_EXPORT void hb_xvmFrame( int iLocals, int iParams )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmFrame(%d, %d)", iLocals, iParams));

   hb_vmFrame( ( USHORT ) iLocals, ( BYTE ) iParams );
}

HB_EXPORT void hb_xvmVFrame( int iLocals, int iParams )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmVFrame(%d, %d)", iLocals, iParams));

   hb_vmVFrame( ( USHORT ) iLocals, ( BYTE ) iParams );
}

HB_EXPORT void hb_xvmSFrame( PHB_SYMB pSymbol )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmSFrame(%p)", pSymbol));

   hb_vmSFrame( pSymbol );
}

HB_EXPORT BOOL hb_xvmDo( USHORT uiParams )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDo(%hu)", uiParams));

   hb_vmDo( uiParams );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmFunction( USHORT uiParams )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmFunction(%hu)", uiParams));

   hb_itemSetNil( hb_stackReturnItem() );
   hb_vmDo( uiParams );
   hb_stackPushReturn();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmSend( USHORT uiParams )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmSend(%hu)", uiParams));

   hb_itemSetNil( hb_stackReturnItem() );
   hb_vmSend( uiParams );
   hb_stackPushReturn();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPushObjectVarRef( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushObjectVarRef()"));

   hb_vmPushObjectVarRef();

   HB_XVM_RETURN
}

HB_EXPORT void hb_xvmRetValue( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmRetValue()"));

   hb_stackPopReturn();
   hb_stackReturnItem()->type &= ~HB_IT_MEMOFLAG;
}

HB_EXPORT void hb_xvmStatics( PHB_SYMB pSymbol, USHORT uiStatics )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmStatics(%p,%hu)", pSymbol, uiStatics));

   hb_vmStatics( pSymbol, uiStatics );
}

HB_EXPORT void hb_xvmThreadStatics( USHORT uiStatics, const BYTE * statics )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmThreadStatics(%hu,%p)", uiStatics, statics));

   hb_vmInitThreadStatics( uiStatics, statics );
}

HB_EXPORT void hb_xvmParameter( PHB_SYMB pSymbol, int iParams )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmParameter(%p,%d)", pSymbol, iParams));

   hb_memvarNewParameter( pSymbol, hb_stackItemFromBase( iParams ) );
}

HB_EXPORT void hb_xvmPushLocal( SHORT iLocal )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushLocal(%hd)", iLocal));

   hb_vmPushLocal( iLocal );
}

HB_EXPORT void hb_xvmPushLocalByRef( SHORT iLocal )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushLocalByRef(%hd)", iLocal));

   hb_vmPushLocalByRef( iLocal );
}

HB_EXPORT void hb_xvmPopLocal( SHORT iLocal )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPopLocal(%hd)", iLocal));

   hb_vmPopLocal( iLocal );
}

static PHB_ITEM hb_xvmLocalPtr( int iLocal )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLocalPtr(%d)", iLocal));

   if( iLocal >= 0 )
   {
      /* local variable or local parameter */
      return hb_stackLocalVariable( &iLocal );
   }
   else
   {
      /* local variable referenced in a codeblock
       * hb_stackSelfItem() points to a codeblock that is currently evaluated
       */
      return hb_codeblockGetRef( hb_stackSelfItem()->item.asBlock.value, ( LONG ) iLocal );
   }
}

HB_EXPORT void hb_xvmCopyLocals( int iDest, int iSource )
{
   PHB_ITEM pDest;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmCopyLocals(%d,%d)", iDest, iSource));

   pDest = hb_xvmLocalPtr( iDest );
   hb_itemCopyToRef( hb_xvmLocalPtr( iSource ),
                     HB_IS_BYREF( pDest ) ? hb_itemUnRef( pDest ) : pDest );
}

HB_EXPORT void hb_xvmPushStatic( USHORT uiStatic )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushStatic(%hu)", uiStatic));

   hb_vmPushStatic( uiStatic );
}

HB_EXPORT void hb_xvmPushStaticByRef( USHORT uiStatic )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushStaticByRef(%hu)", uiStatic));

   hb_vmPushStaticByRef( uiStatic );
}

HB_EXPORT void hb_xvmPopStatic( USHORT uiStatic )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPopStatic(%hu)", uiStatic));

   hb_vmPopStatic( uiStatic );
}

HB_EXPORT BOOL hb_xvmPushVariable( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPushVariable(%p)", pSymbol));

   hb_vmPushVariable( pSymbol );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPopVariable( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPopVariable(%p)", pSymbol));

   /* See the note above in HB_P_POPVARIABLE */
#if 0
   if( pSymbol->pDynSym && hb_dynsymGetMemvar( pSymbol->pDynSym ) )
      hb_memvarSetValue( pSymbol, hb_stackItemFromTop(-1) );
   else if( hb_rddFieldPut( hb_stackItemFromTop(-1), pSymbol ) == FAILURE )
#endif
      hb_memvarSetValue( pSymbol, hb_stackItemFromTop(-1) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT void hb_xvmPushBlockShort( const BYTE * pCode, PHB_SYMB pSymbols )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushBlockShort(%p, %p)", pCode, pSymbols));

   hb_vmPushBlockShort( pCode, pSymbols, FALSE );
}

HB_EXPORT void hb_xvmPushBlock( const BYTE * pCode, PHB_SYMB pSymbols )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushBlock(%p, %p)", pCode, pSymbols));

   hb_vmPushBlock( pCode, pSymbols, FALSE );
}

HB_EXPORT void hb_xvmPushSelf( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushSelf()"));

   hb_vmPush( hb_stackSelfItem() );
}

HB_EXPORT void hb_xvmPushFuncSymbol( PHB_SYMB pSym )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushFuncSymbol(%p)", pSym));

   pItem = hb_stackAllocItem();
   pItem->type = HB_IT_SYMBOL;
   pItem->item.asSymbol.value = pSym;
   pItem->item.asSymbol.stackstate = NULL;
   hb_stackAllocItem()->type = HB_IT_NIL;
}

HB_EXPORT BOOL hb_xvmPopLogical( BOOL * pfValue )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPopLogical(%p)", pfValue));

   *pfValue = hb_vmPopLogical();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPopAlias( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPopAlias()"));

   hb_vmSelectWorkarea( hb_stackItemFromTop( -1 ), NULL ); /* it clears the passed item */
   hb_stackDec();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmSwapAlias( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmSwapAlias()"));

   hb_vmSwapAlias();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPushField( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPushField(%p)", pSymbol));

   hb_rddGetFieldValue( hb_stackAllocItem(), pSymbol );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPushAlias( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushAlias()"));

   hb_vmPushAlias();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPushAliasedField( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPushAliasedField(%p)", pSymbol));

   hb_vmPushAliasedField( pSymbol );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPushAliasedVar( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPushAliasedVar(%p)", pSymbol));

   hb_vmPushAliasedVar( pSymbol );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPopField( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPopField(%p)", pSymbol));

   hb_rddPutFieldValue( hb_stackItemFromTop(-1), pSymbol );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPushMemvar( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPushMemvar(%p)", pSymbol));

   hb_memvarGetValue( hb_stackAllocItem(), pSymbol );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPushMemvarByRef( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPushMemvarByRef(%p)", pSymbol));

   hb_memvarGetRefer( hb_stackAllocItem(), pSymbol );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPopMemvar( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPopMemvar(%p)", pSymbol));

   hb_memvarSetValue( pSymbol, hb_stackItemFromTop(-1) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPopAliasedField( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPopAliasedField(%p)", pSymbol));

   hb_vmPopAliasedField( pSymbol );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPopAliasedVar( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_INFO, ("hb_xvmPopAliasedVar(%p)", pSymbol));

   hb_vmPopAliasedVar( pSymbol );

   HB_XVM_RETURN
}

HB_EXPORT void hb_xvmLocalSetInt( int iLocal, LONG lValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pLocal;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLocalSetInt(%d, %d)", iLocal, lValue));

   if( iLocal >= 0 )
   {
      /* local variable or local parameter */
      pLocal = hb_stackLocalVariable( &iLocal );
      if( HB_IS_BYREF( pLocal ) )
         pLocal = hb_itemUnRef( pLocal );
   }
   else
   {
      /* local variable referenced in a codeblock
       * hb_stackSelfItem() points to a codeblock that is currently evaluated
       */
      pLocal = hb_codeblockGetVar( hb_stackSelfItem(), iLocal );
   }

   if( HB_IS_OBJECT( pLocal ) && hb_objHasOperator( pLocal, HB_OO_OP_ASSIGN ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_ASSIGN, pLocal, pLocal,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
   }
   else
   {
      hb_itemPutNL( pLocal, lValue );
   }
}

HB_EXPORT BOOL hb_xvmLocalAddInt( int iLocal, LONG lAdd )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLocalAddInt(%d,%ld)", iLocal, lAdd));

   hb_vmAddInt( hb_stackLocalVariable( &iLocal ), lAdd );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLocalInc( int iLocal )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pLocal;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLocalInc(%d)", iLocal));

   pLocal = hb_stackLocalVariable( &iLocal );
   hb_vmInc( HB_IS_BYREF( pLocal ) ? hb_itemUnRef( pLocal ) : pLocal );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLocalDec( int iLocal )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pLocal;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLocalDec(%d)", iLocal));

   pLocal = hb_stackLocalVariable( &iLocal );
   hb_vmDec( HB_IS_BYREF( pLocal ) ? hb_itemUnRef( pLocal ) : pLocal );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLocalIncPush( int iLocal )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pLocal;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLocalInc(%d)", iLocal));

   pLocal = hb_stackLocalVariable( &iLocal );
   if( HB_IS_BYREF( pLocal ) )
      pLocal = hb_itemUnRef( pLocal );
   hb_vmInc( pLocal );
   hb_itemCopy( hb_stackAllocItem(), pLocal );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLocalAdd( int iLocal )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pLocal;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLocalAdd(%d)", iLocal));

   pLocal = hb_stackLocalVariable( &iLocal );
   if( HB_IS_BYREF( pLocal ) )
      pLocal = hb_itemUnRef( pLocal );
   hb_vmPlus( pLocal, hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
   hb_stackPop();
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmStaticAdd( USHORT uiStatic )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pStatic;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmStaticAdd(%hu)", uiStatic));

   pStatic = s_aStatics.item.asArray.value->pItems + hb_stackGetStaticsBase() + uiStatic - 1;
   if( HB_IS_BYREF( pStatic ) )
      pStatic = hb_itemUnRef( pStatic );
   hb_vmPlus( pStatic, hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
   hb_stackPop();
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMemvarAdd( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pMemVar, pVal1, pVal2;

   HB_TRACE(HB_TR_INFO, ("hb_xvmMemvarAdd(%p)", pSymbol));

   pVal1 = hb_stackItemFromTop( -2 );
   pVal2 = hb_stackItemFromTop( -1 );
   if( HB_IS_STRING( pVal1 ) && HB_IS_STRING( pVal2 ) )
   {
      pMemVar = hb_memvarGetItem( pSymbol );
      if( pMemVar )
      {
         hb_vmPlus( pMemVar, pVal1, pVal2 );
         hb_stackPop();
         hb_stackPop();
         HB_XVM_RETURN
      }
   }

   hb_vmPlus( pVal1, pVal1, pVal2 );
   hb_memvarSetValue( pSymbol, pVal1 );
   hb_stackPop();
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmAnd( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmAnd()"));

   hb_vmAnd();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmOr( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmOr()"));

   hb_vmOr();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmNot( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmNot()"));

   hb_vmNot();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmNegate( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmNegate()"));

   hb_vmNegate();

   HB_XVM_RETURN
}

HB_EXPORT void hb_xvmDuplicate( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDuplicate()"));

   hb_vmDuplicate();
}

HB_EXPORT void hb_xvmDuplUnRef( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDuplUnRef()"));

   hb_vmDuplUnRef();
}

HB_EXPORT void hb_xvmDuplTwo( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDuplTwo()"));

   hb_vmDuplTwo();
}

HB_EXPORT void hb_xvmPushUnRef( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushUnRef()"));

   hb_vmPushUnRef();
}

HB_EXPORT void hb_xvmSwap( int iCount )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmSwap(%d)", iCount));

   hb_vmSwap( ( BYTE ) iCount );
}

HB_EXPORT BOOL hb_xvmForTest( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmForTest()"));

   hb_vmForTest();

   HB_XVM_RETURN
}

HB_EXPORT void hb_xvmFuncPtr( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmFuncPtr()"));

   hb_vmFuncPtr();
}

HB_EXPORT BOOL hb_xvmEqual( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmEqual()"));

   hb_vmEqual();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmExactlyEqual( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmExactlyEqual()"));

   hb_vmExactlyEqual();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmEqualInt( LONG lValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmEqualInt(%ld)", lValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NIL( pItem ) )
   {
      pItem->type = HB_IT_LOGICAL;
      pItem->item.asLogical.value = FALSE;
   }
   else if( HB_IS_NUMINT( pItem ) )
   {
      pItem->item.asLogical.value = HB_ITEM_GET_NUMINTRAW( pItem ) == ( HB_LONG ) lValue;
      pItem->type = HB_IT_LOGICAL;
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      double dNumber = hb_vmPopNumber();
      hb_vmPushLogical( dNumber == ( double ) lValue );
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_EQUAL ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_EQUAL, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1071, NULL, "=", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmEqualIntIs( LONG lValue, BOOL * pfValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmEqualIntIs(%ld,%p)", lValue, pfValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NIL( pItem ) )
   {
      * pfValue = FALSE;
      hb_stackDec();
   }
   else if( HB_IS_NUMINT( pItem ) )
   {
      * pfValue = HB_ITEM_GET_NUMINTRAW( pItem ) == ( HB_LONG ) lValue;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      * pfValue = hb_vmPopNumber() == ( double ) lValue;
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_EQUAL ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_EQUAL, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
      return hb_xvmPopLogical( pfValue );
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1071, NULL, "=", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
         return hb_xvmPopLogical( pfValue );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmNotEqual( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmNotEqual()"));

   hb_vmNotEqual();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmNotEqualInt( LONG lValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmNotEqualInt(%ld)", lValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NIL( pItem ) )
   {
      pItem->type = HB_IT_LOGICAL;
      pItem->item.asLogical.value = TRUE;
   }
   else if( HB_IS_NUMINT( pItem ) )
   {
      pItem->item.asLogical.value = HB_ITEM_GET_NUMINTRAW( pItem ) != ( HB_LONG ) lValue;
      pItem->type = HB_IT_LOGICAL;
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      double dNumber = hb_vmPopNumber();
      hb_vmPushLogical( dNumber != ( double ) lValue );
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_NOTEQUAL ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_NOTEQUAL, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1072, NULL, "<>", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmNotEqualIntIs( LONG lValue, BOOL * pfValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmNotEqualIntIs(%ld,%p)", lValue, pfValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NIL( pItem ) )
   {
      * pfValue = TRUE;
      hb_stackDec();
   }
   else if( HB_IS_NUMINT( pItem ) )
   {
      * pfValue = HB_ITEM_GET_NUMINTRAW( pItem ) != ( HB_LONG ) lValue;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      * pfValue = hb_vmPopNumber() != ( double ) lValue;
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_NOTEQUAL ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_NOTEQUAL, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
      return hb_xvmPopLogical( pfValue );
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1072, NULL, "<>", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
         return hb_xvmPopLogical( pfValue );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLess( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLess()"));

   hb_vmLess();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLessThenInt( LONG lValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLessThenInt(%ld)", lValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NUMINT( pItem ) )
   {
      pItem->item.asLogical.value = HB_ITEM_GET_NUMINTRAW( pItem ) < ( HB_LONG ) lValue;
      pItem->type = HB_IT_LOGICAL;
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      hb_vmPushLogical( hb_vmPopNumber() < ( double ) lValue );
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_LESS ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_LESS, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1073, NULL, "<", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLessThenIntIs( LONG lValue, BOOL * pfValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLessThenIntIs(%ld,%p)", lValue, pfValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NUMINT( pItem ) )
   {
      * pfValue = HB_ITEM_GET_NUMINTRAW( pItem ) < ( HB_LONG ) lValue;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      * pfValue = hb_vmPopNumber() < ( double ) lValue;
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_LESS ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_LESS, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
      return hb_xvmPopLogical( pfValue );
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1073, NULL, "<", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
         return hb_xvmPopLogical( pfValue );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLessEqual( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLessEqual()"));

   hb_vmLessEqual();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLessEqualThenInt( LONG lValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLessEqualThenInt(%ld)", lValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NUMINT( pItem ) )
   {
      pItem->item.asLogical.value = HB_ITEM_GET_NUMINTRAW( pItem ) <= ( HB_LONG ) lValue;
      pItem->type = HB_IT_LOGICAL;
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      hb_vmPushLogical( hb_vmPopNumber() <= ( double ) lValue );
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_LESSEQUAL ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_LESSEQUAL, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1074, NULL, "<=", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmLessEqualThenIntIs( LONG lValue, BOOL * pfValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLessEqualThenIntIs(%ld,%p)", lValue, pfValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NUMINT( pItem ) )
   {
      * pfValue = HB_ITEM_GET_NUMINTRAW( pItem ) <= ( HB_LONG ) lValue;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      * pfValue = hb_vmPopNumber() <= ( double ) lValue;
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_LESSEQUAL ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_LESSEQUAL, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
      return hb_xvmPopLogical( pfValue );
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1074, NULL, "<=", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
         return hb_xvmPopLogical( pfValue );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmGreater( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmGreater()"));

   hb_vmGreater();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmGreaterThenInt( LONG lValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmGreaterThenInt(%ld)", lValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NUMINT( pItem ) )
   {
      pItem->item.asLogical.value = HB_ITEM_GET_NUMINTRAW( pItem ) > ( HB_LONG ) lValue;
      pItem->type = HB_IT_LOGICAL;
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      hb_vmPushLogical( hb_vmPopNumber() > ( double ) lValue );
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_GREATER ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_GREATER, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1075, NULL, ">", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmGreaterThenIntIs( LONG lValue, BOOL * pfValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmGreaterThenIntIs(%ld,%p)", lValue, pfValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NUMINT( pItem ) )
   {
      * pfValue = HB_ITEM_GET_NUMINTRAW( pItem ) > ( HB_LONG ) lValue;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      * pfValue = hb_vmPopNumber() > ( double ) lValue;
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_GREATER ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_GREATER, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
      return hb_xvmPopLogical( pfValue );
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1075, NULL, ">", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
         return hb_xvmPopLogical( pfValue );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmGreaterEqual( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmGreaterEqual()"));

   hb_vmGreaterEqual();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmGreaterEqualThenInt( LONG lValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmGreaterEqualThenInt(%ld)", lValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NUMINT( pItem ) )
   {
      pItem->item.asLogical.value = HB_ITEM_GET_NUMINTRAW( pItem ) >= ( HB_LONG ) lValue;
      pItem->type = HB_IT_LOGICAL;
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      hb_vmPushLogical( hb_vmPopNumber() >= ( double ) lValue );
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_GREATEREQUAL ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_GREATEREQUAL, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1076, NULL, ">=", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmGreaterEqualThenIntIs( LONG lValue, BOOL * pfValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmGreaterEqualThenIntIs(%ld,%p)", lValue, pfValue));

   pItem = hb_stackItemFromTop( -1 );
   if( HB_IS_NUMINT( pItem ) )
   {
      * pfValue = HB_ITEM_GET_NUMINTRAW( pItem ) >= ( HB_LONG ) lValue;
      hb_stackDec();
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      * pfValue = hb_vmPopNumber() >= ( double ) lValue;
   }
   else if( hb_objHasOperator( pItem, HB_OO_OP_GREATEREQUAL ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_GREATEREQUAL, pItem, pItem,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
      return hb_xvmPopLogical( pfValue );
   }
   else
   {
      PHB_ITEM pResult;

      hb_vmPushLong( lValue );
      pResult = hb_errRT_BASE_Subst( EG_ARG, 1074, NULL, "<=", 2, pItem, hb_stackItemFromTop( -1 ) );

      if( pResult )
      {
         hb_stackPop();
         hb_itemMove( pItem, pResult );
         hb_itemRelease( pResult );
         return hb_xvmPopLogical( pfValue );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmInstring( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmInstring()"));

   hb_vmInstring();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmAddInt( LONG lAdd )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmAddInt(%ld)", lAdd));

   hb_vmAddInt( hb_stackItemFromTop( -1 ), lAdd );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPlus( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPlus()"));

   hb_vmPlus( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ),
              hb_stackItemFromTop( -1 ) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPlusEq( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult, pValue;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPlusEq()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   pValue = hb_stackItemFromTop( -1 );
   hb_vmPlus( pResult, pResult, pValue );
   hb_itemCopy( pValue, pResult );
   hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPlusEqPop( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPlusEqPop()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   hb_vmPlus( pResult, pResult, hb_stackItemFromTop( -1 ) );
   hb_stackPop();
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMinus( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMinus()"));

   hb_vmMinus( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ),
               hb_stackItemFromTop( -1 ) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMinusEq( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult, pValue;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMinusEq()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   pValue = hb_stackItemFromTop( -1 );
   hb_vmMinus( pResult, pResult, pValue );
   hb_itemCopy( pValue, pResult );
   hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMinusEqPop( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMinusEqPop()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   hb_vmMinus( pResult, pResult, hb_stackItemFromTop( -1 ) );
   hb_stackPop();
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMultByInt( LONG lValue )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pValue;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMultByInt(%ld)", lValue));

   pValue = hb_stackItemFromTop( -1 );

   if( HB_IS_NUMERIC( pValue ) )
   {
      int iDec;
      double dValue = hb_itemGetNDDec( pValue, &iDec );

      hb_itemPutNumType( pValue, dValue * lValue, iDec,
                         HB_ITEM_TYPERAW( pValue ), HB_IT_INTEGER );
   }
   else if( hb_objHasOperator( pValue, HB_OO_OP_MULT ) )
   {
      hb_vmPushLong( lValue );
      hb_objOperatorCall( HB_OO_OP_MULT, pValue, pValue,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
   }
   else
   {
      PHB_ITEM pSubst;

      hb_vmPushLong( lValue );
      pSubst = hb_errRT_BASE_Subst( EG_ARG, 1083, NULL, "*", 2, pValue, hb_stackItemFromTop( -1 ) );

      if( pSubst )
      {
         hb_stackPop();
         hb_itemMove( pValue, pSubst );
         hb_itemRelease( pSubst );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMult( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMult()"));

   hb_vmMult( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMultEq( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult, pValue;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMultEq()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   pValue = hb_stackItemFromTop( -1 );
   hb_vmMult( pResult, pResult, pValue );
   hb_itemCopy( pValue, pResult );
   hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMultEqPop( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMultEqPop()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   hb_vmMult( pResult, pResult, hb_stackItemFromTop( -1 ) );
   hb_stackPop();
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmDivideByInt( LONG lDivisor )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pValue;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDivideByInt(%ld)", lDivisor));

   pValue = hb_stackItemFromTop( -1 );

   if( HB_IS_NUMERIC( pValue ) )
   {
      if( lDivisor == 0 )
      {
         PHB_ITEM pSubst;

         hb_vmPushLong( lDivisor );
         pSubst = hb_errRT_BASE_Subst( EG_ZERODIV, 1340, NULL, "/", 2, pValue, hb_stackItemFromTop( -1 ) );

         if( pSubst )
         {
            hb_stackPop();
            hb_itemMove( pValue, pSubst );
            hb_itemRelease( pSubst );
         }
      }
      else
      {
         hb_itemPutND( pValue, hb_itemGetND( pValue ) / lDivisor );
      }
   }
   else if( hb_objHasOperator( pValue, HB_OO_OP_DIVIDE ) )
   {
      hb_vmPushLong( lDivisor );
      hb_objOperatorCall( HB_OO_OP_DIVIDE, pValue, pValue,
                          hb_stackItemFromTop( -1 ), NULL );
      hb_stackPop();
   }
   else
   {
      PHB_ITEM pSubst;

      hb_vmPushLong( lDivisor );
      pSubst = hb_errRT_BASE_Subst( EG_ARG, 1084, NULL, "/", 2, pValue, hb_stackItemFromTop( -1 ) );

      if( pSubst )
      {
         hb_stackPop();
         hb_itemMove( pValue, pSubst );
         hb_itemRelease( pSubst );
      }
   }

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmDivide( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDivide()"));

   hb_vmDivide( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmDivEq( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult, pValue;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDivEq()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   pValue = hb_stackItemFromTop( -1 );
   hb_vmDivide( pResult, pResult, pValue );
   hb_itemCopy( pValue, pResult );
   hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmDivEqPop( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDivEqPop()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   hb_vmDivide( pResult, pResult, hb_stackItemFromTop( -1 ) );
   hb_stackPop();
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmModulus( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmModulus()"));

   hb_vmModulus( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmModEq( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult, pValue;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmModEq()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   pValue = hb_stackItemFromTop( -1 );
   hb_vmModulus( pResult, pResult, pValue );
   hb_itemCopy( pValue, pResult );
   hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmModEqPop( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmModEqPop()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   hb_vmModulus( pResult, pResult, hb_stackItemFromTop( -1 ) );
   hb_stackPop();
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmPower( void )
{
   HB_STACK_TLS_PRELOAD
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPower()"));

   hb_vmPower( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmExpEq( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult, pValue;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmExpEq()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   pValue = hb_stackItemFromTop( -1 );
   hb_vmPower( pResult, pResult, pValue );
   hb_itemCopy( pValue, pResult );
   hb_itemMove( hb_stackItemFromTop( -2 ), pValue );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmExpEqPop( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmExpEqPop()"));

   pResult = hb_itemUnRef( hb_stackItemFromTop( -2 ) );
   hb_vmPower( pResult, pResult, hb_stackItemFromTop( -1 ) );
   hb_stackPop();
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmInc( void )
{
   HB_STACK_TLS_PRELOAD
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmInc()"));

   hb_vmInc( hb_stackItemFromTop( -1 ) );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmIncEq( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult, pValue, pTemp;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmIncEq()"));

   pResult = hb_stackItemFromTop( -1 );
   pValue = hb_itemUnRef( pResult );
   hb_vmInc( pValue );
   pTemp = hb_stackAllocItem();
   hb_itemCopy( pTemp, pValue );
   hb_itemMove( pResult, pTemp );
   hb_stackDec();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmIncEqPop( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmIncEqPop()"));

   hb_vmInc( hb_itemUnRef( hb_stackItemFromTop( -1 ) ) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmDec( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDec()"));

   hb_vmDec( hb_stackItemFromTop( -1 ) );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmDecEq( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pResult, pValue, pTemp;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDecEq()"));

   pResult = hb_stackItemFromTop( -1 );
   pValue = hb_itemUnRef( pResult );
   hb_vmDec( pValue );
   pTemp = hb_stackAllocItem();
   hb_itemCopy( pTemp, pValue );
   hb_itemMove( pResult, pTemp );
   hb_stackDec();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmDecEqPop( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmDecEqPop()"));

   hb_vmDec( hb_itemUnRef( hb_stackItemFromTop( -1 ) ) );
   hb_stackPop();

   HB_XVM_RETURN
}

HB_EXPORT void hb_xvmArrayDim( USHORT uiDimensions )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmArrayDim(%hu)", uiDimensions));

   hb_vmArrayDim( uiDimensions );
}

HB_EXPORT void hb_xvmArrayGen( ULONG ulElements )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmArrayGen(%lu)", ulElements));

   hb_vmArrayGen( ulElements );
}

HB_EXPORT void hb_xvmHashGen( ULONG ulElements )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmHashGen(%lu)", ulElements));

   hb_vmHashGen( ulElements );
}

static void hb_vmArrayItemPush( ULONG ulIndex )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pArray;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayItemPush(%lu)", ulIndex));

   pArray = hb_stackItemFromTop( -1 );

   if( HB_IS_ARRAY( pArray ) )
   {
      if( HB_IS_OBJECT( pArray ) && hb_objHasOperator( pArray, HB_OO_OP_ARRAYINDEX ) )
      {
         hb_vmPushNumInt( ulIndex );
         hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray,
                             hb_stackItemFromTop( -1 ), NULL );
         hb_stackPop();
         return;
      }

      if( HB_IS_VALID_INDEX( ulIndex, pArray->item.asArray.value->ulLen ) )
      {
         PHB_ITEM pItem = hb_stackAllocItem();

         hb_itemCopy( pItem, pArray->item.asArray.value->pItems + ulIndex - 1 );
         hb_itemMove( pArray, pItem );
         hb_stackDec();
      }
      else
      {
         hb_vmPushNumInt( ulIndex );
         if( !HB_IS_OBJECT( pArray ) &&
             hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray,
                                 hb_stackItemFromTop( -1 ), NULL ) )
            hb_stackPop();
         else
#ifdef HB_C52_STRICT
            hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 0 );
#else
            hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ),
                           2, pArray, hb_stackItemFromTop( -1 ) );
#endif
      }
   }
   else if( HB_IS_HASH( pArray ) )
   {
      PHB_ITEM pValue;

      hb_vmPushNumInt( ulIndex );
      pValue = hb_hashGetItemPtr( pArray, hb_stackItemFromTop( -1 ), HB_HASH_AUTOADD_ACCESS );

      if( pValue )
      {
         hb_itemCopy( hb_stackItemFromTop( -1 ), pValue );
         hb_itemMove( pArray, hb_stackItemFromTop( -1 ) );
         hb_stackDec();
      }
      else if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray,
                                   hb_stackItemFromTop( -1 ), NULL ) )
         hb_stackPop();
      else
         hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, hb_stackItemFromTop( -1 ) );
   }
   else
   {
      hb_vmPushNumInt( ulIndex );
      if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray,
                              hb_stackItemFromTop( -1 ), NULL ) )
         hb_stackPop();
      else
         hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, hb_stackItemFromTop( -1 ) );
   }
}

static void hb_vmArrayItemPop( ULONG ulIndex )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pValue;
   PHB_ITEM pArray;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayItemPop(%lu", ulIndex));

   pValue = hb_stackItemFromTop( -2 );
   pArray = hb_stackItemFromTop( -1 );

   if( HB_IS_BYREF( pArray ) )
      pArray = hb_itemUnRef( pArray );

   if( HB_IS_ARRAY( pArray ) )
   {
      if( HB_IS_OBJECT( pArray ) && hb_objHasOperator( pArray, HB_OO_OP_ARRAYINDEX ) )
      {
         hb_vmPushNumInt( ulIndex );
         hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray,
                             hb_stackItemFromTop( -1 ), pValue );
         hb_stackPop();
         hb_stackPop();
         hb_stackPop();
         return;
      }

      if( HB_IS_VALID_INDEX( ulIndex, pArray->item.asArray.value->ulLen ) )
      {
         pValue->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );
         hb_itemMoveRef( pArray->item.asArray.value->pItems + ulIndex - 1, pValue );
         hb_stackPop();
         hb_stackDec();    /* value was moved above hb_stackDec() is enough */
      }
      else
      {
         hb_vmPushNumInt( ulIndex );
         if( !HB_IS_OBJECT( pArray ) &&
             hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray,
                                 hb_stackItemFromTop( -1 ), pValue ) )
         {
            hb_stackPop();
            hb_stackPop();
            hb_stackPop();
         }
         else
#ifdef HB_C52_STRICT
            hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 0 );
#else
            hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ),
                           1, hb_stackItemFromTop( -1 ) );
#endif
      }
   }
   else if( HB_IS_HASH( pArray ) )
   {
      PHB_ITEM pDest;

      hb_vmPushNumInt( ulIndex );
      pDest = hb_hashGetItemPtr( pArray, hb_stackItemFromTop( -1 ), HB_HASH_AUTOADD_ASSIGN );

      if( pDest )
      {
         pValue->type &= ~( HB_IT_MEMOFLAG | HB_IT_DEFAULT );
         hb_itemMoveRef( pDest, pValue );
         hb_stackPop();
         hb_stackPop();
         hb_stackDec();    /* value was moved above hb_stackDec() is enough */
      }
      else if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray,
                                   hb_stackItemFromTop( -1 ), pValue ) )
      {
         hb_stackPop();
         hb_stackPop();
         hb_stackPop();
      }
      else
         hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 3, pArray, hb_stackItemFromTop( -1 ), pValue );
   }
   else
   {
      hb_vmPushNumInt( ulIndex );
      if( hb_objOperatorCall( HB_OO_OP_ARRAYINDEX, pArray, pArray,
                              hb_stackItemFromTop( -1 ), pValue ) )
      {
         hb_stackPop();
         hb_stackPop();
         hb_stackPop();
      }
      else
         hb_errRT_BASE( EG_ARG, 1069, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ),
                        1, hb_stackItemFromTop( -1 ) );
   }
}


HB_EXPORT BOOL hb_xvmArrayPush( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmArrayPush()"));

   hb_vmArrayPush();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmArrayPushRef( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmArrayPushRef()"));

   hb_vmArrayPushRef();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmArrayItemPush( ULONG ulIndex )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmArrayItemPush(%lu)", ulIndex));

   hb_vmArrayItemPush( ulIndex );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmArrayPop( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmArrayPop()"));

   hb_vmArrayPop();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmArrayItemPop( ULONG ulIndex )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmArrayItemPop(%lu)", ulIndex));

   hb_vmArrayItemPop( ulIndex );

   HB_XVM_RETURN
}

HB_EXPORT void hb_xvmPushDouble( double dNumber, int iWidth, int iDec )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushDouble(%lf, %d, %d)", dNumber, iWidth, iDec));

   hb_vmPushDoubleConst( dNumber, iWidth, iDec );
}

#ifdef HB_LONG_LONG_OFF
HB_EXPORT void hb_xvmPushLongLong( double dNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushLongLong(%l.0f)", dNumber));

   hb_vmPushDoubleConst( dNumber, HB_DEFAULT_WIDTH, 0 );
}
#else
HB_EXPORT void hb_xvmPushLongLong( LONGLONG llNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushLongLong(%" PFLL "i)", llNumber));

   hb_vmPushLongLongConst( llNumber );
}
#endif

HB_EXPORT void hb_xvmLocalName( USHORT uiLocal, char * szLocalName )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmLocalName(%hu, %s)", uiLocal, szLocalName));

#ifndef HB_NO_DEBUG
   hb_vmLocalName( uiLocal, szLocalName );
#else
   HB_SYMBOL_UNUSED( uiLocal );
   HB_SYMBOL_UNUSED( szLocalName );
#endif
}

HB_EXPORT void hb_xvmStaticName( BYTE bIsGlobal, USHORT uiStatic, char * szStaticName )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmStaticName(%d, %hu, %s)", (int)bIsGlobal, uiStatic, szStaticName));

#ifndef HB_NO_DEBUG
   hb_vmStaticName( bIsGlobal, uiStatic, szStaticName );
#else
   HB_SYMBOL_UNUSED( bIsGlobal );
   HB_SYMBOL_UNUSED( uiStatic );
   HB_SYMBOL_UNUSED( szStaticName );
#endif
}

HB_EXPORT void hb_xvmModuleName( char * szModuleName )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmModuleName(%s)", szModuleName));

#ifndef HB_NO_DEBUG
   hb_vmModuleName( szModuleName );
#else
   HB_SYMBOL_UNUSED( szModuleName );
#endif
}

HB_EXPORT BOOL hb_xvmMacroArrayGen( USHORT uiArgSets )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroArrayGen(%hu)", uiArgSets));

   hb_vmMacroArrayGen( uiArgSets );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroDo( USHORT uiArgSets )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroDo(%hu)", uiArgSets));

   hb_vmMacroDo( uiArgSets );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroFunc( USHORT uiArgSets )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroFunc(%hu)", uiArgSets));

   hb_vmMacroFunc( uiArgSets );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroSend( USHORT uiArgSets )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroSend(%hu)", uiArgSets));

   hb_vmMacroSend( uiArgSets );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroPush( BYTE bFlags )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroPush(%d)", bFlags));

   hb_macroGetValue( hb_stackItemFromTop( -1 ), 0, bFlags );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroPushRef( void )
{
   HB_STACK_TLS_PRELOAD

   PHB_ITEM pMacro;

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroPushRef()"));

   pMacro = hb_stackItemFromTop( -1 );
   hb_macroPushSymbol( pMacro );
   if( hb_stackGetActionRequest() == 0 )
      hb_memvarGetRefer( pMacro, pMacro->item.asSymbol.value );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroPushIndex( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroPushIndex()"));

   hb_vmMacroPushIndex();

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroPushList( BYTE bFlags )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroPushList(%d)", bFlags));

   hb_macroGetValue( hb_stackItemFromTop( -1 ), HB_P_MACROPUSHLIST, bFlags );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroPushPare( BYTE bFlags )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroPushPare(%d)", bFlags));

   hb_macroGetValue( hb_stackItemFromTop( -1 ), HB_P_MACROPUSHPARE, bFlags );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroPushAliased( BYTE bFlags )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroPushAliased(%d)", bFlags));

   hb_macroPushAliasedValue( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), bFlags );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroPop( BYTE bFlags )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroPop(%d)", bFlags));

   hb_macroSetValue( hb_stackItemFromTop( -1 ), bFlags );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroPopAliased( BYTE bFlags )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroPopAliased(%d)", bFlags));

   hb_macroPopAliasedValue( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), bFlags );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroSymbol( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroSymbol()"));

   hb_macroPushSymbol( hb_stackItemFromTop( -1 ) );

   HB_XVM_RETURN
}

HB_EXPORT BOOL hb_xvmMacroText( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmMacroText()"));

   hb_macroTextValue( hb_stackItemFromTop( -1 ) );

   HB_XVM_RETURN
}

HB_EXPORT void hb_xvmPushVParams( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmPushVParams()"));

   hb_vmPushVParams();
}
HB_EXPORT void hb_xvmWithObjectStart( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_xvmWithObjectStart()"));

   hb_vmWithObjectStart();
}

HB_EXPORT void hb_xvmWithObjectEnd( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmWithObjectEnd()"));

   hb_stackPop();  /* remove with object envelope */
   hb_stackPop();  /* remove implicit object */
}

HB_EXPORT void hb_xvmWithObjectMessage( PHB_SYMB pSymbol )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_xvmWithObjectMessage(%p)", pSymbol));

   if( pSymbol )
      hb_vmPushSymbol( pSymbol );
   hb_vmPush( hb_stackWithObjectItem() );
}



#undef hb_vmFlagEnabled
HB_EXPORT ULONG hb_vmFlagEnabled( ULONG flags )
{
   return s_VMFlags & flags;
}

HB_EXPORT void hb_vmFlagSet( ULONG flags )
{
   s_VMFlags |= flags;
}

HB_EXPORT void hb_vmFlagClear( ULONG flags )
{
   s_VMFlags &= ~flags;
}

/* ------------------------------------------------------------------------ */
/* The debugger support functions */
/* ------------------------------------------------------------------------ */

void hb_vmRequestDebug( void )
{
#ifndef HB_NO_DEBUG
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestDebug()"));

   *( hb_stackDebugRequest() ) = TRUE;
#endif
}

HB_EXPORT BOOL hb_dbg_InvokeDebug( BOOL bInvoke )
{
#ifndef HB_NO_DEBUG
   HB_STACK_TLS_PRELOAD
   BOOL * pfRequest = hb_stackDebugRequest();
   BOOL bRequest = *pfRequest;
   *pfRequest = bInvoke;
   return bRequest;
#else
   HB_SYMBOL_UNUSED( bInvoke );
   return FALSE;
#endif
}

HB_EXPORT HB_DBGENTRY_FUNC hb_dbg_SetEntry( HB_DBGENTRY_FUNC pFunDbgEntry )
{
   HB_DBGENTRY_FUNC pPrevFunc;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbg_SetEntry(%p)", pFunDbgEntry));

#ifndef HB_NO_DEBUG
   pPrevFunc = s_pFunDbgEntry;
   s_pFunDbgEntry = pFunDbgEntry;
#else
   HB_SYMBOL_UNUSED( pFunDbgEntry );
   pPrevFunc = NULL;
#endif

   return pPrevFunc;
}

HB_EXPORT PHB_ITEM hb_dbg_vmVarSGet( int nStatic, int nOffset )
{
   return hb_arrayGetItemPtr( &s_aStatics, nStatic + nOffset );
}

HB_EXPORT ULONG hb_dbg_ProcLevel( void )
{
   return hb_stackCallDepth();
}

/*
 * check if the debugger activation was requested or request the debugger
 * activation if .T. is passed
 */
HB_FUNC( __DBGINVOKEDEBUG )
{
   HB_STACK_TLS_PRELOAD

#ifndef HB_NO_DEBUG
   BOOL * pfRequest = hb_stackDebugRequest();

   hb_retl( *pfRequest );
   *pfRequest = hb_parl( 1 );
#else
   hb_retl( FALSE );
#endif
}

/* $Doc$
 * $FuncName$     <aStat> __dbgvmVarSList()
 * $Description$  Return the statics array. Please aClone before assignments
 * $End$ */
HB_FUNC( __DBGVMVARSLIST )
{
   PHB_ITEM pStatics = hb_itemClone( &s_aStatics );

   hb_itemReturnRelease( pStatics );
}

/* $Doc$
 * $FuncName$     <nStatics> __dbgvmVarSLen()
 * $Description$  Return the statics array length.
 * $End$ */
HB_FUNC( __DBGVMVARSLEN )
{
   HB_STACK_TLS_PRELOAD
   hb_retnl( hb_arrayLen( &s_aStatics ) );
}

/* $Doc$
 * $FuncName$     <xStat> __dbgvmVarSGet(<nStatic>)
 * $Description$  Return a specified statics
 * $End$ */
HB_FUNC( __DBGVMVARSGET )
{
   hb_itemReturn( hb_dbg_vmVarSGet( hb_parni( 1 ), hb_parni( 2 ) ) );
}

/* $Doc$
 * $FuncName$     __dbgvmVarSSet(<nStatic>,<uValue>)
 * $Description$  Sets the value of a specified statics
 * $End$ */
HB_FUNC( __DBGVMVARSSET )
{
   PHB_ITEM pItem = hb_param( 3, HB_IT_ANY );
   if( pItem )
      hb_arraySet( &s_aStatics, hb_parni( 1 ) + hb_parni( 2 ), pItem );
}

HB_FUNC( __DBGPROCLEVEL )
{
   HB_STACK_TLS_PRELOAD
   hb_retnl( hb_dbg_ProcLevel() - 1 );   /* Don't count self */
}

/*
 * These functions are for GLOBAL variables - now they are only for
 * compatibility with xHarbour debugger - Harbour does not support
 * GLOBALs
 */
HB_EXPORT ULONG hb_dbg_vmVarGCount( void )
{
#if 0
   return hb_arrayLen( &s_aGlobals );
#else
   return 0;
#endif
}

HB_EXPORT PHB_ITEM hb_dbg_vmVarGGet( int nGlobal, int nOffset )
{
#if 0
   return hb_arrayGetItemPtr( &s_aGlobals, nGlobal + nOffset );
#else
   HB_SYMBOL_UNUSED( nGlobal );
   HB_SYMBOL_UNUSED( nOffset );
   return NULL;
#endif
}

/* $Doc$
 * $FuncName$     <aStat> __dbgvmVarGList()
 * $Description$  Return a clone of the globals array.
 * $End$ */
HB_FUNC( __DBGVMVARGLIST )
{
#if 0
   PHB_ITEM pGlobals = hb_itemClone( &s_aGlobals );
#else
   PHB_ITEM pGlobals = hb_itemArrayNew( 0 );
#endif

   hb_itemReturnRelease( pGlobals );
}

HB_FUNC( __DBGVMVARGGET )
{
   hb_itemReturn( hb_dbg_vmVarGGet( hb_parni( 1 ), hb_parni( 2 ) ) );
}

HB_FUNC( __DBGVMVARGSET )
{
#if 0
   PHB_ITEM pItem = hb_param( 3, HB_IT_ANY );
   if( pItem )
      hb_arraySet( &s_aGlobals, hb_parni( 1 ) + hb_parni( 2 ), pItem );
#endif
}


/* ------------------------------------------------------------------------ */
/* The garbage collector interface */
/* ------------------------------------------------------------------------ */

/* Mark all statics as used so they will not be released by the
 * garbage collector
 */
void hb_vmIsStaticRef( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmIsStaticRef()"));

   /* statics are stored as an item of array type */
   hb_gcItemRef( &s_aStatics );
}

void hb_vmIsStackRef( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmIsStackRef()"));

#if defined( HB_MT_VM )
   if( s_vmStackLst )
   {
      PHB_THREADSTATE pStack = s_vmStackLst;
      do
      {
         if( pStack->fActive && pStack->pStackId )
            hb_stackIsStackRef( pStack->pStackId, hb_vmTSVarClean );
         pStack = pStack->pNext;
      }
      while( pStack != s_vmStackLst );
   }
#else
   hb_stackIsStackRef( hb_stackId(), NULL );
#endif
}

/* ------------------------------------------------------------------------ */

/* $Doc$
 * $FuncName$     __SETPROFILER( <lOnOff> ) --> <lOldValue>
 * $Description$  Turns on | off the profiler activity
 * $End$ */
HB_FUNC( __SETPROFILER )
{
   HB_STACK_TLS_PRELOAD
#ifdef HB_NO_PROFILER
   hb_retl( FALSE );
#else
   hb_retl( hb_bProfiler );
   if( ISLOG( 1 ) )
      hb_bProfiler = hb_parl( 1 );
#endif
}

/* $Doc$
 * $FuncName$     __TRACEPRGCALLS( <lOnOff> ) --> <lOldValue>
 * $Description$  Turns on | off tracing of PRG-level function and method calls
 * $End$ */
HB_FUNC( __TRACEPRGCALLS )
{
   HB_STACK_TLS_PRELOAD
#if defined( HB_PRG_TRACE )
   hb_retl( hb_bTracePrgCalls );
   if( ISLOG( 1 ) )
      hb_bTracePrgCalls = hb_parl( 1 );
#else
   hb_retl( FALSE );
#endif
}

HB_FUNC( __OPCOUNT ) /* it returns the total amount of opcodes */
{
   HB_STACK_TLS_PRELOAD
   hb_retnl( HB_P_LAST_PCODE - 1 );
}

HB_FUNC( __OPGETPRF ) /* profiler: It returns an array with an opcode called and
                         consumed times { nTimes, nTime },
                         given the opcode index */
{
   HB_STACK_TLS_PRELOAD
#ifndef HB_NO_PROFILER
   ULONG ulOpcode = hb_parnl( 1 );

   hb_reta( 2 );
   if( ulOpcode < HB_P_LAST_PCODE )
   {
      hb_stornl( hb_ulOpcodesCalls[ ulOpcode ], -1, 1 );
      hb_stornl( hb_ulOpcodesTime[ ulOpcode ],  -1, 2 );
   }
   else
#else
   hb_reta( 2 );
#endif
   {
      hb_stornl( 0, -1, 1 );
      hb_stornl( 0, -1, 2 );
   }
}

HB_FUNC( ERRORLEVEL )
{
   HB_STACK_TLS_PRELOAD

   hb_retni( s_nErrorLevel );

   /* NOTE: This should be ISNUM( 1 ), but it's sort of a Clipper bug that it
            accepts other types also and considers them zero. [vszakats] */

   if( hb_pcount() >= 1 )
      /* Only replace the error level if a parameter was passed */
      s_nErrorLevel = hb_parni( 1 );
}


/* NOTE: We should make sure that these get linked.
         Don't make this function static, because it's not called from
         this file. [vszakats] */

void hb_vmForceLink( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmForceLink()"));

   HB_FUNC_EXEC( SYSINIT );
}

/* Force linking default language and codepage modules */
HB_CODEPAGE_REQUEST( HB_CODEPAGE_DEFAULT )
HB_LANG_REQUEST( HB_LANG_DEFAULT )

#undef HB_FORCE_LINK_MAIN

#if !defined(HB_DYNLIB) && defined(HB_OS_WIN_32) && \
    ( defined(__DMC__) || defined(__WATCOMC__) || defined(__MINGW32__) )

#  define HB_FORCE_LINK_MAIN  hb_forceLinkMainWin

#elif defined(HB_OS_LINUX) && defined(__WATCOMC__)

#  define HB_FORCE_LINK_MAIN  hb_forceLinkMainStd

#endif

#ifdef HB_FORCE_LINK_MAIN
HB_EXTERN_BEGIN
extern HB_EXPORT void HB_FORCE_LINK_MAIN( void );
HB_EXTERN_END
void _hb_forceLinkMain()
{
   HB_FORCE_LINK_MAIN();
}
#endif
