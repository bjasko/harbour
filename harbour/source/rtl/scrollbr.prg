/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * ScrollBar class
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

CREATE CLASS SCROLLBAR FUNCTION HBScrollBar

   EXPORTED:

   VAR cargo

   METHOD barLength() SETGET
   METHOD bitmaps( aBitmaps ) SETGET
   METHOD colorSpec( cColorSpec ) SETGET
   METHOD current( nCurrent ) SETGET
   METHOD end( nEnd ) SETGET
   METHOD offset( nOffset ) SETGET
   METHOD orient( nOrient ) SETGET
   METHOD sBlock( bSBlock ) SETGET
   METHOD start( nStart ) SETGET
   METHOD style( cStyle ) SETGET
   METHOD thumbPos( nThumbPos ) SETGET
   METHOD total( nTotal ) SETGET

   METHOD display()
   METHOD update()
   METHOD hitTest( nMRow, nMCol )

   METHOD New( nStart, nEnd, nOffset, bSBlock, nOrient ) /* NOTE: This method is a Harbour extension [vszakats] */

   PROTECTED:

   VAR aBitmaps
   VAR nBarLength
   VAR cColorSpec
   VAR cStyle
   VAR nCurrent   INIT 1
   VAR nEnd       INIT 0
   VAR nOffset
   VAR nOrient
   VAR nStart     INIT 0
   VAR nThumbPos  INIT 1
   VAR nTotal     INIT 100
   VAR bSBlock

   VAR lOverride  INIT .F.

   METHOD CalcThumbPos()

ENDCLASS

METHOD display() CLASS SCROLLBAR

   LOCAL cOldColor
   LOCAL nOldRow
   LOCAL nOldCol
   LOCAL lOldMCur

   LOCAL cStyle
   LOCAL nOffset
   LOCAL nStart
   LOCAL nEnd
   LOCAL nPos

   IF ::CalcThumbPos()

      cOldColor := SetColor()      
      nOldRow := Row()             
      nOldCol := Col()             
      lOldMCur := MSetCursor( .F. )

      cStyle    := ::cStyle
      nOffset   := ::nOffset
      nStart    := ::nStart
      nEnd      := ::nEnd - 1
      
      DispBegin()
      
      SetColor( hb_ColorIndex( ::cColorSpec, 0 ) )
      
      IF ::nOrient == SCROLL_VERTICAL

         FOR nPos := nStart + 1 TO nEnd
            DispOutAt( nPos, nOffset, SubStr( cStyle, 2, 1 ) )
         NEXT

         SetColor( hb_ColorIndex( ::cColorSpec, 1 ) )
         DispOutAt( nStart, nOffset, SubStr( cStyle, 1, 1 ) )
         DispOutAt( nStart + ::nThumbPos, nOffset, SubStr( cStyle, 3, 1 ) )
         DispOutAt( nEnd + 1, nOffset, SubStr( cStyle, 4, 1 ) )
      ELSE

         DispOutAt( nOffset, nStart + 1, Replicate( SubStr( cStyle, 2, 1 ), nEnd - nStart ) )
      
         SetColor( hb_ColorIndex( ::cColorSpec, 1 ) )
         DispOutAt( nOffset, nStart, SubStr( cStyle, 1, 1 ) )
         DispOutAt( nOffset, nStart + ::nThumbPos, SubStr( cStyle, 3, 1 ) )
         DispOutAt( nOffset, nEnd + 1, SubStr( cStyle, 4, 1 ) )

      ENDIF
      
      DispEnd()
      
      MSetCursor( lOldMCur )
      SetColor( cOldColor )
      SetPos( nOldRow, nOldCol )

      RETURN .T.
   ENDIF

   RETURN .F.

METHOD update() CLASS SCROLLBAR

   LOCAL nOldRow
   LOCAL nOldCol
   LOCAL lOldMCur

   LOCAL nThumbPos

   IF ISBLOCK( ::bSBlock )
      Eval( ::bSBlock )
   ENDIF

   IF ::CalcThumbPos() .AND. nThumbPos != ::nThumbPos

      nOldRow := Row()             
      nOldCol := Col()             
      lOldMCur := MSetCursor( .F. )

      DispBegin()

      nThumbPos := ::nThumbPos

      IF ::nOrient == SCROLL_VERTICAL
         DispOutAt( ::nStart + nThumbPos, ::nOffSet, SubStr( ::cStyle, 2, 1 ), hb_ColorIndex( ::cColorSpec, 0 ) )
         DispOutAt( ::nStart + nThumbPos, ::nOffset, SubStr( ::cStyle, 3, 1 ), hb_ColorIndex( ::cColorSpec, 1 ) )
      ELSE
         DispOutAt( ::nOffset, ::nStart + nThumbPos, SubStr( ::cStyle, 2, 1 ), hb_ColorIndex( ::cColorSpec, 0 ) )
         DispOutAt( ::nOffset, ::nStart + nThumbPos, SubStr( ::cStyle, 3, 1 ), hb_ColorIndex( ::cColorSpec, 1 ) )
      ENDIF

      DispEnd()

      MSetCursor( lOldMCur )
      SetPos( nOldRow, nOldCol )

      RETURN .T.
   ENDIF

   RETURN .F.

