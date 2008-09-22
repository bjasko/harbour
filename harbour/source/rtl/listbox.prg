/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Listbox class
 *
 * Copyright 2000 Luiz Rafael Culik <culik@sl.conex.net>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modIFy
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
 * along with this software; see the file COPYING.  IF not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, IF you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  IF you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modIFied files, you must delete
 * this exception notice from them.
 *
 * IF you write modIFications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modIFications.
 * IF you do not wish that, delete this exception notice.
 *
 */

#include "hbclass.ch"

#include "box.ch"
#include "button.ch"
#include "color.ch"
#include "common.ch"
#include "inkey.ch"
#include "setcurs.ch"

/* NOTE: Harbour doesn't support CA-Cl*pper 5.3 GUI functionality, but 
         it has all related variables and methods. */

/* NOTE: CA-Cl*pper 5.3 uses a mixture of QQOut(), DevOut(), Disp*() 
         functions to generate screen output. Harbour uses Disp*() 
         functions only. [vszakats] */

#ifdef HB_COMPAT_C53

#define _ITEM_cTEXT         1
#define _ITEM_cDATA         2

#define _LISTBOX_ITEMDATA( aItem ) iif( aItem[ _ITEM_cDATA ] == NIL, aItem[ _ITEM_cTEXT ], aItem[ _ITEM_cDATA ] )

CREATE CLASS LISTBOX FUNCTION HBListBox

   EXPORTED:

   VAR cargo

   METHOD addItem( cText, cData )
   METHOD close()
   METHOD delItem( nPos )
   METHOD display()
   METHOD findText( cText, nPos, lCaseSensitive, lExact )
   METHOD findData( cData, nPos, lCaseSensitive, lExact ) /* NOTE: Undocumented CA-Cl*pper method. */
   METHOD getData( nPos )
   METHOD getItem( nPos )
   METHOD getText( nPos )
   METHOD hitTest( nMRow, nMCol )
   METHOD insItem( nPos, cText, cData )
   METHOD killFocus()
   METHOD nextItem()
   METHOD open()
   METHOD prevItem()
   METHOD scroll( n )
   METHOD select( nValue )
   METHOD setData( nPos, cData )
   METHOD setFocus()
   METHOD setItem( nPos, aItem )
   METHOD setText( nPos, cText )

   METHOD bitmap( cBitmap ) SETGET
   METHOD bottom( nBottom ) SETGET
   METHOD buffer() SETGET
   METHOD capCol( nCapCol ) SETGET
   METHOD capRow( nCapRow ) SETGET
   METHOD caption( cCaption ) SETGET
   METHOD coldBox( cColdBox ) SETGET
   METHOD colorSpec( cColorSpec ) SETGET
   METHOD dropDown( lDropDown ) SETGET
   METHOD fBlock( bFBlock ) SETGET
   METHOD hasFocus() SETGET
   METHOD hotBox( cHotBox ) SETGET
   METHOD isOpen() SETGET
   METHOD itemCount() SETGET
   METHOD left( nLeft ) SETGET
   METHOD message( cMessage ) SETGET
   METHOD right( nRight ) SETGET
   METHOD sBlock( bSBlock ) SETGET
   METHOD style( cStyle ) SETGET                          /* NOTE: Undocumented CA-Cl*pper method. */
   METHOD textValue() SETGET                              /* NOTE: Undocumented CA-Cl*pper method. */
   METHOD top( nTop ) SETGET
   METHOD topItem( nTopItem ) SETGET
   METHOD typeOut() SETGET
   METHOD value() SETGET                                  /* NOTE: Undocumented CA-Cl*pper method. */
   METHOD vScroll( oVScroll ) SETGET

   METHOD New( nTop, nLeft, nBottom, nRight, lDrop ) /* NOTE: This method is a Harbour extension [vszakats] */

   PROTECTED:

   VAR cBitmap    INIT "dropbox.bmu"
   VAR nBottom
   VAR xBuffer
   VAR nCapCol
   VAR nCapRow
   VAR cCaption   INIT ""
   VAR cColdBox   INIT Chr( 218 ) + Chr( 196 ) + Chr( 191 ) + Chr( 179 ) + Chr( 217 ) + Chr( 196 ) + Chr( 192 ) + Chr( 179 )
   VAR cColorSpec
   VAR lDropDown
   VAR bFBlock
   VAR lHasFocus  INIT .F.
   VAR cHotBox    INIT Chr( 201 ) + Chr( 205 ) + Chr( 187 ) + Chr( 186 ) + Chr( 188 ) + Chr( 205 ) + Chr( 200 ) + Chr( 186 )
   VAR lIsOpen
   VAR nItemCount INIT 0
   VAR nLeft
   VAR cMessage   INIT ""
   VAR nRight
   VAR bSBlock
   VAR cStyle     INIT Chr( 31 )
   VAR cTextValue INIT ""
   VAR nTop
   VAR nTopItem   INIT 0
   VAR nValue     INIT 0
   VAR oVScroll

   VAR aItems     INIT {}
   VAR aSaveScr
   VAR nCursor

   METHOD changeItem( nOldPos, nNewPos )
   METHOD scrollbarPos()

