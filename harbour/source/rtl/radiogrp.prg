/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * RADIOGROUP class
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

#include "button.ch"
#include "color.ch"
#include "common.ch"
#include "setcurs.ch"

/* NOTE: Harbour doesn't support CA-Cl*pper 5.3 GUI functionality, but 
         it has all related variables and methods. */

/* NOTE: CA-Cl*pper 5.3 uses a mixture of QQOut(), DevOut(), Disp*() 
         functions to generate screen output. Harbour uses Disp*() 
         functions only. [vszakats] */

#ifdef HB_COMPAT_C53

CREATE CLASS RADIOGROUP FUNCTION HBRadioGroup

   EXPORT:

   VAR cargo

   METHOD addItem( oRadioButton )
   METHOD delItem( oRadioButton )
   METHOD display()
   METHOD getAccel( oRadioButton )
   METHOD getItem( oRadioButton )
   METHOD hitTest( nMouseRow, nMouseCol )
   METHOD insItem( nPos, oRadioButton )
   METHOD killFocus()
   METHOD nextItem()
   METHOD prevItem()
   METHOD select( xPos )
   METHOD setColor( cColorSpec )
   METHOD setFocus()
   METHOD setStyle( cStyle )

   METHOD bottom( nBottom ) SETGET
   METHOD buffer() SETGET
   METHOD capCol( nCapCol ) SETGET
   METHOD capRow( nCapRow ) SETGET
   METHOD caption( cCaption ) SETGET
   METHOD coldBox( cColdBox ) SETGET
   METHOD colorSpec( cColorSpec ) SETGET
   METHOD fBlock( bFBlock ) SETGET
   METHOD hasFocus() SETGET
   METHOD hotBox( cHotBox ) SETGET
   METHOD itemCount() SETGET
   METHOD left( nLeft ) SETGET
   METHOD message( cMessage ) SETGET
   METHOD right( nRight ) SETGET
   METHOD textValue() SETGET                  /* NOTE: Undocumented CA-Cl*pper var. */
   METHOD top( nTop ) SETGET
   METHOD typeOut() SETGET
   METHOD value() SETGET                      /* NOTE: Undocumented CA-Cl*pper var. */

   METHOD New( nTop, nLeft, nBottom, nRight ) /* NOTE: This method is a Harbour extension [vszakats] */

   PROTECTED:

   VAR nBottom
   VAR xBuffer
   VAR nCapCol
   VAR nCapRow
   VAR cCaption   INIT ""
   VAR cColdBox   INIT Chr( 218 ) + Chr( 196 ) + Chr( 191 ) + Chr( 179 ) + Chr( 217 ) + Chr( 196 ) + Chr( 192 ) + Chr( 179 )
   VAR cColorSpec
   VAR bFBlock
   VAR lHasFocus  INIT .F.
   VAR cHotBox    INIT Chr( 201 ) + Chr( 205 ) + Chr( 187 ) + Chr( 186 ) + Chr( 188 ) + Chr( 205 ) + Chr( 200 ) + Chr( 186 )
   VAR nItemCount INIT 0
   VAR nLeft
   VAR cMessage   INIT ""
   VAR nRight
   VAR cTextValue INIT ""
   VAR nTop
   VAR nValue     INIT 0

   VAR aItems     INIT {}
   VAR nCursor    INIT 0

   METHOD changeButton( xVal, nPos )

ENDCLASS

METHOD addItem( oRadioButton ) CLASS RADIOGROUP

   IF ISOBJECT( oRadioButton ) .AND. oRadioButton:ClassName() == "RADIOBUTTN"
      AAdd( ::aItems, oRadioButton )
      ::nItemCount++
   ENDIF

   RETURN Self