METHOD hitTest( nMRow, nMCol ) CLASS SCROLLBAR

   IF ::nOrient == SCROLL_VERTICAL

      DO CASE
      CASE nMCol != ::nOffset
      CASE nMRow < ::nStart
      CASE nMRow > ::nEnd
      CASE nMRow == ::nStart
         RETURN HTSCROLLUNITDEC
      CASE nMRow == ::nEnd
         RETURN HTSCROLLUNITINC
      CASE nMRow < ::nThumbPos + ::nStart
         RETURN HTSCROLLBLOCKDEC
      CASE nMRow > ::nThumbPos + ::nStart
         RETURN HTSCROLLBLOCKINC
      CASE nMRow == ::nThumbPos + ::nStart
         RETURN HTSCROLLTHUMBDRAG
      ENDCASE

      IF nMCol == ::nOffset + 1 .OR. nMCol == ::nOffset

         DO CASE
         CASE nMCol != ::nOffset .AND. nMCol != ::nOffset + 1
         CASE nMRow < ::nStart
         CASE nMRow > ::nEnd
         CASE nMRow == ::nStart
            RETURN HTSCROLLUNITDEC
         CASE nMRow == ::nEnd
            RETURN HTSCROLLUNITINC
         CASE nMRow < ::nThumbPos + ::nStart
            RETURN HTSCROLLBLOCKDEC
         CASE nMRow > ::nThumbPos + ::nStart
            RETURN HTSCROLLBLOCKINC
         CASE nMRow == ::nThumbPos + ::nStart
            RETURN HTSCROLLTHUMBDRAG
         ENDCASE

      ENDIF

   ELSE

      DO CASE
      CASE nMRow != ::nOffset
      CASE nMCol < ::nStart
      CASE nMCol > ::nEnd
      CASE nMCol == ::nStart
         RETURN HTSCROLLUNITDEC
      CASE nMCol == ::nEnd
         RETURN HTSCROLLUNITINC
      CASE nMCol < ::nThumbPos + ::nStart
         RETURN HTSCROLLBLOCKDEC
      CASE nMCol > ::nThumbPos + ::nStart
         RETURN HTSCROLLBLOCKINC
      CASE nMCol == ::nThumbPos + ::nStart
         RETURN HTSCROLLTHUMBDRAG
      ENDCASE

   ENDIF

   RETURN HTNOWHERE

METHOD barLength() CLASS SCROLLBAR
   RETURN ::nBarLength

METHOD bitmaps( aBitmaps ) CLASS SCROLLBAR

   IF ISARRAY( aBitmaps ) .AND. ;
      Len( aBitmaps ) == 3

      ::aBitmaps := aBitmaps
   ENDIF

   RETURN ::aBitmaps

METHOD colorSpec( cColorSpec ) CLASS SCROLLBAR

   IF ISCHARACTER( cColorSpec ) .AND. ;
      !Empty( hb_ColorIndex( cColorSpec, 1 ) ) .AND. ;
       Empty( hb_ColorIndex( cColorSpec, 2 ) )

      ::cColorSpec := cColorSpec
   ENDIF

   RETURN ::cColorSpec

METHOD current( nCurrent ) CLASS SCROLLBAR

   IF ISNUMBER( nCurrent ) .AND. ;
      nCurrent <= ::nTotal .AND. ;
      nCurrent != ::nCurrent

      ::nCurrent := nCurrent
   ENDIF

   RETURN ::nCurrent

METHOD end( nEnd ) CLASS SCROLLBAR

   IF ISNUMBER( nEnd ) .AND. ;
      nEnd >= ::nStart .AND. ;
      nEnd != ::nEnd

      ::nEnd := nEnd
      ::nBarLength := nEnd - ::nStart - 1
   ENDIF

   RETURN ::nEnd

METHOD offset( nOffset ) CLASS SCROLLBAR

   IF ISNUMBER( nOffset ) .AND. ;
      nOffset != ::nOffset

      ::nOffset := nOffset
   ENDIF

   RETURN ::nOffset

