/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * DBFNTX RDD
 *
 * Copyright 2000 Alexander Kresin <alex@belacy.belgorod.su>
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

#ifndef HB_RDDNTX_H_
#define HB_RDDNTX_H_

#include "hbapirdd.h"
#include "hbapicdp.h"

#if defined(HB_EXTERN_C)
extern "C" {
#endif


/* DBFNTX errors */

#define EDBF_OPEN_DBF                              1001
#define EDBF_CREATE_DBF                            1004
#define EDBF_READ                                  1010
#define EDBF_WRITE                                 1011
#define EDBF_CORRUPT                               1012
#define EDBF_DATATYPE                              1020
#define EDBF_DATAWIDTH                             1021
#define EDBF_UNLOCKED                              1022
#define EDBF_SHARED                                1023
#define EDBF_APPENDLOCK                            1024
#define EDBF_READONLY                              1025
#define EDBF_INVALIDKEY                            1026



/* DBFNTX default extensions */
#define NTX_MEMOEXT                               ".dbt"
#define NTX_INDEXEXT                              ".ntx"

/* DBFNTX constants declarations */

#define TOP_RECORD                                                      1
#define BTTM_RECORD                                                     2
#define PREV_RECORD                                                     3
#define NEXT_RECORD                                                     4

#define NTX_MAX_KEY          256      /* Max len of key */
#define NTXBLOCKSIZE         1024     /* Size of block in NTX file */
#define NTX_LOCK_OFFSET      1000000000
#define NTX_PAGES_PER_TAG    32

/* forward declarations
 */
struct _RDDFUNCS;
struct _NTXAREA;
struct _TAGINFO;
struct _NTXINDEX;

typedef struct _KEYINFO
{
   // PHB_ITEM pItem;
   LONG     Tag;
   LONG     Xtra;
   char     key[ 1 ]; /* value of key */
} KEYINFO;

typedef KEYINFO * LPKEYINFO;

typedef struct _TREE_STACK
{
   LONG     page;
   SHORT    ikey;
}  TREE_STACK;

typedef TREE_STACK * LPTREESTACK;

typedef struct HB_PAGEINFO_STRU
{
   LONG      Page;
   BOOL      Changed;
   BOOL      lBusy;
   USHORT    uiKeys;
   SHORT     CurKey;
   char*     buffer;
} HB_PAGEINFO;

typedef HB_PAGEINFO * LPPAGEINFO;


typedef struct _TAGINFO
{
   char *     TagName;
   LONG       TagRoot;
   char *     KeyExpr;
   char *     ForExpr;
   PHB_ITEM   pKeyItem;
   PHB_ITEM   pForItem;
   PHB_ITEM   topScope;
   PHB_ITEM   bottomScope;
   BOOL       AscendKey;
   BOOL       UniqueKey;
   BOOL       Custom;
   BOOL       TagChanged;
   BOOL       TagBOF;
   BOOL       TagEOF;
   BOOL       NewRoot;
   BOOL       Memory;
   BYTE       KeyType;
   BYTE       OptFlags;
   LONG       TagBlock;
   LONG       RootBlock;
   USHORT     KeyLength;
   USHORT     KeyDec;
   USHORT     MaxKeys;
   LPTREESTACK stack;
   USHORT     stackDepth;
   USHORT     stackLevel;
   ULONG      keyCount;
   ULONG      ulPagesDepth;
   ULONG      ulPages;
   ULONG      ulPagesStart;
   LPKEYINFO  CurKeyInfo;
   LPPAGEINFO pages;
   BOOL       InIndex;
   char*      buffer;
   struct    _NTXINDEX * Owner;
   struct    _TAGINFO * pNext;
} TAGINFO;

typedef TAGINFO * LPTAGINFO;

typedef struct _NTXINDEX
{
   char *    IndexName;
   BOOL      Locked;
   LONG      NextAvail;
   struct   _NTXAREA * Owner;
   FHANDLE   DiskFile;
   LPTAGINFO CompoundTag;
   struct   _NTXINDEX * pNext;   /* The next index in the list */
} NTXINDEX;

typedef NTXINDEX * LPNTXINDEX;

/* Internal structures used by saving file */

typedef struct _NTXHEADER    /* Header of NTX file */
{
   USHORT   type;
   USHORT   version;
   ULONG    root;
   ULONG    next_page;
   USHORT   item_size;
   USHORT   key_size;
   USHORT   key_dec;
   USHORT   max_item;
   USHORT   half_page;
   char     key_expr[ NTX_MAX_KEY ];
   char     unique;
   char     unknown1;
   char     descend;
   char     unknown2;
   char     for_expr[ NTX_MAX_KEY ];
   char     custom;
} NTXHEADER;

typedef NTXHEADER * LPNTXHEADER;

typedef struct _NTXBUFFER    /* Header of each block in NTX file (only block
                                with header has other format */
{
   USHORT   item_count;
   USHORT   item_offset[ 1 ];
} NTXBUFFER;

typedef NTXBUFFER * LPNTXBUFFER;

typedef struct _NTXITEM      /* each item in NTX block has following format */
{
   ULONG    page;     /* subpage (each key in subpage has < value like this key */
   ULONG    rec_no;   /* RecNo of record with this key */
   char     key[ 1 ]; /* value of key */
} NTXITEM;

typedef NTXITEM * LPNTXITEM;

struct _NTXAREA;

/*
 *  DBF WORKAREA
 *  ------------
 *  The Workarea Structure of DBFNTX RDD
 *
 */

typedef struct _NTXAREA
{
   struct _RDDFUNCS * lprfsHost; /* Virtual method table for this workarea */
   USHORT uiArea;                /* The number assigned to this workarea */
   void * atomAlias;             /* Pointer to the alias symbol for this workarea */
   USHORT uiFieldExtent;         /* Total number of fields allocated */
   USHORT uiFieldCount;          /* Total number of fields used */
   LPFIELD lpFields;             /* Pointer to an array of fields */
   void * lpFieldExtents;        /* Void ptr for additional field properties */
   PHB_ITEM valResult;           /* All purpose result holder */
   BOOL fTop;                    /* TRUE if "top" */
   BOOL fBottom;                 /* TRUE if "bottom" */
   BOOL fBof;                    /* TRUE if "bof" */
   BOOL fEof;                    /* TRUE if "eof" */
   BOOL fFound;                  /* TRUE if "found" */
   DBSCOPEINFO dbsi;             /* Info regarding last LOCATE */
   DBFILTERINFO dbfi;            /* Filter in effect */
   LPDBORDERCONDINFO lpdbOrdCondInfo;
   LPDBRELINFO lpdbRelations;    /* Parent/Child relationships used */
   USHORT uiParents;             /* Number of parents for this area */
   USHORT heap;
   USHORT heapSize;
   USHORT rddID;
   USHORT uiMaxFieldNameLength;
   /*
   *  DBFS's additions to the workarea structure
   *
   *  Warning: The above section MUST match WORKAREA exactly!  Any
   *  additions to the structure MUST be added below, as in this
   *  example.
   */

   FHANDLE hDataFile;            /* Data file handle */
   FHANDLE hMemoFile;            /* Memo file handle */
   USHORT uiHeaderLen;           /* Size of header */
   USHORT uiRecordLen;           /* Size of record */
   ULONG ulRecCount;             /* Total records */
   char * szDataFileName;        /* Name of data file */
   char * szMemoFileName;        /* Name of memo file */
   BOOL fHasMemo;                /* WorkArea with Memo fields */
   BOOL fHasTags;                /* WorkArea with MDX or CDX index */
   BOOL fShared;                 /* Shared file */
   BOOL fReadonly;               /* Read only file */
   USHORT * pFieldOffset;        /* Pointer to field offset array */
   BYTE * pRecord;               /* Buffer of record data */
   BOOL fValidBuffer;            /* State of buffer */
   BOOL fPositioned;             /* Positioned record */
   ULONG ulRecNo;                /* Current record */
   BOOL fRecordChanged;          /* Record changed */
   BOOL fAppend;                 /* TRUE if new record is added */
   BOOL fDeleted;                /* TRUE if record is deleted */
   BOOL fUpdateHeader;           /* Update header of file */
   BOOL fFLocked;                /* TRUE if file is locked */
   LPDBRELINFO lpdbPendingRel;   /* Pointer to parent rel struct */
   BYTE bYear;                   /* Last update */
   BYTE bMonth;
   BYTE bDay;
   ULONG * pLocksPos;            /* List of records locked */
   ULONG ulNumLocksPos;          /* Number of records locked */
   PHB_CODEPAGE cdPage;          /* Area's codepage pointer  */

   /*
   *  NTX's additions to the workarea structure
   *
   *  Warning: The above section MUST match WORKAREA exactly!  Any
   *  additions to the structure MUST be added below, as in this
   *  example.
   */

   LPTAGINFO lpCurTag;         /* Pointer to current order */
   LPTAGINFO lpNtxTag;         /* Pointer to tags list */
   BOOL fNtxAppend;            /* TRUE if new record is added */

} NTXAREA;

typedef NTXAREA * LPNTXAREA;

#ifndef NTXAREAP
#define NTXAREAP LPNTXAREA
#endif


/*
 * -- DBFNTX METHODS --
 */

#define SUPERTABLE                         ( &ntxSuper )

#define ntxBof                   NULL
#define ntxEof                   NULL
#define ntxFound                 NULL
static ERRCODE ntxGoBottom( NTXAREAP pArea );
static ERRCODE ntxGoTo( NTXAREAP pArea, ULONG ulRecNo );
#define ntxGoToId                NULL
static ERRCODE ntxGoTop( NTXAREAP pArea );
static ERRCODE ntxSeek( NTXAREAP pArea, BOOL bSoftSeek, PHB_ITEM pKey, BOOL bFindLast );
#define ntxSkip                  NULL
#define ntxSkipFilter            NULL
static ERRCODE ntxSkipRaw( NTXAREAP pArea, LONG lToSkip );
#define ntxAddField              NULL
// static ERRCODE ntxAppend( NTXAREAP pArea, BOOL bUnLockAll );
#define ntxAppend                NULL
#define ntxCreateFields          NULL
#define ntxDeleteRec             NULL
#define ntxDeleted               NULL
#define ntxFieldCount            NULL
#define ntxFieldDisplay          NULL
#define ntxFieldInfo             NULL
#define ntxFieldName             NULL
#define ntxFlush                 NULL
#define ntxGetRec                NULL
#define ntxGetValue              NULL
#define ntxGetVarLen             NULL
static ERRCODE ntxGoCold( NTXAREAP pArea );
static ERRCODE ntxGoHot( NTXAREAP pArea );
#define ntxPutRec                NULL
#define ntxPutValue              NULL
#define ntxRecall                NULL
#define ntxRecCount              NULL
#define ntxRecInfo               NULL
#define ntxRecNo                 NULL
#define ntxSetFieldsExtent       NULL
#define ntxAlias                 NULL
static ERRCODE ntxClose( NTXAREAP pArea );
         /* Close workarea - at first we mus close all indexes and than close
            workarea */
#define ntxCreate                NULL
#define ntxInfo                  NULL
#define ntxNewArea               NULL
#define ntxOpen                  NULL
#define ntxRelease               NULL
static ERRCODE ntxStructSize( NTXAREAP pArea, USHORT * uiSize );
static ERRCODE ntxSysName( NTXAREAP pArea, BYTE * pBuffer );
#define ntxEval                  NULL
static ERRCODE ntxPack( NTXAREAP pArea );
#define ntPackRec                NULL
#define ntxSort                  NULL
#define ntxTrans                 NULL
#define ntxTransRec              NULL
static ERRCODE ntxZap( NTXAREAP pArea );
#define ntxchildEnd              NULL
#define ntxchildStart            NULL
#define ntxchildSync             NULL
#define ntxsyncChildren          NULL
#define ntxclearRel              NULL
#define ntxforceRel              NULL
#define ntxrelArea               NULL
#define ntxrelEval               NULL
#define ntxrelText               NULL
#define ntxsetRel                NULL
static ERRCODE ntxOrderListAdd( NTXAREAP pArea, LPDBORDERINFO pOrderInfo );
         /* Open next index */
static ERRCODE ntxOrderListClear( NTXAREAP pArea );
         /* Close all indexes */
#define ntxOrderListDelete       NULL
static ERRCODE ntxOrderListFocus( NTXAREAP pArea, LPDBORDERINFO pOrderInfo );
static ERRCODE ntxOrderListRebuild( NTXAREAP pArea );
static ERRCODE ntxOrderCondition( NTXAREAP area, LPDBORDERCONDINFO pOrdCondInfo );
static ERRCODE ntxOrderCreate( NTXAREAP pArea, LPDBORDERCREATEINFO pOrderInfo );
         /* Create new Index */
#define ntxOrderDestroy          NULL
static ERRCODE ntxOrderInfo( NTXAREAP pArea, USHORT uiIndex, LPDBORDERINFO pInfo );
         /* Some information about index */
#define ntxClearFilter           NULL
#define ntxClearLocate           NULL
static ERRCODE ntxClearScope( NTXAREAP pArea );
#define ntxCountScope            NULL
#define ntxFilterText            NULL
static ERRCODE ntxScopeInfo( NTXAREAP pArea, USHORT nScope, PHB_ITEM pItem );
#define ntxSetFilter             NULL
#define ntxSetLocate             NULL
static ERRCODE ntxSetScope( NTXAREAP pArea, LPDBORDSCOPEINFO sInfo );
#define ntxSkipScope             NULL
#define ntxCompile               NULL
#define ntxError                 NULL
#define ntxEvalBlock             NULL
#define ntxRawLock               NULL
#define ntxLock                  NULL
#define ntxUnLock                NULL
#define ntxCloseMemFile          NULL
#define ntxCreateMemFile         NULL
#define ntxGetValueFile          NULL
#define ntxOpenMemFile           NULL
#define ntxPutValueFile          NULL
#define ntxReadDBHeader          NULL
#define ntxWriteDBHeader         NULL
#define ntxWhoCares              NULL

#if defined(HB_EXTERN_C)
}
#endif

#endif /* HB_RDDNTX_H_ */
