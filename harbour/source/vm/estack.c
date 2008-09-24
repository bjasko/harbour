/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The eval stack management functions
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

#define HB_STACK_PRELOAD

#define INCL_DOSPROCESS


#include "hbvmopt.h"
#include "hbapi.h"
#include "hbapicls.h"
#include "hbdefs.h"
#include "hbstack.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbapirdd.h"

/* ------------------------------- */

#if !defined( STACK_INITHB_ITEMS )
   #define STACK_INITHB_ITEMS      200
#endif
#if !defined( STACK_EXPANDHB_ITEMS )
   #define STACK_EXPANDHB_ITEMS    20
#endif


/* ------------------------------- */

#undef HB_STACK_TLS_PRELOAD

#if defined( HB_MT_VM )

#  include "hbthread.h"

   static HB_CRITICAL_NEW( TSD_counter );
   static int s_iTSDCounter = 0;

#  ifdef HB_USE_TLS

      /* compiler has native support for TLS */
#     if defined( HB_STACK_MACROS )
#        if defined( __BORLANDC__ )
            PHB_STACK HB_TLS_ATTR hb_stack_ptr = NULL;
#        else
            HB_TLS_ATTR PHB_STACK hb_stack_ptr = NULL;
#        endif
#     else
         static HB_TLS_ATTR PHB_STACK hb_stack_ptr = NULL;
#     endif

#     define hb_stack_alloc()    do { hb_stack_ptr = ( PHB_STACK ) \
                                      hb_xgrab( sizeof( HB_STACK ) ); } while ( 0 )
#     define hb_stack_dealloc()  do { hb_xfree( hb_stack_ptr ); \
                                      hb_stack_ptr = NULL; } while ( 0 )
#     define hb_stack_ready()    (hb_stack_ptr != NULL)

#else

      /* compiler has no native TLS support, we have to implement it ourselves */
#     if defined( HB_STACK_MACROS )
         HB_TLS_KEY hb_stack_key;
#     else
         static HB_TLS_KEY hb_stack_key;
#        define hb_stack_ptr     ( ( PHB_STACK ) hb_tls_get( hb_stack_key ) )
#     endif
      static volatile BOOL s_fInited = FALSE;
#     define hb_stack_alloc()    do { if( !s_fInited ) { \
                                         hb_tls_init( hb_stack_key ); \
                                         s_fInited = TRUE; } \
                                      hb_tls_set( hb_stack_key, \
                                                  hb_xgrab( sizeof( HB_STACK ) ) ); \
                                 } while ( 0 )
#     define hb_stack_dealloc()  do { hb_xfree( hb_stack_ptr ); \
                                      hb_tls_set( hb_stack_key, NULL ); } \
                                 while ( 0 )
#     define hb_stack_ready()    ( s_fInited && hb_tls_get( hb_stack_key ) != NULL )

#endif /* HB_USE_TLS */

#  undef hb_stack
#  ifdef HB_STACK_PRELOAD
#     define HB_STACK_TLS_PRELOAD   PHB_STACK _hb_stack_ptr_ = hb_stack_ptr;
#     define hb_stack               ( * _hb_stack_ptr_ )
#  else
#     define hb_stack   ( * hb_stack_ptr )
#  endif
#else

   /* no MT mode */
#  if defined( HB_STACK_MACROS )
      HB_STACK hb_stack;
#  else
      static HB_STACK hb_stack;
#  endif

#  define hb_stack_alloc()
#  define hb_stack_dealloc()
#  define hb_stack_ready()    (TRUE)

#endif /* HB_MT_VM */

#ifndef HB_STACK_TLS_PRELOAD
#  define HB_STACK_TLS_PRELOAD
#endif

/* ------------------------------- */

static BYTE s_byDirBuffer[ _POSIX_PATH_MAX + 1 ];
static HB_IOERRORS s_IOErrors;

/* ------------------------------- */

static HB_SYMB s_initSymbol = { "hb_stackInit", { HB_FS_STATIC }, { NULL }, NULL };

/* ------------------------------- */

