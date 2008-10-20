/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Clipper Tools like window system
 *
 * Copyright 2006 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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

/* NOTE: User programs should never call this layer directly! */

/* This definition has to be placed before #include "hbapigt.h" */
#define HB_GT_NAME      CTW

#include "hbgtcore.h"
#include "hbinit.h"
#include "hbapiitm.h"

#include "ctwin.h"

static int           s_GtId;
static HB_GT_FUNCS   SuperTable;
#define HB_GTSUPER   (&SuperTable)
#define HB_GTID_PTR  (&s_GtId)

#define HB_GTCTW_GET(p) ( ( PHB_GTCTW ) HB_GTLOCAL( p ) )

#define HB_CTWIN_ALLOC        16
#define HB_CTWIN_MINROWS      1
#define HB_CTWIN_MINCOLS      1
#define HB_CTWIN_MAXROWS      255
#define HB_CTWIN_MAXCOLS      255

typedef struct
{
   int iHandle;

   BOOL fHidden;

   int iShadowAttr;
   int iCursorStyle;

   int iRow;
   int iCol;

   int iTopMargin;
   int iLeftMargin;
   int iBottomMargin;
   int iRightMargin;

   int iHeight;
   int iWidth;

   int iFirstRow;
   int iFirstCol;

   int iColorIndex;
   int iColorCount;
   int * piColors;

   PHB_SCREENCELL screenBuffer;

} HB_CT_WND, * PHB_CT_WND;

typedef struct
{
   PHB_GT pGT;

   int iShadowWidth;
   int iShadowAttr;

   int iOpenWindows;
   int iCurrWindow;
   int iMaxWindow;

   int fBoardSet;
   int iBoardTop;
   int iBoardLeft;
   int iBoardBottom;
   int iBoardRight;

   int fBoardTop;
   int fBoardLeft;
   int fBoardBottom;
   int fBoardRight;

   int iMoveMode;
   int iVerticalStep;
   int iHorizontalStep;

   PHB_CT_WND * windows;
   int * windowStack;
   int * pWindowMap;
   int * pShadowMap;
   int iMapWidth;
   int iMapHeight;

   int iLastKey;

} HB_GTCTW, * PHB_GTCTW;

static int hb_ctw_CalcShadowWidth( int iRows, int iCols )
{
   if( iRows + iRows >= iCols )
      return 1;
   else
      return 2;
}

static void hb_ctw_SetMap( PHB_GTCTW pCTW, int * piMap, int iWindow, int iTop, int iLeft, int iBottom, int iRight )
{
   ULONG lIndex;
   int i;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_SetMap(%p,%p,%d,%d,%d,%d,%d)", pCTW, piMap, iWindow, iTop, iLeft, iBottom, iRight));

   if( iTop < 0 )
      iTop = 0;
   if( iBottom >= pCTW->iMapHeight )
      iBottom = pCTW->iMapHeight - 1;
   if( iLeft < 0 )
      iLeft = 0;
   if( iRight >= pCTW->iMapWidth )
      iRight = pCTW->iMapWidth - 1;

   while( iTop <= iBottom )
   {
      lIndex = iTop * pCTW->iMapWidth + iLeft;
      for( i = iLeft; i <= iRight; ++i, ++lIndex )
         piMap[ lIndex ] = iWindow;
      ++iTop;
   }
}

static void hb_ctw_ClearMap( PHB_GTCTW pCTW )
{
   ULONG ulSize;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_ClearMap(%p)", pCTW));

   ulSize = ( ULONG ) pCTW->iMapHeight * pCTW->iMapWidth * sizeof( int );
   memset( pCTW->pWindowMap, 0, ulSize );
   memset( pCTW->pShadowMap, 0, ulSize );
}

static void hb_ctw_WindowMap( PHB_GTCTW pCTW, int iWindow, BOOL fExpose )
{
   PHB_CT_WND pWnd;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_WindowMap(%p,%d,%d)", pCTW, iWindow, (int) fExpose));

   pWnd = pCTW->windows[ iWindow ];

   if( ! pWnd->fHidden )
   {
      int iLastRow = pWnd->iFirstRow + pWnd->iHeight - 1,
          iLastCol = pWnd->iFirstCol + pWnd->iWidth - 1;

      hb_ctw_SetMap( pCTW, pCTW->pWindowMap, iWindow, 
                     pWnd->iFirstRow, pWnd->iFirstCol,
                     iLastRow, iLastCol );
      hb_ctw_SetMap( pCTW, pCTW->pShadowMap, 0,
                     pWnd->iFirstRow, pWnd->iFirstCol,
                     iLastRow, iLastCol );
      if( pWnd->iShadowAttr >= 0 &&
          iLastRow >= pCTW->iBoardTop && iLastCol >= pCTW->iBoardLeft &&
          pWnd->iFirstRow <= pCTW->iBoardBottom && pWnd->iFirstCol <= pCTW->iBoardRight )
      {
         iLastRow += 1;
         iLastCol += pCTW->iShadowWidth;
         hb_ctw_SetMap( pCTW, pCTW->pShadowMap, iWindow,
                        iLastRow, pWnd->iFirstCol + pCTW->iShadowWidth,
                        iLastRow, iLastCol );
         hb_ctw_SetMap( pCTW, pCTW->pShadowMap, iWindow,
                        pWnd->iFirstRow + 1, pWnd->iFirstCol + pWnd->iWidth,
                        iLastRow - 1, iLastCol );
      }
      if( fExpose )
      {
         HB_GTSUPER_EXPOSEAREA( pCTW->pGT, pWnd->iFirstRow, pWnd->iFirstCol,
                                iLastRow, iLastCol );
      }
   }
}

static void hb_ctw_RemapAllWindows( PHB_GTCTW pCTW )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_RemapAllWindows(%p)", pCTW));

   if( pCTW->iMaxWindow )
   {
      int i;

      hb_ctw_ClearMap( pCTW );
      for( i = 0; i < pCTW->iOpenWindows; ++i )
         hb_ctw_WindowMap( pCTW, pCTW->windowStack[ i ], FALSE );
      HB_GTSUPER_EXPOSEAREA( pCTW->pGT, 0, 0, pCTW->iMapHeight, pCTW->iMapWidth );
   }
}

static int hb_ctw_SetShadowAttr( PHB_GTCTW pCTW, int iAttr )
{
   int iOldAttr;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_SetShadowAttr(%p,%d)", pCTW, iAttr));

   iOldAttr = pCTW->iShadowAttr;
   if( iAttr >= -1 )
      pCTW->iShadowAttr = iAttr;

   return iOldAttr;
}

static int hb_ctw_SetMoveMode( PHB_GTCTW pCTW, int iMode )
{
   int iOldMode;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_SetMoveMode(%p,%d)", pCTW, iMode));

   iOldMode = pCTW->iMoveMode;
   if( iMode >= 0 )
      pCTW->iMoveMode = iMode;

   return iOldMode;
}

static int hb_ctw_SetMoveStep( PHB_GTCTW pCTW, int iVertical, int iHorizontal )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_SetMoveStep(%p,%d,%d)", pCTW, iVertical, iHorizontal));

   if( iVertical < pCTW->iMapHeight && iHorizontal < pCTW->iMapWidth )
   {
      pCTW->iVerticalStep   = iVertical;
      pCTW->iHorizontalStep = iHorizontal;

      return 0;
   }

   return -1;
}

static int hb_ctw_SetWindowBoard( PHB_GTCTW pCTW, int iTop, int iLeft, int iBottom, int iRight )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_SetWindowBoard(%p,%d,%d,%d,%d)", pCTW, iTop, iLeft, iBottom, iRight));

   /*
    * This limitation is only for strict CT3 compatibility, the CTW GTs
    * can work in practice with any virtual board size and position and
    * is limited only by available physical memory, [druzus]
    */
   if( iBottom >= pCTW->iMapHeight )
      iBottom = pCTW->iMapHeight - 1;
   if( iRight >= pCTW->iMapWidth )
      iRight = pCTW->iMapWidth - 1;

   if( iTop >= 0 && iLeft >= 0 && iTop < iBottom && iLeft < iRight )
   {
      pCTW->iBoardTop     = iTop;
      pCTW->iBoardLeft    = iLeft;
      pCTW->iBoardBottom  = iBottom;
      pCTW->iBoardRight   = iRight;
      pCTW->fBoardSet     = TRUE;
      hb_ctw_RemapAllWindows( pCTW );

      return 0;
   }

   return -1;
}

