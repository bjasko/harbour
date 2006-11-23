/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The cache for identifiers
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
 */

#include <string.h>

#include "hbhash.h"
#include "hbcomp.h"

#define HB_IDENT_TABLE_SIZE    509UL

/* create a new identifier or return the existing one 
*/
char * hb_compIdentifierNew( HB_COMP_DECL, char * szName, BOOL bCopy )
{
   char * szIdent;

   szIdent = ( char * ) hb_hashTableFind( HB_COMP_PARAM->pIdentifiers,
                                          ( void * ) szName );
   if( !szIdent )
   {
      if( bCopy )
         szIdent = hb_strdup( szName );
      else
         szIdent = szName;

      hb_hashTableAdd( HB_COMP_PARAM->pIdentifiers,
                       ( void * ) szIdent, ( void * ) szIdent );
   }
   else if( !bCopy )
      hb_xfree( szName );

   return szIdent;
}

/* returns a hash key */
static HB_HASH_FUNC( hb_comp_IdentKey )    /* ULONG func (void *Value, void *Cargo) */
{
   ULONG ulSum = 0;
   char *szName = ( char * )Value;
   
   while( *szName )
     ulSum += *szName++;

   HB_SYMBOL_UNUSED( HashPtr );
   HB_SYMBOL_UNUSED( Cargo );
     
   return ulSum % HB_IDENT_TABLE_SIZE;
}

/* deletes an identifier */
static HB_HASH_FUNC( hb_comp_IdentDel )
{
   hb_xfree( Value );
   HB_SYMBOL_UNUSED( HashPtr );
   HB_SYMBOL_UNUSED( Cargo );
   return 1;
}

/* compares two identifiers */
static HB_HASH_FUNC( hb_comp_IdentComp )
{
   HB_SYMBOL_UNUSED( HashPtr );
   return strcmp( (char *)Value, (char *)Cargo );
}

/* initialize the hash table for identifiers */
void hb_compIdentifierOpen( HB_COMP_DECL )
{
   HB_COMP_PARAM->pIdentifiers = hb_hashTableCreate( HB_IDENT_TABLE_SIZE,
                     hb_comp_IdentKey, hb_comp_IdentDel, hb_comp_IdentComp );
}

/* release identifiers table */
void hb_compIdentifierClose( HB_COMP_DECL )
{
   if( HB_COMP_PARAM->pIdentifiers )
   {
      hb_hashTableKill( HB_COMP_PARAM->pIdentifiers );
      HB_COMP_PARAM->pIdentifiers = NULL;
   }
}
