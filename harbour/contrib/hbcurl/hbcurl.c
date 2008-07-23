/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * libcurl 'easy' API - Harbour interface.
 *
 * Copyright 2008 Viktor Szakats <harbour.01 syenar.hu>
 * originally based on:
 * Copyright 2005 Luiz Rafael Culik Guimaraes <luiz at xharbour.com.br>
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

/* NOTE: Harbour requires libcurl 7.17.0 or upper.
         This was the version where curl_easy_setopt() started to
         make copies of passed strings, which we currently require.
         [vszakats] */

#include "curl/curl.h"
#include "curl/types.h"
#include "curl/easy.h"
#include "curl/multi.h"

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbapifs.h"
#include "hbvm.h"
#include "hbhash.h"

#include "hbcurl.ch"

#define HB_CURL_OPT_BOOL( n )      ( ISLOG( n ) ? ( long ) hb_parl( n ) : ( ISNUM( n ) ? hb_parnl( n ) : 1 ) )
#define HB_CURL_OPT_LARGENUM( n )  ( ( curl_off_t ) hb_parnint( n ) )

#if LIBCURL_VERSION_NUM < 0x071100
   /* #error hbcurl: Harbour hbcurl needs libcurl 7.17.0 or upper. */
#  ifndef HB_CURL_HASH_STRINGS
#     define HB_CURL_HASH_STRINGS
#  endif
#endif

typedef struct _HB_CURL
{
   CURL * curl;

   struct curl_httppost * pHTTPPOST_First;
   struct curl_httppost * pHTTPPOST_Last;
   struct curl_slist *    pHTTPHEADER;
   struct curl_slist *    pHTTP200ALIASES;
   struct curl_slist *    pQUOTE;
   struct curl_slist *    pPOSTQUOTE;
   struct curl_slist *    pPREQUOTE;
   struct curl_slist *    pTELNETOPTIONS;

   BYTE *  ul_name;
   FHANDLE ul_handle;

   BYTE *  dl_name;
   FHANDLE dl_handle;

   BYTE *  ul_ptr;
   size_t  ul_len;
   size_t  ul_pos;

   BYTE *  dl_ptr;
   size_t  dl_len;
   size_t  dl_pos;

   PHB_ITEM pProgressBlock;

#ifdef HB_CURL_HASH_STRINGS
   HB_HASH_TABLE_PTR pHash;
#endif

} HB_CURL, * PHB_CURL;

#ifdef HB_CURL_HASH_STRINGS

/* functions to keep passed string values accessible even if HVM
 * destroy them. It's necessary for old CURL versions which do not
 * make own copy of passed strings
 */
#define HB_CURL_HASH_TABLE_SIZE    509UL

static const char * hb_curl_StrHash( PHB_CURL hb_curl, const char * szValue )
{
   char * szHash;

   szHash = ( char * ) hb_hashTableFind( hb_curl->pHash, ( void * ) szValue );
   if( !szHash )
   {
      szHash = hb_strdup( szValue );
      hb_hashTableAdd( hb_curl->pHash, ( void * ) szHash, ( void * ) szHash );
   }
   return szHash;
}

/* returns a hash key */
static HB_HASH_FUNC( hb_curl_HashKey )    /* ULONG func (void *Value, void *Cargo) */
{
   ULONG ulSum = 0;
   char *szName = ( char * )Value;
   
   while( *szName )
     ulSum += *szName++;

   HB_SYMBOL_UNUSED( HashPtr );
   HB_SYMBOL_UNUSED( Cargo );

   return ulSum % HB_CURL_HASH_TABLE_SIZE;
}

/* deletes an identifier */
static HB_HASH_FUNC( hb_curl_HashDel )
{
   hb_xfree( Value );
   HB_SYMBOL_UNUSED( HashPtr );
   HB_SYMBOL_UNUSED( Cargo );
   return 1;
}

/* compares two identifiers */
static HB_HASH_FUNC( hb_curl_HashCmp )
{
   HB_SYMBOL_UNUSED( HashPtr );
   return strcmp( (char *)Value, (char *)Cargo );
}

#else

#define hb_curl_StrHash( c, s )     (s)

#endif /* HB_CURL_HASH_STRINGS */

/* ---------------------------------------------------------------------------- */
/* Global initialization/deinitialization */

void * hb_curl_xgrab( size_t size )
{
   return hb_xgrab( size );
}

void hb_curl_xfree( void * p )
{
   hb_xfree( p );
}

void * hb_curl_xrealloc( void * p, size_t size )
{
   return hb_xrealloc( p, size );
}

char * hb_curl_strdup( const char * s )
{
   return hb_strdup( s );
}

void * hb_curl_calloc( size_t nelem, size_t elsize )
{
   size_t size = nelem * elsize;
   void * ptr = hb_xgrab( size );

   memset( ptr, '\0', size );

   return ptr;
}

HB_FUNC( CURL_GLOBAL_INIT )
{
   hb_retnl( ( long ) curl_global_init_mem( ISNUM( 1 ) ? hb_parnl( 1 ) : CURL_GLOBAL_ALL,
                                            hb_curl_xgrab,
                                            hb_curl_xfree,
                                            hb_curl_xrealloc,
                                            hb_curl_strdup,
                                            hb_curl_calloc ) );
}

HB_FUNC( CURL_GLOBAL_CLEANUP )
{
   curl_global_cleanup();
}

/* ---------------------------------------------------------------------------- */
/* Callbacks */

size_t hb_curl_read_dummy_callback( void * buffer, size_t size, size_t nmemb, void * Cargo )
{
   HB_SYMBOL_UNUSED( buffer );
   HB_SYMBOL_UNUSED( size );
   HB_SYMBOL_UNUSED( nmemb );
   HB_SYMBOL_UNUSED( Cargo );

   return 0;
}

size_t hb_curl_read_file_callback( void * buffer, size_t size, size_t nmemb, void * Cargo )
{
   PHB_CURL hb_curl = ( PHB_CURL ) Cargo;

   if( hb_curl )
   {
      size_t ret;

      if( hb_curl->ul_handle == FS_ERROR )
      {
         hb_curl->ul_handle = hb_fsOpen( hb_curl->ul_name, FO_READ );

         if( hb_curl->ul_handle == FS_ERROR )
            return ( size_t ) -1;
      }

      ret = ( size_t ) hb_fsReadLarge( hb_curl->ul_handle, ( BYTE * ) buffer, size * nmemb );

      return hb_fsError() ? CURL_READFUNC_ABORT : ret;
   }

   return ( size_t ) -1;
}

size_t hb_curl_read_buff_callback( void * buffer, size_t size, size_t nmemb, void * Cargo )
{
   PHB_CURL hb_curl = ( PHB_CURL ) Cargo;

   if( hb_curl )
   {
      size_t nTodo = size * nmemb;
      size_t nLeft = hb_curl->ul_len - hb_curl->ul_pos;

      if( nTodo > nLeft )
         nTodo = nLeft;

      hb_xmemcpy( buffer, hb_curl->ul_ptr + hb_curl->ul_pos, nTodo );

      hb_curl->ul_pos += nTodo;

      return nTodo;
   }

   return ( size_t ) -1;
}

