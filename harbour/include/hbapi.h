/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for the Extend API, Array API, misc API and base declarations
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

/* TOFIX: There are several things in this file which are not part of the
          standard Harbour API, in other words these things are not
          guaranteed to remain unchanged. To avoid confusion these should be
          moved to somewhere else (like HBRTL.H). [vszakats] */

#ifndef HB_APIEXT_H_
#define HB_APIEXT_H_

#include "hbvmpub.h"

HB_EXTERN_BEGIN


/* this definition signals that number of decimal places for double value
 * was not specified at compile time (the value is a result of optimization
 * performed by the compiler)
 */
#define HB_DEFAULT_WIDTH     255
#define HB_DEFAULT_DECIMALS  255


/* items types and type checking macros */
#define HB_IT_NIL       ( ( HB_TYPE ) 0x00000 )
#define HB_IT_POINTER   ( ( HB_TYPE ) 0x00001 )
#define HB_IT_INTEGER   ( ( HB_TYPE ) 0x00002 )
#define HB_IT_LONG      ( ( HB_TYPE ) 0x00008 )
#define HB_IT_DOUBLE    ( ( HB_TYPE ) 0x00010 )
#define HB_IT_DATE      ( ( HB_TYPE ) 0x00020 )
#define HB_IT_LOGICAL   ( ( HB_TYPE ) 0x00080 )
#define HB_IT_SYMBOL    ( ( HB_TYPE ) 0x00100 )
#define HB_IT_ALIAS     ( ( HB_TYPE ) 0x00200 )
#define HB_IT_STRING    ( ( HB_TYPE ) 0x00400 )
#define HB_IT_MEMOFLAG  ( ( HB_TYPE ) 0x00800 )
#define HB_IT_MEMO      ( HB_IT_MEMOFLAG | HB_IT_STRING )
#define HB_IT_BLOCK     ( ( HB_TYPE ) 0x01000 )
#define HB_IT_BYREF     ( ( HB_TYPE ) 0x02000 )
#define HB_IT_MEMVAR    ( ( HB_TYPE ) 0x04000 )
#define HB_IT_ARRAY     ( ( HB_TYPE ) 0x08000 )
#define HB_IT_ENUM      ( ( HB_TYPE ) 0x10000 )
#define HB_IT_OBJECT    HB_IT_ARRAY
#define HB_IT_NUMERIC   ( ( HB_TYPE ) ( HB_IT_INTEGER | HB_IT_LONG | HB_IT_DOUBLE ) )
#define HB_IT_NUMINT    ( ( HB_TYPE ) ( HB_IT_INTEGER | HB_IT_LONG ) )
#define HB_IT_ANY       ( ( HB_TYPE ) 0xFFFFFFFF )
#define HB_IT_COMPLEX   ( ( HB_TYPE ) ( HB_IT_BLOCK | HB_IT_ARRAY | HB_IT_POINTER | HB_IT_MEMVAR | HB_IT_ENUM | HB_IT_BYREF | HB_IT_STRING ) )
#define HB_IT_GCITEM    ( ( HB_TYPE ) ( HB_IT_BLOCK | HB_IT_ARRAY | HB_IT_POINTER | HB_IT_BYREF ) )

#if 0

/*
 * In Harbour VM HB_IT_BYREF is never ORed with item type. It can be used
 * as stand alone type for locals and statics passed by reference or with
 * HB_IT_MEMVAR for memvars passed by reference so this macro is less usable.
 * only the hb_parinfo() function can return HB_TYPE as HB_IT_BYREF ORed
 * with real type but this value is never set as item type.
 */

#define HB_IS_OF_TYPE( p, t ) ( ( HB_ITEM_TYPE( p ) & ~HB_IT_BYREF ) == t )

/*
 * These macros are slower but can be usable in debugging some code.
 * They are a little bit more safe in buggy code but they can
 * also hide bugs which should be exploited as soon as possible to
 * know that sth is wrong and has to be fixed.
 * the version below which check only chosen bits allow compiler to
 * use some optimizations if used CPU supports it. F.e. on standard
 * x86 machines they can safe few CPU cycles. [druzus]
 */

#define HB_IS_NIL( p )        HB_IS_OF_TYPE( p, HB_IT_NIL )
#define HB_IS_ARRAY( p )      HB_IS_OF_TYPE( p, HB_IT_ARRAY )
#define HB_IS_BLOCK( p )      HB_IS_OF_TYPE( p, HB_IT_BLOCK )
#define HB_IS_DATE( p )       HB_IS_OF_TYPE( p, HB_IT_DATE )
#define HB_IS_DOUBLE( p )     HB_IS_OF_TYPE( p, HB_IT_DOUBLE )
#define HB_IS_INTEGER( p )    HB_IS_OF_TYPE( p, HB_IT_INTEGER )
#define HB_IS_LOGICAL( p )    HB_IS_OF_TYPE( p, HB_IT_LOGICAL )
#define HB_IS_LONG( p )       HB_IS_OF_TYPE( p, HB_IT_LONG )
#define HB_IS_SYMBOL( p )     HB_IS_OF_TYPE( p, HB_IT_SYMBOL )
#define HB_IS_POINTER( p )    HB_IS_OF_TYPE( p, HB_IT_POINTER )
#define HB_IS_MEMVAR( p )     HB_IS_OF_TYPE( p, HB_IT_MEMVAR )
#define HB_IS_MEMO( p )       HB_IS_OF_TYPE( p, HB_IT_MEMO )
#define HB_IS_ENUM( p )       HB_IS_OF_TYPE( p, HB_IT_ENUM )
#define HB_IS_STRING( p )     ( ( HB_ITEM_TYPE( p ) & ~( HB_IT_BYREF | HB_IT_MEMOFLAG ) ) == HB_IT_STRING )
#define HB_IS_BYREF( p )      ( ( HB_ITEM_TYPE( p ) & HB_IT_BYREF ) != 0 )
#define HB_IS_NUMERIC( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_NUMERIC ) != 0 )
#define HB_IS_NUMINT( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_NUMINT ) != 0 )
#define HB_IS_COMPLEX( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_COMPLEX ) != 0 )
#define HB_IS_GCITEM( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_GCITEM ) != 0 )
#define HB_IS_BADITEM( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_COMPLEX ) != 0 && ( HB_ITEM_TYPE( p ) & ~( HB_IT_COMPLEX | HB_IT_MEMOFLAG ) ) != 0 )
#define HB_IS_OBJECT( p )     ( HB_IS_ARRAY( p ) && HB_ARRAY_OBJ( p ) )
#define HB_IS_NUMBER( p )     HB_IS_NUMERIC( p )

#elif 0

/*
 * these macros illustrates possible HB_TYPE bit combinations in HVM,
 * they are the safest one in buggy code which may produce wrong item
 * signatures but also they can be slower on some machines
 */
