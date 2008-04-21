/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * TOPBAR menu class
 *
 * Copyright 2000 Jose Lalin <dezac@corevia.com>
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

#include "hbclass.ch"

#include "button.ch"
#include "color.ch"
#include "common.ch"
#include "inkey.ch"

/* NOTE: Harbour doesn't support CA-Cl*pper 5.3 GUI functionality, but 
         it has all related variables and methods. */

#ifdef HB_COMPAT_C53

CREATE CLASS TOPBARMENU FUNCTION HBTopBarMenu

   EXPORTED:

   METHOD addItem( oItem )
   METHOD delItem( nPos )
   METHOD display()
   METHOD getFirst()
   METHOD getItem( nPos )
   METHOD getLast()
   METHOD getNext()
   METHOD getPrev()
   METHOD getAccel( nKey )
   METHOD getShortCt( nKey )                        /* NOTE: This method exists but it is not documented in the manuals nor the NG's [jlalin] */
   METHOD hitTest( nMRow, nMCol )
   METHOD insItem( nPos, oItem )
   METHOD select( nPos )
   METHOD setItem( nPos, oItem )

   METHOD colorSpec( cColorSpec ) SETGET
   METHOD current() SETGET
   METHOD itemCount() SETGET
   METHOD left( nLeft ) SETGET
   METHOD right( nRight ) SETGET
   METHOD row( nRow ) SETGET

   METHOD New( nRow, nLeft, nRight )                /* NOTE: This method is a Harbour extension [vszakats] */

   PROTECTED:

   VAR cColorSpec
   VAR nCurrent   INIT 0
   VAR nItemCount INIT 0
   VAR nLeft
   VAR nRight
   VAR nRow

   VAR aItems     INIT {}
   VAR nWidth     INIT 0

ENDCLASS

METHOD addItem( oItem ) CLASS TOPBARMENU

   IF ISOBJECT( oItem ) .AND. oItem:ClassName() == "MENUITEM"

      ::nItemCount++
      AAdd( ::aItems, oItem )

      ::nWidth := Max( __CapMetrics( oItem ), ::nWidth )
   ENDIF

   RETURN Self

METHOD delItem( nPos ) CLASS TOPBARMENU
   LOCAL nLen
   LOCAL aItems
   LOCAL nWidth

   IF nPos >= 1 .AND. nPos <= ::nItemCount

      nLen := Len( ::aItems[ nPos ]:caption )

      ADel( ::aItems, nPos )
      ASize( ::aItems, --::nItemCount )

      IF ::nWidth == nLen + 2
          aItems := ::aItems
          nLen := ::nItemCount
          nWidth := 0
          FOR nPos := 1 TO nLen
             nWidth := Max( __CapMetrics( aItems[ nPos ] ), nWidth )
          NEXT
          ::nWidth := nWidth
      ENDIF
   ENDIF

   RETURN Self

METHOD display() CLASS TOPBARMENU

   LOCAL nOldRow := Row()
   LOCAL nOldCol := Col()
   LOCAL lOldMCur := MSetCursor( .F. )

   LOCAL nRow := ::nRow
   LOCAL nLeft := ::nLeft
   LOCAL nRight := ::nRight
   LOCAL aItems := ::aItems
   LOCAL nItemCount := ::nItemCount
   LOCAL nCurrent := ::nCurrent

   LOCAL cColor1 := hb_ColorIndex( ::cColorSpec, 0 )
   LOCAL cColor2 := hb_ColorIndex( ::cColorSpec, 1 )

   LOCAL oPopUp
   LOCAL nItem
   LOCAL cCaption
   LOCAL nCaptionLen
   LOCAL nPos

   DispBegin()

   DispOutAt( nRow, nLeft, Space( nRight - nLeft + 1 ), cColor1 )

   FOR nItem := 1 TO nItemCount

      cCaption := " " + RTrim( aItems[ nItem ]:caption ) + " "
      nCaptionLen := Len( cCaption )

      IF nCaptionLen == 0
         LOOP
      ENDIF

      IF nLeft > nRight - nCaptionLen
         nLeft := nRight - nCaptionLen
      ENDIF

      aItems[ nItem ]:__row := nRow
      aItems[ nItem ]:__col := nLeft

      IF aItems[ nItem ]:isPopUp()
         oPopUp := aItems[ nItem ]:data
         oPopUp:top := nRow + 1
         oPopUp:left := nLeft
         oPopUp:bottom := NIL
         oPopUp:right := NIL
      ENDIF

      IF ( nPos := At( "&", cCaption ) ) > 0
         IF nPos == Len( cCaption )
            nPos := 0
         ELSE
            cCaption := Stuff( cCaption, nPos, 1, "" )
            nCaptionLen--
         ENDIF
      ENDIF

      DispOutAt( nRow, nLeft, cCaption,;
         iif( nItem == nCurrent, cColor2,;
            iif( aItems[ nItem ]:enabled, cColor1, hb_ColorIndex( ::cColorSpec, 4 ) ) ) )

      IF aItems[ nItem ]:enabled .AND. nPos > 0
         DispOutAt( nRow, nLeft + nPos - 1, SubStr( cCaption, nPos, 1 ),;
            iif( nItem == nCurrent, hb_ColorIndex( ::cColorSpec, 3 ), hb_ColorIndex( ::cColorSpec, 2 ) ) )
      ENDIF

      nLeft += nCaptionLen
   NEXT

   IF nCurrent != 0 .AND. ;
      ::aItems[ nCurrent ]:isPopUp() .AND. ;
      ::aItems[ nCurrent ]:data:isOpen

      ::aItems[ nCurrent ]:data:display()
   ENDIF

   DispEnd()

   SetPos( nOldRow, nOldCol )
   MSetCursor( lOldMCur )

   RETURN Self

