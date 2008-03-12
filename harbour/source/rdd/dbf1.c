/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * DBF RDD module
 *
 * Copyright 1999 Bruno Cantero <bruno@issnet.net>
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

#define HB_TRIGVAR_BYREF

#include "hbapi.h"
#include "hbinit.h"
#include "hbvm.h"
#include "hbapiitm.h"
#include "hbrdddbf.h"
#include "hbdbf.h"
#include "hbapierr.h"
#include "hbapilng.h"
#include "hbset.h"
#include "hbdate.h"
#include "hbmath.h"
#include "hbdbsort.h"
#include "hbsxfunc.h"
#include "error.ch"
#include "rddsys.ch"
#include "hbsxdef.ch"

#ifndef HB_CDP_SUPPORT_OFF
#  include "hbapicdp.h"
#endif

static USHORT s_uiRddId = ( USHORT ) -1;
static RDDFUNCS dbfSuper;
static const RDDFUNCS dbfTable = { ( DBENTRYP_BP ) hb_dbfBof,
                                   ( DBENTRYP_BP ) hb_dbfEof,
                                   ( DBENTRYP_BP ) hb_dbfFound,
                                   ( DBENTRYP_V ) hb_dbfGoBottom,
                                   ( DBENTRYP_UL ) hb_dbfGoTo,
                                   ( DBENTRYP_I ) hb_dbfGoToId,
                                   ( DBENTRYP_V ) hb_dbfGoTop,
                                   ( DBENTRYP_BIB ) hb_dbfSeek,
                                   ( DBENTRYP_L ) hb_dbfSkip,
                                   ( DBENTRYP_L ) hb_dbfSkipFilter,
                                   ( DBENTRYP_L ) hb_dbfSkipRaw,
                                   ( DBENTRYP_VF ) hb_dbfAddField,
                                   ( DBENTRYP_B ) hb_dbfAppend,
                                   ( DBENTRYP_I ) hb_dbfCreateFields,
                                   ( DBENTRYP_V ) hb_dbfDeleteRec,
                                   ( DBENTRYP_BP ) hb_dbfDeleted,
                                   ( DBENTRYP_SP ) hb_dbfFieldCount,
                                   ( DBENTRYP_VF ) hb_dbfFieldDisplay,
                                   ( DBENTRYP_SSI ) hb_dbfFieldInfo,
                                   ( DBENTRYP_SVP ) hb_dbfFieldName,
                                   ( DBENTRYP_V ) hb_dbfFlush,
                                   ( DBENTRYP_PP ) hb_dbfGetRec,
                                   ( DBENTRYP_SI ) hb_dbfGetValue,
                                   ( DBENTRYP_SVL ) hb_dbfGetVarLen,
                                   ( DBENTRYP_V ) hb_dbfGoCold,
                                   ( DBENTRYP_V ) hb_dbfGoHot,
                                   ( DBENTRYP_P ) hb_dbfPutRec,
                                   ( DBENTRYP_SI ) hb_dbfPutValue,
                                   ( DBENTRYP_V ) hb_dbfRecall,
                                   ( DBENTRYP_ULP ) hb_dbfRecCount,
                                   ( DBENTRYP_ISI ) hb_dbfRecInfo,
                                   ( DBENTRYP_ULP ) hb_dbfRecNo,
                                   ( DBENTRYP_I ) hb_dbfRecId,
                                   ( DBENTRYP_S ) hb_dbfSetFieldExtent,
                                   ( DBENTRYP_P ) hb_dbfAlias,
                                   ( DBENTRYP_V ) hb_dbfClose,
                                   ( DBENTRYP_VP ) hb_dbfCreate,
                                   ( DBENTRYP_SI ) hb_dbfInfo,
                                   ( DBENTRYP_V ) hb_dbfNewArea,
                                   ( DBENTRYP_VP ) hb_dbfOpen,
                                   ( DBENTRYP_V ) hb_dbfRelease,
                                   ( DBENTRYP_SP ) hb_dbfStructSize,
                                   ( DBENTRYP_P ) hb_dbfSysName,
                                   ( DBENTRYP_VEI ) hb_dbfEval,
                                   ( DBENTRYP_V ) hb_dbfPack,
                                   ( DBENTRYP_LSP ) hb_dbfPackRec,
                                   ( DBENTRYP_VS ) hb_dbfSort,
                                   ( DBENTRYP_VT ) hb_dbfTrans,
                                   ( DBENTRYP_VT ) hb_dbfTransRec,
                                   ( DBENTRYP_V ) hb_dbfZap,
                                   ( DBENTRYP_VR ) hb_dbfChildEnd,
                                   ( DBENTRYP_VR ) hb_dbfChildStart,
                                   ( DBENTRYP_VR ) hb_dbfChildSync,
                                   ( DBENTRYP_V ) hb_dbfSyncChildren,
                                   ( DBENTRYP_V ) hb_dbfClearRel,
                                   ( DBENTRYP_V ) hb_dbfForceRel,
                                   ( DBENTRYP_SVP ) hb_dbfRelArea,
                                   ( DBENTRYP_VR ) hb_dbfRelEval,
                                   ( DBENTRYP_SI ) hb_dbfRelText,
                                   ( DBENTRYP_VR ) hb_dbfSetRel,
                                   ( DBENTRYP_OI ) hb_dbfOrderListAdd,
                                   ( DBENTRYP_V ) hb_dbfOrderListClear,
                                   ( DBENTRYP_OI ) hb_dbfOrderListDelete,
                                   ( DBENTRYP_OI ) hb_dbfOrderListFocus,
                                   ( DBENTRYP_V ) hb_dbfOrderListRebuild,
                                   ( DBENTRYP_VOI ) hb_dbfOrderCondition,
                                   ( DBENTRYP_VOC ) hb_dbfOrderCreate,
                                   ( DBENTRYP_OI ) hb_dbfOrderDestroy,
                                   ( DBENTRYP_OII ) hb_dbfOrderInfo,
                                   ( DBENTRYP_V ) hb_dbfClearFilter,
                                   ( DBENTRYP_V ) hb_dbfClearLocate,
                                   ( DBENTRYP_V ) hb_dbfClearScope,
                                   ( DBENTRYP_VPLP ) hb_dbfCountScope,
                                   ( DBENTRYP_I ) hb_dbfFilterText,
                                   ( DBENTRYP_SI ) hb_dbfScopeInfo,
                                   ( DBENTRYP_VFI ) hb_dbfSetFilter,
                                   ( DBENTRYP_VLO ) hb_dbfSetLocate,
                                   ( DBENTRYP_VOS ) hb_dbfSetScope,
                                   ( DBENTRYP_VPL ) hb_dbfSkipScope,
                                   ( DBENTRYP_B ) hb_dbfLocate,
                                   ( DBENTRYP_P ) hb_dbfCompile,
                                   ( DBENTRYP_I ) hb_dbfError,
                                   ( DBENTRYP_I ) hb_dbfEvalBlock,
                                   ( DBENTRYP_VSP ) hb_dbfRawLock,
                                   ( DBENTRYP_VL ) hb_dbfLock,
                                   ( DBENTRYP_I ) hb_dbfUnLock,
                                   ( DBENTRYP_V ) hb_dbfCloseMemFile,
                                   ( DBENTRYP_VP ) hb_dbfCreateMemFile,
                                   ( DBENTRYP_SVPB ) hb_dbfGetValueFile,
                                   ( DBENTRYP_VP ) hb_dbfOpenMemFile,
                                   ( DBENTRYP_SVPB ) hb_dbfPutValueFile,
                                   ( DBENTRYP_V ) hb_dbfReadDBHeader,
                                   ( DBENTRYP_V ) hb_dbfWriteDBHeader,
                                   ( DBENTRYP_R ) hb_dbfInit,
                                   ( DBENTRYP_R ) hb_dbfExit,
                                   ( DBENTRYP_RVV ) hb_dbfDrop,
                                   ( DBENTRYP_RVV ) hb_dbfExists,
                                   ( DBENTRYP_RSLV ) hb_dbfRddInfo,
                                   ( DBENTRYP_SVP ) hb_dbfWhoCares
                                 };

/*
 * Common functions.
 */

#define HB_BLANK_APPEND       1
#define HB_BLANK_EOF          2
#define HB_BLANK_ROLLBACK     3

#define HB_BLANK_SKIP         100
#define HB_BLANK_AUTOINC      101

static HB_LONG hb_dbfGetRowVer( DBFAREAP pArea, USHORT uiField, HB_LONG * pValue )
{
   DBFFIELD dbField;
   BOOL fLck = FALSE;

   *pValue = 0;
   if( pArea->fShared && !pArea->fFLocked && !pArea->fHeaderLocked )
   {
      if( SELF_RAWLOCK( ( AREAP ) pArea, HEADER_LOCK, 0 ) != SUCCESS )
         return FAILURE;
      fLck = TRUE;
   }

   hb_fsSeek( pArea->hDataFile, sizeof( DBFHEADER ) +
                                uiField * sizeof( DBFFIELD ), FS_SET );
   if( hb_fsRead( pArea->hDataFile, ( BYTE * ) &dbField,
                  sizeof( dbField ) ) == sizeof( dbField ) )
   {
      *pValue = HB_GET_LE_UINT64( &dbField.bReserved2 ) + 1;
      HB_PUT_LE_UINT64( dbField.bReserved2, *pValue );
      hb_fsSeek( pArea->hDataFile, sizeof( DBFHEADER ) +
                                   uiField * sizeof( DBFFIELD ), FS_SET );
      hb_fsWrite( pArea->hDataFile, ( BYTE * ) &dbField, sizeof( dbField ) );
   }

   if( fLck )
   {
      if( SELF_RAWLOCK( ( AREAP ) pArea, HEADER_UNLOCK, 0 ) != SUCCESS )
         return FAILURE;
   }

   return SUCCESS;
}

static HB_LONG hb_dbfGetNextValue( DBFAREAP pArea, USHORT uiField )
{
   HB_LONG nValue = 0;
   DBFFIELD dbField;

   hb_fsSeek( pArea->hDataFile, sizeof( DBFHEADER ) +
                                uiField * sizeof( DBFFIELD ), FS_SET );
   if( hb_fsRead( pArea->hDataFile, ( BYTE * ) &dbField,
                  sizeof( dbField ) ) == sizeof( dbField ) )
   {
      nValue = HB_GET_LE_UINT32( dbField.bCounter );
      HB_PUT_LE_UINT32( dbField.bCounter, nValue + dbField.bStep );
      hb_fsSeek( pArea->hDataFile, sizeof( DBFHEADER ) +
                                   uiField * sizeof( DBFFIELD ), FS_SET );
      hb_fsWrite( pArea->hDataFile, ( BYTE * ) &dbField, sizeof( dbField ) );
   }

   return nValue;
}

static void hb_dbfUpdateStampFields( DBFAREAP pArea )
{
   LONG lJulian = 0, lMilliSec = 0;
   HB_LONG nRowVer = 0;
   LPFIELD pField;
   USHORT uiCount;

   for( uiCount = 0, pField = pArea->lpFields; uiCount < pArea->uiFieldCount; uiCount++, pField++ )
   {
      switch( pField->uiType )
      {
         case HB_FT_MODTIME:
         {
            BYTE * pPtr = pArea->pRecord + pArea->pFieldOffset[ uiCount ];
            if( lJulian == 0 )
               hb_dateTimeStamp( &lJulian, &lMilliSec );
            HB_PUT_LE_UINT32( pPtr, lJulian );
            pPtr += 4;
            HB_PUT_LE_UINT32( pPtr, lMilliSec );
            break;
         }
         case HB_FT_ROWVER:
         {
            BYTE * pPtr = pArea->pRecord + pArea->pFieldOffset[ uiCount ];
            if( nRowVer == 0 )
               hb_dbfGetRowVer( pArea, uiCount, &nRowVer );
            HB_PUT_LE_UINT64( pPtr, nRowVer );
            break;
         }
      }
   }
}

static void hb_dbfSetBlankRecord( DBFAREAP pArea, int iType )
{
   BYTE *pPtr = pArea->pRecord, bFill = ' ', bNext;
   ULONG ulSize = 1; /* 1 byte ' ' for DELETE flag */
   USHORT uiCount;
   LPFIELD pField;

   for( uiCount = 0, pField = pArea->lpFields; uiCount < pArea->uiFieldCount; uiCount++, pField++ )
   {
      USHORT uiLen = pField->uiLen;

      switch( pField->uiType )
      {
         case HB_FT_MEMO:
         case HB_FT_IMAGE:
         case HB_FT_BLOB:
         case HB_FT_OLE:
            bNext = uiLen == 10 ? ' ' : '\0';
            break;

         case HB_FT_DATE:
            bNext = uiLen == 8 ? ' ' : '\0';
            break;

         case HB_FT_STRING:
         case HB_FT_LOGICAL:
            bNext = ' ';
            break;

         case HB_FT_LONG:
         case HB_FT_FLOAT:
            if( pField->uiFlags & HB_FF_AUTOINC )
            {
               if( iType == HB_BLANK_APPEND )
               {
                  bNext = HB_BLANK_AUTOINC;
                  break;
               }
               else if( iType == HB_BLANK_ROLLBACK )
               {
                  bNext = HB_BLANK_SKIP;
                  break;
               }
            }
            bNext = ' ';
            break;

         case HB_FT_AUTOINC:
            if( iType == HB_BLANK_APPEND )
               bNext = HB_BLANK_AUTOINC;
            else if( iType == HB_BLANK_ROLLBACK )
               bNext = HB_BLANK_SKIP;
            else
               bNext = '\0';
            break;

         case HB_FT_INTEGER:
         case HB_FT_DOUBLE:
            if( pField->uiFlags & HB_FF_AUTOINC )
            {
               if( iType == HB_BLANK_APPEND )
               {
                  bNext = HB_BLANK_AUTOINC;
                  break;
               }
               else if( iType == HB_BLANK_ROLLBACK )
               {
                  bNext = HB_BLANK_SKIP;
                  break;
               }
            }
            bNext = '\0';
            break;

         default:
            bNext = '\0';
            break;
      }

      if( bNext == bFill )
      {
         ulSize += uiLen;
      }
      else
      {
         memset( pPtr, bFill, ulSize );
         pPtr += ulSize;
         ulSize = 0;
         if( bNext == HB_BLANK_SKIP )
         {
            pPtr += uiLen;
         }
         else if( bNext == HB_BLANK_AUTOINC )
         {
            HB_LONG nValue = hb_dbfGetNextValue( pArea, uiCount );
            if( pField->uiDec )
               nValue = ( HB_LONG ) hb_numDecConv( ( double ) nValue, - ( int ) pField->uiDec );
            if( pField->uiType == HB_FT_INTEGER ||
                pField->uiType == HB_FT_AUTOINC )
            {
               if( uiLen == 1 )
                  *pPtr = ( signed char ) nValue;
               else if( uiLen == 2 )
                  HB_PUT_LE_UINT16( pPtr, nValue );
               else if( uiLen == 3 )
                  HB_PUT_LE_UINT24( pPtr, nValue );
               else if( uiLen == 4 )
                  HB_PUT_LE_UINT32( pPtr, nValue );
               else if( uiLen == 8 )
                  HB_PUT_LE_UINT64( pPtr, nValue );
            }
            else if( pField->uiType == HB_FT_DOUBLE )
            {
               HB_PUT_LE_DOUBLE( pPtr, nValue );
            }
            else
            {
               USHORT ui = uiLen;
               do
               {
                  pPtr[ --ui ] = ( BYTE ) nValue % 10 + '0';
                  nValue /= 10;
               }
               while( ui && nValue >= 1 );
            }
            pPtr += uiLen;
         }
         else
         {
            ulSize = uiLen;
            bFill = bNext;
         }
      }
   }
   memset( pPtr, bFill, ulSize );

   ulSize = pArea->pRecord - pPtr - ulSize;
   if( ulSize < ( ULONG ) pArea->uiRecordLen )
      memset( pPtr, '\0', ( ULONG ) pArea->uiRecordLen - ulSize );
}

/*
 * Executes user trigger function
 */
static BOOL hb_dbfTriggerDo( DBFAREAP pArea, int iEvent,
                             int iField, PHB_ITEM pItem )
{
   BOOL fResult = TRUE;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfTriggerDo(%p,%d,%d,%p)", pArea, iEvent, iField, pItem));

   if( hb_vmRequestQuery() == 0 )
   {
      if( hb_vmRequestReenter() )
      {
         hb_vmPushDynSym( pArea->pTriggerSym );
         hb_vmPushNil();
         /* nEvent */
         hb_vmPushInteger( iEvent );
         /* nArea */
         hb_vmPushInteger( pArea->uiArea );
         /* nFieldPos (GET/PUT) */
         hb_vmPushInteger( iField );
         /* xTrigVal (PREUSE/GET/PUT) */
         if( pItem )
         {
#ifdef HB_TRIGVAR_BYREF
            hb_vmPushItemRef( pItem );
#else
            hb_vmPush( pItem );
#endif
            hb_vmDo( 4 );
         }
         else
         {
            /* SIx3 makes: hb_vmPushInteger( 0 ); */
            hb_vmDo( 3 );
         }
         fResult = hb_parl( -1 );
         hb_vmRequestRestore();
      }
   }

   return fResult;
}

/*
 * Set user trigger function
 */
static void hb_dbfTriggerSet( DBFAREAP pArea, PHB_ITEM pTrigger )
{
   char * szName;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfTriggerSet(%p,%p)", pArea, pTrigger));

   szName = hb_itemGetCPtr( pTrigger );
   pArea->pTriggerSym = *szName ? hb_dynsymFindName( szName ) : NULL;
   if( pArea->pTriggerSym && !hb_dynsymIsFunction( pArea->pTriggerSym ) )
      pArea->pTriggerSym = NULL;
   pArea->fTrigger = pArea->pTriggerSym != NULL;
}

/*
 * Return the total number of records.
 */
static ULONG hb_dbfCalcRecCount( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfCalcRecCount(%p)", pArea));

   if( pArea->hDataFile == FS_ERROR )
      return 0;
   else
      return ( ULONG ) ( ( hb_fsSeekLarge( pArea->hDataFile, 0, FS_END ) -
                           pArea->uiHeaderLen ) / pArea->uiRecordLen );
}

/*
 * Read current record from file.
 */
static BOOL hb_dbfReadRecord( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfReadRecord(%p)", pArea));

   if( !pArea->fPositioned )
   {
      pArea->fValidBuffer = TRUE;
      return TRUE;
   }

   if( pArea->ulRecNo > pArea->ulRecCount )
   {
      /* Update record count */
      if( pArea->fShared )
         pArea->ulRecCount = hb_dbfCalcRecCount( pArea );

      if( pArea->ulRecNo > pArea->ulRecCount )
      {
         pArea->fEof = pArea->fValidBuffer = TRUE;
         return TRUE;
      }
   }

   /* Read data from file */
   hb_fsSeekLarge( pArea->hDataFile, ( HB_FOFFSET ) pArea->uiHeaderLen +
                   ( HB_FOFFSET ) ( pArea->ulRecNo - 1 ) *
                   ( HB_FOFFSET ) pArea->uiRecordLen, FS_SET );
   if( hb_fsRead( pArea->hDataFile, pArea->pRecord, pArea->uiRecordLen ) !=
       pArea->uiRecordLen )
   {
      PHB_ITEM pError = hb_errNew();

      hb_errPutGenCode( pError, EG_READ );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_READ ) );
      hb_errPutSubCode( pError, EDBF_READ );
      hb_errPutOsCode( pError, hb_fsError() );
      hb_errPutFileName( pError, pArea->szDataFileName );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FALSE;
   }

   if( SELF_GETREC( ( AREAP ) pArea, NULL ) == FAILURE )
      return FALSE;

   /* Set flags */
   pArea->fValidBuffer = pArea->fPositioned = TRUE;
   pArea->fDeleted = pArea->pRecord[ 0 ] == '*';
   return TRUE;
}

/*
 * Write current record to file.
 */
static BOOL hb_dbfWriteRecord( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfWriteRecord(%p)", pArea));

   if( SELF_PUTREC( ( AREAP ) pArea, NULL ) == FAILURE )
      return FALSE;

   pArea->fRecordChanged = FALSE;
   pArea->fDataFlush = TRUE;
   return TRUE;
}

/*
 * Set encryption password
 */
static BOOL hb_dbfPasswordSet( DBFAREAP pArea, PHB_ITEM pPasswd, BOOL fRaw )
{
   BYTE byBuffer[ 8 ];
   ULONG ulLen;
   BOOL fKeySet = FALSE, fSet;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfPasswordSet(%p,%p,%d)", pArea, pPasswd, fRaw));

   ulLen = hb_itemGetCLen( pPasswd );

   fSet = !pArea->fHasMemo && HB_IS_STRING( pPasswd ) && (!fRaw || ulLen == 8);
   if( fSet )
   {
      ulLen = hb_itemGetCLen( pPasswd );
      if( ulLen > 0 )
      {
         if( ulLen < 8 )
         {
            memcpy( byBuffer, hb_itemGetCPtr( pPasswd ), ulLen );
            memset( byBuffer + ulLen, '\0', 8 - ulLen );
         }
         else
            memcpy( byBuffer, hb_itemGetCPtr( pPasswd ), 8 );
      }
   }

   if( pArea->pCryptKey )
      hb_itemPutCL( pPasswd, ( char * ) pArea->pCryptKey, 8 );
   else
      hb_itemClear( pPasswd );

   if( fSet )
   {
      if( pArea->pRecord && pArea->fPositioned )
      {
         SELF_GOCOLD( ( AREAP ) pArea );
         pArea->fValidBuffer = FALSE;
      }
      if( pArea->pCryptKey )
      {
         /* clean the memory with password key - though it's not
          * a serious actions in such case ;-)
          */
         memset( pArea->pCryptKey, '\0', 8 );
         hb_xfree( pArea->pCryptKey );
         pArea->pCryptKey = NULL;
      }
      if( ulLen > 0 )
      {
         /* at this moment only one encryption method is used,
            I'll add other later, [druzus] */
         pArea->bCryptType = DB_CRYPT_SIX;
         pArea->pCryptKey = ( BYTE * ) hb_xgrab( 8 );

         /* SIX encode the key with its own value before use */
         if( !fRaw )
            hb_sxEnCrypt( byBuffer, pArea->pCryptKey, byBuffer, 8 );
         else
            memcpy( pArea->pCryptKey, byBuffer, 8 );
         fKeySet = TRUE;
      }
   }

   return fKeySet;
}

