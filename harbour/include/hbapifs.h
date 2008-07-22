/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for the Filesys API
 *
 * Copyright 1999 David G. Holm <dholm@jsd-llc.com>
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

#ifndef HB_APIFS_H_
#define HB_APIFS_H_

#include "hbapi.h"
#include "fileio.ch"

HB_EXTERN_BEGIN

#define FS_ERROR ( FHANDLE ) F_ERROR

/* File locking flags */
#define FL_LOCK       0x0000   /* Lock a region   */
#define FL_UNLOCK     0x0001   /* Unlock a region */
#define FL_MASK       0x00FF   /* Mask for lock type */

/* Extended file locking flags */
#define FLX_EXCLUSIVE 0x0000   /* Exclusive lock  */
#define FLX_SHARED    0x0100   /* Shared lock     */
#define FLX_WAIT      0x0200   /* Wait for lock until success */

/* File inheritance flags */
#define FO_INHERITED  0x0000   /* Spawned processes can inherit this file handle     */
#define FO_PRIVATE    0x0080   /* Spawned processes can not inherit this file handle */

/* Extended file open mode flags */
#define FXO_TRUNCATE  0x0100   /* Create (truncate if exists) */
#define FXO_APPEND    0x0200   /* Create (append if exists)   */
#define FXO_UNIQUE    0x0400   /* Create unique file FO_EXCL ??? */
#define FXO_FORCEEXT  0x0800   /* Force default extension     */
#define FXO_DEFAULTS  0x1000   /* Use SET command defaults    */
#define FXO_DEVICERAW 0x2000   /* Open devices in raw mode    */
/* xHarbour extension */
#define FXO_SHARELOCK 0x4000   /* emulate DOS SH_DENY* mode in POSIX OS */
#define FXO_COPYNAME  0x8000   /* copy final szPath into pFilename */

/* File attributes flags */
#define HB_FA_ALL             0x00000000

#define HB_FA_READONLY        0x00000001     /* R */
#define HB_FA_HIDDEN          0x00000002     /* H */
#define HB_FA_SYSTEM          0x00000004     /* S */
#define HB_FA_LABEL           0x00000008     /* V */
#define HB_FA_DIRECTORY       0x00000010     /* D | S_ISDIR() */
#define HB_FA_ARCHIVE         0x00000020     /* A | S_ISREG() */
#define HB_FA_DEVICE          0x00000040     /* I | S_ISBLK() */
#define HB_FA_NORMAL          0x00000080     /*   */

#define HB_FA_TEMPORARY       0x00000100     /* T | S_ISFIFO()??? */
#define HB_FA_SPARSE          0x00000200     /* P | S_ISSOCK()??? */
#define HB_FA_REPARSE         0x00000400     /* L | S_ISLNK() */
#define HB_FA_COMPRESSED      0x00000800     /* C | S_ISCHR()??? */
#define HB_FA_OFFLINE         0x00001000     /* O */
#define HB_FA_NOTINDEXED      0x00002000     /* X */
#define HB_FA_ENCRYPTED       0x00004000     /* E */
#define HB_FA_VOLCOMP         0x00008000     /* M volume supports compression. */

/* these definitions should be cleared,
 * now they only help to clean lower level code
 */
#define HB_FA_FIFO            HB_FA_TEMPORARY   /* S_ISFIFO() */
#define HB_FA_FILE            HB_FA_ARCHIVE     /* S_ISREG() */
#define HB_FA_BLKDEVICE       HB_FA_DEVICE      /* S_ISBLK() */
#define HB_FA_CHRDEVICE       HB_FA_COMPRESSED  /* S_ISCHR() */
#define HB_FA_SOCKET          HB_FA_SPARSE      /* S_ISSOCK() */
#define HB_FA_LINK            HB_FA_REPARSE     /* S_ISLNK() */

/* POSIX file permission */
#define HB_FA_SUID            0x08000000     /* set user ID on execution */
#define HB_FA_SGID            0x04000000     /* set group ID on execution */
#define HB_FA_SVTX            0x02000000     /* sticky bit */
#define HB_FA_RUSR            0x01000000     /* read by owner */
#define HB_FA_WUSR            0x00800000     /* write by owner */
#define HB_FA_XUSR            0x00400000     /* execute/search by owner */
#define HB_FA_RGRP            0x00200000     /* read by group */
#define HB_FA_WGRP            0x00100000     /* write by group */
#define HB_FA_XGRP            0x00080000     /* execute/search by group */
#define HB_FA_ROTH            0x00040000     /* read by others */
#define HB_FA_WOTH            0x00020000     /* write by others */
#define HB_FA_XOTH            0x00010000     /* execute/search by others */

