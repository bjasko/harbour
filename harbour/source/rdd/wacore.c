/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Default RDD module
 *
 * Copyright 1999 Bruno Cantero <bruno@issnet.net>
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

#include "hbapi.h"
#include "hbapirdd.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbvm.h"
#include "hbstack.h"
#include "hbthread.h"


#define HB_SET_WA( n )  do \
            { \
               pRddInfo->uiCurrArea = n; \
               pRddInfo->pCurrArea = ( ( pRddInfo->uiCurrArea < pRddInfo->uiWaNumMax ) ? \
                                 pRddInfo->waList[ pRddInfo->waNums[ pRddInfo->uiCurrArea ] ] : \
                                 NULL ); \
            } while( 0 )


/*
 * Insert new WorkArea node at current WA position
 */
static void hb_waNodeInsert( PHB_STACKRDD pRddInfo, AREAP pArea )
{
   USHORT uiWaPos;

   if( pRddInfo->uiCurrArea >= pRddInfo->uiWaNumMax )
   {
      int iSize = ( ( ( int ) pRddInfo->uiCurrArea + 256 ) >> 8 ) << 8;

      if( iSize > HB_RDD_MAX_AREA_NUM )
         iSize = HB_RDD_MAX_AREA_NUM;

      if( pRddInfo->uiWaNumMax == 0 )
      {
         pRddInfo->waNums = (USHORT *) hb_xgrab( iSize * sizeof(USHORT) );
      }
      else
      {
         pRddInfo->waNums = (USHORT *) hb_xrealloc( pRddInfo->waNums, iSize * sizeof(USHORT) );
      }
      memset( &pRddInfo->waNums[ pRddInfo->uiWaNumMax ], 0, ( iSize - pRddInfo->uiWaNumMax ) * sizeof(USHORT) );
      pRddInfo->uiWaNumMax = iSize;
   }

   if( pRddInfo->uiWaSpace == 0 )
   {
      pRddInfo->uiWaSpace = 256;
      pRddInfo->waList = ( void ** ) hb_xgrab( pRddInfo->uiWaSpace * sizeof(void *) );
      memset( &pRddInfo->waList[ 0 ], 0, pRddInfo->uiWaSpace * sizeof(void *) );
      pRddInfo->waList[ 0 ] = NULL;
      uiWaPos = 1;
      pRddInfo->uiWaMax = 2;
   }
   else
   {
      uiWaPos = pRddInfo->uiWaMax++;
      if( pRddInfo->uiWaMax > pRddInfo->uiWaSpace )
      {
         pRddInfo->uiWaSpace = ( ( pRddInfo->uiWaMax + 256 ) >> 8 ) << 8;
         pRddInfo->waList = ( void ** ) hb_xrealloc( pRddInfo->waList, pRddInfo->uiWaSpace * sizeof(void *) );
         memset( &pRddInfo->waList[ pRddInfo->uiWaMax ], 0, ( pRddInfo->uiWaSpace - pRddInfo->uiWaMax ) * sizeof(void *) );
      }
      while( uiWaPos > 1 )
      {
         if( ( ( AREAP ) pRddInfo->waList[ uiWaPos - 1 ] )->uiArea < pRddInfo->uiCurrArea )
            break;
         pRddInfo->waList[ uiWaPos ] = pRddInfo->waList[ uiWaPos - 1 ];
         pRddInfo->waNums[ ( ( AREAP ) pRddInfo->waList[ uiWaPos ] )->uiArea ] = uiWaPos;
         uiWaPos--;
      }
   }
   pRddInfo->waNums[ pRddInfo->uiCurrArea ] = uiWaPos;
   pRddInfo->pCurrArea = pRddInfo->waList[ uiWaPos ] = pArea;
   pArea->uiArea = pRddInfo->uiCurrArea;
}

/*
 * Remove current WorkArea node
 */
