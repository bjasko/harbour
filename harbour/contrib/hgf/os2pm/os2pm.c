/*
 * $Id$
 */


/*
 * Harbour Project source code:
 * Harbour GUI framework for IBM OS/2 Presentation Manager
 *
 * Copyright 2001 Antonio Linares <alinares@fivetech.com>
 * Copyright 2001 Maurilio Longo <maurilio.longo@libero.it>
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



#define INCL_BASE
#define INCL_PM
#include <os2.h>

#include "hbapi.h"
#include "hbvm.h"

#define  LOWORD(l)   ((USHORT)l)
#define  HIWORD(l)   ((USHORT)((ULONG)l >> 16))

HAB hb_pm_GetHab( void );
MRESULT EXPENTRY WndProc( HWND, ULONG, MPARAM, MPARAM );


MRESULT EXPENTRY WndProc( HWND hWnd, ULONG Msg, MPARAM mp1, MPARAM mp2 )
{
   static PHB_DYNS pDynSym = 0;
   MRESULT mResult;
   HPS hps;

   if( ! pDynSym )
      pDynSym = hb_dynsymFind( "HB_GUI" );

   switch (Msg)
   {
      case WM_PAINT:
         hps = WinBeginPaint( hWnd, 0L, NULL );
         GpiErase( hps );
         WinEndPaint( hps );
         return 0;

      default:
         hb_vmPushState();
         hb_vmPushSymbol( hb_dynsymSymbol( pDynSym ) );
         hb_vmPushNil();
         hb_vmPushLong( (LONG) hWnd );
         hb_vmPushLong( (LONG) Msg );
         hb_vmPushLong( (LONG) mp1 );
         hb_vmPushLong( (LONG) mp2 );
         hb_vmDo( 4 );
         if( hb_arrayGetType( hb_param( -1, HB_IT_ANY ), 1 ) == HB_IT_NIL )
            mResult = (MRESULT) WinDefWindowProc( hWnd, Msg, mp1, mp2 );
         else
            mResult = (MRESULT) hb_parnl( -1, 1 );
         hb_vmPopState();

         return mResult;
   }

}


HB_FUNC( WINREGISTERCLASS )
{

   hb_retl( WinRegisterClass( hb_pm_GetHab(),               /* anchor block handle */
                              hb_parc( 1 ),                 /* Class Name */
                              ( PFNWP ) WndProc,            /* default Class procedure */
                              hb_parnl( 2 ),                /* style */
                              hb_parnl( 3 ) ) );            /* extra bytes */
}


HB_FUNC(WINCREATEWINDOW)
{
   HWND hwnd;

   hwnd =  WinCreateWindow( (HWND) hb_parnl(1),             /* hWnd parent  */
                            (PCSZ) hb_parc(2),              /* pszClass     */
                            (PCSZ) hb_parc(3),              /* pszName      */
                            (ULONG) hb_parnl(4),            /* flStyle      */
                            (LONG) hb_parnl(5),             /* x            */
                            (LONG) hb_parnl(6),             /* y            */
                            (LONG) hb_parnl(7),             /* cx           */
                            (LONG) hb_parnl(8),             /* cy           */
                            (HWND) hb_parnl(9),             /* hwndOwner    */
                            (HWND) hb_parnl(10),            /* hwndInsertBehind */
                            (ULONG) hb_parnl(11),           /* id           */
                            (PVOID) hb_parnl(12),           /* pCtlData,    */
                            (PVOID) hb_parnl(13));          /* pPresParams  */

   hb_retnl((LONG) hwnd);
}


HB_FUNC( WINCREATESTDWINDOW )
{
   ULONG lFrame = hb_parnl( 3 );
   HWND hWndClient, hWndFrame;

   hWndFrame = WinCreateStdWindow( ( HWND ) hb_parnl( 1 ),              /* hWndParent */
                                    hb_parnl( 2 ),                      /* style */
                                    &lFrame,                            /* lFrame */
                                    hb_parc( 4 ),                       /* cClassName */
                                    hb_parc( 5 ),                       /* cCaption */
                                    hb_parnl( 6 ),                      /* lStyleClient */
                                    hb_parnl( 7 ),                      /* hModule */
                                    hb_parnl( 8 ),                      /* nId */
                                    ( PHWND ) &hWndClient );            /* Window client handle */

   hb_retnl( (LONG) hWndFrame);
   hb_stornl( ( LONG ) hWndClient, 9 );
}


