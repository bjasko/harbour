/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * GD graphic library chart class
 *
 * Copyright 2004-2005 Francesco Saverio Giudice <info@fsgiudice.com>
 * www - http://www.xharbour.org http://www.harbour-project.org
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

/*
 *
 * See doc/license files for licensing terms.
 *
 */


#include "common.ch"
#include "hbclass.ch"
#include "gd.ch"

#define DEFAULT( x, y ) IIF( x == NIL, x := y, )

CLASS GDChart FROM GDImage

   DATA cTitle
   DATA cAxisX
   DATA cAxisY
   DATA nWidth
   DATA nHeight
   DATA nScaleX
   DATA nScaleY

   DATA aSeries
   DATA aDataOfHashes         // Hash contains graph datas
   DATA hDefs


   METHOD New( sx, sy )  CONSTRUCTOR
   METHOD AddData()
   METHOD AddDef()
   METHOD SetData()
   METHOD SetDefs()

   METHOD PieChart()
   METHOD VerticalBarChart()
   METHOD HorizontalBarChart()
   METHOD LineChart()

   // clone method for gdchart
   METHOD Clone()

   PROTECTED:
   METHOD CloneDataFrom()

ENDCLASS

METHOD New( sx, sy ) CLASS GDChart

   DEFAULT sx TO 320
   DEFAULT sy TO 200

   ::cTitle := "Chart"
   ::aSeries        := {}
   ::hDefs          := {=>}
   ::aDataOfHashes  := {}

   ::Create( sx, sy )
RETURN Self

METHOD AddData( hData ) CLASS GDChart
   IF ValType( hData ) == "H"
      aAdd( ::aDataOfHashes, hData )
   ENDIF
RETURN Self

METHOD SetData( aData ) CLASS GDChart
   IF ValType( aData ) == "A"
      ::aDataOfHashes := aData
   ENDIF
RETURN Self

METHOD AddDef( cDefKey, xDefVal ) CLASS GDChart
   IF ValType( cDefKey ) == "C"
      #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
      hSet( ::hDefs, Upper( cDefKey ), xDefVal )
      #else
      HB_hSet( ::hDefs, Upper( cDefKey ), xDefVal )
      #endif
   ENDIF
RETURN Self

METHOD SetDefs( hDefs ) CLASS GDChart
   IF ValType( hDefs ) == "H"
      ::hDefs := hDefs
   ENDIF
RETURN Self

