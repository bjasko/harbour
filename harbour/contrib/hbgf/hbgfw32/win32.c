/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Harbour GUI framework for Win32
 *
 * Copyright 2001 Alexander S.Kresin <alex@belacy.belgorod.su>
 * Copyright 2001 Antonio Linares <alinares@fivetech.com>
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

#define _WIN32_WINNT 0x0400

#define HB_OS_WIN_32_USED

#include "hbapi.h"
#include "hbvm.h"
#include "hbstack.h"

LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

HB_FUNC( WINREGISTERCLASS )
{
   WNDCLASS     wndclass;
   LPTSTR       lpszClassName = HB_TCHAR_CONVTO( hb_parcx( 1 ) );

   wndclass.lpszClassName = lpszClassName;
   wndclass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;   // hb_parnl( 2 );
   wndclass.cbClsExtra    = hb_parnl( 3 );
   wndclass.cbWndExtra    = 0;
   wndclass.lpfnWndProc   = WndProc;
   wndclass.hInstance     = GetModuleHandle( NULL );
   wndclass.hIcon         = LoadIcon ( NULL, IDI_APPLICATION );
   wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH)( COLOR_BTNFACE + 1 );
   wndclass.lpszMenuName  = NULL;

   hb_retl( RegisterClass (&wndclass) );

   HB_TCHAR_FREE( lpszClassName );
}

HB_FUNC( WINCREATESTDWINDOW )
{
   LPTSTR lpszClassName = HB_TCHAR_CONVTO( hb_parcx( 4 ) );
   LPTSTR lpszCaption   = HB_TCHAR_CONVTO( hb_parcx( 5 ) );

   hb_retptr( CreateWindow( lpszClassName,
                            lpszCaption,
                            hb_parnl( 2 ),           /* style */
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            ( HWND ) hb_parptr( 7 ),  /* hWndParent */
                            ( HMENU ) hb_parptr( 8 ), /* hMenu or nId */
                            GetModuleHandle( NULL ), NULL) );

   HB_TCHAR_FREE( lpszClassName );
   HB_TCHAR_FREE( lpszCaption );
}

