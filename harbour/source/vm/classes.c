/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Base-routines for OOPS system
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
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
 * Copyright 1999 Eddie Runia <eddie@runia.com>
 *    :CLASSSEL()
 *    __clsDelMsg()
 *    __clsModMsg()
 *    __clsInstSuper()
 *    __cls_CntClsData()
 *    __cls_CntData()
 *    __cls_DecData()
 *    __cls_IncData()
 *    __objClone()
 *    __objHasMsg()
 *    __objSendMsg()
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    __CLASSNEW()
 *    __CLASSINSTANCE()
 *    __CLASSADD()
 *    __CLASSNAME()
 *    __CLASSSEL() (based on hb___msgClsSel())
 *
 * Copyright 1999 Janica Lubos <janica@fornax.elf.stuba.sk>
 *    hb_clsDictRealloc()
 *
 * Copyright 2000 ( ->07/2000 ) JF. Lefebvre <jfl@mafact.com> & RA. Cuylen <cakiral@altern.org
 *    Multiple inheritence fully implemented
 *    Forwarding, delegating
 *    Data initialisation & Autoinit for Bool and Numeric
 *    Scoping : Protected / exported
 *
 * Copyright 2000 ( 08/2000-> ) JF. Lefebvre <jfl@mafact.com>
 *    hb_clsDictRealloc()   New version
 *    Now support of shared and not shared class data
 *    Multiple datas declaration fully supported
 *
 *    2000 RGlab
 *    Garbage collector fixe
 *
 * Copyright 2001 JF. Lefebvre <jfl@mafact.com>
 *    Super msg corrected
 *    Scoping : working for protected, hidden and readonly
 *    To Many enhancement and correction to give a full list :-)
 *    Improved class(y) compatibility
 *    Improved TopClass compatibility
 *    __CLS_PAR00() (Allow the creation of class wich not autoinherit of the default HBObject)
 *    Adding HB_CLS_ENFORCERO FLAG to disable Write access to RO VAR
 *    outside of Constructors /!\ Could be related to some incompatibility
 *    Added hb_objGetRealClsName to keep a full class tree ( for 99% cases )
 *    Fixed hb_clsIsParent
 *
 *
 *    hb_objGetMthd() & __CLSADDMSG modified to translate the followings operators
 *
 "+"     = __OpPlus
 "-"     = __OpMinus
 "*"     = __OpMult
 "/"     = __OpDivide
 "%"     = __OpMod
 "^"     = __OpPower
 "**"    = __OpPower
 "++"    = __OpInc
 "--"    = __OpDec
 "=="    = __OpEqual
 "="     = __OpEqual (same as "==")
 "!="    = __OpNotEqual
 "<>"    = __OpNotEqual (same as "!=")
 "#"     = __OpNotEqual (same as "!=")
 "<"     = __OpLess
 "<="    = __OpLessEqual
 ">"     = __OpGreater
 ">="    = __OpGreaterEqual
 "$"     = __OpInstring
 "!"     = __OpNot
 ".NOT." = __OpNot (same as "!")
 ".AND." = __OpAnd
 ".OR."  = __OpOr
 ":="    = __OpAssign   ... not tested ...
 "[]"    = __OpArrayIndex
 *
 *
 * See doc/license.txt for licensing terms.
 *
 */

#include "hbvmopt.h"
#include "hbapi.h"
#include "hbapicls.h"
#include "hbstack.h"
#include "hbapierr.h"
#include "hbapiitm.h"
#include "hbvm.h"
#include "hboo.ch"

#include <ctype.h>             /* For toupper() */

/* DEBUG only*/
/* #include <windows.h> */

typedef struct
{
   PHB_ITEM pInitValue;          /* Init Value for data */
   USHORT   uiType;              /* HB_OO_MSG_DATA, HB_OO_MSG_CLASSDATA or HB_OO_MSG_INITIALIZED */
   USHORT   uiData;              /* Item position in instance area or class data */
   USHORT   uiOffset;            /* Supper cast instance area offset for HB_OO_MSG_DATA or real class item position */
   USHORT   uiSprClass;          /* The real class where method were defined */
} INITDATA, * PINITDATA;

typedef struct
{
   PHB_DYNS pMessage;            /* Method symbolic name */
   PHB_SYMB pFuncSym;            /* Function symbol */
   USHORT   uiSprClass;          /* Originalclass'handel (super or current class'handel if not herited). */ /*Added by RAC&JF*/
   USHORT   uiScope;             /* Scoping value */
   USHORT   uiData;              /* Item position for instance data, class data and shared data (Harbour like, begin from 1) or delegated message index object */
   USHORT   uiOffset;            /* position in pInitData for class datas (from 1) or offset to instance area in inherited instance data and supercast messages (from 0) */
   HB_TYPE  itemType;            /* Type of item in restricted assignment */
   USHORT   uiPrevCls;
   USHORT   uiPrevMth;
#ifndef HB_NO_PROFILER
   ULONG    ulCalls;             /* profiler support */
   ULONG    ulTime;              /* profiler support */
   ULONG    ulRecurse;           /* profiler support */
#endif
} METHOD, * PMETHOD;


#define HB_MSG_POOL
typedef struct
{
   char *   szName;           /* Class name */
   PHB_DYNS pClassSym;        /* Class symbolic name */
   PMETHOD  pMethods;         /* Class methods */
   PHB_SYMB pClassFuncSym;    /* Class function symbol */
   PHB_SYMB pFriendModule;    /* Class friend symbols */
   PINITDATA pInitData;       /* Class/instance Initialization data */
   PHB_ITEM pClassDatas;      /* Harbour Array for Class Datas */
   PHB_ITEM pSharedDatas;     /* Harbour Array for Class Shared Datas */
   PHB_ITEM pInlines;         /* Array for inline codeblocks */
   PHB_SYMB * pFriendSyms;    /* Friend functions' symbols */
   ULONG    ulOpFlags;        /* Flags for overloaded operators */
   USHORT   fHasDestructor;   /* has the class destructor message? */
   USHORT   fHasOnError;      /* has the class OnError message? */
   USHORT   fLocked;          /* Class is locked against modifications */
   USHORT   uiMethods;        /* Total Method initialised Counter */
   USHORT   uiInitDatas;      /* Total Method initialised Counter */
   USHORT   uiDatas;          /* Total Data Counter */
   USHORT   uiDataFirst;      /* First instance item from this class */
   USHORT   uiFriendSyms;     /* Number of friend function's symbols */
   USHORT   uiFriendModule;   /* Number of friend symbols in pFriendModule */
   USHORT   uiHashKey;
#ifdef HB_MSG_POOL
   USHORT * puiMsgIdx;
   USHORT   uiMethodCount;
#endif
} CLASS, * PCLASS;

#define BUCKETBITS      2
#define BUCKETSIZE      ( 1 << BUCKETBITS )
#define BUCKETMASK      ( BUCKETSIZE - 1 )
#define HASHBITS        3
#define HASH_KEY        ( ( 1 << HASHBITS ) - 1 )
#define HASH_KEYMAX     ( 1 << ( 16 - BUCKETBITS ) )
#define hb_clsInited(p) ( (p)->pMethods != NULL )
#define hb_clsBucketPos( p, m )     ( ( (p)->uiSymNum & (m) ) << BUCKETBITS )

#ifdef HB_MSG_POOL
#  define hb_clsMthNum(p) ( ( ULONG ) (p)->uiMethodCount )
#else
#  define hb_clsMthNum(p) ( ( ( ULONG ) (p)->uiHashKey + 1 ) << BUCKETBITS )
#endif

#if defined( HB_REAL_BLOCK_SCOPE )
#  undef HB_CLASSY_BLOCK_SCOPE
#elif !defined( HB_CLASSY_BLOCK_SCOPE )
#  define HB_REAL_BLOCK_SCOPE
#endif

#if !defined( HB_CLASSY_BLOCK_SCOPE )
#  define hb_clsSenderOffset()      hb_stackBaseProcOffset( 1 )
#endif


static HARBOUR  hb___msgGetData( void );
static HARBOUR  hb___msgSetData( void );
static HARBOUR  hb___msgGetClsData( void );
static HARBOUR  hb___msgSetClsData( void );
static HARBOUR  hb___msgGetShrData( void );
static HARBOUR  hb___msgSetShrData( void );
static HARBOUR  hb___msgEvalInline( void );
static HARBOUR  hb___msgVirtual( void );
static HARBOUR  hb___msgSuper( void );
static HARBOUR  hb___msgRealClass( void );
static HARBOUR  hb___msgPerform( void );
static HARBOUR  hb___msgDelegate( void );
static HARBOUR  hb___msgNoMethod( void );
static HARBOUR  hb___msgScopeErr( void );
static HARBOUR  hb___msgTypeErr( void );
static HARBOUR  hb___msgNull( void );

static HARBOUR  hb___msgClassH( void );
static HARBOUR  hb___msgClassName( void );
static HARBOUR  hb___msgClassSel( void );
/* static HARBOUR  hb___msgClass( void ); */
/* static HARBOUR  hb___msgClassParent( void ); */

/*
 * The positions of items in symbol table below have to correspond
 * to HB_OO_OP_* constants in hbapicls.h, [druzus]
 */
static HB_SYMB s_opSymbols[ HB_OO_MAX_OPERATOR + 1 ] = {
   { "__OPPLUS",              {HB_FS_MESSAGE}, {NULL}, NULL },  /* 00 */
   { "__OPMINUS",             {HB_FS_MESSAGE}, {NULL}, NULL },  /* 01 */
   { "__OPMULT",              {HB_FS_MESSAGE}, {NULL}, NULL },  /* 02 */
   { "__OPDIVIDE",            {HB_FS_MESSAGE}, {NULL}, NULL },  /* 03 */
   { "__OPMOD",               {HB_FS_MESSAGE}, {NULL}, NULL },  /* 04 */
   { "__OPPOWER",             {HB_FS_MESSAGE}, {NULL}, NULL },  /* 05 */
   { "__OPINC",               {HB_FS_MESSAGE}, {NULL}, NULL },  /* 06 */
   { "__OPDEC",               {HB_FS_MESSAGE}, {NULL}, NULL },  /* 07 */
   { "__OPEQUAL",             {HB_FS_MESSAGE}, {NULL}, NULL },  /* 08 */
   { "__OPEXACTEQUAL",        {HB_FS_MESSAGE}, {NULL}, NULL },  /* 09 */
   { "__OPNOTEQUAL",          {HB_FS_MESSAGE}, {NULL}, NULL },  /* 10 */
   { "__OPLESS",              {HB_FS_MESSAGE}, {NULL}, NULL },  /* 11 */
   { "__OPLESSEQUAL",         {HB_FS_MESSAGE}, {NULL}, NULL },  /* 12 */
   { "__OPGREATER",           {HB_FS_MESSAGE}, {NULL}, NULL },  /* 13 */
   { "__OPGREATEREQUAL",      {HB_FS_MESSAGE}, {NULL}, NULL },  /* 14 */
   { "__OPASSIGN",            {HB_FS_MESSAGE}, {NULL}, NULL },  /* 15 */
   { "__OPINSTRING",          {HB_FS_MESSAGE}, {NULL}, NULL },  /* 16 */
   { "__OPNOT",               {HB_FS_MESSAGE}, {NULL}, NULL },  /* 17 */
   { "__OPAND",               {HB_FS_MESSAGE}, {NULL}, NULL },  /* 18 */
   { "__OPOR",                {HB_FS_MESSAGE}, {NULL}, NULL },  /* 19 */
   { "__OPARRAYINDEX",        {HB_FS_MESSAGE}, {NULL}, NULL },  /* 20 */
   { "__ENUMINDEX",           {HB_FS_MESSAGE}, {NULL}, NULL },  /* 21 */
   { "__ENUMBASE",            {HB_FS_MESSAGE}, {NULL}, NULL },  /* 22 */
   { "__ENUMVALUE",           {HB_FS_MESSAGE}, {NULL}, NULL },  /* 23 */
   { "__ENUMSTART",           {HB_FS_MESSAGE}, {NULL}, NULL },  /* 24 */
   { "__ENUMSKIP",            {HB_FS_MESSAGE}, {NULL}, NULL },  /* 25 */
   { "__ENUMSTOP",            {HB_FS_MESSAGE}, {NULL}, NULL }   /* 26 */
};

static HB_SYMB s___msgDestructor = { "__msgDestructor", {HB_FS_MESSAGE}, {NULL},               NULL };
static HB_SYMB s___msgOnError    = { "__msgOnError",    {HB_FS_MESSAGE}, {NULL},               NULL };

static HB_SYMB s___msgSetData    = { "__msgSetData",    {HB_FS_MESSAGE}, {hb___msgSetData},    NULL };
static HB_SYMB s___msgGetData    = { "__msgGetData",    {HB_FS_MESSAGE}, {hb___msgGetData},    NULL };
static HB_SYMB s___msgSetClsData = { "__msgSetClsData", {HB_FS_MESSAGE}, {hb___msgSetClsData}, NULL };
static HB_SYMB s___msgGetClsData = { "__msgGetClsData", {HB_FS_MESSAGE}, {hb___msgGetClsData}, NULL };
static HB_SYMB s___msgSetShrData = { "__msgSetShrData", {HB_FS_MESSAGE}, {hb___msgSetShrData}, NULL };
static HB_SYMB s___msgGetShrData = { "__msgGetShrData", {HB_FS_MESSAGE}, {hb___msgGetShrData}, NULL };
static HB_SYMB s___msgEvalInline = { "__msgEvalInline", {HB_FS_MESSAGE}, {hb___msgEvalInline}, NULL };
static HB_SYMB s___msgVirtual    = { "__msgVirtual",    {HB_FS_MESSAGE}, {hb___msgVirtual},    NULL };
static HB_SYMB s___msgSuper      = { "__msgSuper",      {HB_FS_MESSAGE}, {hb___msgSuper},      NULL };
static HB_SYMB s___msgRealClass  = { "__msgRealClass",  {HB_FS_MESSAGE}, {hb___msgRealClass},  NULL };
static HB_SYMB s___msgPerform    = { "__msgPerform",    {HB_FS_MESSAGE}, {hb___msgPerform},    NULL };
static HB_SYMB s___msgDelegate   = { "__msgDelegate",   {HB_FS_MESSAGE}, {hb___msgDelegate},   NULL };
static HB_SYMB s___msgNoMethod   = { "__msgNoMethod",   {HB_FS_MESSAGE}, {hb___msgNoMethod},   NULL };
static HB_SYMB s___msgScopeErr   = { "__msgScopeErr",   {HB_FS_MESSAGE}, {hb___msgScopeErr},   NULL };
static HB_SYMB s___msgTypeErr    = { "__msgTypeErr",    {HB_FS_MESSAGE}, {hb___msgTypeErr},    NULL };

static HB_SYMB s___msgNew        = { "NEW",             {HB_FS_MESSAGE}, {NULL},               NULL };
static HB_SYMB s___msgSymbol     = { "SYMBOL",          {HB_FS_MESSAGE}, {NULL},               NULL };

static HB_SYMB s___msgClassName  = { "CLASSNAME",       {HB_FS_MESSAGE}, {hb___msgClassName},  NULL };
static HB_SYMB s___msgClassH     = { "CLASSH",          {HB_FS_MESSAGE}, {hb___msgClassH},     NULL };
static HB_SYMB s___msgClassSel   = { "CLASSSEL",        {HB_FS_MESSAGE}, {hb___msgClassSel},   NULL };
static HB_SYMB s___msgExec       = { "EXEC",            {HB_FS_MESSAGE}, {hb___msgNull},       NULL };
static HB_SYMB s___msgName       = { "NAME",            {HB_FS_MESSAGE}, {hb___msgNull},       NULL };
/*
static HB_SYMB s___msgClsParent  = { "ISDERIVEDFROM",   {HB_FS_MESSAGE}, {hb___msgClassParent},NULL };
static HB_SYMB s___msgClass      = { "CLASS",           {HB_FS_MESSAGE}, {hb___msgClass},      NULL };
*/
static HB_SYMB s___msgKeys       = { "KEYS",            {HB_FS_MESSAGE}, {hb___msgNull},       NULL };
static HB_SYMB s___msgValues     = { "VALUES",          {HB_FS_MESSAGE}, {hb___msgNull},       NULL };

/* Default enumerator methods (FOR EACH) */
static HB_SYMB s___msgEnumIndex  = { "__ENUMINDEX",     {HB_FS_MESSAGE}, {hb___msgNull},       NULL };
static HB_SYMB s___msgEnumBase   = { "__ENUMBASE",      {HB_FS_MESSAGE}, {hb___msgNull},       NULL };
static HB_SYMB s___msgEnumKey    = { "__ENUMKEY",       {HB_FS_MESSAGE}, {hb___msgNull},       NULL };
static HB_SYMB s___msgEnumValue  = { "__ENUMVALUE",     {HB_FS_MESSAGE}, {hb___msgNull},       NULL };

/* WITH OBJECT base value access/asign methods (:__withobject) */
static HB_SYMB s___msgWithObjectPush = { "__WITHOBJECT",  {HB_FS_MESSAGE}, {hb___msgNull},       NULL };
static HB_SYMB s___msgWithObjectPop  = { "___WITHOBJECT", {HB_FS_MESSAGE}, {hb___msgNull},       NULL };

static PCLASS   s_pClasses     = NULL;
static USHORT   s_uiClasses    = 0;

/* ================================================ */


#if 0
static USHORT hb_clsBucketPos( PHB_DYNS pMsg, USHORT uiMask )
{
   /*
    * we can use PHB_DYNS address as base for hash key.
    * This value is perfectly unique and we do not need anything more
    * but it's not continuous so we will have to add dynamic BUCKETSIZE
    * modification to be 100% sure that we can resolve all symbol name
    * conflicts (though even without it it's rather theoretical problem).
    * [druzus]
    */

    /* Safely divide it by 16 - it's minimum memory allocated for single
     * HB_DYNS structure
     */
    /* 
    return ( ( USHORT ) ( ( HB_PTRDIFF ) pMsg >> 4 ) & uiMask ) << BUCKETBITS;
    */

   /* Using continuous symbol numbers we are 100% sure that we will cover
    * the whole 16bit area and we will never have any problems until number
    * of symbols is limited to 2^16. [druzus]
    */
   return ( pMsg->uiSymNum & uiMask ) << BUCKETBITS;
}
#endif

/*
 * hb_clsDictRealloc( PCLASS )
 *
 * Realloc (widen) class
 */