METHOD delItem( nPos ) CLASS RADIOGROUP

   IF nPos >= 1 .AND. nPos <= ::nItemCount
      Adel( ::aItems[ nPos ] )
      ASize( ::aItems, --::nItemCount )
   ENDIF

   IF ::lHasFocus .AND. ::nItemCount < ::nValue
      ::nValue := ::nItemCount
      ::cTextValue := ::aItems[ ::nValue ]:data
      ::xBuffer := iif( ISNUMBER( ::xBuffer ), ::nValue, ::cTextValue )
   ENDIF

   RETURN Self

METHOD display() CLASS RADIOGROUP

   LOCAL cOldColor := SetColor()      
   LOCAL nOldRow := Row()             
   LOCAL nOldCol := Col()             
   LOCAL lOldMCur := MSetCursor( .F. )

   LOCAL cSelBox
   LOCAL cUnSelBox
   LOCAL cCaption
   LOCAL nPos

   DispBegin()

   IF ::lHasFocus
      cSelBox := ::cHotBox
      cUnSelBox := ::cColdbox
   ELSE
      cSelBox := ::cColdbox
      cUnSelBox := ::cHotBox
   ENDIF

   SetColor( __GUIColor( ::cColorSpec, 1 ) )

   IF !Empty( cSelBox )
      DispBox( ::nTop, ::nLeft, ::nBottom, ::nRight, cSelBox )
   ELSEIF !Empty( cUnSelBox )
      DispBox( ::nTop, ::nLeft, ::nBottom, ::nRight, cUnSelBox )
   ENDIF

   IF !Empty( cCaption := ::cCaption )

      IF !( ( nPos := At( "&", cCaption ) ) == 0 )
         IF nPos == Len( cCaption )
            nPos := 0
         ELSE
            cCaption := Stuff( cCaption, nPos, 1, "" )
         ENDIF
      ENDIF

      DispOutAt( ::nCapRow, ::nCapCol, cCaption, __GUIColor( ::cColorSpec, 2 ) )

      IF nPos != 0
         DispOutAt( ::nCapRow, ::nCapCol + nPos - 1, SubStr( cCaption, nPos, 1 ), __GUIColor( ::cColorSpec, 3 ) )
      ENDIF

   ENDIF

   AEval( ::aItems, {| o | o:display() } )

   DispEnd()

   MSetCursor( lOldMCur )
   SetColor( cOldColor )
   SetPos( nOldRow, nOldCol )

   RETURN Self

METHOD getAccel( xValue ) CLASS RADIOGROUP

   DO CASE
   CASE ISNUMBER( xValue )
      xValue := Chr( xValue )
   CASE !ISCHARACTER( xValue )
      RETURN 0
   ENDCASE

   xValue := Lower( xValue )

   RETURN AScan( ::aItems, {| o | o:isAccel( xValue ) } )

METHOD getItem( nPos ) CLASS RADIOGROUP
   RETURN iif( nPos >= 1 .AND. nPos <= ::nItemCount, ::aItems[ nPos ], NIL )

