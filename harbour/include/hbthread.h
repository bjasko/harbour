/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    header file with MT mode functions
 *
 * Copyright 2008 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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

#ifndef HB_THREAD_H_
#define HB_THREAD_H_

#include "hbapi.h"
#include "hbset.h"

#if ( defined( HB_OS_LINUX ) && !defined( __WATCOMC__ ) ) || \
    defined( HB_OS_DARWIN )
#  include <pthread.h>
#  define HB_PTHREAD_API
#elif defined( HB_OS_WIN_32 )
#  include <windows.h>
#  include <process.h>
#endif

HB_EXTERN_BEGIN

#if defined( HB_PTHREAD_API )

   typedef HB_LONG         HB_THREAD_NO;
   typedef pthread_t       HB_THREAD_ID;
   typedef pthread_t       HB_THREAD_HANDLE;
   typedef pthread_mutex_t HB_RAWCRITICAL_T;
   typedef pthread_cond_t  HB_RAWCOND_T;

#  define HB_THREAD_STARTFUNC( func )     void * func( void * Cargo )
#  define HB_THREAD_END                   return NULL;
#  define HB_THREAD_RAWEND                return NULL;

#  define HB_THREAD_SELF()          pthread_self()
#  define HB_THREAD_EQUAL( x, y )   pthread_equal( x, y )

#  define HB_CRITICAL_INIT(v)       pthread_mutex_init( &(v), NULL )
#  define HB_CRITICAL_DESTROY(v)    pthread_mutex_destroy( &(v) )
#  define HB_CRITICAL_LOCK(v)       pthread_mutex_lock( &(v) )
#  define HB_CRITICAL_UNLOCK(v)     pthread_mutex_unlock( &(v) )
#  define HB_COND_INIT(v)           pthread_cond_init( &(v), NULL )
#  define HB_COND_DESTROY(v)        pthread_cond_destroy( &(v) )
#  define HB_COND_SIGNAL(v)         pthread_cond_signal( &(v) )
#  define HB_COND_SIGNALN(v,n)      pthread_cond_broadcast( &(v) )

#  define HB_COND_OS_SUPPORT        /* OS support for conditional variables */

#  if defined( PTHREAD_MUTEX_INITIALIZER ) && !defined( HB_CRITICAL_NEED_INIT )
      typedef pthread_mutex_t          HB_CRITICAL_T;
#     define HB_CRITICAL_NEW( name )   HB_CRITICAL_T name = PTHREAD_MUTEX_INITIALIZER
#     define HB_CRITICAL_GET(v)        ( v )
#  else
      /* platform does not support static mutex initialization */
#     if !defined( HB_CRITICAL_NEED_INIT )
#        define HB_CRITICAL_NEED_INIT
#     endif
#     define HB_CRITICAL_GET(v)        ( &( (v)->critical ) )
#     define HB_CRITICAL_INITVAL       { }
#  endif

#  if defined( PTHREAD_COND_INITIALIZER ) && !defined( HB_COND_NEED_INIT )
      typedef pthread_cond_t           HB_COND_T;
#     define HB_COND_NEW( name )       HB_COND_T name = PTHREAD_COND_INITIALIZER
#     define HB_COND_GET(v)            ( v )
#  else
      /* platform does not support static condition var initialization */
#     if !defined( HB_COND_NEED_INIT )
#        define HB_COND_NEED_INIT
#     endif
#     define HB_COND_GET(v)            ( &( (v)->cond ) )
#     define HB_COND_INITVAL           { }
#  endif

#elif defined( HB_OS_WIN_32 ) && ! defined( HB_WINCE )

# define HB_MAX_THREAD  32768

   typedef HB_LONG            HB_THREAD_NO;
   typedef unsigned           HB_THREAD_ID;
   typedef HANDLE             HB_THREAD_HANDLE;
   typedef CRITICAL_SECTION   HB_RAWCRITICAL_T;
   typedef HANDLE             HB_RAWCOND_T;