HB_FUNC( HB_FORMSHOWMODAL )
{
   QMSG qmsg;
   HAB  hab = hb_pm_GetHab();

   WinShowWindow( ( HWND ) hb_parnl( 1 ), 1 );
   while( WinGetMsg( hab, &qmsg, 0, 0, 0 ) )
   {
      WinDispatchMsg( hab, &qmsg );
   }
}


/* nOr() is a very used function */
HB_FUNC( NOR )
{
   LONG lRet = 0;
   USHORT i = 0;

   while( i < hb_pcount() )
      lRet = lRet | hb_parnl( ++i );

   hb_retnl( lRet );
}


HB_FUNC( WINSETWINDOWTEXT )
{
   hb_retl( WinSetWindowText( ( HWND ) hb_parnl( 1 ), hb_parc( 2 ) ) );
}


HB_FUNC( WINGETTEXT )
{
   BYTE bBuffer[ 255 ];

   WinQueryWindowText( ( HWND ) hb_parnl( 1 ), 254, bBuffer );
   hb_retc( bBuffer );
}


HB_FUNC( MSGINFO )
{
   HWND hWnd = WinQueryActiveWindow( HWND_DESKTOP);
   PSZ szCaption = ( hb_pcount() > 1 && ISCHAR( 2 ) ? hb_parc( 2 ) : "Information");

   hb_retnl( WinMessageBox( HWND_DESKTOP, hWnd, hb_parc( 1 ), szCaption,
             0, MB_INFORMATION | MB_OK | MB_MOVEABLE | MB_APPLMODAL ) );
}


HAB hb_pm_GetHab() {

   HWND hWnd = WinQueryActiveWindow( HWND_DESKTOP);
   return WinQueryAnchorBlock(hWnd);

}


HB_FUNC( GETHAB )
{
   hb_retnl( ( LONG ) hb_pm_GetHab() );
}


HB_FUNC( WINCREATEMENU )
{
   hb_retnl( (LONG) WinCreateMenu((HWND) hb_parnl(1), (PVOID) NULL));

}


/* Some xBase for C language */
#define IF(x,y,z) ((x)?(y):(z))


HB_FUNC( WINADDMENUITEM )
{
   MENUITEM mit;

   mit.iPosition   = hb_parni( 3 );
   mit.afStyle     = IF( ISCHAR( 2 ), MIS_TEXT, MIS_SEPARATOR );
   mit.afAttribute = IF( ! hb_parl( 6 ), MIA_DISABLED, 0 );
   mit.id          = hb_parni( 5 );
   mit.hwndSubMenu = hb_parnl( 4 );
   mit.hItem       = 0;

   hb_retni( ( LONG ) WinSendMsg( ( HWND ) hb_parnl( 1 ), MM_INSERTITEM,
                                  &mit, ( MPARAM ) hb_parc( 2 ) ) );
}


// Given an id of a menuitem changes it to a MIS_SUBMENU type of menu item
// NOTE: You have to delete and reinsert a menu item if you want to change it from
//       a MIS_TEXT to a MIS_SUBMENU type of menuitem
HB_FUNC( WINMAKESUBMENUITEM )
{
   MENUITEM mit;
   MRESULT rc;
   char text[100];

   rc = WinSendMsg((HWND) hb_parnl(1), MM_QUERYITEM, MPFROM2SHORT(hb_parni(2), FALSE ), &mit );
   if ((BOOL)rc) {
      WinSendMsg((HWND) hb_parnl(1), MM_QUERYITEMTEXT, MPFROM2SHORT(hb_parni(2), 100), &text );
      WinSendMsg((HWND) hb_parnl(1), MM_DELETEITEM, MPFROM2SHORT(hb_parni(2), FALSE ), 0L );

      mit.hwndSubMenu = hb_parnl(3);
      mit.afStyle |= MIS_SUBMENU;

      // re-insert the menuitem
      WinSendMsg((HWND) hb_parnl(1), MM_INSERTITEM, &mit, &text);
   }
   hb_retnl(0);
}


HB_FUNC( WINSETPARENT )
{
   hb_retl( WinSetParent( ( HWND ) hb_parnl( 1 ), ( HWND ) hb_parnl( 2 ),
                           hb_parl( 3 ) ) );
}


