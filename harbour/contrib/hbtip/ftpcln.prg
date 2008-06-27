/*
 * $Id$
 */

/*
 * xHarbour Project source code:
 * TIP Class oriented Internet protocol library
 *
 * Copyright 2003 Giancarlo Niccolai <gian@niccolai.ws>
 *
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

/* 2007-04-19, Hannes Ziegler <hz AT knowlexbase.com>
   Added method :RMD()
   Added method :listFiles()
   Added method :MPut()
   Changed method :downloadFile() to enable display of progress
   Changed method :uploadFile() to enable display of progress
*/

/* 2007-06-01, Toninho@fwi
   Added method UserCommand( cCommand, lPasv, lReadPort, lGetReply )
*/

/* 2007-07-12, miguelangel@marchuet.net
   Added method :NoOp()
   Added method :Rest( nPos )
   Changed method :LS( cSpec )
   Changed method :List( cSpec )
   Changed method :TransferStart()
   Changed method :Stor( cFile )
   Changed method :UploadFile( cLocalFile, cRemoteFile )
   Changed method :DownloadFile( cLocalFile, cRemoteFile )

   Added support to Port transfer mode
   Added method :Port()
   Added method :SendPort()

   Cleaned unused variables.
*/

/* 2007-09-08 21:34 UTC+0100 Patrick Mast <patrick/dot/mast/at/xharbour.com>
   * source\tip\ftpcln.prg
     * Formatting
     + METHOD StartCleanLogFile()
       Starts a clean log file, overwriting current logfile.
     + METHOD fileSize( cFileSpec )
       Calculates the filesize of the given files specifications.
     + DATA cLogFile
       Holds the filename of the current logfile.
     ! Fixed logfilename in New(), now its not limited to 9999 log files anymore
     ! Fixed MGet() due to changes in HB_aTokens()
     ! Fixed listFiles() due to changes in HB_aTokens()
     ! listFiles() is still buggy. Needs to be fixed.
*/

#include "directry.ch"
#include "hbclass.ch"
#include "tip.ch"
#include "common.ch"

STATIC nPort := 16000

/**
* Inet service manager: ftp
*/

CLASS tIPClientFTP FROM tIPClient
   DATA nDataPort
   DATA cDataServer
   DATA bUsePasv
   DATA RegBytes
   DATA RegPasv
   // Socket opened in response to a port command
   DATA SocketControl
   DATA SocketPortServer
   DATA cLogFile

   METHOD New( oUrl, lTrace, oCredentials )
   METHOD Open()
   METHOD Read( nLen )
   METHOD Write( nLen )
   METHOD Close()
   METHOD TransferStart()
   METHOD Commit()

   METHOD GetReply()
   METHOD Pasv()
   METHOD TypeI()
   METHOD TypeA()
   METHOD NoOp()
   METHOD Rest( nPos )
   METHOD List( cSpec )
   METHOD UserCommand( cCommand, lPasv, lReadPort, lGetReply )
   METHOD Pwd()
   METHOD Cwd( cPath )
   METHOD Dele( cPath )
   METHOD Port()
   METHOD SendPort()
   METHOD Retr( cFile )
   METHOD Stor( cFile )
   METHOD Quit()
   METHOD ScanLength()
   METHOD ReadAuxPort()
   METHOD mget()
   // Method bellow contributed by  Rafa Carmona

   METHOD LS( cSpec )
   METHOD Rename( cFrom, cTo )
   // new method for file upload
   METHOD UpLoadFile( cLocalFile, cRemoteFile )
   // new method to download file
   METHOD DownLoadFile( cLocalFile, cRemoteFile )
   // new method to create an directory on ftp server
   METHOD MKD( cPath )

   METHOD RMD( cPath )
   METHOD listFiles( cList )
   METHOD MPut
   METHOD StartCleanLogFile()
   METHOD fileSize( cFileSpec )
ENDCLASS


METHOD New( oUrl,lTrace, oCredentials) CLASS tIPClientFTP
   local cFile :="ftp"
   local n := 0

   ::super:new( oUrl, lTrace, oCredentials)
   ::nDefaultPort := 21
   ::nConnTimeout := 3000
   ::bUsePasv     := .T.
   ::nAccessMode  := TIP_RW  // a read-write protocol
   ::nDefaultSndBuffSize := 65536
   ::nDefaultRcvBuffSize := 65536

   if ::ltrace
      if !file("ftp.log")
         ::nHandle := fcreate("ftp.log")
      else
         while file(cFile+LTrim(str(Int(n)))+".log")
           n++
         enddo
         ::cLogFile:= cFile+LTrim(str(Int(n)))+".log"
         ::nHandle := fcreate(::cLogFile)
      endif
   endif

   // precompilation of regex for better prestations
   ::RegBytes := HB_RegexComp( "\(([0-9]+)[ )a-zA-Z]" )
   ::RegPasv :=  HB_RegexComp( "([0-9]*) *, *([0-9]*) *, *([0-9]*) *, *([0-9]*) *, *([0-9]*) *, *([0-9]*)" )