size_t hb_curl_write_file_callback( void * buffer, size_t size, size_t nmemb, void * Cargo )
{
   PHB_CURL hb_curl = ( PHB_CURL ) Cargo;

   if( hb_curl )
   {
      if( hb_curl->dl_handle == FS_ERROR )
      {
         hb_curl->dl_handle = hb_fsCreate( hb_curl->dl_name, FC_NORMAL );

         if( hb_curl->dl_handle == FS_ERROR )
            return ( size_t ) -1;
      }

      return hb_fsWriteLarge( hb_curl->dl_handle, ( BYTE * ) buffer, size * nmemb );
   }

   return ( size_t ) -1;
}

#define HB_CURL_DL_BUFF_SIZE_INIT ( CURL_MAX_WRITE_SIZE * 2 )
#define HB_CURL_DL_BUFF_SIZE_INCR ( CURL_MAX_WRITE_SIZE * 4 )

size_t hb_curl_write_buff_callback( void * buffer, size_t size, size_t nmemb, void * Cargo )
{
   PHB_CURL hb_curl = ( PHB_CURL ) Cargo;

   if( hb_curl )
   {
      size_t nTodo = size * nmemb;
      size_t nLeft = hb_curl->dl_len - hb_curl->dl_pos;

      if( nTodo > nLeft )
      {
         hb_curl->dl_len += HB_CURL_DL_BUFF_SIZE_INCR;
         hb_curl->dl_ptr = ( BYTE * ) hb_xrealloc( hb_curl->dl_ptr, hb_curl->dl_len );
      }

      hb_xmemcpy( hb_curl->dl_ptr + hb_curl->dl_pos, buffer, nTodo );

      hb_curl->dl_pos += nTodo;

      return nTodo;
   }

   return ( size_t ) -1;
}

int hb_curl_progress_callback( void * Cargo, double dltotal, double dlnow, double ultotal, double ulnow )
{
   if( Cargo )
   {
      PHB_ITEM p1 = hb_itemPutND( NULL, ulnow   > 0 ? ulnow   : dlnow   );
      PHB_ITEM p2 = hb_itemPutND( NULL, ultotal > 0 ? ultotal : dltotal );

      BOOL bResult = hb_itemGetL( hb_vmEvalBlockV( ( PHB_ITEM ) Cargo, 2, p1, p2 ) );

      hb_itemRelease( p1 );
      hb_itemRelease( p2 );

      if( bResult )
         return 1; /* Abort */
   }

   return 0;
}

/* ---------------------------------------------------------------------------- */
/* Helpers */

static void hb_curl_form_free( struct curl_httppost ** ptr )
{
   if( ptr && * ptr )
   {
      curl_formfree( * ptr );
      * ptr = NULL;
   }
}

static void hb_curl_slist_free( struct curl_slist ** ptr )
{
   if( ptr && * ptr )
   {
      curl_slist_free_all( * ptr );
      * ptr = NULL;
   }
}

static void hb_curl_file_ul_free( PHB_CURL hb_curl )
{
   if( hb_curl && hb_curl->ul_name )
   {
      hb_xfree( hb_curl->ul_name );
      hb_curl->ul_name = NULL;

      if( hb_curl->ul_handle != FS_ERROR )
      {
         hb_fsClose( hb_curl->ul_handle );
         hb_curl->ul_handle = FS_ERROR;
      }
   }
}

static void hb_curl_file_dl_free( PHB_CURL hb_curl )
{
   if( hb_curl && hb_curl->dl_name )
   {
      hb_xfree( hb_curl->dl_name );
      hb_curl->dl_name = NULL;

      if( hb_curl->dl_handle != FS_ERROR )
      {
         hb_fsClose( hb_curl->dl_handle );
         hb_curl->dl_handle = FS_ERROR;
      }
   }
}

static void hb_curl_buff_ul_free( PHB_CURL hb_curl )
{
   if( hb_curl && hb_curl->ul_ptr )
   {
      hb_xfree( hb_curl->ul_ptr );
      hb_curl->ul_ptr = NULL;
      hb_curl->ul_len = 0;
      hb_curl->ul_pos = 0;
   }
}

static void hb_curl_buff_dl_free( PHB_CURL hb_curl )
{
   if( hb_curl && hb_curl->dl_ptr )
   {
      hb_xfree( hb_curl->dl_ptr );
      hb_curl->dl_ptr = NULL;
      hb_curl->dl_len = 0;
      hb_curl->dl_pos = 0;
   }
}

/* ---------------------------------------------------------------------------- */
/* Constructor/Destructor */

static void PHB_CURL_free( PHB_CURL hb_curl, BOOL bFree )
{
   curl_easy_setopt( hb_curl->curl, CURLOPT_READFUNCTION, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_READDATA, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_WRITEFUNCTION, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_WRITEDATA, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_PROGRESSFUNCTION, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_PROGRESSDATA, NULL );

   /* Some extra safety. Set these to NULL, before freeing their pointers. */
   curl_easy_setopt( hb_curl->curl, CURLOPT_HTTPPOST, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_HTTPHEADER, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_HTTP200ALIASES, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_QUOTE, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_POSTQUOTE, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_PREQUOTE, NULL );
   curl_easy_setopt( hb_curl->curl, CURLOPT_TELNETOPTIONS, NULL );

   hb_curl_form_free( &hb_curl->pHTTPPOST_First );
   hb_curl_form_free( &hb_curl->pHTTPPOST_Last );
   hb_curl_slist_free( &hb_curl->pHTTPHEADER );
   hb_curl_slist_free( &hb_curl->pHTTP200ALIASES );
   hb_curl_slist_free( &hb_curl->pQUOTE );
   hb_curl_slist_free( &hb_curl->pPOSTQUOTE );
   hb_curl_slist_free( &hb_curl->pPREQUOTE );
   hb_curl_slist_free( &hb_curl->pTELNETOPTIONS );

   hb_curl_file_ul_free( hb_curl );
   hb_curl_file_dl_free( hb_curl );

   hb_curl_buff_ul_free( hb_curl );
   hb_curl_buff_dl_free( hb_curl );

   if( hb_curl->pProgressBlock )
   {
      hb_itemRelease( hb_curl->pProgressBlock );
      hb_curl->pProgressBlock = NULL;
   }
#ifdef HB_CURL_HASH_STRINGS
   if( hb_curl->pHash )
   {
      hb_hashTableKill( hb_curl->pHash );
      hb_curl->pHash = NULL;
   }
#endif

   if( bFree )
   {
      curl_easy_cleanup( hb_curl->curl );
      hb_xfree( hb_curl );
   }
   else
      curl_easy_reset( hb_curl->curl );
}

/* NOTE: Will create a new one. If 'from' is specified, the new one
         will be based on the 'from' one. */

