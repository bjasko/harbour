/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The FileSys API (Harbour level)
 *
 * Copyright 1999 Manuel Ruiz <mrt@joca.es>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version, with one exception:
 *
 * The exception is that if you link the Harbour Runtime Library (HRL)
 * and/or the Harbour Virtual Machine (HVM) with other files to produce
 * an executable, this does not by itself cause the resulting executable
 * to be covered by the GNU General Public License. Your use of that
 * executable is in no way restricted on account of linking the HRL
 * and/or HVM code into it.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA (or visit
 * their web site at http://www.gnu.org/).
 *
 */

/*
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    CURDIR()
 *
 * Copyright 2000 David G. Holm <dholm@jsd-llc.com>
 *    HB_F_EOF()
 *
 * See doc/license.txt for licensing terms.
 *
 */

#include <ctype.h>

#include "hbapi.h"
#include "hbapifs.h"
#include "hbapierr.h"
#include "hbapiitm.h"
#include "hbset.h"
#include "hbstack.h"

HB_FUNC( FOPEN )
{
   if( ISCHAR( 1 ) )
      hb_retni( hb_fsOpen( ( BYTE * ) hb_parc( 1 ),
                           ISNUM( 2 ) ? hb_parni( 2 ) : FO_READ | FO_COMPAT ) );
   else
      hb_errRT_BASE( EG_ARG, 2021, NULL, "FOPEN", 2, hb_paramError( 1 ), hb_paramError( 2 ) ); /* NOTE: Undocumented but existing Clipper Run-time error */
}

HB_FUNC( FCREATE )
{
   if( ISCHAR( 1 ) )
      hb_retni( hb_fsCreate( ( BYTE * ) hb_parc( 1 ),
                             ISNUM( 2 ) ? hb_parni( 2 ) : FC_NORMAL ) );
   else
      hb_retni( FS_ERROR );
}

#ifdef HB_EXTENSION

HB_FUNC( HB_FCREATE )
{
   if( ISCHAR( 1 ) )
      hb_retni( hb_fsCreateEx( ( BYTE * ) hb_parc( 1 ),
                               ISNUM( 2 ) ? hb_parni( 2 ) : FC_NORMAL,
                               ISNUM( 3 ) ? hb_parni( 3 ) : FO_COMPAT ) );
   else
      hb_retni( FS_ERROR );
}

#endif

HB_FUNC( FREAD )
{
   ULONG ulRead;

   if( ISNUM( 1 ) && ISCHAR( 2 ) && ISBYREF( 2 ) && ISNUM( 3 ) )
   {
      PHB_ITEM pItem = hb_itemUnRef( hb_stackItemFromBase( 2 ) );

      if( pItem->item.asString.bStatic ||
          ( * pItem->item.asString.u.pulHolders ) > 1 )
         /*
          * Warning: Don't use hb_itemPutC() here, as it fails if 1 byte buffer used
          * cause 1 byte length strings optimization
         */
         hb_itemPutCPtr( pItem, hb_strdup( hb_parc( 2 ) ), hb_parclen( 2 ) );

      ulRead = hb_parnl( 3 );

      /* NOTE: CA-Clipper determines the maximum size by calling _parcsiz()
               instead of _parclen(), this means that the maximum read length
               will be one more than the length of the passed buffer, because
               the terminating zero could be used if needed. [vszakats] */

      if( ulRead <= hb_parcsiz( 2 ) )
         /* NOTE: Warning, the read buffer will be directly modified,
                  this is normal here ! [vszakats] */
         ulRead = hb_fsReadLarge( hb_parni( 1 ),
                                  ( BYTE * ) hb_parc( 2 ),
                                  ulRead );
      else
         ulRead = 0;
   }
   else
      ulRead = 0;

   hb_retnl( ulRead );
}

HB_FUNC( FWRITE )
{
   if( ISNUM( 1 ) && ISCHAR( 2 ) )
      hb_retnl( hb_fsWriteLarge( hb_parni( 1 ),
                                 ( BYTE * ) hb_parc( 2 ),
                                 ISNUM( 3 ) ? hb_parnl( 3 ) : hb_parclen( 2 ) ) );
   else
      hb_retnl( 0 );
}

HB_FUNC( FERROR )
{
   hb_retni( hb_fsError() );
}

HB_FUNC( FCLOSE )
{
   hb_fsSetError( 0 );

   if( ISNUM( 1 ) )
   {
      hb_fsClose( hb_parni( 1 ) );
      hb_retl( hb_fsError() == 0 );
   }
   else
      hb_retl( FALSE );
}

HB_FUNC( FERASE )
{
   hb_fsSetError( 3 );

   hb_retni( ( ISCHAR( 1 ) &&
               hb_fsDelete( ( BYTE * ) hb_parc( 1 ) ) ) ? 0 : -1 );
}

HB_FUNC( FRENAME )
{
   hb_fsSetError( 2 );

   hb_retni( ( ISCHAR( 1 ) && ISCHAR( 2 ) &&
               hb_fsRename( ( BYTE * ) hb_parc( 1 ), ( BYTE * ) hb_parc( 2 ) ) ) ? 0 : -1 );
}

HB_FUNC( FSEEK )
{
   if( ISNUM( 1 ) && ISNUM( 2 ) )
      hb_retnl( hb_fsSeek( hb_parni( 1 ),
                           hb_parnl( 2 ),
                           ISNUM( 3 ) ? hb_parni( 3 ) : FS_SET ) );
   else
      hb_retnl( 0 );
}

HB_FUNC( FREADSTR )
{
   if( ISNUM( 1 ) && ISNUM( 2 ) )
   {
      ULONG ulToRead = ( ULONG ) hb_parnl( 2 );

      if( ulToRead > 0 )
      {
         FHANDLE fhnd = ( FHANDLE ) hb_parni( 1 );
         BYTE * buffer = ( BYTE * ) hb_xgrab( ulToRead + 1 );
         ULONG ulRead;

         ulRead = hb_fsReadLarge( fhnd, buffer, ulToRead );
         buffer[ ulRead ] = '\0';

         /* NOTE: Clipper will not return zero chars from this functions. */

         hb_retc_buffer( ( char * ) buffer );
      }
      else
         hb_retc( NULL );
   }
   else
      hb_retc( NULL );
}

/* NOTE: This function should not return the leading and trailing */
/*       (back)slashes. [vszakats] */

/* TODO: Xbase++ is able to change to the specified directory. */

HB_FUNC( CURDIR )
{
   BYTE byBuffer[ _POSIX_PATH_MAX + 1 ];
   USHORT uiErrorOld = hb_fsError();

   hb_fsCurDirBuff( ( ISCHAR( 1 ) && hb_parclen( 1 ) > 0 ) ?
      ( USHORT )( toupper( *hb_parc( 1 ) ) - 'A' + 1 ) : 0, byBuffer, _POSIX_PATH_MAX + 1 );

   hb_retc( ( char * ) byBuffer );

   hb_fsSetError( uiErrorOld );
}

#ifdef HB_EXTENSION

HB_FUNC( HB_F_EOF )
{
   if( ISNUM( 1 ) )
   {
      hb_retl( hb_fsEof( hb_parni( 1 ) ) );
   }
   else
   {
      hb_fsSetError( FS_ERROR );
      hb_retl( TRUE );
   }
}

HB_FUNC( HB_OSPATHSEPARATOR )
{
   char ret[2];
   ret[1] = 0;
   ret[0] = OS_PATH_DELIMITER;

   hb_retc( ret );
}

HB_FUNC( HB_OSPATHLISTSEPARATOR )
{
   char ret[2];
   ret[1] = 0;
   ret[0] = OS_PATH_LIST_SEPARATOR;

   hb_retc( ret );
}

HB_FUNC( HB_OSPATHDELIMITERS )
{
   hb_retc( OS_PATH_DELIMITER_LIST );
}


#endif
