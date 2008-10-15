/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Text file browser class
 *
 * Copyright 2008 Lorenzo Fiorini <lorenzo.fiorini@gmail.com>
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

#pragma DEBUGINFO=OFF

#include "hbclass.ch"

CREATE CLASS HBBrwText

   VAR cFileName
   VAR aRows
   VAR nRows
   VAR nActiveLine
   VAR aBreakPoints INIT {}
   VAR lLineNumbers
   VAR nRow
   VAR nFirstCol
   VAR nCol

   VAR oBrw

   VAR cCurLine
   VAR nLineOffset
   VAR nMaxLineLen

   VAR nTop
   VAR nLeft
   VAR nBottom
   VAR nRight

   VAR nWidth
   VAR nHeight

   METHOD New( nTop, nLeft, nBottom, nRight, cFileName, cColors, lLineNumbers )

   METHOD RefreshAll() INLINE ::oBrw:ForceStable():RefreshAll(), Self
   METHOD ForceStable() INLINE ::oBrw:ForceStable(), Self
   METHOD RefreshCurrent() INLINE ::oBrw:RefreshCurrent(), Self
   METHOD GotoLine( n )
   METHOD SetActiveLine( n )
   METHOD GetLine( n )
   METHOD ToggleBreakPoint( nRow, lSet )
   METHOD Search( cString, lCaseSensitive, nMode )

   METHOD GoFirst()
   METHOD GoLast()
   METHOD Skip( n )
   METHOD GoNext()
   METHOD GoPrev()

   METHOD Resize( nTop, nLeft, nBottom, nRight )
   METHOD GetLineColor()

   METHOD Up() INLINE ::oBrw:Up():ForceStable(), Self
   METHOD Down() INLINE ::oBrw:Down():ForceStable(), Self
   METHOD PageUp() INLINE ::oBrw:PageUp():ForceStable(), Self
   METHOD PageDown() INLINE ::oBrw:PageDown():ForceStable(), Self
   METHOD GoTop() INLINE ::oBrw:GoTop():ForceStable(), Self
   METHOD GoBottom() INLINE ::oBrw:GoBottom():ForceStable(), Self

   METHOD Right() INLINE IIF( ::nLineOffset < ::nMaxLineLen, ( ::nLineOffset++, ::RefreshAll() ), ), Self
   METHOD Left() INLINE IIF( ::nLineOffset > 1, ( ::nLineOffset--, ::RefreshAll() ), ), Self

   METHOD RowPos() INLINE ::nRow

   METHOD LoadFile( cFileName )

ENDCLASS

METHOD New( nTop, nLeft, nBottom, nRight, cFileName, cColors, lLineNumbers ) CLASS HBBrwText

   LOCAL oCol

   ::nTop := nTop
   ::nLeft := nLeft
   ::nBottom := nBottom
   ::nRight := nRight

   ::nWidth := nRight - nLeft + 1
   ::nHeight := nBottom - nTop

   ::lLineNumbers := lLineNumbers

   ::oBrw := HBDbBrowser():New( ::nTop, ::nLeft, ::nBottom, ::nRight )

   ::oBrw:colorSpec := cColors

   oCol := HBDbColumnNew( "", {|| ::GetLine() } )

   oCol:colorBlock := {|| ::GetLineColor() }

   ::oBrw:AddColumn( oCol )

   ::oBrw:goTopBlock := {|| ::nRow := 1 }
   ::oBrw:goBottomBlock := {|| ::nRow := ::nRows }
   ::oBrw:skipBlock := {| n | ::Skip( n ) }

   IF !Empty( cFileName )
      ::LoadFile( cFileName )
   ENDIF

   RETURN Self

METHOD GotoLine( n ) CLASS HBBrwText

   ::oBrw:MoveCursor( n - ::nRow )
   ::RefreshAll()

   RETURN Self

METHOD SetActiveLine( n ) CLASS HBBrwText

   ::nActiveLine := n
   ::RefreshAll()

   RETURN Self

METHOD GetLine() CLASS HBBrwText

   RETURN PadR( AllTrim( Str( ::nRow ) ) + ": " + SubStr( ::aRows[ ::nRow ], ::nLineOffset ), ::nWidth )

METHOD ToggleBreakPoint( nRow, lSet) CLASS HBBrwText

   LOCAL nAt := AScan( ::aBreakPoints, nRow )

   IF lSet
      // add it only if not present
      IF nAt == 0
         AAdd( ::aBreakPoints, nRow)
      ENDIF
   ELSE
      IF nAt != 0
         ADel( ::aBreakPoints, nAt )
         ASize( ::aBreakPoints, Len( ::aBreakPoints ) - 1 )
      ENDIF
   ENDIF

   RETURN Self