static BOOL hb_clsDictRealloc( PCLASS pClass )
{
   ULONG ulNewHashKey, ulLimit, ul;
#ifdef HB_MSG_POOL
   USHORT * puiMsgIdx;
#else
   PMETHOD pNewMethods;
#endif

   HB_TRACE(HB_TR_DEBUG, ("hb_clsDictRealloc(%p)", pClass));

   ulNewHashKey = ( ULONG ) pClass->uiHashKey + 1;
   ulLimit = ulNewHashKey << BUCKETBITS;

   do
   {
      ulNewHashKey <<= 1;
      if( ulNewHashKey > HASH_KEYMAX )
      {
         hb_errInternal( 9999, "Not able to realloc classmessage! __clsDictRealloc", NULL, NULL );
         return FALSE;
      }

#ifdef HB_MSG_POOL
      puiMsgIdx = ( USHORT * ) hb_xgrab( ( ulNewHashKey << BUCKETBITS ) * sizeof( USHORT ) );
      memset( puiMsgIdx, 0, ( ulNewHashKey << BUCKETBITS ) * sizeof( USHORT ) );

      for( ul = 0; ul < ulLimit; ul++ )
      {
         USHORT uiMsg = pClass->puiMsgIdx[ ul ];
         if( pClass->puiMsgIdx[ ul ] )
         {
            USHORT uiBucket = BUCKETSIZE;
            USHORT * puiIdx = puiMsgIdx + hb_clsBucketPos(
                        pClass->pMethods[ uiMsg ].pMessage, ulNewHashKey - 1 );
            do
            {
               if( * puiIdx == 0 ) /* this message position is empty */
               {
                  * puiIdx = uiMsg;
                  break;
               }
               ++puiIdx;
            } while( --uiBucket );

            /* Not enough go back to the beginning */
            if( ! uiBucket )
            {
               hb_xfree( puiMsgIdx );
               break;
            }
         }
      }
   }
   while( ul < ulLimit );

   pClass->uiHashKey = ( USHORT ) ( ulNewHashKey - 1 );
   hb_xfree( pClass->puiMsgIdx );
   pClass->puiMsgIdx = puiMsgIdx;

#else

      pNewMethods = ( PMETHOD ) hb_xgrab( ( ulNewHashKey << BUCKETBITS ) * sizeof( METHOD ) );
      memset( pNewMethods, 0, ( ulNewHashKey << BUCKETBITS ) * sizeof( METHOD ) );

      for( ul = 0; ul < ulLimit; ul++ )
      {
         PHB_DYNS pMessage = ( PHB_DYNS ) pClass->pMethods[ ul ].pMessage;

         if( pMessage )
         {
            PMETHOD pMethod = pNewMethods + hb_clsBucketPos( pMessage, ulNewHashKey - 1 );
            USHORT uiBucket = BUCKETSIZE;

            do
            {
               if( ! pMethod->pMessage ) /* this message position is empty */
               {
                  memcpy( pMethod, pClass->pMethods + ul, sizeof( METHOD ) );
                  break;
               }
               ++pMethod;
            } while( --uiBucket );

            /* Not enough go back to the beginning */
            if( ! uiBucket )
            {
               hb_xfree( pNewMethods );
               break;
            }
         }
      }
   }
   while( ul < ulLimit );

   pClass->uiHashKey = ( USHORT ) ( ulNewHashKey - 1 );
   hb_xfree( pClass->pMethods );
   pClass->pMethods = pNewMethods;
#endif

   return TRUE;
}

static void hb_clsDictInit( PCLASS pClass, USHORT uiHashKey )
{
   ULONG ulSize;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsDictInit(%p,%hu)", pClass, uiHashKey));

   pClass->uiHashKey = uiHashKey;
#ifdef HB_MSG_POOL
   ulSize = ( ( ( ULONG ) uiHashKey + 1 ) << BUCKETBITS ) * sizeof( USHORT );
   pClass->puiMsgIdx = ( USHORT * ) hb_xgrab( ulSize );
   memset( pClass->puiMsgIdx, 0, ulSize );

   pClass->uiMethodCount = 1;
   pClass->pMethods = ( PMETHOD ) hb_xgrab( sizeof( METHOD ) );
   memset( pClass->pMethods, 0, sizeof( METHOD ) );
#else
   ulSize = ( ( ( ULONG ) uiHashKey + 1 ) << BUCKETBITS ) * sizeof( METHOD );
   pClass->pMethods = ( PMETHOD ) hb_xgrab( ulSize );
   memset( pClass->pMethods, 0, ulSize );
#endif
}

static PMETHOD hb_clsFindMsg( PCLASS pClass, PHB_DYNS pMsg )
{
#ifdef HB_MSG_POOL

   USHORT uiBucket, * puiMsgIdx;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsFindMsg(%p,%p)", pClass, pMsg));

   puiMsgIdx = pClass->puiMsgIdx + hb_clsBucketPos( pMsg, pClass->uiHashKey );
   uiBucket = BUCKETSIZE;

   do
   {
      if( pClass->pMethods[ * puiMsgIdx ].pMessage == pMsg )
      {
         return &pClass->pMethods[ * puiMsgIdx ];
      }
      ++puiMsgIdx;
   }
   while( --uiBucket );

#else

   PMETHOD pMethod;
   USHORT uiBucket;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsFindMsg(%p,%p)", pClass, pMsg));

   pMethod = pClass->pMethods + hb_clsBucketPos( pMsg, pClass->uiHashKey );
   uiBucket = BUCKETSIZE;

   do
   {
      if( pMethod->pMessage == pMsg )
         return pMethod;
      ++pMethod;
   }
   while( --uiBucket );
#endif

   return NULL;
}

static PMETHOD hb_clsAllocMsg( PCLASS pClass, PHB_DYNS pMsg )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_clsAllocMsg(%p,%p)", pClass, pMsg));

   do
   {

#ifdef HB_MSG_POOL

      USHORT uiBucket = BUCKETSIZE, * puiMsgIdx = pClass->puiMsgIdx +
                                    hb_clsBucketPos( pMsg, pClass->uiHashKey );

      do
      {
         if( * puiMsgIdx == 0 )
         {
            pClass->pMethods = ( PMETHOD ) hb_xrealloc( pClass->pMethods,
                           sizeof( METHOD ) * ( pClass->uiMethodCount + 1 ) );
            memset( &pClass->pMethods[ pClass->uiMethodCount ], 0, sizeof( METHOD ) );
            * puiMsgIdx = pClass->uiMethodCount++;
            return &pClass->pMethods[ * puiMsgIdx ];
         }
         else if( pClass->pMethods[ * puiMsgIdx ].pMessage == pMsg )
            return &pClass->pMethods[ * puiMsgIdx ];
         ++puiMsgIdx;
      }
      while( --uiBucket );

#else

      PMETHOD pMethod = pClass->pMethods + hb_clsBucketPos( pMsg, pClass->uiHashKey );
      USHORT uiBucket = BUCKETSIZE;

      do
      {
         if( ! pMethod->pMessage || pMethod->pMessage == pMsg )
            return pMethod;
         ++pMethod;
      }
      while( --uiBucket );

#endif

   }
   while( hb_clsDictRealloc( pClass ) );

   return NULL;
}

static BOOL hb_clsCanClearMethod( PMETHOD pMethod, BOOL fError )
{
   HB_SYMBOL_UNUSED( pMethod );
   HB_SYMBOL_UNUSED( fError );
#if 0
   if( pMethod->pFuncSym == &s___msgSuper )
   {
      if( fError )
         hb_errRT_BASE( EG_ARG, 3000, "Cannot delete supercast messages", &hb_errFuncName, 0 );
      return FALSE;
   }
#endif
   return TRUE;
}

static void hb_clsFreeMsg( PCLASS pClass, PHB_DYNS pMsg )
{
#ifdef HB_MSG_POOL

   USHORT uiBucket, * puiMsgIdx;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsFreeMsg(%p,%p)", pClass, pMsg));


   puiMsgIdx = pClass->puiMsgIdx + hb_clsBucketPos( pMsg, pClass->uiHashKey );
   uiBucket = BUCKETSIZE;

   do
   {
      if( * puiMsgIdx && pClass->pMethods[ * puiMsgIdx ].pMessage == pMsg )
      {
         if( hb_clsCanClearMethod( &pClass->pMethods[ * puiMsgIdx ], TRUE ) )
         {
            memset( &pClass->pMethods[ * puiMsgIdx ], 0, sizeof( METHOD ) );
            * puiMsgIdx = 0;
            pClass->uiMethods--;       /* Decrease number of messages */
         }
         return;
      }
      ++puiMsgIdx;
   }
   while( --uiBucket );

#else

   PMETHOD pMethod;
   USHORT uiBucket;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsFreeMsg(%p,%p)", pClass, pMsg));

   pMethod = pClass->pMethods + hb_clsBucketPos( pMsg, pClass->uiHashKey );
   uiBucket = BUCKETSIZE;

   do
   {
      if( pMethod->pMessage == pMsg )
      {
         if( hb_clsCanClearMethod( pMethod, TRUE ) )
         {
            /* Move messages */
            while( --uiBucket )
            {
               memcpy( pMethod, pMethod + 1, sizeof( METHOD ) );
               pMethod++;
            }
            memset( pMethod, 0, sizeof( METHOD ) );
            pClass->uiMethods--;       /* Decrease number of messages */
         }
         return;
      }
      ++pMethod;
   }
   while( --uiBucket );

#endif
}

static BOOL hb_clsHasParent( PCLASS pClass, PHB_DYNS pParentSym )
{
   PMETHOD pMethod = hb_clsFindMsg( pClass, pParentSym );

   return pMethod && pMethod->pFuncSym == &s___msgSuper;
}

static USHORT hb_clsParentInstanceOffset( PCLASS pClass, PHB_DYNS pParentSym )
{
   PMETHOD pMethod = hb_clsFindMsg( pClass, pParentSym );

   return ( pMethod && pMethod->pFuncSym == &s___msgSuper ) ? pMethod->uiOffset : 0;
}

static USHORT hb_clsAddInitValue( PCLASS pClass, PHB_ITEM pItem,
                                  USHORT uiType, USHORT uiData,
                                  USHORT uiOffset, USHORT uiSprClass )
{
   PINITDATA pInitData;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsAddInitValue(%p,%p,%hu,%hu,%hu,%hu)", pClass, pItem, uiType, uiData, uiOffset, uiSprClass));

   if( ! pItem || HB_IS_NIL( pItem ) )
      return 0;

   if( ! pClass->uiInitDatas )
   {
      pClass->pInitData = ( PINITDATA ) hb_xgrab( sizeof( INITDATA ) );
      pInitData = pClass->pInitData + pClass->uiInitDatas++;
   }
   else
   {
      USHORT ui = pClass->uiInitDatas;
      pInitData = pClass->pInitData;
      do
      {
         if( pInitData->uiType == uiType &&
             pInitData->uiData + pInitData->uiOffset == uiData + uiOffset )
         {
            hb_itemRelease( pInitData->pInitValue );
            break;
         }
         ++pInitData;
      }
      while( --ui );

      if( ui == 0 )
      {
         pClass->pInitData = ( PINITDATA ) hb_xrealloc( pClass->pInitData,
                  ( ULONG ) ( pClass->uiInitDatas + 1 ) * sizeof( INITDATA ) );
         pInitData = pClass->pInitData + pClass->uiInitDatas++;
      }
   }

   pInitData->pInitValue = hb_itemClone( pItem );
   pInitData->uiType = uiType;
   pInitData->uiData = uiData;
   pInitData->uiOffset = uiOffset;
   pInitData->uiSprClass = uiSprClass;

   return pClass->uiInitDatas;
}

static USHORT hb_clsFindRealClassDataOffset( PMETHOD pMethod )
{
   PMETHOD pRealMth;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsFindRealClassDataOffset(%p)", pMethod));

   pRealMth = hb_clsFindMsg( &s_pClasses[ pMethod->uiSprClass ],
                             pMethod->pMessage );
   if( pRealMth && pRealMth->uiSprClass == pMethod->uiSprClass &&
       ( pRealMth->pFuncSym == &s___msgSetClsData ||
         pRealMth->pFuncSym == &s___msgGetClsData ) )
   {
      return pRealMth->uiData;
   }
   return 0;
}

static USHORT hb_clsFindClassDataOffset( PCLASS pClass, PMETHOD pNewMethod )
{
   USHORT uiData;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsFindClassDataOffset(%p,%p)", pClass, pNewMethod));

   uiData = hb_clsFindRealClassDataOffset( pNewMethod );
   if( uiData )
   {
      ULONG ulLimit = hb_clsMthNum( pClass );
      PMETHOD pMethod = pClass->pMethods;
      do
      {
         if( pMethod->pMessage && pMethod != pNewMethod &&
             pMethod->uiSprClass == pNewMethod->uiSprClass &&
             ( pMethod->pFuncSym == &s___msgSetClsData ||
               pMethod->pFuncSym == &s___msgGetClsData ) &&
             uiData == hb_clsFindRealClassDataOffset( pMethod ) )
         {
            return pMethod->uiData;
         }
         ++pMethod;
      }
      while( --ulLimit );
   }

   return 0;
}

static BOOL hb_clsUpdateHiddenMessages( PMETHOD pSrcMethod, PMETHOD pDstMethod,
                                        PCLASS pDstClass )
{
   PMETHOD pNewMethod = pSrcMethod;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsUpdateHiddenMessages(%p,%p,%p)", pSrcMethod, pDstMethod, pDstClass));

   if( ! pDstMethod->pMessage ||
       ( hb_clsCanClearMethod( pDstMethod, FALSE ) &&
         pDstMethod->uiPrevCls != pDstMethod->uiSprClass &&
         ( pDstMethod->uiScope & HB_OO_CLSTP_HIDDEN ) &&
         ( pDstMethod->uiScope & HB_OO_CLSTP_NONVIRTUAL ) ) )
   {
      while( pNewMethod &&
             pNewMethod->uiPrevCls != pNewMethod->uiSprClass &&
             ( pNewMethod->uiScope & HB_OO_CLSTP_HIDDEN ) &&
             ( pNewMethod->uiScope & HB_OO_CLSTP_NONVIRTUAL ) )
      {
         pNewMethod = hb_clsFindMsg( &s_pClasses[ pNewMethod->uiPrevCls ],
                                     pNewMethod->pMessage );
      }
      if( pNewMethod && pNewMethod != pSrcMethod &&
          !( pNewMethod->uiScope & HB_OO_CLSTP_HIDDEN ) &&
          hb_clsCanClearMethod( pDstMethod, FALSE ) )
      {
         USHORT uiPrevCls = pDstMethod->uiPrevCls,
                uiPrevMth = pDstMethod->uiPrevMth;

         memcpy( pDstMethod, pNewMethod, sizeof( METHOD ) );
         pDstMethod->uiPrevCls = uiPrevCls;
         pDstMethod->uiPrevMth = uiPrevMth;
         pDstMethod->uiScope |= HB_OO_CLSTP_OVERLOADED | HB_OO_CLSTP_SUPER;
         if( pDstMethod->pFuncSym == &s___msgSetData ||
             pDstMethod->pFuncSym == &s___msgGetData )
         {
            pDstMethod->uiOffset = hb_clsParentInstanceOffset( pDstClass,
                              s_pClasses[ pDstMethod->uiSprClass ].pClassSym );
         }
         else if( pDstMethod->pFuncSym == &s___msgSetClsData ||
                  pDstMethod->pFuncSym == &s___msgGetClsData )
         {
            PCLASS pSrcClass = &s_pClasses[ pDstMethod->uiSprClass ];
            USHORT uiData;

            uiData = hb_clsFindClassDataOffset( pDstClass, pDstMethod );

            if( uiData == 0 )
            {
               uiData = hb_arrayLen( pDstClass->pClassDatas ) + 1;
               hb_arraySize( pDstClass->pClassDatas, uiData );
            }
            if( pDstMethod->uiOffset )
            {
               pDstMethod->uiOffset = hb_clsAddInitValue( pDstClass,
                  pSrcClass->pInitData[ pDstMethod->uiOffset - 1 ].pInitValue,
                  HB_OO_MSG_CLASSDATA, uiData, 0, pDstMethod->uiSprClass );
            }
            pDstMethod->uiData = uiData;
         }
         return TRUE;
      }
   }

   return FALSE;
}

static void hb_clsCopyClass( PCLASS pClsDst, PCLASS pClsSrc )
{
   PMETHOD pMethod;
   ULONG ulLimit;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsCopyClass(%p,%p)", pClsDst, pClsSrc));

   hb_clsDictInit( pClsDst, pClsSrc->uiHashKey );
   pClsDst->fHasOnError = pClsSrc->fHasOnError;
   pClsDst->fHasDestructor = pClsSrc->fHasDestructor;

   /* CLASS DATA Not Shared ( new array, new value ) */
   pClsDst->pClassDatas  = hb_arrayClone( pClsSrc->pClassDatas );
   /* do not copy shared data array - just simply create new one */
   pClsDst->pSharedDatas = hb_itemArrayNew( 0 );
   pClsDst->pInlines = hb_arrayClone( pClsSrc->pInlines );
   pClsDst->uiDatas = pClsSrc->uiDatas;
   pClsDst->ulOpFlags = pClsSrc->ulOpFlags;

   if( pClsSrc->uiInitDatas )
   {
      ULONG ulSize = ( ULONG ) pClsSrc->uiInitDatas * sizeof( INITDATA );
      USHORT uiData;

      pClsDst->uiInitDatas = pClsSrc->uiInitDatas;
      pClsDst->pInitData = ( PINITDATA ) hb_xgrab( ulSize );
      memcpy( pClsDst->pInitData, pClsSrc->pInitData, ulSize );
      for( uiData = 0; uiData < pClsDst->uiInitDatas; ++uiData )
      {
         if( pClsDst->pInitData[ uiData ].uiType == HB_OO_MSG_INITIALIZED )
            pClsDst->pInitData[ uiData ].uiType = HB_OO_MSG_CLASSDATA;
         pClsDst->pInitData[ uiData ].pInitValue =
                        hb_itemNew( pClsDst->pInitData[ uiData ].pInitValue );
      }
   }

   ulLimit = hb_clsMthNum( pClsSrc );
#ifdef HB_MSG_POOL
   memcpy( pClsDst->puiMsgIdx, pClsSrc->puiMsgIdx,
      ( ( ( ULONG ) pClsSrc->uiHashKey + 1 ) << BUCKETBITS ) * sizeof( USHORT ) );
   pClsDst->uiMethodCount = pClsSrc->uiMethodCount;
   pClsDst->pMethods = ( PMETHOD ) hb_xrealloc( pClsDst->pMethods,
                                                ulLimit * sizeof( METHOD ) );
#endif
   memcpy( pClsDst->pMethods, pClsSrc->pMethods, ulLimit * sizeof( METHOD ) );
   pClsDst->uiMethods = pClsSrc->uiMethods;

   pMethod = pClsDst->pMethods;
   do
   {
      if( pMethod->pMessage )
      {
         hb_clsUpdateHiddenMessages( pMethod, pMethod, pClsDst );
         pMethod->uiScope |= HB_OO_CLSTP_SUPER;
      }
      ++pMethod;
   }
   while( --ulLimit );
}

static BOOL hb_clsIsFriendSymbol( PCLASS pClass, PHB_SYMB pSym )
{
   USHORT uiCount;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsIsFriendSymbol(%p,%p)", pClass, pSym));

   if( pSym >= pClass->pFriendModule &&
       pSym < pClass->pFriendModule + pClass->uiFriendModule )
      return TRUE;

   for( uiCount = 0; uiCount < pClass->uiFriendSyms; ++uiCount )
   {
      if( pClass->pFriendSyms[ uiCount ] == pSym )
         return TRUE;
   }

   return FALSE;
}

