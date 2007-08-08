/*
 * $Id$
 */

/*
 * File......: COLOR2N.C
 * Author....: David Richardson
 * CIS ID....: 72271,53
 *
 * This function is an original work by David Richardson and is placed in the
 * public domain.
 *
 * Modification history:
 * ---------------------
 *
 *    Rev 1.1   22 Apr 2004 15:44:00   DGH
 * Fixed compiler warnings about pointer vs. integer by changing NULL to 0.
 * Commented out #ifdef and #endif lines, because there is nothing that is
 * even remotely DOS- or Windows-specific in the code.
 *    Rev 1.0   01 Jan 1995 03:01:00   TED
 * Initial release
 *
 */


/*  $DOC$
 *  $FUNCNAME$
 *     FT_COLOR2N()
 *  $CATEGORY$
 *     String
 *  $ONELINER$
 *     Returns the numeric complement of a Clipper color string
 *  $SYNTAX$
 *     FT_COLOR2N( <cColor> ) -> nValue
 *  $ARGUMENTS$
 *     <cColor> is a Clipper color string
 *  $RETURNS$
 *     The numeric complement of a color string or 0 if passed color
 *     is invalid.
 *  $DESCRIPTION$
 *     This function is useful when calling other functions that expect
 *     a numeric color parameter.  It is often more convenient to pass
 *     a converted color string than having to calculate or look up the
 *     corresponding number.
 *  $EXAMPLES$
 *     nColor := FT_COLOR2N( "gr+/b" )         // returns 30
 *
 *     FT_SETATTR( 0, 0, 10, 10, nColor )
 *  $SEEALSO$
 *     FT_N2COLOR()
 *  $END$
 */

#include "hbapi.h"
#include "hbapigt.h"

HB_FUNC( FT_COLOR2N )
{
   int iRet = 0;

   if( ISCHAR( 1 ) )
      iRet = hb_gtColorToN( hb_parc( 1 ) );

   hb_retni( iRet );
}
