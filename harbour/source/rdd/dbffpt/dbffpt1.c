/*
 * $Id$
 */

/*
 * xHarbour Project source code:
 * DBFFPT RDD
 *
 * Copyright 2003 Przemyslaw Czerpak <druzus@acn.waw.pl>
 * www - http://www.xharbour.org
 *
 * The SIX memo conversion algorithms and some piece of code taken from
 * DBFCDX and DBFFPT
 *    Copyright 1999-2002 Bruno Cantero <bruno@issnet.net>
 *    Copyright 2000-2003 Horacio Roldan <harbour_ar@yahoo.com.ar> (portions)
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

#if defined( HB_FPT_NO_READLOCK )
#  undef HB_MEMO_SAFELOCK
#else
/*#  define HB_MEMO_SAFELOCK */
#endif

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbinit.h"
#include "hbapierr.h"
#include "hbapilng.h"
#include "hbset.h"
#include "hbvm.h"
#include "hbdate.h"
#include "hbrddfpt.h"
#include "hbsxfunc.h"
#include "rddsys.ch"

#ifndef HB_CDP_SUPPORT_OFF
#  include "hbapicdp.h"
#endif

static USHORT s_uiRddIdBLOB = ( USHORT ) -1;
static USHORT s_uiRddIdFPT  = ( USHORT ) -1;

static RDDFUNCS fptSuper;
static const RDDFUNCS fptTable =
{

   /* Movement and positioning methods */

   ( DBENTRYP_BP )    hb_fptBof,
   ( DBENTRYP_BP )    hb_fptEof,
   ( DBENTRYP_BP )    hb_fptFound,
   ( DBENTRYP_V )     hb_fptGoBottom,
   ( DBENTRYP_UL )    hb_fptGoTo,
   ( DBENTRYP_I )     hb_fptGoToId,
   ( DBENTRYP_V )     hb_fptGoTop,
   ( DBENTRYP_BIB )   hb_fptSeek,
   ( DBENTRYP_L )     hb_fptSkip,
   ( DBENTRYP_L )     hb_fptSkipFilter,
   ( DBENTRYP_L )     hb_fptSkipRaw,


   /* Data management */

   ( DBENTRYP_VF )    hb_fptAddField,
   ( DBENTRYP_B )     hb_fptAppend,
   ( DBENTRYP_I )     hb_fptCreateFields,
   ( DBENTRYP_V )     hb_fptDeleteRec,
   ( DBENTRYP_BP )    hb_fptDeleted,
   ( DBENTRYP_SP )    hb_fptFieldCount,
   ( DBENTRYP_VF )    hb_fptFieldDisplay,
   ( DBENTRYP_SSI )   hb_fptFieldInfo,
   ( DBENTRYP_SVP )   hb_fptFieldName,
   ( DBENTRYP_V )     hb_fptFlush,
   ( DBENTRYP_PP )    hb_fptGetRec,
   ( DBENTRYP_SI )    hb_fptGetValue,
   ( DBENTRYP_SVL )   hb_fptGetVarLen,
   ( DBENTRYP_V )     hb_fptGoCold,
   ( DBENTRYP_V )     hb_fptGoHot,
   ( DBENTRYP_P )     hb_fptPutRec,
   ( DBENTRYP_SI )    hb_fptPutValue,
   ( DBENTRYP_V )     hb_fptRecall,
   ( DBENTRYP_ULP )   hb_fptRecCount,
   ( DBENTRYP_ISI )   hb_fptRecInfo,
   ( DBENTRYP_ULP )   hb_fptRecNo,
   ( DBENTRYP_I )     hb_fptRecId,
   ( DBENTRYP_S )     hb_fptSetFieldExtent,


   /* WorkArea/Database management */

   ( DBENTRYP_P )     hb_fptAlias,
   ( DBENTRYP_V )     hb_fptClose,
   ( DBENTRYP_VP )    hb_fptCreate,
   ( DBENTRYP_SI )    hb_fptInfo,
   ( DBENTRYP_V )     hb_fptNewArea,
   ( DBENTRYP_VP )    hb_fptOpen,
   ( DBENTRYP_V )     hb_fptRelease,
   ( DBENTRYP_SP )    hb_fptStructSize,
   ( DBENTRYP_P )     hb_fptSysName,
   ( DBENTRYP_VEI )   hb_fptEval,
   ( DBENTRYP_V )     hb_fptPack,
   ( DBENTRYP_LSP )   hb_fptPackRec,
   ( DBENTRYP_VS )    hb_fptSort,
   ( DBENTRYP_VT )    hb_fptTrans,
   ( DBENTRYP_VT )    hb_fptTransRec,
   ( DBENTRYP_V )     hb_fptZap,


   /* Relational Methods */

   ( DBENTRYP_VR )    hb_fptChildEnd,
   ( DBENTRYP_VR )    hb_fptChildStart,
   ( DBENTRYP_VR )    hb_fptChildSync,
   ( DBENTRYP_V )     hb_fptSyncChildren,
   ( DBENTRYP_V )     hb_fptClearRel,
   ( DBENTRYP_V )     hb_fptForceRel,
   ( DBENTRYP_SVP )   hb_fptRelArea,
   ( DBENTRYP_VR )    hb_fptRelEval,
   ( DBENTRYP_SVP )   hb_fptRelText,
   ( DBENTRYP_VR )    hb_fptSetRel,


   /* Order Management */

   ( DBENTRYP_OI )    hb_fptOrderListAdd,
   ( DBENTRYP_V )     hb_fptOrderListClear,
   ( DBENTRYP_OI )    hb_fptOrderListDelete,
   ( DBENTRYP_OI )    hb_fptOrderListFocus,
   ( DBENTRYP_V )     hb_fptOrderListRebuild,
   ( DBENTRYP_VOI )   hb_fptOrderCondition,
   ( DBENTRYP_VOC )   hb_fptOrderCreate,
   ( DBENTRYP_OI )    hb_fptOrderDestroy,
   ( DBENTRYP_OII )   hb_fptOrderInfo,


   /* Filters and Scope Settings */

   ( DBENTRYP_V )     hb_fptClearFilter,
   ( DBENTRYP_V )     hb_fptClearLocate,
   ( DBENTRYP_V )     hb_fptClearScope,
   ( DBENTRYP_VPLP )  hb_fptCountScope,
   ( DBENTRYP_I )     hb_fptFilterText,
   ( DBENTRYP_SI )    hb_fptScopeInfo,
   ( DBENTRYP_VFI )   hb_fptSetFilter,
   ( DBENTRYP_VLO )   hb_fptSetLocate,
   ( DBENTRYP_VOS )   hb_fptSetScope,
   ( DBENTRYP_VPL )   hb_fptSkipScope,
   ( DBENTRYP_B )     hb_fptLocate,


   /* Miscellaneous */

   ( DBENTRYP_P )     hb_fptCompile,
   ( DBENTRYP_I )     hb_fptError,
   ( DBENTRYP_I )     hb_fptEvalBlock,


   /* Network operations */

   ( DBENTRYP_VSP )   hb_fptRawLock,
   ( DBENTRYP_VL )    hb_fptLock,
   ( DBENTRYP_I )     hb_fptUnLock,


   /* Memofile functions */

   ( DBENTRYP_V )     hb_fptCloseMemFile,
   ( DBENTRYP_VP )    hb_fptCreateMemFile,
   ( DBENTRYP_SVPB )  hb_fptGetValueFile,
   ( DBENTRYP_VP )    hb_fptOpenMemFile,
   ( DBENTRYP_SVPB )  hb_fptPutValueFile,


   /* Database file header handling */

   ( DBENTRYP_V )     hb_fptReadDBHeader,
   ( DBENTRYP_V )     hb_fptWriteDBHeader,


   /* non WorkArea functions       */
   ( DBENTRYP_R )     hb_fptInit,
   ( DBENTRYP_R )     hb_fptExit,
   ( DBENTRYP_RVV )   hb_fptDrop,
   ( DBENTRYP_RVV )   hb_fptExists,
   ( DBENTRYP_RSLV )  hb_fptRddInfo,

   /* Special and reserved methods */

   ( DBENTRYP_SVP )   hb_fptWhoCares
};

/*
 * generate Run-Time error
 */
static ERRCODE hb_memoErrorRT( FPTAREAP pArea, USHORT uiGenCode, USHORT uiSubCode, char * szFileName, USHORT uiOsCode, USHORT uiFlags )
{
   ERRCODE errCode = FAILURE;

   if( hb_vmRequestQuery() == 0 )
   {
      PHB_ITEM pError = hb_errNew();

      if( uiGenCode == 0 )
         uiGenCode = hb_dbfGetEGcode( uiSubCode );
      if( uiOsCode == 0 && uiSubCode != EDBF_DATATYPE && uiSubCode != EDBF_DATAWIDTH )
         uiOsCode = hb_fsError();

      hb_errPutGenCode( pError, uiGenCode );
      hb_errPutSubCode( pError, uiSubCode );
      if( uiOsCode )
         hb_errPutOsCode( pError, uiOsCode );
      hb_errPutDescription( pError, hb_langDGetErrorDesc( uiGenCode ) );
      if( szFileName )
         hb_errPutFileName( pError, szFileName );
      if( uiFlags )
         hb_errPutFlags( pError, uiFlags );
      errCode = SELF_ERROR( ( AREAP ) pArea, pError );
      hb_errRelease( pError );
   }
   return errCode;
}


static char * hb_memoDefaultFileExt( int iType, USHORT uiRdd )
{
   if( uiRdd == s_uiRddIdBLOB )
      return DBV_MEMOEXT;

   switch( iType )
   {
      case DB_MEMO_DBT:
         return DBT_MEMOEXT;
      case DB_MEMO_FPT:
         return FPT_MEMOEXT;
      case DB_MEMO_SMT:
         return SMT_MEMOEXT;
   }
   return NULL;
}

/*
 * Exclusive lock memo file.
 */
static BOOL hb_fptFileLockEx( FPTAREAP pArea, BOOL fWait )
{
   BOOL fRet;

   if ( !pArea->fShared )
   {
      fRet = TRUE;
   }
   else
   {
      do
      {
         fRet = hb_fsLock( pArea->hMemoFile, FPT_LOCKPOS, FPT_LOCKSIZE,
                           FL_LOCK | FLX_EXCLUSIVE | ( fWait ? FLX_WAIT : 0 ) );
      } while ( !fRet && fWait );
   }
   return fRet;
}

/*
 * Shared lock memo file.
 */


static BOOL hb_fptFileLockSh( FPTAREAP pArea, BOOL fWait )
{
   BOOL fRet;

   if ( !pArea->fShared )
   {
      fRet = TRUE;
   }
   else
   {
      do
      {
         fRet = hb_fsLock( pArea->hMemoFile, FPT_LOCKPOS, FPT_LOCKSIZE,
                           FL_LOCK | FLX_SHARED | ( fWait ? FLX_WAIT : 0 ) );
      } while ( !fRet && fWait );
   }
   return fRet;
}

/*
 * Unlock memo file.
 */
static BOOL hb_fptFileUnLock( FPTAREAP pArea )
{
   return !pArea->fShared || hb_fsLock( pArea->hMemoFile, FPT_LOCKPOS, FPT_LOCKSIZE, FL_UNLOCK );
}

/*
 * Check if MEMO file has direct access to data
 */
static BOOL hb_fptHasDirectAccess( FPTAREAP pArea )
{
   return pArea->bMemoType == DB_MEMO_FPT &&
          ( pArea->uiMemoVersion == DB_MEMOVER_FLEX ||
            pArea->uiMemoVersion == DB_MEMOVER_CLIP );
}

/*
 * Lock root block pointer.
 */
static BOOL hb_fptRootBlockLock( FPTAREAP pArea )
{
   BOOL fRet;

   if ( !pArea->fShared )
      fRet = TRUE;
   else
      fRet = hb_fsLock( pArea->hMemoFile, FPT_ROOTBLOCK_OFFSET, 4,
                        FL_LOCK | FLX_EXCLUSIVE );
   return fRet;
}

/*
 * Unlock root block pointer.
 */
static BOOL hb_fptRootBlockUnLock( FPTAREAP pArea )
{
   return !pArea->fShared ||
          hb_fsLock( pArea->hMemoFile, FPT_ROOTBLOCK_OFFSET, 4, FL_UNLOCK );
}

/*
 * Read root block pointer.
 */
static ERRCODE hb_fptGetRootBlock( FPTAREAP pArea, ULONG * pulBlock )
{
   *pulBlock = 0;

   if( hb_fptHasDirectAccess( pArea ) )
   {
      BYTE buffer[ 4 ];

      hb_fsSeek( pArea->hMemoFile, FPT_ROOTBLOCK_OFFSET, FS_SET );
      if( hb_fsRead( pArea->hMemoFile, buffer, 4 ) == 4 )
      {
         *pulBlock = HB_GET_LE_UINT32( buffer );
         return SUCCESS;
      }
      else
         return EDBF_READ;
   }
   return EDBF_UNSUPPORTED;
}

/*
 * Write root block pointer.
 */
static ERRCODE hb_fptPutRootBlock( FPTAREAP pArea, ULONG ulBlock )
{
   if( hb_fptHasDirectAccess( pArea ) )
   {
      BYTE buffer[ 4 ];

      HB_PUT_LE_UINT32( buffer, ulBlock );
      hb_fsSeek( pArea->hMemoFile, FPT_ROOTBLOCK_OFFSET, FS_SET );
      if( hb_fsWrite( pArea->hMemoFile, buffer, 4 ) == 4 )
         return SUCCESS;
      else
         return EDBF_WRITE;
   }
   return EDBF_UNSUPPORTED;
}

/*
   GARBAGE COLLECTOR:
   I don't have any documentation about it. All I know is reverse engineering
   or analyzes of other sources. If any one can tell me sth more about it then
   I will be really glad. I use method one for SixMemo and method 2 for FLEX
   memos.

  Method 1.
   FPTHEADER->reserved2[492]     is a list of free pages,
                                 6 bytes for each page
                                    size[2]  (size in blocks) (little endian)
                                    block[4] (block number) (little endian)
                                 signature1[12] has to be cutted down to
                                 10 bytes. The last 2 bytes becomes the
                                 number of entries in free block list (max 82)

 Method 2.
   FPTHEADER->flexDir[4]         is a little endian offset to page
                                 (1024 bytes size) where header is:
                                    type[4] = 1000 (big endian)
                                    size[4] = 1010 (big endian)
                                 then
                                    nItem[2] number of item (little endian)
                                 then 1008 bytes with free blocks list
                                 (max 126 entries) in format:
                                    offset[4]   (little endian)
                                    size[4]     (little endian)
                                 nItem is always odd and after read we have
                                 to recalculate it:
                                    nItem = ( nItem - 3 ) / 4
            if FPTHEADER->flexDir = 0 then we can create it by allocating
            two 1024 bytes pages for flexRev and flexDir page.
               FPTHEADER->flexRev[4] 1024 bytes in next free block
               FPTHEADER->flexDir[4] next 1024 bytes
            flexRev page is copy of flexDir page but the items are stored
            in reversed form size[4] first then offset[4]
               size[4]     (little endian)
               offset[4]   (little endian)
            before writing GC pages (dir and rev, both has to be synced)
            we should first sort the entries moving the shortest blocks
            to the beginning so when we where looking for free block we
            can scan the list from the beginning finding the first one
            large enough. unused bytes in GC page should be filled with 0xAD
            when we free fpt block we should set in its header:
               type[4] = 1001 (big endian)
               size[4] = rest of block size (block size - 8) (big endian)

   TODO: Clipper 5.3 can use more then one GC page. I don't have any
   documentation for that and don't have time for farther hacking
   binary files to find the algorithm. If you have any documentation
   about it, please send it to me.
   OK. I've found a while for analyzing the FPT file created by Clipper
   and I think I know this structure. It's a tree. The node type
   is marked in the first two bytes of GC page encoded as bit field with
   the number of items 2 - means branch node, 3-leaf node. The value in
   GC node is calculated as:
      ( nItem << 2 ) | FPTGCNODE_TYPE
   Each item in branch node has 12 bytes and inside them 3 32bit little
   endian values in pages sorted by offset the are:
      offset,size,subpage
   and in pages sorted by size:
      size,offset,subpage
   size and offset is the biggest (the last one) value in subpage(s)
   and subpage is offset of subpage int the file.
   All values in GC pages are in bytes not blocks - it creates the
   FPT file size limit 2^32 - if they will be in blocks then the
   the FPT file size will be limited by 2^32*block_size
   It's time to implement it ;-)
 */

/*
 * Sort GC free memo block list by size.
 */
static void hb_fptSortGCitems( LPMEMOGCTABLE pGCtable )
{
   ULONG ulOffset, ulSize;
   BOOL fMoved = TRUE;
   int i, j, l;

   /* this table should be allready quite good sorted so this simple
      algorithms will be the most efficient one.
      It will need only one or two passes */
   l = pGCtable->usItems - 1;
   while ( fMoved )
   {
      fMoved = FALSE;
      j = l;
      for( i = 0; i < j; i++ )
      {
         if ( pGCtable->pGCitems[i].ulSize > pGCtable->pGCitems[i+1].ulSize )
         {
            ulOffset = pGCtable->pGCitems[i+1].ulOffset;
            ulSize = pGCtable->pGCitems[i+1].ulSize;
            pGCtable->pGCitems[i+1].ulSize   = pGCtable->pGCitems[i].ulSize;
            pGCtable->pGCitems[i+1].ulOffset = pGCtable->pGCitems[i].ulOffset;
            pGCtable->pGCitems[ i ].ulSize   = ulSize;
            pGCtable->pGCitems[ i ].ulOffset = ulOffset;
            fMoved = TRUE;
            pGCtable->bChanged |= 2;
            l = i;
         }
      }
   }
}

/*
 * Pack GC free memo block list - try to join free blocks.
 */
static void hb_fptPackGCitems( LPMEMOGCTABLE pGCtable )
{
   ULONG ulEnd;
   int i, j;

   /* TODO: better alogrithm this primitve one can be too slow for big
      free block list table */
   for( i = 0; i < pGCtable->usItems; i++ )
   {
      if ( pGCtable->pGCitems[i].ulOffset != 0 &&
           pGCtable->pGCitems[i].ulSize != 0 )
      {
         ulEnd = pGCtable->pGCitems[i].ulOffset + pGCtable->pGCitems[i].ulSize;
         if ( ulEnd == pGCtable->ulNextBlock )
         {
            pGCtable->ulNextBlock -= pGCtable->pGCitems[i].ulSize;
            pGCtable->pGCitems[i].ulOffset = pGCtable->pGCitems[i].ulSize = 0;
            pGCtable->bChanged |= 2;
            i = -1;
         }
         else
         {
            for( j = i + 1; j < pGCtable->usItems; j++ )
            {
               if ( ulEnd == pGCtable->pGCitems[j].ulOffset )
               {
                  pGCtable->pGCitems[i].ulSize += pGCtable->pGCitems[j].ulSize;
                  pGCtable->pGCitems[j].ulOffset = pGCtable->pGCitems[j].ulSize = 0;
                  pGCtable->bChanged |= 2;
                  i = -1;
                  break;
               }
            }
         }
      }
   }

   /* remove empty items */
   for( i = j = 0; i < pGCtable->usItems; i++ )
   {
      if ( pGCtable->pGCitems[i].ulOffset != 0 &&
           pGCtable->pGCitems[i].ulSize != 0 )
      {
         if ( i > j )
         {
            pGCtable->pGCitems[j].ulOffset = pGCtable->pGCitems[i].ulOffset;
            pGCtable->pGCitems[j].ulSize   = pGCtable->pGCitems[i].ulSize;
         }
         j++;
      }
   }
   pGCtable->usItems = j;
}

/*
 * Write proper header into modified GC free memo blocks.
 */
static ERRCODE hb_fptWriteGCitems( FPTAREAP pArea, LPMEMOGCTABLE pGCtable, USHORT usItem )
{
   FPTBLOCK fptBlock;
   ERRCODE errCode = SUCCESS;
   int i /* ,iStart, iStop */ ;

   HB_SYMBOL_UNUSED( usItem ) ;

/*
   if ( usItem == 0 )
   {
      iStart = 0;
      iStop = pGCtable->usItems;
   }
   else
   {
      iStart = usItem;
      iStop = usItem + 1;
   }
*/

   for( i = 0; i < pGCtable->usItems; i++ )
   {
      if ( pGCtable->pGCitems[i].fChanged )
      {
         if( pArea->uiMemoVersion == DB_MEMOVER_FLEX ||
             pArea->uiMemoVersion == DB_MEMOVER_CLIP )
         {
            HB_PUT_BE_UINT32( fptBlock.type, FPTIT_FLEX_UNUSED );
            HB_PUT_BE_UINT32( fptBlock.size, pArea->uiMemoBlockSize *
                              pGCtable->pGCitems[i].ulSize - sizeof( FPTBLOCK ) );
            hb_fsSeekLarge( pArea->hMemoFile,
                            ( HB_FOFFSET ) pGCtable->pGCitems[i].ulOffset *
                            ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );
            if ( hb_fsWrite( pArea->hMemoFile, ( BYTE * ) &fptBlock,
                             sizeof( FPTBLOCK ) ) != sizeof( FPTBLOCK ) )
            {
               errCode = EDBF_WRITE;
            }
            pArea->fMemoFlush = TRUE;
         }
         pGCtable->pGCitems[i].fChanged = FALSE;
      }
   }
   return errCode;
}

/*
 * Add new block to GC free memo blocks list.
 */