/*
 * Encrypt/Decrypt table
 */
static void hb_dbfTableCrypt( DBFAREAP pArea, PHB_ITEM pPasswd, BOOL fEncrypt )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfTableCrypt(%p,%p,%d)", pArea, pPasswd, fEncrypt));

   if( !pArea->fReadonly && !pArea->fShared &&
       fEncrypt ? !pArea->fTableEncrypted && !pArea->fHasMemo :
                   pArea->fTableEncrypted )
   {
      ULONG ulRecords, ulRecNo;

      if( SELF_RECCOUNT( ( AREAP ) pArea, &ulRecords ) == SUCCESS )
      {
         ERRCODE errCode = SUCCESS;
         BYTE * pOldCryptKey, * pNewCryptKey;

         pOldCryptKey = pArea->pCryptKey;
         pArea->pCryptKey = NULL;
         hb_dbfPasswordSet( pArea, pPasswd, FALSE );
         pNewCryptKey = pArea->pCryptKey;
         if( !fEncrypt && pNewCryptKey )
         {
            if( pOldCryptKey )
               hb_xfree( pNewCryptKey );
            else
               pOldCryptKey = pNewCryptKey;
            pNewCryptKey = NULL;
         }
         for( ulRecNo = 1; ulRecNo <= ulRecords; ++ulRecNo )
         {
            pArea->pCryptKey = pOldCryptKey;
            errCode = SELF_GOTO( ( AREAP ) pArea, ulRecNo );
            if( errCode != SUCCESS )
               break;
            if( !hb_dbfReadRecord( pArea ) )
            {
               errCode = FAILURE;
               break;
            }
            pArea->pCryptKey = pNewCryptKey;
            /* Buffer is hot? */
            if( !pArea->fRecordChanged )
            {
               errCode = SELF_GOHOT( ( AREAP ) pArea );
               if( errCode != SUCCESS )
                  break;
            }
            /* Force record encryption/decryption */
            pArea->fEncrypted = fEncrypt;
            /* Save encrypted record */
            errCode = SELF_GOCOLD( ( AREAP ) pArea );
            if( errCode != SUCCESS )
               break;
         }
         pArea->pCryptKey = pNewCryptKey;
         if( pOldCryptKey )
            hb_xfree( pOldCryptKey );
         if( errCode == SUCCESS )
         {
            pArea->fTableEncrypted = fEncrypt;
            pArea->fUpdateHeader = TRUE;
            SELF_WRITEDBHEADER( ( AREAP ) pArea );
         }
      }
   }
}

/*
 * Unlock all records.
 */
static ERRCODE hb_dbfUnlockAllRecords( DBFAREAP pArea )
{
   ERRCODE uiError = SUCCESS;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfUnlockAllRecords(%p, %p)", pArea ));

   if( pArea->pLocksPos )
   {
      ULONG ulCount;

      uiError = SELF_GOCOLD( ( AREAP ) pArea );
      for( ulCount = 0; ulCount < pArea->ulNumLocksPos; ulCount++ )
         SELF_RAWLOCK( ( AREAP ) pArea, REC_UNLOCK, pArea->pLocksPos[ ulCount ] );
      hb_xfree( pArea->pLocksPos );
      pArea->pLocksPos = NULL;
   }
   pArea->ulNumLocksPos = 0;
   return uiError;
}

/*
 * Unlock a records.
 */
static ERRCODE hb_dbfUnlockRecord( DBFAREAP pArea, ULONG ulRecNo )
{
   ERRCODE uiError = SUCCESS;
   ULONG ulCount, * pList;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfUnlockRecord(%p, %lu)", pArea, ulRecNo));

   /* Search the locked record */
   for( ulCount = 0; ulCount < pArea->ulNumLocksPos &&
                     pArea->pLocksPos[ ulCount ] != ulRecNo; ulCount++ ) {}

   if( ulCount < pArea->ulNumLocksPos )
   {
      uiError = SELF_GOCOLD( ( AREAP ) pArea );
      SELF_RAWLOCK( ( AREAP ) pArea, REC_UNLOCK, ulRecNo );
      if( pArea->ulNumLocksPos == 1 )            /* Delete the list */
      {
         hb_xfree( pArea->pLocksPos );
         pArea->pLocksPos = NULL;
         pArea->ulNumLocksPos = 0;
      }
      else                                       /* Resize the list */
      {
         pList = pArea->pLocksPos + ulCount;
         memmove( pList, pList + 1, ( pArea->ulNumLocksPos - ulCount - 1 ) *
                  sizeof( ULONG ) );
         pArea->pLocksPos = ( ULONG * ) hb_xrealloc( pArea->pLocksPos,
                                                     ( pArea->ulNumLocksPos - 1 ) *
                                                     sizeof( ULONG ) );
         pArea->ulNumLocksPos --;
      }
   }
   return uiError;
}

/*
 * Lock a record.
 */
static ERRCODE hb_dbfLockRecord( DBFAREAP pArea, ULONG ulRecNo, BOOL * pResult,
                                 BOOL bExclusive )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfLockRecord(%p, %lu, %p, %i)", pArea, ulRecNo,
            pResult, (int) bExclusive));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   if( pArea->fFLocked )
   {
      * pResult = TRUE;
      return SUCCESS;
   }

   if( ulRecNo == 0 )
      ulRecNo = pArea->ulRecNo;

   if( bExclusive )
   {
      hb_dbfUnlockAllRecords( pArea );
   }
   else if( pArea->ulNumLocksPos > 0 )
   {
      ULONG ul;
      for( ul = 0; ul < pArea->ulNumLocksPos; ul++ )
      {
         if( pArea->pLocksPos[ ul ] == ulRecNo )
         {
            * pResult = TRUE;
            return SUCCESS;
         }
      }
   }

   if( SELF_RAWLOCK( ( AREAP ) pArea, REC_LOCK, ulRecNo ) == SUCCESS )
   {
      if( pArea->ulNumLocksPos == 0 )               /* Create the list */
      {
         pArea->pLocksPos = ( ULONG * ) hb_xgrab( sizeof( ULONG ) );
      }
      else                                          /* Resize the list */
      {
         pArea->pLocksPos = ( ULONG * ) hb_xrealloc( pArea->pLocksPos,
                                                   ( pArea->ulNumLocksPos + 1 ) *
                                                     sizeof( ULONG ) );
      }
      pArea->pLocksPos[ pArea->ulNumLocksPos++ ] = ulRecNo;
      * pResult = TRUE;
      if( ulRecNo == pArea->ulRecNo )
      {
         if( !pArea->fPositioned )
         {
            if( SELF_GOTO( ( AREAP ) pArea, pArea->ulRecNo ) != SUCCESS )
               return FAILURE;
         }
         else if( !pArea->fRecordChanged )
         {
            if( SELF_GOCOLD( ( AREAP ) pArea ) != SUCCESS )
               return FAILURE;
            pArea->fValidBuffer = FALSE;
         }
      }
   }
   else
      * pResult = FALSE;
   return SUCCESS;
}

/*
 * Lock a file.
 */
static ERRCODE hb_dbfLockFile( DBFAREAP pArea, BOOL * pResult )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfLockFile(%p, %p)", pArea, pResult));

   if( !pArea->fFLocked )
   {
      if( pArea->lpdbPendingRel )
      {
         if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
            return FAILURE;
      }

      hb_dbfUnlockAllRecords( pArea );

      SELF_RAWLOCK( ( AREAP ) pArea, FILE_LOCK, 0 );
      * pResult = pArea->fFLocked;

      if( !pArea->fPositioned )
      {
         SELF_GOTO( ( AREAP ) pArea, pArea->ulRecNo );
      }
      else if( !pArea->fRecordChanged )
      {
         SELF_GOCOLD( ( AREAP ) pArea );
         pArea->fValidBuffer = FALSE;
      }
   }
   else
      * pResult = TRUE;

   return SUCCESS;
}

/*
 * Unlock a file.
 */
static ERRCODE hb_dbfUnlockFile( DBFAREAP pArea )
{
   ERRCODE uiError = SUCCESS;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfUnlockFile(%p)", pArea));

   if( pArea->fFLocked )
   {
      uiError = SELF_GOCOLD( ( AREAP ) pArea );
      SELF_RAWLOCK( ( AREAP ) pArea, FILE_UNLOCK, 0 );
   }
   return uiError;
}

/*
 * Test if a record is locked.
 */
static BOOL hb_dbfIsLocked( DBFAREAP pArea, ULONG ulRecNo )
{
   ULONG ulCount;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfIsLocked(%p)", pArea));

   ulCount = pArea->ulNumLocksPos;
   while( ulCount > 0 )
   {
      if( pArea->pLocksPos[ ulCount - 1 ] == ulRecNo )
         return TRUE;
      ulCount --;
   }

   return FALSE;
}

/*
 * Return an array filled all locked records.
 */
static void hb_dbfGetLockArray( DBFAREAP pArea, PHB_ITEM pItem )
{
   ULONG ulCount;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGetLockArray(%p, %p)", pArea, pItem));

   hb_arrayNew( pItem, pArea->ulNumLocksPos );
   for( ulCount = 0; ulCount < pArea->ulNumLocksPos; ulCount++ )
   {
      hb_itemPutNL( hb_arrayGetItemPtr( pItem, ulCount + 1 ), pArea->pLocksPos[ ulCount ] );
   }
}


/*
 * Converts EDBF_* error code into EG_* one.
 * This function is common for different DBF based RDD implementation
 * so I don't make it static
 */
HB_EXPORT ERRCODE hb_dbfGetEGcode( ERRCODE errCode )
{
   ERRCODE errEGcode;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGetEGcode(%hu)", errCode));

   switch( errCode )
   {
      case EDBF_OPEN_DBF:
         errEGcode = EG_OPEN;
         break;
      case EDBF_CREATE_DBF:
         errEGcode = EG_CREATE;
         break;
      case EDBF_READ:
         errEGcode = EG_READ;
         break;
      case EDBF_WRITE:
         errEGcode = EG_WRITE;
         break;
      case EDBF_CORRUPT:
         errEGcode = EG_CORRUPTION;
         break;
      case EDBF_DATATYPE:
         errEGcode = EG_DATATYPE;
         break;
      case EDBF_DATAWIDTH:
         errEGcode = EG_DATAWIDTH;
         break;
      case EDBF_UNLOCKED:
         errEGcode = EG_UNLOCKED;
         break;
      case EDBF_SHARED:
         errEGcode = EG_SHARED;
         break;
      case EDBF_APPENDLOCK:
         errEGcode = EG_APPENDLOCK;
         break;
      case EDBF_READONLY:
         errEGcode = EG_READONLY;
         break;
      case EDBF_LOCK:
         errEGcode = EG_LOCK;
         break;
      case EDBF_INVALIDKEY:
      default:
         errEGcode = EG_UNSUPPORTED;
         break;
   }

   return errEGcode;
}

/*
 * Converts memo block offset into ASCII.
 * This function is common for different MEMO implementation
 * so I left it in DBF.
 */
HB_EXPORT ULONG hb_dbfGetMemoBlock( DBFAREAP pArea, USHORT uiIndex )
{
   ULONG ulBlock= 0;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGetMemoBlock(%p, %hu)", pArea, uiIndex));

   if( pArea->lpFields[ uiIndex ].uiLen == 4 )
   {
      ulBlock =  HB_GET_LE_UINT32( &pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ] );
   }
   else
   {
      USHORT uiCount;
      BYTE bByte;

      for( uiCount = 0; uiCount < 10; uiCount++ )
      {
         bByte = pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] + uiCount ];
         if( bByte >= '0' && bByte <= '9' )
         {
            ulBlock = ulBlock * 10 + ( bByte - '0' );
         }
      }
   }

   return ulBlock;
}

/*
 * Converts ASCII data into memo block offset.
 * This function is common for different MEMO implementation
 * so I left it in DBF.
 */
HB_EXPORT void hb_dbfPutMemoBlock( DBFAREAP pArea, USHORT uiIndex, ULONG ulBlock )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfPutMemoBlock(%p, %hu, %lu)", pArea, uiIndex, ulBlock));

   if( pArea->lpFields[ uiIndex ].uiLen == 4 )
   {
      HB_PUT_LE_UINT32( &pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ], ulBlock );
   }
   else
   {
      SHORT iCount;

      for( iCount = 9; iCount >= 0; iCount-- )
      {
         if( ulBlock > 0 )
         {
            pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] + iCount ] = ( BYTE )( ulBlock % 10 ) + '0';
            ulBlock /= 10;
         }
         else
         {
            pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] + iCount ] = ' ';
         }
      }
   }
}

/*
 * Retrive memo field information stored in DBF file
 * This function is common for different MEMO implementation
 * so I left it in DBF.
 */
HB_EXPORT ERRCODE hb_dbfGetMemoData( DBFAREAP pArea, USHORT uiIndex,
                                     ULONG * pulBlock, ULONG * pulSize,
                                     ULONG * pulType )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGetMemoData(%p, %hu, %p, %p, %p)", pArea, uiIndex, pulBlock, pulSize, pulType));

   *pulBlock = *pulSize = *pulType = 0;

   if( uiIndex >= pArea->uiFieldCount ||
       ( pArea->lpFields[ uiIndex ].uiType != HB_FT_MEMO &&
         pArea->lpFields[ uiIndex ].uiType != HB_FT_IMAGE &&
         pArea->lpFields[ uiIndex ].uiType != HB_FT_BLOB &&
         pArea->lpFields[ uiIndex ].uiType != HB_FT_OLE ) )
      return FAILURE;

   if( pArea->lpFields[ uiIndex ].uiLen == 4 )
   {
      *pulBlock = HB_GET_LE_UINT32( &pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ] );
      return SUCCESS;
   }
   else if( pArea->lpFields[ uiIndex ].uiLen == 10 )
   {
      ULONG ulValue;

      if( pArea->bMemoType == DB_MEMO_SMT )
      {
         LPSMTFIELD pSMTFiled = ( LPSMTFIELD ) &pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ];

         ulValue = HB_GET_LE_UINT16( pSMTFiled->type );
         if( ulValue != 0x2020 )
         {
            *pulType  = ulValue;
            *pulSize  = HB_GET_LE_UINT32( pSMTFiled->length );
            *pulBlock = HB_GET_LE_UINT32( pSMTFiled->block );
         }
      }
      /*
       * check for NULL fields created by Access, they have chr(0) set
       * in the whole memo block address, [druzus]
       */
      else if( pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ] != 0 )
      {
         USHORT uiCount;
         BYTE bByte;

         ulValue = 0;
         for( uiCount = 0; uiCount < 10; uiCount++ )
         {
            bByte = pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] + uiCount ];
            if( bByte >= '0' && bByte <= '9' )
               ulValue = ulValue * 10 + ( bByte - '0' );
            else if( bByte != ' ' || ulValue )
            {
               PHB_ITEM pError = hb_errNew();
               ERRCODE uiError;

               hb_errPutGenCode( pError, EG_CORRUPTION );
               hb_errPutSubCode( pError, EDBF_CORRUPT );
               hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CORRUPTION ) );
               hb_errPutFileName( pError, pArea->szDataFileName );
               hb_errPutFlags( pError, EF_CANDEFAULT );
               uiError = SELF_ERROR( ( AREAP ) pArea, pError );
               hb_itemRelease( pError );

               return uiError == E_DEFAULT ? SUCCESS : FAILURE;
            }
         }
         *pulBlock = ulValue;
      }
      return SUCCESS;
   }

   return FAILURE;
}

/*
 * Write memo data information into  memo field in DBF file
 * This function is common for different MEMO implementation
 * so I left it in DBF.
 */
HB_EXPORT ERRCODE hb_dbfSetMemoData( DBFAREAP pArea, USHORT uiIndex,
                                     ULONG ulBlock, ULONG ulSize, ULONG ulType )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfSetMemoData(%p, %hu, %lu, %lu, %lu)", pArea, uiIndex, ulBlock, ulSize, ulType));

   if( uiIndex >= pArea->uiFieldCount ||
       ( pArea->lpFields[ uiIndex ].uiType != HB_FT_MEMO &&
         pArea->lpFields[ uiIndex ].uiType != HB_FT_IMAGE &&
         pArea->lpFields[ uiIndex ].uiType != HB_FT_BLOB &&
         pArea->lpFields[ uiIndex ].uiType != HB_FT_OLE ) )
      return FAILURE;

   if( pArea->lpFields[ uiIndex ].uiLen == 4 )
   {
      HB_PUT_LE_UINT32( &pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ], ulBlock );
      return SUCCESS;
   }
   else if( pArea->lpFields[ uiIndex ].uiLen == 10 )
   {
      if( pArea->bMemoType == DB_MEMO_SMT )
      {
         LPSMTFIELD pSMTFiled = ( LPSMTFIELD ) &pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ];

         HB_PUT_LE_UINT16( pSMTFiled->type,   ulType );
         HB_PUT_LE_UINT32( pSMTFiled->length, ulSize );
         HB_PUT_LE_UINT32( pSMTFiled->block,  ulBlock );
      }
      else
      {
         SHORT iCount;

         for( iCount = 9; iCount >= 0; iCount-- )
         {
            if( ulBlock > 0 )
            {
               pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] + iCount ] = ( BYTE )( ulBlock % 10 ) + '0';
               ulBlock /= 10;
            }
            else
            {
               pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] + iCount ] = ' ';
            }
         }
      }
      return SUCCESS;
   }

   return FAILURE;
}

/*
 * Get information about locking schemes for additional files (MEMO, INDEX)
 * This function is common for different MEMO implementation
 * so I left it in DBF.
 */
HB_EXPORT BOOL hb_dbfLockIdxGetData( BYTE bScheme, HB_FOFFSET *ulPos, HB_FOFFSET *ulPool )
{
   switch( bScheme )
   {
      case DB_DBFLOCK_CLIP:
         *ulPos  = IDX_LOCKPOS_CLIP;
         *ulPool = IDX_LOCKPOOL_CLIP;
         break;

      case DB_DBFLOCK_CL53:
         *ulPos  = IDX_LOCKPOS_CL53;
         *ulPool = IDX_LOCKPOOL_CL53;
         break;

      case DB_DBFLOCK_CL53EXT:
         *ulPos  = IDX_LOCKPOS_CL53EXT;
         *ulPool = IDX_LOCKPOOL_CL53EXT;
         break;

      case DB_DBFLOCK_VFP:
         *ulPos  = IDX_LOCKPOS_VFP;
         *ulPool = IDX_LOCKPOOL_VFP;
         break;

#ifndef HB_LONG_LONG_OFF
      case DB_DBFLOCK_XHB64:
         *ulPos  = IDX_LOCKPOS_XHB64;
         *ulPool = IDX_LOCKPOOL_XHB64;
         break;
#endif

      default:
         return FALSE;
   }
   return TRUE;
}

/*
 * Set lock using current locking schemes in additional files (MEMO, INDEX)
 * This function is common for different MEMO implementation
 * so I left it in DBF.
 */
HB_EXPORT BOOL hb_dbfLockIdxFile( FHANDLE hFile, BYTE bScheme, USHORT usMode, HB_FOFFSET *pPoolPos )
{
   HB_FOFFSET ulPos, ulPool, ulSize = 1;
   BOOL fRet = FALSE, fWait;

   if( !hb_dbfLockIdxGetData( bScheme, &ulPos, &ulPool ) )
   {
      return fRet;
   }

   do
   {
      switch( usMode & FL_MASK )
      {
         case FL_LOCK:
            if( ulPool )
            {
               if( ( usMode & FLX_SHARED ) != 0 )
                  *pPoolPos = ( HB_FOFFSET ) ( hb_random_num() * ulPool ) + 1;
               else
               {
                  *pPoolPos = 0;
                  ulSize = ulPool + 1;
               }
            }
            else
            {
               *pPoolPos = 0;
            }
            break;

         case FL_UNLOCK:
            if( ulPool )
            {
               if( ! *pPoolPos )
                  ulSize = ulPool + 1;
            }
            else
            {
               *pPoolPos = 0;
            }
            break;

         default:
            return FALSE;
      }
      fRet = hb_fsLockLarge( hFile, ulPos + *pPoolPos, ulSize, usMode );
      fWait = ( !fRet && ( usMode & FLX_WAIT ) != 0 && ( usMode & FL_MASK ) == FL_LOCK );
      /* TODO: call special error handler (LOCKHANDLER) here if fWait */

   } while( fWait );

   return fRet;
}

/*
 * Get DBF locking parameters
 */
static ERRCODE hb_dbfLockData( DBFAREAP pArea,
                               HB_FOFFSET * ulPos, HB_FOFFSET * ulFlSize,
                               HB_FOFFSET * ulRlSize, int * iDir )
{
   switch( pArea->bLockType )
   {
      case DB_DBFLOCK_CLIP:
         *ulPos = DBF_LOCKPOS_CLIP;
         *iDir = DBF_LOCKDIR_CLIP;
         *ulFlSize = DBF_FLCKSIZE_CLIP;
         *ulRlSize = DBF_RLCKSIZE_CLIP;
         break;

      case DB_DBFLOCK_CL53:
         *ulPos = DBF_LOCKPOS_CL53;
         *iDir = DBF_LOCKDIR_CL53;
         *ulFlSize = DBF_FLCKSIZE_CL53;
         *ulRlSize = DBF_RLCKSIZE_CL53;
         break;

      case DB_DBFLOCK_CL53EXT:
         *ulPos = DBF_LOCKPOS_CL53EXT;
         *iDir = DBF_LOCKDIR_CL53EXT;
         *ulFlSize = DBF_FLCKSIZE_CL53EXT;
         *ulRlSize = DBF_RLCKSIZE_CL53EXT;
         break;

      case DB_DBFLOCK_VFP:
         if( pArea->fHasTags )
         {
            *ulPos = DBF_LOCKPOS_VFPX;
            *iDir = DBF_LOCKDIR_VFPX;
            *ulFlSize = DBF_FLCKSIZE_VFPX;
            *ulRlSize = DBF_RLCKSIZE_VFPX;
         }
         else
         {
            *ulPos = DBF_LOCKPOS_VFP;
            *iDir = DBF_LOCKDIR_VFP;
            *ulFlSize = DBF_FLCKSIZE_VFP;
            *ulRlSize = DBF_RLCKSIZE_VFP;
         }
         break;

#ifndef HB_LONG_LONG_OFF
      case DB_DBFLOCK_XHB64:
         *ulPos = DBF_LOCKPOS_XHB64;
         *iDir = DBF_LOCKDIR_XHB64;
         *ulFlSize = DBF_FLCKSIZE_XHB64;
         *ulRlSize = DBF_RLCKSIZE_XHB64;
         break;
#endif
      default:
         *ulPos = *ulFlSize = *ulRlSize = 0;
         *iDir = 0;
         return FAILURE;
   }
   return SUCCESS;
}

