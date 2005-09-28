/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The Debugger
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
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

/* NOTE: Don't use SAY/DevOut()/DevPos() for screen output, otherwise
         the debugger output may interfere with the applications output
         redirection, and is also slower. [vszakats] */

#include "hbclass.ch"
#include "hbmemvar.ch"
#include "box.ch"
#include "inkey.ch"
#include "common.ch"
#include "setcurs.ch"

CLASS TDbWindow  // Debugger windows and dialogs

   DATA   nTop, nLeft, nBottom, nRight
   DATA   cCaption, cMark
   DATA   cBackImage, cColor, cColorFocus
   DATA   lFocused, bGotFocus, bLostFocus
   DATA   bKeyPressed, bPainted, bLButtonDown, bLDblClick
   DATA   lShadow, lVisible
   DATA   Cargo
   DATA   Browser

   METHOD New( nTop, nLeft, nBottom, nRight, cCaption, cColor )
   METHOD Hide()
   METHOD IsOver( nRow, nCol )
   METHOD nWidth() INLINE ::nRight - ::nLeft + 1
   METHOD Clear()
   METHOD ScrollUp( nLines )
   METHOD SetCaption( cCaption )
   METHOD ShowCaption()
   METHOD SetFocus( lOnOff )
   METHOD Show( lFocused )
   METHOD ShowModal()
   METHOD LButtonDown( nMRow, nMCol )
   METHOD LDblClick( nMRow, nMCol )
   METHOD LoadColors() 

   METHOD Move()
   METHOD KeyPressed( nKey )
   METHOD Refresh()
   METHOD Resize()

ENDCLASS

METHOD New( nTop, nLeft, nBottom, nRight, cCaption, cColor ) CLASS TDbWindow

   DEFAULT cColor TO __DbgColors()[ 1 ]

   ::nTop     := nTop
   ::nLeft    := nLeft
   ::nBottom  := nBottom
   ::nRight   := nRight
   ::cCaption := cCaption
   ::cColor   := cColor
   ::cColorFocus := __DbgColors()[ 12 ]
   ::lShadow  := .f.
   ::lVisible := .f.
   ::lFocused := .f.
   //Check mark should be different under xterm terminal
   ::cMark    := '['+ IIF( AT("TERM",UPPER(GETENV("TERM")))>0, 'X', CHR(254) )+ ']'

return Self

METHOD Hide() CLASS TDbWindow

   RestScreen( ::nTop, ::nLeft, ::nBottom + iif( ::lShadow, 1, 0 ),;
               ::nRight + iif( ::lShadow, 2, 0 ), ::cBackImage )
   ::cBackImage := nil
   ::lVisible := .f.

return nil

METHOD IsOver( nRow, nCol ) CLASS TDbWindow

return nRow >= ::nTop .and. nRow <= ::nBottom .and. ;
       nCol >= ::nLeft .and. nCol <= ::nRight

METHOD ScrollUp( nLines ) CLASS TDbWindow

   DEFAULT nLines TO 1

   SetColor( ::cColor )
   Scroll( ::nTop + 1, ::nLeft + 1, ::nBottom - 1, ::nRight - 1, nLines )

return nil

METHOD Clear() CLASS TDbWindow

   SetColor( ::cColor )
   Scroll( ::nTop+1, ::nLeft+1, ::nBottom-1, ::nRight-1 )

RETURN nil


METHOD SetCaption( cCaption ) CLASS TDbWindow

   ::cCaption := cCaption
return nil
  
METHOD ShowCaption CLASS TDbWindow
   if ! Empty( ::cCaption )
      DispOutAt( ::nTop, ::nLeft + ( ( ::nRight - ::nLeft ) / 2 ) - ;
         ( ( Len( ::cCaption ) + 2 ) / 2 ),;
         " " + ::cCaption + " ", ;
         IIF( ::lFocused, ::cColorFocus, ::cColor ) )
   endif

return nil

METHOD SetFocus( lOnOff ) CLASS TDbWindow

   if ! lOnOff .and. ::bLostFocus != nil
      Eval( ::bLostFocus, Self )
   endif

   DispBegin()

   ::lFocused := lOnOff

   IF( lOnOff )
      @ ::nTop, ::nLeft TO ::nBottom, ::nRight DOUBLE COLOR ::cColorFocus
   ELSE
      @ ::nTop, ::nLeft TO ::nBottom, ::nRight COLOR ::cColor
   ENDIF

   DispOutAt( ::nTop, ::nLeft + 1, ::cMark, IIF(lOnOff,::cColorFocus,::cColor) )

   if ! Empty( ::cCaption )
      ::ShowCaption( ::cCaption )
   endif

   if ::bPainted != nil
      Eval( ::bPainted, Self )
   endif

   DispEnd()

   if lOnOff .and. ::bGotFocus != nil
      Eval( ::bGotFocus, Self )
   endif

return nil