METHOD PieChart() CLASS GDChart
  LOCAL hElement, nTot := 0
  LOCAL nDegree := 0
  LOCAL lFilled, lExtruded, nExtrude, nTotExtr := 0, pTile
  LOCAL colorp
  LOCAL nVal, nDim
  LOCAL nPosX, nPosY
  LOCAL cLabel, hFont, cFontName, nPitch, nAngle, textcolor
  LOCAL x, y, nWidth
  LOCAL aPieDataOfHash, hDefs
  LOCAL cFontPitch

  aPieDataOfHash := ::aDataOfHashes
  hDefs          := ::hDefs

  x          := HGetValue( hDefs, "POSX" )
  y          := HGetValue( hDefs, "POSY" )
  nWidth     := HGetValue( hDefs, "WIDTH" )
  cFontPitch := HGetValue( hDefs, "FONTPITCH" )

  DEFAULT x          TO ::CenterWidth()
  DEFAULT y          TO ::CenterHeight()
  DEFAULT nWidth     TO Min( ::Width(), ::Height() )
  DEFAULT cFontPitch TO "TINY"

  DO CASE
     CASE cFontPitch == "TINY"
          ::SetFontTiny()
     CASE cFontPitch == "SMALL"
          ::SetFontSmall()
     CASE cFontPitch == "MEDIUM"
          ::SetFontMediumBold()
     CASE cFontPitch == "LARGE"
          ::SetFontLarge()
     CASE cFontPitch == "GIANT"
          ::SetFontGiant()
  ENDCASE

  //__OutDebug( "x, y, nWidth", x, y, nWidth )


  /*
    hData := ["TITLE"], ["VALUE"], ["FILLED"], ["COLOR"], ["TILE"], ["EXTRUDE"]
  */

  // Before sum of values to determine perentual
  FOR EACH hElement IN aPieDataOfHash
      nTot += hElement["VALUE"]
      // Check extrution
      IF ( nExtrude  := HGetValue( hElement, "EXTRUDE" ) ) <> NIL
         nTotExtr := Max( nTotExtr, nExtrude )
      ENDIF
  NEXT

  nWidth -= ( nTotExtr + 2 ) * 2

  //__OutDebug( "nTotExtr, nWidth", nTotExtr, nWidth )

  // Second,
  FOR EACH hElement IN aPieDataOfHash
      cLabel    := HGetValue( hElement, "LABEL" )
      lFilled   := HGetValue( hElement, "FILLED" )
      nExtrude  := HGetValue( hElement, "EXTRUDE" )
      pTile     := HGetValue( hElement, "TILE" )
      IF nExtrude <> NIL
         lExtruded := TRUE
      ELSE
         lExtruded := FALSE
      ENDIF
      colorp    := HGetValue( hElement, "COLOR" )
      nVal      := hElement["VALUE"]
      nDim      := 360 * ( ( nVal / nTot ) * 100 ) / 100
      DEFAULT lFilled  TO FALSE
      DEFAULT nExtrude TO 0
      DEFAULT colorp   TO ::SetColor( 0, 0, 0 )
      IF lExtruded
         nPosX   := x + nExtrude * cos(::Radians( nDegree + nDim / 2 ))
         nPosY   := y + nExtrude * sin(::Radians( nDegree + nDim / 2 ))
      ELSE
         nPosX   := x
         nPosY   := y
      ENDIF
      IF pTile <> NIL
         ::SetTile( pTile )
         colorp := gdTiled
      ELSE
         if ISARRAY( colorp )
            colorp := ::SetColor( colorp[1], colorp[2], colorp[3] )
         endif
      ENDIF
      IF lFilled
         ::Arc( nPosX, nPosY, nWidth, nWidth, nDegree, nDegree + nDim, TRUE, colorp, gdPie )
      ELSE
         ::Arc( nPosX, nPosY, nWidth, nWidth, nDegree, nDegree + nDim, TRUE, colorp, gdNoFill + gdEdged )
      ENDIF
      IF cLabel <> NIL
         //hFont := HGetValue( hElement, "FONT" )
         //IF hFont == NIL
         //   ::SetFontMediumBold()
            cFontName := NIL
            nPitch    := NIL
            nAngle    := NIL
            textcolor := NIL
         //ELSE
         //   cFontName := HGetValue( hFont, "NAME" )
         //   nPitch    := HGetValue( hFont, "PITCH" )
         //   nAngle    := HGetValue( hFont, "ANGLE" )
         //   textcolor := HGetValue( hFont, "COLOR" )
         //   DEFAULT cFontName TO "Arial"
         //   DEFAULT nPitch    TO 8
         //   DEFAULT nAngle    TO 0
         //ENDIF
         nPosX   := nPosX + ( (nExtrude + nWidth) / 4 ) * cos(::Radians( nDegree + nDim / 2 ))
         nPosY   := nPosY + ( (nExtrude + nWidth) / 4 ) * sin(::Radians( nDegree + nDim / 2 ))
         IF textcolor == NIL
            colorp    := ::GetPixel( nPosX, nPosY )
            textcolor := ::SetColor( 255 - ::Red( colorp ), 255 - ::Green( colorp ), 255 - ::Blue( colorp ) )
         ENDIF
         //cTitle := LTrim( Str( nVal ) )
         IF hFont == NIL
            ::Say( nPosX, nPosY, cLabel, textcolor, gdAlignCenter )
         ELSE
            ::SayFreeType( nPosX, nPosY, cLabel, cFontName, nPitch, nAngle, textcolor, gdAlignCenter )
         ENDIF
      ENDIF

      nDegree += nDim + 0.1
  NEXT
RETURN Self

