/*
 * $Id$
 */

/*
 * GT CLIPPER STANDARD HEADER
 *
 * File......: strcount.c
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

HB_FUNC( GT_STRCOUNT )
{
  char *s1, *s2;
  int count, p1, p2, l1, l2;
  int match;

  if (ISCHAR(1) && ISCHAR(2)) {
    s1  = hb_parc(1);
    s2  = hb_parc(2);
    l1  = hb_parclen(1);
    l2  = hb_parclen(2);

    /* loop through s2 matching passed character (s1) with
       each character of s1 */

    for (count = 0, p2 = 0; p2 <= (l2 - l1); p2++)
    {
      match = 1;

      for (p1 = 0; p1 < l1; p1++)
      {
        if (s1[p1] != s2[p2 + p1])
          match = 0;
      }

      if (match)
        count++;
    }

    hb_retni(count);                  /* return result */
  } else {
    hb_retni(-1);                     /* parameter mismatch - error -1 */
  }
}
