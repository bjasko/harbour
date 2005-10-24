/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * DELIMITED RDD module
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

#include "hbapi.h"
#include "hbinit.h"
#include "hbvm.h"
#include "hbapirdd.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbrdddel.h"
#include "rddsys.ch"

static RDDFUNCS delimSuper;
static RDDFUNCS delimTable = { hb_delimBof,
                               hb_delimEof,
                               hb_delimFound,
                               hb_delimGoBottom,
                               hb_delimGoTo,
                               hb_delimGoToId,
                               hb_delimGoTop,
                               hb_delimSeek,
                               hb_delimSkip,
                               hb_delimSkipFilter,
                               hb_delimSkipRaw,
                               hb_delimAddField,
                               hb_delimAppend,
                               hb_delimCreateFields,
                               hb_delimDeleteRec,
                               hb_delimDeleted,
                               hb_delimFieldCount,
                               hb_delimFieldDisplay,
                               hb_delimFieldInfo,
                               hb_delimFieldName,
                               hb_delimFlush,
                               hb_delimGetRec,
                               hb_delimGetValue,
                               hb_delimGetVarLen,
                               hb_delimGoCold,
                               hb_delimGoHot,
                               hb_delimPutRec,
                               hb_delimPutValue,
                               hb_delimRecall,
                               hb_delimRecCount,
                               hb_delimRecInfo,
                               hb_delimRecNo,
                               hb_delimRecId,
                               hb_delimSetFieldExtent,
                               hb_delimAlias,
                               hb_delimClose,
                               hb_delimCreate,
                               hb_delimInfo,
                               hb_delimNewArea,
                               hb_delimOpen,
                               hb_delimRelease,
                               hb_delimStructSize,
                               hb_delimSysName,
                               hb_delimEval,
                               hb_delimPack,
                               hb_delimPackRec,
                               hb_delimSort,
                               hb_delimTrans,
                               hb_delimTransRec,
                               hb_delimZap,
                               hb_delimChildEnd,
                               hb_delimChildStart,
                               hb_delimChildSync,
                               hb_delimSyncChildren,
                               hb_delimClearRel,
                               hb_delimForceRel,
                               hb_delimRelArea,
                               hb_delimRelEval,
                               hb_delimRelText,
                               hb_delimSetRel,
                               hb_delimOrderListAdd,
                               hb_delimOrderListClear,
                               hb_delimOrderListDelete,
                               hb_delimOrderListFocus,
                               hb_delimOrderListRebuild,
                               hb_delimOrderCondition,
                               hb_delimOrderCreate,
                               hb_delimOrderDestroy,
                               hb_delimOrderInfo,
                               hb_delimClearFilter,
                               hb_delimClearLocate,
                               hb_delimClearScope,
                               hb_delimCountScope,
                               hb_delimFilterText,
                               hb_delimScopeInfo,
                               hb_delimSetFilter,
                               hb_delimSetLocate,
                               hb_delimSetScope,
                               hb_delimSkipScope,
                               hb_delimLocate,
                               hb_delimCompile,
                               hb_delimError,
                               hb_delimEvalBlock,
                               hb_delimRawLock,
                               hb_delimLock,
                               hb_delimUnLock,
                               hb_delimCloseMemFile,
                               hb_delimCreateMemFile,
                               hb_delimGetValueFile,
                               hb_delimOpenMemFile,
                               hb_delimPutValueFile,
                               hb_delimReadDBHeader,
                               hb_delimWriteDBHeader,
                               hb_delimInit,
                               hb_delimExit,
                               hb_delimDrop,
                               hb_delimExists,
                               hb_delimRddInfo,
                               hb_delimWhoCares
                             };


/*
 * -- DELIM METHODS --
 */


HB_FUNC( DELIM ) { ; }

HB_FUNC( DELIM_GETFUNCTABLE )
{
   RDDFUNCS * pTable;
   USHORT * uiCount;

   uiCount = ( USHORT * ) hb_itemGetPtr( hb_param( 1, HB_IT_POINTER ) );
   * uiCount = RDDFUNCSCOUNT;
   pTable = ( RDDFUNCS * ) hb_itemGetPtr( hb_param( 2, HB_IT_POINTER ) );

   HB_TRACE(HB_TR_DEBUG, ("DELIM_GETFUNCTABLE(%i, %p)", uiCount, pTable));

   if( pTable )
      hb_retni( hb_rddInherit( pTable, &delimTable, &delimSuper, 0 ) );
   else
      hb_retni( FAILURE );
}


#define __PRG_SOURCE__ __FILE__

HB_FUNC( DELIM );
HB_FUNC( DELIM_GETFUNCTABLE );

#ifdef HB_PCODE_VER
   #undef HB_PRG_PCODE_VER
   #define HB_PRG_PCODE_VER HB_PCODE_VER
#endif

static void hb_delimRddInit( void * cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   if( hb_rddRegister( "DELIM", RDT_TRANSFER ) > 1 )
   {
      hb_errInternal( HB_EI_RDDINVALID, NULL, NULL, NULL );
   }
}

HB_INIT_SYMBOLS_BEGIN( delim1__InitSymbols )
{ "DELIM",              HB_FS_PUBLIC, {HB_FUNCNAME( DELIM )}, NULL },
{ "DELIM_GETFUNCTABLE", HB_FS_PUBLIC, {HB_FUNCNAME( DELIM_GETFUNCTABLE )}, NULL }
HB_INIT_SYMBOLS_END( delim1__InitSymbols )

HB_CALL_ON_STARTUP_BEGIN( _hb_delim_rdd_init_ )
   hb_vmAtInit( hb_delimRddInit, NULL );
HB_CALL_ON_STARTUP_END( _hb_delim_rdd_init_ )

#if defined(HB_PRAGMA_STARTUP)
   #pragma startup delim1__InitSymbols
   #pragma startup _hb_delim_rdd_init_
#elif defined(HB_MSC_STARTUP)
   #if _MSC_VER >= 1010
      #pragma data_seg( ".CRT$XIY" )
      #pragma comment( linker, "/Merge:.CRT=.data" )
   #else
      #pragma data_seg( "XIY" )
   #endif
   static HB_$INITSYM hb_vm_auto_delim1__InitSymbols = delim1__InitSymbols;
   static HB_$INITSYM hb_vm_auto_delim_rdd_init = _hb_delim_rdd_init_;
   #pragma data_seg()
#endif
