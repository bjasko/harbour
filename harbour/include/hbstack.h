/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The eval stack
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
          moved to somewhere else (like hbrtl.h). [vszakats] */

#ifndef HB_STACK_H_
#define HB_STACK_H_

#include "hbvmpub.h"

HB_EXTERN_BEGIN

#if defined( HB_MT_VM )
#  include "hbthread.h"
#  if !defined( HB_USE_TLS ) && !defined( HB_OS_OS2 ) && 0
#     undef HB_STACK_MACROS
#  endif
#endif

/* thread specific data */
typedef void (*PHB_TSD_FUNC)(void *);
typedef struct
{
   int      iHandle;
   int      iSize;
   PHB_TSD_FUNC pInitFunc;
   PHB_TSD_FUNC pCleanFunc;
}
HB_TSD, * PHB_TSD;
#define HB_TSD_NEW(name,size,init,clean)  \
        HB_TSD name = { 0, size, init, clean }
#define HB_TSD_INIT(name,size,init,clean) do { \
            (name)->iHandle = 0; \
            (name)->iSize = (size); \
            (name)->pInitFunc = (init); \
            (name)->pCleanFunc = (clean); \
         } while( 0 )

typedef struct
{
   USHORT   uiFError;
   USHORT   uiErrorLast;
   USHORT   uiOsErrorLast;
}
HB_IOERRORS, * PHB_IOERRORS;

typedef struct
{
   const char *   szDefaultRDD;     /* default RDD */
   BOOL           fNetError;        /* current NETERR() flag */

   void **        waList;           /* Allocated WorkAreas */
   USHORT         uiWaMax;          /* Number of allocated WA */
   USHORT         uiWaSpace;        /* Number of allocated WA */

   USHORT *       waNums;           /* Allocated WorkAreas */
   USHORT         uiWaNumMax;       /* Number of allocated WA */

   USHORT         uiCurrArea;       /* Current WokrArea number */
   void *         pCurrArea;        /* Current WorkArea pointer */
}
HB_STACKRDD, * PHB_STACKRDD;

#ifdef _HB_API_INTERNAL_

#include "hbset.h"

typedef struct
{
   PHB_TSD  pTSD;
   void *   value;
}
HB_TSD_HOLDER, * PHB_TSD_HOLDER;

typedef struct
{
   PHB_DYNS    pDynSym;
   PHB_ITEM    pPrevMemvar;
}
HB_PRIVATE_ITEM, * PHB_PRIVATE_ITEM;

typedef struct
{
   PHB_PRIVATE_ITEM stack;
   ULONG       size;
   ULONG       count;
   ULONG       base;
}
HB_PRIVATE_STACK, * PHB_PRIVATE_STACK;

#if defined( HB_MT_VM )
typedef struct
{
   void *   pMemvar;       /* memvar pointer ( publics & privates ) */
   USHORT   uiArea;        /* Workarea number */
}
HB_DYN_HANDLES, * PHB_DYN_HANDLES;
#endif

/* stack managed by the virtual machine */
typedef struct
{
   PHB_ITEM * pItems;         /* pointer to the stack items */
   PHB_ITEM * pPos;           /* pointer to the latest used item */
   PHB_ITEM * pEnd;           /* pointer to the end of stack items */
   LONG       wItems;         /* total items that may be holded on the stack */
   HB_ITEM    Return;         /* latest returned value */
   PHB_ITEM * pBase;          /* stack frame position for the current function call */
   LONG       lStatics;       /* statics base for the current function call */
   LONG       lWithObject;    /* stack offset to base current WITH OBJECT item */
   LONG       lRecoverBase;   /* current SEQUENCE envelope offset or 0 if no SEQUENCE is active */
   USHORT     uiActionRequest;/* request for some action - stop processing of opcodes */
   USHORT     uiQuitState;    /* HVM is quiting */
   HB_STACK_STATE state;      /* first (default) stack state frame */
   HB_STACKRDD rdd;           /* RDD related data */
   char       szDate[ 9 ];    /* last returned date from _pards() yyyymmdd format */
   void *     pCDP;           /* current codepage module */
   void *     pLang;          /* current language module */
   const char * szDefaultRDD; /* default RDD */
   int        iArea;          /* current workarea number */
   int        iAreaCount;     /* number of allocated workareas number */
   void **    pWorkAreas;     /* workareas pool */
   BOOL       fNetErr;        /* current NETERR() flag */
   int        iTSD;           /* number of allocated TSD holders */
   PHB_TSD_HOLDER pTSD;       /* thread specific data holder */
   HB_PRIVATE_STACK privates; /* private variables stack */
   HB_SET_STRUCT set;
#if defined( HB_MT_VM )
   int        iUnlocked;      /* counter for nested hb_vmUnlock() calls */
   PHB_DYN_HANDLES pDynH;     /* dynamic symbol handles */
   int        iDynH;          /* number of dynamic symbol handles */
   void *     pStackLst;      /* this stack entry in stack linked list */
   HB_IOERRORS IOErrors;      /* MT safe buffer for IO errors */
   BYTE *     byDirBuffer;    /* MT safe buffer for hb_fsCurDir() results */
#endif
} HB_STACK, * PHB_STACK;