static ERRCODE hb_fptGCfreeBlock( FPTAREAP pArea, LPMEMOGCTABLE pGCtable,
                                  ULONG ulOffset, ULONG ulByteSize, BOOL fRaw )
{
   ERRCODE errCode = SUCCESS;
   ULONG ulSize;

   if( pArea->bMemoType == DB_MEMO_DBT )
   {
      return SUCCESS;
   }
   else if( pArea->bMemoType == DB_MEMO_FPT && !fRaw )
   {
      if( ulByteSize == 0 )
      {
         FPTBLOCK fptBlock;

         hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulOffset *
                         ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );
         if( hb_fsRead( pArea->hMemoFile, ( BYTE * ) &fptBlock,
                        sizeof( FPTBLOCK ) ) == sizeof( FPTBLOCK ) )
         {
            ulByteSize = HB_GET_BE_UINT32( fptBlock.size ) + sizeof( FPTBLOCK );
         }
      }
      else
      {
         ulByteSize += sizeof( FPTBLOCK );
      }
   }

   ulSize = ( ulByteSize + pArea->uiMemoBlockSize - 1 ) / pArea->uiMemoBlockSize;

   if( ulSize == 0 )
   {
      return EDBF_CORRUPT;
   }

   if ( ulOffset + ulSize == pGCtable->ulNextBlock )
   {
      pGCtable->ulNextBlock -= ulSize;
      pGCtable->bChanged |= 1;
      hb_fptPackGCitems( pGCtable );
   }
   else
   {
      BOOL fChanged = FALSE;
      int i;

      for( i = 0; i < pGCtable->usItems; i++ )
      {
         if ( pGCtable->pGCitems[i].ulOffset + pGCtable->pGCitems[i].ulSize == ulOffset )
         {
            ulOffset = pGCtable->pGCitems[i].ulOffset;
            ulSize   = pGCtable->pGCitems[i].ulSize += ulSize;
            fChanged = pGCtable->pGCitems[i].fChanged = TRUE;
            break;
         }
         if ( pGCtable->pGCitems[i].ulOffset == ulOffset + ulSize )
         {
            pGCtable->pGCitems[i].ulOffset = ulOffset;
            ulSize   = pGCtable->pGCitems[i].ulSize += ulSize;
            fChanged = pGCtable->pGCitems[i].fChanged = TRUE;
            break;
         }
      }
      if ( !fChanged )
      {
         if ( pGCtable->usItems <= pGCtable->usMaxItem )
         {
            if ( pGCtable->pGCitems == NULL )
            {
               pGCtable->pGCitems = ( LPMEMOGCITEM ) hb_xgrab( sizeof( MEMOGCITEM ) * ( pGCtable->usMaxItem + 1 ) );
            }
            pGCtable->pGCitems[ pGCtable->usItems ].ulOffset = ulOffset;
            pGCtable->pGCitems[ pGCtable->usItems ].ulSize = ulSize;
            pGCtable->pGCitems[ pGCtable->usItems ].fChanged = fChanged = TRUE;
            pGCtable->usItems++;
         }
         else if ( pGCtable->pGCitems[ 0 ].ulSize < ulSize )
         {
            if ( pGCtable->ulNextBlock == pGCtable->pGCitems[ 0 ].ulOffset +
                                          pGCtable->pGCitems[ 0 ].ulSize )
            {
               pGCtable->ulNextBlock -= pGCtable->pGCitems[ 0 ].ulSize;
            }
            else if ( pGCtable->pGCitems[ 0 ].fChanged )
            {
               errCode = hb_fptWriteGCitems( pArea, pGCtable, 0 );
            }
            pGCtable->pGCitems[ 0 ].ulOffset = ulOffset;
            pGCtable->pGCitems[ 0 ].ulSize = ulSize;
            pGCtable->pGCitems[ 0 ].fChanged = fChanged = TRUE;
         }
      }

      if ( fChanged )
      {
         pGCtable->bChanged |= 2;
         hb_fptPackGCitems( pGCtable );
         hb_fptSortGCitems( pGCtable );
      }
   }

   return errCode;
}

/*
 * Get free memo block from GC free memo blocks list or allocate new one.
 */
static ERRCODE hb_fptGCgetFreeBlock( FPTAREAP pArea, LPMEMOGCTABLE pGCtable,
                                     ULONG * ulOffset, ULONG ulByteSize,
                                     BOOL fRaw )
{
   BOOL fAlloc = FALSE;
   ULONG ulSize;
   int i;


   if( pArea->bMemoType == DB_MEMO_SMT || fRaw )
   {
      ulSize = ( ulByteSize + pArea->uiMemoBlockSize - 1 ) /
               pArea->uiMemoBlockSize;
   }
   else if( pArea->bMemoType == DB_MEMO_FPT )
   {
      ulSize = ( ulByteSize + sizeof( FPTBLOCK ) + pArea->uiMemoBlockSize - 1 ) /
               pArea->uiMemoBlockSize;
   }
   else if( pArea->bMemoType == DB_MEMO_DBT )
   {
      ulSize = ( ulByteSize + pArea->uiMemoBlockSize ) /
               pArea->uiMemoBlockSize;
   }
   else
   {
      ulSize = ( ulByteSize + pArea->uiMemoBlockSize - 1 ) /
               pArea->uiMemoBlockSize;
   }

   for( i = 0; i < pGCtable->usItems; i++ )
   {
      if ( pGCtable->pGCitems[i].ulSize >= ulSize )
      {
         *ulOffset = pGCtable->pGCitems[i].ulOffset;
         pGCtable->pGCitems[i].ulOffset += ulSize;
         pGCtable->pGCitems[i].ulSize -= ulSize;
         if ( pGCtable->pGCitems[i].ulSize == 0 )
         {
            while ( ++i < pGCtable->usItems )
            {
               pGCtable->pGCitems[i-1].ulOffset = pGCtable->pGCitems[i].ulOffset;
               pGCtable->pGCitems[i-1].ulSize   = pGCtable->pGCitems[i].ulSize;
            }
            pGCtable->usItems--;
         }
         else
         {
            pGCtable->pGCitems[i].fChanged = TRUE;
            hb_fptSortGCitems( pGCtable );
         }
         pGCtable->bChanged |= 2;
         fAlloc = TRUE;
         break;
      }
   }
   if ( !fAlloc )
   {
      *ulOffset = pGCtable->ulNextBlock;
      pGCtable->ulNextBlock += ulSize;
      pGCtable->bChanged |= 1;
   }
   return SUCCESS;
}

/*
 * Init GC table free memo blok list.
 */
static void hb_fptInitGCdata( LPMEMOGCTABLE pGCtable )
{
   memset( pGCtable, 0, sizeof(MEMOGCTABLE) );
}

/*
 * Clean GC table free memo blok list.
 */
static void hb_fptDestroyGCdata( LPMEMOGCTABLE pGCtable )
{
   if ( pGCtable->pGCitems != NULL )
   {
      hb_xfree( pGCtable->pGCitems );
      pGCtable->pGCitems = NULL;
      pGCtable->usItems = 0;
   }
   pGCtable->bChanged = 0;
}

/*
 * Read GC table from memo file.
 */
static ERRCODE hb_fptReadGCdata( FPTAREAP pArea, LPMEMOGCTABLE pGCtable )
{
   int i;

   hb_fptDestroyGCdata( pGCtable );
   memset( &pGCtable->fptHeader, 0, sizeof( FPTHEADER ) );

   hb_fsSeek( pArea->hMemoFile, 0, FS_SET );
   if ( hb_fsRead( pArea->hMemoFile, ( BYTE * ) &pGCtable->fptHeader, sizeof( FPTHEADER ) ) >= 512 )
   {
      if( pArea->bMemoType == DB_MEMO_SMT || pArea->bMemoType == DB_MEMO_DBT )
         pGCtable->ulNextBlock = HB_GET_LE_UINT32( pGCtable->fptHeader.nextBlock );
      else
         pGCtable->ulNextBlock = HB_GET_BE_UINT32( pGCtable->fptHeader.nextBlock );
      pGCtable->ulPrevBlock = pGCtable->ulNextBlock;

      if ( pArea->uiMemoVersion == DB_MEMOVER_SIX ||
           pArea->bMemoType == DB_MEMO_SMT )
      {
         pGCtable->bType = DB_MEMOVER_SIX;
         pGCtable->usMaxItem = MAX_SIXFREEBLOCKS;
         pGCtable->usItems = HB_GET_LE_UINT16( pGCtable->fptHeader.nGCitems );
         if ( pGCtable->usItems > pGCtable->usMaxItem )
         {
            return EDBF_CORRUPT;
         }

         pGCtable->pGCitems = ( LPMEMOGCITEM ) hb_xgrab( sizeof( MEMOGCITEM ) * ( pGCtable->usMaxItem + 1 ) );

         for( i = 0; i < pGCtable->usItems; i++ )
         {
            pGCtable->pGCitems[i].ulSize = HB_GET_LE_UINT16( &pGCtable->fptHeader.reserved2[ i * 6 ] );
            pGCtable->pGCitems[i].ulOffset = HB_GET_LE_UINT32( &pGCtable->fptHeader.reserved2[ i * 6 + 2 ] );
            pGCtable->pGCitems[i].fChanged = FALSE;
         }
      }
      else if( pArea->bMemoType == DB_MEMO_FPT &&
               ( pArea->uiMemoVersion == DB_MEMOVER_FLEX ||
                 pArea->uiMemoVersion == DB_MEMOVER_CLIP ) )
      {
         FPTBLOCK fptBlock;
         BYTE *bPageBuf;

         pGCtable->bType = DB_MEMOVER_FLEX;
         pGCtable->usMaxItem = MAX_FLEXFREEBLOCKS;
         pGCtable->ulRevPage = HB_GET_LE_UINT32( pGCtable->fptHeader.flexRev );
         pGCtable->ulDirPage = HB_GET_LE_UINT32( pGCtable->fptHeader.flexDir );
         pGCtable->ulCounter = HB_GET_LE_UINT32( pGCtable->fptHeader.counter );
         if ( pGCtable->ulDirPage )
         {
            hb_fsSeekLarge( pArea->hMemoFile, pGCtable->ulDirPage, FS_SET );
            if ( hb_fsRead( pArea->hMemoFile, ( BYTE * ) &fptBlock,
                                sizeof( FPTBLOCK ) ) != sizeof( FPTBLOCK ) ||
                 HB_GET_BE_UINT32( fptBlock.type ) != FPTIT_FLEX_GC )
            {
               return EDBF_CORRUPT;
            }
            pGCtable->ulSize = HB_GET_BE_UINT32( fptBlock.size );
            bPageBuf = ( BYTE * ) hb_xgrab( pGCtable->ulSize );
            if ( hb_fsRead( pArea->hMemoFile, bPageBuf, ( USHORT ) pGCtable->ulSize ) !=
                                                            ( USHORT ) pGCtable->ulSize )
            {
               hb_xfree( bPageBuf );
               return EDBF_CORRUPT;
            }
            pGCtable->usMaxItem = (USHORT ) ( ( pGCtable->ulSize - 2 ) >> 3 );
            pGCtable->usItems = ( HB_GET_LE_UINT16( bPageBuf ) - 3 ) >> 2;

            pGCtable->pGCitems = ( LPMEMOGCITEM ) hb_xgrab( sizeof( MEMOGCITEM ) *
                     ( HB_MIN( pGCtable->usItems, pGCtable->usMaxItem ) + 1 ) );

            for( i = 0; i < pGCtable->usItems; i++ )
            {
               pGCtable->pGCitems[i].ulOffset = HB_GET_LE_UINT32( &bPageBuf[ i * 8 + 2 ] ) /
                                                      pArea->uiMemoBlockSize;
               pGCtable->pGCitems[i].ulSize = HB_GET_LE_UINT32( &bPageBuf[ i * 8 + 6 ] ) /
                                                      pArea->uiMemoBlockSize;
               pGCtable->pGCitems[i].fChanged = FALSE;
            }
            hb_xfree( bPageBuf );
         }
      }

      if ( pGCtable->pGCitems )
      {
         hb_fptSortGCitems( pGCtable );
      }

      return SUCCESS;
   }
   return EDBF_READ;
}

/*
 * Write GC table into memo file.
 */
static ERRCODE hb_fptWriteGCdata( FPTAREAP pArea, LPMEMOGCTABLE pGCtable )
{
   ERRCODE errCode = SUCCESS;
   ULONG ulHdrSize = 512;
   int i, j;

   if ( pGCtable->bChanged > 0 )
   {
      if ( pGCtable->bType == DB_MEMOVER_SIX )
      {
         USHORT usItems = HB_MIN( pGCtable->usItems, pGCtable->usMaxItem );
         HB_PUT_LE_UINT16( pGCtable->fptHeader.nGCitems, usItems );
         memset( pGCtable->fptHeader.reserved2, 0, sizeof( pGCtable->fptHeader.reserved2 ) );
         j = pGCtable->usItems - usItems;
         for( i = j ; i < pGCtable->usItems; i++ )
         {
            HB_PUT_LE_UINT16( &pGCtable->fptHeader.reserved2[ ( i - j ) * 6 ],
                              (( USHORT ) pGCtable->pGCitems[i].ulSize ) );
            HB_PUT_LE_UINT32( &pGCtable->fptHeader.reserved2[ ( i - j ) * 6 + 2 ],
                              pGCtable->pGCitems[i].ulOffset );
         }
      }
      else if ( pGCtable->bType == DB_MEMOVER_FLEX )
      {
         ulHdrSize = sizeof( FPTHEADER );
         pGCtable->ulCounter++;
         if ( pGCtable->usItems == 0 && pGCtable->ulDirPage )
         {
            ULONG ulOffset = pGCtable->ulDirPage;
            ULONG ulSize = ( pGCtable->ulSize + pArea->uiMemoBlockSize - 1 ) /
                           pArea->uiMemoBlockSize;
            if ( pGCtable->ulRevPage )
            {
               ulSize <<= 1;
               if ( pGCtable->ulDirPage > pGCtable->ulRevPage )
               {
                  ulOffset = pGCtable->ulRevPage;
               }
            }
            ulOffset /= pArea->uiMemoBlockSize;
            if ( ulOffset + ulSize == pGCtable->ulNextBlock )
            {
               pGCtable->ulDirPage = pGCtable->ulRevPage = 0;
               pGCtable->ulNextBlock -= ulSize;
            }
         }
         else if ( pGCtable->usItems > 0 && ! pGCtable->ulDirPage )
         {
            pGCtable->ulSize = FLEXGCPAGE_SIZE;
            errCode = hb_fptGCgetFreeBlock( pArea, pGCtable,
                                &pGCtable->ulDirPage, pGCtable->ulSize, FALSE );
            if ( errCode == SUCCESS )
            {
               pGCtable->ulDirPage *= pArea->uiMemoBlockSize;
               errCode = hb_fptGCgetFreeBlock( pArea, pGCtable,
                                &pGCtable->ulRevPage, pGCtable->ulSize, FALSE );
               pGCtable->ulRevPage *= pArea->uiMemoBlockSize;
            }
            pGCtable->bChanged |= 2;
         }
         if ( pGCtable->ulDirPage && pGCtable->bChanged > 1 )
         {
            FPTBLOCK fptBlock;
            BYTE *bPageBuf;
            USHORT usItems = HB_MIN( pGCtable->usItems, pGCtable->usMaxItem );

            HB_PUT_BE_UINT32( fptBlock.type, FPTIT_FLEX_GC );
            HB_PUT_BE_UINT32( fptBlock.size, pGCtable->ulSize );
            bPageBuf = ( BYTE * ) hb_xgrab( pGCtable->ulSize );
            memset( bPageBuf, 0xAD, pGCtable->ulSize );
            HB_PUT_LE_UINT16( bPageBuf, ( (USHORT) usItems << 2 ) + 3 );
            j = pGCtable->usItems - usItems;
            for( i = j ; i < pGCtable->usItems; i++ )
            {
               HB_PUT_LE_UINT32( &bPageBuf[ ( i - j ) * 8 + 2 ],
                                pGCtable->pGCitems[i].ulOffset * pArea->uiMemoBlockSize );
               HB_PUT_LE_UINT32( &bPageBuf[ ( i - j ) * 8 + 6 ],
                                pGCtable->pGCitems[i].ulSize * pArea->uiMemoBlockSize );
            }
            hb_fsSeekLarge( pArea->hMemoFile, pGCtable->ulDirPage, FS_SET );
            if ( hb_fsWrite( pArea->hMemoFile, ( BYTE * ) &fptBlock,
                             sizeof( FPTBLOCK ) ) != sizeof( FPTBLOCK ) ||
                 hb_fsWrite( pArea->hMemoFile, bPageBuf,
                             ( USHORT ) pGCtable->ulSize ) != ( USHORT ) pGCtable->ulSize )
            {
               errCode = EDBF_WRITE;
            }
            else if ( pGCtable->ulRevPage )
            {
               for( i = j; i < pGCtable->usItems; i++ )
               {
                  HB_PUT_LE_UINT32( &bPageBuf[ ( i - j ) * 8 + 2 ],
                                   ( ( USHORT ) pGCtable->pGCitems[i].ulSize * pArea->uiMemoBlockSize ) );
                  HB_PUT_LE_UINT32( &bPageBuf[ ( i - j ) * 8 + 6 ],
                                   pGCtable->pGCitems[i].ulOffset * pArea->uiMemoBlockSize );
               }
               hb_fsSeekLarge( pArea->hMemoFile, pGCtable->ulRevPage, FS_SET );
               if ( hb_fsWrite( pArea->hMemoFile, ( BYTE * ) &fptBlock,
                                sizeof( FPTBLOCK ) ) != sizeof( FPTBLOCK ) ||
                    hb_fsWrite( pArea->hMemoFile, bPageBuf,
                                ( USHORT )pGCtable->ulSize ) != ( USHORT ) pGCtable->ulSize )
               {
                  errCode = EDBF_WRITE;
               }
            }
            hb_xfree( bPageBuf );
         }
         HB_PUT_LE_UINT32( pGCtable->fptHeader.flexRev, pGCtable->ulRevPage );
         HB_PUT_LE_UINT32( pGCtable->fptHeader.flexDir, pGCtable->ulDirPage );
         HB_PUT_LE_UINT32( pGCtable->fptHeader.counter, pGCtable->ulCounter );
      }

      if ( pGCtable->bChanged > 1 && errCode == SUCCESS )
      {
         errCode = hb_fptWriteGCitems( pArea, pGCtable, 0 );
      }
      if ( errCode == SUCCESS )
      {
         if( pArea->bMemoType == DB_MEMO_SMT || pArea->bMemoType == DB_MEMO_DBT )
            HB_PUT_LE_UINT32( pGCtable->fptHeader.nextBlock, pGCtable->ulNextBlock );
         else
            HB_PUT_BE_UINT32( pGCtable->fptHeader.nextBlock, pGCtable->ulNextBlock );
         hb_fsSeek( pArea->hMemoFile, 0, FS_SET );
         if( hb_fsWrite( pArea->hMemoFile, ( BYTE * ) &pGCtable->fptHeader, ( USHORT )ulHdrSize ) != ( USHORT ) ulHdrSize )
         {
            errCode = EDBF_WRITE;
         }
         else if( pGCtable->ulNextBlock < pGCtable->ulPrevBlock )
         {
            /* trunc file */
            hb_fsSeekLarge( pArea->hMemoFile,
                            ( HB_FOFFSET ) pGCtable->ulNextBlock *
                            ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );
            hb_fsWrite( pArea->hMemoFile, NULL, 0 );
         }
      }
      pArea->fMemoFlush = TRUE;
      pGCtable->bChanged = 0;
   }
   return errCode;
}

/*
 * Return the size of memo.
 */
static ULONG hb_fptGetMemoLen( FPTAREAP pArea, USHORT uiIndex )
{
   ULONG ulBlock, ulSize, ulType;
   FPTBLOCK fptBlock;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptGetMemoLen(%p, %hu)", pArea, uiIndex));

   if( hb_dbfGetMemoData( ( DBFAREAP ) pArea, uiIndex - 1, &ulBlock, &ulSize,
                          &ulType ) == SUCCESS )
   {
      if( ulBlock != 0 )
      {
         if( ulSize == 0 && ( pArea->bMemoType == DB_MEMO_DBT ||
                              pArea->bMemoType == DB_MEMO_FPT ) )
         {
            hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulBlock *
                            ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );
            if( pArea->bMemoType == DB_MEMO_DBT )
            {
               BYTE pBlock[ DBT_DEFBLOCKSIZE ];
               int iLen, i;

               do
               {
                  iLen = hb_fsRead( pArea->hMemoFile, pBlock, DBT_DEFBLOCKSIZE );
                  if( iLen <= 0 )
                     break;
                  i = 0;
                  while( i < iLen && pBlock[ i ] != 0x1A )
                     i++;
                  ulSize += i;
               } while( i == DBT_DEFBLOCKSIZE );
            }
            else if( hb_fsRead( pArea->hMemoFile, ( BYTE * ) &fptBlock,
                                sizeof( FPTBLOCK ) ) == sizeof( FPTBLOCK ) )
               ulSize = HB_GET_BE_UINT32( fptBlock.size );
         }
         return ulSize;
      }
   }
   return 0;
}

/*
 * Return the type of memo.
 */
