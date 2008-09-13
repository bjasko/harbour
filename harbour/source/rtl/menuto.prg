/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * PROMPT/MENU TO commands
 *
 * Released to Public Domain by Phil Barnett <philb@iag.net>
 * www - http://www.harbour-project.org
 *
 */

/* NOTE: Recursive use is supported. */

#include "color.ch"
#include "common.ch"
#include "inkey.ch"
#include "hbmemvar.ch"
#include "setcurs.ch"

#xtranslate COLORARRAY( <x> ) => &( '{"' + StrTran( <x>, ',', '","' ) + '"}' )

THREAD STATIC s_aLevel   := {}
THREAD STATIC s_nPointer := 1

FUNCTION __AtPrompt( nRow, nCol, cPrompt, cMsg, cColor )

   IF s_nPointer < 1
      s_nPointer := 1
   ENDIF

   // add the current level empty array.
   DO WHILE Len( s_aLevel ) < s_nPointer
      AAdd( s_aLevel, {} )
   ENDDO

   // add to the static array
   AAdd( s_aLevel[ s_nPointer ], { nRow, nCol, cPrompt, cMsg, cColor } )

   // put this prompt on the screen right now
   DispOutAt( nRow, nCol, cPrompt, cColor )

   RETURN .F.

FUNCTION __MenuTo( bBlock, cVariable )

   LOCAL nKey
   LOCAL y
   LOCAL q
   LOCAL n
   LOCAL lExit
   LOCAL nArrLen
   LOCAL xMsg
   LOCAL nMsgCol
   LOCAL nMsgRow
   LOCAL lMsgCenter
   LOCAL nSaveCursor
   LOCAL cSaveReadVar

   LOCAL lDeclared
   LOCAL bAction
#ifdef HB_COMPAT_C53
   LOCAL nMouseClik
#endif

   LOCAL nPointer
   LOCAL aColor
   LOCAL cBackColor
   LOCAL cFrontColor

   // Detect if a memvar was passed
   lDeclared := !__mvEXIST( cVariable )
   IF lDeclared
      __mvPUBLIC( cVariable )
   ENDIF

   n := Eval( bBlock )

   // if no prompts were defined, exit with 0

   IF s_nPointer < 1 .OR. s_nPointer > Len( s_aLevel )

      n := 0

   ELSE

      s_nPointer ++
      nPointer := s_nPointer

      nArrLen := Len( s_aLevel[ nPointer - 1 ] )

      // put choice in a valid range

      IF !ISNUMBER( n ) .OR. n < 1
         n := 1
      ENDIF

      IF n > nArrLen
         n := nArrLen
      ENDIF

      //

      nSaveCursor := SetCursor( iif( Set( _SET_INTENSITY ), SC_NONE, NIL ) )
      cSaveReadVar := ReadVar( Upper( cVariable ) )
      xMsg := ""
      nMsgCol := 0
      nMsgRow := Set( _SET_MESSAGE )
      lMsgCenter := Set( _SET_MCENTER )
      lExit := .F.


      DO WHILE n != 0

         // should we display messages?
         IF nMsgRow > 0

            IF ! Empty( xMsg )
               DispOutAt( nMsgRow, nMsgCol, Space( Len( xMsg ) ) )
            ENDIF

            xMsg := s_aLevel[ nPointer - 1, n, 4 ]

            // Code Block messages ( yes, they are documented! )
            IF ISBLOCK( xMsg )
               xMsg := Eval( xMsg )
            ENDIF

            IF !ISCHARACTER( xMsg )
               xMsg := ""
            ENDIF

            IF lMsgCenter
               nMsgCol := Int( ( MaxCol() - Len( xMsg ) ) / 2 )
            ENDIF

            DispOutAt( nMsgRow, nMsgCol, xMsg )

         ENDIF

         // save the current row
         q := n

         IF s_aLevel[ s_nPointer - 1, n, 5 ] != NIL
             aColor := COLORARRAY( s_aLevel[ s_nPointer - 1, n, 5 ] )
             cFrontColor := iif( Empty( aColor[ 1 ] ), NIL, aColor[ 1 ] )
             cBackColor  := iif( Len( aColor ) > 1, aColor[ 2 ], NIL )
         ENDIF

         IF Set( _SET_INTENSITY )
            IF cBackColor == NIL    // Only select Color Enhace if no color was passed
               ColorSelect( CLR_ENHANCED )
            ENDIF
         ENDIF

         // highlight the prompt
         DispOutAt( s_aLevel[ nPointer - 1, n, 1 ],;
                    s_aLevel[ nPointer - 1, n, 2 ],;
                    s_aLevel[ nPointer - 1, n, 3 ],;
                    cBackColor )

         IF Set( _SET_INTENSITY )
            IF cFrontColor == NIL    // Only select Color Enhace if no color was passed
              ColorSelect( CLR_STANDARD )
            ENDIF
         ENDIF

         IF lExit
            EXIT
         ENDIF

         nKey := 0
         DO WHILE nKey == 0

            // wait for a keystroke
            nKey := Inkey( 0 )

            IF ( bAction := SetKey( nKey ) ) != NIL

               Eval( bBlock, n )
               Eval( bAction, ProcName( 1 ), ProcLine( 1 ), Upper( cVariable ) )
               n := Eval( bBlock )

               IF n < 1
                  n := 1
               ELSEIF n > nArrLen
                  n := nArrLen
               ENDIF

               nKey := 0

            ENDIF
         ENDDO

         // check for keystrokes
         SWITCH nKey
