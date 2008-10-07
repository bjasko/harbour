/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for Advantage Database Server RDD
 *
 * Copyright 1999 Alexander S.Kresin <alex@belacy.belgorod.su>
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

#include "hbapirdd.h"

#if defined( HB_OS_WIN_32 ) && !defined( WIN32 )
   #define WIN32
#endif
#if !defined( unix ) && ( defined( __LINUX__ ) || defined( HB_OS_LINUX ) )
   #define unix
#endif

#if defined( __WATCOMC__ ) || defined( __LCC__ )
   #define _declspec( dllexport ) __declspec( dllexport )
#endif

#include "ace.h"

/* Autodetect ACE version. */
#if   defined(ADS_NOTIFICATION_CONNECTION)
   #define _ADS_LIB_VERSION 900 /* or upper */
#elif defined(ADS_UDP_IP_CONNECTION)
   #define _ADS_LIB_VERSION 810 /* or upper */
#elif defined(ADS_REPLICATION_CONNECTION)
   #define _ADS_LIB_VERSION 800 /* or upper */
#elif defined(ADS_NOT_AUTO_OPEN)
   #define _ADS_LIB_VERSION 710 /* or upper */
#elif defined(ADS_FTS_INDEX_ORDER)
   #define _ADS_LIB_VERSION 700 /* or upper */
#elif defined(ADS_COMPRESS_ALWAYS)
   #define _ADS_LIB_VERSION 620 /* or upper */
#elif defined(ADS_USER_DEFINED)
   #define _ADS_LIB_VERSION 611 /* or upper */
#else
   #define _ADS_LIB_VERSION 500
#endif

#if 0
/* Compatibility ACE version override.
   Usage is discouraged and unnecessary unless we want to 
   override autodetection. For the latter, ADS_LIB_VERSION 
   is recommended. If ADS_LIB_VERSION is #defined,
   ADS_REQUIRE_VERSION will be ignored. [vszakats] */
#if !defined( ADS_LIB_VERSION )
   #if   ADS_REQUIRE_VERSION == 5
      #define ADS_LIB_VERSION 500
   #elif ADS_REQUIRE_VERSION == 6
      #define ADS_LIB_VERSION 600
   #elif ADS_REQUIRE_VERSION == 7
      #define ADS_LIB_VERSION 700
   #elif ADS_REQUIRE_VERSION == 8
      #define ADS_LIB_VERSION 810
   #elif ADS_REQUIRE_VERSION == 9
      #define ADS_LIB_VERSION 900
   #endif
#endif
#endif

/* Make sure to not allow a manual override requesting 
   a higher version than the one of ACE. [vszakats] */
#if !defined( ADS_LIB_VERSION )
   #define ADS_LIB_VERSION _ADS_LIB_VERSION
#elif ADS_LIB_VERSION > _ADS_LIB_VERSION
   #undef ADS_LIB_VERSION
   #define ADS_LIB_VERSION _ADS_LIB_VERSION
#endif

/* QUESTION: Why do we redefine this? Normally it is 4082 in 7.10 or upper and 256 in lower versions. [vszakats] */
#undef ADS_MAX_KEY_LENGTH
#if ADS_LIB_VERSION >= 800
   #define ADS_MAX_KEY_LENGTH   4082   /* maximum key value length.  This is the max key length */
#else                                  /* of ADI indexes.  Max CDX key length is 240.  Max */
   #define ADS_MAX_KEY_LENGTH    256   /* NTX key length is 256 */
#endif

/* TOFIX: These should rather be guarded with ADS_LIB_VERSION than being defined here. [vszakats] */
#ifndef ADS_CISTRING
   #define ADS_CISTRING             20    /* CaSe INSensiTIVE character data (>= 7.10) */
#endif
#ifndef ADS_ROWVERSION
   #define ADS_ROWVERSION           21    /* 8 byte integer, incremented for every update, unique to entire table (>= 8.00) */
#endif
#ifndef ADS_MODTIME
   #define ADS_MODTIME              22    /* 8 byte timestamp, updated when record is updated (>= 8.00) */
#endif

HB_EXTERN_BEGIN

#if ADS_LIB_VERSION >= 600
/* NOTE: Undocumented ACE function. */
UNSIGNED32 ENTRYPOINT AdsDeleteFile( ADSHANDLE hConnection, UNSIGNED8 * pucFileName );
#endif

/*
 *  ADS WORKAREA
 *  --------
 *  The Workarea Structure of Advantage Database Server RDD
 *
 */


