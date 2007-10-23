/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * FT_IAMIDLE()
 *
 * Copyright 1999-2007 Viktor Szakats <viktor.szakats@syenar.hu>
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
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
*/

/* File......: ADAPTER.ASM
* Author....: Ted Means
* CIS ID....: 73067,3332
*
* This is an original work by Ted Means and is placed in the
* public domain.
*
* Modification history:
* ---------------------
*
*     Rev 1.0   01 Jan 1995 03:01:00   TED
*  Nanforum Toolkit
*
*/

/* $DOC$
*  $FUNCNAME$
*     FT_IAmIdle()
*  $CATEGORY$
*     DOS/BIOS
*  $ONELINER$
*     Inform the operating system that the application is idle.
*  $SYNTAX$
*     FT_IAmIdle() -> lSuccess
*  $ARGUMENTS$
*     None
*  $RETURNS$
*     .T. if supported, .F. otherwise.
*  $DESCRIPTION$
*     Some multitasking operating environments (e.g. Windows or OS/2) can
*     function more efficiently when applications release the CPU during
*     idle states.  This function allows you "announce" to the operating
*     system that your application is idle.
*
*     Note that if you use this function in conjunction with FT_OnIdle(),
*     you can cause Clipper to automatically release the CPU whenever
*     Clipper itself detects an idle state.
*  $EXAMPLES$
*     while inkey() != K_ESC
*        FT_IAmIdle()         // Wait for ESC and announce idleness
*     end
*
*     * Here's another way to do it:
*
*     FT_OnIdle( {|| FT_IAmIdle()} )
* 
*     Inkey( 0 )              // Automatically reports idleness until key
*                             // is pressed!
*  $SEEALSO$
*     FT_OnIdle()
*  $END$
*/

#include "hbapi.h"

HB_FUNC( FT_IAMIDLE )
{
   hb_releaseCPU();
}
