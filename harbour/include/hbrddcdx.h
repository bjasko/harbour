/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * DBFCDX RDD
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

#ifndef HB_RDDCDX_H_
#define HB_RDDCDX_H_

#include "hbapirdd.h"

#if defined(HB_EXTERN_C)
extern "C" {
#endif


/* DBFCDX errors */

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



/* DBFCDX default extensions */
#define CDX_MEMOEXT                               ".fpt"
#define CDX_INDEXEXT                              ".cdx"



/* FPT's and CDX's */

#define FPT_DEFBLOCKSIZE                             64
#define SIZEOFMEMOFREEBLOCK                           6
#define MAXFREEBLOCKS                                82
#define CDX_MAXKEY                                  240
#define CDX_MAXTAGNAMELEN                            10
#define CDX_PAGELEN                                 512
#define CDX_RIGHTTYPE                                 0
#define CDX_ROOTTYPE                                  1
#define CDX_LEAFTYPE                                  2
#define CDX_LEAFFREESPACE                           488



struct _CDXAREA;

typedef struct _MEMOHEADER
{
   ULONG ulNextBlock;                 /* Next memo entry */
   ULONG ulBlockSize;                 /* Size of block */
} MEMOHEADER;

typedef MEMOHEADER * LPMEMOHEADER;



typedef struct _MEMOBLOCK
{
   ULONG ulType;                      /* 0 = binary, 1 = text */
   ULONG ulSize;                      /* length of data */
} MEMOBLOCK;

typedef MEMOBLOCK * LPMEMOBLOCK;



typedef struct _MEMOFREEBLOCK
{
   USHORT uiBlocks;                   /* Number of blocks */
   ULONG ulBlock;                     /* Block number */
} MEMOFREEBLOCK;

typedef MEMOFREEBLOCK * LPMEMOFREEBLOCK;



typedef struct _MEMOROOT
{
   ULONG ulNextBlock;                 /* Next block in the list */
   ULONG ulBlockSize;                 /* Size of block */
   BYTE szSignature[ 8 ];             /* Signature */
   BYTE fChanged;                     /* TRUE if root block is changed */
   USHORT uiListLen;                  /* Length of list */
   BYTE pFreeList[ 492 ];             /* Array of free memo blocks (82 MEMOFREEBLOCK's) */
} MEMOROOT;

typedef MEMOROOT * LPMEMOROOT;



/* CDX's */

struct _CDXINDEX;    /* forward declaration */
typedef struct _CDXTAG
{
   char * szName;        /* Name of tag */
   PHB_ITEM pKeyItem;    /* item with a macro pcode for a tag key expression */
   PHB_ITEM pForItem;    /* item with a macro pcode for a tag for expression */
   char *     KeyExpr;   /* a tag key expression as text */
   char *     ForExpr;   /* a tag for expression as text */
   USHORT uiType;        /* a type of key expression value */
   USHORT uiLen;         /* length of the key expression value */
   struct _CDXINDEX * pIndex;    /* a parent index info */
   // review this ...
   struct    _CDXTAG * pNext;
   BOOL       AscendKey;        /* ascending/descending order flag */
   BOOL       UniqueKey;        /* unique order flag */
   BOOL       TagChanged;
   BOOL       TagBOF;
   BOOL       TagEOF;
   //BYTE       KeyType;
   BYTE       OptFlags;
   LONG       TagBlock;        /* a page offset where a tag header is stored */
   LONG       RootBlock;       /* a page offset with the root of keys tree */
   //USHORT     KeyLength;
   USHORT     MaxKeys;
   LPKEYINFO  CurKeyInfo;    /* current value of key expression */
   LPPAGEINFO RootPage;
   LPKEYINFO  HotKey;        /* value of hot key expression */
   PHB_ITEM   topScope;
   LPKEYINFO  topScopeKey;
   PHB_ITEM   bottomScope;
   LPKEYINFO  bottomScopeKey;
} CDXTAG;
typedef CDXTAG * LPCDXTAG;

typedef struct _CDXINDEX
{
   char *    szFileName;                 /* Name of index file */
   FHANDLE   hFile;                     /* Index file handle */
   struct _CDXAREA * pArea;           /* Parent WorkArea */
   LPCDXTAG  pCompound;
   LONG      NextAvail;
   // review this...
   LPCDXTAG  TagList;
   struct   _CDXINDEX * pNext;   /* The next index in the list */
   /* USHORT    uiTag;      */        /* current tag focus          */
   BOOL fShared;                 /* Shared file */
   BOOL fReadonly;               /* Read only file */
   int       lockWrite;
   int       lockRead;
   int       changesWritten;
   ULONG     ulVersion;
} CDXINDEX;
typedef CDXINDEX * LPCDXINDEX;

#if (__BORLANDC__ > 1040) /* Use this only above Borland C++ 3.1 */
   #pragma option -a1 /* byte alignment */
#elif defined(__GNUC__)
   #pragma pack(1)
#elif defined(__WATCOMC__)
   #pragma pack(push, 1);
#elif defined(__cplusplus)
   #pragma pack(1)
#endif

/* ----
typedef struct _CDXHEADER
{ ...
   LONG   Root;
   LONG   FreePtr;
   LONG   ChgFlag;
   USHORT Key_Lgth;
   BYTE   IndexOpts;
   BYTE   IndexSig;
   BYTE   Reserve3[ 486 ];
   USHORT AscDesc;
   USHORT Reserve4;
   USHORT ForExpLen;
   USHORT Reserve5;
   USHORT KeyExpLen;
   BYTE   KeyPool[ CDX_BLOCK_SIZE ];
} CDXHEADER;

typedef CDXHEADER * LPCDXHEADER;
----- */

#define CDX_TYPE_UNIQUE        1       /* unique index */
#define CDX_TYPE_FORFILTER     0x08    /* for expression present */
#define CDX_TYPE_BITVECTOR     0x10    /* SoftC? */
#define CDX_TYPE_COMPACT       0x20    /* FoxPro */
#define CDX_TYPE_COMPOUND      0x40    /* FoxPro */
#define CDX_TYPE_STRUCTURE     0x80    /* FoxPro */

typedef struct _CDXTAGHEADER
{
   LONG lRoot;                /* offset of the root node */
   LONG lFreeList;            /* offset of list of free   pages or -1 */
   LONG lChgFlag; //lLength;  /* Version number ? */
   USHORT uiKeySize;          /* key length */
   BYTE bType;                /* index options see CDX_TYPE_* */
   BYTE bSignature;           /* index signature */
   BYTE bReserved1[ 486 ];
   USHORT iDescending;        /* 0 = ascending  1 = descending */
   USHORT iFilterPos;         /* offset of filter expression */
   USHORT iFilterLen;         /* length of filter expression */
   USHORT iExprPos;           /* offset of key expression */
   USHORT iExprLen;           /* length of key expression */
   BYTE   KeyPool[ CDX_PAGELEN ];
} CDXTAGHEADER;
typedef CDXTAGHEADER * LPCDXTAGHEADER;

#define CDX_NODE_BRANCH    0
#define CDX_NODE_ROOT      1
#define CDX_NODE_LEAF      2

typedef struct _CDXLEAFHEADER
{
   USHORT uiNodeType;        /* node type see CDX_NODE_* */
   USHORT uiKeyCount;        /* number of keys */
   LONG lLeftNode;           /* offset of left node or -1 */
   LONG lRightNode;          /* offset of right node or -1 */
   USHORT uiFreeSpace;       /* free space available in a page */
   ULONG ulRecNumMask;       /* record number mask */
   BYTE bDupByteMask;        /* duplicate bytes count mask */
   BYTE bTrailByteMask;      /* trailing bytes count mask */
   BYTE bRecNumLen;          /* number of bits for record number */
   BYTE bDupCntLen;          /* number of bits for duplicate count */
   BYTE bTrailCntLen;        /* number of bits for trailing count */
   BYTE bInfo;               /* total number of bytes for recnn/dup/trail info */
   BYTE bData[ CDX_LEAFFREESPACE ];
} CDXLEAFHEADER;
typedef CDXLEAFHEADER * LPCDXLEAFHEADER;
#if (__BORLANDC__ > 1040) /* Use this only above Borland C++ 3.1 */
   #pragma option -a /* default alignment */
#elif defined(__GNUC__)
   #pragma pack()
#elif defined(__WATCOMC__)
   #pragma pack(pop);
#elif defined(__cplusplus)
   #pragma pack()
#endif


/*
 *  DBF WORKAREA
 *  ------------
 *  The Workarea Structure of DBFCDX RDD
 *
 */

typedef struct _CDXAREA
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

   /*
   *  CDX's additions to the workarea structure
   *
   *  Warning: The above section MUST match WORKAREA exactly!  Any
   *  additions to the structure MUST be added below, as in this
   *  example.
   */

   USHORT uiMemoBlockSize;       /* Size of memo block */
   LPMEMOROOT pMemoRoot;         /* Array of free memo blocks */
   //LPCDXTAG * lpIndexes;         /* Pointer to indexes array */
   LPCDXINDEX lpIndexes;         /* Pointer to indexes array */
   USHORT    uiTag;              /* current tag focus          */

} CDXAREA;