ENDCLASS

METHOD addItem( cText, cData ) CLASS LISTBOX

   IF ISCHARACTER( cText ) .AND. Valtype( cData ) $ "CU"

      AAdd( ::aItems, { cText, cData } )

      ::nItemCount++

      IF ::nItemCount == 1
         ::nTopItem := 1
         IF ::oVScroll != NIL
            ::oVScroll:total := ( ::nItemCount - ( ::nBottom - ::nTop - 2 ) )
         ENDIF
      ENDIF
   ENDIF

   RETURN Self

METHOD close() CLASS LISTBOX

   IF ::lIsOpen
      RestScreen( ::aSaveScr[ 1 ], ::aSaveScr[ 2 ], ::aSaveScr[ 3 ], ::aSaveScr[ 4 ], ::aSaveScr[ 5 ] )
      ::lIsOpen := .F.
      ::aSaveScr := NIL
   ENDIF

   RETURN Self

METHOD delItem( nPos )

   IF nPos >= 1 .AND. nPos <= ::nItemCount

      ADel( ::aItems, nPos )
      ASize( ::aItems, --::nItemCount )

      IF ::nValue > ::nItemCount
         ::nValue := ::nItemCount

         ::cTextValue := iif( ::nValue == 0, "", _LISTBOX_ITEMDATA( ::aItems[ ::nItemCount ] ) )

         IF ::xBuffer == NIL
         ELSEIF ISNUMBER( ::xBuffer )
            ::xBuffer := ::nItemCount
         ELSEIF ::nValue > 0
            ::xBuffer := ::cTextValue
         ENDIF

      ENDIF

      IF ::nTopItem > ::nItemCount
         ::nTopItem := ::nItemCount
      ENDIF

      IF ::oVScroll != NIL
         ::oVScroll:total := ::nItemCount - ( ::nBottom - ::nTop - 2 )
      ENDIF
   ENDIF

   RETURN Self

METHOD display() CLASS LISTBOX

   LOCAL cOldColor := SetColor()      
   LOCAL nOldRow := Row()             
   LOCAL nOldCol := Col()             
   LOCAL lOldMCur := MSetCursor( .F. )

   LOCAL nItem
   LOCAL nEnd
   LOCAL cColor4
   LOCAL cColor3
   LOCAL cColorAny
   LOCAL nTop := ::nTop
   LOCAL nLeft := ::nLeft
   LOCAL nSize := ::nRight - nLeft + 1
   LOCAL cHotBox
   LOCAL cCaption
   LOCAL nPos
   
   IF ::lHasFocus
      cHotBox   := ::cHotBox
      cColor3   := hb_ColorIndex( ::cColorSpec, 2 )
      cColor4   := hb_ColorIndex( ::cColorSpec, 3 )
      cColorAny := iif( ::lIsOpen, hb_ColorIndex( ::cColorSpec, 1 ), hb_ColorIndex( ::cColorSpec, 3 ) )
   ELSE
      cHotBox   := ::cColdBox
      cColor3   := hb_ColorIndex( ::cColorSpec, 0 )
      cColor4   := hb_ColorIndex( ::cColorSpec, 1 )
      cColorAny := hb_ColorIndex( ::cColorSpec, 1 )
   ENDIF

   DispBegin()

   nEnd := ::nTopItem + ::nBottom - ::nTop

   IF ::lDropDown

      DispOutAt( nTop++, nLeft,;
         iif( ::nValue == 0, Space( nSize - 1 ), PadR( ::aItems[ ::nValue ][ _ITEM_cTEXT ], nSize - 1 ) ),;
         cColorAny )

      DispOut( ::cStyle, hb_ColorIndex( ::cColorSpec, 7 ) )

      nEnd--
   ENDIF

   IF ::lIsOpen
      IF !Empty( cHotBox )

         SetColor( hb_ColorIndex( ::cColorSpec, 4 ) )
         Scroll( nTop, nLeft, ::nBottom, ::nRight )
         DispBox( nTop, nLeft, ::nBottom, ::nRight, cHotBox )

         IF ::oVScroll != NIL
            ::oVScroll:display()
         ENDIF

         nTop++
         nLeft++
         nSize -= 2
         nEnd -= 2

      ENDIF

      IF nEnd > ::nItemCount
         nEnd := ::nItemCount
      ENDIF

      FOR nItem := ::nTopItem TO nEnd
         DispOutAt( nTop++, nLeft, PadR( ::aItems[ nItem ][ _ITEM_cTEXT ], nSize ), iif( nItem == ::nValue, cColor4, cColor3 ) )
      NEXT
   ENDIF

   IF !Empty( cCaption := ::cCaption )

      IF ( nPos := At( "&", cCaption ) ) == 0
      ELSEIF nPos == Len( cCaption )
         nPos := 0
      ELSE
         cCaption := Stuff( cCaption, nPos, 1, "" )
      ENDIF

      DispOutAt( ::nCapRow, ::nCapCol - 1, cCaption, hb_ColorIndex( ::cColorSpec, 5 ) )

      IF nPos != 0
         DispOutAt( ::nCapRow, ::nCapCol + nPos - 2, SubStr( cCaption, nPos, 1 ), hb_ColorIndex( ::cColorSpec, 6 ) )
      ENDIF

   ENDIF

   DispEnd()

   MSetCursor( lOldMCur )
   SetColor( cOldColor )
   SetPos( nOldRow, nOldCol )

   RETURN Self

