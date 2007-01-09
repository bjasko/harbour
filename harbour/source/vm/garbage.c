/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The garbage collector for Harbour
 *
 * Copyright 1999 Ryszard Glab <rglab@imid.med.pl>
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

#include "hbvmopt.h"
#include "hbapi.h"
#include "hbstack.h"
#include "hbapicls.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbvm.h"
#include "error.ch"

#if !defined( HB_GC_PTR )

/* holder of memory block information */
/* NOTE: USHORT is used intentionally to fill up the structure to
 * full 16 bytes (on 16/32 bit environment)
 */
typedef struct HB_GARBAGE_
{
   struct HB_GARBAGE_ *pNext;  /* next memory block */
   struct HB_GARBAGE_ *pPrev;  /* previous memory block */
   HB_GARBAGE_FUNC_PTR pFunc;  /* cleanup function called before memory releasing */
   USHORT locked;              /* locking counter */
   BYTE used;                  /* used/unused block */
   BYTE flags;
} HB_GARBAGE, *HB_GARBAGE_PTR;

#ifdef HB_ALLOC_ALIGNMENT
#  define HB_GARBAGE_SIZE     ( ( sizeof( HB_GARBAGE ) + HB_ALLOC_ALIGNMENT - 1 ) - \
                                ( sizeof( HB_GARBAGE ) + HB_ALLOC_ALIGNMENT - 1 ) % HB_ALLOC_ALIGNMENT )
#else
#  define HB_GARBAGE_SIZE     sizeof( HB_GARBAGE )
#endif

#define HB_GC_PTR( p )        ( ( HB_GARBAGE_PTR ) ( ( BYTE * ) ( p ) - HB_GARBAGE_SIZE ) )

#endif /* !defined( HB_GC_PTR ) */

#define HB_MEM_PTR( p )       ( ( void * ) ( ( BYTE * ) ( p ) + HB_GARBAGE_SIZE ) )

/* we may use a cache later */
#define HB_GARBAGE_NEW( ulSize )    ( ( HB_GARBAGE_PTR ) hb_xgrab( HB_GARBAGE_SIZE + ( ulSize ) ) )
#define HB_GARBAGE_FREE( pAlloc )   hb_xfree( ( void * ) ( pAlloc ) )

/* status of memory block */
/* flags stored in 'locked' slot */
#define HB_GC_UNLOCKED     0
#define HB_GC_LOCKED       1  /* do not collect a memory block */
/* flags stored in 'used' slot */
#define HB_GC_USED_FLAG    2  /* the bit for used/unused flag */
#define HB_GC_DELETE       4  /* item marked to delete */
#define HB_GC_DELETELST    8  /* item will be deleted during finalization */
/* flags stored in 'flags' slot */
#define HB_GC_USERSWEEP   16  /* memory block with user defined sweep function */

/* pointer to memory block that will be checked in next step */
static HB_GARBAGE_PTR s_pCurrBlock = NULL;
/* memory blocks are stored in linked list with a loop */

/* pointer to locked memory blocks */
static HB_GARBAGE_PTR s_pLockedBlock = NULL;

/* list of functions that sweeps external memory blocks */
typedef struct _HB_GARBAGE_EXTERN {
   HB_GARBAGE_SWEEPER_PTR pFunc;
   void * pBlock;
   struct _HB_GARBAGE_EXTERN *pNext;
} HB_GARBAGE_EXTERN, *HB_GARBAGE_EXTERN_PTR;

static HB_GARBAGE_EXTERN_PTR s_pSweepExtern = NULL;

/* pointer to memory blocks that will be deleted */
static HB_GARBAGE_PTR s_pDeletedBlock = NULL;

/* marks if block releasing is requested during garbage collecting */
static BOOL s_bCollecting = FALSE;

/* flag for used/unused blocks - the meaning of the HB_GC_USED_FLAG bit
 * is reversed on every collecting attempt
 */
static USHORT s_uUsedFlag = HB_GC_USED_FLAG;

static void  hb_gcUnregisterSweep( void * Cargo );

static void hb_gcLink( HB_GARBAGE_PTR *pList, HB_GARBAGE_PTR pAlloc )
{
   if( *pList )
   {
      /* add new block at the logical end of list */
      pAlloc->pNext = *pList;
      pAlloc->pPrev = (*pList)->pPrev;
      pAlloc->pPrev->pNext = pAlloc;
      (*pList)->pPrev = pAlloc;
   }
   else
   {
      *pList = pAlloc->pNext = pAlloc->pPrev = pAlloc;
   }
}

