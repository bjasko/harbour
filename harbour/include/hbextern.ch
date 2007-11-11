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
EXTERNAL ALTD
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
EXTERNAL FIELDLEN
EXTERNAL FIELDDEC
EXTERNAL FIELDTYPE
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
EXTERNAL ORDBAGCLEAR
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

/* Harbour extensions */

#ifdef HB_EXTENSION
EXTERNAL DBDROP
EXTERNAL DBEXISTS
EXTERNAL RDDINFO
#endif

/* HiPer-SEEK / CFTS compatible functions */

EXTERNAL HS_ADD
EXTERNAL HS_CLOSE
EXTERNAL HS_CREATE
EXTERNAL HS_DELETE
EXTERNAL HS_FILTER
EXTERNAL HS_IFDEL
EXTERNAL HS_INDEX
EXTERNAL HS_KEYCOUNT
EXTERNAL HS_NEXT
EXTERNAL HS_OPEN
EXTERNAL HS_REPLACE
EXTERNAL HS_SET
EXTERNAL HS_UNDELETE
EXTERNAL HS_VERIFY
EXTERNAL HS_VERSION

EXTERNAL CFTSADD
EXTERNAL CFTSCLOSE
EXTERNAL CFTSCREA
EXTERNAL CFTSDELETE
EXTERNAL CFTSIFDEL
EXTERNAL CFTSNEXT
EXTERNAL CFTSOPEN
EXTERNAL CFTSRECN
EXTERNAL CFTSREPLAC
EXTERNAL CFTSSET
EXTERNAL CFTSUNDEL
EXTERNAL CFTSVERI
EXTERNAL CFTSVERS

/* SIX3 compatible functions */

EXTERNAL _SX_STRCOMPRESS
EXTERNAL _SX_STRDECOMPRESS
EXTERNAL SX_FCOMPRESS
EXTERNAL SX_FDECOMPRESS
EXTERNAL SX_ENCRYPT
EXTERNAL SX_DECRYPT
EXTERNAL SX_DTOP
EXTERNAL SX_PTOD

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
EXTERNAL __REPORTFORM
EXTERNAL __RUN
EXTERNAL __SETCENTURY
EXTERNAL __SETFORMAT
EXTERNAL __SETFUNCTION
EXTERNAL __SETHELPK
EXTERNAL __TEXTRESTORE
EXTERNAL __TEXTSAVE
EXTERNAL __TYPEFILE
EXTERNAL __WAIT
EXTERNAL __XHELP
EXTERNAL __XRESTSCREEN
EXTERNAL __XSAVESCREEN

/* Harbour internal functions */

EXTERNAL __DBSQL
EXTERNAL __RDDGETTEMPALIAS
EXTERNAL __GETMSGPRF
EXTERNAL __OPGETPRF
EXTERNAL __OPCOUNT
EXTERNAL __SETPROFILER
EXTERNAL __TRACEPRGCALLS

EXTERNAL __DYNSN2PTR
EXTERNAL __DYNSP2NAME
EXTERNAL __DYNSCOUNT
EXTERNAL __DYNSGETINDEX
EXTERNAL __DYNSGETNAME
EXTERNAL __DYNSISFUN
EXTERNAL __DYNSN2SYM
EXTERNAL __DYNSGETPRF

EXTERNAL __CLASSADD
EXTERNAL __CLASSH
EXTERNAL __CLASSINSTANCE
EXTERNAL __CLASSNAME
EXTERNAL __CLASSNEW
EXTERNAL __CLASSSEL
EXTERNAL __CLS_CNTCLSDATA
EXTERNAL __CLS_CNTDATA
EXTERNAL __CLS_CNTSHRDATA
EXTERNAL __CLS_DECDATA
EXTERNAL __CLS_INCDATA
EXTERNAL __CLSADDFRIEND
EXTERNAL __CLSADDMSG
EXTERNAL __CLSCNTCLASSES
EXTERNAL __CLSDELMSG
EXTERNAL __CLSGETPROPERTIES
EXTERNAL __CLSINST
EXTERNAL __CLSINSTSUPER
EXTERNAL __CLSLOCK
EXTERNAL __CLSMODMSG
EXTERNAL __CLSNEW
EXTERNAL __CLSPARENT

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