static void hb_clsAddFriendSymbol( PCLASS pClass, PHB_SYMB pSym )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_clsAddFriendSymbol(%p,%p)", pClass, pSym));

   if( ! hb_clsIsFriendSymbol( pClass, pSym ) )
   {
      if( pClass->uiFriendSyms == 0 )
      {
         pClass->pFriendSyms = ( PHB_SYMB * ) hb_xgrab( sizeof( PHB_SYMB ) );
         pClass->pFriendSyms[ 0 ] = pSym;
         pClass->uiFriendSyms++;
      }
      else 
      {
         pClass->pFriendSyms = ( PHB_SYMB * ) hb_xrealloc( pClass->pFriendSyms,
                           ( pClass->uiFriendSyms + 1 ) * sizeof( PHB_SYMB ) );
         pClass->pFriendSyms[ pClass->uiFriendSyms++ ] = pSym;
      }
   }
}

/*
 * initialize Classy/OO system at HVM startup
 */
void hb_clsInit( void )
{
   PHB_SYMB pOpSym;
   USHORT uiOperator;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsInit()"));

   for( uiOperator = 0, pOpSym = s_opSymbols; uiOperator <= HB_OO_MAX_OPERATOR;
        ++uiOperator, ++pOpSym )
   {
      pOpSym->pDynSym = hb_dynsymGetCase( pOpSym->szName );
   }

   s___msgDestructor.pDynSym  = hb_dynsymGetCase( s___msgDestructor.szName );
   s___msgOnError.pDynSym     = hb_dynsymGetCase( s___msgOnError.szName );

   s___msgClassName.pDynSym   = hb_dynsymGetCase( s___msgClassName.szName );  /* Standard messages        */
   s___msgClassH.pDynSym      = hb_dynsymGetCase( s___msgClassH.szName );     /* Not present in classdef. */
   s___msgClassSel.pDynSym    = hb_dynsymGetCase( s___msgClassSel.szName );
   s___msgExec.pDynSym        = hb_dynsymGetCase( s___msgExec.szName );
   s___msgName.pDynSym        = hb_dynsymGetCase( s___msgName.szName );
   s___msgNew.pDynSym         = hb_dynsymGetCase( s___msgNew.szName );
   s___msgSymbol.pDynSym      = hb_dynsymGetCase( s___msgSymbol.szName );
   s___msgKeys.pDynSym        = hb_dynsymGetCase( s___msgKeys.szName );
   s___msgValues.pDynSym      = hb_dynsymGetCase( s___msgValues.szName );
/*
   s___msgClsParent.pDynSym   = hb_dynsymGetCase( s___msgClsParent.szName );
   s___msgClass.pDynSym       = hb_dynsymGetCase( s___msgClass.szName );
*/
   s___msgEnumIndex.pDynSym   = hb_dynsymGetCase( s___msgEnumIndex.szName );
   s___msgEnumBase.pDynSym    = hb_dynsymGetCase( s___msgEnumBase.szName );
   s___msgEnumKey.pDynSym     = hb_dynsymGetCase( s___msgEnumKey.szName );
   s___msgEnumValue.pDynSym   = hb_dynsymGetCase( s___msgEnumValue.szName );

   s___msgWithObjectPush.pDynSym = hb_dynsymGetCase( s___msgWithObjectPush.szName );
   s___msgWithObjectPop.pDynSym  = hb_dynsymGetCase( s___msgWithObjectPop.szName );
}

/*
 * hb_clsRelease( <pClass> )
 *
 * Release a class from memory
 */
static void hb_clsRelease( PCLASS pClass )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_clsRelease(%p)", pClass));

   if( pClass->uiInitDatas )
   {
      USHORT ui = pClass->uiInitDatas;
      PINITDATA pInitData = pClass->pInitData;

      do
      {
         hb_itemRelease( pInitData->pInitValue );
         ++pInitData;
      }
      while( --ui );
      hb_xfree( pClass->pInitData );
   }

   if( pClass->szName )
      hb_xfree( pClass->szName );
   if( pClass->pMethods )
      hb_xfree( pClass->pMethods );
   if( pClass->uiFriendSyms )
      hb_xfree( pClass->pFriendSyms );
#ifdef HB_MSG_POOL
   if( pClass->puiMsgIdx )
      hb_xfree( pClass->puiMsgIdx );
#endif
   if( pClass->pClassDatas )
      hb_itemRelease( pClass->pClassDatas );
   if( pClass->pSharedDatas )
      hb_itemRelease( pClass->pSharedDatas );
   if( pClass->pInlines )
      hb_itemRelease( pClass->pInlines );
}


/*
 * hb_clsReleaseAll()
 *
 * Release all classes
 */
void hb_clsReleaseAll( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_clsReleaseAll()"));

   if( s_uiClasses )
   {
      USHORT uiClass = s_uiClasses;

      /* It blocks destructor execution - don't move. [druzus] */
      s_uiClasses = 0;

      do
      {
         hb_clsRelease( &s_pClasses[ uiClass ] );
      }
      while( --uiClass );

      hb_xfree( s_pClasses );
      s_pClasses  = NULL;
   }
}

/* Mark all internal data as used so it will not be released by the
 * garbage collector
 */

void hb_clsIsClassRef( void )
{
   /*
    * All internal items are allocated with hb_itemNew()
    * GC knows them and scan itself so it's not necessary
    * to repeat scanning here [druzus].
    */
#if 0
   USHORT uiClass = s_uiClasses;
   PCLASS pClass = s_pClasses + 1;

   HB_TRACE(HB_TR_DEBUG, ("hb_clsIsClassRef()"));

   while( uiClass-- )
   {
      if( pClass->pInlines )
         hb_gcItemRef( pClass->pInlines );

      if( pClass->pClassDatas )
         hb_gcItemRef( pClass->pClassDatas );

      if( pClass->pSharedDatas )
         hb_gcItemRef( pClass->pSharedDatas );

      if( pClass->uiInitDatas )
      {
         USHORT ui = pClass->uiInitDatas;
         PINITDATA pInitData = pClass->pInitData;

         do
         {
            if( HB_IS_GCITEM( pInitData->pInitValue ) )
               hb_gcItemRef( pInitData->pInitValue );
            ++pInitData;
         }
         while( --ui );
      }
      ++pClass;
   }
#endif
}

HB_EXPORT BOOL hb_clsIsParent( USHORT uiClass, const char * szParentName )
{
   if( uiClass && uiClass <= s_uiClasses )
   {
      PCLASS pClass = &s_pClasses[ uiClass ];

      if( strcmp( pClass->szName, szParentName ) == 0 )
         return TRUE;
      else
      {
         PHB_DYNS pMsg = hb_dynsymFindName( szParentName );

         if( pMsg )
            return hb_clsHasParent( &s_pClasses[ uiClass ], pMsg );
      }
   }

   return FALSE;
}

HB_EXPORT USHORT hb_objGetClass( PHB_ITEM pItem )
{
   if( pItem && HB_IS_ARRAY( pItem ) )
      return pItem->item.asArray.value->uiClass;
   else
      return 0;
}

/* get object class handle using class name and class function name */
HB_EXPORT USHORT hb_objSetClass( PHB_ITEM pItem, const char * szClass, const char * szFunc )
{
   USHORT uiClass = 0;

   if( pItem && HB_IS_ARRAY( pItem ) &&
       pItem->item.asArray.value->uiClass == 0 )
   {
      uiClass = pItem->item.asArray.value->uiClass =
                                          hb_clsFindClass( szClass, szFunc );
   }
   return uiClass;
}

/* ================================================ */

/*
 * Get the class name of an object
 */
HB_EXPORT const char * hb_objGetClsName( PHB_ITEM pObject )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_objGetClsName(%p)", pObject));

   if( HB_IS_ARRAY( pObject ) )
   {
      if( pObject->item.asArray.value->uiClass != 0 )
         return s_pClasses[ pObject->item.asArray.value->uiClass ].szName;
      else
         return "ARRAY";
   }
   /* built in types */
   else if( HB_IS_NIL( pObject ) )
      return "NIL";

   else if( HB_IS_STRING( pObject ) )
      return "CHARACTER";

   else if( HB_IS_NUMERIC( pObject ) )
      return "NUMERIC";

   else if( HB_IS_DATE( pObject ) )
      return "DATE";

   else if( HB_IS_LOGICAL( pObject ) )
      return "LOGICAL";

   else if( HB_IS_BLOCK( pObject ) )
      return "BLOCK";

   else if( HB_IS_HASH( pObject ) )
      return "HASH";

   else if( HB_IS_POINTER( pObject ) )
      return "POINTER";

   else if( HB_IS_SYMBOL( pObject ) )
      return "SYMBOL";

   else 
      return "UNKNOWN";
}

HB_EXPORT const char * hb_clsName( USHORT uiClass )
{
   if( uiClass && uiClass <= s_uiClasses )
      return s_pClasses[ uiClass ].szName;
   else
      return NULL;
}

HB_EXPORT const char * hb_clsFuncName( USHORT uiClass )
{
   if( uiClass && uiClass <= s_uiClasses )
      return s_pClasses[ uiClass ].pClassFuncSym ?
             s_pClasses[ uiClass ].pClassFuncSym->szName :
             "";
   else
      return NULL;
}

HB_EXPORT USHORT hb_clsFindClass( const char * szClass, const char * szFunc )
{
   USHORT uiClass;

   for( uiClass = 1; uiClass <= s_uiClasses; uiClass++ )
   {
      if( strcmp( szClass, s_pClasses[ uiClass ].szName ) == 0 &&
          ( !szFunc || ( !s_pClasses[ uiClass ].pClassFuncSym ? !*szFunc :
            strcmp( szFunc, s_pClasses[ uiClass ].pClassFuncSym->szName ) == 0 ) ) )
      {
         return uiClass;
      }
   }
   return 0;
}

/*
 * Get the real class name of an object message
 * Will return the class name from wich the message is inherited in case
 * of inheritance.
 */
HB_EXPORT const char * hb_objGetRealClsName( PHB_ITEM pObject, const char * szName )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_objGetrealClsName(%p,%s)", pObject, szName));

   if( HB_IS_OBJECT( pObject ) )
   {
      USHORT uiClass;

      uiClass = pObject->item.asArray.value->uiClass;
      if( uiClass && uiClass <= uiClass )
      {
         PHB_DYNS pMsg = hb_dynsymFindName( szName );

         if( pMsg )
         {
            PMETHOD pMethod = hb_clsFindMsg( &s_pClasses[ uiClass ], pMsg );
            if( pMethod )
               uiClass = pMethod->uiSprClass;
         }
         if( uiClass && uiClass <= s_uiClasses )
            return s_pClasses[ uiClass ].szName;
      }
   }

   return hb_objGetClsName( pObject );
}

#if defined( HB_CLASSY_BLOCK_SCOPE )
static LONG hb_clsSenderOffset( void )
{
   LONG lOffset = hb_stackBaseProcOffset( 1 );

   if( lOffset > 0 )
   {
      /* Is it inline method? */
      if( lOffset > 0 && HB_IS_BLOCK( hb_stackItem( lOffset + 1 ) ) &&
          ( hb_stackItem( lOffset )->item.asSymbol.value == &hb_symEval ||
            hb_stackItem( lOffset )->item.asSymbol.value->pDynSym ==
            hb_symEval.pDynSym ) )
      {
         lOffset = hb_stackItem( lOffset )->item.asSymbol.stackstate->lBaseItem;

         /* I do not like it but Class(y) makes sth like that. [druzus] */
         while( lOffset > 0 &&
                hb_stackItem( lOffset )->item.asSymbol.stackstate->uiClass == 0 )
            lOffset = hb_stackItem( lOffset )->item.asSymbol.stackstate->lBaseItem;
      }
      return lOffset;
   }
   return -1;
}
#endif

#if 0
static USHORT hb_clsSenderClasss( void )
{
   LONG lOffset = hb_clsSenderOffset();

   if( lOffset > 0 )
      return hb_stackItem( lOffset )->item.asSymbol.stackstate->uiClass;
   else
      return 0;
}
#endif

static USHORT hb_clsSenderMethodClasss( void )
{
   LONG lOffset = hb_clsSenderOffset();

   if( lOffset > 0 )
   {
      PHB_STACK_STATE pStack = hb_stackItem( lOffset )->item.asSymbol.stackstate;

      if( pStack->uiClass )
         return ( s_pClasses[ pStack->uiClass ].pMethods +
                  pStack->uiMethod )->uiSprClass;
   }
   return 0;
}

static PHB_SYMB hb_clsSenderSymbol( void )
{
   PHB_SYMB pSym = NULL;
   LONG lOffset = hb_clsSenderOffset();

   if( lOffset > 0 )
   {
      pSym = hb_stackItem( lOffset )->item.asSymbol.value;

      if( pSym == &hb_symEval || pSym->pDynSym == hb_symEval.pDynSym )
      {
         PHB_ITEM pBlock = hb_stackItem( lOffset + 1 );

         if( HB_IS_BLOCK( pBlock ) )
            pSym = pBlock->item.asBlock.value->pDefSymb;
      }
   }

   return hb_vmGetRealFuncSym( pSym );
}

static USHORT hb_clsSenderObjectClasss( void )
{
   LONG lOffset = hb_clsSenderOffset();

   if( lOffset > 0 )
   {
      PHB_ITEM pSender = hb_stackItem( lOffset + 1 );

      if( HB_IS_ARRAY( pSender ) )
         return pSender->item.asArray.value->uiClass;
   }
   return 0;
}

static PHB_SYMB hb_clsValidScope( PMETHOD pMethod, PHB_STACK_STATE pStack )
{
   if( pMethod->uiScope & ( HB_OO_CLSTP_HIDDEN | HB_OO_CLSTP_PROTECTED |
                            HB_OO_CLSTP_OVERLOADED ) )
   {
      USHORT uiSenderClass = hb_clsSenderMethodClasss();

      if( uiSenderClass )
      {
         if( uiSenderClass == pMethod->uiSprClass )
            return pMethod->pFuncSym;

         /*
          * Warning!!! Friends cannot access overloaded non virtual methods.
          * This feature is available _ONLY_ for real class members, [druzus]
          */
         if( pMethod->uiScope & HB_OO_CLSTP_OVERLOADED )
         {
            PCLASS pClass = &s_pClasses[ uiSenderClass ];
            PMETHOD pHiddenMthd = hb_clsFindMsg( pClass, pMethod->pMessage );

            if( pHiddenMthd && ( pHiddenMthd->uiScope & HB_OO_CLSTP_NONVIRTUAL ) &&
                pHiddenMthd->uiSprClass == uiSenderClass )
            {
               pStack->uiClass = uiSenderClass;
               pStack->uiMethod = ( USHORT ) ( pHiddenMthd - pClass->pMethods );
               return pHiddenMthd->pFuncSym;
            }
         }

         if( pMethod->uiScope & HB_OO_CLSTP_HIDDEN )
         {
            if( ! hb_clsIsFriendSymbol( &s_pClasses[ pMethod->uiSprClass ],
                              s_pClasses[ uiSenderClass ].pClassFuncSym ) )
               return &s___msgScopeErr;
         }
         else if( pMethod->uiScope & HB_OO_CLSTP_PROTECTED &&
             ! hb_clsHasParent( &s_pClasses[ pStack->uiClass ],
                                s_pClasses[ uiSenderClass ].pClassSym ) &&
             ! hb_clsHasParent( &s_pClasses[ uiSenderClass ],
                                s_pClasses[ pStack->uiClass ].pClassSym ) &&
             ! hb_clsIsFriendSymbol( &s_pClasses[ pMethod->uiSprClass ],
                                     s_pClasses[ uiSenderClass ].pClassFuncSym ) &&
             ( pStack->uiClass == pMethod->uiSprClass ||
               ! hb_clsIsFriendSymbol( &s_pClasses[ pStack->uiClass ],
                              s_pClasses[ uiSenderClass ].pClassFuncSym ) ) )
            return &s___msgScopeErr;
      }
      else if( pMethod->uiScope & ( HB_OO_CLSTP_HIDDEN | HB_OO_CLSTP_PROTECTED ) )
      {
         PHB_SYMB pSym = hb_clsSenderSymbol();

         if( ! hb_clsIsFriendSymbol( &s_pClasses[ pMethod->uiSprClass ], pSym ) )
         {
            if( ( pMethod->uiScope & HB_OO_CLSTP_HIDDEN ) ||
                ! hb_clsIsFriendSymbol( &s_pClasses[ pStack->uiClass ], pSym ) )
               return &s___msgScopeErr;
         }
      }
   }

   return pMethod->pFuncSym;
}

static void hb_clsMakeSuperObject( PHB_ITEM pDest, PHB_ITEM pObject,
                                   USHORT uiSuperClass )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_clsMakeSuperObject(%p, %p, %hu)", pDest, pObject, uiSuperClass));

   /* create a fake object array */
   hb_arrayNew( pDest, 1 );
   /* Now save the Self object as the 1st elem. */
   hb_arraySet( pDest, 1, pObject );
   /* And transform it into a fake object */
   /* backup of actual handel */
   pDest->item.asArray.value->uiPrevCls = pObject->item.asArray.value->uiClass;
   /* superclass handel casting */
   pDest->item.asArray.value->uiClass = uiSuperClass;
}

/*
 * <pFuncSym> = hb_objGetMethod( <pObject>, <pMessage>, <pStackState> )
 *
 * Internal function to the function pointer of a message of an object
 */