static void hb_gcUnlink( HB_GARBAGE_PTR *pList, HB_GARBAGE_PTR pAlloc )
{
   pAlloc->pPrev->pNext = pAlloc->pNext;
   pAlloc->pNext->pPrev = pAlloc->pPrev;
   if( *pList == pAlloc )
   {
      *pList = pAlloc->pNext;
      if( *pList == pAlloc )
         *pList = NULL;    /* this was the last block */
   }
}

/* allocates a memory block */
void * hb_gcAlloc( ULONG ulSize, HB_GARBAGE_FUNC_PTR pCleanupFunc )
{
   HB_GARBAGE_PTR pAlloc;

   pAlloc = HB_GARBAGE_NEW( ulSize );
   if( pAlloc )
   {
      hb_gcLink( &s_pCurrBlock, pAlloc );
      pAlloc->pFunc  = pCleanupFunc;
      pAlloc->locked = 0;
      pAlloc->used   = s_uUsedFlag;
      pAlloc->flags  = 0;
      return HB_MEM_PTR( pAlloc );        /* hide the internal data */
   }
   else
      return NULL;
}

/* release a memory block allocated with hb_gcAlloc() */
void hb_gcFree( void *pBlock )
{
   if( pBlock )
   {
      HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pBlock );

      if( !( pAlloc->used & HB_GC_DELETE ) )
      {
         /* Don't release the block that will be deleted during finalization */
         if( pAlloc->locked )
            hb_gcUnlink( &s_pLockedBlock, pAlloc );
         else
            hb_gcUnlink( &s_pCurrBlock, pAlloc );

         if( pAlloc->flags & HB_GC_USERSWEEP )
            hb_gcUnregisterSweep( pBlock );

         HB_GARBAGE_FREE( pAlloc );
      }
   }
   else
   {
      hb_errInternal( HB_EI_XFREENULL, NULL, NULL, NULL );
   }
}

/* increment reference counter */
#undef hb_gcRefInc
void hb_gcRefInc( void * pBlock )
{
   hb_xRefInc( HB_GC_PTR( pBlock ) );
}

/* decrement reference counter, return TRUE when 0 reached */
#undef hb_gcRefDec
BOOL hb_gcRefDec( void * pBlock )
{
   return hb_xRefDec( HB_GC_PTR( pBlock ) );
}

/* decrement reference counter and free the block when 0 reached */
#undef hb_gcRefFree
void hb_gcRefFree( void * pBlock )
{
   if( pBlock )
   {
      HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pBlock );

      if( hb_xRefDec( pAlloc ) )
      {
         /* Don't release the block that will be deleted during finalization */
         if( !( pAlloc->used & HB_GC_DELETE ) )
         {
            /* unlink the block first to avoid possible problems
             * if cleanup function activate GC
             */
            if( pAlloc->locked )
               hb_gcUnlink( &s_pLockedBlock, pAlloc );
            else
               hb_gcUnlink( &s_pCurrBlock, pAlloc );

            pAlloc->used |= HB_GC_DELETE;

            /* execute clean-up function */
            if( pAlloc->pFunc )
               ( pAlloc->pFunc )( pBlock );

            if( pAlloc->used & HB_GC_DELETE )
               HB_GARBAGE_FREE( pAlloc );
         }
      }
   }
   else
   {
      hb_errInternal( HB_EI_XFREENULL, NULL, NULL, NULL );
   }
}


/* return number of references */
#undef hb_gcRefCount
HB_COUNTER hb_gcRefCount( void * pBlock )
{
   return hb_xRefCount( HB_GC_PTR( pBlock ) );
}


/*
 * Check if block still cannot be access after destructor execution
 */
void hb_gcRefCheck( void * pBlock )
{
   HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pBlock );

   if( !( pAlloc->used & HB_GC_DELETELST ) )
   {
      if( hb_xRefCount( pAlloc ) != 0 )
      {
         if( hb_vmRequestQuery() == 0 )
            hb_errRT_BASE( EG_DESTRUCTOR, 1301, NULL, "Reference to freed block", 0 );

         hb_gcLink( &s_pCurrBlock, pAlloc );
         pAlloc->used = s_uUsedFlag;
      }
   }
}