RETURN Self


METHOD StartCleanLogFile() CLASS tIPClientFTP
  fclose(::nHandle)
  ::nHandle := fcreate(::cLogFile)
RETURN NIL


METHOD Open( cUrl ) CLASS tIPClientFTP

   IF HB_IsString( cUrl )
      ::oUrl := tUrl():New( cUrl )
   ENDIF

   IF Len( ::oUrl:cUserid ) == 0 .or. Len( ::oUrl:cPassword ) == 0
      RETURN .F.
   ENDIF

   IF .not. ::super:Open()
      RETURN .F.
   ENDIF

   HB_InetTimeout( ::SocketCon, ::nConnTimeout )
   IF ::GetReply()
      ::InetSendall( ::SocketCon, "USER " + ::oUrl:cUserid + ::cCRLF )
      IF ::GetReply()
         ::InetSendall( ::SocketCon, "PASS " + ::oUrl:cPassword + ::cCRLF )
         // set binary by default
         IF ::GetReply() .and. ::TypeI()
            RETURN .T.
         ENDIF
      ENDIF
   ENDIF
RETURN .F.

METHOD GetReply() CLASS tIPClientFTP
   LOCAL nLen
   LOCAL cRep

   ::cReply := ::InetRecvLine( ::SocketCon, @nLen, 128 )

   cRep := ::cReply

   IF cRep == NIL
      RETURN .F.
   ENDIF

   // now, if the reply has a "-" as fourth character, we need to proceed...
   DO WHILE .not. Empty(cRep) .and. SubStr( cRep, 4, 1 ) == "-"
      ::cReply := ::InetRecvLine( ::SocketCon, @nLen, 128 )
      cRep := IIf(ValType(::cReply) == "C", ::cReply, "")
   ENDDO

   // 4 and 5 are error codes
   IF ::InetErrorCode( ::SocketCon ) != 0 .or. Left( ::cReply, 1 ) >= "4"
      RETURN .F.
   ENDIF

RETURN .T.

METHOD Pasv() CLASS tIPClientFTP
   LOCAL aRep

   ::InetSendall( ::SocketCon, "PASV" + ::cCRLF )
   IF .not. ::GetReply()
      RETURN .F.
   ENDIF
   aRep := HB_Regex( ::RegPasv, ::cReply )

   IF Empty(aRep)
      RETURN .F.
   ENDIF

   ::cDataServer := aRep[2] + "." + aRep[3] + "." + aRep[4] + "." + aRep[5]
   ::nDataPort := Val(aRep[6]) *256 + Val( aRep[7] )

RETURN .T.


METHOD Close() CLASS tIPClientFTP
   HB_InetTimeOut( ::SocketCon, ::nConnTimeout )
   if ::ltrace
      fClose(::nHandle)
   endif

   ::Quit()
RETURN ::super:Close()


METHOD Quit() CLASS tIPClientFTP
   ::InetSendall( ::SocketCon, "QUIT" + ::cCRLF )
RETURN ::GetReply()


METHOD TypeI() CLASS tIPClientFTP
   ::InetSendall( ::SocketCon, "TYPE I" + ::cCRLF )
RETURN ::GetReply()


METHOD TypeA() CLASS tIPClientFTP
   ::InetSendall( ::SocketCon, "TYPE A" + ::cCRLF )
RETURN ::GetReply()


METHOD NoOp() CLASS tIPClientFTP
   ::InetSendall( ::SocketCon, "NOOP" + ::cCRLF )
RETURN ::GetReply()


METHOD Rest( nPos ) CLASS tIPClientFTP
   ::InetSendall( ::SocketCon, "REST " + AllTrim( Str( iif( Empty( nPos ), 0, nPos ) ) ) + ::cCRLF )
RETURN ::GetReply()


METHOD CWD( cPath ) CLASS tIPClientFTP
   ::InetSendall( ::SocketCon, "CWD " + cPath + ::cCRLF )
