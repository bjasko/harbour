/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * DELIMITED RDD module
 *
 * Copyright 1999 Bruno Cantero <bruno@issnet.net>
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

#define SUPERTABLE ( &delimSuper )

#include "hbapi.h"
#include "hbinit.h"
#include "hbapirdd.h"
#include "rddsys.ch"

HARBOUR HB__DELIMC( void );
HARBOUR HB_DELIM_GETFUNCTABLE( void );

HB_INIT_SYMBOLS_BEGIN( delim1__InitSymbols )
{ "_DELIMC",            _HB_FS_PUBLIC, HB__DELIMC,            0 },
{ "DELIM_GETFUNCTABLE", _HB_FS_PUBLIC, HB_DELIM_GETFUNCTABLE, 0 }
HB_INIT_SYMBOLS_END( delim1__InitSymbols )
#if ! defined(__GNUC__) && ! defined(_MSC_VER)
   #pragma startup delim1__InitSymbols
#endif

static RDDFUNCS delimSuper = { 0 };

/*
 * -- DELIM METHODS --
 */

static RDDFUNCS delimTable = { 0 };

HARBOUR HB__DELIMC( void )
{
}

HARBOUR HB_DELIM_GETFUNCTABLE( void )
{
   RDDFUNCS * pTable;
   USHORT * uiCount;

   uiCount = ( USHORT * ) hb_parnl( 1 );
   * uiCount = RDDFUNCSCOUNT;
   pTable = ( RDDFUNCS * ) hb_parnl( 2 );
   if( pTable )
      hb_retni( hb_rddInherit( pTable, &delimTable, &delimSuper, 0 ) );
   else
      hb_retni( FAILURE );
}