static char * hb_fptGetMemoType( FPTAREAP pArea, USHORT uiIndex )
{
   ULONG ulBlock, ulSize, ulType;
   FPTBLOCK fptBlock;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptGetMemoLen(%p, %hu)", pArea, uiIndex));

   if( hb_dbfGetMemoData( ( DBFAREAP ) pArea, uiIndex - 1, &ulBlock, &ulSize,
                          &ulType ) == SUCCESS )
   {
      if( ulBlock != 0 )
      {
         if( ulType == 0 && pArea->bMemoType == DB_MEMO_FPT )
         {
            hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulBlock *
                            ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );
            if( hb_fsRead( pArea->hMemoFile, ( BYTE * ) &fptBlock,
                           sizeof( FPTBLOCK ) ) != sizeof( FPTBLOCK ) )
               return "U";
            ulType = HB_GET_BE_UINT32( fptBlock.type );
         }
      }

      if( ulType == 0 )
         return "M";

      if( pArea->bMemoType == DB_MEMO_FPT )
      {
         switch ( ulType )
         {
            case FPTIT_SIX_LNUM:
            case FPTIT_SIX_DNUM:
               return "N";
            case FPTIT_SIX_LDATE:
               return "D";
            case FPTIT_SIX_LOG:
               return "L";
            case FPTIT_SIX_CHAR:
               return "M";
            case FPTIT_SIX_ARRAY:
               return "A";
/*
            case FPTIT_SIX_BLOCK:
            case FPTIT_SIX_VREF:
            case FPTIT_SIX_MREF:
*/
            case FPTIT_FLEX_ARRAY:
            case FPTIT_FLEX_VOARR:
               return "A";
            case FPTIT_FLEX_OBJECT:
            case FPTIT_FLEX_VOOBJ:
               return "O";
            case FPTIT_FLEX_NIL:
               return "U";
            case FPTIT_FLEX_TRUE:
            case FPTIT_FLEX_FALSE:
               return "L";
            case FPTIT_FLEX_LDATE:
               return "D";
            case FPTIT_FLEX_CHAR:
            case FPTIT_FLEX_UCHAR:
            case FPTIT_FLEX_SHORT:
            case FPTIT_FLEX_USHORT:
            case FPTIT_FLEX_LONG:
            case FPTIT_FLEX_ULONG:
            case FPTIT_FLEX_DOUBLE:
            case FPTIT_FLEX_LDOUBLE:
               return "N";
            case FPTIT_TEXT:
               return "M";
            case FPTIT_PICT:
            case FPTIT_FLEX_COMPRCH:
               return "C";
         }
         return "U";
      }
      else if( pArea->bMemoType == DB_MEMO_SMT )
      {
         switch ( ulType )
         {
            case SMT_IT_NIL:
               return "U";
            case SMT_IT_CHAR:
               return "M";
            case SMT_IT_INT:
            case SMT_IT_DOUBLE:
               return "N";
            case SMT_IT_DATE:
               return "D";
            case SMT_IT_LOGICAL:
               return "L";
            case SMT_IT_ARRAY:
               return "A";
         }
         return "U";
      }
      return "M";
   }

   return "U";
}

/*
 * Calculate the size of SMT memo item
 */
static ULONG hb_fptCountSMTItemLength( FPTAREAP pArea, PHB_ITEM pItem,
                                       ULONG * pulArrayCount )
{
   ULONG ulLen, i, ulSize;

   switch( hb_itemType( pItem ) )
   {
      case HB_IT_ARRAY: /* HB_IT_OBJECT == HB_IT_ARRAY */
         (*pulArrayCount)++;
         ulSize = 3;
         ulLen = hb_arrayLen( pItem );
         if( ulLen > 0xFFFF )
            ulLen = 0xFFFF;
         for( i = 1 ; i <= ulLen ; i++ )
         {
            ulSize += hb_fptCountSMTItemLength( pArea, hb_arrayGetItemPtr( pItem, i ), pulArrayCount );
         }
         break;
      case HB_IT_MEMO:
      case HB_IT_STRING:
         ulLen = hb_itemGetCLen( pItem );
         if( ulLen > 0xFFFF )
            ulLen = 0xFFFF;
         ulSize = ulLen + 3;
         break;
      case HB_IT_LOGICAL:
         ulSize = 2;
         break;
      case HB_IT_DATE:
         ulSize = 5;
         break;
      case HB_IT_INTEGER:
      case HB_IT_LONG:
      {
         HB_LONG iVal;
         iVal = hb_itemGetNInt( pItem );
         if ( HB_LIM_INT32( iVal ) )
         {
            ulSize = 5;
            break;
         }
      }
      case HB_IT_DOUBLE:
         ulSize = 11;
         break;
      case HB_IT_NIL:
      default:
         ulSize = 1;
   }
   return ulSize;
}

/*
 * Calculate the size of SMT memo data
 */
static ERRCODE hb_fptCountSMTDataLength( FPTAREAP pArea, ULONG * ulLen )
{
   USHORT i, uiSize;
   BYTE buffer[ 2 ];

   if( hb_fsRead( pArea->hMemoFile, buffer, 1 ) != 1 )
      return EDBF_READ;

   *ulLen += 1;
   switch( buffer[ 0 ] )
   {
      case SMT_IT_ARRAY:
         if( hb_fsRead( pArea->hMemoFile, buffer, 2 ) != 2 )
            return EDBF_READ;

         uiSize = HB_GET_LE_UINT16( buffer );
         for( i = 0; i < uiSize; i++ )
         {
            ERRCODE errCode = hb_fptCountSMTDataLength( pArea, ulLen );
            if( errCode != SUCCESS )
               return errCode;
         }
         break;

      case SMT_IT_CHAR:
         if( hb_fsRead( pArea->hMemoFile, buffer, 2 ) != 2 )
            return EDBF_READ;
         uiSize = HB_GET_LE_UINT16( buffer );
         *ulLen += uiSize;
         hb_fsSeek( pArea->hMemoFile, uiSize, FS_RELATIVE );
         break;

      case SMT_IT_INT:
      case SMT_IT_DATE:
         *ulLen += 4;
         hb_fsSeek( pArea->hMemoFile, 4, FS_RELATIVE );
         break;

      case SMT_IT_DOUBLE:
         *ulLen += 10;
         hb_fsSeek( pArea->hMemoFile, 10, FS_RELATIVE );
         break;

      case SMT_IT_LOGICAL:
         *ulLen += 1;
         hb_fsSeek( pArea->hMemoFile, 1, FS_RELATIVE );
         break;

      case SMT_IT_NIL:
         break;

      default:
         return EDBF_CORRUPT;
   }

   return SUCCESS;
}

/*
 * Write VM item as SMT memos.
 */
static ULONG hb_fptStoreSMTItem( FPTAREAP pArea, PHB_ITEM pItem, BYTE ** bBufPtr )
{
   ULONG ulLen, i, ulSize = 0;
   HB_LONG iVal;
   LONG lVal;
   double dVal;
   int iWidth, iDec;

   switch( hb_itemType( pItem ) )
   {
      case HB_IT_ARRAY:
         *(*bBufPtr)++ = SMT_IT_ARRAY;
         ulLen = hb_arrayLen( pItem );
         if( ulLen > 0xFFFF )
            ulLen = 0xFFFF;
         HB_PUT_LE_UINT16( *bBufPtr, ulLen );
         *bBufPtr += 2;
         for( i = 1 ; i <= ulLen ; i++ )
         {
            ulSize += hb_fptStoreSMTItem( pArea, hb_arrayGetItemPtr( pItem, i ),
                                          bBufPtr );
         }
         break;

      case HB_IT_STRING:
      case HB_IT_MEMO:
         *(*bBufPtr)++ = SMT_IT_CHAR;
         ulLen = hb_itemGetCLen( pItem );
         if( ulLen > 0xFFFF )
            ulLen = 0xFFFF;
         HB_PUT_LE_UINT16( *bBufPtr, ulLen );
         *bBufPtr += 2;
         if ( ulLen > 0 )
         {
            memcpy( *bBufPtr, hb_itemGetCPtr( pItem ), ulLen );
#ifndef HB_CDP_SUPPORT_OFF
            hb_cdpnTranslate( ( char *) *bBufPtr, hb_cdp_page, pArea->cdPage, ulLen );
#endif
            *bBufPtr += ulLen;
         }
         break;

      case HB_IT_INTEGER:
      case HB_IT_LONG:
         iVal = hb_itemGetNInt( pItem );
         if( HB_LIM_INT32( iVal ) )
         {
            *(*bBufPtr)++ = SMT_IT_INT;
            HB_PUT_LE_UINT32( *bBufPtr, iVal );
            *bBufPtr += 4;
            break;
         }
      case HB_IT_DOUBLE:
         dVal = hb_itemGetND( pItem );
         hb_itemGetNLen( pItem, &iWidth, &iDec );
         if( iDec )
            iWidth += iDec + 1;
         *(*bBufPtr)++ = SMT_IT_DOUBLE;
         *(*bBufPtr)++ = ( BYTE ) iWidth;
         *(*bBufPtr)++ = ( BYTE ) iDec;
         HB_PUT_LE_DOUBLE( *bBufPtr, dVal );
         *bBufPtr += 8;
         break;

      case HB_IT_DATE:
         *(*bBufPtr)++ = SMT_IT_DATE;
         lVal = hb_itemGetDL( pItem );
         HB_PUT_LE_UINT32( *bBufPtr, lVal );
         *bBufPtr += 4;
         break;

      case HB_IT_LOGICAL:
         *(*bBufPtr)++ = SMT_IT_LOGICAL;
         *(*bBufPtr)++ = hb_itemGetL( pItem ) ? 1 : 0;
         break;

      case HB_IT_NIL:
      default:
         *(*bBufPtr)++ = SMT_IT_NIL;
         break;
   }
   return ulSize;
}

/*
 * Read SMT item from file
 */
static ERRCODE hb_fptReadRawSMTItem( FPTAREAP pArea, PHB_ITEM pItem )
{
   ULONG ulLen, i;
   BYTE buffer[ 10 ], *pBuffer;
   int iWidth, iDec;

   if( hb_fsRead( pArea->hMemoFile, buffer, 1 ) != 1 )
      return EDBF_READ;

   switch( buffer[ 0 ] )
   {
      case SMT_IT_ARRAY:
         if( hb_fsRead( pArea->hMemoFile, buffer, 2 ) != 2 )
            return EDBF_READ;

         ulLen = HB_GET_LE_UINT16( buffer );
         hb_arrayNew( pItem, ulLen );
         for( i = 1 ; i <= ulLen ; i++ )
         {
            ERRCODE errCode = hb_fptReadRawSMTItem( pArea, hb_arrayGetItemPtr( pItem, i ) );
            if ( errCode != SUCCESS )
               return errCode;
         }
         break;

      case SMT_IT_CHAR:
         if( hb_fsRead( pArea->hMemoFile, buffer, 2 ) != 2 )
            return EDBF_READ;
         ulLen = HB_GET_LE_UINT16( buffer );
         pBuffer = ( BYTE * ) hb_xgrab( ulLen + 1 );
         if( hb_fsRead( pArea->hMemoFile, pBuffer, ( USHORT ) ulLen ) != ( USHORT ) ulLen )
         {
            hb_xfree( pBuffer );
            return EDBF_READ;
         }
#ifndef HB_CDP_SUPPORT_OFF
         hb_cdpnTranslate( ( char *) pBuffer, pArea->cdPage, hb_cdp_page, ulLen );
#endif
         hb_itemPutCPtr( pItem, ( char *) pBuffer, ulLen );
         break;

      case SMT_IT_INT:
         if( hb_fsRead( pArea->hMemoFile, buffer, 4 ) != 4 )
            return EDBF_READ;
         hb_itemPutNInt( pItem, ( LONG ) HB_GET_LE_UINT32( buffer ) );
         break;

      case SMT_IT_DOUBLE:
         if( hb_fsRead( pArea->hMemoFile, buffer, 10 ) != 10 )
            return EDBF_READ;
         iWidth = buffer[ 0 ];
         iDec = buffer[ 1 ];
         if( iDec )
            iWidth -= iDec + 1;
         hb_itemPutNDLen( pItem, HB_GET_LE_DOUBLE( &buffer[ 2 ] ), iWidth, iDec );
         break;

      case SMT_IT_DATE:
         if( hb_fsRead( pArea->hMemoFile, buffer, 4 ) != 4 )
            return EDBF_READ;
         hb_itemPutDL( pItem, ( LONG ) HB_GET_LE_UINT32( buffer ) );
         break;

      case SMT_IT_LOGICAL:
         if( hb_fsRead( pArea->hMemoFile, buffer, 1 ) != 1 )
            return EDBF_READ;
         hb_itemPutL( pItem, buffer[ 0 ] != 0 );
         break;

      case SMT_IT_NIL:
         hb_itemClear( pItem );
         break;

      default:
         hb_itemClear( pItem );
         return EDBF_CORRUPT;
   }

   return SUCCESS;
}

/*
 * Read SMT item from memory buffer.
 */
static ERRCODE hb_fptReadSMTItem( FPTAREAP pArea, BYTE ** pbMemoBuf, BYTE * bBufEnd, PHB_ITEM pItem )
{
   ULONG ulLen, i;
   ERRCODE errCode = SUCCESS;
   int iWidth, iDec;

   if( bBufEnd - (*pbMemoBuf) >= 1 )
   {
      switch ( *(*pbMemoBuf)++ )
      {
         case SMT_IT_ARRAY:
            if( bBufEnd - (*pbMemoBuf) < 2 )
            {
               errCode = EDBF_CORRUPT;
               break;
            }
            ulLen = HB_GET_LE_UINT16( *pbMemoBuf );
            *pbMemoBuf += 2;
            if( bBufEnd - (*pbMemoBuf) < ( LONG ) ulLen )
            {
               errCode = EDBF_CORRUPT;
               break;
            }
            hb_arrayNew( pItem, ulLen );
            for ( i = 1 ; i <= ulLen ; i++ )
            {
               errCode = hb_fptReadSMTItem( pArea, pbMemoBuf, bBufEnd,
                                            hb_arrayGetItemPtr( pItem, i ) );
               if ( errCode != SUCCESS )
                  break;
            }
            break;

         case SMT_IT_CHAR:
            if( bBufEnd - (*pbMemoBuf) < 2 )
            {
               errCode = EDBF_CORRUPT;
               break;
            }
            ulLen = HB_GET_LE_UINT16( *pbMemoBuf );
            *pbMemoBuf += 2;
            if( bBufEnd - (*pbMemoBuf) < ( LONG ) ulLen )
            {
               errCode = EDBF_CORRUPT;
            }
            else
            {
#ifndef HB_CDP_SUPPORT_OFF
               hb_cdpnTranslate( ( char *) (*pbMemoBuf), pArea->cdPage, hb_cdp_page, ulLen );
#endif
               hb_itemPutCL( pItem, ( char *) (*pbMemoBuf), ulLen );
               *pbMemoBuf += ulLen;
            }
            break;

         case SMT_IT_INT:
            if( bBufEnd - (*pbMemoBuf) < 4 )
            {
               errCode = EDBF_CORRUPT;
               break;
            }
            hb_itemPutNInt( pItem, ( LONG ) HB_GET_LE_UINT32( *pbMemoBuf ) );
            *pbMemoBuf += 4;
            break;

         case SMT_IT_DOUBLE:
            if( bBufEnd - (*pbMemoBuf) < 10 )
            {
               errCode = EDBF_CORRUPT;
               break;
            }
            iWidth = *(*pbMemoBuf)++;
            iDec = *(*pbMemoBuf)++;
            if( iDec )
               iWidth -= iDec + 1;
            hb_itemPutNDLen( pItem, HB_GET_LE_DOUBLE( *pbMemoBuf ), iWidth, iDec );
            *pbMemoBuf += 8;
            break;

         case SMT_IT_DATE:
            if( bBufEnd - (*pbMemoBuf) < 4 )
            {
               errCode = EDBF_CORRUPT;
               break;
            }
            hb_itemPutDL( pItem, ( LONG ) HB_GET_LE_UINT32( *pbMemoBuf ) );
            *pbMemoBuf += 4;
            break;

         case SMT_IT_LOGICAL:
            if( bBufEnd - (*pbMemoBuf) < 1 )
            {
               errCode = EDBF_CORRUPT;
               break;
            }
            hb_itemPutL( pItem, *(*pbMemoBuf)++ != 0 );
            break;

         case SMT_IT_NIL:
            hb_itemClear( pItem );
            break;

         default:
            hb_itemClear( pItem );
            errCode = EDBF_CORRUPT;
            break;
      }
   }
   else
   {
      errCode = EDBF_CORRUPT;
   }

   return errCode;
}

/*
 * Calculate the size of SIX memo item
 */
static ULONG hb_fptCountSixItemLength( FPTAREAP pArea, PHB_ITEM pItem,
                                       ULONG * pulArrayCount )
{
   ULONG ulLen, i, ulSize;

   switch ( hb_itemType( pItem ) )
   {
      case HB_IT_ARRAY: /* HB_IT_OBJECT == HB_IT_ARRAY */
         (*pulArrayCount)++;
         ulSize = SIX_ITEM_BUFSIZE;
         ulLen = hb_arrayLen( pItem );
         if ( pArea->uiMemoVersion == DB_MEMOVER_SIX )
         {
            /* only 2 bytes (SHORT) for SIX compatibility */
            ulLen = HB_MIN( ulLen, 0xFFFF );
         }
         for ( i = 1 ; i <= ulLen ; i++ )
         {
            ulSize += hb_fptCountSixItemLength( pArea, hb_arrayGetItemPtr( pItem, i ), pulArrayCount );
         }
         break;
      case HB_IT_MEMO:
      case HB_IT_STRING:
         ulSize = SIX_ITEM_BUFSIZE;
         ulLen = hb_itemGetCLen( pItem );
         if( pArea->uiMemoVersion == DB_MEMOVER_SIX && ulLen > 0xFFFF )
         {
            /* only 2 bytes (SHORT) for SIX compatibility */
            ulLen = 0xFFFF;
         }
         ulSize += ulLen;
         break;
      case HB_IT_INTEGER:
      case HB_IT_LONG:
      case HB_IT_DOUBLE:
      case HB_IT_DATE:
      case HB_IT_LOGICAL:
      default:
         ulSize = SIX_ITEM_BUFSIZE;
   }
   return ulSize;
}

/*
 * Write fpt vartype as SIX memos.
 */
static ULONG hb_fptStoreSixItem( FPTAREAP pArea, PHB_ITEM pItem, BYTE ** bBufPtr )
{
   ULONG ulLen, i, ulSize;
   HB_LONG iVal;
   LONG lVal;
   double dVal;
   int iWidth, iDec;
   PHB_ITEM pTmpItem;

   memset( *bBufPtr, '\0', SIX_ITEM_BUFSIZE );
   ulSize = SIX_ITEM_BUFSIZE;
   switch ( hb_itemType( pItem ) )
   {
      case HB_IT_ARRAY: /* HB_IT_OBJECT == HB_IT_ARRAY */
         HB_PUT_LE_UINT16( &(*bBufPtr)[0], FPTIT_SIX_ARRAY );
         ulLen = hb_arrayLen( pItem );
         if ( pArea->uiMemoVersion == DB_MEMOVER_SIX )
         {
            /* only 2 bytes (SHORT) for SIX compatibility */
            ulLen = HB_MIN( ulLen, 0xFFFF );
         }
         HB_PUT_LE_UINT32( &(*bBufPtr)[2], ulLen );
         *bBufPtr += SIX_ITEM_BUFSIZE;
         for ( i = 1 ; i <= ulLen ; i++ )
         {
            pTmpItem = hb_arrayGetItemPtr( pItem, i );
            ulSize += hb_fptStoreSixItem( pArea, pTmpItem, bBufPtr );
         }
         break;

      case HB_IT_INTEGER:
      case HB_IT_LONG:
         iVal = hb_itemGetNInt( pItem );
         hb_itemGetNLen( pItem, &iWidth, &iDec );
         if ( HB_LIM_INT32( iVal ) )
         {
            HB_PUT_LE_UINT16( &(*bBufPtr)[0], FPTIT_SIX_LNUM );
            HB_PUT_LE_UINT16( &(*bBufPtr)[2], iWidth );
            HB_PUT_LE_UINT16( &(*bBufPtr)[4], iDec );
            HB_PUT_LE_UINT32( &(*bBufPtr)[6], iVal );
            *bBufPtr += SIX_ITEM_BUFSIZE;
         }
         else
         {
            HB_PUT_LE_UINT16( &(*bBufPtr)[0], FPTIT_SIX_DNUM );
            HB_PUT_LE_UINT16( &(*bBufPtr)[2], iWidth );
            HB_PUT_LE_UINT16( &(*bBufPtr)[4], iDec );
            HB_PUT_LE_DOUBLE( &(*bBufPtr)[6], ( double ) iVal );
            *bBufPtr += SIX_ITEM_BUFSIZE;
         }
         break;

      case HB_IT_DOUBLE:
         dVal = hb_itemGetND( pItem );
         hb_itemGetNLen( pItem, &iWidth, &iDec );
         HB_PUT_LE_UINT16( &(*bBufPtr)[0], FPTIT_SIX_DNUM );
         HB_PUT_LE_UINT16( &(*bBufPtr)[2], iWidth );
         HB_PUT_LE_UINT16( &(*bBufPtr)[4], iDec );
         HB_PUT_LE_DOUBLE( &(*bBufPtr)[6], dVal );
         *bBufPtr += SIX_ITEM_BUFSIZE;
         break;

      case HB_IT_DATE:
         lVal = hb_itemGetDL( pItem );
         HB_PUT_LE_UINT16( &(*bBufPtr)[0], FPTIT_SIX_LDATE );
         HB_PUT_LE_UINT32( &(*bBufPtr)[6], lVal );
         *bBufPtr += SIX_ITEM_BUFSIZE;
         break;

      case HB_IT_LOGICAL:
         HB_PUT_LE_UINT16( &(*bBufPtr)[0], FPTIT_SIX_LOG );
         (*bBufPtr)[6] = hb_itemGetL( pItem ) ? 1 : 0;
         *bBufPtr += SIX_ITEM_BUFSIZE;
         break;

      case HB_IT_STRING:
      case HB_IT_MEMO:
         HB_PUT_LE_UINT16( &(*bBufPtr)[0], FPTIT_SIX_CHAR );
         ulLen = hb_itemGetCLen( pItem );
         if( pArea->uiMemoVersion == DB_MEMOVER_SIX && ulLen > 0xFFFF )
         {
            /* only 2 bytes (SHORT) for SIX compatibility */
            ulLen = 0xFFFF;
         }
         HB_PUT_LE_UINT32( &(*bBufPtr)[2], ulLen );
         *bBufPtr += SIX_ITEM_BUFSIZE;
         if ( ulLen > 0 )
         {
            memcpy( *bBufPtr, hb_itemGetCPtr( pItem ), ulLen );
#ifndef HB_CDP_SUPPORT_OFF
            hb_cdpnTranslate( ( char *) *bBufPtr, hb_cdp_page, pArea->cdPage, ulLen );
#endif
            *bBufPtr += ulLen;
         }
         break;
      default:
         HB_PUT_LE_UINT16( &(*bBufPtr)[0], FPTIT_SIX_NIL );
         *bBufPtr += SIX_ITEM_BUFSIZE;
         break;
   }
   return ulSize;
}