#  define HB_THREAD_STARTFUNC( func )     unsigned __stdcall func( void * Cargo )
#  define HB_THREAD_END                   _endthreadex( 0 ); return 0;
#  define HB_THREAD_RAWEND                return 0;

#  define HB_THREAD_SELF()          GetCurrentThreadId()
#  define HB_CRITICAL_INITVAL       { 0, 0, 0, 0, 0, 0 }
#  define HB_COND_INITVAL           ( ( HANDLE ) NULL )

#  define HB_CRITICAL_INIT(v)       InitializeCriticalSection( &(v) )
#  define HB_CRITICAL_DESTROY(v)    DeleteCriticalSection( &(v) )
#  define HB_CRITICAL_LOCK(v)       EnterCriticalSection( &(v) )
#  define HB_CRITICAL_UNLOCK(v)     LeaveCriticalSection( &(v) )
#  define HB_COND_INIT(v)           do { (v) = CreateSemaphore( NULL, 0, HB_MAX_THREAD, NULL ); } while(0)
#  define HB_COND_DESTROY(v)        CloseHandle( v )
#  define HB_COND_SIGNAL(v)         ReleaseSemaphore( (v), 1, NULL )
#  define HB_COND_SIGNALN(v,n)      ReleaseSemaphore( (v), (n), NULL )
#  define HB_COND_WAIT(v)           ( WaitForSingleObject( (v), INFINITE ) != WAIT_FAILED )
#  define HB_COND_TIMEDWAIT(v,n)    ( WaitForSingleObject( (v), (n) ) != WAIT_FAILED )

#  undef  HB_COND_OS_SUPPORT
#  define HB_CRITICAL_NEED_INIT
#  define HB_COND_NEED_INIT

#  define HB_THREAD_INFINITE_WAIT   INFINITE

#elif defined( HB_OS_OS2 )

   /* In OS2 thread ID is continuous integer number so we can use it directly
    * anyhow I'd prefer to not make such strict binding to OS values because
    * it make cause troubles when code will be ported to other platforms.
    */
   /* typedef TID                HB_THREAD_NO; */
   typedef HB_LONG            HB_THREAD_NO;
   typedef TID                HB_THREAD_ID;
   typedef TID                HB_THREAD_HANDLE;
   typedef HMTX               HB_RAWCRITICAL_T;
   typedef HEV                HB_RAWCOND_T;

#  define HB_THREAD_STARTFUNC( func )     void func( void * Cargo )
#  define HB_THREAD_END                   _endthread(); return;
#  define HB_THREAD_RAWEND                return;

#  define HB_CRITICAL_INITVAL ( ( HMTX ) 0 )
#  define HB_COND_INITVAL     ( ( HEV ) 0 )
#  define HB_THREAD_SELF()    ( ( TID ) _gettid() )

#  define HB_CRITICAL_INIT(v)       DosCreateMutexSem( NULL, &(v), 0L, FALSE )
#  define HB_CRITICAL_DESTROY(v)    DosCloseMutexSem( v )
#  define HB_CRITICAL_LOCK(v)       DosRequestMutexSem( (v), SEM_INDEFINITE_WAIT )
#  define HB_CRITICAL_UNLOCK(v)     DosReleaseMutexSem( v )
#  define HB_COND_INIT(v)           DosCreateEventSem( NULL, &(v), 0L, FALSE );
#  define HB_COND_DESTROY(v)        DosCloseEventSem( v )
#  define HB_COND_SIGNAL(v)         DosPostEventSem( v )
#  define HB_COND_SIGNALN(v,n)      do { int i = (n); while( --i >= 0 ) DosPostEventSem( v ); } while(0)
#  define HB_COND_WAIT(v)           ( DosWaitEventSem( (v), SEM_INDEFINITE_WAIT ) == NO_ERROR )
#  define HB_COND_TIMEDWAIT(v,n)    ( DosWaitEventSem( (v), (n) ) == NO_ERROR )

#  undef  HB_COND_OS_SUPPORT
#  define HB_CRITICAL_NEED_INIT
#  define HB_COND_NEED_INIT