static void hb_stack_init( PHB_STACK pStack )
{
   LONG i;

   HB_TRACE(HB_TR_DEBUG, ("hb_stack_init(%p)", pStack));

   memset( pStack, 0, sizeof( HB_STACK ) );

   pStack->pItems = ( PHB_ITEM * ) hb_xgrab( sizeof( PHB_ITEM ) * STACK_INITHB_ITEMS );
   pStack->pBase  = pStack->pItems;
   pStack->pPos   = pStack->pItems;       /* points to the first stack item */
   pStack->wItems = STACK_INITHB_ITEMS;
   pStack->pEnd   = pStack->pItems + pStack->wItems;

   for( i = 0; i < pStack->wItems; ++i )
   {
      pStack->pItems[ i ] = ( PHB_ITEM ) hb_xgrab( sizeof( HB_ITEM ) );
      pStack->pItems[ i ]->type = HB_IT_NIL;
   }

   pStack->pPos++;
   hb_itemPutSymbol( * pStack->pItems, &s_initSymbol );
   ( * pStack->pItems )->item.asSymbol.stackstate = &pStack->state;

   pStack->rdd.uiCurrArea = 1;
}

static void hb_stack_destroy_TSD( PHB_STACK pStack )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_stack_destroy_TSD(%p)", pStack));

   while( pStack->iTSD )
   {
      if( pStack->pTSD[pStack->iTSD].pTSD )
      {
         if( pStack->pTSD[pStack->iTSD].pTSD->pCleanFunc )
            pStack->pTSD[pStack->iTSD].pTSD->pCleanFunc(
                                          pStack->pTSD[pStack->iTSD].value );
         hb_xfree( pStack->pTSD[pStack->iTSD].value );
#if !defined( HB_MT_VM )
         pStack->pTSD[pStack->iTSD].pTSD->iHandle = 0;
#endif
      }
      if( --pStack->iTSD == 0 )
      {
         hb_xfree( pStack->pTSD );
         pStack->pTSD = NULL;
      }
   }
}

static void hb_stack_free( PHB_STACK pStack )
{
   LONG i;

   HB_TRACE(HB_TR_DEBUG, ("hb_stack_free(%p)", pStack));

   hb_stack_destroy_TSD( pStack );

   if( pStack->privates.stack )
   {
      hb_xfree( pStack->privates.stack );
      pStack->privates.stack = NULL;
      pStack->privates.size = pStack->privates.count =
      pStack->privates.base = 0;
   }
   i = pStack->wItems - 1;
   while( i >= 0 )
      hb_xfree( pStack->pItems[ i-- ] );
   hb_xfree( pStack->pItems );
   pStack->pItems = pStack->pPos = pStack->pBase = NULL;
#if defined( HB_MT_VM )
   if( pStack->byDirBuffer )
   {
      hb_xfree( pStack->byDirBuffer );
      pStack->byDirBuffer = NULL;
   }
   if( pStack->iDynH )
   {
      hb_xfree( pStack->pDynH );
      pStack->pDynH = NULL;
      pStack->iDynH = 0;
   }
#endif
}

void hb_stackDestroyTSD( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackDestroyTSD()"));

   hb_stack_destroy_TSD( &hb_stack );
}

void * hb_stackGetTSD( PHB_TSD pTSD )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackGetTSD(%p)", pTSD));

#if defined( HB_MT_VM )
   if( pTSD->iHandle == 0 || pTSD->iHandle > hb_stack.iTSD ||
       hb_stack.pTSD[pTSD->iHandle].pTSD == NULL )
   {
      if( pTSD->iHandle == 0 )
      {
         hb_threadEnterCriticalSection( &TSD_counter );
         /* repeated test protected by mutex to avoid race condition */
         if( pTSD->iHandle == 0 )
            pTSD->iHandle = ++s_iTSDCounter;
         hb_threadLeaveCriticalSection( &TSD_counter );
      }

      if( pTSD->iHandle > hb_stack.iTSD )
      {
         hb_stack.pTSD = ( PHB_TSD_HOLDER )
                        hb_xrealloc( hb_stack.pTSD, ( pTSD->iHandle + 1 ) *
                                                    sizeof( HB_TSD_HOLDER ) );
         memset( &hb_stack.pTSD[hb_stack.iTSD + 1], 0,
                 ( pTSD->iHandle - hb_stack.iTSD ) * sizeof( HB_TSD_HOLDER ) );
         hb_stack.iTSD = pTSD->iHandle;
      }
#else
   if( pTSD->iHandle == 0 )
   {
      ULONG ulSize = ( hb_stack.iTSD + 2 ) * sizeof( HB_TSD_HOLDER );
      if( hb_stack.iTSD == 0 )
      {
         hb_stack.pTSD = ( PHB_TSD_HOLDER ) hb_xgrab( ulSize );
         memset( hb_stack.pTSD, 0, ulSize );
      }
      else
      {
         hb_stack.pTSD = ( PHB_TSD_HOLDER ) hb_xrealloc( hb_stack.pTSD, ulSize );
      }
      pTSD->iHandle = ++hb_stack.iTSD;
#endif

      hb_stack.pTSD[pTSD->iHandle].pTSD  = pTSD;
      hb_stack.pTSD[pTSD->iHandle].value = hb_xgrab( pTSD->iSize );
      memset( hb_stack.pTSD[pTSD->iHandle].value, 0, pTSD->iSize );
      if( pTSD->pInitFunc )
         pTSD->pInitFunc( hb_stack.pTSD[pTSD->iHandle].value );
   }
   return hb_stack.pTSD[pTSD->iHandle].value;
}