METHOD orient( nOrient ) CLASS SCROLLBAR

   IF ISNUMBER( nOrient ) .AND. ;
      ( nOrient == SCROLL_VERTICAL .OR. nOrient == SCROLL_HORIZONTAL )

      ::nOrient := nOrient
   ENDIF

   RETURN ::nOrient

METHOD sBlock( bSBlock ) CLASS SCROLLBAR

   IF ISBLOCK( bSBlock )
      ::bSBlock := bSBlock
   ENDIF

   RETURN ::bSBlock

METHOD start( nStart ) CLASS SCROLLBAR

   IF ISNUMBER( nStart ) .AND. ;
      nStart <= ::nEnd .AND. ;
      nStart != ::nStart

      ::nStart := nStart
      ::nBarLength := ::nEnd - nStart - 1
   ENDIF

   RETURN ::nStart

METHOD style( cStyle ) CLASS SCROLLBAR

   IF ISCHARACTER( cStyle ) .AND. ;
      Len( cStyle ) == 4

      ::cStyle := cStyle
   ENDIF

   RETURN ::cStyle

METHOD thumbPos( nThumbPos ) CLASS SCROLLBAR

   IF ISNUMBER( nThumbPos )

      IF nThumbPos < 1
         ::nThumbPos := 1
      ELSEIF nThumbPos >= ::nBarLength
         ::nThumbPos := ::nBarLength
      ELSEIF nThumbPos >= ::nBarLength - 1
         ::nThumbPos := nThumbPos
      ELSE
         ::nThumbPos := nThumbPos
      ENDIF

      ::lOverride := ( nThumbPos != 0 )
   ENDIF

   RETURN ::nThumbPos

METHOD total( nTotal ) CLASS SCROLLBAR

   IF ISNUMBER( nTotal ) .AND. ;
      nTotal >= 2 .AND. ;
      nTotal != ::nTotal

      ::nTotal := nTotal
   ENDIF

   RETURN ::nTotal

METHOD CalcThumbPos() CLASS SCROLLBAR

   LOCAL nBarLength := ::nBarLength
   LOCAL nTotal := ::nTotal

   IF nBarLength < 2 .OR. nTotal < 2
      RETURN .F.
   ENDIF

   IF !::lOverride
      ::ThumbPos := Min( Max( Round( ( ( nBarLength - 1 ) * ::nCurrent + nTotal - 2 * nBarLength + 1 ) / ( nTotal - nBarLength ), 0 ), 1 ), nBarLength )
   ENDIF

   RETURN .T.

/* New definitions for better coding. These are screen codepage dependent, 
   but can be changed with the setStyle method. */
#define SB_UPARROW      Chr( 24 )
#define SB_DNARROW      Chr( 25 )
#define SB_THUMB        Chr( 176 )
#define SB_TRACK        Chr( 178 )
#define SB_LEFTARROW    Chr( 27 )
#define SB_RIGHTARROW   Chr( 26 )

METHOD New( nStart, nEnd, nOffset, bSBlock, nOrient ) CLASS SCROLLBAR

   LOCAL cColor

   DEFAULT nOrient TO SCROLL_VERTICAL

   IF !ISNUMBER( nStart ) .OR. ;
      !ISNUMBER( nEnd ) .OR. ;
      !ISNUMBER( nOffset ) .OR. ;
      !ValType( bSBlock ) $ "BU" .OR. ;
      !ISNUMBER( nOrient ) .OR. ;
      ( nOrient != SCROLL_VERTICAL .AND. nOrient != SCROLL_HORIZONTAL )
      RETURN NIL
   ENDIF

   ::end        := nEnd
   ::offSet     := nOffset
   ::orient     := nOrient
   ::sBlock     := bSBlock
   ::start      := nStart
   ::nBarLength := nEnd - nStart - 1

   IF nOrient == SCROLL_VERTICAL
      ::cStyle := SB_UPARROW + SB_THUMB + SB_TRACK + SB_DNARROW
      ::aBitmaps := { "arrow_u.bmu", "arrow_d.bmu", "arrow_e.bmu" }
   ELSEIF nOrient == SCROLL_HORIZONTAL
      ::cStyle := SB_LEFTARROW + SB_THUMB + SB_TRACK + SB_RIGHTARROW
      ::aBitmaps := { "arrow_l.bmu", "arrow_r.bmu", "arrow_e.bmu" }
   ENDIF

   cColor := SetColor()
   ::cColorSpec := hb_ColorIndex( cColor, CLR_UNSELECTED ) + "," + ;
                   hb_ColorIndex( cColor, CLR_ENHANCED )

   RETURN Self

FUNCTION ScrollBar( nStart, nEnd, nOffset, bSBlock, nOrient )
   RETURN HBScrollBar():New( nStart, nEnd, nOffset, bSBlock, nOrient )

#endif
