/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * 
 *
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
 * Copyright 2007 Lorenzo Fiorini <lorenzo.fiorini / at / gmail.com>
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


#include "hbapi.h"
#include "hbapifs.h"
#include "hbapigt.h"
#include "hbapiitm.h"
#include "hbapirdd.h"
#include "hbapilng.h"
#include "hbapierr.h"
#include "hbdbferr.h"
#include "hbvm.h"

#define HB_FILE_BUF_SIZE      0x10000
typedef struct _HB_FILEBUF
{
   FHANDLE  hFile;
   BYTE *   pBuf;
   ULONG    ulSize;
   ULONG    ulPos;
} HB_FILEBUF;
typedef HB_FILEBUF * PHB_FILEBUF;

static void hb_flushFBuffer( PHB_FILEBUF pFileBuf )
{
   if( pFileBuf->ulPos > 0 )
   {
      hb_fsWriteLarge( pFileBuf->hFile, pFileBuf->pBuf, pFileBuf->ulPos );
      pFileBuf->ulPos = 0;
   }
}

static void hb_addToFBuffer( PHB_FILEBUF pFileBuf, char ch )
{
   if( pFileBuf->ulPos == pFileBuf->ulSize )
      hb_flushFBuffer( pFileBuf );
   pFileBuf->pBuf[ pFileBuf->ulPos++ ] = ( BYTE ) ch;
}

static void hb_addStrnToFBuffer( PHB_FILEBUF pFileBuf, char * str, ULONG ulSize )
{
   ULONG ulPos = 0;
   while( ulPos < ulSize )
   {
      if( pFileBuf->ulPos == pFileBuf->ulSize )
         hb_flushFBuffer( pFileBuf );
      pFileBuf->pBuf[ pFileBuf->ulPos++ ] = ( BYTE ) str[ ulPos++ ];
   }
}

static void hb_addStrToFBuffer( PHB_FILEBUF pFileBuf, char * szStr )
{
   while( *szStr )
   {
      if( pFileBuf->ulPos == pFileBuf->ulSize )
         hb_flushFBuffer( pFileBuf );
      pFileBuf->pBuf[ pFileBuf->ulPos++ ] = ( BYTE ) *szStr++;
   }
}

static void hb_destroyFBuffer( PHB_FILEBUF pFileBuf )
{
   hb_flushFBuffer( pFileBuf );
   if( pFileBuf->pBuf )
      hb_xfree( pFileBuf->pBuf );
   hb_xfree( pFileBuf );
}

static PHB_FILEBUF hb_createFBuffer( FHANDLE hFile, ULONG ulSize )
{
   PHB_FILEBUF pFileBuf = ( PHB_FILEBUF )hb_xgrab( sizeof( HB_FILEBUF ) );

   pFileBuf->hFile = hFile;
   pFileBuf->pBuf = ( BYTE * )hb_xgrab( ulSize );
   pFileBuf->ulSize = ulSize;
   pFileBuf->ulPos = 0;
   return pFileBuf;
}


/* Export field value into the buffer in SQL format */
static BOOL hb_exportBufSqlVar( PHB_FILEBUF pFileBuf, PHB_ITEM pValue,
                                char * szDelim, char * szEsc )
{
   switch( hb_itemType( pValue ) )
   {
      case HB_IT_STRING:
      {
         ULONG ulLen = hb_itemGetCLen( pValue ), ulCnt = 0;
         char *szVal = hb_itemGetCPtr( pValue );

         hb_addStrToFBuffer( pFileBuf, szDelim );
         while( ulLen && HB_ISSPACE( szVal[ ulLen - 1 ] ) )
            ulLen--;

         while( *szVal && ulCnt++ < ulLen )
         {
            if( *szVal == *szDelim || *szVal == *szEsc )
               hb_addToFBuffer( pFileBuf, *szEsc );
            if( ( UCHAR ) *szVal >= 32 )
               hb_addToFBuffer( pFileBuf, *szVal );
            else
            {
               /* printf( "%d %c", *szVal, *szVal ); */
            }
            szVal++;
         }
         hb_addStrToFBuffer( pFileBuf, szDelim );
         break;
      }

      case HB_IT_DATE:
      {
         char szDate[9];

         hb_addStrToFBuffer( pFileBuf, szDelim );
         hb_itemGetDS( pValue, szDate );
         if( szDate[0] == ' ' )
         {
            hb_addStrToFBuffer( pFileBuf, "0100-01-01" );
         }
         else
         {
            hb_addStrnToFBuffer( pFileBuf, &szDate[0], 4 );
            hb_addToFBuffer( pFileBuf, '-' );
            hb_addStrnToFBuffer( pFileBuf, &szDate[4], 2 );
            hb_addToFBuffer( pFileBuf, '-' );
            hb_addStrnToFBuffer( pFileBuf, &szDate[6], 2 );
         }
         hb_addStrToFBuffer( pFileBuf, szDelim );
         break;
      }

      case HB_IT_LOGICAL:
         hb_addStrToFBuffer( pFileBuf, szDelim );
         hb_addToFBuffer( pFileBuf, hb_itemGetCPtr( pValue ) ? 'Y' : 'N' );
         hb_addStrToFBuffer( pFileBuf, szDelim );
         break;

      case HB_IT_INTEGER:
      case HB_IT_LONG:
      case HB_IT_DOUBLE:
      {
         char szResult[ HB_MAX_DOUBLE_LENGTH ];
         int iSize, iWidth, iDec;

         hb_itemGetNLen( pValue, &iWidth, &iDec );
         iSize = ( iDec > 0 ? iWidth + 1 + iDec : iWidth );
         if( hb_itemStrBuf( szResult, pValue, iSize, iDec ) )
         {
            int iPos = 0;
            while( iSize && HB_ISSPACE( szResult[ iPos ] ) )
            {
               iPos++;
               iSize--;
            }
            hb_addStrnToFBuffer( pFileBuf, &szResult[ iPos ], iSize );
         }
         else
            hb_addToFBuffer( pFileBuf, '0' );
         break;
      }
      /* an "M" field or the other, might be a "V" in SixDriver */
      default:
         /* We do not want MEMO contents */
         return FALSE;
   }
   return TRUE;
}