#if defined( HB_STACK_MACROS )
#  if defined( HB_MT_VM )
#     if defined( HB_USE_TLS )
#        if defined( __BORLANDC__ )
            extern PHB_STACK HB_TLS_ATTR hb_stack_ptr;
#        else
            extern HB_TLS_ATTR PHB_STACK hb_stack_ptr;
#        endif
#     else
         extern HB_TLS_KEY hb_stack_key;
#        define hb_stack_ptr  ( ( PHB_STACK ) hb_tls_get( hb_stack_key ) )
#     endif
#     if defined( HB_STACK_PRELOAD ) && !defined( HB_USE_TLS )
#        if defined( __BORLANDC__ )
            static __inline void* hb_stack_ptr_from_tls( void )
            {
               /* mov ecx,hb_stack_key */
               _ECX = hb_stack_key;
               /* mov eax,dword ptr fs:[00000018h] */
               __emit__( 0x64, 0xA1, 0x18, 0x00, 0x00, 0x00 );
               /* mov eax,[eax+ecx*4+00000E10h] */
               __emit__( 0x8B, 0x84, 0x88, 0x10, 0x0E, 0x00, 0x00 );
               /* ret (if function is not inlined) */
               return (void*) _EAX;
            }
#           define HB_STACK_TLS_PRELOAD   PHB_STACK _hb_stack_ptr_ = hb_stack_ptr_from_tls();
#        elif defined( __MINGW32__ )
            static __inline__ void * hb_stack_ptr_from_tls( void )
            {
               void * p;
               __asm__ (
                  "movl  %%fs:(0x18), %0\n\t"
                  "movl  0x0e10(%0,%1,4), %0\n\t"
                  :"=a" (p)
                  :"c" (hb_stack_key)
               );
               return p;
            }
#           define HB_STACK_TLS_PRELOAD   PHB_STACK _hb_stack_ptr_ = hb_stack_ptr_from_tls();
#        endif
#        if defined( HB_STACK_TLS_PRELOAD )
#           undef hb_stack_ptr
#        else
#           define HB_STACK_TLS_PRELOAD   PHB_STACK _hb_stack_ptr_ = hb_stack_ptr;
#        endif
#        define hb_stack      ( * _hb_stack_ptr_ )
#     else
#        define hb_stack      ( * hb_stack_ptr )
#     endif
#  else
      extern HB_STACK hb_stack;
#  endif
#endif
#ifndef HB_STACK_TLS_PRELOAD
#  define HB_STACK_TLS_PRELOAD
#endif

#endif /* _HB_API_INTERNAL_ */

extern HB_ITEM_PTR hb_stackItemFromTop( int nFromTop );
extern HB_ITEM_PTR hb_stackItemFromBase( int nFromBase );
extern LONG        hb_stackTopOffset( void );
extern LONG        hb_stackBaseOffset( void );
extern LONG        hb_stackTotalItems( void );
extern HB_ITEM_PTR hb_stackBaseItem( void );
extern HB_ITEM_PTR hb_stackItem( LONG iItemPos );
extern HB_ITEM_PTR hb_stackSelfItem( void );   /* returns Self object at C function level */
extern HB_ITEM_PTR hb_stackReturnItem( void ); /* returns RETURN Item from stack */
extern char *      hb_stackDateBuffer( void );
extern void *      hb_stackId( void );

extern void        hb_stackDec( void );        /* pops an item from the stack without clearing it's contents */
extern void        hb_stackPop( void );        /* pops an item from the stack */
extern void        hb_stackPush( void );       /* pushes an item on to the stack */
extern HB_ITEM_PTR hb_stackAllocItem( void );  /* allocates new item on the top of stack, returns pointer to it */
extern void        hb_stackPushReturn( void );
extern void        hb_stackPopReturn( void );
extern void        hb_stackRemove( LONG lUntilPos );

/* stack management functions */
extern int        hb_stackCallDepth( void );
extern LONG       hb_stackBaseProcOffset( int iLevel );
extern void       hb_stackBaseProcInfo( char * szProcName, USHORT * puiProcLine ); /* get current .prg function name and line number */
extern void       hb_stackDispLocal( void );  /* show the types of the items on the stack for debugging purposes */
extern void       hb_stackDispCall( void );
extern void       hb_stackFree( void );       /* releases all memory used by the stack */
extern void       hb_stackInit( void );       /* initializes the stack */
extern void       hb_stackIncrease( void );   /* increase the stack size */