PHB_SYMB hb_objGetMethod( PHB_ITEM pObject, PHB_SYMB pMessage,
                          PHB_STACK_STATE pStack )
{
   PCLASS pClass = NULL;
   PHB_DYNS pMsg;

   HB_TRACE(HB_TR_DEBUG, ("hb_objGetMethod(%p, %p, %p)", pObject, pMessage, pStack));

   pMsg = pMessage->pDynSym;

   if( HB_IS_ARRAY( pObject ) )
   {
      if( pObject->item.asArray.value->uiClass )
      {
         pClass = &s_pClasses[ pObject->item.asArray.value->uiClass ];
         if( pStack )
         {

            pStack->uiClass = pObject->item.asArray.value->uiClass;
            if( pObject->item.asArray.value->uiPrevCls )
            {
               /*
                * Copy real object - do not move! the same super casted
                * object can be used more then once and we mustn't destroy it.
                * We can safely use hb_stackReturnItem() here.
                */
               hb_itemCopy( hb_stackReturnItem(), pObject->item.asArray.value->pItems );
               /* move real object back to the stack */
               hb_itemMove( pObject, hb_stackReturnItem() );
            }
#ifdef HB_MSG_POOL
            {
               USHORT uiBucket = BUCKETSIZE, * puiMsgIdx =
                  pClass->puiMsgIdx + hb_clsBucketPos( pMsg, pClass->uiHashKey );

               do
               {
                  PMETHOD pMethod = &pClass->pMethods[ * puiMsgIdx ];
                  if( pMethod->pMessage == pMsg )
                  {
                     pStack->uiMethod = * puiMsgIdx;
                     return hb_clsValidScope( pMethod, pStack );
                  }
                  ++puiMsgIdx;
               }
               while( --uiBucket );
            }
#else
            {
               PMETHOD pMethod = hb_clsFindMsg( pClass, pMsg );
               if( pMethod )
               {
                  pStack->uiMethod = ( USHORT ) ( pMethod - pClass->pMethods );
                  return hb_clsValidScope( pMethod, pStack );
               }
            }
#endif
         }
         else
         {
            PMETHOD pMethod = hb_clsFindMsg( pClass, pMsg );
            if( pMethod )
               return pMethod->pFuncSym;
         }
      }
   }
   else if( HB_IS_BLOCK( pObject ) )
   {
      if( pMsg == hb_symEval.pDynSym )
         return &hb_symEval;
   }
   else if( HB_IS_BYREF( pObject ) )
   {
      if( pStack )
      {
         /* method of enumerator variable from FOR EACH statement
          */
         PHB_ITEM pEnum = hb_itemUnRefOnce( pObject );

         if( HB_IS_ENUM( pEnum ) )
         {
            /*
             * Do actions here - we already have unreferenced pEnum so
             * it will be a little bit faster but in the future it's
             * possible that I'll move it to separate function when
             * I'll add enumerators overloading. [druzus]
             */
            if( pMsg == s___msgEnumIndex.pDynSym )
            {
               hb_itemPutNL( hb_stackReturnItem(), pEnum->item.asEnum.offset );
               if( hb_pcount() > 0 && ISNUM( 1 ) )
                  pEnum->item.asEnum.offset = hb_itemGetNL( hb_param( 1, HB_IT_ANY ) );
               return &s___msgEnumIndex;
            }
            else if( pMsg == s___msgEnumKey.pDynSym )
            {
               PHB_ITEM pBase = HB_IS_BYREF( pEnum->item.asEnum.basePtr ) ?
                                hb_itemUnRef( pEnum->item.asEnum.basePtr ) :
                                pEnum->item.asEnum.basePtr;
               if( HB_IS_HASH( pBase ) )
               {
                  pBase = hb_hashGetKeyAt( pBase, pEnum->item.asEnum.offset );
                  if( pBase )
                     hb_itemCopy( hb_stackReturnItem(), pBase );
               }
               return &s___msgEnumKey;
            }
            else if( pMsg == s___msgEnumBase.pDynSym )
            {
               if( HB_IS_BYREF( pEnum->item.asEnum.basePtr ) )
                  hb_itemCopy( hb_stackReturnItem(),
                               hb_itemUnRef( pEnum->item.asEnum.basePtr ) );
               else
                  hb_itemCopy( hb_stackReturnItem(),
                               pEnum->item.asEnum.basePtr );
               if( hb_pcount() > 0 )
                  hb_itemCopy( pEnum->item.asEnum.basePtr,
                               hb_itemUnRef( hb_stackItemFromBase( 1 ) ) );
               return &s___msgEnumBase;
            }
            else if( pMsg == s___msgEnumValue.pDynSym )
            {
               pEnum = hb_itemUnRef( pEnum );
               hb_itemCopy( hb_stackReturnItem(), pEnum );
               if( hb_pcount() > 0 )
                  hb_itemCopy( pEnum, hb_itemUnRef( hb_stackItemFromBase( 1 ) ) );
               return &s___msgEnumValue;
            }
         }
      }
   }
   else if( HB_IS_SYMBOL( pObject ) )
   {
      if( pMsg == s___msgExec.pDynSym || pMsg == hb_symEval.pDynSym )
      {
         if( ! pObject->item.asSymbol.value->value.pFunPtr &&
               pObject->item.asSymbol.value->pDynSym )
            return pObject->item.asSymbol.value->pDynSym->pSymbol;
         else
            return pObject->item.asSymbol.value;
      }
      else if( pMsg == s___msgName.pDynSym )
      {
         hb_itemPutC( hb_stackReturnItem(),
                      pObject->item.asSymbol.value->szName );
         return &s___msgName;
      }
   }
   else if( HB_IS_HASH( pObject ) )
   {
      if( pMsg == s___msgKeys.pDynSym )
      {
         hb_itemRelease( hb_itemReturnForward( hb_hashGetKeys( pObject ) ) );
         return &s___msgKeys;
      }
      else if( pMsg == s___msgValues.pDynSym )
      {
         hb_itemRelease( hb_itemReturnForward( hb_hashGetValues( pObject ) ) );
         return &s___msgValues;
      }
#if defined( HB_HASH_MSG_ITEMS )
      else
      {
         if( hb_pcount() == 1 && pMessage->szName[ 0 ] == '_' )
         {  /* ASSIGN */
            PHB_ITEM pIndex = hb_itemPutCConst( hb_stackAllocItem(), pMessage->szName + 1 );
            PHB_ITEM pDest = hb_hashGetItemPtr( pObject, pIndex, HB_HASH_AUTOADD_ASSIGN );
            hb_stackPop();
            if( pDest )
            {
               PHB_ITEM pValue = hb_param( 1, HB_IT_ANY );
               hb_itemCopyFromRef( pDest, pValue );
               hb_itemReturn( pValue );
               return &s___msgVirtual;
            }
         }
         else if( hb_pcount() == 0 )
         {  /* ACCESS */
            PHB_ITEM pIndex = hb_itemPutCConst( hb_stackAllocItem(), pMessage->szName );
            PHB_ITEM pValue = hb_hashGetItemPtr( pObject, pIndex, HB_HASH_AUTOADD_ACCESS );
            hb_stackPop();
            if( pValue )
            {
               hb_itemReturn( pValue );
               return &s___msgVirtual;
            }
         }
      }
#endif
   }

   /* Default messages here */
   if( pMsg == s___msgWithObjectPush.pDynSym )
   {
      if( pStack )
      {
         PHB_ITEM pItem = hb_stackWithObjectItem();
         if( pItem )
         {
            /* push current WITH OBJECT object */
            hb_itemCopy( hb_stackReturnItem(), pItem );
            return &s___msgWithObjectPush;
         }
      }
   }
   else if( pMsg == s___msgWithObjectPop.pDynSym )
   {
      if( pStack )
      {
         PHB_ITEM pItem = hb_stackWithObjectItem();
         if( pItem )
         {
            /* replace current WITH OBJECT object */
            hb_itemCopy( pItem, hb_stackItemFromBase( 1 ) );
            hb_itemCopy( hb_stackReturnItem(), pItem );
            return &s___msgWithObjectPop;
         }
      }
   }

   else if( pMsg == s___msgClassName.pDynSym )
      return &s___msgClassName;

   else if( pMsg == s___msgClassH.pDynSym )
      return &s___msgClassH;

   else if( pMsg == s___msgClassSel.pDynSym )
      return &s___msgClassSel;

/*
   else if( pMsg == s___msgClsParent.pDynSym )
      return &s___msgClsParent;

   else if( pMsg == s___msgClass.pDynSym )
      return &s___msgClass;
*/
   if( pStack )
   {
      if( pClass && pClass->fHasOnError )
      {
         PMETHOD pMethod = hb_clsFindMsg( pClass, s___msgOnError.pDynSym );
         if( pMethod )
         {
            pStack->uiMethod = ( USHORT ) ( pMethod - pClass->pMethods );
            return pMethod->pFuncSym;
         }
      }

      /* remove this line if you want default HVM error message */
      return &s___msgNoMethod;
   }
   return NULL;
}

BOOL hb_objGetVarRef( PHB_ITEM pObject, PHB_SYMB pMessage,
                      PHB_STACK_STATE pStack )
{
   PHB_SYMB pExecSym;

#if defined( HB_HASH_MSG_ITEMS )
   if( HB_IS_HASH( pObject ) )
   {
      PHB_ITEM pIndex = hb_itemPutCConst( hb_stackAllocItem(), pMessage->szName + 1 );
      PHB_ITEM pValue = hb_hashGetItemRefPtr( pObject, pIndex );
      hb_stackPop();
      if( pValue )
         hb_itemReturn( pValue );
      return pValue != NULL;
   }
#endif

   pExecSym = hb_objGetMethod( pObject, pMessage, pStack );
   if( pExecSym )
   {
      if( pExecSym->value.pFunPtr == hb___msgSetData )
      {
         USHORT uiObjClass = pObject->item.asArray.value->uiClass;
         PCLASS pClass     = &s_pClasses[ pStack->uiClass ];
         PMETHOD pMethod   = pClass->pMethods + pStack->uiMethod;
         ULONG ulIndex     = pMethod->uiData;

         if( pStack->uiClass != uiObjClass )
            ulIndex += hb_clsParentInstanceOffset( &s_pClasses[ uiObjClass ],
                                 s_pClasses[ pMethod->uiSprClass ].pClassSym );
         else
            ulIndex += pMethod->uiOffset;

         /* will arise only if the class has been modified after first instance */
         if( ulIndex > hb_arrayLen( pObject ) ) /* Resize needed */
            hb_arraySize( pObject, ulIndex );   /* Make large enough */

         return hb_arrayGetItemRef( pObject, ulIndex, hb_stackReturnItem() );
      }
      else if( pExecSym->value.pFunPtr == hb___msgSetClsData )
      {
         PCLASS pClass   = &s_pClasses[ pStack->uiClass ];
         PMETHOD pMethod = pClass->pMethods + pStack->uiMethod;

         return hb_arrayGetItemRef( pClass->pClassDatas, pMethod->uiData,
                                    hb_stackReturnItem() );
      }
      else if( pExecSym->value.pFunPtr == hb___msgSetShrData )
      {
         PCLASS pClass   = &s_pClasses[ pStack->uiClass ];
         PMETHOD pMethod = pClass->pMethods + pStack->uiMethod;

         return hb_arrayGetItemRef( s_pClasses[ pMethod->uiSprClass ].pSharedDatas,
                                    pMethod->uiData, hb_stackReturnItem() );
      }
      else if( pExecSym->value.pFunPtr == hb___msgScopeErr )
         (pExecSym->value.pFunPtr)();
   }

   return FALSE;
}

/*
 * Check if class has object destructors
 */
BOOL hb_clsHasDestructor( USHORT uiClass )
{
   if( uiClass && uiClass <= s_uiClasses )
      return s_pClasses[ uiClass ].fHasDestructor;
   else
      return FALSE;
}

/*
 * Call all known supper destructors
 */
static void hb_objSupperDestructorCall( PHB_ITEM pObject, PCLASS pClass )
{
   PMETHOD pMethod = pClass->pMethods;
   ULONG   ulLimit = hb_clsMthNum( pClass );
   BYTE * pbClasses;
   USHORT uiClass;

   pbClasses = ( BYTE * ) hb_xgrab( s_uiClasses + 1 );
   memset( pbClasses, 0, s_uiClasses + 1 );

   do
   {
      if( pMethod->pMessage )
      {
         if( pMethod->pFuncSym == &s___msgSuper )
         {
            PCLASS pSupperClass = &s_pClasses[ pMethod->uiSprClass ];
            if( pSupperClass->fHasDestructor && pSupperClass != pClass )
               pbClasses[ pMethod->uiSprClass ] |= 1;
         }
         else if( pMethod->pMessage == s___msgDestructor.pDynSym )
            pbClasses[ pMethod->uiSprClass ] |= 2;
      }
      ++pMethod;
   }
   while( --ulLimit );

   for( uiClass = s_uiClasses; uiClass; --uiClass )
   {
      if( pbClasses[ uiClass ] == 1 )
      {
         hb_vmPushSymbol( &s___msgDestructor );
         hb_clsMakeSuperObject( hb_stackAllocItem(), pObject, uiClass );
         hb_vmSend( 0 );
         if( hb_vmRequestQuery() != 0 )
            break;
      }
   }

   hb_xfree( pbClasses );
}

/*
 * Call object destructor
 */
void hb_objDestructorCall( PHB_ITEM pObject )
{
   if( HB_IS_OBJECT( pObject ) &&
       pObject->item.asArray.value->uiClass <= s_uiClasses )
   {
      PCLASS pClass = &s_pClasses[ pObject->item.asArray.value->uiClass ];

      if( pClass->fHasDestructor )
      {
         if( hb_vmRequestReenter() )
         {
            hb_vmPushSymbol( &s___msgDestructor );
            hb_vmPush( pObject );
            hb_vmSend( 0 );
            if( hb_vmRequestQuery() == 0 )
               hb_objSupperDestructorCall( pObject, pClass );
            hb_vmRequestRestore();
         }
      }
   }
}

/*
 * Check if object has a given operator
 */
BOOL hb_objHasOperator( PHB_ITEM pObject, USHORT uiOperator )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_objHasOperator(%p,%hu)", pObject, uiOperator));

   if( HB_IS_OBJECT( pObject ) )
   {
      PCLASS pClass = &s_pClasses[ pObject->item.asArray.value->uiClass ];
      return ( pClass->ulOpFlags & ( 1UL << uiOperator ) ) != 0;
   }

   return FALSE;
}

/*
 * Call object operator. If pMsgArg is NULL then operator is unary.
 * Function return TRUE when object class overloads given operator
 * and FALSE otherwise. [druzus]
 */
BOOL hb_objOperatorCall( USHORT uiOperator, HB_ITEM_PTR pResult, PHB_ITEM pObject,
                         PHB_ITEM pMsgArg1, PHB_ITEM pMsgArg2 )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_objOperatorCall(%hu,%p,%p,%p,%p)", uiOperator, pResult, pObject, pMsgArg1, pMsgArg2));

   if( hb_objHasOperator( pObject, uiOperator ) )
   {
      hb_vmPushSymbol( s_opSymbols + uiOperator );
      hb_vmPush( pObject );
      hb_itemSetNil( hb_stackReturnItem() );
      if( pMsgArg1 )
      {
         hb_vmPush( pMsgArg1 );
         if( pMsgArg2 )
         {
            hb_vmPush( pMsgArg2 );
            hb_vmSend( 2 );
         }
         else
            hb_vmSend( 1 );
      }
      else
         hb_vmSend( 0 );

      /* store the return value */
      hb_itemMove( pResult, hb_stackReturnItem() );
      return TRUE;
   }
   return FALSE;
}

/*
 * return TRUE if object has a given message
 */
HB_EXPORT BOOL hb_objHasMessage( PHB_ITEM pObject, PHB_DYNS pMessage )
{
   return hb_objGetMethod( pObject, pMessage->pSymbol, NULL ) != NULL;
}

/*
 * <bool> = hb_objHasMsg( <pObject>, <szString> )
 *
 * Check whether <szString> is an existing message for object.
 *
 * <uPtr> should be read as a boolean
 */
HB_EXPORT BOOL hb_objHasMsg( PHB_ITEM pObject, const char *szString )
{
   PHB_DYNS pDynSym;

   HB_TRACE(HB_TR_DEBUG, ("hb_objHasMsg(%p, %s)", pObject, szString));

   pDynSym = hb_dynsymFindName( szString );
   if( pDynSym )
   {
      return hb_objGetMethod( pObject, pDynSym->pSymbol, NULL ) != NULL;
   }
   else
   {
      return FALSE;
   }
}

HB_EXPORT PHB_ITEM hb_objSendMessage( PHB_ITEM pObject, PHB_DYNS pMsgSym, ULONG ulArg, ... )
{
   if( pObject && pMsgSym )
   {
      hb_vmPushSymbol( pMsgSym->pSymbol );
      hb_vmPush( pObject );

      if( ulArg )
      {
         unsigned long i;
         va_list ap;

         va_start( ap, ulArg );
         for( i = 0; i < ulArg; i++ )
         {
            hb_vmPush( va_arg( ap, PHB_ITEM ) );
         }
         va_end( ap );
      }
      hb_vmSend( (USHORT) ulArg );
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 3000, NULL, "__ObjSendMessage()", 0 );
   }

   return hb_stackReturnItem();
}

HB_EXPORT PHB_ITEM hb_objSendMsg( PHB_ITEM pObject, const char *sMsg, ULONG ulArg, ... )
{
   hb_vmPushSymbol( hb_dynsymGet( sMsg )->pSymbol );
   hb_vmPush( pObject );
   if( ulArg )
   {
      unsigned long i;
      va_list ap;

      va_start( ap, ulArg );
      for( i = 0; i < ulArg; i++ )
      {
         hb_vmPush( va_arg( ap, PHB_ITEM ) );
      }
      va_end( ap );
   }
   hb_vmSend( (USHORT) ulArg );

   return hb_stackReturnItem();
}

static PHB_DYNS hb_objGetMsgSym( PHB_ITEM pMessage )
{
   PHB_DYNS pDynSym = NULL;

   if( pMessage )
   {
      const char * szMsg = NULL;

      if( HB_IS_STRING( pMessage ) )
         szMsg = pMessage->item.asString.value;
      else if( HB_IS_SYMBOL( pMessage ) )
      {
         pDynSym = pMessage->item.asSymbol.value->pDynSym;
         if( !pDynSym )
            szMsg = pMessage->item.asSymbol.value->szName;
      }

      if( szMsg && *szMsg )
         pDynSym = hb_dynsymGet( szMsg );
   }

   return pDynSym;
}

static PHB_SYMB hb_objGetFuncSym( PHB_ITEM pItem )
{
   if( pItem )
   {
      if( HB_IS_SYMBOL( pItem ) )
         return pItem->item.asSymbol.value;
      else if( HB_IS_STRING( pItem ) )
      {
         PHB_DYNS pDynSym = hb_dynsymFindName( hb_itemGetCPtr( pItem ) );

         if( pDynSym && pDynSym->pSymbol->value.pFunPtr )
            return pDynSym->pSymbol;
      }
   }

   return NULL;
}

static USHORT hb_clsUpdateScope( USHORT uiScope, BOOL fAssign )
{
   if( !fAssign )
      uiScope &= ~HB_OO_CLSTP_READONLY;
   else
   {
      uiScope &= ~HB_OO_CLSTP_PERSIST;

      if( ( uiScope & HB_OO_CLSTP_READONLY ) &&
         !( uiScope & HB_OO_CLSTP_HIDDEN ) )
      {
         /* Class(y) does not allow to write to HIDDEN+READONLY
            instance variables, [druzus] */

         uiScope &= ~HB_OO_CLSTP_READONLY;
         uiScope |= uiScope & HB_OO_CLSTP_PROTECTED ?
                       HB_OO_CLSTP_HIDDEN : HB_OO_CLSTP_PROTECTED;
      }
   }
   return uiScope;
}