static void hb_waNodeDelete( PHB_STACKRDD pRddInfo )
{
   USHORT uiWaPos;

   uiWaPos = pRddInfo->waNums[ pRddInfo->uiCurrArea ];
   pRddInfo->waNums[ pRddInfo->uiCurrArea ] = 0;
   pRddInfo->uiWaMax--;
   if( pRddInfo->uiWaMax <= 1 )
   {
      pRddInfo->uiWaSpace = pRddInfo->uiWaMax = pRddInfo->uiWaNumMax = 0;
      hb_xfree( pRddInfo->waList );
      hb_xfree( pRddInfo->waNums );
      pRddInfo->waList = NULL;
      pRddInfo->waNums = NULL;
   }
   else
   {
      while( uiWaPos < pRddInfo->uiWaMax )
      {
         pRddInfo->waList[ uiWaPos ] = pRddInfo->waList[ uiWaPos + 1 ];
         pRddInfo->waNums[ ( ( AREAP ) pRddInfo->waList[ uiWaPos ] )->uiArea ] = uiWaPos;
         uiWaPos++;
      }
      pRddInfo->waList[ pRddInfo->uiWaMax ] = NULL;
      if( pRddInfo->uiWaSpace - pRddInfo->uiWaMax >= 256 )
      {
         pRddInfo->uiWaSpace = ( ( pRddInfo->uiWaMax + 256 ) >> 8 ) << 8;
         pRddInfo->waList = ( void ** ) hb_xrealloc( pRddInfo->waList, pRddInfo->uiWaSpace * sizeof( void * ) );
      }
   }
   pRddInfo->pCurrArea = NULL;
}

/*
 * Return the next free WorkArea for later use.
 */
HB_EXPORT ERRCODE hb_rddSelectFirstAvailable( void )
{
   PHB_STACKRDD pRddInfo;
   USHORT uiArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddSelectFirstAvailable()"));

   pRddInfo = hb_stackRDD();

   uiArea = 1;
   while( uiArea < pRddInfo->uiWaNumMax )
   {
      if( pRddInfo->waNums[ uiArea ] == 0 )
         break;
      uiArea++;
   }
   if( uiArea >= HB_RDD_MAX_AREA_NUM )
      return FAILURE;
   HB_SET_WA( uiArea );
   return SUCCESS;
}

/*
 * Creare and insert the new WorkArea node
 */
HB_EXPORT USHORT hb_rddInsertAreaNode( const char *szDriver )
{
   PHB_STACKRDD pRddInfo;
   LPRDDNODE pRddNode;
   USHORT uiRddID;
   AREAP pArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddInsertAreaNode(%s)", szDriver));

   pRddInfo = hb_stackRDD();
   if( pRddInfo->uiCurrArea && pRddInfo->pCurrArea )
      return 0;

   pRddNode = hb_rddFindNode( szDriver, &uiRddID );
   if( !pRddNode )
      return 0;

   pArea = ( AREAP ) hb_rddNewAreaNode( pRddNode, uiRddID );
   if( !pArea )
      return 0;

   if( pRddInfo->uiCurrArea == 0 )
   {
      if( hb_rddSelectFirstAvailable() != SUCCESS )
         return 0;
   }

   hb_waNodeInsert( pRddInfo, pArea );

   return pRddInfo->uiCurrArea;
}

/*
 * Closes and releases the current WorkArea preparing it
 * to be used with a new database.
 */
HB_EXPORT void hb_rddReleaseCurrentArea( void )
{
   PHB_STACKRDD pRddInfo;
   AREAP pArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddReleaseCurrentArea()"));

   pRddInfo = hb_stackRDD();
   pArea = ( AREAP ) pRddInfo->pCurrArea;
   if( !pArea )
      return;

   if( SELF_CLOSE( pArea ) == FAILURE )
      return;

   SELF_RELEASE( pArea );

   hb_waNodeDelete( pRddInfo );
}

/*
 * Closes all WorkAreas.
 */
HB_EXPORT void hb_rddCloseAll( void )
{
   PHB_STACKRDD pRddInfo;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddCloseAll()"));

   pRddInfo = hb_stackRDD();
   if( pRddInfo->uiWaMax > 0 )
   {
      BOOL isParents, isFinish = FALSE;
      AREAP pArea;
      USHORT uiIndex;

      do
      {
         isParents = FALSE;
         for( uiIndex = 1; uiIndex < pRddInfo->uiWaMax; uiIndex++ )
         {
            pArea = ( AREAP ) pRddInfo->waList[ uiIndex ];
            HB_SET_WA( pArea->uiArea );
            if( isFinish )
            {
               SELF_RELEASE( pArea );
               pRddInfo->waNums[ pRddInfo->uiCurrArea ] = 0;
               pRddInfo->pCurrArea = NULL;
            }
            else if( pArea->uiParents )
            {
               isParents = TRUE;
            }
            else
            {
               SELF_CLOSE( pArea );
            }
         }
         if( !isParents && !isFinish )
         {
            isParents = isFinish = TRUE;
         }
      }
      while( isParents );

      pRddInfo->uiWaSpace = pRddInfo->uiWaMax = pRddInfo->uiWaNumMax = 0;
      hb_xfree( pRddInfo->waList );
      hb_xfree( pRddInfo->waNums );
      pRddInfo->waList = NULL;
      pRddInfo->waNums = NULL;
      HB_SET_WA( 1 );
   }
}

