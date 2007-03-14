/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The declarations for all harbour defined functions/procedures.
 *
 * Copyright 1999 Ryszard Glab <rglab@imid.med.pl>
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

#ifndef HB_EXTERN_CH_
#define HB_EXTERN_CH_

#include "hbsetup.ch"

/* CA-Cl*pper compatible standard functions */

EXTERNAL AADD
EXTERNAL ABS
EXTERNAL ACHOICE
EXTERNAL ACLONE
EXTERNAL ACOPY
EXTERNAL ADEL
EXTERNAL ADIR
EXTERNAL AEVAL
EXTERNAL AFIELDS
EXTERNAL AFILL
EXTERNAL AINS
EXTERNAL ALERT
EXTERNAL ALIAS
EXTERNAL ALLTRIM
EXTERNAL AMPM
EXTERNAL ARRAY
EXTERNAL ASC
EXTERNAL ASCAN
EXTERNAL ASIZE
EXTERNAL ASORT
EXTERNAL AT
EXTERNAL ATAIL
EXTERNAL BIN2I
EXTERNAL BIN2L
EXTERNAL BIN2W
EXTERNAL BOF
EXTERNAL BREAK
EXTERNAL BROWSE
EXTERNAL CDOW
EXTERNAL CHR
EXTERNAL CMONTH
EXTERNAL COL
EXTERNAL COLORSELECT
EXTERNAL CTOD
EXTERNAL CURDIR
EXTERNAL DATE
EXTERNAL DAY
EXTERNAL DAYS
EXTERNAL DBAPPEND
EXTERNAL DBCLEARFILTER
EXTERNAL DBCLEARINDEX
EXTERNAL DBCLEARRELATION
EXTERNAL DBCLOSEALL
EXTERNAL DBCLOSEAREA
EXTERNAL DBCOMMIT
EXTERNAL DBCOMMITALL
EXTERNAL DBCREATE
EXTERNAL DBCREATEINDEX
EXTERNAL DBDELETE
EXTERNAL DBEDIT
EXTERNAL DBEVAL
EXTERNAL DBF
EXTERNAL DBFILTER
EXTERNAL DBGOBOTTOM
EXTERNAL DBGOTO
EXTERNAL DBGOTOP
EXTERNAL DBRECALL
EXTERNAL DBREINDEX
EXTERNAL DBRELATION
EXTERNAL DBRLOCK
EXTERNAL DBRLOCKLIST
EXTERNAL DBRSELECT
EXTERNAL DBRUNLOCK
EXTERNAL DBSEEK
EXTERNAL DBSELECTAREA
EXTERNAL DBSETDRIVER
EXTERNAL DBSETFILTER
EXTERNAL DBSETINDEX
EXTERNAL DBSETORDER
EXTERNAL DBSETRELATION
EXTERNAL DBSKIP
EXTERNAL DBSTRUCT
EXTERNAL DBUNLOCK
EXTERNAL DBUNLOCKALL
EXTERNAL DBUSEAREA
EXTERNAL DELETED
EXTERNAL DESCEND
EXTERNAL DEVOUT
EXTERNAL DEVOUTPICT
EXTERNAL DEVPOS
EXTERNAL DIRECTORY
EXTERNAL DISKSPACE
EXTERNAL DISPBEGIN
EXTERNAL DISPBOX
EXTERNAL DISPCOUNT
EXTERNAL DISPEND
EXTERNAL DISPOUT
EXTERNAL DISPOUTAT /* Undocumented but always required */
EXTERNAL DO
EXTERNAL DOSERROR
EXTERNAL DOW
EXTERNAL DTOC
EXTERNAL DTOS
EXTERNAL ELAPTIME
EXTERNAL EMPTY
EXTERNAL EOF
EXTERNAL ERRORBLOCK
EXTERNAL ERRORLEVEL
EXTERNAL ERRORNEW
EXTERNAL EVAL
EXTERNAL EXP
EXTERNAL FCLOSE
EXTERNAL FCOUNT
EXTERNAL FCREATE
EXTERNAL FERASE
EXTERNAL FERROR
EXTERNAL FIELDBLOCK
EXTERNAL FIELDGET
EXTERNAL FIELDNAME
EXTERNAL FIELDPOS
EXTERNAL FIELDPUT
#ifdef HB_EXTENSION
EXTERNAL FIELDLEN   /* harbour extension */
EXTERNAL FIELDDEC   /* harbour extension */
EXTERNAL FIELDTYPE  /* harbour extension */
#endif
EXTERNAL FIELDWBLOCK
EXTERNAL FILE
EXTERNAL FKLABEL
EXTERNAL FKMAX
EXTERNAL FLOCK
EXTERNAL FOPEN
EXTERNAL FOUND
EXTERNAL FREAD
EXTERNAL FREADSTR
EXTERNAL FRENAME
EXTERNAL FSEEK
EXTERNAL FWRITE
EXTERNAL GETACTIVE
EXTERNAL GETAPPLYKEY
EXTERNAL GETDOSETKEY
EXTERNAL GETE
EXTERNAL GETENV
EXTERNAL GETNEW
EXTERNAL GETPOSTVALIDATE
EXTERNAL GETPREVALIDATE
EXTERNAL GETREADER
EXTERNAL HARDCR
EXTERNAL HEADER
EXTERNAL I2BIN
EXTERNAL INDEXEXT
EXTERNAL INDEXKEY
EXTERNAL INDEXORD
EXTERNAL INKEY
EXTERNAL INT
EXTERNAL ISALPHA
EXTERNAL ISCOLOR
EXTERNAL ISDIGIT
EXTERNAL ISLOWER
EXTERNAL ISPRINTER
EXTERNAL ISUPPER
EXTERNAL L2BIN
EXTERNAL LASTKEY
EXTERNAL LASTREC
EXTERNAL LEFT
EXTERNAL LEN
EXTERNAL LENNUM
EXTERNAL LOCK
EXTERNAL LOG
EXTERNAL LOWER
EXTERNAL LTRIM
EXTERNAL LUPDATE
EXTERNAL MAX
EXTERNAL MAXCOL
EXTERNAL MAXROW
EXTERNAL MEMOEDIT
EXTERNAL MEMOLINE
EXTERNAL MEMOREAD
EXTERNAL MEMORY
EXTERNAL MEMOTRAN
EXTERNAL MEMOWRIT
EXTERNAL MEMVARBLOCK
EXTERNAL MENUMODAL
EXTERNAL MIN
EXTERNAL MLCOUNT
EXTERNAL MLCTOPOS
EXTERNAL MLPOS
EXTERNAL MOD
EXTERNAL MONTH
EXTERNAL MPOSTOLC
EXTERNAL NETERR
EXTERNAL NETNAME
EXTERNAL NEXTKEY
EXTERNAL NOSNOW
EXTERNAL ORDBAGEXT
EXTERNAL ORDBAGNAME
EXTERNAL ORDCONDSET
EXTERNAL ORDCREATE
EXTERNAL ORDDESTROY
EXTERNAL ORDFOR
EXTERNAL ORDKEY
EXTERNAL ORDLISTADD
EXTERNAL ORDLISTCLEAR
EXTERNAL ORDLISTREBUILD
EXTERNAL ORDNAME
EXTERNAL ORDNUMBER
EXTERNAL ORDSCOPE
EXTERNAL ORDSETFOCUS
EXTERNAL OS
EXTERNAL OUTERR
EXTERNAL OUTSTD
EXTERNAL PAD
EXTERNAL PADC
EXTERNAL PADL
EXTERNAL PADR
EXTERNAL PCOL
EXTERNAL PCOUNT
EXTERNAL PROCLINE
EXTERNAL PROCNAME
EXTERNAL PROW
EXTERNAL QOUT
EXTERNAL QQOUT
EXTERNAL RANGECHECK /* Undocumented but always required */
EXTERNAL RAT
EXTERNAL RDDLIST
EXTERNAL RDDNAME
EXTERNAL RDDREGISTER
EXTERNAL RDDSETDEFAULT
EXTERNAL READEXIT
EXTERNAL READFORMAT
EXTERNAL READINSERT
EXTERNAL READKEY
EXTERNAL READKILL
EXTERNAL READMODAL
EXTERNAL READUPDATED
EXTERNAL READVAR
EXTERNAL RECCOUNT
EXTERNAL RECNO
EXTERNAL RECSIZE
EXTERNAL REPLICATE
EXTERNAL RESTSCREEN
EXTERNAL RIGHT
EXTERNAL RLOCK
EXTERNAL ROUND
EXTERNAL ROW
EXTERNAL RTRIM
EXTERNAL SAVESCREEN
EXTERNAL SCROLL
EXTERNAL SECONDS
EXTERNAL SECS
EXTERNAL SELECT
EXTERNAL SET
EXTERNAL SETBLINK
EXTERNAL SETCANCEL
EXTERNAL SETCOLOR
EXTERNAL SETCURSOR
EXTERNAL SETKEY
EXTERNAL SETMODE
EXTERNAL SETPOS
EXTERNAL SETPOSBS /* Undocumented but always required */
EXTERNAL SETPRC
EXTERNAL SOUNDEX
EXTERNAL SPACE
EXTERNAL SQRT
EXTERNAL STR
EXTERNAL STRTRAN
EXTERNAL STRZERO
EXTERNAL STUFF
EXTERNAL SUBSTR
EXTERNAL TBCOLUMNNEW
EXTERNAL TBROWSEDB
EXTERNAL TBROWSENEW
EXTERNAL TIME
EXTERNAL TONE
EXTERNAL TRANSFORM
EXTERNAL TRIM
EXTERNAL TSTRING
EXTERNAL TYPE
EXTERNAL UPDATED
EXTERNAL UPPER
EXTERNAL USED
EXTERNAL VAL
EXTERNAL VALTYPE
EXTERNAL VERSION
EXTERNAL WORD
EXTERNAL YEAR

