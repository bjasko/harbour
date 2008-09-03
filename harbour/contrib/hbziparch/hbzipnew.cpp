/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * ZipArchive interface for Harbour
 *
 * Copyright 2000-2001 Luiz Rafael Culik <culik@sl.conex.net>
 * Copyright 1999 Leslee Griffith <les.griffith@vantagesystems.ca> (hb_fsDirectory())
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2,  or ( at your option )
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not,  write to
 * the Free Software Foundation,  Inc.,  59 Temple Place,  Suite 330,
 * Boston,  MA 02111-1307 USA ( or visit the web site http://www.gnu.org/ ).
 *
 * As a special exception,  the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that,  if you link the Harbour libraries with other
 * files to produce an executable,  this does not by itself cause the
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
 * Harbour,  as the General Public License permits,  the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files,  you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour,  it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that,  delete this exception notice.
 *
 */

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapifs.h"

#include "directry.ch"

/* -------------------------------------------------------------------------*/

#include "hbdate.h"
#include "hbvm.h"
#include "hbzlib.h"

#include <time.h>

#ifdef HB_EXTERN_C
   #include "ZipArchive.h"
#endif

#define HBZA_FI_FILENAME   1
#define HBZA_FI_LENGTH     2
#define HBZA_FI_METHOD     3
#define HBZA_FI_SIZE       4
#define HBZA_FI_RATIO      5
#define HBZA_FI_DATE       6
#define HBZA_FI_TIME       7
#define HBZA_FI_CRC32      8
#define HBZA_FI_ATTR       9

#define FILE_ATTRIBUTE_READONLY             0x00000001
#define FILE_ATTRIBUTE_HIDDEN               0x00000002
#define FILE_ATTRIBUTE_SYSTEM               0x00000004
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020

typedef struct
{
   int      iWrite;
   int      iExtract;
   int      iRead;
   char *   pszComment;
   int      iReadOnly;
   PHB_ITEM pChangeDiskBlock;
   PHB_ITEM pProgressBlock;
} HB_ZA_SETTINGS;

static HB_ZA_SETTINGS s_hbzaSettings = { 0 };

class CHBZipSegmCallback : public CZipSegmCallback
{
   bool Callback( ZIP_SIZE_TYPE iProgress )
   {
      PHB_ITEM pDisk = hb_itemPutNL( NULL, m_uVolumeNeeded );
      HB_SYMBOL_UNUSED( iProgress );
      hb_vmEvalBlockV( s_hbzaSettings.pChangeDiskBlock, 1, pDisk );
      hb_itemRelease( pDisk );

      return true;
   }
};

class CHBZipActionCallback : public CZipActionCallback
{
   bool Callback( ZIP_SIZE_TYPE iProgress )
   {
      PHB_ITEM pDisk = hb_itemPutNL( NULL, m_uProcessed );
      PHB_ITEM pTotal = hb_itemPutNL( NULL, m_uTotalToProcess );
      HB_SYMBOL_UNUSED( iProgress );
      hb_vmEvalBlockV( s_hbzaSettings.pProgressBlock, 2, pDisk, pTotal );
      hb_itemRelease( pDisk );
      hb_itemRelease( pTotal );

      return true;
   }
};

static char * hb_FNAddZipExt( const char * szFile )
{
   PHB_FNAME pZipFileName = hb_fsFNameSplit( szFile );
   char * pszZipFileName = ( char * ) hb_xgrab( _POSIX_PATH_MAX + 1 );

   if( ! pZipFileName->szExtension )
      pZipFileName->szExtension = ".zip";

   hb_fsFNameMerge( pszZipFileName, pZipFileName );
   hb_xfree( pZipFileName );

   return pszZipFileName;
}

static int hb_CheckSpanMode( char * szFile )
{
   CZipArchive myzip;
   int iReturn = 0;

   CHBZipSegmCallback cb_Segm;
   myzip.SetSegmCallback( &cb_Segm );

   try
   {
      myzip.Open( szFile, s_hbzaSettings.iReadOnly ? CZipArchive::zipOpenReadOnly : CZipArchive::zipOpen, 0 ); /* s.r. to avoid GPF when ZIP file is read only ! */
   }
   catch( CZipException &e )
   {
      switch( e.m_iCause )
      {
      case CZipException::genericError  : iReturn = 100; break;
      case CZipException::badZipFile    : iReturn = 101; break;
      case CZipException::badCrc        : iReturn = 102; break;
      case CZipException::noCallback    : iReturn = 103; break;
      case CZipException::aborted       : iReturn = 104; break;
      case CZipException::abortedAction : iReturn = 105; break;
      case CZipException::abortedSafely : iReturn = 106; break;
      case CZipException::nonRemovable  : iReturn = 107; break;
      case CZipException::tooManyVolumes: iReturn = 108; break;
      case CZipException::tooLongData   : iReturn = 109; break;
      case CZipException::badPassword   : iReturn = 110; break;
      case CZipException::dirWithSize   : iReturn = 111; break;
      case CZipException::internalError : iReturn = 112; break;
      case CZipException::notRemoved    : iReturn = 113; break;
      case CZipException::notRenamed    : iReturn = 114; break;
      case CZipException::platfNotSupp  : iReturn = 115; break;
      case CZipException::cdirNotFound  : iReturn = 116; break;
      }
   }

   if( iReturn == 0 )
   {
      iReturn = myzip.GetSegmMode();
      myzip.Close();
   }
   else
      myzip.Close( true );

   return iReturn;
}

static BOOL hb_zipopenread( CZipArchive * myzip, CHBZipSegmCallback * cb_Segm, char * szFile )
{
   try
   {
      switch( hb_CheckSpanMode( szFile ) )
      {
         case 0:
            myzip->Open( szFile, s_hbzaSettings.iReadOnly ? CZipArchive::zipOpenReadOnly : CZipArchive::zipOpen, 0 );
            break;

         case -1:
         {
            myzip->SetSegmCallback( cb_Segm );
            myzip->Open( szFile, s_hbzaSettings.iReadOnly ? CZipArchive::zipOpenReadOnly : CZipArchive::zipOpen, 0 );
            break;
         }
         case -2:
            myzip->Open( szFile, s_hbzaSettings.iReadOnly ? CZipArchive::zipOpenReadOnly : CZipArchive::zipOpen, 1 );
            break;

         default:
            return FALSE;
      }
   }
   catch( ... )
   {}

   return TRUE;
}

static BOOL hb_IsPassWord( char * szFile )
{
   CZipArchive myzip;
   CHBZipSegmCallback cb_Segm;
   BOOL bReturn = hb_zipopenread( &myzip, &cb_Segm, szFile );

   if( bReturn )
   {
      CZipFileHeader fh;

      myzip.GetFileInfo( fh, 0 );

      bReturn = fh.IsEncrypted();

      myzip.Close();
   }

   return bReturn;
}

static void hb_CallbackFuncFree( void * cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   if( s_hbzaSettings.pChangeDiskBlock )
   {
      hb_itemRelease( s_hbzaSettings.pChangeDiskBlock );
      s_hbzaSettings.pChangeDiskBlock = NULL;
   }
}

static BOOL hb_SetCallbackFunc( PHB_ITEM pChangeDiskBlock )
{
   if( s_hbzaSettings.pChangeDiskBlock )
      hb_itemClear( s_hbzaSettings.pChangeDiskBlock );
   else
   {
      /* register cleanup function, it's executed only once */
      hb_vmAtExit( hb_CallbackFuncFree, NULL );
      s_hbzaSettings.pChangeDiskBlock = hb_itemNew( NULL );
   }

   if( pChangeDiskBlock )
      hb_itemCopy( s_hbzaSettings.pChangeDiskBlock, pChangeDiskBlock );

   return TRUE;
}

static void hb_SetZipBuff( int a, int b, int c )
{
   s_hbzaSettings.iWrite   = HB_MIN( a, 65535 );
   s_hbzaSettings.iExtract = HB_MIN( b, 16384 );
   s_hbzaSettings.iRead    = HB_MIN( c, 32768 );
}

static void hb_SetZipReadOnly( int iRead )
{
   s_hbzaSettings.iReadOnly = iRead;
}

static void hb_SetZipComment( char * szComment )
{
   s_hbzaSettings.pszComment = hb_strdup( szComment );
}

static char * hb_GetZipComment( char * szFile )
{
   CZipArchive myzip;
   CHBZipSegmCallback cb_Segm;
   char * szTemp;

   szTemp = hb_strdup( hb_zipopenread( &myzip, &cb_Segm, szFile ) ? ( const char * ) myzip.GetGlobalComment() : "" );

   myzip.Close();

   return szTemp;
}

static PHB_ITEM hb_GetFileNamesFromZip( char * szFile, BOOL bVerbose )
{
   CZipArchive myzip;
   CHBZipSegmCallback cb_Segm;
   BOOL bReturn = hb_zipopenread( &myzip, &cb_Segm, szFile );
   PHB_ITEM pZipArray;

   if( bReturn )
   {
      ZIP_INDEX_TYPE nZipFCount = myzip.GetCount();
      ZIP_INDEX_TYPE nZipFPos;

      if( s_hbzaSettings.iWrite > 0 )
         myzip.SetAdvanced( s_hbzaSettings.iWrite, s_hbzaSettings.iExtract, s_hbzaSettings.iRead );

      pZipArray = hb_itemArrayNew( nZipFCount );

      for( nZipFPos = 0; nZipFPos < nZipFCount; nZipFPos++ )
      {
         CZipFileHeader fh;

         myzip.GetFileInfo( fh, nZipFPos );

         if( bVerbose )
         {
            PHB_ITEM pTempArray = hb_itemArrayNew( 9 );
            char szAttr[ 5 ];
            char szTime[ 9 ];
            char * szMethod = "Unknown";
            char szCRC[ 9 ];
            int iRatio;

            hb_arraySetC( pTempArray, HBZA_FI_FILENAME, ( char * ) ( LPCTSTR ) fh.GetFileName() );

            if( fh.m_uUncomprSize > 0 )
            {
               iRatio = 100 - ( ( fh.m_uComprSize * 100 ) / fh.m_uUncomprSize );

               if( iRatio < 0 )
                  iRatio = 0;
            }
            else
               iRatio = 0;

            hb_arraySetNL( pTempArray, HBZA_FI_LENGTH , fh.m_uUncomprSize );
            hb_arraySetNL( pTempArray, HBZA_FI_SIZE   , fh.m_uComprSize );
            hb_arraySetNL( pTempArray, HBZA_FI_RATIO  , iRatio );

            {
               DWORD uAttr = fh.GetSystemAttr();
               szAttr[ 0 ] = uAttr & FILE_ATTRIBUTE_READONLY  ? 'r' : '-';
               szAttr[ 1 ] = uAttr & FILE_ATTRIBUTE_HIDDEN    ? 'h' : '-';
               szAttr[ 2 ] = uAttr & FILE_ATTRIBUTE_SYSTEM    ? 's' : 'w';
               szAttr[ 3 ] = uAttr & FILE_ATTRIBUTE_DIRECTORY ? 'D' : uAttr & FILE_ATTRIBUTE_ARCHIVE ? 'a' : '-';

               if( fh.m_uMethod == 0 || uAttr & FILE_ATTRIBUTE_DIRECTORY )
                  szMethod = "Stored";
            }

            szAttr[ 4 ] = fh.IsEncrypted() ? '*' : ' ';

            if( fh.m_uMethod == Z_DEFLATED )
            {
               UINT iLevel = ( UINT ) ( ( fh.m_uFlag & 0x6 ) / 2 );

               switch( iLevel )
               {
                  case 0:
                     szMethod = "DeflatN";
                     break;

                  case 1:
                     szMethod = "DeflatX";
                     break;

                  case 2:
                  case 3:
                     szMethod = "DeflatF";
                     break;
               }
            }

            hb_arraySetC( pTempArray, HBZA_FI_METHOD, szMethod );

            snprintf( szCRC, sizeof( szCRC ), "%8.8lx", ( ULONG ) fh.m_uCrc32 );

            hb_arraySetCL( pTempArray, HBZA_FI_CRC32, szCRC, 8 );
            hb_arraySetDL( pTempArray, HBZA_FI_DATE, hb_dateEncode( ( LONG ) ( fh.m_uModDate >> 9 ) + 1980, ( LONG ) ( ( fh.m_uModDate & ~0xFE00 ) >> 5 ), ( LONG ) fh.m_uModDate & ~0xFFE0 ) );

            {
               time_t theTime = fh.GetTime();
               tm * SzTime = localtime( &theTime );

               snprintf( szTime, sizeof( szTime ), "%02d:%02d:%02d", SzTime->tm_hour, SzTime->tm_min, SzTime->tm_sec );
            }

            hb_arraySetCL( pTempArray, HBZA_FI_TIME, szTime, 8 );
            hb_arraySetCL( pTempArray, HBZA_FI_ATTR, szAttr, 5 );
            hb_arraySetForward( pZipArray, ( ULONG ) nZipFPos + 1, pTempArray );
            hb_itemRelease( pTempArray );
         }
         else
            hb_arraySetC( pZipArray, ( ULONG ) nZipFPos + 1, ( char * ) ( LPCTSTR ) fh.GetFileName() );
      }

      myzip.Close();
   }
   else
      pZipArray = hb_itemArrayNew( 0 );

   return pZipArray;
}

static ULONG hb_GetNumberofFilestoUnzip( char * szFile )
{
   CZipArchive myzip;
   ZIP_INDEX_TYPE nZipFCount = 0;

   CHBZipSegmCallback cb_Segm;
   myzip.SetSegmCallback( &cb_Segm );

   try
   {
      myzip.Open( szFile, s_hbzaSettings.iReadOnly ? CZipArchive::zipOpenReadOnly : CZipArchive::zipOpen, 0 );
      nZipFCount = myzip.GetCount();
      myzip.Close();
   }
   catch( CZipException &e )
   {
      if( e.m_iCause == CZipException::badZipFile || e.m_iCause == CZipException::cdirNotFound )
         nZipFCount = ( ZIP_INDEX_TYPE ) -1;
   }

   return ( ULONG ) nZipFCount;
}

static BOOL hb_DeleteSel( char * szFile, PHB_ITEM pArray )
{
   CZipArchive myzip;
   BOOL bReturn = TRUE;

   try
   {
      switch( hb_CheckSpanMode( szFile ) )
      {
         case 0:
            myzip.Open( szFile, s_hbzaSettings.iReadOnly ? CZipArchive::zipOpenReadOnly : CZipArchive::zipOpen, 0 );
            break;

         case -1:
         case -2:
/*       default: */
            bReturn = FALSE;
      }
   }
   catch( CZipException )
   {}

   if( bReturn )
   {
      CZipStringArray aFiles;
      ULONG nPos;

      for( nPos = 1; nPos <= hb_arrayLen( pArray ); nPos ++ )
         aFiles.Add( hb_arrayGetCPtr( pArray, nPos ) );

      myzip.RemoveFiles( aFiles );

      myzip.Close();
   }

   return bReturn;
}

static BOOL hb_Unzip( char * szFile, PHB_ITEM pBlock, BOOL bWithPath, char * szPassword, const char * szPath, PHB_ITEM pProcFiles, PHB_ITEM pProgress )
{
   CZipArchive myzip;
   CHBZipSegmCallback cb_Segm;
   BOOL bReturn = hb_zipopenread( &myzip, &cb_Segm, szFile );

   if( bReturn )
   {
      CHBZipActionCallback cb_Action;
      char * pszPath = ( char * ) hb_xgrab( _POSIX_PATH_MAX + 1 );
      ZIP_INDEX_TYPE nZipFPos;

      if( szPassword )
         myzip.SetPassword( szPassword );

      if( pProgress && HB_IS_BLOCK( pProgress ) )
      {
         s_hbzaSettings.pProgressBlock = pProgress;
         myzip.SetCallback( &cb_Action );
      }

      hb_strncpy( pszPath, szPath && strcmp( szPath, ".\\" ) ? szPath : "", _POSIX_PATH_MAX );

      myzip.SetRootPath( pszPath );

      if( pProcFiles )
      {
         ULONG nPos;

         for( nPos = 1; nPos <= hb_arrayLen( pProcFiles ); nPos++ )
         {
            nZipFPos = myzip.FindFile( ( LPCTSTR ) hb_arrayGetCPtr( pProcFiles, nPos ), false );

            if( nZipFPos == ( ZIP_INDEX_TYPE ) -1 )
               nZipFPos = myzip.FindFile( ( LPCTSTR ) hb_arrayGetCPtr( pProcFiles, nPos ), true );

            if( nZipFPos != ( ZIP_INDEX_TYPE ) -1 )
            {
               CZipFileHeader fh;
               myzip.GetFileInfo( fh, nZipFPos );

               if( pBlock && HB_IS_BLOCK( pBlock ) )
               {
                  PHB_ITEM pFileName = hb_itemPutC( NULL, ( char * ) ( LPCTSTR ) fh.GetFileName() );
                  PHB_ITEM pFilePos = hb_itemPutNL( NULL, nPos );
                  hb_vmEvalBlockV( pBlock, 2, pFileName, pFilePos );
                  hb_itemRelease( pFileName );
                  hb_itemRelease( pFilePos );
               }

               try
               {
                  myzip.ExtractFile( nZipFPos, ( LPCTSTR ) pszPath, bWithPath ? true : false, NULL, 65536 );
               }
               catch( ... )
               {
                  bReturn = FALSE;
                  myzip.CloseFile( NULL, true );
               }
            }
         }
      }
      else
      {
         for( nZipFPos = 0; nZipFPos < myzip.GetCount(); nZipFPos++ )
         {
            CZipFileHeader fh;
            myzip.GetFileInfo( fh, nZipFPos );

            if( pBlock && HB_IS_BLOCK( pBlock ) )
            {
               PHB_ITEM pFileName = hb_itemPutC( NULL, ( char * ) ( LPCTSTR ) fh.GetFileName() );
               PHB_ITEM pFilePos = hb_itemPutNL( NULL, ( LONG ) nZipFPos );
               hb_vmEvalBlockV( pBlock, 2, pFileName, pFilePos );
               hb_itemRelease( pFileName );
               hb_itemRelease( pFilePos );
            }

            try
            {
               myzip.ExtractFile( nZipFPos, ( LPCTSTR ) pszPath, bWithPath ? true : false, NULL, 65536 );
            }
            catch( ... )
            {
               bReturn = FALSE;
               myzip.CloseFile( NULL, true );
            }
         }
      }

      hb_xfree( pszPath );

      myzip.Close();
   }

   return bReturn;
}

static BOOL hb_TransferFilesFromzip( char * szSource, char * szDest, PHB_ITEM pArray )
{
   CZipArchive myzipSource;
   CZipArchive myzipDest;
   CZipStringArray aFiles;
   BOOL bReturn1 = TRUE;
   BOOL bReturn2 = TRUE;

   try
   {
      switch( hb_CheckSpanMode( szSource ) )
      {
         case 0:
            myzipSource.Open( szSource, s_hbzaSettings.iReadOnly ? CZipArchive::zipOpenReadOnly : CZipArchive::zipOpen, 0 );
            break;

         case -1:
         case -2:
/*       default: */
            bReturn1 = FALSE;
      }
   }
   catch( CZipException )
   {}

   try
   {
      switch( hb_CheckSpanMode( szDest ) )
      {
         case 0:
            myzipDest.Open( szDest, s_hbzaSettings.iReadOnly ? CZipArchive::zipOpenReadOnly : CZipArchive::zipOpen, 0 );
            break;

         case -1:
         case -2:
/*       default: */
            bReturn2 = FALSE;
      }
   }
   catch( CZipException )
   {}

   if( bReturn1 && bReturn2 )
   {
      ULONG nPos;

      for( nPos = 1; nPos <= hb_arrayLen( pArray ); nPos++ )
         aFiles.Add( hb_arrayGetCPtr( pArray, nPos ) );

      myzipDest.GetFromArchive( myzipSource, aFiles, false );

      myzipDest.Close();
      myzipSource.Close();

      return TRUE;
   }

   return FALSE;
}

static BOOL hb_CompressFile( char * szFile, PHB_ITEM pProcFiles, int iCompLevel, PHB_ITEM pBlock, BOOL bOverwrite, char * szPassword, long iSpanSize, BOOL bPath, BOOL bDrive, PHB_ITEM pProgress, BOOL bSpan )
{
   CZipArchive myzip;
   BOOL bReturn = TRUE;
   char * pszZipFileName = hb_FNAddZipExt( szFile );

   CHBZipSegmCallback cb_Segm;
   myzip.SetSegmCallback( &cb_Segm );

   try
   {
      BOOL bFileExist = hb_fsFileExists( pszZipFileName );

      if( ( bFileExist && bOverwrite ) || !bFileExist )
         myzip.Open( pszZipFileName, bSpan ? CZipArchive::zipCreateSegm : CZipArchive::zipCreate, bSpan ? iSpanSize : 0 );
      else if( ! bSpan )
         myzip.Open( pszZipFileName, CZipArchive::zipOpen, 0 );
      else
         bReturn = FALSE;
   }
   catch( CZipException )
   {
      bReturn = FALSE;
   }
   catch( ... )
   {}

   if( bReturn )
   {
      CHBZipActionCallback cb_Action;
      ULONG nPos;

      #ifndef HB_OS_UNIX_COMPATIBLE
         hb_strLower( pszZipFileName, strlen( pszZipFileName ) );
      #endif

      if( szPassword )
         myzip.SetPassword( szPassword );

      if( s_hbzaSettings.pszComment )
      {
         myzip.SetGlobalComment( s_hbzaSettings.pszComment );
         hb_xfree( s_hbzaSettings.pszComment );
      }

      if( pProgress && HB_IS_BLOCK( pProgress ) )
      {
         s_hbzaSettings.pProgressBlock = pProgress;
         myzip.SetCallback( &cb_Action );
      }

      for( nPos = 1; nPos <= hb_arrayLen( pProcFiles ); nPos++ )
      {
         const char * szDummy = hb_arrayGetCPtr( pProcFiles, nPos );
         char * pszDummyLower = hb_strdup( szDummy );

         #ifndef HB_OS_UNIX_COMPATIBLE
            hb_strLower( pszDummyLower, strlen( pszDummyLower ) );
         #endif

         /* Prevent adding current archive file. */
         /* TOFIX: strstr() is not suitable for portable filename comparison. */
         if( ! strstr( pszZipFileName, pszDummyLower ) &&
             ! strstr( pszDummyLower, pszZipFileName ) )
         {
            if( hb_fsFileExists( szDummy ) )
            {
               if( pBlock && HB_IS_BLOCK( pBlock ) )
               {
                  PHB_ITEM pFileName = hb_itemPutC( NULL, szDummy );
                  PHB_ITEM pFilePos = hb_itemPutNI( NULL, nPos );
                  hb_vmEvalBlockV( pBlock, 2, pFileName, pFilePos );
                  hb_itemRelease( pFileName );
                  hb_itemRelease( pFilePos );
               }

               try
               {
                  myzip.AddNewFile( szDummy, iCompLevel, ( bDrive || bPath ) ? true : false, CZipArchive::zipsmSafeSmart, 65536 );
               }
               catch( ... )
               {}
            }
         }

         hb_xfree( pszDummyLower );
      }
   }

   hb_xfree( pszZipFileName );

   try
   {
      myzip.Close();
   }
   catch( CZipException )
   {
      bReturn = FALSE;
   }
   catch( ... )
   {}

   if( bReturn )
   {
      ULONG nPos, nLen = hb_arrayLen( pProcFiles );

      for( nPos = 0; nPos < nLen; nPos++ )
      {
         ULONG nAttr;

         hb_fsGetAttr( ( BYTE * ) hb_arrayGetCPtr( pProcFiles, nPos + 1 ), &nAttr );
         /* TOFIX: Doesn't work for some reason.
                   May be a problem or property of hb_fsSetAttr(). */
         nAttr &= ~HB_FA_ARCHIVE;
         hb_fsSetAttr( ( BYTE * ) hb_arrayGetCPtr( pProcFiles, nPos + 1 ), nAttr );
      }
   }

   return bReturn;
}

/* -------------------------------------------------------------------------*/

static void hb_fsGrabDirectory( PHB_ITEM pDir, const char * szDirSpec, USHORT uiMask, PHB_FNAME fDirSpec, BOOL bFullPath, BOOL bDirOnly )
{
   PHB_FFIND ffind;

   /* Get the file list */
   if( ( ffind = hb_fsFindFirst( szDirSpec, uiMask ) ) != NULL )
   {
      PHB_ITEM pSubarray;

      pSubarray = hb_itemNew( NULL );

      do
      {
         if( ( !bDirOnly || ( ffind->attr & HB_FA_DIRECTORY ) != 0 ) &&
             !( ( ( uiMask & HB_FA_HIDDEN    ) == 0 && ( ffind->attr & HB_FA_HIDDEN    ) != 0 ) ||
                ( ( uiMask & HB_FA_SYSTEM    ) == 0 && ( ffind->attr & HB_FA_SYSTEM    ) != 0 ) ||
                ( ( uiMask & HB_FA_LABEL     ) == 0 && ( ffind->attr & HB_FA_LABEL     ) != 0 ) ||
                ( ( uiMask & HB_FA_DIRECTORY ) == 0 && ( ffind->attr & HB_FA_DIRECTORY ) != 0 ) ) )
         {
            char buffer[ 32 ];

            hb_arrayNew( pSubarray, 5 );
            if( bFullPath )
            {
               char * szFullName = hb_xstrcpy( NULL, fDirSpec->szPath ? fDirSpec->szPath : "", ffind->szName, NULL );
               hb_itemPutC( hb_arrayGetItemPtr( pSubarray, F_NAME ), szFullName );
               hb_xfree( szFullName );
            }
            else
               hb_itemPutC( hb_arrayGetItemPtr( pSubarray, F_NAME), ffind->szName );

            hb_itemPutNInt( hb_arrayGetItemPtr( pSubarray, F_SIZE ), ffind->size );
            hb_itemPutDL( hb_arrayGetItemPtr( pSubarray, F_DATE ), ffind->lDate );
            hb_itemPutC( hb_arrayGetItemPtr( pSubarray, F_TIME ), ffind->szTime );
            hb_itemPutC( hb_arrayGetItemPtr( pSubarray, F_ATTR ), hb_fsAttrDecode( ffind->attr, buffer ) );

            if( !bDirOnly || ( ffind->attr & HB_FA_DIRECTORY ) != 0 )
               hb_arrayAddForward( pDir, pSubarray );
         }
      }
      while( hb_fsFindNext( ffind ) );

      hb_itemRelease( pSubarray );

      hb_fsFindClose( ffind );
   }
}

static void HB_EXPORT hb_fsDirectory( PHB_ITEM pDir, char * szSkleton, char * szAttributes, BOOL bDirOnly, BOOL bFullPath )
{
   USHORT uiMask;
   USHORT uiMaskNoLabel;
   BYTE * szDirSpec;

/*
#if defined(__MINGW32__) || ( defined(_MSC_VER) && _MSC_VER >= 910 )
   PHB_ITEM pEightDotThree = hb_param( 3, HB_IT_LOGICAL );
   BOOL     bEightDotThree = pEightDotThree ? hb_itemGetL( pEightDotThree ) : FALSE; // Do we want 8.3 support?
#endif
*/

   PHB_FNAME pDirSpec = NULL;
   BOOL bAlloc = FALSE;

   /* Get the passed attributes and convert them to Harbour Flags */
   uiMask = HB_FA_ARCHIVE
          | HB_FA_READONLY
          | HB_FA_NORMAL
          | HB_FA_DEVICE
          | HB_FA_TEMPORARY
          | HB_FA_SPARSE
          | HB_FA_REPARSE
          | HB_FA_COMPRESSED
          | HB_FA_OFFLINE
          | HB_FA_NOTINDEXED
          | HB_FA_ENCRYPTED
          | HB_FA_VOLCOMP;

   uiMaskNoLabel = uiMask;

   hb_arrayNew( pDir, 0 );

   if( bDirOnly )
      szAttributes = "D";

   if( szAttributes && strlen( szAttributes ) > 0 )
   {
      if ( ( uiMask |= hb_fsAttrEncode( szAttributes ) ) & HB_FA_LABEL )
      {
         /* NOTE: This is Clipper Doc compatible. (not operationally) */
         uiMask = HB_FA_LABEL;
      }
   }

   if ( szSkleton && strlen( szSkleton ) > 0 )
      szDirSpec = hb_fsNameConv( ( BYTE * ) szSkleton, &bAlloc );
   else
      szDirSpec = ( BYTE * ) HB_OS_ALLFILE_MASK;

   if( bDirOnly || bFullPath )
   {
      if ( ( pDirSpec = hb_fsFNameSplit( ( char * ) szDirSpec ) ) !=NULL )
      {
         if( pDirSpec->szDrive )
            hb_fsChDrv( ( BYTE ) ( pDirSpec->szDrive[ 0 ] - 'A' ) );

         if( pDirSpec->szPath )
            hb_fsChDir( ( BYTE * ) pDirSpec->szPath );
      }
   }

   /* Get the file list */
   hb_fsGrabDirectory( pDir, ( const char * ) szDirSpec, uiMask, pDirSpec, bFullPath, bDirOnly );

   if( uiMask == HB_FA_LABEL )
   {
      uiMaskNoLabel |= hb_fsAttrEncode( szAttributes );
      uiMaskNoLabel &= ~HB_FA_LABEL;
      hb_fsGrabDirectory( pDir, ( const char * ) szDirSpec, uiMaskNoLabel, pDirSpec, bFullPath, bDirOnly );
   }

   if( pDirSpec )
      hb_xfree( pDirSpec );

   if( bAlloc )
      hb_xfree( szDirSpec );
}

static BOOL ZipTestExclude( char * szEntry, PHB_ITEM pExcludeFiles )
{
   ULONG nPos, nLen = hb_arrayLen( pExcludeFiles );

   for( nPos = 0; nPos < nLen; nPos++ )
   {
      if( hb_strMatchFile( hb_arrayGetCPtr( pExcludeFiles, nPos + 1 ), szEntry ) )
         return FALSE;
   }

   return TRUE;
}

static PHB_ITEM ZipCreateExclude( PHB_ITEM pExcludeParam )
{
   PHB_ITEM pExcludeFiles = hb_itemArrayNew( 0 );

   if( pExcludeParam )
   {
      if( HB_IS_STRING( pExcludeParam ) )
      {
         if( strchr( hb_itemGetCPtr( pExcludeParam ), '*' ) ||
             strchr( hb_itemGetCPtr( pExcludeParam ), '?' ) )
         {
            PHB_ITEM pDirFiles;
            ULONG nPos, nLen;

            pDirFiles = hb_itemNew( NULL );

            hb_fsDirectory( pDirFiles, hb_itemGetCPtr( pExcludeParam ), NULL, 0, TRUE );
            nLen = hb_arrayLen( pDirFiles );

            for( nPos = 0; nPos < nLen; nPos++ )
            {
               PHB_ITEM pTemp = hb_itemPutC( NULL, hb_arrayGetCPtr( hb_arrayGetItemPtr( pDirFiles, nPos + 1 ), F_NAME ) );
               hb_arrayAddForward( pExcludeFiles, pTemp );
               hb_itemRelease( pTemp );
            }

            hb_itemRelease( pDirFiles );
         }
         else if( hb_itemGetCLen( pExcludeParam ) )
         {
            PHB_ITEM pTemp = hb_itemPutC( NULL, hb_itemGetCPtr( pExcludeParam ) );
            hb_arrayAddForward( pExcludeFiles, pTemp );
            hb_itemRelease( pTemp );
         }
      }
      else if( HB_IS_ARRAY( pExcludeParam ) )
      {
         ULONG nPos, nLen = hb_arrayLen( pExcludeParam );

         for( nPos = 0; nPos < nLen; nPos++ )
         {
            char * szExclude = hb_arrayGetCPtr( pExcludeParam, nPos + 1 );

            if( strchr( szExclude, '*' ) ||
                strchr( szExclude, '?' ) )
            {
               PHB_ITEM pDirFiles = hb_itemNew( NULL );
               ULONG nDirPos, nDirLen;

               hb_fsDirectory( pDirFiles, szExclude, NULL, 0, TRUE );
               nDirLen = hb_arrayLen( pDirFiles );

               for( nDirPos = 0; nDirPos < nDirLen; nDirPos++ )
               {
                  PHB_ITEM pTemp = hb_itemPutC( NULL, hb_arrayGetCPtr( hb_arrayGetItemPtr( pDirFiles, nDirPos + 1 ), F_NAME ) );
                  hb_arrayAddForward( pExcludeFiles, pTemp );
                  hb_itemRelease( pTemp );
               }

               hb_itemRelease( pDirFiles );
            }
            else
            {
               PHB_ITEM pTemp = hb_itemPutC( NULL, szExclude );
               hb_arrayAddForward( pExcludeFiles, pTemp );
               hb_itemRelease( pTemp );
            }
         }
      }
   }

   return pExcludeFiles;
}

static PHB_ITEM ZipCreateArray( PHB_ITEM pParamFiles, BOOL bFullPath, PHB_ITEM pExcludeParam )
{
   PHB_ITEM pProcFiles = hb_itemArrayNew( 0 );

   if( pParamFiles )
   {
      PHB_ITEM pExcludeFiles = ZipCreateExclude( pExcludeParam );
      PHB_ITEM pParamArray;
      PHB_ITEM pDirFiles = hb_itemNew( NULL );

      ULONG nArrayPos, nArrayLen;

      if( HB_IS_STRING( pParamFiles ) )
      {
         PHB_ITEM pTemp;

         pParamArray = hb_itemArrayNew( 0 );

         pTemp = hb_itemPutC( NULL, hb_itemGetCPtr( pParamFiles ) );
         hb_arrayAddForward( pParamArray, pTemp );
         hb_itemRelease( pTemp );
      }
      else
         pParamArray = hb_arrayClone( pParamFiles );

      nArrayLen = hb_arrayLen( pParamArray );

      for( nArrayPos = 0; nArrayPos < nArrayLen; nArrayPos++ )
      {
         char * szArrEntry = hb_arrayGetCPtr( pParamArray, nArrayPos + 1 );

         if( strchr( szArrEntry, '*' ) ||
             strchr( szArrEntry, '?' ) )
         {
            ULONG nPos, nLen;

            hb_fsDirectory( pDirFiles, szArrEntry, NULL, 0, bFullPath );
            nLen = hb_arrayLen( pDirFiles );

            for( nPos = 0; nPos < nLen; nPos++ )
            {
               char * pszEntry = hb_arrayGetCPtr( hb_arrayGetItemPtr( pDirFiles, nPos + 1 ), F_NAME );

               if( ZipTestExclude( pszEntry, pExcludeFiles ) )
               {
                  PHB_ITEM pTemp = hb_itemPutC( NULL, pszEntry );
                  hb_arrayAddForward( pProcFiles, pTemp );
                  hb_itemRelease( pTemp );
               }
            }

            hb_itemClear( pDirFiles );
         }
         else
         {
            PHB_ITEM pTemp = hb_itemPutC( NULL, szArrEntry );
            hb_arrayAddForward( pProcFiles, pTemp );
            hb_itemRelease( pTemp );
         }
      }

      hb_itemRelease( pParamArray );
      hb_itemRelease( pDirFiles );
      hb_itemRelease( pExcludeFiles );
   }

   return pProcFiles;
}

/*
 * $DOC$
 * $FUNCNAME$
 *     HB_ZIPFILE()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *     Create a zip file
 * $SYNTAX$
 *     HB_ZIPFILE( <cFile>, <cFileToCompress> | <aFiles>, <nLevel>,
 *     <bBlock>, <lOverWrite>, <cPassword>, <lWithPath>, <lWithDrive>,
 *     <pFileProgress> ) ---> lCompress
 * $ARGUMENTS$
 *     <cFile>   Name of the zip file to create
 *
 *     <cFileToCompress>  Name of a file to Compress, Drive and/or path
 *     can be used
 *        _or_
 *     <aFiles>  An array containing files to compress, Drive and/or path
 *     can be used
 *
 *     <nLevel>  Compression level ranging from 0 to 9
 *
 *     <bBlock>  Code block to execute while compressing
 *
 *     <lOverWrite>  Toggle to overwrite the file if exists
 *
 *     <cPassword> Password to encrypt the files
 *
 *     <lWithPath> Toggle to store the path or not
 *
 *     <lWithDrive> Toggle to store the Drive letter and path or not
 *
 *     <pFileProgress> Code block for File Progress
 * $RETURNS$
 *     <lCompress>  .T. if file was create, otherwise .F.
 * $DESCRIPTION$
 *     This function creates a zip file named <cFile>. If the extension
 *     is omitted, .zip will be assumed. If the second parameter is a
 *     character string, this file will be added to the zip file. If the
 *     second parameter is an array, all file names contained in <aFiles>
 *     will be compressed.
 *
 *     If <nLevel> is used, it determines the compression type where 0 means
 *     no compression and 9 means best compression.
 *
 *     If <bBlock> is used, every time the file is opened to compress it
 *     will evaluate bBlock. Parameters of bBlock are cFile and nPos.
 *
 *     If <lOverWrite> is used, it toggles to overwrite or not the existing
 *     file. Default is to overwrite the file,otherwise if <lOverWrite> is false
 *     the new files are added to the <cFile>.
 *
 *     If <cPassword> is used, all files that are added to the archive are encrypted
 *     with the password.
 *
 *     If <lWithPath> is used, it tells  the path should also be stored with
 *     the file name. Default is false.
 *
 *     If <lWithDrive> is used, it tells thats the Drive and path should also be stored
 *     with the file name. Default is false.
 *
 *     If <pFileProgress> is used, an Code block is evaluated, showing the total
 *     of that file has being processed.
 *     The codeblock must be defined as follow {|nPos,nTotal| GaugeUpdate(aGauge1,(nPos/nTotal))}
 *
 * $EXAMPLES$
 *     FUNCTION MAIN()
 *
 *     IF HB_ZIPFILE( "test.zip", "test.prg" )
 *        qout( "File was successfully created" )
 *     ENDIF
 *
 *     IF HB_ZIPFILE( "test1.zip", { "test.prg", "C:\windows\win.ini" } )
 *        qout( "File was successfully created" )
 *     ENDIF
 *
 *     IF HB_ZIPFILE( "test2.zip", { "test.prg", "C:\windows\win.ini" }, 9, {|cFile,nPos,| qout(cFile) } )
 *        qout( "File was successfully created" )
 *     ENDIF
 *
 *     aFiles := { "test.prg", "C:\windows\win.ini" }
 *     nLen   := Len( aFiles )
 *     aGauge := GaugeNew( 5, 5, 7, 40, "W/B", "W+/B" , "." )
 *     GaugeDisplay( aGauge )
 *     HB_ZIPFILE( "test33.zip", aFiles, 9, {|cFile,nPos| GaugeUpdate( aGauge, nPos/nLen ) },, "hello" )
 *     Return NIL
 * $STATUS$
 *     R
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *     All
 * $FILES$
 *     Library is hbziparch.lib
 * $END$
 */

HB_FUNC( HB_ZIPFILE )
{
   BOOL bReturn = FALSE;

   if( ISCHAR( 1 ) )
   {
      PHB_ITEM pProcFiles = ZipCreateArray( hb_param( 2, HB_IT_STRING | HB_IT_ARRAY ),
                                            ISLOG( 11 ) ? hb_parl( 11 ) : TRUE,
                                            hb_param( 10, HB_IT_STRING | HB_IT_ARRAY ) );

      if( hb_arrayLen( pProcFiles ) )
      {
         bReturn = hb_CompressFile( hb_parc( 1 ),
                                    pProcFiles,
                                    ISNUM( 3 ) ? hb_parni( 3 ) : -1,
                                    hb_param( 4, HB_IT_BLOCK ),
                                    hb_parl( 5 ),
                                    hb_parc( 6 ),
                                    0,
                                    hb_parl( 7 ),
                                    hb_parl( 8 ),
                                    hb_param( 9, HB_IT_BLOCK ), 
                                    FALSE );
      }

      hb_itemRelease( pProcFiles );
   }

   hb_retl( bReturn );
}

/*
 * $DOC$
 * $FUNCNAME$
 *     HB_ZIPFILEBYPKSPAN()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *     Create a zip file on removable media
 * $SYNTAX$
 *     HB_ZIPFILEBYPKSPAN( <cFile>, <cFileToCompress> | <aFiles>, <nLevel>,
 *     <bBlock>, <lOverWrite>, <cPassword>, <lWithPath>, <lWithDrive>,
 *     <pFileProgress>) ---> lCompress
 * $ARGUMENTS$
 *     <cFile>   Name of the zip file
 *
 *     <cFileToCompress>  Name of a file to Compress, Drive and/or path
 *     can be used
 *         _or_
 *     <aFiles>  An array containing files to compress, Drive and/or path
 *     can be used
 *
 *     <nLevel>  Compression level ranging from 0 to 9
 *
 *     <bBlock>  Code block to execute while compressing
 *
 *     <lOverWrite>  Toggle to overwrite the file if exists
 *
 *     <cPassword> Password to encrypt the files
 *
 *     <lWithPath> Toggle to store the path or not
 *
 *     <lWithDrive> Toggle to store the Drive letter and path or not
 *
 *     <pFileProgress> Code block for File Progress
 * $RETURNS$
 *     <lCompress>  .T. if file was create, otherwise .F.
 * $DESCRIPTION$
 *     This function creates a zip file named <cFile>. If the extension
 *     is omitted, .zip will be assumed. If the second parameter is a
 *     character string, this file will be added to the zip file. If the
 *     second parameter is an array, all file names contained in <aFiles>
 *     will be compressed.  Also, the use of this function is for creating
 *     backup in removable media like an floppy drive/zip drive.
 *
 *     If <nLevel> is used, it determines the compression type where 0 means
 *     no compression and 9 means best compression.
 *
 *     If <bBlock> is used, every time the file is opened to compress it
 *     will evaluate bBlock. Parameters of bBlock are cFile and nPos.
 *
 *     If <lOverWrite> is used , it toggles to overwrite or not the existing
 *     file. Default is to overwrite the file, otherwise if <lOverWrite> is false
 *     the new files are added to the <cFile>.
 *
 *     If <cPassword> is used, all files that are added to the archive are encrypted
 *     with the password.
 *
 *     If <lWithPath> is used, it tells thats the path should also be stored with
 *     the file name. Default is false.
 *
 *     If <lWithDrive> is used, it tells thats the Drive and path should also be stored
 *     with the file name. Default is false.
 *
 *     If <pFileProgress> is used, an Code block is evaluated, showing the total
 *     of that file has being processed.
 *     The codeblock must be defined as follow {|nPos,nTotal| GaugeUpdate(aGauge1,(nPos/nTotal))}
 *
 *     Before calling this function, Set an Changedisk codeblock by calling
 *     the HB_SETDISKZIP().
 * $EXAMPLES$
 *     FUNCTION MAIN()
 *
 *     hb_setdiskzip( {|nDisk| Alert( "Please insert disk no " + Str( nDisk, 3 ) ) } )
 *
 *     IF HB_ZIPFILEBYPKSPAN( "A:\test.zip", "test.prg" )
 *        qout( "File was successfully created" )
 *     ENDIF
 *
 *     IF HB_ZIPFILEBYPKSPAN( "A:\test1.zip", { "test.prg", "C:\windows\win.ini" } )
 *        qout( "File was successfully created" )
 *     ENDIF
 *
 *     IF HB_ZIPFILEBYPKSPAN( "test2.zip", { "test.prg", "C:\windows\win.ini"}, 9, {|nPos,cFile| qout(cFile) } )
 *        qout( "File was successfully created" )
 *     ENDIF
 *
 *     aFiles := { "test.prg", "C:\windows\win.ini" }
 *     nLen   := Len( aFiles )
 *     aGauge := GaugeNew( 5, 5, 7, 40, "W/B", "W+/B", "." )
 *     GaugeDisplay( aGauge )
 *     HB_ZIPFILEBYPKSPAN( "F:\test33.zip", aFiles, 9, {|cFile,nPos| GaugeUpdate( aGauge, nPos/nLen ) },, "hello" )
 *     // assuming F:\ is a Zip Drive
 *     Return NIL
 * $STATUS$
 *     R
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *     All
 * $FILES$
 *     Library is hbziparch.lib
 * $END$
 */

HB_FUNC( HB_ZIPFILEBYPKSPAN )
{
   BOOL bReturn = FALSE;

   if( ISCHAR( 1 ) )
   {
      PHB_ITEM pProcFiles = ZipCreateArray( hb_param( 2, HB_IT_STRING | HB_IT_ARRAY ),
                                            ISLOG( 11 ) ? hb_parl( 11 ) : TRUE,
                                            hb_param( 10, HB_IT_STRING | HB_IT_ARRAY ) );

      if( hb_arrayLen( pProcFiles ) )
      {
         bReturn = hb_CompressFile( hb_parc( 1 ),
                                    pProcFiles,
                                    ISNUM( 3 ) ? hb_parni( 3 ) : -1,
                                    hb_param( 4, HB_IT_BLOCK ),
                                    hb_parl( 5 ),
                                    hb_parc( 6 ),
                                    0,
                                    hb_parl( 7 ),
                                    hb_parl( 8 ),
                                    hb_param( 9, HB_IT_BLOCK ),
                                    TRUE );
      }

      hb_itemRelease( pProcFiles );
   }

   hb_retl( bReturn );
}

/*
 * $DOC$
 * $FUNCNAME$
 *     HB_ZIPFILEBYTDSPAN()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *     Create a zip file
 * $SYNTAX$
 *     HB_ZIPFILEBYTDSPAN( <cFile> ,<cFileToCompress> | <aFiles>, <nLevel>,
 *     <bBlock>, <lOverWrite>, <cPassword>, <iSize>, <lWithPath>, <lWithDrive>,
 *     <pFileProgress>) ---> lCompress
 * $ARGUMENTS$
 *     <cFile>   Name of the zip file
 *
 *     <cFileToCompress>  Name of a file to Compress, Drive and/or path
 *     can be used
 *         _or_
 *     <aFiles>  An array containing files to compress, Drive and/or path
 *     can be used
 *
 *     <nLevel>  Compression level ranging from 0 to 9
 *
 *     <bBlock>  Code block to execute while compressing
 *
 *     <lOverWrite>  Toggle to overwrite the file if exists
 *
 *     <cPassword> Password to encrypt the files
 *
 *     <iSize> Size of the archive, in bytes. Default is 1457664 bytes
 *
 *     <lWithPath> Toggle to store the path or not
 *
 *     <lWithDrive> Toggle to store the Drive letter and path or not
 *
 *     <pFileProgress> Code block for File Progress
 * $RETURNS$
 *     <lCompress>  .T. if file was create, otherwise .F.
 * $DESCRIPTION$
 *     This function creates a zip file named <cFile>. If the extension
 *     is omitted, .zip will be assumed. If the second parameter is a
 *     character string, this file will be added to the zip file. If the
 *     second parameter is an array, all file names contained in <aFiles>
 *     will be compressed.
 *
 *     If <nLevel> is used, it determines the compression type where 0 means
 *     no compression and 9 means best compression.
 *
 *     If <bBlock> is used, every time the file is opened to compress it
 *     will evaluate bBlock. Parameters of bBlock are cFile and nPos.
 *
 *     If <lOverWrite> is used, it toggles to overwrite or not the existing
 *     file. Default is to overwrite the file, otherwise if <lOverWrite> is
 *     false the new files are added to the <cFile>.
 *
 *     If <lWithPath> is used, it tells thats the path should also be stored '
 *     with the file name. Default is false.
 *
 *     If <lWithDrive> is used, it tells thats the Drive and path should also
 *     be stored with the file name. Default is false.
 *
 *     If <pFileProgress> is used, an Code block is evaluated, showing the total
 *     of that file has being processed.
 *     The codeblock must be defined as follow {|nPos,nTotal| GaugeUpdate(aGauge1,(nPos/nTotal))}
 * $EXAMPLES$
 *     FUNCTION MAIN()
 *
 *     IF HB_ZIPFILEBYTDSPAN( "test.zip", "test.prg" )
 *        qout( "File was successfully created" )
 *     ENDIF
 *
 *     IF HB_ZIPFILEBYTDSPAN( "test1.zip", { "test.prg", "C:\windows\win.ini" } )
 *        qout( "File was successfully created" )
 *     ENDIF
 *
 *     IF HB_ZIPFILEBYTDSPAN( "test2.zip", { "test.prg", "C:\windows\win.ini" }, 9, {|nPos,cFile| qout(cFile) }, "hello",, 521421 )
 *        qout("File was successfully created" )
 *     ENDIF
 *
 *     aFiles := { "test.prg", "C:\windows\win.ini" }
 *     nLen   := Len( aFiles )
 *     aGauge := GaugeNew( 5, 5, 7, 40, "W/B", "W+/B", "." )
 *     GaugeDisplay( aGauge )
 *     HB_ZIPFILEBYTDSPAN( "test33.zip", aFiles, 9, {|cFile,nPos| GaugeUpdate( aGauge, nPos/nLen) },, "hello",, 6585452 )
 *     Return NIL
 * $STATUS$
 *     R
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *     All
 * $FILES$
 *     Library is hbziparch.lib
 * $END$
 */

HB_FUNC( HB_ZIPFILEBYTDSPAN )
{
   BOOL bReturn = FALSE;

   if( ISCHAR( 1 ) )
   {
      PHB_ITEM pProcFiles = ZipCreateArray( hb_param( 2, HB_IT_STRING | HB_IT_ARRAY ),
                                            ISLOG( 12 ) ? hb_parl( 12 ) : TRUE,
                                            hb_param( 11, HB_IT_STRING | HB_IT_ARRAY ) );

      if( hb_arrayLen( pProcFiles ) )
      {
         bReturn = hb_CompressFile( hb_parc( 1 ),
                                    pProcFiles,
                                    ISNUM( 3 ) ? hb_parni( 3 ) : -1,
                                    hb_param( 4, HB_IT_BLOCK ),
                                    hb_parl( 5 ),
                                    hb_parc( 6 ),
                                    ISNUM( 7 ) ? hb_parnl( 7 ) : 1457664,
                                    hb_parl( 8 ),
                                    hb_parl( 9 ),
                                    hb_param( 10, HB_IT_BLOCK ),
                                    TRUE );
      }

      hb_itemRelease( pProcFiles );
   }

   hb_retl( bReturn );
}

/*
 * $DOC$
 * $FUNCNAME$
 *     HB_UNZIPFILE()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *     Unzip a compressed file
 * $SYNTAX$
 *     HB_UNZIPFILE( <cFile>, <bBlock>, <lWithPath>, <cPassWord>, <cPath>,
 *                   <cFile> | <aFile>, <pFileProgress> ) ---> lCompress
 * $ARGUMENTS$
 *     <cFile>   Name of the zip file to extract
 *
 *     <bBlock>  Code block to execute while extracting
 *
 *     <lWithPath> Toggle to create directory if needed
 *
 *     <cPassWord> Password to use to extract files
 *
 *     <cPath>    Path to extract the files to - mandatory
 *
 *     <cFile> | <aFiles> A File or Array of files to extract - mandatory
 *
 *     <pFileProgress> Code block for File Progress
 * $RETURNS$
 *     <lCompress>  .T. if all file was successfully restored, otherwise .F.
 * $DESCRIPTION$
 *     This function restores all files contained inside the <cFile>.
 *     If the extension is omitted, .zip will be assumed. If a file already
 *     exists, it will be overwritten.
 *
 *     If <bBlock> is used, every time the file is opened to compress it
 *     will evaluate bBlock. Parameters of bBlock are cFile and nPos.
 *
 *     The <cPath> is a mandatory parameter. Set to ".\" to extract to the
 *     current directory
 *
 *     If <cFile> or <aFiles> are not provided, no files will be extracted!
 *     Make sure you provide the file or files you want extracted
 *
 *     If <pFileProgress> is used, an Code block is evaluated, showing the total
 *     of that file has being processed.
 *     The codeblock must be defined as follow {|nPos,nTotal| GaugeUpdate(aGauge1,(nPos/nTotal))}
 * $EXAMPLES$
 *     FUNCTION MAIN()
 *
 *     aExtract := hb_GetFilesInZip( "test.zip" )  // extract all files in zip
 *     IF HB_UNZIPFILE( "test.zip",,,, ".\", aExtract )
 *        qout("File was successfully extracted")
 *     ENDIF
 *
 *     aExtract := hb_GetFilesInZip( "test2.zip" )  // extract all files in zip
 *     IF HB_UNZIPFILE( "test2.zip", {|cFile| qout( cFile ) },,, ".\", aExtract )
 *        qout("File was successfully extracted")
 *     ENDIF
 *     Return NIL
 * $STATUS$
 *     R
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *     All
 * $FILES$
 *     Library is hbziparch.lib
 * $END$
 */

static void UnzipAddFileMask( const char * pszMask, PHB_ITEM pProcFiles, PHB_ITEM pZipFiles )
{
   ULONG nPos, nLen = hb_arrayLen( pZipFiles );

   for( nPos = 0; nPos < nLen; nPos++ )
   {
      const char * szEntry = hb_arrayGetCPtr( pZipFiles, nPos + 1 );
      BOOL bAdd = TRUE;

      if( pszMask )
         bAdd = hb_strMatchFile( szEntry, ( const char * ) pszMask );

      if( ! bAdd )
      {
         PHB_FNAME pFileName = hb_fsFNameSplit( szEntry );

         if( pFileName->szName )
         {
            char * pszFile = ( char * ) hb_xgrab( _POSIX_PATH_MAX + 1 );
            pFileName->szPath = "";
            hb_fsFNameMerge( pszFile, pFileName );
            bAdd = hb_strMatchFile( pszMask, pszFile );
            hb_xfree( pszFile );

            if( ! bAdd )
               bAdd = hb_strMatchFile( pszMask, szEntry );
         }

         hb_xfree( pFileName );
      }

      if( bAdd )
      {
         PHB_ITEM pTemp = hb_itemPutC( NULL, szEntry );
         hb_arrayAddForward( pProcFiles, pTemp );
         hb_itemRelease( pTemp );
      }
   }
}

static void hb_procexistingzip( BOOL bUnzip )
{
   BOOL bReturn = FALSE;

   if( ISCHAR( 1 ) )
   {
      char * pszZipFileName = hb_FNAddZipExt( hb_parc( 1 ) );
      PHB_ITEM pParamFiles = hb_param( bUnzip ? 6 : 2, HB_IT_STRING | HB_IT_NUMERIC | HB_IT_ARRAY );
      PHB_ITEM pProcFiles = hb_itemArrayNew( 0 );

      if( pParamFiles )
      {
         PHB_ITEM pZipFiles = hb_GetFileNamesFromZip( pszZipFileName, FALSE );
         ULONG nZipLen = hb_arrayLen( pZipFiles );

         if( HB_IS_ARRAY( pParamFiles ) )
         {
            ULONG nPos, nLen = hb_arrayLen( pParamFiles );
         
            for( nPos = 0; nPos < nLen; nPos++ )
            {
               HB_TYPE type = hb_arrayGetType( pParamFiles, nPos + 1 );
         
               if( type & HB_IT_NUMERIC )
               {
                  ULONG nZipPos = hb_arrayGetNL( pParamFiles, nPos + 1 );
         
                  if( nZipPos > 0 && nZipPos <= nZipLen )
                  {
                     PHB_ITEM pTemp = hb_itemPutC( NULL, hb_arrayGetCPtr( pZipFiles, nZipPos ) );
                     hb_arrayAddForward( pProcFiles, pTemp );
                     hb_itemRelease( pTemp );
                  }
               }
               else if( type & HB_IT_STRING )
                  UnzipAddFileMask( hb_arrayGetCPtr( pParamFiles, nPos + 1 ), pProcFiles, pZipFiles );
            }
         }
         else if( HB_IS_NUMERIC( pParamFiles ) )
         {
            ULONG nZipPos = hb_itemGetNL( pParamFiles );
         
            if( nZipPos > 0 && nZipPos <= nZipLen )
            {
               PHB_ITEM pTemp = hb_itemPutC( NULL, hb_arrayGetCPtr( pZipFiles, nZipPos ) );
               hb_arrayAddForward( pProcFiles, pTemp );
               hb_itemRelease( pTemp );
            }
         }
         else
            UnzipAddFileMask( hb_itemGetCPtr( pParamFiles ), pProcFiles, pZipFiles );

         hb_itemRelease( pZipFiles );
      }

      if( bUnzip )
      {
         if( ! pParamFiles || hb_arrayLen( pProcFiles ) )
         {
            bReturn = hb_Unzip( pszZipFileName,
                                hb_param( 2, HB_IT_BLOCK ),
                                hb_parl( 3 ),
                                hb_parc( 4 ),
                                hb_parc( 5 ),
                                pParamFiles ? pProcFiles : NULL,
                                hb_param( 7, HB_IT_BLOCK ) );
         }
      }
      else
      {
         if( hb_arrayLen( pProcFiles ) )
            bReturn = hb_DeleteSel( pszZipFileName,
                                    pProcFiles );
      }

      hb_itemRelease( pProcFiles );
      hb_xfree( pszZipFileName );
   }

   hb_retl( bReturn );
}

HB_FUNC( HB_UNZIPFILE )
{
   hb_procexistingzip( TRUE );
}

HB_FUNC( HB_UNZIPFILEINDEX )
{
   HB_FUNC_EXEC( HB_UNZIPFILE );
}

HB_FUNC( HB_UNZIPALLFILE )
{
   HB_FUNC_EXEC( HB_UNZIPFILE );
}

/* $DOC$
 * $FUNCNAME$
 *     HB_ZIPDELETEFILES()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *     Delete files from an zip archive
 * $SYNTAX$
 *     HB_ZIPDELETEFILES( <cFile>, <cFiletoDelete> | <aFiles> | <nFilePos> ) --> <lDeleted>
 * $ARGUMENTS$
 *     <cFile>  The name of the zip files from where the files will be deleted
 *
 *     <cFiletoDelete> An File to be removed
 *        _or_
 *     <aFiles>    An Array of Files to be removed
 *        _or_
 *     <nFilePos> The Position of the file to be removed
 * $RETURNS$
 *     <lDeleted> If the files are deleted, it will return .T.; otherwise
 *     it will return .F. in the following cases: Spanned Archives; the file(s)
 *     could not be found in the zip file.
 * $DESCRIPTION$
 *     This  function removes files from an Zip archive.
 * $EXAMPLES$
 *     ? "has the file zipnew.i been deleted ", iif( HB_ZIPDELETEFILES( "\test23.zip", "zipnew.i" ), "Yes", "No" )
 * $STATUS$
 *     R
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *     All
 * $FILES$
 *     Library is hbziparch.lib
 * $END$
 */

HB_FUNC( HB_ZIPDELETEFILES )
{
   hb_procexistingzip( FALSE );
}

/* $DOC$
 * $FUNCNAME$
 *     HB_SETDISKZIP()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *     Set an codeblock for disk changes
 * $SYNTAX$
 *     HB_SETDISKZIP( <bBlock> ) ---> TRUE
 * $ARGUMENTS$
 *     <bBlock> an Code block that contains an function that will be performed
 *     when the need of changing disk are need.
 * $RETURNS$
 *     It always returns True
 * $DESCRIPTION$
 *     This function will set an codeblock that will be evaluated every time
 *     that an changedisk event is necessary. <bBlock> receives nDisk as a
 *     code block param that corresponds to the diskette number to be processed.
 *
 *     Set this function before opening archives that are in removable media.
 *     This block will be released, when the caller finish it job.
 * $EXAMPLES$
 *     HB_SETDISKZIP( {|nDisk| Alert( "Please insert disk no " + Str( nDisk, 3 ) ) } )
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *     All
 * $FILES$
 *     Library is hbziparch.lib
 * $END$
 */

HB_FUNC( HB_SETDISKZIP )
{
   hb_retl( hb_SetCallbackFunc( hb_param( 1, HB_IT_BLOCK ) ) );
}

/* $DOC$
 * $FUNCNAME$
 *     HB_ZIPTESTPK()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *     Test pkSpanned zip files
 * $SYNTAX$
 *     HB_ZIPTESTPK( <cFile> ) --> <nReturnCode>
 * $ARGUMENTS$
 *     <cFile>  File to be tested.
 * $RETURNS$
 *     <nReturn> A code that tells if the current disk is the last of a
 *     pkSpanned disk set.
 * $DESCRIPTION$
 *     This function tests if the disk inserted is the last disk of an backup
 *     set or not.
 *     It will return the follow return code when an error is found
 *
 *     <table>
 *     Error code     Meaning
 *     114            Incorrect Disk
 *     103            No Call back was set with HB_SETDISKZIP()
 *     </table>
 *
 *     Call this function to determine if the disk inserted is the correct
 *     one before any other function.
 * $EXAMPLES$
 *     if HB_ZIPTESTPK( "A:\test22.zip" ) == 114
 *        ? "Invalid Diskette"
 *     endif
 * $STATUS$
 *     R
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *     All
 * $FILES$
 *     Library is hbziparch.lib
 * $END$
 */

HB_FUNC( HB_ZIPTESTPK )
{
   char * pszZipFileName = hb_FNAddZipExt( hb_parc( 1 ) );

   hb_retni( hb_CheckSpanMode( pszZipFileName ) );

   hb_xfree( pszZipFileName );
}

/* $DOC$
 * $FUNCNAME$
 *     HB_SETBUFFER()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *
 * $SYNTAX$
 *     HB_SETBUFFER( [<nWriteBuffer>], [<nExtractBuffer>], [<nReadBuffer>] ) --> NIL
 * $ARGUMENTS$
 *     <nWriteBuffer>   The size of the write buffer.
 *
 *     <nExtractBuffer> The size of the extract buffer.
 *
 *     <nReadBuffer>    The size of the read buffer.
 * $RETURNS$
 *     <NIL>            This function always returns NIL.
 * $DESCRIPTION$
 *     This function set the size of the internal buffers for write/extract/read
 *     operation
 *
 *     If the size of the buffer is smaller then the default, the function
 *     will automatically use the default values, which are 65535/16384/32768
 *     respectively.
 *
 *     This function be called before any of the compression/decompression
 *     functions.
 * $EXAMPLES$
 *     HB_SETBUFFER( 100000, 115214, 65242 )
 * $STATUS$
 *     R
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *     All
 * $FILES$
 *     Library is hbziparch.lib
 * $END$
 */

HB_FUNC( HB_SETBUFFER )
{
   hb_SetZipBuff( hb_parni( 1 ), hb_parni( 2 ), hb_parni( 3 ) );
}

/* $DOC$
 * $FUNCNAME$
 *     HB_SETZIPCOMMENT()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *     Set an Zip archive Comment
 * $SYNTAX$
 *     HB_SETZIPCOMMENT( <cComment> ) --> NIL
 * $ARGUMENTS$
 *     <cComment>   Comment to add to the zip archive
 * $RETURNS$
 *     <NIL> this function always return NIL
 * $DESCRIPTION$
 *     This function stored an global comment to an zip archive.
 *     It should be called before any of the compression functions.
 * $EXAMPLES$
 *     HB_SETZIPCOMMENT( "This is an Test" )
 *     hb_zipfile( "test.zip", { "\windows\ios.ini", "\windows\win.ini" } )
 * $STATUS$
 *     R
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *      All
 * $FILES$
 *      Library is hbziparch.lib
 * $END$
 */

HB_FUNC( HB_SETZIPCOMMENT )
{
   hb_SetZipComment( hb_parc( 1 ) );
}

/* $DOC$
 * $FUNCNAME$
 *     HB_GETZIPCOMMENT()
 * $CATEGORY$
 *     Zip Functions
 * $ONELINER$
 *     Return the comment of an zip file
 * $SYNTAX$
 *     HB_GETZIPCOMMENT( <szFile> ) --> <szComment>
 * $ARGUMENTS$
 *     <szFile>  File to get the comment from
 * $RETURNS$
 *     <szComment>  The comment that was stored in <szFile>
 * $DESCRIPTION$
 *     This function receives a valid zip file name as parameter,
 *     and returns the global comment stored within.
 * $EXAMPLES$
 *     ? "The comment in test.zip is ", HB_GETZIPCOMMENT( "test.zip" )
 * $STATUS$
 *     R
 * $COMPLIANCE$
 *     This function is a Harbour extension
 * $PLATFORMS$
 *     All
 * $FILES$
 *     Library is hbziparch.lib
 * $END$
 */

HB_FUNC( HB_GETZIPCOMMENT )
{
   hb_retc_buffer( hb_GetZipComment( hb_parc( 1 ) ) );
}

HB_FUNC( TRANSFERFROMZIP )
{
   hb_retl( hb_TransferFilesFromzip( hb_parc( 1 ),
                                     hb_parc( 2 ),
                                     hb_param( 3, HB_IT_ARRAY ) ) );
}

HB_FUNC( SETZIPREADONLY )
{
   hb_SetZipReadOnly( hb_parl( 1 ) );
}

HB_FUNC( HB_ZIPWITHPASSWORD )
{
   hb_retl( hb_IsPassWord( hb_parc( 1 ) ) );
}

HB_FUNC( HB_GETFILECOUNT )
{
   ULONG nCount = 0;

   if( ISCHAR( 1 ) )
   {
      char * pszZipFileName = hb_FNAddZipExt( hb_parc( 1 ) );

      nCount = hb_GetNumberofFilestoUnzip( pszZipFileName );

      hb_xfree( pszZipFileName );
   }

   hb_retnl( ( long ) nCount );
}

HB_FUNC( HB_GETFILESINZIP )
{
   if( ISCHAR( 1 ) )
   {
      char * pszZipFileName = hb_FNAddZipExt( hb_parc( 1 ) );

      hb_itemReturnRelease( hb_GetFileNamesFromZip( pszZipFileName, hb_parl( 2 ) ) );

      hb_xfree( pszZipFileName );
   }
   else
      hb_reta( 0 );
}