EXTERNAL __PP_INIT
EXTERNAL __PP_PATH
EXTERNAL __PP_RESET
EXTERNAL __PP_ADDRULE
EXTERNAL __PP_PROCESS
EXTERNAL __PP_STDRULES

EXTERNAL __HRBRUN
EXTERNAL __HRBLOAD
EXTERNAL __HRBDO
EXTERNAL __HRBUNLOAD
EXTERNAL __HRBGETFU
EXTERNAL __HRBDOFU

EXTERNAL __ERRINHANDLER
EXTERNAL __ERRRT_BASE
EXTERNAL __ERRRT_SBASE

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

/* The debugger interface */

EXTERNAL HB_DBG_INVOKEDEBUG
EXTERNAL HB_DBG_VMPARLLIST
EXTERNAL HB_DBG_VMSTKGCOUNT
EXTERNAL HB_DBG_VMSTKGLIST
EXTERNAL HB_DBG_VMSTKLCOUNT
EXTERNAL HB_DBG_VMSTKLLIST
EXTERNAL HB_DBG_VMVARGGET
EXTERNAL HB_DBG_VMVARGLIST
EXTERNAL HB_DBG_VMVARGSET
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
EXTERNAL DBFBLOB, DBFBLOB_GETFUNCTABLE
EXTERNAL DBFSMT

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
EXTERNAL _EINSTVAR
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
EXTERNAL __CAPLENGTH
EXTERNAL __CAPMETRICS
#ifdef HB_C52_UNDOC
EXTERNAL __MICOLUMN
EXTERNAL __MIROW
#endif
EXTERNAL _ISGRAPHIC
EXTERNAL _SETVIDEOMODE
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
EXTERNAL GUIPOSTVALIDATE
EXTERNAL GUIPREVALIDATE
EXTERNAL GUIREADER
EXTERNAL ISDEFCOLOR
EXTERNAL ISDISK
EXTERNAL LISTBOX
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
EXTERNAL ORDCOUNT
EXTERNAL ORDCUSTOM
EXTERNAL ORDFINDREC
EXTERNAL ORDKEYRELPOS
EXTERNAL POPUP
EXTERNAL PUSHBUTTON
EXTERNAL RADIOBUTTO
#ifdef HB_EXTENSION
EXTERNAL RADIOBUTTON
#endif
EXTERNAL RADIOGROUP
EXTERNAL TOPBAR

EXTERNAL TBMOUSE
EXTERNAL TAPPLYKEY
EXTERNAL TBADDCOL
EXTERNAL TBAPPLYKEY
EXTERNAL TBBBLOCK
EXTERNAL TBCLOSE
EXTERNAL TBCREATE
EXTERNAL TBDELCOL
EXTERNAL TBDISPLAY
EXTERNAL TBEDITCELL
EXTERNAL TBFBLOCK
EXTERNAL TBGOBOT
EXTERNAL TBGOTOP
EXTERNAL TBINSCOL
EXTERNAL TBMODAL
EXTERNAL TBSBLOCK
EXTERNAL TBSKIP

EXTERNAL GETCLRPAIR
EXTERNAL SETCLRPAIR
EXTERNAL GETPAIRPOS
EXTERNAL GETPAIRLEN
EXTERNAL GETCLRBACK
EXTERNAL GETCLRFORE
EXTERNAL RADGRDEFCO
EXTERNAL RADITDEFCO
EXTERNAL LISTBDEFCO
EXTERNAL COMBODEFCO
EXTERNAL CHECKDEFCO
EXTERNAL BUTTNDEFCO
EXTERNAL MENUDEFCOL
EXTERNAL APPLYDEFAU

EXTERNAL ISQUICK
EXTERNAL ISSHORTCUT
EXTERNAL ACCELERATOR
EXTERNAL HITTEST
EXTERNAL READSTATS
EXTERNAL SHOWGETMSG
EXTERNAL ERASEGETMSG
EXTERNAL _GETNUMCOL
EXTERNAL SHOWMSG

#endif /* HB_COMPAT_C53 */

/* Xbase++ compatible functions */

#ifdef HB_COMPAT_XPP