/* Export DBF content to a SQL script file */
static ULONG hb_db2Sql( AREAP pArea, PHB_ITEM pFields, HB_LONG llNext,
                        PHB_ITEM pWhile, PHB_ITEM pFor,
                        char * szDelim, char * szSep, char * szEsc,
                        char * szTable, FHANDLE hFile,
                        BOOL fInsert, BOOL fRecno )
{
   PHB_FILEBUF pFileBuf;
   ULONG ulRecords = 0;
   USHORT uiFields = 0, ui;
   PHB_ITEM pTmp;
   BOOL fWriteSep = FALSE;
   char * szNewLine = hb_conNewLine();
   char * szInsert = NULL;
   BOOL fEof = TRUE;
   BOOL fNoFieldPassed = ( pFields == NULL || hb_arrayLen( pFields ) == 0 );

   if( SELF_FIELDCOUNT( pArea, &uiFields ) != SUCCESS )
      return 0;

   if( fInsert && szTable )
      szInsert = hb_xstrcpy( NULL, "INSERT INTO ", szTable, " VALUES ( ", NULL );

   pFileBuf = hb_createFBuffer( hFile, HB_FILE_BUF_SIZE );
   pTmp = hb_itemNew( NULL );

   while( llNext-- > 0 )
   {
      if( pWhile )
      {
         if( SELF_EVALBLOCK( pArea, pWhile ) != SUCCESS ||
             ! hb_itemGetL( pArea->valResult ) )
            break;
      }

      if( SELF_EOF( pArea, &fEof ) != SUCCESS || fEof )
         break;

      if( pFor )
      {
         if( SELF_EVALBLOCK( pArea, pWhile ) != SUCCESS )
            break;
      }
      if( !pFor || hb_itemGetL( pArea->valResult ) )
      {
         ++ulRecords;

         if( szInsert )
            hb_addStrToFBuffer( pFileBuf, szInsert );

         if( fRecno )
         {
            ULONG ulRec = ulRecords;
            char szRecno[ 13 ], * szVal;

            szVal = szRecno + sizeof( szRecno );
            *--szVal = 0;
            do
            {
               *--szVal = ( char ) ( ulRec % 10 ) + '0';
               ulRec /= 10;
            }
            while( ulRec );
            hb_addStrToFBuffer( pFileBuf, szVal );
            hb_addStrToFBuffer( pFileBuf, szSep );
         }

         if( fNoFieldPassed )
         {
            for( ui = 1; ui <= uiFields; ui ++ )
            {
               if( SELF_GETVALUE( pArea, ui, pTmp ) != SUCCESS )
                  break;
               if( fWriteSep )
                  hb_addStrToFBuffer( pFileBuf, szSep );
               fWriteSep = hb_exportBufSqlVar( pFileBuf, pTmp, szDelim, szEsc );
            }
            if( ui <= uiFields )
               break;
         }
         else
         {
            /* TODO: exporting only some fields */
         }

         if( szInsert )
            hb_addStrToFBuffer( pFileBuf, " );" );
         hb_addStrToFBuffer( pFileBuf, szNewLine );
         fWriteSep = FALSE;
      }

      if( SELF_SKIP( pArea, 1 ) != SUCCESS )
         break;

      if( ( llNext % 10000 ) == 0 )
         hb_inkeyPoll();
   }

   if( szInsert )
      hb_xfree( szInsert );
   hb_destroyFBuffer( pFileBuf );
   hb_itemRelease( pTmp );

   /* Writing EOF */
   /* hb_fsWrite( hFile, ( BYTE * ) "\x1A", 1 ); */

   return ulRecords;
}