METHOD findText( cText, nPos, lCaseSensitive, lExact ) CLASS LISTBOX

   LOCAL nPosFound
   LOCAL nPass
   LOCAL nPasses
   LOCAL nSize
   LOCAL lOldExact

   IF !ISCHARACTER( cText )
      RETURN 0
   ENDIF
   IF !ISNUMBER( nPos )
      nPos := 1
   ENDIF
   IF !ISLOGICAL( lCaseSensitive )
      lCaseSensitive := .T.
   ENDIF
   IF !lCaseSensitive
      cText := Lower( cText )
   ENDIF
   IF ISLOGICAL( lExact )
      lOldExact := Set( _SET_EXACT, lExact )
   ENDIF

   nSize := Len( ::aItems ) - nPos + 1
   nPasses := iif( nPos > 1, 2, 1 )

   FOR nPass := 1 TO nPasses

      /* NOTE: Intentionally using "=" comparison to honor the _SET_EXACT setting. */
      IF ( nPosFound := AScan( ::aItems, iif( lCaseSensitive,;
                                              { | aItem | aItem[ _ITEM_cTEXT ] = cText },;
                                              { | aItem | Lower( aItem[ _ITEM_cTEXT ] ) = cText } ), nPos, nSize ) ) > 0
         EXIT
      ENDIF

      nSize := nPos - 1
      nPos := 1
   NEXT

   IF lOldExact != NIL
      Set( _SET_EXACT, lOldExact )
   ENDIF

   RETURN nPosFound

METHOD findData( cData, nPos, lCaseSensitive, lExact ) CLASS LISTBOX

   LOCAL nPosFound
   LOCAL nPass
   LOCAL nPasses
   LOCAL nSize
   LOCAL lOldExact

   IF !ISCHARACTER( cData )
      RETURN 0
   ENDIF
   IF !ISNUMBER( nPos )
      nPos := 1
   ENDIF
   IF !ISLOGICAL( lCaseSensitive )
      lCaseSensitive := .T.
   ENDIF
   IF !lCaseSensitive
      cData := Lower( cData )
   ENDIF
   IF ISLOGICAL( lExact )
      lOldExact := Set( _SET_EXACT, lExact )
   ENDIF

   nSize := Len( ::aItems ) - nPos + 1
   nPasses := iif( nPos > 1, 2, 1 )

   FOR nPass := 1 TO nPasses

      /* NOTE: Intentionally using "=" comparison to honor the _SET_EXACT setting. */
      IF ( nPosFound := AScan( ::aItems, iif( lCaseSensitive,;
                                              { | aItem | _LISTBOX_ITEMDATA( aItem ) = cData },;
                                              { | aItem | Lower( _LISTBOX_ITEMDATA( aItem ) ) = cData } ), nPos, nSize ) ) > 0
         EXIT
      ENDIF

      nSize := nPos - 1
      nPos := 1
   NEXT

   IF lOldExact != NIL
      Set( _SET_EXACT, lOldExact )
   ENDIF

   RETURN nPosFound

METHOD getData( nPos ) CLASS LISTBOX
   RETURN iif( nPos >= 1 .AND. nPos <= ::nItemCount, ::aItems[ nPos ][ _ITEM_cDATA ], NIL )

METHOD getItem( nPos ) CLASS LISTBOX
   RETURN iif( nPos >= 1 .AND. nPos <= ::nItemCount, ::aItems[ nPos ], NIL )

METHOD getText( nPos ) CLASS LISTBOX
   RETURN iif( nPos >= 1 .AND. nPos <= ::nItemCount, ::aItems[ nPos ][ _ITEM_cTEXT ], NIL )