HB_FUNC( HB_FORMSHOWMODAL )
{
   MSG  msg;

   ShowWindow( ( HWND ) hb_parptr( 1 ), 1 );
   while( GetMessage( &msg, NULL, 0, 0 ) )
   {
       TranslateMessage( &msg );
      DispatchMessage( &msg );
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
   LPTSTR lpszText = HB_TCHAR_CONVTO( hb_parcx( 1 ) );

   hb_retl( SetWindowText( (HWND) hb_parptr( 1 ), lpszText ) );
   
   HB_TCHAR_FREE( lpszText );
}


HB_FUNC( WINGETTEXT )
{
   TCHAR bBuffer[ 256 ];
   char * szText;

   GetWindowText( ( HWND ) hb_parptr( 1 ), bBuffer, sizeof( bBuffer ) - 1 );

   szText = HB_TCHAR_CONVFROM( bBuffer );
   hb_retc( szText );
   HB_TCHAR_FREE( szText );
}


HB_FUNC( MSGINFO )
{
   LPTSTR lpStr1 = HB_TCHAR_CONVTO( hb_parcx( 2 ) ),
          lpStr2 = HB_TCHAR_CONVTO( ISCHAR( 2 ) ? hb_parc( 3 ) : ( char * ) "Information" );

   hb_retni( MessageBox( GetActiveWindow(), lpStr1, lpStr2, MB_OK | MB_ICONINFORMATION ) );

   HB_TCHAR_FREE( lpStr1 );
   HB_TCHAR_FREE( lpStr2 );
}


HB_FUNC( WINCREATEMENU )
{
   hb_retptr( CreateMenu() );
}


/* Some xBase for C language */
#define IIF(x,y,z) ((x)?(y):(z))


HB_FUNC( WINADDMENUITEM )
{
   LPTSTR lpszText = NULL;
   MENUITEMINFO mii;
   HMENU hSubMenu = ( !ISNIL(4) )? (HMENU) hb_parptr( 4 ):0;

   mii.cbSize = sizeof( MENUITEMINFO );
   mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID | ((hSubMenu)? MIIM_SUBMENU:0);
   mii.fState = IIF( ! hb_parl( 6 ), MFS_DISABLED, 0 );
   mii.wID = hb_parni( 5 );
   mii.hSubMenu = hSubMenu;
   if( ISCHAR( 2 ) )
   {
      lpszText = HB_TCHAR_CONVTO( hb_parc( 1 ) );
      mii.dwTypeData = lpszText;
      mii.cch = lstrlen( lpszText );
      mii.fType = MFT_STRING;
   }
   else
      mii.fType = MFT_SEPARATOR;

   hb_retl( InsertMenuItem( ( HMENU ) hb_parptr( 1 ), hb_parni( 3 ), 1, &mii ) );
   if( lpszText )
      HB_TCHAR_FREE( lpszText );
}

HB_FUNC( WINCREATESUBMENU )
{

   MENUITEMINFO mii;
   HMENU hSubMenu = CreateMenu();

   mii.cbSize = sizeof( MENUITEMINFO );
   mii.fMask = MIIM_SUBMENU;
   mii.hSubMenu = hSubMenu;

   SetMenuItemInfo( ( HMENU ) hb_parptr( 1 ),
     hb_parni( 2 ),
     0,
     &mii );

   hb_retptr( hSubMenu );
}

HB_FUNC( SETMENU )
{
   hb_retl( SetMenu( ( HWND ) hb_parptr( 1 ), ( HMENU ) hb_parptr( 2 ) ) );
}

HB_FUNC( SENDMESSAGE )
{
   hb_retnl( (LONG) SendMessage(
                       (HWND) hb_parptr( 1 ),   // handle of destination window
                       (UINT) hb_parni( 2 ),    // message to send
                       (WPARAM) hb_parnl( 3 ),  // first message parameter
                       (LPARAM) hb_parnl( 4 )   // second message parameter
                     ) );
}


LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   static PHB_DYNS pDynSym = NULL;

   if( ! pDynSym )
      pDynSym = hb_dynsymFindName( "HB_GUI" );

   if( pDynSym )
   {
      hb_vmPushSymbol( hb_dynsymSymbol( pDynSym ) );
      hb_vmPushNil();
      hb_vmPushPointer( hWnd );
      hb_vmPushLong( message );
      hb_vmPushLong( wParam );
      hb_vmPushLong( lParam );
      hb_vmDo( 4 );

      if( hb_arrayGetType( hb_stackReturnItem(), 1 ) == HB_IT_NIL )
         return DefWindowProc( ( HWND ) hWnd, message, wParam, lParam );
   }

   return hb_parnl( -1, 1 );
}

HB_FUNC( POSTQUITMESSAGE )
{
   PostQuitMessage( hb_parnl( 1 ) );
}

HB_FUNC( NLOWORD )
{
   hb_retnl( LOWORD( hb_parnl( 1 ) ) );
}

HB_FUNC( NHIWORD )
{
   hb_retnl( HIWORD( hb_parnl( 1 ) ) );
}

HB_FUNC( WINGETWIDTH )
{
   HWND hWnd = ( HWND ) hb_parptr( 1 );
   RECT rct;

   GetWindowRect( hWnd, &rct );

   hb_retnl( rct.right - rct.left );
}

HB_FUNC( WINSETWIDTH )
{
   HWND hWnd = ( HWND ) hb_parptr( 1 );
   RECT rct;
   POINT pt;
   WORD wHeight;

   GetWindowRect( hWnd, &rct );
   wHeight = (WORD) ( rct.bottom - rct.top );

   if( GetWindowLong( hWnd, GWL_STYLE ) && WS_CHILD )
   {
      pt.x = rct.left;
      pt.y = rct.top;
      ScreenToClient( GetParent( hWnd ), &pt );
      rct.left = pt.x;
      rct.top  = pt.y;
   }

   MoveWindow( hWnd, rct.left, rct.top, hb_parnl( 2 ), wHeight, TRUE );
}

