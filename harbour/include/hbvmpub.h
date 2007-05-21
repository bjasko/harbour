/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for the generated C language source code
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
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

#ifndef HB_VMPUB_H_
#define HB_VMPUB_H_

#include "hbdefs.h"

HB_EXTERN_BEGIN

#ifdef _HB_API_INTERNAL_

struct _HB_SYMB;

#  define HB_ITEM_TYPE( p )   ( ( p )->type )
#  define HB_OBJ_CLASS( p )   ( ( p )->item.asArray.value->uiClass )
#  define HB_ARRAY_OBJ( p )   ( ( p )->item.asArray.value->uiClass != 0 )

#  if defined(__GNUC__)
#     define HB_ITEM_NIL      { HB_IT_NIL, {} }
#  else
#     define HB_ITEM_NIL      { HB_IT_NIL, NULL }
#  endif

#  define HB_ITEM_GET_NUMINTRAW( p )  ( HB_IS_INTEGER( p ) ? \
                                        ( HB_LONG ) p->item.asInteger.value : \
                                        ( HB_LONG ) p->item.asLong.value )

#  define HB_ITEM_PUT_NUMINTRAW( p, v )  \
               do { \
                  if( HB_LIM_INT( v ) ) \
                  { \
                     (p)->type = HB_IT_INTEGER; \
                     (p)->item.asInteger.length = HB_INT_LENGTH( v ); \
                     (p)->item.asInteger.value = ( int ) (v); \
                  } \
                  else \
                  { \
                     (p)->type = HB_IT_LONG; \
                     (p)->item.asLong.value = (v); \
                     (p)->item.asLong.length = HB_LONG_LENGTH( v ); \
                  } \
               } while ( 0 )

   /* dynamic symbol structure */
   typedef struct _HB_DYNS
   {
      struct _HB_SYMB * pSymbol; /* pointer to its relative local symbol */
      HB_HANDLE hMemvar;         /* Index number into memvars ( publics & privates ) array */
      USHORT    uiArea;          /* Workarea number */
      USHORT    uiSymNum;        /* dynamic symbol number */
#ifndef HB_NO_PROFILER
      ULONG     ulCalls;         /* profiler support */
      ULONG     ulTime;          /* profiler support */
      ULONG     ulRecurse;       /* profiler support */
#endif
   } HB_DYNS, * PHB_DYNS, * HB_DYNS_PTR;

   /* pCode dynamic function - HRB */
   typedef struct _HB_PCODEFUNC
   {
      BYTE *      pCode;         /* function body - PCODE */
      struct _HB_SYMB * pSymbols;/* module symbol table */
   } HB_PCODEFUNC, * PHB_PCODEFUNC;

#else

#  undef HB_API_MACROS
#  undef HB_STACK_MACROS

/* This is ugly trick but works without speed overhead */
#  define HB_ITEM_TYPE( p )   ( * ( HB_TYPE * ) ( p ) )

/* if you do not like it then use this definition */
/* #  define HB_ITEM_TYPE( p )   ( hb_itemType( p ) ) */

#  define HB_OBJ_CLASS( p )   ( hb_objGetClass( p ) )
#  define HB_ARRAY_OBJ( p )   ( hb_arrayIsObject( p ) )

   /* basic types */
   typedef void *  PHB_ITEM;
   typedef void *  HB_ITEM_PTR;
   typedef void *  HB_CODEBLOCK_PTR;
   typedef void *  PHB_PCODEFUNC;

   typedef void    HB_STACK;

   /*
    * The first version reduce the number of modification in existing 3-rd
    * party code but in longer terms I'd prefer to disable it and left
    * only the second one where PHB_DYNS is mapped to void*.
    * This will allow us to fully redesign dynamic symbol internals
    * in the future if it will be necessary. [druzus]
    */
#if 0
   struct _HB_SYMB;
   typedef struct
   {
      struct _HB_SYMB * pSymbol; /* pointer to its relative local symbol */
   } _HB_DYNS, * PHB_DYNS, * HB_DYNS_PTR;
#else
   typedef void *  PHB_DYNS;
   typedef void *  PHB_DYNS_PTR;
#endif

#endif


/* symbol support structure */
typedef struct _HB_SYMB
{
   const char *   szName;           /* the name of the symbol */
   union
   {
      HB_SYMBOLSCOPE value;         /* the scope of the symbol */
      void *         pointer;       /* filler to force alignment */
   } scope;
   union
   {
      PHB_FUNC       pFunPtr;       /* machine code function address for function symbol table entries */
      PHB_PCODEFUNC  pCodeFunc;     /* PCODE function address */
      LONG           lStaticsBase;  /* base offset to array of statics */
   } value;
   PHB_DYNS       pDynSym;          /* pointer to its dynamic symbol if defined */
} HB_SYMB, * PHB_SYMB;

#define HB_DYNS_FUNC( hbfunc )   BOOL hbfunc( PHB_DYNS pDynSymbol, void * Cargo )
typedef HB_DYNS_FUNC( PHB_DYNS_FUNC );

typedef void (*HB_INIT_FUNC)(void *);
/* List of functions used by hb_vmAtInit()/hb_vmAtExit() */
typedef struct _HB_FUNC_LIST
{
   HB_INIT_FUNC   pFunc;
   void *         cargo;
   struct _HB_FUNC_LIST * pNext;
} HB_FUNC_LIST, * PHB_FUNC_LIST;

/* Harbour Functions scope ( HB_SYMBOLSCOPE ) */
#define HB_FS_PUBLIC    ( ( HB_SYMBOLSCOPE ) 0x0001 )
#define HB_FS_STATIC    ( ( HB_SYMBOLSCOPE ) 0x0002 )
#define HB_FS_FIRST     ( ( HB_SYMBOLSCOPE ) 0x0004 )
#define HB_FS_INIT      ( ( HB_SYMBOLSCOPE ) 0x0008 )
#define HB_FS_EXIT      ( ( HB_SYMBOLSCOPE ) 0x0010 )
#define HB_FS_MESSAGE   ( ( HB_SYMBOLSCOPE ) 0x0020 )
#define HB_FS_MEMVAR    ( ( HB_SYMBOLSCOPE ) 0x0080 )
#define HB_FS_PCODEFUNC ( ( HB_SYMBOLSCOPE ) 0x0100 )
#define HB_FS_LOCAL     ( ( HB_SYMBOLSCOPE ) 0x0200 )
#define HB_FS_DYNCODE   ( ( HB_SYMBOLSCOPE ) 0x0400 )
#define HB_FS_DEFERRED  ( ( HB_SYMBOLSCOPE ) 0x0800 )

#define HB_FS_INITEXIT ( HB_FS_INIT | HB_FS_EXIT )

extern HB_EXPORT void hb_vmExecute( const BYTE * pCode, PHB_SYMB pSymbols );  /* invokes the virtual machine */

HB_EXTERN_END

#endif /* HB_VMPUB_H_ */