METHOD hitTest( nMRow, nMCol ) CLASS LISTBOX

   LOCAL nRet
   LOCAL nTop
   LOCAL nHit := 0

   /* Check hit on the scrollbar */
   IF ::lIsOpen .AND. ;
      ::oVScroll != NIL .AND. ;
      ( nHit := ::oVScroll:hitTest( nMRow, nMCol ) ) != 0

      RETURN nHit
   ENDIF

   IF ! ::lIsOpen .OR. Empty( ::cHotBox + ::cColdBox )
      nRet := 0
   ELSE
      nTop := ::nTop
      IF ::lDropDown
         nTop++
      ENDIF

      DO CASE
      CASE nMRow == nTop
         IF nMCol == ::nLeft
            RETURN HTTOPLEFT
         ELSEIF nMCol == ::nRight
            RETURN HTTOPRIGHT
         ELSEIF nMCol >= ::nLeft .AND. nMCol <= ::nRight
            RETURN HTTOP
         ENDIF
      CASE nMRow == ::nBottom
         IF nMCol == ::nLeft
            RETURN HTBOTTOMLEFT
         ELSEIF nMCol == ::nRight
            RETURN HTBOTTOMRIGHT
         ELSEIF nMCol >= ::nLeft .AND. nMCol <= ::nRight
            RETURN HTBOTTOM
         ENDIF
      CASE nMCol == ::nLeft
         IF nMRow >= ::nTop .AND. nMRow <= ::nBottom
            RETURN HTLEFT
         ELSE
            RETURN HTNOWHERE
         ENDIF
      CASE nMCol == ::nRight
         IF nMRow >= ::nTop .AND. nMRow <= ::nBottom
            RETURN HTRIGHT
         ELSE
            RETURN HTNOWHERE
         ENDIF
      ENDCASE
      nRet := 1
   ENDIF

   DO CASE
   CASE ! ::lIsOpen
   CASE nMRow < nTop + nRet
   CASE nMRow > ::nBottom - nRet
   CASE nMCol < ::nLeft + nRet
   CASE nMCol <= ::nRight - nRet
      RETURN ::nTopItem + nMRow - ( nTop + nRet )
   ENDCASE

   DO CASE
   CASE ! ::lDropDown
   CASE nMRow != ::nTop
   CASE nMCol < ::nLeft
   CASE nMCol < ::nRight
      RETURN HTCLIENT
   CASE nMCol == ::nRight
      RETURN HTDROPBUTTON
   ENDCASE

   DO CASE
   CASE Empty( ::cCaption )
   CASE nMRow != ::nCapRow
   CASE nMCol < ::nCapCol
   CASE nMCol < ::nCapCol + __CapLength( ::cCaption )
      RETURN HTCAPTION
   ENDCASE

   RETURN 0

METHOD insItem( nPos, cText, cData )

   IF ISCHARACTER( cText ) .AND. ;
      ISNUMBER( nPos ) .AND. ;
      nPos < ::nItemCount

      ASize( ::aItems, ++::nItemCount )
      AIns( ::aItems, nPos )
      ::aItems[ nPos ] := { cText, cData }

      IF ::nItemCount == 1
         ::nTopItem := 1
      ENDIF

      IF ::oVScroll != NIL
         ::oVScroll:total := ::nItemCount - ( ::nBottom - ::nTop - 2 )
      ENDIF
   ENDIF

   RETURN Self

METHOD killFocus() CLASS LISTBOX
   LOCAL nOldMCur

   IF ::lHasFocus
      ::lHasFocus := .F.

      IF ISBLOCK( ::bFBlock )
         Eval( ::bFBlock )
      ENDIF

      nOldMCur := MSetCursor( .F. )
      DispBegin()

      IF ::lDropDown .AND. ::lIsOpen
         ::close()
      ENDIF
      ::display()

      DispEnd()
      MSetCursor( nOldMCur )

      SetCursor( ::nCursor )
   ENDIF

   RETURN Self

METHOD nextItem() CLASS LISTBOX

   LOCAL nOldValue

   IF ::lHasFocus .AND. ::nItemCount > 0
      ::changeItem( nOldValue := ::nValue, iif( nOldValue == ::nItemCount, nOldValue, nOldValue + 1 ) )
   ENDIF

   RETURN Self

METHOD open() CLASS LISTBOX

   IF ! ::lIsOpen

      ::aSaveScr := { ::nTop + 1,;
                      ::nLeft,;
                      ::nBottom,;
                      ::nRight,;
                      Savescreen( ::nTop + 1, ::nLeft, ::nBottom, ::nRight ) }
      ::lIsOpen := .T.
      ::display()
   ENDIF

   RETURN Self

METHOD prevItem() CLASS LISTBOX

   LOCAL nOldValue

   IF ::lHasFocus .AND. ::nItemCount > 0

      IF ( nOldValue := ::nValue ) == 0
         ::changeItem( nOldValue, 1 )
      ELSEIF nOldValue > 1
         ::changeItem( nOldValue, nOldValue - 1 )
      ENDIF
   ENDIF

   RETURN Self