/*
 * -- DBF METHODS --
 */

/*
 * Determine logical beginning of file.
 */
static ERRCODE hb_dbfBof( DBFAREAP pArea, BOOL * pBof )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfBof(%p, %p)", pArea, pBof));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   * pBof = pArea->fBof;
   return SUCCESS;
}

/*
 * Determine logical end of file.
 */
static ERRCODE hb_dbfEof( DBFAREAP pArea, BOOL * pEof )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfEof(%p, %p)", pArea, pEof));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   * pEof = pArea->fEof;
   return SUCCESS;
}

/*
 * Determine outcome of the last search operation.
 */
static ERRCODE hb_dbfFound( DBFAREAP pArea, BOOL * pFound )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfFound(%p, %p)", pArea, pFound));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   * pFound = pArea->fFound;
   return SUCCESS;
}

/*
 * Position cursor at the last record.
 */
static ERRCODE hb_dbfGoBottom( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGoBottom(%p)", pArea));

   if( SELF_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   /* Update record count */
   if( pArea->fShared )
      pArea->ulRecCount = hb_dbfCalcRecCount( pArea );

   pArea->fTop = FALSE;
   pArea->fBottom = TRUE;
   if( SELF_GOTO( ( AREAP ) pArea, pArea->ulRecCount ) != SUCCESS )
      return FAILURE;

   return SELF_SKIPFILTER( ( AREAP ) pArea, -1 );
}

/*
 * Position cursor at a specific physical record.
 */
static ERRCODE hb_dbfGoTo( DBFAREAP pArea, ULONG ulRecNo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGoTo(%p, %lu)", pArea, ulRecNo));

   if( SELF_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if( pArea->lpdbPendingRel )
   {
      if( pArea->lpdbPendingRel->isScoped )
      {
         if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
            return FAILURE;
      }
      else /* Reset parent rel struct */
         pArea->lpdbPendingRel = NULL;
   }

   /* Update record count */
   if( ulRecNo > pArea->ulRecCount && pArea->fShared )
      pArea->ulRecCount = hb_dbfCalcRecCount( pArea );

   if( ulRecNo <= pArea->ulRecCount && ulRecNo >= 1 )
   {
      pArea->ulRecNo = ulRecNo;
      pArea->fBof = pArea->fEof = pArea->fValidBuffer = FALSE;
      pArea->fPositioned = TRUE;
   }
   else /* Out of space */
   {
      pArea->ulRecNo = pArea->ulRecCount + 1;
      pArea->fBof = pArea->fEof = pArea->fValidBuffer = TRUE;
      pArea->fPositioned = pArea->fDeleted = pArea->fEncrypted = FALSE;

      /* Clear record buffer */
      hb_dbfSetBlankRecord( pArea, HB_BLANK_EOF );
   }
   pArea->fFound = FALSE;

   /* Force relational movement in child WorkAreas */
   if( pArea->lpdbRelations )
      return SELF_SYNCCHILDREN( ( AREAP ) pArea );
   else
      return SUCCESS;
}

/*
 * Position the cursor to a specific, physical identity.
 */
static ERRCODE hb_dbfGoToId( DBFAREAP pArea, PHB_ITEM pItem )
{
   PHB_ITEM pError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGoToId(%p, %p)", pArea, pItem));

   if( HB_IS_NUMERIC( pItem ) )
      return SELF_GOTO( ( AREAP ) pArea, hb_itemGetNL( pItem ) );
   else
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_DATATYPE );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_DATATYPE ) );
      hb_errPutSubCode( pError, EDBF_DATATYPE );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }
}

/*
 * Position cursor at the first record.
 */
static ERRCODE hb_dbfGoTop( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGoTop(%p)", pArea));

   pArea->fTop = TRUE;
   pArea->fBottom = FALSE;

   if( SELF_GOTO( ( AREAP ) pArea, 1 ) == FAILURE )
      return FAILURE;

   return SELF_SKIPFILTER( ( AREAP ) pArea, 1 );
}

/*
 * Reposition cursor relative to current position.
 */
static ERRCODE hb_dbfSkip( DBFAREAP pArea, LONG lToSkip )
{
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfSkip(%p, %ld)", pArea, lToSkip));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   pArea->fTop = pArea->fBottom = FALSE;

   if( lToSkip == 0 || hb_set.HB_SET_DELETED ||
       pArea->dbfi.itmCobExpr || pArea->dbfi.fFilter )
      return SUPER_SKIP( ( AREAP ) pArea, lToSkip );

   uiError = SELF_SKIPRAW( ( AREAP ) pArea, lToSkip );

   /* TODO: remove this hack - it's not necessary if SKIPRAW works
      as it should, Druzus */

   /* Move first record and set Bof flag */
   if( uiError == SUCCESS && pArea->fBof && lToSkip < 0 )
   {
      uiError = SELF_GOTOP( ( AREAP ) pArea );
      pArea->fBof = TRUE;
   }

   /* Update Bof and Eof flags */
   if( lToSkip < 0 )
      pArea->fEof = FALSE;
   else /* if( lToSkip > 0 ) */
      pArea->fBof = FALSE;

   return uiError;
}

/*
 * Reposition cursor, regardless of filter.
 */
static ERRCODE hb_dbfSkipRaw( DBFAREAP pArea, LONG lToSkip )
{
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfSkipRaw(%p, %ld)", pArea, lToSkip));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   if( lToSkip == 0 )
   {
      BOOL bBof, bEof;

      /* Save flags */
      bBof = pArea->fBof;
      bEof = pArea->fEof;

      uiError = SELF_GOTO( ( AREAP ) pArea, pArea->ulRecNo );

      /* Restore flags */
      pArea->fBof = bBof;
      pArea->fEof = bEof;
   }
   else if( lToSkip < 0 && ( ULONG ) ( -lToSkip ) >= pArea->ulRecNo )
   {
      uiError = SELF_GOTO( ( AREAP ) pArea, 1 );
      pArea->fBof = TRUE;
   }
   else
   {
      uiError = SELF_GOTO( ( AREAP ) pArea, pArea->ulRecNo + lToSkip );
   }

   return uiError;
}

/*
 * Add a field to the WorkArea.
 */
static ERRCODE hb_dbfAddField( DBFAREAP pArea, LPDBFIELDINFO pFieldInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfAddField(%p, %p)", pArea, pFieldInfo));

   if( pArea->bMemoType == DB_MEMO_SMT &&
       ( pFieldInfo->uiType == HB_FT_MEMO ||
         pFieldInfo->uiType == HB_FT_IMAGE ||
         pFieldInfo->uiType == HB_FT_BLOB ||
         pFieldInfo->uiType == HB_FT_OLE ) )
      pFieldInfo->uiLen = 10;

   /* Update field offset */
   pArea->pFieldOffset[ pArea->uiFieldCount ] = pArea->uiRecordLen;
   pArea->uiRecordLen += pFieldInfo->uiLen;
   return SUPER_ADDFIELD( ( AREAP ) pArea, pFieldInfo );
}

/*
 * Append a record to the WorkArea.
 */
static ERRCODE hb_dbfAppend( DBFAREAP pArea, BOOL bUnLockAll )
{
   ULONG ulNewRecord;
   PHB_ITEM pError;
   BOOL bLocked;
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfAppend(%p, %d)", pArea, (int) bUnLockAll));

   if( SELF_GOCOLD( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   if( pArea->fTrigger )
   {
      if( !hb_dbfTriggerDo( pArea, EVENT_APPEND, 0, NULL ) )
         return FAILURE;
   }

   if( pArea->fReadonly )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_READONLY );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_READONLY ) );
      hb_errPutSubCode( pError, EDBF_READONLY );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }

   if( pArea->lpdbPendingRel )
   {
      if( pArea->lpdbPendingRel->isScoped )
      {
         if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
            return FAILURE;
      }
      else /* Reset parent rel struct */
         pArea->lpdbPendingRel = NULL;
   }

   if( pArea->fShared )
   {
      bLocked = FALSE;
      if( SELF_RAWLOCK( ( AREAP ) pArea, APPEND_LOCK, 0 ) == SUCCESS )
      {
         /* Update RecCount */
         pArea->ulRecCount = hb_dbfCalcRecCount( pArea );
         ulNewRecord = pArea->ulRecCount + 1;
         if( pArea->fFLocked || hb_dbfIsLocked( pArea, ulNewRecord ) )
            bLocked = TRUE;
         else if( hb_dbfLockRecord( pArea, ulNewRecord, &bLocked, bUnLockAll ) != SUCCESS )
         {
            if( bLocked )
               hb_dbfUnlockRecord( pArea, ulNewRecord );
            SELF_RAWLOCK( ( AREAP ) pArea, APPEND_UNLOCK, 0 );
            return FAILURE;
         }
      }
      if( !bLocked )
      {
         SELF_RAWLOCK( ( AREAP ) pArea, APPEND_UNLOCK, 0 );
         pError = hb_errNew();
         hb_errPutGenCode( pError, EG_APPENDLOCK );
         hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_APPENDLOCK ) );
         hb_errPutSubCode( pError, EDBF_APPENDLOCK );
         hb_errPutFlags( pError, EF_CANDEFAULT );
         SELF_ERROR( ( AREAP ) pArea, pError );
         hb_itemRelease( pError );
         return FAILURE;
      }
   }

   /* Clear record buffer and update pArea */
   hb_dbfSetBlankRecord( pArea, HB_BLANK_APPEND );

   pArea->fValidBuffer = pArea->fUpdateHeader = pArea->fRecordChanged =
   pArea->fAppend = pArea->fPositioned = TRUE;
   pArea->ulRecCount ++;
   pArea->ulRecNo = pArea->ulRecCount;
   pArea->fDeleted = pArea->fBof = pArea->fEof = pArea->fFound = FALSE;
   pArea->fEncrypted = pArea->pCryptKey != NULL && !pArea->fHasMemo;

   if( pArea->fShared )
   {
      uiError = SELF_GOCOLD( ( AREAP ) pArea );
      SELF_RAWLOCK( ( AREAP ) pArea, APPEND_UNLOCK, 0 );
      return uiError;
   }
   return SUCCESS;
}

/*
 * Delete a record.
 */
static ERRCODE hb_dbfDeleteRec( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfDeleteRec(%p)", pArea));

   if( pArea->fTrigger )
   {
      if( !hb_dbfTriggerDo( pArea, EVENT_DELETE, 0, NULL ) )
         return FAILURE;
   }

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   /* Read record */
   if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
      return FAILURE;

   if( !pArea->fPositioned )
      return SUCCESS;

   /* Buffer is hot? */
   if( !pArea->fRecordChanged && SELF_GOHOT( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   pArea->pRecord[ 0 ] = '*';
   pArea->fDeleted = TRUE;
   return SUCCESS;
}

/*
 * Determine deleted status for a record.
 */
static ERRCODE hb_dbfDeleted( DBFAREAP pArea, BOOL * pDeleted )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfDeleted(%p, %p)", pArea, pDeleted));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   /* Read record */
   if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
      return FAILURE;

   * pDeleted = pArea->fDeleted;
   return SUCCESS;
}

/*
 * Write data buffer to the data store.
 */
static ERRCODE hb_dbfFlush( DBFAREAP pArea )
{
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfFlush(%p)", pArea));

   uiError = SELF_GOCOLD( ( AREAP ) pArea );
   if( uiError == SUCCESS )
   {
      if( pArea->fUpdateHeader )
         uiError = SELF_WRITEDBHEADER( ( AREAP ) pArea );
   }

   if( hb_set.HB_SET_HARDCOMMIT && uiError == SUCCESS )
   {
      if( pArea->fDataFlush )
      {
         hb_fsCommit( pArea->hDataFile );
         pArea->fDataFlush = FALSE;
      }
      if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR && pArea->fMemoFlush )
      {
         hb_fsCommit( pArea->hMemoFile );
         pArea->fMemoFlush = FALSE;
      }
   }

   return uiError;
}

/*
 * Retrieve current record buffer
 */
static ERRCODE hb_dbfGetRec( DBFAREAP pArea, BYTE ** pBuffer )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGetRec(%p, %p)", pArea, pBuffer));

   if( pBuffer != NULL )
   {
      /* Read record */
      if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
         return FAILURE;

      *pBuffer = pArea->pRecord;
   }
   else
   {
      if( pArea->pRecord[ 0 ] == 'D' || pArea->pRecord[ 0 ] == 'E' )
      {
         pArea->fEncrypted = TRUE;
         pArea->pRecord[ 0 ] = pArea->pRecord[ 0 ] == 'D' ? '*' : ' ';
         if( pArea->pCryptKey && pArea->bCryptType == DB_CRYPT_SIX )
         {
            hb_sxDeCrypt( pArea->pRecord + 1, pArea->pRecord + 1,
                          pArea->pCryptKey, pArea->uiRecordLen - 1 );
         }
      }
      else
      {
         pArea->fEncrypted = FALSE;
      }
   }
   return SUCCESS;
}

/*
 * Obtain the current value of a field.
 */
static ERRCODE hb_dbfGetValue( DBFAREAP pArea, USHORT uiIndex, PHB_ITEM pItem )
{
   LPFIELD pField;
   BOOL fError;
   PHB_ITEM pError;
   char szBuffer[24];

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGetValue(%p, %hu, %p)", pArea, uiIndex, pItem));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   /* Read record */
   if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
      return FAILURE;

   fError = FALSE;
   uiIndex--;
   pField = pArea->lpFields + uiIndex;
   switch( pField->uiType )
   {
      case HB_FT_STRING:
#ifndef HB_CDP_SUPPORT_OFF
         if( pArea->cdPage != hb_cdp_page )
         {
            char * pVal = ( char * ) hb_xgrab( pField->uiLen + 1 );
            memcpy( pVal, pArea->pRecord + pArea->pFieldOffset[ uiIndex ], pField->uiLen );
            pVal[ pField->uiLen ] = '\0';
            hb_cdpnTranslate( pVal, pArea->cdPage, hb_cdp_page, pField->uiLen );
            hb_itemPutCPtr( pItem, pVal, pField->uiLen );
         }
         else
#endif
         {
            hb_itemPutCL( pItem, ( char * ) pArea->pRecord + pArea->pFieldOffset[ uiIndex ],
                          pField->uiLen );
         }
         break;

      case HB_FT_LOGICAL:
         hb_itemPutL( pItem, pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ] == 'T' ||
                      pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ] == 't' ||
                      pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ] == 'Y' ||
                      pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ] == 'y' );
         break;

      case HB_FT_DATE:
         if( pField->uiLen == 3 )
         {
            hb_itemPutDL( pItem, HB_GET_LE_UINT24( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ) );
         }
         else if( pField->uiLen == 4 )
         {
            hb_itemPutDL( pItem, HB_GET_LE_UINT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ) );
         }
         else
         {
            hb_itemPutDS( pItem, ( char * ) pArea->pRecord + pArea->pFieldOffset[ uiIndex ] );
         }
         break;

      case HB_FT_TIME:
         if( pField->uiLen == 4 )
         {
            hb_itemPutC( pItem, hb_timeStampStr( szBuffer,
                  HB_GET_LE_INT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ) ) );
            break;
         }
         /* no break */

      case HB_FT_MODTIME:
      case HB_FT_DAYTIME:
         hb_itemPutC( pItem, hb_dateTimeStampStr( szBuffer,
               HB_GET_LE_INT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ),
               HB_GET_LE_INT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] + 4 ) ) );
         break;

      case HB_FT_INTEGER:
      case HB_FT_CURRENCY:
      case HB_FT_AUTOINC:
      case HB_FT_ROWVER:
         if( pField->uiDec )
         {
            double dValue;
            int iLen;
            switch( pField->uiLen )
            {
               case 1:
                  dValue = ( SCHAR ) pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ];
                  iLen = 4;
                  break;
               case 2:
                  dValue = HB_GET_LE_INT16( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] );
                  iLen = 6;
                  break;
               case 3:
                  dValue = HB_GET_LE_INT24( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] );
                  iLen = 10;
                  break;
               case 4:
                  dValue = HB_GET_LE_INT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] );
                  iLen = 10;
                  break;
               case 8:
                  dValue = ( double ) HB_GET_LE_INT64( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] );
                  iLen = 20;
                  break;
               default:
                  dValue = 0;
                  iLen = 0;
                  fError = TRUE;
                  break;
            }
            hb_itemPutNDLen( pItem, hb_numDecConv( dValue, ( int ) pField->uiDec ),
                             iLen, ( int ) pField->uiDec );
         }
         else
         {
            switch( pField->uiLen )
            {
               case 1:
                  hb_itemPutNILen( pItem, ( SCHAR ) pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ], 4 );
                  break;
               case 2:
                  hb_itemPutNILen( pItem, ( int ) HB_GET_LE_INT16( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ), 6 );
                  break;
               case 3:
                  hb_itemPutNIntLen( pItem, ( HB_LONG ) HB_GET_LE_INT24( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ), 10 );
                  break;
               case 4:
                  hb_itemPutNIntLen( pItem, ( HB_LONG ) HB_GET_LE_INT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ), 10 );
                  break;
               case 8:
#ifndef HB_LONG_LONG_OFF
                  hb_itemPutNIntLen( pItem, ( HB_LONG ) HB_GET_LE_INT64( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ), 20 );
#else
                  hb_itemPutNLen( pItem, ( double ) HB_GET_LE_INT64( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ), 20, 0 );
#endif
                  break;
               default:
                  fError = TRUE;
                  break;
            }
         }
         break;

      case HB_FT_DOUBLE:
      case HB_FT_CURDOUBLE:
         hb_itemPutNDLen( pItem, HB_GET_LE_DOUBLE( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ),
                          20 - ( pField->uiDec > 0 ? ( pField->uiDec + 1 ) : 0 ),
                          ( int ) pField->uiDec );
         break;

      case HB_FT_LONG:
      case HB_FT_FLOAT:
         /* DBASE documentation defines maximum numeric field size as 20
          * but Clipper allows to create longer fields so I remove this
          * limit, Druzus
          */
         /*
         if( pField->uiLen > 20 )
            fError = TRUE;
         else
         */
         {
            HB_LONG lVal;
            double dVal;
            BOOL fDbl;

            fDbl = hb_strnToNum( (const char *) pArea->pRecord + pArea->pFieldOffset[ uiIndex ],
                                 pField->uiLen, &lVal, &dVal );

            if( pField->uiDec )
            {
               hb_itemPutNDLen( pItem, fDbl ? dVal : ( double ) lVal,
                                ( int ) ( pField->uiLen - pField->uiDec - 1 ),
                                ( int ) pField->uiDec );
            }
            else if( fDbl )
            {
               hb_itemPutNDLen( pItem, dVal, ( int ) pField->uiLen, 0 );
            }
            else
            {
               hb_itemPutNIntLen( pItem, lVal, ( int ) pField->uiLen );
            }
         }
         break;

      case HB_FT_ANY:
         if( pField->uiLen == 3 )
         {
            hb_itemPutDL( pItem, hb_sxPtoD( ( char * ) pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ) );
         }
         else if( pField->uiLen == 4 )
         {
            hb_itemPutNIntLen( pItem, ( HB_LONG ) HB_GET_LE_INT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] ), 10 );
         }
         else
         {
            fError = TRUE;
         }
         break;

      case HB_FT_MEMO:
      default:
         fError = TRUE;
         break;
   }

   /* Any error? */
   if( fError )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_DATATYPE );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_DATATYPE ) );
      hb_errPutOperation( pError, hb_dynsymName( ( PHB_DYNS ) pField->sym ) );
      hb_errPutSubCode( pError, EDBF_DATATYPE );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }

   if( pArea->fTrigger )
   {
      if( !hb_dbfTriggerDo( pArea, EVENT_GET, uiIndex + 1, pItem ) )
         return FAILURE;
   }

   return SUCCESS;
}

/*
 * Obtain the length of a field value.
 */
static ERRCODE hb_dbfGetVarLen( DBFAREAP pArea, USHORT uiIndex, ULONG * pLength )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGetVarLen(%p, %hu, %p)", pArea, uiIndex, pLength));

   * pLength = pArea->lpFields[ uiIndex - 1 ].uiLen;

   return SUCCESS;
}

/*
 * Perform a write of WorkArea memory to the data store.
 */
static ERRCODE hb_dbfGoCold( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGoCold(%p)", pArea));

   if( pArea->fRecordChanged )
   {
      if( pArea->fTrigger )
      {
         /* The pending relation may move the record pointer so we should
            disable them for trigger evaluation */
         LPDBRELINFO lpdbPendingRel = pArea->lpdbPendingRel;
         pArea->lpdbPendingRel = NULL;

         hb_dbfTriggerDo( pArea, EVENT_UPDATE, 0, NULL );

         /* Restore disabled pending relation */
         pArea->lpdbPendingRel = lpdbPendingRel;
      }

      if( pArea->fModStamp )
         hb_dbfUpdateStampFields( pArea );
   
      /* Write current record */
      if( ! hb_dbfWriteRecord( pArea ) )
         return FAILURE;

      if( pArea->fAppend )
      {
         pArea->fUpdateHeader = TRUE;
         pArea->fAppend = FALSE;
      }

      /* Update header */
      if( pArea->fShared && pArea->fUpdateHeader )
         return SELF_WRITEDBHEADER( ( AREAP ) pArea );
   }
   return SUCCESS;
}

/*
 * Mark the WorkArea data buffer as hot.
 */
static ERRCODE hb_dbfGoHot( DBFAREAP pArea )
{
   PHB_ITEM pError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGoHot(%p)", pArea));

   if( pArea->fReadonly )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_READONLY );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_READONLY ) );
      hb_errPutSubCode( pError, EDBF_READONLY );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }
   else if( pArea->fShared && !pArea->fFLocked &&
            !hb_dbfIsLocked( pArea, pArea->ulRecNo ) )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_UNLOCKED );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_UNLOCKED ) );
      hb_errPutSubCode( pError, EDBF_UNLOCKED );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }
   pArea->fRecordChanged = TRUE;
   return SUCCESS;
}

