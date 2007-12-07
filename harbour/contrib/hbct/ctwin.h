/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for Clipper Tools like window system
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

#ifndef HB_CTWIN_H_
#define HB_CTWIN_H_

HB_EXTERN_BEGIN

extern BOOL hb_ctwInit( void );
extern int  hb_ctwSetShadowAttr( int iAttr );
extern int  hb_ctwSetMoveMode( int iMode );
extern int  hb_ctwSetMoveStep( int iVertical, int iHorizontal );
extern int  hb_ctwSetWindowBoard( int iTop, int iLeft, int iBottom, int iRight );
extern int  hb_ctwSetBorderMode( int iTop, int iLeft, int iBottom, int iRight );
extern int  hb_ctwCreateWindow( int iTop, int iLeft, int iBottom, int iRight, BOOL fClear, int iColor );
extern int  hb_ctwCloseAllWindows( void );
extern int  hb_ctwCloseWindow( int iWindow );
extern int  hb_ctwCurrentWindow( void );
extern int  hb_ctwSelectWindow( int iWindow );
extern int  hb_ctwMaxWindow( void );
extern int  hb_ctwChangeMargins( int iWindow, int iTop, int iLeft, int iBottom, int iRight );
extern int  hb_ctwGetWindowCords( int iWindow, BOOL fCenter, int * piTop, int * piLeft, int * piBottom, int * piRight );
extern int  hb_ctwGetFormatCords( int iWindow, BOOL fRelative, int * piTop, int * piLeft, int * piBottom, int * piRight );
extern int  hb_ctwMoveWindow( int iWindow, int iRow, int iCol );
extern int  hb_ctwCenterWindow( int iWindow, BOOL fCenter );
extern int  hb_ctwAddWindowBox( int iWindow, BYTE * szBox, int iColor );

HB_EXTERN_END

#endif /* HB_CTWIN_H_ */