static HB_TYPE hb_clsGetItemType( PHB_ITEM pItem )
{
   if( pItem )
   {
      if( HB_IS_STRING( pItem ) )
      {
         switch( hb_itemGetCPtr( pItem )[ 0 ] )
         {
            case 'C':
            case 'c':
            case '\0':
               if( hb_strnicmp( hb_itemGetCPtr( pItem ), "code", 4 ) == 0 )
                  return HB_IT_BLOCK;
               else
                  return HB_IT_STRING;

            case 'S':
            case 's':
               if( hb_strnicmp( hb_itemGetCPtr( pItem ), "str", 3 ) == 0 )
                  return HB_IT_STRING;
               else
                  return HB_IT_SYMBOL;

            case 'B':
            case 'b':
               return HB_IT_BLOCK;

            case 'D':
            case 'd':
               return HB_IT_DATE;

            case 'L':
            case 'l':
               return HB_IT_LOGICAL;

            case 'I':
            case 'i':
               return HB_IT_NUMINT;

            case 'N':
            case 'n':
               return HB_IT_NUMERIC;

            case 'A':
            case 'a':
               return HB_IT_ARRAY;

            case 'P':
            case 'p':
               return HB_IT_POINTER;

            case 'H':
            case 'h':
               return HB_IT_HASH;
         }
      }
      else if( HB_IS_ARRAY( pItem ) )
         return HB_IT_ARRAY;

      else if( HB_IS_NUMINT( pItem ) )
         return HB_IT_NUMINT;

      else if( HB_IS_NUMERIC( pItem ) )
         return HB_IT_NUMERIC;

      else if( HB_IS_DATE( pItem ) )
         return HB_IT_DATE;

      else if( HB_IS_LOGICAL( pItem ) )
         return HB_IT_LOGICAL;

      else if( HB_IS_BLOCK( pItem ) )
         return HB_IT_BLOCK;

      else if( HB_IS_POINTER( pItem ) )
         return HB_IT_POINTER;

      else if( HB_IS_SYMBOL( pItem ) )
         return HB_IT_SYMBOL;
   }

   return 0;
}

/* ================================================ */

/*
 * <uiType>    HB_OO_MSG_METHOD     : standard method
 *             HB_OO_MSG_ONERROR    : error handler method
 *             HB_OO_MSG_DESTRUCTOR : destructor method
 *             HB_OO_MSG_INLINE     : inline (codeblock) method
 *             HB_OO_MSG_ASSIGN     : assign instance data
 *             HB_OO_MSG_ACCESS     : access instance data
 *             HB_OO_MSG_CLSASSIGN  : assign class data
 *             HB_OO_MSG_CLSACCESS  : access class data
 *             HB_OO_MSG_SUPER      : supercasting
 *             HB_OO_MSG_REALCLASS  : caller method real class casting
 *             HB_OO_MSG_PERFORM    : perform method
 *             HB_OO_MSG_VIRTUAL    : virtual method
 *             HB_OO_MSG_DELEGATE   : delegate method
 *
 * <uiScope> * HB_OO_CLSTP_EXPORTED        1 : default for data and method
 *             HB_OO_CLSTP_PROTECTED       2 : method or data protected
 *             HB_OO_CLSTP_HIDDEN          4 : method or data hidden
 *           * HB_OO_CLSTP_CTOR            8 : method constructor
 *             HB_OO_CLSTP_READONLY       16 : data read only
 *             HB_OO_CLSTP_SHARED         32 : (method or) data shared
 *           * HB_OO_CLSTP_CLASS          64 : message is the name of a superclass
 *           * HB_OO_CLSTP_SUPER         128 : message is herited
 *             HB_OO_CLSTP_PERSIST       256 : message is persistent (PROPERTY)
 *             HB_OO_CLSTP_NONVIRTUAL    512 : Class method constructor
 *             HB_OO_CLSTP_OVERLOADED   1024 : Class method constructor
 *
 *             HB_OO_CLSTP_CLASSCTOR    2048 : Class method constructor
 *             HB_OO_CLSTP_CLASSMETH    4096 : Class method
 *
 * <pFunction> HB_OO_MSG_METHOD     : \
 *             HB_OO_MSG_ONERROR    :  > Pointer to function
 *             HB_OO_MSG_DESTRUCTOR : /
 *             HB_OO_MSG_INLINE     : Code block
 *             HB_OO_MSG_ASSIGN     :  Index to instance area array
 *             HB_OO_MSG_ACCESS     : /
 *             HB_OO_MSG_CLSASSIGN  :  Index class data array
 *             HB_OO_MSG_CLSACCESS  : /
 *             HB_OO_MSG_SUPER      : Handle of super class
 *             HB_OO_MSG_DELEGATE   : delegated message symbol
 *
 * <pInit>     HB_OO_MSG_ACCESS     :  Optional initializer for (Class)DATA
 *             HB_OO_MSG_CLSACCESS  : /
 *             HB_OO_MSG_ASSIGN     :  item type restriction in assignment
 *             HB_OO_MSG_CLSASSIGN  : /
 *             HB_OO_MSG_SUPER      : Superclass handle
 */
