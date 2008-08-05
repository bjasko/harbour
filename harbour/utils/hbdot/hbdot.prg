/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    "DOt Prompt" Console for the Harbour Language
 *
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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

#include "inkey.ch"
#include "setcurs.ch"
#include "hbextern.ch"

#define HB_HISTORY_LEN 32
#define HB_LINE_LEN    256
#define HB_PROMPT      "."

STATIC s_nRow := 2, s_nCol := 0
STATIC s_aIncDir := {}

/* ********************************************************************** */

PROCEDURE _APPMAIN( cFile, ... )
   LOCAL GetList
   LOCAL cLine, cCommand, cPath, cExt
   LOCAL nMaxRow, nMaxCol
   LOCAL aHistory, nHistIndex
   LOCAL bKeyUP, bKeyDown, bKeyIns

#ifdef _DEFAULT_INC_DIR
   AADD( s_aIncDir, "-I" + _DEFAULT_INC_DIR )
#endif
   cPath := getenv( "HB_INC_INSTALL" )
   IF !EMPTY( cPath )
      AADD( s_aIncDir, "-I" + cPath )
   ENDIF
#ifdef __PLATFORM__UNIX
   AADD( s_aIncDir, "-I/usr/include/harbour" )
   AADD( s_aIncDir, "-I/usr/local/include/harbour" )
#endif

   IF PCount() > 0
      SWITCH lower( cFile )
         CASE "-?"
         CASE "-h"
         CASE "--help"
         CASE "/?"
         CASE "/h"
            HB_DotUsage()
            EXIT
         OTHERWISE
            hb_FNameSplit( cFile, NIL, NIL, @cExt )
            IF Lower( cExt ) == ".prg"
               cFile := HB_COMPILEBUF( HB_ARGV( 0 ), "-n", "-w", "-es2", "-q0", ;
                                       s_aIncDir, cFile )
               IF cFile == NIL
                  ERRORLEVEL( 1 )
               ELSE
                  __hrbRun( cFile, ... )
               ENDIF
            ELSE
               __hrbRun( cFile, ... )
            ENDIF
      ENDSWITCH
   ELSE

      CLEAR SCREEN
      SET SCOREBOARD OFF
      GetList := {}
      cCommand := ""
      aHistory := { padr( "quit", HB_LINE_LEN ) }
      nHistIndex := 2

      DO WHILE .T.

         IF cLine == NIL
            cLine := Space( HB_LINE_LEN )
         ENDIF

         HB_DotInfo( cCommand )

         nMaxRow := MaxRow()
         nMaxCol := MaxCol()
         @ nMaxRow, 0 SAY HB_PROMPT
         @ nMaxRow, Col() GET cLine ;
                          PICTURE "@KS" + LTrim( Str( nMaxCol - Col() + 1 ) )

         SetCursor( IIF( ReadInsert(), SC_INSERT, SC_NORMAL ) )

         bKeyIns  := SetKey( K_INS, ;
            {|| SetCursor( IIF( ReadInsert( !ReadInsert() ), ;
                             SC_NORMAL, SC_INSERT ) ) } )
         bKeyUp   := SetKey( K_UP, ;
            {|| IIF( nHistIndex >  1, ;
                     cLine := aHistory[ --nHistIndex ], ) } )
         bKeyDown := SetKey( K_DOWN, ;
            {|| cLine := IIF( nHistIndex < LEN( aHistory ), ;
                aHistory[ ++nHistIndex ], ;
                ( nHistIndex := LEN( aHistory ) + 1, Space( HB_LINE_LEN ) ) ) } )

         READ

         SetKey( K_DOWN, bKeyDown )
         SetKey( K_UP,   bKeyUp   )
         SetKey( K_INS,  bKeyIns  )

         IF LastKey() == K_ESC .OR. EMPTY( cLine )
            cLine := NIL
            IF nMaxRow != MaxRow() .OR. nMaxCol != MaxCol()
               @ nMaxRow, 0 CLEAR
            ENDIF
            LOOP
         ENDIF

         IF EMPTY( aHistory ) .OR. ! ATAIL( aHistory ) == cLine
            IF LEN( aHistory ) < HB_HISTORY_LEN
               AADD( aHistory, cLine )
            ELSE
               ADEL( aHistory, 1 )
               aHistory[ LEN( aHistory ) ] := cLine
            ENDIF
         ENDIF
         nHistIndex := LEN( aHistory ) + 1

         cCommand := AllTrim( cLine, " " )
         cLine := NIL
         @ nMaxRow, 0 CLEAR
         HB_DotInfo( cCommand )

         HB_DotExec( cCommand )

         IF s_nRow >= MaxRow()
            Scroll( 2, 0, MaxRow(), MaxCol(), 1 )
            s_nRow := MaxRow() - 1
         ENDIF

      ENDDO
   ENDIF