METHOD hitTest( nMouseRow, nMouseCol ) CLASS RADIOGROUP

   LOCAL nLen
   LOCAL nPos
   LOCAL aItems

   DO CASE
   CASE Empty( ::cColdbox + ::cHotBox )
   CASE nMouseRow == ::nTop
      IF nMouseCol == ::nLeft
         RETURN HTTOPLEFT
      ELSEIF nMouseCol == ::nRight
         RETURN HTTOPRIGHT
      ELSEIF nMouseCol >= ::nLeft .AND. nMouseCol <= ::nRight
         RETURN HTTOP
      ENDIF
   CASE nMouseRow == ::nBottom
      IF nMouseCol == ::nLeft
         RETURN HTBOTTOMLEFT
      ELSEIF nMouseCol == ::nRight
         RETURN HTBOTTOM
      ELSEIF nMouseCol >= ::nLeft .AND. nMouseCol <= ::nRight
         RETURN HTBOTTOMRIGHT
      ENDIF
   CASE nMouseCol == ::nLeft
      IF nMouseRow >= ::nTop .AND. nMouseRow <= ::nBottom
         RETURN HTLEFT
      ELSE
         RETURN HTNOWHERE
      ENDIF
   CASE nMouseCol == ::nRight
      IF nMouseRow >= ::nTop .AND. nMouseRow <= ::nBottom
         RETURN HTRIGHT
      ELSE
         RETURN HTNOWHERE
      ENDIF
   ENDCASE

   nLen := Len( ::cCaption )

   IF ( nPos := At( "&", ::cCaption ) ) == 0
   ELSEIF nPos < nLen
      nLen--
   ENDIF

   DO CASE
   CASE Empty( ::cCaption )
   CASE nMouseRow != ::nCapRow
   CASE nMouseCol < ::nCapCol
   CASE nMouseCol < ::nCapCol + nLen
      RETURN HTCAPTION
   ENDCASE

   DO CASE
   CASE nMouseRow < ::nTop
   CASE nMouseRow > ::nBottom
   CASE nMouseCol < ::nLeft
   CASE nMouseCol <= ::nRight
      aItems := ::aItems
      nLen := ::nItemCount
      FOR nPos := 1 TO nLen
         IF aItems[ nPos ]:hitTest( nMouseRow, nMouseCol ) != HTNOWHERE
            RETURN nPos
         ENDIF
      NEXT
      RETURN HTCLIENT
   ENDCASE

   RETURN HTNOWHERE

METHOD insItem( nPos, oRadioButtom ) CLASS RADIOGROUP

   IF ISOBJECT( oRadioButtom ) .AND. oRadioButtom:ClassName() == "RADIOBUTTN" .AND. ;
      nPos < ::nItemCount

      ASize( ::aItems, ++::nItemCount )
      AIns( ::aItems, nPos, oRadioButtom )
      ::aItems[ nPos ] := oRadioButtom
   ENDIF

   RETURN ::aItems[ nPos ]

METHOD killFocus() CLASS RADIOGROUP

   LOCAL nPos
   LOCAL nLen
   LOCAL aItems

   LOCAL nCurMCur

   IF ::lHasFocus

      ::lHasFocus := .F.

      IF ISBLOCK( ::bFBlock )
         Eval( ::bFBlock )
      ENDIF

      aItems := ::aItems
      nLen := ::nItemCount

      nCurMCur := MSetCursor( .F. )

      DispBegin()

      FOR nPos := 1 TO nLen
         aItems[ nPos ]:killFocus()
      NEXT

      ::display()

      DispEnd()

      MSetCursor( nCurMCur )
      SetCursor( ::nCursor )

   ENDIF

   RETURN Self

METHOD setFocus() CLASS RADIOGROUP

   LOCAL nPos
   LOCAL nLen
   LOCAL aItems

   LOCAL nCurMCur

   IF !::lHasFocus

      ::nCursor := SetCursor( SC_NONE )
      ::lHasFocus := .T.

      aItems := ::aItems
      nLen := ::nItemCount

      nCurMCur := MSetCursor( .F. )

      DispBegin()

      FOR nPos := 1 TO nLen
         aItems[ nPos ]:setFocus()
      NEXT

      ::display()

      DispEnd()

      MSetCursor( nCurMCur )

      IF ISBLOCK( ::bFBlock )
         Eval( ::bFBlock )
      ENDIF
   ENDIF

   RETURN Self

METHOD nextItem() CLASS RADIOGROUP
   LOCAL nValue

   IF ::lHasFocus .AND. ::nItemCount > 0
      ::changeButton( nValue := ::nValue, iif( nValue == ::nItemCount, 1, nValue + 1 ) )
   ENDIF

   RETURN Self

METHOD prevItem() CLASS RADIOGROUP

   LOCAL nValue
   LOCAL nPos

   IF ::lHasFocus .AND. ::nItemCount > 0

      nValue := ::nValue

      DO CASE
      CASE nValue == 0 ; nPos := 1
      CASE nValue == 1 ; nPos := ::nItemCount
      OTHERWISE        ; nPos := nValue - 1
      ENDCASE

      ::changeButton( nValue, nPos )

   ENDIF

   RETURN Self

