/*
 * $Id$
 */

/*
 * xHarbour Project source code:
 * HBXML - XML DOM oriented routines - Classes encapsulating the document
 *
 * Copyright 2003 Giancarlo Niccolai <gian@niccolai.ws>
 *    See also MXML library related copyright in hbxml.c
 *
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

#include "common.ch"
#include "hbxml.ch"
#include "hbclass.ch"

CLASS TXMLNode
   DATA nType
   DATA cName
   DATA aAttributes
   DATA nBeginLine
   DATA cData

   DATA oNext
   DATA oPrev
   DATA oParent
   DATA oChild

   METHOD New( nType, cName, aAttributes, cData ) CONSTRUCTOR
   METHOD Clone()                      INLINE HBXml_node_clone( Self )
   METHOD CloneTree()                  INLINE HBXml_node_clone_tree( Self )

   METHOD Unlink()                     INLINE HBXml_node_unlink( Self )
   METHOD NextInTree()

   METHOD InsertBefore( oNode )        INLINE HBXml_node_insert_before( Self, oNode )
   METHOD InsertAfter( oNode )         INLINE HBXml_node_insert_after( Self, oNode )
   METHOD InsertBelow( oNode )         INLINE HBXml_node_insert_below( Self, oNode )
   METHOD AddBelow( oNode )            INLINE HBXml_node_add_below( Self, oNode )

   METHOD GetAttribute( cAttrib )         INLINE IIF ( cAttrib $ ::aAttributes, ::aAttributes[ cAttrib ], NIL )
   METHOD SetAttribute( cAttrib, xValue ) INLINE ::aAttributes[ cAttrib ] := xValue

   METHOD Depth()
   METHOD Path()

   METHOD ToString( nStyle )        INLINE HBXml_node_to_string( Self, nStyle )
   METHOD Write( fHandle, nStyle )  INLINE HBXml_node_write( Self, fHandle, nStyle )

   //Useful for debugging purposes
   METHOD ToArray()                 INLINE;
             { ::nType, ::cName, ::aAttributes, ::cData }
ENDCLASS

METHOD New( nType, cName, aAttributes, cData ) class TXmlNode
   IF nType == NIL
      ::nType := HBXML_TYPE_TAG
   ELSE
      ::nType := nType
   ENDIF

   IF aAttributes == NIL
      ::aAttributes := {=>}
   ELSE
      ::aAttributes := aAttributes
   ENDIF

   ::cName := cName
   ::cData := cData
RETURN Self


METHOD NextInTree() CLASS TXmlNode
   LOCAL oNext := NIL, oTemp

   IF ::oChild != NIL
      oNext := ::oChild
   ELSEIF ::oNext != NIL
      oNext := ::oNext
   ELSE
      oTemp := ::oParent
      DO WHILE oTemp != NIL
         IF oTemp:oNext != NIL
            oNext := oTemp:oNext
            EXIT
         ENDIF
         oTemp := oTemp:oParent
      ENDDO
   ENDIF

RETURN oNext


METHOD Depth() CLASS TXmlNode
   IF ::oParent != NIL
      RETURN ::oParent:Depth() + 1
   ENDIF
RETURN 0


METHOD Path() CLASS TXmlNode
   IF ::nType == HBXML_TYPE_DOCUMENT
      RETURN ""
   ENDIF

   IF ::cName != NIL
      IF ::oParent != NIL
         IF ::oParent:Path() != NIL
            RETURN ::oParent:Path() + "/" + ::cName
         ENDIF
      ELSE
         RETURN "/" + ::cName
      ENDIF
   ENDIF
RETURN NIL

/********************************************
   Iterator class
*********************************************/
CLASS TXmlIterator
   METHOD New( oNodeTop )           CONSTRUCTOR
   METHOD Next()
   METHOD Rewind()                  INLINE   ::oNode := ::oTop
   METHOD Find( cName, cAttribute, cValue, cData )

   METHOD GetNode()                 INLINE   ::oNode
   METHOD SetContext()
   METHOD Clone()

PROTECTED:
   METHOD MatchCriteria()

   DATA cName
   DATA cAttribute
   DATA cValue
   DATA cData

HIDDEN:
   DATA nTopLevel

   DATA oNode
   DATA oTop

ENDCLASS


METHOD New( oNodeTop ) CLASS TXmlIterator
   ::oTop  := oNodeTop
   ::oNode := oNodeTop
   ::nTopLevel := oNodeTop:Depth()
RETURN Self


METHOD Clone() CLASS TXmlIterator
   LOCAL oRet

   oRet := TXmlIterator():New( ::oNodeTop )
   oRet:cName := ::cName
   oRet:cAttribute := ::cAttribute
   oRet:cValue := ::cValue
   oRet:cData := ::cData
RETURN oRet

METHOD SetContext() CLASS TXmlIterator
   ::oTop := ::oNode
RETURN Self

METHOD Find( cName, cAttribute, cValue, cData ) CLASS TXmlIterator
   ::cName := cName
   ::cAttribute := cAttribute
   ::cValue := cValue
   ::cData := cData

   IF ::oNode:nType == HBXML_TYPE_DOCUMENT
      IF ::oNode:oChild == NIL
         RETURN NIL
      ENDIF
      ::oNode := ::oNode:oChild
   ENDIF

   IF ::MatchCriteria( ::oNode )
      RETURN ::oNode
   ENDIF

RETURN ::Next()