static PHB_CURL PHB_CURL_create( CURL * from )
{
   CURL * curl = from ? curl_easy_duphandle( from ) : curl_easy_init();

   if( curl )
   {
      PHB_CURL hb_curl = ( PHB_CURL ) hb_xgrab( sizeof( HB_CURL ) );

      memset( ( void * ) hb_curl, 0, sizeof( HB_CURL ) );
      hb_curl->curl = curl;
#ifdef HB_CURL_HASH_STRINGS
      hb_curl->pHash = hb_hashTableCreate( HB_CURL_HASH_TABLE_SIZE,
                           hb_curl_HashKey, hb_curl_HashDel, hb_curl_HashCmp );
#endif

      return hb_curl;
   }
   else
      return NULL;
}

static HB_GARBAGE_FUNC( PHB_CURL_release )
{
   void ** ph = ( void ** ) Cargo;

   /* Check if pointer is not NULL to avoid multiple freeing */
   if( ph && * ph )
   {
      /* Destroy the object */
      PHB_CURL_free( ( PHB_CURL ) * ph, TRUE );

      /* set pointer to NULL to avoid multiple freeing */
      * ph = NULL;
   }
}

static void PHB_CURL_ret( PHB_CURL from )
{
   void ** ph = ( void ** ) hb_gcAlloc( sizeof( PHB_CURL ), PHB_CURL_release );

   * ph = ( void * ) PHB_CURL_create( from );

   hb_retptrGC( ph );
}

static void * PHB_CURL_is( int iParam )
{
   return hb_parptrGC( PHB_CURL_release, iParam );
}

static PHB_CURL PHB_CURL_par( int iParam )
{
   void ** ph = ( void ** ) hb_parptrGC( PHB_CURL_release, iParam );

   return ph ? ( PHB_CURL ) * ph : NULL;
}

/* ---------------------------------------------------------------------------- */
/* Harbour interface */

HB_FUNC( CURL_EASY_INIT )
{
   PHB_CURL_ret( NULL );
}