METHOD select( xValue ) CLASS RADIOGROUP

   LOCAL cType := ValType( xValue )
   LOCAL nPos
   LOCAL nLen

   IF cType == "C"

      nLen := ::nItemCount
      FOR nPos := 1 TO nLen
         IF ::aItems[ nPos ]:data == xValue

            DEFAULT ::xBuffer TO ""
            ::changeButton( ::nValue, nPos )

            EXIT
         ENDIF
      NEXT

      IF nPos > nLen
         ::xBuffer := xValue
      ENDIF

   ELSEIF cType == "N" .AND. xValue >= 1 .AND. xValue <= ::nItemCount

      DEFAULT ::xBuffer TO 0
      ::changeButton( ::nValue, xValue )

   ENDIF

   RETURN Self

METHOD setColor( cColorSpec ) CLASS RADIOGROUP

   LOCAL nPos
   LOCAL nLen := ::nItemCount
   LOCAL aItems := ::aItems

   FOR nPos := 1 TO nLen
      aItems[ nPos ]:colorSpec := cColorSpec
   NEXT

   RETURN Self

METHOD setStyle( cStyle ) CLASS RADIOGROUP
   
   LOCAL nPos
   LOCAL nLen := ::nItemCount
   LOCAL aItems := ::aItems

   FOR nPos := 1 TO nLen
      aItems[ nPos ]:style := cStyle
   NEXT

   RETURN Self

METHOD changeButton( nUnselect, nSelect ) CLASS RADIOGROUP
   LOCAL nCurMCur := MSetCursor( .F. )

   IF nUnselect != nSelect

      DispBegin()

      IF nUnselect > 0
         ::aItems[ nUnselect ]:select( .F. )
         ::aItems[ nUnselect ]:display()
      ENDIF
      IF nSelect > 0
         ::aItems[ nSelect ]:select( .T. )
         ::aItems[ nSelect ]:display()
      ENDIF

      DispEnd()

      ::nValue := nSelect
      ::cTextValue := ::aItems[ nSelect ]:data
      ::xBuffer := iif( ISNUMBER( ::xBuffer ), nSelect, ::cTextValue )

   ENDIF

   MSetCursor( nCurMCur )

   RETURN Self

METHOD bottom( nBottom ) CLASS RADIOGROUP

   IF nBottom != NIL
      ::nBottom := _eInstVar( Self, "BOTTOM", nBottom, "N", 1001 )
   ENDIF

   RETURN ::nBottom

METHOD buffer() CLASS RADIOGROUP
   RETURN ::xBuffer

METHOD capCol( nCapCol ) CLASS RADIOGROUP

   IF nCapCol != NIL
      ::nCapCol := _eInstVar( Self, "CAPCOL", nCapCol, "N", 1001 )
   ENDIF

   RETURN ::nCapCol

METHOD capRow( nCapRow ) CLASS RADIOGROUP

   IF nCapRow != NIL
      ::nCapRow := _eInstVar( Self, "CAPROW", nCapRow, "N", 1001 )
   ENDIF

   RETURN ::nCapRow

METHOD caption( cCaption ) CLASS RADIOGROUP

   IF cCaption != NIL
      ::cCaption := _eInstVar( Self, "CAPTION", cCaption, "C", 1001 )
   ENDIF

   RETURN ::cCaption

METHOD coldBox( cColdBox ) CLASS RADIOGROUP

   IF cColdBox != NIL
      ::cColdBox := _eInstVar( Self, "COLDBOX", cColdBox, "C", 1001, {|| Len( cColdBox ) == 0 .OR. Len( cColdBox ) == 8 } )
   ENDIF

   RETURN ::cColdBox

METHOD colorSpec( cColorSpec ) CLASS RADIOGROUP

   IF cColorSpec != NIL
      ::cColorSpec := _eInstVar( Self, "COLORSPEC", cColorSpec, "C", 1001,;
         {|| !Empty( __GUIColor( cColorSpec, 3 ) ) .AND. Empty( __GUIColor( cColorSpec, 4 ) ) } )
   ENDIF

   RETURN ::cColorSpec