#  define HB_THREAD_INFINITE_WAIT   SEM_INDEFINITE_WAIT

#else

   typedef int HB_THREAD_NO;
   typedef int HB_THREAD_ID;
   typedef int HB_THREAD_HANDLE;
   typedef int HB_CRITICAL_T;
   typedef int HB_RAWCRITICAL_T;
   typedef int HB_COND_T;
   typedef int HB_RAWCOND_T;

#  define HB_THREAD_STARTFUNC( func )     void func( void * Cargo )
#  define HB_THREAD_END                   return;
#  define HB_THREAD_RAWEND                return;

#  define HB_CRITICAL_NEW( name )      HB_CRITICAL_T name = 0
#  define HB_COND_NEW( name )          HB_COND_T name = 0
#  define HB_THREAD_SELF()             (-1)

#  define HB_CRITICAL_LOCK(v)
#  define HB_CRITICAL_UNLOCK(v)
#  define HB_COND_SIGNAL(v)
#  define HB_COND_SIGNALN(v,n)
#  define HB_COND_WAIT(v)           ( FALSE )
#  define HB_COND_TIMEDWAIT(v,n)    ( FALSE )

#endif


#ifdef HB_CRITICAL_NEED_INIT
   typedef struct
   {
      BOOL  fInit;
      HB_RAWCRITICAL_T  critical;
   } HB_CRITICAL_T;
#  define HB_CRITICAL_NEW( name )   HB_CRITICAL_T name = { FALSE, HB_CRITICAL_INITVAL }
#endif /* HB_CRITICAL_NEED_INIT */

#ifdef HB_COND_NEED_INIT
#  if defined( HB_COND_OS_SUPPORT )
      typedef struct
      {
         BOOL     fInit;
         HB_RAWCOND_T   cond;
      } HB_COND_T;
#     define HB_COND_NEW( name )       HB_COND_T name = { FALSE, HB_COND_INITVAL }
#  else
      typedef struct
      {
         BOOL     fInit;
         int      waiters;
         HB_RAWCOND_T      cond;
         HB_RAWCRITICAL_T  critical;
      } HB_COND_T;
#     define HB_COND_NEW( name )       HB_COND_T name = { FALSE, 0, HB_COND_INITVAL, HB_CRITICAL_INITVAL }
#  endif
#endif /* HB_COND_NEED_INIT */

#ifndef HB_THREAD_EQUAL
#  define HB_THREAD_EQUAL( x, y )   ( (x) == (y) )
#endif

#ifndef HB_THREAD_INFINITE_WAIT
#  define HB_THREAD_INFINITE_WAIT   ( ( ULONG ) -1 )
#endif

typedef HB_THREAD_STARTFUNC( PHB_THREAD_STARTFUNC );

typedef struct _HB_THREADSTATE
{
   const char *   pszCDP;
   const char *   pszLang;
   const char *   pszDefRDD;
   PHB_SET_STRUCT pSet;
   void *         pStackId;
   BOOL           fActive;
   BOOL           fFinished;
   PHB_ITEM       pParams;
   PHB_ITEM       pMemvars;
   PHB_ITEM       pResult;
   PHB_ITEM       pThItm;
   HB_THREAD_NO      th_no;
   HB_THREAD_ID      th_id;
   HB_THREAD_HANDLE  th_h;
   struct _HB_THREADSTATE * pPrev;
   struct _HB_THREADSTATE * pNext;
} HB_THREADSTATE, * PHB_THREADSTATE;

extern void hb_threadInit( void );
extern void hb_threadExit( void );

extern PHB_THREADSTATE hb_threadStateNew( void );

/* Critical sections or fast non recursive MUTEXes */
extern void hb_threadEnterCriticalSection( HB_CRITICAL_T * critical );
extern void hb_threadLeaveCriticalSection( HB_CRITICAL_T * critical );