#define HB_FA_UGVS            ( HB_FA_SUID | HB_FA_SGID | HB_FA_SVTX )
#define HB_FA_RWXU            ( HB_FA_RUSR | HB_FA_WUSR | HB_FA_XUSR )
#define HB_FA_RWXG            ( HB_FA_RGRP | HB_FA_WGRP | HB_FA_XGRP )
#define HB_FA_RWXO            ( HB_FA_ROTH | HB_FA_WOTH | HB_FA_XOTH )

/* macros to convert Harbour attributes to POSIX ones */
#define HB_FA_POSIX_OTH(a)    ( ( ( a ) & 0x00070000 ) >> 16 )
#define HB_FA_POSIX_GRP(a)    ( ( ( a ) & 0x00380000 ) >> 15 )
#define HB_FA_POSIX_USR(a)    ( ( ( a ) & 0x01C00000 ) >> 14 )
#define HB_FA_POSIX_SID(a)    ( ( ( a ) & 0x0E000000 ) >> 13 )

#define HB_FA_POSIX_ATTR(a)   ( HB_FA_POSIX_OTH(a) | \
                                HB_FA_POSIX_GRP(a) | \
                                HB_FA_POSIX_USR(a) | \
                                HB_FA_POSIX_SID(a) )


extern HB_EXPORT BOOL       hb_fsChDir      ( BYTE * pszDirName ); /* change working directory */
extern HB_EXPORT USHORT     hb_fsChDrv      ( BYTE nDrive ); /* change working drive */
extern HB_EXPORT void       hb_fsClose      ( FHANDLE hFileHandle ); /* close a file */
extern HB_EXPORT void       hb_fsCommit     ( FHANDLE hFileHandle ); /* commit updates of a file */
extern HB_EXPORT FHANDLE    hb_fsCreate     ( BYTE * pszFileName, ULONG ulAttr ); /* create a file */
extern HB_EXPORT FHANDLE    hb_fsCreateEx   ( BYTE * pszFilename, ULONG ulAttr, USHORT uiFlags ); /* create a file, with specific open mode */
extern HB_EXPORT FHANDLE    hb_fsCreateTemp ( const BYTE * pszDir, const BYTE * pszPrefix, ULONG ulAttr, BYTE * pszName ); /* create a temporary file from components */
extern HB_EXPORT BYTE *     hb_fsCurDir     ( USHORT uiDrive ); /* retrieve a static pointer containing current directory for specified drive */
extern HB_EXPORT USHORT     hb_fsCurDirBuff ( USHORT uiDrive, BYTE * pbyBuffer, ULONG ulLen ); /* copy current directory for given drive into a buffer */
extern HB_EXPORT BYTE       hb_fsCurDrv     ( void ); /* retrieve current drive number */
extern HB_EXPORT BOOL       hb_fsDelete     ( BYTE * pszFileName ); /* delete a file */
extern HB_EXPORT BOOL       hb_fsEof        ( FHANDLE hFileHandle ); /* determine if an open file is position at end-of-file */
extern HB_EXPORT USHORT     hb_fsError      ( void ); /* retrieve file system error */
extern HB_EXPORT USHORT     hb_fsOsError    ( void ); /* retrieve system dependant file system error */
extern HB_EXPORT BOOL       hb_fsFile       ( BYTE * pszFileName ); /* determine if a file exists */
extern HB_EXPORT BOOL       hb_fsIsDirectory( BYTE * pFilename );
extern HB_EXPORT HB_FOFFSET hb_fsFSize      ( BYTE * pszFileName, BOOL bUseDirEntry ); /* determine the size of a file */
extern HB_EXPORT FHANDLE    hb_fsExtOpen    ( BYTE * pszFileName, BYTE * pDefExt,
                                              USHORT uiFlags, BYTE * pPaths, PHB_ITEM pError ); /* open a file using default extension and a list of paths */