RETURN ::GetReply()


METHOD PWD() CLASS tIPClientFTP

   ::InetSendall( ::SocketCon, "PWD"  + ::cCRLF )
   IF .not. ::GetReply()
      RETURN .F.
   ENDIF
   ::cReply := SubStr( ::cReply, At('"', ::cReply) + 1, ;
                                Rat('"', ::cReply) - At('"', ::cReply) - 1 )
RETURN .T.


METHOD DELE( cPath ) CLASS tIPClientFTP
   ::InetSendall( ::SocketCon, "DELE " + cPath + ::cCRLF )
RETURN ::GetReply()


// scan last reply for an hint of length
METHOD ScanLength() CLASS tIPClientFTP
   LOCAL aBytes
   aBytes := HB_Regex( ::RegBytes, ::cReply )
   IF .not. Empty(aBytes)
      ::nLength := Val( aBytes[2] )
   ENDIF
RETURN .T.


METHOD TransferStart() CLASS tIPClientFTP
   LOCAL skt
   ::SocketControl := ::SocketCon

   IF ::bUsePasv
      skt := HB_InetConnectIP( ::cDataServer, ::nDataPort )
      IF skt != NIL .and. ::InetErrorCode( skt ) == 0
         // Get the start message from the control connection
         IF ! ::GetReply()
            HB_InetClose( skt )
            RETURN .F.
         ENDIF

         HB_InetTimeout( skt, ::nConnTimeout )
         /* Set internal socket send buffer to 64k,
         * this should fix the speed problems some users have reported
         */
         IF ! Empty( ::nDefaultSndBuffSize )
            ::InetSndBufSize( skt, ::nDefaultSndBuffSize )
         ENDIF
   
         IF ! Empty( ::nDefaultRcvBuffSize )
            ::InetRcvBufSize( skt, ::nDefaultRcvBuffSize )
         ENDIF

         ::SocketCon := skt
      ENDIF
   ELSE
      ::SocketCon := HB_InetAccept( ::SocketPortServer )
      IF Empty( ::SocketCon )
         ::bInitialized := .F.
         ::SocketCon := ::SocketControl
         ::GetReply()
         RETURN .F.
      ENDIF
      HB_InetSetRcvBufSize( ::SocketCon, 65536 )
      HB_InetSetSndBufSize( ::SocketCon, 65536 )
   ENDIF

RETURN .T.


METHOD Commit() CLASS tIPClientFTP
   HB_InetClose( ::SocketCon )
   ::SocketCon := ::SocketControl
   ::bInitialized := .F.

   IF .not. ::GetReply()
      RETURN .F.
   ENDIF

   // error code?
   IF Left( ::cReply, 1 ) == "5"
      RETURN .F.
   ENDIF

RETURN .T.


METHOD List( cSpec ) CLASS tIPClientFTP
   LOCAL cStr

   IF cSpec == nil
      cSpec := ""
   ELSE
      cSpec := " " + cSpec
   ENDIF
   IF ::bUsePasv
      IF .not. ::Pasv()
         //::bUsePasv := .F.
         RETURN .F.
      ENDIF
   ENDIF

   IF .not. ::bUsePasv
      IF .not. ::Port()
         RETURN .F.
      ENDIF
   ENDIF

   ::InetSendAll( ::SocketCon, "LIST" + cSpec + ::cCRLF )
   cStr := ::ReadAuxPort()
   ::bEof := .f.
RETURN cStr

METHOD UserCommand( cCommand, lPasv, lReadPort, lGetReply ) CLASS tIPClientFTP

   DEFAULT cCommand  TO ""
   DEFAULT lPasv     TO .t.
   DEFAULT lReadPort TO .t.
   DEFAULT lGetReply TO .f.

   if ::bUsePasv .and. lPasv .and. !::Pasv()
      return .f.
   endif

   ::InetSendAll( ::SocketCon, cCommand )

   if lReadPort
      lReadPort := ::ReadAuxPort()
   endif

   if lGetReply
      lGetReply := ::GetReply()
   endif

RETURN .t.


METHOD ReadAuxPort(cLocalFile) CLASS tIPClientFTP
   LOCAL cRet, cList := "",nFile:=0

   IF .not. ::TransferStart()
      RETURN NIL
   END
   IF !empty(cLocalFile)
      nFile:=fcreate(cLocalFile)
   ENDIF
   cRet := ::super:Read( 512 )
   WHILE cRet != NIL .and. len( cRet ) > 0
      IF nFile>0
         fwrite(nFile,cRet)
      else
            cList += cRet
      ENDIF
      cRet := ::super:Read( 512 )
   END

   HB_InetClose( ::SocketCon )
   ::SocketCon := ::SocketControl
   IF ::GetReply()
   IF nFile>0
      fclose(nFile)
      return(.t.)
   ENDIF
    RETURN cList
   ENDIF