METHOD scroll( nMethod ) CLASS LISTBOX

   LOCAL nPos
   LOCAL nTopItem
   LOCAL nItemCount
   LOCAL nThumbPos
   LOCAL nCurrent
   LOCAL nBarLength
   LOCAL nTotal
   LOCAL nSize
   LOCAL nMRow
   LOCAL nPrevMRow
   LOCAL nKey
   LOCAL nCount

   DO CASE
   CASE nMethod == HTSCROLLTHUMBDRAG

      nPrevMRow := MRow()

      DO WHILE ( ( nKey := Inkey( 0 ) ) != K_LBUTTONUP )

         IF nKey == K_MOUSEMOVE

            nMRow := MRow()

            IF nMRow <= ::oVScroll:start()
               nMRow := ::oVScroll:start() + 1
            ENDIF
            IF nMRow >= ::oVScroll:end()
               nMRow := ::oVScroll:end() - 1
            ENDIF

            IF nMRow != nPrevMRow
               nThumbPos  := ::oVScroll:thumbPos() + ( nMRow - nPrevMRow )
               nBarLength := ::oVScroll:barLength()
               nTotal     := ::oVScroll:total()
               nSize      := Min( Max( ( nThumbPos * ( nTotal - nBarLength - 2 ) + 2 * nBarLength + 1 - nTotal ) / ( nBarLength - 1 ), 1 ), nTotal )
               nCurrent   := ::oVScroll:current()
               IF nSize - nCurrent > 0
                  FOR nCount := 1 TO nSize - nCurrent
                     ::scroll( HTSCROLLUNITINC )
                  NEXT
               ELSE
                  FOR nCount := 1 TO nCurrent - nSize
                     ::scroll( HTSCROLLUNITDEC )
                  NEXT
               ENDIF

               nPrevMRow := nMRow
            ENDIF
         ENDIF
      ENDDO

   CASE nMethod == HTSCROLLUNITDEC

      IF ::nTopItem > 1
         ::nTopItem--
         ::oVScroll:current := ::scrollbarPos()
         ::display()
      ENDIF

   CASE nMethod == HTSCROLLUNITINC

      IF ( ::nTopItem + ::nBottom - ::nTop ) <= ::nItemCount + 1
         ::nTopItem++
         ::oVScroll:current := ::scrollbarPos()
         ::display()
      ENDIF

   CASE nMethod == HTSCROLLBLOCKDEC

      nPos     := ::nBottom - ::nTop - iif( ::lDropDown, 2, 1 )
      nTopItem := ::nTopItem - nPos
      IF ::nTopItem > 1
         ::nTopItem := Max( nTopItem, 1 )
         ::oVScroll:current := ::scrollbarPos()
         ::display()
      ENDIF

   CASE nMethod == HTSCROLLBLOCKINC

      nPos       := ::nBottom - ::nTop - 1
      nItemCount := ::nItemCount
      nTopItem   := ::nTopItem + nPos
      IF ::nTopItem < nItemCount - nPos + 1
         IF nTopItem + nPos - 1 > nItemCount
            nTopItem := nItemCount - nPos + 1
         ENDIF
         ::nTopItem := nTopItem
         ::oVScroll:current := ::scrollbarPos()
         ::display()
      ENDIF

   ENDCASE

   RETURN Self

METHOD select( xPos ) CLASS LISTBOX

   LOCAL nValue
   LOCAL nPos
   LOCAL cType := Valtype( xPos )

   DO CASE
   CASE cType == "C"
      nPos := ::findData( xPos )
      IF !( Valtype( ::xBuffer ) $ "CU" )
         ::xBuffer := nPos
      ELSEIF ::nValue == 0
         ::xBuffer := xPos
      ELSE
         ::xBuffer := _LISTBOX_ITEMDATA( ::aItems[ nPos ] )
      ENDIF
   CASE !( cType == "N" )
      RETURN ::nValue
   CASE xPos < 1
      RETURN ::nValue
   CASE xPos > ::nItemCount
      RETURN ::nValue
   CASE xPos == ::nValue
      RETURN ::nValue
   OTHERWISE
      nPos := xPos
      IF Valtype( ::xBuffer ) $ "NU"
         ::xBuffer := nPos
      ELSEIF nPos == 0
         ::xBuffer := ""
      ELSE
         ::xBuffer := _LISTBOX_ITEMDATA( ::aItems[ nPos ] )
      ENDIF
   ENDCASE
   ::nValue := nPos

   ::cTextValue := iif( nPos == 0, "", _LISTBOX_ITEMDATA( ::aItems[ nPos ] ) )

   nPos := iif( Empty( ::cHotBox + ::cColdBox ), 0, 2 )

   nValue := ::nValue - ( ::nBottom - ::nTop - nPos )
   IF ::nTopItem <= nValue
      ::nTopItem := nValue
      IF ::oVScroll != NIL
         ::oVScroll:current := ::scrollbarPos()
      ENDIF
   ELSEIF ::nValue != 0 .AND. ::nTopItem > ::nValue
      ::nTopItem := ::nValue
      IF ::oVScroll != NIL
         ::oVScroll:current := ::scrollbarPos()
      ENDIF
   ENDIF

   ::display()

   IF ISBLOCK( ::bSBlock )
      Eval( ::bSBlock )
   ENDIF

   RETURN ::nValue