void * hb_stackTestTSD( PHB_TSD pTSD )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackTestTSD(%p)", pTSD));

#if defined( HB_MT_VM )
   return ( pTSD->iHandle && pTSD->iHandle <= hb_stack.iTSD ) ?
                          hb_stack.pTSD[pTSD->iHandle].value : NULL;
#else
   return pTSD->iHandle ? hb_stack.pTSD[pTSD->iHandle].value : NULL;
#endif
}

void hb_stackInit( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_stackInit()"));

#if defined( HB_MT_VM )
   hb_stack_alloc();
#endif
   {
      HB_STACK_TLS_PRELOAD
      hb_stack_init( &hb_stack );
   }
}

void hb_stackFree( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackFree()"));

   hb_stack_free( &hb_stack );

#if defined( HB_MT_VM )
   hb_stack_dealloc();
#endif
}

#if defined( HB_MT_VM )

#undef hb_stackList
void * hb_stackList( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackList()"));

   return hb_stack.pStackLst;
}

#undef hb_stackListSet
void hb_stackListSet( void * pStackLst )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackListSet(%p)", pStackLst));

   hb_stack.pStackLst = pStackLst;
}

#undef hb_stackIdSetActionRequest
void hb_stackIdSetActionRequest( void * pStackId, USHORT uiAction )
{
   ( ( PHB_STACK ) pStackId )->uiActionRequest = uiAction;
}

PHB_DYN_HANDLES hb_stackGetDynHandle( PHB_DYNS pDynSym )
{
   HB_STACK_TLS_PRELOAD
   int iDynSym;

   HB_TRACE(HB_TR_DEBUG, ("hb_stackGetDynHandle()"));

   iDynSym = pDynSym->uiSymNum;
   if( iDynSym > hb_stack.iDynH )
   {
      hb_stack.pDynH = ( PHB_DYN_HANDLES ) hb_xrealloc( hb_stack.pDynH,
                                          iDynSym * sizeof( HB_DYN_HANDLES ) );
      memset( &hb_stack.pDynH[ hb_stack.iDynH ], 0,
              ( iDynSym - hb_stack.iDynH ) * sizeof( HB_DYN_HANDLES ) );
      hb_stack.iDynH = iDynSym;
   }

   return &hb_stack.pDynH[ iDynSym - 1 ];
}

#undef hb_stackQuitState
BOOL hb_stackQuitState( void )
{
   HB_STACK_TLS_PRELOAD
   return hb_stack.uiQuitState != 0;
}

#undef hb_stackSetQuitState
void hb_stackSetQuitState( USHORT uiState )
{
   HB_STACK_TLS_PRELOAD
   hb_stack.uiQuitState = uiState;
}

#undef hb_stackUnlock
int hb_stackUnlock( void )
{
   HB_STACK_TLS_PRELOAD
   return ++hb_stack.iUnlocked;
}

#undef hb_stackLock
int hb_stackLock( void )
{
   HB_STACK_TLS_PRELOAD
   return --hb_stack.iUnlocked;
}
#endif

#undef hb_stackGetPrivateStack
PHB_PRIVATE_STACK hb_stackGetPrivateStack( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackGetPrivateStack()"));

   return &hb_stack.privates;
}

#undef hb_stackSetStruct
PHB_SET_STRUCT hb_stackSetStruct( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackSetStruct()"));

   return &hb_stack.set;
}