typedef struct _ADSAREA_
{
   struct _RDDFUNCS * lprfsHost; /* Virtual method table for this workarea */
   USHORT uiArea;                /* The number assigned to this workarea */
   void * atomAlias;             /* Pointer to the alias symbol for this workarea */
   USHORT uiFieldExtent;         /* Total number of fields allocated */
   USHORT uiFieldCount;          /* Total number of fields used */
   LPFIELD lpFields;             /* Pointer to an array of fields */
   void * lpFieldExtents;        /* Void ptr for additional field properties */
   PHB_ITEM valResult;           /* All purpose result holder */
   BOOL fTop;                    /* TRUE if "top" */
   BOOL fBottom;                 /* TRUE if "bottom" */
   BOOL fBof;                    /* TRUE if "bof" */
   BOOL fEof;                    /* TRUE if "eof" */
   BOOL fFound;                  /* TRUE if "found" */
   DBSCOPEINFO dbsi;             /* Info regarding last LOCATE */
   DBFILTERINFO dbfi;            /* Filter in effect */
   LPDBORDERCONDINFO lpdbOrdCondInfo;
   LPDBRELINFO lpdbRelations;    /* Parent/Child relationships used */
   USHORT uiParents;             /* Number of parents for this area */
   USHORT heap;
   USHORT heapSize;
   USHORT rddID;
   USHORT uiMaxFieldNameLength;
   PHB_CODEPAGE cdPage;          /* Area's codepage pointer */

   /*
    *  ADS's additions to the workarea structure
    *
    *  Warning: The above section MUST match WORKAREA exactly!  Any
    *  additions to the structure MUST be added below, as in this
    *  example.
    */

   LPDBRELINFO lpdbPendingRel;   /* Pointer to parent rel struct */

   char *   szDataFileName;      /* Name of data file */
   USHORT   uiRecordLen;         /* Size of record */
   ULONG    ulRecNo;             /* Current record */
   BYTE *   pRecord;             /* Buffer of record data */
   ULONG    maxFieldLen;         /* Max field length in table record */

   BOOL     fPositioned;         /* TRUE if we are not at phantom record */
   BOOL     fShared;             /* Shared file */
   BOOL     fReadonly;           /* Read only file */
   BOOL     fFLocked;            /* TRUE if file is locked */

   int      iFileType;           /* adt/cdx/ntx */

   ADSHANDLE hTable;
   ADSHANDLE hOrdCurrent;
   ADSHANDLE hStatement;
} ADSAREA;

typedef ADSAREA * ADSAREAP;

#define SELF_RESETREL( p )    if( pArea->lpdbPendingRel ) \
                              { \
                                 if( pArea->lpdbPendingRel->isScoped && \
                                    !pArea->lpdbPendingRel->isOptimized ) \
                                    SELF_FORCEREL( ( AREAP ) pArea ); \
                                 else \
                                    pArea->lpdbPendingRel = NULL; \
                              }


#define HB_RDD_ADS_VERSION_STRING "ADS RDD 1.4"

#if defined( HB_OS_WIN_32 )
#  define ADS_USE_OEM_TRANSLATION
#else
#  undef ADS_USE_OEM_TRANSLATION
#endif

#define HB_ADS_PARCONNECTION( n )      ( ISNUM( n ) ? ( ADSHANDLE ) hb_parnl( n ) : hb_ads_hConnect )
#define HB_ADS_RETCONNECTION( h )      hb_retnl( h )
#define HB_ADS_GETCONNECTION( p )      ( ( hb_itemType( p ) & HB_IT_NUMERIC ) ? ( ADSHANDLE ) hb_itemGetNL( p ) : hb_ads_hConnect )
#define HB_ADS_PUTCONNECTION( p, h )   hb_itemPutNL( ( p ), ( LONG ) ( h ) )
#define HB_ADS_DEFCONNECTION( v )      ( ( v ) ? ( ADSHANDLE ) ( v ) : hb_ads_hConnect )


extern int hb_ads_iFileType;                 /* current global setting */
extern int hb_ads_iLockType;
extern int hb_ads_iCheckRights;
extern int hb_ads_iCharType;
extern BOOL hb_ads_bTestRecLocks;
extern ADSHANDLE hb_ads_hConnect;

extern ERRCODE hb_adsCloseCursor( ADSAREAP pArea );
extern ADSAREAP hb_adsGetWorkAreaPointer( void );

#ifdef ADS_USE_OEM_TRANSLATION
   extern BOOL hb_ads_bOEM;
   extern char * hb_adsOemToAnsi( char * pcString, ULONG ulLen );
   extern char * hb_adsAnsiToOem( char * pcString, ULONG ulLen );
   void hb_adsOemAnsiFree( char * pcString );

   /* NOTE: Undocumented ACE function. */
   UNSIGNED32 ENTRYPOINT AdsSetFieldRaw( ADSHANDLE   hObj,
                                         UNSIGNED8 * pucFldName,
                                         UNSIGNED8 * pucBuf,
                                         UNSIGNED32  ulLen );

   /* NOTE: Undocumented ACE function. */
   UNSIGNED32 ENTRYPOINT AdsGetFieldRaw( ADSHANDLE    hTbl,
                                         UNSIGNED8 *  pucFldName,
                                         UNSIGNED8 *  pucBuf,
                                         UNSIGNED32 * pulLen );

#else
#  define hb_adsOemToAnsi( s, l )     ( s )
#  define hb_adsAnsiToOem( s, l )     ( s )
#  define hb_adsOemAnsiFree( s )
#endif

HB_EXTERN_END