/*
 * Read SIX item from memo.
 */
static ERRCODE hb_fptReadSixItem( FPTAREAP pArea, BYTE ** pbMemoBuf, BYTE * bBufEnd, PHB_ITEM pItem )
{
   USHORT usType;
   ULONG ulLen, i;
   ERRCODE errCode = SUCCESS;

   ulLen = SIX_ITEM_BUFSIZE;
   if ( bBufEnd - (*pbMemoBuf) >= ( LONG ) ulLen )
   {
      usType = HB_GET_LE_UINT16( &(*pbMemoBuf)[0] );
      switch ( usType )
      {
         case FPTIT_SIX_LNUM:
            hb_itemPutNL( pItem, ( LONG ) HB_GET_LE_UINT32( &(*pbMemoBuf)[6] ) );
            break;

         case FPTIT_SIX_DNUM:
            hb_itemPutNDLen( pItem, HB_GET_LE_DOUBLE( &(*pbMemoBuf)[6] ),
                                    HB_GET_LE_UINT16( &(*pbMemoBuf)[2] ),
                                    HB_GET_LE_UINT16( &(*pbMemoBuf)[4] ) );
            break;

         case FPTIT_SIX_LDATE:
            hb_itemPutDL( pItem, ( LONG ) HB_GET_LE_UINT32( &(*pbMemoBuf)[6] ) );
            break;

         case FPTIT_SIX_LOG:
            hb_itemPutL( pItem, HB_GET_LE_UINT16( &(*pbMemoBuf)[6] ) != 0 );
            break;

         case FPTIT_SIX_CHAR:
            ulLen = HB_GET_LE_UINT32( &(*pbMemoBuf)[2] );
            if ( pArea->uiMemoVersion == DB_MEMOVER_SIX )
            {
              ulLen &= 0xFFFF; /* only 2 bytes (SHORT) for SIX compatibility */
            }
            (*pbMemoBuf) += SIX_ITEM_BUFSIZE;
            if ( bBufEnd - (*pbMemoBuf) >= ( LONG ) ulLen )
            {
#ifndef HB_CDP_SUPPORT_OFF
               hb_cdpnTranslate( ( char *) (*pbMemoBuf), pArea->cdPage, hb_cdp_page, ulLen );
#endif
               hb_itemPutCL( pItem, ( char *) (*pbMemoBuf), ulLen );
            }
            else
            {
               errCode = EDBF_CORRUPT;
            }
            break;
/*
         case FPTIT_SIX_BLOCK:
         case FPTIT_SIX_VREF:
         case FPTIT_SIX_MREF:
*/
         case FPTIT_SIX_ARRAY:
            ulLen = HB_GET_LE_UINT32( &(*pbMemoBuf)[2] );
            if ( pArea->uiMemoVersion == DB_MEMOVER_SIX )
            {
                 ulLen &= 0xFFFF; /* only 2 bytes (SHORT) for SIX compatibility */
            }
            (*pbMemoBuf) += SIX_ITEM_BUFSIZE;
            hb_arrayNew( pItem, ulLen );
            for ( i = 1 ; i <= ulLen ; i++ )
            {
               errCode = hb_fptReadSixItem( pArea, pbMemoBuf, bBufEnd,
                                            hb_arrayGetItemPtr( pItem, i ) );
               if ( errCode != SUCCESS )
               {
                  break;
               }
            }
            ulLen = 0;
            break;

         case FPTIT_SIX_NIL:
            hb_itemClear( pItem );
            break;

         default:
            errCode = EDBF_CORRUPT;
            hb_itemClear( pItem );
            break;
      }
      *pbMemoBuf += ulLen;
   }
   else
   {
      errCode = EDBF_CORRUPT;
   }

   return errCode;
}

/*
 * Calculate the size of FLEX memo item
 */
static ULONG hb_fptCountFlexItemLength( FPTAREAP pArea, PHB_ITEM pItem,
                                        ULONG * pulArrayCount )
{
   ULONG ulLen, i, ulSize = 1;
   HB_LONG iVal;

   switch ( hb_itemType( pItem ) )
   {
      case HB_IT_ARRAY:
         (*pulArrayCount)++;
         ulSize += 2;
         ulLen = hb_arrayLen( pItem ) & 0xFFFF;
         for ( i = 1 ; i <= ulLen ; i++ )
         {
            ulSize += hb_fptCountFlexItemLength( pArea, hb_arrayGetItemPtr( pItem, i ), pulArrayCount );
         }
         break;
      case HB_IT_MEMO:
      case HB_IT_STRING:
         ulLen = hb_itemGetCLen( pItem );
         if( ulLen > 0xFFFF )
            ulLen = 0xFFFF;
         if( ulLen > 0 )
            ulSize += ulLen + 2;
         break;
      case HB_IT_DATE:
         ulSize += 4;
         break;
      case HB_IT_INTEGER:
      case HB_IT_LONG:
         iVal = hb_itemGetNInt( pItem );
         ulSize += ( HB_LIM_INT8( iVal ) ? 2 :
                   ( HB_LIM_INT16( iVal ) ? 3 :
                   ( HB_LIM_INT32( iVal ) ? 5 : 10 ) ) );
         break;
      case HB_IT_DOUBLE:
         ulSize += 10;
         break;
   }
   return ulSize;
}

/*
 * Store in buffer fpt vartype as FLEX memos.
 */
static void hb_fptStoreFlexItem( FPTAREAP pArea, PHB_ITEM pItem, BYTE ** bBufPtr )
{
   ULONG ulLen, i;
   HB_LONG iVal;
   LONG lVal;
   double dVal;
   int iWidth, iDec;

   switch ( hb_itemType( pItem ) )
   {
      case HB_IT_ARRAY:
         ulLen = hb_arrayLen( pItem ) & 0xFFFF;
         *(*bBufPtr)++ = FPTIT_FLEXAR_ARAY;
         HB_PUT_LE_UINT16( *bBufPtr, ( USHORT ) ulLen );
         *bBufPtr += 2;
         for ( i = 1 ; i <= ulLen ; i++ )
         {
            hb_fptStoreFlexItem( pArea, hb_arrayGetItemPtr( pItem, i ), bBufPtr );
         }
         break;
      case HB_IT_MEMO:
      case HB_IT_STRING:
         ulLen = hb_itemGetCLen( pItem );
         if( ulLen > 0xFFFF )
            ulLen = 0xFFFF;
         if( ulLen == 0 )
         {
            *(*bBufPtr)++ = FPTIT_FLEXAR_NUL;
         }
         else
         {
            *(*bBufPtr)++ = FPTIT_FLEXAR_STR;
            HB_PUT_LE_UINT16( *bBufPtr, ( USHORT ) ulLen );
            *bBufPtr += 2;
            memcpy( *bBufPtr, hb_itemGetCPtr( pItem ), ulLen );
#ifndef HB_CDP_SUPPORT_OFF
            hb_cdpnTranslate( ( char *) *bBufPtr, hb_cdp_page, pArea->cdPage, ulLen );
#endif
            *bBufPtr += ulLen;
         }
         break;
      case HB_IT_DATE:
         *(*bBufPtr)++ = FPTIT_FLEXAR_DATEJ;
         lVal = hb_itemGetDL( pItem );
         HB_PUT_LE_UINT32( *bBufPtr, lVal );
         *bBufPtr += 4;
         break;
      case HB_IT_INTEGER:
      case HB_IT_LONG:
         iVal = hb_itemGetNInt( pItem );
         hb_itemGetNLen( pItem, &iWidth, &iDec );
         if ( HB_LIM_INT8( iVal ) )
         {
            *(*bBufPtr)++ = FPTIT_FLEXAR_CHAR1;
            *(*bBufPtr)++ = (BYTE) iVal;
            *(*bBufPtr)++ = (BYTE) iWidth;
         }
         else if ( HB_LIM_INT16( iVal ) )
         {
            *(*bBufPtr)++ = FPTIT_FLEXAR_SHORT1;
            HB_PUT_LE_UINT16( *bBufPtr, iVal );
            *bBufPtr += 2;
            *(*bBufPtr)++ = (BYTE) iWidth;
         }
         else if ( HB_LIM_INT32( iVal ) )
         {
            *(*bBufPtr)++ = FPTIT_FLEXAR_LONG1;
            HB_PUT_LE_UINT32( *bBufPtr, iVal );
            *bBufPtr += 4;
            *(*bBufPtr)++ = (BYTE) iWidth;
         }
         else
         {
            *(*bBufPtr)++ = FPTIT_FLEXAR_DOUBLE2;
            *(*bBufPtr)++ = (BYTE) iWidth;
            *(*bBufPtr)++ = (BYTE) iDec;
            HB_PUT_LE_DOUBLE( *bBufPtr, (double) iVal );
            *bBufPtr += 8;
         }
         break;
      case HB_IT_DOUBLE:
         dVal = hb_itemGetND( pItem );
         hb_itemGetNLen( pItem, &iWidth, &iDec );
         if( iDec )
            iWidth += iDec + 1;
         *(*bBufPtr)++ = FPTIT_FLEXAR_DOUBLE2;
         *(*bBufPtr)++ = (BYTE) iWidth;
         *(*bBufPtr)++ = (BYTE) iDec;
         HB_PUT_LE_DOUBLE( *bBufPtr, dVal );
         *bBufPtr += 8;
         break;
      case HB_IT_LOGICAL:
         *(*bBufPtr)++ = hb_itemGetL( pItem ) ?
                                   FPTIT_FLEXAR_TRUE : FPTIT_FLEXAR_FALSE;
         break;
      case HB_IT_NIL:
      default:
         *(*bBufPtr)++ = FPTIT_FLEXAR_NIL;
   }
}

/*
 * Read FLEX item from memo.
 */
