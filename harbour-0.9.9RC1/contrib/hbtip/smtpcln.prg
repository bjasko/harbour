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

/* 2007-04-12, Hannes Ziegler <hz AT knowlexbase.com>
   Added method :sendMail()
*/

#include "hbclass.ch"
#include "tip.ch"

/**
* Inet service manager: smtp
*/

CLASS tIPClientSMTP FROM tIPClient

   METHOD New( oUrl, lTrace, oCredentials )
   METHOD Open()
   METHOD Close()
   METHOD Write( cData, nLen, bCommit )
   METHOD Mail( cFrom )
   METHOD Rcpt( cRcpt )
   METHOD Data( cData )
   METHOD Commit()
   METHOD Quit()
   METHOD GetOK()
   /* Method for smtp server that require login */
   METHOD OpenSecure()
   METHOD AUTH( cUser, cPass) // Auth by login method
   METHOD AUTHplain( cUser, cPass) // Auth by plain method
   METHOD ServerSuportSecure(lAuthp,lAuthl)

   METHOD sendMail
   HIDDEN:
   DATA isAuth INIT .F.
ENDCLASS

METHOD New( oUrl, lTrace, oCredentials ) CLASS tIPClientSMTP
local cFile :="sendmail"
local n:=1
   ::super:New( oUrl, lTrace, oCredentials )

   ::nDefaultPort := 25
   ::nConnTimeout := 5000
   ::nAccessMode := TIP_WO  // a write only

   if ::ltrace
      if !file("sendmail.log")
         ::nHandle := fcreate("sendmail.log")
      else
         while file(cFile+alltrim(str(n,4))+".log")
           n++
         enddo
         ::nHandle := fcreate(cFile+alltrim(str(n,4))+".log")
      endif
   endif
RETURN Self

METHOD Open( cUrl ) CLASS tIPClientSMTP

   IF .not. ::super:Open( cUrl )
      RETURN .F.
   ENDIF

   HB_InetTimeout( ::SocketCon, ::nConnTimeout )
   IF .not. Empty ( ::oUrl:cUserid )
      ::InetSendall( ::SocketCon, "HELO " +  ::oUrl:cUserid + ::cCRLF )
   ELSE
      ::InetSendall( ::SocketCon, "HELO tipClientSMTP" + ::cCRLF )
   ENDIF

RETURN ::GetOk()


METHOD GetOk() CLASS tIPClientSMTP
   LOCAL nLen

   ::cReply := ::InetRecvLine( ::SocketCon, @nLen, 512 )
   IF ::InetErrorCode( ::SocketCon ) != 0 .or. Substr( ::cReply, 1, 1 ) == '5'
      RETURN .F.
   ENDIF
RETURN .T.


METHOD Close() CLASS tIPClientSMTP
   HB_InetTimeOut( ::SocketCon, ::nConnTimeout )
   if ::ltrace
      fClose(::nHandle)
   endif
   ::Quit()
RETURN ::super:Close()

METHOD Commit() CLASS tIPClientSMTP
   ::InetSendall( ::SocketCon, ::cCRLF + "." + ::cCRLF )
RETURN ::GetOk()


METHOD Quit() CLASS tIPClientSMTP
   ::InetSendall( ::SocketCon, "QUIT" + ::cCRLF )
   ::isAuth := .F.
RETURN ::GetOk()


METHOD Mail( cFrom ) CLASS tIPClientSMTP
   ::InetSendall( ::SocketCon, "MAIL FROM: <" + cFrom +">" + ::cCRLF )
RETURN ::GetOk()


METHOD Rcpt( cTo ) CLASS tIPClientSMTP
   ::InetSendall( ::SocketCon, "RCPT TO: <" + cTo + ">" + ::cCRLF )
RETURN ::GetOk()


METHOD Data( cData ) CLASS tIPClientSMTP
   ::InetSendall( ::SocketCon, "DATA" + ::cCRLF )
   IF .not. ::GetOk()
      RETURN .F.
   ENDIF
   ::InetSendall(::SocketCon, cData + ::cCRLF + "." + ::cCRLF )
RETURN ::GetOk()