#undef hb_stackId
void * hb_stackId( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackId()"));

   return ( void * ) &hb_stack;
}

#undef hb_stackPop
void hb_stackPop( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackPop()"));

   if( --hb_stack.pPos <= hb_stack.pBase )
      hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL );

   if( HB_IS_COMPLEX( * hb_stack.pPos ) )
      hb_itemClear( * hb_stack.pPos );
}

#undef hb_stackPopReturn
void hb_stackPopReturn( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackPopReturn()"));

   if( HB_IS_COMPLEX( &hb_stack.Return ) )
      hb_itemClear( &hb_stack.Return );

   if( --hb_stack.pPos <= hb_stack.pBase )
      hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL );

   hb_itemRawMove( &hb_stack.Return, * hb_stack.pPos );
}

#undef hb_stackDec
void hb_stackDec( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackDec()"));

   if( --hb_stack.pPos <= hb_stack.pBase )
      hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL );
}

#undef hb_stackDecrease
void hb_stackDecrease( ULONG ulItems )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackDecrease()"));

   if( ( hb_stack.pPos -= ulItems ) <= hb_stack.pBase )
      hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL );
}

#undef hb_stackPush
void hb_stackPush( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackPush()"));

   /* enough room for another item ? */
   if( ++hb_stack.pPos == hb_stack.pEnd )
      hb_stackIncrease();
}

#undef hb_stackAllocItem
HB_ITEM_PTR hb_stackAllocItem( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackAllocItem()"));

   if( ++hb_stack.pPos == hb_stack.pEnd )
      hb_stackIncrease();

   return * ( hb_stack.pPos - 1 );
}

#undef hb_stackPushReturn
void hb_stackPushReturn( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackPushReturn()"));

   hb_itemRawMove( * hb_stack.pPos, &hb_stack.Return );

   /* enough room for another item ? */
   if( ++hb_stack.pPos == hb_stack.pEnd )
      hb_stackIncrease();
}

void hb_stackIncrease( void )
{
   HB_STACK_TLS_PRELOAD
   LONG BaseIndex;   /* index of stack base */
   LONG CurrIndex;   /* index of current top item */
   LONG EndIndex;    /* index of current top item */

   HB_TRACE(HB_TR_DEBUG, ("hb_stackIncrease()"));

   BaseIndex = hb_stack.pBase - hb_stack.pItems;
   CurrIndex = hb_stack.pPos - hb_stack.pItems;
   EndIndex  = hb_stack.pEnd - hb_stack.pItems;

   /* no, make more headroom: */
   hb_stack.pItems = ( PHB_ITEM * ) hb_xrealloc( ( void * ) hb_stack.pItems,
            sizeof( PHB_ITEM ) * ( hb_stack.wItems + STACK_EXPANDHB_ITEMS ) );

   /* fix possibly modified by realloc pointers: */
   hb_stack.pPos   = hb_stack.pItems + CurrIndex;
   hb_stack.pBase  = hb_stack.pItems + BaseIndex;
   hb_stack.wItems += STACK_EXPANDHB_ITEMS;
   hb_stack.pEnd   = hb_stack.pItems + hb_stack.wItems;

   do
   {
      hb_stack.pItems[ EndIndex ] = ( PHB_ITEM ) hb_xgrab( sizeof( HB_ITEM ) );
      hb_stack.pItems[ EndIndex ]->type = HB_IT_NIL;
   }
   while( ++EndIndex < hb_stack.wItems );
}

void hb_stackRemove( LONG lUntilPos )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR * pEnd = hb_stack.pItems + lUntilPos;

   while( hb_stack.pPos > pEnd )
   {
      --hb_stack.pPos;
      if( HB_IS_COMPLEX( * hb_stack.pPos ) )
         hb_itemClear( * hb_stack.pPos );
   }
}

HB_ITEM_PTR hb_stackNewFrame( PHB_STACK_STATE pFrame, USHORT uiParams )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR * pBase, pItem;

   pBase = hb_stack.pPos - uiParams - 2;
   pItem = * pBase;  /* procedure symbol */

   if( ! HB_IS_SYMBOL( pItem ) )
   {
      hb_stackDispLocal();
      hb_errInternal( HB_EI_VMNOTSYMBOL, NULL, "hb_vmDo()", NULL );
   }

   pFrame->lBaseItem = hb_stack.pBase - hb_stack.pItems;
   pFrame->lStatics = hb_stack.lStatics;
   pFrame->ulPrivateBase = hb_memvarGetPrivatesBase();
   pFrame->uiClass = pFrame->uiMethod = pFrame->uiLineNo = 0;

   pItem->item.asSymbol.stackstate = pFrame;
   pItem->item.asSymbol.paramcnt = uiParams;
   /* set default value of 'paramdeclcnt' - it will be updated
    * in hb_vm[V]Frame only
    */
   pItem->item.asSymbol.paramdeclcnt = uiParams;
   hb_stack.pBase = pBase;

   return pItem;
}