static int  hb_ctw_SetBorderMode( PHB_GTCTW pCTW, int iTop, int iLeft, int iBottom, int iRight )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_SetBorderMode(%p,%d,%d,%d,%d)", pCTW, iTop, iLeft, iBottom, iRight));

   if( iTop >= 0 )
      pCTW->fBoardTop     = iTop != 0;
   if( iLeft >= 0 )
      pCTW->fBoardLeft    = iLeft != 0;
   if( iBottom >= 0 )
      pCTW->fBoardBottom  = iBottom != 0;
   if( iRight >= 0 )
      pCTW->fBoardRight   = iRight != 0;

   return 0;
}

static int hb_ctw_CurrentWindow( PHB_GTCTW pCTW )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_CurrentWindow(%p)", pCTW));

   return pCTW->iCurrWindow;
}

static int hb_ctw_SelectWindow( PHB_GTCTW pCTW, int iWindow )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_SelectWindow(%p,%d)", pCTW, iWindow));

   if( iWindow == 0 )
      pCTW->iCurrWindow = iWindow;
   else if( iWindow != pCTW->iCurrWindow &&
            iWindow > 0 && iWindow <= pCTW->iMaxWindow &&
            pCTW->windows[ iWindow ] != NULL )
   {
      pCTW->iCurrWindow = iWindow;
      if( iWindow != pCTW->windowStack[ pCTW->iOpenWindows - 1 ] )
      {
         int i;

         for( i = 0; i < pCTW->iOpenWindows; ++i )
         {
            if( pCTW->windowStack[ i ] == iWindow )
               break;
         }
         while( i < pCTW->iOpenWindows - 1 )
         {
            pCTW->windowStack[ i ] = pCTW->windowStack[ i + 1 ];
            ++i;
         }
         pCTW->windowStack[ pCTW->iOpenWindows - 1 ] = iWindow;
         /* INFO: CT effectively calls hb_ctw_RemapAllWindows() here */
         hb_ctw_WindowMap( pCTW, iWindow, TRUE );
      }
   }

   return pCTW->iCurrWindow;
}

static int hb_ctw_MaxWindow( PHB_GTCTW pCTW )
{
   int i, iMaxHandle = 0;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_MaxWindow(%p)", pCTW));

   for( i = 0; i < pCTW->iOpenWindows; ++i )
   {
      if( iMaxHandle < pCTW->windowStack[ i ] )
         iMaxHandle = pCTW->windowStack[ i ];
   }

   return iMaxHandle;
}

static int hb_ctw_CreateWindow( PHB_GTCTW pCTW, int iTop, int iLeft, int iBottom, int iRight, BOOL fClear, int iColor )
{
   PHB_CT_WND pWnd;
   BYTE bAttr, bColor;
   USHORT usChar;
   int iRow, iCol, iHeight, iWidth, iTmp;
   long lIndex;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_CreateWindow(%p,%d,%d,%d,%d,%d,%d)", pCTW, iTop, iLeft, iBottom, iRight, (int) fClear, iColor));

   if( pCTW->iOpenWindows == pCTW->iMaxWindow )
   {
      int i = pCTW->iMaxWindow;

      if( pCTW->iMaxWindow == 0 )
      {
         ULONG ulSize;

         HB_GTSUPER_GETSIZE( pCTW->pGT, &pCTW->iMapHeight, &pCTW->iMapWidth );
         pCTW->iShadowWidth = hb_ctw_CalcShadowWidth( pCTW->iMapHeight, pCTW->iMapWidth );
         if( !pCTW->fBoardSet )
            hb_ctw_SetWindowBoard( pCTW, 0, 0, pCTW->iMapHeight - 1, pCTW->iMapWidth - 1 );
         ulSize = ( ULONG ) pCTW->iMapHeight * pCTW->iMapWidth * sizeof( int );
         pCTW->pWindowMap = ( int * ) hb_xgrab( ulSize );
         pCTW->pShadowMap = ( int * ) hb_xgrab( ulSize );
         hb_ctw_ClearMap( pCTW );

         pCTW->iMaxWindow = HB_CTWIN_ALLOC;
         pCTW->windows = ( PHB_CT_WND * ) hb_xgrab( ( HB_CTWIN_ALLOC + 1 ) * sizeof( PHB_CT_WND ) );
         pCTW->windowStack = ( int * ) hb_xgrab( HB_CTWIN_ALLOC * sizeof( int ) );
         pCTW->windows[ 0 ] = NULL;
      }
      else
      {
         pCTW->iMaxWindow += HB_CTWIN_ALLOC;
         pCTW->windows = ( PHB_CT_WND * ) hb_xrealloc( pCTW->windows, ( pCTW->iMaxWindow + 1 ) * sizeof( PHB_CT_WND ) );
         pCTW->windowStack = ( int * ) hb_xrealloc( pCTW->windowStack, pCTW->iMaxWindow * sizeof( int ) );
      }
      do
      {
         pCTW->windows[ i + 1 ] = NULL;
         pCTW->windowStack[ i ] = 0;
      }
      while( ++i < pCTW->iMaxWindow );
   }

   iHeight = iBottom - iTop + 1;
   iWidth  = iRight - iLeft + 1;
   iRow = iTop;
   iCol = iLeft;

   if( iHeight > pCTW->iBoardBottom - pCTW->iBoardTop + 1 )
      iHeight = pCTW->iBoardBottom - pCTW->iBoardTop + 1;
   if( iWidth > pCTW->iBoardRight - pCTW->iBoardLeft + 1 )
      iWidth = pCTW->iBoardRight - pCTW->iBoardLeft + 1;

   if( iHeight < HB_CTWIN_MINROWS || iWidth < HB_CTWIN_MINCOLS ||
       iHeight > HB_CTWIN_MAXROWS || iWidth > HB_CTWIN_MAXCOLS )
      return -1;

   iTop    = pCTW->iBoardTop - ( pCTW->fBoardTop ? iHeight : 0 );
   iBottom = pCTW->iBoardBottom + 1 - ( pCTW->fBoardBottom ? 0 : iHeight );
   iLeft   = pCTW->iBoardLeft - ( pCTW->fBoardLeft ? iWidth : 0 );
   iRight  = pCTW->iBoardRight + 1 - ( pCTW->fBoardRight ? 0 : iWidth );

   if( iRow < iTop )
      iRow = iTop;
   else if( iRow > iBottom )
      iRow = iBottom;
   if( iCol < iLeft )
      iCol = iLeft;
   else if( iCol > iRight )
      iCol = iRight;

   pWnd = ( PHB_CT_WND ) hb_xgrab( sizeof( HB_CT_WND ) );
   memset( pWnd, 0,  sizeof( HB_CT_WND ) );

   pWnd->fHidden = FALSE;
   pWnd->iShadowAttr = pCTW->iShadowAttr;
   pWnd->iCursorStyle = HB_GTSELF_GETCURSORSTYLE( pCTW->pGT );

   pWnd->iHeight = iHeight;
   pWnd->iWidth  = iWidth;
   pWnd->iFirstRow = iRow;
   pWnd->iFirstCol = iCol;

   HB_GTSELF_GETCOLORDATA( pCTW->pGT, &pWnd->piColors, &pWnd->iColorCount, &pWnd->iColorIndex );

   pWnd->screenBuffer = ( PHB_SCREENCELL ) hb_xgrab( ( ULONG ) pWnd->iHeight *
                                    pWnd->iWidth * sizeof( HB_SCREENCELL ) );

   if( pWnd->iShadowAttr >= 0 )
      fClear = TRUE;
   bAttr  = 0;
   bColor = iColor ? ( BYTE ) iColor : ( BYTE ) HB_GTSELF_GETCOLOR( pCTW->pGT );
   usChar = ( USHORT ) HB_GTSELF_GETCLEARCHAR( pCTW->pGT );

   lIndex = 0;
   for( iRow = pWnd->iFirstRow; iRow < pWnd->iFirstRow + pWnd->iHeight; ++iRow )
   {
      for( iCol = pWnd->iFirstCol; iCol < pWnd->iFirstCol + pWnd->iWidth; ++iCol )
      {
         if( !fClear && !HB_GTSELF_GETSCRCHAR( pCTW->pGT, iRow, iCol, &bColor, &bAttr, &usChar ) )
         {
            usChar = ( USHORT ) HB_GTSELF_GETCLEARCHAR( pCTW->pGT );
            bColor = ( BYTE ) HB_GTSELF_GETCOLOR( pCTW->pGT );
            bAttr  = 0;
         }
         pWnd->screenBuffer[ lIndex ].c.usChar = usChar;
         pWnd->screenBuffer[ lIndex ].c.bColor = bColor;
         pWnd->screenBuffer[ lIndex ].c.bAttr  = 0;
         ++lIndex;
      }
   }

   for( iTmp = 1; iTmp < pCTW->iMaxWindow; ++iTmp )
   {
      if( pCTW->windows[ iTmp ] == NULL )
         break;
   }
   pWnd->iHandle = iTmp;

   pCTW->windows[ pWnd->iHandle ] = pWnd;
   pCTW->windowStack[ pCTW->iOpenWindows++ ] = pWnd->iHandle;
   pCTW->iCurrWindow = pWnd->iHandle;

   hb_ctw_WindowMap( pCTW, pWnd->iHandle, TRUE );

   return pWnd->iHandle;
}