METHOD VerticalBarChart() CLASS GDChart
  LOCAL hElement, nTot := 0
  LOCAL nDegree := 0
  LOCAL lFilled, lExtruded, nExtrude, pTile
  LOCAL colorp
  LOCAL nVal, nDim
  LOCAL nPosX, nPosY
  LOCAL nSize, nMax
  LOCAL nBorder, nThick, n
  LOCAL x, y, nWidth, nHeight, nMaxValue, color, nMaxLabel, cLabel
  LOCAL lShowAxis, lShowGrid

  LOCAL nLeftLabelSpace   //:= 40
  LOCAL nRightLabelSpace  //:= 40
  LOCAL nBottomLabelSpace //:= 40
  LOCAL nTopLabelSpace    := 40
  LOCAL lShowLabelLeft    := TRUE
  LOCAL lShowLabelRight   := TRUE //FALSE
  LOCAL lShowLabelBottom  := TRUE
  LOCAL lShowLabelTop     := FALSE
  LOCAL cAxisPict
  LOCAL cFontPitch

  LOCAL aDataOfHash, hDefs

  aDataOfHash := ::aDataOfHashes
  hDefs       := ::hDefs

  x          := HGetValue( hDefs, "POSX" )
  y          := HGetValue( hDefs, "POSY" )
  nWidth     := HGetValue( hDefs, "WIDTH" )
  nHeight    := HGetValue( hDefs, "HEIGHT" )
  nMaxValue  := HGetValue( hDefs, "MAXVALUE" )
  color      := HGetValue( hDefs, "COLOR" )
  lShowAxis  := HGetValue( hDefs, "SHOWAXIS" )
  lShowGrid  := HGetValue( hDefs, "SHOWGRID" )
  cAxisPict  := HGetValue( hDefs, "AXISPICT" )
  cFontPitch := HGetValue( hDefs, "FONTPITCH" )

  DEFAULT x          TO 0
  DEFAULT y          TO 0
  DEFAULT nWidth     TO ::Width()
  DEFAULT nHeight    TO ::Height()
  DEFAULT color      TO ::GetColor()
  DEFAULT lShowAxis  TO TRUE
  DEFAULT lShowGrid  TO TRUE
  DEFAULT cAxisPict  TO "@E 9,999.99"
  DEFAULT cFontPitch TO "TINY"

  DEFAULT nBorder TO 4

  /*
    hData := ["TITLE"], ["VALUE"], ["FILLED"], ["COLOR"], ["TILE"], ["EXTRUDE"]
  */

  DO CASE
     CASE cFontPitch == "TINY"
          ::SetFontTiny()
     CASE cFontPitch == "SMALL"
          ::SetFontSmall()
     CASE cFontPitch == "MEDIUM"
          ::SetFontMediumBold()
     CASE cFontPitch == "LARGE"
          ::SetFontLarge()
     CASE cFontPitch == "GIANT"
          ::SetFontGiant()
  ENDCASE



  // Before sum of values to determine perentual
  nMaxLabel := 0
  nMax      := 0
  FOR EACH hElement IN aDataOfHash
      #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
      IF HB_EnumIndex() == 1
      #else
      IF hElement:__enumIndex() == 1
      #endif
         nMax := hElement["VALUE"]
      ELSE
         nMax := Max( nMax, hElement["VALUE"] )
      ENDIF
      cLabel    := HGetValue( hElement, "LABEL" )
      nMaxLabel := Max( nMaxLabel, Len( IIF( cLabel <> NIL, cLabel, "" ) ) )
      nTot      += hElement["VALUE"]
  NEXT

  //__OutDebug( "Len( LTrim( Str( nMax ) ) )", Len( LTrim( cStr( nMax ) ) ), Str( nMax ) )

  DEFAULT nLeftLabelSpace    TO nBorder + Len( LTrim( Transform( nMax, cAxisPict ) ) ) * ::GetFontWidth() + nBorder
  DEFAULT nRightLabelSpace   TO nLeftLabelSpace //nBorder + Len( LTrim( Str( nMax ) ) ) * ::GetFontWidth() + nBorder
  DEFAULT nBottomLabelSpace  TO nBorder + nMaxLabel * ::GetFontWidth() + nBorder
  DEFAULT nMaxValue          TO nMax

  IF lShowAxis
     IF lShowLabelLeft
        x       += nLeftLabelSpace
        nWidth  -= nLeftLabelSpace
     ENDIF
     IF lShowLabelRight
        nWidth  -= nRightLabelSpace
     ENDIF
     IF lShowLabelBottom
        y       += nBottomLabelSpace
        nHeight -= nBottomLabelSpace
     ENDIF
     IF lShowLabelTop
        nHeight -= nTopLabelSpace
     ENDIF
  ENDIF

  nSize := nWidth / Len( aDataOfHash )

  IF lShowGrid
     ::Rectangle( x, ::Height() - ( y + nHeight ), x + nWidth, ::Height() - y, FALSE, color )

     nThick := ::SetThickness( 1 )

     ::ResetStyles()
     ::AddStyle( color )
     ::AddStyle( color )
     ::AddStyle( color )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::SetStyle()
     FOR n := 10 TO 100 STEP 10
         nDim  := ( ( nMaxValue / 100 ) * n )
         nPosY := ( nDim / nMaxValue ) * nHeight
         //__OutDebug( "nDim", nDim )
         ::Line( x, ::Height() - ( y + nPosY), x + nWidth, ::Height() - ( y + nPosY ), gdStyled )
     NEXT
      ::SetThickness( nThick )
  ENDIF
  IF lShowAxis
     // Y Axis
     FOR n := 10 TO 100 STEP 10
         nDim  := ( ( nMaxValue / 100 ) * n )
         cLabel := LTrim( Transform( nDim, cAxisPict ) )
         nPosY := ( nDim / nMaxValue ) * nHeight
         IF lShowLabelLeft
            ::Say( x - nLeftLabelSpace + nBorder, ::Height() - ( y + nPosY ), PadL( cLabel, Len( LTrim( Transform( nMaxValue, cAxisPict ) ) ) ), color )
         ENDIF
         IF lShowLabelRight
            ::Say( x + nWidth + nBorder, ::Height() - ( y + nPosY ), cLabel, color )
         ENDIF
     NEXT
  ENDIF

  // Second,
  FOR EACH hElement IN aDataOfHash
      cLabel    := HGetValue( hElement, "LABEL" )
      lFilled   := HGetValue( hElement, "FILLED" )
      nExtrude  := HGetValue( hElement, "EXTRUDE" )
      pTile     := HGetValue( hElement, "TILE" )
      IF nExtrude <> NIL
         lExtruded := TRUE
      ELSE
         lExtruded := FALSE
      ENDIF
      colorp    := HGetValue( hElement, "COLOR" )
      nVal      := hElement["VALUE"]
      nDim      := ( nVal / nMaxValue ) * nHeight

      DEFAULT lFilled  TO FALSE
      DEFAULT nExtrude TO 0
      DEFAULT colorp   TO ::SetColor( 0, 0, 0 )

      #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
      nPosX   := x + ( nSize * ( HB_EnumIndex() - 1 ) )
      #else
      nPosX   := x + ( nSize * ( hElement:__enumIndex() - 1 ) )
      #endif
      nPosY   := y
      IF pTile <> NIL
         ::SetTile( pTile )
         colorp := gdTiled
      ELSE
         if ISARRAY( colorp )
            colorp := ::SetColor( colorp[1], colorp[2], colorp[3] )
         endif
      ENDIF
      ::Rectangle( nPosX + nBorder, ::Height() - ( nPosY + nDim ), nPosX + nSize - nBorder, ::Height() - nPosY, lFilled, colorp )

      IF lShowAxis
         // Y Axis
         IF lShowLabelBottom
            ::SayVertical( nPosX + nSize / 2 - ::GetFontHeight() / 2, ::Height() - nBorder, PadL( cLabel, nMaxLabel ), color )
         ENDIF
      ENDIF

  NEXT