/* thread specific data */
extern void *     hb_stackGetTSD( PHB_TSD pTSD );
extern void *     hb_stackTestTSD( PHB_TSD pTSD );

extern BYTE *     hb_stackDirBuffer( void );
extern PHB_IOERRORS hb_stackIOErrors( void );
extern PHB_STACKRDD hb_stackRDD( void );

#ifdef _HB_API_INTERNAL_
extern void        hb_stackDecrease( ULONG ulItems );
extern HB_ITEM_PTR hb_stackNewFrame( PHB_STACK_STATE pFrame, USHORT uiParams );
extern void        hb_stackOldFrame( PHB_STACK_STATE pFrame );
extern void        hb_stackClearMevarsBase( void );

extern HB_ITEM_PTR hb_stackLocalVariable( int *piFromBase );
extern PHB_ITEM ** hb_stackItemBasePtr( void );

extern LONG        hb_stackGetRecoverBase( void );
extern void        hb_stackSetRecoverBase( LONG lBase );
extern USHORT      hb_stackGetActionRequest( void );
extern void        hb_stackSetActionRequest( USHORT uiAction );

extern void        hb_stackSetStaticsBase( LONG lBase );
extern LONG        hb_stackGetStaticsBase( void );

extern PHB_ITEM    hb_stackWithObjectItem( void );
extern LONG        hb_stackWithObjectOffset( void );
extern void        hb_stackWithObjectSetOffset( LONG );

extern void        hb_stackDestroyTSD( void );

extern PHB_PRIVATE_STACK hb_stackGetPrivateStack( void );
extern void *      hb_stackGetCDP( void );
extern void        hb_stackSetCDP( void * );
extern void *      hb_stackGetLang( void );
extern void        hb_stackSetLang( void * );

extern void        hb_stackIsStackRef( void *, PHB_TSD_FUNC );

#if defined( HB_MT_VM )
   extern void *           hb_stackList( void );
   extern void             hb_stackListSet( void * pStackLst );
   extern void             hb_stackIdSetActionRequest( void * pStackID, USHORT uiAction );
   extern PHB_DYN_HANDLES  hb_stackGetDynHandle( PHB_DYNS pDynSym );
   extern BOOL             hb_stackQuitState( void );
   extern void             hb_stackSetQuitState( USHORT uiState );
   extern int              hb_stackUnlock( void );
   extern int              hb_stackLock( void );
#endif

#endif /* _HB_API_INTERNAL_ */

#if defined( _HB_API_INTERNAL_ ) || defined( _HB_SET_INTERNAL_ )
   extern PHB_SET_STRUCT hb_stackSetStruct( void );
#endif


#if defined( HB_STACK_MACROS )

#define hb_stackItemFromTop( n )    ( * ( hb_stack.pPos + ( int ) (n) ) )
#define hb_stackItemFromBase( n )   ( * ( hb_stack.pBase + ( int ) (n) + 1 ) )
#define hb_stackTopOffset( )        ( hb_stack.pPos - hb_stack.pItems )
#define hb_stackBaseOffset( )       ( hb_stack.pBase - hb_stack.pItems + 1 )
#define hb_stackTotalItems( )       ( hb_stack.wItems )
#define hb_stackBaseItem( )         ( * hb_stack.pBase )
#define hb_stackSelfItem( )         ( * ( hb_stack.pBase + 1 ) )
#define hb_stackItem( iItemPos )    ( * ( hb_stack.pItems + ( iItemPos ) ) )
#define hb_stackReturnItem( )       ( &hb_stack.Return )
#define hb_stackDateBuffer( )       ( hb_stack.szDate )
#define hb_stackItemBasePtr( )      ( &hb_stack.pItems )
#define hb_stackGetStaticsBase( )   ( hb_stack.lStatics )
#define hb_stackSetStaticsBase( n ) do { hb_stack.lStatics = ( n ); } while ( 0 )
#define hb_stackGetRecoverBase( )   ( hb_stack.lRecoverBase )
#define hb_stackSetRecoverBase( n ) do { hb_stack.lRecoverBase = ( n ); } while( 0 )
#define hb_stackGetActionRequest( ) ( hb_stack.uiActionRequest )
#define hb_stackSetActionRequest( n )     do { hb_stack.uiActionRequest = ( n ); } while( 0 )
#define hb_stackWithObjectItem( )   ( hb_stack.lWithObject ? * ( hb_stack.pItems + hb_stack.lWithObject ) : NULL )
#define hb_stackWithObjectOffset( ) ( hb_stack.lWithObject )
#define hb_stackWithObjectSetOffset( n )  do { hb_stack.lWithObject = ( n ); } while( 0 )
#define hb_stackGetCDP( )           ( hb_stack.pCDP )
#define hb_stackSetCDP( p )         do { hb_stack.pCDP = ( p ); } while ( 0 )
#define hb_stackGetLang( )          ( hb_stack.pLang )
#define hb_stackSetLang( p )        do { hb_stack.pLang = ( p ); } while ( 0 )