static int hb_ctw_CloseWindow( PHB_GTCTW pCTW, int iWindow )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_CloseWindow(%p,%d)", pCTW, iWindow));

   if( iWindow > 0 && iWindow <= pCTW->iMaxWindow && pCTW->windows[ iWindow ] )
   {
      PHB_CT_WND pWnd = pCTW->windows[ iWindow ];
      int i, iWnd, iLast;
      BOOL fHidden = pWnd->fHidden;

      hb_xfree( pWnd->screenBuffer );
      if( pWnd->iColorCount )
         hb_xfree( pWnd->piColors );
      hb_xfree( pWnd );
      pCTW->windows[ iWindow ] = NULL;

      iWnd = 0;
      i = --pCTW->iOpenWindows;
      do
      {
         iLast = pCTW->windowStack[ i ];
         pCTW->windowStack[ i ] = iWnd;
         if( iLast == iWindow )
            break;
         iWnd = iLast;
      }
      while( --i >= 0 );

      if( iWindow == pCTW->iCurrWindow )
         pCTW->iCurrWindow = pCTW->iOpenWindows > 0 ? pCTW->windowStack[ pCTW->iOpenWindows - 1 ] : 0;

      if( !fHidden )
         hb_ctw_RemapAllWindows( pCTW );
      return pCTW->iCurrWindow;
   }

   return -1;
}

static int hb_ctw_CloseAllWindows( PHB_GTCTW pCTW )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_CloseAllWindows(%p)", pCTW));

   if( pCTW->iOpenWindows > 0 )
   {
      PHB_CT_WND pWnd;
      int i, iWindow;

      for( i = 0; i < pCTW->iOpenWindows; ++i )
      {
         iWindow = pCTW->windowStack[ i ];
         pWnd = pCTW->windows[ iWindow ];
         pCTW->windowStack[ i ] = 0;
         pCTW->windows[ iWindow ] = NULL;
         hb_xfree( pWnd->screenBuffer );
         if( pWnd->iColorCount )
            hb_xfree( pWnd->piColors );
         hb_xfree( pWnd );
      }
      pCTW->iOpenWindows = pCTW->iCurrWindow = 0;
      hb_ctw_RemapAllWindows( pCTW );
      return 0;
   }

   return -1;
}

static int hb_ctw_CenterWindow( PHB_GTCTW pCTW, int iWindow, BOOL fCenter )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_CenterWindow(%p,%d,%d)", pCTW, iWindow, (int) fCenter));

   if( iWindow > 0 && iWindow <= pCTW->iOpenWindows )
   {
      PHB_CT_WND pWnd = pCTW->windows[ iWindow ];

      if( pWnd )
      {
         if( fCenter )
         {
            int iHeight = pCTW->iBoardBottom - pCTW->iBoardTop + 1,
                iWidth = pCTW->iBoardRight - pCTW->iBoardLeft + 1;

            pWnd->iFirstRow = pCTW->iBoardTop;
            pWnd->iFirstCol = pCTW->iBoardLeft;

            if( iHeight > pWnd->iHeight )
               pWnd->iFirstRow += ( iHeight - pWnd->iHeight ) >> 1;
            if( iWidth > pWnd->iWidth )
               pWnd->iFirstCol += ( iWidth - pWnd->iWidth ) >> 1;
         }
         else
         {
            if( pWnd->iFirstRow > pCTW->iBoardBottom - pWnd->iHeight + 1 )
               pWnd->iFirstRow = pCTW->iBoardBottom - pWnd->iHeight + 1;
            if( pWnd->iFirstRow < pCTW->iBoardTop )
               pWnd->iFirstRow = pCTW->iBoardTop;
            if( pWnd->iFirstCol > pCTW->iBoardRight - pWnd->iWidth + 1 )
               pWnd->iFirstCol = pCTW->iBoardRight - pWnd->iWidth + 1;
            if( pWnd->iFirstCol < pCTW->iBoardLeft )
               pWnd->iFirstCol = pCTW->iBoardLeft;
         }
         return iWindow;
      }
   }

   return -1;
}

static int hb_ctw_MoveWindow( PHB_GTCTW pCTW, int iWindow, int iRow, int iCol )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_MoveWindow(%p,%d,%d,%d)", pCTW, iWindow, iRow, iCol));

   if( iWindow > 0 && iWindow <= pCTW->iOpenWindows )
   {
      PHB_CT_WND pWnd = pCTW->windows[ iWindow ];

      if( pWnd )
      {
         if( ( iRow + ( pCTW->fBoardTop ? pWnd->iHeight : 0 ) >= pCTW->iBoardTop ) &&
             ( iRow + ( pCTW->fBoardBottom ? 0 : pWnd->iHeight ) <= pCTW->iBoardBottom + 1 ) &&
             ( iCol + ( pCTW->fBoardLeft ? pWnd->iWidth : 0 ) >= pCTW->iBoardLeft ) &&
             ( iCol + ( pCTW->fBoardRight ? 0 : pWnd->iWidth ) <= pCTW->iBoardRight + 1 ) )
         {
            pWnd->iFirstRow = iRow;
            pWnd->iFirstCol = iCol;
            if( ! pWnd->fHidden )
               hb_ctw_RemapAllWindows( pCTW );
            return iWindow;
         }
      }
   }

   return -1;
}

static int hb_ctw_ChangeMargins( PHB_GTCTW pCTW, int iWindow, int iTop, int iLeft, int iBottom, int iRight )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_ChangeMargins(%p,%d,%d,%d,%d,%d)", pCTW, iWindow, iTop, iLeft, iBottom, iRight));

   if( iWindow > 0 && iWindow <= pCTW->iOpenWindows )
   {
      PHB_CT_WND pWnd = pCTW->windows[ iWindow ];

      if( pWnd )
      {
         if( ( iTop += pWnd->iTopMargin ) < 0 )
            iTop = 0;
         if( ( iLeft += pWnd->iLeftMargin ) < 0 )
            iLeft = 0;
         if( ( iBottom += pWnd->iBottomMargin ) < 0 )
            iBottom = 0;
         if( ( iRight += pWnd->iRightMargin ) < 0 )
            iRight = 0;

         if( iTop + iBottom < pWnd->iHeight && iLeft + iRight < pWnd->iWidth )
         {
            pWnd->iTopMargin    = iTop;
            pWnd->iLeftMargin   = iLeft;
            pWnd->iBottomMargin = iBottom;
            pWnd->iRightMargin  = iRight;

            return iWindow;
         }
      }
   }

   return -1;
}

static int hb_ctw_GetWindowCords( PHB_GTCTW pCTW, int iWindow, BOOL fCenter, int * piTop, int * piLeft, int * piBottom, int * piRight )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_GetWindowCords(%p,%d,%d,%p,%p,%p,%p)", pCTW, iWindow, (int) fCenter, piTop, piLeft, piBottom, piRight));

   if( iWindow > 0 && iWindow <= pCTW->iOpenWindows )
   {
      PHB_CT_WND pWnd = pCTW->windows[ iWindow ];

      if( pWnd )
      {
         if( fCenter )
         {
            int iHeight = pCTW->iBoardBottom - pCTW->iBoardTop + 1,
                iWidth = pCTW->iBoardRight - pCTW->iBoardLeft + 1;

            *piTop  = pCTW->iBoardTop;
            *piLeft = pCTW->iBoardLeft;

            if( iHeight > pWnd->iHeight )
               *piTop += ( iHeight - pWnd->iHeight ) >> 1;
            if( iWidth > pWnd->iWidth )
               *piLeft += ( iWidth - pWnd->iWidth ) >> 1;
         }
         else
         {
            *piTop  = pWnd->iFirstRow;
            *piLeft = pWnd->iFirstCol;
         }
         *piBottom = *piTop + pWnd->iHeight - 1;
         *piRight  = *piLeft + pWnd->iWidth - 1;

         return iWindow;
      }
   }

   *piTop = *piLeft = 0;
   *piBottom = HB_GTSELF_MAXROW( pCTW->pGT );
   *piRight  = HB_GTSELF_MAXCOL( pCTW->pGT );

   return -1;
}

