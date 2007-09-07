/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * RADIOBUTTON class
 *
 * Copyright 2000 Luiz Rafael Culik <culik@sl.conex.net>
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

/* NOTE: Harbour doesn't support CA-Cl*pper 5.3 GUI functionality, but 
         it has all related variables and methods. */

/* NOTE: CA-Cl*pper 5.3 uses a mixture of QQOut(), DevOut(), Disp*() 
         functions to generate screen output. Harbour uses Disp*() 
         functions only. [vszakats] */

#ifdef HB_COMPAT_C53

CREATE CLASS RADIOBUTTN FUNCTION HBRadioButton

   EXPORT:

   VAR cargo                                 /* NOTE: CA-Clipper 5.3 has a bug, where this var is filled with NIL everytime its value is read ( cargo := o:cargo ). */

   METHOD display()
   METHOD hitTest( nRow, nCol )
   METHOD isAccel( xKey )
   METHOD killFocus()
   METHOD select( lState )
   METHOD setFocus()

   METHOD bitmaps( aBitmaps ) SETGET
   METHOD buffer() SETGET
   METHOD data( cData ) SETGET               /* NOTE: Undocumented CA-Cl*pper 5.3 method. */
   METHOD capCol( nCapCol ) SETGET
   METHOD capRow( nCapRow ) SETGET
   METHOD caption( cCaption ) SETGET
   METHOD col( nCol ) SETGET
   METHOD colorSpec( cColorSpec ) SETGET
   METHOD fBlock( bFBlock ) SETGET
   METHOD hasFocus() SETGET
   METHOD row( nRow ) SETGET
   METHOD sBlock( bSBlock ) SETGET
   METHOD style( cStyle ) SETGET

   METHOD New( nRow, nCol, cCaption, cData ) /* NOTE: This method is a Harbour extension [vszakats] */

   PROTECTED:

   VAR aBitmaps   INIT { "radio_f.bmu", "radio_e.bmu" }
   VAR lBuffer    INIT .F.
   VAR cData
   VAR nCapCol
   VAR nCapRow
   VAR cCaption
   VAR nCol
   VAR cColorSpec
   VAR bFBlock
   VAR lHasFocus  INIT .F.
   VAR nRow
   VAR bSBlock
   VAR cStyle     INIT "(* )"

ENDCLASS

METHOD setFocus() CLASS RADIOBUTTN

   IF !::lHasFocus
      ::lHasFocus := .T.
      ::display()

      IF ISBLOCK( ::bFBlock )
         Eval( ::bFBlock )
      ENDIF
   ENDIF

   RETURN Self

METHOD select( lState ) CLASS RADIOBUTTN

   LOCAL lOldState := ::lBuffer

   ::lBuffer := iif( ISLOGICAL( lState ), lState, !::lBuffer )

   IF lOldState != ::lBuffer .AND. ;
      ISBLOCK( ::bSBlock )

      Eval( ::bSBlock )
   ENDIF

   RETURN Self

METHOD killFocus() CLASS RADIOBUTTN
   
   IF ::lHasFocus
      ::lHasFocus := .F.

      IF ISBLOCK( ::bFBlock )
         Eval( ::bFBlock )
      ENDIF

      ::display()
   ENDIF

   RETURN Self

METHOD display() CLASS RADIOBUTTN
   
   LOCAL cOldColor := SetColor()      
   LOCAL nOldRow := Row()             
   LOCAL nOldCol := Col()             
   LOCAL lOldMCur := MSetCursor( .F. )

   LOCAL cStyle := ::cStyle
   LOCAL nPos
   LOCAL cOldCaption

   DispBegin()

   SetColor( iif( ::lBuffer, __GUIColor( ::cColorSpec, 4 ), __GUIColor( ::cColorSpec, 2 ) ) )
   SetPos( ::nRow, ::nCol )
   DispOut( Left( cStyle, 1 ) )
   DispOut( iif( ::lBuffer, SubStr( cStyle, 2, 1 ), SubStr( cStyle, 3, 1 ) ) )
   DispOut( Right( cStyle, 1 ) )

   IF !Empty( cOldCaption := ::cCaption )

      IF ( nPos := At( "&", cOldCaption ) ) == 0
      ELSEIF nPos == Len( cOldCaption )
         nPos := 0
      ELSE
         cOldCaption := Stuff( cOldCaption, nPos, 1, "" )
      ENDIF

      DispOutAt( ::nCapRow, ::nCapCol, cOldCaption, __GUIColor( ::cColorSpec, 5 ) )

      IF nPos != 0
         DispOutAt( ::nCapRow, ::nCapCol + nPos - 1, SubStr( cOldCaption, nPos, 1 ), iif( ::lHasfocus, __GUIColor( ::cColorSpec, 7 ), __GUIColor( ::cColorSpec, 6 ) ) )
      ENDIF
   ENDIF

   DispEnd()

   MSetCursor( lOldMCur )
   SetColor( cOldColor )
   SetPos( nOldRow, nOldCol )

   RETURN Self

METHOD isAccel( xKey ) CLASS RADIOBUTTN
   
   LOCAL nPos
   LOCAL cCaption

   IF ISNUMBER( xKey )
      xKey := Chr( xKey )
   ELSEIF !ISCHARACTER( xKey )
      RETURN .F.
   ENDIF

   cCaption := ::cCaption

   RETURN ( nPos := At( "&", cCaption ) ) > 0 .AND. ;
          Lower( SubStr( cCaption, nPos + 1, 1 ) ) == Lower( xKey )