/* Harbour extensions, always on */

EXTERNAL HB_ARGC
EXTERNAL HB_ARGCHECK
EXTERNAL HB_ARGSTRING
EXTERNAL HB_ARGV
EXTERNAL HB_COLORINDEX
EXTERNAL HB_COMPILER
EXTERNAL HB_FNAMEMERGE
EXTERNAL HB_FNAMESPLIT
EXTERNAL HB_LANGNAME
EXTERNAL HB_LANGSELECT
#ifndef HB_CDP_SUPPORT_OFF
EXTERNAL HB_SETCODEPAGE
EXTERNAL HB_TRANSLATE
EXTERNAL HB_CDPLIST
#endif
EXTERNAL HB_OSNEWLINE
EXTERNAL HB_PVALUE
EXTERNAL HB_STOD
EXTERNAL HBCLASS
EXTERNAL HBOBJECT
EXTERNAL HB_WILDMATCH
EXTERNAL HB_CLOCKS2SECS
EXTERNAL HB_MATHERMODE
EXTERNAL HB_MATHERBLOCK
EXTERNAL HB_LIBLOAD
EXTERNAL HB_LIBFREE
EXTERNAL HB_LIBDO

EXTERNAL HB_GTVERSION
EXTERNAL HB_GTSYS
EXTERNAL HB_GTINFO
EXTERNAL HB_SETDISPCP
EXTERNAL HB_SETKEYCP
EXTERNAL HB_SETTERMCP

