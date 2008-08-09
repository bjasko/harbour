/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    ZLIB functions wrapper
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

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbzlib.h"

static ULONG hb_zlibUncompressedSize( const char * szSrc, ULONG ulLen )
{
   Byte buffer[ 1024 ];
   z_stream stream;
   ULONG ulDest = 0;

   memset( &stream, 0, sizeof( z_stream ) );

   stream.next_in   = ( Bytef * ) szSrc;
   stream.avail_in  = ( uInt ) ulLen;
/*
   stream.zalloc    = Z_NULL;
   stream.zfree     = Z_NULL;
   stream.opaque    = NULL;
*/
   if( inflateInit( &stream ) == Z_OK )
   {
      int iStatus;
      do
      {
         stream.next_out  = buffer;
         stream.avail_out = sizeof( buffer );
         iStatus = inflate( &stream, Z_NO_FLUSH );
      }
      while( iStatus == Z_OK );
      if( iStatus == Z_STREAM_END )
         ulDest = stream.total_out;
      inflateEnd( &stream );
   }

   return ulDest;
}

/*
 * HB_ZLIBVERSION( [<nType>] ) -> <cZlibVersion>
 */
HB_FUNC( HB_ZLIBVERSION )
{
   if( hb_parni( 1 ) == 1 )
      hb_retc( ZLIB_VERSION );
   else
      hb_retc( zlibVersion() );
}

/*
 * HB_ZCOMPRESSBOUND( <cData> | <nDataLen> ) -> <nMaxCompressLen>
 */