#define HB_IS_NIL( p )        ( HB_ITEM_TYPE( p ) == HB_IT_NIL )
#define HB_IS_ARRAY( p )      ( HB_ITEM_TYPE( p ) == HB_IT_ARRAY )
#define HB_IS_BLOCK( p )      ( HB_ITEM_TYPE( p ) == HB_IT_BLOCK )
#define HB_IS_DATE( p )       ( HB_ITEM_TYPE( p ) == HB_IT_DATE )
#define HB_IS_DOUBLE( p )     ( HB_ITEM_TYPE( p ) == HB_IT_DOUBLE )
#define HB_IS_INTEGER( p )    ( HB_ITEM_TYPE( p ) == HB_IT_INTEGER )
#define HB_IS_LOGICAL( p )    ( HB_ITEM_TYPE( p ) == HB_IT_LOGICAL )
#define HB_IS_LONG( p )       ( HB_ITEM_TYPE( p ) == HB_IT_LONG )
#define HB_IS_SYMBOL( p )     ( HB_ITEM_TYPE( p ) == HB_IT_SYMBOL )
#define HB_IS_POINTER( p )    ( HB_ITEM_TYPE( p ) == HB_IT_POINTER )
#define HB_IS_MEMO( p )       ( HB_ITEM_TYPE( p ) == HB_IT_MEMO )
#define HB_IS_MEMVAR( p )     ( HB_ITEM_TYPE( p ) == ( HB_IT_MEMVAR | HB_IT_BYREF ) )
#define HB_IS_ENUM( p )       ( HB_ITEM_TYPE( p ) == ( HB_IT_ENUM | HB_IT_BYREF ) )
#define HB_IS_STRING( p )     ( ( HB_ITEM_TYPE( p ) & ~HB_IT_MEMOFLAG ) == HB_IT_STRING )
#define HB_IS_BYREF( p )      ( ( HB_ITEM_TYPE( p ) & ~HB_IT_MEMVAR ) == HB_IT_BYREF )
#define HB_IS_NUMERIC( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_NUMERIC ) != 0 )
#define HB_IS_NUMINT( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_NUMINT ) != 0 )
#define HB_IS_COMPLEX( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_COMPLEX ) != 0 )
#define HB_IS_GCITEM( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_GCITEM ) != 0 )
#define HB_IS_BADITEM( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_COMPLEX ) != 0 && ( HB_ITEM_TYPE( p ) & ~( HB_IT_COMPLEX | HB_IT_MEMOFLAG ) ) != 0 )
#define HB_IS_OBJECT( p )     ( HB_IS_ARRAY( p ) && HB_ARRAY_OBJ( p ) )
#define HB_IS_NUMBER( p )     HB_IS_NUMERIC( p )

#else

/*
 * these ones are can be the most efficiently optimized on some CPUs
 */
#define HB_IS_NIL( p )        ( HB_ITEM_TYPE( p ) == HB_IT_NIL )
#define HB_IS_ARRAY( p )      ( ( HB_ITEM_TYPE( p ) & HB_IT_ARRAY ) != 0 )
#define HB_IS_BLOCK( p )      ( ( HB_ITEM_TYPE( p ) & HB_IT_BLOCK ) != 0 )
#define HB_IS_DATE( p )       ( ( HB_ITEM_TYPE( p ) & HB_IT_DATE ) != 0 )
#define HB_IS_DOUBLE( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_DOUBLE ) != 0 )
#define HB_IS_INTEGER( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_INTEGER ) != 0 )
#define HB_IS_LOGICAL( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_LOGICAL ) != 0 )
#define HB_IS_LONG( p )       ( ( HB_ITEM_TYPE( p ) & HB_IT_LONG ) != 0 )
#define HB_IS_SYMBOL( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_SYMBOL ) != 0 )
#define HB_IS_POINTER( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_POINTER ) != 0 )
#define HB_IS_MEMO( p )       ( ( HB_ITEM_TYPE( p ) & HB_IT_MEMOFLAG ) != 0 )
#define HB_IS_STRING( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_STRING ) != 0 )
#define HB_IS_MEMVAR( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_MEMVAR ) != 0 )
#define HB_IS_ENUM( p )       ( ( HB_ITEM_TYPE( p ) & HB_IT_ENUM ) != 0 )
#define HB_IS_BYREF( p )      ( ( HB_ITEM_TYPE( p ) & HB_IT_BYREF ) != 0 )
#define HB_IS_NUMERIC( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_NUMERIC ) != 0 )
#define HB_IS_NUMINT( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_NUMINT ) != 0 )
#define HB_IS_COMPLEX( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_COMPLEX ) != 0 )
#define HB_IS_GCITEM( p )     ( ( HB_ITEM_TYPE( p ) & HB_IT_GCITEM ) != 0 )
#define HB_IS_BADITEM( p )    ( ( HB_ITEM_TYPE( p ) & HB_IT_COMPLEX ) != 0 && ( HB_ITEM_TYPE( p ) & ~( HB_IT_COMPLEX | HB_IT_MEMOFLAG ) ) != 0 )
#define HB_IS_OBJECT( p )     ( HB_IS_ARRAY( p ) && HB_ARRAY_OBJ( p ) )
#define HB_IS_NUMBER( p )     HB_IS_NUMERIC( p )

#endif


#define ISNIL( n )         ( hb_param( n, HB_IT_ANY ) == NULL || HB_IS_NIL( hb_param( n, HB_IT_ANY ) ) ) /* NOTE: Intentionally using a different method */
#define ISCHAR( n )        ( hb_param( n, HB_IT_STRING ) != NULL )
#define ISNUM( n )         ( hb_param( n, HB_IT_NUMERIC ) != NULL )
#define ISLOG( n )         ( hb_param( n, HB_IT_LOGICAL ) != NULL )
#define ISDATE( n )        ( hb_param( n, HB_IT_DATE ) != NULL )
#define ISMEMO( n )        ( hb_param( n, HB_IT_MEMO ) != NULL )
#define ISBYREF( n )       ( ( hb_parinfo( n ) & HB_IT_BYREF ) != 0 ) /* NOTE: Intentionally using a different method */
#define ISARRAY( n )       ( hb_param( n, HB_IT_ARRAY ) != NULL )
#define ISOBJECT( n )      ( hb_extIsObject( n ) )
#define ISBLOCK( n )       ( hb_param( n, HB_IT_BLOCK ) != NULL ) /* Not available in CA-Cl*pper. */
#define ISPOINTER( n )     ( hb_param( n, HB_IT_POINTER ) != NULL ) /* Not available in CA-Cl*pper. */
#define ISSYMBOL( n )      ( hb_param( n, HB_IT_SYMBOL ) != NULL ) /* Not available in CA-Cl*pper. */


#ifdef _HB_API_INTERNAL_

/* forward declarations */
struct _HB_CODEBLOCK;
struct _HB_BASEARRAY;
struct _HB_ITEM;
struct _HB_VALUE;

typedef struct _HB_STACK_STATE
{
   LONG     lBaseItem;        /* stack base offset of previous func/proc */
   LONG     lStatics;         /* statics offset of previous func/proc */
   ULONG    ulPrivateBase;    /* memvars base offset of previous func/proc */
   USHORT   uiClass;          /* class when message is sent */
   USHORT   uiMethod;         /* number of class method */
   USHORT   uiLineNo;         /* current line number */
} HB_STACK_STATE, * PHB_STACK_STATE; /* used to save/restore stack state in hb_vmDo)_ */


/* Internal structures that holds data */
struct hb_struArray
{
   struct _HB_BASEARRAY * value;
};

struct hb_struBlock
{
   struct _HB_CODEBLOCK * value;
   USHORT paramcnt;
   USHORT lineno;
   USHORT hclass;
   USHORT method;
};

struct hb_struDate
{
   long value;
};

struct hb_struDouble
{
   USHORT length;
   USHORT decimal;
   double value;
};

struct hb_struInteger
{
   USHORT length;
   int value;
};