void hb_stackOldFrame( PHB_STACK_STATE pFrame )
{
   HB_STACK_TLS_PRELOAD
   if( hb_stack.pPos <= hb_stack.pBase )
      hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL );

   do
   {
      --hb_stack.pPos;
      if( HB_IS_COMPLEX( * hb_stack.pPos ) )
         hb_itemClear( * hb_stack.pPos );
   }
   while( hb_stack.pPos > hb_stack.pBase );

   hb_stack.pBase = hb_stack.pItems + pFrame->lBaseItem;
   hb_stack.lStatics = pFrame->lStatics;
   hb_memvarSetPrivatesBase( pFrame->ulPrivateBase );
}

#undef hb_stackItem
HB_ITEM_PTR hb_stackItem( LONG iItemPos )
{
   HB_STACK_TLS_PRELOAD
   if( iItemPos < 0 )
      hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL );

   return * ( hb_stack.pItems + iItemPos );
}

#undef hb_stackItemFromTop
HB_ITEM_PTR hb_stackItemFromTop( int iFromTop )
{
   HB_STACK_TLS_PRELOAD
   if( iFromTop >= 0 )
      hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL );

   return * ( hb_stack.pPos + iFromTop );
}

#undef hb_stackItemFromBase
HB_ITEM_PTR hb_stackItemFromBase( int iFromBase )
{
   HB_STACK_TLS_PRELOAD
   if( iFromBase < 0 )
      hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL );

   return * ( hb_stack.pBase + iFromBase + 1 );
}

#undef hb_stackLocalVariable
HB_ITEM_PTR hb_stackLocalVariable( int *piFromBase )
{
   HB_STACK_TLS_PRELOAD
   HB_ITEM_PTR pBase = *hb_stack.pBase;

/*   
   if( *piFromBase <= 0 )
      hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL );
*/
   if( pBase->item.asSymbol.paramcnt > pBase->item.asSymbol.paramdeclcnt )
   {
      /* function with variable number of parameters:
       * FUNCTION foo( a,b,c,...)
       * LOCAL x,y,z
       * number of passed parameters is bigger then number of declared
       * parameters - skip additional parameters only for local variables
       */
      if( *piFromBase > pBase->item.asSymbol.paramdeclcnt )
         *piFromBase += pBase->item.asSymbol.paramcnt - pBase->item.asSymbol.paramdeclcnt;
   }
   return * ( hb_stack.pBase + *piFromBase + 1 );
}

#undef hb_stackBaseItem
HB_ITEM_PTR hb_stackBaseItem( void )
{
   HB_STACK_TLS_PRELOAD
   return * hb_stack.pBase;
}

/* Returns SELF object, an evaluated codeblock or NIL for normal func/proc
*/
#undef hb_stackSelfItem
HB_ITEM_PTR hb_stackSelfItem( void )
{
   HB_STACK_TLS_PRELOAD
   return * ( hb_stack.pBase + 1 );
}

#undef hb_stackReturnItem
HB_ITEM_PTR hb_stackReturnItem( void )
{
   HB_STACK_TLS_PRELOAD

   HB_TRACE(HB_TR_DEBUG, ("hb_stackReturnItem()"));

   return &hb_stack.Return;
}

#undef hb_stackTopOffset
LONG hb_stackTopOffset( void )
{
   HB_STACK_TLS_PRELOAD

   return hb_stack.pPos - hb_stack.pItems;
}

#undef hb_stackBaseOffset
LONG hb_stackBaseOffset( void )
{
   HB_STACK_TLS_PRELOAD

   return hb_stack.pBase - hb_stack.pItems + 1;
}

#undef hb_stackTotalItems
LONG hb_stackTotalItems( void )
{
   HB_STACK_TLS_PRELOAD

   return hb_stack.wItems;
}