HB_FUNC( __DBSQL )
{
   AREAP pArea = ( AREAP ) hb_rddGetCurrentWorkAreaPointer();
   if( pArea )
   {
      BOOL fExport      = hb_parl( 1 );
      char * szFileName = hb_parc( 2 );
      char * szTable    = hb_parc( 3 );
      PHB_ITEM pFields  = hb_param( 4, HB_IT_ARRAY );
      PHB_ITEM pFor     = hb_param( 5, HB_IT_BLOCK );
      PHB_ITEM pWhile   = hb_param( 6, HB_IT_BLOCK );
      PHB_ITEM pNext    = hb_param( 7, HB_IT_NUMERIC );
      PHB_ITEM pRecord  = ISNIL( 8 ) ? NULL : hb_param( 8, HB_IT_ANY );
      BOOL fRest        = pWhile != NULL || ( ISLOG( 9 ) && hb_parl( 9 ) );
      BOOL fAppend      = ISLOG( 10 ) && hb_parl( 10 );
      BOOL fInsert      = ISLOG( 11 ) && hb_parl( 11 );
      BOOL fRecno       = ISLOG( 12 ) && hb_parl( 12 );
      char * szSep      = hb_parcx( 13 );
      char * szDelim    = hb_parcx( 14 );
      char * szEsc      = hb_parcx( 15 );
      HB_LONG llNext    = HB_LONG_MAX;
      FHANDLE hFile;
      ERRCODE errCode;

      if( ! szFileName )
         hb_errRT_DBCMD( EG_ARG, EDBCMD_DBCMDBADPARAMETER, NULL, &hb_errFuncName );
      else if( fExport )   /* COPY TO SQL */
      {
         PHB_ITEM pError = NULL;
         BOOL fRetry;

         /* Try to create Dat file */
         do
         {
            hFile = hb_fsExtOpen( ( BYTE * ) szFileName, NULL,
                                  ( fAppend ? 0 : FXO_TRUNCATE ) |
                                  FO_READWRITE | FO_EXCLUSIVE |
                                  FXO_DEFAULTS | FXO_SHARELOCK,
                                  NULL, pError );
            if( hFile == F_ERROR )
            {
               if( !pError )
               {
                  pError = hb_errNew();
                  hb_errPutSeverity( pError, ES_ERROR );
                  if( fAppend )
                  {
                     hb_errPutGenCode( pError, EG_OPEN );
                     hb_errPutSubCode( pError, EDBF_OPEN_DBF );
                     hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_OPEN ) );
                  }
                  else
                  {
                     hb_errPutGenCode( pError, EG_CREATE );
                     hb_errPutSubCode( pError, EDBF_CREATE_DBF );
                     hb_errPutDescription( pError, hb_langDGetErrorDesc( EG_CREATE ) );
                  }
                  hb_errPutFileName( pError, szFileName );
                  hb_errPutFlags( pError, EF_CANRETRY | EF_CANDEFAULT );
                  hb_errPutSubSystem( pError, "DBF2SQL" );
               }
               hb_errPutOsCode( pError, hb_fsError() );
               fRetry = hb_errLaunch( pError ) == E_RETRY;
            }
            else
               fRetry = FALSE;
         }
         while( fRetry );

         if( pError )
            hb_itemRelease( pError );

         if( hFile != F_ERROR )
         {
            if( fAppend )
               hb_fsSeekLarge( hFile, 0, FS_END );

            errCode = SUCCESS;
            if( pRecord )
            {
               errCode = SELF_GOTOID( pArea, pRecord );
            }
            else if( pNext )
            {
               llNext = hb_itemGetNInt( pNext );
            }
            else if( !fRest )
            {
               errCode = SELF_GOTOP( pArea );
            }

            if( errCode == SUCCESS )
            {
               hb_retnint( hb_db2Sql( pArea, pFields, llNext, pWhile, pFor,
                                      szDelim, szSep, szEsc,
                                      szTable, hFile, fInsert, fRecno ) );
            }
            hb_fsClose( hFile );
         }
      }
      else
      {
         /* TODO: import code */
      }
   }
   else
      hb_errRT_DBCMD( EG_NOTABLE, EDBCMD_NOTABLE, NULL, &hb_errFuncName );
}