METHOD Next() CLASS TXmlIterator
   LOCAL oFound := ::oNode:NextInTree()

   DO WHILE oFound != NIL
      IF oFound:Depth() <= ::nTopLevel
         RETURN NIL
      ENDIF

      IF ::MatchCriteria( oFound )
         ::oNode := oFound
         RETURN oFound
      ENDIF

      oFound := oFound:NextInTree()
   ENDDO

RETURN NIL

METHOD MatchCriteria( oNode ) CLASS TXmlIterator
   HB_SYMBOL_UNUSED( oNode )
RETURN .T.


/********************************************
   Iterator scan class
*********************************************/

CLASS TXmlIteratorScan FROM TXmlIterator
   METHOD New( oNodeTop ) CONSTRUCTOR
PROTECTED:
   METHOD MatchCriteria( oFound )
ENDCLASS

METHOD New( oNodeTop ) CLASS TXmlIteratorScan
   ::Super:New( oNodeTop )
RETURN Self

METHOD MatchCriteria( oFound ) CLASS TXmlIteratorScan

   IF ::cName != NIL .and. ( oFound:cName == NIL .or. ::cName != oFound:cName )
      RETURN .F.
   ENDIF

   IF ::cAttribute != NIL .and. .not. ::cAttribute $ oFound:aAttributes
      RETURN .F.
   ENDIF

   IF ::cValue != NIL .and. ;
      hb_HScan( oFound:aAttributes, {| xKey, cValue| HB_SYMBOL_UNUSED( xKey ), ::cValue == cValue}) == 0
      RETURN .F.
   ENDIF

   IF ::cData != NIL .and. ( oFound:cData == NIL .or. ::cData != oFound:cData )
      RETURN .F.
   ENDIF

RETURN .T.

/********************************************
   Iterator regex class
*********************************************/

CLASS TXmlIteratorRegex FROM TXmlIterator
   METHOD New( oNodeTop ) CONSTRUCTOR
PROTECTED:
   METHOD MatchCriteria( oFound )
ENDCLASS


METHOD New( oNodeTop ) CLASS TXmlIteratorRegex
   ::Super:New( oNodeTop )
RETURN Self

METHOD MatchCriteria( oFound ) CLASS TXmlIteratorRegex

   IF ::cName != NIL .and. ;
         ( oFound:cName == NIL .or. .not. HB_REGEXMATCH( ::cName, oFound:cName, .t. ) )
      RETURN .F.
   ENDIF

   IF ::cAttribute != NIL .and. ;
         hb_hScan( oFound:aAttributes, {|cKey| HB_REGEXMATCH( ::cAttribute, cKey, .t. ) } ) == 0
      RETURN .F.
   ENDIF

   IF ::cValue != NIL .and. ;
         hb_hScan( oFound:aAttributes, {|xKey, cValue| HB_SYMBOL_UNUSED( xKey ), HB_REGEXMATCH( ::cValue, cValue, .t. ) } ) == 0
      RETURN .F.
   ENDIF

   IF ::cData != NIL .and. ;
         ( oFound:cData == NIL .or. .not. HB_REGEXMATCH( ::cData, oFound:cData, .f. ) )
      RETURN .F.
   ENDIF

RETURN .T.


/********************************************
   Document Class
*********************************************/

CLASS TXmlDocument
   DATA oRoot
   DATA nStatus
   DATA nError
   DATA nLine
   DATA oErrorNode
   DATA nNodeCount

   METHOD New( xElem, nStyle )        CONSTRUCTOR
   METHOD Read( xData, nStyle )       INLINE HBXML_DATAREAD( Self, xData, nStyle )
   METHOD ToString( nStyle )          INLINE ::oRoot:ToString( nStyle )
   METHOD Write( fHandle, nStyle )    INLINE ::oRoot:Write( fHandle, nStyle )

   METHOD FindFirst( cName, cAttrib, cValue, cData )
   METHOD FindFirstRegex( cName, cAttrib, cValue, cData )
   METHOD FindNext()                   INLINE ::oIterator:Next()

   METHOD GetContext()
HIDDEN:
   DATA oIterator

ENDCLASS

METHOD New( xElem, nStyle ) CLASS TXmlDocument
   ::nStatus := HBXML_STATUS_OK
   ::nError := HBXML_ERROR_NONE
   ::nLine := 1
   ::nNodeCount := 0

   IF xElem == NIL
      ::oRoot := TXmlNode():New( HBXML_TYPE_DOCUMENT )
   ELSE
      SWITCH ValType( xElem )
         CASE 'O'
            ::oRoot := xElem
            EXIT

         CASE 'N'
         CASE 'C'
            ::oRoot := TXmlNode():New( HBXML_TYPE_DOCUMENT )
            ::Read( xElem, nStyle )
      ENDSWITCH
   ENDIF

RETURN Self

METHOD FindFirst( cName, cAttrib, cValue, cData ) CLASS TXmlDocument
   ::oIterator := TXmlIteratorScan():New( ::oRoot )
RETURN ::oIterator:Find( cName, cAttrib, cValue, cData )

METHOD FindFirstRegex( cName, cAttrib, cValue, cData ) CLASS TXmlDocument
   ::oIterator := TXmlIteratorRegex():New( ::oRoot )
RETURN ::oIterator:Find( cName, cAttrib, cValue, cData )


METHOD GetContext() CLASS TXmlDocument
   LOCAL oDoc

   oDoc := TXmlDocument():New()
   oDoc:oRoot := ::oIterator:GetNode()

RETURN oDoc