/*
 * Replace the current record.
 */
static ERRCODE hb_dbfPutRec( DBFAREAP pArea, BYTE * pBuffer )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfPutRec(%p, %p)", pArea, pBuffer));

   if( pBuffer != NULL )
   {
      if( pArea->lpdbPendingRel )
      {
         if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
            return FAILURE;
      }

      if( !pArea->fPositioned )
         return SUCCESS;

      if( !pArea->fRecordChanged && SELF_GOHOT( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;

      /* Copy data to buffer */
      memcpy( pArea->pRecord, pBuffer, pArea->uiRecordLen );

      /*
       * TODO: such operation should be forbidden
       * maybe it will be good to return FAILURE when
       *    pArea->pRecord[ 0 ] != '*' && pArea->pRecord[ 0 ] != ' '
       */
      if( pArea->pRecord[ 0 ] == 'D' || pArea->pRecord[ 0 ] == 'E' )
      {
         if( !pArea->fHasMemo )
            pArea->fEncrypted = TRUE;
         pArea->pRecord[ 0 ] = pArea->pRecord[ 0 ] == 'D' ? '*' : ' ';
      }

      pArea->fDeleted = pArea->pRecord[ 0 ] == '*';
   }
   else /* if( pArea->fRecordChanged ) */
   {
      BYTE * pRecord = pArea->pRecord;
      USHORT uiWritten;

      if( pArea->pCryptKey )
      {
         /* This enables record encryption in update operation */
         if( pArea->bCryptType == DB_CRYPT_SIX && !pArea->fHasMemo )
            pArea->fEncrypted = TRUE;

         if( pArea->bCryptType == DB_CRYPT_SIX && pArea->fEncrypted )
         {
            pRecord = ( BYTE * ) hb_xgrab( pArea->uiRecordLen );
            pRecord[ 0 ] = pArea->fDeleted ? 'D' : 'E';
            hb_sxEnCrypt( pArea->pRecord + 1, pRecord + 1, pArea->pCryptKey,
                          pArea->uiRecordLen - 1 );
         }
      }

      /* Write data to file */
      hb_fsSeekLarge( pArea->hDataFile, ( HB_FOFFSET ) pArea->uiHeaderLen +
                      ( HB_FOFFSET ) ( pArea->ulRecNo - 1 ) *
                      ( HB_FOFFSET ) pArea->uiRecordLen, FS_SET );
      uiWritten = hb_fsWrite( pArea->hDataFile, pRecord, pArea->uiRecordLen );

      if( pRecord != pArea->pRecord )
         hb_xfree( pRecord );

      if( uiWritten != pArea->uiRecordLen )
      {
         PHB_ITEM pError = hb_errNew();

         hb_errPutGenCode( pError, EG_WRITE );
         hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_WRITE ) );
         hb_errPutSubCode( pError, EDBF_WRITE );
         hb_errPutOsCode( pError, hb_fsError() );
         hb_errPutFileName( pError, pArea->szDataFileName );
         SELF_ERROR( ( AREAP ) pArea, pError );
         hb_itemRelease( pError );
         return FAILURE;
      }
   }
   return SUCCESS;
}

/*
 * Assign a value to a field.
 */
static ERRCODE hb_dbfPutValue( DBFAREAP pArea, USHORT uiIndex, PHB_ITEM pItem )
{
   USHORT uiSize;
   LPFIELD pField;
   /* this buffer is for date and number conversion,
    * DBASE documentation defines maximum numeric field size as 20
    * but Clipper allows to create longer fields so I removed this
    * limit [druzus]
    */
   char szBuffer[ 256 ];
   PHB_ITEM pError;
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfPutValue(%p, %hu, %p)", pArea, uiIndex, pItem));

   if( pArea->fTrigger )
   {
      if( !hb_dbfTriggerDo( pArea, EVENT_PUT, uiIndex, pItem ) )
         return FAILURE;
   }

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   /* Read record */
   if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
      return FAILURE;

   if( !pArea->fPositioned )
      return SUCCESS;

   /* Buffer is hot? */
   if( !pArea->fRecordChanged && SELF_GOHOT( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   uiError = SUCCESS;
   uiIndex--;
   pField = pArea->lpFields + uiIndex;
   if( pField->uiType == HB_FT_MEMO ||
       pField->uiType == HB_FT_IMAGE ||
       pField->uiType == HB_FT_BLOB ||
       pField->uiType == HB_FT_OLE )
      uiError = EDBF_DATATYPE;
   else
   {
      if( HB_IS_MEMO( pItem ) || HB_IS_STRING( pItem ) )
      {
         if( pField->uiType == HB_FT_STRING )
         {
            uiSize = ( USHORT ) hb_itemGetCLen( pItem );
            if( uiSize > pField->uiLen )
               uiSize = pField->uiLen;
            memcpy( pArea->pRecord + pArea->pFieldOffset[ uiIndex ],
                    hb_itemGetCPtr( pItem ), uiSize );
#ifndef HB_CDP_SUPPORT_OFF
            hb_cdpnTranslate( (char *) pArea->pRecord + pArea->pFieldOffset[ uiIndex ], hb_cdp_page, pArea->cdPage, uiSize );
#endif
            memset( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] + uiSize,
                    ' ', pField->uiLen - uiSize );
         }
         else if( pField->uiType == HB_FT_DAYTIME )
         {
            LONG lJulian, lMillisec;
            BYTE * ptr = pArea->pRecord + pArea->pFieldOffset[ uiIndex ];
            hb_dateTimeStampStrGet( hb_itemGetCPtr( pItem ), &lJulian, &lMillisec );
            HB_PUT_LE_UINT32( ptr, lJulian );
            ptr += 4;
            HB_PUT_LE_UINT32( ptr, lMillisec );
         }
         else
            uiError = EDBF_DATATYPE;
      }
      /* Must precede HB_IS_NUMERIC() because a DATE is also a NUMERIC. (xHarbour) */
      else if( HB_IS_DATE( pItem ) )
      {
         if( pField->uiType == HB_FT_DATE )
         {
            if( pField->uiLen == 3 )
            {
               HB_PUT_LE_UINT24( pArea->pRecord + pArea->pFieldOffset[ uiIndex ],
                                 hb_itemGetDL( pItem ) );
            }
            else if( pField->uiLen == 4 )
            {
               HB_PUT_LE_UINT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ],
                                 hb_itemGetDL( pItem ) );
            }
            else
            {
               hb_itemGetDS( pItem, szBuffer );
               memcpy( pArea->pRecord + pArea->pFieldOffset[ uiIndex ], szBuffer, 8 );
            }
         }
         else if( pField->uiType == HB_FT_ANY && pField->uiLen == 3 )
         {
            hb_sxDtoP( ( char * ) pArea->pRecord + pArea->pFieldOffset[ uiIndex ],
                       hb_itemGetDL( pItem ) );
         }
         else
            uiError = EDBF_DATATYPE;
      }
      else if( HB_IS_NUMBER( pItem ) )
      {
         if( pField->uiType == HB_FT_LONG || pField->uiType == HB_FT_FLOAT )
         {
            if( hb_itemStrBuf( szBuffer, pItem, pField->uiLen, pField->uiDec ) )
            {
               memcpy( pArea->pRecord + pArea->pFieldOffset[ uiIndex ],
                       szBuffer, pField->uiLen );
            }
            else
            {
               uiError = EDBF_DATAWIDTH;
               memset( pArea->pRecord + pArea->pFieldOffset[ uiIndex ],
                       '*', pField->uiLen );
            }
         }
         else if( pField->uiType == HB_FT_INTEGER )
         {
            HB_LONG lVal;
            double dVal;
            int iSize;

            if( pField->uiDec )
            {
               dVal = hb_numDecConv( hb_itemGetND( pItem ), - ( int ) pField->uiDec );
               lVal = ( HB_LONG ) dVal;
               if( ! HB_DBL_LIM_INT64( dVal ) )
                  iSize = 99;
               else
#ifndef HB_LONG_LONG_OFF
                  iSize = HB_LIM_INT8( lVal ) ? 1 :
                        ( HB_LIM_INT16( lVal ) ? 2 :
                        ( HB_LIM_INT24( lVal ) ? 3 :
                        ( HB_LIM_INT32( lVal ) ? 4 : 8 ) ) );
#else
                  iSize = HB_DBL_LIM_INT8( dVal ) ? 1 :
                        ( HB_DBL_LIM_INT16( dVal ) ? 2 :
                        ( HB_DBL_LIM_INT24( dVal ) ? 3 :
                        ( HB_DBL_LIM_INT32( dVal ) ? 4 : 8 ) ) );
#endif
            }
            else if( HB_IS_DOUBLE( pItem ) )
            {
               dVal = hb_itemGetND( pItem );
               lVal = ( HB_LONG ) dVal;
               if( ! HB_DBL_LIM_INT64( dVal ) )
                  iSize = 99;
               else
#ifndef HB_LONG_LONG_OFF
                  iSize = HB_LIM_INT8( lVal ) ? 1 :
                        ( HB_LIM_INT16( lVal ) ? 2 :
                        ( HB_LIM_INT24( lVal ) ? 3 :
                        ( HB_LIM_INT32( lVal ) ? 4 : 8 ) ) );
#else
                  iSize = HB_DBL_LIM_INT8( dVal ) ? 1 :
                        ( HB_DBL_LIM_INT16( dVal ) ? 2 :
                        ( HB_DBL_LIM_INT24( dVal ) ? 3 :
                        ( HB_DBL_LIM_INT32( dVal ) ? 4 : 8 ) ) );
#endif
            }
            else
            {
               lVal = ( HB_LONG ) hb_itemGetNInt( pItem );
#ifdef HB_LONG_LONG_OFF
               dVal = ( double ) lVal;
#endif
               iSize = HB_LIM_INT8( lVal ) ? 1 :
                     ( HB_LIM_INT16( lVal ) ? 2 :
                     ( HB_LIM_INT24( lVal ) ? 3 :
                     ( HB_LIM_INT32( lVal ) ? 4 : 8 ) ) );
            }

            if( iSize > pField->uiLen )
            {
               uiError = EDBF_DATAWIDTH;
            }
            else
            {
               switch( pField->uiLen )
               {
                  case 1:
                     pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ] = ( signed char ) lVal;
                     break;
                  case 2:
                     HB_PUT_LE_UINT16( pArea->pRecord + pArea->pFieldOffset[ uiIndex ], ( UINT16 ) lVal );
                     break;
                  case 3:
                     HB_PUT_LE_UINT24( pArea->pRecord + pArea->pFieldOffset[ uiIndex ], ( UINT32 ) lVal );
                     break;
                  case 4:
                     HB_PUT_LE_UINT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ], ( UINT32 ) lVal );
                     break;
                  case 8:
#ifndef HB_LONG_LONG_OFF
                     HB_PUT_LE_UINT64( pArea->pRecord + pArea->pFieldOffset[ uiIndex ], ( UINT64 ) lVal );
#else
                     HB_PUT_LE_UINT64( pArea->pRecord + pArea->pFieldOffset[ uiIndex ], dVal );
#endif
                     break;
                  default:
                     uiError = EDBF_DATATYPE;
                     break;
               }
            }
         }
         else if( pField->uiType == HB_FT_DOUBLE )
         {
            HB_PUT_LE_DOUBLE( pArea->pRecord + pArea->pFieldOffset[ uiIndex ], hb_itemGetND( pItem ) );
         }
         else if( pField->uiType == HB_FT_ANY && pField->uiLen == 4 )
         {
            HB_LONG lVal = hb_itemGetNInt( pItem );
            if( HB_IS_DOUBLE( pItem ) ?
                        HB_DBL_LIM_INT32( hb_itemGetND( pItem ) ) :
                        HB_LIM_INT32( lVal ) )
            {
               HB_PUT_LE_UINT32( pArea->pRecord + pArea->pFieldOffset[ uiIndex ], ( UINT32 ) lVal );
            }
            else
            {
               uiError = EDBF_DATAWIDTH;
            }
         }
         else
         {
            uiError = EDBF_DATATYPE;
         }
      }
      else if( HB_IS_LOGICAL( pItem ) )
      {
         if( pField->uiType == HB_FT_LOGICAL )
            pArea->pRecord[ pArea->pFieldOffset[ uiIndex ] ] = hb_itemGetL( pItem ) ? 'T' : 'F';
         else
            uiError = EDBF_DATATYPE;
      }
      else
         uiError = EDBF_DATATYPE;
   }

   /* Exit if any error */
   if( uiError != SUCCESS )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, hb_dbfGetEGcode( uiError ) );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( hb_dbfGetEGcode( uiError ) ) );
      hb_errPutOperation( pError, hb_dynsymName( ( PHB_DYNS ) pField->sym ) );
      hb_errPutSubCode( pError, uiError );
      hb_errPutFlags( pError, EF_CANDEFAULT );
      uiError = SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return uiError == E_DEFAULT ? SUCCESS : FAILURE;
   }

   return SUCCESS;
}

/*
 * Undelete the current record.
 */
static ERRCODE hb_dbfRecall( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfRecall(%p)", pArea));

   if( pArea->fTrigger )
   {
      if( !hb_dbfTriggerDo( pArea, EVENT_RECALL, 0, NULL ) )
         return FAILURE;
   }

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   /* Read record */
   if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
      return FAILURE;

   if( !pArea->fPositioned )
      return SUCCESS;

   /* Buffer is hot? */
   if( !pArea->fRecordChanged && SELF_GOHOT( ( AREAP ) pArea ) != SUCCESS )
      return FAILURE;

   pArea->pRecord[ 0 ] = ' ';
   pArea->fDeleted = FALSE;
   return SUCCESS;
}

/*
 * Obtain number of records in WorkArea.
 */
static ERRCODE hb_dbfRecCount( DBFAREAP pArea, ULONG * pRecCount )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfRecCount(%p, %p)", pArea, pRecCount));

   /* Update record count */
   if( pArea->fShared )
      pArea->ulRecCount = hb_dbfCalcRecCount( pArea );

   * pRecCount = pArea->ulRecCount;
   return SUCCESS;
}

/*
 * Obtain physical row number at current WorkArea cursor position.
 */
static ERRCODE hb_dbfRecNo( DBFAREAP pArea, ULONG * ulRecNo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfRecNo(%p, %p)", pArea, ulRecNo));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   *ulRecNo = pArea->ulRecNo;
   return SUCCESS;
}

/*
 * Obtain physical row ID at current WorkArea cursor position.
 */
static ERRCODE hb_dbfRecId( DBFAREAP pArea, PHB_ITEM pRecNo )
{
   ERRCODE errCode;
   ULONG ulRecNo;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfRecId(%p, %p)", pArea, pRecNo));

   errCode = SELF_RECNO( ( AREAP ) pArea, &ulRecNo );

#ifdef HB_C52_STRICT
   /* this is for strict Clipper compatibility but IMHO Clipper should not
      do that and always set fixed size independent to the record number */
   if( ulRecNo < 10000000 )
   {
      hb_itemPutNLLen( pRecNo, ulRecNo, 7 );
   }
   else
   {
      hb_itemPutNLLen( pRecNo, ulRecNo, 10 );
   }
#else
   hb_itemPutNInt( pRecNo, ulRecNo );
#endif
   return errCode;
}

/*
 * Establish the extent of the array of fields for a WorkArea.
 */
static ERRCODE hb_dbfSetFieldExtent( DBFAREAP pArea, USHORT uiFieldExtent )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfSetFieldExtent(%p, %hu)", pArea, uiFieldExtent));

   if( SUPER_SETFIELDEXTENT( ( AREAP ) pArea, uiFieldExtent ) == FAILURE )
      return FAILURE;

   /* Alloc field offsets array */
   if( uiFieldExtent )
   {
      pArea->pFieldOffset = ( USHORT * ) hb_xgrab( uiFieldExtent * sizeof( USHORT ) );
      memset( pArea->pFieldOffset, 0, uiFieldExtent * sizeof( USHORT ) );
   }

   return SUCCESS;
}

/*
 * Close the table in the WorkArea.
 */
static ERRCODE hb_dbfClose( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfClose(%p)", pArea));

   if( pArea->fTrigger )
   {
      if( !hb_dbfTriggerDo( pArea, EVENT_PRECLOSE, 0, NULL ) )
         return FAILURE;
   }

   /* Reset parent rel struct */
   pArea->lpdbPendingRel = NULL;

   /* Update record and unlock records */
   if( pArea->hDataFile != FS_ERROR )
   {
      /* update buffers */
      SELF_GOCOLD( ( AREAP ) pArea );

      /* Unlock all records */
      SELF_UNLOCK( ( AREAP ) pArea, NULL );

      /* Update header */
      if( pArea->fUpdateHeader )
         SELF_WRITEDBHEADER( ( AREAP ) pArea );

      /* It's not Clipper compatible but it reduces the problem with
         byggy Windows network setting */
      if( hb_set.HB_SET_HARDCOMMIT )
         SELF_FLUSH( ( AREAP ) pArea );
   }

   SUPER_CLOSE( ( AREAP ) pArea );

   if( pArea->hDataFile != FS_ERROR )
   {
      hb_fsClose( pArea->hDataFile );
      pArea->hDataFile = FS_ERROR;
   }

   /* Close the memo file */
   if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR )
   {
      hb_fsClose( pArea->hMemoFile );
      pArea->hMemoFile = FS_ERROR;
   }

   /* Free field offset array */
   if( pArea->pFieldOffset )
   {
      hb_xfree( pArea->pFieldOffset );
      pArea->pFieldOffset = NULL;
   }

   /* Free buffer */
   if( pArea->pRecord )
   {
      hb_xfree( pArea->pRecord );
      pArea->pRecord = NULL;
   }

   /* Free encryption password key */
   if( pArea->pCryptKey )
   {
      memset( pArea->pCryptKey, '\0', 8 );
      hb_xfree( pArea->pCryptKey );
      pArea->pCryptKey = NULL;
   }

   /* Free all filenames */
   if( pArea->szDataFileName )
   {
      hb_xfree( pArea->szDataFileName );
      pArea->szDataFileName = NULL;
   }
   if( pArea->szMemoFileName )
   {
      hb_xfree( pArea->szMemoFileName );
      pArea->szMemoFileName = NULL;
   }

   if( pArea->fTrigger )
   {
      hb_dbfTriggerDo( pArea, EVENT_POSTCLOSE, 0, NULL );
      pArea->fTrigger = FALSE;
   }

   return SUCCESS;
}

/*
 * Create a data store in the specified WorkArea.
 */