static int hb_ctw_GetFormatCords( PHB_GTCTW pCTW, int iWindow, BOOL fRelative, int * piTop, int * piLeft, int * piBottom, int * piRight )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_GetFormatCords(%p,%d,%d,%p,%p,%p,%p)", pCTW, iWindow, (int) fRelative, piTop, piLeft, piBottom, piRight));

   if( iWindow > 0 && iWindow <= pCTW->iOpenWindows )
   {
      PHB_CT_WND pWnd = pCTW->windows[ iWindow ];

      if( pWnd )
      {
         if( fRelative )
         {
            *piTop    = pWnd->iTopMargin;
            *piLeft   = pWnd->iLeftMargin;
            *piBottom = pWnd->iBottomMargin;
            *piRight  = pWnd->iRightMargin;
         }
         else
         {
            *piTop    = pWnd->iFirstRow + pWnd->iTopMargin;
            *piLeft   = pWnd->iFirstCol + pWnd->iLeftMargin;
            *piBottom = pWnd->iFirstRow + pWnd->iHeight - pWnd->iBottomMargin - 1;
            *piRight  = pWnd->iFirstCol + pWnd->iWidth - pWnd->iRightMargin - 1;
         }
         return iWindow;
      }
   }

   if( fRelative )
   {
      *piTop = *piLeft = *piBottom = *piRight = 0;
   }
   else
   {
      *piTop = *piLeft = 0;
      *piBottom = HB_GTSELF_MAXROW( pCTW->pGT );
      *piRight  = HB_GTSELF_MAXCOL( pCTW->pGT );
   }

   return -1;
}

static int hb_ctw_AddWindowBox( PHB_GTCTW pCTW, int iWindow, BYTE * szBox, int iColor )
{
   int iMaxRow, iMaxCol;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_AddWindowBox(%p,%d,%p,%d)", pCTW, iWindow, szBox, iColor));

   iMaxRow = HB_GTSELF_MAXROW( pCTW->pGT );
   iMaxCol = HB_GTSELF_MAXCOL( pCTW->pGT );

   if( iMaxRow > 1 && iMaxCol > 1 )
   {
      if( iColor == 0 )
         iColor = HB_GTSELF_GETCOLOR( pCTW->pGT );
      HB_GTSELF_BOX( pCTW->pGT, 0, 0, iMaxRow, iMaxCol, szBox, ( BYTE ) iColor );
      if( iWindow > 0 && iWindow <= pCTW->iOpenWindows &&
          pCTW->windows[ iWindow ] != NULL )
      {
         HB_GTSELF_SETPOS( pCTW->pGT, 0, 0 );
         hb_ctw_ChangeMargins( pCTW, iWindow, 1, 1, 1, 1 );
      }
      else
         HB_GTSELF_SETPOS( pCTW->pGT, 1, 1 );

      return 0;
   }

   return -1;
}

/* ********************************************************************** */

static void hb_ctw_Init( PHB_GT pGT )
{
   PHB_GTCTW pCTW;
   int iRow, iCol;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_Init(%p)",pGT));

   pCTW = ( PHB_GTCTW ) hb_xgrab( sizeof( HB_GTCTW ) );
   memset( pCTW, 0, sizeof( HB_GTCTW ) );
   HB_GTLOCAL( pGT ) = pCTW;
   pCTW->pGT             = pGT;
   pCTW->iShadowWidth    = 2;
   pCTW->iShadowAttr     = -1;
   pCTW->iMoveMode       = 1;
   pCTW->iVerticalStep   = 2;
   pCTW->iHorizontalStep = 5;

   HB_GTSUPER_GETSIZE( pGT, &pCTW->iMapHeight, &pCTW->iMapWidth );

   /* update cursor position to the rules used by CTWIN */
   HB_GTSELF_GETPOS( pGT, &iRow, &iCol );
   HB_GTSELF_SETPOS( pGT, iRow, iCol );
}

/* ********************************************************************** */

static PHB_GTCTW hb_ctw_base( void )
{
   PHB_GT pGT;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_base(%p)"));

   pGT = hb_gt_Base();
   if( pGT )
   {
      if( *HB_GTID_PTR != 0 || hb_gtLoad( HB_GT_DRVNAME( HB_GT_NAME ), NULL ) )
      {
         if( ! HB_GTCTW_GET( pGT ) )
            hb_ctw_Init( pGT );
         if( HB_GTCTW_GET( pGT ) )
            return HB_GTCTW_GET( pGT );
      }
      hb_gt_BaseFree( pGT );
   }

   return NULL;
}

static void hb_ctw_gt_Init( PHB_GT pGT, HB_FHANDLE hFilenoStdin, HB_FHANDLE hFilenoStdout, HB_FHANDLE hFilenoStderr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_Init(%p,%p,%p,%p)", pGT, hFilenoStdin, hFilenoStdout, hFilenoStderr));

   HB_GTSUPER_INIT( pGT, hFilenoStdin, hFilenoStdout, hFilenoStderr );
   hb_ctw_Init( pGT );
}

static void hb_ctw_gt_Exit( PHB_GT pGT )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_Exit(%p)", pGT));

   HB_GTSELF_REFRESH( pGT );

   pCTW = HB_GTCTW_GET( pGT );
   hb_ctw_CloseAllWindows( pCTW );

   HB_GTSUPER_EXIT( pGT );

   if( pCTW )
   {
      if( pCTW->iMaxWindow > 0 )
      {
         hb_xfree( pCTW->windows );
         hb_xfree( pCTW->windowStack );
         hb_xfree( pCTW->pWindowMap );
         hb_xfree( pCTW->pShadowMap );
      }
      hb_xfree( pCTW );
   }
}

static int hb_ctw_MouseRow( PHB_GT pGT )
{
   PHB_GTCTW pCTW;
   int iRow;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_MouseRow(%p)", pGT));

   iRow = HB_GTSUPER_MOUSEROW( pGT );

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
      iRow -= pCTW->windows[ pCTW->iCurrWindow ]->iFirstRow +
              pCTW->windows[ pCTW->iCurrWindow ]->iTopMargin;

   return iRow;
}

static int hb_ctw_MouseCol( PHB_GT pGT )
{
   PHB_GTCTW pCTW;
   int iCol;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_MouseCol(%p)", pGT));

   iCol = HB_GTSUPER_MOUSECOL( pGT );

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
      iCol -= pCTW->windows[ pCTW->iCurrWindow ]->iFirstCol +
              pCTW->windows[ pCTW->iCurrWindow ]->iLeftMargin;

   return iCol;
}

static void hb_ctw_gt_GetPos( PHB_GT pGT, int * piRow, int * piCol )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_GetPos(%p,%p,%p)", pGT, piRow, piCol));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
   {
      *piRow = pCTW->windows[ pCTW->iCurrWindow ]->iRow;
      *piCol = pCTW->windows[ pCTW->iCurrWindow ]->iCol;
   }
   else
      HB_GTSUPER_GETPOS( pGT, piRow, piCol );
}

/*
 * CTWIN uses differ rules when set cursor position out of screen visible
 * area then standard Clipper's GT drivers so we have to replicate it in
 * SETPOS() method, [druzus]
 */
static void hb_ctw_gt_SetPos( PHB_GT pGT, int iRow, int iCol )
{
   PHB_GTCTW pCTW;
   int iHeight, iWidth;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_GetPos(%p,%d,%d)", pGT, iRow, iCol));

   iHeight = HB_GTSELF_MAXROW( pGT ) + 1;
   iWidth = HB_GTSELF_MAXCOL( pGT ) + 1;

   if( iCol > iWidth )
      iCol = iWidth;
   else if( iCol < 0 )
   {
      iRow += iCol / iWidth - 1;
      iCol = iWidth + iCol % iWidth;
   }
   if( iRow > iHeight )
      iRow = iHeight;

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
   {
      if( iRow < - pCTW->windows[ pCTW->iCurrWindow ]->iTopMargin )
         iRow = - pCTW->windows[ pCTW->iCurrWindow ]->iTopMargin;
      pCTW->windows[ pCTW->iCurrWindow ]->iRow = iRow;
      pCTW->windows[ pCTW->iCurrWindow ]->iCol = iCol;
   }
   else
   {
      if( iRow < 0 )
         iRow = 0;
      HB_GTSUPER_SETPOS( pGT, iRow, iCol );
   }
}

static int hb_ctw_gt_MaxCol( PHB_GT pGT )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_MaxCol(%p)", pGT));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
      return pCTW->windows[ pCTW->iCurrWindow ]->iWidth -
             pCTW->windows[ pCTW->iCurrWindow ]->iLeftMargin -
             pCTW->windows[ pCTW->iCurrWindow ]->iRightMargin - 1;
   else
      return HB_GTSUPER_MAXCOL( pGT );
}