#undef hb_stackDateBuffer
char * hb_stackDateBuffer( void )
{
   HB_STACK_TLS_PRELOAD

   return hb_stack.szDate;
}

BYTE * hb_stackDirBuffer( void )
{
#if defined( HB_MT_VM )
   if( hb_stack_ready() )
   {
      HB_STACK_TLS_PRELOAD
      if( !hb_stack.byDirBuffer )
         hb_stack.byDirBuffer = ( BYTE * ) hb_xgrab( _POSIX_PATH_MAX + 1 );
      return hb_stack.byDirBuffer;
   }
#endif
   return s_byDirBuffer;
}

PHB_IOERRORS hb_stackIOErrors( void )
{
#if defined( HB_MT_VM )
   if( hb_stack_ready() )
   {
      HB_STACK_TLS_PRELOAD
      return &hb_stack.IOErrors;
   }
#endif
   return &s_IOErrors;
}

PHB_STACKRDD hb_stackRDD( void )
{
   HB_STACK_TLS_PRELOAD
   return &hb_stack.rdd;
}


#undef hb_stackGetStaticsBase
LONG hb_stackGetStaticsBase( void )
{
   HB_STACK_TLS_PRELOAD
   return hb_stack.lStatics;
}

#undef hb_stackSetStaticsBase
void hb_stackSetStaticsBase( LONG lBase )
{
   HB_STACK_TLS_PRELOAD
   hb_stack.lStatics = lBase;
}

#undef hb_stackGetRecoverBase
LONG hb_stackGetRecoverBase( void )
{
   HB_STACK_TLS_PRELOAD
   return hb_stack.lRecoverBase;
}

#undef hb_stackSetRecoverBase
void hb_stackSetRecoverBase( LONG lBase )
{
   HB_STACK_TLS_PRELOAD
   hb_stack.lRecoverBase = lBase;
}

#undef hb_stackGetActionRequest
USHORT hb_stackGetActionRequest( void )
{
   HB_STACK_TLS_PRELOAD
   return hb_stack.uiActionRequest;
}

#undef hb_stackSetActionRequest
void hb_stackSetActionRequest( USHORT uiAction )
{
   HB_STACK_TLS_PRELOAD
   hb_stack.uiActionRequest = uiAction;
}

#undef hb_stackWithObjectItem
PHB_ITEM hb_stackWithObjectItem( void )
{
   HB_STACK_TLS_PRELOAD
   return hb_stack.lWithObject ?
                        * ( hb_stack.pItems + hb_stack.lWithObject ) : NULL;
}

#undef hb_stackWithObjectOffset
LONG hb_stackWithObjectOffset( void )
{
   HB_STACK_TLS_PRELOAD
   return hb_stack.lWithObject;
}

#undef hb_stackWithObjectSetOffset
void hb_stackWithObjectSetOffset( LONG lOffset )
{
   HB_STACK_TLS_PRELOAD
   hb_stack.lWithObject = lOffset;
}

#undef hb_stackGetCDP
void * hb_stackGetCDP( void )
{
   HB_STACK_TLS_PRELOAD
   return hb_stack.pCDP;
}

#undef hb_stackSetCDP
void hb_stackSetCDP( void * pCDP )
{
   HB_STACK_TLS_PRELOAD
   hb_stack.pCDP = pCDP;
}

#undef hb_stackGetLang
void * hb_stackGetLang( void )
{
   HB_STACK_TLS_PRELOAD
   return hb_stack.pLang;
}

#undef hb_stackSetLang
void hb_stackSetLang( void * pLang )
{
   HB_STACK_TLS_PRELOAD
   hb_stack.pLang = pLang;
}

#undef hb_stackItemBasePtr
PHB_ITEM ** hb_stackItemBasePtr( void )
{
   HB_STACK_TLS_PRELOAD
   return &hb_stack.pItems;
}

void hb_stackClearMevarsBase( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM pBase;

   HB_TRACE(HB_TR_DEBUG, ("hb_stackClearMevarsBase()"));

   pBase = * hb_stack.pBase;

   while( pBase->item.asSymbol.stackstate->ulPrivateBase != 0 )
   {
      pBase->item.asSymbol.stackstate->ulPrivateBase = 0;
      pBase = * ( hb_stack.pItems + pBase->item.asSymbol.stackstate->lBaseItem );
   }
}