METHOD Refresh() CLASS TDbWindow

   DispBegin()

   IF( ::lFocused )
      @ ::nTop, ::nLeft TO ::nBottom, ::nRight DOUBLE COLOR ::cColorFocus
   ELSE
      @ ::nTop, ::nLeft TO ::nBottom, ::nRight COLOR ::cColor
   ENDIF

   DispOutAt( ::nTop, ::nLeft + 1, ::cMark, IIF(::lFocused,::cColorFocus,::cColor) )

   if ! Empty( ::cCaption )
      ::ShowCaption( ::cCaption )
   endif

   if ::bPainted != nil
      Eval( ::bPainted, Self )
   endif

   DispEnd()

return nil

METHOD Show( lFocused ) CLASS TDbWindow

   DEFAULT lFocused TO ::lFocused

   ::cBackImage := SaveScreen( ::nTop, ::nLeft, ::nBottom + iif( ::lShadow, 1, 0 ),;
                              ::nRight + iif( ::lShadow, 2, 0 ) )
   SetColor( ::cColor )
   Scroll( ::nTop, ::nLeft, ::nBottom, ::nRight )
   ::SetFocus( lFocused )

   If ::lShadow
      hb_Shadow( ::nTop, ::nLeft, ::nBottom, ::nRight )
   endif

   ::ShowCaption()
   ::lVisible := .t.

return nil

METHOD ShowModal() CLASS TDbWindow

   local lExit := .f.
   local nKey

   ::lShadow := .t.
   ::Show()

   while ! lExit
      nKey := InKey( 0, INKEY_ALL )

      if ::bKeyPressed != nil
         Eval( ::bKeyPressed, nKey )
      endif

      do case
         case nKey == K_ESC
              lExit := .t.

         case nKey == K_LBUTTONDOWN
              if MRow() == ::nTop .and. MCol() >= ::nLeft + 1 .and. ;
                 MCol() <= ::nLeft + 3
                 lExit := .t.
              endif
      endcase
   end

   ::Hide()

return nil

METHOD LButtonDown( nMRow, nMCol ) CLASS TDbWindow

   if ::bLButtonDown != nil
      Eval( ::bLButtonDown, nMRow, nMCol )
   endif

return nil

METHOD LDblClick( nMRow, nMCol ) CLASS TDbWindow

   if ::bLDblClick != nil
      Eval( ::bLDblClick, nMRow, nMCol )
   endif

return nil

/*Method move()
Move a window across the screen
Copyright Luiz Rafael Culik 1999
*/
METHOD Move() Class TDbWindow

   local nOldTop    := ::nTop
   local nOldLeft   := ::nLeft
   local nOldBottom := ::nbottom
   local nOldRight  := ::nright
   local nKey

   while .t.
      RestScreen( ,,,, ::cbackimage )
      DispBox( ::nTop, ::nLeft, ::nRight, ::nBottom, Replicate( Chr( 176 ), 8 ) + " " )

      nKey := Inkey( 0 )

      do case
         case nkey == K_UP
              if ::ntop != 0
                 ::ntop--
                 ::nbottom--
              endif

         case nKey == K_DOWN
              if ::nBottom != MaxRow()
                 ::nTop++
                 ::nBottom++
              endif

         case nKey == K_LEFT
              if ::nLeft != 0
                 ::nLeft--
                 ::nRight--
              endif

         case nKey == K_RIGHT
              if ::nBottom != MaxRow()
                 ::nLeft++
                 ::nRight++
              endif

         case nKey == K_ESC
              ::nTop    := nOldTop
              ::nLeft   := nOldLeft
              ::nBottom := nOldBottom
              ::nRight  := nOldRight
      endcase

      if nKey == K_ESC .or. nKey == K_ENTER
         exit
      end
   end

   // __keyboard( chr( 0 ) ), inkey() )

return nil

METHOD KeyPressed( nKey ) CLASS TDbWindow

   if ::bKeyPressed != nil
      Eval( ::bKeyPressed, nKey, Self )
   endif

return nil

METHOD LoadColors() CLASS TDbWindow
LOCAL aClr:=__DbgColors()

   ::cColor := aClr[ 1 ]
   IF( ::Browser!=NIL )
      ::Browser:ColorSpec := aClr[ 2 ] + "," + aClr[ 5 ] + "," + aClr[ 3 ]
   ENDIF
   
RETURN nil

METHOD Resize( nTop, nLeft, nBottom, nRight ) CLASS TDbWindow
LOCAL lShow
   
   IF( lShow:=::lVisible )
      ::Hide()
   ENDIF
   IF( nTop != NIL )
      ::nTop := nTop
   ENDIF
   IF( nBottom != NIL )
      ::nBottom := nBottom
   ENDIF
   IF( nLeft != NIL )
      ::nLeft := nLeft
   ENDIF
   IF( nRight != NIL )
      ::nRight := nRight
   ENDIF
   
   IF( ::Browser != NIL )
      ::Browser:Resize( ::nTop+1, ::nLeft+1, ::nBottom-1, ::nRight-1 )
   ENDIF

   IF( lShow )
      ::Show( ::lFocused )
   ENDIF
   
RETURN self