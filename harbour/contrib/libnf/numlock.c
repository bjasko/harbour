/*
 * $Id$
 */

/*
 * File......: NUMLOCK.C
 * Author....: Ted Means
 * CIS ID....: 73067,3332
 *
 * This function is an original work by Ted Means and is placed in the
 * public domain.
 *
 * Modification history:
 * ---------------------
 *
 *    Rev 1.3   15 Jul 1993 00:08:46   GLENN
 * Changed reference to status_byte in order to make this work in
 * protected mode.
 *
 *    Rev 1.2   15 Aug 1991 23:08:12   GLENN
 * Forest Belt proofread/edited/cleaned up doc
 *
 *    Rev 1.1   27 May 1991 14:43:20   GLENN
 * Ted added a parameter to toggle the Numlock on or off.
 *
 *    Rev 1.0   01 Apr 1991 01:02:50   GLENN
 * Nanforum Toolkit
 *
 *
 */




/*  $DOC$
 *  $FUNCNAME$
 *     FT_NUMLOCK()
 *  $CATEGORY$
 *     Keyboard/Mouse
 *  $ONELINER$
 *     Return status of NumLock key
 *  $SYNTAX$
 *     FT_NUMLOCK( [ <lNewSetting> ] ) -> lCurrentSetting
 *  $ARGUMENTS$
 *     <lNewSetting> is optional and if supplied is the new setting
 *     for the CapLock key.  Specify .T. to turn CapLock on, or .F. to
 *     turn it off.
 *  $RETURNS$
 *     lValue is .T. if NumLock is set, .F. if it isn't set.  The value
 *     returned represents the setting in effect prior to any changes that
 *     might by made by <lNewSetting>.
 *  $DESCRIPTION$
 *     This function is useful if you need to know or set the status of the
 *     NumLock key for some reason.
 *  $EXAMPLES$
 *     IF FT_NUMLOCK()
 *        Qout( "NumLock is active" )
 *     ENDIF
 *
 *   Another one, slightly strange, courtesy of Glenn Scott:
 *
 *
 *       function numBlink()
 *          local lOldNum := ft_numlock()
 *
 *          while inkey( .5 ) != 27
 *             ft_numlock( !ft_numlock() )
 *          end
 *
 *          return ft_numlock( lOldNum )
 *  $SEEALSO$
 *     FT_CAPLOCK() FT_CTRL() FT_PRTSCR() FT_SHIFT() FT_ALT()
 *  $END$
 */

#include "hbapigt.h"
#include "hbapiitm.h"

HB_FUNC( FT_NUMLOCK )
{
   int iState = 0, iNewState;
   HB_GT_INFO gtInfo;

   gtInfo.pNewVal = gtInfo.pResult = NULL;
   hb_gtInfo( GTI_KBDSHIFTS, &gtInfo );
   if( gtInfo.pResult )
   {
      iState = hb_itemGetNI( gtInfo.pResult );
      gtInfo.pNewVal = gtInfo.pResult;
      gtInfo.pResult = NULL;
   }

   if( ISLOG( 1 ) )
   {
      iNewState = hb_parl( 1 ) ? ( iState | GTI_KBD_NUMLOCK ) :
                                 ( iState & ~GTI_KBD_NUMLOCK );
      gtInfo.pNewVal = hb_itemPutNI( gtInfo.pNewVal, iNewState );
      hb_gtInfo( GTI_KBDSHIFTS, &gtInfo );
   }

   if( gtInfo.pNewVal )
      hb_itemRelease( gtInfo.pNewVal );
   if( gtInfo.pResult )
      hb_itemRelease( gtInfo.pResult );

   hb_retl( ( iState & GTI_KBD_NUMLOCK ) != 0 );
}