static BOOL hb_clsAddMsg( USHORT uiClass, const char * szMessage,
                          USHORT uiType, USHORT uiScope,
                          PHB_ITEM pFunction, PHB_ITEM pInit )
{
   if( szMessage && uiClass && uiClass <= s_uiClasses )
   {
      PCLASS   pClass   = &s_pClasses[ uiClass ];

      PHB_DYNS pMessage;
      PMETHOD  pNewMeth;
      USHORT   uiOperator, uiSprClass = 0, uiIndex = 0, uiPrevCls, uiPrevMth;
      PHB_SYMB pOpSym, pFuncSym = NULL;
      BOOL     fOK;
      ULONG    ulOpFlags = 0;

      if( pClass->fLocked )
         return FALSE;

      if( !( uiScope & ( HB_OO_CLSTP_EXPORTED | HB_OO_CLSTP_PROTECTED | HB_OO_CLSTP_HIDDEN ) ) )
         uiScope |= HB_OO_CLSTP_EXPORTED;

      /* translate names of operator overloading messages */
      if( uiType == HB_OO_MSG_DESTRUCTOR )
         pMessage = s___msgDestructor.pDynSym;
      else if( uiType == HB_OO_MSG_ONERROR )
         pMessage = s___msgOnError.pDynSym;
      else if (strcmp("+", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_PLUS )->pDynSym;
      else if (strcmp("-", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_MINUS )->pDynSym;
      else if (strcmp("*", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_MULT )->pDynSym;
      else if (strcmp("/", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_DIVIDE )->pDynSym;
      else if (strcmp("%", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_MOD )->pDynSym;
      else if (strcmp("^", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_POWER )->pDynSym;
      else if (strcmp("**", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_POWER )->pDynSym;
      else if (strcmp("++", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_INC )->pDynSym;
      else if (strcmp("--", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_DEC )->pDynSym;
      else if (strcmp("==", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_EXACTEQUAL )->pDynSym;
      else if (strcmp("=", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_EQUAL )->pDynSym;
      else if (strcmp("!=", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_NOTEQUAL )->pDynSym;
      else if (strcmp("<>", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_NOTEQUAL )->pDynSym;
      else if (strcmp("#", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_NOTEQUAL )->pDynSym;
      else if (strcmp("<", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_LESS )->pDynSym;
      else if (strcmp("<=", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_LESSEQUAL )->pDynSym;
      else if (strcmp(">", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_GREATER )->pDynSym;
      else if (strcmp(">=", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_GREATEREQUAL )->pDynSym;
      else if (strcmp(":=", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_ASSIGN )->pDynSym;
      else if (strcmp("$", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_INSTRING )->pDynSym;
      else if (strcmp("!", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_NOT )->pDynSym;
      else if (hb_stricmp(".NOT.", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_NOT )->pDynSym;
      else if (hb_stricmp(".AND.", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_AND )->pDynSym;
      else if (hb_stricmp(".OR.", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_OR )->pDynSym;
      else if( strcmp("[]", szMessage) == 0)
         pMessage = ( s_opSymbols + HB_OO_OP_ARRAYINDEX )->pDynSym;
      else
         pMessage = hb_dynsymGet( szMessage );

      for( uiOperator = 0, pOpSym = s_opSymbols;
           uiOperator <= HB_OO_MAX_OPERATOR; ++uiOperator, ++pOpSym )
      {
         if( pOpSym->pDynSym == pMessage )
         {
            ulOpFlags |= 1UL << uiOperator;
            break;
         }
      }

      /* basic parameter validation */
      switch( uiType )
      {
         case HB_OO_MSG_METHOD:
         case HB_OO_MSG_ONERROR:
         case HB_OO_MSG_DESTRUCTOR:
            pFuncSym = hb_objGetFuncSym( pFunction );
            fOK = pFuncSym != NULL;
            break;

         case HB_OO_MSG_INLINE:
            fOK = pFunction && HB_IS_BLOCK( pFunction );
            break;

         case HB_OO_MSG_SUPER:
            uiIndex = ( USHORT ) hb_itemGetNI( pFunction );
            uiSprClass = ( USHORT ) hb_itemGetNI( pInit );
            fOK = uiSprClass && uiSprClass <= s_uiClasses &&
                  uiIndex <= pClass->uiDatas;
            break;

         case HB_OO_MSG_ASSIGN:
         case HB_OO_MSG_ACCESS:
            uiIndex = ( USHORT ) hb_itemGetNI( pFunction );
            /* This validation can break buggy .prg code which wrongly
             * sets data offsets but IMHO it will help to clean the code.
             * [druzus]
             */
            fOK = uiIndex && uiIndex <= pClass->uiDatas - pClass->uiDataFirst;
            break;

         case HB_OO_MSG_CLSASSIGN:
         case HB_OO_MSG_CLSACCESS:
            uiIndex = ( USHORT ) hb_itemGetNI( pFunction );
            fOK = uiIndex != 0;
            break;

         case HB_OO_MSG_DELEGATE:
         {
            PHB_DYNS pDelegMsg = hb_objGetMsgSym( pFunction );
            if( pDelegMsg )
            {
               pNewMeth = hb_clsFindMsg( pClass, pDelegMsg );
               if( pNewMeth )
                  uiIndex = ( USHORT ) ( pNewMeth - pClass->pMethods );
            }
            fOK = uiIndex != 0;
            break;
         }
         case HB_OO_MSG_REALCLASS:
         case HB_OO_MSG_VIRTUAL:
         case HB_OO_MSG_PERFORM:
            fOK = TRUE;
            break;

         default:
            fOK = FALSE;
      }

      if( !fOK )
      {
         hb_errRT_BASE( EG_ARG, 3000, NULL, &hb_errFuncName, HB_ERR_ARGS_BASEPARAMS );
         return FALSE;
      }

      pNewMeth = hb_clsAllocMsg( pClass, pMessage );
      if( ! pNewMeth )
         return FALSE;

      uiPrevCls = uiClass;
      uiPrevMth = ( USHORT ) ( pClass->pMethods - pNewMeth );

#ifndef HB_VIRTUAL_HIDDEN
      if( uiScope & HB_OO_CLSTP_HIDDEN )
         uiScope |= HB_OO_CLSTP_NONVIRTUAL;
#endif

      if( ! pNewMeth->pMessage )
         pClass->uiMethods++;           /* One more message */
      else
      {
         BOOL fOverLoad = ( pNewMeth->uiScope & HB_OO_CLSTP_OVERLOADED ) ||
                          ( ( pNewMeth->uiScope & HB_OO_CLSTP_NONVIRTUAL ) &&
                            pNewMeth->uiSprClass != uiClass );

         uiPrevCls = pNewMeth->uiPrevCls;
         uiPrevMth = pNewMeth->uiPrevMth;
         if( ! hb_clsCanClearMethod( pNewMeth, TRUE ) )
            return FALSE;

         memset( pNewMeth, 0, sizeof( METHOD ) );
         if( fOverLoad )
            uiScope |= HB_OO_CLSTP_OVERLOADED;
      }
      pNewMeth->pMessage = pMessage;
      pNewMeth->uiSprClass = uiClass;
      pNewMeth->uiPrevCls = uiPrevCls;
      pNewMeth->uiPrevMth = uiPrevMth;

      switch( uiType )
      {
         case HB_OO_MSG_METHOD:

            pNewMeth->pFuncSym = pFuncSym;
            pNewMeth->uiScope = uiScope;
            break;

         case HB_OO_MSG_ASSIGN:

            pNewMeth->uiScope = hb_clsUpdateScope( uiScope, TRUE );
            /* Class(y) does not allow to write to HIDDEN+READONLY
               instance variables, [druzus] */
            if( pNewMeth->uiScope & HB_OO_CLSTP_READONLY &&
                pNewMeth->uiScope & HB_OO_CLSTP_HIDDEN )
               pNewMeth->pFuncSym = &s___msgScopeErr;
            else
            {
               pNewMeth->pFuncSym = &s___msgSetData;
               pNewMeth->uiData = uiIndex;
               pNewMeth->uiOffset = pClass->uiDataFirst;
               pNewMeth->itemType = hb_clsGetItemType( pInit );
            }
            break;

         case HB_OO_MSG_ACCESS:

            pNewMeth->uiScope = hb_clsUpdateScope( uiScope, FALSE );
            pNewMeth->uiData = uiIndex;
            pNewMeth->uiOffset = pClass->uiDataFirst;
            hb_clsAddInitValue( pClass, pInit, HB_OO_MSG_DATA,
                                pNewMeth->uiData, pNewMeth->uiOffset, uiClass );
            pNewMeth->pFuncSym = &s___msgGetData;
            break;

         case HB_OO_MSG_CLSASSIGN:

            pNewMeth->uiData = uiIndex;
            pNewMeth->itemType = hb_clsGetItemType( pInit );
            pNewMeth->uiScope = hb_clsUpdateScope( uiScope, TRUE );
            /* Class(y) does not allow to write to HIDDEN+READONLY
               instance variables, [druzus] */
            if( pNewMeth->uiScope & HB_OO_CLSTP_READONLY &&
                pNewMeth->uiScope & HB_OO_CLSTP_HIDDEN )
               pNewMeth->pFuncSym = &s___msgScopeErr;
            else if( pNewMeth->uiScope & HB_OO_CLSTP_SHARED )
            {
               if( hb_arrayLen( pClass->pSharedDatas ) < ( ULONG ) pNewMeth->uiData )
                  hb_arraySize( pClass->pSharedDatas, pNewMeth->uiData );
               pNewMeth->pFuncSym = &s___msgSetShrData;
            }
            else
            {
               if( hb_arrayLen( pClass->pClassDatas ) < ( ULONG ) pNewMeth->uiData )
                  hb_arraySize( pClass->pClassDatas, pNewMeth->uiData );
               pNewMeth->pFuncSym = &s___msgSetClsData;
            }
            break;

         case HB_OO_MSG_CLSACCESS:

            pNewMeth->uiScope = hb_clsUpdateScope( uiScope, FALSE );
            pNewMeth->uiData = uiIndex;
            if( pNewMeth->uiScope & HB_OO_CLSTP_SHARED )
            {
               if( hb_arrayLen( pClass->pSharedDatas ) < ( ULONG ) pNewMeth->uiData )
                  hb_arraySize( pClass->pSharedDatas, pNewMeth->uiData );

               if( pInit && ! HB_IS_NIL( pInit ) ) /* Initializer found */
               {
                  /* Shared Classdata need to be initialized only once
                   * ACCESS/ASSIGN methods will be inherited by subclasses
                   * and will operate on this value so it's not necessary
                   * to keep the init value. [druzus]
                   */
                  pInit = hb_itemClone( pInit );
                  hb_arraySet( pClass->pSharedDatas, pNewMeth->uiData, pInit );
                  hb_itemRelease( pInit );
               }
               pNewMeth->pFuncSym = &s___msgGetShrData;
            }
            else
            {
               if( hb_arrayLen( pClass->pClassDatas ) < ( ULONG ) pNewMeth->uiData )
                  hb_arraySize( pClass->pClassDatas, pNewMeth->uiData );
               pNewMeth->uiOffset = hb_clsAddInitValue( pClass, pInit,
                           HB_OO_MSG_CLASSDATA, pNewMeth->uiData, 0, uiClass );
               pNewMeth->pFuncSym = &s___msgGetClsData;
            }
            break;

         case HB_OO_MSG_INLINE:

            pNewMeth->pFuncSym = &s___msgEvalInline;
            pNewMeth->uiScope = uiScope;
            hb_arrayAdd( pClass->pInlines, pFunction );
            pNewMeth->uiData = ( USHORT ) hb_arrayLen( pClass->pInlines );
            break;

         case HB_OO_MSG_VIRTUAL:

            pNewMeth->pFuncSym = &s___msgVirtual;
            pNewMeth->uiScope = uiScope;
            break;

         case HB_OO_MSG_SUPER:

            pNewMeth->uiSprClass = uiSprClass; /* store the super handel */
            pNewMeth->uiOffset = uiIndex; /* offset to instance area */
            pNewMeth->uiScope = uiScope;
            pNewMeth->pFuncSym = &s___msgSuper;
            break;

         case HB_OO_MSG_REALCLASS:
            pNewMeth->pFuncSym = &s___msgRealClass;
            pNewMeth->uiScope = uiScope;
            break;

         case HB_OO_MSG_PERFORM:
            pNewMeth->pFuncSym = &s___msgPerform;
            pNewMeth->uiScope = uiScope;
            break;

         case HB_OO_MSG_DELEGATE:
            pNewMeth->pFuncSym = &s___msgDelegate;
            pNewMeth->uiScope = uiScope;
            pNewMeth->uiData = uiIndex;
            break;

         case HB_OO_MSG_ONERROR:

            pNewMeth->pFuncSym = pFuncSym;
            pClass->fHasOnError = TRUE;
            break;

         case HB_OO_MSG_DESTRUCTOR:

            pNewMeth->pFuncSym = pFuncSym;
            pClass->fHasDestructor = TRUE;
            break;

         default:

            hb_errInternal( HB_EI_CLSINVMETHOD, NULL, "__clsAddMsg", NULL );
            return FALSE;
      }

      pClass->ulOpFlags |= ulOpFlags;
   }

   return TRUE;   
}

/*
 * __clsAddMsg( <hClass>, <cMessage>, <pFunction>, <nType>, [xInit], <uiScope>, <xItemType> )
 *
 * Add a message to the class.
 *
 * <hClass>    Class handle
 * <cMessage>  Message
 * <pFunction> HB_OO_MSG_METHOD     : \
 *             HB_OO_MSG_ONERROR    :  > Pointer to function
 *             HB_OO_MSG_DESTRUCTOR : /
 *             HB_OO_MSG_INLINE     : Code block
 *             HB_OO_MSG_DATA       : \
 *             HB_OO_MSG_ASSIGN     :  > Index to instance area array
 *             HB_OO_MSG_ACCESS     : /
 *             HB_OO_MSG_CLASSDATA  : \
 *             HB_OO_MSG_CLSASSIGN  :  > Index class data array
 *             HB_OO_MSG_CLSACCESS  : /
 *             HB_OO_MSG_SUPER      : Handle of super class
 *             HB_OO_MSG_DELEGATE   : delegated message symbol
 *
 * <nType>     see HB_OO_MSG_* above and:
 *             HB_OO_MSG_REALCLASS  : caller method real class casting
 *             HB_OO_MSG_PERFORM    : perform message
 *             HB_OO_MSG_VIRTUAL    : virtual message
 *             HB_OO_MSG_DELEGATE   : delegate method
 *
 * <xInit>     HB_OO_MSG_ACCESS     : \
 *             HB_OO_MSG_CLSACCESS  :   > Optional initializer for DATA
 *             HB_OO_MSG_DATA       :  /
 *             HB_OO_MSG_CLASSDATA  : /
 *             HB_OO_MSG_SUPER      : Superclass handle
 *             HB_OO_MSG_ASSIGN     : \ item type restriction in assignment not
 *             HB_OO_MSG_CLSASSIGN: :   empty character value where first letter
 *                                      is item type or item of a given value
 *
 * <uiScope> * HB_OO_CLSTP_EXPORTED        1 : default for data and method
 *             HB_OO_CLSTP_PROTECTED       2 : method or data protected
 *             HB_OO_CLSTP_HIDDEN          4 : method or data hidden
 *           * HB_OO_CLSTP_CTOR            8 : method constructor
 *             HB_OO_CLSTP_READONLY       16 : data read only
 *             HB_OO_CLSTP_SHARED         32 : (method or) data shared
 *           * HB_OO_CLSTP_CLASS          64 : message is the name of a superclass
 *           * HB_OO_CLSTP_SUPER         128 : message is herited
 *             HB_OO_CLSTP_PERSIST       256 : message is persistent (PROPERTY)
 *             HB_OO_CLSTP_NONVIRTUAL    512 : Class method constructor
 *             HB_OO_CLSTP_OVERLOADED   1024 : Class method constructor
 *
 *             HB_OO_CLSTP_CLASSCTOR    2048 : Class method constructor
 *             HB_OO_CLSTP_CLASSMETH    4096 : Class method
 */

HB_FUNC( __CLSADDMSG )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );
   char * szMessage = hb_parc( 2 );

   if( szMessage && uiClass && uiClass <= s_uiClasses )
   {
      USHORT   nType     = ( USHORT ) hb_parni( 4 );
      USHORT   uiScope   = ( USHORT ) hb_parni( 6 );
      PHB_ITEM pFunction = hb_param( 3, HB_IT_ANY );
      PHB_ITEM pInit     = hb_param( 5, HB_IT_ANY );

      if( nType == HB_OO_MSG_DATA )
      {
         nType = szMessage[ 0 ] == '_' ? HB_OO_MSG_ASSIGN : HB_OO_MSG_ACCESS;
      }
      else if( nType == HB_OO_MSG_CLASSDATA )
      {
            nType = szMessage[ 0 ] == '_' ? HB_OO_MSG_CLSASSIGN :
                                            HB_OO_MSG_CLSACCESS;
      }
      /* to make xHarbour users happy ;-) */
      else if( nType == HB_OO_MSG_PROPERTY ||
               nType == HB_OO_MSG_CLASSPROPERTY )
      {
         char szAssign[ HB_SYMBOL_NAME_LEN + 1 ];
         int iLen = ( int ) hb_parclen( 2 );
         if( iLen >= HB_SYMBOL_NAME_LEN )
            iLen = HB_SYMBOL_NAME_LEN - 1;
         szAssign[ 0 ] = '_';
         memcpy( szAssign + 1, szMessage, iLen );
         szAssign[ iLen + 1 ] = '\0';

         uiScope = ( uiScope | HB_OO_CLSTP_EXPORTED ) &
                  ~( HB_OO_CLSTP_PROTECTED | HB_OO_CLSTP_HIDDEN );
         if( nType == HB_OO_MSG_PROPERTY )
         {
            hb_clsAddMsg( uiClass, szAssign, HB_OO_MSG_ASSIGN,
                          uiScope & ~HB_OO_CLSTP_PERSIST, pFunction, pInit );
            nType = HB_OO_MSG_ACCESS;
         }
         else
         {
            hb_clsAddMsg( uiClass, szAssign, HB_OO_MSG_CLSASSIGN,
                          uiScope & ~HB_OO_CLSTP_PERSIST, pFunction, pInit );
            nType = HB_OO_MSG_CLSACCESS;
         }
      }

      hb_clsAddMsg( uiClass, szMessage, nType, uiScope, pFunction, pInit );
   }
}

/*
 * __clsNew( <szClassName>, <uiDatas>,
 *           [<pSuperArray>], [<pClassFunc>],
 *           [<fModuleFriendly>] ) -> <hClass>
 *
 * Create a new class
 *
 * <szClassName>     Name of the class
 * <uiDatas>         Number of DATAs in the class
 * <pSuperArray>     Optional array with handle(s) of superclass(es)
 * <pClassFunc>      Class function symbol, when NULLpublic function
 *                   with the same name as szClassName is used
 * <fModuleFriendly> when true all functions and classes from the same
 *                   module as pClassFunc are defined as friends
 */
static USHORT hb_clsNew( const char * szClassName, USHORT uiDatas,
                         PHB_ITEM pSuperArray, PHB_SYMB pClassFunc,
                         BOOL fModuleFriendly )
{
   PCLASS pNewCls;
   PMETHOD pMethod;
   USHORT ui, uiSuper, uiSuperCls;
   USHORT * puiClassData = NULL, uiClassDataSize = 0;

   uiSuper  = ( USHORT ) ( pSuperArray ? hb_arrayLen( pSuperArray ) : 0 );
   pClassFunc = hb_vmGetRealFuncSym( pClassFunc );

   if( s_pClasses )
      s_pClasses = ( PCLASS ) hb_xrealloc( s_pClasses, sizeof( CLASS ) * ( s_uiClasses + 2 ) );
   else
   {
      s_pClasses = ( PCLASS ) hb_xgrab( sizeof( CLASS ) * 2 );
      memset( s_pClasses, 0, sizeof( CLASS ) );
   }

   pNewCls = s_pClasses + ++s_uiClasses;
   memset( pNewCls, 0, sizeof( CLASS ) );
   pNewCls->szName = hb_strdup( szClassName );
   pNewCls->pClassSym = hb_dynsymGet( pNewCls->szName );
   if( !pClassFunc )
      pClassFunc = hb_vmGetRealFuncSym( pNewCls->pClassSym->pSymbol );
   pNewCls->pClassFuncSym = pClassFunc;
   if( fModuleFriendly )
      hb_vmFindModuleSymbols( pClassFunc, &pNewCls->pFriendModule,
                                          &pNewCls->uiFriendModule );

   for( ui = 1; ui <= uiSuper; ++ui )
   {
      uiSuperCls = ( USHORT ) hb_arrayGetNI( pSuperArray, ui );
      if( uiSuperCls && uiSuperCls < s_uiClasses )
      {
         PCLASS pSprCls;

         pSprCls = &s_pClasses[ uiSuperCls ];
         if( ! hb_clsInited( pNewCls ) ) /* This is the first superclass */
         {
            hb_clsCopyClass( pNewCls, pSprCls );
         }
         else if( !hb_clsHasParent( pNewCls, pSprCls->pClassSym ) )
         {
            ULONG  ul, ulLimit;
            USHORT nLenClsDatas;

            /* create class data translation tables */
            nLenClsDatas  = ( USHORT ) hb_itemSize( pSprCls->pClassDatas );
            if( nLenClsDatas )
            {
               if( nLenClsDatas > uiClassDataSize )
               {
                  if( puiClassData )
                     puiClassData = ( USHORT * ) hb_xrealloc( puiClassData,
                                             sizeof( USHORT ) * nLenClsDatas );
                  else
                     puiClassData = ( USHORT * ) hb_xgrab( sizeof( USHORT ) *
                                                           nLenClsDatas );
                  uiClassDataSize = nLenClsDatas;
               }
               memset( puiClassData, 0, sizeof( USHORT ) * nLenClsDatas );
            }

            /* Copy super classs handles */
            ulLimit = hb_clsMthNum( pSprCls );
            for( ul = 0; ul < ulLimit; ++ul )
            {
               if( pSprCls->pMethods[ ul ].pMessage &&
                   pSprCls->pMethods[ ul ].pFuncSym == &s___msgSuper )
               {
                  PCLASS pCls = &s_pClasses[ pSprCls->pMethods[ ul ].uiSprClass ];

                  pMethod = hb_clsAllocMsg( pNewCls,
                                            pSprCls->pMethods[ ul ].pMessage );
                  if( ! pMethod )
                     return 0;
                  if( pMethod->pMessage == NULL )
                  {
                     pNewCls->uiMethods++;
                     memcpy( pMethod, pSprCls->pMethods + ul, sizeof( METHOD ) );
                     pMethod->uiOffset = pNewCls->uiDatas;
                     pNewCls->uiDatas += pCls->uiDatas - pCls->uiDataFirst;
                  }
               }
            }

            /* add class casting if not exist */
            pMethod = hb_clsAllocMsg( pNewCls, pSprCls->pClassSym );
            if( ! pMethod )
               return 0;
            if( pMethod->pMessage == NULL )
            {
               pNewCls->uiMethods++;
               pMethod->pMessage = pSprCls->pClassSym;
               pMethod->uiSprClass = uiSuperCls;
               pMethod->uiScope = HB_OO_CLSTP_EXPORTED;
               pMethod->pFuncSym = &s___msgSuper;
               pMethod->uiOffset = pNewCls->uiDatas;
               pNewCls->uiDatas += pSprCls->uiDatas - pSprCls->uiDataFirst;
            }

            /* Copy instance area init data */
            if( pSprCls->uiInitDatas )
            {
               USHORT ui;
               for( ui = 0; ui < pSprCls->uiInitDatas; ++ui )
               {
                  if( pSprCls->pInitData[ ui ].uiType == HB_OO_MSG_DATA )
                  {
                     USHORT uiCls = pSprCls->pInitData[ ui ].uiSprClass;
                     hb_clsAddInitValue( pNewCls,
                        pSprCls->pInitData[ ui ].pInitValue, HB_OO_MSG_DATA,
                        pSprCls->pInitData[ ui ].uiData,
                        hb_clsParentInstanceOffset( pNewCls,
                                          s_pClasses[ uiCls ].pClassSym ),
                        uiCls );
                  }
               }
            }

            /* Now working on other methods */
            ulLimit = hb_clsMthNum( pSprCls );
            for( ul = 0; ul < ulLimit; ++ul )
            {
               if( pSprCls->pMethods[ ul ].pMessage )
               {
                  pMethod = hb_clsAllocMsg( pNewCls, pSprCls->pMethods[ ul ].pMessage );
                  if( ! pMethod )
                     return 0;

                  /* Ok, this bucket is empty */
                  if( pMethod->pMessage == NULL )
                  {
                     /* Now, we can increment the msg count */
                     pNewCls->uiMethods++;
                     memcpy( pMethod, pSprCls->pMethods + ul, sizeof( METHOD ) );
                     if( ! hb_clsUpdateHiddenMessages( pMethod, pMethod, pNewCls ) )
                     {
                        if( pMethod->pFuncSym == &s___msgSetClsData ||
                            pMethod->pFuncSym == &s___msgGetClsData )
                        {
                           if( pMethod->uiData > nLenClsDatas )
                              hb_errInternal( HB_EI_CLSINVMETHOD, NULL, "__clsNew", NULL );

                           if( puiClassData[ pMethod->uiData - 1 ] == 0 )
                           {
                              puiClassData[ pMethod->uiData - 1 ] = ( USHORT )
                                          hb_arrayLen( pNewCls->pClassDatas ) + 1;
                              hb_arraySize( pNewCls->pClassDatas,
                                            puiClassData[ pMethod->uiData - 1 ] );
                           }
                           if( pMethod->uiOffset )
                           {
                              pMethod->uiOffset = hb_clsAddInitValue( pNewCls,
                                 pSprCls->pInitData[ pMethod->uiOffset - 1 ].pInitValue,
                                 HB_OO_MSG_CLASSDATA, puiClassData[ pMethod->uiData - 1 ],
                                 0, uiSuperCls );
                           }
                           pMethod->uiData = puiClassData[ pMethod->uiData - 1 ];
                        }
                        else if( pMethod->pFuncSym == &s___msgSetData ||
                                 pMethod->pFuncSym == &s___msgGetData )
                        {
                           pMethod->uiOffset = hb_clsParentInstanceOffset( pNewCls,
                                 s_pClasses[ pMethod->uiSprClass ].pClassSym );
                        }
                        pMethod->uiScope |= HB_OO_CLSTP_SUPER;
                     }
                  }
                  else
                  {
                     if( pSprCls->pMethods[ ul ].uiScope &
                         ( HB_OO_CLSTP_OVERLOADED | HB_OO_CLSTP_NONVIRTUAL ) )
                        pMethod->uiScope |= HB_OO_CLSTP_OVERLOADED;

                     hb_clsUpdateHiddenMessages( pSprCls->pMethods + ul, pMethod, pNewCls );
                  }
               }
            }
            pNewCls->ulOpFlags |= pSprCls->ulOpFlags;
         }
      }
   }
   if( puiClassData )
      hb_xfree( puiClassData );

   if( ! hb_clsInited( pNewCls ) )
   {
      hb_clsDictInit( pNewCls, HASH_KEY );
      pNewCls->pClassDatas  = hb_itemArrayNew( 0 );
      pNewCls->pSharedDatas = hb_itemArrayNew( 0 );
      pNewCls->pInlines     = hb_itemArrayNew( 0 );
   }

   /* add self class casting */
   if( hb_stricmp( pNewCls->szName, pNewCls->pClassSym->pSymbol->szName ) == 0 )
   {
      pMethod = hb_clsAllocMsg( pNewCls, pNewCls->pClassSym );
      if( ! pMethod )
         return 0;
      if( pMethod->pMessage == NULL )
      {
         pNewCls->uiMethods++;
         pMethod->pMessage = pNewCls->pClassSym;
         pMethod->uiSprClass = s_uiClasses;
         pMethod->uiScope = HB_OO_CLSTP_EXPORTED;
         pMethod->pFuncSym = &s___msgSuper;
         pMethod->uiOffset = pNewCls->uiDatas;
      }
   }
   pNewCls->uiDataFirst = pNewCls->uiDatas;
   pNewCls->uiDatas += uiDatas;

   return s_uiClasses;
}

/*
 * hb_clsNew( <szClassName>, < ) -> <hClass>
 *
 * <hClass> := __clsNew( <cClassName>, <nDatas>, [<ahSuper>], [<pClassFunc>], [<lModuleFriendly>] )
 *
 * Create a new class
 *
 * <cClassName> Name of the class
 * <nDatas>     Number of DATAs in the class
 * <ahSuper>    Optional array with handle(s) of superclass(es)
 * <pClassFunc> Class function symbol
 * <lModuleFriendly> when true all functions and classes from the same
 *                   module as pClassFunc are defined as friends
 */
HB_FUNC( __CLSNEW )
{
   char * szClassName;
   PHB_ITEM pDatas, pSuperArray, pClassFunc, pModFriend;

   szClassName = hb_parc( 1 );

   pDatas = hb_param( 2, HB_IT_ANY );

   pSuperArray = hb_param( 3, HB_IT_ANY );
   if( pSuperArray && HB_IS_NIL( pSuperArray ) )
      pSuperArray = NULL;

   pClassFunc = hb_param( 4, HB_IT_ANY );
   if( pClassFunc && HB_IS_NIL( pClassFunc ) )
      pClassFunc = NULL;

   pModFriend = hb_param( 5, HB_IT_ANY );
   if( pModFriend && HB_IS_NIL( pModFriend ) )
      pModFriend = NULL;
   
   if( szClassName && 
       ( ! pDatas || HB_IS_NUMERIC( pDatas ) ) &&
       ( ! pSuperArray || HB_IS_ARRAY( pSuperArray ) ) &&
       ( ! pClassFunc || HB_IS_SYMBOL( pClassFunc ) ) &&
       ( ! pModFriend || HB_IS_LOGICAL( pModFriend ) ) )
   {
      USHORT uiClass;
      uiClass = hb_clsNew( szClassName, ( USHORT ) hb_itemGetNI( pDatas ),
                           pSuperArray, hb_itemGetSymbol( pClassFunc ),
                           hb_itemGetL( pModFriend ) );
      hb_retni( uiClass );
   }
   else
      hb_errRT_BASE( EG_ARG, 3000, NULL, "__CLSNEW", HB_ERR_ARGS_BASEPARAMS );
}

/*
 *  __clsAddFriend( <hClass>, <pFyncSym> )
 *
 *  Add friend function
 */
HB_FUNC( __CLSADDFRIEND )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );

   if( uiClass && uiClass <= s_uiClasses )
   {
      PCLASS pClass = &s_pClasses[ uiClass ];

      if( !pClass->fLocked )
      {
         PHB_SYMB pSym = hb_vmGetRealFuncSym( hb_itemGetSymbol( hb_param( 2,
                                                         HB_IT_SYMBOL ) ) );
         if( pSym )
            hb_clsAddFriendSymbol( pClass, pSym );
      }
   }
}

/*
 * __clsDelMsg( <hClass>, <cMessage> )
 *
 * Delete message (only for INLINE and METHOD)
 *
 * <hClass>   class handle
 * <cMessage> message
 */
HB_FUNC( __CLSDELMSG )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );
   PHB_ITEM pString = hb_param( 2, HB_IT_STRING );

   if( uiClass && uiClass <= s_uiClasses && pString &&
       ! s_pClasses[ uiClass ].fLocked )
   {
      PHB_DYNS pMsg = hb_dynsymFindName( pString->item.asString.value );

      if( pMsg )
         hb_clsFreeMsg( &s_pClasses[ uiClass ], pMsg );
   }
}


/*
 * hb_clsInst( <hClass> ) -> <pObjectItm>
 *
 * Create a new object from class definition <hClass>
 */
static PHB_ITEM hb_clsInst( USHORT uiClass )
{
   PHB_ITEM pSelf = NULL;

   if( uiClass && uiClass <= s_uiClasses )
   {
      PCLASS   pClass = &s_pClasses[ uiClass ];

      pSelf = hb_itemNew( NULL );
      hb_arrayNew( pSelf, pClass->uiDatas );
      pSelf->item.asArray.value->uiClass = uiClass;

      /* Initialise value if initialisation was requested */
      if( pClass->uiInitDatas )
      {
         PINITDATA pInitData = pClass->pInitData;
         USHORT ui = pClass->uiInitDatas;
         PHB_ITEM pDestItm;

         do
         {
            if( pInitData->uiType == HB_OO_MSG_DATA )
               pDestItm = hb_arrayGetItemPtr( pSelf,
                                    pInitData->uiData + pInitData->uiOffset );
            else if( pInitData->uiType == HB_OO_MSG_CLASSDATA )
            {
               pDestItm = hb_arrayGetItemPtr( pClass->pClassDatas,
                                              pInitData->uiData );
               /* do not initialize it again */
               pInitData->uiType = HB_OO_MSG_INITIALIZED;
            }
            else
               pDestItm = NULL;

            if( pDestItm )
            {
               PHB_ITEM pInit = hb_itemClone( pInitData->pInitValue );
               hb_itemMove( pDestItm, pInit );
               hb_itemRelease( pInit );
            }
            ++pInitData;
         }
         while( --ui );
      }
   }

   return pSelf;
}

/*
 * <oNewObject> := __clsInst( <hClass> )
 *
 * Create a new object from class definition <hClass>
 */
HB_FUNC( __CLSINST )
{
   PHB_ITEM pSelf = hb_clsInst( ( USHORT ) hb_parni( 1 ) );

   if( pSelf )
      hb_itemRelease( hb_itemReturnForward( pSelf ) );
}

/*
 * __clsLock( <hClass> )
 * Block farther class modifications
 */
HB_FUNC( __CLSLOCK )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );

   if( uiClass && uiClass <= s_uiClasses )
      s_pClasses[ uiClass ].fLocked = TRUE;
}

/*
 * __clsModMsg( <hClass>, <cMessage>, <pFunc> )
 *
 * Modify message (only for INLINE and METHOD)
 */
HB_FUNC( __CLSMODMSG )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );
   PHB_ITEM pString = hb_param( 2, HB_IT_STRING );

   if( uiClass && uiClass <= s_uiClasses && pString &&
       ! s_pClasses[ uiClass ].fLocked )
   {
      PHB_DYNS pMsg = hb_dynsymFindName( pString->item.asString.value );

      if( pMsg )
      {
         PCLASS  pClass  = &s_pClasses[ uiClass ];
         PMETHOD pMethod = hb_clsFindMsg( pClass, pMsg );

         if( pMethod )
         { 
            PHB_SYMB pFuncSym = pMethod->pFuncSym;

            if( pFuncSym == &s___msgSetData || pFuncSym == &s___msgGetData )
            {
               hb_errRT_BASE( EG_ARG, 3004, "Cannot modify a DATA item", "__CLSMODMSG", 0 );
            }
            else if( pFuncSym == &s___msgEvalInline )
            {
               PHB_ITEM pBlock = hb_param( 3, HB_IT_BLOCK );

               if( pBlock == NULL )
                  hb_errRT_BASE( EG_ARG, 3000, "Cannot modify INLINE method", "__CLSMODMSG", 0 );
               else
                  hb_arraySet( s_pClasses[ pMethod->uiSprClass ].pInlines,
                               pMethod->uiData, pBlock );
            }
            else                                      /* Modify METHOD */
            {
               PHB_SYMB pFuncSym = hb_objGetFuncSym( hb_param( 3, HB_IT_ANY ) );

               if( pFuncSym == NULL )
                  hb_errRT_BASE( EG_ARG, 3000, NULL, "__CLSMODMSG", 0 );
               else
                  pMethod->pFuncSym = pFuncSym;
            }
         }
      }
   }
}


/*
 * <cClassName> := __objGetClsName( <hClass> | <oObj> )
 *
 * Returns class name of <oObj> or <hClass>
 */
HB_FUNC( __OBJGETCLSNAME )
{
   PHB_ITEM pObject = hb_param( 1, HB_IT_OBJECT );
   USHORT uiClass;

   if( pObject )
      uiClass = pObject->item.asArray.value->uiClass;
   else
      uiClass = ( USHORT ) hb_parni( 1 );

   hb_retc( hb_clsName( uiClass ) );
}


/*
 * <lRet> := __objHasMsg( <oObj>, <cSymbol> )
 *
 * Is <cSymbol> a valid message for the <oObj>
 */
HB_FUNC( __OBJHASMSG )
{
   PHB_DYNS pMessage = hb_objGetMsgSym( hb_param( 2, HB_IT_ANY ) );

   if( pMessage )
      hb_retl( hb_objHasMessage( hb_param( 1, HB_IT_ANY ), pMessage ) );
   else
      hb_errRT_BASE_SubstR( EG_ARG, 1099, NULL, "__OBJHASMSG", HB_ERR_ARGS_BASEPARAMS );
}

/*
 * <xRet> = __objSendMsg( <oObj>, <cSymbol>, <xArg,..>
 *
 * Send a message to an object
 */
HB_FUNC( __OBJSENDMSG )
{
   PHB_DYNS pMessage = hb_objGetMsgSym( hb_param( 2, HB_IT_ANY ) );

   if( pMessage )
   {
      USHORT uiPCount = hb_pcount();
      USHORT uiParam;

      hb_vmPushSymbol( pMessage->pSymbol );     /* Push message symbol */
      hb_vmPush( hb_param( 1, HB_IT_ANY ) );    /* Push object */

      for( uiParam = 3; uiParam <= uiPCount; ++uiParam )    /* Push arguments on stack */
      {
         hb_vmPush( hb_stackItemFromBase( uiParam ) );
      }
      hb_vmSend( ( USHORT ) ( uiPCount - 2 ) );             /* Execute message */
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 3000, NULL, "__OBJSENDMSG", HB_ERR_ARGS_BASEPARAMS );
   }
}

/*
 * <oNew> := __objClone( <oOld> )
 *
 * Clone an object. Note the similarity with aClone ;-)
 */
HB_FUNC( __OBJCLONE )
{
   PHB_ITEM pSrcObject = hb_param( 1, HB_IT_OBJECT );
   PHB_ITEM pDstObject;

   if( pSrcObject )
   {
      pDstObject = hb_arrayClone( pSrcObject );
      hb_itemRelease( hb_itemReturnForward( pDstObject ) );
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 3001, NULL, "__OBJCLONE", 0 );
   }
}

/*
 * <hClass> := __clsInstSuper( <cName> )
 *
 * Instance super class and return class handle
 */
HB_FUNC( __CLSINSTSUPER )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_STRING | HB_IT_SYMBOL );
   USHORT uiClassH = 0, uiClass;
   PHB_SYMB pClassFuncSym = NULL;

   if( pItem )
   {
      if( HB_IS_SYMBOL( pItem ) )
         pClassFuncSym = hb_itemGetSymbol( pItem );
      else if( HB_IS_STRING( pItem ) )
      {
         PHB_DYNS pDynSym = hb_dynsymFindName( hb_itemGetCPtr( pItem ) );
         if( pDynSym )
            pClassFuncSym = pDynSym->pSymbol;
      }
      pClassFuncSym = hb_vmGetRealFuncSym( pClassFuncSym );
   }

   if( pClassFuncSym )
   {
      for( uiClass = 1; uiClass <= s_uiClasses; uiClass++ )
      {
         if( s_pClasses[ uiClass ].pClassFuncSym == pClassFuncSym )
         {
            uiClassH = uiClass;
            break;
         }
      }
      if( uiClassH == 0 )
      {
         hb_vmPushSymbol( pClassFuncSym );
         hb_vmPushNil();
         hb_vmFunction( 0 ); /* Execute super class */

         if( hb_vmRequestQuery() == 0 )
         {
            PHB_ITEM pObject = hb_stackReturnItem();

            if( HB_IS_OBJECT( pObject ) )
            {
               uiClass = pObject->item.asArray.value->uiClass;

               if( s_pClasses[ uiClass ].pClassFuncSym == pClassFuncSym )
                  uiClassH = uiClass;
               else
               {
                  for( uiClass = 1; uiClass <= s_uiClasses; uiClass++ )
                  { 
                     if( s_pClasses[ uiClass ].pClassFuncSym == pClassFuncSym )
                     {
                        uiClassH = uiClass;
                        break;
                     }
                  }
                  /* still not found, try to send NEW() message */
                  if( uiClassH == 0 )
                  {
                     hb_vmPushSymbol( &s___msgNew );
                     hb_vmPush( pObject );
                     hb_vmSend( 0 );

                     pObject = hb_stackReturnItem();
                     if( HB_IS_OBJECT( pObject ) )
                     {
                        uiClass = pObject->item.asArray.value->uiClass;
                        if( s_pClasses[ uiClass ].pClassFuncSym == pClassFuncSym )
                           uiClassH = uiClass;
                     }
                  }
               }
            }

            /* This disables destructor execution for this object */
            if( uiClassH && HB_IS_OBJECT( pObject ) )
               pObject->item.asArray.value->uiClass = 0;
            else if( hb_vmRequestQuery() == 0 )
            {
               hb_errRT_BASE( EG_ARG, 3002, "Super class does not return an object", "__CLSINSTSUPER", 0 );
            }
         }
      }
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 3003, "Cannot find super class", "__CLSINSTSUPER", 0 );
   }

   hb_retni( uiClassH );
}

/*
 * <nSeq> = __ClsCntClasses()
 *
 * Return number of classes
 */
HB_FUNC( __CLSCNTCLASSES )
{
   hb_retni( ( int ) s_uiClasses );
}

/*
 * <nSeq> = __cls_CntClsData( <hClass> )
 *
 * Return number of class datas
 */
HB_FUNC( __CLS_CNTCLSDATA )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );

   hb_retni( uiClass && uiClass <= s_uiClasses ?
                  hb_arrayLen( s_pClasses[ uiClass ].pClassDatas ) : 0 );
}

/*
 * <nSeq> = __cls_CntShrData( <hClass> )
 *
 * Return number of class datas
 */
HB_FUNC( __CLS_CNTSHRDATA )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );

   hb_retni( uiClass && uiClass <= s_uiClasses ?
                  hb_arrayLen( s_pClasses[ uiClass ].pSharedDatas ) : 0 );
}

/*
 * <nSeq> = __cls_CntData( <hClass> )
 *
 * Return number of datas
 */
HB_FUNC( __CLS_CNTDATA )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );

   hb_retni( uiClass && uiClass <= s_uiClasses ?
             s_pClasses[ uiClass ].uiDatas : 0 );
}

/*
 * <nSeq> = __cls_DecData( <hClass> )
 *
 * Decrease number of datas and return new value
 */
HB_FUNC( __CLS_DECDATA )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );

   if( uiClass && uiClass <= s_uiClasses &&
       s_pClasses[ uiClass ].uiDatas > s_pClasses[ uiClass ].uiDataFirst )
   {
      if( !s_pClasses[ uiClass ].fLocked )
         s_pClasses[ uiClass ].uiDatas--;
      hb_retni( s_pClasses[ uiClass ].uiDatas - s_pClasses[ uiClass ].uiDataFirst );
   }
   else
      hb_retni( 0 );
}

/*
 * <nSeq> = __cls_IncData( <hClass> )
 * Increase number of datas and return offset to new value
 */
HB_FUNC( __CLS_INCDATA )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );

   if( uiClass && uiClass <= s_uiClasses )
   {
      if( !s_pClasses[ uiClass ].fLocked )
         s_pClasses[ uiClass ].uiDatas++;
      hb_retni( s_pClasses[ uiClass ].uiDatas - s_pClasses[ uiClass ].uiDataFirst );
   }
   else
      hb_retni( 0 );
}