METHOD getFirst() CLASS TOPBARMENU

   LOCAL n

   FOR n := 1 TO ::nItemCount
      IF ::aItems[ n ]:enabled
         RETURN n
      ENDIF
   NEXT

   RETURN 0

METHOD getItem( nPos ) CLASS TOPBARMENU
   RETURN iif( nPos >= 1 .AND. nPos <= ::nItemCount, ::aItems[ nPos ], NIL )

METHOD getLast() CLASS TOPBARMENU

   LOCAL n

   FOR n := ::nItemCount TO 1 STEP -1
      IF ::aItems[ n ]:enabled
         RETURN n
      ENDIF
   NEXT

   RETURN 0

METHOD getNext() CLASS TOPBARMENU

   LOCAL n

   IF ::nCurrent < ::nItemCount
      FOR n := ::nCurrent + 1 TO ::nItemCount
         IF ::aItems[ n ]:enabled
            RETURN n
         ENDIF
      NEXT
   ENDIF

   RETURN 0

METHOD getPrev() CLASS TOPBARMENU

   LOCAL n

   IF ::nCurrent > 1
      FOR n := ::nCurrent - 1 TO 1 STEP -1
         IF ::aItems[ n ]:enabled
            RETURN n
         ENDIF
      NEXT
   ENDIF

   RETURN 0

/* NOTE: This method corrects two bugs in Cl*pper:
         1) when two menuitems have the same key and the
            first item is disabled
         2) when a menuitem is disabled it will ignore the key [jlalin] */

METHOD getAccel( nKey ) CLASS TOPBARMENU

   LOCAL nIndex := AScan( { K_ALT_A, K_ALT_B, K_ALT_C, K_ALT_D, K_ALT_E, K_ALT_F,;
                            K_ALT_G, K_ALT_H, K_ALT_I, K_ALT_J, K_ALT_K, K_ALT_L,;
                            K_ALT_M, K_ALT_N, K_ALT_O, K_ALT_P, K_ALT_Q, K_ALT_R,;
                            K_ALT_S, K_ALT_T, K_ALT_U, K_ALT_V, K_ALT_W, K_ALT_X,;
                            K_ALT_Y, K_ALT_Z, K_ALT_1, K_ALT_2, K_ALT_3, K_ALT_4,;
                            K_ALT_5, K_ALT_6, K_ALT_7, K_ALT_8, K_ALT_9, K_ALT_0 }, nKey )

   LOCAL cKey
   LOCAL nAt
   LOCAL n

   IF nIndex > 0

      cKey := SubStr( "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890", nIndex, 1 )

      FOR n := 1 TO ::nItemCount
      
         IF ( nAt := At( "&", ::aItems[ n ]:caption ) ) > 0 .AND. ;
            Upper( SubStr( ::aItems[ n ]:caption, nAt + 1, 1 ) ) == cKey
      
#ifdef HB_EXTENSION
            IF ::aItems[ n ]:enabled
#endif
               RETURN n
#ifdef HB_EXTENSION
            ENDIF
#endif
         ENDIF
      NEXT
   ENDIF

   RETURN 0

METHOD getShortCt( nKey ) CLASS TOPBARMENU

   LOCAL n

   FOR n := 1 TO ::nItemCount
      IF ::aItems[ n ]:shortcut == nKey
         RETURN n
      ENDIF
   NEXT

   RETURN 0

/* NOTE: In my tests I can't get other values than HTNOWHERE or a value
         greather than 0 (selected item), althought the NG's says that
         it returns other HT* values [jlalin]

         This method correct a bug in Cl*pper:
         when click on a disabled menuitem it will ignore it [jlalin] */