static ERRCODE hb_fptReadFlexItem( FPTAREAP pArea, BYTE ** pbMemoBuf, BYTE * bBufEnd, PHB_ITEM pItem, BOOL bRoot )
{
   BYTE usType;
   ULONG ulLen, i;
   ERRCODE errCode = SUCCESS;

   if ( bRoot )
   {
      usType = FPTIT_FLEXAR_ARAY;
   }
   else if ( bBufEnd - (*pbMemoBuf) > 0 )
   {
      usType = *(*pbMemoBuf)++;
   }
   else
   {
      return EDBF_CORRUPT;
   }
   switch ( usType )
   {
      case FPTIT_FLEXAR_NIL:
         hb_itemClear( pItem );
         break;
      case FPTIT_FLEXAR_TRUE:
         hb_itemPutL( pItem, TRUE );
         break;
      case FPTIT_FLEXAR_FALSE:
         hb_itemPutL( pItem, FALSE );
         break;
      case FPTIT_FLEXAR_LOGIC:
         if ( bBufEnd - (*pbMemoBuf) >= 1 )
         {
            hb_itemPutL( pItem, *(*pbMemoBuf)++ != 0 );
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_DATEJ:
      case FPTIT_FLEXAR_DATEX:
         if ( bBufEnd - (*pbMemoBuf) >= 4 )
         {
            hb_itemPutDL( pItem, ( LONG ) HB_GET_LE_UINT32( *pbMemoBuf ) );
            *pbMemoBuf += 4;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_CHAR:
         if ( bBufEnd - (*pbMemoBuf) >= 1 )
         {
            hb_itemPutNI( pItem, ( signed char ) *(*pbMemoBuf)++ );
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_CHAR1:
         if ( bBufEnd - (*pbMemoBuf) >= 2 )
         {
            hb_itemPutNILen( pItem, ( signed char ) **pbMemoBuf, (*pbMemoBuf)[1] );
            *pbMemoBuf += 2;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_CHAR2:
         if ( bBufEnd - (*pbMemoBuf) >= 3 )
         {
            int iLen = (*pbMemoBuf)[1], iDec = (*pbMemoBuf)[2];
            if( iDec )
            {
               iLen -= iDec + 1;
               hb_itemPutNDLen( pItem, ( signed char ) **pbMemoBuf, iLen, iDec );
            }
            else
            {
               hb_itemPutNILen( pItem, ( signed char ) **pbMemoBuf, iLen );
            }
            *pbMemoBuf += 3;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_UCHAR:
         if ( bBufEnd - (*pbMemoBuf) >= 1 )
         {
            hb_itemPutNI( pItem, ( unsigned char ) *(*pbMemoBuf)++ );
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_UCHAR1:
         if ( bBufEnd - (*pbMemoBuf) >= 2 )
         {
            hb_itemPutNILen( pItem, ( unsigned char ) **pbMemoBuf, (*pbMemoBuf)[1] );
            *pbMemoBuf += 2;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_UCHAR2:
         if ( bBufEnd - (*pbMemoBuf) >= 3 )
         {
            int iLen = (*pbMemoBuf)[1], iDec = (*pbMemoBuf)[2];
            if( iDec )
            {
               iLen -= iDec + 1;
               hb_itemPutNDLen( pItem, ( unsigned char ) **pbMemoBuf, iLen, iDec );
            }
            else
            {
               hb_itemPutNILen( pItem, ( unsigned char ) **pbMemoBuf, iLen );
            }
            *pbMemoBuf += 3;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_SHORT:
         if ( bBufEnd - (*pbMemoBuf) >= 2 )
         {
            hb_itemPutNI( pItem, ( SHORT ) HB_GET_LE_UINT16( *pbMemoBuf ) );
            *pbMemoBuf += 2;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_SHORT1:
         if ( bBufEnd - (*pbMemoBuf) >= 3 )
         {
            hb_itemPutNILen( pItem, ( SHORT ) HB_GET_LE_UINT16( *pbMemoBuf ),
                             (*pbMemoBuf)[2] );
            *pbMemoBuf += 3;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_SHORT2:
         if ( bBufEnd - (*pbMemoBuf) >= 4 )
         {
            int iLen = (*pbMemoBuf)[2], iDec = (*pbMemoBuf)[3];
            if( iDec )
            {
               iLen -= iDec + 1;
               hb_itemPutNDLen( pItem, ( SHORT ) HB_GET_LE_UINT16( *pbMemoBuf ), iLen, iDec );
            }
            else
            {
               hb_itemPutNILen( pItem, ( SHORT ) HB_GET_LE_UINT16( *pbMemoBuf ), iLen );
            }
            *pbMemoBuf += 4;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_USHORT:
         if ( bBufEnd - (*pbMemoBuf) >= 2 )
         {
            hb_itemPutNInt( pItem, ( USHORT ) HB_GET_LE_UINT16( *pbMemoBuf ) );
            *pbMemoBuf += 2;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_USHORT1:
         if ( bBufEnd - (*pbMemoBuf) >= 3 )
         {
            hb_itemPutNIntLen( pItem, ( USHORT ) HB_GET_LE_UINT16( *pbMemoBuf ),
                               (*pbMemoBuf)[2] );
            *pbMemoBuf += 3;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_USHORT2:
         if ( bBufEnd - (*pbMemoBuf) >= 4 )
         {
            int iLen = (*pbMemoBuf)[2], iDec = (*pbMemoBuf)[3];
            if( iDec )
            {
               iLen -= iDec + 1;
               hb_itemPutNDLen( pItem, ( USHORT ) HB_GET_LE_UINT16( *pbMemoBuf ), iLen, iDec );
            }
            else
            {
               hb_itemPutNIntLen( pItem, ( USHORT ) HB_GET_LE_UINT16( *pbMemoBuf ), iLen );
            }
            *pbMemoBuf += 4;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_LONG:
         if ( bBufEnd - (*pbMemoBuf) >= 4 )
         {
            hb_itemPutNL( pItem, ( LONG ) HB_GET_LE_UINT32( *pbMemoBuf ) );
            *pbMemoBuf += 4;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_LONG1:
         if ( bBufEnd - (*pbMemoBuf) >= 5 )
         {
            hb_itemPutNLLen( pItem, ( LONG ) HB_GET_LE_UINT32( *pbMemoBuf ),
                                    (*pbMemoBuf)[4] );
            *pbMemoBuf += 5;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_LONG2:
         if ( bBufEnd - (*pbMemoBuf) >= 6 )
         {
            int iLen = (*pbMemoBuf)[4], iDec = (*pbMemoBuf)[5];
            if( iDec )
            {
               iLen -= iDec + 1;
               hb_itemPutNDLen( pItem, (LONG) HB_GET_LE_UINT32( *pbMemoBuf ), iLen, iDec );
            }
            else
            {
               hb_itemPutNLLen( pItem, (LONG) HB_GET_LE_UINT32( *pbMemoBuf ), iLen );
            }
            *pbMemoBuf += 6;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_ULONG2:
         if ( bBufEnd - (*pbMemoBuf) >= 6 )
         {
            int iLen = (*pbMemoBuf)[4], iDec = (*pbMemoBuf)[5];
            if( iDec )
            {
               iLen -= iDec + 1;
               hb_itemPutNDLen( pItem, (ULONG) HB_GET_LE_UINT32( *pbMemoBuf ), iLen, iDec );
            }
            else
            {
               hb_itemPutNIntLen( pItem, (ULONG) HB_GET_LE_UINT32( *pbMemoBuf ), iLen );
            }
            *pbMemoBuf += 6;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_DOUBLE:
         if ( bBufEnd - (*pbMemoBuf) >= 8 )
         {
            hb_itemPutND( pItem, HB_GET_LE_DOUBLE( *pbMemoBuf ) );
            *pbMemoBuf += 8;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_DOUBLE2:
         if ( bBufEnd - (*pbMemoBuf) >= 10 )
         {
            int iLen = (*pbMemoBuf)[0], iDec = (*pbMemoBuf)[1];
            if( iDec )
               iLen -= iDec + 1;
            hb_itemPutNDLen( pItem, HB_GET_LE_DOUBLE( *pbMemoBuf + 2 ), iLen, iDec );
            *pbMemoBuf += 10;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_LDOUBLE:
         if ( bBufEnd - (*pbMemoBuf) >= 10 )
         {
            /* TODO: write a cross platform converter from
                     10 digit long double to double */
            hb_itemPutND( pItem, 0.0 /* HB_GET_LE_DOUBLE( *pbMemoBuf ) */ );
            *pbMemoBuf += 10;
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      case FPTIT_FLEXAR_NUL:
         hb_itemPutCL( pItem, NULL, 0);
         break;

      case FPTIT_FLEXAR_STR:
         if ( bBufEnd - (*pbMemoBuf) >= 2 )
         {
            ulLen = HB_GET_LE_UINT16( *pbMemoBuf );
            *pbMemoBuf += 2;
            if ( bBufEnd - (*pbMemoBuf) >= ( LONG ) ulLen )
            {
#ifndef HB_CDP_SUPPORT_OFF
               hb_cdpnTranslate( ( char *) *pbMemoBuf, pArea->cdPage, hb_cdp_page, ulLen );
#endif
               hb_itemPutCL( pItem, ( char *) *pbMemoBuf, ulLen );
               *pbMemoBuf += ulLen;
            }
            else
            {
               errCode = EDBF_CORRUPT;
            }
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;

      case FPTIT_FLEXAR_ARAY:
         if ( bBufEnd - (*pbMemoBuf) >= 2 )
         {
            ulLen = HB_GET_LE_UINT16( *pbMemoBuf );
            *pbMemoBuf += 2;
            if ( bBufEnd - (*pbMemoBuf) >= ( LONG ) ulLen )
            {
               hb_arrayNew( pItem, ulLen );
               for ( i = 1 ; i <= ulLen ; i++ )
               {
                  errCode = hb_fptReadFlexItem( pArea, pbMemoBuf, bBufEnd,
                                    hb_arrayGetItemPtr( pItem, i ), FALSE );
                  if ( errCode != SUCCESS )
                  {
                     break;
                  }
               }
            }
            else
            {
               errCode = EDBF_CORRUPT;
            }
         }
         else
         {
            errCode = EDBF_CORRUPT;
         }
         break;
      default:
         /* fprintf(stderr,"Uknown FLEX array item: 0x%x = %d\n", usType, usType); fflush(stderr); */
         errCode = EDBF_CORRUPT;
         hb_itemClear( pItem );
         break;
   }
   return errCode;
}

static ERRCODE hb_fptExportToFile( FPTAREAP pArea, FHANDLE hFile, ULONG ulSize )
{
   ERRCODE errCode = SUCCESS;

   if( ulSize )
   {
      ULONG ulWritten = 0, ulRead, ulBufSize;
      BYTE * bBuffer;

      ulBufSize = HB_MIN( ( 1 << 16 ), ulSize );
      bBuffer = ( BYTE * ) hb_xgrab( ulBufSize );

      while( errCode == SUCCESS && ulWritten < ulSize )
      {
         ulRead = hb_fsReadLarge( pArea->hMemoFile, bBuffer,
                                  HB_MIN( ulBufSize, ulSize - ulWritten ) );
         if( ulRead <= 0 )
            errCode = EDBF_READ;
         else if( hb_fsWriteLarge( hFile, bBuffer, ulRead ) != ulRead )
            errCode = EDBF_WRITE;
         else
            ulWritten += ulRead;
      }
      hb_xfree( bBuffer );
   }

   return errCode;
}

static ERRCODE hb_fptReadRawBlock( FPTAREAP pArea, BYTE * bBuffer, FHANDLE hFile,
                                   ULONG ulBlock, ULONG ulSize )
{
   ERRCODE errCode = SUCCESS;

   if( ulBlock == 0 )
      return EDBF_CORRUPT;

   hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulBlock *
                   ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );

   if( hFile != FS_ERROR )
   {
      errCode = hb_fptExportToFile( pArea, hFile, ulSize );
   }
   else
   {
      if( hb_fsReadLarge( pArea->hMemoFile, bBuffer, ulSize ) != ulSize )
         errCode = EDBF_READ;
   }

   return errCode;
}

static ERRCODE hb_fptReadBlobBlock( FPTAREAP pArea, PHB_ITEM pItem,
                                    FHANDLE hFile, ULONG ulBlock,
                                    USHORT uiMode )
{
   ULONG ulSize;
   BYTE buffer[ 4 ];

   if( ulBlock == 0 )
      return EDBF_CORRUPT;

   /* TODO: uiMode => BLOB_IMPORT_COMPRESS, BLOB_IMPORT_ENCRYPT */
   HB_SYMBOL_UNUSED( uiMode );

   hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulBlock *
                   ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );

   if( hb_fsReadLarge( pArea->hMemoFile, buffer, 4 ) != 4 )
      return EDBF_READ;

   ulSize = HB_GET_LE_UINT32( buffer );
   if( hFile != FS_ERROR )
      return hb_fptExportToFile( pArea, hFile, ulSize );

   if( ulSize == 0 )
      hb_itemPutC( pItem, "" );
   else
   {
      BYTE * bBuffer = ( BYTE * ) hb_xalloc( ulSize + 1 );

      if( !bBuffer )
      {
         /* in most cases this means that file is corrupted */
         return EDBF_CORRUPT;
      }
      if( hb_fsReadLarge( pArea->hMemoFile, bBuffer, ulSize ) != ulSize )
      {
         hb_xfree( bBuffer );
         return EDBF_READ;
      }
      hb_itemPutCPtr( pItem, ( char * ) bBuffer, ulSize );
   }
   return SUCCESS;
}

static ERRCODE hb_fptReadSMTBlock( FPTAREAP pArea, PHB_ITEM pItem,
                                   ULONG ulBlock, ULONG ulSize )
{
   if( ulBlock == 0 )
      return EDBF_CORRUPT;

   hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulBlock *
                   ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );

   if( ulSize == 0 )
   {
      return hb_fptReadRawSMTItem( pArea, pItem );
   }
   else
   {
      ERRCODE errCode;
      BYTE * bBuffer = ( BYTE * ) hb_xalloc( ulSize ), * bMemoBuf;

      if( !bBuffer )
      {
         /* in most cases this means that file is corrupted */
         return EDBF_CORRUPT;
      }

      if( hb_fsReadLarge( pArea->hMemoFile, bBuffer, ulSize ) != ulSize )
      {
         errCode = EDBF_READ;
      }
      else
      {
         bMemoBuf = bBuffer;
         errCode = hb_fptReadSMTItem( pArea, &bMemoBuf, bMemoBuf + ulSize, pItem );
      }
      hb_xfree( bBuffer );
      return errCode;
   }
}

/*
 * Read fpt vartype memos.
 */
static ERRCODE hb_fptGetMemo( FPTAREAP pArea, USHORT uiIndex, PHB_ITEM pItem,
                              FHANDLE hFile, ULONG ulBlock, ULONG ulStart,
                              ULONG ulCount )
{
   ERRCODE errCode;
   ULONG ulSize, ulType;
   BYTE * pBuffer, * bMemoBuf;
   FPTBLOCK fptBlock;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptGetMemo(%p, %hu, %p, %p, %lu, %lu)", pArea, uiIndex, pItem, hFile, ulStart, ulCount));

   if( uiIndex )
   {
      errCode = hb_dbfGetMemoData( ( DBFAREAP ) pArea, uiIndex - 1,
                                   &ulBlock, &ulSize, &ulType );
   }
   else if( ! hb_fptHasDirectAccess( pArea ) )
   {
      errCode = EDBF_UNSUPPORTED;
   }
   else
   {
      ulSize = ulType = 0;
      errCode = SUCCESS;
   }

   if( errCode != SUCCESS )
      return errCode;

   if( ulBlock > 0 )
   {
      if( pArea->bMemoType == DB_MEMO_FPT )
      {
         hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulBlock *
                         ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );
         if( hb_fsRead( pArea->hMemoFile, ( BYTE * ) &fptBlock,
                        sizeof( FPTBLOCK ) ) != sizeof( FPTBLOCK ) )
         {
            return EDBF_READ;
         }
         ulType = HB_GET_BE_UINT32( fptBlock.type );
         ulSize = HB_GET_BE_UINT32( fptBlock.size );
      }
      else
      {
         if( pArea->bMemoType == DB_MEMO_DBT )
         {
            ulSize = hb_fptGetMemoLen( pArea, uiIndex );
            ulType = FPTIT_BINARY;
         }
         hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulBlock *
                         ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );
      }

      if( ulStart || ulCount )
      {
         if( pArea->bMemoType == DB_MEMO_FPT )
         {
            if( ulType != FPTIT_TEXT && ulType != FPTIT_PICT )
               ulStart = ulCount = 0;
         }
         else if( pArea->bMemoType == DB_MEMO_SMT )
         {
            if( ulType != SMT_IT_CHAR )
               ulStart = ulCount = 0;
         }
      }

      if( ulStart >= ulSize )
         ulSize = 0;
      else
         ulSize -= ulStart;
      if( ulCount && ulCount < ulSize )
         ulSize = ulCount;
      if( ulStart && ulSize )
         hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulStart, FS_RELATIVE );

      if( hFile != FS_ERROR )
      {
         return hb_fptExportToFile( pArea, hFile, ulSize );
      }

      if( pArea->bMemoType == DB_MEMO_FPT )
      {
         pBuffer = ( BYTE * ) hb_xalloc( HB_MAX( ulSize + 1, 8 ) );
         if( pBuffer )
            memset( pBuffer, '\0', 8 );
      }
      else
      {
         pBuffer = ( BYTE * ) hb_xalloc( ulSize + 1 );
      }
      
      if( !pBuffer )
      {
         /* in most cases this means that file is corrupted */
         return EDBF_CORRUPT;
      }

      if ( ulSize != 0 && hb_fsReadLarge( pArea->hMemoFile, pBuffer, ulSize ) != ulSize )
      {
         errCode = EDBF_READ;
      }
      else if( pArea->bMemoType == DB_MEMO_DBT )
      {
#ifndef HB_CDP_SUPPORT_OFF
         hb_cdpnTranslate( ( char *) pBuffer, pArea->cdPage, hb_cdp_page, ulSize );
#endif
         pBuffer[ ulSize ] = '\0';
         hb_itemPutCPtr( pItem, ( char * ) pBuffer, ulSize );
         hb_itemSetCMemo( pItem );
         pBuffer = NULL;
      }
      else if( pArea->bMemoType == DB_MEMO_SMT )
      {
         if( ulType == SMT_IT_CHAR )
         {
#ifndef HB_CDP_SUPPORT_OFF
            hb_cdpnTranslate( ( char *) pBuffer, pArea->cdPage, hb_cdp_page, ulSize );
#endif
            pBuffer[ ulSize ] = '\0';
            hb_itemPutCPtr( pItem, ( char * ) pBuffer, ulSize );
            hb_itemSetCMemo( pItem );
            pBuffer = NULL;
         }
         else if( !ulSize || pBuffer[ 0 ] != ( BYTE ) ulType )
         {
            errCode = EDBF_CORRUPT;
            hb_itemClear( pItem );
         }
         else
         {
            bMemoBuf = pBuffer;
            errCode = hb_fptReadSMTItem( pArea, &bMemoBuf, bMemoBuf + ulSize, pItem );
         }
      }
      else
      {
         switch( ulType )
         {
            case FPTIT_SIX_LNUM:
            case FPTIT_SIX_DNUM:
            case FPTIT_SIX_LDATE:
            case FPTIT_SIX_LOG:
            case FPTIT_SIX_CHAR:
            case FPTIT_SIX_ARRAY:
/*
            case FPTIT_SIX_BLOCK:
            case FPTIT_SIX_VREF:
            case FPTIT_SIX_MREF:
*/
               bMemoBuf = pBuffer;
               errCode = hb_fptReadSixItem( pArea, &bMemoBuf, bMemoBuf + ulSize, pItem );
               break;
            case FPTIT_FLEX_ARRAY:
               bMemoBuf = pBuffer;
               errCode = hb_fptReadFlexItem( pArea, &bMemoBuf, bMemoBuf + ulSize, pItem, TRUE );
               break;
            case FPTIT_FLEX_NIL:
               hb_itemClear( pItem );
               break;
            case FPTIT_FLEX_TRUE:
               hb_itemPutL( pItem, TRUE );
               break;
            case FPTIT_FLEX_FALSE:
               hb_itemPutL( pItem, FALSE );
               break;
            case FPTIT_FLEX_LDATE:
               hb_itemPutDL( pItem, (LONG) HB_GET_LE_UINT32( pBuffer ) );
               break;
            case FPTIT_FLEX_CHAR:
               hb_itemPutNI( pItem, (signed char) pBuffer[0] );
               break;
            case FPTIT_FLEX_UCHAR:
               hb_itemPutNI( pItem, (BYTE) pBuffer[0] );
               break;
            case FPTIT_FLEX_SHORT:
               hb_itemPutNI( pItem, (SHORT) HB_GET_LE_UINT16( pBuffer ) );
               break;
            case FPTIT_FLEX_USHORT:
               hb_itemPutNInt( pItem, HB_GET_LE_UINT16( pBuffer ) );
               break;
            case FPTIT_FLEX_LONG:
               hb_itemPutNL( pItem, (LONG) HB_GET_LE_UINT32( pBuffer ) );
               break;
            case FPTIT_FLEX_ULONG:
               hb_itemPutNInt( pItem, HB_GET_LE_UINT32( pBuffer ) );
               break;
            case FPTIT_FLEX_DOUBLE:
               hb_itemPutND( pItem, HB_GET_LE_DOUBLE( pBuffer ) );
               break;
            case FPTIT_FLEX_LDOUBLE:
               /* TODO: write a cross platform converter from
                        10 digit long double to double */
               hb_itemPutND( pItem, 0.0 /* HB_GET_LE_DOUBLE( pBuffer ) */ );
               break;
            case FPTIT_TEXT:
#ifndef HB_CDP_SUPPORT_OFF
               hb_cdpnTranslate( ( char *) pBuffer, pArea->cdPage, hb_cdp_page, ulSize );
#endif
               pBuffer[ ulSize ] = '\0';
               hb_itemPutCPtr( pItem, ( char * ) pBuffer, ulSize );
               hb_itemSetCMemo( pItem );
               pBuffer = NULL;
               break;
            case FPTIT_PICT:
               pBuffer[ ulSize ] = '\0';
               hb_itemPutCPtr( pItem, ( char * ) pBuffer, ulSize );
               pBuffer = NULL;
               break;
            default:
               hb_itemClear( pItem );
               break;
         }
      }
      if ( pBuffer )
         hb_xfree(pBuffer);
   }
   else
   {
      hb_itemPutC( pItem, "" );
      hb_itemSetCMemo( pItem );
   }
   return errCode;
}

/*
 * Write memo data.
 */
static ERRCODE hb_fptWriteMemo( FPTAREAP pArea, ULONG ulBlock, ULONG ulSize,
                                BYTE *bBufPtr, FHANDLE hFile,
                                ULONG ulType, ULONG ulLen, ULONG * ulStoredBlock )
{
   MEMOGCTABLE fptGCtable;
   ERRCODE errCode;
   BOOL bWrite;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptWriteMemo(%p, %lu, %lu, %p, %p, %hu, %lu, %p)",
         pArea, ulBlock, ulSize, bBufPtr, hFile, ulType, ulLen, ulStoredBlock));

   bWrite = ( ulLen != 0 || ( pArea->bMemoType == DB_MEMO_FPT &&
              ulType != FPTIT_TEXT && ulType != FPTIT_BINARY &&
              ulType != FPTIT_DUMMY ) );

   if( ulBlock == 0 && !bWrite )
   {
      * ulStoredBlock = 0;
      return SUCCESS;
   }

   hb_fptInitGCdata( &fptGCtable );
   errCode = hb_fptReadGCdata( pArea, &fptGCtable );
   if( errCode != SUCCESS )
   {
      return errCode;
   }

   if( ulBlock > 0 )
   {
      errCode = hb_fptGCfreeBlock( pArea, &fptGCtable, ulBlock, ulSize,
                                   ulType == FPTIT_DUMMY );
      if( errCode != SUCCESS )
      {
         hb_fptDestroyGCdata( &fptGCtable );
         return errCode;
      }
   }

   /* Write memo header and data */
   if( bWrite )
   {
      errCode = hb_fptGCgetFreeBlock( pArea, &fptGCtable, ulStoredBlock, ulLen,
                                      ulType == FPTIT_DUMMY );
      if( errCode != SUCCESS )
      {
         hb_fptDestroyGCdata( &fptGCtable );
         return errCode;
      }

      hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) *ulStoredBlock *
                      ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );

      if( pArea->bMemoType == DB_MEMO_FPT && ulType != FPTIT_DUMMY )
      {
         FPTBLOCK fptBlock;
         HB_PUT_BE_UINT32( fptBlock.type, ulType );
         HB_PUT_BE_UINT32( fptBlock.size, ulLen );
         if( hb_fsWrite( pArea->hMemoFile, ( BYTE * ) &fptBlock,
                         sizeof( FPTBLOCK ) ) != sizeof( FPTBLOCK ) )
            errCode = EDBF_WRITE;
      }

      if( errCode == SUCCESS && ulLen > 0 )
      {
         /* TODO: uiMode => BLOB_IMPORT_COMPRESS, BLOB_IMPORT_ENCRYPT */
         if( hFile != FS_ERROR )
         {
            ULONG ulWritten = 0, ulRead, ulBufSize = HB_MIN( ( 1 << 16 ), ulLen );
            BYTE * bBuffer = ( BYTE * ) hb_xgrab( ulBufSize );

            do
            {
               ulRead = hb_fsReadLarge( hFile, bBuffer,
                                        HB_MIN( ulBufSize, ulLen - ulWritten ) );
               if( ulRead <= 0 )
                  errCode = EDBF_READ;
               else if( hb_fsWriteLarge( pArea->hMemoFile, bBuffer,
                                         ulRead ) != ulRead )
                  errCode = EDBF_WRITE;
               else
                  ulWritten += ulRead;
            }
            while( errCode == SUCCESS && ulWritten < ulLen );

            hb_xfree(bBuffer);
         }
         else if( hb_fsWriteLarge( pArea->hMemoFile, bBufPtr, ulLen ) != ulLen )
         {
            errCode = EDBF_WRITE;
         }
      }
      /* if written block is smaller then block size we should write at last
         block byte 0xAF to be FLEX compatible */
      if( errCode == SUCCESS )
      {
         if( pArea->bMemoType == DB_MEMO_DBT )
         {
            hb_fsWrite( pArea->hMemoFile, ( BYTE * ) "\x1A\x1A", 2 );
         }
         else if( pArea->uiMemoVersion == DB_MEMOVER_FLEX &&
             ( ulLen + sizeof( FPTBLOCK ) ) % pArea->uiMemoBlockSize != 0 )
         {
            ULONG ulBlocks = ( ulLen + sizeof( FPTBLOCK ) + pArea->uiMemoBlockSize - 1 ) /
                              pArea->uiMemoBlockSize;
            hb_fsSeekLarge( pArea->hMemoFile,
                            ( HB_FOFFSET ) ( *ulStoredBlock + ulBlocks ) *
                            ( HB_FOFFSET ) pArea->uiMemoBlockSize - 1, FS_SET );
            hb_fsWrite( pArea->hMemoFile, ( BYTE * ) "\xAF", 1 );
         }
      }
      pArea->fMemoFlush = TRUE;
   }
   else
   {
      * ulStoredBlock = 0;
   }

   if ( errCode == SUCCESS )
   {
      errCode = hb_fptWriteGCdata( pArea, &fptGCtable );
   }
   hb_fptDestroyGCdata( &fptGCtable );

   return errCode;
}

/*
 * Assign a value to the specified memo field.
 */
static ERRCODE hb_fptPutMemo( FPTAREAP pArea, USHORT uiIndex, PHB_ITEM pItem,
                              ULONG * pulBlock )
{
   ULONG ulBlock, ulSize, ulType, ulOldSize, ulOldType, ulArrayCount = 0;
   BYTE itmBuffer[FLEX_ITEM_BUFSIZE];
   BYTE  *bBufPtr = NULL, *bBufAlloc = NULL;
   ERRCODE errCode;
   HB_LONG iVal;
   LONG lVal;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptPutMemo(%p, %hu, %p, %p)", pArea, uiIndex, pItem, pulBlock));

   if( HB_IS_STRING( pItem ) )
   {
      ulType = FPTIT_TEXT;
      ulSize = hb_itemGetCLen( pItem );
      bBufPtr = ( BYTE *) hb_itemGetCPtr( pItem );
#ifndef HB_CDP_SUPPORT_OFF
      if( ulSize > 0 && pArea->cdPage != hb_cdp_page )
      {
         bBufAlloc = ( BYTE * ) hb_xgrab( ulSize );
         memcpy( bBufAlloc, bBufPtr, ulSize );
         hb_cdpnTranslate( ( char *) bBufAlloc, hb_cdp_page, pArea->cdPage, ulSize );
         bBufPtr = bBufAlloc;
      }
      if( pArea->bMemoType == DB_MEMO_SMT )
      {
         ulType = SMT_IT_CHAR;
      }
#endif
   }
   else if( pArea->bMemoType == DB_MEMO_DBT )
   {
      return EDBF_DATATYPE;
   }
   else if( pArea->bMemoType == DB_MEMO_SMT )
   {
      ulSize = hb_fptCountSMTItemLength( pArea, pItem, &ulArrayCount );
      if( ulSize == 0 )
         return EDBF_DATATYPE;
      bBufPtr = bBufAlloc = ( BYTE * ) hb_xgrab( ulSize );
      hb_fptStoreSMTItem( pArea, pItem, &bBufPtr );
      ulType = ( ULONG ) bBufAlloc[ 0 ];
      bBufPtr = bBufAlloc;
   }
   else if( pArea->uiMemoVersion == DB_MEMOVER_SIX )
   {
      if( HB_IS_NIL( pItem ) )
      {
         ulType = FPTIT_SIX_NIL;
         ulSize = 0;
      }
      else
      {
         ulSize = hb_fptCountSixItemLength( pArea, pItem, &ulArrayCount );
         if ( ulSize > 0 )
         {
            bBufPtr = bBufAlloc = ( BYTE * ) hb_xgrab( ulSize );
            hb_fptStoreSixItem( pArea, pItem, &bBufPtr );
            ulType = ( ULONG ) HB_GET_LE_UINT16( bBufAlloc );
            bBufPtr = bBufAlloc;
         }
         else
         {
            return EDBF_DATATYPE;
         }
      }
   }
   else if ( pArea->uiMemoVersion == DB_MEMOVER_FLEX )
   {
      switch ( hb_itemType( pItem ) )
      {
         case HB_IT_ARRAY:
            ulType = FPTIT_FLEX_ARRAY;
            ulSize = hb_fptCountFlexItemLength( pArea, pItem, &ulArrayCount ) - 1;
            if( ulSize > 0 )
            {
               bBufPtr = bBufAlloc = (BYTE *) hb_xgrab( ulSize + 1 );
               hb_fptStoreFlexItem( pArea, pItem, &bBufPtr );
               bBufPtr = bBufAlloc + 1; /* FLEX doesn't store the first byte of array ID */
            }
            break;
         case HB_IT_NIL:
            ulType = FPTIT_FLEX_NIL;
            ulSize = 0;
            break;
         case HB_IT_LOGICAL:
            ulType = hb_itemGetL( pItem ) ? FPTIT_FLEX_TRUE : FPTIT_FLEX_FALSE;
            ulSize = 0;
            break;
         case HB_IT_DATE:
            ulType = FPTIT_FLEX_LDATE;
            ulSize = 4;
            lVal = hb_itemGetDL( pItem );
            HB_PUT_LE_UINT32( itmBuffer, lVal );
            bBufPtr = itmBuffer;
            break;
         case HB_IT_INTEGER:
         case HB_IT_LONG:
            iVal = hb_itemGetNInt( pItem );
            if ( HB_LIM_INT8( iVal ) )
            {
               ulType = FPTIT_FLEX_CHAR;
               ulSize = 1;
               *itmBuffer = ( BYTE ) iVal;
               bBufPtr = itmBuffer;
            }
            else if ( HB_LIM_INT16( iVal ) )
            {
               ulType = FPTIT_FLEX_SHORT;
               ulSize = 2;
               HB_PUT_LE_UINT16( itmBuffer, iVal );
               bBufPtr = itmBuffer;
            }
            else if ( HB_LIM_INT32( iVal ) )
            {
               ulType = FPTIT_FLEX_LONG;
               ulSize = 4;
               HB_PUT_LE_UINT32( itmBuffer, iVal );
               bBufPtr = itmBuffer;
            }
            else
            {
               double d = (double) iVal;
               ulType = FPTIT_FLEX_DOUBLE;
               ulSize = 8;
               HB_PUT_LE_DOUBLE( itmBuffer, d );
               bBufPtr = itmBuffer;
            }
            break;
         case HB_IT_DOUBLE:
         {
            double d = hb_itemGetND( pItem );
            ulType = FPTIT_FLEX_DOUBLE;
            ulSize = 8;
            HB_PUT_LE_DOUBLE( itmBuffer, d );
            bBufPtr = itmBuffer;
            break;
         }
         default:
            ulType = FPTIT_BINARY;
            ulSize = 0;
            break;
      }
   }
   else
   {
      return EDBF_DATATYPE;
   }

   if( uiIndex )
   {
      errCode = hb_dbfGetMemoData( (DBFAREAP) pArea, uiIndex - 1,
                                   &ulBlock, &ulOldSize, &ulOldType );
   }
   else if( !pulBlock || ! hb_fptHasDirectAccess( pArea ) )
   {
      errCode = EDBF_UNSUPPORTED;
   }
   else
   {
      ulBlock = *pulBlock;
      ulOldSize = ulOldType = 0;
      errCode = SUCCESS;
   }

   if( errCode == SUCCESS )
      errCode = hb_fptWriteMemo( pArea, ulBlock, ulOldSize, bBufPtr, FS_ERROR,
                                 ulType, ulSize, &ulBlock );

   if( bBufAlloc != NULL )
   {
      hb_xfree( bBufAlloc );
   }

   if( errCode == SUCCESS )
   {
      if( uiIndex )
         hb_dbfSetMemoData( (DBFAREAP) pArea, uiIndex - 1, ulBlock, ulSize, ulType );
      else
         *pulBlock = ulBlock;
   }
   return errCode;
}

#ifdef HB_MEMO_SAFELOCK
/*
 * Check if memo field has any data
 */
static BOOL hb_fptHasMemoData( FPTAREAP pArea, USHORT uiIndex )
{
   if( --uiIndex < pArea->uiFieldCount )
   {
      LPFIELD pField = pArea->lpFields + uiIndex;

      if( pField->uiType == HB_IT_ANY )
      {
         BYTE * pFieldBuf = pArea->pRecord + pArea->pFieldOffset[ uiIndex ];

         if( pField->uiLen >= 6 )
         {
            USHORT uiType = HB_GET_LE_INT16( pFieldBuf + pField->uiLen - 2 );

            switch( uiType )
            {
               case HB_VF_ARRAY:
               case HB_VF_BLOB:
               case HB_VF_BLOBCOMPRESS:
               case HB_VF_BLOBENCRYPT:
                  return TRUE;
               case HB_VF_CHAR:
                  return pField->uiLen - 2 < uiType;
               case HB_VF_DNUM:
                  return pField->uiLen <= 12;
            }
         }
      }
      else if( pField->uiType == HB_IT_MEMO )
      {
         BYTE * pFieldBuf = pArea->pRecord + pArea->pFieldOffset[ uiIndex ];
         USHORT uiLen = pField->uiLen;

         if( uiLen == 4 )
            return HB_GET_LE_UINT32( pFieldBuf ) != 0;
         if( uiLen == 10 )
         {
            if( pArea->bMemoType == DB_MEMO_SMT )
               return HB_GET_LE_UINT32( ( ( LPSMTFIELD ) pFieldBuf )->block ) != 0;
            do
            {
               if( *pFieldBuf >= '1' && *pFieldBuf <= '9' )
                  return TRUE;
               ++pFieldBuf;
            }
            while( --uiLen );
         }
      }
   }
   return FALSE;
}
#endif

static ERRCODE hb_fptLockForRead( FPTAREAP pArea, USHORT uiIndex, BOOL *fUnLock )
{
   ERRCODE uiError;
   BOOL fLocked;

   *fUnLock = FALSE;
#ifdef HB_MEMO_SAFELOCK
   if( pArea->lpdbPendingRel )
   {
      uiError = SELF_FORCEREL( ( AREAP ) pArea );
      if( uiError != SUCCESS )
         return uiError;
   }

   if( ( uiIndex > 0 && pArea->lpFields[ uiIndex - 1 ].uiType == HB_IT_ANY &&
         pArea->lpFields[ uiIndex - 1 ].uiLen < 6 ) ||
       !pArea->fPositioned || !pArea->fShared ||
       pArea->fFLocked || pArea->fRecordChanged )
   {
      fLocked = TRUE;
   }
   else
   {
      PHB_ITEM pRecNo = hb_itemNew( NULL ), pResult = hb_itemNew( NULL );

      uiError = SELF_RECINFO( ( AREAP ) pArea, pRecNo, DBRI_LOCKED, pResult );
      fLocked = hb_itemGetL( pResult );
      hb_itemRelease( pRecNo );
      hb_itemRelease( pResult );
      if( uiError != SUCCESS )
         return uiError;
   }

   if( !fLocked )
   {
      if( !pArea->fValidBuffer || uiIndex == 0 ||
          hb_fptHasMemoData( pArea, uiIndex ) )
      {
         if( !hb_fptFileLockSh( pArea, TRUE ) )
            return FAILURE;

         *fUnLock = TRUE;
         pArea->fValidBuffer = FALSE;
      }
   }
#else
   HB_SYMBOL_UNUSED( uiIndex );
#endif
   /* update any pending relations and reread record if necessary */
   uiError = SELF_DELETED( ( AREAP ) pArea, &fLocked );

   return uiError;
}

static ERRCODE hb_fptGetVarField( FPTAREAP pArea, USHORT uiIndex, PHB_ITEM pItem, FHANDLE hFile )
{
   LPFIELD pField;
   ERRCODE uiError;
   BYTE * pFieldBuf;
   BOOL fUnLock = FALSE;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptGetVarField(%p, %hu, %p, %p)", pArea, uiIndex, pItem, hFile));

   pField = pArea->lpFields + uiIndex - 1;

   if( pField->uiType == HB_IT_ANY )
   {
      USHORT uiType;

      uiError = hb_fptLockForRead( pArea, uiIndex, &fUnLock );
      if( uiError != SUCCESS )
         return uiError;

      pFieldBuf = pArea->pRecord + pArea->pFieldOffset[ uiIndex - 1 ];
      if( pField->uiLen >= 6 )
         uiType = HB_GET_LE_INT16( pFieldBuf + pField->uiLen - 2 );
      else
         uiType = 0;

      if( pField->uiLen == 3 || uiType == HB_VF_DATE )
         hb_itemPutDL( pItem, hb_sxPtoD( ( char * ) pFieldBuf ) );
      else if( pField->uiLen == 4 || uiType == HB_VF_INT )
         hb_itemPutNIntLen( pItem, ( HB_LONG ) HB_GET_LE_INT32( pFieldBuf ), 10 );
      else if( pField->uiLen == 2 )
         hb_itemPutNIntLen( pItem, ( int ) HB_GET_LE_INT16( pFieldBuf ), 10 );
      else if( pField->uiLen == 1 )
         hb_itemPutNILen( pItem, ( signed char ) pFieldBuf[ 0 ], 4 );
      else if( pField->uiLen >= 6 )
      {
         ULONG ulBlock = HB_GET_LE_UINT32( pFieldBuf + pField->uiLen - 6 );

         if( uiType <= HB_VF_CHAR ) /* 64000 max string size */
         {
            char * pString = ( char * ) hb_xgrab( uiType + 1 ), * pBuf;

            pBuf = pString;
            if( uiType <= pField->uiLen - 2 )
               memcpy( pString, pFieldBuf, uiType );
            else
            {
               ULONG ulSize = uiType;

               if( pField->uiLen > 6 )
               {
                  USHORT uiVLen = pField->uiLen - 6;
                  memcpy( pBuf, pFieldBuf, uiVLen );
                  ulSize -= uiVLen;
                  pBuf += uiVLen;
               }
               uiError = hb_fptReadRawBlock( pArea, ( BYTE * ) pBuf, FS_ERROR, ulBlock, ulSize );
            }
            if( uiError == SUCCESS )
            {
               pString[ uiType ] = '\0';
#ifndef HB_CDP_SUPPORT_OFF
               hb_cdpnTranslate( pString, pArea->cdPage, hb_cdp_page, uiType );
#endif
               if( hFile != FS_ERROR )
               {
                  if( hb_fsWrite( hFile, ( BYTE * ) pString, uiType ) != uiType )
                     uiError = EDBF_WRITE;
                  hb_xfree( pString );
               }
               else
               {
                  hb_itemPutCPtr( pItem, pString, uiType );
               }
            }
            else
               hb_xfree( pString );
         }
         else if( uiType == HB_VF_LOG )
         {
            if( hFile != FS_ERROR )
               uiError = EDBF_DATATYPE;
            else
               hb_itemPutL( pItem, pFieldBuf[ 0 ] != 0 );
         }
         else if( uiType == HB_VF_DNUM ) /* n>12 VFIELD else MEMO (bLen[1],bDec[1],dVal[8]) */
         {
            if( hFile != FS_ERROR )
               uiError = EDBF_DATATYPE;
            else
            {
               BYTE pBuffer[ 11 ];
               int iWidth, iDec;

               /* should be <= 11 - it's SIX bug but I replicated it for
                  compatibility */
               if( pField->uiLen <= 12 )
               {
                  uiError = hb_fptReadRawBlock( pArea, pBuffer, FS_ERROR, ulBlock, 11 );
                  if( uiError == SUCCESS )
                  {
                     if( pBuffer[ 0 ] == SMT_IT_DOUBLE )
                        pFieldBuf = pBuffer + 1;
                     else
                        uiError = EDBF_CORRUPT;
                  }
               }
               if( uiError == SUCCESS )
               {
                  iWidth = *pFieldBuf++;
                  iDec = *pFieldBuf++;
                  if( iDec )
                     iWidth += iDec + 1;
                  hb_itemPutNDLen( pItem, HB_GET_LE_DOUBLE( pFieldBuf ), iWidth, iDec );
               }
            }
         }
         else if( uiType == HB_VF_ARRAY ) /* MEMO only as SMT ARRAY */
         {
            if( hFile != FS_ERROR )
               uiError = EDBF_DATATYPE;
            else
               uiError = hb_fptReadSMTBlock( pArea, pItem, ulBlock, 0 );
         }
         else if( uiType == HB_VF_BLOB )
            uiError = hb_fptReadBlobBlock( pArea, pItem, hFile, ulBlock, 0 );
         else if( uiType == HB_VF_BLOBCOMPRESS )
            uiError = hb_fptReadBlobBlock( pArea, pItem, hFile, ulBlock, BLOB_IMPORT_COMPRESS );
         else if( uiType == HB_VF_BLOBENCRYPT )
            uiError = hb_fptReadBlobBlock( pArea, pItem, hFile, ulBlock, BLOB_IMPORT_ENCRYPT );
         else
            uiError = EDBF_DATATYPE;
      }
   }
   else if( pField->uiType == HB_IT_MEMO )
   {
      uiError = hb_fptLockForRead( pArea, uiIndex, &fUnLock );
      if( uiError != SUCCESS )
         return uiError;

      uiError = hb_fptGetMemo( pArea, uiIndex, pItem, hFile, 0, 0, 0 );
   }
   else if( hFile == FS_ERROR )
   {
      return SUPER_GETVALUE( ( AREAP ) pArea, uiIndex, pItem );
   }
   else
   {
      return FAILURE;
   }

   if( fUnLock )
      hb_fptFileUnLock( pArea );

   return uiError;
}

static ERRCODE hb_fptGetVarFile( FPTAREAP pArea, ULONG ulBlock, BYTE * szFile, USHORT uiMode )
{
   USHORT uiError;
   FHANDLE hFile;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptGetVarFile(%p, %lu, %s, %hu)", pArea, ulBlock, szFile, uiMode));

   hFile = hb_fsExtOpen( szFile, NULL, FO_WRITE | FO_EXCLUSIVE |
                         FXO_DEFAULTS | FXO_SHARELOCK |
                         ( uiMode == FILEGET_APPEND ?
                         FXO_APPEND : FXO_TRUNCATE ),
                         NULL, NULL );

   if( hFile == FS_ERROR )
   {
      uiError = uiMode != FILEGET_APPEND ? EDBF_CREATE : EDBF_OPEN_DBF;
   }
   else
   {
      hb_fsSeekLarge( hFile, 0, FS_END );
      uiError = hb_fptGetMemo( pArea, 0, NULL, hFile, ulBlock, 0, 0 );
      hb_fsClose( hFile );
   }

   /* Exit if any error */
   if( uiError != SUCCESS )
   {
      if( uiError != FAILURE )
      {
         hb_memoErrorRT( pArea, 0, uiError,
                         uiError == EDBF_OPEN_DBF || uiError == EDBF_CREATE ||
                         uiError == EDBF_WRITE ? ( char * ) szFile :
                         pArea->szMemoFileName, 0, 0 );
      }
      return FAILURE;
   }
   return SUCCESS;
}

static ULONG hb_fptPutVarFile( FPTAREAP pArea, ULONG ulBlock, BYTE * szFile )
{
   USHORT uiError;
   FHANDLE hFile;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptPutVarFile(%p, %lu, %s)", pArea, ulBlock, szFile));

   hFile = hb_fsExtOpen( szFile, NULL, FO_READ | FO_DENYNONE |
                         FXO_DEFAULTS | FXO_SHARELOCK, NULL, NULL );
   if( hFile == FS_ERROR )
   {
      uiError = EDBF_OPEN_DBF;
   }
   else
   {
      ULONG ulSize;
      HB_FOFFSET size = hb_fsSeekLarge( hFile, 0, FS_END );
      hb_fsSeek( hFile, 0, FS_SET );
      if( ( HB_FOFFSET ) ( size & 0xFFFFFFFFUL ) == size )
         ulSize = HB_MIN( ( ULONG ) size, 0xFFFFFFFFUL - sizeof( FPTBLOCK ) );
      else
         ulSize = ( ULONG ) HB_MIN( size, ( HB_FOFFSET ) ( 0xFFFFFFFFUL - sizeof( FPTBLOCK ) ) );

      if( hb_fptFileLockEx( pArea, TRUE ) )
      {
         uiError = hb_fptWriteMemo( pArea, ulBlock, 0, NULL, hFile,
                                    0, ulSize, &ulBlock );
         hb_fptFileUnLock( pArea );
      }
      else
      {
         uiError = EDBF_LOCK;
      }
      hb_fsClose( hFile );
   }

   if( uiError != SUCCESS )
   {
      hb_memoErrorRT( pArea, 0, uiError,
                      uiError == EDBF_OPEN_DBF || uiError == EDBF_READ ?
                      ( char * ) szFile : pArea->szMemoFileName, 0, 0 );
      ulBlock = 0;
   }

   return ulBlock;
}

static ERRCODE hb_fptPutVarField( FPTAREAP pArea, USHORT uiIndex, PHB_ITEM pItem )
{
   LPFIELD pField;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptPutVarField(%p, %hu, %p)", pArea, uiIndex, pItem));

   pField = pArea->lpFields + uiIndex - 1;

   if( pField->uiType == HB_IT_ANY || pField->uiType == HB_IT_MEMO )
   {
      BYTE * pFieldBuf = pArea->pRecord + pArea->pFieldOffset[ uiIndex - 1 ];
      ERRCODE uiError;
      BOOL bDeleted;

      /* update any pending relations and reread record if necessary */
      uiError = SELF_DELETED( ( AREAP ) pArea, &bDeleted );
      if( uiError != SUCCESS )
         return uiError;

      if( !pArea->fPositioned )
         return SUCCESS;

      /* Buffer is hot? */
      if( !pArea->fRecordChanged )
      {
         uiError = SELF_GOHOT( ( AREAP ) pArea );
         if ( uiError != SUCCESS )
            return uiError;
      }

      if( pField->uiType == HB_IT_MEMO )
      {
         if( !hb_fptFileLockEx( pArea, TRUE ) )
            return EDBF_LOCK;
         uiError = hb_fptPutMemo( pArea, uiIndex, pItem, NULL );
#if defined( HB_MEMO_SAFELOCK )
         if( uiError == SUCCESS )
         {
            /* Force writer record to eliminate race condition */
            SELF_GOCOLD( ( AREAP ) pArea );
         }
#endif
         hb_fptFileUnLock( pArea );
      }
      else if( pField->uiLen == 3 )
      {
         if( ! HB_IS_DATE( pItem ) )
         {
            return EDBF_DATATYPE;
         }
         hb_sxDtoP( ( char * ) pFieldBuf, hb_itemGetDL( pItem ) );
      }
      else if( pField->uiLen == 4 )
      {
         HB_LONG lVal;

         if( ! HB_IS_NUMBER( pItem ) )
            return EDBF_DATATYPE;
         lVal = hb_itemGetNInt( pItem );
         if( HB_IS_DOUBLE( pItem ) ?
                        ! HB_DBL_LIM_INT32( hb_itemGetND( pItem ) ) :
                        ! HB_LIM_INT32( lVal ) )
         {
            return EDBF_DATAWIDTH;
         }
         HB_PUT_LE_UINT32( pFieldBuf, ( UINT32 ) lVal );
      }
      else if( pField->uiLen < 6 )
      {
         return EDBF_DATATYPE;
      }
      else
      {
         BYTE buffer[ 11 ], *pBlock, *pAlloc = NULL;
         ULONG ulOldBlock = 0, ulOldSize = 0, ulNewSize = 0;
         USHORT uiType = HB_GET_LE_INT16( pFieldBuf + pField->uiLen - 2 );

         if( ( uiType <= HB_VF_CHAR && uiType > pField->uiLen - 2 ) ||
             ( uiType == HB_VF_DNUM && pField->uiLen <= 12 ) ||
             uiType == HB_VF_ARRAY || uiType == HB_VF_BLOB ||
             uiType == HB_VF_BLOBCOMPRESS || uiType == HB_VF_BLOBENCRYPT )
         {
            ulOldBlock = HB_GET_LE_UINT32( pFieldBuf + pField->uiLen - 6 );
            if( ulOldBlock )
            {
               if( uiType <= HB_VF_CHAR )
                  ulOldSize = uiType - ( pField->uiLen - 6 );
               else if( uiType == HB_VF_DNUM )
                  ulOldSize = 11;
               else if( uiType == HB_VF_ARRAY )
               {
                  hb_fsSeekLarge( pArea->hMemoFile, ( HB_FOFFSET ) ulOldBlock *
                                  ( HB_FOFFSET ) pArea->uiMemoBlockSize, FS_SET );
                  if( hb_fptCountSMTDataLength( pArea, &ulOldSize ) != SUCCESS )
                     ulOldSize = 0;
               }
            }
         }

         if( HB_IS_DATE( pItem ) )
         {
            hb_sxDtoP( ( char * ) pFieldBuf, hb_itemGetDL( pItem ) );
            uiType = HB_VF_DATE;
         }
         else if( HB_IS_LOGICAL( pItem ) )
         {
            pFieldBuf[ 0 ] = hb_itemGetL( pItem ) ? 1 : 0;
            uiType = HB_VF_LOG;
         }
         else if( HB_IS_NIL( pItem ) )
         {
            uiType = 0;
         }
         else if( HB_IS_NUMBER( pItem ) )
         {
            HB_LONG lVal;
            lVal = hb_itemGetNInt( pItem );

            if( !HB_IS_DOUBLE( pItem ) && HB_LIM_INT32( lVal ) )
            {
               HB_PUT_LE_UINT32( pFieldBuf, ( UINT32 ) lVal );
               uiType = HB_VF_INT;
            }
            else
            {
               double dVal = hb_itemGetND( pItem );
               int iWidth, iDec;

               hb_itemGetNLen( pItem, &iWidth, &iDec );
               if( iDec )
                  iWidth += iDec + 1;
               buffer[ 0 ] = SMT_IT_DOUBLE;
               buffer[ 1 ] = ( BYTE ) iWidth;
               buffer[ 2 ] = ( BYTE ) iDec;
               HB_PUT_LE_DOUBLE( &buffer[ 3 ], dVal );
               uiType = HB_VF_DNUM;
               if( pField->uiLen > 12 )
                  memcpy( pFieldBuf, buffer + 1, 10 );
               else
               {
                  pBlock = buffer;
                  ulNewSize = 11;
               }
            }
         }
         else if( HB_IS_STRING( pItem ) )
         {
            ULONG ulLen = hb_itemGetCLen( pItem );

            if( ulLen > HB_VF_CHAR )
               ulLen = HB_VF_CHAR;
            uiType = ( USHORT ) ulLen;
            pAlloc = ( BYTE * ) hb_xgrab( uiType + 1 );
            memcpy( pAlloc, hb_itemGetCPtr( pItem ), uiType );
#ifndef HB_CDP_SUPPORT_OFF
            hb_cdpnTranslate( ( char * ) pAlloc, hb_cdp_page, pArea->cdPage, uiType );
#endif
            if( uiType <= pField->uiLen - 2 )
            {
               memcpy( pFieldBuf, pAlloc, uiType );
            }
            else
            {
               pBlock = pAlloc;
               ulNewSize = uiType;
               if( pField->uiLen > 6 )
               {
                  memcpy( pFieldBuf, pBlock, pField->uiLen - 6 );
                  ulNewSize -= pField->uiLen - 6;
                  pBlock += pField->uiLen - 6;
               }
            }
         }
         else if( HB_IS_ARRAY( pItem ) )
         {
            ULONG ulArrayCount = 0;
            ulNewSize = hb_fptCountSMTItemLength( pArea, pItem, &ulArrayCount );
            pBlock = pAlloc = ( BYTE * ) hb_xgrab( ulNewSize );
            hb_fptStoreSMTItem( pArea, pItem, &pBlock );
            pBlock = pAlloc;
            uiType = HB_VF_ARRAY;
         }
         else
         {
            return EDBF_DATATYPE;
         }

         HB_PUT_LE_UINT16( pFieldBuf + pField->uiLen - 2, uiType );
         if( ulNewSize )
            HB_PUT_LE_UINT32( pFieldBuf + pField->uiLen - 6, 0 );
         if( ulOldBlock != 0 || ulNewSize != 0 )
         {
            if( !hb_fptFileLockEx( pArea, TRUE ) )
            {
               uiError = EDBF_LOCK;
            }
            else
            {
               uiError = hb_fptWriteMemo( pArea, ulOldBlock, ulOldSize,
                                          pBlock, FS_ERROR,
                                          FPTIT_DUMMY, ulNewSize, &ulOldBlock );
               if( uiError == SUCCESS )
               {
                  if( ulNewSize )
                     HB_PUT_LE_UINT32( pFieldBuf + pField->uiLen - 6, ulOldBlock );
#if defined( HB_MEMO_SAFELOCK )
                  /* Force writer record to eliminate race condition */
                  SELF_GOCOLD( ( AREAP ) pArea );
#endif
               }
               hb_fptFileUnLock( pArea );
            }
         }
         if( pAlloc )
            hb_xfree( pAlloc );
      }

      return uiError;
   }
   return SUPER_PUTVALUE( ( AREAP ) pArea, uiIndex, pItem );
}


/* FPT METHODS */

/*
 * Open a data store in the WorkArea.
 * ( DBENTRYP_VP )    hb_fptOpen            : NULL
 */

/*
 * Retrieve the size of the WorkArea structure.
 * ( DBENTRYP_SP )    hb_fptStructSize
 */
static ERRCODE hb_fptStructSize( FPTAREAP pArea, USHORT * uiSize )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_fptStrucSize(%p, %p)", pArea, uiSize));
   HB_SYMBOL_UNUSED( pArea );

   * uiSize = sizeof( FPTAREA );
   return SUCCESS;
}

/*
 * Obtain the length of a field value.
 * ( DBENTRYP_SVL )   hb_fptGetVarLen
 */
static ERRCODE hb_fptGetVarLen( FPTAREAP pArea, USHORT uiIndex, ULONG * pLength )
{

   HB_TRACE(HB_TR_DEBUG, ("hb_fptGetVarLen(%p, %hu, %p)", pArea, uiIndex, pLength));

   if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR &&
       pArea->lpFields[ uiIndex - 1 ].uiType == HB_IT_MEMO )
   {
      ERRCODE uiError;
      BOOL fUnLock;

      uiError = hb_fptLockForRead( pArea, uiIndex, &fUnLock );
      if( uiError == SUCCESS )
         *pLength = hb_fptGetMemoLen( pArea, uiIndex );
      else
         *pLength = 0;

      if( fUnLock )
         hb_fptFileUnLock( pArea );

      return uiError;
   }

   return SUPER_GETVARLEN( ( AREAP ) pArea, uiIndex, pLength );
}

/*
 * Obtain the current value of a field.
 * ( DBENTRYP_SI )    hb_fptGetValue
 */
static ERRCODE hb_fptGetValue( FPTAREAP pArea, USHORT uiIndex, PHB_ITEM pItem )
{
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptGetValue(%p, %hu, %p)", pArea, uiIndex, pItem));

   if( !uiIndex || uiIndex > pArea->uiFieldCount )
      return FAILURE;

   uiError = hb_fptGetVarField( pArea, uiIndex, pItem, FS_ERROR );

   if( uiError != SUCCESS )
   {
      if( uiError == FAILURE )
         return FAILURE;
      hb_memoErrorRT( pArea, 0, uiError, pArea->szMemoFileName, 0, 0 );
   }
   return SUCCESS;
}

/*
 * Assign a value to a field.
 * ( DBENTRYP_SI )    hb_fptPutValue
 */
static ERRCODE hb_fptPutValue( FPTAREAP pArea, USHORT uiIndex, PHB_ITEM pItem )
{
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptPutValue(%p, %hu, %p)", pArea, uiIndex, pItem));

   if( !uiIndex || uiIndex > pArea->uiFieldCount )
      return FAILURE;

   uiError = hb_fptPutVarField( pArea, uiIndex, pItem );
   if( uiError != SUCCESS )
   {
      if( uiError == FAILURE )
         return FAILURE;
      hb_memoErrorRT( pArea, 0, uiError, pArea->szMemoFileName, 0, EF_CANDEFAULT );
   }
   return SUCCESS;
}

/* ( DBENTRYP_V )     hb_fptCloseMemFile    : NULL */

/*
 * Create a memo file in the WorkArea.
 * ( DBENTRYP_VP )    hb_fptCreateMemFile
 */
static ERRCODE hb_fptCreateMemFile( FPTAREAP pArea, LPDBOPENINFO pCreateInfo )
{
   FPTHEADER fptHeader;
   ULONG ulNextBlock, ulSize, ulLen;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptCreateMemFile(%p, %p)", pArea, pCreateInfo));

   if( pCreateInfo )
   {
      BYTE szFileName[ _POSIX_PATH_MAX + 1 ];
      PHB_FNAME pFileName;
      PHB_ITEM pError = NULL, pItem = NULL;
      BOOL bRetry;

      if( !pArea->bMemoType )
      {
         pItem = hb_itemPutNI( pItem, 0 );
         if( SELF_INFO( ( AREAP ) pArea, DBI_MEMOTYPE, pItem ) != SUCCESS )
         {
            hb_itemRelease( pItem );
            return FAILURE;
         }
         pArea->bMemoType = hb_itemGetNI( pItem );
/*
         if( !pArea->bMemoType )
         {
            pArea->bMemoType = DB_MEMO_FPT;
            pArea->uiMemoVersion = DB_MEMOVER_FLEX;
         }
*/
         if( pArea->bMemoType != DB_MEMO_DBT &&
             pArea->bMemoType != DB_MEMO_FPT &&
             pArea->bMemoType != DB_MEMO_SMT )
         {
            hb_memoErrorRT( pArea, EG_CREATE, EDBF_MEMOTYPE,
                            ( char * ) pCreateInfo->abName, 0, 0 );
            hb_itemRelease( pItem );
            return FAILURE;
         }
      }
      if( !pArea->uiMemoVersion )
      {
         if( pArea->bMemoType == DB_MEMO_SMT )
            pArea->uiMemoVersion = DB_MEMOVER_SIX;
         else if( pArea->bMemoType == DB_MEMO_FPT )
         {
            pItem = hb_itemPutNI( pItem, 0 );
            if( SELF_INFO( ( AREAP ) pArea, DBI_MEMOVERSION, pItem ) != SUCCESS )
            {
               hb_itemRelease( pItem );
               return FAILURE;
            }
            pArea->uiMemoVersion = hb_itemGetNI( pItem );
         }
         else
            pArea->uiMemoVersion = DB_MEMOVER_STD;
      }
      if( !pArea->uiMemoBlockSize )
      {
         pItem = hb_itemPutNI( pItem, 0 );
         if( SELF_INFO( ( AREAP ) pArea, DBI_MEMOBLOCKSIZE, pItem ) != SUCCESS )
         {
            hb_itemRelease( pItem );
            return FAILURE;
         }
         pArea->uiMemoBlockSize = hb_itemGetNI( pItem );
      }

      /* create file name */
      pFileName = hb_fsFNameSplit( ( char * ) pCreateInfo->abName );
      if( ! pFileName->szExtension )
      {
         pItem = hb_itemPutC( pItem, "" );
         SELF_INFO( ( AREAP ) pArea, DBI_MEMOEXT, pItem );
         pFileName->szExtension = hb_itemGetCPtr( pItem );
         hb_fsFNameMerge( ( char * ) szFileName, pFileName );
      }
      else
      {
         hb_strncpy( ( char * ) szFileName, ( char * ) pCreateInfo->abName, _POSIX_PATH_MAX );
      }
      hb_xfree( pFileName );


      if( pItem )
      {
         hb_itemRelease( pItem );
      }

      /* Try create */
      do
      {
         pArea->hMemoFile = hb_fsExtOpen( szFileName, NULL,
                                          FO_READWRITE | FO_EXCLUSIVE | FXO_TRUNCATE |
                                          FXO_DEFAULTS | FXO_SHARELOCK,
                                          NULL, pError );
         if( pArea->hMemoFile == FS_ERROR )
         {
            if( !pError )
            {
               pError = hb_errNew();
               hb_errPutGenCode( pError, EG_CREATE );
               hb_errPutSubCode( pError, EDBF_CREATE_MEMO );
               hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CREATE ) );
               hb_errPutOsCode( pError, hb_fsError() );
               hb_errPutFileName( pError, ( char * ) szFileName );
               hb_errPutFlags( pError, EF_CANRETRY );
            }
            bRetry = ( SELF_ERROR( ( AREAP ) pArea, pError ) == E_RETRY );
         }
         else
            bRetry = FALSE;
      } while( bRetry );
      if( pError )
         hb_itemRelease( pError );

      if( pArea->hMemoFile == FS_ERROR )
         return FAILURE;

      pArea->szMemoFileName = hb_strdup( ( char * ) szFileName );
   }
   else /* For zap file */
      hb_fsSeek( pArea->hMemoFile, 0, FS_SET );

   memset( &fptHeader, 0, sizeof( FPTHEADER ) );
   ulSize = 512;
   if ( pArea->uiMemoVersion == DB_MEMOVER_SIX )
   {
      memcpy( fptHeader.signature1, "SIxMemo", 8 );
   }
   else
   {
      memcpy( fptHeader.signature1, "Harbour", 8 );
      if( pArea->uiMemoVersion == DB_MEMOVER_FLEX ||
          pArea->uiMemoVersion == DB_MEMOVER_CLIP )
      {
         memcpy( fptHeader.signature2, "FlexFile3\003", 11 );
         ulSize = sizeof( FPTHEADER );
         if( pArea->rddID == s_uiRddIdBLOB )
         {
            HB_PUT_LE_UINT16( fptHeader.flexSize, pArea->uiMemoBlockSize );
         }
      }
   }
   ulNextBlock = ( ulSize + pArea->uiMemoBlockSize - 1 ) / pArea->uiMemoBlockSize;
   if( pArea->bMemoType == DB_MEMO_SMT || pArea->bMemoType == DB_MEMO_DBT )
   {
      HB_PUT_LE_UINT32( fptHeader.nextBlock, ulNextBlock );
      HB_PUT_LE_UINT32( fptHeader.blockSize, pArea->uiMemoBlockSize );
   }
   else
   {
      HB_PUT_BE_UINT32( fptHeader.nextBlock, ulNextBlock );
      HB_PUT_BE_UINT32( fptHeader.blockSize, pArea->uiMemoBlockSize );
   }
   if( hb_fsWrite( pArea->hMemoFile, ( BYTE * ) &fptHeader, ( USHORT) ulSize ) != ( USHORT ) ulSize )
      return FAILURE;

   ulLen = ulNextBlock * pArea->uiMemoBlockSize - ulSize;
   memset( &fptHeader, 0, sizeof( FPTHEADER ) );
   while ( ulLen > 0 )
   {
      ulSize = HB_MIN( ulLen, sizeof( FPTHEADER ) );
      if( hb_fsWrite( pArea->hMemoFile, ( BYTE * ) &fptHeader, ( USHORT ) ulSize ) != ( USHORT ) ulSize )
         return FAILURE;
      ulLen -= ulSize;
   }
   /* trunc file */
   hb_fsWrite( pArea->hMemoFile, NULL, 0 );
   pArea->fMemoFlush = TRUE;
   return SUCCESS;
}


/*
 * BLOB2FILE - retrieve memo contents into file
 * ( DBENTRYP_SVPB )  hb_fptGetValueFile
 */
static ERRCODE hb_fptGetValueFile( FPTAREAP pArea, USHORT uiIndex, BYTE * szFile, USHORT uiMode )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_fptGetValueFile(%p, %hu, %s, %hu)", pArea, uiIndex, szFile, uiMode));

   if( !uiIndex || uiIndex > pArea->uiFieldCount )
      return FAILURE;

   if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR &&
       ( pArea->lpFields[ uiIndex - 1 ].uiType == HB_IT_MEMO ||
         pArea->lpFields[ uiIndex - 1 ].uiType == HB_IT_ANY ) )
   {
      USHORT uiError;
      FHANDLE hFile;

      hFile = hb_fsExtOpen( szFile, NULL, FO_WRITE | FO_EXCLUSIVE |
                            FXO_DEFAULTS | FXO_SHARELOCK |
                            ( uiMode == FILEGET_APPEND ?
                            FXO_APPEND : FXO_TRUNCATE ),
                            NULL, NULL );

      if( hFile == FS_ERROR )
      {
         uiError = uiMode != FILEGET_APPEND ? EDBF_CREATE : EDBF_OPEN_DBF;
      }
      else
      {
         hb_fsSeekLarge( hFile, 0, FS_END );
         uiError = hb_fptGetVarField( pArea, uiIndex, NULL, hFile );
         hb_fsClose( hFile );
      }

      /* Exit if any error */
      if( uiError != SUCCESS )
      {
         if( uiError != FAILURE )
         {
            hb_memoErrorRT( pArea, 0, uiError,
                            uiError == EDBF_OPEN_DBF || uiError == EDBF_CREATE ||
                            uiError == EDBF_WRITE ? ( char * ) szFile :
                            pArea->szMemoFileName, 0, 0 );
         }
         return FAILURE;
      }
      return SUCCESS;
   }
   return SUPER_GETVALUEFILE( ( AREAP ) pArea, uiIndex, szFile, uiMode );
}

/*
 * Open a memo file in the specified WorkArea.
 * ( DBENTRYP_VP )    hb_fptOpenMemFile
 */
static ERRCODE hb_fptOpenMemFile( FPTAREAP pArea, LPDBOPENINFO pOpenInfo )
{
   BYTE szFileName[ _POSIX_PATH_MAX + 1 ];
   PHB_FNAME pFileName;
   PHB_ITEM pError;
   USHORT uiFlags;
   BOOL bRetry;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptOpenMemFile(%p, %p)", pArea, pOpenInfo));

   if( pArea->rddID == s_uiRddIdBLOB )
   {
      pArea->bMemoType = DB_MEMO_FPT;
      pArea->uiMemoVersion = DB_MEMOVER_FLEX;
   }
   else if( pArea->bMemoType != DB_MEMO_DBT &&
            pArea->bMemoType != DB_MEMO_FPT &&
            pArea->bMemoType != DB_MEMO_SMT )
   {
      hb_memoErrorRT( pArea, EG_OPEN, EDBF_MEMOTYPE,
                      ( char * ) pOpenInfo->abName, 0, 0 );
      return FAILURE;
   }

   /* create file name */
   pFileName = hb_fsFNameSplit( ( char * ) pOpenInfo->abName );
   if( ! pFileName->szExtension )
   {
      PHB_ITEM pItem = hb_itemPutC( NULL, "" );
      SELF_INFO( ( AREAP ) pArea, DBI_MEMOEXT, pItem );
      pFileName->szExtension = hb_itemGetCPtr( pItem );
      hb_fsFNameMerge( ( char * ) szFileName, pFileName );
      hb_itemRelease( pItem );
   }
   else
   {
      hb_strncpy( ( char * ) szFileName, ( char * ) pOpenInfo->abName, _POSIX_PATH_MAX );
   }
   hb_xfree( pFileName );

   uiFlags = (pOpenInfo->fReadonly ? FO_READ : FO_READWRITE) |
             (pOpenInfo->fShared ? FO_DENYNONE : FO_EXCLUSIVE);
   pError = NULL;

   /* Try open */
   do
   {
      pArea->hMemoFile = hb_fsExtOpen( szFileName, NULL, uiFlags |
                                       FXO_DEFAULTS | FXO_SHARELOCK,
                                       NULL, pError );
      if( pArea->hMemoFile == FS_ERROR )
      {
         if( !pError )
         {
            pError = hb_errNew();
            hb_errPutGenCode( pError, EG_OPEN );
            hb_errPutSubCode( pError, EDBF_OPEN_MEMO );
            hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_OPEN ) );
            hb_errPutOsCode( pError, hb_fsError() );
            hb_errPutFileName( pError, ( char * ) szFileName );
            hb_errPutFlags( pError, EF_CANRETRY | EF_CANDEFAULT );
         }
         bRetry = ( SELF_ERROR( ( AREAP ) pArea, pError ) == E_RETRY );
      }
      else
         bRetry = FALSE;
   } while( bRetry );

   if( pError )
      hb_itemRelease( pError );

   if( pArea->hMemoFile == FS_ERROR )
      return FAILURE;

   pArea->szMemoFileName = hb_strdup( ( char * ) szFileName );

   if( pArea->bMemoType == DB_MEMO_DBT )
   {
      pArea->uiMemoBlockSize = DBT_DEFBLOCKSIZE;
   }
   else
   {
      FPTHEADER fptHeader;
      memset( &fptHeader, 0, sizeof( FPTHEADER ) );
      if( hb_fptFileLockSh( pArea, TRUE ) )
      {
         hb_fsSeek( pArea->hMemoFile, 0, FS_SET );
         if( hb_fsRead( pArea->hMemoFile, ( BYTE * ) &fptHeader, sizeof( FPTHEADER ) ) >= 512 )
         {
            pArea->uiMemoVersion = DB_MEMOVER_STD;
            if( pArea->bMemoType == DB_MEMO_SMT )
               pArea->uiMemoBlockSize = ( USHORT ) HB_GET_LE_UINT32( fptHeader.blockSize );
            else
               pArea->uiMemoBlockSize = ( USHORT ) HB_GET_BE_UINT32( fptHeader.blockSize );
            /* Check for compatibility with SIX memo headers */
            if( memcmp( fptHeader.signature1, "SIxMemo", 7 ) == 0 )
            {
               pArea->uiMemoVersion = DB_MEMOVER_SIX;
            }
            /* Check for compatibility with CLIP (www.itk.ru) memo headers */
            else if( memcmp( fptHeader.signature1, "Made by CLIP", 12 ) == 0 )
            {
               pArea->uiMemoVersion = DB_MEMOVER_CLIP;
            }
            /* Check for compatibility with Clipper 5.3/FlexFile3 malformed memo headers */
            if( pArea->uiMemoVersion != DB_MEMOVER_SIX &&
                memcmp( fptHeader.signature2, "FlexFile3\003", 10) == 0 )
            {
               USHORT usSize = HB_GET_LE_UINT16( fptHeader.flexSize );
               pArea->uiMemoVersion = DB_MEMOVER_FLEX;
               if( usSize != 0 && ( pArea->uiMemoBlockSize == 0 ||
                                    pArea->rddID == s_uiRddIdBLOB ) )
               {
                  pArea->uiMemoBlockSize = usSize;
               }
            }
         }
         hb_fptFileUnLock( pArea );
      }
   }

   if( pArea->uiMemoBlockSize == 0 )
   {
      hb_memoErrorRT( pArea, EG_CORRUPTION, EDBF_CORRUPT,
                      ( char * ) pArea->szMemoFileName, 0, 0 );
      return FAILURE;
   }

   return SUCCESS;
}

/*
 * FILE2BLOB - store file contents in MEMO
 * ( DBENTRYP_SVPB )   hb_fptPutValueFile
 */
static ERRCODE hb_fptPutValueFile( FPTAREAP pArea, USHORT uiIndex, BYTE * szFile, USHORT uiMode )
{
   LPFIELD pField;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptPutValueFile(%p, %hu, %s, %hu)", pArea, uiIndex, szFile, uiMode));

   if( !uiIndex || uiIndex > pArea->uiFieldCount )
      return FAILURE;

   pField = pArea->lpFields + uiIndex - 1;

   if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR &&
       ( pField->uiType == HB_IT_MEMO || ( pField->uiType == HB_IT_ANY &&
                                           pField->uiLen >= 6 ) ) )
   {
      USHORT uiError;
      BOOL bDeleted;
      FHANDLE hFile;

      /* update any pending relations and reread record if necessary */
      uiError = SELF_DELETED( ( AREAP ) pArea, &bDeleted );
      if( uiError != SUCCESS )
         return uiError;

      if( !pArea->fPositioned )
         return FAILURE;

      /* Buffer is hot? */
      if( !pArea->fRecordChanged && SELF_GOHOT( ( AREAP ) pArea ) == FAILURE )
         return FAILURE;

      hFile = hb_fsExtOpen( szFile, NULL, FO_READ | FO_DENYNONE |
                            FXO_DEFAULTS | FXO_SHARELOCK, NULL, NULL );
      if( hFile == FS_ERROR )
      {
         uiError = EDBF_OPEN_DBF;
      }
      else if( pField->uiType == HB_IT_ANY )
      {
         PHB_ITEM pItem;
         BYTE * pAlloc;
         ULONG ulSize;
         HB_FOFFSET size = hb_fsSeekLarge( hFile, 0, FS_END );

         hb_fsSeek( hFile, 0, FS_SET );
         ulSize = ( ULONG ) HB_MIN( size, HB_VF_CHAR );
         pAlloc = ( BYTE * ) hb_xgrab( ulSize + 1 );
         if( hb_fsRead( hFile, pAlloc, ( USHORT ) ulSize ) != ( USHORT ) ulSize )
         {
            uiError = EDBF_READ;
            hb_xfree( pAlloc );
         }
         else
         {
            pAlloc[ ulSize ] = '\0';
            pItem = hb_itemPutCPtr( NULL, ( char * ) pAlloc, ulSize );
            uiError = hb_fptPutVarField( pArea, uiIndex, pItem );
            hb_itemRelease( pItem );
         }
      }
      else if( !hb_fptFileLockEx( pArea, TRUE ) )
      {
         hb_fsClose( hFile );
         uiError = EDBF_LOCK;
      }
      else
      {
         ULONG ulSize, ulBlock, ulType, ulOldSize, ulOldType;
         HB_FOFFSET size = hb_fsSeekLarge( hFile, 0, FS_END );

         hb_fsSeek( hFile, 0, FS_SET );
         if( ( HB_FOFFSET ) ( size & 0xFFFFFFFFUL ) == size )
         {
            ulSize = HB_MIN( ( ULONG ) size, 0xFFFFFFFFUL - sizeof( FPTBLOCK ) );
         }
         else
         {
            ulSize = ( ULONG ) HB_MIN( size, ( HB_FOFFSET ) ( 0xFFFFFFFFUL - sizeof( FPTBLOCK ) ) );
         }

         if( pArea->bMemoType == DB_MEMO_SMT )
            ulType = SMT_IT_CHAR;
         else
            ulType = FPTIT_BINARY;


         uiError = hb_dbfGetMemoData( (DBFAREAP) pArea, uiIndex - 1,
                                      &ulBlock, &ulOldSize, &ulOldType );
         if( uiError == SUCCESS )
            uiError = hb_fptWriteMemo( pArea, ulBlock, ulOldSize, NULL, hFile,
                                       ulType, ulSize, &ulBlock );
         if( uiError == SUCCESS )
            uiError = hb_dbfSetMemoData( (DBFAREAP) pArea, uiIndex - 1, ulBlock, ulSize, ulType );
#if defined( HB_MEMO_SAFELOCK )
         if( uiError == SUCCESS )
         {
            /* Force writer record to eliminate race condition */
            SELF_GOCOLD( ( AREAP ) pArea );
         }
#endif
         hb_fptFileUnLock( pArea );
         hb_fsClose( hFile );
      }
      /* Exit if any error */
      if( uiError != SUCCESS )
      {
         hb_memoErrorRT( pArea, 0, uiError,
                         uiError == EDBF_OPEN_DBF || uiError == EDBF_READ ?
                         ( char * ) szFile : pArea->szMemoFileName, 0, 0 );
         return FAILURE;
      }
      return SUCCESS;
   }

   return SUPER_PUTVALUEFILE( ( AREAP ) pArea, uiIndex, szFile, uiMode );
}

/*
 * Retrieve information about the current driver.
 * ( DBENTRYP_SI )    hb_fptInfo
 */
static ERRCODE hb_fptInfo( FPTAREAP pArea, USHORT uiIndex, PHB_ITEM pItem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_fptInfo(%p, %hu, %p)", pArea, uiIndex, pItem));

   switch( uiIndex )
   {
      case DBI_MEMOEXT:
         if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR )
         {
            PHB_FNAME pFileName;

            pFileName = hb_fsFNameSplit( ( char * ) pArea->szMemoFileName );
            hb_itemPutC( pItem, pFileName->szExtension );
            hb_xfree( pFileName );
         }
         else
         {
            LPDBFDATA pData = ( LPDBFDATA ) SELF_RDDNODE( pArea )->lpvCargo;
            if( pData->szMemoExt[ 0 ] )
               hb_itemPutC( pItem, pData->szMemoExt );
            else if( pArea->bMemoType == DB_MEMO_FPT &&
                     hb_set.HB_SET_MFILEEXT && hb_set.HB_SET_MFILEEXT[ 0 ] )
               hb_itemPutC( pItem, hb_set.HB_SET_MFILEEXT );
            else
            {
               char * szExt = hb_memoDefaultFileExt( pArea->bMemoType, pArea->rddID );
               if( !szExt )
                  szExt = hb_memoDefaultFileExt( pData->bMemoType, pArea->rddID );
               hb_itemPutC( pItem, szExt );
            }
         }
         break;

      case DBI_MEMOBLOCKSIZE:
         if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR )
            hb_itemPutNI( pItem, pArea->uiMemoBlockSize );
         else if( pArea->bMemoType && pArea->uiMemoBlockSize )
            hb_itemPutNI( pItem, pArea->uiMemoBlockSize );
         else if( pArea->bMemoType == DB_MEMO_DBT )
            hb_itemPutNI( pItem, DBT_DEFBLOCKSIZE );
         else
         {
            hb_itemClear( pItem );
            return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_MEMOBLOCKSIZE, 0, pItem );
         }
         break;

      case DBI_MEMOTYPE:
         if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR )
            hb_itemPutNI( pItem, pArea->bMemoType );
         else if( pArea->bMemoType )
            hb_itemPutNI( pItem, pArea->bMemoType );
         else
         {
            hb_itemClear( pItem );
            return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_MEMOTYPE, 0, pItem );
         }
         break;

      case DBI_MEMOVERSION:
         if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR )
            hb_itemPutNI( pItem, pArea->uiMemoVersion );
         else if( pArea->bMemoType != DB_MEMO_NONE &&
                  pArea->uiMemoVersion != 0 )
            hb_itemPutNI( pItem, pArea->uiMemoVersion );
         else
         {
            hb_itemClear( pItem );
            return SELF_RDDINFO( SELF_RDDNODE( pArea ), RDDI_MEMOVERSION, 0, pItem );
         }
         break;

      /* case DBI_RDD_VERSION */

      case DBI_BLOB_DIRECT_EXPORT:  /* BLOBDirectExport() { <nPointer>, <cTargetFile>, <kMOde> } */
      {
         ERRCODE errCode = FAILURE;

         if( HB_IS_ARRAY( pItem ) )
         {
            ULONG ulBlock = hb_arrayGetNL( pItem, 1 );
            BYTE * szFile = ( BYTE * ) hb_arrayGetCPtr( pItem, 2 );

            if( ulBlock && szFile && *szFile )
               errCode = hb_fptGetVarFile( pArea, ulBlock, szFile,
                                           hb_arrayGetNI( pItem, 3 ) );
         }
         hb_itemPutL( pItem, errCode == SUCCESS );
         break;
      }
      case DBI_BLOB_DIRECT_GET:     /* BLOBDirectGet() { <nPointer>, <nStart>, <nCount> } */
      {
         /* pItem := { <nPointer>, <nStart>, <nCount> } */
         ULONG ulBlock, ulStart, ulCount;
         USHORT errCode;

         if( HB_IS_ARRAY( pItem ) )
         {
            ulBlock = hb_arrayGetNL( pItem, 1 );
            ulStart = hb_arrayGetNL( pItem, 2 );
            if( ulStart )
               --ulStart;
            ulCount = hb_arrayGetNL( pItem, 3 );
         }
         else
         {
            ulBlock = ulStart = ulCount = 0;
         }
         errCode = hb_fptGetMemo( pArea, 0, pItem, FS_ERROR, ulBlock, ulStart, ulCount );
         if( errCode != SUCCESS )
         {
            if( errCode != FAILURE )
               hb_memoErrorRT( pArea, 0, errCode, pArea->szMemoFileName, 0, 0 );
            return FAILURE;
         }
         break;
      }
      case DBI_BLOB_DIRECT_IMPORT:  /* BLOBDirectImport() { <nOldPointer>, <cSourceFile> } */
         if( HB_IS_ARRAY( pItem ) )
            hb_itemPutNInt( pItem, hb_fptPutVarFile( pArea,
                                             hb_arrayGetNL( pItem, 1 ),
                                  ( BYTE * ) hb_arrayGetCPtr( pItem, 2 ) ) );
         else
            hb_itemPutNI( pItem, 0 );
         break;

      case DBI_BLOB_DIRECT_PUT:     /* BLOBDirectPut() { <nOldPointer>, <xBlob> } */
      {
         /* pItem := { <nOldPointer>, <xBlob> } */
         USHORT errCode = EDBF_UNSUPPORTED;
         ULONG ulBlock = 0;

         if( HB_IS_ARRAY( pItem ) )
         {
            PHB_ITEM pValue = hb_arrayGetItemPtr( pItem, 2 );
            ulBlock = hb_arrayGetNL( pItem, 1 );
            if( pValue )
            {
               if( hb_fptFileLockEx( pArea, TRUE ) )
               {
                  errCode = hb_fptPutMemo( pArea, 0, pValue, &ulBlock );
                  hb_fptFileUnLock( pArea );
               }
               else
                  errCode = EDBF_LOCK;
            }
         }
         hb_itemPutNInt( pItem, ulBlock );
         if( errCode != SUCCESS )
         {
            if( errCode != FAILURE )
               hb_memoErrorRT( pArea, 0, errCode, pArea->szMemoFileName, 0, 0 );
            return FAILURE;
         }
         break;
      }
      case DBI_BLOB_ROOT_GET:       /* BLOBRootGet() */
      {
         ULONG ulBlock;
         USHORT errCode;

         errCode = hb_fptGetRootBlock( pArea, &ulBlock );
         if( errCode == SUCCESS )
         {
            errCode = hb_fptGetMemo( pArea, 0, pItem, FS_ERROR, ulBlock, 0, 0 );
         }
         if( errCode != SUCCESS )
         {
            if( errCode != FAILURE )
               hb_memoErrorRT( pArea, 0, errCode, pArea->szMemoFileName, 0, 0 );
            hb_itemClear( pItem );
            return FAILURE;
         }
         break;
      }
      case DBI_BLOB_ROOT_PUT:       /* BLOBRootPut( <xBlob> ) */
      {
         ULONG ulBlock;
         USHORT errCode;

         errCode = hb_fptGetRootBlock( pArea, &ulBlock );
         if( errCode == SUCCESS )
         {
            if( hb_fptFileLockEx( pArea, TRUE ) )
            {
               errCode = hb_fptPutMemo( pArea, 0, pItem, &ulBlock );
               hb_fptFileUnLock( pArea );
               if( errCode == SUCCESS )
                  errCode = hb_fptPutRootBlock( pArea, ulBlock );
            }
            else
               errCode = EDBF_LOCK;
         }
         if( errCode != SUCCESS )
         {
            if( errCode != FAILURE )
               hb_memoErrorRT( pArea, 0, errCode, pArea->szMemoFileName, 0, 0 );
            hb_itemPutL( pItem, FALSE );
            return FAILURE;
         }
         hb_itemPutL( pItem, TRUE );
         break;
      }
      case DBI_BLOB_ROOT_LOCK:      /* BLOBRootLock() */
         hb_itemPutL( pItem, hb_fptRootBlockLock( pArea ) );
         break;

      case DBI_BLOB_ROOT_UNLOCK:    /* BLOBRootUnlock() */
         hb_itemPutL( pItem, hb_fptRootBlockUnLock( pArea ) );
         break;

      case DBI_BLOB_DIRECT_LEN:
      case DBI_BLOB_DIRECT_TYPE:
      case DBI_BLOB_INTEGRITY:
      case DBI_BLOB_OFFSET:
      case DBI_BLOB_RECOVER:
         /* TODO: implement it */
         break;

      default:
         return SUPER_INFO( ( AREAP ) pArea, uiIndex, pItem );
   }

   return SUCCESS;
}