typedef CDXAREA * LPCDXAREA;

#ifndef CDXAREAP
#define CDXAREAP LPCDXAREA
#endif


/*
 * -- DBFCDX METHODS --
 */

#define SUPERTABLE                         ( &cdxSuper )

#define hb_cdxBof                                  NULL
#define hb_cdxEof                                  NULL
#define hb_cdxFound                                NULL
extern ERRCODE hb_cdxGoBottom( CDXAREAP pArea );
//#define hb_cdxGoTo                                 NULL
extern ERRCODE hb_cdxGoTo( CDXAREAP pArea, ULONG ulRecNo );
#define hb_cdxGoToId                               NULL
extern ERRCODE hb_cdxGoTop( CDXAREAP pArea );
extern ERRCODE hb_cdxSeek( CDXAREAP pArea, BOOL bSoftSeek, PHB_ITEM pKey, BOOL bFindLast );
#define hb_cdxSkip                                 NULL
#define hb_cdxSkipFilter                           NULL
extern ERRCODE hb_cdxSkipRaw( CDXAREAP pArea, LONG lToSkip );
#define hb_cdxAddField                             NULL
#define hb_cdxAppend                               NULL
#define hb_cdxCreateFields                         NULL
#define hb_cdxDeleteRec                            NULL
#define hb_cdxDeleted                              NULL
#define hb_cdxFieldCount                           NULL
#define hb_cdxFieldDisplay                         NULL
#define hb_cdxFieldInfo                            NULL
#define hb_cdxFieldName                            NULL
#define hb_cdxFlush                                NULL
#define hb_cdxGetRec                               NULL
extern ERRCODE hb_cdxGetValue( CDXAREAP pArea, USHORT uiIndex, PHB_ITEM pItem );
extern ERRCODE hb_cdxGetVarLen( CDXAREAP pArea, USHORT uiIndex, ULONG * pLength );
//#define hb_cdxGoCold                               NULL
extern ERRCODE hb_cdxGoCold( CDXAREAP pArea );
//#define hb_cdxGoHot                                NULL
extern ERRCODE hb_cdxGoHot( CDXAREAP pArea );
#define hb_cdxPutRec                               NULL
extern ERRCODE hb_cdxPutValue( CDXAREAP pArea, USHORT uiIndex, PHB_ITEM pItem );
#define hb_cdxRecall                               NULL
#define hb_cdxRecCount                             NULL
#define hb_cdxRecInfo                              NULL
#define hb_cdxRecNo                                NULL
#define hb_cdxSetFieldExtent                       NULL
#define hb_cdxAlias                                NULL
extern ERRCODE hb_cdxClose( CDXAREAP pArea );
#define hb_cdxCreate                               NULL
extern ERRCODE hb_cdxInfo( CDXAREAP pArea, USHORT uiIndex, PHB_ITEM pItem );
#define hb_cdxNewArea                              NULL
extern ERRCODE hb_cdxOpen( CDXAREAP pArea, LPDBOPENINFO pOpenInfo );
#define hb_cdxRelease                              NULL
extern ERRCODE hb_cdxStructSize( CDXAREAP pArea, USHORT * uiSize );
extern ERRCODE hb_cdxSysName( CDXAREAP pArea, BYTE * pBuffer );
#define hb_cdxEval                                 NULL
extern ERRCODE hb_cdxPack ( CDXAREAP pArea );
#define hb_cdxPackRec                              NULL
#define hb_cdxSort                                 NULL
#define hb_cdxTrans                                NULL
#define hb_cdxTransRec                             NULL
/* #define hb_cdxZap                                  NULL */
extern ERRCODE hb_cdxZap ( CDXAREAP pArea );
#define hb_cdxChildEnd                             NULL
#define hb_cdxChildStart                           NULL
#define hb_cdxChildSync                            NULL
#define hb_cdxSyncChildren                         NULL
#define hb_cdxClearRel                             NULL
#define hb_cdxForceRel                             NULL
#define hb_cdxRelArea                              NULL
#define hb_cdxRelEval                              NULL
#define hb_cdxRelText                              NULL
#define hb_cdxSetRel                               NULL
extern ERRCODE hb_cdxOrderListAdd( CDXAREAP pArea, LPDBORDERINFO pOrderInfo );
extern ERRCODE hb_cdxOrderListClear( CDXAREAP pArea );
#define hb_cdxOrderListDelete                      NULL
extern ERRCODE hb_cdxOrderListFocus( CDXAREAP pArea, LPDBORDERINFO pOrderInfo );
#define hb_cdxOrderListRebuild                     NULL
#define hb_cdxOrderCondition                       NULL
extern ERRCODE hb_cdxOrderCreate( CDXAREAP pArea, LPDBORDERCREATEINFO pOrderInfo );
#define hb_cdxOrderDestroy                         NULL
extern ERRCODE hb_cdxOrderInfo( CDXAREAP pArea, USHORT uiIndex, LPDBORDERINFO pOrderInfo );
#define hb_cdxClearFilter                          NULL
#define hb_cdxClearLocate                          NULL
/* #define hb_cdxClearScope                           NULL */
static ERRCODE hb_cdxClearScope( CDXAREAP pArea );
#define hb_cdxCountScope                           NULL
#define hb_cdxFilterText                           NULL
/* #define hb_cdxScopeInfo                            NULL */
static ERRCODE hb_cdxScopeInfo( CDXAREAP pArea, USHORT nScope, PHB_ITEM pItem );
#define hb_cdxSetFilter                            NULL
#define hb_cdxSetLocate                            NULL
/* #define hb_cdxSetScope                             NULL */
static ERRCODE hb_cdxSetScope( CDXAREAP pArea, LPDBORDSCOPEINFO sInfo );
#define hb_cdxSkipScope                            NULL
#define hb_cdxCompile                              NULL
#define hb_cdxError                                NULL
#define hb_cdxEvalBlock                            NULL
#define hb_cdxRawLock                              NULL
#define hb_cdxLock                                 NULL
#define hb_cdxUnLock                               NULL
#define hb_cdxCloseMemFile                         NULL
extern ERRCODE hb_cdxCreateMemFile( CDXAREAP pArea, LPDBOPENINFO pCreateInfo );
#define hb_cdxGetValueFile                         NULL
extern ERRCODE hb_cdxOpenMemFile( CDXAREAP pArea, LPDBOPENINFO pOpenInfo );
#define hb_cdxPutValueFile                         NULL
extern ERRCODE hb_cdxReadDBHeader( CDXAREAP pArea );
extern ERRCODE hb_cdxWriteDBHeader( CDXAREAP pArea );
#define hb_cdxWhoCares                             NULL

#if defined(HB_EXTERN_C)
}
#endif

#endif /* HB_RDDCDX_H_ */
