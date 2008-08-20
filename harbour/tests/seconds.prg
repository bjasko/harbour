//
// $Id$
//

/* Test SECONDS() */
/* Harbour Project source code
   http://www.harbour-project.org/
   Donated to the public domain on 2001-03-08 by David G. Holm <dholm@jsd-llc.com>
*/

function Main( cParam )
local n, limit := 10
local cNewLine := HB_OSNewLine()

   IF ! EMPTY( cParam )
      limit := VAL( cParam )
   ENDIF
   OUTSTD( cNewLine )
   OUTSTD( SECONDS() )
   FOR n := 1 TO limit
      IF EMPTY( cParam )
         OUTSTD( cNewLine )
         OUTSTD( "Pause: " )
         INKEY(0)
      ENDIF
      OUTSTD( cNewLine )
      OUTSTD( SECONDS() )
   NEXT

RETURN NIL