extern HB_EXPORT USHORT     hb_fsIsDrv      ( BYTE nDrive ); /* determine if a drive number is a valid drive */
extern HB_EXPORT BOOL       hb_fsIsDevice   ( FHANDLE hFileHandle ); /* determine if a file is attached to a device (console?) */
extern HB_EXPORT BOOL       hb_fsLock       ( FHANDLE hFileHandle, ULONG ulStart, ULONG ulLength, USHORT uiMode ); /* request a lock on a portion of a file */
extern HB_EXPORT BOOL       hb_fsLockLarge  ( FHANDLE hFileHandle, HB_FOFFSET ulStart,
                                              HB_FOFFSET ulLength, USHORT uiMode ); /* request a lock on a portion of a file using 64bit API */
extern HB_EXPORT BOOL       hb_fsMkDir      ( BYTE * pszDirName ); /* create a directory */
extern HB_EXPORT FHANDLE    hb_fsOpen       ( BYTE * pszFileName, USHORT uiFlags ); /* open a file */
extern HB_EXPORT USHORT     hb_fsRead       ( FHANDLE hFileHandle, BYTE * pBuff, USHORT ulCount ); /* read contents of a file into a buffer (<=64K) */
extern HB_EXPORT ULONG      hb_fsReadLarge  ( FHANDLE hFileHandle, BYTE * pBuff, ULONG ulCount ); /* read contents of a file into a buffer (>64K) */
extern HB_EXPORT BOOL       hb_fsRmDir      ( BYTE * pszDirName ); /* remove a directory */
extern HB_EXPORT BOOL       hb_fsRename     ( BYTE * pszOldName, BYTE * pszNewName ); /* rename a file */
extern HB_EXPORT ULONG      hb_fsSeek       ( FHANDLE hFileHandle, LONG lOffset, USHORT uiMode ); /* reposition an open file */
extern HB_EXPORT HB_FOFFSET hb_fsSeekLarge  ( FHANDLE hFileHandle, HB_FOFFSET llOffset, USHORT uiFlags ); /* reposition an open file using 64bit API */
extern HB_EXPORT ULONG      hb_fsTell       ( FHANDLE hFileHandle ); /* retrieve the current position of a file */
extern HB_EXPORT BOOL       hb_fsSetDevMode ( FHANDLE hFileHandle, USHORT uiDevMode ); /* change the device mode of a file (text/binary) */
extern HB_EXPORT BOOL       hb_fsGetFileTime( BYTE * pszFileName, LONG * plJulian, LONG * plMillisec ); /* TODO */
extern HB_EXPORT BOOL       hb_fsSetFileTime( BYTE * pszFileName, LONG lJulian, LONG lMillisec );
extern HB_EXPORT BOOL       hb_fsGetAttr    ( BYTE * pszFileName, ULONG * pulAttr ); /* TODO */
extern HB_EXPORT BOOL       hb_fsSetAttr    ( BYTE * pszFileName, ULONG ulAttr );
extern HB_EXPORT void       hb_fsSetError   ( USHORT uiError ); /* set the file system DOS error number */
extern HB_EXPORT void       hb_fsSetIOError ( BOOL fResult, USHORT uiOperation ); /* set the file system error number after IO operation */
extern HB_EXPORT USHORT     hb_fsWrite      ( FHANDLE hFileHandle, const BYTE * pBuff, USHORT ulCount ); /* write to an open file from a buffer (<=64K) */
extern HB_EXPORT ULONG      hb_fsWriteLarge ( FHANDLE hFileHandle, const BYTE * pBuff, ULONG ulCount ); /* write to an open file from a buffer (>64K) */
extern HB_EXPORT FHANDLE    hb_fsPOpen      ( BYTE * pFilename, BYTE * pMode );
extern HB_EXPORT FHANDLE    hb_fsGetOsHandle( FHANDLE hFileHandle );
extern HB_EXPORT USHORT     hb_fsGetFError  ( void ); /* get FERROR() flag */
extern HB_EXPORT void       hb_fsSetFError  ( USHORT uiError ); /* set FERROR() flag */
extern HB_EXPORT BOOL       hb_fsFileExists ( const char * pszFileName ); /* check if a file exists (wildcard chars not accepted). */
extern HB_EXPORT BOOL       hb_fsDirExists  ( const char * pszDirName ); /* check if a directory exists (wildcard chars not accepted). */

#define hb_fsFLock( h, s, l )   hb_fsLock( h, s, l, FL_LOCK )
#define hb_fsFUnlock( h, s, l ) hb_fsLock( h, s, l, FL_UNLOCK )