RETURN NIL


METHOD Stor( cFile ) CLASS tIPClientFTP

   IF ::bUsePasv
      IF .not. ::Pasv()
         //::bUsePasv := .F.
         RETURN .F.
      ENDIF
   ENDIF

   ::InetSendall( ::SocketCon, "STOR " + cFile + ::cCRLF )

   // It is important not to delete these lines in order not to disrupt the timing of
   // the responses, which can lead to failures in transfers.
   IF ! ::bUsePasv
      ::GetReply()
   ENDIF

RETURN ::TransferStart()


METHOD Port() CLASS tIPClientFTP

   ::SocketPortServer := HB_InetCreate( ::nConnTimeout )
   nPort ++
   DO WHILE nPort < 24000
      HB_InetServer( nPort, ::SocketPortServer )
      IF ::InetErrorCode( ::SocketPortServer ) == 0
         RETURN ::SendPort()
      ENDIF
      nPort ++
   ENDDO

RETURN .F.


METHOD SendPort() CLASS tIPClientFTP
   LOCAL cAddr
   LOCAL cPort, nPort

   cAddr := HB_InetGetHosts( NetName() )[1]
   cAddr := StrTran( cAddr, ".", "," )
   nPort := HB_InetPort( ::SocketPortServer )
   cPort := "," + AllTrim( Str( Int( nPort / 256 ) ) ) +  "," + AllTrim( Str( Int( nPort % 256 ) ) )

   ::InetSendall( ::SocketCon, "PORT " + cAddr + cPort  + ::cCRLF )
RETURN ::GetReply()


METHOD Read( nLen ) CLASS tIPClientFTP
   LOCAL cRet

   IF .not. ::bInitialized

      IF .not. Empty( ::oUrl:cPath )

         IF .not. ::CWD( ::oUrl:cPath )

            ::bEof := .T.  // no data for this transaction
            RETURN .F.

         ENDIF

      ENDIF

      IF Empty( ::oUrl:cFile )

         RETURN ::List()

      ENDIF

      IF .not. ::Retr( ::oUrl:cFile )

         ::bEof := .T.  // no data for this transaction
         RETURN .F.

      ENDIF

      // now channel is open
      ::bInitialized := .T.

   ENDIF

   cRet := ::super:Read( nLen )

   IF cRet == NIL

      ::Commit()
      ::bEof := .T.

   ENDIF

RETURN cRet

*
* FTP transfer wants commit only at end.
*
METHOD Write( cData, nLen ) CLASS tIPClientFTP

   IF .not. ::bInitialized

      IF Empty( ::oUrl:cFile )

         RETURN -1

      ENDIF

      IF .not. Empty( ::oUrl:cPath )

         IF .not. ::CWD( ::oUrl:cPath )
            RETURN -1
         ENDIF

      ENDIF

      IF .not. ::Stor( ::oUrl:cFile )
         RETURN -1
      ENDIF

      // now channel is open
      ::bInitialized := .T.
   ENDIF

RETURN ::super:Write( cData, nLen, .F. )

/*
 * HZ: What's cLocalFile good for? It's unused
 */

METHOD Retr( cFile ) CLASS tIPClientFTP

   IF ::bUsePasv
      IF .not. ::Pasv()
         //::bUsePasv := .F.
         RETURN .F.
      ENDIF
   ENDIF

   ::InetSendAll( ::SocketCon, "RETR " + cFile + ::cCRLF )

   IF ::TransferStart()
      ::ScanLength()
      RETURN .T.
   ENDIF

RETURN .F.

METHOD MGET( cSpec,cLocalPath ) CLASS tIPClientFTP

   LOCAL cStr,cfile,aFiles

   IF cSpec == nil
      cSpec := ""
   ENDIF
   IF cLocalPath=nil
      cLocalPath:=""
   ENDIF
   IF ::bUsePasv
      IF .not. ::Pasv()
         //::bUsePasv := .F.
         RETURN .F.
      ENDIF
   ENDIF

   ::InetSendAll( ::SocketCon, "NLST " + cSpec + ::cCRLF )
   cStr := ::ReadAuxPort()

   IF !empty(cStr)
      aFiles:=hb_atokens(strtran(cStr,chr(13),""),chr(10))
      FOR each cFile in aFiles
         IF !Empty(cFile) //PM:09-08-2007 Needed because of the new HB_aTokens()
            ::downloadfile( cLocalPath+trim(cFile), trim(cFile) )
         ENDIF
      NEXT

   ENDIF