/*
 * Retrieve information about a field.
 * ( DBENTRYP_SSI )   hb_fptFieldInfo
 */
static ERRCODE hb_fptFieldInfo( FPTAREAP pArea, USHORT uiIndex, USHORT uiType, PHB_ITEM pItem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_fptFieldInfo(%p, %hu, %hu, %p)", pArea, uiIndex, uiType, pItem));

   if( !uiIndex || uiIndex > pArea->uiFieldCount )
      return FAILURE;

   if( pArea->fHasMemo && pArea->hMemoFile != FS_ERROR &&
       pArea->lpFields[ uiIndex - 1 ].uiType == HB_IT_MEMO )
   {
      ULONG ulBlock, ulSize, ulType;
      BOOL bDeleted;

      SELF_DELETED( ( AREAP ) pArea, &bDeleted );
      switch( uiType )
      {
         case DBS_BLOB_GET:         /* BLOBGet() { <nStart>, <nCount> } */
         {
            /* pItem := { <nStart>, <nCount> } */
            ULONG ulStart, ulCount;
            USHORT errCode;

            if( HB_IS_ARRAY( pItem ) )
            {
               ulStart = hb_arrayGetNL( pItem, 1 );
               if( ulStart )
                  --ulStart;
               ulCount = hb_arrayGetNL( pItem, 2 );
            }
            else
            {
               ulStart = ulCount = 0;
            }
            errCode = hb_fptGetMemo( pArea, uiIndex, pItem, FS_ERROR, 0, ulStart, ulCount );
            if( errCode != SUCCESS )
            {
               if( errCode != FAILURE )
                  hb_memoErrorRT( pArea, 0, errCode, pArea->szMemoFileName, 0, 0 );
               return FAILURE;
            }
            return SUCCESS;
         }
         case DBS_BLOB_LEN:
            hb_itemPutNL( pItem, hb_fptGetMemoLen( pArea, uiIndex ) );
            return SUCCESS;
         case DBS_BLOB_OFFSET:
            /* Clipper 5.3 does not support it :-( [druzus] */
            hb_dbfGetMemoData( (DBFAREAP) pArea, uiIndex - 1,
                               &ulBlock, &ulSize, &ulType );
            hb_itemPutNInt( pItem, ( HB_FOFFSET ) ulBlock * pArea->uiMemoBlockSize +
                                   ( pArea->bMemoType == DB_MEMO_FPT ? sizeof( FPTBLOCK ) : 0 ) );
            return SUCCESS;
         case DBS_BLOB_POINTER:
            /*
             * Clipper 5.3 it returns the same value as DBS_BLOB_OFFSET
             * in Harbour - it's a Clipper bug [druzus]
             */
            hb_dbfGetMemoData( (DBFAREAP) pArea, uiIndex - 1,
                               &ulBlock, &ulSize, &ulType );
            hb_itemPutNL( pItem, ulBlock );
            return SUCCESS;
         case DBS_BLOB_TYPE:
            hb_itemPutC( pItem, hb_fptGetMemoType( pArea, uiIndex ) );
            return SUCCESS;
      }
   }
   return SUPER_FIELDINFO( ( AREAP ) pArea, uiIndex, uiType, pItem );
}