int hb_stackCallDepth( void )
{
   HB_STACK_TLS_PRELOAD
   LONG lOffset = hb_stack.pBase - hb_stack.pItems;
   int iLevel = 0;

   while( lOffset > 0 )
   {
      lOffset = ( * ( hb_stack.pItems + lOffset ) )->item.asSymbol.stackstate->lBaseItem;
      ++iLevel;
   }

   return iLevel;
}

LONG hb_stackBaseProcOffset( int iLevel )
{
   HB_STACK_TLS_PRELOAD
   LONG lOffset = hb_stack.pBase - hb_stack.pItems;

   while( iLevel-- > 0 && lOffset > 0 )
      lOffset = ( * ( hb_stack.pItems + lOffset ) )->item.asSymbol.stackstate->lBaseItem;

   if( iLevel < 0 && ( lOffset > 0 || HB_IS_SYMBOL( * hb_stack.pItems ) ) )
      return lOffset;
   else
      return -1;
}

void hb_stackBaseProcInfo( char * szProcName, USHORT * puiProcLine )
{
   /*
    * This function is called by FM module and has to be ready for execution
    * before hb_stack initialization, [druzus]
    * szProcName should be at least HB_SYMBOL_NAME_LEN + 1 bytes buffer
    */

#if defined( HB_MT_VM )
   if( !hb_stack_ready() )
   {
      szProcName[ 0 ] = '\0';
      * puiProcLine = 0;
      return;
   }
#endif
   {
      HB_STACK_TLS_PRELOAD
      if( hb_stack.pPos > hb_stack.pBase )
      {
         hb_strncpy( szProcName, ( * hb_stack.pBase )->item.asSymbol.value->szName,
                     HB_SYMBOL_NAME_LEN );
         * puiProcLine = ( * hb_stack.pBase )->item.asSymbol.stackstate->uiLineNo;
      }
      else
      {
         szProcName[ 0 ] = '\0';
         * puiProcLine = 0;
      }
   }
}

/* NOTE: DEBUG function */
void hb_stackDispLocal( void )
{
   HB_STACK_TLS_PRELOAD
   PHB_ITEM * pBase;

   HB_TRACE(HB_TR_DEBUG, ("hb_stackDispLocal()"));

   printf( hb_conNewLine() );
   printf( HB_I_("Virtual Machine Stack Dump at %s(%i):"),
           ( *hb_stack.pBase )->item.asSymbol.value->szName,
           ( *hb_stack.pBase )->item.asSymbol.stackstate->uiLineNo );
   printf( hb_conNewLine() );
   printf( "--------------------------" );

   for( pBase = hb_stack.pBase; pBase <= hb_stack.pPos; pBase++ )
   {
      printf( hb_conNewLine() );

      switch( hb_itemType( *pBase ) )
      {
         case HB_IT_NIL:
            printf( HB_I_("NIL ") );
            break;

         case HB_IT_ARRAY:
            if( hb_arrayIsObject( *pBase ) )
               printf( HB_I_("OBJECT = %s "), hb_objGetClsName( *pBase ) );
            else
               printf( HB_I_("ARRAY ") );
            break;

         case HB_IT_BLOCK:
            printf( HB_I_("BLOCK ") );
            break;

         case HB_IT_DATE:
            {
               char szDate[ 9 ];
               printf( HB_I_("DATE = \"%s\" "), hb_itemGetDS( *pBase, szDate ) );
            }
            break;

         case HB_IT_DOUBLE:
            printf( HB_I_("DOUBLE = %f "), hb_itemGetND( *pBase ) );
            break;

         case HB_IT_LOGICAL:
            printf( HB_I_("LOGICAL = %s "), hb_itemGetL( *pBase ) ? ".T." : ".F." );
            break;

         case HB_IT_LONG:
            printf( HB_I_("LONG = %" PFHL "i ") , hb_itemGetNInt( *pBase ) );
            break;

         case HB_IT_INTEGER:
            printf( HB_I_("INTEGER = %i "), hb_itemGetNI( *pBase ) );
            break;

         case HB_IT_STRING:
            printf( HB_I_("STRING = \"%s\" "), hb_itemGetCPtr( *pBase ) );
            break;

         case HB_IT_SYMBOL:
            printf( HB_I_("SYMBOL = %s "), ( *pBase )->item.asSymbol.value->szName );
            break;

         case HB_IT_POINTER:
            printf( HB_I_("POINTER = %p "), ( *pBase )->item.asPointer.value );
            break;

         default:
            printf( HB_I_("UNKNOWN = TYPE %i "), hb_itemType( *pBase ) );
            break;
      }
   }
}

