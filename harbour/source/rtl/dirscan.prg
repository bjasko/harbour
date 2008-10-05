/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    HB_DirScan()
 *
 * Copyright 2008 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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

STATIC FUNCTION hb_doScan( cPath, cMask, cAttr, cPathSep )

   LOCAL aFile
   LOCAL lMatch
   LOCAL aResult := {}

   FOR EACH aFile IN Directory( cPath + hb_osFileMask(), cAttr + "D" )
      lMatch := hb_FileMatch( aFile[ 1 ], cMask )
      IF "D" $ aFile[ 5 ]
         IF lMatch .AND. "D" $ cAttr
            AAdd( aResult, aFile )
         ENDIF
         IF !( aFile[ 1 ] == "." .OR. aFile[ 1 ] == ".." .OR. aFile[ 1 ] == "" )
            AEval( hb_DoScan( cPath + aFile[ 1 ] + cPathSep, cMask, cAttr, cPathSep ), ;
                   { |x| x[ 1 ] := aFile[ 1 ] + cPathSep + x[ 1 ], ;
                         AAdd( aResult, x ) } )
         ENDIF
      ELSEIF lMatch
         AAdd( aResult, aFile )
      ENDIF
   NEXT

   RETURN aResult

FUNCTION hb_DirScan( cPath, cFileMask, cAttr )

   LOCAL cFilePath
   LOCAL cPathSep := hb_osPathSeparator()

   IF Empty( cPath )
      cFilePath := ""
   ELSE
      cFilePath := cPath
      IF !Right( cPath, 1 ) $ hb_osPathDelimiters()
         cFilePath += cPathSep
      ENDIF
   ENDIF

   RETURN HB_DoScan( cFilePath, ;
                     iif( Empty( cFileMask ), hb_osFileMask(), cFileMask ), ;
                     iif( ValType( cAttr ) $ "CM", cAttr, "" ), ;
                     cPathSep )
