/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    a small speed test program
 *
 * Copyright 2006 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
 * www - http://www.harbour-project.org
 *
 */

#define ARR_LEN 16
#define N_LOOPS 1000000

//#define NO_DBF_TEST
#define NO_KEYBOARD_TEST
#define NO_ASSOC_TEST

#define REAL_TIME

#ifdef __CLIP__
    #define ASSOC_ARRAY map()
    #undef REAL_TIME
#endif
#ifdef FlagShip
    #undef REAL_TIME
    #xtranslate seconds() => fs_seconds()
#endif
#ifdef __XHARBOUR__
    #define ASSOC_ARRAY Hash()
    #undef REAL_TIME
#endif
#ifdef __HARBOUR__
    #undef REAL_TIME
#endif

#ifdef REAL_TIME
    #xtranslate secondscpu([<x>]) => seconds()
#endif
#ifndef EOL
    #define EOL chr(10)
#endif
#command ? => outstd(EOL)
#command ? <xx,...> => outstd(<xx>, EOL)

#ifdef NO_ASSOC_TEST
    #undef ASSOC_ARRAY
#endif

function main()
#ifndef NO_DBF_TEST
field F_C, F_N, F_D
#endif
memvar M_C, M_N, M_D
local L_C, L_N, L_D
local i, j, t, c, c2, n, d, o, bc, tn, total, totalr, aa,;
      a[ARR_LEN], a2[ARR_LEN], a3[ARR_LEN], aDb, cFi
#ifdef ASSOC_ARRAY
    local aAssoc := ASSOC_ARRAY
#endif
private M_C := dtos(date()),;
        M_N := 112345.67,;
        M_D := date()

#ifndef __CLIP__
//#ifdef __XHARBOUR__
//  setcancel(.f.)
//  altd(0)
//#endif
#endif
#ifdef __CLIP__
  SET MACRO_IN_STRING OFF
  //CLEAR SCREEN
#endif
#ifdef FlagShip
  FS_SET( "zerobyte", .t. )
  FS_SET( "devel", .f. )
//  FS_SET( "break", 0 )
//  FS_SET( "debug", 0 )
#endif

for i:=1 to len(a)
    a[i]:=i
    a2[i]:=dtos(date())+chr(i%64+32)+chr(i%64+64)+str(i,10)
    a3[i]:=stuff(dtos(date()),7,0,".")
next

? ""
? VERSION()+", "+OS()
? "ARR_LEN =", ARR_LEN
? "N_LOOPS =", N_LOOPS

t:=secondscpu()
for i:=1 to N_LOOPS
next
tn:=secondscpu()-t
? "empty loops overhead =", tn
#ifdef REAL_TIME
    ? "real time -> seconds()"
#else
    ? "CPU usage -> secondsCPU()"
#endif
? ""

aDb:={ {"F_C", "C", 10, 0},;
       {"F_N", "N", 10, 2},;
       {"F_D", "D",  8, 0} }
cFi:="tst_tmp.dbf"
if file(cFi)
    ferase(cFi)
endif

#ifndef NO_DBF_TEST
dbcreate(cFi, aDb)
select(1)
use &cFi shared
dbappend()
replace F_C with dtos(date())
replace F_N with 112345.67
replace F_D with date()
dbcommit()
dbunlock()

cFi:="tst_tmp2.dbf"
if file(cFi)
    ferase(cFi)
endif
dbcreate(cFi, aDb)
select(2)
use &cFi exclusive
dbappend()
replace F_C with dtos(date())
replace F_N with 112345.67
replace F_D with date()
dbcommit()
dbunlock()
#endif

L_C := dtos(date())
L_N := 112345.67
L_D := date()


total:=secondscpu()
totalr:=seconds()
t:=secondscpu()
for i:=1 to N_LOOPS
    c:=L_C