HB_EXPORT void hb_rddFlushAll( void )
{
   PHB_STACKRDD pRddInfo = hb_stackRDD();
   USHORT uiArea = hb_rddGetCurrentWorkAreaNumber(), uiIndex;

   for( uiIndex = 1; uiIndex < pRddInfo->uiWaMax; ++uiIndex )
   {
      hb_rddSelectWorkAreaNumber( ( ( AREAP ) pRddInfo->waList[ uiIndex ] )->uiArea );
      SELF_FLUSH( ( AREAP ) pRddInfo->pCurrArea );
   }
   hb_rddSelectWorkAreaNumber( uiArea );
}

HB_EXPORT void hb_rddUnLockAll( void )
{
   PHB_STACKRDD pRddInfo = hb_stackRDD();
   USHORT uiArea = hb_rddGetCurrentWorkAreaNumber(), uiIndex;

   for( uiIndex = 1; uiIndex < pRddInfo->uiWaMax; ++uiIndex )
   {
      hb_rddSelectWorkAreaNumber( ( ( AREAP ) pRddInfo->waList[ uiIndex ] )->uiArea );
      SELF_UNLOCK( ( AREAP ) pRddInfo->pCurrArea, NULL );
   }
   hb_rddSelectWorkAreaNumber( uiArea );
}

/*
 * call a pCallBack function with all open workareas ###
 */
HB_EXPORT ERRCODE hb_rddIterateWorkAreas( WACALLBACK pCallBack, void * cargo )
{
   PHB_STACKRDD pRddInfo;
   ERRCODE errCode = SUCCESS;
   USHORT uiIndex;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddIterateWorkAreas(%p,%p)", pCallBack, cargo));

   pRddInfo = hb_stackRDD();
   for( uiIndex = 1; uiIndex < pRddInfo->uiWaMax; uiIndex++ )
   {
      errCode = pCallBack( ( AREAP ) pRddInfo->waList[ uiIndex ], cargo );
      if( errCode != SUCCESS )
         break;
   }
   return errCode;
}

HB_EXPORT BOOL hb_rddGetNetErr( void )
{
   return hb_stackRDD()->fNetError;
}

HB_EXPORT void hb_rddSetNetErr( BOOL fNetErr )
{
   hb_stackRDD()->fNetError = fNetErr;
}

/*
 * Get (/set) default RDD driver
 */
HB_EXPORT const char * hb_rddDefaultDrv( const char * szDriver )
{
   PHB_STACKRDD pRddInfo = hb_stackRDD();

   if( szDriver && *szDriver )
   {
      char szNewDriver[ HB_RDD_MAX_DRIVERNAME_LEN + 1 ];
      USHORT uiRddID;

      hb_strncpyUpper( szNewDriver, szDriver, sizeof( szNewDriver ) - 1 );
      if( !hb_rddFindNode( szNewDriver, &uiRddID ) )
         return NULL;

      pRddInfo->szDefaultRDD = hb_rddGetNode( uiRddID )->szName;
   }
   else if( !pRddInfo->szDefaultRDD && hb_rddGetNode( 0 ) )
   {
      const char *szDrvTable[] = { "DBFNTX", "DBFCDX", "DBFFPT", "DBF", NULL };
      int i;

      pRddInfo->szDefaultRDD = "";
      for( i = 0; szDrvTable[ i ]; ++i )
      {
         if( hb_rddFindNode( szDrvTable[ i ], NULL ) )
         {
            pRddInfo->szDefaultRDD = szDrvTable[ i ];
            break;
         }
      }
   }

   return pRddInfo->szDefaultRDD;
}

/*
 * Function for getting given workarea pointer
 */