static ERRCODE hb_dbfCreate( DBFAREAP pArea, LPDBOPENINFO pCreateInfo )
{
   ERRCODE errCode = SUCCESS;
   USHORT uiSize, uiCount;
   BOOL fRetry, fError, fRawBlob;
   DBFFIELD * pBuffer, *pThisField;
   PHB_FNAME pFileName;
   PHB_ITEM pItem = NULL, pError;
   BYTE szFileName[ _POSIX_PATH_MAX + 1 ];

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfCreate(%p, %p)", pArea, pCreateInfo));

   pArea->lpdbOpenInfo = pCreateInfo;

   pFileName = hb_fsFNameSplit( ( char * ) pCreateInfo->abName );

   if( hb_set.HB_SET_DEFEXTENSIONS && ! pFileName->szExtension )
   {
      pItem = hb_itemPutC( pItem, "" );
      if( SELF_INFO( ( AREAP ) pArea, DBI_TABLEEXT, pItem ) != SUCCESS )
      {
         hb_itemRelease( pItem );
         hb_xfree( pFileName );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }
      pFileName->szExtension = hb_itemGetCPtr( pItem );
      hb_fsFNameMerge( ( char * ) szFileName, pFileName );
   }
   else
   {
      hb_strncpy( ( char * ) szFileName, ( char * ) pCreateInfo->abName, _POSIX_PATH_MAX );
   }
   hb_xfree( pFileName );

   pItem = hb_itemPutL( pItem, FALSE );
   fRawBlob = SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_BLOB_SUPPORT, pCreateInfo->ulConnection, pItem ) == SUCCESS &&
              hb_itemGetL( pItem );

   if( pArea->bTableType == 0 )
   {
      pItem = hb_itemPutNI( pItem, 0 );
      if( SELF_INFO( ( AREAP ) pArea, DBI_TABLETYPE, pItem ) != SUCCESS )
      {
         hb_itemRelease( pItem );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }
      pArea->bTableType = hb_itemGetNI( pItem );
   }

   if( pArea->bLockType == 0 )
   {
      pItem = hb_itemPutNI( pItem, 0 );
      if( SELF_INFO( ( AREAP ) pArea, DBI_LOCKSCHEME, pItem ) != SUCCESS )
      {
         hb_itemRelease( pItem );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }
      pArea->bLockType = hb_itemGetNI( pItem );
      if( pArea->bLockType == 0 )
      {
         pArea->bLockType = DB_DBFLOCK_CLIP;
      }
   }

   if( pArea->bTableType == DB_DBF_VFP && !fRawBlob )
   {
      pArea->bMemoType = DB_MEMO_FPT;
   }
   else if( pArea->bMemoType == 0 )
   {
      /* get memo type */
      pItem = hb_itemPutNI( pItem, 0 );
      if( SELF_INFO( ( AREAP ) pArea, DBI_MEMOTYPE, pItem ) != SUCCESS )
      {
         hb_itemRelease( pItem );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }
      pArea->bMemoType = ( BYTE ) hb_itemGetNI( pItem );
   }

   pArea->bCryptType = DB_CRYPT_NONE;

   if( pItem )
      hb_itemRelease( pItem );

   if( pArea->uiFieldCount * sizeof( DBFFIELD ) + sizeof( DBFHEADER ) +
       ( pArea->bTableType == DB_DBF_VFP ? 1 : 2 ) > UINT16_MAX )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_CREATE );
      hb_errPutSubCode( pError, EDBF_DATAWIDTH );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CREATE ) );
      hb_errPutFileName( pError, ( char * ) pCreateInfo->abName );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      pArea->lpdbOpenInfo = NULL;
      return FAILURE;
   }

   if( !fRawBlob )
   {
      pError = NULL;
      /* Try create */
      do
      {
         pArea->hDataFile = hb_fsExtOpen( szFileName, NULL,
                                          FO_READWRITE | FO_EXCLUSIVE | FXO_TRUNCATE |
                                          FXO_DEFAULTS | FXO_SHARELOCK | FXO_COPYNAME,
                                          NULL, pError );
         if( pArea->hDataFile == FS_ERROR )
         {
            if( !pError )
            {
               pError = hb_errNew();
               hb_errPutGenCode( pError, EG_CREATE );
               hb_errPutSubCode( pError, EDBF_CREATE_DBF );
               hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CREATE ) );
               hb_errPutFileName( pError, ( char * ) szFileName );
               hb_errPutFlags( pError, EF_CANRETRY | EF_CANDEFAULT );
            }
            hb_errPutOsCode( pError, hb_fsError() );
            fRetry = ( SELF_ERROR( ( AREAP ) pArea, pError ) == E_RETRY );
         }
         else
            fRetry = FALSE;
      } while( fRetry );

      if( pError )
      {
         hb_itemRelease( pError );
      }

      if( pArea->hDataFile == FS_ERROR )
      {
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }
   }

   pArea->szDataFileName = hb_strdup( ( char * ) szFileName );

   uiSize = pArea->uiFieldCount * sizeof( DBFFIELD );
   if( pArea->uiFieldCount )
   {
      pBuffer = ( DBFFIELD * ) hb_xgrab( uiSize );
      memset( pBuffer, 0, uiSize );
   }
   else
   {
      pBuffer = NULL;
   }
   pThisField = pBuffer;

   pArea->fHasMemo = fError = FALSE;

   /* Size for deleted flag */
   pArea->uiRecordLen = 1;

   for( uiCount = 0; uiCount < pArea->uiFieldCount; uiCount++ )
   {
      LPFIELD pField = pArea->lpFields + uiCount;
      strncpy( ( char * ) pThisField->bName,
               hb_dynsymName( ( PHB_DYNS ) pField->sym ), 10 );
      pArea->pFieldOffset[ uiCount ] = pArea->uiRecordLen;
      /* field offset */
      if( pArea->bTableType == DB_DBF_VFP )
         HB_PUT_LE_UINT16( pThisField->bReserved1, pArea->uiRecordLen );

      switch( pField->uiType )
      {
         case HB_FT_STRING:
            pThisField->bType = 'C';
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bDec = ( BYTE ) ( pField->uiLen >> 8 );
            pArea->uiRecordLen += pField->uiLen;
            break;

         case HB_FT_LOGICAL:
            pThisField->bType = 'L';
            pThisField->bLen = 1;
            pArea->uiRecordLen++;
            break;

         case HB_FT_MEMO:
            pThisField->bType = 'M';
            if( pField->uiLen != 4 || pArea->bMemoType == DB_MEMO_SMT )
               pField->uiLen = 10;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pArea->uiRecordLen += pField->uiLen;
            pArea->fHasMemo = TRUE;
            break;

         case HB_FT_BLOB:
            pThisField->bType = 'W';
            if( pField->uiLen != 4 || pArea->bMemoType == DB_MEMO_SMT )
               pField->uiLen = 10;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bFieldFlags = HB_FF_BINARY;
            pArea->uiRecordLen += pField->uiLen;
            pArea->fHasMemo = TRUE;
            break;

         case HB_FT_IMAGE:
            pThisField->bType = 'P';
            if( pField->uiLen != 4 || pArea->bMemoType == DB_MEMO_SMT )
               pField->uiLen = 10;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bFieldFlags = HB_FF_BINARY;
            pArea->uiRecordLen += pField->uiLen;
            pArea->fHasMemo = TRUE;
            break;

         case HB_FT_OLE:
            pThisField->bType = 'G';
            if( pField->uiLen != 4 || pArea->bMemoType == DB_MEMO_SMT )
               pField->uiLen = 10;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bFieldFlags = HB_FF_BINARY;
            pArea->uiRecordLen += pField->uiLen;
            pArea->fHasMemo = TRUE;
            break;

         case HB_FT_ANY:
            pThisField->bType = 'V';
            if( pField->uiLen < 3 || pField->uiLen == 5 )
            {
               pField->uiLen = 6;
            }
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bDec = ( BYTE ) ( pField->uiLen >> 8 );
            pArea->uiRecordLen += pField->uiLen;
            if( pThisField->bLen >= 6 )
            {
               pArea->uiMemoVersion = DB_MEMOVER_SIX;
               pArea->fHasMemo = TRUE;
            }
            /*
            if( pArea->bTableType == DB_DBF_VFP )
               fError = TRUE;
            */
            break;

         case HB_FT_DATE:
            pThisField->bType = 'D';
            if( pField->uiLen != 3 && pField->uiLen != 4 )
            {
               pField->uiLen = pThisField->bLen = 8;
            }
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pArea->uiRecordLen += pField->uiLen;
            break;

         case HB_FT_LONG:
            pThisField->bType = 'N';
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bDec = ( BYTE ) pField->uiDec;
            pArea->uiRecordLen += pField->uiLen;
            break;

         case HB_FT_FLOAT:
            pThisField->bType = 'F';
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bDec = ( BYTE ) pField->uiDec;
            pArea->uiRecordLen += pField->uiLen;
            break;

         case HB_FT_DOUBLE:
         case HB_FT_CURDOUBLE:
            pThisField->bType = 'B';
            pField->uiLen = 8;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bDec = ( BYTE ) pField->uiDec;
            pArea->uiRecordLen += pField->uiLen;
            break;

         case HB_FT_INTEGER:
         case HB_FT_CURRENCY:
            pThisField->bType = ( pArea->bTableType == DB_DBF_VFP &&
                                  pField->uiLen == 8 && pField->uiDec == 4 ) ?
                                'Y' : 'I';
            if( ( pField->uiLen > 4 && pField->uiLen != 8 ) ||
                pField->uiLen == 0 )
            {
               pField->uiLen = 4;
            }
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bDec = ( BYTE ) pField->uiDec;
            pArea->uiRecordLen += pField->uiLen;
            break;

         case HB_FT_TIME:
            pThisField->bType = 'T';
            pField->uiLen = 4;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pArea->uiRecordLen += pField->uiLen;
            break;

         case HB_FT_DAYTIME:
            pThisField->bType = pArea->bTableType == DB_DBF_VFP ? 'T' : '@';
            pField->uiLen = 8;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pThisField->bFieldFlags = HB_FF_BINARY;
            pArea->uiRecordLen += pField->uiLen;
            break;

         case HB_FT_MODTIME:
            pThisField->bType = '=';
            pField->uiLen = 8;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            pArea->uiRecordLen += pField->uiLen;
            pArea->fModStamp = TRUE;
            break;

         case HB_FT_ROWVER:
            pThisField->bType = '^';
            pField->uiLen = 8;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            HB_PUT_LE_UINT32( pThisField->bCounter, 1 );
            pThisField->bStep = 1;
            pArea->uiRecordLen += pField->uiLen;
            pArea->fModStamp = TRUE;
            break;

         case HB_FT_AUTOINC:
            pThisField->bType = '+';
            pField->uiLen = 4;
            pThisField->bLen = ( BYTE ) pField->uiLen;
            HB_PUT_LE_UINT32( pThisField->bCounter, 1 );
            pThisField->bStep = 1;
            pArea->uiRecordLen += pField->uiLen;
            pArea->fAutoInc = TRUE;
            break;

         default:
            fError = TRUE;
      }
      if( fError )
      {
         hb_xfree( pBuffer );
         SELF_CLOSE( ( AREAP ) pArea );

         pError = hb_errNew();
         hb_errPutGenCode( pError, EG_CREATE );
         hb_errPutSubCode( pError, EDBF_DATATYPE );
         hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CREATE ) );
         hb_errPutFileName( pError, ( char * ) pCreateInfo->abName );
         SELF_ERROR( ( AREAP ) pArea, pError );
         hb_itemRelease( pError );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }
      pThisField++;
   }
   pArea->fShared = FALSE;    /* pCreateInfo->fShared; */
   pArea->fReadonly = FALSE;  /* pCreateInfo->fReadonly */
   pArea->ulRecCount = 0;
   pArea->uiHeaderLen = sizeof( DBFHEADER ) + uiSize +
                        ( pArea->bTableType == DB_DBF_VFP ? 1 : 2 );
   if( fRawBlob )
   {
      pArea->fHasMemo = TRUE;
   }
   if( !pArea->fHasMemo )
   {
      pArea->bMemoType = DB_MEMO_NONE;
   }
   pArea->uiMemoBlockSize = 0;

#ifndef HB_CDP_SUPPORT_OFF
   if( pCreateInfo->cdpId )
   {
      pArea->cdPage = hb_cdpFind( (char *) pCreateInfo->cdpId );
      if( !pArea->cdPage )
         pArea->cdPage = hb_cdp_page;
   }
   else
      pArea->cdPage = hb_cdp_page;
#endif

   pItem = hb_itemNew( NULL );
   if( SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_PENDINGPASSWORD,
                     pCreateInfo->ulConnection, pItem ) == SUCCESS )
   {
      if( hb_dbfPasswordSet( pArea, pItem, FALSE ) )
         pArea->fTableEncrypted = TRUE;
   }
   else
   {
      hb_itemClear( pItem );
      if( SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_PASSWORD,
                        pCreateInfo->ulConnection, pItem ) == SUCCESS )
      {
         if( hb_dbfPasswordSet( pArea, pItem, FALSE ) )
            pArea->fTableEncrypted = TRUE;
      }
   }
   hb_itemRelease( pItem );

   if( !fRawBlob )
   {
      /* Force write new header */
      pArea->fUpdateHeader = TRUE;
      /* Write header */
      errCode = SELF_WRITEDBHEADER( ( AREAP ) pArea );
      if( errCode != SUCCESS )
      {
         if( pBuffer )
            hb_xfree( pBuffer );
         SELF_CLOSE( ( AREAP ) pArea );
         pArea->lpdbOpenInfo = NULL;
         return errCode;
      }

      /* Write fields and eof mark */
      if( hb_fsWrite( pArea->hDataFile, ( BYTE * ) pBuffer, uiSize ) != uiSize ||
          ( pArea->bTableType == DB_DBF_VFP ?
               hb_fsWrite( pArea->hDataFile, ( BYTE * ) "\r\032", 2 ) != 2 :
               hb_fsWrite( pArea->hDataFile, ( BYTE * ) "\r\0\032", 3 ) != 3 ) )
      {
         /* TODO: add RT error */
         if( pBuffer )
            hb_xfree( pBuffer );
         SELF_CLOSE( ( AREAP ) pArea );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }

      pArea->fDataFlush = TRUE;
      if( pBuffer )
         hb_xfree( pBuffer );
   }

   /* Create memo file */
   if( pArea->fHasMemo )
   {
      pFileName = hb_fsFNameSplit( ( char * ) szFileName );
      pFileName->szExtension = NULL;
      hb_fsFNameMerge( ( char * ) szFileName, pFileName );
      hb_xfree( pFileName );
      pCreateInfo->abName = szFileName;
      errCode = SELF_CREATEMEMFILE( ( AREAP ) pArea, pCreateInfo );
   }
   /* If successful call SUPER_CREATE to finish system jobs */
   if( errCode == SUCCESS )
   {
      errCode = SUPER_CREATE( ( AREAP ) pArea, pCreateInfo );
   }

   if( errCode != SUCCESS )
   {
      SELF_CLOSE( ( AREAP ) pArea );
      pArea->lpdbOpenInfo = NULL;
      return errCode;
   }

   /* Alloc buffer */
   pArea->pRecord = ( BYTE * ) hb_xgrab( pArea->uiRecordLen );
   pArea->fValidBuffer = FALSE;

   /* Update the number of record for corrupted headers */
   pArea->ulRecCount = hb_dbfCalcRecCount( pArea );
   pArea->lpdbOpenInfo = NULL;

   /* Position cursor at the first record */
   return SELF_GOTOP( ( AREAP ) pArea );
}

/*
 * Retrieve information about the current driver.
 */
static ERRCODE hb_dbfInfo( DBFAREAP pArea, USHORT uiIndex, PHB_ITEM pItem )
{
   ERRCODE errCode = SUCCESS;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfInfo(%p, %hu, %p)", pArea, uiIndex, pItem));

   switch( uiIndex )
   {
      case DBI_ISDBF:
      case DBI_CANPUTREC:
         hb_itemPutL( pItem, TRUE );
         break;

      case DBI_GETHEADERSIZE:
         hb_itemPutNL( pItem, pArea->uiHeaderLen );
         break;

      case DBI_LASTUPDATE:
         hb_itemPutD( pItem, 1900 + pArea->dbfHeader.bYear,
                             pArea->dbfHeader.bMonth,
                             pArea->dbfHeader.bDay );
         break;

      case DBI_GETRECSIZE:
         hb_itemPutNL( pItem, pArea->uiRecordLen );
         break;

      case DBI_GETLOCKARRAY:
         hb_dbfGetLockArray( pArea, pItem );
         break;

      case DBI_TABLEEXT:
         hb_itemClear( pItem );
         return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_TABLEEXT, 0, pItem );

      case DBI_FULLPATH:
         hb_itemPutC( pItem, pArea->szDataFileName);
         break;

      case DBI_MEMOTYPE:
         hb_itemPutNI( pItem, DB_MEMO_NONE );
         break;

      case DBI_TABLETYPE:
         if( pArea->hDataFile == FS_ERROR )
         {
            hb_itemClear( pItem );
            return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_TABLETYPE, 0, pItem );
         }
         hb_itemPutNI( pItem, pArea->bTableType );
         break;

      case DBI_FILEHANDLE:
         hb_itemPutNL( pItem, ( LONG ) pArea->hDataFile );
         break;

      case DBI_MEMOHANDLE:
         hb_itemPutNL( pItem, ( LONG ) pArea->hMemoFile );
         break;

      case DBI_SHARED:
      {
         BOOL fShared = pArea->fShared;

         if( HB_IS_LOGICAL( pItem ) )
         {
            pArea->fShared = hb_itemGetL( pItem );
         }
         hb_itemPutL( pItem, fShared );
         break;
      }
      case DBI_ISFLOCK:
         hb_itemPutL( pItem, pArea->fFLocked );
         break;

      case DBI_ISREADONLY:
         hb_itemPutL( pItem, pArea->fReadonly );
         break;

      case DBI_VALIDBUFFER:
         hb_itemPutL( pItem, pArea->fValidBuffer );
         break;

      case DBI_POSITIONED:
         hb_itemPutL( pItem, pArea->fPositioned );
         break;

      case DBI_ISENCRYPTED:
         hb_itemPutL( pItem, pArea->fTableEncrypted );
         break;

      case DBI_DECRYPT:
         hb_dbfTableCrypt( pArea, pItem, FALSE );
         hb_itemPutL( pItem, !pArea->fTableEncrypted );
         break;

      case DBI_ENCRYPT:
         hb_dbfTableCrypt( pArea, pItem, TRUE );
         hb_itemPutL( pItem, pArea->fTableEncrypted );
         break;

      case DBI_LOCKCOUNT:
         hb_itemPutNL( pItem, pArea->ulNumLocksPos );
         break;

      case DBI_LOCKOFFSET:
      {
         HB_FOFFSET ulPos, ulFlSize, ulRlSize;
         int iDir;

         hb_dbfLockData( pArea, &ulPos, &ulFlSize, &ulRlSize, &iDir );
         hb_itemPutNInt( pItem, ulPos );
         break;
      }

      case DBI_LOCKSCHEME:
      {
         int iScheme = hb_itemGetNI( pItem );
         if( pArea->bLockType )
         {
            hb_itemPutNI( pItem, pArea->bLockType );
         }
         else
         {
            hb_itemClear( pItem );
            errCode = SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_LOCKSCHEME, 0, pItem );
         }
         switch( iScheme )
         {
            case DB_DBFLOCK_CLIP:
            case DB_DBFLOCK_CL53:
            case DB_DBFLOCK_CL53EXT:
            case DB_DBFLOCK_VFP:
#ifndef HB_LONG_LONG_OFF
            case DB_DBFLOCK_XHB64:
#endif
               pArea->bLockType = ( BYTE ) iScheme;
         }
         break;
      }
      case DBI_ROLLBACK:
         if( pArea->fRecordChanged )
         {
            if( pArea->fAppend )
            {
               hb_dbfSetBlankRecord( pArea, HB_BLANK_ROLLBACK );
               pArea->fDeleted = FALSE;
            }
            else
            {
               pArea->fRecordChanged = pArea->fValidBuffer = FALSE;
            }
         }
         break;

      case DBI_PASSWORD:
         hb_dbfPasswordSet( pArea, pItem, FALSE );
         break;

      case DBI_TRIGGER:
         if( HB_IS_LOGICAL( pItem ) )
            pArea->fTrigger = pArea->pTriggerSym && hb_itemGetL( pItem );
         else
         {
            PHB_DYNS pTriggerSym = pArea->pTriggerSym;
            if( HB_IS_STRING( pItem ) )
               hb_dbfTriggerSet( pArea, pItem );
            hb_itemPutC( pItem, pTriggerSym ? hb_dynsymName( pTriggerSym ) : NULL );
         }
         break;

      case DBI_OPENINFO:
         hb_itemPutPtr( pItem, pArea->lpdbOpenInfo );
         break;

      case DBI_DIRTYREAD:
      {
         BOOL fDirty = HB_DIRTYREAD( pArea );

         if( HB_IS_LOGICAL( pItem ) )
            pArea->uiDirtyRead = hb_itemGetL( pItem ) ?
                                 HB_IDXREAD_DIRTY : HB_IDXREAD_CLEAN;
         else if( !HB_IS_NIL( pItem ) )
            pArea->uiDirtyRead = HB_IDXREAD_DEFAULT;

         hb_itemPutL( pItem, fDirty );
         break;
      }
      case DBI_DB_VERSION:
      case DBI_RDD_VERSION:
      {
         char szBuf[ 64 ];
         int iSub = hb_itemGetNI( pItem );

         if( iSub == 1 )
            snprintf( szBuf, sizeof( szBuf ), "%d.%d (%s)", 0, 1, "DBF" );
         else if( iSub == 2 )
            snprintf( szBuf, sizeof( szBuf ), "%d.%d (%s:%d)", 0, 1, "DBF", pArea->rddID );
/*
            snprintf( szBuf, sizeof( szBuf ), "%d.%d (%s:%d)", 0, 1, pArea->pRddNode->szName, pArea->rddID );
*/
         else
            snprintf( szBuf, sizeof( szBuf ), "%d.%d", 0, 1 );
         hb_itemPutC( pItem, szBuf );
         break;
      }

      default:
         return SUPER_INFO( ( AREAP ) pArea, uiIndex, pItem );

   }

   return errCode;
}

/*
 * Retrieve information about a raw
 */
static ERRCODE hb_dbfRecInfo( DBFAREAP pArea, PHB_ITEM pRecID, USHORT uiInfoType, PHB_ITEM pInfo )
{
   ULONG ulRecNo = hb_itemGetNL( pRecID ), ulPrevRec = 0;
   ERRCODE errResult = SUCCESS;
   BOOL bDeleted;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfRecInfo(%p, %p, %hu, %p)", pArea, pRecID, uiInfoType, pInfo));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   if( ulRecNo == 0 )
   {
      ulRecNo = pArea->ulRecNo;
   }
   else if( ulRecNo != pArea->ulRecNo )
   {
      switch( uiInfoType )
      {
         case DBRI_DELETED:
         case DBRI_ENCRYPTED:
         case DBRI_RAWRECORD:
         case DBRI_RAWMEMOS:
         case DBRI_RAWDATA:
            ulPrevRec = pArea->ulRecNo;
            errResult = SELF_GOTO( ( AREAP ) pArea, ulRecNo );
            if( errResult != SUCCESS )
               return errResult;
            break;
      }
   }

   switch( uiInfoType )
   {
      case DBRI_DELETED:
         errResult = SELF_DELETED( ( AREAP ) pArea, &bDeleted );
         if( errResult == SUCCESS )
            hb_itemPutL( pInfo, bDeleted );
         break;

      case DBRI_LOCKED:
         /* Clipper also checks only fShared and RLOCK and ignore FLOCK */
         hb_itemPutL( pInfo, !pArea->fShared || /* pArea->fFLocked || */
                              hb_dbfIsLocked( pArea, ulRecNo ) );
         break;

      case DBRI_RECSIZE:
         hb_itemPutNL( pInfo, pArea->uiRecordLen );
         break;

      case DBRI_RECNO:
         hb_itemPutNL( pInfo, ulRecNo );
         break;

      case DBRI_UPDATED:
         hb_itemPutL( pInfo, ulRecNo == pArea->ulRecNo && pArea->fRecordChanged );
         break;

      case DBRI_ENCRYPTED:
         if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
            errResult = FAILURE;
         else
            hb_itemPutL( pInfo, pArea->fEncrypted );
         break;

      case DBRI_RAWRECORD:
         if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
            errResult = FAILURE;
         else
            hb_itemPutCL( pInfo, ( char * ) pArea->pRecord, pArea->uiRecordLen );
         break;

      case DBRI_RAWMEMOS:
      case DBRI_RAWDATA:
      {
         USHORT uiFields;
         BYTE *pResult;
         ULONG ulLength, ulLen;

         if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
         {
            errResult = FAILURE;
            break;
         }
         ulLength = uiInfoType == DBRI_RAWDATA ? pArea->uiRecordLen : 0;
         pResult = ( BYTE * ) hb_xgrab( ulLength + 1 );
         if( ulLength )
         {
            memcpy( pResult, pArea->pRecord, ulLength );
         }

         if( pArea->fHasMemo )
         {
            for( uiFields = 0; uiFields < pArea->uiFieldCount; uiFields++ )
            {
               if( pArea->lpFields[ uiFields ].uiType == HB_FT_MEMO ||
                   pArea->lpFields[ uiFields ].uiType == HB_FT_IMAGE ||
                   pArea->lpFields[ uiFields ].uiType == HB_FT_BLOB ||
                   pArea->lpFields[ uiFields ].uiType == HB_FT_OLE )
               {
                  errResult = SELF_GETVALUE( ( AREAP ) pArea, uiFields + 1, pInfo );
                  if( errResult != SUCCESS )
                     break;
                  ulLen = hb_itemGetCLen( pInfo );
                  if( ulLen > 0 )
                  {
                     pResult = ( BYTE * ) hb_xrealloc( pResult, ulLength + ulLen + 1 );
                     memcpy( pResult + ulLength, hb_itemGetCPtr( pInfo ), ulLen );
                     ulLength += ulLen;
                  }
               }
            }
         }
         hb_itemPutCPtr( pInfo, ( char * ) pResult, ulLength );
         break;
      }

      default:
         errResult = SUPER_RECINFO( ( AREAP ) pArea, pRecID, uiInfoType, pInfo );
   }
   if( ulPrevRec != 0 )
   {
      if( SELF_GOTO( ( AREAP ) pArea, ulPrevRec ) != SUCCESS &&
          errResult == SUCCESS )
         errResult = FAILURE;
   }
   return errResult;
}