/*
 * Retrieve (set) information about RDD
 * ( DBENTRYP_RSLV )   hb_fptRddInfo
 */
static ERRCODE hb_fptRddInfo( LPRDDNODE pRDD, USHORT uiIndex, ULONG ulConnect, PHB_ITEM pItem )
{
   LPDBFDATA pData;

   HB_TRACE(HB_TR_DEBUG, ("hb_fptRddInfo(%p, %hu, %lu, %p)", pRDD, uiIndex, ulConnect, pItem));

   pData = ( LPDBFDATA ) pRDD->lpvCargo;

   switch( uiIndex )
   {
      case RDDI_MEMOEXT:
      {
         char * szNew = hb_itemGetCPtr( pItem );

         if( szNew && szNew[0] == '.' && szNew[1] )
            szNew = hb_strdup( szNew );
         else
            szNew = NULL;

         if( pData->szMemoExt[ 0 ] )
            hb_itemPutC( pItem, pData->szMemoExt );
         else if( pData->bMemoType == DB_MEMO_FPT && pRDD->rddID != s_uiRddIdBLOB &&
                  hb_set.HB_SET_MFILEEXT && hb_set.HB_SET_MFILEEXT[ 0 ] )
            hb_itemPutC( pItem, hb_set.HB_SET_MFILEEXT );
         else
            hb_itemPutC( pItem, hb_memoDefaultFileExt( pData->bMemoType, pRDD->rddID ) );

         if( szNew )
         {
            hb_strncpy( pData->szMemoExt, szNew, HB_MAX_FILE_EXT );
            hb_xfree( szNew );
         }
         break;
      }
      case RDDI_MEMOBLOCKSIZE:
      {
         int iSize = hb_itemGetNI( pItem );

         if( pData->uiMemoBlockSize )
            hb_itemPutNI( pItem, pData->uiMemoBlockSize );
         else if( hb_set.HB_SET_MBLOCKSIZE > 0 && hb_set.HB_SET_MBLOCKSIZE <= 0xFFFF )
            hb_itemPutNI( pItem, hb_set.HB_SET_MBLOCKSIZE );
         else if( pData->bMemoType == DB_MEMO_DBT )
            hb_itemPutNI( pItem, DBT_DEFBLOCKSIZE );
         else if( pData->bMemoType == DB_MEMO_SMT )
            hb_itemPutNI( pItem, SMT_DEFBLOCKSIZE );
         else
            hb_itemPutNI( pItem, FPT_DEFBLOCKSIZE );

         if( iSize > 0 && iSize <= 0xFFFF )
            pData->uiMemoBlockSize = iSize;
         break;
      }
      case RDDI_MEMOTYPE:
      {
         int iType = hb_itemGetNI( pItem );

         hb_itemPutNI( pItem, pData->bMemoType ? pData->bMemoType : DB_MEMO_FPT );

         if( pRDD->rddID != s_uiRddIdBLOB )
         {
            switch( iType )
            {
               case DB_MEMO_DBT:
               case DB_MEMO_FPT:
               case DB_MEMO_SMT:
                  pData->bMemoType = ( BYTE ) iType;
            }
         }
         break;
      }

      case RDDI_MEMOVERSION:
      {
         int iType = hb_itemGetNI( pItem );

         hb_itemPutNI( pItem, pData->bMemoExtType ? pData->bMemoExtType : DB_MEMOVER_FLEX );
         switch( iType )
         {
            case DB_MEMOVER_STD:
            case DB_MEMOVER_SIX:
            case DB_MEMOVER_FLEX:
            case DB_MEMOVER_CLIP:
               pData->bMemoExtType = ( BYTE ) iType;
         }
         break;
      }

      case RDDI_MEMOGCTYPE:
         hb_itemPutNI( pItem, 0 );
         break;

      case RDDI_MEMOREADLOCK:
#if defined( HB_MEMO_SAFELOCK )
         hb_itemPutL( pItem, pRDD->rddID != s_uiRddIdBLOB );
#else
         hb_itemPutL( pItem, FALSE );
#endif
         break;
      case RDDI_MEMOREUSE:
         hb_itemPutL( pItem, TRUE );
         break;

      case RDDI_BLOB_SUPPORT:
         hb_itemPutL( pItem, pRDD->rddID == s_uiRddIdBLOB );
         break;

      default:
         return SUPER_RDDINFO( pRDD, uiIndex, ulConnect, pItem );
   }

   return SUCCESS;
}