RETURN cStr

METHOD MPUT( cFileSpec, cAttr ) CLASS tIPClientFTP

   LOCAL cPath,cFile, cExt, aFile, aFiles
   LOCAL nCount := 0
   LOCAL cStr := ""

   IF !( Valtype( cFileSpec ) == "C" )
      RETURN 0
   ENDIF

   HB_FNameSplit( cFileSpec, @cPath, @cFile, @cExt  )

   aFiles := Directory( cPath + cFile + cExt, cAttr )

   FOR each aFile in aFiles
      IF ::uploadFile( cPath + aFile[F_NAME], aFile[F_NAME] )
         cStr += HB_InetCrlf() + aFile[F_NAME]
      ENDIF
   NEXT
RETURN SubStr(cStr,3)


METHOD UpLoadFile( cLocalFile, cRemoteFile ) CLASS tIPClientFTP

   LOCAL cPath := ""
   LOCAL cFile := ""
   LOCAL cExt  := ""

   HB_FNameSplit( cLocalFile, @cPath, @cFile,@cExt  )

   DEFAULT cRemoteFile to cFile + cExt

   ::bEof := .F.
   ::oUrl:cFile := cRemoteFile

   IF ! ::bInitialized

      IF Empty( ::oUrl:cFile )
         RETURN .F.
      ENDIF

      IF ! Empty( ::oUrl:cPath )

         IF ! ::CWD( ::oUrl:cPath )
            RETURN .F.
         ENDIF

      ENDIF

      IF ! ::bUsePasv .AND. ! ::Port()
         RETURN .F.
      ENDIF

      IF ! ::Stor( ::oUrl:cFile )
         RETURN .F.
      ENDIF

      // now channel is open
      ::bInitialized := .T.
   ENDIF

RETURN ::WriteFromFile( cLocalFile )


METHOD LS( cSpec ) CLASS tIPClientFTP

   LOCAL cStr

   IF cSpec == nil
      cSpec := ""
   ENDIF

   IF ::bUsePasv .AND. ! ::Pasv()
      //::bUsePasv := .F.
      RETURN .F.
   ENDIF

   IF ! ::bUsePasv .AND. ! ::Port()
      RETURN .F.
   ENDIF

   ::InetSendAll( ::SocketCon, "NLST " + cSpec + ::cCRLF )
   IF ::GetReply()
      cStr := ::ReadAuxPort()
   ELSE
      cStr := ""
   ENDIF

RETURN cStr

/*Rename a traves del ftp */

METHOD Rename( cFrom, cTo ) CLASS tIPClientFTP
    Local lResult  := .F.

    ::InetSendAll( ::SocketCon, "RNFR " + cFrom + ::cCRLF )

    IF ::GetReply()

       ::InetSendAll( ::SocketCon, "RNTO " + cTo + ::cCRLF )
       lResult := ::GetReply()

    ENDIF

RETURN lResult

METHOD DownLoadFile( cLocalFile, cRemoteFile ) CLASS tIPClientFTP

   LOCAL cPath := ""
   LOCAL cFile := ""
   LOCAL cExt  := ""

   HB_FNameSplit( cLocalFile, @cPath, @cFile, @cExt  )


   DEFAULT cRemoteFile to cFile+cExt

   ::bEof := .F.
   ::oUrl:cFile := cRemoteFile

   IF ! ::bInitialized

      IF ! Empty( ::oUrl:cPath ) .AND. ! ::CWD( ::oUrl:cPath )
         ::bEof := .T.  // no data for this transaction
         RETURN .F.
      ENDIF

      IF ! ::bUsePasv .AND. ! ::Port()
         RETURN .F.
      ENDIF

      IF ! ::Retr( ::oUrl:cFile )
         ::bEof := .T.  // no data for this transaction
         RETURN .F.
      ENDIF

      // now channel is open
      ::bInitialized := .T.

   ENDIF

RETURN ::ReadToFile( cLocalFile, , ::nLength )


// Create a new folder
METHOD MKD( cPath ) CLASS tIPClientFTP
   ::InetSendall( ::SocketCon, "MKD " + cPath + ::cCRLF )