EXTERNAL BIN2U
EXTERNAL DBCOPYEXTSTRUCT
EXTERNAL DBCOPYSTRUCT
EXTERNAL DBJOIN
EXTERNAL DBLIST
EXTERNAL DBSORT
EXTERNAL DBTOTAL
EXTERNAL DBUPDATE
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
EXTERNAL _DBEXPORT
EXTERNAL _TYPEFILE

#endif /* HB_COMPAT_XPP */

/* Harbour extensions */

EXTERNAL HB_ARGC
EXTERNAL HB_ARGCHECK
EXTERNAL HB_ARGSTRING
EXTERNAL HB_ARGV
EXTERNAL HB_COLORINDEX
EXTERNAL HB_COMPILER
EXTERNAL HB_PCODEVER
EXTERNAL HB_BUILDDATE
EXTERNAL HB_FILEEXISTS
EXTERNAL HB_DIREXISTS
EXTERNAL HB_FNAMEMERGE
EXTERNAL HB_FNAMESPLIT
EXTERNAL HB_LANGNAME
EXTERNAL HB_LANGSELECT
EXTERNAL HB_LANGERRMSG
EXTERNAL HB_LANGMESSAGE
#ifndef HB_CDP_SUPPORT_OFF
EXTERNAL HB_SETCODEPAGE
EXTERNAL HB_TRANSLATE
EXTERNAL HB_CDPLIST
EXTERNAL HB_STRTOUTF8
EXTERNAL HB_UTF8TOSTR
EXTERNAL HB_UTF8LEN
EXTERNAL HB_UTF8LEFT
EXTERNAL HB_UTF8RIGHT
EXTERNAL HB_UTF8PEEK
EXTERNAL HB_UTF8POKE
EXTERNAL HB_UTF8STUFF
EXTERNAL HB_UTF8SUBSTR
EXTERNAL HB_UTF8STRTRAN
#endif
EXTERNAL HB_ISARRAY
EXTERNAL HB_ISBLOCK
EXTERNAL HB_ISCHAR
EXTERNAL HB_ISDATE
EXTERNAL HB_ISHASH
EXTERNAL HB_ISLOGICAL
EXTERNAL HB_ISMEMO
EXTERNAL HB_ISNIL
EXTERNAL HB_ISNULL
EXTERNAL HB_ISNUMERIC
EXTERNAL HB_ISOBJECT
EXTERNAL HB_ISPOINTER
EXTERNAL HB_ISSTRING
EXTERNAL HB_ISSYMBOL

EXTERNAL HB_IDLEADD
EXTERNAL HB_IDLEDEL
EXTERNAL HB_IDLERESET
EXTERNAL HB_IDLESLEEP
EXTERNAL HB_IDLESTATE
EXTERNAL HB_RELEASECPU

EXTERNAL HB_OSNEWLINE
EXTERNAL HB_APARAMS
EXTERNAL HB_PVALUE
EXTERNAL HB_FORNEXT
EXTERNAL HB_EXECFROMARRAY
EXTERNAL HB_METHODNAME
EXTERNAL HB_SETMACRO
EXTERNAL HB_STOD
EXTERNAL HB_WILDMATCH
EXTERNAL HB_CLOCKS2SECS
EXTERNAL HB_MATHERMODE
EXTERNAL HB_MATHERBLOCK
EXTERNAL HB_SERIALIZE
EXTERNAL HB_DESERIALIZE
EXTERNAL HB_HEXTONUM
EXTERNAL HB_NUMTOHEX
EXTERNAL HB_HEXTOSTR
EXTERNAL HB_STRTOHEX
EXTERNAL HB_ISPRINTER
EXTERNAL HB_GETENV

EXTERNAL HB_INISETCOMMENT
EXTERNAL HB_INIREAD
EXTERNAL HB_INIWRITE

EXTERNAL HB_LIBLOAD
EXTERNAL HB_LIBFREE
EXTERNAL HB_LIBDO
EXTERNAL HB_LIBERROR

EXTERNAL HB_RANDOM
EXTERNAL HB_RANDOMINT
EXTERNAL HB_RANDOMSEED