METHOD OpenSecure( cUrl ) CLASS tIPClientSMTP

   Local cUser

   IF .not. ::super:Open( cUrl )
      RETURN .F.
   ENDIF

   HB_InetTimeout( ::SocketCon, ::nConnTimeout )

   cUser := ::oUrl:cUserid

   IF .not. Empty ( ::oUrl:cUserid )
      ::InetSendall( ::SocketCon, "EHLO " +  cUser + ::cCRLF )
   ELSE
      ::InetSendall( ::SocketCon, "EHLO tipClientSMTP" + ::cCRLF )
   ENDIF

RETURN ::getOk()

METHOD AUTH( cUser, cPass) CLASS tIPClientSMTP

   Local cs:=''
   Local cEncodedUser
   Local cEncodedPAss

   cUser := StrTran( cUser,"&at;", "@")

   cEncodedUser := alltrim(HB_BASE64(cuser,len(cuser)))
   cEncodedPAss :=alltrim(HB_BASE64(cPass,len(cpass)))


   ::InetSendall( ::SocketCon, "AUTH LOGIN" +::ccrlf )

   if ::GetOk()
      ::InetSendall( ::SocketCon, cEncodedUser+::cCrlf  )
      if ::Getok()
         ::InetSendall( ::SocketCon, cEncodedPass +::cCrlf )
      endif
   endif

   return ( ::isAuth := ::GetOk() )

METHOD AuthPlain( cUser, cPass) CLASS tIPClientSMTP

   Local cBase := BUILDUSERPASSSTRING( cUser, cPass )
   Local cen   := HB_BASE64( cBase, 2 + Len( cUser ) + Len( cPass ) )

   ::InetSendall( ::SocketCon, "AUTH PLAIN" + cen + ::cCrlf)
   return ( ::isAuth := ::GetOk() )


METHOD Write( cData, nLen, bCommit ) CLASS tIPClientSMTP
Local aTo,cRecpt
   IF .not. ::bInitialized
      //IF Empty( ::oUrl:cUserid ) .or. Empty( ::oUrl:cFile )
      IF Empty( ::oUrl:cFile )  //GD user id not needed if we did not auth
         RETURN -1
      ENDIF

      IF .not. ::Mail( ::oUrl:cUserid )
         RETURN -1
      ENDIF
      aTo:= HB_RegexSplit(",", ::oUrl:cFile )

      FOR each cRecpt in Ato
         IF .not.   ::Rcpt(cRecpt)
            RETURN -1
         ENDIF
      NEXT

      ::InetSendall( ::SocketCon, "DATA" + ::cCRLF )
      IF .not. ::GetOk()
         RETURN -1
      ENDIF
      ::bInitialized := .T.
   ENDIF

   ::nLastWrite := ::super:Write( cData, nLen, bCommit )
RETURN ::nLastWrite

METHOD ServerSuportSecure(lAuthp,lAuthl) CLASS  tIPClientSMTP
   Local lAuthLogin := .F.,lAuthPlain :=.F.

   IF ::OPENSECURE()
      WHILE .T.
         ::GetOk()
         IF ::cReply == NIL
            EXIT
         ELSEIF "LOGIN" $ ::cReply
            lAuthLogin := .T.
         ELSEIF "PLAIN" $ ::cReply
            lAuthPlain := .T.
         ENDIF
      ENDDO
    ::CLOSE()
 ENDIF

   lAuthp:=lAuthPlain
   lAuthl:=lAuthLogin

RETURN  lAuthLogin .OR. lAuthPlain


METHOD sendMail( oTIpMail ) CLASS TIpClientSmtp
   LOCAL cFrom, cTo, aTo

   IF .NOT. ::isOpen
      RETURN .F.
   ENDIF

   IF .NOT. ::isAuth
      ::auth( ::oUrl:cUserId, ::oUrl:cPassWord )
      IF .NOT. ::isAuth
         RETURN .F.
      ENDIF
   ENDIF

   cFrom := oTIpMail:getFieldPart( "From" )
   cTo   := oTIpMail:getFieldPart( "To" )

   cTo   := StrTran( cTo, HB_InetCRLF(), "" )
   cTo   := StrTran( cTo, Chr(9)    , "" )
   cTo   := StrTran( cTo, Chr(32)   , "" )

   aTo   := HB_RegExSplit( "," , cTo )

   ::mail( cFrom )
   FOR EACH cTo IN aTo
      ::rcpt( cTo   )
   NEXT

RETURN ::data( oTIpMail:toString() )