/* NOTE: Undocumented Clipper function */

/* see for parameter compatibility with Clipper. */
HB_FUNC( __CLASSNEW )
{
   HB_FUNC_EXEC( __CLSNEW );
}


/* NOTE: Undocumented Clipper function */

HB_FUNC( __CLASSINSTANCE )
{
   HB_FUNC_EXEC( __CLSINST );
}


/* NOTE: Undocumented Clipper function */

HB_FUNC( __CLASSADD )
{
   HB_FUNC_EXEC( __CLSADDMSG );
}


/* NOTE: Undocumented Clipper function */

HB_FUNC( __CLASSNAME )
{
   hb_retc( hb_clsName( ( USHORT ) hb_parni( 1 ) ) );
}

/* NOTE: Undocumented Clipper function */

HB_FUNC( __CLASSSEL )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );
   PHB_ITEM pReturn = hb_itemNew( NULL );

   if( uiClass && uiClass <= s_uiClasses )
   {
      PCLASS  pClass  = &s_pClasses[ uiClass ];
      PMETHOD pMethod = pClass->pMethods;
      ULONG   ulLimit = hb_clsMthNum( pClass ), ulPos = 0;

      hb_arrayNew( pReturn, pClass->uiMethods ); /* Create a transfer array */

      do
      {
         if( pMethod->pMessage )    /* Hash Entry used ? */
         {
            PHB_ITEM pItem = hb_arrayGetItemPtr( pReturn, ++ulPos );
            if( pItem )
               hb_itemPutC( pItem, pMethod->pMessage->pSymbol->szName );
            else
               break;  /* Generate internal error? */
         }
         ++pMethod;
      }
      while( --ulLimit );

      if( ulPos < ( ULONG ) pClass->uiMethods )
         hb_arraySize( pReturn, ulPos );
   }

   hb_itemRelease( hb_itemReturnForward( pReturn ) );
}

/* to be used from Classes ERROR HANDLER method */
HB_FUNC( __GETMESSAGE )
{
   hb_retc( hb_stackItem( hb_stackBaseItem()->item.asSymbol.stackstate->lBaseItem )->item.asSymbol.value->szName );
}

HB_FUNC( __CLSPARENT )
{
   hb_retl( hb_clsIsParent( hb_parni( 1 ) , hb_parc( 2 ) ) );
}

HB_FUNC( __SENDER )
{
   LONG lOffset = hb_stackBaseProcOffset( 2 );

   if( lOffset > 0 )
   {
      PHB_ITEM pSelf = hb_stackItem( lOffset + 1 );

      /* Is it inline method? */
      if( lOffset > 0 && HB_IS_BLOCK( pSelf ) &&
          hb_stackItem( lOffset )->item.asSymbol.value == &hb_symEval )
      {
         pSelf = hb_stackItem( hb_stackItem( lOffset )->
                               item.asSymbol.stackstate->lBaseItem + 1 );
      }

      if( HB_IS_OBJECT( pSelf ) )
      {
         hb_itemReturn( pSelf );
      }
   }
}

/*
 * ClassH( <obj> ) -> <hClass>
 *
 * Returns class handle of <obj>
 */
HB_FUNC( __CLASSH )
{
   PHB_ITEM pObject = hb_param( 1, HB_IT_OBJECT );

   hb_retni( pObject ? pObject->item.asArray.value->uiClass : 0 );
}

/* ================================================ */

/*
 * <hClass> := <obj>:ClassH()
 *
 * Returns class handle of <obj>
 */
static HARBOUR hb___msgClassH( void )
{
   hb_retni( hb_stackBaseItem()->item.asSymbol.stackstate->uiClass );
}


/*
 * <cClassName> := <obj>:ClassName()
 *
 * Return class name of <obj>. Can also be used for all types.
 */
static HARBOUR hb___msgClassName( void )
{
   USHORT uiClass = hb_stackBaseItem()->item.asSymbol.stackstate->uiClass;

   if( uiClass )
      hb_retc( s_pClasses[ uiClass ].szName );
   else
      hb_retc( hb_objGetClsName( hb_stackSelfItem() ) );
}


/*
 * <aMessages> := <obj>:ClassSel()
 *
 * Returns all the messages in <obj>
 */
static HARBOUR hb___msgClassSel( void )
{
   USHORT uiClass = hb_stackBaseItem()->item.asSymbol.stackstate->uiClass;

   if( uiClass && uiClass <= s_uiClasses )
   {
      PHB_ITEM pReturn = hb_itemNew( NULL );
      PCLASS  pClass  = &s_pClasses[ uiClass ];
      PMETHOD pMethod = pClass->pMethods;
      ULONG ulLimit = hb_clsMthNum( pClass ), ulPos = 0;
      USHORT nParam;

      nParam = hb_pcount() > 0 ? ( USHORT ) hb_parni( 1 ) : HB_MSGLISTALL;
      hb_arrayNew( pReturn, pClass->uiMethods );

      do
      {
         if( pMethod->pMessage )  /* Hash Entry used ? */
         {
            if( ( nParam == HB_MSGLISTALL )  ||
                ( nParam == HB_MSGLISTCLASS &&
                  (
                    ( pMethod->pFuncSym == &s___msgSetClsData ) ||
                    ( pMethod->pFuncSym == &s___msgGetClsData ) ||
                    ( pMethod->pFuncSym == &s___msgSetShrData ) ||
                    ( pMethod->pFuncSym == &s___msgGetShrData )
                  )
                ) ||
                ( nParam == HB_MSGLISTPURE &&
                  !(
                    ( pMethod->pFuncSym == &s___msgSetClsData ) ||
                    ( pMethod->pFuncSym == &s___msgGetClsData ) ||
                    ( pMethod->pFuncSym == &s___msgSetShrData ) ||
                    ( pMethod->pFuncSym == &s___msgGetShrData )
                   )
                )
              )
            {
               hb_itemPutC( hb_arrayGetItemPtr( pReturn, ++ulPos ),
                            pMethod->pMessage->pSymbol->szName );
            }
         }
         ++pMethod;
      }
      while( --ulLimit && ulPos < ( ULONG ) pClass->uiMethods );

      if( ulPos < ( ULONG ) pClass->uiMethods )
         hb_arraySize( pReturn, ulPos );
      hb_itemRelease( hb_itemReturnForward( pReturn ) );
   }
}

#if 0

/*
 * __msgClass()
 *
 * Internal function to return Self at Self:Class call (classy compatibility)
 */
static HARBOUR hb___msgClass( void )
{
   hb_itemReturnForward( hb_stackSelfItem() );
}

/* Added by JfL&RaC
 * <logical> <= <obj>:IsDerivedFrom( xParam )
 *
 * Return true if <obj> is derived from xParam.
 * xParam can be either an obj or a classname
 */
static HARBOUR hb___msgClassParent( void )
{
   char * szParentName = NULL;
   PHB_ITEM pItem;
   USHORT uiClass;

   uiClass = hb_stackBaseItem()->item.asSymbol.stackstate->uiClass;
   pItemParam = hb_param( 1, HB_IT_ANY );

   if( pItemParam )
   {
      if( HB_IS_OBJECT( pItemParam ) )
         szParentName = hb_objGetClsName( pItemParam );
      else if( HB_IS_STRING( pItemParam ) )
         szParentName = hb_parc( pItemParam );
   }

   hb_retl( szParentName && hb_clsIsParent( uiClass , szParentName ) );
}

#endif


/*
 * __msgEvalInline()
 *
 * Internal function executed for inline methods
 */
static HARBOUR hb___msgEvalInline( void )
{
   PHB_STACK_STATE pStack = hb_stackBaseItem()->item.asSymbol.stackstate;
   PCLASS pClass   = &s_pClasses[ pStack->uiClass ];
   PMETHOD pMethod = pClass->pMethods + pStack->uiMethod;
   USHORT uiPCount = hb_pcount(), uiParam;
   PHB_ITEM pBlock;

   hb_vmPushSymbol( &hb_symEval );

   hb_vmPush( hb_arrayGetItemPtr( s_pClasses[ pMethod->uiSprClass ].pInlines,
                                  pMethod->uiData ) );
   pBlock = hb_stackItemFromTop( -1 );    /* Push block */
   pBlock->item.asBlock.hclass = pStack->uiClass;
   pBlock->item.asBlock.method = pStack->uiMethod;

   hb_vmPush( hb_stackSelfItem() );       /* Push self as first argument */

   for( uiParam = 1; uiParam <= uiPCount; uiParam++ )
   {
      hb_vmPush( hb_stackItemFromBase( uiParam ) );
   }

   hb_vmSend( uiPCount + 1 );
}