METHOD hitTest( nMRow, nMCol ) CLASS TOPBARMENU

   LOCAL aItems
   LOCAL nColumn
   LOCAL n

   IF nMRow == ::nRow

      aItems := ::aItems

      FOR n := 1 TO ::nItemCount

         nColumn := aItems[ n ]:__col

         IF nMCol >= nColumn .AND. nMCol <= nColumn + Len( aItems[ n ]:caption )
#ifdef HB_EXTENSION
            IF aItems[ n ]:enabled
#endif
               RETURN n
#ifdef HB_EXTENSION
            ENDIF
#endif
         ENDIF
      NEXT
   ENDIF

   RETURN HTNOWHERE

METHOD insItem( nPos, oItem ) CLASS TOPBARMENU

   IF nPos >= 1 .AND. nPos <= ::nItemCount .AND. ;
      ISOBJECT( oItem ) .AND. oItem:ClassName() == "MENUITEM"

      ASize( ::aItems, ++::nItemCount )
      AIns( ::aItems, nPos )
      ::aItems[ nPos ] := oItem

      ::nWidth := Max( __CapMetrics( oItem ), ::nWidth )
   ENDIF

   RETURN Self

METHOD select( nPos ) CLASS TOPBARMENU

   IF ( nPos >= 1 .AND. nPos <= ::nItemCount .AND. ;
      ::nCurrent != nPos .AND. ;
      ::aItems[ nPos ]:enabled ) .OR. nPos == 0

//    IF ::isOpen() .AND. ;
//       ::nCurrent > 0 .AND. ;
//       ::aItems[ ::nCurrent ]:isPopUp()
//
//       ::aItems[ ::nCurrent ]:data:close()
//    ENDIF

      ::nCurrent := nPos
   ENDIF

   RETURN Self

METHOD setItem( nPos, oItem ) CLASS TOPBARMENU

   IF nPos >= 1 .AND. nPos <= ::nItemCount .AND. ;
      ISOBJECT( oItem ) .AND. oItem:ClassName() == "MENUITEM"

      ::aItems[ nPos ] := oItem

      ::nWidth := Max( __CapMetrics( oItem ), ::nWidth )
   ENDIF

   RETURN Self

METHOD colorSpec( cColorSpec ) CLASS TOPBARMENU

   IF cColorSpec != NIL
      ::cColorSpec := _eInstVar( Self, "COLORSPEC", cColorSpec, "C", 1001,;
         {|| !Empty( hb_ColorIndex( cColorSpec, 5 ) ) .AND. Empty( hb_ColorIndex( cColorSpec, 6 ) ) } )
   ENDIF

   RETURN ::cColorSpec

METHOD current() CLASS TOPBARMENU
   RETURN ::nCurrent

METHOD itemCount() CLASS TOPBARMENU
   RETURN ::nItemCount

METHOD left( nLeft ) CLASS TOPBARMENU

   IF nLeft != NIL
      ::nLeft := _eInstVar( Self, "LEFT", nLeft, "N", 1001 )
   ENDIF

   RETURN ::nLeft

METHOD right( nRight ) CLASS TOPBARMENU

   IF nRight != NIL
      ::nRight := _eInstVar( Self, "RIGHT", nRight, "N", 1001 )
   ENDIF

   RETURN ::nRight

METHOD row( nRow ) CLASS TOPBARMENU

   IF nRow != NIL
      /* NOTE: CA-Clipper 5.3 has a bug, where it would show "TOP" in case of an error. */
      ::nRow := _eInstVar( Self, "ROW", nRow, "N", 1001 )
   ENDIF

   RETURN ::nRow

/* -------------------------------------------- */

METHOD New( nRow, nLeft, nRight ) CLASS TOPBARMENU

   LOCAL cColor

   IF !ISNUMBER( nRow ) .OR. ;
      !ISNUMBER( nLeft ) .OR. ;
      !ISNUMBER( nRight )
      RETURN NIL
   ENDIF

   ::nLeft  := nLeft
   ::nRight := nRight
   ::nRow   := nRow

   IF IsDefColor()
      ::cColorSpec := "N/W,W/N,W+/W,W+/N,N+/W,W/N"
   ELSE
      cColor := SetColor()
      ::cColorSpec := hb_ColorIndex( cColor, CLR_UNSELECTED ) + "," +;
                      hb_ColorIndex( cColor, CLR_ENHANCED   ) + "," +;
                      hb_ColorIndex( cColor, CLR_BACKGROUND ) + "," +;
                      hb_ColorIndex( cColor, CLR_ENHANCED   ) + "," +;
                      hb_ColorIndex( cColor, CLR_STANDARD   ) + "," +;
                      hb_ColorIndex( cColor, CLR_BORDER     )
   ENDIF

   RETURN Self

/* -------------------------------------------- */

FUNCTION TopBar( nRow, nLeft, nRight )
   RETURN HBTopBarMenu():New( nRow, nLeft, nRight )

#endif