void hb_stackDispCall( void )
{
   char buffer[ HB_SYMBOL_NAME_LEN + HB_SYMBOL_NAME_LEN + 5 + 10 ]; /* additional 10 bytes for line info (%hu) overhead */
   char file[ _POSIX_PATH_MAX + 1 ];
   USHORT uiLine;
   int iLevel;

   HB_TRACE(HB_TR_DEBUG, ("hb_stackDispCall()"));

   iLevel = 0;

   while( hb_procinfo( iLevel++, buffer, &uiLine, file ) )
   {
      int l = strlen( buffer );
      snprintf( buffer + l, sizeof( buffer ) - l, "(%hu)%s%s", uiLine, *file ? HB_I_(" in ") : "", file );

      hb_conOutErr( "Called from ", 0 );
      hb_conOutErr( buffer, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );
   }
}

/* ------------------------------------------------------------------------ */
/* The garbage collector interface */
/* ------------------------------------------------------------------------ */

#if !defined( HB_MT_VM )
/* helper function to scan all visible memvar variables
 */
static HB_DYNS_FUNC( hb_stackMemvarScan )
{
   PHB_ITEM pMemvar;

   HB_SYMBOL_UNUSED( Cargo );

   pMemvar = hb_dynsymGetMemvar( pDynSymbol );
   if( pMemvar && HB_IS_GCITEM( pMemvar ) )
      hb_gcItemRef( pMemvar );

   return TRUE;
}
#endif

/* Mark all memvars (PRIVATEs and PUBLICs) */
static void hb_stackIsMemvarRef( PHB_STACK pStack )
{
   /* 1. Mark all hidden memvars (PRIVATEs and PUBLICs) */
   PHB_PRIVATE_STACK pPrivateStack = &pStack->privates;
   ULONG ulCount = pPrivateStack->count;

   while( ulCount )
   {
      PHB_ITEM pMemvar = pPrivateStack->stack[ --ulCount ].pPrevMemvar;
      if( pMemvar && HB_IS_GCITEM( pMemvar ) )
         hb_gcItemRef( pMemvar );
   }
   /* 2. Mark all visible memvars (PRIVATEs and PUBLICs) */
#if defined( HB_MT_VM )
   {
      int iDynSym = pStack->iDynH;

      while( --iDynSym >= 0 )
      {
         PHB_ITEM pMemvar = ( PHB_ITEM ) pStack->pDynH[ iDynSym ].pMemvar;
         if( pMemvar && HB_IS_GCITEM( pMemvar ) )
            hb_gcItemRef( pMemvar );
      }
   }
#else
   hb_dynsymEval( hb_stackMemvarScan, NULL );
#endif
}

/* Mark all thread static variables */
static void hb_stackIsTsdRef( PHB_STACK pStack, PHB_TSD_FUNC pCleanFunc )
{
   int iTSD = pStack->iTSD;

   while( iTSD )
   {
      if( pStack->pTSD[iTSD].pTSD &&
          pStack->pTSD[iTSD].pTSD->pCleanFunc == pCleanFunc )
      {
         PHB_ITEM pItem = ( PHB_ITEM ) pStack->pTSD[iTSD].value;
         if( HB_IS_GCITEM( pItem ) )
            hb_gcItemRef( pItem );
      }
      --iTSD;
   }
}

/* Mark all locals as used so they will not be released by the
 * garbage collector
 */
void hb_stackIsStackRef( void * pStackId, PHB_TSD_FUNC pCleanFunc )
{
   PHB_STACK pStack;
   long lCount;

   HB_TRACE(HB_TR_DEBUG, ("hb_stackIsStackRef()"));

   pStack = ( PHB_STACK ) pStackId;
   lCount = pStack->pPos - pStack->pItems;
   while( lCount > 0 )
   {
      PHB_ITEM pItem = pStack->pItems[ --lCount ];

      if( HB_IS_GCITEM( pItem ) )
         hb_gcItemRef( pItem );
   }

   hb_gcItemRef( &pStack->Return );

   hb_stackIsMemvarRef( pStack );

   if( pCleanFunc )
      hb_stackIsTsdRef( pStack, pCleanFunc );
}