METHOD setData( nPos, cData ) CLASS LISTBOX

   IF nPos >= 1 .AND. nPos <= ::nItemCount
      ::aItems[ nPos ][ _ITEM_cDATA ] := cData
   ENDIF

   RETURN Self

METHOD setFocus() CLASS LISTBOX

   IF ! ::lHasFocus

      ::nCursor := SetCursor( SC_NONE )
      ::lHasFocus := .T.

      ::display()

      IF ISBLOCK( ::bFBlock )
         Eval( ::bFBlock )
      ENDIF

   ENDIF

   RETURN Self

METHOD setItem( nPos, aItem ) CLASS LISTBOX

   IF nPos >= 1 .AND. nPos <= ::nItemCount .AND. ;
      Len( aItem ) == 2 .AND. ;
      ISCHARACTER( aItem[ _ITEM_cTEXT ] )

      ::aItems[ nPos ] := aItem
   ENDIF

   RETURN Self

METHOD setText( nPos, cText ) CLASS LISTBOX

   IF nPos >= 1 .AND. nPos <= ::nItemCount
      ::aItems[ nPos ][ _ITEM_cTEXT ] := cText
   ENDIF

   RETURN Self

/* -------------------------------------------- */

METHOD changeItem( nOldPos, nNewPos ) CLASS LISTBOX

   LOCAL nValue

   IF nOldPos != nNewPos

      ::nValue := nNewPos
      ::cTextValue := iif( ::nValue == 0, "", _LISTBOX_ITEMDATA( ::aItems[ ::nValue ] ) )

      IF ::xBuffer == NIL
      ELSEIF ISNUMBER( ::xBuffer )
         ::xBuffer := ::nValue
      ELSEIF ::nValue > 0
         ::xBuffer := ::cTextValue
      ENDIF

      IF ::nTopItem > ::nValue
         ::nTopItem := ::nValue
         IF ::oVScroll != NIL
            ::oVScroll:current := ::scrollbarPos()
         ENDIF
      ELSE
         nValue := ::nValue - ( ::nBottom - ::nTop - iif( Empty( ::cHotBox + ::cColdBox ), 0, 2 ) + iif( ::lDropDown, 1, 0 ) )

         IF ::nTopItem <= nValue
            ::nTopItem := nValue
            IF ::oVScroll != NIL
               ::oVScroll:current := ::scrollbarPos()
            ENDIF
         ENDIF
      ENDIF

      ::display()

      IF ISBLOCK( ::bSBlock )
         Eval( ::bSBlock )
      ENDIF
   ENDIF

   RETURN Self

METHOD scrollbarPos() CLASS LISTBOX

   LOCAL nSize     := ::nBottom - ::nTop - iif( ::lDropDown, 2, 1 )
   LOCAL nCount    := ::nItemCount
   LOCAL nLength   := ::oVScroll:barLength

   RETURN ( ( nCount - nLength ) * ::nTopItem + nLength - nSize ) / ( nCount - nSize )

/* -------------------------------------------- */

METHOD bitmap( cBitmap ) CLASS LISTBOX

   IF cBitmap != NIL .AND. ::lDropDown
      ::cBitmap := __eInstVar53( Self, "BITMAP", cBitmap, "C", 1001 )
   ENDIF

   RETURN ::cBitmap

METHOD bottom( nBottom ) CLASS LISTBOX

   IF nBottom != NIL
      ::nBottom := __eInstVar53( Self, "BOTTOM", nBottom, "N", 1001 )
      IF ::oVScroll != NIL
         ::oVScroll:end := ::nBottom - 1
      ENDIF
   ENDIF

   RETURN ::nBottom

METHOD buffer() CLASS LISTBOX
   RETURN ::xBuffer

METHOD capCol( nCapCol ) CLASS LISTBOX

   IF nCapCol != NIL
      ::nCapCol := __eInstVar53( Self, "CAPCOL", nCapCol, "N", 1001 )
   ENDIF

   RETURN ::nCapCol

METHOD capRow( nCapRow ) CLASS LISTBOX

   IF nCapRow != NIL
      ::nCapRow := __eInstVar53( Self, "CAPROW", nCapRow, "N", 1001 )
   ENDIF

   RETURN ::nCapRow

