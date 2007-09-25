/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * 
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


#ifndef __PLATFORM__DOS
EXTERNAL INETINIT
EXTERNAL INETCLEANUP
EXTERNAL INETCREATE
EXTERNAL INETCLOSE
EXTERNAL INETDESTROY
EXTERNAL INETSTATUS
EXTERNAL INETERRORCODE
EXTERNAL INETERRORDESC
EXTERNAL INETCLEARERROR
EXTERNAL INETCOUNT
EXTERNAL INETADDRESS
EXTERNAL INETPORT
EXTERNAL INETFD
EXTERNAL INETSETTIMEOUT
EXTERNAL INETGETTIMEOUT
EXTERNAL INETCLEARTIMEOUT
EXTERNAL INETSETTIMELIMIT
EXTERNAL INETGETTIMELIMIT
EXTERNAL INETCLEARTIMELIMIT
EXTERNAL INETSETPERIODCALLBACK
EXTERNAL INETGETPERIODCALLBACK
EXTERNAL INETCLEARPERIODCALLBACK
EXTERNAL INETRECV
EXTERNAL INETRECVALL
EXTERNAL INETRECVLINE
EXTERNAL INETRECVENDBLOCK
EXTERNAL INETDATAREADY
EXTERNAL INETSEND
EXTERNAL INETSENDALL
EXTERNAL INETGETHOSTS
EXTERNAL INETGETALIAS
EXTERNAL INETSERVER
EXTERNAL INETACCEPT
EXTERNAL INETCONNECT
EXTERNAL INETCONNECTIP
EXTERNAL INETDGRAMBIND
EXTERNAL INETDGRAM
EXTERNAL INETDGRAMSEND
EXTERNAL INETDGRAMRECV
EXTERNAL INETCRLF
EXTERNAL ISINETSOCKET
#endif /* __PLATFORM__DOS */

EXTERNAL HASH
EXTERNAL HHASKEY
EXTERNAL HGETPOS
EXTERNAL HGET
EXTERNAL HSET
EXTERNAL HDEL
EXTERNAL HGETKEYAT
EXTERNAL HGETVALUEAT
EXTERNAL HSETVALUEAT
EXTERNAL HGETPAIRAT
EXTERNAL HDELAT
EXTERNAL HGETKEYS
EXTERNAL HGETVALUES
EXTERNAL HFILL
EXTERNAL HCLONE
EXTERNAL HCOPY
EXTERNAL HMERGE
EXTERNAL HEVAL
EXTERNAL HSCAN
EXTERNAL HSETCASEMATCH
EXTERNAL HGETCASEMATCH
EXTERNAL HSETAUTOADD
EXTERNAL HGETAUTOADD
EXTERNAL HALLOCATE
EXTERNAL HDEFAULT

EXTERNAL WILDMATCH