EXTERNAL __DYNSN2PTR
EXTERNAL __DYNSP2NAME


/* CA-Cl*pper compatible internal functions */

EXTERNAL CLIPPER520
EXTERNAL __ACCEPT
EXTERNAL __ATPROMPT
EXTERNAL __COPYFILE
EXTERNAL __DBAPP
EXTERNAL __DBARRANGE
EXTERNAL __DBCONTINUE
EXTERNAL __DBCOPY
EXTERNAL __DBCOPYSTRUCT
EXTERNAL __DBCOPYXSTRUCT
EXTERNAL __DBCREATE
EXTERNAL __DBDELIM
EXTERNAL __DBJOIN
EXTERNAL __DBLIST
EXTERNAL __DBLOCATE
EXTERNAL __DBOPENSDF
EXTERNAL __DBPACK
EXTERNAL __DBSDF
EXTERNAL __DBSETFOUND
EXTERNAL __DBSETLOCATE
EXTERNAL __DBSORT
EXTERNAL __DBSTRUCTFILTER
EXTERNAL __DBTOTAL
EXTERNAL __DBTRANS
EXTERNAL __DBTRANSREC
EXTERNAL __DBUPDATE
EXTERNAL __DBZAP
EXTERNAL __DIR
EXTERNAL __EJECT
EXTERNAL __GET
EXTERNAL __GETA
EXTERNAL __GETMESSAGE
EXTERNAL __KEYBOARD
EXTERNAL __KILLREAD
EXTERNAL __LABELFORM
EXTERNAL __MCLEAR
EXTERNAL __MENUTO
EXTERNAL __MRELEASE
EXTERNAL __MRESTORE
EXTERNAL __MSAVE
EXTERNAL __MXRELEASE
EXTERNAL __QUIT
EXTERNAL __RDDSETDEFAULT
EXTERNAL __REPORTFORM
EXTERNAL __RUN
EXTERNAL __SETCENTURY
EXTERNAL __SETFORMAT
EXTERNAL __SETHELPK
EXTERNAL __TEXTRESTORE
EXTERNAL __TEXTSAVE
EXTERNAL __TYPEFILE
EXTERNAL __WAIT
EXTERNAL __XHELP
EXTERNAL __XRESTSCREEN
EXTERNAL __XSAVESCREEN