/*
 * Clear the WorkArea for use.
 */
static ERRCODE hb_dbfNewArea( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfNewArea(%p)", pArea));

   if( SUPER_NEW( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   pArea->hDataFile = pArea->hMemoFile = pArea->hMemoTmpFile = FS_ERROR;
   pArea->fDataFlush = pArea->fMemoFlush = FALSE;
   /* Index dirty read flag initialized to global RDD setting */
   pArea->uiDirtyRead = HB_IDXREAD_DEFAULT;
   /* Size for deleted records flag */
   pArea->uiRecordLen = 1;
   return SUCCESS;
}

/*
 * Open a data store in the WorkArea.
 */
static ERRCODE hb_dbfOpen( DBFAREAP pArea, LPDBOPENINFO pOpenInfo )
{
   ERRCODE errCode;
   USHORT uiFlags, uiFields, uiSize, uiCount, uiSkip;
   BOOL fRetry, fRawBlob;
   PHB_ITEM pError, pItem;
   PHB_FNAME pFileName;
   BYTE * pBuffer;
   LPDBFFIELD pField;
   DBFIELDINFO dbFieldInfo;
   BYTE szFileName[ _POSIX_PATH_MAX + 1 ];
   char szAlias[ HARBOUR_MAX_RDD_ALIAS_LENGTH + 1 ];

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfOpen(%p, %p)", pArea, pOpenInfo));

   pArea->lpdbOpenInfo = pOpenInfo;

   pItem = hb_itemNew( NULL );
   
   if( SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_PENDINGTRIGGER,
                     pOpenInfo->ulConnection, pItem ) == SUCCESS )
   {
      if( HB_IS_STRING( pItem ) )
         hb_dbfTriggerSet( pArea, pItem );
   }

   if( !pArea->fTrigger )
   {
      if( SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_TRIGGER,
                        pOpenInfo->ulConnection, pItem ) == SUCCESS )
      {
         if( HB_IS_STRING( pItem ) )
            hb_dbfTriggerSet( pArea, pItem );
      }
   }

   if( pArea->fTrigger )
   {
      hb_itemPutC( pItem, ( char * ) pOpenInfo->abName );
      if( !hb_dbfTriggerDo( pArea, EVENT_PREUSE, 0, pItem ) )
      {
         hb_itemRelease( pItem );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }
      hb_strncpy( ( char * ) szFileName, hb_itemGetCPtr( pItem ), _POSIX_PATH_MAX );
   }
   else
      hb_strncpy( ( char * ) szFileName, ( char * ) pOpenInfo->abName, _POSIX_PATH_MAX );

   if( !pArea->bLockType )
   {
      hb_itemClear( pItem );
      if( SELF_INFO( ( AREAP ) pArea, DBI_LOCKSCHEME, pItem ) != SUCCESS )
      {
         hb_itemRelease( pItem );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }
      pArea->bLockType = hb_itemGetNI( pItem );
      if( !pArea->bLockType )
         pArea->bLockType = DB_DBFLOCK_CLIP;
   }
#ifndef HB_CDP_SUPPORT_OFF
   if( pOpenInfo->cdpId )
   {
      pArea->cdPage = hb_cdpFind( (char *) pOpenInfo->cdpId );
      if( !pArea->cdPage )
         pArea->cdPage = hb_cdp_page;
   }
   else
      pArea->cdPage = hb_cdp_page;
#endif
   pArea->fShared = pOpenInfo->fShared;
   pArea->fReadonly = pOpenInfo->fReadonly;
   /* Force exclusive mode
    *   0: AUTOSHARE disabled.
    *   1: AUTOSHARE enabled.
    *   2: force exclusive mode.
    * */
   if( hb_set.HB_SET_AUTOSHARE == 2 )
      pArea->fShared = FALSE;
   uiFlags = (pArea->fReadonly ? FO_READ : FO_READWRITE) |
             (pArea->fShared ? FO_DENYNONE : FO_EXCLUSIVE);
   pError = NULL;

   pFileName = hb_fsFNameSplit( ( char * ) szFileName );
   /* Add default file name extension if necessary */
   if( hb_set.HB_SET_DEFEXTENSIONS && ! pFileName->szExtension )
   {
      hb_itemClear( pItem );
      if( SELF_INFO( ( AREAP ) pArea, DBI_TABLEEXT, pItem ) != SUCCESS )
      {
         hb_xfree( pFileName );
         hb_itemRelease( pItem );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }
      pFileName->szExtension = hb_itemGetCPtr( pItem );
      hb_fsFNameMerge( ( char * ) szFileName, pFileName );
   }

   /* Create default alias if necessary */
   if( !pOpenInfo->atomAlias && pFileName->szName )
   {
      hb_strncpyUpperTrim( szAlias, pFileName->szName, HARBOUR_MAX_RDD_ALIAS_LENGTH );
      pOpenInfo->atomAlias = ( BYTE * ) szAlias;
   }
   hb_xfree( pFileName );

   hb_itemClear( pItem );
   fRawBlob = SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_BLOB_SUPPORT, pOpenInfo->ulConnection, pItem ) == SUCCESS &&
              hb_itemGetL( pItem );

   hb_itemRelease( pItem );

   if( fRawBlob )
   {
      uiFields = uiSkip = 0;
      pBuffer = NULL;
      pArea->fHasMemo = TRUE;
   }
   else
   {
      /* Try open */
      do
      {
         pArea->hDataFile = hb_fsExtOpen( szFileName, NULL, uiFlags |
                                          FXO_DEFAULTS | FXO_SHARELOCK | FXO_COPYNAME,
                                          NULL, pError );
         if( pArea->hDataFile == FS_ERROR )
         {
            if( !pError )
            {
               pError = hb_errNew();
               hb_errPutGenCode( pError, EG_OPEN );
               hb_errPutSubCode( pError, EDBF_OPEN_DBF );
               hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_OPEN ) );
               hb_errPutFileName( pError, ( char * ) szFileName );
               hb_errPutFlags( pError, EF_CANRETRY | EF_CANDEFAULT );
            }
            hb_errPutOsCode( pError, hb_fsError() );
            fRetry = ( SELF_ERROR( ( AREAP ) pArea, pError ) == E_RETRY );
         }
         else
            fRetry = FALSE;
      } while( fRetry );

      if( pError )
      {
         hb_itemRelease( pError );
         pError = NULL;
      }

      /* Exit if error */
      if( pArea->hDataFile == FS_ERROR )
      {
         SELF_CLOSE( ( AREAP ) pArea );
         pArea->lpdbOpenInfo = NULL;
         return FAILURE;
      }

      /* Allocate only after succesfully open file */
      pArea->szDataFileName = hb_strdup( ( char * ) szFileName );

      /* Read file header and exit if error */
      errCode = SELF_READDBHEADER( ( AREAP ) pArea );
      if( errCode != SUCCESS )
      {
         SELF_CLOSE( ( AREAP ) pArea );
         pArea->lpdbOpenInfo = NULL;
         return errCode;
      }

      /* Add fields */
      uiSkip = 0;
      uiFields = ( pArea->uiHeaderLen - sizeof( DBFHEADER ) ) / sizeof( DBFFIELD );
      uiSize = uiFields * sizeof( DBFFIELD );
      pBuffer = uiFields ? ( BYTE * ) hb_xgrab( uiSize ) : NULL;

      /* Read fields and exit if error */
      do
      {
         hb_fsSeek( pArea->hDataFile, sizeof( DBFHEADER ), FS_SET );
         if( hb_fsRead( pArea->hDataFile, pBuffer, uiSize ) != uiSize )
         {
            errCode = FAILURE;
            if( !pError )
            {
               pError = hb_errNew();
               hb_errPutGenCode( pError, EG_CORRUPTION );
               hb_errPutSubCode( pError, EDBF_CORRUPT );
               hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CORRUPTION ) );
               hb_errPutFileName( pError, pArea->szDataFileName );
               hb_errPutFlags( pError, EF_CANRETRY | EF_CANDEFAULT );
            }
            fRetry = ( SELF_ERROR( ( AREAP ) pArea, pError ) == E_RETRY );
         }
         else
         {
            errCode = SUCCESS;
            break;
         }
      } while( fRetry );

      if( pError )
      {
         hb_itemRelease( pError );
      }

      /* Exit if error */
      if( errCode != SUCCESS )
      {
         if( pBuffer )
            hb_xfree( pBuffer );
         SELF_CLOSE( ( AREAP ) pArea );
         pArea->lpdbOpenInfo = NULL;
         return errCode;
      }

      /* some RDDs use the additional space in the header after field arrray
         for private data we should check for 0x0D marker to not use this
         data as fields description */
      for( uiCount = 0; uiCount < uiFields; uiCount++ )
      {
         pField = ( LPDBFFIELD ) ( pBuffer + uiCount * sizeof( DBFFIELD ) );
         if( pField->bName[ 0 ] == 0x0d )
         {
            uiFields = uiCount;
            break;
         }
         else if( pArea->bTableType == DB_DBF_VFP &&
                  pField->bFieldFlags & 0x01 )
         {
            uiSkip++;
         }
      }
      uiFields -= uiSkip;
   }

   /* CL5.3 allow to create and open DBFs without fields */
#ifdef HB_C52_STRICT
   if( uiFields == 0 )
   {
      errCode = FAILURE;
   }
   else
#endif
   {
      errCode = SELF_SETFIELDEXTENT( ( AREAP ) pArea, uiFields );
      if( errCode != SUCCESS )
      {
         SELF_CLOSE( ( AREAP ) pArea );
         pArea->lpdbOpenInfo = NULL;
         return errCode;
      }
   }

   /* Clear dbFieldInfo structure */
   memset( &dbFieldInfo, 0, sizeof( dbFieldInfo ) );

   /* Size for deleted flag */
   pArea->uiRecordLen = 1;
   for( uiCount = 0; uiCount < uiFields + uiSkip; uiCount++ )
   {
      pField = ( LPDBFFIELD ) ( pBuffer + uiCount * sizeof( DBFFIELD ) );
      dbFieldInfo.atomName = pField->bName;
      dbFieldInfo.atomName[10] = '\0';
      /* hb_strUpper( (char *) dbFieldInfo.atomName, 11 ); */
      dbFieldInfo.uiLen = pField->bLen;
      dbFieldInfo.uiDec = 0;
      dbFieldInfo.uiTypeExtended = 0;
      /* We cannot accept bFieldFlags as is because Clipper
       * creates tables where this field is random so we have to
       * try to guess the flags ourself. But if we know that table
       * was created by VFP which uses field flags then we can
       * retrive information from bFieldFlags.
       */
      if( pArea->bTableType == DB_DBF_VFP )
         dbFieldInfo.uiFlags = pField->bFieldFlags;
      else
         dbFieldInfo.uiFlags = 0;
      switch( pField->bType )
      {
         case 'C':
            dbFieldInfo.uiType = HB_FT_STRING;
            dbFieldInfo.uiLen = pField->bLen + pField->bDec * 256;
            break;

         case 'L':
            dbFieldInfo.uiType = HB_FT_LOGICAL;
            dbFieldInfo.uiLen = 1;
            break;

         case 'D':
            dbFieldInfo.uiType = HB_FT_DATE;
            if( dbFieldInfo.uiLen != 3 && dbFieldInfo.uiLen != 4 )
               dbFieldInfo.uiLen = 8;
            break;

         case 'I':
            dbFieldInfo.uiType = HB_FT_INTEGER;
            if( ( dbFieldInfo.uiLen > 4 && dbFieldInfo.uiLen != 8 ) ||
                dbFieldInfo.uiLen == 0 )
               dbFieldInfo.uiLen = 4;
            dbFieldInfo.uiDec = pField->bDec;
            break;

         case 'Y':
            dbFieldInfo.uiType = HB_FT_CURRENCY;
            if( ( dbFieldInfo.uiLen > 4 && dbFieldInfo.uiLen != 8 ) ||
                dbFieldInfo.uiLen == 0 )
               dbFieldInfo.uiLen = 8;
            dbFieldInfo.uiDec = pField->bDec;
            break;

         case '2':
         case '4':
            dbFieldInfo.uiType = HB_FT_INTEGER;
            dbFieldInfo.uiLen = pField->bType - '0';
            break;

         case 'N':
            dbFieldInfo.uiType = HB_FT_LONG;
            dbFieldInfo.uiDec = pField->bDec;
            /* DBASE documentation defines maximum numeric field size as 20
             * but Clipper allows to create longer fields so I removed this
             * limit, Druzus
             */
            /*
            if( pField->bLen > 20 )
               errCode = FAILURE;
            */
            break;

         case 'F':
            dbFieldInfo.uiType = HB_FT_FLOAT;
            dbFieldInfo.uiDec = pField->bDec;
            /* See note above */
            break;

         case '8':
         case 'B':
            dbFieldInfo.uiType = HB_FT_DOUBLE;
            dbFieldInfo.uiDec = pField->bDec;
            if( dbFieldInfo.uiLen != 8 )
               errCode = FAILURE;
            break;

         /* types which are not supported by VM - mapped to different ones */
         case 'T':
            if( dbFieldInfo.uiLen == 8 )
               dbFieldInfo.uiType = HB_FT_DAYTIME;
            else if( dbFieldInfo.uiLen == 4 )
               dbFieldInfo.uiType = HB_FT_TIME;
            else
               errCode = FAILURE;
            break;

         case '@':
            dbFieldInfo.uiType = HB_FT_DAYTIME;
            if( dbFieldInfo.uiLen != 8 )
               errCode = FAILURE;
            break;

         case '=':
            dbFieldInfo.uiType = HB_FT_MODTIME;
            if( dbFieldInfo.uiLen != 8 )
               errCode = FAILURE;
            pArea->fModStamp = TRUE;
            break;

         case '^':
            dbFieldInfo.uiType = HB_FT_ROWVER;
            if( dbFieldInfo.uiLen != 8 )
               errCode = FAILURE;
            pArea->fModStamp = TRUE;
            break;

         case '+':
            dbFieldInfo.uiType = HB_FT_AUTOINC;
            if( dbFieldInfo.uiLen != 4 )
               errCode = FAILURE;
            pArea->fAutoInc = TRUE;
            break;

         case 'Q':
            dbFieldInfo.uiType = HB_FT_VARLENGTH;
            dbFieldInfo.uiFlags |= HB_FF_BINARY;
            break;

         case 'V':
            if( pArea->bTableType == DB_DBF_VFP )
            {
               dbFieldInfo.uiType = HB_FT_VARLENGTH;
            }
            else
            {
               dbFieldInfo.uiType = HB_FT_ANY;
               if( dbFieldInfo.uiLen >= 6 )
               {
                  pArea->uiMemoVersion = DB_MEMOVER_SIX;
                  pArea->fHasMemo = TRUE;
               }
            }
            break;

         case 'M':
            dbFieldInfo.uiType = HB_FT_MEMO;
            pArea->fHasMemo = TRUE;
            break;

         case 'P':
            dbFieldInfo.uiType = HB_FT_IMAGE;
            dbFieldInfo.uiFlags |= HB_FF_BINARY;
            pArea->fHasMemo = TRUE;
            break;

         case 'W':
            dbFieldInfo.uiType = HB_FT_BLOB;
            dbFieldInfo.uiFlags |= HB_FF_BINARY;
            pArea->fHasMemo = TRUE;
            break;

         case 'G':
            dbFieldInfo.uiType = HB_FT_OLE;
            dbFieldInfo.uiFlags |= HB_FF_BINARY;
            pArea->fHasMemo = TRUE;
            break;

         case '0':
            if( pArea->bTableType == DB_DBF_VFP && pField->bFieldFlags & 0x01 )
            {
               if( memcmp( dbFieldInfo.atomName, "_NullFlags", 10 ) == 0 )
               {
                  /* TODO: NULLABLE and VARLENGTH support */
               }
               pArea->uiRecordLen += dbFieldInfo.uiLen;
               continue;
            }

         default:
            errCode = FAILURE;
            break;
      }

      /* Add field */
      if( errCode == SUCCESS )
         errCode = SELF_ADDFIELD( ( AREAP ) pArea, &dbFieldInfo );

      /* Exit if error */
      if( errCode != SUCCESS )
         break;
   }
   if( pBuffer )
      hb_xfree( pBuffer );

   /* Exit if error */
   if( errCode != SUCCESS )
   {
      if( hb_vmRequestQuery() == 0 )
      {
         pError = hb_errNew();
         hb_errPutGenCode( pError, EG_CORRUPTION );
         hb_errPutSubCode( pError, EDBF_CORRUPT );
         hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CORRUPTION ) );
         hb_errPutFileName( pError, pArea->szDataFileName );
         hb_errPutFlags( pError, EF_CANDEFAULT );
         SELF_ERROR( ( AREAP ) pArea, pError );
         hb_itemRelease( pError );
      }
      SELF_CLOSE( ( AREAP ) pArea );
      pArea->lpdbOpenInfo = NULL;
      return errCode;
   }

   pItem = hb_itemNew( NULL );
   if( SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_PENDINGPASSWORD,
                     pOpenInfo->ulConnection, pItem ) == SUCCESS )
   {
      hb_dbfPasswordSet( pArea, pItem, FALSE );
   }
   else
   {
      hb_itemClear( pItem );
      if( SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_PASSWORD,
                        pOpenInfo->ulConnection, pItem ) == SUCCESS )
      {
         hb_dbfPasswordSet( pArea, pItem, FALSE );
      }
   }
   hb_itemRelease( pItem );

   /* Open memo file if exists */
   if( pArea->fHasMemo )
   {
      pFileName = hb_fsFNameSplit( ( char * ) szFileName );
      pFileName->szExtension = NULL;
      hb_fsFNameMerge( ( char * ) szFileName, pFileName );
      hb_xfree( pFileName );
      pOpenInfo->abName = szFileName;
      errCode = SELF_OPENMEMFILE( ( AREAP ) pArea, pOpenInfo );
   }

   if( errCode == SUCCESS )
   {
      /* If successful call SUPER_OPEN to finish system jobs */
      errCode = SUPER_OPEN( ( AREAP ) pArea, pOpenInfo );
   }

   if( errCode != SUCCESS )
   {
      SELF_CLOSE( ( AREAP ) pArea );
      pArea->lpdbOpenInfo = NULL;
      return FAILURE;
   }

   /* Alloc buffer */
   pArea->pRecord = ( BYTE * ) hb_xgrab( pArea->uiRecordLen );
   pArea->fValidBuffer = FALSE;

   /* Update the number of record for corrupted headers */
   pArea->ulRecCount = hb_dbfCalcRecCount( pArea );

   /* Position cursor at the first record */
   errCode = SELF_GOTOP( ( AREAP ) pArea );

   if( pArea->fTrigger )
      hb_dbfTriggerDo( pArea, EVENT_POSTUSE, 0, NULL );

   pArea->lpdbOpenInfo = NULL;

   return errCode;
}

/*
 * Retrieve the size of the WorkArea structure.
 */
static ERRCODE hb_dbfStructSize( DBFAREAP pArea, USHORT * uiSize )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfStrucSize(%p, %p)", pArea, uiSize));
   HB_SYMBOL_UNUSED( pArea );

   * uiSize = sizeof( DBFAREA );
   return SUCCESS;
}

/*
 * Pack helper function called for each packed record
 */
static ERRCODE hb_dbfPackRec( DBFAREAP pArea, ULONG ulRecNo, BOOL *fWritten )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfPackRec(%p, %lu, %p)", pArea, ulRecNo, fWritten));

   HB_SYMBOL_UNUSED( ulRecNo );

   *fWritten = !pArea->fDeleted;

   return SUCCESS;
}

/*
 * Remove records marked for deletion from a database.
 */