HB_FUNC( WINGETHEIGHT )
{
   HWND hWnd = ( HWND ) hb_parptr( 1 );
   RECT rct;

   GetWindowRect( hWnd, &rct );

   hb_retnl( rct.bottom - rct.top );
}

HB_FUNC( WINSETHEIGHT )
{
   HWND hWnd = ( HWND ) hb_parptr( 1 );
   RECT rct;
   POINT pt;
   WORD wWidth;

   GetWindowRect( hWnd, &rct );
   wWidth = (WORD) ( rct.right - rct.left );

   if( GetWindowLong( hWnd, GWL_STYLE ) && WS_CHILD )
   {
      pt.x = rct.left;
      pt.y = rct.top;
      ScreenToClient( GetParent( hWnd ), &pt );
      rct.left = pt.x;
      rct.top  = pt.y;
   }

   MoveWindow( hWnd, rct.left, rct.top, wWidth, hb_parnl( 2 ), TRUE );
}

HB_FUNC( WINGETTOP )
{
   HWND hWnd = ( HWND ) hb_parptr( 1 );
   RECT rct;
   POINT pt;

   GetWindowRect( hWnd, &rct );

   if( GetWindowLong( hWnd, GWL_STYLE ) && WS_CHILD )
   {
      pt.x = rct.left;
      pt.y = rct.top;
      ScreenToClient( GetParent( hWnd ), &pt );
      rct.left = pt.x;
      rct.top  = pt.y;
   }

   hb_retnl( rct.top );
}

HB_FUNC( WINSETTOP )
{
   HWND hWnd = ( HWND ) hb_parptr( 1 );
   RECT rct;
   POINT pt;
   WORD wHeight, wWidth;

   GetWindowRect( hWnd, &rct );
   wHeight = (WORD) ( rct.bottom - rct.top );
   wWidth  = (WORD) ( rct.right - rct.left );

   if( GetWindowLong( hWnd, GWL_STYLE ) && WS_CHILD )
   {
      pt.x = rct.left;
      pt.y = hb_parnl( 2 );
      ScreenToClient( GetParent( hWnd ), &pt );
      rct.left = pt.x;
      rct.top  = pt.y;
   }

   MoveWindow( hWnd, rct.left, hb_parnl( 2 ), wWidth, wHeight, TRUE );
}

HB_FUNC( WINGETLEFT )
{
   HWND hWnd = ( HWND ) hb_parptr( 1 );
   RECT rct;
   POINT pt;

   GetWindowRect( hWnd, &rct );

   if( GetWindowLong( hWnd, GWL_STYLE ) && WS_CHILD )
   {
      pt.x = rct.left;
      pt.y = rct.top;
      ScreenToClient( GetParent( hWnd ), &pt );
      rct.left = pt.x;
      rct.top  = pt.y;
   }

   hb_retnl( rct.left );
}

HB_FUNC( WINSETLEFT )
{
   HWND hWnd = ( HWND ) hb_parptr( 1 );
   RECT rct;
   POINT pt;
   WORD wHeight, wWidth;

   GetWindowRect( hWnd, &rct );
   wHeight = (WORD) ( rct.bottom - rct.top );
   wWidth  = (WORD) ( rct.right - rct.left );

   if( GetWindowLong( hWnd, GWL_STYLE ) && WS_CHILD )
   {
      pt.x = hb_parnl( 2 );
      pt.y = rct.top;
      ScreenToClient( GetParent( hWnd ), &pt );
      rct.left = pt.x;
      rct.top  = pt.y;
   }

   MoveWindow( hWnd, hb_parnl( 2 ), rct.top, wWidth, wHeight, TRUE );
}

HB_FUNC( SHOWWINDOW )
{
   hb_retl( ShowWindow( ( HWND ) hb_parptr( 1 ), hb_parl( 2 ) ) );
}