/* Harbour internal functions */

EXTERNAL __CLASSADD
EXTERNAL __CLASSINSTANCE
EXTERNAL __CLASSNAME
EXTERNAL __CLASSNEW
EXTERNAL __CLASSSEL
EXTERNAL __CLS_CNTCLSDATA
EXTERNAL __CLS_CNTDATA
EXTERNAL __CLS_DECDATA
EXTERNAL __CLS_INCDATA
EXTERNAL __CLSADDMSG
EXTERNAL __CLSDELMSG
EXTERNAL __CLSINST
EXTERNAL __CLSINSTSUPER
EXTERNAL __CLSMODMSG
EXTERNAL __CLSNEW
EXTERNAL __CLSPARENT
EXTERNAL __ERRINHANDLER
EXTERNAL __ERRRT_BASE
EXTERNAL __ERRRT_SBASE
EXTERNAL __HRBRUN
EXTERNAL __HRBLOAD
EXTERNAL __HRBDO
EXTERNAL __HRBUNLOAD
EXTERNAL __HRBGETFU
EXTERNAL __HRBDOFU
EXTERNAL __MVCLEAR
EXTERNAL __MVDBGINFO
EXTERNAL __MVEXIST
EXTERNAL __MVGET
EXTERNAL __MVPRIVATE
EXTERNAL __MVPUBLIC
EXTERNAL __MVPUT
EXTERNAL __MVRELEASE
EXTERNAL __MVRESTORE
EXTERNAL __MVSAVE
EXTERNAL __MVSCOPE
EXTERNAL __MVXRELEASE
EXTERNAL __OBJADDDATA
EXTERNAL __OBJADDINLINE
EXTERNAL __OBJADDMETHOD
EXTERNAL __OBJCLONE
EXTERNAL __OBJDELDATA
EXTERNAL __OBJDELINLINE
EXTERNAL __OBJDELMETHOD
EXTERNAL __OBJDERIVEDFROM
EXTERNAL __OBJGETCLSNAME
EXTERNAL __OBJGETMETHODLIST
EXTERNAL __OBJGETMSGLIST
EXTERNAL __OBJGETVALUELIST
EXTERNAL __OBJHASDATA
EXTERNAL __OBJHASMETHOD
EXTERNAL __OBJHASMSG
EXTERNAL __OBJMODINLINE
EXTERNAL __OBJMODMETHOD
EXTERNAL __OBJSENDMSG
EXTERNAL __OBJSETVALUELIST
EXTERNAL __SENDER

/* The debugger interface */

EXTERNAL HB_DBG_VMPARLLIST
EXTERNAL HB_DBG_VMSTKGCOUNT
EXTERNAL HB_DBG_VMSTKGLIST
EXTERNAL HB_DBG_VMSTKLCOUNT
EXTERNAL HB_DBG_VMSTKLLIST
EXTERNAL HB_DBG_VMVARLGET
EXTERNAL HB_DBG_VMVARLSET
EXTERNAL HB_DBG_VMVARSGET
EXTERNAL HB_DBG_VMVARSSET
EXTERNAL HB_DBG_VMVARSLEN
EXTERNAL HB_DBG_VMVARSLIST
EXTERNAL HB_DBG_PROCLEVEL