HB_FUNC( CURL_EASY_DUPLICATE )
{
   if( PHB_CURL_is( 1 ) )
      PHB_CURL_ret( PHB_CURL_par( 1 ) );
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

HB_FUNC( CURL_EASY_CLEANUP )
{
   if( PHB_CURL_is( 1 ) )
   {
      void ** ph = ( void ** ) hb_parptrGC( PHB_CURL_release, 1 );

      if( ph && * ph )
      {
         /* Destroy the object */
         PHB_CURL_free( ( PHB_CURL ) * ph, TRUE );

         /* set pointer to NULL to avoid multiple freeing */
         * ph = NULL;
      }
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#if LIBCURL_VERSION_NUM >= 0x070C01

HB_FUNC( CURL_EASY_RESET )
{
   if( PHB_CURL_is( 1 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );

      if( hb_curl )
         PHB_CURL_free( hb_curl, FALSE );
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#endif

#if LIBCURL_VERSION_NUM >= 0x071200

HB_FUNC( CURL_EASY_PAUSE )
{
   if( PHB_CURL_is( 1 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );

      hb_retnl( hb_curl ? ( long ) curl_easy_pause( hb_curl->curl, hb_parni( 2 ) ) : HB_CURLE_ERROR );
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#endif

HB_FUNC( CURL_EASY_PERFORM )
{
   if( PHB_CURL_is( 1 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );

      hb_retnl( hb_curl ? ( long ) curl_easy_perform( hb_curl->curl ) : HB_CURLE_ERROR );
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#if LIBCURL_VERSION_NUM >= 0x071202

/* NOTE: curl_easy_send( curl, cBuffer, @nSentBytes ) -> nResult */
HB_FUNC( CURL_EASY_SEND )
{
   if( PHB_CURL_is( 1 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );
      CURLcode res = ( CURLcode ) HB_CURLE_ERROR;

      if( hb_curl )
      {
         size_t size = 0;

         res = curl_easy_send( hb_curl->curl, ( void * ) hb_parcx( 2 ), ( size_t ) hb_parclen( 2 ), &size );

         hb_stornl( size, 3 );
      }

      hb_retnl( ( long ) res );
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/* NOTE: curl_easy_recv( curl, @cBuffer ) -> nResult */
HB_FUNC( CURL_EASY_RECV )
{
   if( PHB_CURL_is( 1 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );
      CURLcode res = ( CURLcode ) HB_CURLE_ERROR;

      if( hb_curl )
      {
         size_t size = ( size_t ) hb_parclen( 2 );
         void * buffer;

         if( size < 1024 )
            size = 1024;

         buffer = hb_xgrab( size );

         res = curl_easy_recv( hb_curl->curl, buffer, size, &size );

         if( ! hb_storclen_buffer( ( char * ) buffer, size, 2 ) )
            hb_xfree( buffer );
      }

      hb_retnl( ( long ) res );
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#endif

HB_FUNC( CURL_EASY_SETOPT )
{
   if( PHB_CURL_is( 1 ) && ISNUM( 2 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );
      CURLcode res = ( CURLcode ) HB_CURLE_ERROR;

      if( hb_curl )
      {
         switch( hb_parni( 2 ) )
         {
         /* Behavior */

         case HB_CURLOPT_VERBOSE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_VERBOSE, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_HEADER:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_HEADER, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_NOPROGRESS:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_NOPROGRESS, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_NOSIGNAL:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_NOSIGNAL, HB_CURL_OPT_BOOL( 3 ) );
            break;

         /* Callback */

         /* These are hidden on the Harbour level: */
         /* HB_CURLOPT_WRITEFUNCTION */
         /* HB_CURLOPT_WRITEDATA */
         /* HB_CURLOPT_READFUNCTION */
         /* HB_CURLOPT_READDATA */
#if LIBCURL_VERSION_NUM >= 0x070C03
         /* HB_CURLOPT_IOCTLFUNCTION */
         /* HB_CURLOPT_IOCTLDATA */
#endif
         /* HB_CURLOPT_SEEKFUNCTION */
         /* HB_CURLOPT_SEEKDATA */
         /* HB_CURLOPT_SOCKOPTFUNCTION */
         /* HB_CURLOPT_SOCKOPTDATA */
         /* HB_CURLOPT_OPENSOCKETFUNCTION */
         /* HB_CURLOPT_OPENSOCKETDATA */
         /* HB_CURLOPT_PROGRESSFUNCTION */
         /* HB_CURLOPT_PROGRESSDATA */
         /* HB_CURLOPT_HEADERFUNCTION */
         /* HB_CURLOPT_HEADERDATA / CURLOPT_WRITEHEADER */
         /* HB_CURLOPT_DEBUGFUNCTION */
         /* HB_CURLOPT_DEBUGDATA */
         /* HB_CURLOPT_SSL_CTX_FUNCTION */
         /* HB_CURLOPT_SSL_CTX_DATA */
         /* HB_CURLOPT_CONV_TO_NETWORK_FUNCTION */
         /* HB_CURLOPT_CONV_FROM_NETWORK_FUNCTION */
         /* HB_CURLOPT_CONV_FROM_UTF8_FUNCTION */

         /* Error */

         /* HB_CURLOPT_ERRORBUFFER */
         /* HB_CURLOPT_STDERR */

         case HB_CURLOPT_FAILONERROR:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FAILONERROR, HB_CURL_OPT_BOOL( 3 ) );
            break;

         /* Network */

         /* This is the only option that must be set before curl_easy_perform() is called. */
         case HB_CURLOPT_URL:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_URL, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_PROXY:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_PROXY, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_PROXYPORT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_PROXYPORT, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_PROXYTYPE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_PROXYTYPE, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_HTTPPROXYTUNNEL:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_HTTPPROXYTUNNEL, HB_CURL_OPT_BOOL( 3 ) );
            break;
/*
         case HB_CURLOPT_SOCKS5_RESOLVE_LOCAL:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SOCKS5_RESOLVE_LOCAL, HB_CURL_OPT_BOOL( 3 ) );
            break;
*/
         case HB_CURLOPT_INTERFACE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_INTERFACE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_LOCALPORT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_LOCALPORT, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_LOCALPORTRANGE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_LOCALPORTRANGE, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_DNS_CACHE_TIMEOUT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_DNS_CACHE_TIMEOUT, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_DNS_USE_GLOBAL_CACHE: /* OBSOLETE */
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_DNS_USE_GLOBAL_CACHE, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_BUFFERSIZE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_BUFFERSIZE, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_PORT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_PORT, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_TCP_NODELAY:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_TCP_NODELAY, HB_CURL_OPT_BOOL( 3 ) );
            break;

         /* Names and passwords options (Authentication) */

         case HB_CURLOPT_NETRC:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_NETRC, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_NETRC_FILE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_NETRC_FILE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_USERPWD:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_USERPWD, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_PROXYUSERPWD:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_PROXYUSERPWD, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_HTTPAUTH:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_HTTPAUTH, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_PROXYAUTH:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_PROXYAUTH, hb_parnl( 3 ) );
            break;

         /* HTTP options */

         case HB_CURLOPT_AUTOREFERER:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_AUTOREFERER, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_ENCODING:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_ENCODING, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_FOLLOWLOCATION:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FOLLOWLOCATION, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_UNRESTRICTED_AUTH:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_UNRESTRICTED_AUTH, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_MAXREDIRS:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_MAXREDIRS, hb_parnl( 3 ) );
            break;
#if LIBCURL_VERSION_NUM >= 0x071101
         case HB_CURLOPT_POST301:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_POST301, HB_CURL_OPT_BOOL( 3 ) );
            break;
#endif
         case HB_CURLOPT_PUT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_PUT, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_POST:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_POST, HB_CURL_OPT_BOOL( 3 ) );
            break;
#if LIBCURL_VERSION_NUM >= 0x071101
         case HB_CURLOPT_POSTFIELDS:
         case HB_CURLOPT_COPYPOSTFIELDS:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_COPYPOSTFIELDS, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
#endif
         case HB_CURLOPT_POSTFIELDSIZE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_POSTFIELDSIZE, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_POSTFIELDSIZE_LARGE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_POSTFIELDSIZE_LARGE, HB_CURL_OPT_LARGENUM( 3 ) );
            break;
         case HB_CURLOPT_HTTPPOST:
            {
               PHB_ITEM pArray = hb_param( 3, HB_IT_ARRAY );

               if( pArray )
               {
                  ULONG ulPos;
                  ULONG ulArrayLen = hb_arrayLen( pArray );

                  for( ulPos = 0; ulPos < ulArrayLen; ulPos++ )
                  {
                     PHB_ITEM pSubArray = hb_arrayGetItemPtr( pArray, ulPos + 1 );

                     curl_formadd( &hb_curl->pHTTPPOST_First,
                                   &hb_curl->pHTTPPOST_Last,
                                   CURLFORM_COPYNAME, hb_curl_StrHash( hb_curl, hb_arrayGetCPtr( pSubArray, 1 ) ),
                                   CURLFORM_FILE, hb_curl_StrHash( hb_curl, hb_arrayGetCPtr( pSubArray, 2 ) ),
                                   CURLFORM_END );
                  }

                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_HTTPPOST, hb_curl->pHTTPPOST_First );
               }
            }
            break;
         case HB_CURLOPT_REFERER:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_REFERER, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_USERAGENT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_USERAGENT, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_HTTPHEADER:
            {
               PHB_ITEM pArray = hb_param( 3, HB_IT_ARRAY );

               curl_easy_setopt( hb_curl->curl, CURLOPT_HTTPHEADER, NULL );
               hb_curl_slist_free( &hb_curl->pHTTPHEADER );

               if( pArray )
               {
                  ULONG ulPos;
                  ULONG ulArrayLen = hb_arrayLen( pArray );

                  for( ulPos = 0; ulPos < ulArrayLen; ulPos++ )
                     hb_curl->pHTTPHEADER = curl_slist_append( hb_curl->pHTTPHEADER, hb_curl_StrHash( hb_curl, hb_arrayGetCPtr( pArray, ulPos + 1 ) ) );

                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_HTTPHEADER, hb_curl->pHTTPHEADER );
               }
            }
            break;
         case HB_CURLOPT_HTTP200ALIASES:
            {
               PHB_ITEM pArray = hb_param( 3, HB_IT_ARRAY );

               curl_easy_setopt( hb_curl->curl, CURLOPT_HTTP200ALIASES, NULL );
               hb_curl_slist_free( &hb_curl->pHTTP200ALIASES );

               if( pArray )
               {
                  ULONG ulPos;
                  ULONG ulArrayLen = hb_arrayLen( pArray );

                  for( ulPos = 0; ulPos < ulArrayLen; ulPos++ )
                     hb_curl->pHTTP200ALIASES = curl_slist_append( hb_curl->pHTTP200ALIASES, hb_curl_StrHash( hb_curl, hb_arrayGetCPtr( pArray, ulPos + 1 ) ) );

                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_HTTP200ALIASES, hb_curl->pHTTP200ALIASES );
               }
            }
            break;
         case HB_CURLOPT_COOKIE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_COOKIE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_COOKIEFILE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_COOKIEFILE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_COOKIEJAR:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_COOKIEJAR, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_COOKIESESSION:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_COOKIESESSION, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_COOKIELIST:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_COOKIELIST, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_HTTPGET:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_HTTPGET, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_HTTP_VERSION:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_HTTP_VERSION, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_IGNORE_CONTENT_LENGTH:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_IGNORE_CONTENT_LENGTH, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_HTTP_CONTENT_DECODING:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_IGNORE_CONTENT_LENGTH, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_HTTP_TRANSFER_DECODING:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_HTTP_TRANSFER_DECODING, HB_CURL_OPT_BOOL( 3 ) );
            break;

         /* FTP options */

         case HB_CURLOPT_FTPPORT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTPPORT, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_QUOTE:
            {
               PHB_ITEM pArray = hb_param( 3, HB_IT_ARRAY );

               curl_easy_setopt( hb_curl->curl, CURLOPT_QUOTE, NULL );
               hb_curl_slist_free( &hb_curl->pQUOTE );

               if( pArray )
               {
                  ULONG ulPos;
                  ULONG ulArrayLen = hb_arrayLen( pArray );

                  for( ulPos = 0; ulPos < ulArrayLen; ulPos++ )
                     hb_curl->pQUOTE = curl_slist_append( hb_curl->pQUOTE, hb_curl_StrHash( hb_curl, hb_arrayGetCPtr( pArray, ulPos + 1 ) ) );

                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_QUOTE, hb_curl->pQUOTE );
               }
            }
            break;
         case HB_CURLOPT_POSTQUOTE:
            {
               PHB_ITEM pArray = hb_param( 3, HB_IT_ARRAY );

               curl_easy_setopt( hb_curl->curl, CURLOPT_POSTQUOTE, NULL );
               hb_curl_slist_free( &hb_curl->pPOSTQUOTE );

               if( pArray )
               {
                  ULONG ulPos;
                  ULONG ulArrayLen = hb_arrayLen( pArray );

                  for( ulPos = 0; ulPos < ulArrayLen; ulPos++ )
                     hb_curl->pPOSTQUOTE = curl_slist_append( hb_curl->pPOSTQUOTE, hb_curl_StrHash( hb_curl, hb_arrayGetCPtr( pArray, ulPos + 1 ) ) );

                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_POSTQUOTE, hb_curl->pPOSTQUOTE );
               }
            }
            break;
         case HB_CURLOPT_PREQUOTE:
            {
               PHB_ITEM pArray = hb_param( 3, HB_IT_ARRAY );

               curl_easy_setopt( hb_curl->curl, CURLOPT_PREQUOTE, NULL );
               hb_curl_slist_free( &hb_curl->pPREQUOTE );

               if( pArray )
               {
                  ULONG ulPos;
                  ULONG ulArrayLen = hb_arrayLen( pArray );

                  for( ulPos = 0; ulPos < ulArrayLen; ulPos++ )
                     hb_curl->pQUOTE = curl_slist_append( hb_curl->pPREQUOTE, hb_curl_StrHash( hb_curl, hb_arrayGetCPtr( pArray, ulPos + 1 ) ) );

                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_PREQUOTE, hb_curl->pPREQUOTE );
               }
            }
            break;
         case HB_CURLOPT_DIRLISTONLY: /* HB_CURLOPT_FTPLISTONLY */
#if LIBCURL_VERSION_NUM > 0x071004
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_DIRLISTONLY, HB_CURL_OPT_BOOL( 3 ) );
#else
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTPLISTONLY, HB_CURL_OPT_BOOL( 3 ) );
#endif
            break;
         case HB_CURLOPT_APPEND: /* HB_CURLOPT_FTPAPPEND */