next
dsp_time( "c:=L_C -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    n:=L_N
next
dsp_time( "n:=L_N -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    d:=L_D
next
dsp_time( "d:=L_D -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    c:=M_C
next
dsp_time( "c:=M_C -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    n:=M_N
next
dsp_time( "n:=M_N -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    d:=M_D
next
dsp_time( "d:=M_D -> ", t, tn)

#ifndef NO_DBF_TEST
select(1)
t:=secondscpu()
for i:=1 to N_LOOPS
    c:=F_C
next
dsp_time( "(sh) c:=F_C -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    n:=F_N
next
dsp_time( "(sh) n:=F_N -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    d:=F_D
next
dsp_time( "(sh) d:=F_D -> ", t, tn)

select(2)
t:=secondscpu()
for i:=1 to N_LOOPS
    c:=F_C
next
dsp_time( "(ex) c:=F_C -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    n:=F_N
next
dsp_time( "(ex) n:=F_N -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    d:=F_D
next
dsp_time( "(ex) d:=F_D -> ", t, tn)
#endif

o:=errorNew()
t:=secondscpu()
for i:=1 to N_LOOPS
    n:=o:GenCode
next
dsp_time( "n:=o:GenCode -> ", t, tn)

#ifdef ASSOC_ARRAY
t:=secondscpu()
for i:=1 to N_LOOPS
    aAssoc[a2[i%ARR_LEN+1]+ltrim(str(i%100))]:=i
next
dsp_time( "aAssoc[a2[i%ARR_LEN+1]+ltrim(str(i%100)]:=i -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    c:=aAssoc[a2[i%ARR_LEN+1]+ltrim(str(i%100))]
next
dsp_time( "c:=aAssoc[a2[i%ARR_LEN+1]+ltrim(str(i%100)] -> ", t, tn)
#endif

#ifndef NO_KEYBOARD_TEST
t:=secondscpu()
for i:=1 to N_LOOPS
    inkey()
next
dsp_time( "inkey() -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    nextkey()
next
dsp_time( "nextkey() -> ", t, tn)
#endif

t:=secondscpu()
for i:=1 to N_LOOPS
    round(i/1000,2)
next
dsp_time( "round(i/1000,2) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    str(i/1000)
next
dsp_time( "str(i/1000) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    val(a3[i%ARR_LEN+1])
next
dsp_time( "val(a3[i%ARR_LEN+1]) -> ", t, tn)

t:=secondscpu()
j:=date()
for i:=1 to N_LOOPS
    dtos(j+i%10000-5000)
next
dsp_time( "dtos(j+i%10000-5000) -> ", t, tn)

bc:={||i%ARR_LEN}
t:=secondscpu()
for i:=1 to N_LOOPS
    eval(bc)
next
dsp_time( "eval({||i%ARR_LEN}) -> ", t, tn)

bc:={|x|x%ARR_LEN}
t:=secondscpu()
for i:=1 to N_LOOPS
    eval(bc,i)
next
dsp_time( "eval({|x|x%ARR_LEN}) -> ", t, tn)

bc:={|x|f1(x)}
t:=secondscpu()
for i:=1 to N_LOOPS
    eval(bc,i)
next
dsp_time( "eval({|x|f1(x)}) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    n:=&("f1("+str(i)+")")
next
dsp_time( "&('f1('+str(i)+')') -> ", t, tn)

bc:=&("{|x|f1(x)}")
t:=secondscpu()
for i:=1 to N_LOOPS
    eval(bc,i)
next
dsp_time( "eval([&('{|x|f1(x)}')]) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    j:=valtype(a)+valtype(i)
next
dsp_time( "j := valtype(a)+valtype(i) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    j := str(i%100,2) $ a2[i%ARR_LEN+1]
next
dsp_time( "j := str(i%100,2) $ a2[i%ARR_LEN+1] -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    j := val(a2[i%ARR_LEN+1])
next
dsp_time( "j := val(a2[i%ARR_LEN+1]) -> ", t, tn)

c:=dtos(date())
t:=secondscpu()
for i:=1 to N_LOOPS
    j := a2[i%ARR_LEN+1] == c
next
dsp_time( "j := a2[i%ARR_LEN+1] == s -> ", t, tn)

c:=dtos(date())
t:=secondscpu()
for i:=1 to N_LOOPS
    j := a2[i%ARR_LEN+1] = c
next
dsp_time( "j := a2[i%ARR_LEN+1] = s -> ", t, tn)

c:=dtos(date())
t:=secondscpu()
for i:=1 to N_LOOPS
    j := a2[i%ARR_LEN+1] >= c
next
dsp_time( "j := a2[i%ARR_LEN+1] >= s -> ", t, tn)

c:=dtos(date())
t:=secondscpu()
for i:=1 to N_LOOPS
    j := a2[i%ARR_LEN+1] < c
next
dsp_time( "j := a2[i%ARR_LEN+1] < s -> ", t, tn)

aa:={}
t:=secondscpu()
for i:=1 to N_LOOPS
    if i%1000 == 0
   aa:={}
    endif
    aadd(aa,{i,j,c,a,a2,t,bc})
next
dsp_time( "aadd(aa,{i,j,s,a,a2,t,bc}) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    f0()
next
dsp_time( "f0() -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    f1(i)
next
dsp_time( "f1(i) -> ", t, tn)

c:=dtos(date())
t:=secondscpu()
for i:=1 to N_LOOPS
    f2(c)
next
dsp_time( "f2(c["+ltrim(str(len(c)))+"]) -> ", t, tn)

c:=replicate(c,5000)
t:=secondscpu()
for i:=1 to N_LOOPS
    f2(c)
next
dsp_time( "f2(c["+ltrim(str(len(c)))+"]) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    f2(@c)
next
dsp_time( "f2(@c["+ltrim(str(len(c)))+"]) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    f2(c)
    c2:=c
next
dsp_time( "f2(c["+ltrim(str(len(c)))+"]); c2:=c -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    f2(@c)
    c2:=c
next
dsp_time( "f2(@c["+ltrim(str(len(c)))+"]); c2:=c -> ", t, tn)

c:=dtos(date())
t:=secondscpu()
for i:=1 to N_LOOPS
    f3(a,a2,c,i,j,t,bc)
next
dsp_time( "f3(a,a2,c,i,j,t,bc) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    f2(a2)
next
dsp_time( "f2(a2) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    c:=f4()
next
dsp_time( "s:=f4() -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    ascan(a,i%ARR_LEN)
next
dsp_time( "ascan(a,i%ARR_LEN) -> ", t, tn)

c:=dtos(date())
t:=secondscpu()
for i:=1 to N_LOOPS
    ascan(a2,c+chr(i%64+64))
next
dsp_time( "ascan(a2,c+chr(i%64+64)) -> ", t, tn)

t:=secondscpu()
for i:=1 to N_LOOPS
    ascan(a,{|x|x==i%ARR_LEN})
next
dsp_time( "ascan(a,{|x|x==i%ARR_LEN}) -> ", t, tn)

? replicate("=",60)
dsp_time( "total application time:", total, 0)
? padr( "total real time:",50)+str(seconds()-totalr,8,2)
?
return nil

function dsp_time(s,t,tn)
? padr(s,50)+str(max(secondscpu()-t-tn,0),8,2)
return nil

function f0(x)
return nil

function f1(x)
return x+1

function f2(s)
return nil

function f3(a,b,c,d,e,f,g,h,i)
return nil

function f4()
return space(50000)

#ifdef FlagShip
    function fs_seconds()
    LOCAL_DOUBLE nret := 0
    #Cinline
    {
        #include <sys/time.h>
        struct timeval tv;
        struct timezone tz;
        if( !gettimeofday(&tv, NULL) )
            nret = (double) tv.tv_sec + (double) (tv.tv_usec) / 1000000;
/*
            nret = (double) (tv.tv_sec - tz.tz_minuteswest * 60 ) % 86400 +
                   (double) tv.tv_usec / 1000000;
*/
    }
    #endCinline
    return ( nret )

    #ifndef FlagShip5
        FUNCTION cursesinit()
        CALL fgsIoctl2
        #Cinline
        {
            #include <fcntl.h>
            int arg;
            if ((arg = fcntl(0, F_GETFL, 0)) != -1)
                fcntl(0, F_SETFL, arg | O_NONBLOCK);
        }
        #endCinline
        return nil
    #endif
#endif