#define hb_stackId( )               ( ( void * ) &hb_stack )
#if defined( HB_MT_VM )
#  define hb_stackList( )           ( hb_stack.pStackLst )
#  define hb_stackListSet( p )      do { hb_stack.pStackLst = ( p ); } while ( 0 )
#  define hb_stackQuitState( )      ( hb_stack.uiQuitState != 0 )
#  define hb_stackSetQuitState( n ) do { hb_stack.uiQuitState = ( n ); } while( 0 )
#  define hb_stackUnlock()          ( ++hb_stack.iUnlocked )
#  define hb_stackLock()            ( --hb_stack.iUnlocked )
#endif

#define hb_stackAllocItem( )        ( ( ++hb_stack.pPos == hb_stack.pEnd ? \
                                        hb_stackIncrease() : (void) 0 ), \
                                      * ( hb_stack.pPos - 1 ) )

#ifdef HB_STACK_SAFEMACROS

#define hb_stackDecrease( n )       do { \
                                       if( ( hb_stack.pPos -= (n) ) <= hb_stack.pBase ) \
                                          hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL ); \
                                    } while ( 0 )

#define hb_stackDec( )              do { \
                                       if( --hb_stack.pPos <= hb_stack.pBase ) \
                                          hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL ); \
                                    } while ( 0 )

#define hb_stackPop( )              do { \
                                       if( --hb_stack.pPos <= hb_stack.pBase ) \
                                          hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL ); \
                                       if( HB_IS_COMPLEX( * hb_stack.pPos ) ) \
                                          hb_itemClear( * hb_stack.pPos ); \
                                    } while ( 0 )

#define hb_stackPopReturn( )        do { \
                                       if( HB_IS_COMPLEX( &hb_stack.Return ) ) \
                                          hb_itemClear( &hb_stack.Return ); \
                                       if( --hb_stack.pPos <= hb_stack.pBase ) \
                                          hb_errInternal( HB_EI_STACKUFLOW, NULL, NULL, NULL ); \
                                       hb_itemRawMove( &hb_stack.Return, * hb_stack.pPos ); \
                                    } while ( 0 )

#else

#define hb_stackDecrease( n )       do { hb_stack.pPos -= (n); } while ( 0 )
#define hb_stackDec( )              do { --hb_stack.pPos; } while ( 0 )
#define hb_stackPop( )              do { --hb_stack.pPos; \
                                       if( HB_IS_COMPLEX( * hb_stack.pPos ) ) \
                                          hb_itemClear( * hb_stack.pPos ); \
                                    } while ( 0 )
#define hb_stackPopReturn( )        do { \
                                       if( HB_IS_COMPLEX( &hb_stack.Return ) ) \
                                          hb_itemClear( &hb_stack.Return ); \
                                       --hb_stack.pPos; \
                                       hb_itemRawMove( &hb_stack.Return, * hb_stack.pPos ); \
                                    } while ( 0 )

#endif

#define hb_stackPush( )             do { \
                                       if( ++hb_stack.pPos == hb_stack.pEnd ) \
                                          hb_stackIncrease(); \
                                    } while ( 0 )

#define hb_stackPushReturn( )       do { \
                                       hb_itemRawMove( * hb_stack.pPos, &hb_stack.Return ); \
                                       if( ++hb_stack.pPos == hb_stack.pEnd ) \
                                          hb_stackIncrease(); \
                                    } while ( 0 )

#define hb_stackLocalVariable( p )  ( ( ( ( *hb_stack.pBase )->item.asSymbol.paramcnt > \
                                          ( * hb_stack.pBase )->item.asSymbol.paramdeclcnt ) && \
                                        ( * (p) ) > ( * hb_stack.pBase )->item.asSymbol.paramdeclcnt ) ? \
                                      ( * ( hb_stack.pBase + ( int ) ( * (p) += \
                                          ( * hb_stack.pBase )->item.asSymbol.paramcnt - \
                                          ( * hb_stack.pBase )->item.asSymbol.paramdeclcnt ) + 1 ) ) : \
                                      ( * ( hb_stack.pBase + ( int ) ( * (p) ) + 1 ) ) )

#define hb_stackGetPrivateStack( )  ( &hb_stack.privates )
#define hb_stackSetStruct( )        ( &hb_stack.set )

#endif


HB_EXTERN_END

#endif /* HB_STACK_H_ */