#if LIBCURL_VERSION_NUM > 0x071004
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_APPEND, HB_CURL_OPT_BOOL( 3 ) );
#else
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTPAPPEND, HB_CURL_OPT_BOOL( 3 ) );
#endif
            break;
         case HB_CURLOPT_FTP_USE_EPRT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_USE_EPRT, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_FTP_USE_EPSV:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_USE_EPSV, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_FTP_CREATE_MISSING_DIRS:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_CREATE_MISSING_DIRS, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_FTP_RESPONSE_TIMEOUT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_RESPONSE_TIMEOUT, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_FTP_ALTERNATIVE_TO_USER:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_ALTERNATIVE_TO_USER, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_FTP_SKIP_PASV_IP:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_SKIP_PASV_IP, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_USE_SSL: /* HB_CURLOPT_FTP_SSL */
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_SSL, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_FTPSSLAUTH:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTPSSLAUTH, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_FTP_SSL_CCC:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_SSL_CCC, hb_parnl( 3 ) );
            break;
#if LIBCURL_VERSION_NUM >= 0x070C03
         case HB_CURLOPT_FTP_ACCOUNT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_ACCOUNT, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
#endif
         case HB_CURLOPT_FTP_FILEMETHOD:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FTP_FILEMETHOD, hb_parnl( 3 ) );
            break;

         /* Protocol */

         case HB_CURLOPT_TRANSFERTEXT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_TRANSFERTEXT, HB_CURL_OPT_BOOL( 3 ) );
            break;
#if LIBCURL_VERSION_NUM >= 0x071200
         case HB_CURLOPT_PROXY_TRANSFER_MODE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_PROXY_TRANSFER_MODE, HB_CURL_OPT_BOOL( 3 ) );
            break;
#endif
         case HB_CURLOPT_CRLF:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_CRLF, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_RANGE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_RANGE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_RESUME_FROM:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_RESUME_FROM, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_RESUME_FROM_LARGE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_RESUME_FROM_LARGE, HB_CURL_OPT_LARGENUM( 3 ) );
            break;
         case HB_CURLOPT_CUSTOMREQUEST:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_CUSTOMREQUEST, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_FILETIME:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FILETIME, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_NOBODY:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_NOBODY, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_INFILESIZE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_INFILESIZE, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_INFILESIZE_LARGE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_INFILESIZE_LARGE, HB_CURL_OPT_LARGENUM( 3 ) );
            break;
         case HB_CURLOPT_UPLOAD:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_UPLOAD, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_DOWNLOAD: /* Harbour extension */
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_UPLOAD, ! HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_MAXFILESIZE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_MAXFILESIZE, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_MAXFILESIZE_LARGE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_MAXFILESIZE_LARGE, HB_CURL_OPT_LARGENUM( 3 ) );
            break;
         case HB_CURLOPT_TIMECONDITION:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_TIMECONDITION, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_TIMEVALUE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_TIMEVALUE, hb_parnl( 3 ) );
            break;

         /* Connection */

         case HB_CURLOPT_TIMEOUT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_TIMEOUT, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_TIMEOUT_MS:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_TIMEOUT_MS, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_LOW_SPEED_LIMIT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_LOW_SPEED_LIMIT, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_LOW_SPEED_TIME:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_LOW_SPEED_TIME, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_MAX_SEND_SPEED_LARGE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_MAX_SEND_SPEED_LARGE, HB_CURL_OPT_LARGENUM( 3 ) );
            break;
         case HB_CURLOPT_MAX_RECV_SPEED_LARGE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_MAX_RECV_SPEED_LARGE, HB_CURL_OPT_LARGENUM( 3 ) );
            break;
         case HB_CURLOPT_MAXCONNECTS:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_MAXCONNECTS, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_CLOSEPOLICY: /* OBSOLETE, does nothing. */
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_CLOSEPOLICY, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_FRESH_CONNECT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FRESH_CONNECT, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_FORBID_REUSE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_FORBID_REUSE, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_CONNECTTIMEOUT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_CONNECTTIMEOUT, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_CONNECTTIMEOUT_MS:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_CONNECTTIMEOUT_MS, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_IPRESOLVE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_IPRESOLVE, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_CONNECT_ONLY:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_CONNECT_ONLY, HB_CURL_OPT_BOOL( 3 ) );
            break;

         /* SSL and Security */

         case HB_CURLOPT_SSLCERT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSLCERT, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_SSLCERTTYPE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSLCERTTYPE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_SSLKEY:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSLKEY, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_SSLKEYTYPE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSLKEYTYPE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_KEYPASSWD:
#if LIBCURL_VERSION_NUM > 0x071004
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_KEYPASSWD, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
#elif LIBCURL_VERSION_NUM > 0x070902
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSLKEYPASSWD, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
#else
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSLCERTPASSWD, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
#endif
            break;
         case HB_CURLOPT_SSLENGINE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSLENGINE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_SSLENGINE_DEFAULT:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSLENGINE_DEFAULT, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_SSLVERSION:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSLVERSION, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_SSL_VERIFYPEER:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSL_VERIFYPEER, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_CAINFO:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_CAINFO, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_CAPATH:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_CAPATH, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_RANDOM_FILE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_RANDOM_FILE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_EGDSOCKET:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_EGDSOCKET, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_SSL_VERIFYHOST:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSL_VERIFYHOST, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_SSL_CIPHER_LIST:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSL_CIPHER_LIST, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_SSL_SESSIONID_CACHE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSL_VERIFYHOST, HB_CURL_OPT_BOOL( 3 ) );
            break;
         case HB_CURLOPT_KRBLEVEL: /* HB_CURLOPT_KRB4LEVEL */
#if LIBCURL_VERSION_NUM > 0x071003
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_KRBLEVEL, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
#else
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_KRB4LEVEL, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
#endif
            break;

         /* SSH options */

         case HB_CURLOPT_SSH_AUTH_TYPES:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSH_AUTH_TYPES, hb_parnl( 3 ) );
            break;
#if LIBCURL_VERSION_NUM >= 0x071101
         case HB_CURLOPT_SSH_HOST_PUBLIC_KEY_MD5:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
#endif
         case HB_CURLOPT_SSH_PUBLIC_KEYFILE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSH_PUBLIC_KEYFILE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;
         case HB_CURLOPT_SSH_PRIVATE_KEYFILE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_SSH_PRIVATE_KEYFILE, hb_curl_StrHash( hb_curl, hb_parc( 3 ) ) );
            break;

         /* Other options */

         case HB_CURLOPT_PRIVATE:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_PRIVATE, hb_parptr( 3 ) );
            break;

         /* HB_CURLOPT_SHARE */

#if LIBCURL_VERSION_NUM >= 0x071004
         case HB_CURLOPT_NEW_FILE_PERMS:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_NEW_FILE_PERMS, hb_parnl( 3 ) );
            break;
         case HB_CURLOPT_NEW_DIRECTORY_PERMS:
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_NEW_DIRECTORY_PERMS, hb_parnl( 3 ) );
            break;