static HB_GARBAGE_FUNC( hb_gcGripRelease )
{
    /* Item was already released in hb_gcGripDrop() - then we have nothing
     * to do here
     */
    HB_SYMBOL_UNUSED( Cargo );
}

HB_ITEM_PTR hb_gcGripGet( HB_ITEM_PTR pOrigin )
{
   HB_GARBAGE_PTR pAlloc;

   pAlloc = HB_GARBAGE_NEW( sizeof( HB_ITEM ) );
   if( pAlloc )
   {
      HB_ITEM_PTR pItem = ( HB_ITEM_PTR ) HB_MEM_PTR( pAlloc );

      hb_gcLink( &s_pLockedBlock, pAlloc );
      pAlloc->pFunc  = hb_gcGripRelease;
      pAlloc->locked = 1;
      pAlloc->used   = s_uUsedFlag;
      pAlloc->flags  = 0;

      pItem->type = HB_IT_NIL;
      if( pOrigin )
         hb_itemCopy( pItem, pOrigin );

      return pItem;
   }
   else
      return NULL;
}

void hb_gcGripDrop( HB_ITEM_PTR pItem )
{
   if( pItem )
   {
      HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pItem );

      if( HB_IS_COMPLEX( pItem ) )
         hb_itemClear( pItem );    /* clear value stored in this item */

      hb_gcUnlink( &s_pLockedBlock, pAlloc );
      HB_GARBAGE_FREE( pAlloc );
   }
}

/* Lock a memory pointer so it will not be released if stored
   outside of harbour variables
*/
void * hb_gcLock( void * pBlock )
{
   if( pBlock )
   {
      HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pBlock );

      if( ! pAlloc->locked )
      {
         hb_gcUnlink( &s_pCurrBlock, pAlloc );
         hb_gcLink( &s_pLockedBlock, pAlloc );
      }
      ++pAlloc->locked;
   }

   return pBlock;
}

/* Unlock a memory pointer so it can be released if there is no
   references inside of harbour variables
*/
void * hb_gcUnlock( void * pBlock )
{
   if( pBlock )
   {
      HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pBlock );

      if( pAlloc->locked )
      {
         if( --pAlloc->locked == 0 )
         {
            hb_gcUnlink( &s_pLockedBlock, pAlloc );
            hb_gcLink( &s_pCurrBlock, pAlloc );
            pAlloc->used = s_uUsedFlag;
         }
      }
   }
   return pBlock;
}

/* Mark a passed item as used so it will be not released by the GC
*/
void hb_gcItemRef( HB_ITEM_PTR pItem )
{
   while( HB_IS_BYREF( pItem ) )
   {
      if( HB_IS_ENUM( pItem ) )
         return;
      else if( ! HB_IS_MEMVAR( pItem ) &&
               pItem->item.asRefer.offset == 0 &&
               pItem->item.asRefer.value >= 0 )
      {
         /* array item reference */
         HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pItem->item.asRefer.BasePtr.array );
         if( pAlloc->used == s_uUsedFlag )
         {
            ULONG ulSize = pItem->item.asRefer.BasePtr.array->ulLen;
            pAlloc->used ^= HB_GC_USED_FLAG;
            pItem = pItem->item.asRefer.BasePtr.array->pItems;
            while( ulSize )
            {
               hb_gcItemRef( pItem++ );
               --ulSize;
            }
         }
         return;
      }
      pItem = hb_itemUnRefOnce( pItem );
   }

   if( HB_IS_ARRAY( pItem ) )
   {
      HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pItem->item.asArray.value );

      /* Check this array only if it was not checked yet */
      if( pAlloc->used == s_uUsedFlag )
      {
         ULONG ulSize = pItem->item.asArray.value->ulLen;
         /* mark this block as used so it will be no re-checked from
          * other references
          */
         pAlloc->used ^= HB_GC_USED_FLAG;

         /* mark also all array elements */
         pItem = pItem->item.asArray.value->pItems;
         while( ulSize )
         {
            hb_gcItemRef( pItem++ );
            --ulSize;
         }
      }
   }
   else if( HB_IS_BLOCK( pItem ) )
   {
      HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pItem->item.asBlock.value );

      if( pAlloc->used == s_uUsedFlag )
      {
         HB_CODEBLOCK_PTR pCBlock = pItem->item.asBlock.value;
         USHORT ui = 1;

         pAlloc->used ^= HB_GC_USED_FLAG;  /* mark this codeblock as used */

         /* mark as used all detached variables in a codeblock */
         while( ui <= pCBlock->uiLocals )
         {
            hb_gcItemRef( &pCBlock->pLocals[ ui++ ] );
         }
      }
   }
   else if( HB_IS_POINTER( pItem ) )
   {
      if( pItem->item.asPointer.collect )
      {
         HB_GARBAGE_PTR pAlloc = HB_GC_PTR( pItem->item.asPointer.value );

         if( pAlloc->used == s_uUsedFlag )
         {
            if( ! (pAlloc->flags & HB_GC_USERSWEEP) )
            {
               /* Mark this pointer as used only if it doesn't have
                  registerd his own sweeper function that checks if
                  this pointer is still used.
                  The sweeper function will be called elsewhere.
               */
               pAlloc->used ^= HB_GC_USED_FLAG;  /* mark this codeblock as used */
            }
         }
      }
   }
   /* all other data types don't need the GC */
}