HB_FUNC( WINSETOWNER )
{
   hb_retl( WinSetOwner( ( HWND ) hb_parnl( 1 ), ( HWND ) hb_parnl( 2 ) ) );
}


HB_FUNC( WINSENDMSG )
{
   hb_retnl( ( LONG ) WinSendMsg( ( HWND ) hb_parnl( 1 ), hb_parnl( 2 ),
             ( MPARAM ) IF( ISCHAR( 3 ), (ULONG) hb_parc( 3 ), hb_parnl( 3 ) ),
             ( MPARAM ) IF( ISCHAR( 4 ), (ULONG) hb_parc( 4 ), hb_parnl( 4 ) ) ) );
}


HB_FUNC( WINGETWIDTH )
{
   HWND hWnd = ( HWND ) hb_parnl( 1 );
   RECTL rcWnd;

   WinQueryWindowRect( hWnd, &rcWnd );
   hb_retnl( rcWnd.xRight - rcWnd.xLeft );
}


HB_FUNC( WINSETWIDTH )
{
   HWND hWnd = ( HWND ) hb_parnl( 1 );
   SWP swp;

   WinQueryWindowPos( hWnd, &swp );
   WinSetWindowPos( hWnd, HWND_TOP, swp.x,
                    swp.y, (LONG) hb_parnl( 2 ), swp.cy,
                    SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ZORDER );
}


HB_FUNC( SENDMESSAGE )
{
   hb_retnl( (LONG) WinSendMsg(
                       (HWND) hb_parnl( 1 ), // handle of destination window
                       (ULONG) hb_parnl( 2 ), // message to send
                       (MPARAM) hb_parnl( 3 ),  // first message parameter
                       (MPARAM) hb_parnl( 4 )   // second message parameter
                     ) );
}


HB_FUNC( NLOWORD )
{
   hb_retnl( LOWORD( hb_parnl( 1 ) ) );
}


HB_FUNC( NHIWORD )
{
   hb_retnl( HIWORD( hb_parnl( 1 ) ) );
}


HB_FUNC( WINGETHEIGHT )
{
   HWND hWnd = ( HWND ) hb_parnl( 1 );
   RECTL rct;

   WinQueryWindowRect( hWnd, &rct );

   hb_retnl( rct.yBottom - rct.yTop );
}


HB_FUNC( WINSETHEIGHT )
{
   HWND hWnd = ( HWND ) hb_parnl( 1 );
   SWP swp;

   WinQueryWindowPos( hWnd, &swp );
   WinSetWindowPos( hWnd, HWND_TOP, swp.x,
                    swp.y, swp.cx, hb_parnl( 2 ),
                    SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ZORDER );
}


HB_FUNC( WINGETTOP )
{
   HWND hWnd = ( HWND ) hb_parnl( 1 );
   RECTL rct;

   WinQueryWindowRect( hWnd, &rct );

   hb_retnl( rct.yTop );
}


HB_FUNC( WINSETTOP )
{
   HWND hWnd = ( HWND ) hb_parnl( 1 );
   SWP swp;

   WinQueryWindowPos( hWnd, &swp );
   WinSetWindowPos( hWnd, HWND_TOP, hb_parnl( 2 ),
                    swp.y, swp.cx, swp.cy,
                    SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ZORDER );
}


HB_FUNC( WINGETLEFT )
{
   HWND hWnd = ( HWND ) hb_parnl( 1 );
   RECTL rct;

   WinQueryWindowRect( hWnd, &rct );

   hb_retnl( rct.xLeft );
}


HB_FUNC( WINSETLEFT )
{
   HWND hWnd = ( HWND ) hb_parnl( 1 );
   SWP swp;

   WinQueryWindowPos( hWnd, &swp );
   WinSetWindowPos( hWnd, HWND_TOP, swp.x,
                    hb_parnl( 2 ), swp.cx, swp.cy,
                    SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ZORDER );
}


HB_FUNC( POSTQUITMESSAGE )
{
   WinPostMsg((HWND) hb_parnl( 1 ), WM_QUIT, 0L, 0L);
   //PostQuitMessage( hb_parnl( 1 ) );
}


HB_FUNC( SHOWWINDOW )
{
   hb_retl( WinShowWindow( ( HWND ) hb_parnl( 1 ), hb_parl( 2 ) ) );
}


HB_FUNC( WINGETLASTERROR )
{
   hb_retnl((LONG) WinGetLastError( hb_pm_GetHab() ) );
}