HB_FUNC( HB_ZCOMPRESSBOUND )
{
   if( ISCHAR( 1 ) )
      hb_retnint( compressBound( hb_parclen( 1 ) ) );
   else if( ISNUM( 1 ) )
      hb_retnint( compressBound( ( uLong ) hb_parnint( 1 ) ) );
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_ZUNCOMPRESSLEN( <cCompressedData> ) -> <nUnCompressedDataLen> or 0 on error
 */
HB_FUNC( HB_ZUNCOMPRESSLEN )
{
   ULONG ulLen = hb_parclen( 1 );

   hb_retnint( ulLen ? hb_zlibUncompressedSize( hb_parc( 1 ), ulLen ) : 0 );
}

/*
 * HB_ZCOMPRESS( <cData>, [<nDstBufLen>|<@cBuffer>], [<@nResult>], [<nLevel>] )
 *    => <cCompressedData> or NIL on Error
 */
HB_FUNC( HB_ZCOMPRESS )
{
   char * szData = hb_parc( 1 );
   if( szData )
   {
      ULONG ulLen = hb_parclen( 1 );

      if( ulLen )
      {
         PHB_ITEM pBuffer = ISBYREF( 2 ) ? hb_param( 2, HB_IT_STRING ) : NULL;
         uLong ulDstLen;
         char * pDest;
         int iResult;

         if( pBuffer )
         {
            pBuffer = hb_itemUnShareString( pBuffer );
            ulDstLen = hb_itemGetCLen( pBuffer );
            pDest = hb_itemGetCPtr( pBuffer );
         }
         else
         {
            ulDstLen = ISNUM( 2 ) ? ( uLong ) hb_parnint( 2 ) :
                                    compressBound( ulLen );
            pDest = ( char * ) hb_xalloc( ulDstLen + 1 );
         }

         if( pDest )
         {
            if( ISNUM( 4 ) )
               iResult = compress2( ( Bytef * ) pDest, &ulDstLen, ( Bytef * ) szData, ulLen, hb_parni( 4 ) );
            else
               iResult = compress( ( Bytef * ) pDest, &ulDstLen, ( Bytef * ) szData, ulLen );

            if( !pBuffer )
            {
               if( iResult == Z_OK )
                  hb_retclen_buffer( pDest, ulDstLen );
               else
                  hb_xfree( pDest );
            }
            else if( iResult == Z_OK )
               hb_retclen( pDest, ulDstLen );
         }
         else
            iResult = Z_MEM_ERROR;

         hb_storni( iResult, 3 );
      }
      else
      {
         hb_retc( NULL );
         hb_storni( Z_OK, 3 );
      }
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_ZUNCOMPRESS( <cCompressedData>, [<nDstBufLen>|<@cBuffer>], [<@nResult>] )
 *    => <cUnCompressedData> or NIL on Error
 */
HB_FUNC( HB_ZUNCOMPRESS )
{
   PHB_ITEM pBuffer = ISBYREF( 2 ) ? hb_param( 2, HB_IT_STRING ) : NULL;
   char * szData = hb_parc( 1 );

   if( szData )
   {
      ULONG ulLen = hb_parclen( 1 );

      if( ulLen )
      {
         uLong ulDstLen;
         char * pDest;
         int iResult;

         if( pBuffer )
         {
            pBuffer = hb_itemUnShareString( pBuffer );
            ulDstLen = hb_itemGetCLen( pBuffer );
            pDest = hb_itemGetCPtr( pBuffer );
         }
         else
         {
            ulDstLen = ISNUM( 2 ) ? ( uLong ) hb_parnint( 2 ) :
                                    hb_zlibUncompressedSize( szData, ulLen );
            pDest = ( char * ) hb_xalloc( ulDstLen + 1 );
         }

         if( pDest )
         {
            iResult = uncompress( ( Bytef * ) pDest, &ulDstLen, ( Bytef * ) szData, ulLen );

            if( !pBuffer )
            {
               if( iResult == Z_OK )
                  hb_retclen_buffer( pDest, ulDstLen );
               else
                  hb_xfree( pDest );
            }
            else if( iResult == Z_OK )
               hb_retclen( pDest, ulDstLen );
         }
         else
            iResult = Z_MEM_ERROR;

         hb_storni( iResult, 3 );
      }
      else
      {
         hb_retc( NULL );
         hb_storni( Z_OK, 3 );
      }
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}


/* GZIP stream destructor */
static HB_GARBAGE_FUNC( hb_gz_Destructor )
{
   gzFile * gz = ( gzFile * ) Cargo;
   if( * gz )
   {
      gzclose( * gz );
      * gz = NULL;
   }
}

static gzFile hb_gzParam( int iParam )
{
   gzFile * gzHolder = ( gzFile * ) hb_parptrGC( hb_gz_Destructor, iParam );

   if( gzHolder && * gzHolder )
      return * gzHolder;

   hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
   return NULL;
}

/*
 * HB_GZOPEN( <cFile>, <cMode> ) => <pGZipStram> or NIL on Error
 */
HB_FUNC( HB_GZOPEN )
{
   char * cFile = hb_parc( 1 ), * cMode = hb_parc( 2 );
   if( cFile && cMode )
   {
      gzFile gz = gzopen( cFile, cMode );
      if( gz )
      {
         gzFile * gzHolder = ( gzFile * ) hb_gcAlloc( sizeof( gzFile ),
                                                      hb_gz_Destructor );
         * gzHolder = gz;
         hb_retptrGC( gzHolder );
      }
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZDOPEN( <hFile>, <cMode> ) => <pGZipStram> or NIL on Error
 */
HB_FUNC( HB_GZDOPEN )
{
   char * cMode = hb_parc( 2 );
   if( ISNUM( 1 ) && cMode )
   {
      gzFile gz = gzdopen( hb_parni( 1 ), cMode );
      if( gz )
      {
         gzFile * gzHolder = ( gzFile * ) hb_gcAlloc( sizeof( gzFile ),
                                                      hb_gz_Destructor );
         * gzHolder = gz;
         hb_retptrGC( gzHolder );
      }
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZCLOSE( <pGZipStream> ) => <nResult>
 */
HB_FUNC( HB_GZCLOSE )
{
   gzFile * gzHolder = ( gzFile * ) hb_parptrGC( hb_gz_Destructor, 1 );

   if( gzHolder )
   {
      gzFile gz = * gzHolder;
      * gzHolder = NULL;
      hb_retni( gzclose( gz ) );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZSETPARAMS( <pGZipStream>, <nLevel>, <nStrategy> ) => <nResult>
 */
HB_FUNC( HB_GZSETPARAMS )
{
   if( ISNUM( 2 ) && ISNUM( 3 ) )
   {
      gzFile gz = hb_gzParam( 1 );
      if( gz )
         hb_retni( gzsetparams( gz, hb_parni( 2 ), hb_parni( 3 ) ) );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZREAD( <pGZipStream>, <@cData>, [ <nLen> ] ) => <nResult>
 */
HB_FUNC( HB_GZREAD )
{
   PHB_ITEM pBuffer = ISBYREF( 2 ) ? hb_param( 2, HB_IT_STRING ) : NULL;

   if( pBuffer )
   {
      gzFile gz = hb_gzParam( 1 );
      if( gz )
      {
         pBuffer = hb_itemUnShareString( pBuffer );
         hb_retni( gzread( gz, hb_itemGetCPtr( pBuffer ), ISNUM( 3 ) ?
                     ( ULONG ) hb_parnl( 3 ) : hb_itemGetCLen( pBuffer ) ) );
      }
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZWRITE( <pGZipStream>, <cData>, [ <nLen> ] ) => <nResult>
 */
HB_FUNC( HB_GZWRITE )
{
   char * szData = hb_parc( 2 );
   if( szData )
   {
      gzFile gz = hb_gzParam( 1 );
      if( gz )
         hb_retni( gzwrite( gz, szData, ISNUM( 3 ) ?
                            ( ULONG ) hb_parnl( 3 ) : hb_parclen( 2 ) ) );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZGETS( <pGZipStream>, <nMaxBytes> ) => <cLine> or NIL on error
 */
HB_FUNC( HB_GZGETS )
{
   int iLen = hb_parni( 2 );
   if( iLen > 0 )
   {
      gzFile gz = hb_gzParam( 1 );
      if( gz )
      {
         char * szBuffer = ( char * ) hb_xalloc( iLen + 1 );

         if( szBuffer )
         {
            if( gzgets( gz, szBuffer, iLen ) != Z_NULL )
               hb_retc_buffer( szBuffer );
            else
               hb_xfree( szBuffer );
         }
      }
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZPUTS( <pGZipStream>, <cData> ) => <nResult>
 */
HB_FUNC( HB_GZPUTS )
{
   char * szData = hb_parc( 2 );
   if( szData )
   {
      gzFile gz = hb_gzParam( 1 );
      if( gz )
         hb_retni( gzputs( gz, szData ) );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZGETC( <pGZipStream> ) => <nByte>
 */
HB_FUNC( HB_GZGETC )
{
   gzFile gz = hb_gzParam( 1 );
   if( gz )
      hb_retni( gzgetc( gz ) );
}

/*
 * HB_GZUNGETC( <nByte>, <pGZipStream> ) => <nByte>
 */
HB_FUNC( HB_GZUNGETC )
{
   if( ISNUM( 1 ) )
   {
      gzFile gz = hb_gzParam( 2 );
      if( gz )
         hb_retni( gzungetc( hb_parni( 1 ), gz ) );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZFLUSH( <pGZipStream>, [ <nFlush> ] ) => <nResult>
 */
HB_FUNC( HB_GZFLUSH )
{
   gzFile gz = hb_gzParam( 1 );
   if( gz )
      hb_retni( gzflush( gz, ISNUM( 2 ) ? hb_parni( 2 ) : Z_SYNC_FLUSH ) );
}

/*
 * HB_GZSEEK( <pGZipStream>, <nOffset>, [ <nWhence> ] ) => <nOffset>
 */
HB_FUNC( HB_GZSEEK )
{
   if( ISNUM( 2 ) )
   {
      gzFile gz = hb_gzParam( 1 );
      if( gz )
         hb_retnint( gzseek( gz, ( z_off_t ) hb_parnint( 2 ), ISNUM( 3 ) ?
                             hb_parni( 3 ) : SEEK_SET ) );
   }
   else
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/*
 * HB_GZREWIND( <pGZipStream> ) => <nResult>
 */
HB_FUNC( HB_GZREWIND )
{
   gzFile gz = hb_gzParam( 1 );
   if( gz )
      hb_retni( gzrewind( gz ) );
}

/*
 * HB_GZTELL( <pGZipStream> ) => <lResult>
 */
HB_FUNC( HB_GZTELL )
{
   gzFile gz = hb_gzParam( 1 );
   if( gz )
      hb_retnint( gztell( gz ) );
}

/*
 * HB_GZEOF( <pGZipStream> ) => <lResult>
 */
HB_FUNC( HB_GZEOF )
{
   gzFile gz = hb_gzParam( 1 );
   if( gz )
      hb_retl( gzeof( gz ) != 0 );
}

#if ZLIB_VERNUM >= 0x1230
/*
 * HB_GZDIRECT( <pGZipStream> ) => <lResult>
 */
HB_FUNC( HB_GZDIRECT )
{
   gzFile gz = hb_gzParam( 1 );
   if( gz )
      hb_retl( gzdirect( gz ) != 0 );
}
#endif

/*
 * HB_GZERROR( <pGZipStream>, [ <@nError> ] ) => <cError>
 */
HB_FUNC( HB_GZERROR )
{
   gzFile gz = hb_gzParam( 1 );
   if( gz )
   {
      int iErrNum = 0;

      hb_retc( gzerror( gz, &iErrNum ) );
      hb_storni( iErrNum, 2 );
   }
}

/*
 * HB_GZCLEARERR( <pGZipStream> ) => NIL
 */
HB_FUNC( HB_GZCLEARERR )
{
   gzFile gz = hb_gzParam( 1 );
   if( gz )
      gzclearerr( gz );
}