#if defined( OS_UNIX_COMPATIBLE ) && !defined( HB_USE_SHARELOCKS_OFF )
#  define HB_USE_SHARELOCKS
#  define HB_SHARELOCK_POS          0x7fffffffUL
#  define HB_SHARELOCK_SIZE         0x1UL
#  if defined( HB_USE_BSDLOCKS_OFF )
#     undef HB_USE_BSDLOCKS
#  elif defined( HB_OS_LINUX ) && \
        !defined( __WATCOMC__ ) && !defined( HB_USE_BSDLOCKS )
      /* default usage of BSD locks in *BSD systems for emulating
       * DOS/Windows DENY_* flags has been disabled because tests
       * on FreeBSD 6.2 and MacOSX shows that this implementation
       * can create self deadlock when used simultaneously with
       * POSIX locks - thanks to Phil and Lorenzo for locating the
       * problem and tests [druzus]
       */
#     define HB_USE_BSDLOCKS
#  endif
#endif

#define HB_MAX_DRIVE_LENGTH   10
#define HB_MAX_FILE_EXT       10

/* Filename support */
typedef struct
{
   char * szPath;
   char * szName;
   char * szExtension;
   char * szDrive;
   char   szBuffer[ _POSIX_PATH_MAX + HB_MAX_DRIVE_LENGTH + 4 ];
} HB_FNAME, * PHB_FNAME, * HB_FNAME_PTR;

extern HB_EXPORT PHB_FNAME hb_fsFNameSplit( const char * pszFileName ); /* Split given filename into path, name and extension */
extern HB_EXPORT char * hb_fsFNameMerge( char * pszFileName, PHB_FNAME pFileName ); /* This function joins path, name and extension into a string with a filename */

/* Searchable path support */
typedef struct _HB_PATHNAMES
{
   char * szPath;
   struct _HB_PATHNAMES * pNext;
   BOOL   fFree;
} HB_PATHNAMES;

extern HB_EXPORT void    hb_fsAddSearchPath( const char * szPath, HB_PATHNAMES ** pSearchList );
extern HB_EXPORT void    hb_fsFreeSearchPath( HB_PATHNAMES * pSearchList );
       
extern HB_EXPORT BOOL    hb_spFile( BYTE * pFilename, BYTE * pRetPath );
extern HB_EXPORT FHANDLE hb_spOpen( BYTE * pFilename, USHORT uiFlags );
extern HB_EXPORT FHANDLE hb_spCreate( BYTE * pFilename, ULONG ulAttr );
extern HB_EXPORT FHANDLE hb_spCreateEx( BYTE * pFilename, ULONG ulAttr, USHORT uiFlags );

/* File Find API structure */
typedef struct
{
   char        szName[ _POSIX_PATH_MAX + 1 ];
   LONG        lDate;
   char        szDate[ 9 ]; /* in YYYYMMDD format */
   char        szTime[ 9 ]; /* in HH:MM:SS format */
   ULONG       attr;
   HB_FOFFSET  size;

   /* Private */

   const char * pszFileMask;
   ULONG  attrmask;
   BOOL   bFirst;

   void * info; /* Pointer to the platform specific find info */

} HB_FFIND, * PHB_FFIND;

/* File Find API functions */
extern HB_EXPORT PHB_FFIND hb_fsFindFirst( const char * pszFileName, ULONG ulAttrMask );
extern HB_EXPORT BOOL      hb_fsFindNext( PHB_FFIND ffind );
extern HB_EXPORT void      hb_fsFindClose( PHB_FFIND ffind );

/* Misc helper functions */
extern ULONG     hb_fsAttrFromRaw( ULONG raw_attr );
extern ULONG     hb_fsAttrToRaw( ULONG  ulAttr );
extern ULONG     hb_fsAttrEncode( const char * szAttr );
extern char *    hb_fsAttrDecode( ULONG  ulAttr, char * szAttr );
extern HB_EXPORT BYTE * hb_fsNameConv( BYTE * szFileName, BOOL * pfFree );
extern HB_EXPORT BYTE * hb_fileNameConv( char *str );
extern HB_EXPORT BOOL   hb_fsMaxFilesError( void );

/* wrapper to fopen() which calls hb_fsNameConv() */
extern FILE * hb_fopen( const char *path, const char *mode );

HB_EXTERN_END

#endif /* HB_APIFS_H_ */