/* RDD related symbols */

EXTERNAL _DBF, DBF_GETFUNCTABLE
EXTERNAL DBFFPT, DBFFPT_GETFUNCTABLE
EXTERNAL DBFNTX, DBFNTX_GETFUNCTABLE
EXTERNAL DBFCDX, DBFCDX_GETFUNCTABLE
EXTERNAL DELIM, DELIM_GETFUNCTABLE
EXTERNAL SDF, SDF_GETFUNCTABLE

EXTERNAL RDDSYS

/* CA-Cl*pper 5.2 compatible undocumented functions */

#ifdef HB_C52_UNDOC

EXTERNAL __ACCEPTSTR
EXTERNAL __ATCLEAR
EXTERNAL __BOX
EXTERNAL __BOXD
EXTERNAL __BOXS
EXTERNAL __CLEAR
EXTERNAL __DBAPPEND
EXTERNAL __DBCLEARINDEX
EXTERNAL __DBCLEARRELATION
EXTERNAL __DBCLOSE
EXTERNAL __DBCLOSEAREA
EXTERNAL __DBCOMMIT
EXTERNAL __DBCOMMITALL
EXTERNAL __DBCREATINDEX
EXTERNAL __DBDELETE
EXTERNAL __DBGOBOTTOM
EXTERNAL __DBGOTO
EXTERNAL __DBGOTOP
EXTERNAL __DBRECALL
EXTERNAL __DBREINDEX
EXTERNAL __DBSEEK
EXTERNAL __DBSELECT
EXTERNAL __DBSETFILTER
EXTERNAL __DBSETINDEX
EXTERNAL __DBSETORDER
EXTERNAL __DBSETRELATION
EXTERNAL __DBSKIP
EXTERNAL __DBUNLALL
EXTERNAL __DBUNLOCK
EXTERNAL __DBUSE
EXTERNAL __DEFPATH
EXTERNAL __FLEDIT
EXTERNAL __INPUT
EXTERNAL __NONOALERT
EXTERNAL __QQPUB
EXTERNAL _NATMSGVER
EXTERNAL _NATSORTVER
EXTERNAL DBGSHADOW
EXTERNAL DEFPATH
EXTERNAL ISAFFIRM
EXTERNAL ISNEGATIVE
EXTERNAL NATIONMSG
EXTERNAL PROCFILE
EXTERNAL SETTYPEAHEAD

#endif /* HB_C52_UNDOC */

/* CA-Cl*pper 5.3 compatible functions */

#ifdef HB_COMPAT_C53

EXTERNAL __CAPTION
EXTERNAL __GUICOLOR
EXTERNAL _CHECKBOX_
EXTERNAL _LISTBOX_
EXTERNAL _PUSHBUTT_
EXTERNAL _RADIOGRP_
EXTERNAL CHECKBOX
EXTERNAL CLIPPER530
EXTERNAL DBFIELDINFO
EXTERNAL DBFILEGET
EXTERNAL DBFILEPUT
EXTERNAL DBINFO
EXTERNAL DBORDERINFO
EXTERNAL DBRECORDINFO
EXTERNAL DBTABLEEXT
EXTERNAL DIRCHANGE
EXTERNAL DIRREMOVE
EXTERNAL DISKCHANGE
EXTERNAL DISKNAME
EXTERNAL FSETDEVMOD
EXTERNAL GUIAPPLYKEY
EXTERNAL GUIGETPOSTVALIDATE
EXTERNAL GUIGETPREVALIDATE
EXTERNAL GUIREADER
EXTERNAL ISDEFCOLOR
EXTERNAL ISDISK
EXTERNAL MAKEDIR
EXTERNAL MCOL
EXTERNAL MDBLCLK
EXTERNAL MENUITEM
EXTERNAL MHIDE
EXTERNAL MLEFTDOWN
EXTERNAL MPRESENT
EXTERNAL MRESTSTATE
EXTERNAL MRIGHTDOWN
EXTERNAL MROW
EXTERNAL MSAVESTATE
EXTERNAL MSETBOUNDS
EXTERNAL MSETCURSOR
EXTERNAL MSETPOS
EXTERNAL MSHOW
EXTERNAL ORDDESCEND
EXTERNAL ORDISUNIQUE
EXTERNAL ORDKEYADD
EXTERNAL ORDKEYCOUNT
EXTERNAL ORDKEYDEL
EXTERNAL ORDKEYGOTO
EXTERNAL ORDKEYNO
EXTERNAL ORDKEYVAL
EXTERNAL ORDSETRELATION
EXTERNAL ORDSKIPUNIQUE
EXTERNAL POPUP
EXTERNAL RADIOBUTTO
EXTERNAL RADIOGROUP
EXTERNAL TOPBAR

