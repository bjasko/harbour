/*
 * $Id$
 */

#include "extend.h"

#ifdef DOS
#include <dos.h>
#include <dir.h>
#include <bios.h>
#endif

HARBOUR HB_CD( void )
{
#ifdef __DOS__
   hb_retni( ISCHAR( 1 ) ? chdir( hb_parc( 1 ) ) : 0 );
#else
   hb_retni( 0 );
#endif
}

HARBOUR HB_MD(void)
{
#ifdef __DOS__
   hb_retni( ISCHAR( 1 ) ? mkdir( hb_parc( 1 ) ) : 0 );
#else
   hb_retni( 0 );
#endif
}

HARBOUR HB_RD( void )
{
#ifdef __DOS__
   hb_retni( ISCHAR( 1 ) ? rmdir( hb_parc( 1 ) ) : 0 );
#else
   hb_retni( 0 );
#endif
}

HARBOUR HB_DISKUSED( void )
{
#ifdef DOS
   struct diskfree_t disk;
   long bytsfree, bytsfull;

   _dos_getdiskfree( 0, &disk );

   bytsfree = ( long ) disk.avail_clusters *
              ( long ) disk.sectors_per_cluster *
              ( long ) disk.bytes_per_sector;
   bytsfull = ( long ) disk.total_clusters *
              ( long ) disk.sectors_per_cluster *
              ( long ) disk.bytes_per_sector;

   hb_retnl( bytsfull - bytsfree );
#else
   hb_retnl( 0 );
#endif
}

HARBOUR HB_DISKFREE( void )
{
#ifdef DOS
   struct diskfree_t disk;

   _dos_getdiskfree( 0, &disk );

   hb_retnl( ( long ) disk.avail_clusters *
             ( long ) disk.sectors_per_cluster *
             ( long ) disk.bytes_per_sector );
#else
   hb_retnl( 0 );
#endif
}

HARBOUR HB_DISKFULL( void )
{
#ifdef DOS
   struct diskfree_t disk;

   _dos_getdiskfree( 0, &disk );

   hb_retnl( ( long ) disk.total_clusters *
             ( long ) disk.sectors_per_cluster *
             ( long ) disk.bytes_per_sector );
#else
   hb_retnl( 0 );
#endif
}