RETURN Self

METHOD HorizontalBarChart() CLASS GDChart
  LOCAL hElement, nTot := 0
  LOCAL nDegree := 0
  LOCAL lFilled, lExtruded, nExtrude, pTile
  LOCAL colorp
  LOCAL nVal, nDim
  LOCAL nPosX, nPosY
  LOCAL nSize, nMax
  LOCAL nBorder, nThick, n
  LOCAL x, y, nWidth, nHeight, nMaxValue, color, nMaxLabel, cLabel
  LOCAL lShowAxis, lShowGrid

  LOCAL nLeftLabelSpace   //:= 40
  LOCAL nRightLabelSpace  //:= 40
  LOCAL nBottomLabelSpace //:= 40
  LOCAL nTopLabelSpace    //:= 40
  LOCAL lShowLabelLeft    := TRUE
  LOCAL lShowLabelRight   := TRUE
  LOCAL lShowLabelBottom  := TRUE
  LOCAL lShowLabelTop     := TRUE
  LOCAL cAxisPict
  LOCAL cFontPitch

  LOCAL aDataOfHash, hDefs

  aDataOfHash := ::aDataOfHashes
  hDefs       := ::hDefs

  x          := HGetValue( hDefs, "POSX" )
  y          := HGetValue( hDefs, "POSY" )
  nWidth     := HGetValue( hDefs, "WIDTH" )
  nHeight    := HGetValue( hDefs, "HEIGHT" )
  nMaxValue  := HGetValue( hDefs, "MAXVALUE" )
  color      := HGetValue( hDefs, "COLOR" )
  lShowAxis  := HGetValue( hDefs, "SHOWAXIS" )
  lShowGrid  := HGetValue( hDefs, "SHOWGRID" )
  cAxisPict  := HGetValue( hDefs, "AXISPICT" )
  cFontPitch := HGetValue( hDefs, "FONTPITCH" )

  DEFAULT x          TO 0
  DEFAULT y          TO 0
  DEFAULT nWidth     TO ::Width()
  DEFAULT nHeight    TO ::Height()
  DEFAULT color      TO ::GetColor()
  DEFAULT lShowAxis  TO TRUE
  DEFAULT lShowGrid  TO TRUE
  DEFAULT cAxisPict  TO "@E 9,999.99"
  DEFAULT cFontPitch TO "TINY"

  DEFAULT nBorder TO 4

  /*
    hData := ["TITLE"], ["VALUE"], ["FILLED"], ["COLOR"], ["TILE"], ["EXTRUDE"]
  */

  DO CASE
     CASE cFontPitch == "TINY"
          ::SetFontTiny()
     CASE cFontPitch == "SMALL"
          ::SetFontSmall()
     CASE cFontPitch == "MEDIUM"
          ::SetFontMediumBold()
     CASE cFontPitch == "LARGE"
          ::SetFontLarge()
     CASE cFontPitch == "GIANT"
          ::SetFontGiant()
  ENDCASE

  // Before sum of values to determine perentual
  nMaxLabel := 0
  nMax      := 0
  FOR EACH hElement IN aDataOfHash
      #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
      IF HB_EnumIndex() == 1
      #else
      IF hElement:__enumIndex() == 1
      #endif
         nMax := hElement["VALUE"]
      ELSE
         nMax := Max( nMax, hElement["VALUE"] )
      ENDIF
      cLabel    := HGetValue( hElement, "LABEL" )
      nMaxLabel := Max( nMaxLabel, Len( IIF( cLabel <> NIL, cLabel, "" ) ) )
      nTot      += hElement["VALUE"]
  NEXT
  DEFAULT nLeftLabelSpace    TO nBorder + nMaxLabel * ::GetFontWidth() + nBorder
  DEFAULT nRightLabelSpace   TO nBorder + ( Len( LTrim( Transform( nMax, cAxisPict ) ) ) * ::GetFontWidth() / 2 )
  DEFAULT nTopLabelSpace     TO nBorder + ::GetFontHeight() + nBorder
  DEFAULT nBottomLabelSpace  TO nTopLabelSpace // nBorder + ::GetFontHeight() + nBorder
  DEFAULT nMaxValue          TO nMax

  IF lShowAxis
     IF lShowLabelLeft
        x       += nLeftLabelSpace
        nWidth  -= nLeftLabelSpace
     ENDIF
     IF lShowLabelRight
        nWidth  -= nRightLabelSpace
     ENDIF
     IF lShowLabelBottom
        y       += nBottomLabelSpace
        nHeight -= nBottomLabelSpace
     ENDIF
     IF lShowLabelTop
        nHeight -= nTopLabelSpace
     ENDIF
  ENDIF

  nSize := nHeight / Len( aDataOfHash )

  IF lShowGrid
     ::Rectangle( x, ::Height() - ( y + nHeight ), x + nWidth, ::Height() - y, FALSE, color )

     nThick := ::SetThickness( 1 )

     ::ResetStyles()
     ::AddStyle( color )
     ::AddStyle( color )
     ::AddStyle( color )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::SetStyle()
     FOR n := 10 TO 100 STEP 10
         nDim  := ( ( nMaxValue / 100 ) * n )
         nPosX := ( nDim / nMaxValue ) * nWidth
         ::Line( x + nPosX, y, x + nPosX, y + nHeight, gdStyled )
     NEXT
      ::SetThickness( nThick )
  ENDIF
  IF lShowAxis
     // X Axis
     FOR n := 0 TO 100 STEP 10
         nDim   := ( ( nMaxValue / 100 ) * n )
         cLabel := LTrim( Transform( nDim, cAxisPict ) )
         nPosX  := ( nDim / nMaxValue ) * nWidth - ( ( Len( cLabel ) / 2 ) * ::GetFontWidth() )
         IF lShowLabelTop
            ::Say( x + nPosX, y - nTopLabelSpace + nBorder, cLabel, color )
         ENDIF
         IF lShowLabelBottom
            ::Say( x + nPosX, y + nHeight + nBorder, cLabel, color )
         ENDIF
     NEXT
  ENDIF

  // Second,
  FOR EACH hElement IN aDataOfHash
      cLabel    := HGetValue( hElement, "LABEL" )
      lFilled   := HGetValue( hElement, "FILLED" )
      nExtrude  := HGetValue( hElement, "EXTRUDE" )
      pTile     := HGetValue( hElement, "TILE" )
      IF nExtrude <> NIL
         lExtruded := TRUE
      ELSE
         lExtruded := FALSE
      ENDIF
      colorp    := HGetValue( hElement, "COLOR" )
      nVal      := hElement["VALUE"]
      nDim      := ( nVal / nMaxValue ) * nWidth
      //__OutDebug( "nDim", nDim )
      DEFAULT lFilled  TO FALSE
      DEFAULT nExtrude TO 0
      DEFAULT colorp   TO ::SetColor( 0, 0, 0 )

      nPosX   := x
      #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
      nPosY   := y + ( nSize * ( HB_EnumIndex() - 1 ) )
      #else
      nPosY   := y + ( nSize * ( hElement:__enumIndex() - 1 ) )
      #endif

      IF pTile <> NIL
         ::SetTile( pTile )
         colorp := gdTiled
      ELSE
         if ISARRAY( colorp )
            colorp := ::SetColor( colorp[1], colorp[2], colorp[3] )
         endif
      ENDIF
      ::Rectangle( nPosX, nPosY + nBorder, nPosX + nDim,  nPosY + nSize - nBorder, lFilled, colorp )

      IF lShowAxis
         // Y Axis
         IF lShowLabelBottom
            ::Say( nBorder, nPosY + nSize / 2 - ::GetFontHeight() / 2, PadL( cLabel, nMaxLabel ), color )
         ENDIF
      ENDIF

  NEXT