static ERRCODE hb_dbfPack( DBFAREAP pArea )
{
   ULONG ulRecIn, ulRecOut, ulEvery, ulUserEvery;
   PHB_ITEM pError, pBlock;
   BOOL fWritten;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfPack(%p)", pArea));

   if( pArea->fReadonly )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_READONLY );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_READONLY ) );
      hb_errPutSubCode( pError, EDBF_READONLY );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }
   if( pArea->fShared )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_SHARED );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_SHARED ) );
      hb_errPutSubCode( pError, EDBF_SHARED );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }

   if( pArea->fTrigger )
   {
      if( !hb_dbfTriggerDo( pArea, EVENT_PACK, 0, NULL ) )
         return FAILURE;
   }

   if( SELF_GOCOLD( ( AREAP ) pArea ) != SUCCESS )
      return FAILURE;

   if( HB_IS_ARRAY( pArea->valResult ) && hb_arrayLen( pArea->valResult ) == 2 )
   {
      pBlock = hb_arrayGetItemPtr( pArea->valResult, 1 );
      ulUserEvery = hb_arrayGetNL( pArea->valResult, 2 );
      if( ulUserEvery < 1 )
         ulUserEvery = 1;
   }
   else
   {
      pBlock = NULL;
      ulUserEvery = 1;
   }

   ulRecOut = ulEvery = 0;
   ulRecIn = 1;
   while( ulRecIn <= pArea->ulRecCount )
   {
      if( SELF_GOTO( ( AREAP ) pArea, ulRecIn ) != SUCCESS )
         return FAILURE;
      if( !hb_dbfReadRecord( pArea ) )
         return FAILURE;

      /* Execute the Code Block */
      if( pBlock )
      {
         if( ++ulEvery >= ulUserEvery )
         {
            ulEvery = 0;
            if( SELF_EVALBLOCK( ( AREAP ) pArea, pBlock ) != SUCCESS )
               return FAILURE;
         }
      }

      if( SELF_PACKREC( ( AREAP ) pArea, ulRecOut + 1, &fWritten ) != SUCCESS )
         return FAILURE;

      if( fWritten )
      {
         ulRecOut++;
         if( pArea->ulRecNo != ulRecOut || pArea->fRecordChanged )
         {
            pArea->ulRecNo = ulRecOut;
            pArea->fRecordChanged = TRUE;
            if( ! hb_dbfWriteRecord( pArea ) )
               return FAILURE;
         }
      }
      ulRecIn++;
   }

   /* Execute the Code Block for pending record */
   if( pBlock && ulEvery > 0 )
   {
      if( SELF_EVALBLOCK( ( AREAP ) pArea, pBlock ) != SUCCESS )
         return FAILURE;
   }

   if( pArea->ulRecCount != ulRecOut )
   {
      pArea->ulRecCount = ulRecOut;
      /* Force write new header */
      pArea->fUpdateHeader = TRUE;
      if( SELF_WRITEDBHEADER( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }
   return SELF_GOTO( ( AREAP ) pArea, 1 );
}

#ifndef HB_CDP_SUPPORT_OFF
HB_EXPORT void hb_dbfTranslateRec( DBFAREAP pArea, BYTE * pBuffer, PHB_CODEPAGE cdp_src, PHB_CODEPAGE cdp_dest )
{
   USHORT uiIndex;
   LPFIELD pField;

   for( uiIndex = 0, pField = pArea->lpFields; uiIndex < pArea->uiFieldCount; uiIndex++, pField++ )
   {
      if( pField->uiType == HB_FT_STRING && ( pField->uiFlags && HB_FF_BINARY ) == 0 )
      {
         hb_cdpnTranslate( ( char * ) pBuffer + pArea->pFieldOffset[ uiIndex ], cdp_src, cdp_dest, pField->uiLen );
      }
   }

}
#endif

/*
 * Physically reorder a database.
 */
static ERRCODE hb_dbfSort( DBFAREAP pArea, LPDBSORTINFO pSortInfo )
{
   ULONG ulRecNo;
   USHORT uiCount;
   BOOL bMoreRecords, bLimited, bValidRecord;
   ERRCODE uiError;
   DBQUICKSORT dbQuickSort;
   BYTE * pBuffer;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfSort(%p, %p)", pArea, pSortInfo));

   if( SELF_GOCOLD( ( AREAP ) pArea ) != SUCCESS )
      return FAILURE;

   if( !hb_dbQSortInit( &dbQuickSort, pSortInfo, pArea->uiRecordLen ) )
      return FAILURE;

   uiError = SUCCESS;
   uiCount = 0;
   pBuffer = dbQuickSort.pBuffer;
   ulRecNo = 1;
   if( pSortInfo->dbtri.dbsci.itmRecID )
   {
      uiError = SELF_GOTOID( ( AREAP ) pArea, pSortInfo->dbtri.dbsci.itmRecID );
      bMoreRecords = bLimited = TRUE;
   }
   else if( pSortInfo->dbtri.dbsci.lNext )
   {
      ulRecNo = hb_itemGetNL( pSortInfo->dbtri.dbsci.lNext );
      bLimited = TRUE;
      bMoreRecords = ( ulRecNo > 0 );
   }
   else
   {
      if( !pSortInfo->dbtri.dbsci.itmCobWhile &&
          ( !pSortInfo->dbtri.dbsci.fRest ||
            !hb_itemGetL( pSortInfo->dbtri.dbsci.fRest ) ) )
         uiError = SELF_GOTOP( ( AREAP ) pArea );
      bMoreRecords = TRUE;
      bLimited = FALSE;
   }

   while( uiError == SUCCESS && !pArea->fEof && bMoreRecords )
   {
      if( pSortInfo->dbtri.dbsci.itmCobWhile )
      {
         if( SELF_EVALBLOCK( ( AREAP ) pArea, pSortInfo->dbtri.dbsci.itmCobWhile ) != SUCCESS )
         {
            hb_dbQSortExit( &dbQuickSort );
            return FAILURE;
         }
         bMoreRecords = hb_itemGetL( pArea->valResult );
      }

      if( bMoreRecords && pSortInfo->dbtri.dbsci.itmCobFor )
      {
         if( SELF_EVALBLOCK( ( AREAP ) pArea, pSortInfo->dbtri.dbsci.itmCobFor ) != SUCCESS )
         {
            hb_dbQSortExit( &dbQuickSort );
            return FAILURE;
         }
         bValidRecord = hb_itemGetL( pArea->valResult );
      }
      else
         bValidRecord = bMoreRecords;

      if( bValidRecord )
      {
         if( uiCount == dbQuickSort.uiMaxRecords )
         {
            if( !hb_dbQSortAdvance( &dbQuickSort, uiCount ) )
            {
               hb_dbQSortExit( &dbQuickSort );
               return FAILURE;
            }
            pBuffer = dbQuickSort.pBuffer;
            uiCount = 0;
         }

         /* Read record */
         if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
         {
            hb_dbQSortExit( &dbQuickSort );
            return FAILURE;
         }

         /* Copy data */
         memcpy( pBuffer, pArea->pRecord, pArea->uiRecordLen );
#ifndef HB_CDP_SUPPORT_OFF
         if( pArea->cdPage != hb_cdp_page )
         {
            hb_dbfTranslateRec( pArea, pBuffer, pArea->cdPage, hb_cdp_page );
         }
#endif
         pBuffer += pArea->uiRecordLen;
         uiCount++;
      }

      if( bMoreRecords && bLimited )
         bMoreRecords = ( --ulRecNo > 0 );
      if( bMoreRecords )
         uiError = SELF_SKIP( ( AREAP ) pArea, 1 );
   }

   /* Copy last records */
   if( uiCount > 0 )
   {
      if( !hb_dbQSortAdvance( &dbQuickSort, uiCount ) )
      {
         hb_dbQSortExit( &dbQuickSort );
         return FAILURE;
      }
   }

   /* Sort records */
   hb_dbQSortComplete( &dbQuickSort );
   return SUCCESS;
}

/*
 * Copy one or more records from one WorkArea to another.
 */
static ERRCODE hb_dbfTrans( DBFAREAP pArea, LPDBTRANSINFO pTransInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfTrans(%p, %p)", pArea, pTransInfo));

   if( pTransInfo->uiFlags & DBTF_MATCH )
   {
      if( pArea->fHasMemo || pArea->cdPage != pTransInfo->lpaDest->cdPage )
         pTransInfo->uiFlags &= ~DBTF_PUTREC;
      else if( pArea->rddID == pTransInfo->lpaDest->rddID )
         pTransInfo->uiFlags |= DBTF_PUTREC;
      else
      {
         PHB_ITEM pPutRec = hb_itemPutL( NULL, FALSE );
         if( SELF_INFO( ( AREAP ) pTransInfo->lpaDest, DBI_CANPUTREC, pPutRec ) != SUCCESS )
         {
            hb_itemRelease( pPutRec );
            return FAILURE;
         }
         if( hb_itemGetL( pPutRec ) )
            pTransInfo->uiFlags |= DBTF_PUTREC;
         else
            pTransInfo->uiFlags &= ~DBTF_PUTREC;
         hb_itemRelease( pPutRec );
      }
   }
   return SUPER_TRANS( ( AREAP ) pArea, pTransInfo );
}

/*
 * Physically remove all records from data store.
 */
static ERRCODE hb_dbfZap( DBFAREAP pArea )
{
   PHB_ITEM pError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfZap(%p)", pArea));

   if( pArea->fReadonly )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_READONLY );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_READONLY ) );
      hb_errPutSubCode( pError, EDBF_READONLY );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }
   if( pArea->fShared )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_SHARED );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_SHARED ) );
      hb_errPutSubCode( pError, EDBF_SHARED );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }

   if( pArea->fTrigger )
   {
      if( !hb_dbfTriggerDo( pArea, EVENT_ZAP, 0, NULL ) )
         return FAILURE;
   }

   if( SELF_GOCOLD( ( AREAP ) pArea ) != SUCCESS )
      return FAILURE;

   pArea->fUpdateHeader = TRUE;
   pArea->ulRecCount = 0;
   if( SELF_WRITEDBHEADER( ( AREAP ) pArea ) != SUCCESS )
      return FAILURE;
   if( SELF_GOTO( ( AREAP ) pArea, 0 ) != SUCCESS )
      return FAILURE;

   /* Zap memo file */
   if( pArea->fHasMemo )
   {
      if( SELF_CREATEMEMFILE( ( AREAP ) pArea, NULL ) != SUCCESS )
         return FAILURE;
   }
   return SUCCESS;
}

/*
 * Report end of relation.
 */
static ERRCODE hb_dbfChildEnd( DBFAREAP pArea, LPDBRELINFO pRelInfo )
{
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfChildEnd(%p, %p)", pArea, pRelInfo));

   if( pArea->lpdbPendingRel == pRelInfo )
      uiError = SELF_FORCEREL( ( AREAP ) pArea );
   else
      uiError = SUCCESS;
   SUPER_CHILDEND( ( AREAP ) pArea, pRelInfo );
   return uiError;
}

/*
 * Report initialization of a relation.
 */
static ERRCODE hb_dbfChildStart( DBFAREAP pArea, LPDBRELINFO pRelInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfChildStart(%p, %p)", pArea, pRelInfo));

   if( SELF_CHILDSYNC( ( AREAP ) pArea, pRelInfo ) != SUCCESS )
      return FAILURE;
   return SUPER_CHILDSTART( ( AREAP ) pArea, pRelInfo );
}

/*
 * Post a pending relational movement.
 */
static ERRCODE hb_dbfChildSync( DBFAREAP pArea, LPDBRELINFO pRelInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfChildSync(%p, %p)", pArea, pRelInfo));

   /*
    * !!! The side effect of calling GOCOLD() inside CHILDSYNC() is
    * evaluation of index expressions (index KEY and FOR condition)
    * when the pArea is not the current one - it means that the
    * used RDD has to set proper work area before eval.
    * IMHO GOCOLD() could be safely removed from this place but I'm not
    * sure it's Clipper compatible - I will have to check it, Druzus.
    */
   /*
    * I've checked in CL5.3 Technical Reference Guide that only
    * FORCEREL() should ensure that the work area buffer is not HOT
    * and then call RELEVAL() - I hope it describes the CL5.3 DBF* RDDs
    * behavior so I replicate it - the GOCOLD() is moved from CHILDSYNC()
    * to FORCEREL(), Druzus.
    */
   /*
    * After some cleanups, the core DBF* code can work with GOCOLD() here
    * and in FORCEREL() without any problems. Because calling GOCOLD() in
    * FORCEREL() may interacts with badly written users RDD which inherits
    * from DBF* RDDs and/or user triggers then I decided to keep it here,
    * Druzus.
    */

   if( SELF_GOCOLD( ( AREAP ) pArea ) != SUCCESS )
      return FAILURE;

   pArea->lpdbPendingRel = pRelInfo;

   if( pArea->lpdbRelations )
      return SELF_SYNCCHILDREN( ( AREAP ) pArea );

   return SUCCESS;
}

/*
 * Force relational seeks in the specified WorkArea.
 */
static ERRCODE hb_dbfForceRel( DBFAREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfForceRel(%p)", pArea));

   if( pArea->lpdbPendingRel )
   {
      LPDBRELINFO lpdbPendingRel;

      lpdbPendingRel = pArea->lpdbPendingRel;
      pArea->lpdbPendingRel = NULL;

      /* update buffers */
      /* commented out - see comment above in CHILDSYNC() method, Druzus */
      /* SELF_GOCOLD( ( AREAP ) pArea ); */

      return SELF_RELEVAL( ( AREAP ) pArea, lpdbPendingRel );
   }
   return SUCCESS;
}

/*
 * Set the filter condition for the specified WorkArea.
 */
static ERRCODE hb_dbfSetFilter( DBFAREAP pArea, LPDBFILTERINFO pFilterInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfSetFilter(%p, %p)", pArea, pFilterInfo));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   return SUPER_SETFILTER( ( AREAP ) pArea, pFilterInfo );
}

/*
 * Perform a network lowlevel lock in the specified WorkArea.
 */
static ERRCODE hb_dbfRawLock( DBFAREAP pArea, USHORT uiAction, ULONG ulRecNo )
{
   ERRCODE uiErr = SUCCESS;
   HB_FOFFSET ulPos, ulFlSize, ulRlSize;
   int iDir;
   BOOL fLck;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfRawLock(%p, %hu, %lu)", pArea, uiAction, ulRecNo));

   if( pArea->fShared )
   {
      if( hb_dbfLockData( pArea, &ulPos, &ulFlSize, &ulRlSize, &iDir ) == FAILURE )
         return FAILURE;

      switch( uiAction )
      {
         case FILE_LOCK:
            if( !pArea->fFLocked )
            {
               if( iDir < 0 )
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos - ulFlSize, ulFlSize, FL_LOCK );
               else
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos + 1, ulFlSize, FL_LOCK );

               if( !fLck )
                  uiErr = FAILURE;
               else
                  pArea->fFLocked = TRUE;
            }
            break;

         case FILE_UNLOCK:
            if( pArea->fFLocked )
            {
               if( iDir < 0 )
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos - ulFlSize, ulFlSize, FL_UNLOCK );
               else
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos + 1, ulFlSize, FL_UNLOCK );

               if( !fLck )
                  uiErr = FAILURE;
               pArea->fFLocked = FALSE;
            }
            break;

         case REC_LOCK:
            if( !pArea->fFLocked )
            {
               if( iDir < 0 )
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos - ulRecNo, ulRlSize, FL_LOCK );
               else if( iDir == 2 )
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos + ( ulRecNo - 1 ) * pArea->uiRecordLen + pArea->uiHeaderLen, ulRlSize, FL_LOCK );
               else
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos + ulRecNo, ulRlSize, FL_LOCK );

               if( !fLck )
                  uiErr = FAILURE;
            }
            break;

         case REC_UNLOCK:
            if( !pArea->fFLocked )
            {
               if( iDir < 0 )
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos - ulRecNo, ulRlSize, FL_UNLOCK );
               else if( iDir == 2 )
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos + ( ulRecNo - 1 ) * pArea->uiRecordLen + pArea->uiHeaderLen, ulRlSize, FL_UNLOCK );
               else
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos + ulRecNo, ulRlSize, FL_UNLOCK );
               if( !fLck )
                  uiErr = FAILURE;
            }
            break;

         case APPEND_LOCK:
         case HEADER_LOCK:
            if( !pArea->fHeaderLocked )
            {
               do
               {
                  fLck = hb_fsLockLarge( pArea->hDataFile, ulPos, 1, FL_LOCK | FLX_WAIT );
                  /* TODO: call special error handler (LOCKHANDLER) hiere if !fLck */
               } while( !fLck );
               if( !fLck )
                  uiErr = FAILURE;
               else
                  pArea->fHeaderLocked = TRUE;
            }
            break;

         case APPEND_UNLOCK:
         case HEADER_UNLOCK:
            if( pArea->fHeaderLocked )
            {
               if( !hb_fsLockLarge( pArea->hDataFile, ulPos, 1, FL_UNLOCK ) )
                  uiErr = FAILURE;
               pArea->fHeaderLocked = FALSE;
            }
            break;
      }
   }
   return uiErr;
}

/*
 * Perform a network lock in the specified WorkArea.
 */
static ERRCODE hb_dbfLock( DBFAREAP pArea, LPDBLOCKINFO pLockInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfLock(%p, %p)", pArea, pLockInfo));

   if( pArea->fShared )
   {
      switch( pLockInfo->uiMethod )
      {
         case DBLM_EXCLUSIVE:
            return hb_dbfLockRecord( pArea, 0, &pLockInfo->fResult, TRUE );

         case DBLM_MULTIPLE:
            return hb_dbfLockRecord( pArea, hb_itemGetNL( pLockInfo->itmRecID ),
                                     &pLockInfo->fResult, FALSE );

         case DBLM_FILE:
            return hb_dbfLockFile( pArea, &pLockInfo->fResult );

         default:
            pLockInfo->fResult = FALSE;
      }
   }
   else
      pLockInfo->fResult = TRUE;

   return SUCCESS;
}

/*
 * Release network locks in the specified WorkArea.
 */
static ERRCODE hb_dbfUnLock( DBFAREAP pArea, PHB_ITEM pRecNo )
{
   ERRCODE uiError;
   ULONG ulRecNo;

   HB_TRACE(HB_TR_DEBUG, ("dbfUnLock(%p, %p)", pArea, pRecNo));

   ulRecNo = hb_itemGetNL( pRecNo );

   uiError = SUCCESS;
   if( pArea->fShared )
   {
      if( pArea->ulNumLocksPos > 0 )
      {
         /* Unlock all records? */
         if( ulRecNo == 0 )
            uiError = hb_dbfUnlockAllRecords( pArea );
         else if( hb_dbfIsLocked( pArea, ulRecNo ) )
            uiError = hb_dbfUnlockRecord( pArea, ulRecNo );
      }
      if( pArea->fFLocked )
      {
         uiError = hb_dbfUnlockFile( pArea );
      }
   }
   return uiError;
}

/*
 * Create a memo file in the WorkArea.
 */
static ERRCODE hb_dbfCreateMemFile( DBFAREAP pArea, LPDBOPENINFO pCreateInfo )
{
   PHB_ITEM pError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfCreateMemFile(%p, %p)", pArea, pCreateInfo));

   if( pCreateInfo )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_CREATE );
      hb_errPutSubCode( pError, EDBF_DATATYPE );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CREATE ) );
      hb_errPutFileName( pError, ( char * ) pCreateInfo->abName );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
   }
   pArea->fHasMemo = FALSE;
   return FAILURE;
}

/*
 * BLOB2FILE - retrieve memo contents into file
 */
static ERRCODE hb_dbfGetValueFile( DBFAREAP pArea, USHORT uiIndex, BYTE * szFile, USHORT uiMode )
{
   USHORT uiError = SUCCESS;
   LPFIELD pField;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfGetValueFile(%p, %hu, %s, %hu)", pArea, uiIndex, szFile, uiMode));

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   /* Read record */
   if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
      return FAILURE;

   if( --uiIndex >= pArea->uiFieldCount )
      return FAILURE;

   pField = pArea->lpFields + uiIndex;
   if( pField->uiType == HB_FT_STRING )
   {
      FHANDLE hFile;

      hFile = hb_fsExtOpen( szFile, NULL, FO_WRITE | FO_EXCLUSIVE |
                            FXO_DEFAULTS | FXO_SHARELOCK |
                            ( uiMode == FILEGET_APPEND ? FXO_APPEND : FXO_TRUNCATE ),
                            NULL, NULL );
      if( hFile == FS_ERROR )
      {
         uiError = uiMode != FILEGET_APPEND ? EDBF_CREATE : EDBF_OPEN_DBF;
      }
      else
      {
         hb_fsSeekLarge( hFile, 0, FS_END );
         if( hb_fsWrite( hFile, pArea->pRecord + pArea->pFieldOffset[ uiIndex ],
                         pField->uiLen ) != pField->uiLen )
         {
            uiError = EDBF_WRITE;
         }
         hb_fsClose( hFile );
      }
   }
   else
   {
      uiError = EDBF_DATATYPE;
   }

   /* Exit if any error */
   if( uiError != SUCCESS )
   {
      PHB_ITEM pError = hb_errNew();
      hb_errPutGenCode( pError, hb_dbfGetEGcode( uiError ) );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( hb_dbfGetEGcode( uiError ) ) );
      hb_errPutSubCode( pError, uiError );
      hb_errPutFlags( pError, EF_CANDEFAULT );
      if( uiError != EDBF_DATATYPE )
      {
         hb_errPutOsCode( pError, hb_fsError() );
         hb_errPutFileName( pError, ( char * ) szFile );
      }
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }
   return SUCCESS;
}

/*
 * Open a memo file in the specified WorkArea.
 */
static ERRCODE hb_dbfOpenMemFile( DBFAREAP pArea, LPDBOPENINFO pOpenInfo )
{
   PHB_ITEM pError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfOpenMemFile(%p, %p)", pArea, pOpenInfo));

   pError = hb_errNew();
   hb_errPutGenCode( pError, EG_OPEN );
   hb_errPutSubCode( pError, EDBF_OPEN_DBF );
   hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_OPEN ) );
   hb_errPutFileName( pError, ( char * ) pOpenInfo->abName );
   SELF_ERROR( ( AREAP ) pArea, pError );
   hb_itemRelease( pError );
   return FAILURE;
}

/*
 * FILE2BLOB - store file contents in MEMO
 */
static ERRCODE hb_dbfPutValueFile( DBFAREAP pArea, USHORT uiIndex, BYTE * szFile, USHORT uiMode )
{
   USHORT uiError = SUCCESS, uiRead;
   LPFIELD pField;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfPutValueFile(%p, %hu, %s, %hu)", pArea, uiIndex, szFile, uiMode));

   HB_SYMBOL_UNUSED( uiMode );

   if( pArea->lpdbPendingRel )
   {
      if( SELF_FORCEREL( ( AREAP ) pArea ) != SUCCESS )
         return FAILURE;
   }

   /* Read record */
   if( !pArea->fValidBuffer && !hb_dbfReadRecord( pArea ) )
      return FAILURE;

   if( --uiIndex >= pArea->uiFieldCount )
      return FAILURE;

   if( !pArea->fPositioned )
      return FAILURE;

   /* Buffer is hot? */
   if( !pArea->fRecordChanged && SELF_GOHOT( ( AREAP ) pArea ) == FAILURE )
      return FAILURE;

   pField = pArea->lpFields + uiIndex;
   if( pField->uiType == HB_FT_STRING )
   {
      FHANDLE hFile;

      hFile = hb_fsExtOpen( szFile, NULL, FO_READ | FO_DENYNONE |
                            FXO_DEFAULTS | FXO_SHARELOCK, NULL, NULL );
      if( hFile == FS_ERROR )
      {
         uiError = EDBF_OPEN_DBF;
      }
      else
      {
         uiRead = hb_fsRead( hFile, pArea->pRecord +
                             pArea->pFieldOffset[ uiIndex ], pField->uiLen );
         if( uiRead != ( USHORT ) FS_ERROR && uiRead < pField->uiLen )
            memset( pArea->pRecord + pArea->pFieldOffset[ uiIndex ] + uiRead,
                    ' ', pField->uiLen - uiRead );
         hb_fsClose( hFile );
      }
   }
   else
   {
      uiError = EDBF_DATATYPE;
   }

   /* Exit if any error */
   if( uiError != SUCCESS )
   {
      PHB_ITEM pError = hb_errNew();
      hb_errPutGenCode( pError, hb_dbfGetEGcode( uiError ) );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( hb_dbfGetEGcode( uiError ) ) );
      hb_errPutSubCode( pError, uiError );
      hb_errPutFlags( pError, EF_CANDEFAULT );
      if( uiError != EDBF_DATATYPE )
      {
         hb_errPutOsCode( pError, hb_fsError() );
         hb_errPutFileName( pError, ( char * ) szFile );
      }
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }
   return SUCCESS;
}