RETURN

/* ********************************************************************** */

STATIC PROCEDURE HB_DotUsage()

   OutStd( 'Harbour "DOt Prompt" Console' + HB_OSNewLine() +;
           'Copyright 1999-2008, Przemyslaw Czerpak' + HB_OSNewLine() + ;
           'http://www.harbour-project.org' + HB_OSNewLine() +;
           HB_OSNewLine() +;
           'Syntax:  hbdot [<hrbfile[.prg|.hrb]> [<parameters,...>]]' + HB_OSNewLine() + ;
           HB_OSNewLine() +;
           "Note:  Linked with " + Version() + HB_OSNewLine() )

RETURN

/* ********************************************************************** */

STATIC PROCEDURE HB_DotInfo( cCommand )

   LOCAL r := Row(), c := Col()

   IF cCommand != NIL
      DispOutAt( 0, 0, "PP: " )
      DispOutAt( 0, 4, PadR( cCommand, MaxCol() - 3 ), "N/R" )
   ENDIF
   IF Used()
      DispOutAt( 1, 0, ;
         PadR( "RDD: " + PadR( RddName(), 6 ) + ;
               " | Area:" + Str( Select(), 3 ) + ;
               " | Dbf: " + PadR( Alias(), 10 ) + ;
               " | Index: " + PadR( OrdName( IndexOrd() ), 8 ) + ;
               " | # " + Str( RecNo(), 7 ) + "/" + Str( RecCount(), 7 ), ;
               MaxCol() + 1 ), "N/BG" )
   ELSE
      DispOutAt( 1, 0, ;
         PadR( "RDD: " + Space( 6 ) + ;
               " | Area:" + Space( 3 ) + ;
               " | Dbf: " + Space( 10 ) + ;
               " | Index: " + Space( 8 ) + ;
               " | # " + Space( 7 ) + "/" + Space( 7 ), ;
               MaxCol() + 1 ), "N/BG" )
   ENDIF
   SetPos( r, c )

RETURN

/* ********************************************************************** */

STATIC PROCEDURE HB_DotErr( oErr, cCommand )

   LOCAL xArg, cMessage

   cMessage := "Sorry, could not execute:;;" + cCommand + ";;"
   IF oErr:ClassName == "ERROR"
      cMessage += oErr:Description
      IF ValType( oErr:Args ) == 'A' .AND. Len( oErr:Args ) > 0
         cMessage += ";Arguments:"
         FOR EACH xArg IN oErr:Args
            cMessage += ";" + HB_CStr( xArg )
         NEXT
      ENDIF
   ELSEIF ValType( oErr ) == 'C'
      cMessage += oErr
   ENDIF
   cMessage += ";;" + ProcName( 2 ) + '(' + LTrim( Str( ProcLine( 2 ) ) ) + ')'

   Alert( cMessage )

   BREAK( oErr )

/* ********************************************************************** */

STATIC PROCEDURE HB_DotExec( cCommand )
   LOCAL pHRB, cHRB, cFunc, bBlock, cEol

   cEol := hb_osNewLine()
   cFunc := "STATIC FUNC __HBDOT()" + cEol + ;
            "RETURN {||" + cEol + ;
            "   " + cCommand + cEol + ;
            "   RETURN __MVSETBASE()" + cEol + ;
            "}" + cEol

   BEGIN SEQUENCE WITH {|oErr| HB_DotErr( oErr, cCommand ) }

      cHRB := HB_COMPILEFROMBUF( cFunc, HB_ARGV( 0 ), "-n", "-q2", s_aIncDir )
      IF cHRB == NIL
         EVAL( ErrorBlock(), "Syntax error." )
      ELSE
         pHRB := __hrbLoad( cHRB )
         IF pHrb != NIL
            bBlock := __hrbDo( pHRB )
            DevPos( s_nRow, s_nCol )
            Eval( bBlock )
            s_nRow := Row()
            s_nCol := Col()
            IF s_nRow < 2
               s_nRow := 2
            ENDIF
         ENDIF
      ENDIF

   ENDSEQUENCE

   __MVSETBASE()

RETURN

/* ********************************************************************** */

/* request for full screen GT driver */
#if   defined( __PLATFORM__WINCE )
REQUEST HB_GT_WVT_DEFAULT
#elif defined( __PLATFORM__WINDOWS )
REQUEST HB_GT_WIN_DEFAULT
#elif defined( __PLATFORM__UNIX )
REQUEST HB_GT_TRM_DEFAULT
#elif defined( __PLATFORM__DOS )
REQUEST HB_GT_DOS_DEFAULT
#elif defined( __PLATFORM__OS2 )
REQUEST HB_GT_OS2_DEFAULT
#endif

/* ********************************************************************** */