RETURN Self

METHOD LineChart() CLASS GDChart
  LOCAL hElement
  LOCAL nDegree := 0
  LOCAL lFilled, lExtruded, nExtrude, pTile
  LOCAL colorp
  LOCAL nVal, nDim
  LOCAL nPosX, nPosY
  LOCAL cLabel
  LOCAL nSize, nMax, nMin, nTotRange, nCeiling
  LOCAL nBorder, nThick, n
  LOCAL x, y, nWidth, nHeight, nMaxValue, nMinValue, color, nMaxLabel, nMinLabel
  LOCAL lShowAxis, lShowGrid

  LOCAL nLeftLabelSpace   //:= 40
  LOCAL nRightLabelSpace  //:= 40
  LOCAL nBottomLabelSpace //:= 40
  LOCAL nTopLabelSpace    := 40
  LOCAL lShowLabelLeft    := TRUE
  LOCAL lShowLabelRight   := TRUE //FALSE
  LOCAL lShowLabelBottom  := TRUE
  LOCAL lShowLabelTop     := FALSE
  LOCAL cAxisPict
  LOCAL cFontPitch

  LOCAL aDataOfHash, hDefs, aPoints

  aDataOfHash := ::aDataOfHashes
  hDefs       := ::hDefs

  x          := HGetValue( hDefs, "POSX" )
  y          := HGetValue( hDefs, "POSY" )
  nWidth     := HGetValue( hDefs, "WIDTH" )
  nHeight    := HGetValue( hDefs, "HEIGHT" )
  nMaxValue  := HGetValue( hDefs, "MAXVALUE" )
  nMinValue  := HGetValue( hDefs, "MINVALUE" )
  color      := HGetValue( hDefs, "COLOR" )
  lShowAxis  := HGetValue( hDefs, "SHOWAXIS" )
  lShowGrid  := HGetValue( hDefs, "SHOWGRID" )
  cAxisPict  := HGetValue( hDefs, "AXISPICT" )
  cFontPitch := HGetValue( hDefs, "FONTPITCH" )

  DEFAULT x          TO 0
  DEFAULT y          TO 0
  DEFAULT nWidth     TO ::Width()
  DEFAULT nHeight    TO ::Height()
  DEFAULT color      TO ::GetColor()
  DEFAULT lShowAxis  TO TRUE
  DEFAULT lShowGrid  TO TRUE
  DEFAULT cAxisPict  TO "@E 9,999.99"
  DEFAULT cFontPitch TO "TINY"

  DEFAULT nBorder TO 4

  /*
    hData := ["TITLE"], ["VALUE"], ["FILLED"], ["COLOR"], ["TILE"], ["EXTRUDE"]
  */

  DO CASE
     CASE cFontPitch == "TINY"
          ::SetFontTiny()
     CASE cFontPitch == "SMALL"
          ::SetFontSmall()
     CASE cFontPitch == "MEDIUM"
          ::SetFontMediumBold()
     CASE cFontPitch == "LARGE"
          ::SetFontLarge()
     CASE cFontPitch == "GIANT"
          ::SetFontGiant()
  ENDCASE

  // Before sum of values to determine percentual
  nMaxLabel := 0
  nMax      := 0
  FOR EACH hElement IN aDataOfHash
      #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
      IF HB_EnumIndex() == 1
      #else
      IF hElement:__enumIndex() == 1
      #endif
         nMax := hElement["VALUE"]
      ELSE
         nMax := Max( nMax, hElement["VALUE"] )
      ENDIF
      cLabel    := HGetValue( hElement, "LABEL" )
      nMaxLabel := Max( nMaxLabel, Len( IIF( cLabel <> NIL, cLabel, "" ) ) )
  NEXT

  // Before sum of values to determine percentual
  nMinLabel := 0
  nMin      := 0
  FOR EACH hElement IN aDataOfHash
      #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
      IF HB_EnumIndex() == 1
      #else
      IF hElement:__enumIndex() == 1
      #endif
         nMin := hElement["VALUE"]
      ELSE
         nMin := Min( nMin, hElement["VALUE"] )
      ENDIF
      cLabel    := HGetValue( hElement, "LABEL" )
      nMinLabel := Max( nMinLabel, Len( IIF( cLabel <> NIL, cLabel, "" ) ) )
  NEXT

  DEFAULT nLeftLabelSpace    TO nBorder + Max( Len( LTrim( Transform( nMax, cAxisPict ) ) ), Len( LTrim( Transform( nMin, cAxisPict ) ) ) ) * ::GetFontWidth() + nBorder
  DEFAULT nRightLabelSpace   TO nLeftLabelSpace
  DEFAULT nBottomLabelSpace  TO nBorder + nMaxLabel * ::GetFontWidth() + nBorder
  DEFAULT nMaxValue          TO nMax
  DEFAULT nMinValue          TO nMin

  IF lShowAxis
     IF lShowLabelLeft
        x       += nLeftLabelSpace
        nWidth  -= nLeftLabelSpace
     ENDIF
     IF lShowLabelRight
        nWidth  -= nRightLabelSpace
     ENDIF
     IF lShowLabelBottom
        y       += nBottomLabelSpace
        nHeight -= nBottomLabelSpace
     ENDIF
     IF lShowLabelTop
        nHeight -= nTopLabelSpace
     ENDIF
  ENDIF

  nSize := Len( aDataOfHash ) - 1

  if nSize > 1
     nSize := nWidth / nSize
  else
     nSize := nWidth
  endif

  nTotRange := nMaxValue + iif( nMinValue < 0, abs( nMinValue ), 0 )

  nCeiling := 0

  do while ( nTotRange / ( 10 ^ nCeiling ) ) > 100
     nCeiling++
  enddo

  nCeiling := 10 ^ nCeiling

  nMaxValue := ceiling( nMaxValue / nCeiling ) * nCeiling
  nMinValue := iif( nMinValue < 0, -ceiling( abs( nMinValue ) / nCeiling ) * nCeiling, ceiling( nMinValue / nCeiling ) * nCeiling )

  nTotRange := nMaxValue + iif( nMinValue < 0, abs( nMinValue ), 0 )

  IF lShowGrid
     ::Rectangle( x, ::Height() - ( y + nHeight ), x + nWidth, ::Height() - y, FALSE, color )

     nThick := ::SetThickness( 1 )

     ::ResetStyles()
     ::AddStyle( color )
     ::AddStyle( color )
     ::AddStyle( color )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::AddStyle( gdTransparent )
     ::SetStyle()
     FOR n := 10 TO 100 STEP 10
         nDim  := ( ( nTotRange / 100 ) * n )
         nPosY := ( nDim / nTotRange ) * nHeight
         //__OutDebug( "nDim", nDim )
         ::Line( x, ::Height() - ( y + nPosY), x + nWidth, ::Height() - ( y + nPosY ), gdStyled )
     NEXT
     FOR EACH hElement IN aDataOfHash
         #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
         nPosX   := x + ( nSize * ( HB_EnumIndex() - 1 ) )
         #else
         nPosX   := x + ( nSize * ( hElement:__enumIndex() - 1 ) )
         #endif
         ::Line( nPosX, ::Height() - y, nPosX, ::Height() - ( y + nHeight ), gdStyled )
     NEXT
      ::SetThickness( nThick )
  ENDIF

  IF lShowAxis
     // Y Axis
     FOR n := 0 TO 100 STEP 10
         nDim  := ( ( nTotRange / 100 ) * n )
         cLabel := LTrim( Transform( nMinValue + ( nTotRange / 10 ) * ( n / 10 ), cAxisPict ) )
         nPosY := ( nDim / nTotRange ) * nHeight
         IF lShowLabelLeft
            ::Say( x - nLeftLabelSpace + nBorder, ::Height() - ( y + nPosY ), cLabel, color )
         ENDIF
         IF lShowLabelRight
            ::Say( x + nWidth + nBorder, ::Height() - ( y + nPosY ), cLabel, color )
         ENDIF
     NEXT
  ENDIF

  // Second,
  aPoints := {}
  FOR EACH hElement IN aDataOfHash
      cLabel    := HGetValue( hElement, "LABEL" )
      lFilled   := HGetValue( hElement, "FILLED" )
      nExtrude  := HGetValue( hElement, "EXTRUDE" )
      pTile     := HGetValue( hElement, "TILE" )
      IF nExtrude <> NIL
         lExtruded := TRUE
      ELSE
         lExtruded := FALSE
      ENDIF
      colorp    := HGetValue( hElement, "COLOR" )
      nVal      := hElement["VALUE"]
      nDim      := ( ( nVal + abs( nMinValue ) ) / nTotRange ) * nHeight

      DEFAULT lFilled  TO FALSE
      DEFAULT nExtrude TO 0
      DEFAULT colorp   TO ::SetColor( 0, 0, 0 )

      #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
      nPosX   := x + ( nSize * ( HB_EnumIndex() - 1 ) )
      #else
      nPosX   := x + ( nSize * ( hElement:__enumIndex() - 1 ) )
      #endif
      nPosY   := y
      IF pTile <> NIL
         ::SetTile( pTile )
         colorp := gdTiled
      ELSE
         if ISARRAY( colorp )
            colorp := ::SetColor( colorp[1], colorp[2], colorp[3] )
         endif
      ENDIF
      //::Rectangle( nPosX + nBorder, ::Height() - ( nPosY + nDim ), nPosX + nSize - nBorder, ::Height() - nPosY, lFilled, colorp )
      aAdd( aPoints, { nPosX, ::Height() - ( nPosY + nDim ) } )

      IF lShowAxis
         // Y Axis
         IF lShowLabelBottom
            ::SayVertical( nPosX - ::GetFontHeight() / 2, ::Height() - nBorder, PadL( cLabel, nMaxLabel ), color )
         ENDIF
      ENDIF

  NEXT

  // Draw lines
  nThick := ::SetThickness( 3 )

  //::ResetStyles()
  //::AddStyle( color )
  //::AddStyle( color )
  //::AddStyle( color )
  //::AddStyle( gdTransparent )
  //::AddStyle( gdTransparent )
  //::AddStyle( gdTransparent )
  //::AddStyle( gdTransparent )
  //::AddStyle( gdTransparent )
  //::SetStyle()
  FOR n := 1 TO Len( aPoints ) - 1
      ::Line( aPoints[ n ][ 1 ], aPoints[ n ][ 2 ], aPoints[ n + 1 ][ 1 ], aPoints[ n + 1 ][ 2 ], color )
  NEXT
  ::SetThickness( nThick )