static int hb_ctw_gt_MaxRow( PHB_GT pGT )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_MaxRow(%p)", pGT));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
      return pCTW->windows[ pCTW->iCurrWindow ]->iHeight -
             pCTW->windows[ pCTW->iCurrWindow ]->iTopMargin -
             pCTW->windows[ pCTW->iCurrWindow ]->iBottomMargin - 1;
   else
      return HB_GTSUPER_MAXROW( pGT );
}

/*
 * CTWIN uses differ rules in console output then standard Clipper's
 * GT drivers so we have to overload WRITECON() method, [druzus]
 */
#define WRITECON_BUFFER_SIZE 512

static void hb_ctw_gt_WriteCon( PHB_GT pGT, BYTE * pText, ULONG ulLength )
{
   int iLen = 0;
   BOOL bDisp = FALSE;
   BOOL bBell = FALSE;
   int iRow, iCol, iMaxRow, iMaxCol;
   BYTE szString[ WRITECON_BUFFER_SIZE ];

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_WriteCon(%p,%p,%lu)", pGT, pText, ulLength));

   iMaxRow = HB_GTSELF_MAXROW( pGT );
   iMaxCol = HB_GTSELF_MAXCOL( pGT );

   HB_GTSELF_GETPOS( pGT, &iRow, &iCol );

   if( iRow > iMaxRow || iCol > iMaxCol )
   {
      if( iRow > iMaxRow )
         iRow = iMaxRow;
      if( iCol > iMaxCol )
         iCol = iMaxCol;
      HB_GTSELF_SETPOS( pGT, iRow, iCol );
   }

   while( ulLength-- )
   {
      BYTE ch = *pText++;

      switch( ch )
      {
         case HB_CHAR_BEL:
            bDisp = bBell = TRUE;
            break;

         case HB_CHAR_BS:
            if( iCol > 0 )
            {
               --iCol;
               bDisp = TRUE;
            }
            else if( iRow > 0 )
            {
               iCol = iMaxCol;
               --iRow;
               bDisp = TRUE;
            }
            if( bDisp )
            {
               if( iLen )
                  szString[ iLen - 1 ] = ' ';
               else
               {
                  HB_GTSELF_SETPOS( pGT, iRow, iCol );
                  szString[ iLen++ ] = ' ';
               }
            }
            break;

         case HB_CHAR_LF:
            iCol = 0;
            ++iRow;
            bDisp = TRUE;
            break;

         case HB_CHAR_CR:
            iCol = 0;
            if( *pText == HB_CHAR_LF )
            {
               ++iRow;
               ++pText;
               --ulLength;
            }
            bDisp = TRUE;
            break;

         default:
            szString[ iLen++ ] = ch;
            if( ++iCol > iMaxCol )
            {
               iCol = 0;
               ++iRow;
               bDisp = TRUE;
            }
            else if( iLen >= WRITECON_BUFFER_SIZE )
               bDisp = TRUE;
      }

      if( bDisp || ulLength == 0 )
      {
         if( iLen )
            HB_GTSELF_WRITE( pGT, szString, iLen );

         iLen = 0;
         if( iRow > iMaxRow )
         {
            HB_GTSELF_SCROLL( pGT, 0, 0, iMaxRow, iMaxCol,
                              ( BYTE ) HB_GTSELF_GETCOLOR( pGT ),
                              ( BYTE ) HB_GTSELF_GETCLEARCHAR( pGT ),
                          iRow - iMaxRow, 0 );
            iRow = iMaxRow;
            iCol = 0;
         }
         HB_GTSELF_SETPOS( pGT, iRow, iCol );
         bDisp = FALSE;

         /* To emulate scrolling */
         HB_GTSELF_FLUSH( pGT );

         if( bBell )
         {
            HB_GTSELF_BELL( pGT );
            bBell = FALSE;
         }
      }
   }
}


static int hb_ctw_gt_GetCursorStyle( PHB_GT pGT )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_GetCursorStyle(%p)", pGT));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
      return pCTW->windows[ pCTW->iCurrWindow ]->iCursorStyle;
   else
      return HB_GTSUPER_GETCURSORSTYLE( pGT );
}

static void hb_ctw_gt_SetCursorStyle( PHB_GT pGT, int iStyle )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_SetCursorStyle(%p,%d)", pGT, iStyle));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
   {
      switch( iStyle )
      {
         case SC_NONE:
         case SC_NORMAL:
         case SC_INSERT:
         case SC_SPECIAL1:
         case SC_SPECIAL2:
            pCTW->windows[ pCTW->iCurrWindow ]->iCursorStyle = iStyle;
            break;
         default:
            pCTW->windows[ pCTW->iCurrWindow ]->iCursorStyle = SC_NORMAL;
            break;
      }
   }
   else
      HB_GTSUPER_SETCURSORSTYLE( pGT, iStyle );
}

static void hb_ctw_gt_GetColorStr( PHB_GT pGT, char * pszColorString )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_GetColorStr(%p,%p)", pGT, pszColorString));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
   {
      PHB_CT_WND pWnd = pCTW->windows[ pCTW->iCurrWindow ];
      HB_GTSUPER_COLORSTOSTRING( pGT, pWnd->piColors, pWnd->iColorCount, pszColorString, HB_CLRSTR_LEN );
   }
   else
      HB_GTSUPER_GETCOLORSTR( pGT, pszColorString );
}

static void hb_ctw_gt_SetColorStr( PHB_GT pGT, const char * szColorString )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_SetColorStr(%p,%s)", pGT, szColorString));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
   {
      PHB_CT_WND pWnd = pCTW->windows[ pCTW->iCurrWindow ];
      HB_GTSUPER_STRINGTOCOLORS( pGT, szColorString, &pWnd->piColors, &pWnd->iColorCount );
      pWnd->iColorIndex = HB_CLR_STANDARD;
   }
   else
      HB_GTSUPER_SETCOLORSTR( pGT, szColorString );
}

static void hb_ctw_gt_ColorSelect( PHB_GT pGT, int iColorIndex )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_ColorSelect(%p,%d)", pGT, iColorIndex));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
   {
      PHB_CT_WND pWnd = pCTW->windows[ pCTW->iCurrWindow ];
      if( iColorIndex >= 0 && iColorIndex < pWnd->iColorCount )
         pWnd->iColorIndex = iColorIndex;
   }
   else
      HB_GTSUPER_COLORSELECT( pGT, iColorIndex );
}

static int hb_ctw_gt_GetColor( PHB_GT pGT )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_GetColor(%p)", pGT));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
   {
      PHB_CT_WND pWnd = pCTW->windows[ pCTW->iCurrWindow ];
      return pWnd->piColors[ pWnd->iColorIndex ];
   }
   else
      return HB_GTSUPER_GETCOLOR( pGT );
}

static void hb_ctw_gt_GetColorData( PHB_GT pGT, int ** pColorsPtr, int * piColorCount, int * piColorIndex )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_GetColor(%p,%p,%p,%p)", pGT, pColorsPtr, piColorCount, piColorIndex));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
   {
      PHB_CT_WND pWnd = pCTW->windows[ pCTW->iCurrWindow ];

      *pColorsPtr = ( int * ) hb_xgrab( pWnd->iColorCount * sizeof( int ) );
      memcpy( *pColorsPtr, pWnd->piColors, pWnd->iColorCount * sizeof( int ) );
      *piColorCount = pWnd->iColorCount;
      *piColorIndex = pWnd->iColorIndex;
   }
   else
      HB_GTSUPER_GETCOLORDATA( pGT, pColorsPtr, piColorCount, piColorIndex );
}

static void hb_ctw_gt_GetScrCursor( PHB_GT pGT, int * piRow, int * piCol, int * piStyle )
{
   PHB_GTCTW pCTW;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_GetScrCursor(%p,%p,%p,%p)", pGT, piRow, piCol, piStyle));

   HB_GTSUPER_GETSCRCURSOR( pGT, piRow, piCol, piStyle );
   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow > 0 )
   {
      PHB_CT_WND pWnd = pCTW->windows[ pCTW->iCurrWindow ];
      *piRow += pWnd->iFirstRow +
                pWnd->iTopMargin;
      *piCol += pWnd->iFirstCol +
                pWnd->iLeftMargin;
      if( *piStyle != SC_NONE &&
          ( *piRow < pCTW->iBoardTop  || *piRow > pCTW->iBoardBottom ||
            *piCol < pCTW->iBoardLeft || *piCol > pCTW->iBoardRight ) )
         *piStyle = SC_NONE;
   }
}