struct hb_struLogical
{
   BOOL value;
};

struct hb_struLong
{
   USHORT length;
   HB_LONG value;
};

struct hb_struPointer
{
   void * value;
   BOOL collect;
   BOOL single;
};

struct hb_struMemvar
{
   struct _HB_VALUE ** itemsbase;
   LONG value;
};

struct hb_struRefer
{
   union {
      struct _HB_BASEARRAY * array;       /* array (statics and array item references) */
      struct _HB_CODEBLOCK * block;       /* codeblock */
      struct _HB_ITEM * itemPtr;          /* item pointer  */
      struct _HB_ITEM ** *itemsbasePtr;   /* local variables */
   } BasePtr;
   LONG offset;                           /* 0 for static variables */
   LONG value;
};

struct hb_struEnum
{
   struct _HB_ITEM * basePtr;             /* base item pointer */
   struct _HB_ITEM * valuePtr;            /* value item pointer */
   LONG offset;
};

struct hb_struString
{
   ULONG length;
   ULONG allocated;     /* size of memory block allocated for string value, 0 for static strings */
   char * value;
};

struct hb_struSymbol
{
   PHB_SYMB        value;
   PHB_STACK_STATE stackstate;      /* function stack state */
   USHORT          paramcnt;        /* number of passed parameters in function call */
   USHORT          paramdeclcnt;    /* number of declared parameters in function definition */
};

/* items hold at the virtual machine stack */
typedef struct _HB_ITEM
{
   HB_TYPE type;
   union
   {
      struct hb_struArray     asArray;
      struct hb_struBlock     asBlock;
      struct hb_struDate      asDate;
      struct hb_struDouble    asDouble;
      struct hb_struInteger   asInteger;
      struct hb_struLogical   asLogical;
      struct hb_struLong      asLong;
      struct hb_struPointer   asPointer;
      struct hb_struMemvar    asMemvar;
      struct hb_struRefer     asRefer;
      struct hb_struEnum      asEnum;
      struct hb_struString    asString;
      struct hb_struSymbol    asSymbol;
   } item;
} HB_ITEM, * PHB_ITEM, * HB_ITEM_PTR;

typedef struct _HB_BASEARRAY
{
   PHB_ITEM    pItems;       /* pointer to the array items */
   ULONG       ulLen;        /* number of items in the array */
   USHORT      uiClass;      /* offset to the classes base if it is an object */
   USHORT      uiPrevCls;    /* for fixing after access super */
} HB_BASEARRAY, * PHB_BASEARRAY, * HB_BASEARRAY_PTR;

/* internal structure for codeblocks */
typedef struct _HB_CODEBLOCK
{
   BYTE *      pCode;        /* codeblock pcode */
   PHB_SYMB    pSymbols;     /* codeblocks symbols */
   PHB_SYMB    pDefSymb;     /* symbol where the codeblock was created */
   PHB_ITEM    pLocals;      /* table with referenced local variables */
   LONG        lStatics;     /* STATICs base address */
   USHORT      uiLocals;     /* number of referenced local variables */
   SHORT       dynBuffer;    /* is pcode buffer allocated dynamically, SHORT used instead of BOOL intentionally to force optimal alignment */
} HB_CODEBLOCK, * PHB_CODEBLOCK, * HB_CODEBLOCK_PTR;

typedef struct _HB_VALUE
{
   HB_ITEM_PTR pVarItem;
   HB_COUNTER  counter;
   HB_HANDLE   hPrevMemvar;
} HB_VALUE, * PHB_VALUE, * HB_VALUE_PTR;

typedef struct _HB_NESTED_CLONED
{
   PHB_BASEARRAY            pSrcBaseArray;
   PHB_ITEM                 pDest;
   struct _HB_NESTED_CLONED * pNext;
} HB_NESTED_CLONED, * PHB_NESTED_CLONED;

#endif


/* RDD method return codes */
typedef USHORT ERRCODE;
#define SUCCESS            0
#define FAILURE            1

extern HB_SYMB  hb_symEval;

/* Extend API */
extern HB_EXPORT char *     hb_parc( int iParam, ... );  /* retrieve a string parameter */
extern HB_EXPORT char *     hb_parcx( int iParam, ... );  /* retrieve a string parameter */
extern HB_EXPORT ULONG      hb_parclen( int iParam, ... ); /* retrieve a string parameter length */
extern HB_EXPORT ULONG      hb_parcsiz( int iParam, ... ); /* retrieve a by-reference string parameter length, including terminator */
extern HB_EXPORT char *     hb_pards( int iParam, ... ); /* retrieve a date as a string yyyymmdd */
extern HB_EXPORT char *     hb_pardsbuff( char * szDate, int iParam, ... ); /* retrieve a date as a string yyyymmdd */
extern HB_EXPORT LONG       hb_pardl( int iParam, ... ); /* retrieve a date as a LONG NUMBER  */
extern HB_EXPORT ULONG      hb_parinfa( int iParamNum, ULONG uiArrayIndex ); /* retrieve length or element type of an array parameter */
extern HB_EXPORT ULONG      hb_parinfo( int iParam ); /* Determine the param count or data type */
extern HB_EXPORT int        hb_parl( int iParam, ... ); /* retrieve a logical parameter as an int */
extern HB_EXPORT double     hb_parnd( int iParam, ... ); /* retrieve a numeric parameter as a double */
extern HB_EXPORT int        hb_parni( int iParam, ... ); /* retrieve a numeric parameter as a integer */
extern HB_EXPORT long       hb_parnl( int iParam, ... ); /* retrieve a numeric parameter as a long */
extern HB_EXPORT HB_LONG    hb_parnint( int iParam, ... ); /* retrieve a numeric parameter as a HB_LONG */
extern HB_EXPORT void *     hb_parptr( int iParam, ... ); /* retrieve a parameter as a pointer */
extern HB_EXPORT PHB_ITEM   hb_param( int iParam, long lMask ); /* retrieve a generic parameter */
extern HB_EXPORT PHB_ITEM   hb_paramError( int iParam ); /* Returns either the generic parameter or a NIL item if param not provided */
extern HB_EXPORT BOOL       hb_extIsArray( int iParam );
extern HB_EXPORT BOOL       hb_extIsObject( int iParam );

#ifndef HB_LONG_LONG_OFF
extern HB_EXPORT LONGLONG   hb_parnll( int iParam, ... ); /* retrieve a numeric parameter as a long long */
#endif