RETURN Self

METHOD Clone() CLASS GDChart
  LOCAL oDestImage
  LOCAL pImage

  IF ::IsTrueColor()
     oDestImage := GDChart():CreateTrueColor( ::Width, ::Height )
  ELSE
     oDestImage := GDChart():Create( ::Width, ::Height )
  ENDIF

  pImage := oDestImage:pImage
  oDestImage := oDestImage:CloneDataFrom( Self )
  //oDestImage := __objClone( Self )
  oDestImage:pImage := pImage
  ::Copy( 0, 0, ::Width, ::Height, 0, 0, oDestImage )


  //pImage := oDestImage:pImage
  //// Signal that this image must not be destroyed
  //oDestImage:lDestroy := FALSE
  //oDestImage := NIL
  //oDestImage:pImage := pImage

RETURN oDestImage


METHOD CloneDataFrom( oSrc )
   // copy values from Source to Dest
   // please update in case of new datas

   ::Super:CloneDataFrom( oSrc )

   ::cTitle        := oSrc:cTitle
   ::cAxisX        := oSrc:cAxisX
   ::cAxisY        := oSrc:cAxisY
   ::nWidth        := oSrc:nWidth
   ::nHeight       := oSrc:nHeight
   ::nScaleX       := oSrc:nScaleX
   ::nScaleY       := oSrc:nScaleY

   ::aSeries       := AClone( oSrc:aSeries )
   ::aDataOfHashes := AClone( oSrc:aDataOfHashes )
   #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
   ::hDefs         := HClone( oSrc:hDefs )
   #else
   ::hDefs         := HB_HClone( oSrc:hDefs )
   #endif

RETURN Self


STATIC FUNCTION HGetValue( hHash, cKey )
  LOCAL nPos
  LOCAL xVal
  IF hHash <> NIL
     #if defined( __XHARBOUR ) .or. ( defined( __HARBOUR__ ) .and. defined( HB_COMPAT_XHB ) )
     xVal := IIF( ( nPos := HGetPos( hHash, cKey )) == 0, NIL, HGetValueAt( hHash, nPos) )
     #else
     xVal := IIF( ( nPos := HB_HPos( hHash, cKey )) == 0, NIL, HB_HValueAt( hHash, nPos) )
     #endif
  ENDIF
 //RETURN IIF( cKey IN hHash:Keys, hHash[ cKey ], NIL )
RETURN xVal