#endif /* HB_COMPAT_C53 */

/* Xbase++ compatible functions */

#ifdef HB_COMPAT_XPP

EXTERNAL BIN2U
EXTERNAL CONVTOANSICP
EXTERNAL CONVTOOEMCP
EXTERNAL CURDRIVE
EXTERNAL DBSKIPPER
EXTERNAL NUMBUTTONS
EXTERNAL SETMOUSE
EXTERNAL STOD
EXTERNAL U2BIN
EXTERNAL W2BIN
EXTERNAL ORDWILDSEEK

#endif /* HB_COMPAT_XPP */

/* Harbour extensions */

#ifdef HB_EXTENSION

EXTERNAL __DYNSCOUNT
EXTERNAL __DYNSGETINDEX
EXTERNAL __DYNSGETNAME

EXTERNAL __PP_INIT
EXTERNAL __PP_PATH
EXTERNAL __PP_RESET
EXTERNAL __PP_ADDRULE
EXTERNAL __PP_PROCESS
EXTERNAL __PP_STDRULES

EXTERNAL HB_ANSITOOEM
EXTERNAL HB_CLRAREA
EXTERNAL HB_COLORTON
EXTERNAL HB_DISKSPACE
EXTERNAL HB_FCREATE
EXTERNAL HB_FSIZE
EXTERNAL HB_FTEMPCREATE
EXTERNAL HB_GCALL
EXTERNAL HB_KEYPUT
EXTERNAL HB_OEMTOANSI
EXTERNAL HB_SETKEYARRAY
EXTERNAL HB_SETKEYCHECK
EXTERNAL HB_SETKEYGET
EXTERNAL HB_SETKEYSAVE
EXTERNAL HB_SHADOW
EXTERNAL HB_TRACELEVEL
EXTERNAL HB_TRACESTATE
EXTERNAL HB_VALTOSTR

EXTERNAL HB_REGEXCOMP
EXTERNAL HB_ISREGEX
EXTERNAL HB_ATX
EXTERNAL HB_REGEX
EXTERNAL HB_REGEXMATCH
EXTERNAL HB_REGEXSPLIT
EXTERNAL HB_REGEXATX
EXTERNAL HB_REGEXALL

#endif /* HB_EXTENSION */

/* FlagShip extension */

#ifdef HB_COMPAT_FLAGSHIP

EXTERNAL STRPEEK
EXTERNAL STRPOKE
EXTERNAL SECONDSCPU

#endif /* HB_COMPAT_FLAGSHIP */

/* xHarbour extension */

#ifdef HB_COMPAT_XHB

#ifndef __PLATFORM__DOS
EXTERNAL INETINIT
EXTERNAL INETCLEANUP
EXTERNAL INETCREATE
EXTERNAL INETCLOSE
EXTERNAL INETDESTROY
EXTERNAL INETSTATUS
EXTERNAL INETSTATUSDESC
EXTERNAL INETERRORCODE
EXTERNAL INETERRORDESC
EXTERNAL INETCLEARERROR
EXTERNAL INETCOUNT
EXTERNAL INETADDRESS
EXTERNAL INETPORT
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
#endif /* __PLATFORM__DOS */

#endif /* HB_COMPAT_XHB */

#endif /* HB_EXTERN_CH_ */