EXTERNAL HB_BITAND
EXTERNAL HB_BITOR
EXTERNAL HB_BITXOR
EXTERNAL HB_BITNOT
EXTERNAL HB_BITSET
EXTERNAL HB_BITRESET
EXTERNAL HB_BITTEST
EXTERNAL HB_BITSHIFT

EXTERNAL HB_ADLER32
EXTERNAL HB_CRC
EXTERNAL HB_CRC16
EXTERNAL HB_CRC32
EXTERNAL HB_CRCCT
EXTERNAL HB_MD5
EXTERNAL HB_MD5FILE

EXTERNAL HB_GTALERT
EXTERNAL HB_GTVERSION
EXTERNAL HB_GTSYS
EXTERNAL HB_GTINFO
EXTERNAL HB_SETDISPCP
EXTERNAL HB_SETKEYCP
EXTERNAL HB_SETTERMCP
EXTERNAL HB_GFXPRIMITIVE
EXTERNAL HB_GFXTEXT

EXTERNAL HB_GETSTDERR
EXTERNAL HB_GETSTDIN
EXTERNAL HB_GETSTDOUT

/* Codepage support */
EXTERNAL HB_CODEPAGE_BG866
EXTERNAL HB_CODEPAGE_BGISO
EXTERNAL HB_CODEPAGE_BGMIK
EXTERNAL HB_CODEPAGE_BGWIN
EXTERNAL HB_CODEPAGE_CS852
EXTERNAL HB_CODEPAGE_CSISO
EXTERNAL HB_CODEPAGE_CSKAM
EXTERNAL HB_CODEPAGE_CSWIN
EXTERNAL HB_CODEPAGE_DE
EXTERNAL HB_CODEPAGE_DEWIN
EXTERNAL HB_CODEPAGE_EL
EXTERNAL HB_CODEPAGE_ELWIN
EXTERNAL HB_CODEPAGE_ES
EXTERNAL HB_CODEPAGE_ESMWIN
EXTERNAL HB_CODEPAGE_ESWIN
EXTERNAL HB_CODEPAGE_FR
EXTERNAL HB_CODEPAGE_HR1250
EXTERNAL HB_CODEPAGE_HR437
EXTERNAL HB_CODEPAGE_HR852
EXTERNAL HB_CODEPAGE_HU852
EXTERNAL HB_CODEPAGE_HU852S
EXTERNAL HB_CODEPAGE_HUISO
EXTERNAL HB_CODEPAGE_HUISOS
EXTERNAL HB_CODEPAGE_HUWIN
EXTERNAL HB_CODEPAGE_HUWINS
EXTERNAL HB_CODEPAGE_IT437
EXTERNAL HB_CODEPAGE_IT850
EXTERNAL HB_CODEPAGE_ITISB
EXTERNAL HB_CODEPAGE_ITISO
EXTERNAL HB_CODEPAGE_LT
EXTERNAL HB_CODEPAGE_PL852
EXTERNAL HB_CODEPAGE_PLISO
EXTERNAL HB_CODEPAGE_PLMAZ
EXTERNAL HB_CODEPAGE_PLWIN
EXTERNAL HB_CODEPAGE_PT850
EXTERNAL HB_CODEPAGE_PTISO
EXTERNAL HB_CODEPAGE_RU1251
EXTERNAL HB_CODEPAGE_RU866
EXTERNAL HB_CODEPAGE_RUKOI8
EXTERNAL HB_CODEPAGE_SK852
EXTERNAL HB_CODEPAGE_SKISO
EXTERNAL HB_CODEPAGE_SKKAM
EXTERNAL HB_CODEPAGE_SKWIN
EXTERNAL HB_CODEPAGE_SL437
EXTERNAL HB_CODEPAGE_SL852
EXTERNAL HB_CODEPAGE_SLISO
EXTERNAL HB_CODEPAGE_SLWIN
EXTERNAL HB_CODEPAGE_SRWIN
EXTERNAL HB_CODEPAGE_SV850
EXTERNAL HB_CODEPAGE_SVCLIP
EXTERNAL HB_CODEPAGE_SVWIN
EXTERNAL HB_CODEPAGE_TR857
EXTERNAL HB_CODEPAGE_TRWIN
EXTERNAL HB_CODEPAGE_UA1251
EXTERNAL HB_CODEPAGE_UA866
EXTERNAL HB_CODEPAGE_UAKOI8