METHOD caption( cCaption ) CLASS LISTBOX

   IF cCaption != NIL
      ::cCaption := __eInstVar53( Self, "CAPTION", cCaption, "C", 1001 )
      IF ::nCapCol == NIL
         ::nCapRow := ::nTop
         ::nCapCol := ::nLeft - Len( ::cCaption )
      ENDIF
   ENDIF

   RETURN ::cCaption

METHOD coldBox( cColdBox ) CLASS LISTBOX

   IF cColdBox != NIL
      ::cColdBox := __eInstVar53( Self, "COLDBOX", cColdBox, "C", 1001, {|| Len( cColdBox ) == 0 .OR. Len( cColdBox ) == 8 } )
   ENDIF

   RETURN ::cColdBox

METHOD colorSpec( cColorSpec ) CLASS LISTBOX

   IF cColorSpec != NIL
      ::cColorSpec := __eInstVar53( Self, "COLORSPEC", cColorSpec, "C", 1001,;
         iif( ::lDropDown,;
            {|| !Empty( hb_ColorIndex( cColorSpec, 7 ) ) .AND. Empty( hb_ColorIndex( cColorSpec, 8 ) ) },;
            {|| !Empty( hb_ColorIndex( cColorSpec, 6 ) ) .AND. Empty( hb_ColorIndex( cColorSpec, 7 ) ) } ) )
   ENDIF

   RETURN ::cColorSpec

METHOD dropDown( lDropDown ) CLASS LISTBOX

   IF lDropDown != NIL

      ::lDropDown := __eInstVar53( Self, "DROPDOWN", lDropDown, "L", 1001 )

      IF !::lDropDown .AND. !::lIsOpen
         ::lIsOpen := .T.
      ENDIF

      ::display()
   ENDIF

   RETURN ::lDropDown

METHOD fBlock( bFBlock ) CLASS LISTBOX
   
   IF PCount() > 0
      ::bFBlock := iif( bFBlock == NIL, NIL, __eInstVar53( Self, "FBLOCK", bFBlock, "B", 1001 ) )
   ENDIF

   RETURN ::bFBlock

METHOD hasFocus() CLASS LISTBOX
   RETURN ::lHasFocus

METHOD hotBox( cHotBox ) CLASS LISTBOX

   IF cHotBox != NIL
      ::cHotBox := __eInstVar53( Self, "HOTBOX", cHotBox, "C", 1001, {|| Len( cHotBox ) == 0 .OR. Len( cHotBox ) == 8 } )
   ENDIF

   RETURN ::cHotBox

METHOD isOpen() CLASS LISTBOX
   RETURN ::lIsOpen

METHOD itemCount() CLASS LISTBOX
   RETURN ::nItemCount

METHOD left( nLeft ) CLASS LISTBOX

   IF nLeft != NIL
      ::nLeft := __eInstVar53( Self, "LEFT", nLeft, "N", 1001 )
   ENDIF

   RETURN ::nLeft

METHOD message( cMessage ) CLASS LISTBOX

   IF cMessage != NIL
      ::cMessage := __eInstVar53( Self, "MESSAGE", cMessage, "C", 1001 )
   ENDIF

   RETURN ::cMessage

METHOD right( nRight ) CLASS LISTBOX

   IF nRight != NIL
      ::nRight := __eInstVar53( Self, "RIGHT", nRight, "N", 1001 )
      IF ::oVScroll != NIL
         ::oVScroll:offset := ::nRight
      ENDIF
   ENDIF

   RETURN ::nRight

METHOD sBlock( bSBlock ) CLASS LISTBOX
   
   IF PCount() > 0
      ::bSBlock := iif( bSBlock == NIL, NIL, __eInstVar53( Self, "SBLOCK", bSBlock, "B", 1001 ) )
   ENDIF

   RETURN ::bSBlock

METHOD style( cStyle ) CLASS LISTBOX

   IF cStyle != NIL
      ::cStyle := __eInstVar53( Self, "STYLE", cStyle, "C", 1001, {|| Len( cStyle ) == 1 } )
   ENDIF

   RETURN ::cStyle

METHOD textValue() CLASS LISTBOX
   RETURN ::cTextValue

METHOD top( nTop ) CLASS LISTBOX

   IF nTop != NIL
      ::nTop := __eInstVar53( Self, "TOP", nTop, "N", 1001 )
      IF ::oVScroll != NIL
         ::oVScroll:start := ::nTop + 1
      ENDIF
   ENDIF

   RETURN ::nTop

