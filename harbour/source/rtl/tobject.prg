/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Base Object from wich all object finally inherit
 *
 * Copyright 2000 JfL&RaC <jfl@mafact.com>, <rac@mafact.com>
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

/*
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 * Copyright 2000 J. Lefebvre <jfl@mafact.com> & RA. Cuylen <rac@mafact.com>
 *    1.40 07/13/2000 JFL&RAC
 *    Now supporting of New and Init method as Class(y) use it
 *    So oMyObj:new(Var1, Var2) will call oMyObj:Init(Var1, Var2)
 *    Currently limited to 20 params
 *
 *    1.41 07/18/2000 JFL&RAC
 *    Improving class(y) compatibility
 *    adding messages :error() and ::MsgNotFound()
 *
 * See doc/license.txt for licensing terms.
 *
 */

/* WARNING: Can not use the preprocessor      */
/* otherwise it will auto inherit from itself */

#include "common.ch"
#include "hboo.ch"
#include "error.ch"

FUNCTION HBObject()
   STATIC s_oClass

   IF s_oClass == NIL

      s_oClass := HBClass():New( "HBObject",, @HBObject() )

      /* Those Five worked fine but their C version from classes.c are probably better in term of speed */
      /*s_oClass:AddInline( "CLASSNAME"      , {| Self | __OBJGETCLSNAME( Self )     }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "CLASSH"         , {| Self | __CLASSH( Self )            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "CLASSSEL"       , {| Self | __CLASSSEL( Self:CLASSH() ) }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "EVAL"           , {| Self | __EVAL( Self )              }, HB_OO_CLSTP_EXPORTED ) */

      /* xBase++ */
#ifdef HB_COMPAT_XPP
      s_oClass:AddInline( "ISDERIVEDFROM"  , {| Self, xPar1 | __ObjDerivedFrom( Self, xPar1 ) }, HB_OO_CLSTP_EXPORTED )
#endif
      /* Class(y) */
      s_oClass:AddInline( "ISKINDOF"       , {| Self, xPar1 | __ObjDerivedFrom( Self, xPar1 ) }, HB_OO_CLSTP_EXPORTED )
                                    
      s_oClass:AddMethod( "NEW"  , @HBObject_New()  , HB_OO_CLSTP_EXPORTED )
      s_oClass:AddMethod( "INIT" , @HBObject_Init() , HB_OO_CLSTP_EXPORTED )

      s_oClass:AddMethod( "ERROR", @HBObject_Error() , HB_OO_CLSTP_EXPORTED )

      s_oClass:SetOnError( @HBObject_DftonError() )

      s_oClass:AddInline( "MSGNOTFOUND" , {| Self, cMsg | ::Error( "Message not found", Self:className, cMsg, iif( Left( cMsg, 1 ) == "_", 1005, 1004 ) ) }, HB_OO_CLSTP_EXPORTED )

      /*s_oClass:AddMultiData( , , HB_OO_CLSTP_EXPORTED, { "CLASS" }, .F. ) */

      /*s_oClass:AddInline( "ADDMETHOD" , { | Self, cMeth, pFunc, nScopeMeth         | __clsAddMsg( __CLASSH( Self ) , cMeth , pFunc ,HB_OO_MSG_METHOD , NIL, iif( nScopeMeth == NIL, 1, nScopeMeth ) ) }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "ADDVAR"    , { | Self, cVAR, nScopeMeth, uiData, hClass | __clsAddMsg( hClass:=__CLASSH( Self ) ,     cVar , uidata := __CLS_INCDATA( hClass ), HB_OO_MSG_ACCESS, NIL, iif( nScopeMeth == NIL, 1, nScopeMeth ) )  , ; */
      /*                                                                               __clsAddMsg( hClass                   , "_"+cVar , uiData                           , HB_OO_MSG_ASSIGN, NIL, iif( nScopeMeth == NIL, 1, nScopeMeth ) ) }, HB_OO_CLSTP_EXPORTED ) */

      /* Those one exist within Class(y), so we will probably try to implement it               */

      /*s_oClass:AddInline( "asString"       , {| Self | ::class:name + " object"   }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "asExpStr"       , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "basicSize"      , {| Self | Len( Self )                }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "become"         , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "isEqual"        , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "isScalar"       , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "copy"           , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "deepCopy"       , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */

      /*s_oClass:AddInline( "deferred"       , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */

      /*s_oClass:AddInline( "exec"           , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "error           , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "hash"           , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "null"           , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "size"           , {| Self | Len( Self )                }, HB_OO_CLSTP_EXPORTED ) */

      /* Those three are already treated within Classes.c */
      /*s_oClass:AddInline( "protectErr"     , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "hiddenErr"      , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "readOnlyErr"    , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */

      /* No idea when those two could occur !!? */
      /*s_oClass:AddInline( "wrongClass"     , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */
      /*s_oClass:AddInline( "badMethod"      , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */

      /* this one exist within VO and seem to be Auto Called when object ran out of scope */
      /*s_oClass:AddInline( "Axit"           , {| Self |                            }, HB_OO_CLSTP_EXPORTED ) */

      s_oClass:Create()

   ENDIF

/*
   oInstance := s_oClass:Instance()
   oInstance:class := s_oClass
   RETURN oInstance
*/

   RETURN s_oClass:Instance()


STATIC function HBObject_New( ... )
   RETURN QSelf():Init( ... )

STATIC FUNCTION HBObject_Init()
   RETURN QSelf()

STATIC FUNCTION HBObject_Dftonerror( ... )
   RETURN QSelf():MSGNOTFOUND( __GetMessage(), ... )

STATIC FUNCTION HBObject_Error( cDesc, cClass, cMsg, nCode )

   DEFAULT nCode TO 1004

   RETURN __errRT_SBASE( iif( nCode == 1005, EG_NOVARMETHOD, EG_NOMETHOD ), nCode, cDesc, cClass + ":" + cMsg, 1, QSelf() )