extern HB_EXPORT int    hb_pcount( void );          /* returns the number of suplied parameters */
extern HB_EXPORT void   hb_ret( void );             /* post a NIL return value */
extern HB_EXPORT void   hb_retc( const char * szText );   /* returns a string */
extern HB_EXPORT void   hb_retc_buffer( char * szText ); /* sames as above, but accepts an allocated buffer */
extern HB_EXPORT void   hb_retc_const( const char * szText ); /* returns a string as a pcode based string */
extern HB_EXPORT void   hb_retclen( const char * szText, ULONG ulLen ); /* returns a string with a specific length */
extern HB_EXPORT void   hb_retclen_buffer( char * szText, ULONG ulLen ); /* sames as above, but accepts an allocated buffer */
extern HB_EXPORT void   hb_retds( const char * szDate );  /* returns a date, must use yyyymmdd format */
extern HB_EXPORT void   hb_retd( int iYear, int iMonth, int iDay ); /* returns a date */
extern HB_EXPORT void   hb_retdl( long lJulian );   /* returns a long value as a julian date */
extern HB_EXPORT void   hb_retl( int iTrueFalse );  /* returns a logical integer */
extern HB_EXPORT void   hb_retnd( double dNumber ); /* returns a double */
extern HB_EXPORT void   hb_retni( int iNumber );    /* returns a integer number */
extern HB_EXPORT void   hb_retnl( long lNumber );/* returns a long number */
extern HB_EXPORT void   hb_retnint( HB_LONG lNumber );/* returns a long number */
extern HB_EXPORT void   hb_retnlen( double dNumber, int iWidth, int iDec ); /* returns a double, with specific width and decimals */
extern HB_EXPORT void   hb_retndlen( double dNumber, int iWidth, int iDec ); /* returns a double, with specific width and decimals */
extern HB_EXPORT void   hb_retnilen( int iNumber, int iWidth ); /* returns a integer number, with specific width */
extern HB_EXPORT void   hb_retnllen( long lNumber, int iWidth ); /* returns a long number, with specific width */
extern HB_EXPORT void   hb_retnintlen( HB_LONG lNumber, int iWidth ); /* returns a long long number, with specific width */
extern HB_EXPORT void   hb_reta( ULONG ulLen );  /* returns an array with a specific length */
extern HB_EXPORT void   hb_retptr( void * ptr );  /* returns a pointer */
extern HB_EXPORT void   hb_retptrGC( void * ptr );  /* returns a pointer to an allocated memory, collected by GC */
#ifndef HB_LONG_LONG_OFF
extern HB_EXPORT void   hb_retnll( LONGLONG lNumber );/* returns a long long number */
extern HB_EXPORT void   hb_retnlllen( LONGLONG lNumber, int iWidth ); /* returns a long long number, with specific width */
#endif

/* xHarbour compatible function */
#define hb_retcAdopt( szText )               hb_retc_buffer( (szText) )
#define hb_retclenAdopt( szText, ulLen )     hb_retclen_buffer( (szText), (ulLen) )
#define hb_retclenAdoptRaw( szText, ulLen )  hb_retclen_buffer( (szText), (ulLen) )
#define hb_retcStatic( szText )              hb_retc_const( (szText) )

#ifdef HB_API_MACROS

#define hb_pcount()                          ( ( int ) ( hb_stackBaseItem() )->item.asSymbol.paramcnt )

#define hb_ret()                             hb_itemClear( hb_stackReturnItem() )
#define hb_reta( ulLen )                     hb_arrayNew( hb_stackReturnItem(), ulLen )
#define hb_retc( szText )                    hb_itemPutC( hb_stackReturnItem(), szText )
#define hb_retc_buffer( szText )             hb_itemPutCPtr( hb_stackReturnItem(), szText, strlen( szText ) )
#define hb_retc_const( szText )              hb_itemPutCConst( hb_stackReturnItem(), szText )
#define hb_retclen( szText, ulLen )          hb_itemPutCL( hb_stackReturnItem(), szText, ulLen )
#define hb_retclen_buffer( szText, ulLen )   hb_itemPutCPtr( hb_stackReturnItem(), szText, ulLen )
#define hb_retds( szDate )                   hb_itemPutDS( hb_stackReturnItem(), szDate )
#define hb_retd( iYear, iMonth, iDay )       hb_itemPutD( hb_stackReturnItem(), iYear, iMonth, iDay )
#define hb_retdl( lJulian )                  hb_itemPutDL( hb_stackReturnItem(), lJulian )
#define hb_retl( iLogical )                  hb_itemPutL( hb_stackReturnItem(), (iLogical) ? TRUE : FALSE )
#define hb_retnd( dNumber )                  hb_itemPutND( hb_stackReturnItem(), dNumber )
#define hb_retni( iNumber )                  hb_itemPutNI( hb_stackReturnItem(), iNumber )
#define hb_retnl( lNumber )                  hb_itemPutNL( hb_stackReturnItem(), lNumber )
#define hb_retnll( lNumber )                 hb_itemPutNLL( hb_stackReturnItem(), lNumber )
#define hb_retnlen( dNumber, iWidth, iDec )  hb_itemPutNLen( hb_stackReturnItem(), dNumber, iWidth, iDec )
#define hb_retndlen( dNumber, iWidth, iDec ) hb_itemPutNDLen( hb_stackReturnItem(), dNumber, iWidth, iDec )
#define hb_retnilen( iNumber, iWidth )       hb_itemPutNILen( hb_stackReturnItem(), iNumber, iWidth )
#define hb_retnllen( lNumber, iWidth )       hb_itemPutNLLen( hb_stackReturnItem(), lNumber, iWidth )
#define hb_retnlllen( lNumber, iWidth )      hb_itemPutNLLLen( hb_stackReturnItem(), lNumber, iWidth )
#define hb_retnint( iNumber )                hb_itemPutNInt( hb_stackReturnItem(), iNumber )
#define hb_retnintlen( lNumber, iWidth )     hb_itemPutNIntLen( hb_stackReturnItem(), lNumber, iWidth )
#define hb_retptr( pointer )                 hb_itemPutPtr( hb_stackReturnItem(), pointer )
#define hb_retptrGC( pointer )               hb_itemPutPtrGC( hb_stackReturnItem(), pointer )

#endif /* HB_API_MACROS */


extern HB_EXPORT int    hb_storc( char * szText, int iParam, ... ); /* stores a szString on a variable by reference */
extern HB_EXPORT int    hb_storclen( char * szText, ULONG ulLength, int iParam, ... ); /* stores a fixed length string on a variable by reference */
extern HB_EXPORT int    hb_stords( char * szDate, int iParam, ... );   /* szDate must have yyyymmdd format */
extern HB_EXPORT int    hb_storl( int iLogical, int iParam, ... ); /* stores a logical integer on a variable by reference */
extern HB_EXPORT int    hb_storni( int iValue, int iParam, ... ); /* stores an integer on a variable by reference */
extern HB_EXPORT int    hb_stornl( long lValue, int iParam, ... ); /* stores a long on a variable by reference */
extern HB_EXPORT int    hb_stornd( double dValue, int iParam, ... ); /* stores a double on a variable by reference */
extern HB_EXPORT int    hb_stornint( HB_LONG lValue, int iParam, ... ); /* stores a HB_LONG on a variable by reference */
extern HB_EXPORT int    hb_storptr( void * pointer, int iParam, ... ); /* stores a pointer on a variable by reference */
#ifndef HB_LONG_LONG_OFF
extern HB_EXPORT int    hb_stornll( LONGLONG lValue, int iParam, ... ); /* stores a long long on a variable by reference */
#endif

extern HB_EXPORT void   hb_xinit( void );                           /* Initialize fixed memory subsystem */
extern HB_EXPORT void   hb_xexit( void );                           /* Deinitialize fixed memory subsystem */
extern HB_EXPORT void * hb_xalloc( ULONG ulSize );                  /* allocates memory, returns NULL on failure */
extern HB_EXPORT void * hb_xgrab( ULONG ulSize );                   /* allocates memory, exits on failure */
extern HB_EXPORT void   hb_xfree( void * pMem );                    /* frees memory */
extern HB_EXPORT void * hb_xrealloc( void * pMem, ULONG ulSize );   /* reallocates memory */
extern HB_EXPORT ULONG  hb_xsize( void * pMem );                    /* returns the size of an allocated memory block */
extern HB_EXPORT ULONG  hb_xquery( USHORT uiMode );                 /* Query different types of memory information */

