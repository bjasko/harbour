/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    Harbour class API
 *
 * Copyright 2006 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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

#ifndef HB_APICLS_H_
#define HB_APICLS_H_

#include "hbapi.h"

HB_EXTERN_BEGIN

#ifdef _HB_API_INTERNAL_

#define HB_OO_OP_PLUS         0
#define HB_OO_OP_MINUS        1
#define HB_OO_OP_MULT         2
#define HB_OO_OP_DIVIDE       3
#define HB_OO_OP_MOD          4
#define HB_OO_OP_POWER        5
#define HB_OO_OP_INC          6
#define HB_OO_OP_DEC          7
#define HB_OO_OP_EQUAL        8
#define HB_OO_OP_EXACTEQUAL   9
#define HB_OO_OP_NOTEQUAL     10
#define HB_OO_OP_LESS         11
#define HB_OO_OP_LESSEQUAL    12
#define HB_OO_OP_GREATER      13
#define HB_OO_OP_GREATEREQUAL 14
#define HB_OO_OP_ASSIGN       15
#define HB_OO_OP_INSTRING     16
#define HB_OO_OP_NOT          17
#define HB_OO_OP_AND          18
#define HB_OO_OP_OR           19
#define HB_OO_OP_ARRAYINDEX   20
#define HB_OO_OP_ENUMINDEX    21
#define HB_OO_OP_ENUMBASE     22
#define HB_OO_OP_ENUMVALUE    23
#define HB_OO_OP_ENUMSTART    24
#define HB_OO_OP_ENUMSKIP     25
#define HB_OO_OP_ENUMSTOP     26

#define HB_OO_MAX_OPERATOR    26

extern void       hb_clsInit( void );           /* initialize Classy/OO system at HVM startup */
extern void       hb_clsReleaseAll( void );     /* releases all defined classes */
extern void       hb_clsIsClassRef( void );     /* classes.c - mark all class internals as used */
extern BOOL       hb_clsHasDestructor( USHORT uiClass );

extern PHB_SYMB   hb_objGetMethod( PHB_ITEM pObject, PHB_SYMB pSymMsg, PHB_STACK_STATE pStack ); /* returns the method pointer of an object class */
extern BOOL       hb_objGetVarRef( PHB_ITEM pObject, PHB_SYMB pMessage, PHB_STACK_STATE pStack ); /* create object variable reference */
extern BOOL       hb_objHasOperator( PHB_ITEM pObject, USHORT uiOperator );
extern BOOL       hb_objOperatorCall( USHORT uiOperator, HB_ITEM_PTR pResult, PHB_ITEM pObject, PHB_ITEM pMsgArg1, PHB_ITEM pMsgArg2 );
extern void       hb_objDestructorCall( PHB_ITEM pObject );

#ifndef HB_NO_PROFILER
/* profiler for object management */
extern void       hb_mthAddTime( ULONG );       /* profiler from classes.c */
#endif

#endif

/* class management */
HB_EXPORT extern const char * hb_clsName( USHORT uiClass );
HB_EXPORT extern const char * hb_clsFuncName( USHORT uiClass );
HB_EXPORT extern BOOL       hb_clsIsParent( USHORT uiClass, const char * szParentName ); /* is a class handle inherited from szParentName Class ? */
HB_EXPORT extern USHORT     hb_clsFindClass( const char * szClass, const char * szFunc );

/* object management */
HB_EXPORT extern USHORT     hb_objGetClass( PHB_ITEM pItem );      /* get object class handle */
HB_EXPORT extern USHORT     hb_objSetClass( PHB_ITEM pItem, const char * szClass, const char * szFunc );    /* get object class handle using class name and class function name */
HB_EXPORT extern const char * hb_objGetClsName( PHB_ITEM pObject );  /* retrieves an object class name */
HB_EXPORT extern const char * hb_objGetRealClsName( PHB_ITEM pObject, const char * szString  ); /* retrieves an object class name for a specific message */

HB_EXPORT extern BOOL       hb_objHasMsg( PHB_ITEM pObject, const char * szString ); /* returns TRUE/FALSE whether szString is an existing message for object */
HB_EXPORT extern BOOL       hb_objHasMessage( PHB_ITEM pObject, PHB_DYNS pMessage );
HB_EXPORT extern void       hb_objSendMsg( PHB_ITEM pObj, const char *sMsg, ULONG ulArg, ... );
HB_EXPORT extern void       hb_objSendMessage( PHB_ITEM pObj, PHB_DYNS pMessage, ULONG ulArg, ... );


/* Harbour equivalent for Clipper internal __mdCreate() */
USHORT hb_clsCreate( USHORT usSize, const char * szClassName );
/* Harbour equivalent for Clipper internal __mdAdd() */
void hb_clsAdd( USHORT usClassH, const char * szMethodName, PHB_FUNC pFuncPtr );
/* Harbour equivalent for Clipper internal __mdAssociate() */
void hb_clsAssociate( USHORT usClassH );


HB_EXTERN_END

#endif /* HB_APICLS_H_ */