/* lang support */
EXTERNAL HB_LANG_BG866
EXTERNAL HB_LANG_BGISO
EXTERNAL HB_LANG_BGWIN
EXTERNAL HB_LANG_CA
EXTERNAL HB_LANG_CS852
EXTERNAL HB_LANG_CSISO
EXTERNAL HB_LANG_CSKAM
EXTERNAL HB_LANG_CSWIN
EXTERNAL HB_LANG_DE
EXTERNAL HB_LANG_DEWIN
EXTERNAL HB_LANG_EL
EXTERNAL HB_LANG_ELWIN
EXTERNAL HB_LANG_EN
EXTERNAL HB_LANG_EO
EXTERNAL HB_LANG_ES
EXTERNAL HB_LANG_ESWIN
EXTERNAL HB_LANG_EU
EXTERNAL HB_LANG_FR
EXTERNAL HB_LANG_GL
EXTERNAL HB_LANG_HE862
EXTERNAL HB_LANG_HEWIN
EXTERNAL HB_LANG_HR852
EXTERNAL HB_LANG_HRISO
EXTERNAL HB_LANG_HU852
EXTERNAL HB_LANG_HUCWI
EXTERNAL HB_LANG_HUISO
EXTERNAL HB_LANG_HUWIN
EXTERNAL HB_LANG_ID
EXTERNAL HB_LANG_IS850
EXTERNAL HB_LANG_IT
EXTERNAL HB_LANG_KO
EXTERNAL HB_LANG_NL
EXTERNAL HB_LANG_PL852
EXTERNAL HB_LANG_PLISO
EXTERNAL HB_LANG_PLMAZ
EXTERNAL HB_LANG_PLWIN
EXTERNAL HB_LANG_PT
EXTERNAL HB_LANG_RO
EXTERNAL HB_LANG_RU866
EXTERNAL HB_LANG_RUKOI8
EXTERNAL HB_LANG_RUWIN
EXTERNAL HB_LANG_SL437
EXTERNAL HB_LANG_SL852
EXTERNAL HB_LANG_SLISO
EXTERNAL HB_LANG_SLWIN
EXTERNAL HB_LANG_SR852
EXTERNAL HB_LANG_SRISO
EXTERNAL HB_LANG_SRWIN
EXTERNAL HB_LANG_TR857
EXTERNAL HB_LANG_TRWIN
EXTERNAL HB_LANG_ZHB5
EXTERNAL HB_LANG_ZHGB

EXTERNAL HB_AT
EXTERNAL HB_AINS
EXTERNAL HB_ADEL
EXTERNAL HB_ASCAN
EXTERNAL HB_RASCAN
EXTERNAL HB_ANSITOOEM
EXTERNAL HB_CLRAREA
EXTERNAL HB_COLORTON
EXTERNAL HB_NTOCOLOR
EXTERNAL HB_DISKSPACE
EXTERNAL HB_FCREATE
EXTERNAL HB_FSIZE
EXTERNAL HB_FEOF
EXTERNAL HB_FCOMMIT
EXTERNAL HB_FTEMPCREATE
EXTERNAL HB_GCALL
EXTERNAL HB_KEYPUT
EXTERNAL HB_OEMTOANSI
EXTERNAL HB_OSERROR
EXTERNAL HB_OSDRIVESEPARATOR
EXTERNAL HB_OSPATHDELIMITERS
EXTERNAL HB_OSPATHLISTSEPARATOR
EXTERNAL HB_OSPATHSEPARATOR
EXTERNAL HB_SETKEYARRAY
EXTERNAL HB_SETKEYCHECK
EXTERNAL HB_SETKEYGET
EXTERNAL HB_SETKEYSAVE
EXTERNAL HB_SETLASTKEY
EXTERNAL HB_SHADOW
EXTERNAL HB_TRACELEVEL
EXTERNAL HB_TRACESTATE
EXTERNAL HB_TRACESTRING
EXTERNAL HB_CSTR
EXTERNAL HB_VALTOEXP
EXTERNAL HB_VALTOSTR
EXTERNAL HB_STRTOEXP
EXTERNAL HB_TOKENCOUNT
EXTERNAL HB_TOKENGET
EXTERNAL HB_TOKENPTR
EXTERNAL HB_ATOKENS