#ifdef _HB_API_INTERNAL_
extern void       hb_xRefInc( void * pMem );    /* increment reference counter */
extern BOOL       hb_xRefDec( void * pMem );    /* decrement reference counter, return TRUE when 0 reached */
extern void       hb_xRefFree( void * pMem );   /* decrement reference counter and free the block when 0 reached */
extern HB_COUNTER hb_xRefCount( void * pMem );  /* return number of references */
extern void *     hb_xRefResize( void * pMem, ULONG ulSave, ULONG ulSize );   /* reallocates memory, create copy if reference counter greater then 1 */

#if 0

/*
 * I used this macros only to test some speed overhead,
 * They may not be supported in the future so please do
 * not create any code which needs them. [druzus]
 */

#define hb_xRefInc( p )             (++(*HB_COUNTER_PTR( p )))
#define hb_xRefDec( p )             (--(*HB_COUNTER_PTR( p ))==0)
#define hb_xRefFree( p )            do { \
                                       if( hb_xRefDec( p ) ) \
                                          hb_xfree( p ); \
                                    } while( 0 )
#define hb_xRefCount( p )           (*HB_COUNTER_PTR( p ))

#endif

#endif /* _HB_API_INTERNAL_ */

/* #if UINT_MAX == ULONG_MAX */
/* it fails on 64bit platforms where int has 32 bit and long has 64 bit.
   we need these functions only when max(size_t) < max(long)
   and only on 16bit platforms, so the below condition seems to be
   more reasonable. */
#if UINT_MAX > USHRT_MAX
   /* NOTE: memcpy/memset can work with ULONG data blocks */
   #define  hb_xmemcpy  memcpy
   #define  hb_xmemset  memset
#else
   /* NOTE: otherwise, the hb_xmemcpy and hb_xmemset functions
            will be used to copy and/or set ULONG data blocks */
extern HB_EXPORT void * hb_xmemcpy( void * pDestArg, void * pSourceArg, ULONG ulLen ); /* copy more than memcpy() can */
extern HB_EXPORT void * hb_xmemset( void * pDestArg, int iFill, ULONG ulLen ); /* set more than memset() can */
#endif

/* garbage collector */
#define HB_GARBAGE_FUNC( hbfunc )   void hbfunc( void * Cargo ) /* callback function for cleaning garbage memory pointer */
typedef HB_GARBAGE_FUNC( HB_GARBAGE_FUNC_ );
typedef HB_GARBAGE_FUNC_ * HB_GARBAGE_FUNC_PTR;

#define HB_GARBAGE_SWEEPER( hbfunc )   BOOL hbfunc( void * Cargo ) /* callback function for cleaning garbage memory pointer */
typedef HB_GARBAGE_SWEEPER( HB_GARBAGE_SWEEPER_ );
typedef HB_GARBAGE_SWEEPER_ * HB_GARBAGE_SWEEPER_PTR;

extern void  hb_gcRegisterSweep( HB_GARBAGE_SWEEPER_PTR pSweep, void * Cargo );

extern PHB_ITEM   hb_gcGripGet( HB_ITEM_PTR pItem );
extern void       hb_gcGripDrop( HB_ITEM_PTR pItem );

extern HB_EXPORT void *hb_gcAlloc( ULONG ulSize, HB_GARBAGE_FUNC_PTR pFunc ); /* allocates a memory controlled by the garbage collector */
extern void       hb_gcFree( void *pAlloc ); /* deallocates a memory allocated by the garbage collector */
extern void *     hb_gcLock( void *pAlloc ); /* do not release passed memory block */
extern void *     hb_gcUnlock( void *pAlloc ); /* passed block is allowed to be released */
#ifdef _HB_API_INTERNAL_
extern void       hb_gcItemRef( HB_ITEM_PTR pItem ); /* checks if passed item refers passed memory block pointer */
extern void       hb_vmIsLocalRef( void ); /* hvm.c - mark all local variables as used */
extern void       hb_vmIsStaticRef( void ); /* hvm.c - mark all static variables as used */
extern void       hb_memvarsIsMemvarRef( void ); /* memvars.c - mark all memvar variables as used */
extern void       hb_gcReleaseAll( void ); /* release all memory blocks unconditionally */

extern void       hb_gcRefCheck( void * pBlock ); /* Check if block still cannot be access after destructor execution */
extern void       hb_gcRefInc( void * pAlloc );  /* increment reference counter */
extern BOOL       hb_gcRefDec( void * pAlloc );  /* decrement reference counter, return TRUE when 0 reached */
extern void       hb_gcRefFree( void * pAlloc ); /* decrement reference counter and free the block when 0 reached */
extern HB_COUNTER hb_gcRefCount( void * pAlloc );  /* return number of references */

#if 0
#define hb_gcRefInc( p )      hb_xRefInc( HB_GC_PTR( p ) )
#define hb_gcRefDec( p )      hb_xRefDec( HB_GC_PTR( p ) )
#define hb_gcRefCount( p )    hb_xRefCount( HB_GC_PTR( p ) )
#endif

#endif /* _HB_API_INTERNAL_ */
extern void       hb_gcCollect( void ); /* checks if a single memory block can be released */
extern void       hb_gcCollectAll( void ); /* checks if all memory blocks can be released */

