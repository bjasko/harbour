/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * GENOS2 support module for hbdoc document Extractor 
 *
 * Copyright 2000 Luiz Rafael Culik <culik@sl.conex.net>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version, with one exception:
 *
 * The exception is that if you link the Harbour Runtime Library (HRL)
 * and/or the Harbour Virtual Machine (HVM) with other files to produce
 * an executable, this does not by itself cause the resulting executable
 * to be covered by the GNU General Public License. Your use of that
 * executable is in no way restricted on account of linking the HRL
 * and/or HVM code into it.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA (or visit
 * their web site at http://www.gnu.org/).
 *
 */

#ifdef __HARBOUR__
   #define NANFOR
#endif

#include "directry.ch"
#include "fileio.ch"
#include "inkey.ch"
#include "hbdocdef.ch"
#include 'common.ch'
//  output lines on the screen

#define INFILELINE   10
#define MODULELINE   12
#define LINELINE     14
#define ERRORLINE    20
#define LONGLINE     600
#define LONGONELINE  86
MEMVAR aDirlist,aDocInfo
STATIC aAlso
STATIC nCommentLen
STATIC lEof
STATIC aFiTable := {}
STATIC lIsTable :=.F.
STATIC aColorTable:={{'aqua',''},{'black',''},{'fuchia',''},{'grey',''},{'green',''},{'lime',''},{'maroon',''},{'navy',''},{'olive',''},{'purple',''},{'red',''},{'silver',''},{'teal',''},{'white',''},{'yellow',''}}
FUNCTION ProcessOs2()

   //

   //  Copyright (C) 2000 Luiz Rafael Culik
   //
   //  Purpose: Process each of the files in the directory
   //  and generate .tr file format output
   //  Modification History:
   //         Version    Date        Who       Notes
   //          V1.00     1/20/2000   LRC       Initial Version
   //
   //  Calling parameters: None
   //
   //  Notes: None
   // -
   //  LOCAL variables:

   LOCAL i
   LOCAL j
   LOCAL nFiles
//   LOCAL nCommentLen
//   LOCAL lEof
   LOCAL lDoc
   LOCAL cBuffer
   LOCAL nEnd
   LOCAL nCount,nAlso

   LOCAL cBar       := "���������������������������������������������������������������������������������"
   LOCAL nMode
   local cname
   LOCAL cFuncName
   LOCAL cOneLine
   LOCAL cCategory
   LOCAL cFileName
   LOCAL nLineCnt
   LOCAL cSeeAlso
   LOCAL cTemp
   LOCAL cChar
   LOCAL oOs2
   LOCAL lData := .F.
   LOCAL lMethod := .F.
   LOCAL nPos,nEpos,nPosend,cBuffEnd
   LOCAL lIsDataLink := .F. 
   LOCAL lIsMethodLink := .F.

   LOCAL lBlankLine := .F.                 // Blank line encountered and sent out
   LOCAL lAddBlank  := .F.                 // Need to add a blank line if next line is not blank
   LOCAL nReadHandle
   LOCAL cDoc      := DELIM + "DOC" + DELIM                   // DOC keyword
   LOCAL cEnd      := DELIM + "END" + DELIM                   // END keyword
   LOCAL cFunc     := DELIM + "FUNCNAME" + DELIM              // FUNCNAME keyword
   LOCAL cCat      := DELIM + "CATEGORY" + DELIM              // CATEGORY keyword
   LOCAL cOne      := DELIM + "ONELINER" + DELIM              // ONELINER keyword
   LOCAL cSyn      := DELIM + "SYNTAX" + DELIM                // SYNTAX keyword
   LOCAL cArg      := DELIM + "ARGUMENTS" + DELIM             // ARGUMENTS keyword
   LOCAL cRet      := DELIM + "RETURNS" + DELIM               // RETURNS keyword
   LOCAL cDesc     := DELIM + "DESCRIPTION" + DELIM           // DESCRIPTION keyword
   LOCAL cExam     := DELIM + "EXAMPLES" + DELIM              // EXAMPLES keyword
   LOCAL cSee      := DELIM + "SEEALSO" + DELIM               // SEEALSO keyword
   LOCAL cInc      := DELIM + "INCLUDE" + DELIM               // INCLUDE keyword
   LOCAL cComm     := DELIM + "COMMANDNAME" + DELIM           // COMMAND keyword
   LOCAL cCompl    := DELIM + "COMPLIANCE" + DELIM
   LOCAL cTest     := DELIM + 'TESTS' + DELIM
   LOCAL cStatus   := DELIM + 'STATUS' + DELIM
   LOCAL cPlat     := DELIM + 'PLATFORMS' + DELIM
   LOCAL cFiles    := DELIM + 'FILES' + DELIM
   LOCAL cSubCode  := DELIM + 'SUBCODE' + DELIM
   LOCAL cFunction := DELIM + 'FUNCTION' +DELIM
   LOCAL cConstruct := DELIM + 'CONSTRUCTOR' + DELIM
   LOCAL cDatalink  := DELIM + 'DATALINK' + DELIM
   LOCAL cDatanolink  := DELIM + 'DATANOLINK' + DELIM
   LOCAL cMethodslink := DELIM + 'METHODSLINK' + DELIM
   LOCAL cMethodsNolink := DELIM + 'METHODSNOLINK' + DELIM
   LOCAL cData      := DELIM +"DATA"+ DELIM
   LOCAL cMethod    := DELIM +'METHOD' +DELIM
   LOCAL cClassDoc  := DELIM+ "CLASSDOC" + DELIM
   LOCAL cTable     := DELIM +"TABLE" + DELIM

   lData := .F.
   lMethod := .F.
   lIsDataLink := .F.
   lIsMethodLink := .F.
   

   nFiles   := LEN( aDirList )
   //
   //  Entry Point
   //
   //  Put up information labels
   @ INFILELINE, 20 SAY "Extracting: "
   @ MODULELINE, 20 SAY "Documenting: "
   //  loop through all of the files
   oOs2 := tOS2():new( "ipf\Harbour.ipf" )
   FOR i := 1 TO nFiles

      //  Open file for input

      nCommentLen := IIF( AT( ".ASM", UPPER( aDirList[ i, F_NAME ] ) ) > 0, 2, 4 )
      nReadHandle :=      FT_FUSE( aDirList[ i, F_NAME ] )
      @ INFILELINE, 33 CLEAR TO INFILELINE, MAXCOL()
      @ INFILELINE, 33 SAY PAD( aDirList[ i, F_NAME ], 47 )
      @ MODULELINE, 33 CLEAR TO LINELINE, MAXCOL()
      @ LINELINE, 27   SAY "Line:"

      nLineCnt := 0

      IF nReadHandle < 0
         write_error( "Can't open file: (Dos Error " + STR( FERROR() ) + ")",,,, aDirList[ i, F_NAME ] )
         @ ERRORLINE,  0 CLEAR TO ERRORLINE, MAXCOL()
         @ ERRORLINE, 20 SAY "Can't open file: (Dos Error " + STR( FERROR() ) + ") File=" + aDirList[ i, F_NAME ]
         LOOP
      ENDIF
      lEof := .F.
      lDoc := .F.
      //  First find the author

      DO WHILE .NOT. lEof

         //  Read a line

         cBuffer := TRIM( SUBSTR( ReadLN( @lEof ), nCommentLen ) )
         nLineCnt ++
         IF nLineCnt % 10 = 0
            @ LINELINE, 33 SAY STR( nLineCnt, 5, 0 )
         ENDIF
         //  check to see if we are in doc mode or getting out of doc mode

         IF AT( cDoc, cBuffer ) > 0 .or.  AT( cClassDoc, cBuffer ) > 0
            IF lDoc
               write_error( cDoc + " encountered during extraction of Doc" ;
                            + " at line" + STR( nLinecnt, 5, 0 ),,,, aDirList[ i, F_NAME ] )
            ENDIF
            lDoc    := .T.
            cBuffer := TRIM( SUBSTR( ReadLN( @lEof ), ;
                             nCommentLen ) )
            nLineCnt ++
            cCategory := cFuncName := cSeeAlso := ""
            nMode     := D_IGNORE
         ELSEIF AT( cEnd, cBuffer ) > 0
            IF .NOT. lDoc
               write_error( cEnd + " encountered outside of Doc area at line" ;
                            + STR( nLinecnt, 5, 0 ),,,, aDirList[ i, F_NAME ] )
            ELSE
               //  Add a new entry to our list of files

               IF EMPTY( cCategory )
                  write_error( "Blank Category",,,, aDirList[ i, F_NAME ] )
                  cCategory := "Unknown"
               ENDIF
               IF EMPTY( cFuncName )
                  write_error( "Blank Function Name",,,, aDirList[ i, F_NAME ] )
                  cFuncName := "Unknown"
               ENDIF
               AADD( aDocInfo, { cCategory, cFuncName, cOneLine, cFileName } )
               //  Now close down this little piece
               lDoc := .F.
               if lData .or. lmethod
                    oos2:writeText(':efn.')
                endif
               IF .NOT. EMPTY( cSeeAlso )
                  oOs2:WritePar( ".br" + CRLF + "See Also:" )
                  FOR nAlso := 1 TO LEN( aAlso )

                     IF nAlso == 1
                        oOs2:WriteLink( aAlso[ nAlso ] )
                     ELSE
                        oOs2:WriteLink( aAlso[ nAlso ] )
                     ENDIF
                  NEXT

               ENDIF

               nMode := D_IGNORE
            ENDIF

            @ MODULELINE, 33 CLEAR TO MODULELINE, MAXCOL()
         ENDIF

         //  Act on the input
         IF lDoc
            //  1) function name

            IF AT( cFunc, cBuffer ) > 0 .OR. AT( cComm, cBuffer ) > 0 .OR. AT( cSubCode, cBuffer ) >0
               cBuffer := ReadLN( @lEof )
               nLineCnt ++
               //  Save the function name
               cFuncName := UPPER( ALLTRIM( SUBSTR( cBuffer, nCommentLen ) ) )
               @ MODULELINE, 33 CLEAR TO MODULELINE, MAXCOL()
               @ MODULELINE, 33 SAY cFuncName

               nMode := D_NORMAL

               //  Open a new file
               IF AT( "FT_", cFuncName ) > 0
                  cTemp := SUBSTR( cFuncName, 4 )
               ELSE
                  cTemp := cFuncName
               ENDIF

               IF ( nEnd := AT( "(", cTemp ) ) > 0
                  cTemp := LEFT( cTemp, nEnd - 1 )
               ENDIF
               cFileName := ""

               //  Strip off any other non-alphabetic/numeric characters
               FOR j := 1 TO LEN( cTemp )
                  cChar := SUBSTR( cTemp, j, 1 )
                  IF ( cChar >= "0" .AND. cChar <= "9" ) .OR. ;
                       ( cChar >= "A" .AND. cChar <= "Z" ) .OR. cChar = "_"
                     cFileName += cChar
                  ENDIF
               NEXT

               //  See if file name is present already. If so then modify

               cFileName := LEFT( cFileName, 21 )
               nEnd      := 1
               nCount    := 0
               DO WHILE nEnd > 0
                  nEnd := ASCAN( aDocInfo, { | a | a[ 4 ] == cFileName + ".ipf" } )
                  IF nEnd > 0

                     //  This will break if there are more than 10 files with the same first
                     //  seven characters. We take our chances.

                     IF LEN( cFileName ) = 21
                        cFileName := STUFF( cFileName, 21, 1, STR( nCount, 1, 0 ) )
                     ELSE
                        cFileName += STR( nCount, 1, 0 )
                     ENDIF
                     nCount ++
                  ENDIF
               ENDDO
               //  Add on the extension

               IF oOs2:nHandle < 1
                  ? "Error creating", cFileName, ".ipf"
                  write_error( "Error creating",,,, cFileName + ".ipf" )
               ENDIF
            ELSEIF AT( cdata, cBuffer ) > 0 .OR. AT( cmethod, cBuffer ) > 0
                   if AT( cdata, cBuffer ) > 0
                      lData := .T.
                      lMethod := .F.
                   ELSEIF AT( cmethod, cBuffer ) > 0
                      lMethod := .T.
                      lData:= .F.
                   ENDIF
               cBuffer := ReadLN( @lEof )
               nLineCnt ++
               //  Save the function name
               cFuncName := UPPER( ALLTRIM( SUBSTR( cBuffer, nCommentLen ) ) )
               @ MODULELINE, 33 CLEAR TO MODULELINE, MAXCOL()
               @ MODULELINE, 33 SAY cFuncName

               nMode := D_NORMAL

               //  2) Category

            ELSEIF AT( cCat, cBuffer ) > 0
               cBuffer := ReadLN( @lEof )
               nLineCnt ++
               //  get the category
               cCategory := UPPER( ALLTRIM( SUBSTR( cBuffer, nCommentLen ) ) )

               //  3) One line description

            ELSEIF AT( cOne, cBuffer ) > 0
               cBuffer := ReadLN( @lEof )
               nLineCnt ++
               cOneLine := ALLTRIM( SUBSTR( cBuffer, nCommentLen ) )
               IF LEN( cOneLine ) > LONGONELINE
                  write_error( "OneLine", cOneLine, nLineCnt, LONGONELINE, ;
                               aDirList[ i, F_NAME ] )
               ENDIF

               nMode := D_ONELINE
               //  Now start writing out what we know
               if lData
                    oOs2:WriteJumpTitle( left(cFilename,At('.',cFilename)-1)+ cFuncName, "Data "+cFuncName )
               Elseif lMethod
                    oOs2:WriteJumpTitle( left(cFilename,At('.',cFilename)-1)+cFuncName, "Method " +cFuncName )
               Else

               oOs2:WriteTitle( PAD( cFuncName, 21 ), cFuncName )
               oOs2:WriteParBold( cOneLine )

               Endif
               //  4) all other stuff

            ELSE

               IF AT( cSyn, cBuffer ) > 0

                  oOs2:WriteParBold( " Syntax" )

                  nMode     := D_SYNTAX
                  lAddBlank := .T.

               ELSEIF AT( cConstruct, cBuffer ) > 0

                     oOs2:WriteParBold( " Constructor syntax" )

                  nMode     := D_SYNTAX
                  lAddBlank := .T.



               ELSEIF AT( cArg, cBuffer ) > 0

                  IF !lBlankLine

                     oOs2:WriteParBold( " Arguments" )

                  ENDIF

                  nMode     := D_ARG
                  lAddBlank := .T.

               ELSEIF AT( cRet, cBuffer ) > 0

                  IF !lBlankLine
                     oOs2:WritePar( "" )
                  ENDIF

                  oOs2:WriteParBold( " Returns" )

                  nMode     := D_RETURN
                  lAddBlank := .T.

               ELSEIF AT( cDesc, cBuffer ) > 0

                  IF !lBlankLine
                     oOs2:WritePar( "" )
                  ENDIF
                  oOs2:WriteParBold( " Description" )

                  nMode     := D_DESCRIPTION
                  lAddBlank := .T.
               ELSEIF AT( cdatalink, cBuffer ) > 0
                  IF !lBlankLine
                     oOs2:WritePar( "" ) //:endpar()
                  ENDIF
        
                  oOs2:WriteParBold( " Data" )
                  nMode     := D_DATALINK
                  lAddBlank := .T.

                  lIsDataLink := .T.

               ELSEIF AT( cDatanolink, cBuffer ) > 0
                  if !lIsDataLink
                    oOs2:WriteParBold( " Data" )
  
                  endif
                  nMode     := D_NORMAL
                  lAddBlank := .T.


               ELSEIF AT(  cMethodslink, cBuffer ) > 0

                  oOs2:WriteParBold( " Method" )
                  nMode     := D_METHODLINK
                  lAddBlank := .T.

                  lIsMethodLink := .T.
                  
               ELSEIF AT(  cMethodsnolink, cBuffer ) > 0
                  if !lIsMethodLink
                  oOs2:WriteParBold( " Methods" )
                  endif

                  nMode     := D_NORMAL
                  lAddBlank := .T.



               ELSEIF AT( cExam, cBuffer ) > 0

                  IF !lBlankLine
                     oOs2:WritePar( "" )
                  ENDIF
                  oOs2:WriteParBold( " Examples" )
                  nMode     := D_EXAMPLE
                  lAddBlank := .T.
               ELSEIF AT( cTest, cBuffer ) > 0

                  IF !lBlankLine
                     oOs2:WritePar( "" )
                  ENDIF

                  oOs2:WriteParBold( " Tests" )
                  nMode     := D_EXAMPLE
                  lAddBlank := .T.

               ELSEIF AT( cStatus, cBuffer ) > 0

                  nMode := D_STATUS

               ELSEIF AT( cCompl, cBuffer ) > 0

                  IF !lBlankLine
                     oOs2:WritePar( "" )
                  ENDIF
                  oOs2:WriteParBold( " Compliance" )
                  nMode     := D_COMPLIANCE
                  lAddBlank := .T.
               ELSEIF AT( cPlat, cBuffer ) > 0

                  IF !lBlankLine
                     oOs2:WritePar( "" )
                  ENDIF
                  oOs2:WriteParBold( " Platforms" )
                  nMode     := D_NORMAL
                  lAddBlank := .T.
               ELSEIF AT( cFiles, cBuffer ) > 0

                  IF !lBlankLine
                     oOs2:WritePar( "" )
                  ENDIF
                  oOs2:WriteParBold( " Files" )

                  nMode     := D_NORMAL
                  lAddBlank := .T.
               ELSEIF AT( cFunction, cBuffer ) > 0

                  IF !lBlankLine
                     oOs2:WritePar( "" )
                  ENDIF
                  oOs2:WriteParBold( " Function" )

                  nMode     := D_NORMAL
                  lAddBlank := .T.

               ELSEIF AT( cSee, cBuffer ) > 0
                  nMode := D_SEEALSO
               ELSEIF AT( cInc, cBuffer ) > 0
                  nMode := D_INCLUDE

                  //  All other input is trimmed of comments and sent out

               ELSE
                  //  translate any \$ into $
                  cBuffer := STRTRAN( cBuffer, "\" + DELIM, DELIM )
                  IF nMode = D_SYNTAX
                     IF LEN( cBuffer ) > LONGLINE
                        write_error( "Syntax", cBuffer, nLineCnt, ;
                                     LONGLINE, aDirList[ i, F_NAME ] )
                     ENDIF
                     lBlankLine := EMPTY( cBuffer )
                     if At("<par>",cBuffer)>0
                         strtran(cBuffer,"<par>",'')
                         strtran(cBuffer,"</par>",'')
                         cBuffer:=Alltrim(cBuffer)
                         cbuFfer:='<par>'+cBuffer+'</par>'
                     endif

                      procos2desc(cbuffer,oOs2,"Syntax")  

                  ELSEIF nMode = D_RETURN

                     IF LEN( cBuffer ) > LONGLINE
                        write_error( "Arguments", cBuffer, nLineCnt, ;
                                     LONGLINE, aDirList[ i, F_NAME ] )
                     ENDIF
                     lBlankLine := EMPTY( cBuffer )

                  procos2desc(cbuffer,oOs2,"Arguments")



                  ELSEIF nMode = D_ARG
                     IF LEN( cBuffer ) > LONGLINE
                        write_error( "Arguments", cBuffer, nLineCnt, ;
                                     LONGLINE, aDirList[ i, F_NAME ] )
                     ENDIF
                     lBlankLine := EMPTY( cBuffer )
                                       procos2desc(cbuffer,oOs2,"Arguments")
                  ELSEIF nMode = D_NORMAL
                     IF LEN( cBuffer ) > LONGLINE
                        write_error( "General", cBuffer, nLineCnt, ;
                                     LONGLINE, aDirList[ i, F_NAME ] )
                     ENDIF
                     lBlankLine := EMPTY( cBuffer )
                                                  procos2desc(cBuffer,oOs2)
                  ELSEIF nMode = D_DATALINK
                     IF LEN( cBuffer ) > LONGLINE
                        write_error( "General", cBuffer, nLineCnt, ;
                                     LONGLINE, aDirList[ i, F_NAME ] )
                     ENDIF
                     lBlankLine := EMPTY( cBuffer )
                     IF lAddBlank
                        lAddBlank := .F.
                     ENDIF
                     cTemp:=Substr(cBuffer,1,AT(":",cBuffer)-1)
                     cBuffer:=Substr(cBuffer,AT(":",cBuffer)+1)
                     oOs2:WriteJumpLink(Left(cfilename,At('.',cFilename)-1)+alltrim(cTemp),cBuffer)
                  ELSEIF nMode = D_METHODLINK
                     IF LEN( cBuffer ) > LONGLINE
                        write_error( "General", cBuffer, nLineCnt, ;
                                     LONGLINE, aDirList[ i, F_NAME ] )
                     ENDIF
                     lBlankLine := EMPTY( cBuffer )
                     IF lAddBlank
                        lAddBlank := .F.
                     ENDIF
                     cTemp:=Substr(cBuffer,1,AT("()",cBuffer)+1)
                     cName:=Substr(cBuffer,1,AT("()",cBuffer)-1)
                     cBuffer:=Substr(cBuffer,AT("()",cBuffer)+2)
                     oOs2:WriteJumpLink(Left(cfilename,At('.',cFilename)-1)+alltrim(cTemp) ,cTemp,cBuffer)

                  ELSEIF nMode = D_SEEALSO
                     IF .NOT. EMPTY( cBuffer )
                        cSeeAlso := ProcOs2Also( StripFiles( ALLTRIM( cBuffer ) ) )
                     ENDIF
                  ELSEIF nMode = D_INCLUDE
                     //  read next line
                     IF .NOT. EMPTY( cBuffer )
                        IF !lBlankLine
                           oOs2:WritePar( "" )
                        ENDIF
                     ENDIF
                  ELSEIF nMode = D_COMPLIANCE
                     IF LEN( cBuffer ) > LONGLINE
                        write_error( "General", cBuffer, nLineCnt, ;
                                     LONGLINE, aDirList[ i, F_NAME ] )
                     ENDIF
                     lBlankLine := EMPTY( cBuffer )
                     procos2desc(cBuffer,oOs2,"Compliance")


                  ELSEIF nMode = D_DESCRIPTION
                     IF LEN( cBuffer ) > LONGLINE
                        write_error( "General", cBuffer, nLineCnt, ;
                                     LONGLINE, aDirList[ i, F_NAME ] )
                     ENDIF
                     lBlankLine := EMPTY( cBuffer )
                     procos2desc(cBuffer,oOs2,"Description")
        

                  ELSEIF nMode = D_EXAMPLE
                     IF LEN( cBuffer ) > LONGLINE
                        write_error( "General", cBuffer, nLineCnt, ;
                                     LONGLINE, aDirList[ i, F_NAME ] )
                     ENDIF
                     lBlankLine := EMPTY( cBuffer )

                     IF lAddBlank
                        oOs2:WritePar( "" ) //:endpar()
                        lAddBlank := .F.
                     ENDIF

                     procos2desc(cBuffer,oOs2,"Example")

                  ELSEIF nMode = D_STATUS
                     IF !EMPTY( cBuffer )
                        oOs2:WriteParBold( "Status" )
                     ENDIF
                     ProcStatusOs2( oOs2, cBuffer )

                  ELSE

                     //  unknown data from somewhere

                     write_error( "Unknown Data Type " + cBuffer,, ;
                                  nLineCnt, ;
                                  LONGONELINE, aDirList[ i, F_NAME ] )

                  ENDIF
               ENDIF
            ENDIF
         ENDIF
      ENDDO
      //  Close down the input file

      FT_FUSE()
   NEXT
   oOs2:Close()
RETURN NIL
*+��������������������������������������������������������������������
*+
*+    Function ProcStatusOs2()
*+
*+��������������������������������������������������������������������
*+
FUNCTION ProcStatusOs2( nWriteHandle, cBuffer )
   IF LEN( ALLTRIM( cBuffer ) ) >1
      nWriteHandle:WritePar( cBuffer)
   ELSEIF SUBSTR( ALLTRIM( cBuffer ), 1 ) == "R"
      nWriteHandle:WritePar( "   Ready" )
   ELSEIF SUBSTR( ALLTRIM( cBuffer ), 1 ) == "S"
      nWriteHandle:WritePar( "   Started" )
   ELSE
      nWriteHandle:WritePar( "   Not Started" )
   ENDIF
RETURN nil
*+��������������������������������������������������������������������
*+
*+    Function ProcOs2Also()
*+
*+��������������������������������������������������������������������
*+
FUNCTION ProcOs2Also( cSeealso )
   aAlso := {}
   aAlso := ListAsArray2( cSeealso, "," )
RETURN aAlso

FUNCTION Formatos2Buff(cBuffer,cStyle,ongi)

LOCAL cReturn:=''
LOCAL cLine:=''
LOCAL cBuffend:=''
LOCAL cEnd,cStart ,coline:=''
LOCAL lEndBuff:=.f.

LOCAL nPos,nPosEnd,lArgBold:=.f.

      cReturn :=cBuffer+' '
      IF AT('</par>',cReturn)>0 .OR. EMPTY(cBuffer)
         IF EMPTY(cbuffer)
         cReturn:=''
         ENDIF
         Return cReturn
      ENDIF
   IF cStyle != "Syntax" .AND. cStyle !="Arguments"
       DO WHILE !lEndBuff
                cLine :=  TRIM(SUBSTR( ReadLN( @lEof ), nCommentLen ) )
          IF AT('</par>',cLine)>0 
             lEndBuff:=.t.
          ENDIF
      IF EMPTY(cLine)
         lEndBuff:=.t.

            ft_fskip(-1)
      ENDIF
      if At(DELIM,cline)>0
        
          FT_Fskip(-1)
        lEndBuff:=.t.
      endif  
      if at(DELIM,cLine)=0
      cReturn+=' '+alltrim(cLine)+ ' '
      endif
    enddo
    creturn:=strtran(creturn,"<par>","")
    creturn:=strtran(creturn,"</par>","")
    
    cReturn:='<par>'+creturn+'    </par>'

  ELSEIF cStyle=='Syntax'
           cReturn:='<par>'+cReturn+' </par>'
  ELSEIF cStyle=='Arguments'
  nPos:=0
    if at("<par>",cReturn)>0
            cReturn:=STRTRAN(cReturn,"<par>","")
            cReturn:=STRTRAN(cReturn,"</par>","")
            cReturn:=alltrim(cReturn)
            nPos:=AT(" ",cReturn)
            cOLine:=left(cReturn,nPos-1)
            cReturn:=STRTRAN(cReturn,coLine,"")
            if at("@",cOLine)>0 .or. at("()",cOLine)>0 .or. at("<",cOLine)>0 .or. at("_",cOLine)>0
             lArgBold:=.T.
            else
            lArgBold:= .f.
           endif

    endif
       DO WHILE !lEndBuff
                cLine :=  TRIM(SUBSTR( ReadLN( @lEof ), nCommentLen ) )
      IF AT('</par>',cLine)>0 
         lEndBuff:=.t.
      ENDIF
      IF EMPTY(cLine)
         lEndBuff:=.t.
                 ft_fskip(-1)
      ENDIF
      if At(DELIM,cline)>0
        ft_fskip(-1)
        lEndBuff:=.t.
      endif
      if at(DELIM,cline)=0
      cReturn+=' '+alltrim(cLine)+ ' '
      endif
    enddo
    creturn:=strtran(creturn,"<par>","")
    creturn:=strtran(creturn,"</par>","")
    if lArgBold
        cReturn:='       <par><b>'+cOLine+'</b> '+cReturn+'    </par>'
      else
            cReturn:='       <par>'+cOLine+' '+cReturn+'    </par>'
      endif
    lArgBold:=.F.

   ENDIF
Return cReturn

func checkos2color(cbuffer,ncolorpos)
LOCAL ncolorend,nreturn,cOldColorString,cReturn,ccolor

do while at("<color:",cbuffer)>0
          nColorPos:=AT("<color:",cBuffer)
          ccolor:=substr(cbuffer,ncolorpos+7)
          nColorend:=AT(">",ccolor)
          ccolor:=substr(ccolor,1,nColorend-1)
          cOldColorString:=Substr(cbuffer,ncolorpos)
          nColorend:=AT(">",cOldColorString)
          cOldColorString:=Substr(cOldColorString,1,nColorEnd)
nreturn:=ascan(acolortable,{|x,y| upper(x[1])==upper(ccolor)})
if nreturn >0
  creturn:=+acolortable[nreturn,2]
endif
cBuffer:=strtran(cBuffer,cOldColorString,cReturn)
enddo
return cbuffer

Function ProcOs2Table(cBuffer)

LOCAL nPos,cItem,cItem2,cItem3,xtype,nColorpos,cColor
      if AT("<color:",cBuffer)>0
         nColorPos:=AT(":",cBuffer)
         cColor:=SubStr(cBuffer,nColorpos+1)
         nPos:=at(">",ccolor)
            cColor:=substr(ccolor,1,nPos-1)
         cBuffer:=strtran(cbuffer,"</color>","")
         cBuffer:=STRTRAn(cbuffer,"<color:","")
         cBuffer:=STRTRAn(cbuffer,">","")
         cBuffer:=Strtran(cBuffer,ccolor,'')
         nColorpos:=ASCAn(aColorTable,{|x,y| upper(x[1])==upper(ccolor)})
         cColor:=aColortable[nColorPos,2]
      Endif
      if !empty(cBuffer)
      cItem:=cBuffer
      else
      citem:=''
      endif
        AADD(afiTable,cItem)


Return Nil          


func maxos2elem(a)
LOCAL nsize:=len(a)
LOCAL max:=0
LOCAL tam:=0,max2:=0
LOCAL nPos:=1
LOCAL cString
LOCAL ncount
for ncount:=1 to nsize
      tam:=len(a[ncount])
    max:=if(tam>max,tam,max)
next
nPos:=ascan(a,{|x| Len(x)==max})
return max
  

Function Genos2Table(oOs2)
LOCAL y,nLen2,x,nMax,nSpace,lCar:=.f.,nMax2,nSpace2,nPos1,nPos2,LColor,nPos
LOCAL aLensFItem:={}
LOCAL aLensSItem:={}

  FOR X:=1 to LEN(afitable)
     AADD(aLensFItem,Len(afiTable[x]))

  NEXT
  ASORT(aLensFItem,,,{|x,y| x > y})



        oOs2:WritePar("")
//  nMax2:=checkcar(aTable,1)+1
 nMax2:=alensfitem[1]
 nPos:=maxrtfelem(afitable)
 nPos2:=ascan(alensfitem,{|x| x==nPos})  

oOs2:WriteText(':cgraphic.')
oOs2:WritePar(" "+repl('�',80))
FOR x:=1 to len(afiTable)
  oOs2:WritePar(IF(at("|",afiTable[x])>0,Strtran(afiTable[x],"|"," "),afiTable[x]))
Next
oOs2:WritePar(" "+repl('�',80))
oOs2:WriteText(':ecgraphic.')
 oOs2:WritePar("")
afiTable:={}

Return Nil


FUNCTION  Procos2Desc(cBuffer,oOs2,cStyle)
LOCAL cLine:=''
LOCAL lArgBold
LOCAL npos,CurPos:=0
LOCAL nColorPos,ccolor:='',creturn:='',ncolorend,NIDENTLEVEL,coline
LOCAL lEndPar:= .F.

LOCAL lEndFixed:=.F.
LOCAL lEndTable:=.F.
default cStyle to "Default"
lendfixed:=.F.
if at('<par>',cBuffer)==0 .and. !empty(cBuffer) .and. cstyle<>"Example"
    cBuffer:='<par>'+cBuffer
endif
if empty(cBuffer)
oOs2:WritePar("")
endif


if cStyle<>"Example" .and. at("<table>",cBuffer)==0 .and. AT("<fixed>",cBuffer)=0
   if AT("<par>",cBuffer)>=0 .or. AT("</par>",cBuffer)=0   .and. !empty(cbuffer) 
      If AT("<par>",cBuffer)>0 .and. AT("</par>",cBuffer)>0
      
         if cStyle=="Arguments"
            creturn:=cBuffer

            cReturn:=STRTRAN(cReturn,"<par>","")
            cReturn:=STRTRAN(cReturn,"</par>","")
            cReturn:=alltrim(cReturn)
            nPos:=AT(" ",cReturn)
            cOLine:=left(cReturn,nPos-1)
            cReturn:=STRTRAN(cReturn,coLine,"")
            if at("@",cOLine)>0 .or. at("()",cOLine)>0 .or. at("<",cOLine)>0 .or. at("_",cOLine)>0
             lArgBold:=.T.
            else
            lArgBold:= .f.
           endif

        //            cBuffer:= strtran(cBuffer,"<par>","<par><b>")
    if lArgBold
        cReturn:='       <par><b>'+cOLine+'</b> '+cReturn+'    </par>'
      else
            cReturn:='       <par>'+cOLine+' '+cReturn+'    </par>'
      endif




          cbuffer:=cReturn

             endif
 
      else
      cBuffer:=Formatos2Buff(cBuffer,cStyle,oOs2)
      endif
endif
endif


If AT('<par>',cBuffer)>0 .and. AT('</par>',cBuffer)>0
      cBuffer:=Strtran(cBuffer,'<par>','')
      cBuffer:=StrTran(cBuffer,'<b>',':hp2. ')
      cBuffer:=StrTran(cBuffer,'</b>',':ehp2. ')
      cBuffer:=StrTran(cBuffer,'<em>',':hp3. ')
      cBuffer:=StrTran(cBuffer,'</em>',':ehp3. ')
      cBuffer:=StrTran(cBuffer,'<i>',':hp1. ')
      cBuffer:=StrTran(cBuffer,'</i>',':ehp1. ')
      cBuffer:=Strtran(cBuffer,'</color>','')
      nColorPos:=at('<color:',cBuffer)
      if ncolorpos>0
      checkos2color(@cbuffer,ncolorpos)
      endif

      If cStyle=="Description" .or. cStyle=="Compliance"
          nIdentLevel:=6
          nPos:=0
          if AT('</par>',cBuffer)>0
             cBuffer:=strtran(cBuffer,"</par>","")
          endif
          if  !empty(cBuffer)
             cBuffer:=SUBSTR(cBuffer,2)
             oOs2:WritePar(cBuffer)
          endif

      ELSEIf cStyle=="Arguments"

         if AT('</par>',cBuffer)>0
            cBuffer:=strtran(cBuffer,"</par>","")
         endif
         if  !empty(cBuffer)
                   cBuffer:=SUBSTR(cBuffer,2)
            oOs2:writeText(cBuffer)
         endif

      ELSEIf cStyle=="Syntax"
          if AT('</par>',cBuffer)>0
             cBuffer:=strtran(cBuffer,"</par>","")
          endif
          if  !empty(cBuffer)
                    cBuffer:=SUBSTR(cBuffer,2)
             oOs2:WriteParBold(cBuffer)
          endif

Elseif cStyle=="Default"
          if AT('</par>',cBuffer)>0
             cBuffer:=strtran(cBuffer,"</par>","")
          endif
          if  !empty(cBuffer)
                    cBuffer:=SUBSTR(cBuffer,2)
             oOs2:WritePar(cBuffer)
          endif


endif
endif
If AT('<fixed>',cBuffer)>0 .or. cStyle="Example"

         if at('<fixed>',cBuffer)=0 .or. !empty(cBuffer)
            cBuffer:=Strtran(cBuffer,"<par>","")
            cBuffer:=Strtran(cBuffer,"<fixed>","")
            oOs2:WritePar(cBuffer)
         endif                

      DO WHILE !lendFixed
         cLine := TRIM( SUBSTR( ReadLN( @lEof ), nCommentLen ) )
         IF AT( "</fixed>", cLine ) > 0
            lendfixed := .t.
            cLine     := STRTRAN( cLine, "</fixed>", "" )
         ENDIF
         IF AT( DELIM, cLine ) = 0
            cReturn += ALLTRIM( cLine ) + ' '
         ENDIF
         IF AT( DELIM, cLine ) > 0
            FT_FSKIP( - 1 )
            lEndfixed := .t.

         ENDIF
         IF AT( DELIM, cLine ) == 0
            oOs2:WritePar( cLine )
         ENDIF
      ENDDO


end
if AT('<table>',cBuffer)>0
    do while !lendTable
        cBuffer :=  TRIM(SUBSTR( ReadLN( @lEof ), nCommentLen ) )
        if  at("</table>",cBuffer)>0
          lendTable:=.t.
        else
          procos2table(cBuffer)
    endif
    enddo
    if lEndTable
      Genos2Table(oOs2)
    endif
endif


return nil