HB_EXPORT void * hb_rddGetWorkAreaPointer( int iArea )
{
   PHB_STACKRDD pRddInfo;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddGetWorkAreaPointer(%d)", iArea));

   pRddInfo = hb_stackRDD();

   if( iArea == 0 )
      return pRddInfo->pCurrArea;
   else if( iArea >= 1 && ( UINT ) iArea < ( UINT ) pRddInfo->uiWaNumMax )
      return pRddInfo->waList[ pRddInfo->waNums[ iArea ] ];
   else
      return NULL;
}

/*
 * Function for getting current workarea pointer
 */
HB_EXPORT void * hb_rddGetCurrentWorkAreaPointer( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_rddGetCurrentWorkAreaPointer()"));

   return hb_stackRDD()->pCurrArea;
}

/*
 * Return the current WorkArea number.
 */
HB_EXPORT int hb_rddGetCurrentWorkAreaNumber( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_rddGetCurrentWorkAreaNumber()"));

   return hb_stackRDD()->uiCurrArea;
}

/*
 * Select a WorkArea by the number.
 */
HB_EXPORT ERRCODE hb_rddSelectWorkAreaNumber( int iArea )
{
   PHB_STACKRDD pRddInfo;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddSelectWorkAreaNumber(%d)", iArea));

   pRddInfo = hb_stackRDD();
   if( iArea < 1 || iArea > HB_RDD_MAX_AREA_NUM )
      HB_SET_WA( 0 );
   else
      HB_SET_WA( iArea );

   return ( pRddInfo->pCurrArea == NULL ) ? FAILURE : SUCCESS;
}


/* *********************************************************** */

/*
 * Moving work ares between threads
 */

static HB_CRITICAL_NEW( s_waMtx );
static HB_COND_NEW( s_waCond );
static PHB_ITEM s_pDetachedAreas = NULL;

static HB_GARBAGE_FUNC( hb_waHolderDestructor )
{
   AREAP * pHolder = ( AREAP * ) Cargo;

   if( *pHolder )
   {
      AREAP pArea;
      int iArea;

      pArea = *pHolder;
      *pHolder = NULL;

      iArea = hb_rddGetCurrentWorkAreaNumber();

      hb_rddSelectFirstAvailable();
      hb_waNodeInsert( hb_stackRDD(), pArea );
      hb_rddReleaseCurrentArea();

      hb_rddSelectWorkAreaNumber( iArea );
   }
}

void hb_rddCloseDetachedAreas( void )
{
   PHB_ITEM pDetachedArea;

   /* protect by critical section access to s_pDetachedAreas array */
   hb_threadEnterCriticalSection( &s_waMtx );
   pDetachedArea = s_pDetachedAreas;
   s_pDetachedAreas = NULL;
   /* leave critical section */
   hb_threadLeaveCriticalSection( &s_waMtx );
   /* release detached areas */
   if( pDetachedArea )
      hb_itemRelease( pDetachedArea );
}

HB_EXPORT ERRCODE hb_rddDetachArea( AREAP pArea, PHB_ITEM pCargo )
{
   AREAP * pHolder;
   PHB_ITEM pDetachedArea;
   ULONG ulPos;
   int iArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddDetachArea(%p,%p)", pArea, pCargo));

   /* save current WA number */
   iArea = hb_rddGetCurrentWorkAreaNumber();
   /* select given WA */
   hb_rddSelectWorkAreaNumber( pArea->uiArea );
   /* flush buffers */
   SELF_GOCOLD( pArea );

   /* tests shows that xbase++ does not remove locks */
   /* SELF_UNLOCK( pArea, NULL ); */

   /* xbase++ documentation says that child area are also detached but
    * but tests shows that it's not true and chilled and parent relations
    * are still active and corresponding WA are not detached together.
    * Harbour clears all child and parent relations.
    */
   SELF_CLEARREL( pArea );
   hb_rddCloseAllParentRelations( pArea );

   /* detach WA and alias */
   hb_waNodeDelete( hb_stackRDD() );
   pArea->uiArea = 0;
   if( pArea->atomAlias )
      hb_dynsymSetAreaHandle( ( PHB_DYNS ) pArea->atomAlias, 0 );

   /* restore previous WA number */
   hb_rddSelectWorkAreaNumber( iArea );

   /* protect by critical section access to s_pDetachedAreas array */
   hb_threadEnterCriticalSection( &s_waMtx );
   if( ! s_pDetachedAreas )
   {
      s_pDetachedAreas = hb_itemArrayNew( 1 );
      ulPos = 1;
   }
   else
   {
      ulPos = hb_arrayLen( s_pDetachedAreas ) + 1;
      hb_arraySize( s_pDetachedAreas, ulPos );
   }
   pDetachedArea = hb_arrayGetItemPtr( s_pDetachedAreas, ulPos );
   hb_arrayNew( pDetachedArea, 2 );
   if( pCargo )
      hb_arraySet( pDetachedArea, 2, pCargo );
   pHolder = ( AREAP * ) hb_gcAlloc( sizeof( AREAP ), hb_waHolderDestructor );
   *pHolder = pArea;
   hb_arraySetPtrGC( pDetachedArea, 1, pHolder );
   /* siagnal waiting processes that new area is available */
   hb_threadCondBroadcast( &s_waCond );
   /* leave critical section */
   hb_threadLeaveCriticalSection( &s_waMtx );

   return SUCCESS;
}