EXTERNAL HB_HASH
EXTERNAL HB_HHASKEY
EXTERNAL HB_HPOS
EXTERNAL HB_HGET
EXTERNAL HB_HSET
EXTERNAL HB_HDEL
EXTERNAL HB_HKEYAT
EXTERNAL HB_HVALUEAT
EXTERNAL HB_HPAIRAT
EXTERNAL HB_HDELAT
EXTERNAL HB_HKEYS
EXTERNAL HB_HVALUES
EXTERNAL HB_HFILL
EXTERNAL HB_HCLONE
EXTERNAL HB_HCOPY
EXTERNAL HB_HMERGE
EXTERNAL HB_HEVAL
EXTERNAL HB_HSCAN
EXTERNAL HB_HCASEMATCH
EXTERNAL HB_HAUTOADD
EXTERNAL HB_HALLOCATE
EXTERNAL HB_HDEFAULT
EXTERNAL HB_HSETAUTOADD
EXTERNAL HB_HSETCASEMATCH

EXTERNAL HB_REGEXCOMP
EXTERNAL HB_ISREGEX
EXTERNAL HB_ATX
EXTERNAL HB_REGEX
EXTERNAL HB_REGEXHAS
EXTERNAL HB_REGEXLIKE
EXTERNAL HB_REGEXMATCH
EXTERNAL HB_REGEXSPLIT
EXTERNAL HB_REGEXATX
EXTERNAL HB_REGEXALL

#ifndef __PLATFORM__DOS
EXTERNAL HB_INETINIT
EXTERNAL HB_INETCLEANUP
EXTERNAL HB_INETCREATE
EXTERNAL HB_INETCLOSE
EXTERNAL HB_INETFD
EXTERNAL HB_INETSTATUS
EXTERNAL HB_INETERRORCODE
EXTERNAL HB_INETERRORDESC
EXTERNAL HB_INETCLEARERROR
EXTERNAL HB_INETCOUNT
EXTERNAL HB_INETADDRESS
EXTERNAL HB_INETPORT
EXTERNAL HB_INETTIMEOUT
EXTERNAL HB_INETCLEARTIMEOUT
EXTERNAL HB_INETTIMELIMIT
EXTERNAL HB_INETCLEARTIMELIMIT
EXTERNAL HB_INETPERIODCALLBACK
EXTERNAL HB_INETCLEARPERIODCALLBACK
EXTERNAL HB_INETRECV
EXTERNAL HB_INETRECVALL
EXTERNAL HB_INETRECVLINE
EXTERNAL HB_INETRECVENDBLOCK
EXTERNAL HB_INETDATAREADY
EXTERNAL HB_INETSEND
EXTERNAL HB_INETSENDALL
EXTERNAL HB_INETGETHOSTS
EXTERNAL HB_INETGETALIAS
EXTERNAL HB_INETSERVER
EXTERNAL HB_INETACCEPT
EXTERNAL HB_INETCONNECT
EXTERNAL HB_INETCONNECTIP
EXTERNAL HB_INETDGRAMBIND
EXTERNAL HB_INETDGRAM
EXTERNAL HB_INETDGRAMSEND
EXTERNAL HB_INETDGRAMRECV
EXTERNAL HB_INETCRLF
EXTERNAL HB_INETISSOCKET
#endif /* __PLATFORM__DOS */

EXTERNAL HBCLASS
EXTERNAL HBOBJECT
EXTERNAL HBSCALAR
EXTERNAL HBARRAY
EXTERNAL HBBLOCK
EXTERNAL HBCHARACTER
EXTERNAL HBDATE
EXTERNAL HBLOGICAL
EXTERNAL HBNIL
EXTERNAL HBNUMERIC

/* FlagShip extension */

#ifdef HB_COMPAT_FLAGSHIP

EXTERNAL STRPEEK
EXTERNAL STRPOKE
EXTERNAL SECONDSCPU

#endif /* HB_COMPAT_FLAGSHIP */

#endif /* HB_EXTERN_CH_ */
