/*
 * $Id$
 */

/*
 *   ScrollBar class test
 *
 * Harbour Project source code
 * http://www.harbour-project.org/
 *
 *  Example donated to Harbour Project by Diego Pego, 
 *  modified by Alejandro de Garate
 */

 #include "directry.ch"
 #include "achoice.ch"
 #include "inkey.ch"

#define B_THIN  ( CHR( 219 ) + CHR( 223 ) + CHR( 219 ) + CHR( 219 ) + ;
                  CHR( 219 ) + CHR( 220 ) + CHR( 219 ) + CHR( 219 ) )

 procedure main()

     InitScrlBar()

 RETURN


 FUNCTION InitScrlBar()

    LOCAL tmpFileList := {}, i
    MEMVAR aFileList, filesScroll
    PRIVATE aFileList := {}, filesScroll

    CLS
    SETBLINK(.F.)
    @ 00,00,24,79 BOX REPLIC( CHR(178), 9) COLOR "GR+/W*" 
    @ 04,28 SAY "            Directory            " COLOR "W+/B"
    @ 05,28,15,60 BOX B_THIN + " " COLOR "W/W*" 

    // get the current folder files to display on the aChoice menu
    tmpFileList := directory()

    FOR i := 1 TO LEN( tmpFileList )
        AADD( aFileList, tmpFileList[ i ][ F_NAME ])
    NEXT

    filesScroll := ScrollBar( 06, 14, 60, NIL, 1 ) 

    filesScroll:total := LEN( aFileList )

    filesScroll:SetColor("W+/W, W+/W")   // New method!
    SET COLOR TO "N/W*, W+/B,,,W/N"

    filesScroll:display()
 
    i := ACHOICE( 06, 29, 14, 59, aFileList, , "updateFilesScroll")

    @ 23,0 SAY IIF( i < 1,"", aFileList[ i ]) COLOR "N/W*"
    SET COLOR TO 
    @ 24,0 
 RETURN 0


 // function used to update scrollbar
 FUNCTION updateFilesScroll( modo )

    LOCAL newPos, valRet := AC_CONT, ;     // Default to continue
          ultTecla := LASTKEY()
    MEMVAR filesScroll
    newPos := filesScroll:current

    DO CASE
       CASE ultTecla == K_CTRL_PGUP
            newPos := 1
       CASE ultTecla == K_CTRL_PGDN
            newPos := filesScroll:total
       CASE ultTecla == K_CTRL_HOME
            newPos := newPos - (filesScroll:barLength + 1)
       CASE ultTecla == K_CTRL_END
            newPos := newPos + (filesScroll:barLength + 1)
       CASE ultTecla == K_PGUP
            newPos := newPos - (filesScroll:barLength + 1)
       CASE ultTecla == K_PGDN
            newPos := newPos + (filesScroll:barLength + 1)
       CASE ultTecla == K_UP
            newPos--
       CASE ultTecla == K_DOWN
            newPos++
       CASE modo == AC_EXCEPT
           DO CASE
              case ultTecla == K_RETURN
                   valRet := AC_SELECT
              CASE ultTecla == K_ESC
                   valRet := AC_ABORT
              OTHERWISE
                   valRet := AC_GOTO
           ENDCASE
    ENDCASE

    IF (newPos < 1)
        newPos := 1
    ELSEIF (newPos >= filesScroll:total)
        newPos := filesScroll:total
    ENDIF

    filesScroll:current := newPos
    filesScroll:update()

 RETURN valRet