#endif

         /* Telnet options */

         case HB_CURLOPT_TELNETOPTIONS:
            {
               PHB_ITEM pArray = hb_param( 3, HB_IT_ARRAY );

               curl_easy_setopt( hb_curl->curl, CURLOPT_TELNETOPTIONS, NULL );
               hb_curl_slist_free( &hb_curl->pTELNETOPTIONS );

               if( pArray )
               {
                  ULONG ulPos;
                  ULONG ulArrayLen = hb_arrayLen( pArray );

                  for( ulPos = 0; ulPos < ulArrayLen; ulPos++ )
                     hb_curl->pTELNETOPTIONS = curl_slist_append( hb_curl->pTELNETOPTIONS, hb_curl_StrHash( hb_curl, hb_arrayGetCPtr( pArray, ulPos + 1 ) ) );

                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_TELNETOPTIONS, hb_curl->pTELNETOPTIONS );
               }
            }
            break;

         /* Undocumented */

         /* HB_CURLOPT_WRITEINFO */

         /* Harbour specials */

         case HB_CURLOPT_PROGRESSBLOCK:
            {
               PHB_ITEM pProgressBlock = hb_param( 3, HB_IT_BLOCK );

               if( hb_curl->pProgressBlock )
               {
                  curl_easy_setopt( hb_curl->curl, CURLOPT_PROGRESSFUNCTION, NULL );
                  curl_easy_setopt( hb_curl->curl, CURLOPT_PROGRESSDATA, NULL );

                  hb_itemRelease( hb_curl->pProgressBlock );
                  hb_curl->pProgressBlock = NULL;
               }

               if( pProgressBlock )
               {
                  hb_curl->pProgressBlock = hb_itemNew( pProgressBlock );

                  curl_easy_setopt( hb_curl->curl, CURLOPT_PROGRESSFUNCTION, hb_curl_progress_callback );
                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_PROGRESSDATA, ( void * ) hb_curl->pProgressBlock );
               }
            }
            break;

         case HB_CURLOPT_UL_FILE_SETUP:
            {
               hb_curl_file_ul_free( hb_curl );

               if( ISCHAR( 3 ) )
               {
                  hb_curl->ul_name = ( BYTE * ) hb_strdup( hb_parc( 3 ) );
                  hb_curl->ul_handle = FS_ERROR;

                  curl_easy_setopt( hb_curl->curl, CURLOPT_READFUNCTION, hb_curl_read_file_callback );
                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_READDATA, ( void * ) hb_curl );
               }
            }
            break;

         case HB_CURLOPT_UL_FILE_CLOSE:
            hb_curl_file_ul_free( hb_curl );
            res = CURLE_OK;
            break;

         case HB_CURLOPT_DL_FILE_SETUP:
            {
               hb_curl_file_dl_free( hb_curl );

               if( ISCHAR( 3 ) )
               {
                  hb_curl->dl_name = ( BYTE * ) hb_strdup( hb_parc( 3 ) );
                  hb_curl->dl_handle = FS_ERROR;

                  curl_easy_setopt( hb_curl->curl, CURLOPT_WRITEFUNCTION, hb_curl_write_file_callback );
                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_WRITEDATA, ( void * ) hb_curl );
               }
            }
            break;

         case HB_CURLOPT_DL_FILE_CLOSE:
            hb_curl_file_dl_free( hb_curl );
            res = CURLE_OK;
            break;

         case HB_CURLOPT_UL_BUFF_SETUP:
            {
               hb_curl_buff_ul_free( hb_curl );

               if( ISCHAR( 3 ) )
               {
                  hb_curl->ul_pos = 0;
                  hb_curl->ul_len = hb_parclen( 3 );
                  hb_curl->ul_ptr = ( BYTE * ) hb_xgrab( hb_curl->ul_len );

                  hb_xmemcpy( hb_curl->ul_ptr, hb_parc( 3 ), hb_curl->ul_len );

                  curl_easy_setopt( hb_curl->curl, CURLOPT_READFUNCTION, hb_curl_read_buff_callback );
                  res = curl_easy_setopt( hb_curl->curl, CURLOPT_READDATA, ( void * ) hb_curl );
               }
            }
            break;

         case HB_CURLOPT_DL_BUFF_SETUP:
            {
               hb_curl_buff_dl_free( hb_curl );

               hb_curl->dl_pos = 0;
               hb_curl->dl_len = ISNUM( 3 ) ? hb_parnl( 3 ) : HB_CURL_DL_BUFF_SIZE_INIT;
               hb_curl->dl_ptr = ( BYTE * ) hb_xgrab( hb_curl->dl_len );

               curl_easy_setopt( hb_curl->curl, CURLOPT_WRITEFUNCTION, hb_curl_write_buff_callback );
               res = curl_easy_setopt( hb_curl->curl, CURLOPT_WRITEDATA, ( void * ) hb_curl );
            }
            break;

         case HB_CURLOPT_DL_BUFF_GET:
            hb_storclen( ( char * ) hb_curl->dl_ptr, hb_curl->dl_pos, 3 );
            if( hb_curl->dl_ptr )
               res = CURLE_OK;
            break;

         case HB_CURLOPT_UL_NULL_SETUP:
            hb_curl_file_ul_free( hb_curl );
            hb_curl_buff_ul_free( hb_curl );

            curl_easy_setopt( hb_curl->curl, CURLOPT_READFUNCTION, hb_curl_read_dummy_callback );
            res = curl_easy_setopt( hb_curl->curl, CURLOPT_READDATA, ( void * ) hb_curl );
            break;
         }
      }

      hb_retnl( ( long ) res );
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/* Harbour extension. */
HB_FUNC( CURL_EASY_DL_BUFF_GET )
{
   if( PHB_CURL_is( 1 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );

      if( hb_curl )
         hb_retclen( ( char * ) hb_curl->dl_ptr, hb_curl->dl_pos );
      else
         hb_retc_null();
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#define HB_CURL_INFO_TYPE_INVALID       0
#define HB_CURL_INFO_TYPE_STR           1
#define HB_CURL_INFO_TYPE_PTR           2
#define HB_CURL_INFO_TYPE_LONG          3
#define HB_CURL_INFO_TYPE_DOUBLE        4
#define HB_CURL_INFO_TYPE_SLIST         5

#define HB_CURL_EASY_GETINFO( hb_curl, n, p ) \
        ( hb_curl ? curl_easy_getinfo( hb_curl->curl, n, p ) : ( CURLcode ) HB_CURLE_ERROR )

/* NOTE: curl_easy_getinfo( curl, x, @nError ) -> xValue */
HB_FUNC( CURL_EASY_GETINFO )
{
   if( PHB_CURL_is( 1 ) && ISNUM( 2 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );
      CURLcode res = ( CURLcode ) HB_CURLE_ERROR;

      int type = HB_CURL_INFO_TYPE_INVALID;

      char * ret_string = NULL;
      char * ret_ptr = NULL;
      long ret_long = 0;
      struct curl_slist * ret_slist = NULL;
      double ret_double = 0.0;

      switch( hb_parni( 2 ) )
      {
      case HB_CURLINFO_EFFECTIVE_URL:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_EFFECTIVE_URL, &ret_string );
         type = HB_CURL_INFO_TYPE_STR;
         break;
      case HB_CURLINFO_RESPONSE_CODE:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_RESPONSE_CODE, &ret_long );
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_HTTP_CONNECTCODE:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_HTTP_CONNECTCODE, &ret_long );
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_FILETIME:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_FILETIME, &ret_long );
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_TOTAL_TIME:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_TOTAL_TIME, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_NAMELOOKUP_TIME:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_NAMELOOKUP_TIME, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_CONNECT_TIME:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_CONNECT_TIME, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_PRETRANSFER_TIME:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_PRETRANSFER_TIME, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_STARTTRANSFER_TIME:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_STARTTRANSFER_TIME, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_REDIRECT_TIME:
#if LIBCURL_VERSION_NUM >= 0x070907
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_REDIRECT_TIME, &ret_double );
#endif
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_REDIRECT_COUNT:
#if LIBCURL_VERSION_NUM >= 0x070907
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_REDIRECT_COUNT, &ret_long );
#endif
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_REDIRECT_URL:
#if LIBCURL_VERSION_NUM >= 0x071202
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_REDIRECT_URL, &ret_string );
#endif
         type = HB_CURL_INFO_TYPE_STR;
         break;
      case HB_CURLINFO_SIZE_UPLOAD:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_SIZE_UPLOAD, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_SIZE_DOWNLOAD:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_SIZE_DOWNLOAD, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_SPEED_DOWNLOAD:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_SPEED_DOWNLOAD, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_SPEED_UPLOAD:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_SPEED_UPLOAD, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_HEADER_SIZE:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_HEADER_SIZE, &ret_long );
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_REQUEST_SIZE:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_REQUEST_SIZE, &ret_long );
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_SSL_VERIFYRESULT:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_SSL_VERIFYRESULT, &ret_long );
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_SSL_ENGINES:
#if LIBCURL_VERSION_NUM >= 0x071203
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_SSL_ENGINES, &ret_slist );
#endif
         type = HB_CURL_INFO_TYPE_SLIST;
         break;
      case HB_CURLINFO_CONTENT_LENGTH_DOWNLOAD:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_CONTENT_LENGTH_UPLOAD:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_CONTENT_LENGTH_UPLOAD, &ret_double );
         type = HB_CURL_INFO_TYPE_DOUBLE;
         break;
      case HB_CURLINFO_CONTENT_TYPE:
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_CONTENT_TYPE, &ret_string );
         type = HB_CURL_INFO_TYPE_STR;
         break;
      case HB_CURLINFO_PRIVATE:
#if LIBCURL_VERSION_NUM >= 0x070A03
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_PRIVATE, &ret_ptr );
#endif
         type = HB_CURL_INFO_TYPE_PTR;
         break;
      case HB_CURLINFO_HTTPAUTH_AVAIL:
#if LIBCURL_VERSION_NUM >= 0x070A08
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_HTTPAUTH_AVAIL, &ret_long );
#endif
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_PROXYAUTH_AVAIL:
#if LIBCURL_VERSION_NUM >= 0x070A08
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_PROXYAUTH_AVAIL, &ret_long );
#endif
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_OS_ERRNO:
#if LIBCURL_VERSION_NUM >= 0x070C02
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_OS_ERRNO, &ret_long );
#endif
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_NUM_CONNECTS:
#if LIBCURL_VERSION_NUM >= 0x070C03
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_NUM_CONNECTS, &ret_long );
#endif
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_COOKIELIST:
#if LIBCURL_VERSION_NUM >= 0x070E01
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_COOKIELIST, &ret_slist );
#endif
         type = HB_CURL_INFO_TYPE_SLIST;
         break;
      case HB_CURLINFO_LASTSOCKET:
#if LIBCURL_VERSION_NUM >= 0x070F02
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_LASTSOCKET, &ret_long );
#endif
         type = HB_CURL_INFO_TYPE_LONG;
         break;
      case HB_CURLINFO_FTP_ENTRY_PATH:
#if LIBCURL_VERSION_NUM >= 0x070F04
         res = HB_CURL_EASY_GETINFO( hb_curl, CURLINFO_FTP_ENTRY_PATH, &ret_string );
#endif
         type = HB_CURL_INFO_TYPE_STR;
         break;
      }

      switch( type )
      {
      case HB_CURL_INFO_TYPE_STR:
         hb_retc( ret_string );
         break;
      case HB_CURL_INFO_TYPE_PTR:
         hb_retptr( ( void * ) ret_ptr );
         break;
      case HB_CURL_INFO_TYPE_LONG:
         hb_retnl( ret_long );
         break;
      case HB_CURL_INFO_TYPE_DOUBLE:
         hb_retnd( ret_double );
         break;
      case HB_CURL_INFO_TYPE_SLIST:
         {
            if( ret_slist )
            {
               PHB_ITEM pArray;
               int nCount;
               struct curl_slist * walk_ret_slist;

               /* Count */
               for( walk_ret_slist = ret_slist, nCount = 0; walk_ret_slist->next; nCount++ )
                  walk_ret_slist = walk_ret_slist->next;

               /* Fill */
               pArray = hb_itemArrayNew( nCount );
               for( walk_ret_slist = ret_slist, nCount = 1; walk_ret_slist->next; )
               {
                  hb_arraySetC( pArray, nCount++, walk_ret_slist->data );
                  walk_ret_slist = walk_ret_slist->next;
               }
               hb_itemReturnRelease( pArray );

               curl_slist_free_all( ret_slist );
            }
            else
               hb_reta( 0 );
         }
         break;
      }

      hb_stornl( ( long ) res, 3 );
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#if LIBCURL_VERSION_NUM >= 0x070F04

HB_FUNC( CURL_EASY_ESCAPE )
{
   if( PHB_CURL_is( 1 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );

      if( hb_curl )
      {
         char * buffer = curl_easy_escape( hb_curl->curl, hb_parcx( 2 ), hb_parclen( 2 ) );
         hb_retc( buffer );
         curl_free( buffer );
      }
      else
         hb_retc_null();
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

HB_FUNC( CURL_EASY_UNESCAPE )
{
   if( PHB_CURL_is( 1 ) )
   {
      PHB_CURL hb_curl = PHB_CURL_par( 1 );

      if( hb_curl )
      {
         int nLen = 0;
         char * buffer = curl_easy_unescape( hb_curl->curl, hb_parcx( 2 ), hb_parclen( 2 ), &nLen );
         hb_retclen( buffer, nLen );
         curl_free( buffer );
      }
      else
         hb_retc_null();
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#endif

/* ---------------------------------------------------------------------------- */
/* Harbour interface (session independent) */

#if 0

/* NOTE: Obsolete, superceded by curl_easy_escape() */
HB_FUNC( CURL_ESCAPE )
{
   if( ISCHAR( 1 ) )
   {
      char * buffer = curl_escape( hb_parc( 1 ), hb_parclen( 1 ) );
      hb_retc( buffer );
      curl_free( buffer );
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

/* NOTE: Obsolete, superceded by curl_easy_unescape() */
HB_FUNC( CURL_UNESCAPE )
{
   if( ISCHAR( 1 ) )
   {
      char * buffer = curl_unescape( hb_parc( 1 ), hb_parclen( 1 ) );
      hb_retc( buffer );
      curl_free( buffer );
   }
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#endif

HB_FUNC( CURL_VERSION )
{
   hb_retc( curl_version() );
}

HB_FUNC( CURL_VERSION_INFO )
{
   curl_version_info_data * data = curl_version_info( CURLVERSION_NOW );

   if( data )
   {
      PHB_ITEM pArray = hb_itemArrayNew( 13 );

      hb_arraySetC(  pArray,  1, data->version );                     /* LIBCURL_VERSION */
      hb_arraySetNI( pArray,  2, data->version_num );                 /* LIBCURL_VERSION_NUM */
      hb_arraySetC(  pArray,  3, data->host );                        /* OS/host/cpu/machine when configured */
      hb_arraySetNI( pArray,  4, data->features );                    /* bitmask, see defines below */
      hb_arraySetC(  pArray,  5, data->ssl_version );                 /* human readable string */
      hb_arraySetNI( pArray,  6, data->ssl_version_num );             /* not used anymore, always 0 */
      hb_arraySetC(  pArray,  7, data->libz_version );                /* human readable string */
      hb_arraySetC(  pArray,  9, data->age >= CURLVERSION_SECOND ? data->ares : NULL );
      hb_arraySetNI( pArray, 10, data->age >= CURLVERSION_SECOND ? data->ares_num : 0 );
      hb_arraySetC(  pArray, 11, data->age >= CURLVERSION_THIRD  ? data->libidn : NULL );
      hb_arraySetNI( pArray, 12, data->age >= CURLVERSION_FOURTH ? data->iconv_ver_num : 0 ); /* Same as '_libiconv_version' if built with HAVE_ICONV */
      hb_arraySetC(  pArray, 13, data->age >= CURLVERSION_FOURTH ? data->libssh_version : NULL ); /* human readable string */

      {
         PHB_ITEM pProtocols;
         int nCount = 0;
         const char * const * prot = data->protocols;

         while( *( prot++ ) )
            nCount++;

         pProtocols = hb_arrayGetItemPtr( pArray, 8 );
         hb_arrayNew( pProtocols, nCount );

         for( prot = data->protocols, nCount = 1; *prot; prot++ )
            hb_arraySetC( pProtocols, nCount++, *prot );
      }

      hb_itemReturnRelease( pArray );
   }
   else
      hb_reta( 0 );
}

#if LIBCURL_VERSION_NUM >= 0x070C00

HB_FUNC( CURL_EASY_STRERROR )
{
   if( ISNUM( 1 ) )
      hb_retc( curl_easy_strerror( ( CURLcode ) hb_parnl( 1 ) ) );
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

HB_FUNC( CURL_SHARE_STRERROR )
{
   if( ISNUM( 1 ) )
      hb_retc( curl_share_strerror( ( CURLSHcode ) hb_parnl( 1 ) ) );
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

HB_FUNC( CURL_MULTI_STRERROR )
{
   if( ISNUM( 1 ) )
      hb_retc( curl_multi_strerror( ( CURLMcode ) hb_parnl( 1 ) ) );
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}

#endif

/* NOTE: This returns the number of seconds since January 1st 1970 in the UTC time zone. */
HB_FUNC( CURL_GETDATE )
{
   if( ISCHAR( 1 ) )
      hb_retnint( ( HB_LONG ) curl_getdate( hb_parc( 1 ), NULL ) );
   else
      hb_errRT_BASE( EG_ARG, 2010, NULL, HB_ERR_FUNCNAME, HB_ERR_ARGS_BASEPARAMS );
}