METHOD fBlock( bFBlock ) CLASS RADIOGROUP
   
   IF PCount() > 0
      ::bFBlock := iif( bFBlock == NIL, NIL, _eInstVar( Self, "FBLOCK", bFBlock, "B", 1001 ) )
   ENDIF

   RETURN ::bFBlock

METHOD hasFocus() CLASS RADIOGROUP
   RETURN ::lHasFocus

METHOD hotBox( cHotBox ) CLASS RADIOGROUP

   IF cHotBox != NIL
      ::cHotBox := _eInstVar( Self, "HOTBOX", cHotBox, "C", 1001, {|| Len( cHotBox ) == 0 .OR. Len( cHotBox ) == 8 } )
   ENDIF

   RETURN ::cHotBox

METHOD itemCount() CLASS RADIOGROUP
   RETURN ::nItemCount

METHOD left( nLeft ) CLASS RADIOGROUP

   IF nLeft != NIL
      ::nLeft := _eInstVar( Self, "LEFT", nLeft, "N", 1001 )
   ENDIF

   RETURN ::nLeft

METHOD message( cMessage ) CLASS RADIOGROUP

   IF cMessage != NIL
      ::cMessage := _eInstVar( Self, "MESSAGE", cMessage, "C", 1001 )
   ENDIF

   RETURN ::cMessage

METHOD right( nRight ) CLASS RADIOGROUP

   IF nRight != NIL
      ::nRight := _eInstVar( Self, "RIGHT", nRight, "N", 1001 )
   ENDIF

   RETURN ::nRight

METHOD textValue() CLASS RADIOGROUP 
   RETURN ::cTextValue

METHOD top( nTop ) CLASS RADIOGROUP

   IF nTop != NIL
      ::nTop := _eInstVar( Self, "TOP", nTop, "N", 1001 )
   ENDIF

   RETURN ::nTop

METHOD typeOut() CLASS RADIOGROUP
   RETURN ::nItemCount == 0 .OR. ::nValue > ::nItemCount

METHOD value() CLASS RADIOGROUP
   RETURN ::nValue

METHOD New( nTop, nLeft, nBottom, nRight ) CLASS RADIOGROUP

   LOCAL cColor

   IF !ISNUMBER( nTop ) .OR. ;
      !ISNUMBER( nLeft ) .OR. ;
      !ISNUMBER( nBottom ) .OR. ;
      !ISNUMBER( nRight )
      RETURN NIL
   ENDIF

   ::nTop    := nTop
   ::nLeft   := nLeft
   ::nBottom := nBottom
   ::nRight  := nRight
   ::nCapCol := nLeft + 2
   ::nCapRow := nTop

   IF IsDefColor()
      ::cColorSpec := "W/N,W/N,W+/N"
   ELSE
      cColor := SetColor()
      ::cColorSpec := __GUIColor( cColor, CLR_BORDER     + 1 ) + "," + ;
                      __GUIColor( cColor, CLR_STANDARD   + 1 ) + "," + ;
                      __GUIColor( cColor, CLR_BACKGROUND + 1 )
   ENDIF

   RETURN Self

FUNCTION RadioGroup( nTop, nLeft, nBottom, nRight )
   RETURN HBRadioGroup():New( nTop, nLeft, nBottom, nRight )

FUNCTION _RADIOGRP_( nTop, nLeft, nBottom, nRight, xValue, aItems, cCaption, cMessage, cColorSpec, bFBlock )

   LOCAL o := RadioGroup( nTop, nLeft, nBottom, nRight )

   IF o != NIL

      o:caption := cCaption
      o:message := cMessage
      o:colorSpec := cColorSpec
      o:fBlock := bFBlock

      AEval( aItems, {| aItem | o:AddItem( aItem ) } )

      o:select( xValue )

   ENDIF

   RETURN o

#endif