METHOD hitTest( nRow, nCol ) CLASS RADIOBUTTN

   LOCAL nPos
   LOCAL nLen

   IF nRow == ::Row .AND. ;
      nCol >= ::Col .AND. ;
      nCol < ::Col + 3
      RETURN HTCLIENT
   ENDIF

   nLen := Len( ::cCaption )

   IF ( nPos := At( "&", ::cCaption ) ) == 0 .AND. nPos < nLen
      nLen--
   ENDIF

   IF nRow == ::CapRow .AND. ;
      nCol >= ::CapCol .AND. ;
      nCol < ::CapCol + nLen
      RETURN HTCLIENT
   ENDIF

   RETURN HTNOWHERE

METHOD bitmaps( aBitmaps ) CLASS RADIOBUTTN

   IF aBitmaps != NIL
      ::aBitmaps := _eInstVar( Self, "BITMAPS", aBitmaps, "A", 1001, {|| Len( aBitmaps ) == 2 } )
   ENDIF

   RETURN ::aBitmaps

METHOD buffer() CLASS RADIOBUTTN
   RETURN ::lBuffer

METHOD data( cData ) CLASS RADIOBUTTN
   
   IF PCount() > 0
      ::cData := iif( cData == NIL, NIL, _eInstVar( Self, "DATA", cData, "C", 1001 ) )
   ENDIF

   RETURN iif( ::cData == NIL, __Caption( ::Caption ), ::cData )

METHOD capCol( nCapCol ) CLASS RADIOBUTTN

   IF nCapCol != NIL
      ::nCapCol := _eInstVar( Self, "CAPCOL", nCapCol, "N", 1001 )
   ENDIF

   RETURN ::nCapCol

METHOD capRow( nCapRow ) CLASS RADIOBUTTN

   IF nCapRow != NIL
      ::nCapRow := _eInstVar( Self, "CAPROW", nCapRow, "N", 1001 )
   ENDIF

   RETURN ::nCapRow

METHOD caption( cCaption ) CLASS RADIOBUTTN

   IF cCaption != NIL
      ::cCaption := _eInstVar( Self, "CAPTION", cCaption, "C", 1001 )
   ENDIF

   RETURN ::cCaption

METHOD col( nCol ) CLASS RADIOBUTTN

   IF nCol != NIL
      ::nCol := _eInstVar( Self, "COL", nCol, "N", 1001 )
   ENDIF

   RETURN ::nCol

METHOD colorSpec( cColorSpec ) CLASS RADIOBUTTN

   IF cColorSpec != NIL
      ::cColorSpec := _eInstVar( Self, "COLORSPEC", cColorSpec, "C", 1001,;
         {|| !Empty( __GUIColor( cColorSpec, 7 ) ) .AND. Empty( __GUIColor( cColorSpec, 8 ) ) } )
   ENDIF

   RETURN ::cColorSpec

METHOD fBlock( bFBlock ) CLASS RADIOBUTTN
   
   IF PCount() > 0
      ::bFBlock := iif( bFBlock == NIL, NIL, _eInstVar( Self, "FBLOCK", bFBlock, "B", 1001 ) )
   ENDIF

   RETURN ::bFBlock

METHOD hasFocus() CLASS RADIOBUTTN
   RETURN ::lHasFocus

METHOD row( nRow ) CLASS RADIOBUTTN

   IF nRow != NIL
      ::nRow := _eInstVar( Self, "ROW", nRow, "N", 1001 )
   ENDIF

   RETURN ::nRow

METHOD sBlock( bSBlock ) CLASS RADIOBUTTN
   
   IF PCount() > 0
      ::bSBlock := iif( bSBlock == NIL, NIL, _eInstVar( Self, "SBLOCK", bSBlock, "B", 1001 ) )
   ENDIF

   RETURN ::bSBlock

METHOD style( cStyle ) CLASS RADIOBUTTN

   IF cStyle != NIL
      ::cStyle := _eInstVar( Self, "STYLE", cStyle, "C", 1001, {|| Len( cStyle ) == 0 .OR. Len( cStyle ) == 4 } )
   ENDIF

   RETURN ::cStyle

METHOD New( nRow, nCol, cCaption, cData ) CLASS RADIOBUTTN

   LOCAL cColor

   IF !ISNUMBER( nRow ) .OR. ;
      !ISNUMBER( nCol )
      RETURN NIL
   ENDIF

   IF !ISCHARACTER( cCaption )
      cCaption := ""
   ENDIF

   ::nCapRow  := nRow
   ::nCapCol  := nCol + 3 + 1
   ::cCaption := cCaption
   ::nCol     := nCol
   ::nRow     := nRow
   ::cData    := cData /* NOTE: Every type is allowed here to be fully compatible */

   IF IsDefColor()
      ::cColorSpec := "W/N,W+/N,W+/N,N/W,W/N,W/N,W+/N"
   ELSE
      cColor := SetColor()
      ::cColorSpec := __GUIColor( cColor, CLR_UNSELECTED + 1 ) + "," +;
                      __GUIColor( cColor, CLR_UNSELECTED + 1 ) + "," +;
                      __GUIColor( cColor, CLR_ENHANCED   + 1 ) + "," +;
                      __GUIColor( cColor, CLR_ENHANCED   + 1 ) + "," +;
                      __GUIColor( cColor, CLR_STANDARD   + 1 ) + "," +;
                      __GUIColor( cColor, CLR_STANDARD   + 1 ) + "," +;
                      __GUIColor( cColor, CLR_BACKGROUND + 1 )
   ENDIF

   RETURN Self

FUNCTION RadioButto( nRow, nCol, cCaption, cData ) /* NOTE: cData argument is undocumented */
   RETURN HBRadioButton():New( nRow, nCol, cCaption, cData )

#ifdef HB_EXTENSION

FUNCTION RadioButton( nRow, nCol, cCaption, cData ) /* NOTE: cData argument is undocumented */
   RETURN HBRadioButton():New( nRow, nCol, cCaption, cData )

#endif

#endif