extern BOOL hb_threadCondSignal( HB_COND_T * cond );
extern BOOL hb_threadCondBroadcast( HB_COND_T * cond );
extern BOOL hb_threadCondWait( HB_COND_T * cond, HB_CRITICAL_T * mutex );
extern BOOL hb_threadCondTimedWait( HB_COND_T * cond, HB_CRITICAL_T * mutex, ULONG ulMilliSec );

extern HB_THREAD_HANDLE hb_threadCreate( HB_THREAD_ID * th_id, PHB_THREAD_STARTFUNC start_func, void * Cargo );
extern BOOL        hb_threadJoin( HB_THREAD_HANDLE th_h );
extern BOOL        hb_threadDetach( HB_THREAD_HANDLE th_h );

/* used by .prg code */
extern PHB_ITEM hb_threadMutexCreate( BOOL fSync );
extern BOOL     hb_threadMutexLock( PHB_ITEM pItem );
extern BOOL     hb_threadMutexTimedLock( PHB_ITEM pItem, ULONG ulMilliSec );
extern BOOL     hb_threadMutexUnlock( PHB_ITEM pItem );
extern void     hb_threadMutexNotify( PHB_ITEM pItem, PHB_ITEM pNotifier, BOOL fWaiting );
extern PHB_ITEM hb_threadMutexSubscribe( PHB_ITEM pItem, BOOL fClear );
extern PHB_ITEM hb_threadMutexTimedSubscribe( PHB_ITEM pItem, ULONG ulMilliSec, BOOL fClear );

#if defined( HB_MT_VM ) && defined( _HB_API_INTERNAL_ )

extern void hb_threadMutexUnlockAll( void );

#if defined( HB_NO_TLS )
#  undef HB_USE_TLS
#elif !defined( HB_USE_TLS )
   /* enable native compiler TLS support be default for this compilers
    * which are known that it will work correctly
    */
#  if defined( _MSC_VER ) && !defined( __POCC__ ) && !defined( __XCC__ )
#     define HB_USE_TLS
#  elif defined( __GNUC__ ) && __GNUC__ >= 3 && \
        defined( __GLIBC__ ) && defined( __GLIBC_MINOR__ ) && \
        ( __GLIBC__ > 2 || ( __GLIBC__ == 2 && __GLIBC_MINOR__ >= 6 ) ) && \
        defined( HB_OS_LINUX ) && \
        ( defined( __i386__ ) || defined( __x86_64__ ) )
#     define HB_USE_TLS
#  endif
#endif

#ifdef HB_USE_TLS
#  if ( defined( __GNUC__ ) && __GNUC__ >= 3 ) || defined( __BORLANDC__ )
#     define HB_TLS_ATTR      __thread
#  elif defined( _MSC_VER ) || defined( __WATCOMC__ ) || defined( __DMC__ )
#     define HB_TLS_ATTR      __declspec( thread )
#  else
#     undef HB_USE_TLS
#     error "TLS support undefined for this compiler" /* */
#  endif
#endif

#ifndef HB_USE_TLS
#  if defined( HB_PTHREAD_API )
#     define HB_TLS_KEY       pthread_key_t
#     define hb_tls_init(k)   pthread_key_create( &k, NULL )
#     define hb_tls_set(k,v)  pthread_setspecific( k, ( void * ) (v) )
#     define hb_tls_get(k)    pthread_getspecific( k )
#  elif defined( HB_OS_WIN_32 )
#     define HB_TLS_KEY       DWORD
#     define hb_tls_init(k)   do { k = TlsAlloc(); } while( 0 )
#     define hb_tls_set(k,v)  TlsSetValue( k, ( void * ) (v) )
#     define hb_tls_get(k)    TlsGetValue( k )
#  elif defined( HB_OS_OS2 )
#     define HB_TLS_KEY       PULONG
#     define hb_tls_init(k)   DosAllocThreadLocalMemory( 1, &k )
#     define hb_tls_set(k,v)  do { *k = ( ULONG ) (v); } while( 0 )
#     define hb_tls_get(k)    ( *k )
#  endif
#endif

#endif

HB_EXTERN_END

#endif /* HB_THREAD_H_ */