static BOOL hb_ctw_gt_GetScrChar( PHB_GT pGT, int iRow, int iCol,
                                  BYTE * pbColor, BYTE * pbAttr, USHORT * pusChar )
{
   PHB_GTCTW pCTW;
   int iWindow, iShadow;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_GetScrChar(%p,%d,%d,%p,%p,%p)", pGT, iRow, iCol, pbColor, pbAttr, pusChar));

   pCTW = HB_GTCTW_GET( pGT );
   iWindow = pCTW->iCurrWindow;
   iShadow = 0;
   if( pCTW->iOpenWindows > 0 )
   {
      if( iRow < pCTW->iBoardTop  || iRow > pCTW->iBoardBottom ||
          iCol < pCTW->iBoardLeft || iCol > pCTW->iBoardRight )
         iWindow = 0;
      else
      {
         long lIndex = ( long ) iRow * pCTW->iMapWidth + iCol;
         iWindow = pCTW->pWindowMap[ lIndex ];
         iShadow = pCTW->pShadowMap[ lIndex ];
      }
   }

   if( iWindow > 0 )
   {
      PHB_CT_WND pWnd = pCTW->windows[ iWindow ];
      iRow -= pWnd->iFirstRow;
      iCol -= pWnd->iFirstCol;
      if( iCol >= 0 && iRow >= 0 && iRow < pWnd->iHeight && iCol < pWnd->iWidth )
      {
         long lIndex = ( long ) iRow * pWnd->iWidth + iCol;
         *pusChar = pWnd->screenBuffer[ lIndex ].c.usChar;
         *pbColor = pWnd->screenBuffer[ lIndex ].c.bColor;
         *pbAttr  = pWnd->screenBuffer[ lIndex ].c.bAttr;
      }
      else
         return FALSE;
   }
   else if( ! HB_GTSUPER_GETSCRCHAR( pGT, iRow, iCol, pbColor, pbAttr, pusChar ) )
      return FALSE;

   if( iShadow > 0 )
   {
      PHB_CT_WND pWnd = pCTW->windows[ iShadow ];
      if( pWnd->iShadowAttr >= 0 )
         *pbColor = ( BYTE ) pWnd->iShadowAttr;
      *pbAttr |= HB_GT_ATTR_SHADOW;
   }

   return TRUE;
}

static BOOL hb_ctw_gt_GetChar( PHB_GT pGT, int iRow, int iCol,
                               BYTE * pbColor, BYTE * pbAttr, USHORT * pusChar )
{
   PHB_GTCTW pCTW;
   PHB_CT_WND pWnd;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_GetChar(%p,%d,%d,%p,%p,%p)", pGT, iRow, iCol, pbColor, pbAttr, pusChar));

   pCTW = HB_GTCTW_GET( pGT );
   if( pCTW->iCurrWindow == 0 )
      /* TODO: it may badly interacts with character translations */
      return HB_GTSELF_GETSCRCHAR( pGT, iRow, iCol, pbColor, pbAttr, pusChar );

   pWnd = pCTW->windows[ pCTW->iCurrWindow ];
   iRow += pWnd->iTopMargin;
   iCol += pWnd->iLeftMargin;

   if( iCol >= 0 && iRow >= 0 && iRow < pWnd->iHeight && iCol < pWnd->iWidth )
   {
      long lIndex = ( long ) iRow * pWnd->iWidth + iCol;
      *pusChar = pWnd->screenBuffer[ lIndex ].c.usChar;
      *pbColor = pWnd->screenBuffer[ lIndex ].c.bColor;
      *pbAttr  = pWnd->screenBuffer[ lIndex ].c.bAttr;
      return TRUE;
   }

   return FALSE;
}

static BOOL hb_ctw_gt_PutChar( PHB_GT pGT, int iRow, int iCol,
                               BYTE bColor, BYTE bAttr, USHORT usChar )
{
   PHB_GTCTW pCTW;
   int iWindow;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_PutChar(%p,%d,%d,%d,%d,%hu)", pGT, iRow, iCol, (int)bColor, (int)bAttr, (int)usChar));

   pCTW = HB_GTCTW_GET( pGT );
   iWindow = pCTW->iCurrWindow;
   if( iWindow == 0 && pCTW->iOpenWindows > 0 )
   {
      if( iRow >= pCTW->iBoardTop  && iRow <= pCTW->iBoardBottom &&
          iCol >= pCTW->iBoardLeft && iCol <= pCTW->iBoardRight )
      {
         long lIndex = ( long ) iRow * pCTW->iMapWidth + iCol;
         iWindow = pCTW->pWindowMap[ lIndex ];
#if 0
         /* When window with shadow is closed CT3 restores attributes
          * which existed before shadow was displayed. In some application
          * which switches to window 0 for pass-throw output it causes that
          * wrong attributes appears after this operation. In Harbour it's
          * fixed so such problem do not exist. Anyhow some code may switch
          * to window 0, make savescreen()/restscreen() and in such case
          * all shadow attributes are copied to window 0 buffer. The code
          * below is workaround for it. [druzus]
          */
         if( pCTW->pShadowMap[ lIndex ] != 0 )
         {
            int iShadow = pCTW->pShadowMap[ lIndex ];
            if( pCTW->windows[ iShadow ]->iShadowAttr >= 0 &&
                ( BYTE ) pCTW->windows[ iShadow ]->iShadowAttr == bColor )
            {
               BYTE bClr, bAtr;
               USHORT usCh;
               if( HB_GTSELF_GETSCRCHAR( pGT, iRow, iCol, &bClr, &bAtr, &usCh ) )
               {
                  if( usCh == usChar && bClr == bColor )
                     return TRUE;
               }
            }
         }
#endif
         pCTW->pShadowMap[ lIndex ] = 0;
      }
   }

   if( iWindow > 0 )
   {
      int iWndRow, iWndCol, iWndHeight, iWndWidth;
      PHB_CT_WND pWnd = pCTW->windows[ iWindow ];

      if( pCTW->iCurrWindow == 0 )
      {
         iWndRow = iRow - pWnd->iFirstRow;
         iWndCol = iCol - pWnd->iFirstCol;
         iWndHeight = pWnd->iWidth;
         iWndWidth  = pWnd->iWidth;
      }
      else
      {
         iWndRow = iRow + pWnd->iTopMargin;
         iWndCol = iCol + pWnd->iLeftMargin;
         iRow = iWndRow + pWnd->iFirstRow;
         iCol = iWndCol + pWnd->iFirstCol;
         iWndHeight = pWnd->iHeight - pWnd->iBottomMargin;
         iWndWidth  = pWnd->iWidth - pWnd->iRightMargin;
      }
      if( iWndCol >= 0 && iWndRow >= 0 &&
          iWndRow < iWndHeight &&
          iWndCol < iWndWidth )
      {
         long lIndex = ( long ) iWndRow * pWnd->iWidth + iWndCol;

         pWnd->screenBuffer[ lIndex ].c.usChar = usChar;
         pWnd->screenBuffer[ lIndex ].c.bColor = bColor;
         pWnd->screenBuffer[ lIndex ].c.bAttr  = bAttr;
         if( ! pWnd->fHidden )
         {
            if( pCTW->iCurrWindow == 0 )
               HB_GTSUPER_TOUCHCELL( pGT, iRow, iCol );
            else if( iRow >= pCTW->iBoardTop  && iRow <= pCTW->iBoardBottom &&
                     iCol >= pCTW->iBoardLeft && iCol <= pCTW->iBoardRight )
               HB_GTSUPER_TOUCHCELL( pGT, iRow, iCol );
         }
         return TRUE;
      }
      return FALSE;
   }

   return HB_GTSUPER_PUTCHAR( pGT, iRow, iCol, bColor, bAttr, usChar );
}

static BOOL hb_ctw_gt_Resize( PHB_GT pGT, int iRows, int iCols )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_Resize(%p,%d,%d)", pGT, iRows, iCols));

   if( HB_GTSUPER_RESIZE( pGT, iRows, iCols ) )
   {
      PHB_GTCTW pCTW = HB_GTCTW_GET( pGT );

      if( pCTW->iMaxWindow > 0 )
      {
         ULONG ulSize;

         pCTW->iMapHeight = iRows;
         pCTW->iMapWidth  = iCols;
         pCTW->iShadowWidth = hb_ctw_CalcShadowWidth( pCTW->iMapHeight, pCTW->iMapWidth );
         ulSize = ( ULONG ) pCTW->iMapHeight * pCTW->iMapWidth * sizeof( int );
         pCTW->pWindowMap = ( int * ) hb_xrealloc( pCTW->pWindowMap, ulSize );
         pCTW->pShadowMap = ( int * ) hb_xrealloc( pCTW->pShadowMap, ulSize );
      }
      if( pCTW->fBoardSet )
         hb_ctw_SetWindowBoard( pCTW, 0, 0, iRows - 1, iCols - 1 );
      return TRUE;
   }
   return FALSE;
}

