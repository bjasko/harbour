/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * DBFCDX RDD
 *
 * Copyright 1999 Bruno Cantero <bruno@issnet.net>
 * Copyright 2000-2002 Horacio Roldan <harbour_ar@yahoo.com.ar> (portions)
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
#define CDX_MAX_TAG_NAME_LEN            10

#define CDX_MAX_REC_NUM                 0x7FFFFFFFL
#define CDX_IGNORE_REC_NUM              -1
#define PAGE_ROOT                       1
#define PAGE_NODE                       2
#define PAGE_LEAF                       3
#define TOP_RECORD                      1
#define BTTM_RECORD                     2
#define PREV_RECORD                     3
#define NEXT_RECORD                     4
*/

#define CDX_INTERNAL_SPACE              500
#define CDX_EXTERNAL_SPACE              488

#define SORT_CHUNK_LIMIT                16384
#define SORT_ACTIVE_LIST                0
#define SORT_END_OF_KEY                 1
#define SORT_END_OF_WORD                2
#define SORT_STACK_OF_CHAR              3
#define SORT_NOT_KEY                    0x10

#if (__BORLANDC__ > 1040) /* Use this only above Borland C++ 3.1 */
   #pragma option -a1 /* byte alignment */
#elif defined(__GNUC__)
   #pragma pack(1)
#elif defined(__WATCOMC__)
   #pragma pack(push, 1);
#elif defined(__cplusplus)
   #pragma pack(1)
#endif

typedef struct
{
   USHORT FreeSpace;
   LONG   RecNumMask;
   BYTE   DupCntMask;
   BYTE   TrlCntMask;
   BYTE   RecNumBits;
   BYTE   DupCntBits;
   BYTE   TrlCntBits;
   BYTE   ShortBytes;
   BYTE   ExtData[ CDX_EXTERNAL_SPACE ];
} CDXEXTERNAL;

typedef struct
{
   BYTE   IntData[ CDX_INTERNAL_SPACE ];
} CDXINTERNAL;

typedef struct _CDXDATA
{
   USHORT Node_Atr;
   USHORT Entry_Ct;
   LONG   Left_Ptr;
   LONG   Rght_Ptr;
   union
   {
      CDXEXTERNAL External;
      CDXINTERNAL Internal;
   } cdxu;
} CDXDATA;
typedef CDXDATA * LPCDXDATA;

typedef struct _SORTSWAPPAGE
{
   char        mark[2];
   USHORT      pageNum;
   ULONG       pageLen;
   ULONG       keyCount;
   USHORT      nCurPos;
   USHORT      nBufLeft;
   ULONG       nFileOffset;
   ULONG       nBytesLeft;
   ULONG       keysLeft;
   ULONG       tmpRecNo;  /* to speed up access */
   BYTE        tmpKeyLen;
   char *      tmpKeyVal;
   char        page[ 512 ];
} SORTSWAPPAGE;
typedef SORTSWAPPAGE * LPSORTSWAPPAGE;

typedef struct _SORTSWAPITEM
{
   ULONG    recno;
   BYTE     keyLen;
   char     key[ 1 ];
} SORTSWAPITEM;
typedef SORTSWAPITEM * LPSORTSWAPITEM;

#if (__BORLANDC__ > 1040) /* Use this only above Borland C++ 3.1 */
   #pragma option -a /* default alignment */
#elif defined(__GNUC__)
   #pragma pack()
#elif defined(__WATCOMC__)
   #pragma pack(pop);
#elif defined(__cplusplus)
   #pragma pack()
#endif

/*SORT stuff*/
typedef struct
{
   BYTE   Character;
   BYTE   NUse;
   USHORT WordArray;
   USHORT Fill02;
   USHORT LevelLink;
} SORT_A;

typedef struct
{
   BYTE Fill03[ 4 ];
   BYTE ChrStack[ 4 ];
} SORT_B;

typedef struct
{
   LONG Fill04;
   LONG ChrFill;
} SORT_C;

#define SORT_GET_NUSE( w )              (( w ) & 0x07 )
#define SORT_SET_NUSE( w, n )           (( w ) = ( ( w ) & 0xF8 ) | ( n ) )
#define SORT_GET_STACK_LEN( w )         (( w ) >> 6 )
#define SORT_SET_STACK_LEN( w, n )      (( w ) = ( ( w ) & 0x3F ) | ( ( n ) << 6 ) )

typedef struct
{
   union
   {
      SORT_A A;
      SORT_B B;
      SORT_C C;
   } sortu;
} SORTDATA;

typedef SORTDATA * LPSORTDATA;

typedef struct _CDXKEYINFO
{
   BYTE *   Value;
   USHORT   length;
   USHORT   realLength;
   BOOL     fString;
   ULONG    Tag;
   ULONG    Xtra;
   struct _CDXKEYINFO * pNext;
} CDXKEYINFO;
typedef CDXKEYINFO * LPCDXKEYINFO;

typedef struct
{
   LONG       WordCount;
   LONG       RootLink;
   LONG       LevelPtr;
   LONG       PriorPtr;
   LONG       KeyTot;
   LONG       KeyCnt;
   LONG       LastTag;
   LONG *     ChunkList;
   BYTE *     SortBuffer;
   USHORT     SortChunk;
   USHORT     NodeLimit;
   USHORT     NodeMask;
   USHORT     NodeShift;
   USHORT     ChunkSize;
   USHORT     ChunkLimit;
   USHORT     ChunkCur;
   USHORT     NodeCur;
   USHORT     KeySize;
   USHORT     WCur;
   BOOL       Unique;
   BOOL       Ascend;
   BOOL       Closing;

   USHORT     nSwapPages;
   LPSORTSWAPPAGE pSwapPage;
   FHANDLE    hTempFile;
   char *     szTempFileName;
   LONG       TotalWordCount;

   BYTE       WPch[ 256 ];
   SORTDATA * WAdr;
   struct _CDXTAG * CurTag;
   LPCDXDATA  NodeList[ 32 ];
   LPCDXKEYINFO  KeyWork;
   LPCDXKEYINFO  LastKey;
} SORTINFO;

typedef SORTINFO * LPSORTINFO;