/* array management */
extern HB_EXPORT BOOL       hb_arrayNew( PHB_ITEM pItem, ULONG ulLen ); /* creates a new array */
extern HB_EXPORT ULONG      hb_arrayLen( PHB_ITEM pArray ); /* retrieves the array len */
extern HB_EXPORT BOOL       hb_arrayIsObject( PHB_ITEM pArray ); /* retrieves if the array is an object */
extern HB_EXPORT void *     hb_arrayId( PHB_ITEM pArray ); /* retrieves the array unique ID */
extern HB_EXPORT BOOL       hb_arrayAdd( PHB_ITEM pArray, PHB_ITEM pItemValue ); /* add a new item to the end of an array item */
extern HB_EXPORT BOOL       hb_arrayAddForward( PHB_ITEM pArray, PHB_ITEM pValue ); /* add a new item to the end of an array item with no incrementing of reference counters */
extern HB_EXPORT BOOL       hb_arrayIns( PHB_ITEM pArray, ULONG ulIndex ); /* insert a nil item into an array, without changing the length */
extern HB_EXPORT BOOL       hb_arrayDel( PHB_ITEM pArray, ULONG ulIndex ); /* delete an array item, without changing length */
extern HB_EXPORT BOOL       hb_arraySize( PHB_ITEM pArray, ULONG ulLen ); /* sets the array total length */
extern HB_EXPORT BOOL       hb_arrayLast( PHB_ITEM pArray, PHB_ITEM pResult ); /* retrieve last item in an array */
extern HB_EXPORT BOOL       hb_arraySet( PHB_ITEM pArray, ULONG ulIndex, PHB_ITEM pItem ); /* sets an array element */
extern HB_EXPORT BOOL       hb_arrayGet( PHB_ITEM pArray, ULONG ulIndex, PHB_ITEM pItem ); /* retrieves an item */
extern HB_EXPORT BOOL       hb_arrayGetItemRef( PHB_ITEM pArray, ULONG ulIndex, PHB_ITEM pItem ); /* create a reference to an array element */
/* hb_arrayGetItemPtr() is dangerous */
extern HB_EXPORT PHB_ITEM   hb_arrayGetItemPtr( PHB_ITEM pArray, ULONG ulIndex ); /* returns pointer to specified element of the array */
extern HB_EXPORT ULONG      hb_arrayCopyC( PHB_ITEM pArray, ULONG ulIndex, char * szBuffer, ULONG ulLen ); /* copy a string into an array item */
extern HB_EXPORT char *     hb_arrayGetC( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the string contained on an array element */
extern HB_EXPORT char *     hb_arrayGetCPtr( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the string pointer on an array element */
extern HB_EXPORT ULONG      hb_arrayGetCLen( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the string length contained on an array element */
extern HB_EXPORT void *     hb_arrayGetPtr( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the pointer contained on an array element */
extern HB_EXPORT BOOL       hb_arrayGetL( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the logical value contained on an array element */
extern HB_EXPORT int        hb_arrayGetNI( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the int value contained on an array element */
extern HB_EXPORT long       hb_arrayGetNL( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the long numeric value contained on an array element */
extern HB_EXPORT HB_LONG    hb_arrayGetNInt( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the HB_LONG value contained on an array element */
extern HB_EXPORT double     hb_arrayGetND( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the double value contained on an array element */
extern HB_EXPORT char *     hb_arrayGetDS( PHB_ITEM pArray, ULONG ulIndex, char * szDate ); /* retrieves the date value contained in an array element */
extern HB_EXPORT long       hb_arrayGetDL( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the date value contained in an array element, as a long integer */
extern HB_EXPORT HB_TYPE    hb_arrayGetType( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the type of an array item */
extern HB_EXPORT BOOL       hb_arrayFill( PHB_ITEM pArray, PHB_ITEM pValue, ULONG * pulStart, ULONG * pulCount ); /* fill an array with a given item */
extern HB_EXPORT ULONG      hb_arrayScan( PHB_ITEM pArray, PHB_ITEM pValue, ULONG * pulStart, ULONG * pulCount ); /* scan an array for a given item, or until code-block item returns TRUE */
extern HB_EXPORT BOOL       hb_arrayEval( PHB_ITEM pArray, PHB_ITEM bBlock, ULONG * pulStart, ULONG * pulCount ); /* execute a code-block for every element of an array item */
extern HB_EXPORT BOOL       hb_arrayCopy( PHB_ITEM pSrcArray, PHB_ITEM pDstArray, ULONG * pulStart, ULONG * pulCount, ULONG * pulTarget ); /* copy items from one array to another */
extern HB_EXPORT PHB_ITEM   hb_arrayClone( PHB_ITEM pArray ); /* returns a duplicate of an existing array, including all nested items */
extern HB_EXPORT BOOL       hb_arraySort( PHB_ITEM pArray, ULONG * pulStart, ULONG * pulCount, PHB_ITEM pBlock ); /* sorts an array item */
extern HB_EXPORT PHB_ITEM   hb_arrayFromStack( USHORT uiLen ); /* Creates and returns an Array of n Elements from the Eval Stack - Does NOT pop the items. */
extern HB_EXPORT PHB_ITEM   hb_arrayFromParams( int iLevel ); /* Creates and returns an Array of Generic Parameters for a given call level */
extern HB_EXPORT PHB_ITEM   hb_arrayBaseParams( void ); /* Creates and returns an Array of Generic Parameters for current base symbol. */
extern HB_EXPORT PHB_ITEM   hb_arraySelfParams( void ); /* Creates and returns an Array of Generic Parameters for current base symbol with self item */
#ifndef HB_LONG_LONG_OFF
extern HB_EXPORT LONGLONG   hb_arrayGetNLL( PHB_ITEM pArray, ULONG ulIndex ); /* retrieves the long long numeric value contained on an array element */
#endif
#ifdef _HB_API_INTERNAL_
/* internal array API not exported */
extern void hb_arrayPushBase( PHB_BASEARRAY pBaseArray );
#endif

/* string management */

#define HB_ISSPACE( c ) ( ( c ) == ' ' || \
                          ( c ) == HB_CHAR_HT || \
                          ( c ) == HB_CHAR_LF || \
                          ( c ) == HB_CHAR_CR )

extern HB_EXPORT int      hb_stricmp( const char * s1, const char * s2 ); /* compare two strings without regards to case */
extern HB_EXPORT int      hb_strnicmp( const char * s1, const char * s2, ULONG ulLen ); /* compare two string without regards to case, limited by length */
extern HB_EXPORT char *   hb_strupr( char * pszText ); /* convert a string in-place to upper-case */
extern HB_EXPORT char *   hb_strdup( const char * pszText ); /* returns a pointer to a newly allocated copy of the source string */
extern HB_EXPORT char *   hb_strndup( const char * pszText, ULONG ulLen ); /* returns a pointer to a newly allocated copy of the source string not longer then ulLen */
extern HB_EXPORT ULONG    hb_strnlen( const char * pszText, ULONG ulLen ); /* like strlen() but result is limited to ulLen */
extern HB_EXPORT char *   hb_xstrcat( char *dest, const char *src, ... ); /* Concatenates multiple strings into a single result */
extern HB_EXPORT char *   hb_xstrcpy( char *szDest, const char *szSrc, ...); /* Concatenates multiple strings into a single result */
extern HB_EXPORT BOOL     hb_compStrToNum( const char* szNum, ULONG ulLen, HB_LONG * plVal, double * pdVal, int * piDec, int * piWidth );  /* converts string to number, sets iDec, iWidth and returns TRUE if results is double, used by compiler */
extern HB_EXPORT BOOL     hb_valStrnToNum( const char* szNum, ULONG ulLen, HB_LONG * plVal, double * pdVal, int * piDec, int * piWidth );  /* converts string to number, sets iDec, iWidth and returns TRUE if results is double, used by VAL() */
extern HB_EXPORT BOOL     hb_strToNum( const char* szNum, HB_LONG * plVal, double * pdVal ); /* converts string to number, returns TRUE if results is double */
extern HB_EXPORT BOOL     hb_strnToNum( const char* szNum, ULONG ulLen, HB_LONG * plVal, double * pdVal ); /* converts string to number, returns TRUE if results is double */

extern HB_EXPORT BOOL     hb_strMatchRegExp( const char * szString, const char * szMask ); /* compare two strings using a regular expression pattern */
extern HB_EXPORT BOOL     hb_strMatchWild(const char *szString, const char *szPattern ); /* compare two strings using pattern with wildcard (?*) - patern have to be prefix of given string */
extern HB_EXPORT BOOL     hb_strMatchWildExact( const char *szString, const char *szPattern ); /* compare two strings using pattern with wildcard (?*) - patern have to cover whole string */
extern HB_EXPORT BOOL     hb_strEmpty( const char * szText, ULONG ulLen ); /* returns whether a string contains only white space */
extern HB_EXPORT void     hb_strDescend( char * szStringTo, const char * szStringFrom, ULONG ulLen ); /* copy a string to a buffer, inverting each character */
extern HB_EXPORT ULONG    hb_strAt( const char * szSub, ULONG ulSubLen, const char * szText, ULONG ulLen ); /* returns an index to a sub-string within another string */
extern HB_EXPORT char *   hb_strUpper( char * szText, ULONG ulLen ); /* convert an existing string buffer to upper case */
extern HB_EXPORT char *   hb_strUpperCopy( char * szText, ULONG ulLen );
extern HB_EXPORT char *   hb_strLower( char * szText, ULONG ulLen ); /* convert an existing string buffer to lower case */
extern HB_EXPORT char *   hb_strncpy( char * pDest, const char * pSource, ULONG ulLen ); /* copy at most ulLen bytes from string buffer to another buffer and _always_ set 0 in destin buffer */
extern HB_EXPORT char *   hb_strncat( char * pDest, const char * pSource, ULONG ulLen ); /* copy at most ulLen-strlen(pDest) bytes from string buffer to another buffer and _always_ set 0 in destin buffer */
extern HB_EXPORT char *   hb_strncpyTrim( char * pDest, const char * pSource, ULONG ulLen );
extern HB_EXPORT char *   hb_strncpyUpper( char * pDest, const char * pSource, ULONG ulLen ); /* copy an existing string buffer to another buffer, as upper case */
extern HB_EXPORT char *   hb_strncpyUpperTrim( char * pDest, const char * pSource, ULONG ulLen );
extern HB_EXPORT double   hb_strVal( const char * szText, ULONG ulLen ); /* return the numeric value of a character string representation of a number */
extern HB_EXPORT char *   hb_strLTrim( const char * szText, ULONG * ulLen ); /* return a pointer to the first non-white space character */
extern HB_EXPORT ULONG    hb_strRTrimLen( const char * szText, ULONG ulLen, BOOL bAnySpace ); /* return length of a string, ignoring trailing white space (or true spaces) */
extern HB_EXPORT double   hb_strVal( const char * szText, ULONG ulLen );
extern HB_EXPORT HB_LONG  hb_strValInt( const char * szText, int * iOverflow );
extern HB_EXPORT char *   hb_strRemEscSeq( char * szText, ULONG * ulLen ); /* remove C ESC sequences and converts them to Clipper chars */

extern HB_EXPORT double   hb_numRound( double dResult, int iDec ); /* round a number to a specific number of digits */
extern HB_EXPORT double   hb_numInt( double dNum ); /* take the integer part of the number */
extern HB_EXPORT double   hb_numDecConv( double dNum, int iDec );


/* architecture dependent number conversions */
extern HB_EXPORT void     hb_put_ieee754( BYTE * ptr, double d );
extern HB_EXPORT double   hb_get_ieee754( BYTE * ptr );
extern HB_EXPORT void     hb_put_ord_ieee754( BYTE * ptr, double d );
extern HB_EXPORT double   hb_get_ord_ieee754( BYTE * ptr );
extern HB_EXPORT double   hb_get_rev_double( BYTE * ptr );
extern HB_EXPORT double   hb_get_std_double( BYTE * ptr );

#if defined( HB_LONG_LONG_OFF )
extern HB_EXPORT double   hb_get_le_int64( BYTE * ptr );
extern HB_EXPORT double   hb_get_le_uint64( BYTE * ptr );
extern HB_EXPORT void     hb_put_le_uint64( BYTE * ptr, double d );
#endif

/* dynamic symbol table management */
extern HB_EXPORT PHB_DYNS  hb_dynsymGet( char * szName );    /* finds and creates a dynamic symbol if not found */
extern HB_EXPORT PHB_DYNS  hb_dynsymGetCase( char * szName );    /* finds and creates a dynamic symbol if not found - case sensitive */
extern HB_EXPORT PHB_DYNS  hb_dynsymNew( PHB_SYMB pSymbol ); /* creates a new dynamic symbol based on a local one */
extern HB_EXPORT PHB_DYNS  hb_dynsymFind( char * szName );   /* finds a dynamic symbol */
extern HB_EXPORT PHB_DYNS  hb_dynsymFindName( char * szName ); /* converts to uppercase and finds a dynamic symbol */
extern HB_EXPORT void      hb_dynsymLog( void );             /* displays all dynamic symbols */
extern HB_EXPORT void      hb_dynsymRelease( void );         /* releases the memory of the dynamic symbol table */
extern HB_EXPORT void      hb_dynsymEval( PHB_DYNS_FUNC pFunction, void * Cargo ); /* enumerates all dynamic symbols */
extern HB_EXPORT PHB_SYMB  hb_dynsymGetSymbol( char * szName ); /* finds and creates a dynamic symbol if not found and return pointer to its HB_SYMB structure */
extern HB_EXPORT PHB_SYMB  hb_dynsymFindSymbol( char * szName ); /* finds a dynamic symbol and return pointer to its HB_SYMB structure */
extern HB_EXPORT PHB_SYMB  hb_dynsymSymbol( PHB_DYNS pDynSym );
extern HB_EXPORT char *    hb_dynsymName( PHB_DYNS pDynSym ); /* return dynamic symbol name */
extern HB_EXPORT HB_HANDLE hb_dynsymMemvarHandle( PHB_DYNS pDynSym ); /* return memvar handle number bound with given dynamic symbol */
extern HB_EXPORT int       hb_dynsymAreaHandle( PHB_DYNS pDynSym ); /* return work area number bound with given dynamic symbol */
extern HB_EXPORT void      hb_dynsymSetAreaHandle( PHB_DYNS pDynSym, int iArea ); /* set work area number for a given dynamic symbol */

/* Symbol management */
extern HB_EXPORT PHB_SYMB hb_symbolNew( char * szName ); /* create a new symbol */

/* Command line and environment argument management */
extern HB_EXPORT void hb_cmdargInit( int argc, char * argv[] ); /* initialize command line argument API's */
extern int       hb_cmdargARGC( void ); /* retrieve command line argument count */
extern char **   hb_cmdargARGV( void ); /* retrieve command line argument buffer pointer */
extern BOOL      hb_cmdargIsInternal( const char * szArg ); /* determine if a string is an internal setting */
extern BOOL      hb_cmdargCheck( const char * pszName ); /* Check if a given internal switch (like //INFO) was set */
extern char *    hb_cmdargString( const char * pszName ); /* Returns the string value of an internal switch (like //TEMPPATH:"C:\") */
extern int       hb_cmdargNum( const char * pszName ); /* Returns the numeric value of an internal switch (like //F:90) */
extern ULONG     hb_cmdargProcessVM( int*, int* ); /* Check for command line internal arguments */
#if defined( HB_OS_WIN_32 ) && defined( HB_OS_WIN_32_USED )
extern HB_EXPORT void hb_winmainArgInit( HANDLE hInstance, HANDLE hPrevInstance, int iCmdShow ); /* Set WinMain() parameters */
extern HB_EXPORT BOOL hb_winmainArgGet( HANDLE * phInstance, HANDLE * phPrevInstance, int * piCmdShow ); /* Retrieve WinMain() parameters */
#endif

/* Codeblock management */
extern HB_EXPORT void * hb_codeblockId( PHB_ITEM pItem ); /* retrieves the codeblock unique ID */
extern HB_CODEBLOCK_PTR hb_codeblockNew( const BYTE * pBuffer, USHORT uiLocals, const BYTE * pLocalPosTable, PHB_SYMB pSymbols, ULONG ulLen ); /* create a code-block */
extern HB_CODEBLOCK_PTR hb_codeblockMacroNew( BYTE * pBuffer, ULONG ulLen );
extern PHB_ITEM         hb_codeblockGetVar( PHB_ITEM pItem, LONG iItemPos ); /* get local variable referenced in a codeblock */
extern PHB_ITEM         hb_codeblockGetRef( HB_CODEBLOCK_PTR pCBlock, LONG iItemPos ); /* get local variable passed by reference */
extern void             hb_codeblockEvaluate( HB_ITEM_PTR pItem ); /* evaluate a codeblock */

/* memvars subsystem */
extern void       hb_memvarsInit( void ); /* initialize the memvar API system */
extern void       hb_memvarsClear( void ); /* clear all PUBLIC and PRIVATE variables */
extern void       hb_memvarsFree( void ); /* release the memvar API system */
extern void       hb_memvarValueIncRef( HB_HANDLE hValue ); /* increase the reference count of a global value */
extern void       hb_memvarValueDecRef( HB_HANDLE hValue ); /* decrease the reference count of a global value */
extern void       hb_memvarSetValue( PHB_SYMB pMemvarSymb, HB_ITEM_PTR pItem ); /* copy an item into a symbol */
extern ERRCODE    hb_memvarGet( HB_ITEM_PTR pItem, PHB_SYMB pMemvarSymb ); /* copy an symbol value into an item */
extern void       hb_memvarGetValue( HB_ITEM_PTR pItem, PHB_SYMB pMemvarSymb ); /* copy an symbol value into an item, with error trapping */
extern void       hb_memvarGetRefer( HB_ITEM_PTR pItem, PHB_SYMB pMemvarSymb ); /* copy a reference to a symbol value into an item, with error trapping */
extern ULONG      hb_memvarGetPrivatesBase( void ); /* retrieve current PRIVATE variables stack base */
extern void       hb_memvarSetPrivatesBase( ULONG ulBase ); /* release PRIVATE variables created after specified base */
extern void       hb_memvarUpdatePrivatesBase( void ); /* Update PRIVATE base ofsset so they will not be removed when function return */
extern void       hb_memvarNewParameter( PHB_SYMB pSymbol, PHB_ITEM pValue );
extern char *     hb_memvarGetStrValuePtr( char * szVarName, ULONG *pulLen );
extern void       hb_memvarCreateFromItem( PHB_ITEM pMemvar, BYTE bScope, PHB_ITEM pValue );
extern int        hb_memvarScope( char * szVarName, ULONG ulLength ); /* retrieve scope of a dynamic variable symbol */
extern PHB_ITEM   hb_memvarDetachLocal( HB_ITEM_PTR pLocal ); /* Detach a local variable from the eval stack */
#ifdef _HB_API_INTERNAL_
extern PHB_ITEM   hb_memvarGetItem( PHB_SYMB pMemvarSymb );
#endif

/* console I/O subsystem */
extern void     hb_conInit( void ); /* initialize the console API system */
extern void     hb_conRelease( void ); /* release the console API system */
extern char *   hb_conNewLine( void ); /* retrieve a pointer to a static buffer containing new-line characters */
extern void     hb_conOutStd( const char * pStr, ULONG ulLen ); /* output an string to STDOUT */
extern void     hb_conOutErr( const char * pStr, ULONG ulLen ); /* output an string to STDERR */
extern USHORT   hb_conSetCursor( BOOL bSetCursor, USHORT usNewCursor ); /* retrieve and optionally set cursor shape */
extern char *   hb_conSetColor( const char * szColor ); /* retrieve and optionally set console color */
extern void     hb_conXSaveRestRelease( void ); /* release the save/restore API */

/* compiler and macro compiler */
extern char *   hb_compReservedName( char * szName ); /* determines if a string contains a reserve word */
extern char *   hb_compEncodeString( int iMethod, const char * szText, ULONG * pulLen );
extern char *   hb_compDecodeString( int iMethod, const char * szText, ULONG * pulLen );

/* misc */
extern char *   hb_procname( int iLevel, char * szName, BOOL bskipBlock ); /* retrieve a procedure name into a buffer */
extern BOOL     hb_procinfo( int iLevel, char * szName, USHORT * puiLine, char * szFile );

/* macro compiler */
#if defined( HB_MACRO_SUPPORT )
struct HB_MACRO_;
typedef struct HB_MACRO_ * HB_MACRO_PTR;
#else
typedef void * HB_MACRO_PTR;
#endif
extern void   hb_macroGetValue( HB_ITEM_PTR pItem, BYTE iContext, BYTE flags ); /* retrieve results of a macro expansion */
extern void   hb_macroSetValue( HB_ITEM_PTR pItem, BYTE flags ); /* assign a value to a macro-expression item */
extern void   hb_macroTextValue( HB_ITEM_PTR pItem ); /* macro text substitution */
extern void   hb_macroPushSymbol( HB_ITEM_PTR pItem ); /* handle a macro function calls, e.g. var := &macro() */
extern void   hb_macroRun( HB_MACRO_PTR pMacro ); /* executes pcode compiled by macro compiler */
extern HB_MACRO_PTR hb_macroCompile( char * szString ); /* compile a string and return a pcode buffer */
extern void   hb_macroDelete( HB_MACRO_PTR pMacro ); /* release all memory allocated for macro evaluation */
extern char * hb_macroTextSubst( char * szString, ULONG *pulStringLen ); /* substitute macro variables occurences within a given string */
extern BOOL   hb_macroIsIdent( char * szString ); /* determine if a string is a valid function or variable name */
extern void   hb_macroPopAliasedValue( HB_ITEM_PTR pAlias, HB_ITEM_PTR pVar, BYTE flags ); /* compiles and evaluates an aliased macro expression */
extern void   hb_macroPushAliasedValue( HB_ITEM_PTR pAlias, HB_ITEM_PTR pVar, BYTE flags ); /* compiles and evaluates an aliased macro expression */
extern char * hb_macroGetType( HB_ITEM_PTR pItem ); /* determine the type of an expression */
extern char * hb_macroExpandString( char *szString, ULONG ulLength, BOOL *pbNewString ); /* expands valid '&' operator */

/* idle states */
extern void   hb_releaseCPU( void );
extern void   hb_idleState( void ); /* services a single idle state */
extern void   hb_idleReset( void ); /* services a single idle state */
extern void   hb_idleSleep( double dSeconds ); /* sleep for a given time serving idle task */
extern void   hb_idleShutDown( void ); /* closes all background tasks */

/* misc */
extern char * hb_verPlatform( void ); /* retrieves a newly allocated buffer containing platform version */
extern char * hb_verCompiler( void ); /* retrieves a newly allocated buffer containing compiler version */
extern char * hb_verHarbour( void ); /* retrieves a newly allocated buffer containing harbour version */
extern void   hb_verBuildInfo( void ); /* display harbour, compiler, and platform versions to standard console */
extern HB_EXPORT BOOL   hb_iswinnt(void); /* return .T. if OS == WinNt, 2000, XP */

/* environment variables access */
/* WARNING: This returned pointer must be freed if not NULL using hb_xfree( ( void * ) ptr ); */
extern char * hb_getenv( const char * name );

/* Version tracking related things */
#ifdef HB_FILE_VER_STATIC
   #define HB_FILE_VER( id ) static char s_hb_file_ver[] = id;
#else
   #define HB_FILE_VER( id )
#endif

/* Translation related things */

/* Dummy define for start */
#define HB_I_( x ) x

HB_EXTERN_END

#if defined( HB_MACRO_SUPPORT )
#include "hbcompdf.h"
#endif

#endif /* HB_APIEXT_H_ */
