/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    HVM speed test program
 *
 * Copyright 2008 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
 * www - http://www.harbour-project.org
 *
 */


#define N_TESTS 54
#define N_LOOPS 1000000
#define ARR_LEN 16

#command ? => outstd(EOL)
#command ? <xx,...> => outstd(EOL);outstd(<xx>)

#include "common.ch"

#ifdef __HARBOUR__
    #define EOL hb_OSNewLine()
#else
   #define HB_SYMBOL_UNUSED( symbol )  ( ( symbol ) )
   #ifndef __CLIP__
      #xtranslate secondsCPU() => seconds()
   #endif
   #ifndef EOL
      #define EOL chr(10)
   #endif
#endif

#xcommand _( [<cmds,...>] ) => [<cmds>]

#xcommand TEST <testfunc>           ;
          [ WITH <locals,...> ]     ;
          [ INIT <init> ]           ;
          [ EXIT <exit> ]           ;
          [ INFO <info> ]           ;
          CODE [<*testExp*>] =>     ;
   func <testfunc> ;                ;
      local time, i, x := nil ;     ;
      [ local <locals> ; ]          ;
      [ <init> ; ]                  ;
      time := secondscpu() ;        ;
      for i:=1 to N_LOOPS ;         ;
         [<testExp>;]               ;
      next ;                        ;
      time := secondscpu() - time ; ;
      [ <exit> ; ]                  ;
   return { procname() + ": " + iif( <.info.>, <(info)>, #<testExp> ), time }


proc main( ... )
   local aParams, nMT, cExclude, lScale, cParam, cMemTests, lSyntax, i

   aParams := hb_aparams()
   lSyntax := lScale := .f.
   cMemTests := "029 030 023 025 027 040 041 043 052 053 019 022 031 032 054 "
   cExclude := ""
   nMT := 0
   for each cParam in aParams
      cParam := lower( cParam )
      if cParam = "--thread"
         if substr( cParam, 9, 1 ) == "="
            if isdigit( substr( cParam, 10, 1 ) )
               nMT := val( substr( cParam, 10 ) )
            elseif substr( cParam, 10 ) == "all"
               nMT := -1
            else
               lSyntax = .t.
            endif
         elseif empty( substr( cParam, 9 ) )
            nMT := -1
         else
            lSyntax = .t.
         endif
      elseif cParam = "--exclude="
         if substr( cParam, 11 ) == "mem"
            cExclude += cMemTests
         else
            cExclude += strtran( strtran( strtran( substr( cParam, 11 ), ;
                        ".", " " ), ".", " " ), "/", " " ) + " "
         endif
      elseif cParam = "--only="
         cExclude := ""
         if substr( cParam, 8 ) == "mem"
            cParam := cMemTests
         endif
         for i := 1 to N_TESTS
            if !strzero( i, 3 ) $ cParam
               cExclude += strzero( i, 3 ) + " "
            endif
         next
      elseif cParam = "--scale"
         lScale := .t.
      else
         lSyntax = .t.
      endif
      if lSyntax
         ? "Unknown option:", cParam
         ? "syntax:", hb_argv( 0 ), "[--thread[=<num>]] [--only=<test(s)>] [--exclude=<test(s)>]"
         ?
         return
      endif
   next
   test( nMT, cExclude, lScale )
return


#ifdef __XHARBOUR__

   #xtranslate hb_mtvm()                  => hb_multiThread()
   #xtranslate hb_threadWaitForAll()      => WaitForThreads()
   #xtranslate hb_mutexNotify(<x,...>)    => Notify(<x>)

#ifndef __ST__


   /* do not expect that this code will work with xHarbour.
    * xHarbour has many race conditions which are exploited quite fast
    * on real multi CPU machines so it crashes in different places :-(
    * probably this code should be forwared to xHarbour developers as
    * some type of MT test
    */

   /* this define is only for test if emulation function works
    * without running real test which causes that xHarbour crashes
    */
   //#define _DUMY_XHB_TEST_


   function hb_mutexSubscribe( mtx, nTimeOut, xSubscribed )
      local lSubscribed
      if valtype( nTimeOut ) == "N"
         nTimeOut := round( nTimeOut * 1000, 0 )
         xSubscribed := Subscribe( mtx, nTimeOut, @lSubscribed )
      else
         xSubscribed := Subscribe( mtx )
         lSubscribed := .t.
      endif
   return lSubscribed

   /* in xHarbour there is race condition in JoinThread() which
    * fails if thread end before we call it so we cannot use it :-(
    * this code tries to simulate it and also add support for thread
    * return value
    */

   function hb_threadStart( ... )
      local thId
      thId := StartThread( @threadFirstFunc(), hb_aParams() )
      /* Just like in JoinThread() the same race condition exists in
       * GetThreadId() so we will use HVM thread numbers internally
       */
#ifdef _DUMY_XHB_TEST_
   return val( substr( hb_aParams()[1], 2 ) )
#else
   return GetThreadId( thId )
#endif

   function hb_threadJoin( thId, xResult )
      xResult := results( thId )
   return .t.

   static function threadFirstFunc( aParams )
      local xResult
#ifdef _DUMY_XHB_TEST_
      xResult := { "skipped test " + aParams[1], val( substr( aParams[1], 2 ) ) + 0.99 }
      results( val( substr( aParams[1], 2 ) ), xResult )
#else
      xResult := hb_execFromArray( aParams )
      results( GetThreadId(), xResult )
#endif
   return nil

   static function results( nThread, xResult )
      static s_aResults
      static s_mutex
      if s_aResults == nil
         s_aResults := HSetAutoAdd( hash(), .t. )
         s_mutex := hb_mutexCreate()
      endif
      if pcount() < 2
         while ! nThread $ s_aResults
            Subscribe( s_mutex, 1000 )
         enddo
         xResult := s_aResults[ nThread ]
      else
         s_aResults[ nThread ] := xResult
         /* We cannot use NotifyAll() here because it will create
          * race condition. In this program only one thread join
          * results so we can use simple Notify() as workaround
          */
         //NotifyAll( s_mutex )
         Notify( s_mutex )
      endif
   return xResult

#else

   function hb_threadStart()
   return nil
   function hb_threadJoin()
   return nil
   function hb_mutexCreate()
   return nil
   function hb_mutexSubscribe()
   return nil
   function hb_mutexLock()
   return nil
   function hb_mutexUnlock()
   return nil
   function notify()
   return nil
   function waitForThreads()
   return nil

#endif

/* initialize mutex in hb_trheadDoOnce() */
init proc once_init()
   hb_threadOnce()
return

function hb_threadOnce( xOnceControl, bAction )
   static s_mutex
   local lFirstCall := .f.
   if s_mutex == NIL
      s_mutex := hb_mutexCreate()
   endif
   if xOnceControl == NIL
      hb_mutexLock( s_mutex )
      if xOnceControl == NIL
         xOnceControl := .t.
         lFirstCall := .t.
         if bAction != NIL
            eval( bAction )
         endif
      endif
      hb_mutexUnlock( s_mutex )
   endif
return lFirstCall

#endif


TEST t000 INFO "empty loop overhead" CODE

TEST t001 WITH L_C:=dtos(date()) CODE x := L_C

TEST t002 WITH L_N:=112345.67    CODE x := L_N

TEST t003 WITH L_D:=date()       CODE x := L_D

TEST t004 INIT _( static s_once, S_C ) ;
          INIT iif( hb_threadOnce( @s_once ), S_C := dtos(date()), ) ;
          CODE x := S_C

TEST t005 INIT _( static s_once, S_N ) ;
          INIT iif( hb_threadOnce( @s_once ), S_N := 112345.67, ) ;
          CODE x := S_N

TEST t006 INIT _( static s_once, S_D ) ;
          INIT iif( hb_threadOnce( @s_once ), S_D := date(), ) ;
          CODE x := S_D

TEST t007 INIT _( memvar M_C ) INIT _( private M_C := dtos( date() ) ) ;
          CODE x := M_C

TEST t008 INIT _( memvar M_N ) INIT _( private M_N := 112345.67 ) ;
          CODE x := M_N

TEST t009 INIT _( memvar M_D ) INIT _( private M_D := date() ) ;
          CODE x := M_D

TEST t010 INIT _( memvar P_C ) ;
          INIT _( static s_once ) ;
          INIT _( public P_C ) ;
          INIT iif( hb_threadOnce( @s_once ), P_C := dtos( date() ), ) ;
          CODE x := P_C

TEST t011 INIT _( memvar P_N ) ;
          INIT _( static s_once ) ;
          INIT _( public P_N ) ;
          INIT iif( hb_threadOnce( @s_once ), P_N := 112345.67, ) ;
          CODE x := P_N

TEST t012 INIT _( memvar P_D ) ;
          INIT _( static s_once ) ;
          INIT _( public P_D ) ;
          INIT iif( hb_threadOnce( @s_once ), P_D := date(), ) ;
          CODE x := P_D

TEST t013 INIT _( field F_C ) INIT use_dbsh() EXIT close_db() ;
          CODE x := F_C

TEST t014 INIT _( field F_N ) INIT use_dbsh() EXIT close_db() ;
          CODE x := F_N

TEST t015 INIT _( field F_D ) INIT use_dbsh() EXIT close_db() ;
          CODE x := F_D

TEST t016 WITH o := errorNew() CODE x := o:GenCode

TEST t017 WITH o := errorNew() CODE x := o[8]

TEST t018 CODE round( i / 1000, 2 )

TEST t019 CODE str( i / 1000 )

TEST t020 WITH s := stuff( dtos( date() ), 7, 0, "." ) CODE val( s )

TEST t021 WITH a := afill( array( ARR_LEN ), ;
                           stuff( dtos( date() ), 7, 0, "." ) ) ;
          CODE val( a [ i % ARR_LEN + 1 ] )

TEST t022 WITH d := date() CODE dtos( d - i % 10000 )

TEST t023 CODE eval( { || i % ARR_LEN } )

TEST t024 WITH bc := { || i % ARR_LEN } ;
          INFO eval( bc := { || i % ARR_LEN } ) ;
          CODE eval( bc )

TEST t025 CODE eval( { |x| x % ARR_LEN }, i )

TEST t026 WITH bc := { |x| x % ARR_LEN } ;
          INFO eval( bc := { |x| x % ARR_LEN }, i ) ;
          CODE eval( bc, i )

TEST t027 CODE eval( { |x| f1( x ) }, i )

TEST t028 WITH bc := { |x| f1( x ) } ;
          INFO eval( bc := { |x| f1( x ) }, i ) ;
          CODE eval( bc, i )

TEST t029 CODE x := &( "f1(" + str(i) + ")" )

TEST t030 WITH bc CODE bc := &( "{|x|f1(x)}" ); eval( bc, i )

TEST t031 CODE x := valtype( x ) +  valtype( i )

TEST t032 WITH a := afill( array( ARR_LEN ), ;
                           stuff( dtos( date() ), 7, 0, "." ) ) ;
          CODE x := strzero( i % 100, 2 ) $ a[ i % ARR_LEN + 1 ]

TEST t033 WITH a := array( ARR_LEN ), s := dtos( date() ) ;
          INIT aeval( a, { |x,i| a[i] := left( s + s, i ), x } ) ;
          CODE x := a[ i % ARR_LEN + 1 ] == s

TEST t034 WITH a := array( ARR_LEN ), s := dtos( date() ) ;
          INIT aeval( a, { |x,i| a[i] := left( s + s, i ), x } ) ;
          CODE x := a[ i % ARR_LEN + 1 ] = s

TEST t035 WITH a := array( ARR_LEN ), s := dtos( date() ) ;
          INIT aeval( a, { |x,i| a[i] := left( s + s, i ), x } ) ;
          CODE x := a[ i % ARR_LEN + 1 ] >= s

TEST t036 WITH a := array( ARR_LEN ), s := dtos( date() ) ;
          INIT aeval( a, { |x,i| a[i] := left( s + s, i ), x } ) ;
          CODE x := a[ i % ARR_LEN + 1 ] <= s

TEST t037 WITH a := array( ARR_LEN ), s := dtos( date() ) ;
          INIT aeval( a, { |x,i| a[i] := left( s + s, i ), x } ) ;
          CODE x := a[ i % ARR_LEN + 1 ] < s

TEST t038 WITH a := array( ARR_LEN ), s := dtos( date() ) ;
          INIT aeval( a, { |x,i| a[i] := left( s + s, i ), x } ) ;
          CODE x := a[ i % ARR_LEN + 1 ] > s

TEST t039 WITH a := array( ARR_LEN ) ;
          INIT aeval( a, { |x,i| a[i] := i, x } ) ;
          CODE ascan( a, i % ARR_LEN )

TEST t040 WITH a := array( ARR_LEN ) ;
          INIT aeval( a, { |x,i| a[i] := i, x } ) ;
          CODE ascan( a, { |x| x == i % ARR_LEN } )

TEST t041 WITH a := {}, a2 := { 1, 2, 3 }, bc := { |x| f1(x) }, ;
               s := dtos( date() ), s2 := "static text" ;
          CODE if i%1000==0;a:={};end; aadd(a,{i,1,.t.,s,s2,a2,bc})

TEST t042 WITH a := {} CODE x := a

TEST t043 CODE x := {}

TEST t044 CODE f0()

TEST t045 CODE f1( i )

TEST t046 WITH c := dtos( date() ) ;
          INFO f2( c[1...8] ) ;
          CODE f2( c )

TEST t047 WITH c := repl( dtos( date() ), 5000 ) ;
          INFO f2( c[1...40000] ) ;
          CODE f2( c )

TEST t048 WITH c := repl( dtos( date() ), 5000 ) ;
          INFO f2( @c[1...40000] ) ;
          CODE f2( c )

TEST t049 WITH c := repl( dtos( date() ),5000 ), c2 ;
          INFO "f2( @c[1...40000] ), c2 := c" ;
          CODE f2( @c ); c2 := c

TEST t050 WITH a := {}, a2 := { 1, 2, 3 }, bc := { |x| f1(x) }, ;
               s := dtos( date() ), s2 := "static text", n := 1.23 ;
          CODE f3( a, a2, s, i, s2, bc, i, n, x )

TEST t051 WITH a := { 1, 2, 3 } CODE f2( a )

TEST t052 CODE x := f4()

TEST t053 CODE x := f5()

TEST t054 WITH c := dtos( date() ) CODE f_prv( c )

function thTest( mtxJobs, aResults )
   local xJob
   while .T.
      hb_mutexSubscribe( mtxJobs,, @xJob )
      if xJob == NIL
         exit
      endif
      aResults[ xJob ] := &( "t" + strzero( xJob, 3 ) )()
   enddo
return nil

function thTestScale( mtxJobs, mtxResults )
   local xJob
   while .T.
      hb_mutexSubscribe( mtxJobs,, @xJob )
      if xJob == NIL
         exit
      endif
      hb_mutexNotify( mtxResults, &( "t" + strzero( xJob, 3 ) )() )
   enddo
return nil

proc test( nMT, cExclude, lScale )
local nLoopOverHead, nTimes, nSeconds, cNum, aThreads, aResults, ;
      mtxJobs, mtxResults, nTimeST, nTimeMT, nTimeTotST, nTimeTotMT, ;
      cTest, x, i, j

create_db()

#ifdef __HARBOUR__
#include "hbmemory.ch"
if MEMORY( HB_MEM_USEDMAX ) != 0
   ? "Warning !!! Memory statistic enabled."
   ?
endif
#endif

//? "Startup loop to increase CPU clock..."
//x := seconds() + 5; while x > seconds(); enddo

#ifdef __HARBOUR__
   if !hb_mtvm()
      if lScale
         ? "scale test available only in MULTI THREAD mode"
         ?
         return
      endif
      if nMT != 0
         ? "SINGLE THREAD mode, number of threads set to 0"
         nMT := 0
      endif
   endif
   ? date(), time(), os()
   ? version() + iif( hb_mtvm(), " (MT)" + iif( nMT != 0, "+", "" ), "" ), ;
     hb_compiler()
#else
   ? date(), time(), os()
   ? version()
#endif
if lScale .and. nMT < 1
   nMT := 1
endif

? "THREADS:", iif( nMT < 0, "all->" + ltrim( str( N_TESTS ) ), ltrim( str( nMT ) ) )
? "N_LOOPS:", ltrim( str( N_LOOPS ) )
if !empty( cExclude )
   ? "excluded tests:", cExclude
endif

x :=t000()
nLoopOverHead := x[2]

if lScale
   ? space(56) + "1 th." + str(nMT,3) + " th.  factor"
   ? replicate("=",76)
else
   ? dsp_result( x, 0 )
   ? replicate("=",68)
endif

nSeconds := seconds()
nTimes := secondsCPU()

#ifdef __HARBOUR__
   if lScale
      mtxJobs := hb_mutexCreate()
      mtxResults := hb_mutexCreate()
      nTimeTotST := nTimeTotMT := 0
      for i:=1 to nMT
         hb_threadStart( "thTestScale", mtxJobs, mtxResults )
      next
      for i:=1 to N_TESTS
         cTest := strzero( i, 3 )
         if !cTest $ cExclude

            /* linear execution */
            nTimeST := seconds()
            for j:=1 to nMT
               hb_mutexNotify( mtxJobs, i )
               hb_mutexSubscribe( mtxResults,, @x )
               cTest := x[1]
            next
            nTimeST := seconds() - nTimeST
            nTimeTotST += nTimeST

            /* simultaneous execution */
            nTimeMT := seconds()
            for j:=1 to nMT
               hb_mutexNotify( mtxJobs, i )
            next
            for j:=1 to nMT
               hb_mutexSubscribe( mtxResults,, @x )
               cTest := x[1]
            next
            nTimeMT := seconds() - nTimeMT
            nTimeTotMT += nTimeMT

            ? dsp_scaleResult( cTest, nTimeST, nTimeMT, nMT, nLoopOverHead )
         endif

      next
      for i:=1 to nMT
         hb_mutexNotify( mtxJobs, NIL )
      next
      hb_threadWaitForAll()
   elseif nMT < 0
      aThreads := array( N_TESTS )
      for i:=1 to N_TESTS
         cNum := strzero( i, 3 )
         if !cNum $ cExclude
            aThreads[ i ] := hb_threadStart( "t" + cNum )
         endif
      next
      for i:=1 to N_TESTS
         if aThreads[ i ] != NIL .and. hb_threadJoin( aThreads[ i ], @x )
            ? dsp_result( x, nLoopOverHead )
         endif
       next
   elseif nMT > 0
      aResults := array( N_TESTS )
      mtxJobs := hb_mutexCreate()
      for i:=1 to nMT
         hb_threadStart( "thTest", mtxJobs, aResults )
      next
      for i:=1 to N_TESTS
         if !strzero( i, 3 ) $ cExclude
            hb_mutexNotify( mtxJobs, i )
         endif
      next
      for i:=1 to nMT
         hb_mutexNotify( mtxJobs, NIL )
      next
      hb_threadWaitForAll()
      for i:=1 to N_TESTS
         if aResults[ i ] != NIL
            ? dsp_result( aResults[ i ], nLoopOverHead )
         endif
      next
      mtxJobs := NIL
   else
      for i:=1 to N_TESTS
         cNum := strzero( i, 3 )
         if !cNum $ cExclude
            ? dsp_result( &( "t" + cNum )(), nLoopOverHead )
         endif
      next
   endif
#else
   for i:=1 to N_TESTS
      cNum := strzero( i, 3 )
      if !cNum $ cExclude
         ? dsp_result( &( "t" + cNum )(), nLoopOverHead )
      endif
   next
#endif

nTimes := secondsCPU() - nTimes
nSeconds := seconds() - nSeconds

if lScale
   ? replicate("=",76)
   ? dsp_scaleResult( "  TOTAL  ", nTimeTotST, nTimeTotMT, nMT, 0 )
   ? replicate("=",76)
else
   ? replicate("=",68)
endif
? dsp_result( { "total application time:", nTimes }, 0)
? dsp_result( { "total real time:", nSeconds }, 0 )
?

remove_db()
return

function f0()
return nil

function f1(x)
return x

function f2(x)
HB_SYMBOL_UNUSED( x )
return nil

function f3(a,b,c,d,e,f,g,h,i)
HB_SYMBOL_UNUSED( a )
HB_SYMBOL_UNUSED( b )
HB_SYMBOL_UNUSED( c )
HB_SYMBOL_UNUSED( d )
HB_SYMBOL_UNUSED( e )
HB_SYMBOL_UNUSED( f )
HB_SYMBOL_UNUSED( g )
HB_SYMBOL_UNUSED( h )
HB_SYMBOL_UNUSED( i )
return nil

function f4()
return space(4000)

function f5()
return space(5)

function f_prv(x)
   memvar PRV_C
   private PRV_C := x
return nil

/*
function f_pub(x)
   memvar PUB_C
   public PUB_C := x
return nil

function f_stat(x)
   static STAT_C
   STAT_C := x
return nil
*/

static func dsp_result( aResult, nLoopOverHead )
   return padr( "[ " + left( aResult[1], 56 ) + " ]", 60, "." ) + ;
          strtran( str( max( aResult[2] - nLoopOverHead, 0 ), 8, 2 ), " ", "." )

static func dsp_scaleResult( cTest, nTimeST, nTimeMT, nMT, nLoopOverHead )
   if .f.
      nTimeST := max( 0, nTimeST - nMT * nLoopOverHead )
      nTimeMT := max( 0, nTimeMT - nMT * nLoopOverHead )
   endif
   return padr( "[ " + left( cTest, 50 ) + " ]", 54, "_" ) + ;
          str( nTimeST, 6, 2 ) + " " + str( nTimeMT, 6, 2 ) + " ->" + ;
          str( nTimeST / nTimeMT, 6, 2 )


#define TMP_FILE "_tst_tmp.dbf"
static proc create_db()
   remove_db()
   dbcreate( TMP_FILE, { {"F_C", "C", 10, 0},;
                         {"F_N", "N", 10, 2},;
                         {"F_D", "D",  8, 0} } )
   use TMP_FILE exclusive
   dbappend()
   replace F_C with dtos(date())
   replace F_N with 112345.67
   replace F_D with date()
   close
return

static proc remove_db()
   ferase( TMP_FILE )
return

static proc close_db()
   close
return

static proc use_dbsh()
   use TMP_FILE shared
return