static BOOL hb_ctw_gt_Info( PHB_GT pGT, int iType, PHB_GT_INFO pInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_Info(%p,%d,%p)", pGT, iType, pInfo));

   switch ( iType )
   {
      case HB_GTI_ISCTWIN:
         pInfo->pResult = hb_itemPutL( pInfo->pResult, TRUE );
         break;

      case HB_GTI_NEWWIN:
      {
         BOOL fResult;

         hb_ctw_SelectWindow( HB_GTCTW_GET( pGT ), 0 );
         fResult = HB_GTSUPER_INFO( pGT, iType, pInfo );

         if( fResult && hb_arrayLen( pInfo->pResult ) >= 8 )
            hb_itemPutNI( hb_arrayGetItemPtr( pInfo->pResult, 8 ),
                          HB_GTCTW_GET( pGT )->iCurrWindow );
         return fResult;
      }
      case HB_GTI_GETWIN:
      {
         BOOL fResult;
         int iWindow = HB_GTCTW_GET( pGT )->iCurrWindow;

         hb_ctw_SelectWindow( HB_GTCTW_GET( pGT ), 0 );
         fResult = HB_GTSUPER_INFO( pGT, iType, pInfo );
         if( fResult && hb_arrayLen( pInfo->pResult ) >= 8 )
            hb_itemPutNI( hb_arrayGetItemPtr( pInfo->pResult, 8 ), iWindow );
         return fResult;
      }
      case HB_GTI_SETWIN:
      {
         BOOL fResult;

         hb_ctw_SelectWindow( HB_GTCTW_GET( pGT ), 0 );
         fResult = HB_GTSUPER_INFO( pGT, iType, pInfo );
         if( hb_arrayLen( pInfo->pNewVal ) == 8 )
            hb_ctw_SelectWindow( HB_GTCTW_GET( pGT ),
                                 hb_arrayGetNI( pInfo->pNewVal, 8 ) );
         return fResult;
      }
      default:
         return HB_GTSUPER_INFO( pGT, iType, pInfo );
   }

   return TRUE;
}

static int hb_ctw_gt_Alert( PHB_GT pGT, PHB_ITEM pMessage, PHB_ITEM pOptions,
                            int iClrNorm, int iClrHigh, double dDelay )
{
   int iOptions, iRet = 0;

   HB_TRACE( HB_TR_DEBUG, ("hb_ctw_gt_Alert(%p,%p,%p,%d,%d,%f)", pGT, pMessage, pOptions, iClrNorm, iClrHigh, dDelay ) );

   iOptions = ( int ) hb_arrayLen( pOptions );

   if( HB_IS_STRING( pMessage ) && iOptions > 0 )
   {
      int iRows, iCols;
      BOOL fScreen;

      HB_GTSELF_GETSIZE( pGT, &iRows, &iCols );
      if( iCols <= 4 || iRows <= 4 )
         fScreen = FALSE;
      else
      {
         HB_GT_INFO gtInfo;
         gtInfo.pNewVal = gtInfo.pResult = NULL;
         HB_GTSELF_INFO( pGT, HB_GTI_FULLSCREEN, &gtInfo );
         fScreen = gtInfo.pResult && hb_itemGetL( gtInfo.pResult );
         HB_GTSELF_INFO( pGT, HB_GTI_KBDSUPPORT, &gtInfo );
         if( gtInfo.pResult )
         {
            if( !hb_itemGetL( gtInfo.pResult ) )
               fScreen = FALSE;
            hb_itemRelease( gtInfo.pResult );
         }
      }
      if( fScreen )
      {
         PHB_GTCTW pCTW = HB_GTCTW_GET( pGT );
         ULONG ulWidth = 0, ulCurrWidth = 0, ul = 0, ul2, ulMaxWidth, ulLast;
         int iKey, iDspCount, iLines = 0, iTop, iLeft, iBottom, iRight,
             iMnuCol, iPos, iClr, iWnd, i;
         char * szMessage = hb_itemGetCPtr( pMessage );
         ULONG ulLen = hb_itemGetCLen( pMessage );

         ulMaxWidth = iCols - 4;
         while( ul < ulLen )
         {
            if( szMessage[ ul ] == '\n' )
            {
               ++iLines;
               if( ulCurrWidth > ulWidth )
                  ulWidth = ulCurrWidth;
               ulCurrWidth = 0;
            }
            else
               ++ulCurrWidth;
            ++ul;
         }
         if( ulCurrWidth )
            ++iLines;
         if( ulCurrWidth > ulWidth )
            ulWidth = ulCurrWidth;
         ulCurrWidth = 0;
         for( i = 1; i <= iOptions; ++i )
         {
            ulCurrWidth += hb_arrayGetCLen( pOptions, i ) + ( i > 1 ? 3 : 0 );
         }
         if( ulCurrWidth > ulWidth )
            ulWidth = ulCurrWidth;
         if( ulWidth > ulMaxWidth )
            ulWidth = ulMaxWidth;
         if( iRows < iLines + 4 )
            iLines = iRows - 4;
         iTop = ( iRows - iLines - 4 ) >> 1;
         iLeft = ( iCols - ulWidth - 4 ) >> 1;
         iBottom = iTop + iLines + 3;
         iRight = iLeft + ulWidth + 3;
         if( iClrNorm == 0 )
            iClrNorm = 79;
         if( iClrHigh == 0 )
            iClrHigh = 31;

         iDspCount = HB_GTSELF_DISPCOUNT( pGT );
         if( iDspCount == 0 )
            HB_GTSELF_DISPBEGIN( pGT );

         iWnd = hb_ctw_CreateWindow( pCTW, iTop, iLeft, iBottom, iRight, TRUE, iClrNorm );
         hb_ctw_AddWindowBox( pCTW, iWnd, ( BYTE * ) _B_SINGLE, iClrNorm );
         HB_GTSELF_SETCURSORSTYLE( pGT, SC_NONE );
         ulLast = 0;
         i = 0;
         for( ul = 0; ul < ulLen; ++ul )
         {
            if( szMessage[ ul ] == '\n' )
            {
               if( ul > ulLast )
               {
                  ul2 = ul - ulLast;
                  if( ul2 > ulWidth )
                     ul2 = ulWidth;
                  HB_GTSELF_PUTTEXT( pGT, i, ( ( ulWidth - ul2 + 1 ) >> 1 ) + 1, ( BYTE ) iClrNorm,
                                     ( BYTE * ) szMessage + ulLast, ul2 );
               }
               ulLast = ul + 1;
               if( ++i >= iLines )
                  break;
            }
         }
         if( ul > ulLast && i < iLines )
         {
            ul2 = ul - ulLast;
            if( ul2 > ulWidth )
               ul2 = ulWidth;
            HB_GTSELF_PUTTEXT( pGT, i, ( ( ulWidth - ul2 + 1 ) >> 1 ) + 1, ( BYTE ) iClrNorm,
                               ( BYTE * ) szMessage + ulLast, ul2 );
         }

         iPos = 1;
         while( iRet == 0 )
         {
            HB_GTSELF_DISPBEGIN( pGT );
            iMnuCol = ( ( ulWidth - ulCurrWidth ) >> 1 ) + 1;
            for( i = 1; i <= iOptions; ++i )
            {
               iClr = i == iPos ? iClrHigh : iClrNorm;
               ulLen = hb_arrayGetCLen( pOptions, i );
               HB_GTSELF_PUTTEXT( pGT, iLines + 1, iMnuCol, ( BYTE ) iClr,
                                  ( BYTE * ) hb_arrayGetCPtr( pOptions, i ), ulLen );
               iMnuCol += ulLen + 3;
            }
            while( HB_GTSELF_DISPCOUNT( pGT ) )
               HB_GTSELF_DISPEND( pGT );
            HB_GTSELF_REFRESH( pGT );

            iKey = HB_GTSELF_INKEYGET( pGT, TRUE, dDelay, INKEY_ALL );
            /* TODO: add support for SET KEY blocks */

            if( iKey == K_ESC )
               break;
            else if( iKey == K_ENTER || iKey == K_SPACE || iKey == 0 )
            {
               iRet = iPos;
            }
            else if( iKey == K_LEFT || iKey == K_SH_TAB )
            {
               if( --iPos == 0 )
                  iPos = iOptions;
               dDelay = 0.0;
            }
            else if( iKey == K_RIGHT || iKey == K_TAB )
            {
               if( ++iPos > iOptions )
                  iPos = 1;
               dDelay = 0.0;
            }
#ifdef HB_COMPAT_C53
            else if( iKey == K_LBUTTONDOWN )
            {
               int iMRow = HB_GTSELF_MOUSEROW( pGT ),
                   iMCol = HB_GTSELF_MOUSECOL( pGT );
               if( iMRow == iLines + 1 )
               {
                  iMnuCol = ( ( ulWidth - ulCurrWidth ) >> 1 ) + 1;
                  for( i = 1; i <= iOptions; ++i )
                  {
                     ulLen = hb_arrayGetCLen( pOptions, i );
                     if( iMCol >= iMnuCol && iMCol < iMnuCol + ( int ) ulLen )
                     {
                        iRet = i;
                        break;
                     }
                     iMnuCol += ulLen + 3;
                  }
               }
            }
#endif
            else if( iKey >= 32 && iKey <= 255 )
            {
               int iUp = hb_charUpper( iKey );
               for( i = 1; i <= iOptions; ++i )
               {
                  char *szValue = hb_arrayGetCPtr( pOptions, i );
                  if( szValue && iUp == hb_charUpper( *szValue ) )
                  {
                     iRet = i;
                     break;
                  }
               }
            }
         }

         hb_ctw_CloseWindow( pCTW, iWnd );
         HB_GTSELF_REFRESH( pGT );

         while( HB_GTSELF_DISPCOUNT( pGT ) < iDspCount )
            HB_GTSELF_DISPBEGIN( pGT );

         return iRet;
      }
   }

   return HB_GTSUPER_ALERT( pGT, pMessage, pOptions, iClrNorm, iClrHigh, dDelay );
}