HB_FUNC( DBFDBT ) {;}
HB_FUNC( DBFSMT ) {;}
HB_FUNC( DBFFPT ) {;}
HB_FUNC( DBFBLOB ) {;}

static void hb_dbffptRegisterRDD( USHORT * pusRddId )
{
   RDDFUNCS * pTable;
   USHORT * uiCount, uiRddId;

   uiCount = ( USHORT * ) hb_parptr( 1 );
   pTable = ( RDDFUNCS * ) hb_parptr( 2 );
   uiRddId = hb_parni( 4 );

   HB_TRACE(HB_TR_DEBUG, ("DBFFPT_GETFUNCTABLE(%p, %p)", uiCount, pTable));

   if( pTable )
   {
      ERRCODE errCode;

      if ( uiCount )
         * uiCount = RDDFUNCSCOUNT;

      errCode = hb_rddInherit( pTable, &fptTable, &fptSuper, "DBF" );
      if ( errCode == SUCCESS )
         *pusRddId = uiRddId;
      hb_retni( errCode );
   }
   else
      hb_retni( FAILURE );
}

HB_FUNC( DBFFPT_GETFUNCTABLE )
{
   HB_TRACE(HB_TR_DEBUG, ("DBFFPT_GETFUNCTABLE()"));

   hb_dbffptRegisterRDD( &s_uiRddIdFPT );
}

HB_FUNC( DBFBLOB_GETFUNCTABLE )
{
   HB_TRACE(HB_TR_DEBUG, ("DBFBLOB_GETFUNCTABLE()"));

   hb_dbffptRegisterRDD( &s_uiRddIdBLOB );
}

#define __PRG_SOURCE__ __FILE__

#ifdef HB_PCODE_VER
#  undef HB_PRG_PCODE_VER
#  define HB_PRG_PCODE_VER HB_PCODE_VER
#endif

HB_FUNC_EXTERN( _DBF );

static void hb_dbffptRddInit( void * cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   if( hb_rddRegister( "DBF",     RDT_FULL ) > 1 ||
       hb_rddRegister( "DBFFPT",  RDT_FULL ) > 1 ||
       hb_rddRegister( "DBFBLOB", RDT_FULL ) > 1 )
   {
      hb_errInternal( HB_EI_RDDINVALID, NULL, NULL, NULL );

      /* not executed, only to force DBF RDD linking */
      HB_FUNC_EXEC( _DBF );
   }
}

HB_INIT_SYMBOLS_BEGIN( dbffpt1__InitSymbols )
{ "DBFFPT",               {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( DBFFPT )},               NULL },
{ "DBFFPT_GETFUNCTABLE",  {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( DBFFPT_GETFUNCTABLE )},  NULL },
{ "DBFBLOB",              {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( DBFBLOB )},              NULL },
{ "DBFBLOB_GETFUNCTABLE", {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( DBFBLOB_GETFUNCTABLE )}, NULL }
HB_INIT_SYMBOLS_END( dbffpt1__InitSymbols )

HB_CALL_ON_STARTUP_BEGIN( _hb_dbffpt_rdd_init_ )
   hb_vmAtInit( hb_dbffptRddInit, NULL );
HB_CALL_ON_STARTUP_END( _hb_dbffpt_rdd_init_ )

#if defined(HB_PRAGMA_STARTUP)
#  pragma startup dbffpt1__InitSymbols
#  pragma startup _hb_dbffpt_rdd_init_
#elif defined(HB_MSC_STARTUP)
#  if _MSC_VER >= 1010
#     pragma data_seg( ".CRT$XIY" )
#     pragma comment( linker, "/Merge:.CRT=.data" )
#  else
#     pragma data_seg( "XIY" )
#  endif
   static HB_$INITSYM hb_vm_auto_dbffpt1__InitSymbols = dbffpt1__InitSymbols;
   static HB_$INITSYM hb_vm_auto_dbffpt_rdd_init = _hb_dbffpt_rdd_init_;
#  pragma data_seg()
#endif
