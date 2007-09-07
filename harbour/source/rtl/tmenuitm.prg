/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * MENUITEM class
 *
 * Copyright 2000 Jose Lalin <dezac@corevia.com>
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

#include "hbclass.ch"

#include "button.ch"
#include "common.ch"

/* NOTE: Harbour doesn't support CA-Cl*pper 5.3 GUI functionality, but 
         it has all related variables and methods. */

#ifdef HB_COMPAT_C53

CREATE CLASS MENUITEM FUNCTION HBMenuItem

   EXPORT:

   VAR cargo

   METHOD caption( cCaption ) SETGET
   METHOD checked( lChecked ) SETGET
   METHOD data( boData ) SETGET
   METHOD enabled( lEnabled ) SETGET
   METHOD id( nID ) SETGET
   METHOD message( cMessage ) SETGET
   METHOD shortcut( nShortcut ) SETGET
   METHOD style( cStyle ) SETGET

   VAR col        INIT -1 AS NUMERIC                    /* NOTE: This is a Harbour extension. */
   VAR row        INIT -1 AS NUMERIC                    /* NOTE: This is a Harbour extension. */

   METHOD isPopUp()

   METHOD New( cCaption, boData, nShortcut, cMsg, nID ) /* NOTE: This method is a Harbour extension [vszakats] */

   PROTECTED:

   VAR cCaption   INIT ""
   VAR lChecked   INIT .F.
   VAR boData
   VAR lEnabled   INIT .T.
   VAR nID
   VAR cMessage
   VAR nShortcut
   VAR cStyle     INIT Chr( 251 ) + Chr( 16 )

ENDCLASS

METHOD caption( cCaption ) CLASS MENUITEM

   IF cCaption != NIL

      ::cCaption := _eInstVar( Self, "CAPTION", cCaption, "C", 1001 )

      IF ::cCaption == MENU_SEPARATOR
         ::boData   := NIL
         ::lChecked := .F.
         ::lEnabled := .F.
      ENDIF
   ENDIF

   RETURN ::cCaption

METHOD checked( lChecked ) CLASS MENUITEM

   IF lChecked != NIL .AND. !( ::cCaption == MENU_SEPARATOR )
      ::lChecked := _eInstVar( Self, "CHECKED", lChecked, "L", 1001 )
   ENDIF

   RETURN ::lChecked

METHOD data( boData ) CLASS MENUITEM

   IF boData != NIL
      IF ISBLOCK( boData )
         ::boData := boData
      ELSE
         ::boData := _eInstVar( Self, "DATA", boData, "O", 1001, {|| boData:ClassName() == "POPUPMENU" } )
      ENDIF
   ENDIF

   RETURN ::boData

METHOD enabled( lEnabled ) CLASS MENUITEM

   IF lEnabled != NIL .AND. !( ::cCaption == MENU_SEPARATOR )
      ::lEnabled := _eInstVar( Self, "ENABLED", lEnabled, "L", 1001 )
   ENDIF

   RETURN ::lEnabled

METHOD id( nID ) CLASS MENUITEM

   IF nID != NIL
      ::nID := _eInstVar( Self, "ID", nID, "N", 1001 )
   ENDIF

   RETURN ::nID

METHOD message( cMessage ) CLASS MENUITEM

   IF cMessage != NIL
      ::cMessage := _eInstVar( Self, "MESSAGE", cMessage, "C", 1001 )
   ENDIF

   RETURN ::cMessage

METHOD shortcut( nShortcut ) CLASS MENUITEM

   IF nShortcut != NIL
      ::nShortcut := _eInstVar( Self, "SHORTCUT", nShortcut, "N", 1001 )
   ENDIF

   RETURN ::nShortcut

METHOD style( cStyle ) CLASS MENUITEM

   IF cStyle != NIL
      ::cStyle := _eInstVar( Self, "STYLE", cStyle, "C", 1001, {|| Len( cStyle ) == 2 } )
   ENDIF

   RETURN ::cStyle

METHOD isPopUp() CLASS MENUITEM
   RETURN ISOBJECT( ::data ) .AND. ::data:ClassName() == "POPUPMENU"

METHOD New( cCaption, boData, nShortcut, cMessage, nID ) CLASS MENUITEM

   IF !ISNUMBER( nShortcut )
      nShortcut := 0
   ENDIF
   IF !ISCHARACTER( cMessage )
      cMessage := ""
   ENDIF
   IF !ISNUMBER( nID )
      nID := 0
   ENDIF

   ::data      := boData
   ::nID       := nID
   ::cMessage  := cMessage
   ::nShortcut := nShortcut
   ::caption   := cCaption

   RETURN Self

FUNCTION MenuItem( cCaption, boData, nShortcut, cMessage, nID )
   RETURN HBMenuItem():New( cCaption, boData, nShortcut, cMessage, nID )

#ifdef HB_C52_UNDOC

FUNCTION __miColumn( o, nColumn )

   IF ISOBJECT( o ) .AND. o:ClassName() == "MENUITEM"

      IF ISNUMBER( nColumn )
         o:col := nColumn
      ENDIF

      RETURN o:col
   ENDIF

   RETURN -1

FUNCTION __miRow( o, nRow )

   IF ISOBJECT( o ) .AND. o:ClassName() == "MENUITEM"

      IF ISNUMBER( nRow )
         o:row := nRow
      ENDIF

      RETURN o:row
   ENDIF

   RETURN -1

#endif

#endif