HB_EXPORT AREAP hb_rddRequestArea( char * szAlias, PHB_ITEM pCargo,
                                   BOOL fNewArea, BOOL fWait )
{
   PHB_DYNS pSymAlias = NULL;
   AREAP pArea = NULL;

   if( pCargo )
      hb_itemClear( pCargo );

   /* close current WA or chose 1-st free available */
   if( !fNewArea )
   {
      hb_rddReleaseCurrentArea();
   }
   else if( hb_rddSelectFirstAvailable() != SUCCESS )
   {
      hb_errRT_DBCMD( EG_ARG, EDBCMD_BADPARAMETER, NULL, HB_ERR_FUNCNAME );
      return NULL;
   }

   if( szAlias )
   {
      pSymAlias = hb_dynsymFindName( szAlias );

      /* verify if the alias name is valid symbol */
      if( hb_rddVerifyAliasName( szAlias ) != SUCCESS )
      {
         hb_errRT_DBCMD_Ext( EG_BADALIAS, EDBCMD_BADALIAS, NULL, szAlias, EF_CANDEFAULT );
         return NULL;
      }
      /* verify if the alias is already in use */
      if( hb_dynsymAreaHandle( pSymAlias ) != 0 )
      {
         hb_errRT_DBCMD_Ext( EG_DUPALIAS, EDBCMD_DUPALIAS, NULL, szAlias, EF_CANDEFAULT );
         return NULL;
      }
   }

   /* protect by critical section access to s_pDetachedAreas array */
   hb_threadEnterCriticalSection( &s_waMtx );
   for( ;; )
   {
      if( s_pDetachedAreas )
      {
         ULONG ulLen = hb_arrayLen( s_pDetachedAreas ), ulPos = 1;
         if( pSymAlias )
         {
            for( ulPos = 1; ulPos <= ulLen; ++ulPos )
            {
               AREAP * pDetachedArea = ( AREAP * )
                  hb_arrayGetPtr( hb_arrayGetItemPtr( s_pDetachedAreas, ulPos ), 1 );
               if( pSymAlias == ( PHB_DYNS ) ( *pDetachedArea )->atomAlias )
                  break;
            }
         }
         if( ulPos <= ulLen )
         {
            PHB_ITEM pArray = hb_arrayGetItemPtr( s_pDetachedAreas, ulPos );
            AREAP * pDetachedArea = ( AREAP * ) hb_arrayGetPtr( pArray, 1 );

            pArea = *pDetachedArea;
            *pDetachedArea = NULL;
            if( pCargo )
               hb_arrayGet( pArray, 2, pCargo );
            hb_arrayDel( s_pDetachedAreas, ulPos );
            hb_arraySize( s_pDetachedAreas, ulLen - 1 );
         }
      }

      if( pArea || !fWait )
         break;

      hb_vmUnlock();
      /* wait for detached workareas */
      hb_threadCondWait( &s_waCond, &s_waMtx );
      hb_vmLock();
      if( hb_vmRequestQuery() != 0 )
         break;
   }
   /* leave critical section */
   hb_threadLeaveCriticalSection( &s_waMtx );

   /* atach WA and set alias */
   if( pArea )
   {
      hb_waNodeInsert( hb_stackRDD(), pArea );
      if( pArea->atomAlias )
         hb_dynsymSetAreaHandle( ( PHB_DYNS ) pArea->atomAlias, pArea->uiArea );
   }

   return pArea;
}