METHOD LoadFile( cFileName ) CLASS HBBrwText

   LOCAL nMaxLineLen := 0
   LOCAL cLine

   ::cFileName := cFileName
   ::aRows := Text2Array( MemoRead( cFileName ) )
   ::nRows := Len( ::aRows )

   FOR EACH cLine in ::aRows
      nMaxLineLen := Max( nMaxLineLen, Len( cLine ) )
   NEXT
   ::nMaxLineLen := nMaxLineLen
   ::nLineOffset := 1

   RETURN NIL

METHOD Resize( nTop, nLeft, nBottom, nRight ) CLASS HBBrwText

   LOCAL lResize := .F.

   IF nTop != NIL .AND. nTop != ::nTop
      ::nTop := nTop
      lResize := .T.
   ENDIF
   IF nLeft != NIL .AND. nLeft != ::nLeft
      ::nLeft := nLeft
      lResize := .T.
   ENDIF
   IF nBottom != NIL .AND. nBottom != ::nBottom
      ::nBottom := nBottom
      lResize := .T.
   ENDIF
   IF nRight != NIL .AND. nRight != ::nRight
      ::nRight := nRight
      lResize := .T.
   ENDIF
   IF lResize
     ::oBrw:Resize( nTop, nLeft, nBottom, nRight )
     ::nWidth := nRight - nLeft + 1
   ENDIF

   RETURN Self

METHOD GetLineColor() CLASS HBBrwText

   LOCAL aColor
   LOCAL lBreak

   lBreak := AScan( ::aBreakPoints, ::nRow ) > 0

   IF lBreak
     aColor := { 3, 2 }
   ELSEIF ::nRow == ::nActiveLine
     aColor := { 4, 4 }
   ELSE
     aColor := { 1, 2 }
   ENDIF

   RETURN aColor

METHOD Search( cString, lCaseSensitive, nMode ) CLASS HBBrwText

   LOCAL bMove
   LOCAL lFound := .F.

   IF !lCaseSensitive
      cString := Upper( cString )
   ENDIF

   DO CASE
   CASE nMode == 0 // From Top
      ::GoTop()
      bMove := {|| ::Skip( 1 ) }
   CASE nMode == 1 // Forward
      bMove := {|| ::Skip( 1 ) }
   CASE nMode == 2 // Backward
      bMove := {|| ::Skip( -1 ) }
   ENDCASE

   DO WHILE Eval( bMove ) != 0
      IF cString $ IIF( lCaseSensitive, ::cCurLine, Upper( ::cCurLine ) )
         lFound := .T.
         ::RefreshAll()
         EXIT
      ENDIF
   ENDDO

   RETURN lFound

METHOD GoFirst() CLASS HBBrwText

   ::nRow := 1

   RETURN .T.

METHOD GoLast() CLASS HBBrwText

   ::nRow := ::nRows

   RETURN .T.

METHOD Skip( n ) CLASS HBBrwText

   LOCAL nSkipped := 0

   IF n > 0
      DO WHILE nSkipped != n .AND. ::GoNext()
         nSkipped++
      ENDDO
   ELSE
      DO WHILE nSkipped != n .AND. ::GoPrev()
         nSkipped--
      ENDDO
   ENDIF

   RETURN nSkipped

METHOD GoPrev() CLASS HBBrwText

   LOCAL lMoved := .F.

   IF ::nRow > 1
      ::nRow--
      lMoved := .T.
   ENDIF

   RETURN lMoved

METHOD GoNext() CLASS HBBrwText

   LOCAL lMoved := .F.

   IF ::nRow < ::nRows
      ::nRow++
      lMoved := .T.
   ENDIF

   RETURN lMoved

STATIC FUNCTION WhichEOL( cString )

   LOCAL nCRPos := At( Chr( 13 ), cString )
   LOCAL nLFPos := At( Chr( 10 ), cString )

   IF nCRPos > 0 .AND. nLFPos == 0
      RETURN Chr( 13 )
   ELSEIF nCRPos == 0 .AND. nLFPos >  0
      RETURN Chr( 10 )
   ELSEIF nCRPos > 0 .AND. nLFPos == nCRPos + 1
      RETURN Chr( 13 ) + Chr( 10 )
   ENDIF

   RETURN HB_OSNewLine()

STATIC FUNCTION Text2Array( cString )

   RETURN hb_aTokens( cString, WhichEOL( cString ) )