RETURN ::GetReply()


// Delete an existing folder
METHOD RMD( cPath ) CLASS tIPClientFTP
   ::InetSendall( ::SocketCon, "RMD " + cPath + ::cCRLF )
RETURN ::GetReply()


// Return total file size for <cFileSpec>
METHOD fileSize( cFileSpec ) CLASS tIPClientFTP
   LOCAL aFiles:=::ListFiles( cFileSpec ), nSize:=0, n
   FOR n =1 TO Len(aFiles)
       nSize+=Val(aFiles[n][7]) // Should [7] not be [F_SIZE] ?
   NEXT
RETURN nSize


// Parse the :list() string into a Directory() compatible 2-dim array
METHOD listFiles( cFileSpec ) CLASS tIPClientFTP
   LOCAL aMonth:= { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }
   LOCAL cList, aList, aFile, cEntry, nStart, nEnd
   LOCAL cYear, cMonth, cDay, cTime

   cList := ::list( cFileSpec )

   IF Empty( cList )
      RETURN {}
   ENDIF

   aList := HB_ATokens( StrTran( cList, Chr(13),""), Chr(10) )

   FOR EACH cEntry IN aList

      IF Empty( cEntry ) //PM:09-08-2007 Needed because of the new HB_aTokens()
      
         hb_ADel(aList, cEntry:__enumIndex(), .T.)
      
      ELSE
   
         aFile         := Array( F_LEN+3 )
         nStart        := 1
         nEnd          := hb_At( Chr(32), cEntry, nStart )

         // file permissions (attributes)
         aFile[F_ATTR] := SubStr( cEntry, nStart, nEnd-nStart )
         nStart        := nEnd

         // # of links
         DO WHILE SubStr( cEntry, ++nStart, 1 ) == " " ; ENDDO
         nEnd          := hb_At( Chr(32), cEntry, nStart )
         aFile[F_LEN+1]:= Val( SubStr( cEntry, nStart, nEnd-nStart ) )
         nStart        := nEnd

         // owner name
         DO WHILE SubStr( cEntry, ++nStart, 1 ) == " " ; ENDDO
         nEnd          := hb_At( Chr(32), cEntry, nStart )
         aFile[F_LEN+2]:= SubStr( cEntry, nStart, nEnd-nStart )
         nStart        := nEnd

         // group name
         DO WHILE SubStr( cEntry, ++nStart, 1 ) == " " ; ENDDO
         nEnd          := hb_At( Chr(32), cEntry, nStart )
         aFile[F_LEN+3]:= SubStr( cEntry, nStart, nEnd-nStart )
         nStart        := nEnd

         // file size
         DO WHILE SubStr( cEntry, ++nStart, 1 ) == " " ; ENDDO
         nEnd          := hb_At( Chr(32), cEntry, nStart )
         aFile[F_SIZE] := Val( SubStr( cEntry, nStart, nEnd-nStart ) )
         nStart        := nEnd

         // Month
         DO WHILE SubStr( cEntry, ++nStart, 1 ) == " " ; ENDDO
         nEnd          := hb_At( Chr(32), cEntry, nStart )
         cMonth        := SubStr( cEntry, nStart, nEnd-nStart )
         cMonth        := PadL( AScan( aMonth, cMonth ), 2, "0" )
         nStart        := nEnd

         // Day
         DO WHILE SubStr( cEntry, ++nStart, 1 ) == " " ; ENDDO
         nEnd          := hb_At( Chr(32), cEntry, nStart )
         cDay          := SubStr( cEntry, nStart, nEnd-nStart )
         nStart        := nEnd

         // year
         DO WHILE SubStr( cEntry, ++nStart, 1 ) == " " ; ENDDO
         nEnd          := hb_At( Chr(32), cEntry, nStart )
         cYear         := SubStr( cEntry, nStart, nEnd-nStart )
         nStart        := nEnd

         IF ":" $ cYear
            cTime := cYear
            cYear := Str( Year(Date()), 4, 0 )
         ELSE
            cTime := ""
         ENDIF

         // file name
         DO WHILE SubStr( cEntry, ++nStart, 1 ) == " " ; ENDDO

         aFile[F_NAME] := SubStr( cEntry, nStart )
         aFile[F_DATE] := hb_StoD( cYear+cMonth+cDay )
         aFile[F_TIME] := cTime

         aList[ cEntry:__enumIndex() ] := aFile

      ENDIF

   NEXT

RETURN aList
