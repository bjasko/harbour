/*
 * File......: PICKDAY.PRG
 * Author....: Greg Lief
 * CIS ID....: 72460,1760
 *
 * This is an original work by Mr. Grump and is placed in the
 * public domain.
 *
 * Modification history:
 * ---------------------
 *
 *    Rev 1.2   15 Aug 1991 23:04:24   GLENN
 * Forest Belt proofread/edited/cleaned up doc
 *
 *    Rev 1.1   14 Jun 1991 19:52:40   GLENN
 * Minor edit to file header
 *
 *    Rev 1.0   01 Apr 1991 01:02:00   GLENN
 * Nanforum Toolkit
 *
 */

/*  $DOC$
 *  $FUNCNAME$
 *     FT_PICKDAY()
 *  $CATEGORY$
 *     Menus/Prompts
 *  $ONELINER$
 *     Picklist of days of week
 *  $SYNTAX$
 *     FT_PICKDAY() -> cDayOfWeek
 *  $ARGUMENTS$
 *     None
 *  $RETURNS$
 *     Character string containing day of week
 *  $DESCRIPTION$
 *     This function is ideal if you need the user to select a day.
 *  $EXAMPLES$
 *     mday := FT_PICKDAY()
 *  $END$
 */

#include "box.ch"

// test code
#ifdef FT_TEST

FUNCTION MAIN
QOUT("You selected " + FT_PICKDAY())
return nil

#endif


function FT_PICKDAY
LOCAL DAYS := { "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", ;
                "FRIDAY", "SATURDAY" }, SEL := 0
LOCAL OLDSCRN := SAVESCREEN(8, 35, 16, 45), oldcolor := setcolor('+w/r')
@ 8, 35, 16, 45 box B_SINGLE + " "
/* do not allow user to Esc out, which would cause array access error */
do while sel = 0
   sel = achoice(9, 36, 15, 44, days)
enddo
/* restore previous screen contents and color */
restscreen(8, 35, 16, 45, oldscrn)
setcolor(oldcolor)
return days[sel]