/* Register a function which sweeps memory blocks stored outside of
 * internal harbour structures
 *
 * NOTICE!: Cargo have to be a pointer to memory allocated with
 *          hb_gcAlloc()
 */
void hb_gcRegisterSweep( HB_GARBAGE_SWEEPER_PTR pSweep, void * Cargo )
{
   HB_GARBAGE_EXTERN_PTR pExt;

   pExt = ( HB_GARBAGE_EXTERN_PTR ) hb_xgrab( sizeof( HB_GARBAGE_EXTERN ) );
   pExt->pFunc = pSweep;
   pExt->pBlock = Cargo;
   pExt->pNext = s_pSweepExtern;
   s_pSweepExtern = pExt;

   /* set user sweep flag */
   HB_GC_PTR( Cargo )->flags ^= HB_GC_USERSWEEP;
}

static void hb_gcUnregisterSweep( void * Cargo )
{
   HB_GARBAGE_EXTERN_PTR pExt;
   HB_GARBAGE_EXTERN_PTR pPrev;

   pPrev = pExt = s_pSweepExtern;
   while( pExt )
   {
      if( pExt->pBlock == Cargo )
      {
         HB_GARBAGE_PTR pAlloc = HB_GC_PTR( Cargo );

         /* clear user sweep flag */
         pAlloc->flags &= ~ HB_GC_USERSWEEP;

         if( pExt == s_pSweepExtern )
         {
            s_pSweepExtern = pExt->pNext;
         }
         else
         {
            pPrev->pNext = pExt->pNext;
         }

         hb_xfree( (void *) pExt );
         pExt = NULL;
      }
      else
      {
         pPrev = pExt;
         pExt = pExt->pNext;
      }
   }
}

void hb_gcCollect( void )
{
   /* TODO: decrease the amount of time spend collecting */
   hb_gcCollectAll();
}