#ifdef HB_COMPAT_C53
            CASE K_MOUSEMOVE
               EXIT
            CASE K_LBUTTONDOWN
            CASE K_LDBLCLK
               IF ( nMouseClik := HitTest( s_aLevel[ nPointer - 1 ], ;
                                           MRow(), MCol() ) ) > 0
                  n := nMouseClik
               ENDIF
               IF nKey == K_LDBLCLK
                   lExit := .T.
               ENDIF
               EXIT
#endif
            CASE K_DOWN
            CASE K_RIGHT
               IF ++n > nArrLen
                  n := iif( Set( _SET_WRAP ), 1, nArrLen )
               ENDIF
               EXIT
            CASE K_UP
            CASE K_LEFT
               IF --n < 1
                  n := iif( Set( _SET_WRAP ), nArrLen, 1 )
               ENDIF
               EXIT
            CASE K_HOME
               n := 1
               EXIT
            CASE K_END
               n := nArrLen
               EXIT
            CASE K_ENTER
            CASE K_PGUP
            CASE K_PGDN
               lExit := .T.
               EXIT
            CASE K_ESC
               n := 0
               EXIT
            OTHERWISE
               // did user hit a hot key?
               FOR y := 1 TO nArrLen
                  IF Upper( Left( LTrim( s_aLevel[ nPointer - 1, y, 3 ] ), 1 ) ) == Upper( Chr( nKey ) )
                     n := y
                     lExit := .T.
                     EXIT
                  ENDIF
               NEXT
         ENDSWITCH

         IF n != 0
            DispOutAt( s_aLevel[ nPointer - 1, q, 1 ],;
                       s_aLevel[ nPointer - 1, q, 2 ],;
                       s_aLevel[ nPointer - 1, q, 3 ],;
                       cFrontColor )
         ENDIF

      ENDDO

      ReadVar( cSaveReadVar )
      SetCursor( nSaveCursor )

      s_nPointer := nPointer
      s_nPointer --
      ASize( s_aLevel, s_nPointer - 1 )

   ENDIF

   Eval( bBlock, n )

   IF lDeclared
      __mvXRELEASE( cVariable )
   ENDIF

   SetPos( MaxRow() - 1, 0 )

   RETURN n

#ifdef HB_COMPAT_C53

STATIC FUNCTION HitTest( aMenu, nMRow, nMCol )

   LOCAL aMenuItem

   FOR EACH aMenuItem IN aMenu
      IF nMRow == aMenuItem[ 1 ] .AND. ;
         nMCol >= aMenuItem[ 2 ] .AND. ;
         nMCol < aMenuItem[ 2 ] + LEN( aMenuItem[ 3 ] )

         RETURN aMenuItem:__enumIndex()
      ENDIF
   NEXT

   RETURN 0

#endif