static int hb_ctw_gt_ReadKey( PHB_GT pGT, int iEventMask )
{
   int iKey;

   HB_TRACE(HB_TR_DEBUG, ("hb_ctw_gt_ReadKey(%p,%d)", pGT, iEventMask));

   iKey = HB_GTSUPER_READKEY( pGT, iEventMask );

   if( iKey != 0 )
      HB_GTCTW_GET( pGT )->iLastKey = iKey;

   return iKey;
}

/* PUBLIC FUNCTIONS */

BOOL hb_ctwInit( void )
{
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
      hb_gt_BaseFree( pCTW->pGT );
   return pCTW != NULL;
}

int  hb_ctwSetShadowAttr( int iAttr )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_SetShadowAttr( pCTW, iAttr );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwSetMoveMode( int iMode )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_SetMoveMode( pCTW, iMode );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwSetMoveStep( int iVertical, int iHorizontal )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_SetMoveStep( pCTW, iVertical, iHorizontal );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwSetWindowBoard( int iTop, int iLeft, int iBottom, int iRight )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_SetWindowBoard( pCTW, iTop, iLeft, iBottom, iRight );
      HB_GTSELF_FLUSH( pCTW->pGT );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwSetBorderMode( int iTop, int iLeft, int iBottom, int iRight )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_SetBorderMode( pCTW, iTop, iLeft, iBottom, iRight );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwCreateWindow( int iTop, int iLeft, int iBottom, int iRight, BOOL fClear, int iColor )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_CreateWindow( pCTW, iTop, iLeft, iBottom, iRight, fClear, iColor );
      HB_GTSELF_FLUSH( pCTW->pGT );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwCloseAllWindows( void )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_CloseAllWindows( pCTW );
      HB_GTSELF_FLUSH( pCTW->pGT );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwCloseWindow( int iWindow )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_CloseWindow( pCTW, iWindow );
      HB_GTSELF_FLUSH( pCTW->pGT );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwCurrentWindow( void )
{
   int iResult = 0;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_CurrentWindow( pCTW );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwSelectWindow( int iWindow )
{
   int iResult = 0;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_SelectWindow( pCTW, iWindow );
      HB_GTSELF_FLUSH( pCTW->pGT );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwMaxWindow( void )
{
   int iResult = 0;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_MaxWindow( pCTW );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwChangeMargins( int iWindow, int iTop, int iLeft, int iBottom, int iRight )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_ChangeMargins( pCTW, iWindow, iTop, iLeft, iBottom, iRight );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwGetWindowCords( int iWindow, BOOL fCenter, int * piTop, int * piLeft, int * piBottom, int * piRight )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_GetWindowCords( pCTW, iWindow, fCenter, piTop, piLeft, piBottom, piRight );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwGetFormatCords( int iWindow, BOOL fRelative, int * piTop, int * piLeft, int * piBottom, int * piRight )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_GetFormatCords( pCTW, iWindow, fRelative, piTop, piLeft, piBottom, piRight );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwMoveWindow( int iWindow, int iRow, int iCol )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_MoveWindow( pCTW, iWindow, iRow, iCol );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwCenterWindow( int iWindow, BOOL fCenter )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_CenterWindow( pCTW, iWindow, fCenter );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwAddWindowBox( int iWindow, BYTE * szBox, int iColor )
{
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = hb_ctw_AddWindowBox( pCTW, iWindow, szBox, iColor );
      HB_GTSELF_FLUSH( pCTW->pGT );
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

int  hb_ctwLastKey( void )
{
   /* keyread() in CT3 uses 64512 bytes length buffer
    * when it reach this limit and new key is added the
    * buffer size is decreased by 1024 to 63488 bytes
    * before adding key. TODO: check if buffer is shifted
    */
   int iResult = -1;
   PHB_GTCTW pCTW = hb_ctw_base();
   if( pCTW )
   {
      iResult = pCTW->iLastKey;
      hb_gt_BaseFree( pCTW->pGT );
   }
   return iResult;
}

static BOOL hb_gt_FuncInit( PHB_GT_FUNCS pFuncTable )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_FuncInit(%p)", pFuncTable));

   pFuncTable->Init                       = hb_ctw_gt_Init;
   pFuncTable->Exit                       = hb_ctw_gt_Exit;
   pFuncTable->MouseRow                   = hb_ctw_MouseRow;
   pFuncTable->MouseCol                   = hb_ctw_MouseCol;
   pFuncTable->MaxCol                     = hb_ctw_gt_MaxCol;
   pFuncTable->MaxRow                     = hb_ctw_gt_MaxRow;
   pFuncTable->GetPos                     = hb_ctw_gt_GetPos;
   pFuncTable->SetPos                     = hb_ctw_gt_SetPos;
   pFuncTable->WriteCon                   = hb_ctw_gt_WriteCon;
   pFuncTable->GetCursorStyle             = hb_ctw_gt_GetCursorStyle;
   pFuncTable->SetCursorStyle             = hb_ctw_gt_SetCursorStyle;
   pFuncTable->GetColorStr                = hb_ctw_gt_GetColorStr;
   pFuncTable->SetColorStr                = hb_ctw_gt_SetColorStr;
   pFuncTable->ColorSelect                = hb_ctw_gt_ColorSelect;
   pFuncTable->GetColor                   = hb_ctw_gt_GetColor;
   pFuncTable->GetColorData               = hb_ctw_gt_GetColorData;
   pFuncTable->GetScrCursor               = hb_ctw_gt_GetScrCursor;
   pFuncTable->GetScrChar                 = hb_ctw_gt_GetScrChar;
   pFuncTable->GetChar                    = hb_ctw_gt_GetChar;
   pFuncTable->PutChar                    = hb_ctw_gt_PutChar;
   pFuncTable->Resize                     = hb_ctw_gt_Resize;
   pFuncTable->Info                       = hb_ctw_gt_Info;
   pFuncTable->Alert                      = hb_ctw_gt_Alert;
   pFuncTable->ReadKey                    = hb_ctw_gt_ReadKey;

   return TRUE;
}

/* ********************************************************************** */

static const HB_GT_INIT gtInit = { HB_GT_DRVNAME( HB_GT_NAME ),
                                   hb_gt_FuncInit,
                                   HB_GTSUPER,
                                   HB_GTID_PTR };

HB_GT_ANNOUNCE( HB_GT_NAME )

HB_CALL_ON_STARTUP_BEGIN( _hb_startup_gt_Init_ )
   hb_gtRegister( &gtInit );
HB_CALL_ON_STARTUP_END( _hb_startup_gt_Init_ )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup _hb_startup_gt_Init_
#elif defined( HB_MSC_STARTUP )
   #if defined( HB_OS_WIN_64 )
      #pragma section( HB_MSC_START_SEGMENT, long, read )
   #endif
   #pragma data_seg( HB_MSC_START_SEGMENT )
   static HB_$INITSYM hb_vm_auto__hb_startup_gt_Init_ = _hb_startup_gt_Init_;
   #pragma data_seg()
#endif

/* *********************************************************************** */
