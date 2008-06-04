/*
 * $Id$
 */

/*
 * GT CLIPPER STANDARD HEADER
 *
 * File......: chrfirst.c
 * Author....: Andy M Leighton
 * BBS.......: The Dark Knight Returns
 * Net/Node..: 050/069
 * User Name.: Andy Leighton
 * Date......: 23/05/93
 * Revision..: 1.00
 *
 * This is an original work by Andy Leighton and is placed in the
 * public domain.
 */

#include "hbapi.h"

HB_FUNC( GT_CHRFIRST )
{
  char *string;
  char *cset;
  int l1, l2;
  int p1, p2;

  if (ISCHAR(1) && ISCHAR(2)) {
    string = hb_parc(2);
    cset   = hb_parc(1);
    l1     = hb_parclen(2);
    l2     = hb_parclen(1);
    p1     = p2 = 0;

    do {
      for (p2 = 0; (p2 < l2) && (cset[p2] != string[p1]); ++p2)
         ;
      if (p2 < l2) {
         hb_retni(string[p1]);
         break;
      }
    } while (p1++ < l1);

    if (p2 >= l2)
      hb_retni(0);

  } else {
    hb_retni(-1);               /* parameter mismatch - error NullStr */
  }
}