METHOD topItem( nTopItem ) CLASS LISTBOX

   IF nTopItem != NIL

      __eInstVar53( Self, "TOPITEM", nTopItem, "N", 1001, {|| nTopItem > 0 .AND. nTopItem <= ::nItemCount } )

      nTopItem := Min( nTopItem, ::nItemCount - ( ::nBottom - ::nTop - iif( Empty( ::cHotBox + ::cColdBox ), 0, 2 ) ) )

      IF ::nTopItem != nTopItem
         ::nTopItem := nTopItem

         IF ::oVScroll != NIL
            ::oVScroll:current := ::scrollbarPos()
         ENDIF

         ::display()
      ENDIF
   ENDIF

   RETURN ::nTopItem

METHOD typeOut() CLASS LISTBOX
   RETURN ::nItemCount == 0

METHOD value() CLASS LISTBOX
   RETURN ::nValue

METHOD vScroll( oVScroll ) CLASS LISTBOX

   IF PCount() > 0
      IF oVScroll == NIL
         ::oVScroll := NIL
      ELSE
         ::oVScroll := __eInstVar53( Self, "VSCROLL", oVScroll, "O", 1001, {|| oVScroll:ClassName() == "SCROLLBAR" .AND. oVScroll:orient == SCROLL_VERTICAL } )
         ::oVScroll:total := ::nItemCount
      ENDIF
   ENDIF

   RETURN ::oVScroll

/* -------------------------------------------- */

METHOD New( nTop, nLeft, nBottom, nRight, lDropDown )

   LOCAL cColor

   IF !ISNUMBER( nTop ) .OR. ;
      !ISNUMBER( nLeft ) .OR. ;
      !ISNUMBER( nBottom ) .OR. ;
      !ISNUMBER( nRight )
      RETURN NIL
   ENDIF

   DEFAULT lDropDown TO .F.

   ::nBottom   := nBottom
   ::nRight    := nRight
   ::nTop      := nTop
   ::nLeft     := nLeft
   ::nCapCol   := nLeft
   ::nCapRow   := nTop
   ::lIsOpen   := !lDropDown
   ::lDropDown := lDropDown
   ::aSaveScr  := { nTop + 1, nleft, nBottom, nRight, SaveScreen( nTop + 1, nLeft, nBottom, nRight ) }

   IF IsDefColor()
      ::cColorSpec := "W/N,W+/N,W+/N,N/W,W/N,W/N,W+/N,W/N"
   ELSE
      cColor := SetColor()
      ::cColorSpec := hb_ColorIndex( cColor, CLR_UNSELECTED ) + "," +;
                      hb_ColorIndex( cColor, CLR_UNSELECTED ) + "," +;
                      hb_ColorIndex( cColor, CLR_UNSELECTED ) + "," +;
                      hb_ColorIndex( cColor, CLR_ENHANCED   ) + "," +;
                      hb_ColorIndex( cColor, CLR_BORDER     ) + "," +;
                      hb_ColorIndex( cColor, CLR_STANDARD   ) + "," +;
                      hb_ColorIndex( cColor, CLR_BACKGROUND )
   ENDIF

   RETURN Self

FUNCTION ListBox( nTop, nLeft, nBottom, nRight, lDropDown )
   RETURN HBListBox():New( nTop, nLeft, nBottom, nRight, lDropDown )

FUNCTION _LISTBOX_( nTop, nLeft, nBottom, nRight, nSelect, aItems, cCaption,;
                    cMessage, cColorSpec, bFBlock, bSBlock, lDropDown, lIsOpen, cBitmap )

   LOCAL o := HBListBox():New( nTop, nLeft, nBottom, nRight, lDropDown )

   LOCAL nPos
   LOCAL nLen
   LOCAL xItem

   IF o != NIL

      IF ISCHARACTER( cCaption )
         o:caption := cCaption
         o:capCol  := nLeft - __CapLength( cCaption )
      ENDIF

      o:colorSpec := cColorSpec
      o:message   := cMessage
      o:fBlock    := bFBlock
      o:sBlock    := bSBlock
      o:isOpen    := lIsOpen

      nLen := Len( aItems )
      FOR nPos := 1 TO nLen

         xItem := aItems[ nPos ]

         IF ! ISARRAY( xItem )
            o:addItem( xItem )
         ELSEIF Len( xItem ) == _ITEM_cTEXT
            o:addItem( xItem[ _ITEM_cTEXT ] )
         ELSE
            o:addItem( xItem[ _ITEM_cTEXT ], xItem[ _ITEM_cDATA ] )
         ENDIF
      NEXT

      IF ISLOGICAL( lIsOpen ) .AND. lIsOpen
         IF ISLOGICAL( lDropDown ) .AND. lDropDown
            nTop++
         ENDIF
         o:VScroll := ScrollBar( nTop + 1, nBottom - 1, nRight,, SCROLL_VERTICAL )
      ENDIF

      IF ISCHARACTER( cBitmap )
         o:bitmap := cBitmap
      ENDIF

      o:select( nSelect )
   ENDIF

   RETURN o

#endif