/*
 * Read the database file header record in the WorkArea.
 */
static ERRCODE hb_dbfReadDBHeader( DBFAREAP pArea )
{
   BOOL fRetry, fError;
   PHB_ITEM pError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfReadDBHeader(%p)", pArea));

   pError = NULL;
   /* Try read */
   do
   {
      fError = FALSE;

      hb_fsSeek( pArea->hDataFile, 0, FS_SET );
      if( hb_fsRead( pArea->hDataFile, ( BYTE * ) &pArea->dbfHeader,
                     sizeof( DBFHEADER ) ) != sizeof( DBFHEADER ) )
      {
         fError = TRUE;
      }
      else
      {
         pArea->fAutoInc = pArea->fModStamp =
         pArea->fTableEncrypted = pArea->fHasMemo = FALSE;
         pArea->bTableType = DB_DBF_STD;
         pArea->bMemoType  = DB_MEMO_NONE;
         pArea->bCryptType = DB_CRYPT_NONE;

         pArea->fHasTags = ( pArea->dbfHeader.bHasTags & 0x01 ) != 0;

         switch( pArea->dbfHeader.bVersion )
         {
            case 0x31:
               pArea->fAutoInc = TRUE;
            case 0x30:
               pArea->bTableType = DB_DBF_VFP;
               if( pArea->dbfHeader.bHasTags & 0x02 )
               {
                  pArea->bMemoType = DB_MEMO_FPT;
                  pArea->fHasMemo = TRUE;
               }
               break;

            case 0x03:
               break;

            case 0x83:
               pArea->fHasMemo = TRUE;
               pArea->bMemoType = DB_MEMO_DBT;
               break;

            case 0xE5:
               pArea->fHasMemo = TRUE;
               pArea->bMemoType = DB_MEMO_SMT;
               break;

            case 0xF5:
               pArea->fHasMemo = TRUE;
               pArea->bMemoType = DB_MEMO_FPT;
               break;

            case 0x06:
               pArea->fTableEncrypted = TRUE;
               pArea->bCryptType = DB_CRYPT_SIX;
               break;

            case 0x86:
               pArea->fTableEncrypted = TRUE;
               pArea->fHasMemo = TRUE;
               pArea->bCryptType = DB_CRYPT_SIX;
               pArea->bMemoType = DB_MEMO_DBT;
               break;

            case 0xE6:
               pArea->fHasMemo = TRUE;
               pArea->fTableEncrypted = TRUE;
               pArea->bCryptType = DB_CRYPT_SIX;
               pArea->bMemoType = DB_MEMO_SMT;
               break;

            case 0xF6:
               pArea->fHasMemo = TRUE;
               pArea->fTableEncrypted = TRUE;
               pArea->bCryptType = DB_CRYPT_SIX;
               pArea->bMemoType = DB_MEMO_FPT;
               break;

            default:
               fError = TRUE;
         }
      }
      if( fError )
      {
         if( !pError )
         {
            pError = hb_errNew();
            hb_errPutGenCode( pError, EG_CORRUPTION );
            hb_errPutSubCode( pError, EDBF_CORRUPT );
            hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CORRUPTION ) );
            hb_errPutFileName( pError, pArea->szDataFileName );
            hb_errPutOsCode( pError, hb_fsError() );
            hb_errPutFlags( pError, EF_CANRETRY | EF_CANDEFAULT );
         }
         fRetry = ( SELF_ERROR( ( AREAP ) pArea, pError ) == E_RETRY );
      }
      else
         fRetry = FALSE;
   } while( fRetry );

   if( pError )
      hb_itemRelease( pError );

   /* Read error? */
   if( fError )
      return FAILURE;

   pArea->uiHeaderLen = HB_GET_LE_UINT16( pArea->dbfHeader.uiHeaderLen );
   pArea->ulRecCount  = HB_GET_LE_UINT32( pArea->dbfHeader.ulRecCount );

   return SUCCESS;
}

/*
 * Write the database file header record in the WorkArea.
 */
static ERRCODE hb_dbfWriteDBHeader( DBFAREAP pArea )
{
   int iYear, iMonth, iDay;
   BOOL fLck = FALSE;
   ERRCODE errCode;
   PHB_ITEM pError;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfWriteDBHeader(%p)", pArea));

   if( pArea->fReadonly )
   {
      pError = hb_errNew();
      hb_errPutGenCode( pError, EG_READONLY );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_READONLY ) );
      hb_errPutSubCode( pError, EDBF_READONLY );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
      return FAILURE;
   }

   pArea->dbfHeader.bHasTags = pArea->fHasTags ? 0x01 : 0x00;
   if( pArea->bTableType == DB_DBF_VFP )
   {
      pArea->dbfHeader.bVersion = ( pArea->fAutoInc ? 0x31 : 0x30 );
      if( pArea->fHasMemo && pArea->bMemoType == DB_MEMO_FPT )
         pArea->dbfHeader.bHasTags |= 0x02;
   }
   else
   {
      pArea->dbfHeader.bVersion = 0x03;
      if( pArea->fHasMemo )
      {
         if( pArea->bMemoType == DB_MEMO_DBT )
            pArea->dbfHeader.bVersion = 0x83;
         else if( pArea->bMemoType == DB_MEMO_FPT )
            pArea->dbfHeader.bVersion = 0xF5;
         else if( pArea->bMemoType == DB_MEMO_SMT )
            pArea->dbfHeader.bVersion = 0xE5;
      }
      if( pArea->fTableEncrypted && pArea->bCryptType == DB_CRYPT_SIX )
         pArea->dbfHeader.bVersion = ( pArea->dbfHeader.bVersion & 0xf0 ) | 0x06;
   }

   hb_dateToday( &iYear, &iMonth, &iDay );
   pArea->dbfHeader.bYear = ( BYTE ) ( iYear - 1900 );
   pArea->dbfHeader.bMonth = ( BYTE ) iMonth;
   pArea->dbfHeader.bDay = ( BYTE ) iDay;

   /* Update record count */
   if( pArea->fShared )
   {
      if( !pArea->fHeaderLocked )
      {
         if( SELF_RAWLOCK( ( AREAP ) pArea, HEADER_LOCK, 0 ) != SUCCESS )
            return FAILURE;
         fLck = TRUE;
      }
      pArea->ulRecCount = hb_dbfCalcRecCount( pArea );
   }
   else
   {
      /* Exclusive mode */
      /* Seek to logical eof and write eof mark */
      hb_fsSeekLarge( pArea->hDataFile, ( HB_FOFFSET ) pArea->uiHeaderLen +
                      ( HB_FOFFSET ) pArea->uiRecordLen *
                      ( HB_FOFFSET ) pArea->ulRecCount, FS_SET );
      hb_fsWrite( pArea->hDataFile, ( BYTE * ) "\032", 1 );
      hb_fsWrite( pArea->hDataFile, NULL, 0 );
   }

   HB_PUT_LE_UINT32( pArea->dbfHeader.ulRecCount,  pArea->ulRecCount );
   HB_PUT_LE_UINT16( pArea->dbfHeader.uiHeaderLen, pArea->uiHeaderLen );
   HB_PUT_LE_UINT16( pArea->dbfHeader.uiRecordLen, pArea->uiRecordLen );
   hb_fsSeek( pArea->hDataFile, 0, FS_SET );
   if( hb_fsWrite( pArea->hDataFile, ( BYTE * ) &pArea->dbfHeader,
                   sizeof( DBFHEADER ) ) == sizeof( DBFHEADER ) )
   {
      errCode = SUCCESS;
   }
   else
   {
      errCode = FAILURE;
   }
   /* TODO: add RT error */
   pArea->fDataFlush = TRUE;
   pArea->fUpdateHeader = FALSE;
   if( fLck )
   {
      if( SELF_RAWLOCK( ( AREAP ) pArea, HEADER_UNLOCK, 0 ) != SUCCESS )
         return FAILURE;
   }

   if( errCode != SUCCESS )
   {
      pError = hb_errNew();

      hb_errPutGenCode( pError, EG_WRITE );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_WRITE ) );
      hb_errPutSubCode( pError, EDBF_WRITE );
      hb_errPutFileName( pError, pArea->szDataFileName );
      hb_errPutOsCode( pError, hb_fsError() );
      SELF_ERROR( ( AREAP ) pArea, pError );
      hb_itemRelease( pError );
   }
   return errCode;
}

static ERRCODE hb_dbfDrop( LPRDDNODE pRDD, PHB_ITEM pItemTable, PHB_ITEM pItemIndex )
{
   char szFileName[ _POSIX_PATH_MAX + 1 ], * szFile, * szExt;
   PHB_ITEM pFileExt = NULL;
   PHB_FNAME pFileName;
   BOOL fTable = FALSE, fResult = FALSE;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfDrop(%p,%p,%p)", pRDD, pItemTable, pItemIndex));

   szFile = hb_itemGetCPtr( pItemIndex );
   if( !szFile[ 0 ] )
   {
      /* Try to delete index file */
      szFile = hb_itemGetCPtr( pItemTable );
      if( !szFile[ 0 ] )
         return FALSE;
      fTable = TRUE;
   }

   pFileName = hb_fsFNameSplit( szFile );

   if( hb_set.HB_SET_DEFEXTENSIONS && !pFileName->szExtension )
   {
      /* Add default extension if missing */
      pFileExt = hb_itemPutC( NULL, "" );
      if( SELF_RDDINFO( pRDD, fTable ? RDDI_TABLEEXT : RDDI_ORDBAGEXT, 0, pFileExt ) == SUCCESS )
         pFileName->szExtension = hb_itemGetCPtr( pFileExt );
   }
   hb_fsFNameMerge( szFileName, pFileName );
   hb_xfree( pFileName );

   /* Use hb_spFile first to locate table which can be in differ path */
   if( hb_spFile( ( BYTE * ) szFileName, ( BYTE * ) szFileName ) )
   {
      fResult = hb_fsDelete( ( BYTE * ) szFileName );
      if( fResult && fTable )
      {
         /*
          * Database table file has been deleted, now check if memo is
          * supported and if yes then try to delete memo file if it exists
          * in the same directory as table file
          * hb_fsFNameSplit() repeated intentionally to respect
          * the path set by hb_spFile()
          */
         pFileName = hb_fsFNameSplit( szFileName );
         pFileExt = hb_itemPutC( pFileExt, "" );
         if( SELF_RDDINFO( pRDD, RDDI_MEMOEXT, 0, pFileExt ) == SUCCESS )
         {
            szExt = hb_itemGetCPtr( pFileExt );
            if( szExt[ 0 ] )
            {
               pFileName->szExtension = szExt;
               hb_fsFNameMerge( szFileName, pFileName );
               hb_fsDelete( ( BYTE * ) szFileName );
            }
         }
         /*
          * and try to delete production index also if it exists
          * in the same directory as table file
          */
         pFileExt = hb_itemPutC( pFileExt, "" );
         if( SELF_RDDINFO( pRDD, RDDI_ORDSTRUCTEXT, 0, pFileExt ) == SUCCESS )
         {
            szExt = hb_itemGetCPtr( pFileExt );
            if( szExt[ 0 ] )
            {
               pFileName->szExtension = szExt;
               hb_fsFNameMerge( szFileName, pFileName );
               hb_fsDelete( ( BYTE * ) szFileName );
            }
         }
         hb_xfree( pFileName );
      }
   }
   if( pFileExt )
   {
      hb_itemRelease( pFileExt );
   }

   return fResult ? SUCCESS : FAILURE;
}

static ERRCODE hb_dbfExists( LPRDDNODE pRDD, PHB_ITEM pItemTable, PHB_ITEM pItemIndex )
{
   char szFileName[ _POSIX_PATH_MAX + 1 ], * szFile;
   PHB_ITEM pFileExt = NULL;
   PHB_FNAME pFileName;
   BOOL fTable = FALSE;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfExists(%p,%p,%p)", pRDD, pItemTable, pItemIndex));

   szFile = hb_itemGetCPtr( pItemIndex );
   if( !szFile[ 0 ] )
   {
      szFile = hb_itemGetCPtr( pItemTable );
      if( !szFile[ 0 ] )
         return FALSE;
      fTable = TRUE;
   }

   pFileName = hb_fsFNameSplit( szFile );

   if( hb_set.HB_SET_DEFEXTENSIONS && !pFileName->szExtension )
   {
      pFileExt = hb_itemPutC( NULL, "" );
      if( SELF_RDDINFO( pRDD, fTable ? RDDI_TABLEEXT : RDDI_ORDBAGEXT, 0, pFileExt ) == SUCCESS )
         pFileName->szExtension = hb_itemGetCPtr( pFileExt );
   }
   hb_fsFNameMerge( szFileName, pFileName );
   hb_xfree( pFileName );
   if( pFileExt )
   {
      hb_itemRelease( pFileExt );
   }

   return hb_spFile( ( BYTE * ) szFileName, NULL ) ? SUCCESS : FAILURE;
}

static ERRCODE hb_dbfInit( LPRDDNODE pRDD )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfInit(%p)", pRDD));

   pRDD->lpvCargo = hb_xgrab( sizeof( DBFDATA ) );
   memset( pRDD->lpvCargo, 0, sizeof( DBFDATA ) );

   ( ( LPDBFDATA ) pRDD->lpvCargo )->bTableType = DB_DBF_STD;
   ( ( LPDBFDATA ) pRDD->lpvCargo )->bCryptType = DB_CRYPT_NONE;
   ( ( LPDBFDATA ) pRDD->lpvCargo )->uiDirtyRead = HB_IDXREAD_CLEANMASK;

   return SUCCESS;
}

static ERRCODE hb_dbfExit( LPRDDNODE pRDD )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dbfExit(%p)", pRDD));

   if( pRDD->lpvCargo )
   {
      LPDBFDATA pData = ( LPDBFDATA ) pRDD->lpvCargo;

      if( pData->szTrigger )
         hb_xfree( pData->szTrigger );
      if( pData->szPendingTrigger )
         hb_xfree( pData->szPendingTrigger );
      if( pData->szPasswd )
         hb_xfree( pData->szPasswd );
      if( pData->szPendingPasswd )
         hb_xfree( pData->szPendingPasswd );

      hb_xfree( pRDD->lpvCargo );
      pRDD->lpvCargo = NULL;
   }
   s_uiRddId = ( USHORT ) -1;

   return SUCCESS;
}

static ERRCODE hb_dbfRddInfo( LPRDDNODE pRDD, USHORT uiIndex, ULONG ulConnect, PHB_ITEM pItem )
{
   LPDBFDATA pData;

   HB_TRACE(HB_TR_DEBUG, ("hb_dbfRddInfo(%p, %hu, %lu, %p)", pRDD, uiIndex, ulConnect, pItem));

   pData = ( LPDBFDATA ) pRDD->lpvCargo;

   switch( uiIndex )
   {
      case RDDI_ISDBF:
      case RDDI_CANPUTREC:
      case RDDI_LOCAL:
         hb_itemPutL( pItem, TRUE );
         break;

      case RDDI_TABLEEXT:
      {
         char * szNew = hb_itemGetCPtr( pItem );

         if( szNew[0] == '.' && szNew[1] )
            szNew = hb_strdup( szNew );
         else
            szNew = NULL;

         hb_itemPutC( pItem, pData->szTableExt[ 0 ] ? pData->szTableExt : DBF_TABLEEXT );
         if( szNew )
         {
            hb_strncpy( pData->szTableExt, szNew, HB_MAX_FILE_EXT );
            hb_xfree( szNew );
         }
         break;
      }
      case RDDI_TABLETYPE:
      {
         int iType = hb_itemGetNI( pItem );
         hb_itemPutNI( pItem, pData->bTableType ? pData->bTableType : DB_DBF_STD );
         switch( iType )
         {
            case DB_DBF_STD:        /* standard dBase/Clipper DBF file */
            case DB_DBF_VFP:        /* VFP DBF file */
               pData->bTableType = iType;
         }
         break;
      }
      case RDDI_LOCKSCHEME:
      {
         int iScheme = hb_itemGetNI( pItem );

         hb_itemPutNI( pItem, pData->bLockType ? pData->bLockType :
                              hb_set.HB_SET_DBFLOCKSCHEME );
         switch( iScheme )
         {
            case DB_DBFLOCK_CLIP:
            case DB_DBFLOCK_CL53:
            case DB_DBFLOCK_CL53EXT:
            case DB_DBFLOCK_VFP:
#ifndef HB_LONG_LONG_OFF
            case DB_DBFLOCK_XHB64:
#endif
               pData->bLockType = ( int ) iScheme;
         }
         break;
      }
      case RDDI_DIRTYREAD:
      {
         BOOL fDirty = ( pData->uiDirtyRead == HB_IDXREAD_DIRTYMASK );
         if( HB_IS_LOGICAL( pItem ) )
         {
            pData->uiDirtyRead = hb_itemGetL( pItem ) ?
                                 HB_IDXREAD_DIRTYMASK : HB_IDXREAD_CLEANMASK;
         }
         hb_itemPutL( pItem, fDirty );
         break;
      }
      case RDDI_TRIGGER:
      {
         char * szTrigger = pData->szTrigger;
         BOOL fFree = FALSE;

         if( HB_IS_STRING( pItem ) )
         {
            fFree = TRUE;
            pData->szTrigger = hb_itemGetCLen( pItem ) > 0 ?
                               hb_itemGetC( pItem ) : NULL;
         }

         if( fFree && szTrigger )
            hb_itemPutCPtr( pItem, szTrigger, strlen( szTrigger ) );
         else
            hb_itemPutC( pItem, szTrigger );

         if( !szTrigger && !fFree )
            return FAILURE;

         break;
      }
      case RDDI_PENDINGTRIGGER:
         if( HB_IS_STRING( pItem ) )
         {
            if( pData->szPendingTrigger )
            {
               hb_xfree( pData->szPendingTrigger );
               pData->szPendingTrigger = NULL;
            }
            if( hb_itemGetCLen( pItem ) > 0 )
               pData->szPendingTrigger = hb_itemGetC( pItem );
         }
         else if( pData->szPendingTrigger )
         {
            hb_itemPutCPtr( pItem, pData->szPendingTrigger,
                            strlen( pData->szPendingTrigger ) );
            pData->szPendingTrigger = NULL;
         }
         else
            return FAILURE;
         break;

      case RDDI_PASSWORD:
      {
         char * szPasswd = pData->szPasswd;
         BOOL fFree = FALSE;

         if( HB_IS_STRING( pItem ) )
         {
            fFree = TRUE;
            pData->szPasswd = hb_itemGetCLen( pItem ) > 0 ?
                              hb_itemGetC( pItem ) : NULL;
         }

         if( fFree && szPasswd )
            hb_itemPutCPtr( pItem, szPasswd, strlen( szPasswd ) );
         else
            hb_itemPutC( pItem, szPasswd );

         if( !szPasswd && !fFree )
            return FAILURE;

         break;
      }
      case RDDI_PENDINGPASSWORD:
         if( HB_IS_STRING( pItem ) )
         {
            if( pData->szPendingPasswd )
            {
               hb_xfree( pData->szPendingPasswd );
               pData->szPendingPasswd = NULL;
            }
            if( hb_itemGetCLen( pItem ) > 0 )
               pData->szPendingPasswd = hb_itemGetC( pItem );
         }
         else if( pData->szPendingPasswd )
         {
            hb_itemPutCPtr( pItem, pData->szPendingPasswd,
                            strlen( pData->szPendingPasswd ) );
            pData->szPendingPasswd = NULL;
         }
         else
            return FAILURE;
         break;

      default:
         return SUPER_RDDINFO( pRDD, uiIndex, ulConnect, pItem );

   }

   return SUCCESS;
}


HB_FUNC( _DBF ) { ; }

HB_FUNC( DBF_GETFUNCTABLE )
{
   RDDFUNCS * pTable;
   USHORT * uiCount, uiRddId;

   uiCount = ( USHORT * ) hb_parptr( 1 );
   pTable = ( RDDFUNCS * ) hb_parptr( 2 );
   uiRddId = hb_parni( 4 );

   HB_TRACE(HB_TR_DEBUG, ("DBF_GETFUNCTABLE(%p, %p)", uiCount, pTable));

   if( pTable )
   {
      ERRCODE errCode;

      if( uiCount )
         * uiCount = RDDFUNCSCOUNT;
      errCode = hb_rddInherit( pTable, &dbfTable, &dbfSuper, NULL );
      hb_retni( errCode );
      if( errCode == SUCCESS )
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


#define __PRG_SOURCE__ __FILE__

#ifdef HB_PCODE_VER
   #undef HB_PRG_PCODE_VER
   #define HB_PRG_PCODE_VER HB_PCODE_VER
#endif

static void hb_dbfRddInit( void * cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   if( hb_rddRegister( "DBF", RDT_FULL ) > 1 )
   {
      hb_errInternal( HB_EI_RDDINVALID, NULL, NULL, NULL );
   }
}

HB_INIT_SYMBOLS_BEGIN( dbf1__InitSymbols )
{ "_DBF",             {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( _DBF )}, NULL },
{ "DBF_GETFUNCTABLE", {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( DBF_GETFUNCTABLE )}, NULL }
HB_INIT_SYMBOLS_END( dbf1__InitSymbols )

HB_CALL_ON_STARTUP_BEGIN( _hb_dbf_rdd_init_ )
   hb_vmAtInit( hb_dbfRddInit, NULL );
HB_CALL_ON_STARTUP_END( _hb_dbf_rdd_init_ )

#if defined(HB_PRAGMA_STARTUP)
   #pragma startup dbf1__InitSymbols
   #pragma startup _hb_dbf_rdd_init_
#elif defined(HB_MSC_STARTUP)
   #if _MSC_VER >= 1010
      #pragma data_seg( ".CRT$XIY" )
      #pragma comment( linker, "/Merge:.CRT=.data" )
   #else
      #pragma data_seg( "XIY" )
   #endif
   static HB_$INITSYM hb_vm_auto_dbf1__InitSymbols = dbf1__InitSymbols;
   static HB_$INITSYM hb_vm_auto_dbf_rdd_init = _hb_dbf_rdd_init_;
   #pragma data_seg()
#endif
