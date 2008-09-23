/*
 * $Id$
 */

/*
 * xHarbour Project source code:
 * DBFCDX RDD (ver.2)
 *
 * Copyright 1999-2002 Bruno Cantero <bruno@issnet.net>
 * Copyright 2000-2003 Horacio Roldan <harbour_ar@yahoo.com.ar> (portions)
 * Copyright 2003 Przemyslaw Czerpak <druzus@priv.onet.pl> - all code except
 * hb_cdxTagDoIndex and related hb_cdxSort* rewritten.
 * Copyright 2004 Przemyslaw Czerpak <druzus@priv.onet.pl> - rest of code rewritten
 * www - http://www.xharbour.org
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

#define HB_CDX_NEW_SORT

#if !defined( HB_SIXCDX )
#  define HB_CDX_PACKTRAIL
#endif

#define HB_CDX_DBGCODE
/*
#define HB_CDX_DBGCODE_EXT
#define HB_CDX_DSPDBG_INFO
#define HB_CDP_SUPPORT_OFF
#define HB_CDX_DBGTIME
#define HB_CDX_DBGUPDT
*/

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbinit.h"
#include "hbapierr.h"
#include "hbapilng.h"
#include "hbvm.h"
#include "hbset.h"
#include "hbrddcdx.h"
#include "hbmath.h"
#include "rddsys.ch"
#include "hbregex.h"

#define CDXNODE_DATA( p )     ( ( LPDBFDATA ) ( p )->lpvCargo )
#define CDXAREA_DATA( p )     CDXNODE_DATA( SELF_RDDNODE( p ) )

#ifndef HB_CDP_SUPPORT_OFF
   /* for nation sorting support */
   #include "hbapicdp.h"
   #define hb_cdpcharcmp( c1, c2, cdpage )  \
                                       ( ( cdpage && cdpage->lSort )    ? \
                                         hb_cdpchrcmp( c1, c2, cdpage ) : \
                                         ( (BYTE)(c1) - (BYTE)(c2) ) )
/*
   #define hb_cdpcharcmp( c1, c2, cdpage )     ( (BYTE)(c1) - (BYTE)(c2) )
 */
#endif

/*
 * Tag->fRePos = TURE means that rootPage->...->childLeafPage path is
 * bad and has to be reloaded
 * CurKey->rec == 0 means that there is no correct CurKey
 */

/* create a new Tag (make index) */
static void hb_cdxTagDoIndex( LPCDXTAG pTag, BOOL fReindex );

/* Close Tag */
static void hb_cdxTagClose( LPCDXTAG pTag );

/* free Tag pages from cache */
static void hb_cdxTagPoolFree( LPCDXTAG pTag, int nPagesLeft );

/* Store tag header to index files */
static void hb_cdxTagHeaderStore( LPCDXTAG pTag );

/* write all changed pages in tag cache */
static void hb_cdxTagPoolFlush( LPCDXTAG pTag );

/* Discard all pages in cache (TagClose and TagPoolFree for all Tags) */
static void hb_cdxIndexDiscardBuffers( LPCDXINDEX pIndex );

/* write all changed pages in cache (pagePool and Tag Header) */
static void hb_cdxIndexFlushBuffers( LPCDXINDEX pIndex );

/* free cached pages of index file */
static void hb_cdxIndexPoolFree( LPCDXINDEX pIndex, int nPagesLeft );

/* split Root Page */
static int hb_cdxPageRootSplit( LPCDXPAGE pPage );

/* free create index structur */
static void hb_cdxSortFree( LPCDXSORTINFO pSort );

static USHORT s_uiRddId = ( USHORT ) -1;

static RDDFUNCS cdxSuper;
static const RDDFUNCS cdxTable =
{

   /* Movement and positioning methods */

   ( DBENTRYP_BP )    hb_cdxBof,
   ( DBENTRYP_BP )    hb_cdxEof,
   ( DBENTRYP_BP )    hb_cdxFound,
   ( DBENTRYP_V )     hb_cdxGoBottom,
   ( DBENTRYP_UL )    hb_cdxGoTo,
   ( DBENTRYP_I )     hb_cdxGoToId,
   ( DBENTRYP_V )     hb_cdxGoTop,
   ( DBENTRYP_BIB )   hb_cdxSeek,
   ( DBENTRYP_L )     hb_cdxSkip,
   ( DBENTRYP_L )     hb_cdxSkipFilter,
   ( DBENTRYP_L )     hb_cdxSkipRaw,


   /* Data management */

   ( DBENTRYP_VF )    hb_cdxAddField,
   ( DBENTRYP_B )     hb_cdxAppend,
   ( DBENTRYP_I )     hb_cdxCreateFields,
   ( DBENTRYP_V )     hb_cdxDeleteRec,
   ( DBENTRYP_BP )    hb_cdxDeleted,
   ( DBENTRYP_SP )    hb_cdxFieldCount,
   ( DBENTRYP_VF )    hb_cdxFieldDisplay,
   ( DBENTRYP_SSI )   hb_cdxFieldInfo,
   ( DBENTRYP_SVP )   hb_cdxFieldName,
   ( DBENTRYP_V )     hb_cdxFlush,
   ( DBENTRYP_PP )    hb_cdxGetRec,
   ( DBENTRYP_SI )    hb_cdxGetValue,
   ( DBENTRYP_SVL )   hb_cdxGetVarLen,
   ( DBENTRYP_V )     hb_cdxGoCold,
   ( DBENTRYP_V )     hb_cdxGoHot,
   ( DBENTRYP_P )     hb_cdxPutRec,
   ( DBENTRYP_SI )    hb_cdxPutValue,
   ( DBENTRYP_V )     hb_cdxRecall,
   ( DBENTRYP_ULP )   hb_cdxRecCount,
   ( DBENTRYP_ISI )   hb_cdxRecInfo,
   ( DBENTRYP_ULP )   hb_cdxRecNo,
   ( DBENTRYP_I )     hb_cdxRecId,
   ( DBENTRYP_S )     hb_cdxSetFieldExtent,


   /* WorkArea/Database management */

   ( DBENTRYP_P )     hb_cdxAlias,
   ( DBENTRYP_V )     hb_cdxClose,
   ( DBENTRYP_VP )    hb_cdxCreate,
   ( DBENTRYP_SI )    hb_cdxInfo,
   ( DBENTRYP_V )     hb_cdxNewArea,
   ( DBENTRYP_VP )    hb_cdxOpen,
   ( DBENTRYP_V )     hb_cdxRelease,
   ( DBENTRYP_SP )    hb_cdxStructSize,
   ( DBENTRYP_P )     hb_cdxSysName,
   ( DBENTRYP_VEI )   hb_cdxEval,
   ( DBENTRYP_V )     hb_cdxPack,
   ( DBENTRYP_LSP )   hb_cdxPackRec,
   ( DBENTRYP_VS )    hb_cdxSort,
   ( DBENTRYP_VT )    hb_cdxTrans,
   ( DBENTRYP_VT )    hb_cdxTransRec,
   ( DBENTRYP_V )     hb_cdxZap,


   /* Relational Methods */

   ( DBENTRYP_VR )    hb_cdxChildEnd,
   ( DBENTRYP_VR )    hb_cdxChildStart,
   ( DBENTRYP_VR )    hb_cdxChildSync,
   ( DBENTRYP_V )     hb_cdxSyncChildren,
   ( DBENTRYP_V )     hb_cdxClearRel,
   ( DBENTRYP_V )     hb_cdxForceRel,
   ( DBENTRYP_SVP )   hb_cdxRelArea,
   ( DBENTRYP_VR )    hb_cdxRelEval,
   ( DBENTRYP_SI )    hb_cdxRelText,
   ( DBENTRYP_VR )    hb_cdxSetRel,


   /* Order Management */

   ( DBENTRYP_OI )    hb_cdxOrderListAdd,
   ( DBENTRYP_V )     hb_cdxOrderListClear,
   ( DBENTRYP_OI )    hb_cdxOrderListDelete,
   ( DBENTRYP_OI )    hb_cdxOrderListFocus,
   ( DBENTRYP_V )     hb_cdxOrderListRebuild,
   ( DBENTRYP_VOI )   hb_cdxOrderCondition,
   ( DBENTRYP_VOC )   hb_cdxOrderCreate,
   ( DBENTRYP_OI )    hb_cdxOrderDestroy,
   ( DBENTRYP_OII )   hb_cdxOrderInfo,


   /* Filters and Scope Settings */

   ( DBENTRYP_V )     hb_cdxClearFilter,
   ( DBENTRYP_V )     hb_cdxClearLocate,
   ( DBENTRYP_V )     hb_cdxClearScope,
   ( DBENTRYP_VPLP )  hb_cdxCountScope,
   ( DBENTRYP_I )     hb_cdxFilterText,
   ( DBENTRYP_SI )    hb_cdxScopeInfo,
   ( DBENTRYP_VFI )   hb_cdxSetFilter,
   ( DBENTRYP_VLO )   hb_cdxSetLocate,
   ( DBENTRYP_VOS )   hb_cdxSetScope,
   ( DBENTRYP_VPL )   hb_cdxSkipScope,
   ( DBENTRYP_B )     hb_cdxLocate,


   /* Miscellaneous */

   ( DBENTRYP_P )     hb_cdxCompile,
   ( DBENTRYP_I )     hb_cdxError,
   ( DBENTRYP_I )     hb_cdxEvalBlock,


   /* Network operations */

   ( DBENTRYP_VSP )   hb_cdxRawLock,
   ( DBENTRYP_VL )    hb_cdxLock,
   ( DBENTRYP_I )     hb_cdxUnLock,


   /* Memofile functions */

   ( DBENTRYP_V )     hb_cdxCloseMemFile,
   ( DBENTRYP_VP )    hb_cdxCreateMemFile,
   ( DBENTRYP_SVPB )  hb_cdxGetValueFile,
   ( DBENTRYP_VP )    hb_cdxOpenMemFile,
   ( DBENTRYP_SVPB )  hb_cdxPutValueFile,


   /* Database file header handling */

   ( DBENTRYP_V )     hb_cdxReadDBHeader,
   ( DBENTRYP_V )     hb_cdxWriteDBHeader,


   /* non WorkArea functions       */
   ( DBENTRYP_R )     hb_cdxInit,
   ( DBENTRYP_R )     hb_cdxExit,
   ( DBENTRYP_RVVL )  hb_cdxDrop,
   ( DBENTRYP_RVVL )  hb_cdxExists,
   ( DBENTRYP_RSLV )  hb_cdxRddInfo,


   /* Special and reserved methods */

   ( DBENTRYP_SVP )   hb_cdxWhoCares
};


#ifdef HB_CDX_DSPDBG_INFO
static void hb_cdxDspTags( LPCDXINDEX pIndex )
{
   LPCDXTAG pTag = NULL;

   printf( "\r\n*TAGS*" );
   while ( pIndex )
   {
      printf( "\r\nBAG: [%s] ->", pIndex->szFileName );
      pTag = pIndex->TagList;
      while ( pTag )
      {
         printf( " {%s}", pTag->szName );
         pTag = pTag->pNext;
      }
      pIndex = pIndex->pNext;
   }
   printf( "\r\n*END*\r\n" ); fflush( stdout );
}
#endif

#ifdef HB_CDX_DBGTIME
#include <sys/time.h>
typedef LONGLONG CDXDBGTIME;

static CDXDBGTIME cdxTimeIntBld = 0;
static CDXDBGTIME cdxTimeExtBld = 0;
static CDXDBGTIME cdxTimeIntBlc = 0;
static CDXDBGTIME cdxTimeExtBlc = 0;
static CDXDBGTIME cdxTimeGetKey = 0;
static CDXDBGTIME cdxTimeFreeKey = 0;
static CDXDBGTIME cdxTimeIdxBld = 0;

static CDXDBGTIME hb_cdxGetTime()
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return ( (CDXDBGTIME) tv.tv_sec * 1000000 + (CDXDBGTIME) tv.tv_usec );
}
#endif
#ifdef HB_CDX_DBGUPDT
static ULONG cdxWriteNO = 0;
static ULONG cdxReadNO = 0;
static SHORT cdxStackSize = 0;
static SHORT cdxTmpStackSize = 0;
#endif

/*
 * internal DBFCDX function
 */


/*
 * generate internal error
 */
static void hb_cdxErrInternal( const char * szMsg )
{
   hb_errInternal( 9201, szMsg ? szMsg : "hb_cdxErrInternal: data integrity error.", NULL, NULL );
}

/*
 * generate Run-Time error
 */
static ERRCODE hb_cdxErrorRT( CDXAREAP pArea, USHORT uiGenCode, USHORT uiSubCode, const char * filename, USHORT uiOsCode, USHORT uiFlags )
{
   PHB_ITEM pError;
   ERRCODE iRet = FAILURE;

   if ( hb_vmRequestQuery() == 0 )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, uiGenCode );
      hb_errPutSubCode( pError, uiSubCode );
      hb_errPutOsCode( pError, uiOsCode );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( uiGenCode ) );
      if ( filename )
         hb_errPutFileName( pError, filename );
      if ( uiFlags )
         hb_errPutFlags( pError, uiFlags );
      iRet = SELF_ERROR( ( AREAP ) pArea, pError );
      hb_errRelease( pError );
   }
   return iRet;
}

/*
 * create index sort table
 */
static void hb_cdxMakeSortTab( CDXAREAP pArea )
{
#ifndef HB_CDP_SUPPORT_OFF
   if ( pArea->cdPage && pArea->cdPage->lSort && !pArea->bCdxSortTab )
   {
      int i, j, l;
      BYTE * pbSort;
      BYTE b;

      pArea->bCdxSortTab = ( BYTE * ) hb_xgrab( 256 );
      pbSort = ( BYTE * ) hb_xgrab( 256 );
      /* this table should be allready quite good sorted so this simple
         algorithms will be one of the most efficient one. */
      for ( i = 0; i <= 255; i++ )
         pbSort[i] = ( BYTE ) i;
      l = 255;
      do
      {
         j = l;
         for( i = 0; i < j; i++ )
         {
            if ( hb_cdpchrcmp( pbSort[i], pbSort[i+1], pArea->cdPage ) > 0 )
            {
               b = pbSort[i+1];
               pbSort[i+1] = pbSort[i];
               pbSort[i] = b;
               l = i;
            }
         }
      } while ( j != l );
      for ( i = 0; i <= 255; i++ )
         pArea->bCdxSortTab[pbSort[i]] = i;
      hb_xfree( pbSort );
   }
#else
   HB_SYMBOL_UNUSED( pArea );
#endif
}

/*
 * create new index key
 */
static LPCDXKEY hb_cdxKeyNew( void )
{
   LPCDXKEY pKey;

   pKey = ( LPCDXKEY ) hb_xgrab( sizeof( CDXKEY ) );
   memset( pKey, 0, sizeof( CDXKEY ) );
   return pKey;
}

/*
 * Free index key
 */
static void hb_cdxKeyFree( LPCDXKEY pKey )
{
   if ( pKey->val )
       hb_xfree( pKey->val );
   hb_xfree( pKey );
}

/*
 * copy index key, if dst is null create new dst key else destroy dst
 */
static LPCDXKEY hb_cdxKeyCopy( LPCDXKEY pKeyDest, LPCDXKEY pKey )
{
   if ( !pKeyDest )
      pKeyDest = hb_cdxKeyNew();
   else
   {
      pKeyDest->rec = 0;
      if ( pKeyDest->val && pKeyDest->len != pKey->len )
      {
         hb_xfree( pKeyDest->val );
         pKeyDest->val = NULL;
         pKeyDest->len = 0;
      }
   }
   if ( pKey )
   {
      if ( pKey->len )
      {
         if ( !pKeyDest->val )
            pKeyDest->val = (BYTE *) hb_xgrab( pKey->len + 1 );
         memcpy( pKeyDest->val, pKey->val, pKey->len );
         pKeyDest->len = pKey->len;
         pKeyDest->val[ pKeyDest->len ] = '\0';
      }
      pKeyDest->rec = pKey->rec;
   }
   return pKeyDest;
}

/*
 * store bytes value in inkdex key
 */
static LPCDXKEY hb_cdxKeyPut( LPCDXKEY pKey, BYTE * pbVal, USHORT uiLen, ULONG ulRec )
{
   if ( !pKey )
      pKey = hb_cdxKeyNew();
   else
   {
      if ( pKey->val && pKey->len != uiLen )
      {
         hb_xfree( pKey->val );
         pKey->val = NULL;
         pKey->len = 0;
      }
   }
   if ( pbVal && uiLen )
   {
      pKey->len = (BYTE) uiLen;
      if ( !pKey->val )
         pKey->val = ( BYTE * ) hb_xgrab( uiLen + 1 );
      memcpy( pKey->val, pbVal, uiLen );
      pKey->val[ uiLen ] = '\0';
   }
   pKey->rec = ulRec;
   return pKey;
}

/*
 * store string0 value in index key
 */
static LPCDXKEY hb_cdxKeyPutC( LPCDXKEY pKey, char * szText, USHORT uiRealLen, ULONG ulRec  )
{
   USHORT uiLen;

   if ( !pKey )
      pKey = hb_cdxKeyNew();
   else
   {
      if ( pKey->val )
      {
         hb_xfree( pKey->val );
         pKey->val = NULL;
         pKey->len = 0;
      }
   }
   uiLen = (USHORT) ( szText ? strlen( szText ) : 0 );
   if ( uiLen > uiRealLen )
      uiLen = uiRealLen;
   pKey->len = ( BYTE ) uiRealLen;
   pKey->val = ( BYTE * ) hb_xgrab( uiRealLen + 1 );
   if ( uiLen )
      memcpy( pKey->val, szText, uiLen );
   if ( uiLen < uiRealLen )
      memset( &pKey->val[ uiLen ], ' ', uiRealLen - uiLen );
   pKey->val[ uiRealLen ] = '\0';
   pKey->rec = ulRec;
   return pKey;
}

/*
 * compare two values using Tag conditions (len & type)
 */
static int hb_cdxValCompare( LPCDXTAG pTag, BYTE * val1, BYTE len1,
                             BYTE * val2, BYTE len2, BOOL fExact )
{
   int iLimit, iResult = 0;

   iLimit = (len1 > len2) ? len2 : len1;

   if ( pTag->uiType == 'C' )
   {
#ifndef HB_CDP_SUPPORT_OFF
      if ( pTag->pIndex->pArea->bCdxSortTab )
      {
         BYTE * pSort = pTag->pIndex->pArea->bCdxSortTab;
         int iPos = 0;
         while ( iResult == 0 && iPos < iLimit )
         {
            iResult = pSort[ val1[ iPos ] ] - pSort[ val2[ iPos ] ];
            iPos++;
         }
      }
      else
#endif
         if ( iLimit > 0 )
            iResult = memcmp( val1, val2, iLimit );

      if ( iResult == 0 )
      {
         if ( len1 > len2 )
            iResult = 1;
         else if ( len1 < len2 && fExact )
            iResult = -1;
      }
   }
   else
   {
      if ( iLimit == 0 || (iResult = memcmp( val1, val2, iLimit )) == 0 )
      {
         if ( len1 > len2 )
            iResult = 1;
         else if ( len1 < len2 )
            iResult = -1;
      }
   }
   return iResult;
}

/*
 * get CDX key type for given item
 */
static BYTE hb_cdxItemType( PHB_ITEM pItem )
{
   switch( hb_itemType( pItem ) )
   {
      case HB_IT_STRING:
      case HB_IT_STRING | HB_IT_MEMO:
         return 'C';

      case HB_IT_INTEGER:
      case HB_IT_LONG:
      case HB_IT_DOUBLE:
         return 'N';

      case HB_IT_DATE:
         return 'D';

      case HB_IT_LOGICAL:
         return 'L';

      default:
         return 'U';
   }
}


/*
 * store Item in index key
 * TODO: uiType check and generate RT error if necessary
 */
static LPCDXKEY hb_cdxKeyPutItem( LPCDXKEY pKey, PHB_ITEM pItem, ULONG ulRec, LPCDXTAG pTag, BOOL fTrans, BOOL fSize )
{
   BYTE buf[CDX_MAXKEY], *ptr;
   ULONG ulLen = 0;
   double d;

   ptr = &buf[0];

   switch ( hb_cdxItemType( pItem ) )
   {
      case 'C':
         ulLen = hb_itemGetCLen( pItem );
         if( ulLen > (ULONG) pTag->uiLen )
            ulLen = pTag->uiLen;
         if ( fSize && ulLen < (ULONG) pTag->uiLen )
         {
            memcpy( ptr, hb_itemGetCPtr( pItem ), ulLen );
            memset( ptr + ulLen, pTag->bTrail, pTag->uiLen - ulLen );
            ulLen = pTag->uiLen;
         }
         else
         {
            ptr = ( BYTE * ) hb_itemGetCPtr( pItem );
         }
         break;
      case 'N':
         d = hb_itemGetND( pItem );
         HB_DBL2ORD( &d, ptr );
         ulLen = 8;
         break;
      case 'D':
         d = (double) hb_itemGetDL( pItem );
         HB_DBL2ORD( &d, ptr );
         ulLen = 8;
         break;
      case 'L':
         *ptr = (BYTE) ( hb_itemGetL( pItem ) ? 'T' : 'F' );
         ulLen = 1;
         break;
      default:
         ptr = NULL;
#ifdef HB_CDX_DBGCODE
         /* TODO: RTerror */
         printf( "hb_cdxKeyPutItem( invalid item type: %i )", hb_itemType( pItem ) );
#endif
         break;
   }
   pKey = hb_cdxKeyPut( pKey, ptr, ( USHORT ) ulLen, ulRec );
#ifndef HB_CDP_SUPPORT_OFF
   if ( fTrans && pTag->uiType == 'C' )
      hb_cdpnTranslate( ( char * ) pKey->val, hb_vmCDP(), pTag->pIndex->pArea->cdPage, pKey->len );
#else
   HB_SYMBOL_UNUSED( fTrans );
#endif
   return pKey;
}

/*
 * get Item from index key
 */
static PHB_ITEM hb_cdxKeyGetItem( LPCDXKEY pKey, PHB_ITEM pItem, LPCDXTAG pTag, BOOL fTrans )
{
   double d;

   if ( pKey )
   {
      switch( pTag->uiType )
      {
         case 'C':
#ifndef HB_CDP_SUPPORT_OFF
            if( fTrans && pTag->pIndex->pArea->cdPage != hb_vmCDP() )
            {
               char * pVal = ( char * ) hb_xgrab( pKey->len + 1 );
               memcpy( pVal, pKey->val, pKey->len );
               pVal[ pKey->len ] = '\0';
               hb_cdpnTranslate( pVal, pTag->pIndex->pArea->cdPage, hb_vmCDP(),
                                 pKey->len );
               pItem = hb_itemPutCLPtr( pItem, pVal, pKey->len );
            }
            else
#else
            HB_SYMBOL_UNUSED( fTrans );
#endif
            {
               pItem = hb_itemPutCL( pItem, ( char * ) pKey->val, pKey->len );
            }
            break;
         case 'N':
            HB_ORD2DBL( pKey->val, &d );
            pItem = hb_itemPutND( pItem, d );
            break;
         case 'D':
            HB_ORD2DBL( pKey->val, &d );
            pItem = hb_itemPutDL( pItem, ( LONG ) d );
            break;
         case 'L':
            pItem = hb_itemPutL( pItem, pKey->val[0] == 'T' );
            break;
         default:
            if ( pItem )
               hb_itemClear( pItem );
            else
               pItem = hb_itemNew( NULL );
#ifdef HB_CDX_DBGCODE
            printf( "hb_cdxKeyGetItem() ??? (%x)\n", pTag->uiType );
#endif
      }
   }
   else if ( pItem )
      hb_itemClear( pItem );
   else
      pItem = hb_itemNew( NULL );

   return pItem;
}

/*
 * evaluate key expression and create new Key from the result
 */
static LPCDXKEY hb_cdxKeyEval( LPCDXKEY pKey, LPCDXTAG pTag )
{
   CDXAREAP pArea = pTag->pIndex->pArea;
   PHB_ITEM pItem;
#ifndef HB_CDP_SUPPORT_OFF
   PHB_CODEPAGE cdpTmp = hb_cdpSelect( pArea->cdPage );
#endif

   if ( pTag->nField )
   {
      pItem = hb_itemNew( NULL );
      SELF_GETVALUE( ( AREAP ) pArea, pTag->nField, pItem );
      pKey = hb_cdxKeyPutItem( pKey, pItem, pArea->ulRecNo, pTag, FALSE, TRUE );
      hb_itemRelease( pItem );
   }
   else 
   {
      int iCurrArea = hb_rddGetCurrentWorkAreaNumber();

      if ( iCurrArea != pArea->uiArea )
         hb_rddSelectWorkAreaNumber( pArea->uiArea );
      else
         iCurrArea = 0;

      pItem = hb_vmEvalBlockOrMacro( pTag->pKeyItem );
      pKey = hb_cdxKeyPutItem( pKey, pItem, pArea->ulRecNo, pTag, FALSE, TRUE );

      if ( iCurrArea )
         hb_rddSelectWorkAreaNumber( iCurrArea );
   }

#ifndef HB_CDP_SUPPORT_OFF
   hb_cdpSelect( cdpTmp );
#endif

   return pKey;
}

/*
 * evaluate conditional expression and return the result
 */
static BOOL hb_cdxEvalCond( CDXAREAP pArea, PHB_ITEM pCondItem, BOOL fSetWA )
{
   int iCurrArea = 0;
   BOOL fRet;

   if ( fSetWA )
   {
      iCurrArea = hb_rddGetCurrentWorkAreaNumber();
      if ( iCurrArea != pArea->uiArea )
         hb_rddSelectWorkAreaNumber( pArea->uiArea );
      else
         iCurrArea = 0;
   }

   fRet = hb_itemGetL( hb_vmEvalBlockOrMacro( pCondItem ) );

   if ( iCurrArea )
      hb_rddSelectWorkAreaNumber( iCurrArea );

   return fRet;
}

/*
 * evaluate seek/skip block: {|key, rec| ... }
 */
static BOOL hb_cdxEvalSeekCond( LPCDXTAG pTag, PHB_ITEM pCondItem )
{
   BOOL fRet;
   PHB_ITEM pKeyVal, pKeyRec;

   pKeyVal = hb_cdxKeyGetItem( pTag->CurKey, NULL, pTag, TRUE );
   pKeyRec = hb_itemPutNInt( NULL, pTag->CurKey->rec );

   fRet = hb_itemGetL( hb_vmEvalBlockV( pCondItem, 2, pKeyVal, pKeyRec ) );

   hb_itemRelease( pKeyVal );
   hb_itemRelease( pKeyRec );

   return fRet;
}

/*
 * check if Key is in top scope
 */
static BOOL hb_cdxTopScope( LPCDXTAG pTag )
{
   LPCDXKEY pKey;

   if ( pTag->UsrAscend )
   {
      pKey = pTag->topScopeKey;
      return !pKey || !pKey->len ||
             hb_cdxValCompare( pTag, pKey->val, pKey->len,
                               pTag->CurKey->val, pTag->CurKey->len, FALSE ) <= 0;
   }
   else
   {
      pKey = pTag->bottomScopeKey;
      return !pKey || !pKey->len ||
             hb_cdxValCompare( pTag, pKey->val, pKey->len,
                               pTag->CurKey->val, pTag->CurKey->len, FALSE ) >= 0;
   }
}

/*
 * check if Key is in bottom scope
 */
static BOOL hb_cdxBottomScope( LPCDXTAG pTag )
{
   LPCDXKEY pKey;

   if ( pTag->UsrAscend )
   {
      pKey = pTag->bottomScopeKey;
      return !pKey || !pKey->len ||
             hb_cdxValCompare( pTag, pKey->val, pKey->len,
                               pTag->CurKey->val, pTag->CurKey->len, FALSE ) >= 0;
   }
   else
   {
      pKey = pTag->topScopeKey;
      return !pKey || !pKey->len ||
             hb_cdxValCompare( pTag, pKey->val, pKey->len,
                               pTag->CurKey->val, pTag->CurKey->len, FALSE ) <= 0;
   }
}

/*
 * clear top or bottom scope
 */
static void hb_cdxTagClearScope( LPCDXTAG pTag, USHORT nScope )
{
   CDXAREAP pArea = pTag->pIndex->pArea;
   LPCDXKEY *pScopeKey;
   PHB_ITEM *pScope;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxTagClearScope(%p, %hu)", pTag, nScope));

   /* resolve any pending scope relations first */
   if( pArea->lpdbPendingRel && pArea->lpdbPendingRel->isScoped )
      SELF_FORCEREL( ( AREAP ) pArea );

   if ( pTag->UsrAscend ? nScope == 0 : nScope != 0 )
   {
      pScope    = &pTag->topScope;
      pScopeKey = &pTag->topScopeKey;
   }
   else
   {
      pScope    = &pTag->bottomScope;
      pScopeKey = &pTag->bottomScopeKey;
   }
   if ( *pScope )
   {
      hb_itemRelease( *pScope );
      *pScope = NULL;
   }
   if ( *pScopeKey )
   {
      hb_cdxKeyFree( *pScopeKey );
      *pScopeKey = NULL;
      pTag->curKeyState &= ~( CDX_CURKEY_RAWCNT | CDX_CURKEY_LOGCNT );
      if ( nScope == 0 )
         pTag->curKeyState &= ~( CDX_CURKEY_RAWPOS | CDX_CURKEY_LOGPOS );
   }
}

/*
 * set top or bottom scope
 */
static void hb_cdxTagSetScope( LPCDXTAG pTag, USHORT nScope, PHB_ITEM pItem )
{
   CDXAREAP pArea = pTag->pIndex->pArea;
   PHB_ITEM pScopeVal;

   /* resolve any pending scope relations first */
   if( pArea->lpdbPendingRel && pArea->lpdbPendingRel->isScoped )
      SELF_FORCEREL( ( AREAP ) pArea );

   pScopeVal = ( hb_itemType( pItem ) == HB_IT_BLOCK ) ?
                           hb_vmEvalBlock( pItem ) : pItem;

   if ( pTag->uiType == hb_cdxItemType( pScopeVal ) )
   {
      PHB_ITEM *pScope;
      LPCDXKEY *pScopeKey;
      ULONG ulRec;

      if ( pTag->UsrAscend ? nScope == 0 : nScope != 0 )
      {
         pScope    = &(pTag->topScope);
         pScopeKey = &(pTag->topScopeKey);
         ulRec = CDX_IGNORE_REC_NUM;
      }
      else
      {
         pScope    = &(pTag->bottomScope);
         pScopeKey = &(pTag->bottomScopeKey);
         ulRec = CDX_MAX_REC_NUM;
      }

      if ( *pScope == NULL )
         *pScope = hb_itemNew( NULL );
      hb_itemCopy( *pScope, pItem );
      *pScopeKey = hb_cdxKeyPutItem( *pScopeKey, pScopeVal, ulRec, pTag, TRUE, FALSE );
      pTag->curKeyState &= ~( CDX_CURKEY_RAWCNT | CDX_CURKEY_LOGCNT );
      if ( nScope == 0 )
         pTag->curKeyState &= ~( CDX_CURKEY_RAWPOS | CDX_CURKEY_LOGPOS );
   }
   else
   {
      /* TODO: !!!
       * RT error: DBFCDX/1051  Scope Type Mismatch
       * hb_cdxErrorRT
       */
   }
}

static void hb_cdxTagGetScope( LPCDXTAG pTag, USHORT nScope, PHB_ITEM pItem )
{
   CDXAREAP pArea = pTag->pIndex->pArea;
   PHB_ITEM *pScope;

   /* resolve any pending scoped relations first */
   if( pArea->lpdbPendingRel && pArea->lpdbPendingRel->isScoped )
      SELF_FORCEREL( ( AREAP ) pArea );

   pScope = ( pTag->UsrAscend ? nScope == 0 : nScope != 0 ) ?
                     &(pTag->topScope) : &(pTag->bottomScope);
   if ( *pScope )
      hb_itemCopy( pItem, *pScope );
   else
      hb_itemClear( pItem );
}

/*
 * refresh top and bottom scope value if set as codeblock
 */
static void hb_cdxTagRefreshScope( LPCDXTAG pTag )
{
   PHB_ITEM pItem;

   if( pTag->pIndex->pArea->lpdbPendingRel &&
       pTag->pIndex->pArea->lpdbPendingRel->isScoped )
      SELF_FORCEREL( ( AREAP ) pTag->pIndex->pArea );

   if ( hb_itemType( pTag->topScope ) == HB_IT_BLOCK )
   {
      pItem = hb_vmEvalBlock( pTag->topScope );
      pTag->topScopeKey = hb_cdxKeyPutItem( pTag->topScopeKey, pItem, 
                                 pTag->topScopeKey->rec, pTag, TRUE, FALSE );
   }
   if ( hb_itemType( pTag->bottomScope ) == HB_IT_BLOCK )
   {
      pItem = hb_vmEvalBlock( pTag->bottomScope );
      pTag->bottomScopeKey = hb_cdxKeyPutItem( pTag->bottomScopeKey, pItem, 
                                 pTag->bottomScopeKey->rec, pTag, TRUE, FALSE );
   }
}

#ifdef HB_CDX_DBGCODE_EXT
/*
 * check internal integrity of page pool
 */
static void hb_cdxTagPoolCheck( LPCDXTAG pTag )
{
   LPCDXPAGE pPage, pPrevPage;

   pPage = pTag->pagePool;
   pPrevPage = NULL;
   while ( pPage )
   {
      if ( pPage->pPoolPrev != pPrevPage || pPage->TagParent != pTag )
         hb_cdxErrInternal( "hb_cdxTagPoolCheck: data integrity error." );
      pPrevPage = pPage;
      pPage = pPage->pPoolNext;
   }
}

/*
 * check if the Tag buffers was not changed without write lock
 */
static void hb_cdxTagCheckBuffers( LPCDXTAG pTag )
{
   BOOL fChanged = FALSE;

   hb_cdxTagPoolCheck( pTag );
   if ( pTag->TagChanged )
      fChanged = TRUE;
   else
   {
      LPCDXPAGE pPage = pTag->pagePool;
      while ( pPage && !fChanged )
      {
         fChanged = pPage->fChanged;
         pPage = pPage->pPoolNext;
      }
   }
   if ( fChanged )
      hb_cdxErrInternal( "hb_cdxTagCheckBuffers: modification without write lock." );
}

/*
 * check if the Index buffers was not changed without write lock
 */
static void hb_cdxIndexCheckBuffers( LPCDXINDEX pIndex )
{
   LPCDXTAG pTag;

   if ( pIndex->fChanged || ( pIndex->freeLst && pIndex->freeLst->fStat ) )
      hb_cdxErrInternal( "hb_cdxIndexCheckBuffers: modification without write lock." );

   if ( pIndex->pCompound )
      hb_cdxTagCheckBuffers( pIndex->pCompound );
   pTag = pIndex->TagList;
   while ( pTag )
   {
      hb_cdxTagCheckBuffers( pTag );
      pTag = pTag->pNext;
   }
}
#endif

/*
 * get free index page
 */
static ULONG hb_cdxIndexGetAvailPage( LPCDXINDEX pIndex, BOOL bHeader )
{
   HB_FHANDLE hFile = pIndex->hFile;
   BYTE byBuf[4];
   ULONG ulPos;

   if ( pIndex->fReadonly )
   {
      hb_errInternal( 9101, "hb_cdxIndexGetAvailPage on readonly database.", NULL, NULL );
   }
   if ( pIndex->fShared && !pIndex->lockWrite )
   {
      hb_errInternal( 9102, "hb_cdxIndexGetAvailPage on not locked index file.", NULL, NULL );
   }

   if ( pIndex->freePage != 0 && pIndex->freePage != CDX_DUMMYNODE && !bHeader )
   {
      ulPos = pIndex->freePage;
      if ( pIndex->freeLst != NULL )
      {
         LPCDXLIST pLst = pIndex->freeLst;
         pIndex->freePage = pLst->ulAddr;
         pIndex->freeLst = pLst->pNext;
         hb_xfree( pLst );
      }
      else
      {
         if ( hb_fsSeekLarge( hFile, ulPos, FS_SET ) != ( HB_FOFFSET ) ulPos ||
              hb_fsRead( hFile, (BYTE *) byBuf, 4 ) != 4 )
            hb_errInternal( EDBF_READ, "hb_cdxIndexGetAvailPage: Read index page failed.", NULL, NULL );
         pIndex->freePage = HB_GET_LE_UINT32( byBuf );
#ifdef HB_CDX_DBGUPDT
         cdxReadNO++;
#endif
      }
   }
   else
   {
      int iCnt = ( bHeader ? CDX_HEADERPAGES : 1 );

      if ( pIndex->nextAvail != CDX_DUMMYNODE )
         ulPos = pIndex->nextAvail;
      else
         ulPos = ( ULONG ) hb_fsSeekLarge( hFile, 0, FS_END );
      pIndex->nextAvail = ulPos + iCnt * CDX_PAGELEN;

      /* TODO: ### */
      if ( bHeader )
      {
         BYTE byBuf[CDX_PAGELEN];
         memset( byBuf, 0, CDX_PAGELEN );
         if ( hb_fsSeekLarge( hFile, ulPos, FS_SET ) != ( HB_FOFFSET ) ulPos )
            hb_errInternal( EDBF_WRITE, "Write in index page failed.(1)", NULL, NULL );
         while ( iCnt-- )
         {
            if ( hb_fsWrite( hFile, byBuf, CDX_PAGELEN ) != CDX_PAGELEN )
               hb_errInternal( EDBF_WRITE, "Write in index page failed.(2)", NULL, NULL );
         }
         pIndex->fChanged = TRUE;
      }
   }
   return ulPos;
}

/*
 * free index page
 */
static void hb_cdxIndexPutAvailPage( LPCDXINDEX pIndex, ULONG ulPos, BOOL bHeader )
{
   if ( ulPos != 0 && ulPos != CDX_DUMMYNODE )
   {
      int iCnt = ( bHeader ? CDX_HEADERPAGES : 1 );
      LPCDXLIST pLst;

      if ( pIndex->fReadonly )
         hb_errInternal( 9101, "hb_cdxIndexPutAvailPage on readonly database.", NULL, NULL );
      if ( pIndex->fShared && !pIndex->lockWrite )
         hb_errInternal( 9102, "hb_cdxIndexPutAvailPage on not locked index file.", NULL, NULL );

      while ( iCnt-- )
      {
         pLst = (LPCDXLIST) hb_xgrab( sizeof( CDXLIST ) );
         pLst->ulAddr = pIndex->freePage;
         pIndex->freePage = ulPos;
         pLst->fStat = TRUE;
         pLst->pNext = pIndex->freeLst;
         pIndex->freeLst = pLst;
         ulPos += CDX_PAGELEN;
      }
   }
}

/*
 * flush list of free pages into index file
 */
static void hb_cdxIndexFlushAvailPage( LPCDXINDEX pIndex )
{
   LPCDXLIST pLst = pIndex->freeLst;
   BYTE byPageBuf[CDX_PAGELEN];
   ULONG ulPos;
   BOOL fClean = TRUE;

   if ( pIndex->fReadonly )
      hb_errInternal( 9101, "hb_cdxIndexPutAvailPage on readonly database.", NULL, NULL );
   if ( pIndex->fShared && !pIndex->lockWrite )
      hb_errInternal( 9102, "hb_cdxIndexPutAvailPage on not locked index file.", NULL, NULL );

   ulPos = pIndex->freePage;
   while ( pLst && pLst->fStat )
   {
      if ( fClean )
      {
         memset( byPageBuf, 0, CDX_PAGELEN );
         fClean = FALSE;
      }
      HB_PUT_LE_UINT32( byPageBuf, pLst->ulAddr );
      if ( hb_fsSeekLarge( pIndex->hFile, ulPos, FS_SET ) != ( HB_FOFFSET ) ulPos ||
           hb_fsWrite( pIndex->hFile, byPageBuf, CDX_PAGELEN ) != CDX_PAGELEN )
      {
         hb_errInternal( EDBF_WRITE, "Write in index page failed.", NULL, NULL );
      }
      pIndex->fChanged = TRUE;
      ulPos = pLst->ulAddr;
      pLst->fStat = FALSE;
      pLst = pLst->pNext;
#ifdef HB_CDX_DBGUPDT
      cdxWriteNO++;
#endif
   }
}

/*
 * drop list of free pages in index file
 */
static void hb_cdxIndexDropAvailPage( LPCDXINDEX pIndex )
{
   LPCDXLIST pLst;

   while ( pIndex->freeLst )
   {
      pLst = pIndex->freeLst->pNext;
      hb_xfree( pIndex->freeLst );
      pIndex->freeLst = pLst;
   }
}

/*
 * write index page
 */
static void hb_cdxIndexPageWrite( LPCDXINDEX pIndex, ULONG ulPos, BYTE * pBuffer,
                                  USHORT uiSize )
{
   if ( pIndex->fReadonly )
      hb_errInternal( 9101, "hb_cdxIndexPageWrite on readonly database.", NULL, NULL );
   if ( pIndex->fShared && !pIndex->lockWrite )
      hb_errInternal( 9102, "hb_cdxIndexPageWrite on not locked index file.", NULL, NULL );

   if ( hb_fsSeekLarge( pIndex->hFile, ulPos, FS_SET ) != ( HB_FOFFSET ) ulPos ||
        hb_fsWrite( pIndex->hFile, pBuffer, uiSize ) != uiSize )
      hb_errInternal( EDBF_WRITE, "Write in index page failed.", NULL, NULL );
   pIndex->fChanged = TRUE;
#ifdef HB_CDX_DBGUPDT
   cdxWriteNO++;
#endif
}

/*
 * read index page
 */
static void hb_cdxIndexPageRead( LPCDXINDEX pIndex, ULONG ulPos, BYTE * pBuffer,
                                 USHORT uiSize )
{
   if ( pIndex->fShared && !( pIndex->lockRead || pIndex->lockWrite ) )
      hb_errInternal( 9103, "hb_cdxIndexPageRead on not locked index file.", NULL, NULL );

   if ( hb_fsSeekLarge( pIndex->hFile, ulPos, FS_SET ) != ( HB_FOFFSET ) ulPos ||
        hb_fsRead( pIndex->hFile, pBuffer, uiSize ) != uiSize )
      hb_errInternal( EDBF_READ, "hb_cdxIndexPageRead: Read index page failed.", NULL, NULL );
#ifdef HB_CDX_DBGUPDT
   cdxReadNO++;
#endif
}

/*
 * check if index was updated by other process and if it was discard buffers
 */
static void hb_cdxIndexCheckVersion( LPCDXINDEX pIndex )
{
   BYTE byBuf[8];
   ULONG ulVer, ulFree;

   if ( hb_fsSeek( pIndex->hFile, 0x04, FS_SET ) != 0x04 ||
       hb_fsRead( pIndex->hFile, byBuf, 8 ) != 8 )
   {
      if ( pIndex->lockWrite > 0 && hb_fsSeek( pIndex->hFile, 0, FS_END ) == 0 )
         memset( byBuf, 0, 8 );
      else
         hb_errInternal( 2155, "hb_cdxIndexCheckVersion: Read error on index heading page.", NULL, NULL );
   }
#ifdef HB_CDX_DBGUPDT
   cdxReadNO++;
#endif
   ulFree = HB_GET_LE_UINT32( &byBuf[0] );
   ulVer = HB_GET_BE_UINT32( &byBuf[4] );
   if ( !pIndex->fShared )
      pIndex->ulVersion = pIndex->freePage;
   else if ( ulVer != pIndex->ulVersion || ulFree != pIndex->freePage )
   {
      pIndex->nextAvail = CDX_DUMMYNODE;
      pIndex->ulVersion = ulVer;
      pIndex->freePage = ulFree;
      hb_cdxIndexDiscardBuffers( pIndex );
   }
   /* TODO: !!! ## remove it it's for test only */
   /* hb_cdxIndexDiscardBuffers( pIndex ); */
}

/*
 * lock index for reading (shared lock)
 */
static BOOL hb_cdxIndexLockRead( LPCDXINDEX pIndex )
{
   BOOL ret;

   if ( pIndex->lockRead > 0 || pIndex->lockWrite > 0 ||
        !pIndex->pArea->fShared || !pIndex->fShared ||
        HB_DIRTYREAD( pIndex->pArea ) )
   {
      pIndex->lockRead++;
      return TRUE;
   }
#ifdef HB_CDX_DBGCODE
   if ( pIndex->lockRead != 0 )
      hb_errInternal( 9105, "hb_cdxIndexLockRead: bad count of locks.", NULL, NULL );

   if ( pIndex->WrLck || pIndex->RdLck )
      hb_errInternal( 9107, "hb_cdxIndexLockRead: lock failure (*)", NULL, NULL );
   pIndex->RdLck = TRUE;
#endif

   ret = hb_dbfLockIdxFile( pIndex->hFile, pIndex->pArea->bLockType,
                            FL_LOCK | FLX_SHARED | FLX_WAIT, &pIndex->ulLockPos );
   if ( !ret )
      hb_cdxErrorRT( pIndex->pArea, EG_LOCK, EDBF_LOCK, pIndex->szFileName, hb_fsError(), 0 );

   if ( ret )
   {
      pIndex->lockRead++;
      hb_cdxIndexCheckVersion( pIndex );
   }
   return ret;
}

/*
 * lock index for writing (exclusive lock)
 */
static BOOL hb_cdxIndexLockWrite( LPCDXINDEX pIndex )
{
   BOOL ret;

   if ( pIndex->fReadonly )
      hb_errInternal( 9101, "hb_cdxIndexLockWrite: readonly index.", NULL, NULL );
   if ( pIndex->lockRead )
      hb_errInternal( 9105, "hb_cdxIndexLockWrite: writeLock after readLock.", NULL, NULL );
   if ( pIndex->lockWrite > 0 )
   {
      pIndex->lockWrite++;
      return TRUE;
   }
   if ( pIndex->lockWrite != 0 )
      hb_errInternal( 9105, "hb_cdxIndexLockWrite: bad count of locks.", NULL, NULL );

   if ( !pIndex->pArea->fShared || !pIndex->fShared )
      ret = TRUE;
   else
   {
#ifdef HB_CDX_DBGCODE
      if ( pIndex->WrLck || pIndex->RdLck )
         hb_errInternal( 9107, "hb_cdxIndexLockWrite: lock failure (*)", NULL, NULL );
      pIndex->WrLck = TRUE;
#endif
      ret = hb_dbfLockIdxFile( pIndex->hFile, pIndex->pArea->bLockType,
                               FL_LOCK | FLX_EXCLUSIVE | FLX_WAIT, &pIndex->ulLockPos );
   }
   if ( !ret )
      hb_cdxErrorRT( pIndex->pArea, EG_LOCK, EDBF_LOCK, pIndex->szFileName, hb_fsError(), 0 );

   if ( ret )
   {
      pIndex->lockWrite++;
      if ( pIndex->fShared || pIndex->nextAvail == CDX_DUMMYNODE )
         hb_cdxIndexCheckVersion( pIndex );
   }
   return ret;
}

/*
 * remove index read lock (shared lock)
 */
static BOOL hb_cdxIndexUnLockRead( LPCDXINDEX pIndex )
{
   pIndex->lockRead--;
   if ( pIndex->lockRead < 0 )
   {
      hb_errInternal( 9106, "hb_cdxIndexUnLockRead: bad count of locks.", NULL, NULL );
   }
   if ( pIndex->lockRead || pIndex->lockWrite )
   {
      return TRUE;
   }
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxIndexCheckBuffers( pIndex );
#endif

   hb_cdxIndexPoolFree( pIndex, CDX_PAGECACHESIZE );

   if ( pIndex->pArea->fShared && pIndex->fShared &&
        !HB_DIRTYREAD( pIndex->pArea ) )
   {
#ifdef HB_CDX_DBGCODE
      if ( pIndex->WrLck || ! pIndex->RdLck )
         hb_errInternal( 9108, "hb_cdxIndexUnLockRead: unlock error (*)", NULL, NULL );
      pIndex->RdLck = FALSE;
#endif
      if ( !hb_dbfLockIdxFile( pIndex->hFile, pIndex->pArea->bLockType, FL_UNLOCK, &pIndex->ulLockPos ) )
      {
         hb_errInternal( 9108, "hb_cdxIndexUnLockRead: unlock error.", NULL, NULL );
      }
   }
   return TRUE;
}

/*
 * remove index write lock (exclusive lock)
 */
static BOOL hb_cdxIndexUnLockWrite( LPCDXINDEX pIndex )
{
   if ( pIndex->lockWrite > 1 )
   {
      pIndex->lockWrite--;
      return TRUE;
   }

   if ( pIndex->lockWrite < 1 )
   {
      hb_errInternal( 9106, "hb_cdxIndexUnLockWrite: bad count of locks.", NULL, NULL );
   }
   if ( pIndex->lockRead )
   {
      hb_errInternal( 9105, "hb_cdxIndexUnLockWrite: writeUnLock before readUnLock.", NULL, NULL );
   }

   hb_cdxIndexFlushBuffers( pIndex );
   hb_cdxIndexPoolFree( pIndex, CDX_PAGECACHESIZE );

   pIndex->lockWrite--;
   if ( pIndex->pArea->fShared && pIndex->fShared )
   {
      if ( pIndex->fChanged )
      {
         BYTE byBuf[8];
         (pIndex->ulVersion)++;
         HB_PUT_LE_UINT32( &byBuf[0], pIndex->freePage );
         HB_PUT_BE_UINT32( &byBuf[4], pIndex->ulVersion );
         if ( hb_fsSeek( pIndex->hFile, 0x04, FS_SET ) != 0x04 ||
              hb_fsWrite( pIndex->hFile, byBuf, 8) != 8 )
         {
            hb_errInternal( EDBF_WRITE, "Write in index page failed (ver)", NULL, NULL );
         }
         pIndex->fFlush = TRUE;
         pIndex->fChanged = FALSE;
      }
#ifdef HB_CDX_DBGCODE
      if ( ! pIndex->WrLck || pIndex->RdLck )
         hb_errInternal( 9108, "hb_cdxIndexUnLockWrite: unlock error (*)", NULL, NULL );
      pIndex->WrLck = FALSE;
#endif
      if ( !hb_dbfLockIdxFile( pIndex->hFile, pIndex->pArea->bLockType, FL_UNLOCK, &pIndex->ulLockPos ) )
      {
         hb_errInternal( 9108, "hb_cdxIndexUnLockWrite: unlock error.", NULL, NULL );
      }
   }
   else
   {
      if ( pIndex->ulVersion != pIndex->freePage )
      {
         BYTE byBuf[4];
         HB_PUT_LE_UINT32( &byBuf[0], pIndex->freePage );
         if ( hb_fsSeek( pIndex->hFile, 0x04, FS_SET ) != 0x04 ||
              hb_fsWrite( pIndex->hFile, byBuf, 4) != 4 )
         {
            hb_errInternal( EDBF_WRITE, "Write in index page failed (ver.ex)", NULL, NULL );
         }
         pIndex->ulVersion = pIndex->freePage;
         pIndex->fFlush = TRUE;
#ifdef HB_CDX_DBGUPDT
         cdxWriteNO++;
#endif
      }
      else if ( pIndex->fChanged )
      {
         pIndex->fFlush = TRUE;
      }
      pIndex->fChanged = FALSE;
   }
   return TRUE;
}

/*
 * discard all pages in cache (TagClose and TagPoolFree for all Tags)
 */
static void hb_cdxIndexDiscardBuffers( LPCDXINDEX pIndex )
{
   LPCDXTAG pTag;

#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxIndexCheckBuffers( pIndex );
#endif

   hb_cdxIndexDropAvailPage( pIndex );
   if ( pIndex->pCompound )
   {
      hb_cdxTagClose( pIndex->pCompound );
      hb_cdxTagPoolFree( pIndex->pCompound, 0 );
      pIndex->pCompound->fRePos = TRUE;
      pIndex->pCompound->curKeyState = 0;
      if ( pIndex->pCompound->CurKey )
         pIndex->pCompound->CurKey->rec = 0;
   }
   pTag = pIndex->TagList;
   while ( pTag )
   {
      hb_cdxTagClose( pTag );
      hb_cdxTagPoolFree( pTag, 0 );
      pTag->fRePos = TRUE;
      pTag->curKeyState = 0;
      if ( pTag->CurKey && !pTag->Custom )
         pTag->CurKey->rec = 0;
      pTag = pTag->pNext;
   }
}

/*
 * write all changed pages in cache (pagePool, pages in Tags and Tag Header)
 */
static void hb_cdxIndexFlushBuffers( LPCDXINDEX pIndex )
{
   LPCDXTAG pTag;

   if ( pIndex->pCompound )
   {
      hb_cdxTagPoolFlush( pIndex->pCompound );
      if ( pIndex->pCompound->TagChanged )
         hb_cdxTagHeaderStore( pIndex->pCompound );
   }
   pTag = pIndex->TagList;
   while ( pTag )
   {
      hb_cdxTagPoolFlush( pTag );
      if ( pTag->TagChanged )
         hb_cdxTagHeaderStore( pTag );
      pTag = pTag->pNext;
   }
   hb_cdxIndexFlushAvailPage( pIndex );
}

/*
 * free cached pages of index file
 */
static void hb_cdxIndexPoolFree( LPCDXINDEX pIndex, int nPagesLeft )
{
   LPCDXTAG pTag;

   if ( pIndex->pCompound )
   {
      hb_cdxTagPoolFree( pIndex->pCompound, nPagesLeft );
   }
   pTag = pIndex->TagList;
   while ( pTag )
   {
      hb_cdxTagPoolFree( pTag, nPagesLeft );
      pTag = pTag->pNext;
   }
}

/*
 * get key value ptr from index page
 */
static BYTE * hb_cdxPageGetKeyVal( LPCDXPAGE pPage, int iKey )
{
#ifdef HB_CDX_DBGCODE
   if ( iKey < 0 || iKey >= pPage->iKeys )
      hb_cdxErrInternal( "hb_cdxPageGetKeyVal: wrong iKey index." );
#endif
   if ( pPage->pKeyBuf )
      return &pPage->pKeyBuf[ iKey * ( pPage->TagParent->uiLen +  6 ) ];
   else if ( pPage->PageType & CDX_NODE_LEAF )
   {
      int iPos, iLen, iTmp, iTrl, iDup;
      BYTE bTrail;

      iLen = pPage->TagParent->uiLen;
      bTrail = pPage->TagParent->bTrail;
      if ( iKey < pPage->bufKeyNum - 1 )
         pPage->bufKeyNum = 0;
      if ( pPage->bufKeyNum == 0 )
      {
         pPage->bufKeyPos = CDX_EXT_FREESPACE;
         pPage->bufKeyLen = iLen;
      }
      while ( pPage->bufKeyNum <= iKey )
      {
         iPos = pPage->bufKeyNum * pPage->ReqByte;
         iTmp = HB_GET_LE_UINT16( &pPage->node.extNode.keyPool[ iPos + pPage->ReqByte - 2 ] ) >>
                ( 16 - pPage->TCBits - pPage->DCBits );
         iDup = ( pPage->bufKeyNum == 0 ) ? 0 : ( iTmp & pPage->DCMask );
         iTrl = ( iTmp >> pPage->DCBits ) & pPage->TCMask;
         if ( ( iTmp = iLen - iDup - iTrl ) > 0 )
         {
            pPage->bufKeyPos -= iTmp;
            memcpy( &pPage->bufKeyVal[ iDup ],
                    &pPage->node.extNode.keyPool[ pPage->bufKeyPos ], iTmp );
         }
#ifdef HB_CDX_DBGCODE
         else if ( iTmp < 0 )
         {
            printf("\r\npPage->Page=%lx, iLen=%d, iDup=%d, iTrl=%d", pPage->Page, iLen, iDup, iTrl); fflush(stdout);
            hb_cdxErrInternal( "hb_cdxPageGetKeyVal: index corrupted." );
         }
#endif
         if ( iTrl > 0 && ( iTmp = pPage->bufKeyLen - iLen + iTrl ) > 0 )
            memset( &pPage->bufKeyVal[ iLen - iTrl ], bTrail, iTmp );
         pPage->bufKeyLen = iLen - iTrl;
         pPage->bufKeyNum++;
      }
      return pPage->bufKeyVal;
   }
   else
      return &pPage->node.intNode.keyPool[ iKey * ( pPage->TagParent->uiLen + 8 ) ];
}

/*
 * get record number from index page
 */
static ULONG hb_cdxPageGetKeyRec( LPCDXPAGE pPage, int iKey )
{
#ifdef HB_CDX_DBGCODE
   if ( iKey < 0 || iKey >= pPage->iKeys )
      hb_cdxErrInternal( "hb_cdxPageGetKeyRec: wrong iKey index." );
#endif
   if ( pPage->pKeyBuf )
      return HB_GET_LE_UINT32( &pPage->pKeyBuf[ ( iKey + 1 ) * ( pPage->TagParent->uiLen + 6 ) - 6 ] );
   else if ( pPage->PageType & CDX_NODE_LEAF )
      return HB_GET_LE_UINT32( &pPage->node.extNode.keyPool[ iKey * pPage->ReqByte ] ) & pPage->RNMask;
   else
      return HB_GET_BE_UINT32( &pPage->node.intNode.keyPool[
                        ( iKey + 1 ) * ( pPage->TagParent->uiLen + 8 ) - 8 ] );
}

/*
 * get child page number from interrior index page
 */
static ULONG hb_cdxPageGetKeyPage( LPCDXPAGE pPage, int iKey )
{
#ifdef HB_CDX_DBGCODE
   if ( iKey < 0 || iKey >= pPage->iKeys )
      hb_cdxErrInternal( "hb_cdxPageGetKeyPage: wrong iKey index." );
   if ( pPage->PageType & CDX_NODE_LEAF )
      hb_cdxErrInternal( "hb_cdxPageGetKeyPage: page is a leaf." );
#endif
   return HB_GET_BE_UINT32( &pPage->node.intNode.keyPool[
                        ( iKey + 1 ) * ( pPage->TagParent->uiLen + 8 ) - 4 ] );
}

#if 0
/*
 * get key from uncompressed page
 */
static LPCDXKEY hb_cdxPageGetKey( LPCDXPAGE pPage, int iKey, LPCDXKEY pKey )
{
   return hb_cdxKeyPut( pKey,
                        hb_cdxPageGetKeyVal( pPage, iKey ),
                        pPage->TagParent->uiLen,
                        hb_cdxPageGetKeyRec( pPage, iKey ) );
}
#endif

#ifdef HB_CDX_DBGCODE_EXT
/*
 * check if keys are sorted in proper order
 */
static void hb_cdxPageCheckKeys( LPCDXPAGE pPage )
{
   int i, K, iLen = pPage->TagParent->uiLen;
   ULONG ulRec, ulRecPrev;
   BYTE * pbVal, pbValPrev[CDX_MAXKEY];

   if ( pPage->iKeys > 1 )
   {
      pPage->bufKeyNum = 0;
      pbVal = hb_cdxPageGetKeyVal( pPage, 0 );
      ulRec = hb_cdxPageGetKeyRec( pPage, 0 );
      for ( i = 1; i < pPage->iKeys; i++ )
      {
         memcpy( pbValPrev, pbVal, iLen );
         ulRecPrev = ulRec;
         pbVal = hb_cdxPageGetKeyVal( pPage, i );
         ulRec = hb_cdxPageGetKeyRec( pPage, i );
         K = hb_cdxValCompare( pPage->TagParent,
                               pbValPrev, iLen,
                               pbVal, iLen, TRUE );
         if ( K > 0 || ( K == 0 && ulRecPrev >= ulRec ) )
         {
            printf( "\r\nikey=%d, pPage->iKeys=%d, K=%d, ulRecPrev=%ld, ulRec=%ld",
                    i, pPage->iKeys, K, ulRecPrev, ulRec );fflush(stdout);
            printf( "\r\npbValPrev=[%s] pbVal=[%s], [%d], pPage->pKeyBuf=%p, pPage->iCurKey=%d",
                    pbValPrev, pbVal, memcmp( pbValPrev, pbVal, iLen ),
                    pPage->pKeyBuf, pPage->iCurKey );fflush(stdout);
            hb_cdxErrInternal( "hb_cdxPageCheckKeys: index corrupted." );
         }
      }
   }
}

/*
 * Check decoded leaf page if all trailing and duplicate characters are set
 */
static void hb_cdxPageCheckDupTrl( LPCDXPAGE pPage, BYTE * pKeyBuf, int iKeys, BOOL fSpc )
{
   int iNum = pPage->TagParent->uiLen, iKey, iPos, iFree = CDX_EXT_FREESPACE;
   int iLen = iNum + 6;
   BYTE  bDup, bTrl;
   BYTE  bTrail = pPage->TagParent->bTrail;
   BOOL  bErr = FALSE;

   for ( iKey = 0; iKey < iKeys; iKey++ )
   {
      iPos = iKey * iLen;
      bTrl = bDup = 0;
      while ( bTrl < iNum && pKeyBuf[ iPos + iNum - bTrl - 1 ] == bTrail )
         ++bTrl;
      if ( iKey > 0 )
      {
#ifdef HB_CDX_PACKTRAIL
         int iMax = iNum - bTrl;
#else
         int iMax = iNum - HB_MAX( pKeyBuf[ iPos - 1 ], bTrl );
#endif
         while ( bDup < iMax && pKeyBuf[ iPos + bDup ] ==
                                pKeyBuf[ iPos - iLen + bDup ] )
            ++bDup;
      }
      if ( bTrl != pKeyBuf[ iPos + iNum + 5 ] )
      {
         printf("\r\nbTrl=%d, keybuf->bTrl=%d, iKey=%d/%d\r\n", bTrl, pKeyBuf[ iPos + iNum + 5 ], iKey, iKeys );
         fflush(stdout);
         bErr = TRUE;
      }
      if ( bDup != ( iKey == 0 ? 0 : pKeyBuf[ iPos + iNum + 4 ] ) )
      {
         printf("\r\nbDup=%d, keybuf->bDup=%d (bTrl=%d), iKey=%d/%d\r\n", bDup, pKeyBuf[ iPos + iNum + 4 ], bTrl, iKey, iKeys );
         fflush(stdout);
         bErr = TRUE;
      }
      if ( iKey > 0 )
      {
         int K;
         K = hb_cdxValCompare( pPage->TagParent,
                               &pKeyBuf[ iPos - iLen ], iNum,
                               &pKeyBuf[ iPos ], iNum, TRUE );
         if ( K > 0 || ( K == 0 &&
                         HB_GET_LE_UINT32( &pKeyBuf[ iPos + iNum - iLen ] ) >=
                         HB_GET_LE_UINT32( &pKeyBuf[ iPos + iNum ] ) ) )
         {
            printf( "\r\nikey=%d, iKeys=%d, K=%d, ulRecPrev=%ld, ulRec=%ld",
                    iKey, iKeys, K,
                    (ULONG) HB_GET_LE_UINT32( &pKeyBuf[ iPos + iNum - iLen ] ),
                    (ULONG) HB_GET_LE_UINT32( &pKeyBuf[ iPos + iNum ] ) );
            printf( "\r\npbValPrev=[%s] pbVal=[%s], [%d], pKeyBuf=%p",
                    &pKeyBuf[ iPos - iLen ], &pKeyBuf[ iPos ],
                    memcmp( &pKeyBuf[ iPos - iLen ], &pKeyBuf[ iPos ], iNum ),
                    pKeyBuf );
            fflush(stdout);
            bErr = TRUE;
         }
      }
      iFree -= iNum + pPage->ReqByte - bDup - bTrl;
   }
   if ( fSpc && ( iFree != pPage->iFree /* || iFree < 0 */ ) )
   {
      printf( "\r\nFreeSpace calculated wrong! iFree=%d, pPage->iFree=%d",
              iFree, pPage->iFree );
      fflush(stdout);
      bErr = TRUE;
   }
   if ( bErr )
   {
      printf("\r\nPage=%lx, Page->iFree=%d, iLen=%d\r\n", pPage->Page, pPage->iFree, iNum );
      fflush(stdout);
      hb_cdxErrInternal( "hb_cdxPageCheckDupTrl: index corrupted." );
   }
}

static void hb_cdxPageLeafDecode( LPCDXPAGE pPage, BYTE * pKeyBuf );
static void hb_cdxPageCheckDupTrlRaw( LPCDXPAGE pPage )
{
   BYTE *pKeyBuf = (BYTE *) hb_xgrab( pPage->iKeys * ( pPage->TagParent->uiLen + 6 ) );

   hb_cdxPageLeafDecode( pPage, pKeyBuf );
   hb_cdxPageCheckDupTrl( pPage, pKeyBuf, pPage->iKeys, TRUE );
   hb_xfree( pKeyBuf );
}
#endif

/*
 * put record and duplicate + trailing counters into leaf page
 */
static void hb_cdxSetLeafRecord( BYTE *pDst, ULONG ulRec, int iDup, int iTrl,
                                 int iReq, int iDCbits, int iTCbits )
{
   int i;
   USHORT usBit;

   usBit = ( ( iTrl << iDCbits ) | iDup ) << ( 16 - iTCbits - iDCbits );
   for ( i = 0; i < iReq; i++, ulRec >>= 8 )
   {
      if ( i < iReq - 2 )
         pDst[ i ] = ( BYTE ) ( ulRec & 0xff );
      else if ( i == iReq - 2 )
         pDst[ i ] = ( BYTE ) ( ulRec & 0xff ) | ( usBit & 0xff );
      else
         pDst[ i ] = ( BYTE ) ( ulRec & 0xff ) | ( ( usBit >> 8 ) & 0xff );
   }
}

/*
 * encode keys in buffer into cdx leaf node
 */
static void hb_cdxPageLeafEncode( LPCDXPAGE pPage, BYTE * pKeyBuf, int iKeys )
{
   int iKey, iTrl, iDup, iReq, iTmp, iNum, iLen;
   BYTE *pKeyPos, *pRecPos, *pSrc;

#ifdef HB_CDX_DBGCODE
   if ( ( pPage->PageType & CDX_NODE_LEAF ) == 0 )
   {
      printf("\r\npPage->Page=%lx. left=%lx, right=%lx",
             pPage->Page, pPage->Left, pPage->Right); fflush(stdout);
      hb_cdxErrInternal( "hb_cdxPageLeafEncode: page is not a leaf." );
   }
#endif
#ifdef HB_CDX_DBGCODE_EXT
   if ( ! pKeyBuf )
      hb_cdxErrInternal( "hb_cdxPageLeafEncode: page has no buffer." );
   hb_cdxPageCheckDupTrl( pPage, pKeyBuf, iKeys, TRUE );
#endif
   iNum = pPage->TagParent->uiLen;
   iLen = iNum + 6;
   iReq = pPage->ReqByte;
   pKeyPos = &pPage->node.extNode.keyPool[ CDX_EXT_FREESPACE ];
   pRecPos = &pPage->node.extNode.keyPool[ 0 ];
   pSrc = &pKeyBuf[ 0 ];
   for ( iKey = 0; iKey < iKeys; iKey++, pSrc += iLen, pRecPos += iReq )
   {
      iDup = pSrc[ iNum + 4 ];
      iTrl = pSrc[ iNum + 5 ];
      iTmp = iNum - iTrl - iDup;
      hb_cdxSetLeafRecord( pRecPos, HB_GET_LE_UINT32( &pSrc[ iNum ] ), iDup, iTrl,
                           iReq, pPage->DCBits, pPage->TCBits );
      if ( iTmp > 0 )
      {
         pKeyPos -= iTmp;
         memcpy( pKeyPos, &pSrc[ iDup ], iTmp );
      }
#ifdef HB_CDX_DBGCODE
      else if ( iTmp < 0 )
      {
         printf("\r\n[%s][%s]", pSrc - iLen, pSrc);
         printf("\r\npPage->Page=0x%lx, iKey=%d, iNum=%d, iDup=%d, iTrl=%d", pPage->Page, iKey, iNum, iDup, iTrl); fflush(stdout);
         hb_cdxErrInternal( "hb_cdxPageLeafEncode: index corrupted." );
      }
#endif
   }
   if ( pRecPos < pKeyPos )
      memset( pRecPos, 0, pKeyPos - pRecPos );
#ifdef HB_CDX_DBGCODE
   if ( pKeyPos - pRecPos != pPage->iFree )
   {
      printf("\r\nPage=0x%lx, calc=%d, iFree=%d, req=%d, keys=%d, keyLen=%d\r\n",
             pPage->Page, (int) (pKeyPos - pRecPos), pPage->iFree, pPage->ReqByte, iKeys, iNum );
      fflush(stdout);
      hb_cdxErrInternal( "hb_cdxPageLeafEncode: FreeSpace calculated wrong!." );
   }
   if ( pPage->iFree < 0 )
      hb_cdxErrInternal( "hb_cdxPageLeafEncode: FreeSpace calculated wrong!!." );
#endif
   pPage->iKeys = iKeys;
   pPage->fChanged = TRUE;
   pPage->bufKeyNum = 0;
#ifdef HB_CDX_DBGCODE_EXT_EXT
   {
      BYTE * pKeyBf = pPage->pKeyBuf;
      pPage->pKeyBuf = NULL;
      hb_cdxPageCheckKeys( pPage );
      pPage->pKeyBuf = pKeyBf;
   }
   hb_cdxPageCheckKeys( pPage );
   hb_cdxPageCheckDupTrl( pPage, pKeyBuf, pPage->iKeys, TRUE );
#endif
}

/*
 * decode keys in page into buffer
 */
static void hb_cdxPageLeafDecode( LPCDXPAGE pPage, BYTE * pKeyBuf )
{
   int iKey, iTmp, iBits, iDup, iTrl, iNew, iReq, iLen = pPage->TagParent->uiLen;
   BYTE *pDst, *pSrc, *pRec, *pTmp, bTrail = pPage->TagParent->bTrail;
   ULONG ulRec;

#ifdef HB_CDX_DBGCODE
   if ( ( pPage->PageType & CDX_NODE_LEAF ) == 0 )
   {
      printf("\r\npPage->Page=%lx", pPage->Page); fflush(stdout);
      hb_cdxErrInternal( "hb_cdxPageLeafDecode: page is not a leaf." );
   }
#endif
   iBits = ( 16 - pPage->TCBits - pPage->DCBits );
   pDst = pKeyBuf;
   pSrc = &pPage->node.extNode.keyPool[ CDX_EXT_FREESPACE ];
   pRec = pPage->node.extNode.keyPool;
   iReq = pPage->ReqByte;
   for ( iKey = 0; iKey < pPage->iKeys; iKey++, pRec += iReq )
   {
      pTmp = &pRec[ iReq - 2 ];
      iTmp = HB_GET_LE_UINT16( pTmp ) >> iBits;
      iDup = ( iKey == 0 ) ? 0 : ( iTmp & pPage->DCMask );
      iTrl = ( iTmp >> pPage->DCBits ) & pPage->TCMask;
      iNew = iLen - iDup - iTrl;
      if ( iDup > 0 )
      {
         memcpy( pDst, pDst - iLen - 6, iDup );
         pDst += iDup;
      }
      if ( iNew > 0 )
      {
         pSrc -= iNew;
         memcpy( pDst, pSrc, iNew );
         pDst += iNew;
      }
#ifdef HB_CDX_DBGCODE
      else if ( iNew < 0 )
      {
         printf("\r\npPage->Page=%lx, iLen=%d, iDup=%d, iTrl=%d", pPage->Page, iLen, iDup, iTrl); fflush(stdout);
         hb_cdxErrInternal( "hb_cdxPageLeafDecode: index corrupted." );
      }
#endif
      if ( iTrl > 0 )
      {
         memset( pDst, bTrail, iTrl );
         pDst += iTrl;
      }
      ulRec = HB_GET_LE_UINT32( pRec ) & pPage->RNMask;
      HB_PUT_LE_UINT32( pDst, ulRec );
      pDst += 4;
      *(pDst++) = ( BYTE ) iDup;
      *(pDst++) = ( BYTE ) iTrl;
   }
#ifdef HB_CDX_DBGCODE_EXT
   {
      BOOL fChg = pPage->fChanged;
      hb_cdxPageLeafEncode( pPage, pKeyBuf, pPage->iKeys );
      pPage->fChanged = fChg;
   }
#endif
}

/*
 * init space leaf page
 */
static void hb_cdxPageLeafInitSpace( LPCDXPAGE pPage )
{
   int iLen = pPage->TagParent->uiLen;
   BYTE  bBits;

   for ( bBits = 0; iLen; bBits++, iLen >>= 1 ) {}
   pPage->ReqByte = 3;
   pPage->RNBits  = 24 - ( bBits << 1 );
   pPage->DCBits  = pPage->TCBits = bBits;
   pPage->DCMask  = pPage->TCMask = (BYTE) HB_CDXBITMASK( bBits );
   pPage->RNMask  = HB_CDXBITMASK( pPage->RNBits );
   pPage->iFree   = CDX_EXT_FREESPACE;
}

/*
 * calculate the size of keys stored in buffer, return
 * the number of keys wich can be stored in the page
 */
static void hb_cdxPageCalcLeafSpace( LPCDXPAGE pPage, BYTE * pKeyBuf, int iKeys )
{
   int iNum = pPage->TagParent->uiLen, iKey, iSize;
   int iLen = iNum + 6;
   BYTE  bDup, bTrl, ReqByte, *bPtr;
   ULONG ulRec, RNMask;

   hb_cdxPageLeafInitSpace( pPage );
   pPage->iKeys = 0;
   RNMask = pPage->RNMask;
   ReqByte = pPage->ReqByte;
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckDupTrl( pPage, pKeyBuf, iKeys, FALSE );
#endif
   for ( iKey = 0; iKey < iKeys; iKey++ )
   {
      bPtr = &pKeyBuf[ iKey * iLen + iNum ];
      bTrl = bPtr[ 5 ];
      if ( iKey == 0 )
         bDup = bPtr[ 4 ] = 0;
      else
         bDup = bPtr[ 4 ];
      ulRec = HB_GET_LE_UINT32( bPtr );
      iSize = ReqByte + iNum - bTrl - bDup;
      if ( ulRec > RNMask )
      {
         BYTE RNBits = pPage->RNBits;
         while ( ulRec > RNMask )
         {
            ReqByte++;
            RNBits += 8;
            RNMask = ( RNMask << 8 ) | 0xFF;
            iSize += ( iKey + 1 );
         }
         if ( iSize > pPage->iFree )
            break;
#ifdef HB_CDX_DSPDBG_INFO_X
         printf("\r\npPage->Page=%lx, ulRec=%lx, RNMask=%lx/%lx, RNBits=%d/%d, DCB=%d, TCB=%d (%lx), iKey=%d/%d",
                pPage->Page, ulRec, RNMask, pPage->RNMask, RNBits, pPage->RNBits,
                pPage->DCBits, pPage->TCBits, HB_CDXBITMASK( RNBits ), iKey, iKeys);
         fflush(stdout);
#endif
         pPage->RNMask = RNMask;
         pPage->RNBits = RNBits;
         pPage->ReqByte = ReqByte;
      }
      else if ( iSize > pPage->iFree )
         break;
      pPage->iFree -= iSize;
      pPage->iKeys++;
   }
}

/*
 * remove key from page
 */
static int hb_cdxPageLeafDelKey( LPCDXPAGE pPage )
{
   int iKey = pPage->iCurKey, iLen = pPage->TagParent->uiLen + 6, iSpc;
   int iRet = 0;

#ifdef HB_CDX_DBGCODE
   if ( ( pPage->PageType & CDX_NODE_LEAF ) == 0 )
      hb_cdxErrInternal( "hb_cdxPageLeafDelKey: page is not a leaf." );
   if ( iKey < 0 || iKey >= pPage->iKeys )
      hb_cdxErrInternal( "hb_cdxPageLeafDelKey: wrong iKey index." );
#endif
   if ( !pPage->pKeyBuf )
   {
      BYTE *pKeyBuf = (BYTE *) hb_xgrab( ( pPage->iKeys ) * iLen );
      hb_cdxPageLeafDecode( pPage, pKeyBuf );
      pPage->pKeyBuf = pKeyBuf;
   }
#ifdef HB_CDX_DSPDBG_INFO
   printf("\r\ndelkey: Page=%lx, iKey=%d/%d, rec=%ld, iFree=%d",
          pPage->Page, iKey, pPage->iKeys,
          (ULONG) HB_GET_LE_UINT32( &pPage->pKeyBuf[ ( iKey + 1 ) * iLen - 6 ] ),
          pPage->iFree );
   fflush(stdout);
#endif
   iSpc = pPage->ReqByte + pPage->TagParent->uiLen -
          pPage->pKeyBuf[ ( iKey + 1 ) * iLen - 2 ] -
          pPage->pKeyBuf[ ( iKey + 1 ) * iLen - 1 ];
   if ( iKey < pPage->iKeys - 1 )
   {
      int iPos = ( iKey + 2 ) * iLen - 2, iDup = 0;
      iSpc -= pPage->pKeyBuf[ iPos ];
      if ( iKey > 0 )
      {
         int iPrev = ( iKey - 1 ) * iLen, iNext = ( iKey + 1 ) * iLen,
             iNum = pPage->TagParent->uiLen;
#ifdef HB_CDX_PACKTRAIL
         iNum -= pPage->pKeyBuf[ iNext + iLen - 1 ];
#else
         iNum -= HB_MAX( pPage->pKeyBuf[ iNext + iLen - 1 ],
                         pPage->pKeyBuf[ iPrev + iLen - 1 ] );
#endif
         iDup = HB_MIN( pPage->pKeyBuf[ iPos ],
                        pPage->pKeyBuf[ iNext - 2] );
         if ( iDup > iNum )
         {
            iDup = iNum;
         }
         else
         {
            while ( iDup < iNum && pPage->pKeyBuf[ iPrev + iDup ] ==
                                   pPage->pKeyBuf[ iNext + iDup ] )
               ++iDup;
         }
#ifdef HB_CDX_DSPDBG_INFO
         printf("+%d=%d", iSpc+iDup, pPage->iFree+iSpc+iDup );
         if ( iSpc+iDup < 0 )
            printf( " iLen=%d, iDup=%d, iNum=%d pd=%d pt=%d cd=%d ct=%d nd=%d nt=%d",
                     iLen-6, iDup, iNum,
                     pPage->pKeyBuf[ iPrev + iLen - 2 ],
                     pPage->pKeyBuf[ iPrev + iLen - 1 ],
                     pPage->pKeyBuf[ ( iKey + 1 ) * iLen - 2 ],
                     pPage->pKeyBuf[ ( iKey + 1 ) * iLen - 1 ],
                     pPage->pKeyBuf[ iNext + iLen - 2 ],
                     pPage->pKeyBuf[ iNext + iLen - 1 ] );
         fflush(stdout);
#endif
      }
      iSpc += ( pPage->pKeyBuf[ iPos ] = ( BYTE ) iDup );
   }
   pPage->iFree += iSpc;
   if ( --pPage->iKeys > iKey )
   {
      memmove( &pPage->pKeyBuf[ iKey * iLen ],
               &pPage->pKeyBuf[ ( iKey + 1 ) * iLen ],
               ( pPage->iKeys - iKey ) * iLen );
   }
   pPage->fBufChanged = pPage->fChanged = TRUE;
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pPage );
   hb_cdxPageCheckDupTrl( pPage, pPage->pKeyBuf, pPage->iKeys, TRUE );
#endif
   if ( iKey >= pPage->iKeys )
      iRet |= NODE_NEWLASTKEY;
   if ( pPage->iKeys == 0 )
      iRet |= NODE_JOIN;
   else if ( pPage->iFree < 0 )
      iRet |= NODE_SPLIT;
   /* if ( pPage->iFree >= CDX_EXT_FREESPACE / 2 ) */
   if ( pPage->iFree >= pPage->ReqByte )
      iRet |= NODE_BALANCE;
   return iRet;
}

/*
 * add key to page at current position
 */
static int hb_cdxPageLeafAddKey( LPCDXPAGE pPage, LPCDXKEY pKey )
{
   int iKey, iNum = pPage->TagParent->uiLen;
   int iLen = iNum + 6, iSpc, iTrl, iDup, iMax, iPos;
   BYTE  bTrail = pPage->TagParent->bTrail;
   int iRet = 0;

#ifdef HB_CDX_DSPDBG_INFO
   printf("\r\naddkey: Page=%lx, iKey=%d/%d, rec=%ld",
          pPage->Page, pPage->iCurKey, pPage->iKeys, pKey->rec);
   fflush(stdout);
#endif
#ifdef HB_CDX_DBGCODE
   if ( ( pPage->PageType & CDX_NODE_LEAF ) == 0 )
      hb_cdxErrInternal( "hb_cdxPageLeafAddKey: page is not a leaf." );
   if ( pPage->iCurKey < 0 || pPage->iCurKey > pPage->iKeys )
      hb_cdxErrInternal( "hb_cdxPageLeafAddKey: wrong iKey index." );
#endif
   if ( !pPage->pKeyBuf )
   {
      BYTE *pKeyBuf = (BYTE *) hb_xgrab( ( pPage->iKeys + 1 ) * iLen );
      hb_cdxPageLeafDecode( pPage, pKeyBuf );
      pPage->pKeyBuf = pKeyBuf;
   }
   else
   {
      pPage->pKeyBuf = (BYTE*) hb_xrealloc( pPage->pKeyBuf, ( pPage->iKeys + 1 ) * iLen );
   }

#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pPage );
   hb_cdxPageCheckDupTrl( pPage, pPage->pKeyBuf, pPage->iKeys, TRUE );
#endif

   iTrl = iDup = 0;
   iKey = pPage->iCurKey;
   iPos = iKey * iLen;
   if ( iKey < pPage->iKeys )
   {
      iDup = pPage->pKeyBuf[ iPos + iNum + 4 ];
      memmove( &pPage->pKeyBuf[ iPos + iLen ], &pPage->pKeyBuf[ iPos ],
               iLen * ( pPage->iKeys - iKey ) );
   }
   if ( pKey->len >= iNum )
      memcpy( &pPage->pKeyBuf[ iPos ], pKey->val, iNum );
   else
   {
      memcpy( &pPage->pKeyBuf[ iPos ], pKey->val, pKey->len );
      memset( &pPage->pKeyBuf[ iPos + pKey->len ], bTrail, iNum - pKey->len );
   }
   HB_PUT_LE_UINT32( &pPage->pKeyBuf[ iPos + iNum ], pKey->rec );
   while ( iTrl < iNum && pPage->pKeyBuf[ iPos + iNum - iTrl - 1 ] == bTrail )
      ++iTrl;
   if ( iKey > 0 )
   {
#ifdef HB_CDX_PACKTRAIL
      iMax = iNum - iTrl;
#else
      iMax = iNum - HB_MAX( iTrl, pPage->pKeyBuf[ iPos - 1 ] );
#endif
      if ( iDup > iMax )
      {
         iDup = iMax;
      }
      else
      {
         while ( iDup < iMax && pPage->pKeyBuf[ iPos + iDup ] ==
                                pPage->pKeyBuf[ iPos + iDup - iLen ] )
            ++iDup;
      }
   }
   pPage->pKeyBuf[ iPos + iNum + 4 ] = (BYTE) iDup;
   pPage->pKeyBuf[ iPos + iNum + 5 ] = (BYTE) iTrl;
   iSpc = pPage->ReqByte + iNum - iTrl - iDup;
   if ( iKey < pPage->iKeys )
   {
#ifdef HB_CDX_PACKTRAIL
      iMax = iNum - pPage->pKeyBuf[ iPos + iLen + iLen - 1 ];
#else
      iMax = iNum - HB_MAX( iTrl, pPage->pKeyBuf[ iPos + iLen + iLen - 1 ] );
#endif
      iSpc += pPage->pKeyBuf[ iPos + iLen + iLen - 2 ];
      iDup = 0;
      while ( iDup < iMax && pPage->pKeyBuf[ iPos + iDup ] ==
                             pPage->pKeyBuf[ iPos + iDup + iLen ] )
         ++iDup;
      iSpc -= ( pPage->pKeyBuf[ iPos + iLen + iLen - 2 ] = ( BYTE ) iDup );
   }
   pPage->iKeys++;
   while ( pKey->rec > pPage->RNMask )
   {
      pPage->RNMask = ( pPage->RNMask << 8 ) | 0xFF;
      pPage->ReqByte++;
      pPage->RNBits += 8;
      iSpc += pPage->iKeys;
   }
   pPage->iFree -= iSpc;
   pPage->fBufChanged = pPage->fChanged = TRUE;
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pPage );
   hb_cdxPageCheckDupTrl( pPage, pPage->pKeyBuf, pPage->iKeys, TRUE );
#endif
   if ( iKey >= pPage->iKeys - 1 )
      iRet |= NODE_NEWLASTKEY;
   if ( pPage->iFree < 0 )
      iRet |= NODE_SPLIT;
   if ( pPage->iFree >= pPage->ReqByte &&
        pPage->Left != CDX_DUMMYNODE && pPage->Right != CDX_DUMMYNODE )
      iRet |= NODE_BALANCE;
   return iRet;
}

/*
 * set (insert) key in interior node record to (with) given value
 */
static void hb_cdxPageIntSetKey( LPCDXPAGE pPage, int iKey, BOOL fIns, BYTE * pbVal, ULONG ulRec, ULONG ulPag )
{
   int iLen = pPage->TagParent->uiLen;
   int iPos = iKey * ( iLen + 8 );

#ifdef HB_CDX_DSPDBG_INFO
   printf("\r\nintSetKey (%s): Page=%lx, iKey=%d/%d, ulPag=%lx",
          fIns ? "ins" : "set", pPage->Page, iKey, pPage->iKeys, ulPag);
   fflush(stdout);
#endif
#ifdef HB_CDX_DBGCODE
   if ( ( pPage->PageType & CDX_NODE_LEAF ) != 0 )
      hb_cdxErrInternal( "hb_cdxPageIntSetKey: page is a leaf!" );
   if ( iKey < 0 || iKey >= pPage->iKeys + ( fIns ? 1 : 0 ) )
   {
      hb_cdxErrInternal( "hb_cdxPageIntSetKey: wrong iKey index." );
   }
#endif
   if ( fIns )
   {
      if ( iKey < pPage->iKeys )
      {
         memmove( &pPage->node.intNode.keyPool[ iPos + iLen + 8 ],
                  &pPage->node.intNode.keyPool[ iPos ],
                  ( iLen + 8 ) * ( pPage->iKeys - iKey ) );
      }
      pPage->iKeys++;
   }
   if ( pbVal )
      memcpy( &pPage->node.intNode.keyPool[ iPos ], pbVal, iLen );
   else if ( fIns )
      memset( &pPage->node.intNode.keyPool[ iPos ],
              pPage->TagParent->bTrail, iLen );
   if ( ulRec )
      HB_PUT_BE_UINT32( &pPage->node.intNode.keyPool[ iPos + iLen ], ulRec );
   HB_PUT_BE_UINT32( &pPage->node.intNode.keyPool[ iPos + iLen + 4 ], ulPag );
   pPage->fChanged = TRUE;
}

/*
 * delete key in interior node record
 */
static void hb_cdxPageIntDelKey( LPCDXPAGE pPage, int iKey )
{
   int iLen = pPage->TagParent->uiLen + 8;

#ifdef HB_CDX_DSPDBG_INFO
   printf("\r\nintDelKey: Page=%lx, iKey=%d/%d, ulPag=%lx",
          pPage->Page, iKey, pPage->iKeys,
          (ULONG) HB_GET_BE_UINT32( &pPage->node.intNode.keyPool[ (iKey+1) * iLen - 4 ] ) );
   fflush(stdout);
#endif
#ifdef HB_CDX_DBGCODE
   if ( ( pPage->PageType & CDX_NODE_LEAF ) != 0 )
      hb_cdxErrInternal( "hb_cdxPageIntDelKey: page is a leaf!" );
   if ( iKey < 0 || iKey >= pPage->iKeys )
   {
      hb_cdxErrInternal( "hb_cdxPageIntDelKey: wrong iKey index." );
   }
#endif
   pPage->iKeys--;
   if ( pPage->iKeys > iKey )
   {
      memmove( &pPage->node.intNode.keyPool[ iKey * iLen ],
               &pPage->node.intNode.keyPool[ ( iKey + 1 ) * iLen ], ( pPage->iKeys - iKey ) * iLen );
   }
   memset( &pPage->node.intNode.keyPool[ pPage->iKeys * iLen ], 0, iLen );
   pPage->fChanged = TRUE;
}

/*
 * (re)load CDX page from index file
 */
static void hb_cdxPageLoad( LPCDXPAGE pPage )
{
   if ( pPage->pKeyBuf )
   {
      hb_xfree( pPage->pKeyBuf );
      pPage->pKeyBuf = NULL;
      pPage->fBufChanged = FALSE;
   }
   hb_cdxIndexPageRead( pPage->TagParent->pIndex, pPage->Page, (BYTE *) &pPage->node, sizeof( CDXNODE ) );
   pPage->PageType = ( BYTE ) HB_GET_LE_UINT16( pPage->node.intNode.attr );
   pPage->Left = HB_GET_LE_UINT32( pPage->node.intNode.leftPtr );
   pPage->Right = HB_GET_LE_UINT32( pPage->node.intNode.rightPtr );
   pPage->iKeys = HB_GET_LE_UINT16( pPage->node.intNode.nKeys );
   pPage->fChanged  = FALSE;

   if ( ( pPage->PageType & CDX_NODE_LEAF ) != 0 )
   {
      pPage->iFree      = HB_GET_LE_UINT16( pPage->node.extNode.freeSpc );
      pPage->RNMask     = HB_GET_LE_UINT32( pPage->node.extNode.recMask );
      /* TODO: redundant, use it directly */
      pPage->DCMask     = pPage->node.extNode.dupMask;
      pPage->TCMask     = pPage->node.extNode.trlMask;
      pPage->RNBits     = pPage->node.extNode.recBits;
      pPage->DCBits     = pPage->node.extNode.dupBits;
      pPage->TCBits     = pPage->node.extNode.trlBits;
      pPage->ReqByte    = pPage->node.extNode.keyBytes;
      pPage->bufKeyNum  = 0;
#if 0
      if ( !pPage->pKeyBuf )
      {
         BYTE *pKeyBuf = (BYTE *) hb_xgrab( ( pPage->iKeys + 1 ) * ( pPage->TagParent->uiLen + 6 ) );
         hb_cdxPageLeafDecode( pPage, pKeyBuf );
         pPage->pKeyBuf = pKeyBuf;
      }
#endif
   }
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pPage );
#endif
}

/*
 * store page into index file
 */
static void hb_cdxPageStore( LPCDXPAGE pPage )
{
#ifdef HB_CDX_DBGCODE
   if ( pPage->Page == 0 || pPage->Page == CDX_DUMMYNODE )
      hb_cdxErrInternal( "hb_cdxPageStore: Page number wrong!." );
   if ( pPage->PageType & CDX_NODE_LEAF )
   {
      if ( pPage->iFree < 0 )
         hb_cdxErrInternal( "hb_cdxPageStore: FreeSpace calculated wrong!." );
   }
   else if ( pPage->iKeys > pPage->TagParent->MaxKeys )
      hb_cdxErrInternal( "hb_cdxPageStore: number of keys exceed!." );
#endif
   HB_PUT_LE_UINT16( pPage->node.intNode.attr, ( UINT16 ) pPage->PageType );
   HB_PUT_LE_UINT16( pPage->node.intNode.nKeys, pPage->iKeys );
   HB_PUT_LE_UINT32( pPage->node.intNode.leftPtr, pPage->Left );
   HB_PUT_LE_UINT32( pPage->node.intNode.rightPtr, pPage->Right );

   if ( ( pPage->PageType & CDX_NODE_LEAF ) != 0 )
   {
      HB_PUT_LE_UINT16( pPage->node.extNode.freeSpc, pPage->iFree );
      HB_PUT_LE_UINT32( pPage->node.extNode.recMask, pPage->RNMask );
      /* TODO: redundant, use it directly */
      pPage->node.extNode.dupMask  = pPage->DCMask;
      pPage->node.extNode.trlMask  = pPage->TCMask;
      pPage->node.extNode.recBits  = pPage->RNBits;
      pPage->node.extNode.dupBits  = pPage->DCBits;
      pPage->node.extNode.trlBits  = pPage->TCBits;
      pPage->node.extNode.keyBytes = pPage->ReqByte;

      if ( pPage->pKeyBuf && pPage->fBufChanged )
      {
         hb_cdxPageLeafEncode( pPage, pPage->pKeyBuf, pPage->iKeys );
         pPage->fBufChanged = FALSE;
      }
#ifdef HB_CDX_DBGCODE_EXT
      if ( pPage->pKeyBuf )
      {
         hb_xfree( pPage->pKeyBuf );
         pPage->pKeyBuf = NULL;
      }
#endif
   }
   hb_cdxIndexPageWrite( pPage->TagParent->pIndex, pPage->Page, (BYTE *) &pPage->node, sizeof( CDXNODE ) );
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pPage );
#endif
   pPage->fChanged = FALSE;
}

/*
 * create new empty page and allocate space for it in index file if ulPage == 0
 * or load it from index file if ulPage != CDX_DUMMYNODE
 */
static LPCDXPAGE hb_cdxPageNew( LPCDXTAG pTag, LPCDXPAGE pOwnerPage, ULONG ulPage )
{
   LPCDXPAGE pPage = NULL;

#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxTagPoolCheck( pTag );
#endif
   if ( ulPage && ulPage != CDX_DUMMYNODE && pTag->pagePool )
   {
      pPage = pTag->pagePool;
      while ( pPage && pPage->Page != ulPage )
      {
         pPage = pPage->pPoolNext;
      }
   }
   if ( pPage )
   {
      if ( pPage->pPoolPrev )
      {
         pPage->pPoolPrev->pPoolNext = pPage->pPoolNext;
         if ( pPage->pPoolNext )
         {
            pPage->pPoolNext->pPoolPrev = pPage->pPoolPrev;
         }
         pPage->pPoolPrev = NULL;
         pPage->pPoolNext = pTag->pagePool;
         pPage->pPoolNext->pPoolPrev = pPage;
         pTag->pagePool = pPage;
      }
   }
   else
   {
      pPage = ( LPCDXPAGE ) hb_xgrab( sizeof( CDXPAGE ) );
      memset( pPage, 0, sizeof( CDXPAGE ) );
      pPage->PageType = CDX_NODE_UNUSED;
      pPage->Left = pPage->Right = CDX_DUMMYNODE;
      pPage->TagParent = pTag;

      if ( ulPage && ulPage != CDX_DUMMYNODE )
      {
         pPage->Page = ulPage;
         hb_cdxPageLoad( pPage );
      }
      else if ( ! ulPage  )
      {
         pPage->Page = hb_cdxIndexGetAvailPage( pTag->pIndex, FALSE );
         pPage->fChanged = TRUE;
      }
      pPage->pPoolPrev = NULL;
      pPage->pPoolNext = pTag->pagePool;
      pTag->pagePool   = pPage;
      if ( pPage->pPoolNext )
         pPage->pPoolNext->pPoolPrev = pPage;
   }
   pPage->Owner = pOwnerPage;
   pPage->iCurKey = -1;
   pPage->bUsed = 1;
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxTagPoolCheck( pTag );
#endif
   return pPage;
}

/*
 * free single page
 */
static void hb_cdxPageFree( LPCDXPAGE pPage, BOOL fReal )
{
#ifdef HB_CDX_DBGCODE_EXT
   LPCDXTAG pTag = pPage->TagParent;
   hb_cdxTagPoolCheck( pTag );
#endif
   if ( pPage->Child != NULL )
   {
      hb_cdxPageFree( pPage->Child, fReal );
      pPage->Child = NULL;
   }

   if ( pPage->PageType == CDX_NODE_UNUSED )
   {
      fReal = TRUE;
      pPage->fChanged = FALSE;
   }

   if ( fReal )
   {
      if ( pPage->fChanged )
         hb_cdxPageStore( pPage );

#ifdef HB_CDX_DBGCODE_EXT
      hb_cdxTagPoolCheck( pTag );
#endif
      if ( pPage->pPoolPrev )
      {
         pPage->pPoolPrev->pPoolNext = pPage->pPoolNext;
         if ( pPage->pPoolNext )
         {
            pPage->pPoolNext->pPoolPrev = pPage->pPoolPrev;
         }
      }
      else
      {
         pPage->TagParent->pagePool = pPage->pPoolNext;
         if ( pPage->pPoolNext )
         {
            pPage->pPoolNext->pPoolPrev = NULL;
         }
      }
#ifdef HB_CDX_DBGCODE_EXT
      hb_cdxTagPoolCheck( pTag );
#endif
   }

   if ( pPage->Owner != NULL && pPage->Owner->Child == pPage )
      pPage->Owner->Child = NULL;
   pPage->Owner = NULL;
   pPage->bUsed = 0;

   if ( fReal )
   {
      if ( pPage->PageType == CDX_NODE_UNUSED )
         hb_cdxIndexPutAvailPage( pPage->TagParent->pIndex, pPage->Page, FALSE );
      if ( pPage->pKeyBuf )
         hb_xfree( pPage->pKeyBuf );
      hb_xfree( pPage );
   }
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxTagPoolCheck( pTag );
#endif
}

/*
 * read child page
 */
static void hb_cdxPageGetChild( LPCDXPAGE pPage )
{
   ULONG ulPage;

#ifdef HB_CDX_DBGCODE
   if ( ( pPage->PageType & CDX_NODE_LEAF ) != 0 )
      hb_cdxErrInternal( "hb_cdxPageGetChild: index corrupted." );
#endif

   ulPage = hb_cdxPageGetKeyPage( pPage, pPage->iCurKey );
   if ( pPage->Child != NULL )
   {
      if ( pPage->Child->Page != ulPage )
      {
         hb_cdxPageFree( pPage->Child, FALSE );
         pPage->Child = NULL;
      }
   }
#ifdef HB_CDX_DSPDBG_INFO
   printf("GetChild: Parent=%lx, Child=%lx\r\n", pPage->Page, ulPage); fflush(stdout);
#endif
   if ( pPage->Child == NULL )
      pPage->Child = hb_cdxPageNew( pPage->TagParent, pPage, ulPage );
}

static int hb_cdxPageKeyLeafBalance( LPCDXPAGE pPage, int iChildRet )
{
   LPCDXPAGE childs[ CDX_BALANCE_LEAFPAGES + 2 ], lpTmpPage;
   int iChKeys[ CDX_BALANCE_LEAFPAGES + 2 ],
       iChFree[ CDX_BALANCE_LEAFPAGES + 2 ];
   int iFirstKey, iBlncKeys = CDX_BALANCE_LEAFPAGES;
   int iLen = pPage->TagParent->uiLen + 6,
       iKeys = 0, iFree = 0, iSkip = 0, iBufSize = 0;
   BYTE * pKeyPool = NULL, * pPtr;
   BOOL fIns;
   ULONG ulPage;
   int iRet = 0, i;

#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pPage );
#endif

   if ( pPage->iCurKey > 0 )
      iFirstKey = pPage->iCurKey - 1;
   else
   {
      iFirstKey = 0;
      --iBlncKeys;
      if ( pPage->Left != CDX_DUMMYNODE )
         iRet |= NODE_BALANCE;
   }
   if ( iBlncKeys > pPage->iKeys - iFirstKey )
   {
      iBlncKeys = pPage->iKeys - iFirstKey;
      if ( pPage->Right != CDX_DUMMYNODE )
         iRet |= NODE_BALANCE;
   }

#ifdef HB_CDX_DSPDBG_INFO
   printf("\r\nleaf balance: Page=%lx (%d/%d)", pPage->Page, iFirstKey, iBlncKeys);
   fflush(stdout);
#endif

   if ( ( iChildRet & ( NODE_SPLIT | NODE_JOIN ) ) == 0 &&
        ( iBlncKeys < 2 || ( iChildRet & NODE_BALANCE ) == 0 ) )
      return iRet;

   for ( i = 0; i < iBlncKeys; i++ )
   {
      ulPage = hb_cdxPageGetKeyPage( pPage, iFirstKey + i );
      if ( pPage->Child && pPage->Child->Page == ulPage )
      {
         childs[i] = pPage->Child;
         pPage->Child = NULL;
      }
      else
      {
         childs[i] = hb_cdxPageNew( pPage->TagParent, pPage, ulPage );
      }
#ifdef HB_CDX_DBGCODE
      if ( i > 0 && ( childs[i]->Page != childs[i-1]->Right ||
                      childs[i]->Left != childs[i-1]->Page ) )
      {
         printf("\r\nchilds[%d]->Page=%lx, childs[%d]->Right=%lx, childs[%d]->Page=%lx, childs[%d]->Left=%lx",
                i-1, childs[i-1]->Page, i-1, childs[i-1]->Right,
                i, childs[i]->Page, i, childs[i]->Left);
         fflush(stdout);
         hb_cdxErrInternal( "hb_cdxPageKeyLeafBalance: index corrupted." );
      }
#endif
      iChKeys[i] = childs[i]->iKeys;
      iChFree[i] = childs[i]->iFree;
      if ( childs[i]->iFree >= childs[i]->ReqByte ) /* TODO: increase limit for last page */
         iFree += childs[i]->iFree;
      else if ( childs[i]->iFree >= 0 )
      {
         if ( i == iSkip )
            ++iSkip;
#if 1
         else if ( i + 1 == iBlncKeys && ( iChildRet & NODE_SPLIT ) == 0 )
         {
            iBlncKeys--;
            hb_cdxPageFree( childs[i], FALSE );
         }
#endif
      }
      if ( i >= iSkip && i < iBlncKeys )
         iKeys += childs[i]->iKeys;

#ifdef HB_CDX_DSPDBG_INFO
      printf(", childs[%d]->Page=%lx(%d/%d)", i, childs[i]->Page, childs[i]->iKeys, childs[i]->iFree);
      printf("(%d/%d/%d:%d,%lx)", i, iSkip, iBlncKeys, iKeys, childs[i]->Right);
      fflush(stdout);
#endif
   }
   if ( ( iChildRet & NODE_SPLIT ) == 0 )
   {
      for ( i = iBlncKeys - 1; i > iSkip && childs[i]->iFree >= 0 && childs[i]->iFree < childs[i]->ReqByte; i-- )
      {
         iKeys -= childs[i]->iKeys;
         hb_cdxPageFree( childs[i], FALSE );
         iBlncKeys--;
      }
   }
   if ( ( iChildRet & ( NODE_SPLIT | NODE_JOIN ) ) == 0 &&
        ( iBlncKeys < 2 || iFree < CDX_EXT_FREESPACE ) )
   {
      for ( i = 0; i < iBlncKeys; i++ )
         hb_cdxPageFree( childs[i], FALSE );
      return iRet;
   }
#ifdef HB_CDX_DSPDBG_INFO
   printf("\r\nleaf balance: Page=%lx iKeys=%d", pPage->Page, iKeys);
   fflush(stdout);
#endif
   if ( iKeys > 0 )
   {
      iBufSize = iKeys;
      pPtr = pKeyPool = (BYTE *) hb_xgrab( iBufSize * iLen );
      for ( i = iSkip; i < iBlncKeys && iKeys > 0; i++ )
      {
         if ( childs[i]->iKeys > 0 )
         {
            if ( childs[i]->pKeyBuf )
               memcpy( pPtr, childs[i]->pKeyBuf, childs[i]->iKeys * iLen );
            else
               hb_cdxPageLeafDecode( childs[i], pPtr );
            /* update number of duplicate characters when join pages */
            if ( pPtr > pKeyPool )
            {
               BYTE bDup = 0, bMax;
#ifdef HB_CDX_PACKTRAIL
               bMax = iLen - 6 - pPtr[ iLen - 1 ];
#else
               bMax = iLen - 6 - HB_MAX( pPtr[ iLen - 1 ], pPtr[ -1 ] );
#endif
               while ( bDup < bMax && pPtr[ bDup ] == pPtr[ bDup - iLen ] )
                  ++bDup;
               pPtr[ iLen - 2 ] = bDup;
               if ( iSkip == i - 1 && childs[iSkip]->iFree >= 0 &&
                    iLen - 6 - bDup - pPtr[ iLen - 1 ] >
                               childs[iSkip]->iFree - childs[iSkip]->ReqByte )
               {
                  memmove( pKeyPool, pPtr, childs[i]->iKeys * iLen );
                  pPtr = pKeyPool;
                  iKeys -= childs[i-1]->iKeys;
                  iSkip++;
#ifdef HB_CDX_DSPDBG_INFO
                  printf("\r\niSkip=%d, iBlncKeys=%d", iSkip, iBlncKeys);
                  fflush(stdout);
#endif
               }
            }
            pPtr += childs[i]->iKeys * iLen;
#ifdef HB_CDX_DSPDBG_INFO
            printf(", childs[%d]->iKeys=%d", i, childs[i]->iKeys);
            fflush(stdout);
#endif
         }
      }
   }

#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckDupTrl( pPage, pKeyPool, iKeys, FALSE );
#endif
   pPtr = pKeyPool;
   fIns = FALSE;
   i = iSkip;
   while ( iKeys > 0 )
   {
      if ( i == iBlncKeys )
      {
         if ( childs[i-1]->Right != CDX_DUMMYNODE )
            lpTmpPage = hb_cdxPageNew( pPage->TagParent, pPage, childs[i-1]->Right );
         else
            lpTmpPage = NULL;

#if 1
         if ( !fIns && lpTmpPage != NULL )
         {
            int j, iSize = 0, iMaxReq;
            ULONG ulMaxRec = 0, ul;
            BYTE * pbKey, bMax;

            for ( j = 0; j < iKeys; j++ )
            {
               if ( ulMaxRec < ( ul = HB_GET_LE_UINT32( &pPtr[ ( j + 1 ) * iLen - 6 ] ) ) )
                  ulMaxRec = ul;
               iSize += iLen - 6 - ( j == 0 ? 0 : pPtr[ ( j + 1 ) * iLen - 2 ] ) - pPtr[ ( j + 1 ) * iLen - 1 ];
            }
            pbKey = hb_cdxPageGetKeyVal( lpTmpPage, 0 );
            bMax = ( HB_GET_LE_UINT16( &lpTmpPage->node.extNode.keyPool[ lpTmpPage->ReqByte - 2 ] )
                         >> ( 16 - lpTmpPage->TCBits ) ) & lpTmpPage->TCMask;
#ifdef HB_CDX_PACKTRAIL
            bMax = iLen - 6 - bMax;
#else
            bMax = iLen - 6 - HB_MAX( pPtr[ iKeys * iLen - 1 ], bMax );
#endif
            for ( j = 0; j < bMax &&
                         pPtr[ ( iKeys - 1 ) * iLen + j ] == pbKey[ j ]; j++ ) {}
#ifdef HB_CDX_DSPDBG_INFO
            printf("\r\nbDup=%d, bTrl=%d ", j, iLen - 6 - bMax ); fflush(stdout);
#endif
            iSize -= j;
            iMaxReq = lpTmpPage->ReqByte;
            ul = lpTmpPage->RNMask;
            while ( ulMaxRec > ul )
            {
               ++iMaxReq;
               ul = ( ul << 8 ) | 0xFF;
            }
            iSize += iKeys * iMaxReq;
            iSize = lpTmpPage->iFree - iSize -
                     ( iMaxReq - lpTmpPage->ReqByte ) * lpTmpPage->iKeys;
            if ( iSize < 0 )
               fIns = TRUE;
            else
            {
#ifdef HB_CDX_DSPDBG_INFO
               printf("\r\ninserting bDup=%d #keys=%d/%d (%d) parent=%lx, child=%lx (%d), rec=%ld",
                      j, iKeys, lpTmpPage->iKeys, i, pPage->Page, lpTmpPage->Page, iSize, (ULONG) HB_GET_LE_UINT32( pPtr + iLen - 6 ));
               fflush(stdout);
#endif
               if ( iBufSize >= iKeys + lpTmpPage->iKeys )
               {
                  memmove( pKeyPool, pPtr, iKeys * iLen );
               }
               else
               {
                  BYTE * pTmp;
                  iBufSize = iKeys + lpTmpPage->iKeys;
                  pTmp = (BYTE *) hb_xgrab( iBufSize * iLen );
                  memcpy( pTmp, pPtr, iKeys * iLen );
                  hb_xfree( pKeyPool );
                  pKeyPool = pTmp;
               }
               if ( lpTmpPage->iKeys > 0 )
               {
                  BYTE bDup = 0, bMax;
                  pPtr = &pKeyPool[ iKeys * iLen ];
                  if ( lpTmpPage->pKeyBuf )
                     memcpy( pPtr, lpTmpPage->pKeyBuf, lpTmpPage->iKeys * iLen );
                  else
                     hb_cdxPageLeafDecode( lpTmpPage, pPtr );
#ifdef HB_CDX_PACKTRAIL
                  bMax = iLen - 6 - pPtr[ iLen - 1 ];
#else
                  bMax = iLen - 6 - HB_MAX( pPtr[ iLen - 1 ], pPtr[ -1 ] );
#endif
                  while ( bDup < bMax && pPtr[ bDup ] == pPtr[ bDup - iLen ] )
                     ++bDup;
                  pPtr[ iLen - 2 ] = bDup;
                  iKeys += lpTmpPage->iKeys;
#ifdef HB_CDX_DSPDBG_INFO
                  printf(" bDup2=%d, bTrl2=%d ", bDup, pPtr[ iLen - 1 ] ); fflush(stdout);
#endif
               }
               pPtr = pKeyPool;
               childs[i] = lpTmpPage;
               if ( iFirstKey + i >= pPage->iKeys )
                  iRet |= NODE_NEWLASTKEY;
#ifdef HB_CDX_DBGCODE_EXT
               childs[i]->iKeys = 0;
               if ( childs[i]->pKeyBuf )
               {
                  hb_xfree( childs[i]->pKeyBuf );
                  childs[i]->pKeyBuf = NULL;
                  childs[i]->fBufChanged = FALSE;
               }
               hb_cdxPageCalcLeafSpace( childs[i], pPtr, iKeys );
               hb_cdxPageLeafEncode( childs[i], pPtr, childs[i]->iKeys );
               iSize += ( iMaxReq - childs[i]->ReqByte ) * childs[i]->iKeys;
               if ( iSize != childs[i]->iFree )
               {
                  printf("\r\ninserting, iSize=%d, childs[i]->iFree=%d", iSize, childs[i]->iFree); fflush(stdout);
                  printf("\r\niKeys=%d, iMaxReq=%d", iKeys, iMaxReq); fflush(stdout);
                  hb_cdxErrInternal( "hb_cdxPageGetChild: index corrupted." );
               }
#endif
            }
#endif
         }
         else
            fIns = TRUE;

         if ( fIns )
         {
            childs[ i ] = hb_cdxPageNew( pPage->TagParent, pPage, 0 );
            childs[ i ]->PageType = CDX_NODE_LEAF;
            /* Update siblings links */
            childs[ i ]->Left  = childs[i-1]->Page;
            childs[ i ]->Right = childs[i-1]->Right;
            childs[i-1]->Right = childs[ i ]->Page;
            childs[i-1]->fChanged = TRUE;
            if ( lpTmpPage != NULL )
            {
               lpTmpPage->Left = childs[i]->Page;
               lpTmpPage->fChanged = TRUE;
               hb_cdxPageFree( lpTmpPage, FALSE );
            }
            iBlncKeys++;
            iRet |= NODE_BALANCE;
#ifdef HB_CDX_DSPDBG_INFO
            printf("\r\nleaf balance: new child[%d]->Page=%lx",i,childs[i]->Page);
            fflush(stdout);
#endif
         }
      }
      childs[i]->iKeys = 0;
      if ( childs[i]->pKeyBuf )
      {
         hb_xfree( childs[i]->pKeyBuf );
         childs[i]->pKeyBuf = NULL;
         childs[i]->fBufChanged = FALSE;
      }
      hb_cdxPageCalcLeafSpace( childs[i], pPtr, iKeys );
      if ( i == iSkip && i < iBlncKeys && !childs[i]->fChanged &&
           childs[i]->iKeys == iChKeys[i] &&
           childs[i]->iFree == iChFree[i] )
      {
#ifdef HB_CDX_DSPDBG_INFO
         printf("\r\niskip++\r\n");fflush(stdout);
#endif
         iSkip++;
      }
      else
      {
         hb_cdxPageLeafEncode( childs[i], pPtr, childs[i]->iKeys );
      }
      pPtr += childs[i]->iKeys * iLen;
      iKeys -= childs[i]->iKeys;
      /* update parent key */
      if ( i < iBlncKeys )
         hb_cdxPageIntSetKey( pPage, iFirstKey + i, fIns,
                              pPtr - iLen, HB_GET_LE_UINT32( pPtr - 6 ),
                              childs[i]->Page );
      else
         iBlncKeys++;
#ifdef HB_CDX_DSPDBG_INFO
      printf(" (%d/%d)", childs[i]->iKeys,childs[i]->iFree);
      fflush(stdout);
#endif
#ifdef HB_CDX_DBGCODE_EXT
      hb_cdxPageCheckKeys( childs[i] );
#endif
      i++;
   }
   if ( i < iBlncKeys )
   {
      /* Update siblings links */
#if 1
      if ( childs[iBlncKeys-1]->Right != CDX_DUMMYNODE &&
           ( i > 1 || ( i == 1 && childs[0]->Left == CDX_DUMMYNODE ) ) )
      {
         ULONG Page;
         Page = childs[iBlncKeys-1]->Page;
         childs[iBlncKeys-1]->Page = childs[i-1]->Page;
         childs[i-1]->Page = Page;
         hb_cdxPageIntSetKey( pPage, iFirstKey + i - 1, FALSE, NULL, 0, Page );
         childs[i-1]->Right = childs[iBlncKeys-1]->Right;
         childs[i-1]->fChanged = TRUE;
         if ( i > 1 )
         {
            childs[i-2]->Right = Page;
            childs[i-2]->fChanged = TRUE;
         }
      }
      else
#endif
      {
         ULONG Left, Right;
         Right = childs[iBlncKeys-1]->Right;
         if ( i > 0 )
         {
            Left = childs[i-1]->Page;
            childs[i-1]->Right = Right;
            childs[i-1]->fChanged  = TRUE;
         }
         else
         {
            Left = childs[0]->Left;
            if ( Left != CDX_DUMMYNODE )
            {
               lpTmpPage = hb_cdxPageNew( pPage->TagParent, pPage, Left );
               lpTmpPage->Right = Right;
               lpTmpPage->fChanged  = TRUE;
               hb_cdxPageFree( lpTmpPage, FALSE );
            }
         }
         if ( Right != CDX_DUMMYNODE )
         {
            lpTmpPage = hb_cdxPageNew( pPage->TagParent, pPage, Right );
            lpTmpPage->Left = Left;
            lpTmpPage->fChanged  = TRUE;
            hb_cdxPageFree( lpTmpPage, FALSE );
         }
      }
      /* Unlink empty pages from parent */
      while ( i < iBlncKeys )
      {
         /* Delete parent key */
         iBlncKeys--;
#ifdef HB_CDX_DSPDBG_INFO
         printf("\r\nleaf balance: free child[%d]->Page=%lx", iBlncKeys, childs[iBlncKeys]->Page);
         fflush(stdout);
#endif
         if ( childs[iBlncKeys]->pKeyBuf )
         {
            hb_xfree( childs[iBlncKeys ]->pKeyBuf );
            childs[iBlncKeys]->pKeyBuf = NULL;
            childs[iBlncKeys]->fBufChanged = FALSE;
         }
         hb_cdxPageIntDelKey( pPage, iFirstKey + iBlncKeys );
         childs[iBlncKeys]->Owner    = NULL;
         childs[iBlncKeys]->fChanged = FALSE;
         childs[iBlncKeys]->PageType = CDX_NODE_UNUSED;
         childs[iBlncKeys]->Left     = CDX_DUMMYNODE;
         childs[iBlncKeys]->Right    = CDX_DUMMYNODE;
         hb_cdxPageFree( childs[iBlncKeys], FALSE );
      }
      iRet |= NODE_BALANCE;
   }
   for ( i = 0; i < iBlncKeys; i++ )
      hb_cdxPageFree( childs[i], FALSE );

   if ( pKeyPool )
      hb_xfree( pKeyPool );
   pPage->fChanged = TRUE;
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pPage );
#endif
   if ( pPage->iKeys > pPage->TagParent->MaxKeys )
      iRet |= NODE_SPLIT;
   return iRet;
}

static int hb_cdxPageKeyIntBalance( LPCDXPAGE pPage, int iChildRet )
{
   LPCDXPAGE childs[ CDX_BALANCE_INTPAGES + 2 ], lpTmpPage;
   int iFirstKey, iBlncKeys = CDX_BALANCE_INTPAGES;
   int iLen = pPage->TagParent->uiLen + 8, iKeys = 0, iNeedKeys, iNodeKeys,
       iMin = pPage->TagParent->MaxKeys, iMax = 0, iDiv;
   ULONG ulPage;
   BYTE * pKeyPool = NULL, *pPtr;
   BOOL fForce = ( iChildRet & ( NODE_SPLIT | NODE_JOIN ) ) != 0;
   int iRet = 0, i;

   if ( !fForce && ( iChildRet & NODE_BALANCE ) == 0 )
      return iRet;

   if ( pPage->Child && pPage->Child->Child )
      hb_cdxPageFree( pPage->Child->Child, FALSE );

#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pPage );
#endif

   if ( pPage->iKeys <= iBlncKeys || pPage->iCurKey <= iBlncKeys / 2 )
      iFirstKey = 0;
   else if ( pPage->iCurKey + ( iBlncKeys >> 1 ) >= pPage->iKeys )
      iFirstKey = pPage->iKeys - iBlncKeys;
   else
      iFirstKey = pPage->iCurKey - ( iBlncKeys >> 1 );
   if ( iBlncKeys > pPage->iKeys - iFirstKey )
   {
      iBlncKeys = pPage->iKeys - iFirstKey;
      iRet |= NODE_BALANCE;
   }

#ifdef HB_CDX_DSPDBG_INFO
   printf("\r\nbalance: Page=%lx(%d) (%d/%d)", pPage->Page, pPage->iKeys, iFirstKey, iBlncKeys);
   fflush(stdout);
#endif

   if ( !fForce && iBlncKeys < 2 )
      return iRet;

   for ( i = 0; i < iBlncKeys; i++ )
   {
      ulPage = hb_cdxPageGetKeyPage( pPage, iFirstKey + i );
      if ( pPage->Child && pPage->Child->Page == ulPage )
      {
         childs[i] = pPage->Child;
         pPage->Child = NULL;
      }
      else
      {
         childs[i] = hb_cdxPageNew( pPage->TagParent, pPage, ulPage );
      }
#ifdef HB_CDX_DBGCODE
      if ( i > 0 && ( childs[i]->Page != childs[i-1]->Right ||
                      childs[i]->Left != childs[i-1]->Page ) )
      {
         printf("\r\nchilds[%d]->Page=%lx, childs[%d]->Right=%lx, childs[%d]->Page=%lx, childs[%d]->Left=%lx",
                i-1, childs[i-1]->Page, i-1, childs[i-1]->Right,
                i, childs[i]->Page, i, childs[i]->Left);
         fflush(stdout);
         hb_cdxErrInternal( "hb_cdxPageKeyIntBalance: index corrupted." );
      }
#endif
      iKeys += childs[i]->iKeys;

      if ( childs[i]->iKeys > iMax )
         iMax = childs[i]->iKeys;
      if ( childs[i]->iKeys < iMin )
         iMin = childs[i]->iKeys;
#ifdef HB_CDX_DSPDBG_INFO
      printf(", childs[%d]->Page=%lx(%d)", i, childs[i]->Page, childs[i]->iKeys);
      fflush(stdout);
#endif
   }
   iNeedKeys = ( iKeys + pPage->TagParent->MaxKeys - 1 )
                       / pPage->TagParent->MaxKeys;
#if 1
   if ( iNeedKeys == 1 && iBlncKeys > 1 && childs[0]->Left != CDX_DUMMYNODE &&
        childs[iBlncKeys-1]->Right != CDX_DUMMYNODE &&
        iKeys >= ( CDX_BALANCE_INTPAGES << 1 ) &&
        iKeys > ( ( pPage->TagParent->MaxKeys * 3 ) >> 1 ) )
   {
      iNeedKeys = 2;
   }
#endif
#if 1
   iDiv = HB_MAX( iMax - iMin - ( pPage->TagParent->MaxKeys >> 1 ) + 1,
                  iBlncKeys - iNeedKeys );
#else
   iDiv = iMax - iMin;
#endif
   if ( iKeys > 0 && ( iDiv >= 2 || fForce ) )
   {
#if 1
      if ( iBlncKeys == 1 && iKeys > pPage->TagParent->MaxKeys &&
           childs[0]->Right != CDX_DUMMYNODE )
      {
         lpTmpPage = hb_cdxPageNew( pPage->TagParent, pPage, childs[0]->Right );
         iKeys += lpTmpPage->iKeys;
         childs[iBlncKeys++] = lpTmpPage;
         if ( iFirstKey + iBlncKeys > pPage->iKeys )
            iRet |= NODE_NEWLASTKEY;
         iNeedKeys = ( iKeys + pPage->TagParent->MaxKeys - 1 )
                             / pPage->TagParent->MaxKeys;
      }
      else
#endif
      {
         iMin = HB_MAX( iKeys / iNeedKeys, 2 );
         iMax = HB_MAX( ( iKeys + iNeedKeys - 1 ) / iNeedKeys, iMin );
         for ( i = iBlncKeys - 1; i > 1 &&
                  childs[i]->iKeys >= iMin && childs[i]->iKeys <= iMax; i-- )
         {
            iKeys -= childs[i]->iKeys;
            hb_cdxPageFree( childs[i], FALSE );
            iBlncKeys--;
            iMin = HB_MAX( iKeys / iNeedKeys, 2 );
            iMax = HB_MAX( ( iKeys + iNeedKeys - 1 ) / iNeedKeys, iMin );
         }
         while ( iBlncKeys > 2 && childs[0]->iKeys >= iMin && childs[0]->iKeys <= iMax )
         {
            iKeys -= childs[0]->iKeys;
            hb_cdxPageFree( childs[0], FALSE );
            iBlncKeys--;
            iFirstKey++;
            for ( i = 0; i < iBlncKeys; i++ )
            {
               childs[i] = childs[i+1];
            }
            iMin = HB_MAX( iKeys / iNeedKeys, 2 );
            iMax = HB_MAX( ( iKeys + iNeedKeys - 1 ) / iNeedKeys, iMin );
         }
      }
   }
   if ( !fForce && ( iBlncKeys < 2 || iDiv < 2 ) )
   {
      for ( i = 0; i < iBlncKeys; i++ )
         hb_cdxPageFree( childs[i], FALSE );
      return iRet;
   }

   if ( iKeys > 0 )
   {
      pPtr = pKeyPool = (BYTE*) hb_xgrab( iKeys * iLen );
      for ( i = 0; i < iBlncKeys; i++ )
      {
         if ( childs[i]->iKeys > 0 )
         {
            memcpy( pPtr, childs[i]->node.intNode.keyPool, childs[i]->iKeys * iLen );
            pPtr += childs[i]->iKeys * iLen;
         }
      }
   }

   if ( iNeedKeys > iBlncKeys )
   {
      if ( iBlncKeys < 2 )
         i = iBlncKeys;
      else
      {
         i = iBlncKeys - 1;
         childs[iBlncKeys] = childs[i];
      }
      childs[ i ] = hb_cdxPageNew( pPage->TagParent, pPage, 0 );
      childs[ i ]->PageType = CDX_NODE_BRANCH;
      childs[ i ]->iKeys    = 0;
      childs[ i ]->fChanged = TRUE;
      /* Add new parent key */
      hb_cdxPageIntSetKey( pPage, iFirstKey + i, TRUE,
                           NULL, 0, childs[iBlncKeys]->Page );
      /* Update siblings links */
      childs[ i ]->Left  = childs[i-1]->Page;
      childs[ i ]->Right = childs[i-1]->Right;
      childs[i-1]->Right = childs[ i ]->Page;
      if ( i < iBlncKeys )
         childs[i+1]->Left = childs[i]->Page;
      else if ( childs[i]->Right != CDX_DUMMYNODE )
      {
         lpTmpPage = hb_cdxPageNew( pPage->TagParent, pPage, childs[iBlncKeys]->Right );
         lpTmpPage->Left = childs[i]->Page;
         lpTmpPage->fChanged  = TRUE;
         hb_cdxPageFree( lpTmpPage, FALSE );
      }
#ifdef HB_CDX_DSPDBG_INFO
      printf("\r\nint balance: new child[%d]->Page=%lx",iBlncKeys,childs[iBlncKeys]->Page);
      fflush(stdout);
#endif
      iBlncKeys++;
      iRet |= NODE_BALANCE;
   }
   else if ( iNeedKeys < iBlncKeys )
   {
      ULONG Left, Right;

      /* Update siblings links */
      if ( iNeedKeys > 1 )
      {
         childs[iNeedKeys-2]->Right = childs[iBlncKeys-1]->Page;
         childs[iBlncKeys-1]->Left = childs[iNeedKeys-2]->Page;
         lpTmpPage = childs[iBlncKeys-1];
         childs[iBlncKeys-1] = childs[iNeedKeys-1];
         childs[iNeedKeys-1] = lpTmpPage;
      }
      else if ( iNeedKeys > 0 && childs[0]->Left == CDX_DUMMYNODE )
      {
         lpTmpPage = childs[iBlncKeys-1];
         childs[iBlncKeys-1] = childs[0];
         childs[0] = lpTmpPage;
         childs[0]->Left = CDX_DUMMYNODE;
      }
      else
      {
         Right = childs[iBlncKeys-1]->Right;
         if ( iNeedKeys > 0 )
         {
            Left = childs[iNeedKeys-1]->Page;
            childs[iNeedKeys-1]->Right = Right;
         }
         else
         {
            Left = childs[0]->Left;
            if ( Left != CDX_DUMMYNODE )
            {
               lpTmpPage = hb_cdxPageNew( pPage->TagParent, pPage, Left );
               lpTmpPage->Right = Right;
               lpTmpPage->fChanged = TRUE;
               hb_cdxPageFree( lpTmpPage, FALSE );
            }
         }
         if ( Right != CDX_DUMMYNODE )
         {
            lpTmpPage = hb_cdxPageNew( pPage->TagParent, pPage, Right );
            lpTmpPage->Left = Left;
            lpTmpPage->fChanged = TRUE;
            hb_cdxPageFree( lpTmpPage, FALSE );
         }
      }
      /* Unlink empty pages from parent */
      for ( i = iBlncKeys - 1; i >= iNeedKeys; i-- )
      {
         /* Delete parent key */
#ifdef HB_CDX_DSPDBG_INFO
         printf("\r\nbalance: free child[%d]->Page=%lx",i,childs[i]->Page);
         fflush(stdout);
#endif
         hb_cdxPageIntDelKey( pPage, iFirstKey + i );
         childs[i]->Owner    = NULL;
         childs[i]->fChanged = FALSE;
         childs[i]->PageType = CDX_NODE_UNUSED;
         childs[i]->Left     = CDX_DUMMYNODE;
         childs[i]->Right    = CDX_DUMMYNODE;
         childs[i]->iKeys    = 0;
         hb_cdxPageFree( childs[i], FALSE );
      }
      iBlncKeys = iNeedKeys;
      iRet |= NODE_BALANCE;
   }

   /*
    * Redistribute childs internal node's keys and update parent keys
    */
   if ( iKeys > 0 )
   {
      pPtr = pKeyPool;
      for ( i = 0; i < iBlncKeys; i++ )
      {
         iNodeKeys = ( iKeys + iBlncKeys - i - 1 ) / ( iBlncKeys - i );
#ifdef HB_CDX_DBGCODE
         if ( iNodeKeys > pPage->TagParent->MaxKeys )
            hb_cdxErrInternal( "hb_cdxPageKeyIntBalance: iNodeKeys calculated wrong!." );
#endif
         /* TODO: do nothing if iNodeKeys == childs[i]->iKeys && i == iSkip */
         memcpy( childs[i]->node.intNode.keyPool, pPtr, iNodeKeys * iLen );
         childs[i]->iKeys = iNodeKeys;
         childs[i]->fChanged = TRUE;
         pPtr += iNodeKeys * iLen;
         iKeys -= iNodeKeys;
         /* update parent key */
         if ( iFirstKey + i < pPage->iKeys )
         {
            hb_cdxPageIntSetKey( pPage, iFirstKey + i, FALSE,
                                 pPtr - iLen, HB_GET_BE_UINT32( pPtr - 8 ),
                                 childs[i]->Page );
         }
#ifdef HB_CDX_DSPDBG_INFO
         printf(" (%d)", childs[i]->iKeys);
#endif
#ifdef HB_CDX_DBGCODE_EXT
         hb_cdxPageCheckKeys( childs[i] );
#endif
         hb_cdxPageFree( childs[i], FALSE );
      }
      hb_xfree( pKeyPool );
   }
   pPage->fChanged = TRUE;
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pPage );
#endif
   if ( pPage->iKeys > pPage->TagParent->MaxKeys )
      iRet |= NODE_SPLIT;
   return iRet;
}

/*
 * balance keys in child pages
 */
static int hb_cdxPageBalance( LPCDXPAGE pPage, int iChildRet )
{
   int iRet = 0;

   if ( ( pPage->PageType & CDX_NODE_LEAF ) != 0 )
      iRet = iChildRet;
   else
   {
      if ( iChildRet & NODE_NEWLASTKEY )
      {
         if ( pPage->Child->iKeys == 0 )
         {
            iChildRet |= NODE_JOIN;
            iRet |= NODE_NEWLASTKEY;
         }
         else
         {
            hb_cdxPageIntSetKey( pPage, pPage->iCurKey, FALSE,
                                 hb_cdxPageGetKeyVal( pPage->Child, pPage->Child->iKeys-1 ),
                                 hb_cdxPageGetKeyRec( pPage->Child, pPage->Child->iKeys-1 ),
                                 pPage->Child->Page );
#ifdef HB_CDX_DBGCODE_EXT
            hb_cdxPageCheckKeys( pPage );
#endif
            pPage->fChanged = TRUE;
            if ( pPage->iCurKey >= pPage->iKeys - 1 )
               iRet |= NODE_NEWLASTKEY;
         }
      }
      if ( ( pPage->Child->PageType & CDX_NODE_LEAF ) != 0 )
         iRet |= hb_cdxPageKeyLeafBalance( pPage, iChildRet );
      else
         iRet |= hb_cdxPageKeyIntBalance( pPage, iChildRet );
   }
   if ( !pPage->Owner )
   {
      if ( pPage->iKeys == 0 )
      {
         pPage->PageType |= CDX_NODE_LEAF;
         hb_cdxPageLeafInitSpace( pPage );
      }
      else if ( iRet & NODE_SPLIT )
         iRet = hb_cdxPageRootSplit( pPage );
   }
   return iRet;
}

/*
 * split Root Page
 */
static int hb_cdxPageRootSplit( LPCDXPAGE pPage )
{
   LPCDXPAGE pNewRoot;
   ULONG ulPage;

   pNewRoot = hb_cdxPageNew( pPage->TagParent, NULL, 0 );
   /*
    * do not change root page address if it's unnecessary
    * so we don't have to update Tag header
    */
   pPage->TagParent->RootPage = pNewRoot;
   ulPage = pNewRoot->Page;
   pNewRoot->Page = pPage->Page;
   pPage->Page = ulPage;

   pPage->Owner = pNewRoot;
   pPage->PageType &= ~CDX_NODE_ROOT;
   pNewRoot->PageType = CDX_NODE_ROOT | CDX_NODE_BRANCH;
   pNewRoot->fChanged = TRUE;
   pNewRoot->Child    = pPage;
   pNewRoot->iCurKey  = 0;
   hb_cdxPageIntSetKey( pNewRoot, 0, TRUE,
                        hb_cdxPageGetKeyVal( pPage, pPage->iKeys-1 ),
                        hb_cdxPageGetKeyRec( pPage, pPage->iKeys-1 ),
                        pPage->Page );
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxPageCheckKeys( pNewRoot );
   hb_cdxTagPoolCheck( pPage->TagParent );
#endif
   hb_cdxPageBalance( pNewRoot, NODE_SPLIT );
   return 0;
}

/*
 * remove current Key from Tag
 */
static int hb_cdxPageKeyRemove( LPCDXPAGE pPage )
{
   int iChildRet;

   if ( pPage->PageType & CDX_NODE_LEAF )
      iChildRet = hb_cdxPageLeafDelKey( pPage );
   else /* interior node */
      iChildRet = hb_cdxPageKeyRemove( pPage->Child );
   return hb_cdxPageBalance( pPage, iChildRet );
}

/*
 * add Key to Tag at current position
 */
static int hb_cdxPageKeyInsert( LPCDXPAGE pPage, LPCDXKEY pKey )
{
   int iChildRet;

   if ( pPage->PageType & CDX_NODE_LEAF )
      iChildRet = hb_cdxPageLeafAddKey( pPage, pKey );
   else /* interior node */
      iChildRet = hb_cdxPageKeyInsert( pPage->Child, pKey );
#ifdef HB_CDX_DBGUPDT
   cdxTmpStackSize++;
#endif
   return hb_cdxPageBalance( pPage, iChildRet );
}

/*
 * Store Tag header to index files
 */
static void hb_cdxTagHeaderStore( LPCDXTAG pTag )
{
   USHORT uiKeyLen, uiForLen;
   CDXTAGHEADER tagHeader;

   if ( !pTag->TagChanged )
      return;

   /*
    * TODO: !!! read the following field from the index file,
    *       at least freePtr has to be read for pTag->TagBlock == 0
    * tagHeader.freePtr  [ 4 ]      offset of list of free pages or -1
    * tagHeader.reserved1[ 4 ]      Version number ???
    * tagHeader.reserved2[ 486 ]
    */

   pTag->TagChanged = FALSE;
   pTag->OptFlags &= ~( CDX_TYPE_UNIQUE | CDX_TYPE_FORFILTER |
                        CDX_TYPE_TEMPORARY | CDX_TYPE_CUSTOM );
   if ( pTag->UniqueKey )
      pTag->OptFlags |= CDX_TYPE_UNIQUE;
   if ( pTag->pForItem != NULL )
      pTag->OptFlags |= CDX_TYPE_FORFILTER;
#if defined( HB_SIXCDX )
   if ( pTag->Custom )
      pTag->OptFlags |= CDX_TYPE_TEMPORARY | CDX_TYPE_CUSTOM;
   else if ( pTag->ChgOnly )
      pTag->OptFlags |= CDX_TYPE_CUSTOM;
   else if ( pTag->Partial )
      pTag->OptFlags |= CDX_TYPE_TEMPORARY;
#else
   if ( pTag->Temporary )
      pTag->OptFlags |= CDX_TYPE_TEMPORARY;
   if ( pTag->Custom )
      pTag->OptFlags |= CDX_TYPE_CUSTOM;
#endif

   memset( &tagHeader, 0, sizeof( CDXTAGHEADER ) );
   HB_PUT_LE_UINT32( tagHeader.rootPtr, pTag->RootBlock );
   HB_PUT_LE_UINT16( tagHeader.keySize, pTag->uiLen );
   tagHeader.indexOpt = pTag->OptFlags;
   tagHeader.indexSig = 1;
   if ( !pTag->AscendKey )
      HB_PUT_LE_UINT16( tagHeader.ascendFlg, 1 );

   uiKeyLen = pTag->KeyExpr == NULL ? 0 : strlen( pTag->KeyExpr );
   uiForLen = pTag->ForExpr == NULL ? 0 : strlen( pTag->ForExpr );

   if( uiKeyLen + uiForLen > CDX_HEADEREXPLEN - 2 )
   {
      hb_cdxErrorRT( pTag->pIndex->pArea, EG_DATAWIDTH, EDBF_KEYLENGTH, NULL, 0, 0 );
   }
   else
   {
      HB_PUT_LE_UINT16( tagHeader.keyExpPos, 0 );
      HB_PUT_LE_UINT16( tagHeader.keyExpLen, uiKeyLen + 1 );
      HB_PUT_LE_UINT16( tagHeader.forExpPos, uiKeyLen + 1 );
      HB_PUT_LE_UINT16( tagHeader.forExpLen, uiForLen + 1 );
      if( uiKeyLen > 0 )
      {
         memcpy( tagHeader.keyExpPool, pTag->KeyExpr, uiKeyLen );
      }
      if( uiForLen > 0 )
      {
         memcpy( tagHeader.keyExpPool + uiKeyLen + 1, pTag->ForExpr, uiForLen );
      }
   }
   hb_cdxIndexPageWrite( pTag->pIndex, pTag->TagBlock, (BYTE *) &tagHeader, sizeof( CDXTAGHEADER ) );
}

/*
 * Read a tag definition from the index file
 */
static void hb_cdxTagLoad( LPCDXTAG pTag )
{
   CDXTAGHEADER tagHeader;
   USHORT uiForPos, uiForLen, uiKeyPos, uiKeyLen;
   ULONG ulRecNo;

   /* read the page from a file */
   hb_cdxIndexPageRead( pTag->pIndex, pTag->TagBlock, (BYTE *) &tagHeader, sizeof( CDXTAGHEADER ) );

   uiForPos = HB_GET_LE_UINT16( tagHeader.forExpPos );
   uiForLen = HB_GET_LE_UINT16( tagHeader.forExpLen );
   uiKeyPos = HB_GET_LE_UINT16( tagHeader.keyExpPos );
   uiKeyLen = HB_GET_LE_UINT16( tagHeader.keyExpLen );

   pTag->RootBlock = HB_GET_LE_UINT32( tagHeader.rootPtr );

   /* Return if:
    * no root page allocated
    * invalid root page offset (position inside an index file)
    * invalid key value length
    */
   if( pTag->RootBlock == 0 || pTag->RootBlock % CDX_PAGELEN != 0 ||
       ( HB_FOFFSET ) pTag->RootBlock >= hb_fsSeekLarge( pTag->pIndex->hFile, 0, FS_END ) ||
       HB_GET_LE_UINT16( tagHeader.keySize ) > CDX_MAXKEY ||
       uiForPos + uiForLen > CDX_HEADEREXPLEN ||
       uiKeyPos + uiKeyLen > CDX_HEADEREXPLEN ||
       ( uiKeyPos < uiForPos ? ( uiKeyPos + uiKeyLen > uiForPos && tagHeader.keyExpPool[ uiForPos ] ) :
                               ( uiForPos + uiForLen > uiKeyPos && tagHeader.keyExpPool[ uiForPos ] ) ) )
   {
      pTag->RootBlock = 0; /* To force RT error - index corrupted */
      return;
   }

   /* some wrong RDDs do not set expression length this is workaround for them */
   if( !uiKeyLen )
      uiKeyLen = ( uiForPos >= uiKeyPos ? uiForPos : CDX_HEADEREXPLEN ) - uiKeyPos;
   if( !uiForLen )
      uiForLen = ( uiForPos <= uiKeyPos ? uiKeyPos : CDX_HEADEREXPLEN ) - uiForPos;

   pTag->KeyExpr   = ( char * ) hb_xgrab( uiKeyLen + 1 );
   hb_strncpyTrim( pTag->KeyExpr, ( const char * ) tagHeader.keyExpPool, uiKeyLen );

   pTag->uiLen     = HB_GET_LE_UINT16( tagHeader.keySize );
   pTag->MaxKeys   = CDX_INT_FREESPACE / ( pTag->uiLen + 8 );

   pTag->OptFlags  = tagHeader.indexOpt;
   pTag->UniqueKey = ( pTag->OptFlags & CDX_TYPE_UNIQUE ) != 0;
#if defined( HB_SIXCDX )
   pTag->Temporary = FALSE;
   pTag->Custom    = ( pTag->OptFlags & CDX_TYPE_CUSTOM ) != 0 &&
                     ( pTag->OptFlags & CDX_TYPE_TEMPORARY ) != 0;
   pTag->ChgOnly   = ( pTag->OptFlags & CDX_TYPE_CUSTOM ) != 0 &&
                     ( pTag->OptFlags & CDX_TYPE_TEMPORARY ) == 0;
   pTag->Partial   = ( pTag->OptFlags & CDX_TYPE_CUSTOM ) != 0 ||
                     ( pTag->OptFlags & CDX_TYPE_TEMPORARY ) != 0;
#if 0
   /* For CDX format SIx3 really makes sth like that */
   pTag->Template  = hb_strnicmp( pTag->KeyExpr, "sxChar(", 7 ) == 0 ||
                     hb_strnicmp( pTag->KeyExpr, "sxDate(", 7 ) == 0 ||
                     hb_strnicmp( pTag->KeyExpr, "sxNum(", 6 ) == 0 ||
                     hb_strnicmp( pTag->KeyExpr, "sxLog(", 6 ) == 0 )
   /* SIx3 does not support repeated key value for the same record */
   pTag->MultiKey  = FALSE;
#endif
   pTag->Template  = pTag->MultiKey = pTag->Custom;
#else
   pTag->Temporary = ( pTag->OptFlags & CDX_TYPE_TEMPORARY ) != 0;
   pTag->Custom    = ( pTag->OptFlags & CDX_TYPE_CUSTOM ) != 0;
   pTag->ChgOnly   = FALSE;
   pTag->Partial   = pTag->Temporary || pTag->Custom;
   pTag->Template  = pTag->MultiKey = pTag->Custom;
#endif

   pTag->AscendKey = pTag->UsrAscend = ( HB_GET_LE_UINT16( tagHeader.ascendFlg ) == 0 );
   pTag->UsrUnique = FALSE;
   if ( pTag->OptFlags & CDX_TYPE_STRUCTURE || ! *pTag->KeyExpr )
      return;

   if ( SELF_COMPILE( ( AREAP ) pTag->pIndex->pArea, ( BYTE * ) pTag->KeyExpr ) == FAILURE )
   {
      pTag->RootBlock = 0; /* To force RT error - index corrupted */
      return;
   }
   pTag->pKeyItem = pTag->pIndex->pArea->valResult;
   pTag->pIndex->pArea->valResult = NULL;

   /* go to a blank record before testing expression */
   ulRecNo = pTag->pIndex->pArea->ulRecNo;
   SELF_GOTO( ( AREAP ) pTag->pIndex->pArea, 0 );

   pTag->uiType = hb_cdxItemType( hb_vmEvalBlockOrMacro( pTag->pKeyItem ) );
   pTag->bTrail = ( pTag->uiType == 'C' ) ? ' ' : '\0';
   if ( pTag->uiType == 'C' )
      hb_cdxMakeSortTab( pTag->pIndex->pArea );

   pTag->nField = hb_rddFieldExpIndex( ( AREAP ) pTag->pIndex->pArea,
                                       pTag->KeyExpr );

   /* Check if there is a FOR expression: pTag->OptFlags & CDX_TYPE_FORFILTER */
   if ( tagHeader.keyExpPool[ uiForPos ] != 0 )
   {
      pTag->ForExpr = ( char * ) hb_xgrab( uiForLen + 1 );
      hb_strncpyTrim( pTag->ForExpr, ( const char * ) tagHeader.keyExpPool +
                      uiForPos, uiForLen );
      if ( SELF_COMPILE( ( AREAP ) pTag->pIndex->pArea, ( BYTE * ) pTag->ForExpr ) == FAILURE )
         pTag->RootBlock = 0; /* To force RT error - index corrupted */
      else
      {
         pTag->pForItem = pTag->pIndex->pArea->valResult;
         pTag->pIndex->pArea->valResult = NULL;
         if ( hb_cdxItemType( hb_vmEvalBlockOrMacro( pTag->pForItem ) ) != 'L' )
            pTag->RootBlock = 0; /* To force RT error - index corrupted */
      }
   }
   SELF_GOTO( ( AREAP ) pTag->pIndex->pArea, ulRecNo );

   if ( pTag->uiLen > CDX_MAXKEY || pTag->uiType == 'U' ||
        ( pTag->uiType == 'N' && pTag->uiLen != 8 ) ||
        ( pTag->uiType == 'D' && pTag->uiLen != 8 ) ||
        ( pTag->uiType == 'L' && pTag->uiLen != 1 ) )
   {
      pTag->RootBlock = 0; /* To force RT error - index corrupted */
   }
}

/*
 * release structure with a tag information from memory
 */
static void hb_cdxTagFree( LPCDXTAG pTag )
{
   if ( pTag->RootPage != NULL )
   {
      hb_cdxPageFree( pTag->RootPage, FALSE );
      pTag->RootPage = NULL;
   }
   hb_cdxTagPoolFlush( pTag );
   hb_cdxTagPoolFree( pTag, 0 );
   if ( pTag->TagChanged )
      hb_cdxTagHeaderStore( pTag );
   if ( pTag->szName != NULL )
      hb_xfree( pTag->szName );
   if ( pTag->KeyExpr != NULL )
      hb_xfree( pTag->KeyExpr );
   if ( pTag->pKeyItem != NULL )
      hb_vmDestroyBlockOrMacro( pTag->pKeyItem );
   if ( pTag->ForExpr != NULL )
      hb_xfree( pTag->ForExpr );
   if ( pTag->pForItem != NULL )
      hb_vmDestroyBlockOrMacro( pTag->pForItem );
   hb_cdxKeyFree( pTag->CurKey );
   if ( pTag->HotKey )
      hb_cdxKeyFree( pTag->HotKey );
   hb_cdxTagClearScope( pTag, 0);
   hb_cdxTagClearScope( pTag, 1);
   hb_xfree( pTag );
}

/*
 * Creates a new structure with a tag information
 * TagHdr = offset of index page where a tag header is stored
 *            if CDX_DUMMYNODE then allocate space ofor a new tag header
 */
static LPCDXTAG hb_cdxTagNew( LPCDXINDEX pIndex, char *szTagName, ULONG TagHdr )
{
   LPCDXTAG pTag;
   char szName[ CDX_MAXTAGNAMELEN + 1 ];

   pTag = ( LPCDXTAG ) hb_xgrab( sizeof( CDXTAG ) );
   memset( pTag, 0, sizeof( CDXTAG ) );
   hb_strncpyUpperTrim( szName, szTagName, sizeof( szName ) - 1 );
   pTag->szName = hb_strdup( szName );
   pTag->pIndex = pIndex;
   pTag->AscendKey = pTag->UsrAscend = TRUE;
   pTag->UsrUnique = FALSE;
   pTag->uiType = 'C';
   pTag->bTrail = ' ';
   pTag->CurKey = hb_cdxKeyNew();
   if ( TagHdr == CDX_DUMMYNODE )
   {
      pTag->TagBlock = hb_cdxIndexGetAvailPage( pIndex, TRUE );
      pTag->TagChanged = TRUE;
      pTag->OptFlags = CDX_TYPE_COMPACT | CDX_TYPE_COMPOUND;
   }
   else
   {
      pTag->TagBlock = TagHdr;
      hb_cdxTagLoad( pTag );
      if ( pTag->RootBlock == 0 )
      {
         /* index file is corrupted */
         hb_cdxTagFree( pTag );
         pTag = NULL;
      }
   }
   return pTag;
}

/*
 * close Tag (free used pages into page pool)
 */
static void hb_cdxTagClose( LPCDXTAG pTag )
{
   if ( pTag->RootPage != NULL )
   {
      hb_cdxPageFree( pTag->RootPage, FALSE );
      pTag->RootPage = NULL;
   }
   if ( pTag->TagChanged )
   {
      hb_cdxTagHeaderStore( pTag );
   }
   pTag->fRePos = TRUE;
}

/*
 * (re)open Tag
 */
static void hb_cdxTagOpen( LPCDXTAG pTag )
{
   CDXTAGHEADER tagHeader;

   if ( !pTag->RootPage )
   {
      hb_cdxIndexPageRead( pTag->pIndex, pTag->TagBlock, (BYTE *) &tagHeader, sizeof( CDXTAGHEADER ) );
      pTag->RootBlock = HB_GET_LE_UINT32( tagHeader.rootPtr );
      if ( pTag->RootBlock && pTag->RootBlock != CDX_DUMMYNODE )
         pTag->RootPage = hb_cdxPageNew( pTag, NULL, pTag->RootBlock );
      if ( !pTag->RootPage )
         hb_cdxErrInternal("hb_cdxTagOpen: index corrupted");
   }
}

/*
 * free Tag pages from cache
 */
static void hb_cdxTagPoolFree( LPCDXTAG pTag, int nPagesLeft )
{
   LPCDXPAGE pPage, pPageNext;

#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxTagPoolCheck( pTag );
#endif
   pPage = pTag->pagePool;
   while ( nPagesLeft && pPage )
   {
      pPage = pPage->pPoolNext;
      nPagesLeft--;
   }
   while ( pPage )
   {
      pPageNext = pPage->pPoolNext;
      if ( ! pPage->bUsed )
      {
         hb_cdxPageFree( pPage, TRUE );
      }
      pPage = pPageNext;
   }
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxTagPoolCheck( pTag );
#endif
}

/*
 * write all changed pages in tag cache
 */
static void hb_cdxTagPoolFlush( LPCDXTAG pTag )
{
   LPCDXPAGE pPage;

   pPage = pTag->pagePool;
   while ( pPage )
   {
      if ( pPage->fChanged )
      {
         hb_cdxPageStore( pPage );
      }
      pPage = pPage->pPoolNext;
   }
#ifdef HB_CDX_DBGCODE_EXT
   hb_cdxTagPoolCheck( pTag );
#endif
}

/*
 * retrive CurKey from current Tag possition
 */
static void hb_cdxSetCurKey( LPCDXPAGE pPage )
{
   while ( pPage->Child )
      pPage = pPage->Child;

   hb_cdxKeyPut( pPage->TagParent->CurKey,
                 hb_cdxPageGetKeyVal( pPage, pPage->iCurKey ),
                 pPage->TagParent->uiLen,
                 hb_cdxPageGetKeyRec( pPage, pPage->iCurKey ) );
}

/*
 * seek given Key in the Page or in its children
 */
static int hb_cdxPageSeekKey( LPCDXPAGE pPage, LPCDXKEY pKey, ULONG ulKeyRec, BOOL fExact )
{
   int l, r, n, k;
   BOOL fLeaf = ( pPage->PageType & CDX_NODE_LEAF ) != 0;

   if ( fLeaf && !pPage->pKeyBuf && pPage->iKeys > 0 )
   {
      int iLen = pPage->TagParent->uiLen + 6;
      BYTE *pKeyBuf = (BYTE *) hb_xgrab( pPage->iKeys * iLen );
      hb_cdxPageLeafDecode( pPage, pKeyBuf );
      pPage->pKeyBuf = pKeyBuf;
   }

   k = ( ulKeyRec == CDX_MAX_REC_NUM ) ? -1 : 1;
   n = -1;
   l = 0;
   r = pPage->iKeys - 1;
   while ( l < r )
   {
      n = (l + r ) >> 1;
      k = hb_cdxValCompare( pPage->TagParent, pKey->val, pKey->len,
                            hb_cdxPageGetKeyVal( pPage, n ),
                            (BYTE) pPage->TagParent->uiLen, fExact );
      if ( k == 0 )
      {
         if ( ulKeyRec == CDX_MAX_REC_NUM )
            k = 1;
         else if ( ulKeyRec != CDX_IGNORE_REC_NUM )
         {
            ULONG ulRec = hb_cdxPageGetKeyRec( pPage, n );
            if ( ulKeyRec > ulRec )
               k = 1;
            else if ( ulKeyRec < ulRec )
               k = -1;
         }
      }
      if ( k > 0 )
         l = n + 1;
      else
         r = n;
   }
   pPage->iCurKey = l;
   if ( r < 0 )
      return k;

   if ( !fLeaf )
   {
      hb_cdxPageGetChild( pPage );
#ifdef HB_CDX_DBGCODE
      if ( memcmp( hb_cdxPageGetKeyVal( pPage, pPage->iCurKey ),
                   hb_cdxPageGetKeyVal( pPage->Child, pPage->Child->iKeys-1 ),
                   pPage->TagParent->uiLen ) != 0 ||
           hb_cdxPageGetKeyRec( pPage, pPage->iCurKey ) !=
           hb_cdxPageGetKeyRec( pPage->Child, pPage->Child->iKeys-1 ) )
      {
         printf("\r\nkeyLen=%d", pPage->TagParent->uiLen);
         printf("\r\nparent=%lx, iKey=%d, rec=%ld", pPage->Page, pPage->iCurKey, hb_cdxPageGetKeyRec( pPage, pPage->iCurKey ));
         printf("\r\n child=%lx, iKey=%d, rec=%ld", pPage->Child->Page, pPage->Child->iKeys-1, hb_cdxPageGetKeyRec( pPage->Child, pPage->Child->iKeys-1 ));
         printf("\r\nparent val=[%s]", hb_cdxPageGetKeyVal( pPage, pPage->iCurKey ));
         printf("\r\n child val=[%s]", hb_cdxPageGetKeyVal( pPage->Child, pPage->Child->iKeys-1 ));
         fflush(stdout);
         hb_cdxErrInternal("hb_cdxPageSeekKey: wrong parent key.");
      }
#endif
      k = hb_cdxPageSeekKey( pPage->Child, pKey, ulKeyRec, fExact );
   }
   else if ( l != n || ulKeyRec == CDX_MAX_REC_NUM )
   {
      k = hb_cdxValCompare( pPage->TagParent, pKey->val, pKey->len,
                            hb_cdxPageGetKeyVal( pPage, pPage->iCurKey ),
                            (BYTE) pPage->TagParent->uiLen, fExact );
      if ( k == 0 && ulKeyRec != CDX_MAX_REC_NUM &&
                     ulKeyRec != CDX_IGNORE_REC_NUM )
      {
         ULONG ulRec = hb_cdxPageGetKeyRec( pPage, pPage->iCurKey );
         if ( ulKeyRec > ulRec )
            k = 1;
         else if ( ulKeyRec < ulRec )
            k = -1;
      }
   }
   if ( ulKeyRec == CDX_MAX_REC_NUM )
   {
      if ( pPage->iCurKey > 0 && k < 0 )
      {
         pPage->iCurKey--;
         if ( !fLeaf )
         {
            hb_cdxPageGetChild( pPage );
            k = hb_cdxPageSeekKey( pPage->Child, pKey, ulKeyRec, fExact );
         }
         else
            k = hb_cdxValCompare( pPage->TagParent, pKey->val, pKey->len,
                                  hb_cdxPageGetKeyVal( pPage, pPage->iCurKey ),
                                  (BYTE) pPage->TagParent->uiLen, fExact );
      }
   }
   else if ( k > 0 && fLeaf )
      pPage->iCurKey++;
   return k;
}

/*
 * an interface for fast check record number in record filter
 */
static BOOL hb_cdxCheckRecordScope( CDXAREAP pArea, ULONG ulRec )
{
   LONG lRecNo = ( LONG ) ulRec;

   if ( SELF_COUNTSCOPE( ( AREAP ) pArea, NULL, &lRecNo ) == SUCCESS && lRecNo == 0 )
   {
      return FALSE;
   }
   return TRUE;
}

/*
 * check and avaluate record filter
 */
static BOOL hb_cdxCheckRecordFilter( CDXAREAP pArea, ULONG ulRecNo )
{
   BOOL lResult = FALSE;
   BOOL fDeleted = hb_setGetDeleted();

   if( pArea->dbfi.itmCobExpr || fDeleted )
   {
      if( pArea->ulRecNo != ulRecNo || pArea->lpdbPendingRel )
         SELF_GOTO( ( AREAP ) pArea, ulRecNo );

      if( fDeleted )
         SUPER_DELETED( ( AREAP ) pArea, &lResult );

      if( !lResult && pArea->dbfi.itmCobExpr )
      {
         PHB_ITEM pResult = hb_vmEvalBlock( pArea->dbfi.itmCobExpr );
         lResult = HB_IS_LOGICAL( pResult ) && !hb_itemGetL( pResult );
      }
   }
   return !lResult;
}

/*
 * read Top Key from Page or its children
 */
static BOOL hb_cdxPageReadTopKey( LPCDXPAGE pPage )
{
   while ( ( pPage->PageType & CDX_NODE_LEAF ) == 0 && pPage->iKeys > 0 )
   {
      pPage->iCurKey = 0;
      hb_cdxPageGetChild( pPage );
      pPage = pPage->Child;
   }
   if ( pPage->iKeys == 0 )
      return FALSE;
   pPage->iCurKey = 0;

   hb_cdxSetCurKey( pPage );
   return TRUE;
}

/*
 * read Bottom Key from Page or its children
 */
static BOOL hb_cdxPageReadBottomKey( LPCDXPAGE pPage )
{
   while ( ( pPage->PageType & CDX_NODE_LEAF ) == 0 && pPage->iKeys > 0 )
   {
      pPage->iCurKey = pPage->iKeys - 1;
      hb_cdxPageGetChild( pPage );
      pPage = pPage->Child;
   }
   if ( pPage->iKeys == 0 )
      return FALSE;
   pPage->iCurKey = pPage->iKeys - 1;

   hb_cdxSetCurKey( pPage );
   return TRUE;
}

/*
 * read Previous Key from Page or its children
 */
static BOOL hb_cdxPageReadPrevKey( LPCDXPAGE pPage )
{
   LPCDXPAGE pOwnerPage = NULL;

   while ( pPage->Child )
   {
      pOwnerPage = pPage;
      pPage = pPage->Child;
   }

   do
   {
      pPage->iCurKey--;
      while ( pPage->iCurKey < 0 )
      {
         if ( pPage->Left == CDX_DUMMYNODE || !pOwnerPage )
         {
            pPage->iCurKey = 0;
            if ( pPage->iKeys > 0 )
               hb_cdxSetCurKey( pPage );
            return FALSE;
         }
         pOwnerPage->Child = hb_cdxPageNew( pPage->TagParent, pPage->Owner, pPage->Left );
         hb_cdxPageFree( pPage, !pPage->fChanged );
         pPage = pOwnerPage->Child;
         pPage->iCurKey = pPage->iKeys - 1;
      }
      if( pPage->iCurKey == 0 )
      {
         hb_cdxSetCurKey( pPage );
         if( !hb_cdxTopScope( pPage->TagParent ) ||
             !hb_cdxBottomScope( pPage->TagParent ) )
            break;
      }
   }
   while( ( pPage->TagParent->OptFlags & CDX_TYPE_STRUCTURE ) == 0 &&
          !hb_cdxCheckRecordScope( pPage->TagParent->pIndex->pArea,
                                   hb_cdxPageGetKeyRec( pPage, pPage->iCurKey ) ) );
   if( pPage->iCurKey != 0 )
      hb_cdxSetCurKey( pPage );
   return TRUE;
}

/*
 * read Next Key from Page or its children
 */
static BOOL hb_cdxPageReadNextKey( LPCDXPAGE pPage )
{
   LPCDXPAGE pOwnerPage = NULL;

   while ( pPage->Child )
   {
      pOwnerPage = pPage;
      pPage = pPage->Child;
   }

   do
   {
      pPage->iCurKey++;
      while ( pPage->iCurKey >= pPage->iKeys )
      {
         if ( pPage->Right == CDX_DUMMYNODE || !pOwnerPage )
         {
            pPage->iCurKey = pPage->iKeys;
            return FALSE;
         }
         pOwnerPage->Child = hb_cdxPageNew( pPage->TagParent, pPage->Owner, pPage->Right );
         hb_cdxPageFree( pPage, !pPage->fChanged );
         pPage = pOwnerPage->Child;
         pPage->iCurKey = 0;
      }
      if( pPage->iCurKey == 0 )
      {
         hb_cdxSetCurKey( pPage );
         if( !hb_cdxTopScope( pPage->TagParent ) ||
             !hb_cdxBottomScope( pPage->TagParent ) )
            break;
      }
   }
   while( ( pPage->TagParent->OptFlags & CDX_TYPE_STRUCTURE ) == 0 &&
          !hb_cdxCheckRecordScope( pPage->TagParent->pIndex->pArea,
                                   hb_cdxPageGetKeyRec( pPage, pPage->iCurKey ) ) );
   if( pPage->iCurKey != 0 )
      hb_cdxSetCurKey( pPage );
   return TRUE;
}

/*
 * read Previous Unique Key from Page or its children
 */
static BOOL hb_cdxPageReadPrevUniqKey( LPCDXPAGE pPage )
{
   LPCDXPAGE pOwnerPage = NULL;

   while ( pPage->Child )
   {
      pOwnerPage = pPage;
      pPage = pPage->Child;
   }
   while ( pPage->iCurKey < 0 || memcmp( pPage->TagParent->CurKey->val, hb_cdxPageGetKeyVal( pPage, pPage->iCurKey ), pPage->TagParent->uiLen ) == 0 )
   {
      if ( pPage->iCurKey > 0 )
         pPage->iCurKey--;
      else
      {
         if ( pPage->Left == CDX_DUMMYNODE || !pOwnerPage )
         {
            pPage->iCurKey = 0;
            if ( pPage->iKeys > 0 )
               hb_cdxSetCurKey( pPage );
            return FALSE;
         }
         pOwnerPage->Child = hb_cdxPageNew( pPage->TagParent, pPage->Owner, pPage->Left );
         hb_cdxPageFree( pPage, !pPage->fChanged );
         pPage = pOwnerPage->Child;
         pPage->iCurKey = pPage->iKeys - 1;
      }
   }

   hb_cdxSetCurKey( pPage );
   return TRUE;
}

/*
 * read Next Unique Key from Page or its children
 */
static BOOL hb_cdxPageReadNextUniqKey( LPCDXPAGE pPage )
{
   LPCDXPAGE pOwnerPage = NULL;
   /* BYTE pbVal[CDX_MAXKEY]; */

   while ( pPage->Child )
   {
      pOwnerPage = pPage;
      pPage = pPage->Child;
   }
/*
   memcpy( pbVal, hb_cdxPageGetKeyVal( pPage, pPage->iCurKey ), pPage->TagParent->uiLen );
   pPage->iCurKey++;
   while ( pPage->iCurKey >= pPage->iKeys || memcmp( pbVal, hb_cdxPageGetKeyVal( pPage, pPage->iCurKey ), pPage->TagParent->uiLen ) == 0 )
*/
   while ( pPage->iCurKey >= pPage->iKeys || memcmp( pPage->TagParent->CurKey->val, hb_cdxPageGetKeyVal( pPage, pPage->iCurKey ), pPage->TagParent->uiLen ) == 0 )
   {
      if ( pPage->iCurKey < pPage->iKeys - 1 )
         pPage->iCurKey++;
      else
      {
         if ( pPage->Right == CDX_DUMMYNODE || !pOwnerPage )
         {
            pPage->iCurKey = pPage->iKeys - 1;
            if ( pPage->iKeys > 0 )
               hb_cdxSetCurKey( pPage );
            return FALSE;
         }
         pOwnerPage->Child = hb_cdxPageNew( pPage->TagParent, pPage->Owner, pPage->Right );
         hb_cdxPageFree( pPage, !pPage->fChanged );
         pPage = pOwnerPage->Child;
         pPage->iCurKey = 0;
      }
   }
   hb_cdxSetCurKey( pPage );
   return TRUE;
}

/*
 * read the TOP/BOTTOM/NEXT/PREVIOUS Key from Tag
 */
static void hb_cdxTagKeyRead( LPCDXTAG pTag, BYTE bTypRead )
{
   BOOL fAfter = FALSE, fBof, fEof;

   pTag->CurKey->rec = 0;
   pTag->fRePos = FALSE;
   hb_cdxTagOpen( pTag );

   if ( pTag->UsrUnique )
   {
      switch( bTypRead )
      {
         case NEXT_RECORD:
            bTypRead = NXTU_RECORD;
            break;

         case PREV_RECORD:
            bTypRead = PRVU_RECORD;
         case BTTM_RECORD:
            fAfter = TRUE;
            break;
      }
   }
   if ( pTag->UsrAscend )
   {
      fBof = pTag->TagBOF;
      fEof = pTag->TagEOF;
   }
   else
   {
      fBof = pTag->TagEOF;
      fEof = pTag->TagBOF;
      switch( bTypRead )
      {
         case TOP_RECORD:
            bTypRead = BTTM_RECORD;
            break;

         case BTTM_RECORD:
            bTypRead = TOP_RECORD;
            break;

         case PREV_RECORD:
            bTypRead = NEXT_RECORD;
            break;

         case NEXT_RECORD:
            bTypRead = PREV_RECORD;
            break;

         case PRVU_RECORD:
            bTypRead = NXTU_RECORD;
            break;

         case NXTU_RECORD:
            bTypRead = PRVU_RECORD;
            break;
      }
   }
   switch( bTypRead )
   {
      case TOP_RECORD:
         fBof = fEof = !hb_cdxPageReadTopKey( pTag->RootPage );
         break;

      case BTTM_RECORD:
         fBof = fEof = !hb_cdxPageReadBottomKey( pTag->RootPage );
         break;

      case PREV_RECORD:
         if ( !fBof )
            fBof = !hb_cdxPageReadPrevKey( pTag->RootPage );
         break;

      case NEXT_RECORD:
         if ( !fEof )
            fEof = !hb_cdxPageReadNextKey( pTag->RootPage );
         break;

      case PRVU_RECORD:
         if ( !fBof )
            fBof = !hb_cdxPageReadPrevUniqKey( pTag->RootPage );
         break;

      case NXTU_RECORD:
         if ( !fEof )
            fEof = !hb_cdxPageReadNextUniqKey( pTag->RootPage );
         break;
   }

   if ( fEof )
      pTag->CurKey->rec = 0;
   else if ( fAfter && !fBof )
   {
      if ( pTag->UsrAscend )
      {
         if ( hb_cdxPageReadPrevUniqKey( pTag->RootPage ) )
            hb_cdxPageReadNextKey( pTag->RootPage );
      }
      else
      {
         if ( hb_cdxPageReadNextUniqKey( pTag->RootPage ) )
            hb_cdxPageReadPrevKey( pTag->RootPage );
      }
   }

   if ( pTag->UsrAscend )
   {
      pTag->TagBOF = fBof;
      pTag->TagEOF = fEof;
   }
   else
   {
      pTag->TagBOF = fEof;
      pTag->TagEOF = fBof;
   }
}

/*
 * find pKey in pTag return 0 or TagNO
 */
static ULONG hb_cdxTagKeyFind( LPCDXTAG pTag, LPCDXKEY pKey )
{
   int K;
   ULONG ulKeyRec = pKey->rec;

   pTag->fRePos = FALSE;
   hb_cdxTagOpen( pTag );

   pTag->TagBOF = pTag->TagEOF = FALSE;
   K = hb_cdxPageSeekKey( pTag->RootPage, pKey, ulKeyRec, FALSE );
   if ( ulKeyRec == CDX_MAX_REC_NUM )
      K = - K;

   if ( K > 0 )
   {
      pTag->CurKey->rec = 0;
      pTag->TagEOF = TRUE;
   }
   else
   {
      hb_cdxSetCurKey( pTag->RootPage );
      if ( K == 0 )
         return pTag->CurKey->rec;
   }
   return 0;
}

#if 0
/*
 * find pKey in pTag return 0 or record number, respect descend/unique flags
 */
static ULONG hb_cdxTagKeySeek( LPCDXTAG pTag, LPCDXKEY pKey )
{
   int K;
   ULONG ulKeyRec = pKey->rec;

   if ( pTag->UsrUnique )
   {
      if ( pTag->UsrAscend )
      {
         if ( ulKeyRec == CDX_MAX_REC_NUM )
            ulKeyRec = CDX_IGNORE_REC_NUM;
      }
      else if ( ulKeyRec == CDX_IGNORE_REC_NUM )
         ulKeyRec = CDX_MAX_REC_NUM;
   }
   else if ( ! pTag->UsrAscend )
   {
      if ( ulKeyRec == CDX_MAX_REC_NUM )
         ulKeyRec = CDX_IGNORE_REC_NUM;
      else if ( ulKeyRec == CDX_IGNORE_REC_NUM )
         ulKeyRec = CDX_MAX_REC_NUM;
   }

   pTag->CurKey->rec = 0;
   pTag->fRePos = FALSE;
   hb_cdxTagOpen( pTag );

   pTag->TagBOF = pTag->TagEOF = FALSE;
   K = hb_cdxPageSeekKey( pTag->RootPage, pKey, ulKeyRec, FALSE );
   if ( ulKeyRec == CDX_MAX_REC_NUM )
      K = - K;

   if ( K > 0 )
      pTag->TagEOF = TRUE;
   else
   {
      hb_cdxSetCurKey( pTag->RootPage );
      if ( K == 0 )
         return pTag->CurKey->rec;
   }
   return 0;
}
#endif

/*
 * add the Key into the Tag
 */
static BOOL hb_cdxTagKeyAdd( LPCDXTAG pTag, LPCDXKEY pKey )
{
   hb_cdxTagOpen( pTag );
   if( hb_cdxPageSeekKey( pTag->RootPage, pKey,
                          pTag->UniqueKey ? CDX_IGNORE_REC_NUM : pKey->rec,
                          TRUE ) != 0 || ( pTag->Custom && pTag->MultiKey &&
                                           !pTag->UniqueKey ) )
   {
      hb_cdxPageKeyInsert( pTag->RootPage, pKey );
      pTag->curKeyState &= ~( CDX_CURKEY_RAWPOS | CDX_CURKEY_LOGPOS |
                              CDX_CURKEY_RAWCNT | CDX_CURKEY_LOGCNT );
      pTag->fRePos = TRUE;
      /* TODO: !!! remove when page leaf balance can save CurKey */
      hb_cdxTagKeyFind( pTag, pKey );
      return TRUE;
   }
   return FALSE;
}

/*
 * delete the Key from the Tag
 */
static BOOL hb_cdxTagKeyDel( LPCDXTAG pTag, LPCDXKEY pKey )
{
   if ( hb_cdxTagKeyFind( pTag, pKey ) != 0 )
   {
      hb_cdxPageKeyRemove( pTag->RootPage );
      pTag->curKeyState &= ~( CDX_CURKEY_RAWPOS | CDX_CURKEY_LOGPOS |
                              CDX_CURKEY_RAWCNT | CDX_CURKEY_LOGCNT );
      pTag->CurKey->rec = 0;
      return TRUE;
   }
   return FALSE;
}

/*
 * Go to the first visible record in Tag
 */
static void hb_cdxTagGoTop( LPCDXTAG pTag )
{
   LPCDXKEY pKey = pTag->UsrAscend ? pTag->topScopeKey : pTag->bottomScopeKey;
   ULONG ulPos = 1;

   if ( pKey )
      hb_cdxTagKeyFind( pTag, pKey );
   else
      hb_cdxTagKeyRead( pTag, TOP_RECORD );

   do
   {
      if ( pTag->CurKey->rec == 0 || pTag->TagEOF || ! hb_cdxBottomScope( pTag ) )
      {
         pTag->TagBOF = pTag->TagEOF = TRUE;
         pTag->CurKey->rec = 0;
         break;
      }
      else if ( ( pTag->OptFlags & CDX_TYPE_STRUCTURE ) != 0 ||
                hb_cdxCheckRecordScope( pTag->pIndex->pArea, pTag->CurKey->rec ) )
      {
         pTag->rawKeyPos = ulPos;
         CURKEY_SETRAWPOS( pTag );
         break;
      }
      hb_cdxTagKeyRead( pTag, NEXT_RECORD );
      ulPos++;
   }
   while ( TRUE );
}

/*
 * Go to the last visible record in Tag
 */
static void hb_cdxTagGoBottom( LPCDXTAG pTag )
{
   LPCDXKEY pKey = pTag->UsrAscend ? pTag->bottomScopeKey : pTag->topScopeKey;
   ULONG ulPos = 0;

   if ( pKey )
      hb_cdxTagKeyFind( pTag, pKey );
   else
      hb_cdxTagKeyRead( pTag, BTTM_RECORD );

   do
   {
      if ( pTag->CurKey->rec == 0 || pTag->TagBOF || ! hb_cdxTopScope( pTag ) )
      {
         pTag->TagBOF = pTag->TagEOF = TRUE;
         pTag->CurKey->rec = 0;
         break;
      }
      else if ( ( pTag->OptFlags & CDX_TYPE_STRUCTURE ) != 0 ||
                hb_cdxCheckRecordScope( pTag->pIndex->pArea, pTag->CurKey->rec ) )
      {
         if ( CURKEY_RAWCNT( pTag ) )
         {
            pTag->rawKeyPos = pTag->rawKeyCount - ulPos;
            CURKEY_SETRAWPOS( pTag );
         }
         break;
      }
      hb_cdxTagKeyRead( pTag, PREV_RECORD );
      ulPos++;
   }
   while ( TRUE );
}

/*
 * skip to Next Key in the Tag
 */
static void hb_cdxTagSkipNext( LPCDXTAG pTag )
{
   BOOL fPos = CURKEY_RAWPOS( pTag ), fEof = FALSE;
   ULONG ulSkip = 1;

   if ( pTag->CurKey->rec != 0 )
   {
      if ( !hb_cdxTopScope( pTag ) )
      {
         ulSkip = 0;
         hb_cdxTagGoTop( pTag );
      }
      else
         hb_cdxTagKeyRead( pTag, NEXT_RECORD );
   }

   while ( !fEof )
   {
      if ( pTag->TagEOF || pTag->CurKey->rec == 0 ||
           !hb_cdxBottomScope( pTag ) || !hb_cdxTopScope( pTag ) )
         fEof = TRUE;
      else if ( ( pTag->OptFlags & CDX_TYPE_STRUCTURE ) != 0 ||
                hb_cdxCheckRecordScope( pTag->pIndex->pArea, pTag->CurKey->rec ) )
         break;
      hb_cdxTagKeyRead( pTag, NEXT_RECORD );
      ulSkip++;
   }

   if ( fEof )
   {
      pTag->CurKey->rec = 0;
      pTag->TagEOF = TRUE;
   }
   else if ( fPos )
   {
      pTag->rawKeyPos += ulSkip;
      CURKEY_SETRAWPOS( pTag );
   }
}

/*
 * skip to Previous Key in the Tag
 */
static void hb_cdxTagSkipPrev( LPCDXTAG pTag )
{
   BOOL fPos = CURKEY_RAWPOS( pTag ), fBof = FALSE;
   ULONG ulSkip = 1;

   if ( pTag->CurKey->rec == 0 )
   {
      ulSkip = 0;
      hb_cdxTagGoBottom( pTag );
   }
   else
      hb_cdxTagKeyRead( pTag, PREV_RECORD );

   while ( !fBof )
   {
      if ( pTag->TagBOF || pTag->CurKey->rec == 0 ||
           !hb_cdxBottomScope( pTag ) || !hb_cdxTopScope( pTag ) )
         fBof = TRUE;
      else if ( ( pTag->OptFlags & CDX_TYPE_STRUCTURE ) != 0 ||
                hb_cdxCheckRecordScope( pTag->pIndex->pArea, pTag->CurKey->rec ) )
         break;
      hb_cdxTagKeyRead( pTag, PREV_RECORD );
      ulSkip++;
   }


   if ( fBof )
   {
      hb_cdxTagGoTop( pTag );
      pTag->TagBOF = TRUE;
   }
   else if ( fPos )
   {
      pTag->rawKeyPos -= ulSkip;
      CURKEY_SETRAWPOS( pTag );
   }
}

/*
 * Reorder the Tag list by their position in index file (not names)
 * to be Clipper compatible
 */
static void hb_cdxReorderTagList( LPCDXTAG * TagListPtr )
{
   LPCDXTAG *pTagPtr, pTagTmp;
   BOOL fRepeat = TRUE;

   while ( fRepeat )
   {
      fRepeat = FALSE;
      pTagPtr = TagListPtr;
      while ( *pTagPtr && (*pTagPtr)->pNext )
      {
         if ( (*pTagPtr)->TagBlock > (*pTagPtr)->pNext->TagBlock )
         {
            pTagTmp = (*pTagPtr);
            (*pTagPtr) = (*pTagPtr)->pNext;
            pTagTmp->pNext = (*pTagPtr)->pNext;
            (*pTagPtr)->pNext = pTagTmp;
            fRepeat = TRUE;
         }
         pTagPtr = &(*pTagPtr)->pNext;
      }
   }
}

/*
 * create new order header, store it and then make an order
 */
static LPCDXTAG hb_cdxIndexCreateTag( BOOL fStruct, LPCDXINDEX pIndex,
                                      char * szTagName,
                                      char * KeyExp, PHB_ITEM pKeyItem,
                                      BYTE bType, USHORT uiLen,
                                      char * ForExp, PHB_ITEM pForItem,
                                      BOOL fAscnd, BOOL fUniq, BOOL fCustom,
                                      BOOL fReindex )
{
   LPCDXTAG pTag;

   pTag = hb_cdxTagNew( pIndex, szTagName, CDX_DUMMYNODE );

   if ( fStruct )
      pTag->OptFlags |= CDX_TYPE_STRUCTURE;

   if ( bType == 'C' )
      hb_cdxMakeSortTab( pTag->pIndex->pArea );

   if ( KeyExp != NULL )
   {
      pTag->KeyExpr = hb_strduptrim( KeyExp );
      pTag->nField = hb_rddFieldExpIndex( ( AREAP ) pTag->pIndex->pArea,
                                          pTag->KeyExpr );
   }
   pTag->pKeyItem = pKeyItem;
   if ( ForExp != NULL )
      pTag->ForExpr = hb_strduptrim( ForExp );

   pTag->pForItem = pForItem;
   pTag->AscendKey = pTag->UsrAscend = fAscnd;
   pTag->UniqueKey = fUniq;
   pTag->UsrUnique = FALSE;
   pTag->Custom    = fCustom;
   pTag->Template  = pTag->MultiKey = pTag->Custom;
   pTag->Partial   = pTag->ChgOnly = FALSE;
   pTag->uiType = bType;
   pTag->bTrail = ( pTag->uiType == 'C' ) ? ' ' : '\0';
   pTag->uiLen = uiLen;
   pTag->MaxKeys = CDX_INT_FREESPACE / ( uiLen + 8 );
   pTag->TagChanged = TRUE;
   hb_cdxTagDoIndex( pTag, fReindex );

   return pTag;
}

/*
 * create structural (compound) tag
 */
static void hb_cdxIndexCreateStruct( LPCDXINDEX pIndex, char * szTagName )
{
   /* here we can change default tag name */
   pIndex->pCompound = hb_cdxIndexCreateTag( TRUE, pIndex, szTagName,
                           NULL, NULL, 'C', CDX_MAXTAGNAMELEN, NULL, NULL,
                           TRUE, FALSE, FALSE, FALSE );
}

/*
 * free page and all child pages
 */
static void hb_cdxIndexFreePages( LPCDXPAGE pPage )
{
   if ( ( pPage->PageType & CDX_NODE_LEAF ) == 0 )
   {
      LPCDXPAGE pChildPage;
      int iKey;

      for ( iKey = 0; iKey < pPage->iKeys; iKey++ )
      {
         pChildPage = hb_cdxPageNew( pPage->TagParent, NULL,
                                     hb_cdxPageGetKeyPage( pPage, iKey ) );
         if ( pChildPage )
            hb_cdxIndexFreePages( pChildPage );
      }
   }
   pPage->PageType = CDX_NODE_UNUSED;
   hb_cdxPageFree( pPage, FALSE );
}

/*
 * remove Tag from Bag
 */
static void hb_cdxIndexDelTag( LPCDXINDEX pIndex, char * szTagName )
{
   LPCDXTAG *pTagPtr = &pIndex->TagList;

   while ( *pTagPtr && hb_stricmp( (*pTagPtr)->szName, szTagName ) != 0 )
      pTagPtr = &(*pTagPtr)->pNext;

   if ( *pTagPtr )
   {
      LPCDXTAG pTag = *pTagPtr;
      LPCDXKEY pKey = hb_cdxKeyPutC( NULL, szTagName, pIndex->pCompound->uiLen,
                                     pTag->TagBlock );
      if ( hb_cdxTagKeyDel( pIndex->pCompound, pKey ) )
      {
         if ( pTag != pIndex->TagList || pTag->pNext != NULL )
         {
            LPCDXPAGE pPage;

            hb_cdxTagOpen( pTag );
            pPage = pTag->RootPage;
            hb_cdxTagClose( pTag );
            if ( ! pIndex->fShared )
            {
               if ( pPage )
                  hb_cdxIndexFreePages( pPage );
               hb_cdxIndexPutAvailPage( pIndex, pTag->TagBlock, TRUE );
            }
            pTag->TagChanged = FALSE;
         }
      }
      *pTagPtr = pTag->pNext;
      hb_cdxTagFree( pTag );
      hb_cdxKeyFree( pKey );
   }
}

/*
 * add tag to order bag
 */
static LPCDXTAG hb_cdxIndexAddTag( LPCDXINDEX pIndex, char * szTagName,
                                   char * szKeyExp, PHB_ITEM pKeyItem,
                                   BYTE bType, USHORT uiLen,
                                   char * szForExp, PHB_ITEM pForItem,
                                   BOOL fAscend, BOOL fUnique, BOOL fCustom,
                                   BOOL fReindex )
{
   LPCDXTAG pTag, *pTagPtr;
   LPCDXKEY pKey;

   /* Delete previous tag first to free the place for new one
    * its redundant Tag should be already deleted
    */
   hb_cdxIndexDelTag( pIndex, szTagName );

   /* Create new tag an add to tag list */
   pTag = hb_cdxIndexCreateTag( FALSE, pIndex, szTagName, szKeyExp, pKeyItem,
                                bType, uiLen, szForExp, pForItem,
                                fAscend, fUnique, fCustom, fReindex );
   pTagPtr = &pIndex->TagList;
   while ( *pTagPtr )
      pTagPtr = &(*pTagPtr)->pNext;
   *pTagPtr = pTag;
   pKey = hb_cdxKeyPutC( NULL, szTagName, pIndex->pCompound->uiLen, pTag->TagBlock );
   hb_cdxTagKeyAdd( pIndex->pCompound, pKey );
   hb_cdxKeyFree( pKey );
   return pTag;
}

/*
 * rebuild from scratch all orders in index file
 */
static void hb_cdxIndexReindex( LPCDXINDEX pIndex )
{
   LPCDXTAG pCompound, pTagList, pTag;

   hb_cdxIndexLockWrite( pIndex );
   hb_cdxIndexDiscardBuffers( pIndex );

   pCompound = pIndex->pCompound;
   pTagList = pIndex->TagList;
   pIndex->pCompound = NULL;
   pIndex->TagList = NULL;

   pIndex->ulVersion = 0;
   pIndex->nextAvail = 0;
   pIndex->freePage = 0;
   hb_fsSeek( pIndex->hFile, 0, FS_SET );
   hb_fsWrite( pIndex->hFile, NULL, 0 );
   pIndex->fChanged = TRUE;

   /* Rebuild the compound (master) tag */
   if ( pCompound )
   {
      hb_cdxIndexCreateStruct( pIndex, pCompound->szName );
      hb_cdxTagFree( pCompound );
   }

   /* Rebuild each tag */
   while ( pTagList )
   {
      pTag = pTagList;
      hb_cdxIndexAddTag( pIndex, pTag->szName, pTag->KeyExpr, pTag->pKeyItem,
         (BYTE) pTag->uiType, pTag->uiLen, pTag->ForExpr, pTag->pForItem,
         pTag->AscendKey, pTag->UniqueKey, pTag->Custom, TRUE );
      pTagList = pTag->pNext;
      pTag->pKeyItem = pTag->pForItem = NULL;
      hb_cdxTagFree( pTag );
   }
   hb_cdxIndexUnLockWrite( pIndex );
}

/*
 * create new index structure
 */
static LPCDXINDEX hb_cdxIndexNew( CDXAREAP pArea )
{
   LPCDXINDEX pIndex;

   pIndex = ( LPCDXINDEX ) hb_xgrab( sizeof( CDXINDEX ) );
   memset( pIndex, 0, sizeof( CDXINDEX ) );
   pIndex->hFile = FS_ERROR;
   pIndex->pArea = pArea;
   pIndex->nextAvail = CDX_DUMMYNODE;
   return pIndex;
}

/*
 * free (close) all tag in index file
 */
static void hb_cdxIndexFreeTags( LPCDXINDEX pIndex )
{
   LPCDXTAG pTag;

   /* Free Compound tag */
   if ( pIndex->pCompound != NULL )
   {
      hb_cdxTagFree( pIndex->pCompound );
      pIndex->pCompound = NULL;
   }

   while ( pIndex->TagList )
   {
      pTag = pIndex->TagList;
      pIndex->TagList = pTag->pNext;
      hb_cdxTagFree( pTag );
   }
}

/*
 * free (close) index and all tags in it
 */
static void hb_cdxIndexFree( LPCDXINDEX pIndex )
{
   /* Free List of Free Pages */
   hb_cdxIndexDropAvailPage( pIndex );

   /* free all tags */
   hb_cdxIndexFreeTags( pIndex );

   /* Close file */
   if( pIndex->hFile != FS_ERROR )
   {
      hb_fsClose( pIndex->hFile );
      if( pIndex->fDelete )
      {
         hb_fsDelete( ( BYTE * ) ( pIndex->szRealName ?
                                   pIndex->szRealName : pIndex->szFileName ) );
      }
   }

#ifdef HB_CDX_DBGCODE
   if( pIndex->fShared && ( pIndex->lockWrite || pIndex->lockRead ) &&
       hb_vmRequestQuery() == 0 )
      hb_errInternal( 9104, "hb_cdxIndexFree: index file still locked.", NULL, NULL );

   if( ( pIndex->WrLck || pIndex->RdLck ) &&
       hb_vmRequestQuery() == 0 )
      hb_errInternal( 9104, "hb_cdxIndexFree: index file still locked (*)", NULL, NULL );
#endif

   if ( pIndex->szFileName != NULL )
      hb_xfree( pIndex->szFileName );
   if( pIndex->szRealName )
      hb_xfree( pIndex->szRealName );

   hb_xfree( pIndex );
}

/*
 * load orders from index file
 */
static BOOL hb_cdxIndexLoad( LPCDXINDEX pIndex, char * szBaseName )
{
   LPCDXTAG TagList, * pTagPtr;
   BOOL fResult = FALSE;

   TagList = NULL;
   pTagPtr = &TagList;

   hb_cdxIndexLockRead( pIndex );
   /* load the tags*/
   pIndex->pCompound = hb_cdxTagNew( pIndex, szBaseName, 0L );

   /* check if index is not corrupted */
   if ( pIndex->pCompound )
   {
      fResult = TRUE;
      pIndex->pCompound->OptFlags = CDX_TYPE_COMPACT | CDX_TYPE_COMPOUND | CDX_TYPE_STRUCTURE;
      hb_cdxTagGoTop( pIndex->pCompound );
      while ( !pIndex->pCompound->TagEOF )
      {
         *pTagPtr = hb_cdxTagNew( pIndex, (char *) pIndex->pCompound->CurKey->val,
                                  pIndex->pCompound->CurKey->rec );
         /* tag is corrupted - break tags loading */
         if ( *pTagPtr == NULL )
         {
            fResult = FALSE;
            break;
         }
         pTagPtr = &(*pTagPtr)->pNext;
         hb_cdxTagSkipNext( pIndex->pCompound );
      }
   }

   hb_cdxIndexUnLockRead( pIndex );
   hb_cdxReorderTagList( &TagList );
   pTagPtr = &pIndex->TagList;
   while ( *pTagPtr != NULL )
      pTagPtr = &(*pTagPtr)->pNext;
   (*pTagPtr) = TagList;

#ifdef HB_CDX_DSPDBG_INFO
   hb_cdxDspTags( pIndex );
#endif

   return fResult;
}

/*
 * create index file name
 */
static void hb_cdxCreateFName( CDXAREAP pArea, char * szBagName, BOOL * fProd,
                               char * szFileName, char * szBaseName )
{
   PHB_FNAME pFileName;
   PHB_ITEM pExt = NULL;
   BOOL fName = szBagName && *szBagName;

   pFileName = hb_fsFNameSplit( fName ? szBagName : pArea->szDataFileName );

   if( szBaseName )
   {
      if( pFileName->szName )
         hb_strncpyUpperTrim( szBaseName, pFileName->szName, CDX_MAXTAGNAMELEN );
      else
         szBaseName[ 0 ] = '\0';
   }

   if( ( hb_setGetDefExtension() && !pFileName->szExtension ) || !fName )
   {
      DBORDERINFO pExtInfo;
      memset( &pExtInfo, 0, sizeof( pExtInfo ) );
      pExt = pExtInfo.itmResult = hb_itemPutC( NULL, NULL );
      if( SELF_ORDINFO( ( AREAP ) pArea, DBOI_BAGEXT, &pExtInfo ) == SUCCESS &&
          hb_itemGetCLen( pExt ) > 0 )
      {
         pFileName->szExtension = hb_itemGetCPtr( pExt );
      }
   }
   hb_fsFNameMerge( szFileName, pFileName );

   if( fProd )
   {
      if( ! pFileName->szName )
         *fProd = FALSE;
      else if( !fName )
         *fProd = TRUE;
      else
      {
         PHB_FNAME pTableFileName = hb_fsFNameSplit( pArea->szDataFileName );

         *fProd = pTableFileName->szName &&
                  hb_stricmp( pTableFileName->szName, pFileName->szName ) == 0;
         if( *fProd && pFileName->szExtension && ! pExt )
         {
            DBORDERINFO pExtInfo;
            memset( &pExtInfo, 0, sizeof( pExtInfo ) );
            pExt = pExtInfo.itmResult = hb_itemPutC( NULL, NULL );
            if( SELF_ORDINFO( ( AREAP ) pArea, DBOI_BAGEXT, &pExtInfo ) == SUCCESS )
            {
               *fProd = hb_stricmp( pFileName->szExtension,
                                    hb_itemGetCPtr( pExt ) ) == 0;
            }
         }
         hb_xfree( pTableFileName );
      }
   }
   hb_xfree( pFileName );
   if( pExt )
      hb_itemRelease( pExt );
}

/*
 * free (close) used indexes, if not fAll then keep structure index
 */
static void hb_cdxOrdListClear( CDXAREAP pArea, BOOL fAll, LPCDXINDEX pKeepInd )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_cdxOrdListClear(%p, %d)", pArea, (int) fAll));

   if ( pArea->lpIndexes )
   {
      LPCDXINDEX pIndex, * pIndexPtr;

      if ( !fAll )
      {
         /* TODO: we have to control this on open */
         PHB_FNAME pFileNameDbf, pFileNameCdx;
         pFileNameDbf = hb_fsFNameSplit( pArea->szDataFileName );
         pFileNameCdx = hb_fsFNameSplit( pArea->lpIndexes->szFileName );
         fAll = hb_stricmp( pFileNameDbf->szName ? pFileNameDbf->szName : "",
                            pFileNameCdx->szName ? pFileNameCdx->szName : "" ) != 0;
         if( !fAll )
         {
            DBORDERINFO pExtInfo;
            PHB_ITEM pExt;

            memset( &pExtInfo, 0, sizeof( pExtInfo ) );
            pExt = pExtInfo.itmResult = hb_itemPutC( NULL, NULL );
            if( SELF_ORDINFO( ( AREAP ) pArea, DBOI_BAGEXT, &pExtInfo ) == SUCCESS )
            {
               fAll = hb_stricmp( pFileNameCdx->szExtension,
                                  hb_itemGetCPtr( pExt ) ) != 0;
            }
            hb_itemRelease( pExt );
         }
         hb_xfree( pFileNameDbf );
         hb_xfree( pFileNameCdx );
      }
      pIndexPtr = fAll ? &pArea->lpIndexes : &pArea->lpIndexes->pNext;
      while ( *pIndexPtr )
      {
         pIndex = *pIndexPtr;
         if ( pKeepInd == pIndex )
            pIndexPtr = &pIndex->pNext;
         else
         {
            *pIndexPtr = pIndex->pNext;
            hb_cdxIndexFree( pIndex );
         }
      }
   }
}


/*
 * find order bag by its name
 */
static LPCDXINDEX hb_cdxFindBag( CDXAREAP pArea, char * szBagName )
{
   LPCDXINDEX pIndex;
   PHB_FNAME pFileName;
   char * szBaseName, * szBasePath, * szBaseExt;

   pFileName = hb_fsFNameSplit( szBagName );
   szBaseName = hb_strdup( pFileName->szName ? pFileName->szName : "" );
   szBasePath = pFileName->szPath ? hb_strdup( pFileName->szPath ) : NULL;
   szBaseExt = pFileName->szExtension ? hb_strdup( pFileName->szExtension ) : NULL;
   hb_strUpper( szBaseName, strlen(szBaseName) );

   pIndex = pArea->lpIndexes;
   while ( pIndex )
   {
      hb_xfree( pFileName );
      pFileName = hb_fsFNameSplit( pIndex->szFileName );
      if ( !hb_stricmp( pFileName->szName ? pFileName->szName : "", szBaseName ) &&
           ( !szBasePath ||
             ( pFileName->szPath && !hb_stricmp( pFileName->szPath, szBasePath ) ) ) &&
           ( !szBaseExt ||
             ( pFileName->szExtension && !hb_stricmp( pFileName->szExtension, szBaseExt ) ) ) )
         break;
      pIndex = pIndex->pNext;
   }
   hb_xfree( pFileName );
   hb_xfree( szBaseName );
   if ( szBasePath )
      hb_xfree( szBasePath );
   if ( szBaseExt )
      hb_xfree( szBaseExt );
   return pIndex;
}

/*
 * get Tag by number
 */
static LPCDXTAG hb_cdxGetTagByNumber( CDXAREAP pArea, USHORT uiTag )
{
   LPCDXTAG pTag = NULL;
   LPCDXINDEX pIndex = pArea->lpIndexes;

   while ( uiTag && pIndex )
   {
      pTag = pIndex->TagList;
      while ( uiTag && pTag )
      {
         if ( --uiTag )
            pTag = pTag->pNext;
      }
      pIndex = pIndex->pNext;
   }
   return pTag;
}

/*
 * get Tag number
 */
static USHORT hb_cdxGetTagNumber( CDXAREAP pArea, LPCDXTAG pFindTag )
{
   USHORT uiTag = 0;
   LPCDXTAG pTag = NULL;
   LPCDXINDEX pIndex = pArea->lpIndexes;

   if ( pFindTag )
   {
      while ( pIndex && ( pTag != pFindTag ) )
      {
         pTag = pIndex->TagList;
         while ( pTag )
         {
            uiTag++;
            if ( pTag == pFindTag )
               break;
            pTag = pTag->pNext;
         }
         pIndex = pIndex->pNext;
      }
      if ( !pTag )
         uiTag = 0;
   }
   return uiTag;
}

/*
 * find Tag in tag list
 */
static LPCDXTAG hb_cdxFindTag( CDXAREAP pArea, PHB_ITEM pTagItem,
                               PHB_ITEM pBagItem, USHORT *puiTag )
{
   LPCDXTAG pTag = NULL;
   int iTag = 0, iFind = 0;
   char szTag[ CDX_MAXTAGNAMELEN + 1 ];
   LPCDXINDEX pIndex = pArea->lpIndexes;
   BOOL fBag;

   hb_strncpyUpperTrim( szTag, hb_itemGetCPtr( pTagItem ), sizeof( szTag ) - 1 );
   if( ! szTag[0] )
      iFind = hb_itemGetNI( pTagItem );

   fBag = hb_itemGetCLen( pBagItem ) > 0;
   if( fBag )
   {
      if( szTag[ 0 ] )
         pIndex = hb_cdxFindBag( pArea, hb_itemGetCPtr( pBagItem ) );
   }
   else
   {
      int iBag = hb_itemGetNI( pBagItem );

      if( iBag > 0 )
      {
         fBag = TRUE;
         while( pIndex )
         {
            if( --iBag == 0 )
               break;
            pIndex = pIndex->pNext;
         }
      }
      else if( iBag < 0 )
      {
         pIndex = NULL;
      }
   }

   if( pIndex && ( iFind > 0 || szTag[0] ) )
   {
      do
      {
         pTag = pIndex->TagList;
         while( pTag )
         {
            iTag++;
            if ( ( iFind != 0 ? iTag == iFind : !hb_stricmp( pTag->szName, szTag ) ) )
               break;
            pTag = pTag->pNext;
         }
         if ( pTag || fBag )
            break;
         pIndex = pIndex->pNext;
      } while ( pIndex );
   }

   if( puiTag )
   {
      if( !pTag )
         *puiTag = 0;
      else if( fBag )
         *puiTag = hb_cdxGetTagNumber( pArea, pTag );
      else
         *puiTag = iTag;
   }

   return pTag;
}

/*
 * get current active Tag
 */
static LPCDXTAG hb_cdxGetActiveTag( CDXAREAP pArea )
{
   LPCDXTAG pTag;

   if ( !pArea->uiTag )
      return NULL;
   pTag = hb_cdxGetTagByNumber( pArea, pArea->uiTag );
   if ( !pTag )
      pArea->uiTag = 0;
   return pTag;
}

/*
 * refresh CurKey value and set proper path from RootPage to LeafPage
 */
static BOOL hb_cdxCurKeyRefresh( CDXAREAP pArea, LPCDXTAG pTag )
{
   if( pArea->lpdbPendingRel )
      SELF_FORCEREL( ( AREAP ) pArea );

   if ( !pArea->fPositioned )
   {
      pTag->TagEOF = TRUE;
      pTag->fRePos = FALSE;
      pTag->CurKey->rec = 0;
      return FALSE;
   }
   else if ( pTag->fRePos || pTag->CurKey->rec != pArea->ulRecNo )
   {
      BYTE buf[CDX_MAXKEY];
      BOOL fBuf = FALSE;
      LPCDXKEY pKey = NULL;

      /* Try to find previous if it's key for the same record */
      if ( pTag->CurKey->rec == pArea->ulRecNo )
      {
         fBuf = TRUE;
         memcpy( buf, pTag->CurKey->val, pTag->CurKey->len );
         pKey = hb_cdxKeyCopy( pKey, pTag->CurKey );
         hb_cdxTagKeyFind( pTag, pKey );
      }
      if ( pTag->CurKey->rec != pArea->ulRecNo )
      {
         BOOL fValidBuf = pArea->fValidBuffer;
         /* not found, create new key from DBF and if differs seek again */
         pKey = hb_cdxKeyEval( pKey, pTag );
         if ( !fBuf || memcmp( buf, pKey->val, pKey->len ) != 0 )
         {
            hb_cdxTagKeyFind( pTag, pKey );
         }
         /* not found, if key was generated from DBF buffer then force to
          * update it, create the new key and if differs seek again */
         if ( pTag->CurKey->rec != pArea->ulRecNo && fValidBuf )
         {
            SELF_GOTO( ( AREAP ) pArea, pArea->ulRecNo );
            memcpy( buf, pKey->val, pKey->len );
            pKey = hb_cdxKeyEval( pKey, pTag );
            if ( memcmp( buf, pKey->val, pKey->len ) != 0 )
               hb_cdxTagKeyFind( pTag, pKey );
         }
      }
      hb_cdxKeyFree( pKey );
      return ( pTag->CurKey->rec != 0 && pTag->CurKey->rec == pArea->ulRecNo );
   }
   return TRUE;
}

/*
 * skip to next/previous unique key
 */
static ERRCODE hb_cdxDBOISkipUnique( CDXAREAP pArea, LPCDXTAG pTag, BOOL fForward )
{
   ERRCODE retval;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxDBOISkipUnique(%p, %p, %i)", pArea, pTag, fForward));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if ( ! pTag )
      return SELF_SKIP( ( AREAP ) pArea, fForward ? 1 : -1 );

   if( pArea->lpdbPendingRel )
      SELF_FORCEREL( ( AREAP ) pArea );

   pArea->fTop = pArea->fBottom = FALSE;

   if ( !pArea->fPositioned )
   {
      if ( fForward )
         retval = SELF_GOTO( ( AREAP ) pArea, 0 );
      else
         retval = SELF_GOBOTTOM( ( AREAP ) pArea );
   }
   else
   {
      LPCDXKEY pKey = NULL;
      BOOL fOut = FALSE;

      hb_cdxIndexLockRead( pTag->pIndex );
      hb_cdxTagRefreshScope( pTag );
      if ( ! hb_cdxCurKeyRefresh( pArea, pTag ) )
      {
         if ( pTag->TagEOF || ( fForward ? !hb_cdxBottomScope( pTag ) :
                                           !hb_cdxTopScope( pTag ) ) )
            fOut = TRUE;
         else if ( ( fForward ? pTag->UsrAscend && hb_cdxTopScope( pTag ) :
                               !pTag->UsrAscend && hb_cdxBottomScope( pTag ) ) &&
                   pTag->CurKey->rec != 0 )
         {
            pKey = hb_cdxKeyEval( pKey, pTag );
         }
      }
      if ( fForward )
      {
         if ( pArea->fPositioned && !pTag->TagEOF )
         {
            if ( !pKey )
            {
               pKey = hb_cdxKeyCopy( NULL, pTag->CurKey );
               hb_cdxTagSkipNext( pTag );
            }
            while ( !pTag->TagEOF )
            {
               if ( hb_cdxValCompare( pTag, pKey->val, pKey->len,
                           pTag->CurKey->val, pTag->CurKey->len, TRUE ) != 0 )
               {
                  SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );
                  SELF_SKIPFILTER( ( AREAP ) pArea, 1 );
                  break;
               }
               hb_cdxTagSkipNext( pTag );
            }
         }
         retval = SELF_GOTO( ( AREAP ) pArea, ( !pArea->fPositioned || pTag->TagEOF )
                                              ? 0 : pTag->CurKey->rec );
      }
      else
      {
         if ( !fOut && !pTag->TagBOF )
         {
            if ( !pKey )
            {
               pKey = hb_cdxKeyCopy( NULL, pTag->CurKey );
               hb_cdxTagSkipPrev( pTag );
            }
            while ( !pTag->TagBOF )
            {
               if ( hb_cdxValCompare( pTag, pKey->val, pKey->len,
                           pTag->CurKey->val, pTag->CurKey->len, TRUE ) != 0 )
               {
                  SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );
                  SELF_SKIPFILTER( ( AREAP ) pArea, -1 );
                  break;
               }
               hb_cdxTagSkipPrev( pTag );
            }
         }

         if ( fOut || pTag->TagBOF )
         {
            retval = SELF_GOTOP( ( AREAP ) pArea );
            pArea->fBof = TRUE;
         }
         else
         {
            retval = SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );
         }
      }
      hb_cdxIndexUnLockRead( pTag->pIndex );
      if ( pKey )
         hb_cdxKeyFree( pKey );
   }
   /* Update Bof and Eof flags */
   if( fForward )
      pArea->fBof = FALSE;
   else
      pArea->fEof = FALSE;

   return retval;
}

/*
 * skip while code block doesn't return TRUE
 */
static BOOL hb_cdxDBOISkipEval( CDXAREAP pArea, LPCDXTAG pTag, BOOL fForward,
                                PHB_ITEM pEval )
{
   BOOL fFound = FALSE, fFirst = TRUE;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxDBOISkipEval(%p, %p, %i, %p)", pArea, pTag, fForward, pEval));

   if( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FALSE;

   if( ! pTag || hb_itemType( pEval ) != HB_IT_BLOCK )
   {
      if( SELF_SKIP( ( AREAP ) pArea, fForward ? 1 : -1 ) == FAILURE )
         return FALSE;
      return fForward ? !pArea->fEof : !pArea->fBof;
   }

   if( pArea->lpdbPendingRel )
      SELF_FORCEREL( ( AREAP ) pArea );

   pArea->fTop = pArea->fBottom = FALSE;

   hb_cdxIndexLockRead( pTag->pIndex );
   hb_cdxTagRefreshScope( pTag );
   if ( ! hb_cdxCurKeyRefresh( pArea, pTag ) )
   {
      if ( !pTag->TagEOF && pTag->CurKey->rec != 0 &&
           ( fForward ? pTag->UsrAscend : !pTag->UsrAscend ) &&
           hb_cdxTopScope( pTag ) && hb_cdxBottomScope( pTag ) )
         fFirst = FALSE;
   }
   if ( fForward )
   {
      if ( fFirst )
         hb_cdxTagSkipNext( pTag );
      while ( !pTag->TagEOF )
      {
         if ( SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec ) == FAILURE )
            break;
         if ( hb_cdxEvalSeekCond( pTag, pEval ) )
         {
            ULONG ulRecNo = pArea->ulRecNo;
            SELF_SKIPFILTER( ( AREAP ) pArea, 1 );
            if ( pArea->ulRecNo == ulRecNo || hb_cdxEvalSeekCond( pTag, pEval ) )
            {
               fFound = TRUE;
               break;
            }
         }
         hb_cdxTagSkipNext( pTag );
      }
      if ( !fFound )
         SELF_GOTO( ( AREAP ) pArea, 0 );
   }
   else
   {
      if ( fFirst )
         hb_cdxTagSkipPrev( pTag );
      while ( !pTag->TagBOF )
      {
         if ( SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec ) == FAILURE )
            break;
         if ( hb_cdxEvalSeekCond( pTag, pEval ) )
         {
            ULONG ulRecNo = pArea->ulRecNo;
            SELF_SKIPFILTER( ( AREAP ) pArea, -1 );
            if ( pArea->ulRecNo == ulRecNo || hb_cdxEvalSeekCond( pTag, pEval ) )
            {
               fFound = TRUE;
               break;
            }
         }
         hb_cdxTagSkipPrev( pTag );
      }
      if ( !fFound )
      {
         SELF_GOTOP( ( AREAP ) pArea );
         pArea->fBof = TRUE;
      }
   }
   hb_cdxIndexUnLockRead( pTag->pIndex );

   /* Update Bof and Eof flags */
   if( fForward )
      pArea->fBof = FALSE;
   else
      pArea->fEof = FALSE;

   return fFound;
}

/*
 * skip while comparison with given pattern with wildcards doesn't return TRUE
 */
static BOOL hb_cdxDBOISkipWild( CDXAREAP pArea, LPCDXTAG pTag, BOOL fForward,
                                PHB_ITEM pWildItm )
{
   BOOL fFound = FALSE, fFirst = TRUE;
   char *szPattern, *szFree = NULL;
   int iFixed = 0, iStop;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxDBOISkipWild(%p, %p, %i, %p)", pArea, pTag, fForward, pWildItm));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FALSE;

   szPattern = hb_itemGetCPtr( pWildItm );

   if ( ! pTag || pTag->uiType != 'C' || !szPattern || !*szPattern )
   {
      if ( SELF_SKIP( ( AREAP ) pArea, fForward ? 1 : -1 ) == FAILURE )
         return FALSE;
      return fForward ? pArea->fPositioned : !pArea->fBof;
   }

#ifndef HB_CDP_SUPPORT_OFF
   if( pArea->cdPage != hb_vmCDP() )
   {
      szPattern = szFree = hb_strdup( szPattern );
      hb_cdpTranslate( szPattern, hb_vmCDP(), pArea->cdPage );
   }
#endif
   while( iFixed < pTag->uiLen && szPattern[ iFixed ] &&
          szPattern[ iFixed ] != '*' && szPattern[ iFixed ] != '?' )
   {
      ++iFixed;
   }

   if( pArea->lpdbPendingRel )
      SELF_FORCEREL( ( AREAP ) pArea );

   pArea->fTop = pArea->fBottom = FALSE;

   hb_cdxIndexLockRead( pTag->pIndex );
   hb_cdxTagRefreshScope( pTag );
   if ( ! hb_cdxCurKeyRefresh( pArea, pTag ) )
   {
      if ( !pTag->TagEOF && pTag->CurKey->rec != 0 &&
           ( fForward ? pTag->UsrAscend : !pTag->UsrAscend ) &&
           hb_cdxTopScope( pTag ) && hb_cdxBottomScope( pTag ) )
         fFirst = FALSE;
   }

   iStop = pTag->UsrAscend ? -1 : 1;
   if( !fForward )
      iStop = -iStop;

   if( iFixed && !pTag->TagEOF && pTag->CurKey->rec != 0 &&
       hb_cdxValCompare( pTag, ( BYTE * ) szPattern, iFixed,
                         pTag->CurKey->val, iFixed, FALSE ) == -iStop )
   {
      LPCDXKEY pKey;

      pKey = hb_cdxKeyPut( NULL, ( BYTE * ) szPattern, iFixed, 
                     pTag->UsrAscend ? CDX_IGNORE_REC_NUM : CDX_MAX_REC_NUM );
      if( !hb_cdxTagKeyFind( pTag, pKey ) )
      {
         if( fForward )
            pTag->TagEOF = TRUE;
         else
            pTag->TagBOF = TRUE;
      }
      hb_cdxKeyFree( pKey );
      fFirst = FALSE;
   }

   if ( fForward )
   {
      if ( fFirst )
         hb_cdxTagSkipNext( pTag );
      while ( !pTag->TagEOF )
      {
         if ( hb_strMatchWild( (const char *) pTag->CurKey->val, szPattern ) )
         {
            ULONG ulRecNo = pTag->CurKey->rec;
            if( SELF_GOTO( ( AREAP ) pArea, ulRecNo ) != SUCCESS )
               break;
            SELF_SKIPFILTER( ( AREAP ) pArea, 1 );
            if ( pArea->ulRecNo == ulRecNo ||
                 hb_strMatchWild( (const char *) pTag->CurKey->val, szPattern ) )
            {
               fFound = TRUE;
               break;
            }
         }
         if( iFixed && hb_cdxValCompare( pTag, ( BYTE * ) szPattern, iFixed,
                                pTag->CurKey->val, iFixed, FALSE ) == iStop )
         {
            break;
         }
         hb_cdxTagSkipNext( pTag );
      }
      if( !fFound )
         SELF_GOTO( ( AREAP ) pArea, 0 );
   }
   else
   {
      if ( fFirst )
         hb_cdxTagSkipPrev( pTag );
      while ( !pTag->TagBOF )
      {
         if ( hb_strMatchWild( (const char *) pTag->CurKey->val, szPattern ) )
         {
            ULONG ulRecNo = pTag->CurKey->rec;
            if( SELF_GOTO( ( AREAP ) pArea, ulRecNo ) != SUCCESS )
               break;
            SELF_SKIPFILTER( ( AREAP ) pArea, -1 );
            if ( pArea->ulRecNo == ulRecNo ||
                 hb_strMatchWild( (const char *) pTag->CurKey->val, szPattern ) )
            {
               fFound = TRUE;
               break;
            }
         }
         if( iFixed && hb_cdxValCompare( pTag, ( BYTE * ) szPattern, iFixed,
                                pTag->CurKey->val, iFixed, FALSE ) == iStop )
         {
            break;
         }
         hb_cdxTagSkipPrev( pTag );
      }
      if ( !fFound )
      {
         SELF_GOTOP( ( AREAP ) pArea );
         pArea->fBof = TRUE;
      }
   }
   hb_cdxIndexUnLockRead( pTag->pIndex );

   /* Update Bof and Eof flags */
   if( fForward )
      pArea->fBof = FALSE;
   else
      pArea->fEof = FALSE;

   if( szFree )
      hb_xfree( szFree );

   return fFound;
}

static BOOL hb_cdxRegexMatch( CDXAREAP pArea, PHB_REGEX pRegEx, LPCDXKEY pKey )
{
   char * szKey = ( char * ) pKey->val;
#ifndef HB_CDP_SUPPORT_OFF
   char szBuff[ CDX_MAXKEY + 1 ];

   if( pArea->cdPage != hb_vmCDP() )
   {
      memcpy( szBuff, szKey, pKey->len + 1 );
      hb_cdpnTranslate( szBuff, pArea->cdPage, hb_vmCDP(), pKey->len );
      szKey = szBuff;
   }
#else
   HB_SYMBOL_UNUSED( pArea );
#endif
   return hb_regexMatch( pRegEx, szKey, pKey->len, FALSE );
}

/*
 * skip while regular expression on index key val doesn't return TRUE
 */
static BOOL hb_cdxDBOISkipRegEx( CDXAREAP pArea, LPCDXTAG pTag, BOOL fForward,
                                 PHB_ITEM pRegExItm )
{
   BOOL fFound = FALSE, fFirst = TRUE;
   PHB_REGEX pRegEx;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxDBOISkipRegEx(%p, %p, %i, %p)", pArea, pTag, fForward, pRegExItm));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FALSE;

   if( !pTag || pTag->uiType != 'C' || ( pRegEx = hb_regexGet( pRegExItm, 0 ) ) == NULL )
   {
      if ( SELF_SKIP( ( AREAP ) pArea, fForward ? 1 : -1 ) == FAILURE )
         return FALSE;
      return fForward ? pArea->fPositioned : !pArea->fBof;
   }

   if( pArea->lpdbPendingRel )
      SELF_FORCEREL( ( AREAP ) pArea );

   pArea->fTop = pArea->fBottom = FALSE;

   hb_cdxIndexLockRead( pTag->pIndex );
   hb_cdxTagRefreshScope( pTag );
   if ( ! hb_cdxCurKeyRefresh( pArea, pTag ) )
   {
      if ( !pTag->TagEOF && pTag->CurKey->rec != 0 &&
           ( fForward ? pTag->UsrAscend : !pTag->UsrAscend ) &&
           hb_cdxTopScope( pTag ) && hb_cdxBottomScope( pTag ) )
         fFirst = FALSE;
   }
   if ( fForward )
   {
      if ( fFirst )
         hb_cdxTagSkipNext( pTag );
      while ( !pTag->TagEOF )
      {
         if( hb_cdxRegexMatch( pArea, pRegEx, pTag->CurKey ) )
         {
            ULONG ulRecNo = pArea->ulRecNo;
            SELF_SKIPFILTER( ( AREAP ) pArea, 1 );
            if ( pArea->ulRecNo == ulRecNo ||
                 hb_cdxRegexMatch( pArea, pRegEx, pTag->CurKey ) )
            {
               fFound = TRUE;
               break;
            }
         }
         hb_cdxTagSkipNext( pTag );
      }
      SELF_GOTO( ( AREAP ) pArea, fFound ? pTag->CurKey->rec : 0 );
   }
   else
   {
      if ( fFirst )
         hb_cdxTagSkipPrev( pTag );
      while ( !pTag->TagBOF )
      {
         if( hb_cdxRegexMatch( pArea, pRegEx, pTag->CurKey ) )
         {
            ULONG ulRecNo = pArea->ulRecNo;
            SELF_SKIPFILTER( ( AREAP ) pArea, -1 );
            if ( pArea->ulRecNo == ulRecNo ||
                 hb_cdxRegexMatch( pArea, pRegEx, pTag->CurKey ) )
            {
               fFound = TRUE;
               break;
            }
         }
         hb_cdxTagSkipPrev( pTag );
      }
      if ( fFound )
         SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );
      else
      {
         SELF_GOTOP( ( AREAP ) pArea );
         pArea->fBof = TRUE;
      }
   }
   hb_cdxIndexUnLockRead( pTag->pIndex );

   /* Update Bof and Eof flags */
   if( fForward )
      pArea->fBof = FALSE;
   else
      pArea->fEof = FALSE;

   hb_regexFree( pRegEx );

   return fFound;
}

/*
 * evaluate given C function in given scope
 */
static ULONG hb_cdxDBOIScopeEval( LPCDXTAG pTag, HB_EVALSCOPE_FUNC pFunc, void *pParam, PHB_ITEM pItemLo, PHB_ITEM pItemHi )
{
   ULONG ulCount = 0, ulLen = ( ULONG ) pTag->uiLen;
   LPCDXKEY pCurKey = hb_cdxKeyCopy( NULL, pTag->CurKey ),
            pTopScopeKey = pTag->topScopeKey,
            pBtmScopeKey = pTag->bottomScopeKey;

   /* TODO: RT error when item type differ then Tag type */
   if ( !pItemLo || HB_IS_NIL( pItemLo ) )
      pTag->topScopeKey = NULL;
   else
      pTag->topScopeKey = hb_cdxKeyPutItem( NULL, pItemLo, CDX_IGNORE_REC_NUM, pTag, TRUE, FALSE );

   if ( !pItemHi || HB_IS_NIL( pItemHi ) )
      pTag->bottomScopeKey = NULL;
   else
      pTag->bottomScopeKey = hb_cdxKeyPutItem( NULL, pItemHi, CDX_MAX_REC_NUM, pTag, TRUE, FALSE );

   hb_cdxIndexLockRead( pTag->pIndex );
   hb_cdxTagGoTop( pTag );
   while( !pTag->TagEOF )
   {
      pFunc( pTag->CurKey->rec, pTag->CurKey->val, ulLen, pParam );
      ulCount++;
      hb_cdxTagSkipNext( pTag );
   }
   hb_cdxIndexUnLockRead( pTag->pIndex );

   if ( pTag->topScopeKey )
      hb_cdxKeyFree( pTag->topScopeKey );
   pTag->topScopeKey = pTopScopeKey;
   if ( pTag->bottomScopeKey )
      hb_cdxKeyFree( pTag->bottomScopeKey );
   pTag->bottomScopeKey = pBtmScopeKey;
   pTag->curKeyState &= ~( CDX_CURKEY_RAWPOS | CDX_CURKEY_LOGPOS );

   pTag->fRePos = TRUE;
   hb_cdxKeyCopy( pTag->CurKey, pCurKey );
   hb_cdxKeyFree( pCurKey );

   return ulCount;
}

/*
 * return number of keys in order
 */
static LONG hb_cdxDBOIKeyCount( CDXAREAP pArea, LPCDXTAG pTag, BOOL fFilters )
{
   ULONG ulKeyCount = 0;
   BOOL fLogOpt = pArea->dbfi.itmCobExpr || !pArea->dbfi.fFilter;

   if ( pTag )
   {
      BOOL fCheckFilter = ( fLogOpt && fFilters && pArea->dbfi.itmCobExpr );
      ULONG ulRecNo = pArea->ulRecNo;
      LPCDXKEY pCurKey;
      hb_cdxIndexLockRead( pTag->pIndex );
      hb_cdxTagRefreshScope( pTag );

      if ( pTag && ( fFilters ? fLogOpt && CURKEY_LOGCNT( pTag ) : CURKEY_RAWCNT( pTag ) ) )
      {
         ulKeyCount = fFilters ? pTag->logKeyCount : pTag->rawKeyCount;
      }
      else
      {
         if ( pTag->topScopeKey || pTag->bottomScopeKey || pTag->UsrUnique || pArea->dbfi.fFilter )
         {
            pCurKey = hb_cdxKeyCopy( NULL, pTag->CurKey );
            hb_cdxTagGoTop( pTag );
            while ( !pTag->TagEOF )
            {
               if ( !fCheckFilter || hb_cdxCheckRecordFilter( pArea, pTag->CurKey->rec ) )
                  ulKeyCount++;
               hb_cdxTagSkipNext( pTag );
            }
            pTag->fRePos = TRUE;
            hb_cdxKeyCopy( pTag->CurKey, pCurKey );
            hb_cdxKeyFree( pCurKey );
            if ( fCheckFilter )
               SELF_GOTO( ( AREAP ) pArea, ulRecNo );
         }
         else
         {
            LPCDXPAGE pPage;
            pCurKey = hb_cdxKeyCopy( NULL, pTag->CurKey );
            if ( pTag->UsrAscend )
               hb_cdxTagGoTop( pTag );
            else
               hb_cdxTagGoBottom( pTag );
            pPage = pTag->RootPage;
            while ( pPage->Child )
               pPage = pPage->Child;
            ulKeyCount = pPage->iKeys;
            if ( pPage->Right != CDX_DUMMYNODE )
            {
               ULONG ulPage = pPage->Right;
               pPage = hb_cdxPageNew( pTag, NULL, CDX_DUMMYNODE );
               pPage->Page = ulPage;
               while ( pPage->Page != CDX_DUMMYNODE )
               {
                  hb_cdxPageLoad( pPage );
                  ulKeyCount += pPage->iKeys;
                  pPage->Page = pPage->Right;
               }
               hb_cdxPageFree( pPage, TRUE );
            }
            pTag->fRePos = TRUE;
            hb_cdxKeyCopy( pTag->CurKey, pCurKey );
            hb_cdxKeyFree( pCurKey );
         }
         if ( !fFilters )
         {
            pTag->rawKeyCount = ulKeyCount;
            pTag->curKeyState |= CDX_CURKEY_RAWCNT;
         }
         else if ( fLogOpt )
         {
            pTag->logKeyCount = ulKeyCount;
            pTag->curKeyState |= CDX_CURKEY_LOGCNT;
         }
      }
      hb_cdxIndexUnLockRead( pTag->pIndex );
   }
   else  /* no filter, no order */
   {
      if ( fLogOpt && fFilters && pArea->dbfi.itmCobExpr )
      {
         ULONG ulRecNo = pArea->ulRecNo;

         if ( SELF_GOTOP( ( AREAP ) pArea ) == SUCCESS )
         {
            while ( !pArea->fEof )
            {
               ulKeyCount++;
               if ( SELF_SKIP( ( AREAP ) pArea, 1 ) != SUCCESS )
                  break;
            }
            SELF_GOTO( ( AREAP ) pArea, ulRecNo );
         }
      }
      else
      {
         SELF_RECCOUNT( ( AREAP ) pArea, &ulKeyCount );
      }
   }
   return ulKeyCount;
}

/*
 * return logical key position in order
 */
static LONG hb_cdxDBOIKeyNo( CDXAREAP pArea, LPCDXTAG pTag, BOOL fFilters )
{
   ULONG ulKeyNo = 0;
   BOOL fLogOpt = pArea->dbfi.itmCobExpr || !pArea->dbfi.fFilter;

   if( pArea->lpdbPendingRel )
      SELF_FORCEREL( ( AREAP ) pArea );

   if ( !pArea->fPositioned )
      ulKeyNo = 0;
   else if ( pTag )
   {
      BOOL fCheckFilter = ( fLogOpt && fFilters && pArea->dbfi.itmCobExpr );
      ULONG ulRecNo = pArea->ulRecNo;

      hb_cdxIndexLockRead( pTag->pIndex );
      hb_cdxTagRefreshScope( pTag );

      if ( fFilters ? ( fLogOpt && CURKEY_LOGPOS( pTag ) ) : 
                      ( CURKEY_RAWPOS( pTag ) && 
                                          pTag->rawKeyRec == pArea->ulRecNo ) )
      {
         ulKeyNo = fFilters ? pTag->logKeyPos : pTag->rawKeyPos;
      }
      else
      {
         hb_cdxTagOpen( pTag );
         if ( hb_cdxCurKeyRefresh( pArea, pTag ) )
         {
            if ( pTag->topScopeKey || pTag->bottomScopeKey || pTag->UsrUnique || pArea->dbfi.fFilter )
            {
               if ( hb_cdxBottomScope( pTag ) && hb_cdxTopScope( pTag ) &&
                    ( !fCheckFilter || hb_cdxCheckRecordFilter( pArea, ulRecNo ) ) )
               {
                  LPCDXKEY pCurKey = hb_cdxKeyCopy( NULL, pTag->CurKey );
                  if ( !hb_cdxCheckRecordScope( pArea, pTag->CurKey->rec ) )
                     hb_cdxTagSkipPrev( pTag );
                  while ( !pTag->TagBOF )
                  {
                     if ( !fCheckFilter || hb_cdxCheckRecordFilter( pArea, pTag->CurKey->rec ) )
                        ulKeyNo++;
                     hb_cdxTagSkipPrev( pTag );
                  }
                  pTag->fRePos = TRUE;
                  hb_cdxKeyCopy( pTag->CurKey, pCurKey );
                  hb_cdxKeyFree( pCurKey );
                  if ( fCheckFilter )
                     SELF_GOTO( ( AREAP ) pArea, ulRecNo );
               }
            }
            else
            {
               LPCDXPAGE pPage = pTag->RootPage;
               while ( pPage->Child )
                  pPage = pPage->Child;
               if ( pTag->UsrAscend )
               {
                  ulKeyNo = pPage->iCurKey + 1;
                  if ( pPage->Left != CDX_DUMMYNODE )
                  {
                     ULONG ulPage = pPage->Left;
                     pPage = hb_cdxPageNew( pTag, NULL, CDX_DUMMYNODE );
                     pPage->Page = ulPage;
                     while ( pPage->Page != CDX_DUMMYNODE )
                     {
                        hb_cdxPageLoad( pPage );
                        ulKeyNo += pPage->iKeys;
                        pPage->Page = pPage->Left;
                     }
                     hb_cdxPageFree( pPage, TRUE );
                  }
               }
               else
               {
                  ulKeyNo = pPage->iKeys - pPage->iCurKey;
                  if ( pPage->Right != CDX_DUMMYNODE )
                  {
                     ULONG ulPage = pPage->Right;
                     pPage = hb_cdxPageNew( pTag, NULL, CDX_DUMMYNODE );
                     pPage->Page = ulPage;
                     while ( pPage->Page != CDX_DUMMYNODE )
                     {
                        hb_cdxPageLoad( pPage );
                        ulKeyNo += pPage->iKeys;
                        pPage->Page = pPage->Right;
                     }
                     hb_cdxPageFree( pPage, TRUE );
                  }
               }
            }
            if ( ulKeyNo != 0 )
            {
               if ( !fFilters )
               {
                  pTag->rawKeyPos = ulKeyNo;
                  CURKEY_SETRAWPOS( pTag );
               }
               else if ( fLogOpt )
               {
                  pTag->logKeyPos = ulKeyNo;
                  CURKEY_SETLOGPOS( pTag );
               }
            }
         }
      }
      hb_cdxIndexUnLockRead( pTag->pIndex );
   }
   else
   {
      ULONG ulRecNo = pArea->ulRecNo;

      if ( fLogOpt && fFilters && pArea->dbfi.itmCobExpr )
      {
         if ( hb_cdxCheckRecordFilter( pArea, ulRecNo ) )
         {
            do
            {
               ulKeyNo++;
               if ( SELF_SKIP( ( AREAP ) pArea, -1 ) != SUCCESS )
                  break;
            } while ( !( ( AREAP ) pArea )->fBof );
            SELF_GOTO( ( AREAP ) pArea, ulRecNo );
         }
      }
      else
      {
         ulKeyNo = ulRecNo;
      }
   }
   return ulKeyNo;
}

/*
 * DBOI_KEYGOTO goto specific logical record in the index file
 */
static ERRCODE hb_cdxDBOIKeyGoto( CDXAREAP pArea, LPCDXTAG pTag, ULONG ulKeyNo, BOOL fFilters )
{
   ERRCODE retval;
   ULONG ulKeyCnt = ulKeyNo;
   BOOL fLogOpt = pArea->dbfi.itmCobExpr || !pArea->dbfi.fFilter;

   if ( ulKeyNo == 0 )
      retval = SELF_GOTO( ( AREAP ) pArea, 0 );
   else if ( pTag )
   {
      BOOL fCheckFilter = ( fLogOpt && fFilters && pArea->dbfi.itmCobExpr );
      hb_cdxIndexLockRead( pTag->pIndex );
      hb_cdxTagRefreshScope( pTag );
      if ( ! pArea->lpdbPendingRel && ( fFilters ?
               fLogOpt && CURKEY_LOGPOS( pTag ) && pTag->logKeyPos == ulKeyNo :
               ( CURKEY_RAWPOS( pTag ) && pTag->rawKeyPos == ulKeyNo ) ) )
      {
         retval = SELF_GOTO( ( AREAP ) pArea, fFilters ? pTag->logKeyRec : pTag->rawKeyRec );
      }
      else
      {
         if ( pTag->topScopeKey || pTag->bottomScopeKey || pTag->UsrUnique || pArea->dbfi.fFilter )
         {
            hb_cdxTagGoTop( pTag );
            if ( fCheckFilter )
               while ( !pTag->TagEOF )
               {
                  if ( hb_cdxCheckRecordFilter( pArea, pTag->CurKey->rec ) )
                  {
                     if ( ! --ulKeyCnt )
                        break;
                  }
                  hb_cdxTagSkipNext( pTag );
               }
            else
               while( !pTag->TagEOF && --ulKeyCnt )
                  hb_cdxTagSkipNext( pTag );
         }
         else
         {
            LPCDXPAGE pPage, pOwnerPage = NULL;
            ULONG ulNextPg;
            hb_cdxTagGoTop( pTag );
            pPage = pTag->RootPage;
            while ( pPage->Child )
            {
               pOwnerPage = pPage;
               pPage = pPage->Child;
            }
            while ( (ULONG) pPage->iKeys < ulKeyCnt && pOwnerPage &&
                    ( ulNextPg = pTag->UsrAscend ?
                      pPage->Right : pPage->Left ) != CDX_DUMMYNODE )
            {
               ulKeyCnt -= pPage->iKeys;
               pOwnerPage->Child = hb_cdxPageNew( pPage->TagParent, pPage->Owner, ulNextPg );
               hb_cdxPageFree( pPage, FALSE );
               pPage = pOwnerPage->Child;
            }
            if ( (ULONG) pPage->iKeys >= ulKeyCnt )
            {
               pPage->iCurKey = pTag->UsrAscend ? ( int ) ulKeyCnt - 1 : pPage->iKeys - ( int ) ulKeyCnt;
               hb_cdxSetCurKey( pPage );
            }
            else
            {
               pTag->CurKey->rec = 0;
            }
         }
         retval = SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );
         if ( pArea->fPositioned )
         {
            if ( !fFilters )
            {
               pTag->rawKeyPos = ulKeyNo;
               CURKEY_SETRAWPOS( pTag );
            }
            else if ( fLogOpt )
            {
               pTag->logKeyPos = ulKeyNo;
               CURKEY_SETLOGPOS( pTag );
            }
         }
      }
      hb_cdxIndexUnLockRead( pTag->pIndex );
   }
   else
   {
      if ( fLogOpt && fFilters && pArea->dbfi.itmCobExpr )
      {
         retval = SELF_GOTOP( ( AREAP ) pArea );
         if( retval == SUCCESS && --ulKeyCnt )
            retval = SELF_SKIP( ( AREAP ) pArea, ulKeyCnt );
      }
      else
      {
         retval = SELF_GOTO( ( AREAP ) pArea, ulKeyNo );
      }
   }

   return retval;
}

static double hb_cdxCountRelKeyPos( LPCDXPAGE pPage )
{
   return ( ( pPage->Child ? hb_cdxCountRelKeyPos( pPage->Child ) : 0.5 ) +
            pPage->iCurKey ) / pPage->iKeys;
}

static BOOL hb_cdxGoToRelKeyPos( LPCDXPAGE pPage, double dPos )
{
   do
   {
      if( pPage->iKeys == 0 )
         return FALSE;

      pPage->iCurKey = ( int ) ( dPos * pPage->iKeys );
      if( pPage->iCurKey >= pPage->iKeys )
         pPage->iCurKey = pPage->iKeys - 1;

      if( ( pPage->PageType & CDX_NODE_LEAF ) != 0 )
         break;

      dPos = dPos * pPage->iKeys - pPage->iCurKey;
      if( dPos < 0.0 )
         dPos = 0.0;
      else if( dPos >= 1.0 )
         dPos = 1.0;

      hb_cdxPageGetChild( pPage );
      pPage = pPage->Child;
   }
   while( pPage );

   return TRUE;
}

static double hb_cdxDBOIGetRelKeyPos( CDXAREAP pArea, LPCDXTAG pTag )
{
   ULONG ulRecNo = 0, ulRecCount = 0;
   double dPos = 0.0;

   /* resolve any pending relations */
   SELF_RECNO( ( AREAP ) pArea, &ulRecNo );

   if( !pArea->fPositioned )
   {
      if( ulRecNo > 1 )
         dPos = 1.0;
   }
   else if( !pTag )
   {
      SELF_RECCOUNT( ( AREAP ) pArea, &ulRecCount );
      if( ulRecCount != 0 )
         dPos = ( 0.5 + ulRecNo ) / ulRecCount;
   }
   else
   {
      LPCDXKEY pKey;
      double dStart, dStop, dFact = 0.0000000000001;
      BOOL fOK = TRUE;

      if( pTag->UsrAscend )
      {
         dStart = 0.0;
         dStop = 1.0;
      }
      else
      {
         dStart = 1.0;
         dStop = 0.0;
      }

      hb_cdxIndexLockRead( pTag->pIndex );
      hb_cdxTagRefreshScope( pTag );

      pKey = pTag->UsrAscend ? pTag->topScopeKey : pTag->bottomScopeKey;
      if( pKey )
      {
         hb_cdxTagKeyFind( pTag, pKey );
         if( pTag->CurKey->rec == 0 || pTag->TagEOF || ! hb_cdxBottomScope( pTag ) )
            fOK = FALSE;
         else
            dStart = hb_cdxCountRelKeyPos( pTag->RootPage );
      }
      pKey = pTag->UsrAscend ? pTag->bottomScopeKey : pTag->topScopeKey;
      if( pKey && fOK )
      {
         hb_cdxTagKeyFind( pTag, pKey );
         if( pTag->CurKey->rec == 0 || pTag->TagBOF || ! hb_cdxTopScope( pTag ) )
            fOK = FALSE;
         else
            dStop = hb_cdxCountRelKeyPos( pTag->RootPage );
      }
      if( fOK )
      {
         if( !pTag->UsrAscend )
         {
            double dTmp = dStart;
            dStart = dStop;
            dStop = dTmp;
         }
         pTag->fRePos = TRUE;
         if( hb_cdxCurKeyRefresh( pArea, pTag ) &&
             hb_cdxTopScope( pTag ) && hb_cdxBottomScope( pTag ) )
         {
            if( dStart >= dStop - dFact )
               dPos = 0.5;
            else
            {
               dPos = hb_cdxCountRelKeyPos( pTag->RootPage );
               dPos = ( dPos - dStart ) / ( dStop - dStart );
               if( !pTag->UsrAscend )
                  dPos = 1.0 - dPos;
               /* fix possible differences in FL representation */
               if( dPos <= 0.0 )
                  dPos = 0.0;
               else if( dPos >= 1.0 )
                  dPos = 1.0;
            }
         }
      }
      hb_cdxIndexUnLockRead( pTag->pIndex );
   }

   return dPos;
}

static void hb_cdxDBOISetRelKeyPos( CDXAREAP pArea, LPCDXTAG pTag, double dPos )
{
   if( !pTag )
   {
      if( dPos >= 1.0 )
      {
         SELF_GOBOTTOM( ( AREAP ) pArea );
      }
      else if( dPos <= 0.0 )
      {
         SELF_GOTOP( ( AREAP ) pArea );
      }
      else
      {
         ULONG ulRecCount, ulRecNo;
         SELF_RECCOUNT( ( AREAP ) pArea, &ulRecCount );
         ulRecNo = ( ULONG ) dPos * ulRecCount + 1;
         if( ulRecNo >= ulRecCount )
            ulRecNo = ulRecCount;
         SELF_GOTO( ( AREAP ) pArea, ulRecNo );
         SELF_SKIPFILTER( ( AREAP ) pArea, 1 );
         if( pArea->fEof )
            SELF_GOTOP( ( AREAP ) pArea );
      }
   }
   else
   {
      BOOL fForward = TRUE, fOK = TRUE, fTop = FALSE;
      hb_cdxIndexLockRead( pTag->pIndex );
      hb_cdxTagRefreshScope( pTag );

      if( dPos >= 1.0 )
      {
         fForward = FALSE;
      }
      else if( dPos <= 0.0 )
      {
         fTop = TRUE;
      }
      else
      {
         LPCDXKEY pKey;
         double dStart, dStop, dFact = 0.0000000000001;
         BOOL fOK = TRUE;

         if( pTag->UsrAscend )
         {
            dStart = 0.0;
            dStop = 1.0;
         }
         else
         {
            dStart = 1.0;
            dStop = 0.0;
         }

         pKey = pTag->UsrAscend ? pTag->topScopeKey : pTag->bottomScopeKey;
         if( pKey )
         {
            hb_cdxTagKeyFind( pTag, pKey );
            if( pTag->CurKey->rec == 0 || pTag->TagEOF || ! hb_cdxBottomScope( pTag ) )
               fOK = FALSE;
            else
               dStart = hb_cdxCountRelKeyPos( pTag->RootPage );
         }
         pKey = pTag->UsrAscend ? pTag->bottomScopeKey : pTag->topScopeKey;
         if( pKey && fOK )
         {
            hb_cdxTagKeyFind( pTag, pKey );
            if( pTag->CurKey->rec == 0 || pTag->TagBOF || ! hb_cdxTopScope( pTag ) )
               fOK = FALSE;
            else
               dStop = hb_cdxCountRelKeyPos( pTag->RootPage );
         }
         if( fOK )
         {
            if( !pTag->UsrAscend )
            {
               double dTmp = dStart;
               dStart = dStop;
               dStop = dTmp;
               dPos = 1.0 - dPos;
            }
            if( dStart >= dStop - dFact )
            {
               fTop = TRUE;
            }
            else
            {
               dPos = dPos * ( dStop - dStart ) + dStart;
               pTag->fRePos = FALSE;
               hb_cdxTagOpen( pTag );
               pTag->TagBOF = pTag->TagEOF = FALSE;
               if( !hb_cdxGoToRelKeyPos( pTag->RootPage, dPos ) )
               {
                  fTop = TRUE;
               }
               else
               {
                  hb_cdxSetCurKey( pTag->RootPage );
                  if( !hb_cdxTopScope( pTag ) )
                     fTop = TRUE;
                  else if( !hb_cdxBottomScope( pTag ) )
                     fForward = FALSE;
               }
            }
         }
      }
      if( !fOK )
      {
         SELF_GOTO( ( AREAP ) pArea, 0 );
      }
      else
      {
         if( fForward )
         {
            if( fTop )
               hb_cdxTagGoTop( pTag );
            while( !pTag->TagEOF )
            {
               if ( hb_cdxCheckRecordFilter( pArea, pTag->CurKey->rec ) )
                  break;
               hb_cdxTagSkipNext( pTag );
            }
            if( pTag->TagEOF && !fTop )
               fForward = FALSE;
         }
         if( !fForward )
         {
            hb_cdxTagGoBottom( pTag );
            while( !pTag->TagBOF )
            {
               if ( hb_cdxCheckRecordFilter( pArea, pTag->CurKey->rec ) )
                  break;
               hb_cdxTagSkipPrev( pTag );
            }
            if( pTag->TagBOF )
            {
               pTag->CurKey->rec = 0;
            }
         }
         SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );
      }
      hb_cdxIndexUnLockRead( pTag->pIndex );
   }
}

/*
 * DBOI_FINDREC find a specific record in the tag - it's useful for
 * custom indexes when the same record can be stored more then once
 * or when the used index key is unknown
 */
static BOOL hb_cdxDBOIFindRec( CDXAREAP pArea, LPCDXTAG pTag, ULONG ulRecNo, BOOL fCont )
{
   BOOL fFound = FALSE;

   if ( pTag && ulRecNo )
   {
      if( pArea->lpdbPendingRel && pArea->lpdbPendingRel->isScoped )
         SELF_FORCEREL( ( AREAP ) pArea );

      hb_cdxIndexLockRead( pTag->pIndex );
      hb_cdxTagRefreshScope( pTag );
      if ( fCont )
      {
         if ( ! hb_cdxCurKeyRefresh( pArea, pTag ) )
            ulRecNo = 0;
         else
            hb_cdxTagSkipNext( pTag );
      }
      else
      {
         hb_cdxTagGoTop( pTag );
      }
      if ( ulRecNo )
      {
         while ( !pTag->TagBOF && !pTag->TagEOF && hb_cdxBottomScope( pTag ) )
         {
            if ( pTag->CurKey->rec == ulRecNo )
            {
               fFound = TRUE;
               break;
            }
            hb_cdxTagSkipNext( pTag );
         }
      }
      hb_cdxIndexUnLockRead( pTag->pIndex );
   }
   SELF_GOTO( ( AREAP ) pArea, fFound ? ulRecNo : 0 );
   return fFound;
}

static void hb_cdxClearLogPosInfo( CDXAREAP pArea )
{
   LPCDXINDEX pIndex = pArea->lpIndexes;
   LPCDXTAG pTag;

   while ( pIndex )
   {
      pTag = pIndex->TagList;
      while ( pTag )
      {
         pTag->curKeyState &= ~( CDX_CURKEY_LOGPOS | CDX_CURKEY_LOGCNT );
         pTag = pTag->pNext;
      }
      pIndex = pIndex->pNext;
   }
}

/*
 * -- DBFCDX METHODS --
 */

/* ( DBENTRYP_BP )    hb_cdxBof     : NULL */
/* ( DBENTRYP_BP )    hb_cdxEof     : NULL */
/* ( DBENTRYP_BP )    hb_cdxFound   : NULL */

/* ( DBENTRYP_V )     hb_cdxGoBottom */
static ERRCODE hb_cdxGoBottom( CDXAREAP pArea )
{
   LPCDXTAG pTag;
   ERRCODE retval;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxGoBottom(%p)", pArea));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   pTag = hb_cdxGetActiveTag( pArea );
   if ( ! pTag )
      return SUPER_GOBOTTOM( ( AREAP ) pArea );

   if( pArea->lpdbPendingRel && pArea->lpdbPendingRel->isScoped )
      SELF_FORCEREL( ( AREAP ) pArea );

   hb_cdxIndexLockRead( pTag->pIndex );
   hb_cdxTagRefreshScope( pTag );

   hb_cdxTagGoBottom( pTag );

   pArea->fTop = FALSE;
   pArea->fBottom = TRUE;

   retval = SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );

   if ( retval != FAILURE && pArea->fPositioned )
   {
      retval = SELF_SKIPFILTER( ( AREAP ) pArea, -1 );

      if ( pArea->fPositioned && CURKEY_LOGCNT( pTag ) )
      {
         pTag->logKeyPos = pTag->logKeyCount;
         CURKEY_SETLOGPOS( pTag );
      }
   }
   hb_cdxIndexUnLockRead( pTag->pIndex );

   return retval;
}

/* ( DBENTRYP_UL )    hb_cdxGoTo    : NULL */
/* ( DBENTRYP_I )     hb_cdxGoToId  : NULL */

/* ( DBENTRYP_V )     hb_cdxGoTop */
static ERRCODE hb_cdxGoTop( CDXAREAP pArea )
{
   LPCDXTAG pTag;
   ERRCODE retval;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxGoTop(%p)", pArea));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   pTag = hb_cdxGetActiveTag( pArea );
   if ( ! pTag )
      return SUPER_GOTOP( ( AREAP ) pArea );

   if( pArea->lpdbPendingRel && pArea->lpdbPendingRel->isScoped )
      SELF_FORCEREL( ( AREAP ) pArea );

   hb_cdxIndexLockRead( pTag->pIndex );
   hb_cdxTagRefreshScope( pTag );

   hb_cdxTagGoTop( pTag );

   pArea->fTop = TRUE;
   pArea->fBottom = FALSE;

   retval = SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );

   if ( retval != FAILURE && pArea->fPositioned )
      retval = SELF_SKIPFILTER( ( AREAP ) pArea, 1 );

   if ( retval != FAILURE && pArea->fPositioned )
   {
      pTag->logKeyPos = 1;
      CURKEY_SETLOGPOS( pTag );
   }

   hb_cdxIndexUnLockRead( pTag->pIndex );
   return retval;
}

/* ( DBENTRYP_BIB )   hb_cdxSeek */
static ERRCODE hb_cdxSeek( CDXAREAP pArea, BOOL fSoftSeek, PHB_ITEM pKeyItm, BOOL fFindLast )
{
   LPCDXTAG pTag;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxSeek(%p, %d, %p, %d)", pArea, fSoftSeek, pKeyItm, fFindLast));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   pTag = hb_cdxGetActiveTag( pArea );

   if ( ! pTag )
   {
      hb_cdxErrorRT( pArea, EG_NOORDER, EDBF_NOTINDEXED, NULL, 0, EF_CANDEFAULT );
      return FAILURE;
   }
   else
   {
      LPCDXKEY pKey;
      ERRCODE retval = SUCCESS;
      BOOL  fEOF = FALSE, fLast;
      ULONG ulRec;

      if( pArea->lpdbPendingRel && pArea->lpdbPendingRel->isScoped )
         SELF_FORCEREL( ( AREAP ) pArea );

      pArea->fTop = pArea->fBottom = FALSE;
      pArea->fEof = FALSE;

      if ( pTag->UsrUnique )
         fLast = !pTag->UsrAscend;
      else
         fLast = pTag->UsrAscend ? fFindLast : !fFindLast;

      /* TODO: runtime error if valtype(pKeyItm) != pTag->Type */
      pKey = hb_cdxKeyPutItem( NULL, pKeyItm, fLast ? CDX_MAX_REC_NUM : CDX_IGNORE_REC_NUM, pTag, TRUE, FALSE );

      hb_cdxIndexLockRead( pTag->pIndex );
      hb_cdxTagRefreshScope( pTag );
      ulRec = hb_cdxTagKeyFind( pTag, pKey );
      if ( ( ulRec == 0 && ! fSoftSeek ) || pTag->TagEOF )
         fEOF = TRUE;
      else /* if ( fSoftSeek ) */
      {
         if ( ! hb_cdxBottomScope( pTag ) )
            fEOF = TRUE;
         else if ( ! hb_cdxTopScope( pTag ) )
         {
            hb_cdxTagGoTop( pTag );
            if ( pTag->CurKey->rec == 0 )
               fEOF = TRUE;
         }
      }
      hb_cdxIndexUnLockRead( pTag->pIndex );
      if ( !fEOF )
      {
         retval = SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );
         if ( retval != FAILURE && pArea->fPositioned )
         {
            retval = SELF_SKIPFILTER( ( AREAP ) pArea, fFindLast ? -1 : 1 );
            if ( retval != FAILURE && ulRec && pArea->fPositioned )
            {
               pArea->fFound = ( ulRec == pArea->ulRecNo ||
                        hb_cdxValCompare( pTag, pKey->val, pKey->len,
                           pTag->CurKey->val, pTag->CurKey->len, FALSE ) == 0 );
               if ( ! pArea->fFound && ! fSoftSeek )
                  fEOF = TRUE;
            }
         }
      }
      if ( retval != FAILURE &&
           ( fEOF || ! hb_cdxTopScope( pTag ) ||
                     ! hb_cdxBottomScope( pTag ) ) )
      {
         retval = SELF_GOTO( ( AREAP ) pArea, 0 );
      }
      pArea->fBof = FALSE;
      hb_cdxKeyFree( pKey );
      return retval;
   }
}

/* ( DBENTRYP_L )     hb_cdxSkip        : NULL */
static ERRCODE hb_cdxSkip( CDXAREAP pArea, LONG lToSkip )
{
   LPCDXTAG pTag;
   ULONG ulPos, ulRec;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxSkip(%p, %ld)", pArea, lToSkip));

   if( pArea->lpdbPendingRel )
      SELF_FORCEREL( ( AREAP ) pArea );

   pTag = lToSkip == 0 ? NULL : hb_cdxGetActiveTag( pArea );
   if( pTag && pArea->fPositioned && CURKEY_LOGPOS( pTag ) )
   {
      ulPos = pTag->logKeyPos;
      ulRec = pTag->logKeyRec;
   }
   else
   {
      ulPos = ulRec = 0;
   }

   if ( SUPER_SKIP( ( AREAP ) pArea, lToSkip ) == FAILURE )
      return FAILURE;

   if ( pTag )
   {
      if ( ulPos && ( pTag->logKeyPos != ulPos || pTag->logKeyRec != ulRec ||
           ( pTag->curKeyState & CDX_CURKEY_LOGPOS ) == 0 ) )
      {
         ulPos = 0;
      }

      if ( lToSkip > 0 )
      {
         if ( pArea->fEof )
         {
            if ( lToSkip == 1 && ulPos && !CURKEY_LOGCNT( pTag ) )
            {
               pTag->logKeyCount = ulPos;
               pTag->curKeyState |= CDX_CURKEY_LOGCNT;
            }
         }
         else if ( ulPos )
         {
            pTag->logKeyPos += lToSkip;
            pTag->logKeyRec = pArea->ulRecNo;
         }
      }
      else if ( pArea->fBof )
      {
         if ( pArea->fPositioned )
         {
            pTag->logKeyPos = 1;
            CURKEY_SETLOGPOS( pTag );
         }
      }
      else if ( ulPos )
      {
         pTag->logKeyPos += lToSkip;
         pTag->logKeyRec = pArea->ulRecNo;
      }
   }
   return SUCCESS;
}

/* ( DBENTRYP_L )     hb_cdxSkipFilter  : NULL */

/* ( DBENTRYP_L )     hb_cdxSkipRaw */
static ERRCODE hb_cdxSkipRaw( CDXAREAP pArea, LONG lToSkip )
{
   LPCDXTAG pTag;
   ERRCODE retval;
   BOOL fOut = FALSE, fForward;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxSkipRaw(%p, %ld)", pArea, lToSkip));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   pTag = hb_cdxGetActiveTag( pArea );

   if ( ! pTag || lToSkip == 0 )
      return SUPER_SKIPRAW( ( AREAP ) pArea, lToSkip );

   if( pArea->lpdbPendingRel )
      SELF_FORCEREL( ( AREAP ) pArea );

   fForward = ( lToSkip > 0 );

   hb_cdxIndexLockRead( pTag->pIndex );
   hb_cdxTagRefreshScope( pTag );
   if ( ! hb_cdxCurKeyRefresh( pArea, pTag ) )
   {
      if ( fForward )
      {
         if ( pTag->TagEOF || !hb_cdxBottomScope( pTag ) )
            fOut = TRUE;
         else if ( pTag->UsrAscend && hb_cdxTopScope( pTag ) )
            lToSkip--;
      }
      else if ( pArea->fPositioned )
      {
         if ( pTag->TagEOF || !hb_cdxTopScope( pTag ) )
            fOut = TRUE;
         else if ( !pTag->UsrAscend && hb_cdxBottomScope( pTag ) )
            lToSkip++;
      }
   }
   if ( fForward )
   {
      if ( !fOut )
      {
         while ( lToSkip-- > 0 )
         {
            hb_cdxTagSkipNext( pTag );
            if ( pTag->TagEOF )
            {
               fOut = TRUE;
               break;
            }
         }
      }
      retval = SELF_GOTO( ( AREAP ) pArea, ( pTag->TagEOF || fOut )
                                           ? 0 : pTag->CurKey->rec );
   }
   else /* if ( lToSkip < 0 ) */
   {
      if ( fOut )
         hb_cdxTagGoTop( pTag );
      else
      {
         while ( lToSkip++ < 0 )
         {
            hb_cdxTagSkipPrev( pTag );
            if ( pTag->TagBOF )
            {
               fOut = TRUE;
               break;
            }
         }
      }
      retval = SELF_GOTO( ( AREAP ) pArea, pTag->CurKey->rec );
      pArea->fBof = fOut;
   }
   hb_cdxIndexUnLockRead( pTag->pIndex );
   /* Update Bof and Eof flags */
   /*
   if( fForward )
      pArea->fBof = FALSE;
   else
      pArea->fEof = FALSE;
   */
   return retval;
}

/* ( DBENTRYP_VF )    hb_cdxAddField        : NULL */
/* ( DBENTRYP_B )     hb_cdxAppend          : NULL */
/* ( DBENTRYP_I )     hb_cdxCreateFields    : NULL */
/* ( DBENTRYP_V )     hb_cdxDeleteRec       : NULL */
/* ( DBENTRYP_BP )    hb_cdxDeleted         : NULL */
/* ( DBENTRYP_SP )    hb_cdxFieldCount      : NULL */
/* ( DBENTRYP_VF )    hb_cdxFieldDisplay    : NULL */
/* ( DBENTRYP_SSI )   hb_cdxFieldInfo       : NULL */
/* ( DBENTRYP_SVP )   hb_cdxFieldName       : NULL */

/* ( DBENTRYP_V )     hb_cdxFlush           : NULL */
/*
 * Flush _system_ buffers to disk
 */
static ERRCODE hb_cdxFlush( CDXAREAP pArea )
{
   LPCDXINDEX pIndex;
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxFlush(%p)", pArea));

   if( SELF_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   uiError = SUPER_FLUSH( ( AREAP ) pArea );

   if ( hb_setGetHardCommit() )
   {
      pIndex = pArea->lpIndexes;
      while ( pIndex )
      {
         if ( pIndex->hFile != FS_ERROR && pIndex->fFlush )
         {
            hb_fsCommit( pIndex->hFile );
            pIndex->fFlush = FALSE;
         }
         pIndex = pIndex->pNext;
      }
   }

   return uiError;
}

/* ( DBENTRYP_PP )    hb_cdxGetRec          : NULL */
/* ( DBENTRYP_SI )    hb_cdxGetValue        : NULL */
/* ( DBENTRYP_SVL )   hb_cdxGetVarLen       : NULL */

/* ( DBENTRYP_V )     hb_cdxGoCold */
/*
 * Perform a write of WorkArea memory to the data store.
 */
static ERRCODE hb_cdxGoCold( CDXAREAP pArea )
{
   BOOL fRecordChanged = pArea->fRecordChanged;
   BOOL fAppend = pArea->fAppend;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxGoCold(%p)", pArea));

   if ( SUPER_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if ( ( fRecordChanged || pArea->fCdxAppend ) && pArea->lpIndexes )
   {
      LPCDXTAG pTag = pArea->lpIndexes->TagList;
      LPCDXKEY pKey = NULL;
      BOOL fAdd, fDel, fLck = FALSE;
      LPDBRELINFO lpdbPendingRel;

      if ( pArea->fShared )
      {
         if ( fAppend )
         {
            if ( pArea->fCdxAppend )
               hb_cdxErrInternal( "hb_cdxGoCold: multiple appending without GOCOLD." );
            pArea->fCdxAppend = TRUE;
            return SUCCESS;
         }
         else
         {
            fAppend = pArea->fCdxAppend;
            pArea->fCdxAppend = FALSE;
         }
      }

      /* The pending relation may move the record pointer so we should
         disable them for KEY/FOR evaluation */
      lpdbPendingRel = pArea->lpdbPendingRel;
      pArea->lpdbPendingRel = NULL;

      /* TODO:
       * There is possible race condition here but not very dangerous.
       * To avoid it we should Lock all index file before SUPER_GOCOLD
       * but it makes other problem if two stations open the database index
       * files in a different order then they can block each other.
       * Without changes in locking scheme we can do only one thing which
       * is enough if there is only one index file: lock first index only
       * before SUPER_GOCOLD
       * Druzus, 05 Oct 2003 10:27:52 CEST
       */

      while ( pTag )
      {
         if ( !pTag->Custom )
         {
            pKey = hb_cdxKeyEval( pKey, pTag );

            if ( pTag->pForItem != NULL )
               fAdd = hb_cdxEvalCond( pArea, pTag->pForItem, TRUE );
            else
               fAdd = TRUE;

            if ( fAppend )
               fDel = FALSE;
            else
            {
               if ( hb_cdxValCompare( pTag, pKey->val, pKey->len,
                        pTag->HotKey->val, pTag->HotKey->len, TRUE ) == 0 )
               {
                  fDel = !fAdd &&  pTag->HotFor;
                  fAdd =  fAdd && !pTag->HotFor;
               }
               else
               {
                  fDel = pTag->HotFor;
               }
            }
            if ( fDel || fAdd )
            {
               if ( !fLck )
               {
                  hb_cdxIndexLockWrite( pTag->pIndex );
                  fLck = TRUE;
               }
               if ( fDel )
                  hb_cdxTagKeyDel( pTag, pTag->HotKey );
               if ( fAdd )
                  hb_cdxTagKeyAdd( pTag, pKey );
            }
#if 0
            if ( pTag->HotKey )
            {
               hb_cdxKeyFree( pTag->HotKey );
               pTag->HotKey = NULL;
            }
#endif
         }
         if ( pTag->pNext )
            pTag = pTag->pNext;
         else
         {
            if ( fLck )
            {
               hb_cdxIndexUnLockWrite( pTag->pIndex );
               fLck = FALSE;
            }
            if ( pTag->pIndex->pNext )
               pTag = pTag->pIndex->pNext->TagList;
            else
               pTag = NULL;
         }
      }

      if ( pKey )
         hb_cdxKeyFree( pKey );

      /* Restore disabled pending relation */
      pArea->lpdbPendingRel = lpdbPendingRel;
   }

   return SUCCESS;
}

/* ( DBENTRYP_V )     hb_cdxGoHot */
/*
 * Mark the WorkArea data buffer as hot.
 */
static ERRCODE hb_cdxGoHot( CDXAREAP pArea )
{

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxGoHot(%p)", pArea));

   if ( pArea->fRecordChanged )
      hb_cdxErrInternal( "hb_cdxGoHot: multiple marking buffer as hot." );

   if ( SUPER_GOHOT( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if ( pArea->lpIndexes && !pArea->fCdxAppend )
   {
      LPCDXTAG pTag = pArea->lpIndexes->TagList;
      while ( pTag )
      {
         if ( !pTag->Custom )
         {
            pTag->HotKey = hb_cdxKeyEval( pTag->HotKey, pTag );
            pTag->HotFor = pTag->pForItem == NULL || hb_cdxEvalCond( pArea, pTag->pForItem, TRUE );
         }

         if ( pTag->pNext )
            pTag = pTag->pNext;
         else
         {
            if ( pTag->pIndex->pNext )
               pTag = pTag->pIndex->pNext->TagList;
            else
               pTag = NULL;
         }
      }
   }
   return SUCCESS;
}

/* ( DBENTRYP_P )     hb_cdxPutRec          : NULL */
/* ( DBENTRYP_SI )    hb_cdxPutValue        : NULL */
/* ( DBENTRYP_V )     hb_cdxRecall          : NULL */
/* ( DBENTRYP_ULP )   hb_cdxRecCount        : NULL */
/* ( DBENTRYP_ISI )   hb_cdxRecInfo         : NULL */
/* ( DBENTRYP_ULP )   hb_cdxRecNo           : NULL */
/* ( DBENTRYP_I )     hb_cdxRecId           : NULL */
/* ( DBENTRYP_S )     hb_cdxSetFieldExtent  : NULL */
/* ( DBENTRYP_P )     hb_cdxAlias           : NULL */

/* ( DBENTRYP_V )     hb_cdxClose */
/*
 * Close the table in the WorkArea.
 */
static ERRCODE hb_cdxClose( CDXAREAP pArea )
{
   ERRCODE errCode;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxClose(%p)", pArea));

   if ( SELF_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   errCode = SUPER_CLOSE( ( AREAP ) pArea );

   if( errCode == SUCCESS )
   {
      if ( pArea->pSort )
      {
         hb_cdxSortFree( pArea->pSort );
         pArea->pSort = NULL;
      }

      hb_cdxOrdListClear( pArea, TRUE, NULL );
      if ( pArea->bCdxSortTab )
      {
         hb_xfree( pArea->bCdxSortTab );
         pArea->bCdxSortTab = NULL;
      }
#ifdef HB_CDX_DBGTIME
      printf( "\r\ncdxTimeIntBld=%f, cdxTimeExtBld=%f, cdxTimeBld=%f\r\n"
              "cdxTimeGetKey=%f, cdxTimeFreeKey=%f\r\n"
              "cdxTimeExtBlc=%f, cdxTimeIntBlc=%f\r\n"
              "cdxTimeIdxBld=%f\r\n"
              "cdxTimeTotal=%f\r\n",
              (double) cdxTimeIntBld / 1000000, (double) cdxTimeExtBld / 1000000,
              (double) ( cdxTimeIntBld + cdxTimeExtBld ) / 1000000,
              (double) cdxTimeGetKey / 1000000, (double) cdxTimeFreeKey / 1000000,
              (double) cdxTimeIntBlc / 1000000, (double) cdxTimeExtBlc / 1000000,
              (double) cdxTimeIdxBld / 1000000,
              (double) ( cdxTimeIntBld + cdxTimeExtBld + cdxTimeIdxBld +
                         cdxTimeGetKey + cdxTimeFreeKey +
                         cdxTimeExtBlc + cdxTimeIntBlc ) / 1000000 );
      fflush(stdout);
      cdxTimeIntBld = cdxTimeExtBld = 0;
#endif
#ifdef HB_CDX_DBGUPDT
      printf( "\r\n#reads=%ld, #writes=%ld, stacksize=%d\r\n", cdxReadNO, cdxWriteNO, cdxStackSize );
      fflush(stdout);
      cdxReadNO = cdxWriteNO = 0;
#endif
   }

   return errCode;
}

/* ( DBENTRYP_VP )    hb_cdxCreate          : NULL */
/* ( DBENTRYP_SI )    hb_cdxInfo            : NULL */
/* ( DBENTRYP_V )     hb_cdxNewArea         : NULL */

/* ( DBENTRYP_VP )    hb_cdxOpen */
/*
 * Open a data store in the WorkArea.
 */
static ERRCODE hb_cdxOpen( CDXAREAP pArea, LPDBOPENINFO pOpenInfo )
{
   ERRCODE errCode = SUCCESS;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxOpen(%p, %p)", pArea, pOpenInfo));

   if ( !pArea->bLockType )
   {
      PHB_ITEM pItem = hb_itemNew( NULL );
      if( SELF_INFO( ( AREAP ) pArea, DBI_LOCKSCHEME, pItem ) != SUCCESS )
      {
         hb_itemRelease( pItem );
         return FAILURE;
      }
      pArea->bLockType = hb_itemGetNI( pItem );
      hb_itemRelease( pItem );
      if( pArea->bLockType == 0 )
      {
         pArea->bLockType = DB_DBFLOCK_VFP;
      }
   }
   if ( SUPER_OPEN( ( AREAP ) pArea, pOpenInfo ) == FAILURE )
   {
      return FAILURE;
   }

   /* open (production) structural index */
   if( CDXAREA_DATA( pArea )->fStrictStruct ? pArea->fHasTags : hb_setGetAutOpen() )
   {
      char szFileName[ _POSIX_PATH_MAX + 1 ];

      pArea->fHasTags = FALSE;
      hb_cdxCreateFName( pArea, NULL, NULL, szFileName, NULL );
      if ( hb_spFile( ( BYTE * ) szFileName, NULL ) ||
           CDXAREA_DATA( pArea )->fStrictStruct )
      {
         DBORDERINFO pOrderInfo;

         pOrderInfo.itmResult = hb_itemPutNI( NULL, 0 );
         pOrderInfo.atomBagName = hb_itemPutC( NULL, szFileName );
         pOrderInfo.itmNewVal = NULL;
         pOrderInfo.itmOrder  = NULL;
         errCode = SELF_ORDLSTADD( ( AREAP ) pArea, &pOrderInfo );
         if( errCode == SUCCESS )
         {
            pOrderInfo.itmOrder  = hb_itemPutNI( NULL, hb_setGetAutOrder() );
            errCode = SELF_ORDLSTFOCUS( ( AREAP ) pArea, &pOrderInfo );
            hb_itemRelease( pOrderInfo.itmOrder );
            if( errCode == SUCCESS )
               errCode = SELF_GOTOP( ( AREAP ) pArea );
         }
         hb_itemRelease( pOrderInfo.atomBagName );
         hb_itemRelease( pOrderInfo.itmResult );
      }
   }
   else
   {
      pArea->fHasTags = FALSE;
   }

   return errCode;
}

/* ( DBENTRYP_V )     hb_cdxRelease         : NULL */

/* ( DBENTRYP_SP )    hb_cdxStructSize */
/*
 * Retrieve the size of the WorkArea structure.
 */
static ERRCODE hb_cdxStructSize( CDXAREAP pArea, USHORT * uiSize )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_cdxStrucSize(%p, %p)", pArea, uiSize));
   HB_SYMBOL_UNUSED( pArea );

   * uiSize = sizeof( CDXAREA );
   return SUCCESS;
}

/* ( DBENTRYP_P )     hb_cdxSysName         : NULL */

/* ( DBENTRYP_VEI )   hb_cdxEval            : NULL */

/* ( DBENTRYP_V )     hb_cdxPack */
static ERRCODE hb_cdxPack( CDXAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_cdxPack(%p)", pArea ));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if ( SUPER_PACK( ( AREAP ) pArea ) == SUCCESS )
   {
      return SELF_ORDLSTREBUILD( ( AREAP ) pArea );
   }
   else
      return FAILURE;
}

/* ( DBENTRYP_LSP )   hb_cdxPackRec         : NULL */
/* ( DBENTRYP_VS )    hb_cdxSort            : NULL */
/* ( DBENTRYP_VT )    hb_cdxTrans           : NULL */
/* ( DBENTRYP_VT )    hb_cdxTransRec        : NULL */

/* ( DBENTRYP_V )     hb_cdxZap */
static ERRCODE hb_cdxZap ( CDXAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("nb_cdxZap(%p)", pArea ));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if ( SUPER_ZAP( ( AREAP ) pArea ) == SUCCESS )
   {
      return SELF_ORDLSTREBUILD( ( AREAP ) pArea );
   }
   else
      return FAILURE;
}

/* ( DBENTRYP_VR )    hb_cdxChildEnd        : NULL */
/* ( DBENTRYP_VR )    hb_cdxChildStart      : NULL */
/* ( DBENTRYP_VR )    hb_cdxChildSync       : NULL */
/* ( DBENTRYP_V )     hb_cdxSyncChildren    : NULL */
/* ( DBENTRYP_V )     hb_cdxClearRel        : NULL */
/* ( DBENTRYP_V )     hb_cdxForceRel        : NULL */
/* ( DBENTRYP_SVP )   hb_cdxRelArea         : NULL */
/* ( DBENTRYP_VR )    hb_cdxRelEval         : NULL */
/* ( DBENTRYP_SI )    hb_cdxRelText         : NULL */
/* ( DBENTRYP_VR )    hb_cdxSetRel          : NULL */

/* ( DBENTRYP_OI )    hb_cdxOrderListAdd */
static ERRCODE hb_cdxOrderListAdd( CDXAREAP pArea, LPDBORDERINFO pOrderInfo )
{
   USHORT uiFlags;
   HB_FHANDLE hFile;
   char szBaseName[ CDX_MAXTAGNAMELEN + 1 ];
   char szFileName[ _POSIX_PATH_MAX + 1 ];
   LPCDXINDEX pIndex, * pIndexPtr;
   BOOL fProd, bRetry;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxOrderListAdd(%p, %p)", pArea, pOrderInfo));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if ( hb_itemGetCLen( pOrderInfo->atomBagName ) == 0 )
      return FAILURE;

   hb_cdxCreateFName( pArea, hb_itemGetCPtr( pOrderInfo->atomBagName ),
                      &fProd, szFileName, szBaseName );

/*
   if ( ! szBaseName[0] )
      return FAILURE;
*/
   pIndex = hb_cdxFindBag( pArea, szFileName );

   if ( pIndex )
   {
       /* index already open, do nothing */
      if ( ! pArea->uiTag )
      {
         pArea->uiTag = hb_cdxGetTagNumber( pArea, pIndex->TagList );
         SELF_GOTOP( ( AREAP ) pArea );
      }
      return SUCCESS;
   }

   uiFlags = ( pArea->fReadonly ? FO_READ : FO_READWRITE ) |
             ( pArea->fShared ? FO_DENYNONE : FO_EXCLUSIVE );
   do
   {
      hFile = hb_fsExtOpen( ( BYTE * ) szFileName, NULL, uiFlags |
                            FXO_DEFAULTS | FXO_SHARELOCK | FXO_COPYNAME,
                            NULL, NULL );
      if( hFile == FS_ERROR )
         bRetry = ( hb_cdxErrorRT( pArea, EG_OPEN, EDBF_OPEN_INDEX, szFileName,
                                   hb_fsError(), EF_CANRETRY | EF_CANDEFAULT ) == E_RETRY );
      else
      {
         if( hb_fsSeekLarge( hFile, 0, FS_END ) <= ( HB_FOFFSET ) sizeof( CDXTAGHEADER ) )
         {
            hb_fsClose( hFile );
            hFile = FS_ERROR;
            hb_cdxErrorRT( pArea, EG_CORRUPTION, EDBF_CORRUPT,
                           szFileName, hb_fsError(), EF_CANDEFAULT );
         }
         bRetry = FALSE;
      }

   } while ( bRetry );

   if ( hFile == FS_ERROR )
   {
      return FAILURE;
   }

   pIndex = hb_cdxIndexNew( pArea );
   pIndex->hFile      = hFile;
   pIndex->fShared    = pArea->fShared;
   pIndex->fReadonly  = pArea->fReadonly;
   pIndex->szFileName = hb_strdup( szFileName );

   pIndexPtr = &pArea->lpIndexes;
   while ( *pIndexPtr != NULL )
      pIndexPtr = &( *pIndexPtr )->pNext;
   *pIndexPtr = pIndex;

   if ( ! hb_cdxIndexLoad( pIndex, szBaseName ) )
   {
      /* index file is corrupted */
      *pIndexPtr = NULL;
      hb_cdxIndexFree( pIndex );
      hb_cdxErrorRT( pArea, EG_CORRUPTION, EDBF_CORRUPT,
                     szFileName, hb_fsError(), EF_CANDEFAULT );
      return FAILURE;
   }

   if( fProd )
      pArea->fHasTags = TRUE;

   /* dbfcdx specific: If there was no controlling order, set this one.
    * This is the behaviour of Clipper's dbfcdx, although
    * Clipper doc says a different rule
    */
   if ( ! pArea->uiTag )
   {
      pArea->uiTag = hb_cdxGetTagNumber( pArea, pIndex->TagList );
      SELF_GOTOP( ( AREAP ) pArea );
   }
   return SUCCESS;
}

/* ( DBENTRYP_V )     hb_cdxOrderListClear */
/*
 * Clear the current order list.
 */
static ERRCODE hb_cdxOrderListClear( CDXAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_cdxOrderListClear(%p)", pArea));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   hb_cdxOrdListClear( pArea, !( CDXAREA_DATA( pArea )->fStrictStruct ?
                       pArea->fHasTags : hb_setGetAutOpen() ), NULL );
   pArea->uiTag = 0;

   return SUCCESS;
}

/* ( DBENTRYP_OI )    hb_cdxOrderListDelete */
static ERRCODE hb_cdxOrderListDelete( CDXAREAP pArea, LPDBORDERINFO pOrderInfo )
{
   char szTagName[ CDX_MAXTAGNAMELEN + 1 ];
   char szFileName[ _POSIX_PATH_MAX + 1 ];
   LPCDXINDEX pIndex, * pIndexPtr;
   BOOL fProd;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxOrderListDelete(%p, %p)", pArea, pOrderInfo));

   if( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   hb_cdxCreateFName( pArea, hb_itemGetCPtr( pOrderInfo->atomBagName ), &fProd,
                      szFileName, szTagName );

   if( fProd && ( CDXAREA_DATA( pArea )->fStrictStruct ?
                  pArea->fHasTags : hb_setGetAutOpen() ) )
      pIndex = NULL;
   else
      pIndex = hb_cdxFindBag( pArea, szFileName );

   if( pIndex )
   {
      LPCDXTAG pTag = hb_cdxGetActiveTag( pArea );
      if( pTag && pTag->pIndex == pIndex )
         pArea->uiTag = 0;
      pIndexPtr = &pArea->lpIndexes;
      while( *pIndexPtr )
      {
         if( pIndex == *pIndexPtr )
         {
            *pIndexPtr = pIndex->pNext;
            hb_cdxIndexFree( pIndex );
            break;
         }
         pIndexPtr = &(*pIndexPtr)->pNext;
      }
   }
   return SUCCESS;
}

/* ( DBENTRYP_OI )    hb_cdxOrderListFocus */
static ERRCODE hb_cdxOrderListFocus( CDXAREAP pArea, LPDBORDERINFO pOrderInfo )
{
   LPCDXTAG pTag;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxOrderListFocus(%p, %p)", pArea, pOrderInfo));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if ( ! pArea->lpIndexes )
      return SUCCESS;

   pTag = hb_cdxGetActiveTag( pArea );
   if ( pTag )
      pOrderInfo->itmResult = hb_itemPutC( pOrderInfo->itmResult, pTag->szName );

   if ( pOrderInfo->itmOrder )
   {
      hb_cdxFindTag( pArea, pOrderInfo->itmOrder, pOrderInfo->atomBagName, &(pArea->uiTag) );
      /* TODO: RTerror if not found? */
   }

   return SUCCESS;
}

/* ( DBENTRYP_V )     hb_cdxOrderListRebuild */
static ERRCODE hb_cdxOrderListRebuild( CDXAREAP pArea )
{
   LPCDXINDEX pIndex, * pIndexPtr;
   USHORT uiPrevTag;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxPack(%p)", pArea ));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if ( pArea->fShared )
   {
      hb_cdxErrorRT( pArea, EG_SHARED, EDBF_SHARED, pArea->szDataFileName, 0, 0 );
      return FAILURE;
   }
   if ( pArea->fReadonly )
   {
      hb_cdxErrorRT( pArea, EG_READONLY, EDBF_READONLY, pArea->szDataFileName, 0, 0 );
      return FAILURE;
   }

   if ( ! pArea->lpIndexes )
      return SUCCESS;

   uiPrevTag = pArea->uiTag;
   pArea->uiTag = 0;

   pIndex = pArea->lpIndexes;
   pArea->lpIndexes = NULL;
   pIndexPtr = &pArea->lpIndexes;
   while ( pIndex )
   {
      (*pIndexPtr) = pIndex;
      pIndex = pIndex->pNext;
      (*pIndexPtr)->pNext = NULL;
      hb_cdxIndexReindex( *pIndexPtr );
      pIndexPtr = &(*pIndexPtr)->pNext;
   }

   pArea->uiTag = uiPrevTag;
   /* Clear pArea->lpdbOrdCondInfo */
   SELF_ORDSETCOND( ( AREAP ) pArea, NULL );

   return SELF_GOTOP( ( AREAP ) pArea );
}

/* ( DBENTRYP_VOI )   hb_cdxOrderCondition  : NULL */

/* ( DBENTRYP_VOC )   hb_cdxOrderCreate */
/*
 * create new order
 */
static ERRCODE hb_cdxOrderCreate( CDXAREAP pArea, LPDBORDERCREATEINFO pOrderInfo )
{
   ULONG ulRecNo;
   BOOL fNewFile, fOpenedIndex, fProd, fAscend = TRUE, fCustom = FALSE,
        fTemporary = FALSE, fExclusive = FALSE;
   PHB_ITEM pKeyExp, pForExp = NULL, pResult;
   char szCpndTagName[ CDX_MAXTAGNAMELEN + 1 ], szTagName[ CDX_MAXTAGNAMELEN + 1 ];
   char szFileName[ _POSIX_PATH_MAX + 1 ], szTempFile[ _POSIX_PATH_MAX + 1 ];
   char *szFor = NULL;
   LPCDXINDEX pIndex;
   LPCDXTAG pTag;
   USHORT uiLen;
   BYTE bType;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxOrderCreate(%p, %p)", pArea, pOrderInfo));

   if( SELF_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if( pArea->lpdbPendingRel )
      SELF_FORCEREL( ( AREAP ) pArea );

   if( hb_strlentrim( hb_itemGetCPtr( pOrderInfo->abExpr ) ) +
       ( pArea->lpdbOrdCondInfo && pArea->lpdbOrdCondInfo->abFor ?
         hb_strlentrim( ( const char * ) pArea->lpdbOrdCondInfo->abFor ) : 0 ) >
       CDX_HEADEREXPLEN - 2 )
   {
      hb_cdxErrorRT( pArea, EG_DATAWIDTH, EDBF_KEYLENGTH, NULL, 0, 0 );
      return FAILURE;
   }

   if( SELF_COMPILE( (AREAP) pArea, ( BYTE * ) hb_itemGetCPtr( pOrderInfo->abExpr ) ) == FAILURE )
      return FAILURE;

   pKeyExp = pArea->valResult;
   pArea->valResult = NULL;
   /* If we have a codeblock for the expression, use it */
   if( pOrderInfo->itmCobExpr )
   {
      hb_vmDestroyBlockOrMacro( pKeyExp );
      pKeyExp = hb_itemNew( pOrderInfo->itmCobExpr );
   }

   /* Get a blank record before testing expression */
   ulRecNo = pArea->ulRecNo;
   SELF_GOTO( ( AREAP ) pArea, 0 );
   if( SELF_EVALBLOCK( ( AREAP ) pArea, pKeyExp ) == FAILURE )
   {
      hb_vmDestroyBlockOrMacro( pKeyExp );
      SELF_GOTO( ( AREAP ) pArea, ulRecNo );
      return FAILURE;
   }
   pResult = pArea->valResult;
   pArea->valResult = NULL;

   bType = hb_cdxItemType( pResult );
   switch ( bType )
   {
      case 'N':
      case 'D':
         uiLen = 8;
         break;
      case 'L':
         uiLen = 1;
         break;
      case 'C':
         uiLen = ( USHORT ) hb_itemGetCLen( pResult );
#if !(defined(HB_COMPAT_C53) && defined(HB_C52_STRICT))
         if( uiLen > CDX_MAXKEY )
            uiLen = CDX_MAXKEY;
#endif
         break;
      default:
         bType = 'U';
         uiLen = 0;
   }
   hb_itemRelease( pResult );

   /* Make sure KEY has proper type and length */
   if ( bType == 'U' || uiLen == 0 )
   {
      hb_vmDestroyBlockOrMacro( pKeyExp );
      SELF_GOTO( ( AREAP ) pArea, ulRecNo );
      hb_cdxErrorRT( pArea, bType == 'U' ? EG_DATATYPE : EG_DATAWIDTH, EDBF_INVALIDKEY, NULL, 0, 0 );
      return FAILURE;
   }
#if defined(HB_COMPAT_C53) && defined(HB_C52_STRICT)
   else if ( bType == 'C' && uiLen > CDX_MAXKEY )
   {
      if( hb_cdxErrorRT( pArea, EG_DATAWIDTH, EDBF_INVALIDKEY, NULL, 0, EF_CANDEFAULT ) == E_DEFAULT )
        uiLen = CDX_MAXKEY;
      else
      {
        hb_vmDestroyBlockOrMacro( pKeyExp );
        SELF_GOTO( ( AREAP ) pArea, ulRecNo );
        return FAILURE;
      }
   }
#endif
   if( pArea->lpdbOrdCondInfo )
   {
      fAscend = !pArea->lpdbOrdCondInfo->fDescending;
      fCustom = pArea->lpdbOrdCondInfo->fCustom;
      fTemporary = pArea->lpdbOrdCondInfo->fTemporary;
      fExclusive = pArea->lpdbOrdCondInfo->fExclusive;

      /* Check conditional expression */
      szFor = ( char * ) pArea->lpdbOrdCondInfo->abFor;
      if( szFor )
      {
         if( SELF_COMPILE( (AREAP) pArea, ( BYTE * ) szFor ) == FAILURE )
         {
            hb_vmDestroyBlockOrMacro( pKeyExp );
            SELF_GOTO( ( AREAP ) pArea, ulRecNo );
            return FAILURE;
         }
         pForExp = pArea->valResult;
         pArea->valResult = NULL;
      }
      /* If we have a codeblock for the conditional expression, use it */
      if( pArea->lpdbOrdCondInfo->itmCobFor )
      {
         if( pForExp )
            hb_vmDestroyBlockOrMacro( pForExp );
         pForExp = hb_itemNew( pArea->lpdbOrdCondInfo->itmCobFor );
      }
   }
   /* Test conditional expression */
   if ( pForExp )
   {
      BOOL fOK;

      if ( SELF_EVALBLOCK( ( AREAP ) pArea, pForExp ) == FAILURE )
      {
         hb_vmDestroyBlockOrMacro( pKeyExp );
         hb_vmDestroyBlockOrMacro( pForExp );
         SELF_GOTO( ( AREAP ) pArea, ulRecNo );
         return FAILURE;
      }
      fOK = hb_itemType( pArea->valResult ) == HB_IT_LOGICAL;
      hb_itemRelease( pArea->valResult );
      pArea->valResult = NULL;
      if ( ! fOK )
      {
         hb_vmDestroyBlockOrMacro( pKeyExp );
         hb_vmDestroyBlockOrMacro( pForExp );
         SELF_GOTO( ( AREAP ) pArea, ulRecNo );
         hb_cdxErrorRT( pArea, EG_DATATYPE, EDBF_INVALIDFOR, NULL, 0, 0 );
         return FAILURE;
      }
   }

   SELF_GOTO( ( AREAP ) pArea, ulRecNo );

   /*
    * abBagName -> cBag, atomBagName -> cTag
    * The following scheme implemented:
    * 1. abBagName == NULL   -> add the Tag to the structural index
    * 2. atomBagName == NULL -> overwrite any index file of abBagName
    * 3. add the Tag to index file
    */

   hb_cdxCreateFName( pArea, ( char * ) pOrderInfo->abBagName,
                      &fProd, szFileName, szCpndTagName );

   if ( pOrderInfo->atomBagName && pOrderInfo->atomBagName[0] )
   {
      hb_strncpyUpperTrim( szTagName, ( char * ) pOrderInfo->atomBagName, sizeof( szTagName ) - 1 );
      fNewFile = FALSE;
   }
   else
   {
      hb_strncpy( szTagName, szCpndTagName, sizeof( szTagName ) - 1 );
      fNewFile = TRUE;
   }

   if ( !pArea->lpdbOrdCondInfo ||
        ( pArea->lpdbOrdCondInfo->fAll && !pArea->lpdbOrdCondInfo->fAdditive ) )
      hb_cdxOrdListClear( pArea, !( CDXAREA_DATA( pArea )->fStrictStruct ?
                          pArea->fHasTags : hb_setGetAutOpen() ), NULL );

   pIndex = hb_cdxFindBag( pArea, szFileName );

   if ( fNewFile && pIndex != NULL )
   {
      LPCDXINDEX * pIndexPtr = &pArea->lpIndexes;
      while ( *pIndexPtr )
      {
         if ( pIndex == *pIndexPtr )
         {
            *pIndexPtr = pIndex->pNext;
            break;
         }
         pIndexPtr = &(*pIndexPtr)->pNext;
      }
      hb_cdxIndexFree( pIndex );
      pIndex = NULL;
   }
   fOpenedIndex = ( pIndex != NULL );

   if ( !fOpenedIndex )
   {
      HB_FHANDLE hFile;
      BOOL bRetry, fShared = pArea->fShared && !fTemporary && !fExclusive;

      do
      {
         if( fTemporary )
         {
            hFile = hb_fsCreateTemp( NULL, NULL, FC_NORMAL, ( BYTE * ) szTempFile );
            fNewFile = TRUE;
         }
         else
         {
            hFile = hb_fsExtOpen( ( BYTE * ) szFileName, NULL, FO_READWRITE |
                                  ( fShared ? FO_DENYNONE : FO_EXCLUSIVE ) |
                                  ( fNewFile ? FXO_TRUNCATE : FXO_APPEND ) |
                                  FXO_DEFAULTS | FXO_SHARELOCK | FXO_COPYNAME,
                                  NULL, NULL );
         }
         if( hFile == FS_ERROR )
            bRetry = ( hb_cdxErrorRT( pArea, EG_CREATE, EDBF_CREATE, szFileName,
                                      hb_fsError(), EF_CANRETRY | EF_CANDEFAULT ) == E_RETRY );
         else
         {
            bRetry = FALSE;
            if ( !fNewFile )
               fNewFile = ( hb_fsSeekLarge( hFile, 0, FS_END ) == 0 );
         }
      }
      while ( bRetry );

      if ( hFile != FS_ERROR )
      {
         pIndex = hb_cdxIndexNew( pArea );
         pIndex->hFile      = hFile;
         pIndex->fShared    = fShared;
         pIndex->fReadonly  = FALSE;
         pIndex->szFileName = hb_strdup( szFileName );
         pIndex->fDelete = fTemporary;
         if( fTemporary )
            pIndex->szRealName = hb_strdup( szTempFile );

         if ( !fNewFile )
         {
            /* index file is corrupted? */
            if ( ! hb_cdxIndexLoad( pIndex, szCpndTagName ) )
            {
               /* TODO: What should be default? */
               /*
               hb_cdxIndexFree( pIndex );
               hb_fsClose( hFile );
               hFile = FS_ERROR;
               hb_cdxErrorRT( pArea, EG_CORRUPTION, EDBF_CORRUPT,
                              szFileName, hb_fsError(), EF_CANDEFAULT );
               */
               hb_cdxIndexFreeTags( pIndex );
               fNewFile = TRUE;
            }
         }
      }

      if ( hFile == FS_ERROR )
      {
         hb_vmDestroyBlockOrMacro( pKeyExp );
         if ( pForExp != NULL )
            hb_vmDestroyBlockOrMacro( pForExp );
         return FAILURE;
      }
   }

   hb_cdxIndexLockWrite( pIndex );
   if ( !fNewFile )
   {
      /* Delete new tag if exist */
      hb_cdxIndexDelTag( pIndex, szTagName );
      fNewFile = ( pIndex->TagList == NULL );
   }

   if ( fNewFile )
   {
      hb_fsSeek( pIndex->hFile, 0, FS_SET );
      hb_fsWrite( pIndex->hFile, NULL, 0 );
      pIndex->fChanged = TRUE;
      hb_cdxIndexDropAvailPage( pIndex );
      if ( pIndex->pCompound != NULL )
         hb_cdxTagFree( pIndex->pCompound );
      pIndex->nextAvail = pIndex->freePage = 0;
      hb_cdxIndexCreateStruct( pIndex, szCpndTagName );
   }

   pTag = hb_cdxIndexAddTag( pIndex, szTagName, hb_itemGetCPtr( pOrderInfo->abExpr ),
                             pKeyExp, bType, uiLen, szFor, pForExp,
                             fAscend , pOrderInfo->fUnique, fCustom, FALSE );

   if ( pArea->lpdbOrdCondInfo && ( !pArea->lpdbOrdCondInfo->fAll &&
                                    !pArea->lpdbOrdCondInfo->fAdditive ) )
   {
      hb_cdxOrdListClear( pArea, !( CDXAREA_DATA( pArea )->fStrictStruct ?
                          pArea->fHasTags : hb_setGetAutOpen() ), NULL );
   }
   hb_cdxIndexUnLockWrite( pIndex );
   /* Update DBF header */
   if( !pArea->fHasTags && !fOpenedIndex && !pIndex->fDelete && fProd )
   {
      pArea->fHasTags = TRUE;
      if ( !pArea->fReadonly && ( pArea->dbfHeader.bHasTags & 0x01 ) == 0 &&
           ( hb_setGetAutOpen() || CDXAREA_DATA( pArea )->fStrictStruct ) )
         SELF_WRITEDBHEADER( ( AREAP ) pArea );
   }
   else
   {
      fProd = FALSE;
   }

   if( !fOpenedIndex )
   {
      if ( fProd || pArea->lpIndexes == NULL )
      {
         pIndex->pNext = pArea->lpIndexes;
         pArea->lpIndexes = pIndex;
      }
      else
      {
         LPCDXINDEX pIndexTmp = pArea->lpIndexes;
         while ( pIndexTmp->pNext )
            pIndexTmp = pIndexTmp->pNext;
         pIndexTmp->pNext = pIndex;
      }
   }

   pArea->uiTag = hb_cdxGetTagNumber( pArea, pTag );

   /* Clear pArea->lpdbOrdCondInfo */
   SELF_ORDSETCOND( ( AREAP ) pArea, NULL );

   return SELF_GOTOP( ( AREAP ) pArea );
}

/* ( DBENTRYP_OI )    hb_cdxOrderDestroy */
static ERRCODE hb_cdxOrderDestroy( CDXAREAP pArea, LPDBORDERINFO pOrderInfo )
{
   LPCDXINDEX pIndex, pIndexTmp;
   LPCDXTAG pTag;
   USHORT uiTag;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxOrderDestroy(%p, %p)", pArea, pOrderInfo));

   if ( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if ( ! pArea->lpIndexes )
      return SUCCESS;

   if ( pOrderInfo->itmOrder )
   {
      pTag = hb_cdxFindTag( pArea, pOrderInfo->itmOrder, pOrderInfo->atomBagName, &uiTag );
      if ( pTag )
      {
         pIndex = pTag->pIndex;
         if ( /* !pIndex->fShared && */ !pIndex->fReadonly )
         {
            hb_cdxIndexLockWrite( pIndex );
            hb_cdxIndexDelTag( pIndex, pTag->szName );
            hb_cdxIndexUnLockWrite( pIndex );
            if ( !pIndex->TagList )
            {
               if ( pArea->lpIndexes == pIndex )
               {
                  pArea->lpIndexes = pIndex->pNext;
                  if ( pArea->fHasTags )
                  {
                     pArea->fHasTags = FALSE;
                     if ( !pArea->fReadonly && ( pArea->dbfHeader.bHasTags & 0x01 ) != 0 &&
                          ( hb_setGetAutOpen() ||
                            CDXAREA_DATA( pArea )->fStrictStruct ) )
                        SELF_WRITEDBHEADER( ( AREAP ) pArea );
                  }
               }
               else
               {
                  pIndexTmp = pArea->lpIndexes;
                  while ( pIndexTmp->pNext && ( pIndexTmp->pNext != pIndex ) )
                  {
                     pIndexTmp = pIndexTmp->pNext;
                  }
                  if ( pIndexTmp->pNext == pIndex )
                  {
                     pIndexTmp->pNext = pIndex->pNext;
                  }
               }
               pIndex->fDelete = TRUE;
               hb_cdxIndexFree( pIndex );
            }
            if( uiTag < pArea->uiTag )
               pArea->uiTag--;
            else if( uiTag == pArea->uiTag )
               pArea->uiTag = 0;
         }
         else
         {
            /* TODO: allow this operation for shared mode? */
            hb_errInternal( 1023, "hb_cdxOrderDestroy: exclusive required.", NULL, NULL );
         }
      }
   }
   return SUCCESS;
}

/* ( DBENTRYP_OII )   hb_cdxOrderInfo */
/*
 * Provides information about order management.
 */
static ERRCODE hb_cdxOrderInfo( CDXAREAP pArea, USHORT uiIndex, LPDBORDERINFO pInfo )
{
   LPCDXTAG pTag;
   USHORT   uiTag = 0;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxOrderInfo(%p, %hu, %p)", pArea, uiIndex, pInfo));

   switch( uiIndex )
   {
      case DBOI_STRICTREAD:
         if( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         else
            pInfo->itmResult = hb_itemNew( NULL );
         return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_STRICTREAD, 0, pInfo->itmResult );

      case DBOI_OPTIMIZE:
         if( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         else
            pInfo->itmResult = hb_itemNew( NULL );
         return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_OPTIMIZE, 0, pInfo->itmResult );

      case DBOI_AUTOOPEN:
         if( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         else
            pInfo->itmResult = hb_itemNew( NULL );
         return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_AUTOOPEN, 0, pInfo->itmResult );

      case DBOI_AUTOORDER:
         if( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         else
            pInfo->itmResult = hb_itemNew( NULL );
         return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_AUTOORDER, 0, pInfo->itmResult );

      case DBOI_AUTOSHARE:
         if( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         else
            pInfo->itmResult = hb_itemNew( NULL );
         return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_AUTOSHARE, 0, pInfo->itmResult );

      case DBOI_BAGEXT:
         if( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         else
            pInfo->itmResult = hb_itemNew( NULL );
         return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_ORDBAGEXT, 0, pInfo->itmResult );

      case DBOI_EVALSTEP:
         pInfo->itmResult = hb_itemPutNL( pInfo->itmResult,
                  pArea->lpdbOrdCondInfo ? pArea->lpdbOrdCondInfo->lStep : 0 );
         return SUCCESS;

      case DBOI_KEYSINCLUDED:
         pInfo->itmResult = hb_itemPutNL( pInfo->itmResult,
                          pArea->pSort ? pArea->pSort->ulTotKeys : 0 );
         return SUCCESS;

      case DBOI_I_TAGNAME:
         pInfo->itmResult = hb_itemPutC( pInfo->itmResult,
                      pArea->pSort ? pArea->pSort->pTag->szName : NULL );
         return SUCCESS;

      case DBOI_I_BAGNAME:
         pInfo->itmResult = hb_itemPutC( pInfo->itmResult,
                pArea->pSort ? pArea->pSort->pTag->pIndex->szFileName : NULL );
         return SUCCESS;

      case DBOI_ISREINDEX:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                               pArea->pSort ? pArea->pSort->fReindex : FALSE );
         return SUCCESS;

      case DBOI_LOCKOFFSET:
      case DBOI_HPLOCKING:
      {
         HB_FOFFSET ulPos, ulPool;

         hb_dbfLockIdxGetData( pArea->bLockType, &ulPos, &ulPool );
         if ( uiIndex == DBOI_LOCKOFFSET )
            pInfo->itmResult = hb_itemPutNInt( pInfo->itmResult, ulPos );
         else
            pInfo->itmResult = hb_itemPutL( pInfo->itmResult, ulPool > 0 );
         return SUCCESS;
      }

      case DBOI_ORDERCOUNT:
      {
         LPCDXINDEX pIndex;
         char *pszBag = hb_itemGetCLen( pInfo->atomBagName ) > 0 ?
                           hb_itemGetCPtr( pInfo->atomBagName ) : NULL;
         pIndex = pszBag ? hb_cdxFindBag( pArea, pszBag ) : pArea->lpIndexes;
         while ( pIndex )
         {
            pTag = pIndex->TagList;
            while ( pTag )
            {
               ++uiTag;
               pTag = pTag->pNext;
            }
            pIndex = pszBag ? NULL : pIndex->pNext;
         }
         pInfo->itmResult = hb_itemPutNI( pInfo->itmResult, uiTag );
         return SUCCESS;
      }

      case DBOI_BAGCOUNT:
      {
         LPCDXINDEX pIndex = pArea->lpIndexes;
         while ( pIndex )
         {
            ++uiTag;
            pIndex = pIndex->pNext;
         }
         pInfo->itmResult = hb_itemPutNI( pInfo->itmResult, uiTag );
         return SUCCESS;
      }

      case DBOI_BAGNUMBER:
      {
         LPCDXINDEX pIndex = pArea->lpIndexes, pIndexSeek;

         if( hb_itemGetCLen( pInfo->atomBagName ) > 0 )
            pIndexSeek = hb_cdxFindBag( pArea,
                                  hb_itemGetCPtr( pInfo->atomBagName ) );
         else
         {
            pTag = hb_cdxGetTagByNumber( pArea, pArea->uiTag );
            pIndexSeek = pTag ? pTag->pIndex : NULL;
         }

         if( pIndexSeek )
         {
            do
            {
               ++uiTag;
               if( pIndex == pIndexSeek )
                  break;
               pIndex = pIndex->pNext;
            }
            while ( pIndex );
         }
         pInfo->itmResult = hb_itemPutNI( pInfo->itmResult,
                                               pIndex ? uiTag : 0 );
         return SUCCESS;
      }

      case DBOI_BAGORDER:
      {
         LPCDXINDEX pIndex = pArea->lpIndexes, pIndexSeek;

         if( hb_itemGetCLen( pInfo->atomBagName ) > 0 )
            pIndexSeek = hb_cdxFindBag( pArea,
                                  hb_itemGetCPtr( pInfo->atomBagName ) );
         else
         {
            pTag = hb_cdxGetTagByNumber( pArea, pArea->uiTag );
            pIndexSeek = pTag ? pTag->pIndex : NULL;
         }

         if( pIndexSeek )
         {
            ++uiTag;
            do
            {
               if( pIndex == pIndexSeek )
                  break;
               pTag = pIndex->TagList;
               while ( pTag )
               {
                  ++uiTag;
                  pTag = pTag->pNext;
               }
               pIndex = pIndex->pNext;
            }
            while ( pIndex );
         }
         pInfo->itmResult = hb_itemPutNI( pInfo->itmResult,
                                               pIndex ? uiTag : 0 );
         return SUCCESS;
      }
   }

   if( FAST_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if( pInfo->itmOrder )
   {
      pTag = hb_cdxFindTag( pArea, pInfo->itmOrder, pInfo->atomBagName, &uiTag );
   }
   else
   {
      uiTag = pArea->uiTag;
      pTag = hb_cdxGetTagByNumber( pArea, uiTag );
   }

   switch( uiIndex )
   {
      case DBOI_CONDITION:
         pInfo->itmResult = hb_itemPutC( pInfo->itmResult, pTag ? pTag->ForExpr : NULL );
         if ( pTag && pInfo->itmNewVal && HB_IS_STRING( pInfo->itmNewVal ) )
         {
            if ( pTag->ForExpr != NULL )
            {
               hb_xfree( pTag->ForExpr );
               pTag->ForExpr = NULL;
            }
            if ( pTag->pForItem != NULL )
            {
               hb_vmDestroyBlockOrMacro( pTag->pForItem );
               pTag->pForItem = NULL;
            }
            if ( hb_itemGetCLen( pInfo->itmNewVal ) > 0 )
            {
               char * pForExpr = hb_itemGetCPtr( pInfo->itmNewVal );

               if ( SELF_COMPILE( ( AREAP ) pArea, ( BYTE *) pForExpr ) == SUCCESS )
               {
                  PHB_ITEM pForItem = pArea->valResult;
                  pArea->valResult = NULL;
                  if ( SELF_EVALBLOCK( ( AREAP ) pArea, pForItem ) == SUCCESS )
                  {
                     if ( hb_itemType( pArea->valResult ) == HB_IT_LOGICAL )
                     {
                        pTag->pForItem = pForItem;
                        pForItem = NULL;
                     }
                     hb_itemRelease( pArea->valResult );
                     pArea->valResult = NULL;
                  }
                  if ( pForItem )
                     hb_vmDestroyBlockOrMacro( pForItem );
               }
            }
         }
         break;

      case DBOI_EXPRESSION:
         pInfo->itmResult = hb_itemPutC( pInfo->itmResult, pTag ? pTag->KeyExpr : NULL );
         break;

      case DBOI_POSITION:
         if ( pInfo->itmNewVal && HB_IS_NUMERIC( pInfo->itmNewVal ) )
         {
            pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
               hb_cdxDBOIKeyGoto( pArea, pTag,
                  hb_itemGetNL( pInfo->itmNewVal ), TRUE ) == SUCCESS );
         }
         else
            pInfo->itmResult = hb_itemPutNL( pInfo->itmResult,
                                    hb_cdxDBOIKeyNo( pArea, pTag, TRUE ) );
         break;

      /* TODO: is this ok?  DBOI_RECNO == DBOI_KEYNORAW ? No, it isn't. */
      /* case DBOI_RECNO: */
      case DBOI_KEYNORAW:
         if ( pInfo->itmNewVal && HB_IS_NUMERIC( pInfo->itmNewVal ) )
         {
            pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
               hb_cdxDBOIKeyGoto( pArea, pTag,
                  hb_itemGetNL( pInfo->itmNewVal ), FALSE ) == SUCCESS );
         }
         else
            pInfo->itmResult = hb_itemPutNL( pInfo->itmResult,
                                    hb_cdxDBOIKeyNo( pArea, pTag, FALSE ) );
         break;

      case DBOI_KEYCOUNT:
         pInfo->itmResult = hb_itemPutNL( pInfo->itmResult,
                                    hb_cdxDBOIKeyCount( pArea, pTag, TRUE ) );
         break;

      case DBOI_KEYCOUNTRAW:
         pInfo->itmResult = hb_itemPutNL( pInfo->itmResult,
                                    hb_cdxDBOIKeyCount( pArea, pTag, FALSE ) );
         break;

      case DBOI_RELKEYPOS:
         if ( pInfo->itmNewVal && HB_IS_NUMERIC( pInfo->itmNewVal ) )
            hb_cdxDBOISetRelKeyPos( pArea, pTag, 
                                    hb_itemGetND( pInfo->itmNewVal ) );
         else
            pInfo->itmResult = hb_itemPutND( pInfo->itmResult,
                                       hb_cdxDBOIGetRelKeyPos( pArea, pTag ) );
         break;

      case DBOI_FINDREC:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                  hb_cdxDBOIFindRec( pArea, pTag,
                              hb_itemGetNL( pInfo->itmNewVal ), FALSE ) );
         break;

      case DBOI_FINDRECCONT:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                  hb_cdxDBOIFindRec( pArea, pTag, 
                              hb_itemGetNL( pInfo->itmNewVal ), TRUE ) );
         break;

      case DBOI_SKIPUNIQUE:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                        hb_cdxDBOISkipUnique( pArea, pTag,
                           hb_itemGetNI( pInfo->itmNewVal ) >= 0 ) == SUCCESS );
         break;

      case DBOI_SKIPEVAL:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
               hb_cdxDBOISkipEval( pArea, pTag, TRUE, pInfo->itmNewVal ) );
         break;

      case DBOI_SKIPEVALBACK:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
               hb_cdxDBOISkipEval( pArea, pTag, FALSE, pInfo->itmNewVal ) );
         break;

      case DBOI_SKIPWILD:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
               hb_cdxDBOISkipWild( pArea, pTag, TRUE, pInfo->itmNewVal ) );
         break;

      case DBOI_SKIPWILDBACK:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
               hb_cdxDBOISkipWild( pArea, pTag, FALSE, pInfo->itmNewVal ) );
         break;

      case DBOI_SKIPREGEX:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
            hb_cdxDBOISkipRegEx( pArea, pTag, TRUE, pInfo->itmNewVal ) );
         break;

      case DBOI_SKIPREGEXBACK:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
            hb_cdxDBOISkipRegEx( pArea, pTag, FALSE, pInfo->itmNewVal ) );
         break;

      case DBOI_SCOPEEVAL:
         if ( pTag && pInfo->itmNewVal &&
              hb_arrayLen( pInfo->itmNewVal ) == DBRMI_SIZE &&
              hb_arrayGetPtr( pInfo->itmNewVal, DBRMI_FUNCTION ) != NULL )
         {
            pInfo->itmResult = hb_itemPutNL( pInfo->itmResult,
                  hb_cdxDBOIScopeEval( pTag, ( HB_EVALSCOPE_FUNC )
                       hb_arrayGetPtr( pInfo->itmNewVal, DBRMI_FUNCTION ),
                       hb_arrayGetPtr( pInfo->itmNewVal, DBRMI_PARAM ),
                       hb_arrayGetItemPtr( pInfo->itmNewVal, DBRMI_LOVAL ),
                       hb_arrayGetItemPtr( pInfo->itmNewVal, DBRMI_HIVAL ) ) );
         }
         else
         {
            /* TODO: RT error */
            ;
         }
         break;

      case DBOI_NAME:
         pInfo->itmResult = hb_itemPutC( pInfo->itmResult, pTag ? pTag->szName : NULL );
         break;

      case DBOI_NUMBER:
         pInfo->itmResult = hb_itemPutNI( pInfo->itmResult, uiTag );
         break;

      case DBOI_BAGNAME:
         if ( pTag )
         {
            PHB_FNAME pFileName = hb_fsFNameSplit( pTag->pIndex->szFileName );
            pInfo->itmResult = hb_itemPutC( pInfo->itmResult, pFileName->szName );
            hb_xfree( pFileName );
         }
         else
            pInfo->itmResult = hb_itemPutC( pInfo->itmResult, NULL );
         break;

      case DBOI_FULLPATH:
         pInfo->itmResult = hb_itemPutC( pInfo->itmResult, pTag ? pTag->pIndex->szFileName : NULL );
         break;

      case DBOI_FILEHANDLE:
         pInfo->itmResult = hb_itemPutNInt( pInfo->itmResult, ( HB_NHANDLE ) ( pTag ? pTag->pIndex->hFile : FS_ERROR ) );
         break;

      case DBOI_ISCOND:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult, pTag && pTag->ForExpr != NULL );
         break;

      case DBOI_ISDESC:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult, pTag && !pTag->UsrAscend );
         if ( pTag && pInfo->itmNewVal && HB_IS_LOGICAL( pInfo->itmNewVal ) )
         {
            pTag->UsrAscend = ! hb_itemGetL( pInfo->itmNewVal );
            pTag->curKeyState &= ~( CDX_CURKEY_RAWPOS | CDX_CURKEY_LOGPOS );
         }
         break;

      case DBOI_UNIQUE:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult, ( pTag ? pTag->UniqueKey || pTag->UsrUnique : FALSE ) );
         if ( pTag && pInfo->itmNewVal && HB_IS_LOGICAL( pInfo->itmNewVal ) && !pTag->UniqueKey )
         {
            pTag->UsrUnique = hb_itemGetL( pInfo->itmNewVal );
            pTag->curKeyState &= ~( CDX_CURKEY_RAWPOS | CDX_CURKEY_LOGPOS |
                                    CDX_CURKEY_RAWCNT | CDX_CURKEY_LOGCNT );
         }
         break;

      case DBOI_KEYTYPE:
         if ( pTag )
         {
            char szType[2];
            szType[0] = (char) pTag->uiType;
            szType[1] = 0;
            pInfo->itmResult = hb_itemPutC( pInfo->itmResult, szType );
         }
         else
            pInfo->itmResult = hb_itemPutC( pInfo->itmResult, NULL );
         break;

      case DBOI_KEYSIZE:
         pInfo->itmResult = hb_itemPutNI( pInfo->itmResult, pTag ? pTag->uiLen : 0 );
         break;

      case DBOI_KEYDEC:
         /* there is no fixed number of decimal places for numeric keys
            in CDX format */
         pInfo->itmResult = hb_itemPutNI( pInfo->itmResult, 0 );
         break;

      case DBOI_KEYVAL:
         hb_itemClear( pInfo->itmResult );
         if( pArea->lpdbPendingRel )
            SELF_FORCEREL( ( AREAP ) pArea );
         if ( pTag && pArea->fPositioned )
         {
            if ( pTag->CurKey->rec != pArea->ulRecNo )
            {
               hb_cdxIndexLockRead( pTag->pIndex );
               hb_cdxCurKeyRefresh( pArea, pTag );
               hb_cdxIndexUnLockRead( pTag->pIndex );
            }
            if ( pTag->CurKey->rec == pArea->ulRecNo )
               pInfo->itmResult = hb_cdxKeyGetItem( pTag->CurKey,
                                           pInfo->itmResult, pTag, TRUE );
         }
         break;

      case DBOI_SCOPETOP:
         if ( pTag )
         {
            if ( pInfo->itmResult )
               hb_cdxTagGetScope( pTag, 0, pInfo->itmResult );
            if ( pInfo->itmNewVal )
               hb_cdxTagSetScope( pTag, 0, pInfo->itmNewVal );
         }
         else if ( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         break;

      case DBOI_SCOPEBOTTOM:
         if ( pTag )
         {
            if ( pInfo->itmResult )
               hb_cdxTagGetScope( pTag, 1, pInfo->itmResult );
            if ( pInfo->itmNewVal )
               hb_cdxTagSetScope( pTag, 1, pInfo->itmNewVal );
         }
         else if ( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         break;

      case DBOI_SCOPESET:
         if ( pTag )
         {
            if ( pInfo->itmNewVal )
            {
               hb_cdxTagSetScope( pTag, 0, pInfo->itmNewVal );
               hb_cdxTagSetScope( pTag, 1, pInfo->itmNewVal );
            }
         }
         if ( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         break;

      case DBOI_SCOPETOPCLEAR:
         if ( pTag )
         {
            if ( pInfo->itmResult )
               hb_cdxTagGetScope( pTag, 0, pInfo->itmResult );
            hb_cdxTagClearScope( pTag, 0 );
         }
         else if ( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         break;

      case DBOI_SCOPEBOTTOMCLEAR:
         if ( pTag )
         {
            if ( pInfo->itmResult )
               hb_cdxTagGetScope( pTag, 1, pInfo->itmResult );
            hb_cdxTagClearScope( pTag, 1 );
         }
         else if ( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         break;

      case DBOI_SCOPECLEAR:
         if ( pTag )
         {
            hb_cdxTagClearScope( pTag, 0 );
            hb_cdxTagClearScope( pTag, 1 );
         }
         if ( pInfo->itmResult )
            hb_itemClear( pInfo->itmResult );
         break;

      case DBOI_CUSTOM:
         if( hb_itemType( pInfo->itmNewVal ) == HB_IT_LOGICAL )
         {
            if( hb_cdxIndexLockWrite( pTag->pIndex ) )
            {
               if( !pTag->Template )
               {
                  BOOL fNewVal = hb_itemGetL( pInfo->itmNewVal );
                  if( pTag->Custom ? ! fNewVal : fNewVal )
                  {
                     pTag->Custom = fNewVal;
                     pTag->Partial = TRUE;
                     pTag->ChgOnly = FALSE;
                     pTag->TagChanged = TRUE;
                     /* This is a hacks to emulate both SIX3 and COMIX behavior
                      * which should be cleaned. I intentionally not used
                      * HB_SIXCDX macro here [druzus]
                      */
                     if( pTag->Custom )
                        pTag->Template = pTag->MultiKey = TRUE;
                  }
               }
               hb_cdxIndexUnLockWrite( pTag->pIndex );
            }
         }
         /* Warning: it's not CL53 compatible. CL53 returns previous
          * CUSTOM flag value not current one. [druzus]
          */
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult, pTag && pTag->Custom );
         break;

      case DBOI_PARTIAL:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult, pTag && pTag->Partial );
         break;

      case DBOI_CHGONLY:
         /* TODO: set */
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult, pTag && pTag->ChgOnly );
         break;

      /* TODO: */
      /*
      case DBOI_TEMPLATE:
      case DBOI_MULTIKEY:
      */

      case DBOI_KEYADD:
         if ( !pTag )
         {
            pInfo->itmResult = hb_itemPutL( pInfo->itmResult, FALSE );
         }
         else
         {
            if ( pTag->Custom )
            {
               if( pArea->lpdbPendingRel )
                  SELF_FORCEREL( ( AREAP ) pArea );

               if( !pArea->fPositioned ||
                   ( pTag->pForItem && 
                     !hb_cdxEvalCond( pArea, pTag->pForItem, TRUE ) ) )
               {
                  pInfo->itmResult = hb_itemPutL( pInfo->itmResult, FALSE );
               }
               else
               {
                  LPCDXKEY pKey;
                  hb_cdxIndexLockWrite( pTag->pIndex );
                  if ( pInfo->itmNewVal && !HB_IS_NIL( pInfo->itmNewVal ) &&
                       pTag->Template )
                     pKey = hb_cdxKeyPutItem( NULL, pInfo->itmNewVal, pArea->ulRecNo, pTag, TRUE, TRUE );
                  else
                     pKey = hb_cdxKeyEval( NULL, pTag );
                  pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                                                  hb_cdxTagKeyAdd( pTag, pKey ) );
                  hb_cdxIndexUnLockWrite( pTag->pIndex );
                  hb_cdxKeyFree( pKey );
               }
            }
            else
            {
               hb_cdxErrorRT( pArea, 0, EDBF_NOTCUSTOM, NULL, 0, 0 );
            }
         }
         break;

      case DBOI_KEYDELETE:
         if ( !pTag )
         {
            pInfo->itmResult = hb_itemPutL( pInfo->itmResult, FALSE );
         }
         else
         {
            if ( pTag->Custom )
            {
               if( pArea->lpdbPendingRel )
                  SELF_FORCEREL( ( AREAP ) pArea );

               if( !pArea->fPositioned ||
                   ( pTag->pForItem && 
                     !hb_cdxEvalCond( pArea, pTag->pForItem, TRUE ) ) )
               {
                  pInfo->itmResult = hb_itemPutL( pInfo->itmResult, FALSE );
               }
               else
               {
                  LPCDXKEY pKey;
                  hb_cdxIndexLockWrite( pTag->pIndex );
                  if ( pInfo->itmNewVal && !HB_IS_NIL( pInfo->itmNewVal ) &&
                       pTag->Template )
                     pKey = hb_cdxKeyPutItem( NULL, pInfo->itmNewVal, pArea->ulRecNo, pTag, TRUE, TRUE );
                  else
                  {
                     if ( pTag->CurKey->rec != pArea->ulRecNo )
                        hb_cdxCurKeyRefresh( pArea, pTag );

                     if ( pTag->CurKey->rec == pArea->ulRecNo )
                        pKey = hb_cdxKeyCopy( NULL, pTag->CurKey );
                     else
                        pKey = hb_cdxKeyEval( NULL, pTag );
                  }
                  pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                                                  hb_cdxTagKeyDel( pTag, pKey ) );
                  hb_cdxIndexUnLockWrite( pTag->pIndex );
                  hb_cdxKeyFree( pKey );
               }
            }
            else
            {
               hb_cdxErrorRT( pArea, 0, EDBF_NOTCUSTOM, NULL, 0, 0 );
            }
         }
         break;

      case DBOI_READLOCK:
         if( pTag )
         {
            if( hb_itemType( pInfo->itmNewVal ) == HB_IT_LOGICAL )
            {
               if( hb_itemGetL( pInfo->itmNewVal ) )
                  hb_cdxIndexLockRead( pTag->pIndex );
               else
                  hb_cdxIndexUnLockRead( pTag->pIndex );
            }
            pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                                                 pTag->pIndex->lockRead > 0 );
         }
         else
            pInfo->itmResult = hb_itemPutL( pInfo->itmResult, FALSE );
         break;

      case DBOI_WRITELOCK:
         if( pTag )
         {
            if( hb_itemType( pInfo->itmNewVal ) == HB_IT_LOGICAL )
            {
               if( hb_itemGetL( pInfo->itmNewVal ) )
                  hb_cdxIndexLockWrite( pTag->pIndex );
               else
                  hb_cdxIndexUnLockWrite( pTag->pIndex );
            }
            pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                                                 pTag->pIndex->lockWrite > 0 );
         }
         else
            pInfo->itmResult = hb_itemPutL( pInfo->itmResult, FALSE );
         break;

      case DBOI_UPDATECOUNTER:
         if( pTag )
         {
            /* refresh update counter */
            if( hb_cdxIndexLockRead( pTag->pIndex ) )
               hb_cdxIndexUnLockRead( pTag->pIndex );
            pInfo->itmResult = hb_itemPutNInt( pInfo->itmResult,
                                                    pTag->pIndex->ulVersion );
         }
         else
            pInfo->itmResult = hb_itemPutNI( pInfo->itmResult, 0 );
         break;

      case DBOI_SHARED:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                                              pTag && pTag->pIndex->fShared );
         break;

      case DBOI_ISREADONLY:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult,
                                              pTag && pTag->pIndex->fReadonly );
         break;

      case DBOI_ISMULTITAG:
      case DBOI_ISSORTRECNO:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult, pTag != NULL );
         break;

      case DBOI_LARGEFILE:
         pInfo->itmResult = hb_itemPutL( pInfo->itmResult, FALSE );
         break;

      case DBOI_INDEXTYPE:
         pInfo->itmResult = hb_itemPutNI( pInfo->itmResult, pTag ?
                                    DBOI_TYPE_COMPOUND : DBOI_TYPE_UNDEF );
         break;

      default:
         return SUPER_ORDINFO( ( AREAP ) pArea, uiIndex, pInfo );

   }
   return SUCCESS;
}

/* ( DBENTRYP_V )     hb_cdxClearFilter */
static ERRCODE hb_cdxClearFilter( CDXAREAP pArea )
{
   hb_cdxClearLogPosInfo( pArea );
   return SUPER_CLEARFILTER( ( AREAP ) pArea );
}

/* ( DBENTRYP_V )     hb_cdxClearLocate     : NULL */
/* ( DBENTRYP_V )     hb_cdxClearScope      : NULL */

/* ( DBENTRYP_VPLP )  hb_cdxCountScope */
static ERRCODE hb_cdxCountScope( CDXAREAP pArea, void * pPtr, LONG * plRec )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_cdxCountScope(%p, %p, %p)", pArea, pPtr, plRec));

   if ( pPtr == NULL )
   {
      return SUCCESS;
   }
   return SUPER_COUNTSCOPE( ( AREAP ) pArea, pPtr, plRec );
}

/* ( DBENTRYP_I )     hb_cdxFilterText      : NULL */
/* ( DBENTRYP_SI )    hb_cdxScopeInfo       : NULL */

/* ( DBENTRYP_VFI )   hb_cdxSetFilter */
static ERRCODE hb_cdxSetFilter( CDXAREAP pArea, LPDBFILTERINFO pFilterInfo )
{
   hb_cdxClearLogPosInfo( pArea );
   return SUPER_SETFILTER( ( AREAP ) pArea, pFilterInfo );
}

/* ( DBENTRYP_VLO )   hb_cdxSetLocate       : NULL */
/* ( DBENTRYP_VOS )   hb_cdxSetScope        : NULL */
/* ( DBENTRYP_VPL )   hb_cdxSkipScope       : NULL */
/* ( DBENTRYP_B )     hb_cdxLocate          : NULL */

/* ( DBENTRYP_P )     hb_cdxCompile         : NULL */
/* ( DBENTRYP_I )     hb_cdxError           : NULL */
/* ( DBENTRYP_I )     hb_cdxEvalBlock       : NULL */

/* ( DBENTRYP_VSP )   hb_cdxRawLock         : NULL */
/* ( DBENTRYP_VL )    hb_cdxLock            : NULL */
/* ( DBENTRYP_UL )    hb_cdxUnLock          : NULL */

/* ( DBENTRYP_V )     hb_cdxCloseMemFile    : NULL */
/* ( DBENTRYP_VP )    hb_cdxCreateMemFile   : NULL */
/* ( DBENTRYP_SVPB )  hb_cdxGetValueFile    : NULL */
/* ( DBENTRYP_VP )    hb_cdxOpenMemFile     : NULL */
/* ( DBENTRYP_SVPB )  hb_cdxPutValueFile    : NULL */

/* ( DBENTRYP_V )     hb_cdxReadDBHeader    : NULL */
/* ( DBENTRYP_V )     hb_cdxWriteDBHeader   : NULL */
/* ( DBENTRYP_SVP )   hb_cdxWhoCares        : NULL */

/*
 * Retrieve (set) information about RDD
 * ( DBENTRYP_RSLV )   hb_fptFieldInfo
 */
static ERRCODE hb_cdxRddInfo( LPRDDNODE pRDD, USHORT uiIndex, ULONG ulConnect, PHB_ITEM pItem )
{
   LPDBFDATA pData;

   HB_TRACE(HB_TR_DEBUG, ("hb_cdxRddInfo(%p, %hu, %lu, %p)", pRDD, uiIndex, ulConnect, pItem));

   pData = ( LPDBFDATA ) pRDD->lpvCargo;

   switch( uiIndex )
   {
      case RDDI_ORDBAGEXT:
      case RDDI_ORDEREXT:
      case RDDI_ORDSTRUCTEXT:
      {
         char * szNew = hb_itemGetCPtr( pItem );

         if( szNew[0] == '.' && szNew[1] )
            szNew = hb_strdup( szNew );
         else
            szNew = NULL;

         hb_itemPutC( pItem, pData->szIndexExt[ 0 ] ? pData->szIndexExt : CDX_INDEXEXT );
         if( szNew )
         {
            hb_strncpy( pData->szIndexExt, szNew, sizeof( pData->szIndexExt ) - 1 );
            hb_xfree( szNew );
         }
         break;
      }

      case RDDI_MULTIKEY:
      case RDDI_MULTITAG:
      case RDDI_SORTRECNO:
      case RDDI_STRUCTORD:
         hb_itemPutL( pItem, TRUE );
         break;

      case RDDI_STRICTSTRUCT:
      {
         BOOL fStrictStruct = pData->fStrictStruct;
         if( hb_itemType( pItem ) == HB_IT_LOGICAL )
            pData->fStrictStruct = hb_itemGetL( pItem );
         hb_itemPutL( pItem, fStrictStruct );
         break;
      }

      default:
         return SUPER_RDDINFO( pRDD, uiIndex, ulConnect, pItem );

   }

   return SUCCESS;
}



/* ######################################################################### */

static int hb_cdxQuickSortCompare( LPCDXSORTINFO pSort, BYTE * pKey1, BYTE * pKey2 )
{
   int i, iLen = pSort->keyLen;

   i = hb_cdxValCompare( pSort->pTag, pKey1, iLen, pKey2, iLen, TRUE );

   if ( i == 0 )
   {
      i = ( HB_GET_LE_UINT32( pKey1 + iLen ) < HB_GET_LE_UINT32( pKey2 + iLen ) ) ? -1 : 1;
   }

   return i;
}

static BOOL hb_cdxQSort( LPCDXSORTINFO pSort, BYTE * pSrc, BYTE * pBuf, LONG lKeys )
{
   if ( lKeys > 1 )
   {
      int iLen = pSort->keyLen + 4;
      LONG l1, l2;
      BYTE * pPtr1, * pPtr2, *pDst;
      BOOL f1, f2;

      l1 = lKeys >> 1;
      l2 = lKeys - l1;
      pPtr1 = &pSrc[ 0 ];
      pPtr2 = &pSrc[ l1 * iLen ];

      f1 = hb_cdxQSort( pSort, pPtr1, &pBuf[ 0 ], l1 );
      f2 = hb_cdxQSort( pSort, pPtr2, &pBuf[ l1 * iLen ], l2 );
      if ( f1 )
      {
         pDst = pBuf;
      }
      else
      {
         pDst = pSrc;
         pPtr1 = &pBuf[ 0 ];
      }
      if ( !f2 )
      {
         pPtr2 = &pBuf[ l1 * iLen ];
      }
      while ( l1 > 0 && l2 > 0 )
      {
         if ( hb_cdxQuickSortCompare( pSort, pPtr1, pPtr2 ) <= 0 )
         {
            memcpy( pDst, pPtr1, iLen );
            pPtr1 += iLen;
            l1--;
         }
         else
         {
            memcpy( pDst, pPtr2, iLen );
            pPtr2 += iLen;
            l2--;
         }
         pDst += iLen;
      }
      if ( l1 > 0 )
      {
         memcpy( pDst, pPtr1, iLen * l1 );
      }
      else if ( l2 > 0 && f1 == f2 )
      {
         memcpy( pDst, pPtr2, iLen * l2 );
      }
      return !f1;
   }
   return TRUE;
}

static void hb_cdxSortSortPage( LPCDXSORTINFO pSort )
{
   ULONG ulSize = pSort->ulKeys * ( pSort->keyLen + 4 );
#ifdef HB_CDX_DBGTIME
   cdxTimeIdxBld -= hb_cdxGetTime();
#endif
   if ( !hb_cdxQSort( pSort, pSort->pKeyPool, &pSort->pKeyPool[ ulSize ], pSort->ulKeys ) )
   {
      memcpy( pSort->pKeyPool, &pSort->pKeyPool[ ulSize ], ulSize );
   }
#ifdef HB_CDX_DBGTIME
   cdxTimeIdxBld += hb_cdxGetTime();
#endif
}

static void hb_cdxSortAddNodeKey( LPCDXSORTINFO pSort, int iLevel, BYTE *pKeyVal, ULONG ulRec, ULONG ulPage )
{
   LPCDXPAGE pPage;
   BOOL fNew;
   int iLen = pSort->keyLen, iDup = 0, iTrl = 0, iTmp, iPos;
   BYTE *pTmp;

   pPage = pSort->NodeList[ iLevel ];
   if ( iLevel == 0 )
   {
      while ( iTrl < iLen && pKeyVal[ iLen - iTrl - 1 ] == pSort->bTrl )
      {
         iTrl++;
      }
      if ( pPage != NULL && pPage->iKeys > 0 )
      {
#ifdef HB_CDX_PACKTRAIL
         int iMax = iLen - iTrl;
#else
         int iMax = iLen - HB_MAX( iTrl, pSort->iLastTrl );
#endif
         while ( pKeyVal[ iDup ] == pSort->pLastKey[ iDup ] && iDup < iMax )
         {
            iDup++;
         }
      }
#ifndef HB_CDX_PACKTRAIL
      pSort->iLastTrl = iTrl;
#endif
   }
   if( pPage == NULL )
   {
      fNew = TRUE;
   }
   else
   {
      if ( iLevel == 0 )
      {
         fNew = ( pPage->iFree - ( iLen - iDup - iTrl ) - pPage->ReqByte ) < 0;
      }
      else
      {
         fNew = ( pSort->NodeList[ iLevel ]->iKeys >= pSort->pTag->MaxKeys );
      }
   }

   if( fNew )
   {
      pPage = hb_cdxPageNew( pSort->pTag, NULL, 0 );
      pPage->PageType = ( iLevel == 0 ) ? CDX_NODE_LEAF : CDX_NODE_BRANCH;
      if ( iLevel == 0 )
      {
         hb_cdxPageLeafInitSpace( pPage );
         iDup = 0;
         while ( pSort->ulMaxRec > pPage->RNMask )
         {
            pPage->ReqByte++;
            pPage->RNBits += 8;
            pPage->RNMask = ( pPage->RNMask << 8 ) | 0xFF;
         }
      }
      if ( pSort->NodeList[ iLevel ] != NULL )
      {
         pSort->NodeList[ iLevel ]->Right = pPage->Page;
         pPage->Left = pSort->NodeList[ iLevel ]->Page;
         if ( iLevel == 0 )
         {
#ifdef HB_CDX_DBGCODE_EXT
            hb_cdxPageCheckDupTrlRaw( pSort->NodeList[ iLevel ] );
#endif
            hb_cdxSortAddNodeKey( pSort, iLevel + 1, pSort->pLastKey, pSort->ulLastRec, pSort->NodeList[ iLevel ]->Page );
         }
         else
         {
            iPos = ( pSort->NodeList[ iLevel ]->iKeys - 1 ) * ( iLen + 8 );
            pTmp = &pSort->NodeList[ iLevel ]->node.intNode.keyPool[ iPos ];
            hb_cdxSortAddNodeKey( pSort, iLevel + 1, pTmp, HB_GET_BE_UINT32( &pTmp[ iLen ] ), pSort->NodeList[ iLevel ]->Page );
         }
         hb_cdxPageFree( pSort->NodeList[ iLevel ], TRUE );
      }
      pSort->NodeList[ iLevel ] = pPage;
   }
   if ( iLevel == 0 )
   {
      iPos = pPage->iKeys * pPage->ReqByte;
      hb_cdxSetLeafRecord( &pPage->node.extNode.keyPool[ iPos ],
                           ulRec, iDup, iTrl,
                           pPage->ReqByte, pPage->DCBits, pPage->TCBits );
      iTmp = iLen - iDup - iTrl;
      if ( iTmp > 0 )
      {
         memcpy( &pPage->node.extNode.keyPool[ pPage->iFree + iPos - iTmp ],
                 &pKeyVal[ iDup ], iTmp );
      }
      pPage->iFree -= iTmp + pPage->ReqByte;
      pPage->iKeys++;
#ifdef HB_CDX_DBGCODE_EXT
      hb_cdxPageCheckDupTrlRaw( pSort->NodeList[ iLevel ] );
#endif
   }
   else
   {
      pPage = pSort->NodeList[ iLevel ];
      iPos = pPage->iKeys * ( iLen + 8 );
      pTmp = &pPage->node.intNode.keyPool[ iPos ];
      memcpy( pTmp, pKeyVal, iLen );
      HB_PUT_BE_UINT32( &pTmp[ iLen ], ulRec );
      HB_PUT_BE_UINT32( &pTmp[ iLen + 4 ], ulPage );
      pPage->iKeys++;
   }
}

static void hb_cdxSortWritePage( LPCDXSORTINFO pSort )
{
   ULONG ulSize = pSort->ulKeys * ( pSort->keyLen + 4 );

   hb_cdxSortSortPage( pSort );

   if ( pSort->hTempFile == FS_ERROR )
   {
      BYTE szName[ _POSIX_PATH_MAX + 1 ];
      pSort->hTempFile = hb_fsCreateTemp( NULL, NULL, FC_NORMAL, szName );
      if ( pSort->hTempFile == FS_ERROR )
      {
         hb_errInternal( 9301, "hb_cdxSortWritePage: Can't create temporary file.", NULL, NULL );
      }
      pSort->szTempFileName = hb_strdup( ( char * ) szName );
   }
   pSort->pSwapPage[ pSort->ulCurPage ].ulKeys = pSort->ulKeys;
   pSort->pSwapPage[ pSort->ulCurPage ].nOffset = hb_fsSeekLarge( pSort->hTempFile, 0, FS_END );
   if ( hb_fsWriteLarge( pSort->hTempFile, pSort->pKeyPool, ulSize ) != ulSize )
   {
      hb_errInternal( 9302, "hb_cdxSortWritePage: Write error in temporary file.", NULL, NULL );
   }
   pSort->ulKeys = 0;
   pSort->ulCurPage++;
}

static void hb_cdxSortGetPageKey( LPCDXSORTINFO pSort, ULONG ulPage,
                                  BYTE ** pKeyVal, ULONG *pulRec )
{
   int iLen = pSort->keyLen;

   if ( pSort->pSwapPage[ ulPage ].ulKeyBuf == 0 )
   {
      ULONG ulKeys = HB_MIN( pSort->ulPgKeys, pSort->pSwapPage[ ulPage ].ulKeys );
      ULONG ulSize = ulKeys * ( iLen + 4 );

      if ( hb_fsSeekLarge( pSort->hTempFile, pSort->pSwapPage[ ulPage ].nOffset, SEEK_SET ) != pSort->pSwapPage[ ulPage ].nOffset ||
           hb_fsReadLarge( pSort->hTempFile, pSort->pSwapPage[ ulPage ].pKeyPool, ulSize ) != ulSize )
      {
         hb_errInternal( 9303, "hb_cdxSortGetPageKey: Read error from temporary file.", NULL, NULL );
      }
      pSort->pSwapPage[ ulPage ].nOffset += ulSize;
      pSort->pSwapPage[ ulPage ].ulKeyBuf = ulKeys;
      pSort->pSwapPage[ ulPage ].ulCurKey = 0;
   }
   *pKeyVal = &pSort->pSwapPage[ ulPage ].pKeyPool[ pSort->pSwapPage[ ulPage ].ulCurKey * ( iLen + 4 ) ];
   *pulRec = HB_GET_LE_UINT32( *pKeyVal + iLen );
}

#ifdef HB_CDX_NEW_SORT
static void hb_cdxSortOrderPages( LPCDXSORTINFO pSort )
{
   int iLen = pSort->keyLen, i;
   LONG l, r, m;
   ULONG n, ulPage, ulRec;
   BYTE *pKey = NULL, *pTmp;

   pSort->ulFirst = 0;
   pSort->pSortedPages = ( ULONG * ) hb_xgrab( pSort->ulPages * sizeof( ULONG ) );
   pSort->pSortedPages[ 0 ] = 0;

   if ( pSort->ulTotKeys > 0 )
   {
      for ( n = 0; n < pSort->ulPages; n++ )
      {
         hb_cdxSortGetPageKey( pSort, n, &pKey, &ulRec );
         l = 0;
         r = n - 1;
         while ( l <= r )
         {
            m = ( l + r ) >> 1;
            ulPage = pSort->pSortedPages[ m ];
            pTmp = &pSort->pSwapPage[ ulPage ].pKeyPool[ pSort->pSwapPage[ ulPage ].ulCurKey * ( iLen + 4 ) ];
            i = hb_cdxValCompare( pSort->pTag, pKey, iLen, pTmp, iLen, TRUE );
            if ( i == 0 )
               i = ( ulRec < HB_GET_LE_UINT32( &pTmp[ iLen ] ) ) ? -1 : 1;
            if ( i > 0 )
               l = m + 1;
            else
               r = m - 1;
         }
         for ( r = n; r > l; r-- )
            pSort->pSortedPages[ r ] = pSort->pSortedPages[ r - 1 ];
         pSort->pSortedPages[ l ] = n;
      }
   }
}

static BOOL hb_cdxSortKeyGet( LPCDXSORTINFO pSort, BYTE ** pKeyVal, ULONG *pulRec )
{
   int iLen = pSort->keyLen, i;
   LONG l, r, m;
   ULONG ulPage;

   ulPage = pSort->pSortedPages[ pSort->ulFirst ];

   /* check if first page has some keys yet */
   if ( pSort->pSwapPage[ ulPage ].ulKeys > 0 )
   {
      BYTE *pKey, *pTmp;
      ULONG ulRec;

      /*
       * last key was taken from this page - we have to resort it.
       * This is done intentionally here to be sure that the key
       * value return by this function will not be overwritten by
       * next keys in page read from temporary file in function
       * hb_cdxSortGetPageKey() - please do not move this part down
       * even it seems to be correct
       */
      hb_cdxSortGetPageKey( pSort, ulPage, &pKey, &ulRec );

      l = pSort->ulFirst + 1;
      r = pSort->ulPages - 1;
      while ( l <= r )
      {
         m = ( l + r ) >> 1;
         ulPage = pSort->pSortedPages[ m ];
         pTmp = &pSort->pSwapPage[ ulPage ].pKeyPool[ pSort->pSwapPage[ ulPage ].ulCurKey * ( iLen + 4 ) ];
         i = hb_cdxValCompare( pSort->pTag, pKey, iLen, pTmp, iLen, TRUE );
         if ( i == 0 )
            i = ( ulRec < HB_GET_LE_UINT32( &pTmp[ iLen ] ) ) ? -1 : 1;

         if ( i > 0 )
            l = m + 1;
         else
            r = m - 1;
      }
      if ( l > ( LONG ) pSort->ulFirst + 1 )
      {
         ulPage = pSort->pSortedPages[ pSort->ulFirst ];
         for ( r = pSort->ulFirst + 1; r < l; r++ )
            pSort->pSortedPages[ r - 1 ] = pSort->pSortedPages[ r ];
         pSort->pSortedPages[ l - 1 ] = ulPage;
      }
   }
   else
   {
      pSort->ulFirst++;
   }
   if ( pSort->ulFirst < pSort->ulPages )
   {
      ulPage = pSort->pSortedPages[ pSort->ulFirst ];
      hb_cdxSortGetPageKey( pSort, ulPage, pKeyVal, pulRec );
      pSort->pSwapPage[ ulPage ].ulCurKey++;
      pSort->pSwapPage[ ulPage ].ulKeys--;
      pSort->pSwapPage[ ulPage ].ulKeyBuf--;
      return TRUE;
   }
   return FALSE;
}

#else

static BOOL hb_cdxSortKeyGet( LPCDXSORTINFO pSort, BYTE ** pKeyVal, ULONG *pulRec )
{
   int i, iLen = pSort->keyLen;
   ULONG ulPage, ulKeyPage = 0, ulRec = 0, ulRecTmp;
   BYTE *pKey = NULL, *pTmp;

   for ( ulPage = 0; ulPage < pSort->ulPages; ulPage++ )
   {
      if ( pSort->pSwapPage[ ulPage ].ulKeys > 0 )
      {
         hb_cdxSortGetPageKey( pSort, ulPage, &pTmp, &ulRecTmp );
         if ( ! pKey )
         {
            i = 1;
         }
         else
         {
            i = hb_cdxValCompare( pSort->pTag, pKey, iLen, pTmp, iLen, TRUE );
            if ( i == 0 )
            {
               i = ( ulRec < ulRecTmp ) ? -1 : 1;
            }
         }
         if ( i > 0 )
         {
            pKey = pTmp;
            ulRec = ulRecTmp;
            ulKeyPage = ulPage;
         }
      }
   }
   if ( pKey )
   {
      pSort->pSwapPage[ ulKeyPage ].ulCurKey++;
      pSort->pSwapPage[ ulKeyPage ].ulKeys--;
      pSort->pSwapPage[ ulKeyPage ].ulKeyBuf--;
      *pulRec = ulRec;
      *pKeyVal = pKey;
      return TRUE;
   }
   return FALSE;
}

#endif

static void hb_cdxSortKeyAdd( LPCDXSORTINFO pSort, ULONG ulRec, BYTE * pKeyVal, int iKeyLen )
{
   int iLen = pSort->keyLen;
   BYTE *pDst;

   if ( pSort->ulKeys >= pSort->ulPgKeys )
   {
      hb_cdxSortWritePage( pSort );
   }
   pDst = &pSort->pKeyPool[ pSort->ulKeys * ( iLen + 4 ) ];

   if ( iLen > iKeyLen )
   {
      memcpy( pDst, pKeyVal, iKeyLen );
      memset( &pDst[ iKeyLen ], pSort->bTrl, iLen - iKeyLen );
   }
   else
   {
      memcpy( pDst, pKeyVal, iLen );
   }
   HB_PUT_LE_UINT32( &pDst[ iLen ], ulRec );
   pSort->ulKeys++;
   pSort->ulTotKeys++;
}

static LPCDXSORTINFO hb_cdxSortNew( LPCDXTAG pTag, ULONG ulRecCount )
{
   LPCDXSORTINFO pSort;
   BYTE * pBuf;
   int iLen = pTag->uiLen;
   ULONG ulSize, ulMax, ulMin;

   if ( ulRecCount == 0 )
      ulRecCount = 1;

   pSort = ( LPCDXSORTINFO ) hb_xgrab( sizeof( CDXSORTINFO ) );
   memset( pSort, 0, sizeof( CDXSORTINFO ) );
   ulMax = ulMin = ( ULONG ) ceil( sqrt( ( double ) ulRecCount ) );
   ulSize = ( 1L << 20 ) / ( iLen + 4 );
   while ( ulMax < ulSize )
      ulMax <<= 1;
   if ( ulMax > ulRecCount )
      ulMax = ulRecCount;

   do
   {
      ulSize = ulMax * ( iLen + 4 );
      pBuf = ( BYTE * ) hb_xalloc( ulSize << 2 );
      if ( pBuf )
      {
         hb_xfree( pBuf );
         pBuf = ( BYTE * ) hb_xalloc( ulSize << 1 );
      }
      else
      {
         ulMax >>= 1;
      }
   }
   while ( ! pBuf && ulMax >= ulMin );

   if ( ! pBuf )
   {
      /* call hb_xgrab() to force out of memory error,
       * though in multi process environment this call may return
       * with success when other process free some memory
       * (also the size of buf is reduced to absolute minimum).
       * Sorry but I'm to lazy to implement indexing with smaller
       * memory though it's possible - just simply I can even create
       * index on-line by key adding like in normal update process.
       * The memory necessary to index file is now ~
       *    ~ (keySize+4+sizeof(CDXSWAPPAGE)) * sqrt(ulRecCount) * 2
       * so the maximum is for DBF with 2^32 records and keySize 240 ~
       * ~ 2^17 * 268 ~=~ 35 Mb
       * this is not a problem for current computers and I do not see
       * any way to use DBFs with four billions records and indexes with
       * such long (240 bytes) keys on the old ones - they will be simply
       * to slow. IMHO it's also better to signal out of memory here and
       * force some system upgrades then run process which will have to
       * take many hours, Druzus.
       */
      ulMax = ulMin;
      pBuf = ( BYTE * ) hb_xgrab( ( ulMax << 1 ) * ( iLen + 4 ) );
   }

   pSort->pTag = pTag;
   pSort->hTempFile = FS_ERROR;
   pSort->keyLen = iLen;
   pSort->bTrl = pTag->bTrail;
   pSort->fUnique = pTag->UniqueKey;
   pSort->ulMaxKey = ulMax << 1;
   pSort->ulPgKeys = ulMax;
   pSort->ulMaxRec = ulRecCount;
   pSort->pKeyPool = pBuf;
   pSort->ulPages = ( ulRecCount + pSort->ulPgKeys - 1 ) / pSort->ulPgKeys;
   pSort->pSwapPage = ( LPCDXSWAPPAGE ) hb_xgrab( sizeof( CDXSWAPPAGE ) * pSort->ulPages );
   memset( pSort->pSwapPage, 0, sizeof( CDXSWAPPAGE ) * pSort->ulPages );

   return pSort;
}

static void hb_cdxSortFree( LPCDXSORTINFO pSort )
{
   if ( pSort->hTempFile != FS_ERROR )
   {
      hb_fsClose( pSort->hTempFile );
   }
   if ( pSort->szTempFileName )
   {
      hb_fsDelete( (BYTE *)  ( pSort->szTempFileName ) );
      hb_xfree( pSort->szTempFileName );
   }
   if ( pSort->pKeyPool )
   {
      hb_xfree( pSort->pKeyPool );
   }
   if ( pSort->pSwapPage )
   {
      hb_xfree( pSort->pSwapPage );
   }
   if ( pSort->pRecBuff )
   {
      hb_xfree( pSort->pRecBuff );
   }
   if ( pSort->pSortedPages )
   {
      hb_xfree( pSort->pSortedPages );
   }
   hb_xfree( pSort );
}

static void hb_cdxSortOut( LPCDXSORTINFO pSort )
{
   BOOL fUnique = pSort->fUnique, fNext;
   ULONG ulPage, ulRec, ulKey;
   BYTE * pKeyVal;
   int iLen = pSort->keyLen, iLevel;

   pSort->ulPages = pSort->ulCurPage + 1;
   pSort->ulPgKeys = pSort->ulMaxKey / pSort->ulPages;
   /*
   printf( "\r\npSort->ulMaxKey=%ld, pSort->ulPages=%ld, pSort->ulPgKeys=%ld, size=%ld\r\n",
           pSort->ulMaxKey, pSort->ulPages, pSort->ulPgKeys,
           pSort->ulMaxKey * ( pSort->keyLen + 4 ) ); fflush(stdout);
   */
   if ( pSort->ulPages > 1 )
   {
      BYTE * pBuf = pSort->pKeyPool;
      hb_cdxSortWritePage( pSort );
      for ( ulPage = 0; ulPage < pSort->ulPages; ulPage++ )
      {
         pSort->pSwapPage[ ulPage ].ulKeyBuf = 0;
         pSort->pSwapPage[ ulPage ].ulCurKey = 0;
         pSort->pSwapPage[ ulPage ].pKeyPool = pBuf;
         pBuf += pSort->ulPgKeys * ( pSort->keyLen + 4 );
      }
   }
   else
   {
      hb_cdxSortSortPage( pSort );
      pSort->pSwapPage[ 0 ].ulKeys = pSort->ulKeys;
      pSort->pSwapPage[ 0 ].ulKeyBuf = pSort->ulKeys;
      pSort->pSwapPage[ 0 ].ulCurKey = 0;
      pSort->pSwapPage[ 0 ].pKeyPool = pSort->pKeyPool;
   }

#ifdef HB_CDX_NEW_SORT
   hb_cdxSortOrderPages( pSort );
#endif

   for ( ulKey = 0; ulKey < pSort->ulTotKeys; ulKey++ )
   {
      if ( ! hb_cdxSortKeyGet( pSort, &pKeyVal, &ulRec ) )
      {
         hb_errInternal( 9304, "hb_cdxSortOut: memory structure corrupted.", NULL, NULL );
      }
      if ( fUnique )
      {
         if ( ulKey != 0 && hb_cdxValCompare( pSort->pTag, pSort->pLastKey, iLen, pKeyVal, iLen, TRUE ) == 0 )
         {
            continue;
         }
      }
#ifdef HB_CDX_DBGCODE_EXT
      if ( ulKey != 0 )
      {
         int i = hb_cdxValCompare( pSort->pTag, pSort->pLastKey, iLen, pKeyVal, iLen, TRUE );
         if ( i == 0 )
         {
            i = ( pSort->ulLastRec < ulRec ) ? -1 : 1;
         }
         if ( i > 0 )
         {
            printf("\r\nulKey=%ld, pKeyVal=[%s][%ld], pKeyLast=[%s][%ld]\r\n",
                   ulKey, pKeyVal, ulRec, pSort->pLastKey, pSort->ulLastRec); fflush(stdout);
            hb_errInternal( 9305, "hb_cdxSortOut: sorting fails.", NULL, NULL );
         }
      }
#endif
      hb_cdxSortAddNodeKey( pSort, 0, pKeyVal, ulRec, 0 );
      memcpy( pSort->pLastKey, pKeyVal, iLen );
      pSort->ulLastRec = ulRec;
   }

#ifdef HB_CDX_DBGCODE
   if ( hb_cdxSortKeyGet( pSort, &pKeyVal, &ulRec ) )
   {
      hb_errInternal( 9306, "hb_cdxSortOut: memory structure corrupted(2).", NULL, NULL );
   }
#endif

   if ( pSort->NodeList[ 0 ] == NULL )
   {
      pSort->NodeList[ 0 ] = hb_cdxPageNew( pSort->pTag, NULL, 0 );
      pSort->NodeList[ 0 ]->PageType = CDX_NODE_LEAF;
      hb_cdxPageLeafInitSpace( pSort->NodeList[ 0 ] );
   }

   iLevel = 0;
   fNext = TRUE;
   do
   {
      if ( iLevel + 1 == CDX_STACKSIZE || pSort->NodeList[ iLevel + 1 ] == NULL )
      {
         pSort->NodeList[ iLevel ]->PageType |= CDX_NODE_ROOT;
         pSort->pTag->RootBlock = pSort->NodeList[ iLevel ]->Page;
         fNext = FALSE;
      }
      else
      {
         hb_cdxSortAddNodeKey( pSort, iLevel + 1, pSort->pLastKey, pSort->ulLastRec, pSort->NodeList[ iLevel ]->Page );
      }
      hb_cdxPageFree( pSort->NodeList[ iLevel ], TRUE );
      iLevel++;
   }
   while ( fNext );
}

static void hb_cdxTagEmptyIndex( LPCDXTAG pTag )
{
   pTag->RootPage  = hb_cdxPageNew( pTag, NULL, 0 );
   pTag->RootBlock = pTag->RootPage->Page;
   pTag->RootPage->PageType = CDX_NODE_ROOT | CDX_NODE_LEAF;
   hb_cdxPageLeafInitSpace( pTag->RootPage );
}

static void hb_cdxTagDoIndex( LPCDXTAG pTag, BOOL fReindex )
{
   LPCDXAREA pArea = pTag->pIndex->pArea;
   LPCDXSORTINFO pSort;
   PHB_ITEM pForItem, pWhileItem = NULL, pEvalItem = NULL, pItem = NULL;
   ULONG ulRecCount, ulRecNo = pArea->ulRecNo;
   LONG lStep = 0;
#ifndef HB_CDP_SUPPORT_OFF
   PHB_CODEPAGE cdpTmp = hb_cdpSelect( pArea->cdPage );
#endif

   if ( pArea->lpdbOrdCondInfo )
   {
      pEvalItem = pArea->lpdbOrdCondInfo->itmCobEval;
      pWhileItem = pArea->lpdbOrdCondInfo->itmCobWhile;
      lStep = pArea->lpdbOrdCondInfo->lStep;
   }

   if( pTag->Custom || ( pTag->OptFlags & CDX_TYPE_STRUCTURE ) )
   {
      ulRecCount = 0;
   }
   else if( SELF_RECCOUNT( ( AREAP ) pArea, &ulRecCount ) != SUCCESS )
   {
      return;
   }

   pArea->pSort = pSort = hb_cdxSortNew( pTag, ulRecCount );
   pSort->fReindex = fReindex;

#if defined( HB_SIXCDX )
   if ( ( pTag->OptFlags & CDX_TYPE_STRUCTURE ) == 0 && pEvalItem )
   {
      SELF_GOTO( ( AREAP ) pArea, 0 );
      if ( !hb_cdxEvalCond( pArea, pEvalItem, FALSE ) )
      {
         hb_cdxSortFree( pSort );
         pArea->pSort = NULL;
         SELF_GOTO( ( AREAP ) pArea, ulRecNo );
         return;
      }
   }
#endif
   if( ulRecCount == 0 )
   {
      hb_cdxTagEmptyIndex( pTag );
   }
   else
   {
      USHORT uiSaveTag = pArea->uiTag;
      ULONG ulStartRec = 0, ulNextCount = 0;
      BOOL fDirectRead, fUseFilter = FALSE;
      BYTE * pSaveRecBuff = pArea->pRecord, cTemp[8];
      int iRecBuff = 0, iRecBufSize = USHRT_MAX / pArea->uiRecordLen, iRec;

      pForItem = pTag->pForItem;
      if ( pTag->nField )
         pItem = hb_itemNew( NULL );

      if ( !pArea->lpdbOrdCondInfo || pArea->lpdbOrdCondInfo->fAll )
      {
         pArea->uiTag = 0;
      }
      else
      {
         if( pArea->lpdbOrdCondInfo->itmRecID )
            ulStartRec = hb_itemGetNL( pArea->lpdbOrdCondInfo->itmRecID );
         if ( ulStartRec )
         {
            ulNextCount = 1;
         }
         else if ( pArea->lpdbOrdCondInfo->fRest || pArea->lpdbOrdCondInfo->lNextCount > 0 )
         {
            if( pArea->lpdbOrdCondInfo->itmStartRecID )
               ulStartRec = hb_itemGetNL( pArea->lpdbOrdCondInfo->itmStartRecID );
            if( !ulStartRec )
               ulStartRec = ulRecNo;
            if( pArea->lpdbOrdCondInfo->lNextCount > 0 )
               ulNextCount = pArea->lpdbOrdCondInfo->lNextCount;
         }
         else if( pArea->lpdbOrdCondInfo->fUseFilter )
         {
            fUseFilter = TRUE;
         }
         else if ( !pArea->lpdbOrdCondInfo->fUseCurrent )
         {
            pArea->uiTag = 0;
         }
      }
      fDirectRead = !hb_setGetStrictRead() && /* !pArea->lpdbRelations && */
                    ( !pArea->lpdbOrdCondInfo || pArea->lpdbOrdCondInfo->fAll ||
                      ( pArea->uiTag == 0 && !fUseFilter ) );

      if ( fDirectRead )
         pSort->pRecBuff = (BYTE *) hb_xgrab( pArea->uiRecordLen * iRecBufSize );

      if ( ulStartRec == 0 && pArea->uiTag == 0 )
         ulStartRec = 1;

      if ( ulStartRec == 0 )
      {
         SELF_GOTOP( ( AREAP ) pArea );
      }
      else
      {
         SELF_GOTO( ( AREAP ) pArea, ulStartRec );
         if ( fUseFilter )
            SELF_SKIPFILTER( ( AREAP ) pArea, 1 );
      }

      ulRecNo = pArea->ulRecNo;

      while ( !pArea->fEof )
      {
         if ( fDirectRead )
         {
            if ( ulRecNo > ulRecCount )
               break;
            if ( iRecBuff == 0 || iRecBuff >= iRecBufSize )
            {
               if ( ulRecCount - ulRecNo >= ( ULONG ) iRecBufSize )
                  iRec = iRecBufSize;
               else
                  iRec = ulRecCount - ulRecNo + 1;
               if( ulNextCount > 0 && ulNextCount < ( ULONG ) iRec )
                  iRec = ( int ) ulNextCount;
               hb_fsSeekLarge( pArea->hDataFile,
                               ( HB_FOFFSET ) pArea->uiHeaderLen + 
                               ( HB_FOFFSET ) ( ulRecNo - 1 ) * 
                               ( HB_FOFFSET ) pArea->uiRecordLen, FS_SET );
               hb_fsReadLarge( pArea->hDataFile, pSort->pRecBuff, pArea->uiRecordLen * iRec );
               iRecBuff = 0;
            }
            pArea->pRecord = pSort->pRecBuff + iRecBuff * pArea->uiRecordLen;
            pArea->ulRecNo = ulRecNo;
            if( SELF_GETREC( ( AREAP ) pArea, NULL ) == FAILURE )
               break;
            pArea->fValidBuffer = pArea->fPositioned = TRUE;
            pArea->fDeleted = pArea->pRecord[ 0 ] == '*';
            /* Force relational movement in child WorkAreas */
            if( pArea->lpdbRelations )
               if( SELF_SYNCCHILDREN( ( AREAP ) pArea ) == FAILURE )
                  break;
            iRecBuff++;
         }

#if !defined( HB_SIXCDX )
         if ( pEvalItem )
         {
            if ( lStep >= pArea->lpdbOrdCondInfo->lStep )
            {
               lStep = 0;
               if ( !hb_cdxEvalCond( pArea, pEvalItem, FALSE ) )
                  break;
            }
            ++lStep;
         }
#endif

         if ( pWhileItem && !hb_cdxEvalCond( NULL, pWhileItem, FALSE ) )
            break;

         if ( ulRecNo <= ulRecCount &&
              ( pForItem == NULL || hb_cdxEvalCond( pArea, pForItem, FALSE ) ) )
         {
            double d;

            if ( pTag->nField )
               SELF_GETVALUE( ( AREAP ) pArea, pTag->nField, pItem );
            else
               pItem = hb_vmEvalBlockOrMacro( pTag->pKeyItem );

            switch( hb_itemType( pItem ) )
            {
               case HB_IT_STRING:
               case HB_IT_STRING | HB_IT_MEMO:
                  hb_cdxSortKeyAdd( pSort, pArea->ulRecNo,
                                    ( BYTE * ) hb_itemGetCPtr( pItem ),
                                    hb_itemGetCLen( pItem ) );
                  break;

               case HB_IT_INTEGER:
               case HB_IT_LONG:
               case HB_IT_DOUBLE:
                  d = hb_itemGetND( pItem );
                  HB_DBL2ORD( &d, &cTemp[0] );
                  hb_cdxSortKeyAdd( pSort, pArea->ulRecNo, cTemp, 8 );
                  break;

               case HB_IT_DATE:
                  d = (double) hb_itemGetDL( pItem );
                  HB_DBL2ORD( &d, &cTemp[0] );
                  hb_cdxSortKeyAdd( pSort, pArea->ulRecNo, cTemp, 8 );
                  break;

               case HB_IT_LOGICAL:
                  cTemp[0] = (BYTE) (hb_itemGetL( pItem ) ? 'T' : 'F');
                  hb_cdxSortKeyAdd( pSort, pArea->ulRecNo, cTemp, 1 );
                  break;

               default:
                  if ( hb_vmRequestQuery() )
                  {
                     pEvalItem = NULL;
                     ulNextCount = 1;
                  }
                  else
                  {
                     printf( "hb_cdxTagDoIndex: hb_itemType( pItem ) = %i", hb_itemType( pItem ) );
                  }
                  break;
            }
         }

         if( ulNextCount > 0 )
         {
            if( --ulNextCount == 0 )
               break;
         }

#if defined( HB_SIXCDX )
         if ( pEvalItem )
         {
            if ( lStep >= pArea->lpdbOrdCondInfo->lStep )
            {
               lStep = 0;
               if ( !hb_cdxEvalCond( pArea, pEvalItem, FALSE ) )
                  break;
            }
            ++lStep;
         }
#endif

         if( fDirectRead )
            ulRecNo++;
         else
         {
            if( SELF_SKIPRAW( ( AREAP ) pArea, 1 ) == FAILURE )
               break;
            if( fUseFilter && SELF_SKIPFILTER( ( AREAP ) pArea, 1 ) == FAILURE )
               break;
            ulRecNo = pArea->ulRecNo;
         }
      }

      hb_cdxSortOut( pSort );
      if ( pTag->nField )
         hb_itemRelease( pItem );

      if ( fDirectRead )
      {
         pArea->pRecord = pSaveRecBuff;
         SELF_GOTO( ( AREAP ) pArea, ulRecNo );
      }
      pArea->uiTag = uiSaveTag;

#if !defined( HB_SIXCDX )
      if ( pEvalItem && lStep )
      {
         /* pArea->fEof = TRUE; */
         hb_cdxEvalCond( pArea, pEvalItem, FALSE );
      }
#endif
   }

#if defined( HB_SIXCDX )
   if ( pEvalItem )
   {
      SELF_GOTO( ( AREAP ) pArea, 0 );
      pArea->fBof = FALSE;
      hb_cdxEvalCond( pArea, pEvalItem, FALSE );
   }
#endif

   hb_cdxSortFree( pSort );
   pArea->pSort = NULL;

#ifndef HB_CDP_SUPPORT_OFF
   hb_cdpSelect( cdpTmp );
#endif
}

#define __PRG_SOURCE__ __FILE__
#ifdef HB_PCODE_VER
   #undef HB_PRG_PCODE_VER
   #define HB_PRG_PCODE_VER HB_PCODE_VER
#endif

HB_FUNC_EXTERN( _DBF );

#if defined( HB_SIXCDX )

HB_FUNC( SIXCDX ) {;}

HB_FUNC( SIXCDX_GETFUNCTABLE )
{
   RDDFUNCS * pTable;
   USHORT * uiCount, uiRddId;

   uiCount = ( USHORT * ) hb_parptr( 1 );
   pTable = ( RDDFUNCS * ) hb_parptr( 2 );
   uiRddId = hb_parni( 4 );

   HB_TRACE(HB_TR_DEBUG, ("SIXCDX_GETFUNCTABLE(%p, %p)", uiCount, pTable));

   if ( pTable )
   {
      ERRCODE errCode;

      if ( uiCount )
         * uiCount = RDDFUNCSCOUNT;
      errCode = hb_rddInherit( pTable, &cdxTable, &cdxSuper, "DBFFPT" );
      if ( errCode != SUCCESS )
         errCode = hb_rddInherit( pTable, &cdxTable, &cdxSuper, "DBFDBT" );
      if ( errCode != SUCCESS )
         errCode = hb_rddInherit( pTable, &cdxTable, &cdxSuper, "DBF" );
      hb_retni( errCode );
      if ( errCode == SUCCESS )
      {
         /*
          * we successfully register our RDD so now we can initialize it
          * You may think that this place is RDD init statement, Druzus
          */
         s_uiRddId = uiRddId;
      }
   }
   else
      hb_retni( FAILURE );
}

static void hb_dbfcdxRddInit( void * cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   if( hb_rddRegister( "DBF",    RDT_FULL ) <= 1 )
   {
      hb_rddRegister( "DBFFPT", RDT_FULL );
      if( hb_rddRegister( "SIXCDX", RDT_FULL ) <= 1 )
      {
         return;
      }
   }

   hb_errInternal( HB_EI_RDDINVALID, NULL, NULL, NULL );

   /* not executed, only to force DBF RDD linking */
   HB_FUNC_EXEC( _DBF );
}

HB_INIT_SYMBOLS_BEGIN( dbfcdx1__InitSymbols )
{ "SIXCDX",              {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( SIXCDX )}, NULL },
{ "SIXCDX_GETFUNCTABLE", {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( SIXCDX_GETFUNCTABLE )}, NULL }
HB_INIT_SYMBOLS_END( dbfcdx1__InitSymbols )

#else

HB_FUNC( DBFCDX ) {;}

HB_FUNC( DBFCDX_GETFUNCTABLE )
{
   RDDFUNCS * pTable;
   USHORT * uiCount, uiRddId;

   uiCount = ( USHORT * ) hb_parptr( 1 );
   pTable = ( RDDFUNCS * ) hb_parptr( 2 );
   uiRddId = hb_parni( 4 );

   HB_TRACE(HB_TR_DEBUG, ("DBFCDX_GETFUNCTABLE(%p, %p)", uiCount, pTable));

   if ( pTable )
   {
      ERRCODE errCode;

      if ( uiCount )
         * uiCount = RDDFUNCSCOUNT;
      errCode = hb_rddInherit( pTable, &cdxTable, &cdxSuper, "DBFFPT" );
      if ( errCode != SUCCESS )
         errCode = hb_rddInherit( pTable, &cdxTable, &cdxSuper, "DBFDBT" );
      if ( errCode != SUCCESS )
         errCode = hb_rddInherit( pTable, &cdxTable, &cdxSuper, "DBF" );
      if ( errCode == SUCCESS )
      {
         /*
          * we successfully register our RDD so now we can initialize it
          * You may think that this place is RDD init statement, Druzus
          */
         s_uiRddId = uiRddId;
      }
      hb_retni( errCode );
   }
   else
      hb_retni( FAILURE );
}

static void hb_dbfcdxRddInit( void * cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   if( hb_rddRegister( "DBF", RDT_FULL ) <= 1 )
   {
      hb_rddRegister( "DBFFPT", RDT_FULL );
      if( hb_rddRegister( "DBFCDX", RDT_FULL ) <= 1 )
      {
         return;
      }
   }

   hb_errInternal( HB_EI_RDDINVALID, NULL, NULL, NULL );

   /* not executed, only to force DBF RDD linking */
   HB_FUNC_EXEC( _DBF );
}

HB_INIT_SYMBOLS_BEGIN( dbfcdx1__InitSymbols )
{ "DBFCDX",              {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( DBFCDX )}, NULL },
{ "DBFCDX_GETFUNCTABLE", {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( DBFCDX_GETFUNCTABLE )}, NULL }
HB_INIT_SYMBOLS_END( dbfcdx1__InitSymbols )

#endif

HB_CALL_ON_STARTUP_BEGIN( _hb_dbfcdx_rdd_init_ )
   hb_vmAtInit( hb_dbfcdxRddInit, NULL );
HB_CALL_ON_STARTUP_END( _hb_dbfcdx_rdd_init_ )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup dbfcdx1__InitSymbols
   #pragma startup _hb_dbfcdx_rdd_init_
#elif defined( HB_MSC_STARTUP )
   #if defined( HB_OS_WIN_64 )
      #pragma section( HB_MSC_START_SEGMENT, long, read )
   #endif
   #pragma data_seg( HB_MSC_START_SEGMENT )
   static HB_$INITSYM hb_vm_auto_dbfcdx1__InitSymbols = dbfcdx1__InitSymbols;
   static HB_$INITSYM hb_vm_auto_dbfcdx_rdd_init = _hb_dbfcdx_rdd_init_;
   #pragma data_seg()
#endif