/* Check all memory block if they can be released
*/
void hb_gcCollectAll( void )
{
   if( s_pCurrBlock && !s_bCollecting )
   {
      HB_GARBAGE_PTR pAlloc, pDelete;

      s_bCollecting = TRUE;

      /* Step 1 - mark */
      /* All blocks are already marked because we are flipping
       * the used/unused flag
       */

      /* Step 2 - sweep */
      /* check all known places for blocks they are referring */
      hb_vmIsLocalRef();
      hb_vmIsStaticRef();
      hb_memvarsIsMemvarRef();
      hb_gcItemRef( hb_stackReturnItem() );
      hb_clsIsClassRef();

      if( s_pSweepExtern )
      {
         HB_GARBAGE_EXTERN_PTR *pExtPtr = &s_pSweepExtern;

         do
         {
            pAlloc = HB_GC_PTR( ( *pExtPtr )->pBlock );

            if( ( ( *pExtPtr )->pFunc )( ( *pExtPtr )->pBlock ) )
            {
               /* block is still used */
               pAlloc->used ^= HB_GC_USED_FLAG;
               pExtPtr = &( *pExtPtr )->pNext;
            }
            else
            {
               HB_GARBAGE_EXTERN_PTR pFree = *pExtPtr;

               pAlloc->flags &= ~ HB_GC_USERSWEEP;
               *pExtPtr = ( *pExtPtr )->pNext;
               hb_xfree( pFree );
            }
         }
         while( *pExtPtr );
      }

      /* check list of locked block for blocks referenced from
       * locked block
       */
      if( s_pLockedBlock )
      {
         pAlloc = s_pLockedBlock;
         do
         {  /* it is not very elegant method but it works well */
            if( pAlloc->pFunc == hb_gcGripRelease )
            {
               hb_gcItemRef( ( HB_ITEM_PTR ) HB_MEM_PTR( pAlloc ) );
            }
            pAlloc = pAlloc->pNext;
         } while( s_pLockedBlock != pAlloc );
      }

      /* Step 3 - finalize */
      /* Release all blocks that are still marked as unused */

      /*
       * infinite loop can appear when we are executing clean-up functions
       * scanning s_pCurrBlock. It's possible that one of them will free
       * the GC block which we are using as stop condition. Only blocks
       * for which we set HB_GC_DELETE flag are guarded against releasing.
       * To avoid such situation first we are moving blocks which will be
       * deleted to separate list. It's additional operation but it can
       * even increase the speed when we are deleting only few percent
       * of all allocated blocks because in next passes we will scan only
       * deleted block list. [druzus]
       */

      pAlloc = NULL; /* for stop condition */
      do
      {
         if( s_pCurrBlock->used == s_uUsedFlag )
         {
            pDelete = s_pCurrBlock;
            s_pCurrBlock->used |= HB_GC_DELETE | HB_GC_DELETELST;
            hb_gcUnlink( &s_pCurrBlock, s_pCurrBlock );
            hb_gcLink( &s_pDeletedBlock, pDelete );
         }
         else
         {
            /* at least one block will not be deleted, set new stop condition */
            if( ! pAlloc )
               pAlloc = s_pCurrBlock;
            s_pCurrBlock = s_pCurrBlock->pNext;
         }

      } while( pAlloc != s_pCurrBlock );

      /* do we have any deleted blocks? */
      if( s_pDeletedBlock )
      {
         /* call a cleanup function */
         pAlloc = s_pDeletedBlock;
         do
         {
            if( s_pDeletedBlock->pFunc )
               ( s_pDeletedBlock->pFunc )( HB_MEM_PTR( s_pDeletedBlock ) );

            s_pDeletedBlock = s_pDeletedBlock->pNext;

         } while( pAlloc != s_pDeletedBlock );

         /* release all deleted blocks */
         do
         {
            pDelete = s_pDeletedBlock;
            hb_gcUnlink( &s_pDeletedBlock, s_pDeletedBlock );
            if( hb_xRefCount( pDelete ) != 0 )
            {
               if( hb_vmRequestQuery() == 0 )
                  hb_errRT_BASE( EG_DESTRUCTOR, 1301, NULL, "Reference to freed block", 0 );

               hb_gcLink( &s_pCurrBlock, pAlloc );
               pAlloc->used = s_uUsedFlag;
            }
            else
               HB_GARBAGE_FREE( pDelete );

         } while( s_pDeletedBlock );
      }

      /* Step 4 - flip flag */
      /* Reverse used/unused flag so we don't have to mark all blocks
       * during next collecting
       */
      s_uUsedFlag ^= HB_GC_USED_FLAG;

      s_bCollecting = FALSE;
   }
}

void hb_gcReleaseAll( void )
{
   if( s_pCurrBlock )
   {
      HB_GARBAGE_PTR pAlloc, pDelete;

      s_bCollecting = TRUE;

      pAlloc = s_pCurrBlock;
      do
      {
         /* call a cleanup function */
         if( s_pCurrBlock->pFunc )
         {
            HB_TRACE( HB_TR_INFO, ( "Cleanup, %p", s_pCurrBlock ) );

            s_pCurrBlock->used |= HB_GC_DELETE | HB_GC_DELETELST;
            ( s_pCurrBlock->pFunc )( HB_MEM_PTR( s_pCurrBlock ) );
         }

         s_pCurrBlock = s_pCurrBlock->pNext;

      } while ( s_pCurrBlock && pAlloc != s_pCurrBlock );

      do
      {
         HB_TRACE( HB_TR_INFO, ( "Release %p", s_pCurrBlock ) );
         pDelete = s_pCurrBlock;
         hb_gcUnlink( &s_pCurrBlock, s_pCurrBlock );
         HB_GARBAGE_FREE( pDelete );

      } while ( s_pCurrBlock );
   }

   s_bCollecting = FALSE;
}

/* service a single garbage collector step
 * Check a single memory block if it can be released
*/
HB_FUNC( HB_GCSTEP )
{
   hb_gcCollect();
}

/* Check all memory blocks if they can be released
*/
HB_FUNC( HB_GCALL )
{
   hb_gcCollectAll();
}