static HARBOUR hb___msgPerform( void )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ANY );
   USHORT uiPCount = hb_pcount(), uiParam;
   PHB_SYMB pSym = NULL;

   if( pItem )
   {
      if( HB_IS_SYMBOL( pItem ) )
         pSym = pItem->item.asSymbol.value;
      
      else if( HB_IS_OBJECT( pItem ) &&
               s_pClasses[ pItem->item.asArray.value->uiClass ].pClassSym ==
               s___msgSymbol.pDynSym )
      {
         /* Dirty hack */
         pItem = hb_arrayGetItemPtr( pItem, 1 );
         if( pItem && HB_IS_SYMBOL( pItem ) )
            pSym = pItem->item.asSymbol.value;
      }

      if( pSym )
      {
         hb_vmPushSymbol( pSym );
         hb_vmPush( hb_stackSelfItem() );

         for( uiParam = 2; uiParam <= uiPCount; uiParam++ )
         {
            hb_vmPush( hb_stackItemFromBase( uiParam ) );
         }
         hb_vmSend( uiPCount - 1 );
      }
   }
}

static HARBOUR hb___msgDelegate( void )
{
   PCLASS pClass   = &s_pClasses[
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiClass ];
   PMETHOD pMethod = pClass->pMethods +
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;
   PHB_SYMB pExecSym = pClass->pMethods[ pMethod->uiData ].pFuncSym;

   if( pExecSym && pExecSym->value.pFunPtr )
   {
      if( pExecSym->scope.value & HB_FS_PCODEFUNC )
         /* Running pCode dynamic function from .HRB */
         hb_vmExecute( pExecSym->value.pCodeFunc->pCode,
                    pExecSym->value.pCodeFunc->pSymbols );
      else
         pExecSym->value.pFunPtr();
   }
   else
   {
      hb___msgNoMethod();
   }
}

/*
 * __msgNoMethod()
 *
 * Internal function for generating error when not existing message is sent
 */
static HARBOUR hb___msgNoMethod( void )
{
   PHB_SYMB pSym = hb_itemGetSymbol( hb_stackBaseItem() );

#if 1  /* Clipper compatible error message */
   if( pSym->szName[ 0 ] == '_' )
      hb_errRT_BASE_SubstR( EG_NOVARMETHOD, 1005, NULL, pSym->szName + 1, HB_ERR_ARGS_SELFPARAMS );
   else
      hb_errRT_BASE_SubstR( EG_NOMETHOD, 1004, NULL, pSym->szName, HB_ERR_ARGS_SELFPARAMS );
#else
   char szDesc[ 128 ];

   if( pSym->szName[ 0 ] == '_' )
   {
      snprintf( szDesc, sizeof( szDesc ), "Class: '%s' has no property", hb_objGetClsName( hb_stackSelfItem() ) );
      hb_errRT_BASE_SubstR( EG_NOVARMETHOD, 1005, szDesc, pSym->szName + 1, HB_ERR_ARGS_BASEPARAMS );
   }
   else
   {
      snprintf( szDesc, sizeof( szDesc ), "Class: '%s' has no exported method", hb_objGetClsName( hb_stackSelfItem() ) );
      hb_errRT_BASE_SubstR( EG_NOMETHOD, 1004, szDesc, pSym->szName, HB_ERR_ARGS_BASEPARAMS );
   }
#endif
}

/*
 * __msgScopeErr()
 *
 * Internal function for generating error when not existing message is sent
 */
static HARBOUR hb___msgScopeErr( void )
{
   char * pszProcName;
   PHB_ITEM pObject = hb_stackSelfItem();
   PMETHOD pMethod = s_pClasses[
      hb_stackBaseItem()->item.asSymbol.stackstate->uiClass ].pMethods +
      hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;

   pszProcName = hb_xstrcpy( NULL,
                     s_pClasses[ pObject->item.asArray.value->uiClass ].szName,
                     ":", pMethod->pMessage->pSymbol->szName, NULL );
   if( pMethod->uiScope & HB_OO_CLSTP_HIDDEN )
      hb_errRT_BASE( EG_NOMETHOD, 41, "Scope violation (hidden)", pszProcName, 0 );
   else
      hb_errRT_BASE( EG_NOMETHOD, 42, "Scope violation (protected)", pszProcName, 0 );
   hb_xfree( pszProcName );
}

static HARBOUR hb___msgTypeErr( void )
{
   char * pszProcName;
   PHB_ITEM pObject = hb_stackSelfItem();
   PMETHOD pMethod = s_pClasses[
      hb_stackBaseItem()->item.asSymbol.stackstate->uiClass ].pMethods +
      hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;

   pszProcName = hb_xstrcpy( NULL,
                     s_pClasses[ pObject->item.asArray.value->uiClass ].szName,
                     ":", pMethod->pMessage->pSymbol->szName + 1, NULL );
   hb_errRT_BASE( EG_NOMETHOD, 44, "Assigned value is wrong class", pszProcName, HB_ERR_ARGS_BASEPARAMS );
   hb_xfree( pszProcName );
}

/*
 * __msgSuper()
 *
 * Internal function to return a superobject
 */
static HARBOUR hb___msgSuper( void )
{
   PHB_STACK_STATE pStack = hb_stackBaseItem()->item.asSymbol.stackstate;

   hb_clsMakeSuperObject( hb_stackReturnItem(), hb_stackSelfItem(),
      s_pClasses[ pStack->uiClass ].pMethods[ pStack->uiMethod ].uiSprClass );
}

/*
 * __msgRealClass()
 *
 * Internal function to return a superobject of class where the method was
 * defined
 */
static HARBOUR hb___msgRealClass( void )
{
   PHB_ITEM pObject = hb_stackSelfItem();
   USHORT uiClass = hb_clsSenderMethodClasss();

   if( uiClass && uiClass != pObject->item.asArray.value->uiClass &&
       hb_clsSenderObjectClasss() == pObject->item.asArray.value->uiClass )
   {
      hb_clsMakeSuperObject( hb_stackReturnItem(), pObject, uiClass );
   }
   else
   {
      hb_itemReturnForward( pObject );
   }
}

/*
 * __msgGetClsData()
 *
 * Internal function to return a CLASSDATA
 */
static HARBOUR hb___msgGetClsData( void )
{
   PCLASS pClass   = &s_pClasses[
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiClass ];
   PMETHOD pMethod = pClass->pMethods +
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;

   hb_arrayGet( pClass->pClassDatas, pMethod->uiData, hb_stackReturnItem() );
}


/*
 * __msgSetClsData()
 *
 * Internal function to set a CLASSDATA
 */
static HARBOUR hb___msgSetClsData( void )
{
   PCLASS pClass   = &s_pClasses[
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiClass ];
   PMETHOD pMethod = pClass->pMethods +
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;
   PHB_ITEM pReturn = hb_param( 1, HB_IT_ANY );

   if( !pReturn )
      hb_arrayGet( pClass->pClassDatas, pMethod->uiData, hb_stackReturnItem() );

   else
   {
      if( pMethod->itemType && ! ( pMethod->itemType & pReturn->type ) )
      {
         if( pMethod->itemType == HB_IT_NUMINT && HB_IS_NUMERIC( pReturn ) )
            hb_itemPutNInt( pReturn, hb_itemGetNInt( pReturn ) );
         else
         {
            (s___msgTypeErr.value.pFunPtr)();
            return;
         }
      }

      hb_arraySet( pClass->pClassDatas, pMethod->uiData, pReturn );
      hb_itemReturnForward( pReturn );
   }
}

/*
 * __msgGetShrData()
 *
 * Internal function to return a SHAREDDATA
 */
static HARBOUR hb___msgGetShrData( void )
{
   PCLASS pClass   = &s_pClasses[
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiClass ];
   PMETHOD pMethod = pClass->pMethods +
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;

   hb_arrayGet( s_pClasses[ pMethod->uiSprClass ].pSharedDatas,
                pMethod->uiData, hb_stackReturnItem() );
}

/*
 * __msgSetShrData()
 *
 * Internal function to set a SHAREDDATA
 */
static HARBOUR hb___msgSetShrData( void )
{
   PCLASS pClass   = &s_pClasses[
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiClass ];
   PMETHOD pMethod = pClass->pMethods +
                  hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;
   PHB_ITEM pReturn = hb_param( 1, HB_IT_ANY );

   if( !pReturn )
      hb_arrayGet( s_pClasses[ pMethod->uiSprClass ].pSharedDatas,
                   pMethod->uiData, hb_stackReturnItem() );
   else
   {
      if( pMethod->itemType && ! ( pMethod->itemType & pReturn->type ) )
      {
         if( pMethod->itemType == HB_IT_NUMINT && HB_IS_NUMERIC( pReturn ) )
            hb_itemPutNInt( pReturn, hb_itemGetNInt( pReturn ) );
         else
         {
            (s___msgTypeErr.value.pFunPtr)();
            return;
         }
      }

      hb_arraySet( s_pClasses[ pMethod->uiSprClass ].pSharedDatas,
                   pMethod->uiData, pReturn );
      hb_itemReturnForward( pReturn );
   }
}

/*
 * __msgGetData()
 *
 * Internal function to return a DATA
 */
static HARBOUR hb___msgGetData( void )
{
   PHB_ITEM pObject  = hb_stackSelfItem();
   USHORT uiObjClass = pObject->item.asArray.value->uiClass;
   USHORT uiClass    = hb_stackBaseItem()->item.asSymbol.stackstate->uiClass;
   PCLASS pClass     = &s_pClasses[ uiClass ];
   PMETHOD pMethod   = pClass->pMethods +
                       hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;
   ULONG ulIndex     = pMethod->uiData;

   if( uiClass != uiObjClass )
   {
      ulIndex += hb_clsParentInstanceOffset( &s_pClasses[ uiObjClass ],
                           s_pClasses[ pMethod->uiSprClass ].pClassSym );
   }
   else
   {
      ulIndex += pMethod->uiOffset;
   }

   hb_arrayGet( pObject, ulIndex, hb_stackReturnItem() );
}

/*
 * __msgSetData()
 *
 * Internal function to set a DATA
 */
static HARBOUR hb___msgSetData( void )
{
   PHB_ITEM pReturn  = hb_param( 1, HB_IT_ANY );
   PHB_ITEM pObject  = hb_stackSelfItem();
   USHORT uiObjClass = pObject->item.asArray.value->uiClass;
   USHORT uiClass    = hb_stackBaseItem()->item.asSymbol.stackstate->uiClass;
   PCLASS pClass     = &s_pClasses[ uiClass ];
   PMETHOD pMethod   = pClass->pMethods +
                       hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;
   ULONG ulIndex     = pMethod->uiData;

   if( uiClass != uiObjClass )
   {
      ulIndex += hb_clsParentInstanceOffset( &s_pClasses[ uiObjClass ],
                           s_pClasses[ pMethod->uiSprClass ].pClassSym );
   }
   else
   {
      ulIndex += pMethod->uiOffset;
   }

   if( !pReturn )
      hb_arrayGet( pObject, ulIndex, hb_stackReturnItem() );

   else
   {
      if( pMethod->itemType && ! ( pMethod->itemType & pReturn->type ) )
      {
         if( pMethod->itemType == HB_IT_NUMINT && HB_IS_NUMERIC( pReturn ) )
            hb_itemPutNInt( pReturn, hb_itemGetNInt( pReturn ) );
         else
         {
            (s___msgTypeErr.value.pFunPtr)();
            return;
         }
      }

      /* will arise only if the class has been modified after first instance */
      if( ulIndex > hb_arrayLen( pObject ) ) /* Resize needed ? */
         hb_arraySize( pObject, ulIndex );   /* Make large enough */
      hb_arraySet( pObject, ulIndex, pReturn );
      hb_itemReturnForward( pReturn );
   }
}

/* No comment :-) */
static HARBOUR hb___msgVirtual( void )
{
   /* hb_ret(); */ /* NOTE: It's safe to comment this out */
   ;
}

static HARBOUR hb___msgNull( void )
{
   ;
}

#ifndef HB_NO_PROFILER
void hb_mthAddTime( ULONG ulClockTicks )
{
   PMETHOD pMethod =
            s_pClasses[ hb_stackSelfItem()->item.asArray.value->uiClass ].
            pMethods + hb_stackBaseItem()->item.asSymbol.stackstate->uiMethod;

   pMethod->ulCalls++;
   pMethod->ulTime += ulClockTicks;
}
#endif

HB_FUNC( __GETMSGPRF ) /* profiler: returns a method called and consumed times */
                       /* ( nClass, cMsg ) --> aMethodInfo { nTimes, nTime } */
{
#ifndef HB_NO_PROFILER
   USHORT uiClass = ( USHORT ) hb_parni( 1 );
   char * cMsg    = hb_parc( 2 );

   hb_reta( 2 );
   if( uiClass && uiClass <= s_uiClasses && cMsg && *cMsg )
   {
      PHB_DYNS pMsg = hb_dynsymFindName( cMsg );

      if( pMsg )
      {
         PMETHOD pMethod = hb_clsFindMsg( &s_pClasses[ uiClass ], pMsg );

         if( pMethod )
         {
            hb_stornl( pMethod->ulCalls, -1, 1 );
            hb_stornl( pMethod->ulTime, -1, 2 );
            return;
         }
      }
   }
#else
   hb_reta( 2 );
#endif
   hb_stornl( 0, -1, 1 );
   hb_stornl( 0, -1, 2 );
}

/* __ClsGetProperties( nClassHandle ) --> aPropertiesNames
 * Notice that this function works quite similar to __CLASSSEL()
 * except that just returns the name of the datas and methods
 * that have been declared as PROPERTY (or PERSISTENT) */

HB_FUNC( __CLSGETPROPERTIES )
{
   USHORT uiClass = ( USHORT ) hb_parni( 1 );
   PHB_ITEM pReturn = hb_itemNew( NULL );

   if( uiClass && uiClass <= s_uiClasses )
   {
      PCLASS  pClass  = &s_pClasses[ uiClass ];
      PMETHOD pMethod = pClass->pMethods;
      ULONG ulLimit = hb_clsMthNum( pClass );
      PHB_ITEM pItem = NULL;

      hb_arrayNew( pReturn, 0 );

      do
      {
         if( pMethod->pMessage && ( pMethod->uiScope & HB_OO_CLSTP_PERSIST ) )
         {
            pItem = hb_itemPutC( pItem, pMethod->pMessage->pSymbol->szName );
            hb_arrayAdd( pReturn, pItem );
         }
         ++pMethod;
      }
      while( --ulLimit );

      if( pItem )
         hb_itemRelease( pItem );
   }

   hb_itemRelease( hb_itemReturnForward( pReturn ) );
}

/* Real dirty function, though very usefull under certain circunstances:
 * It allows to change the class handle of an object into another class handle,
 * so the object behaves like a different Class of object.
 * Based on objects.lib SetClsHandle() */

HB_FUNC( HB_SETCLSHANDLE ) /* ( oObject, nClassHandle ) --> nPrevClassHandle */
{
   PHB_ITEM pObject = hb_param( 1, HB_IT_OBJECT );
   USHORT uiPrevClassHandle = 0;

   if( pObject )
   {
      USHORT uiClass = ( USHORT ) hb_parni( 2 );

      uiPrevClassHandle = pObject->item.asArray.value->uiClass;
      if( uiClass <= s_uiClasses )
         pObject->item.asArray.value->uiClass = uiClass;
   }

   hb_retnl( uiPrevClassHandle );
}

/* Harbour equivalent for Clipper internal __mdCreate() */
USHORT hb_clsCreate( USHORT usSize, const char * szClassName )
{
   return hb_clsNew( szClassName, usSize, NULL, NULL, FALSE );
}

/* Harbour equivalent for Clipper internal __mdAdd() */
void hb_clsAdd( USHORT usClassH, const char * szMethodName, PHB_FUNC pFuncPtr )
{
   PHB_SYMB pExecSym;
   PHB_ITEM pFuncItem;

   /*
    * We can use empty name "" for this symbol in hb_symbolNew()
    * It's only envelop for function with additional execution
    * information for HVM not registered symbol. [druzus]
    */
   pExecSym = hb_symbolNew( "" );
   pExecSym->value.pFunPtr = pFuncPtr;
   pFuncItem = hb_itemPutSymbol( NULL, pExecSym );

   hb_clsAddMsg( usClassH, szMethodName, HB_OO_MSG_METHOD, 0, pFuncItem, NULL );

   hb_itemRelease( pFuncItem );
}

/* Harbour equivalent for Clipper internal __mdAssociate() */
void hb_clsAssociate( USHORT usClassH )
{
   PHB_ITEM pSelf = hb_clsInst( usClassH );

   if( pSelf )
      hb_itemRelease( hb_itemReturnForward( pSelf ) );
}


#if 1
/*
 * __CLS_PARAM() and __CLS_PAR00() functions are only for backward binary
 * compatibility. They will be removed in the future so please do not use
 * them.
 */
HB_FUNC( __CLS_PARAM )
{
   PHB_ITEM array;
   USHORT uiParam = ( USHORT ) hb_pcount();
   USHORT n;

   if( uiParam >= 1 )
   {
      array = hb_itemArrayNew( uiParam );
      for( n = 1; n <= uiParam; n++ )
      {
         hb_arraySet( array, n, hb_param( n, HB_IT_ANY ) );
      }
   }
   else
   {
      array = hb_itemArrayNew( 1 );
      hb_itemPutC( hb_arrayGetItemPtr( array, 1 ), "HBObject" );
   }

   hb_itemRelease( hb_itemReturnForward( array ) );
}

HB_FUNC( __CLS_PAR00 )
{
   PHB_ITEM array;
   USHORT uiParam = ( USHORT ) hb_pcount();
   USHORT n;

   array = hb_itemArrayNew( uiParam );
   for( n = 1; n <= uiParam; n++ )
   {
      hb_arraySet( array, n, hb_param( n, HB_IT_ANY ) );
   }

   hb_itemRelease( hb_itemReturnForward( array ) );
}

/*
 * This function is only for backward binary compatibility
 * It will be removed in the future so please do not use it.
 * Use hb_objHasMessage() instead.
 */
#if defined(__cplusplus)
   extern "C" BOOL hb_objGetpMethod( PHB_ITEM pObject, PHB_SYMB pMessage );
#endif
BOOL hb_objGetpMethod( PHB_ITEM pObject, PHB_SYMB pMessage )
{
   return hb_objHasMessage( pObject, pMessage->pDynSym );
}

#endif

#if 0
/*
 * return real function name ignoring aliasing
 */
const char * hb_clsRealMethodName( void )
{
   LONG lOffset = hb_stackBaseProcOffset( 1 );
   const char * szName = NULL;

   if( lOffset > 0 )
   {
      PHB_STACK_STATE pStack = hb_stackItem( lOffset )->item.asSymbol.stackstate;

      if( pStack->uiClass && pStack->uiClass <= s_uiClasses )
      {
         PCLASS pClass = &s_pClasses[ pStack->uiClass ];

         if( ( ULONG ) pStack->uiMethod < hb_clsMthNum( pClass ) )
         {
            PMETHOD pMethod = pClass->pMethods + pStack->uiMethod;

            if( pMethod->pMessage )
               szName = pMethod->pMessage->pSymbol->szName;
         }
      }
   }
   return szName;
}
#endif
