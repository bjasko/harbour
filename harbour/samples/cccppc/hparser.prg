/*
 * $Id$
 */

//*******************************************************************
// hparser.prg: Hessmark parser(1): a '#'-al kezd�d� sorok elemz�se.
// 1999, Csisz�r Levente

/*
   1999.05.15
   
      - #define, #xtranslate elemz�se.
      
   1999.04.22
      - Elemzi a #define sorokat, �s a makr�kat elhelyezi egy 
        sz�t�rban.
        
     - Ami nincs:
          1. Az error tokeneket ki kellene v�logatni, �s a v�g�n 
             odatenni az output-ra.

          2. A cr-lf-et most m�g az inputon (az creader:read()-ban
             lf-re konvert�lja, mert a parsebuffer �jra sz�tszedi
             cr karakterre �s lf karakterre.
             Ez nem j�, m�sik megold�st kell keresni.
             
          3. A t�bbi # utas�t�st implement�lni kell.

          4. A #define makr�k v�grehajt�s�t meg kell �rni
             (nem felt�tlen�l itt!)
*/

//*******************************************************************
#include "objgen.ch"

//*******************************************************************
#define _STRICT_PARENT_
#include "token.och"
#include "mmarker.och"
#include "rmarker.och"
#include "maltrset.och"
#include "rsmmarkr.och"
#include "ralter.och"
// #include "edefdict.och"
// #include "defdict.och"
#include "extrdict.och"
#include "xtrdict.och"
// #include "freader.och"
#include "incl.och"
#include "prserr.och"
//*******************************************************************
// #include "cr_lf.ch"
#include "ctoken.ch"
// #include "prserr.ch"

//*******************************************************************
#include "hparser.ch"

//*******************************************************************
#define _HPARSER_PRG_
#define _IMPLEMENT_ONEW_

#include "hparser.och"

#undef ID_STRING
//*******************************************************************
// �sszehasonl�tja a this:item-et egy karaterrel.
#define eqItemChar(aChar) (valtype(this:item)=="C" .and. ;
                           this:item==(aChar))

//*******************************************************************
implement oinit(inputReader,name,defDict,xtrDict,inclObj,errorStream)
           
   super:oinit(inputReader,name,errorStream)
   this:soreleje:=.t.
   // this:errorItem:=nil
   this:defdict:=defDict//C.DEFDICT:onew()
   this:xtrdict:=xtrDict//C.XTRDICT:onew()
   this:ifStack:={}
   this:branch:=.t.
   this:inclObj:=inclObj
   // this:errorStream:=errorStream
   
return this
   
//*******************************************************************
cimplement oinitclass()

   C.PRSERR:registerError(HPRERR_INVALIDMN  ,;
      "Invalid name follows '#': $1")
   C.PRSERR:registerError(HPRERR_SDEFINE    ,;
      "Syntax error in #define")
   C.PRSERR:registerError(HPRERR_LDEFINE    ,;
      "Label missing in #define")
   C.PRSERR:registerError(HPRERR_PDEFINE    ,;
      "Comma or right parenthesis missing in #define")
   C.PRSERR:registerError(HPRERR_SUNDEF     ,;
      "Error in #undef"/*"Label missing in #undef"*/)
   C.PRSERR:registerError(HPRERR_MDUPLICATE ,;
      "Duplicate #define: $1, $2")
   C.PRSERR:registerError(HPRERR_SXTRANSLATE,;
      "Syntax error in #xtranslate/#xcommand/#translate/#command")
   C.PRSERR:registerError(HPRERR_XTRANSLATEEOL,;
      "Not expected end of line in #xtranslate/#xcommand/#translate/#command")
   C.PRSERR:registerError(HPRERR_XTRRNESTED,;
      "Nested [] in the right side of #xtranslate/#xcommand/#translate/#command")
   C.PRSERR:registerError(HPRERR_XTRLUNCLOSED,;
      "Unclosed [] in the left side of #xtranslate/#xcommand/#translate/#command")
   C.PRSERR:registerError(HPRERR_XTRUNDEFRM,;
      "Result marker not found in #xtranslate/#xcommand/#translate/#command's match markers: $0")
   C.PRSERR:registerError(HPRERR_IFDEFNL    ,;
      "Label missing in #ifdef/#ifndef")
   C.PRSERR:registerError(HPRERR_SIFDEF     ,;
      "Syntax error in #ifdef/#ifndef")
   C.PRSERR:registerError(HPRERR_ELSE       ,;
      "#else does not match #if")
   C.PRSERR:registerError(HPRERR_ELSE2      ,;
      "Duplicated #else")
   C.PRSERR:registerError(HPRERR_ENDIF      ,;
      "#endif does not match #if")
   C.PRSERR:registerError(HPRERR_ENDIFMISSING,;
      "#endif missing at end of file")
   C.PRSERR:registerError(HPRERR_INCLUDE    ,;
      "Bad or missing filename in #include")
   C.PRSERR:registerError(HPRERR_INCLUDEOPEN,;
      "Can't open include file: $1")
   C.PRSERR:registerError(HPRERR_INCLUDEFIND,;
      "Can't find include file: $1")
   C.PRSERR:registerError(HPRERR_INCLUDENEST,;
      "Too many nested include: $1")
   C.PRSERR:registerError(HPRERR_USER,;
      "#error $1")

return class

//*******************************************************************
implement readItem()
local w,tkId,tkStr
   
   /*
     Olvasunk a pufferb�l. Ha ki�r�lt, akkor:
     
     - ha sorelej�n vagyunk, akkor h�vjuk a parseLine()-t.
       Ez a sorv�gjelet mindig a pufferben hagyja, hacsak nincs v�ge
       a fil�nek.
     - ha sor k�zep�n vagyunk, akkor olvasunk, �s megn�zz�k, hogy sor 
       v�g�re jutottunk-e.
       
    Probl�ma: Ha egy fil�t lez�runk, �s van benne lez�ratlan if, 
              akkor nem ad hibajelz�st.
   */
   
   if (nil!=(w:=this:getParserBuffer()))
      // Nem kell csin�lni semmit.
   elseif (this:soreleje)
      // Ez beolvassa az eg�sz sort.
      if (this:branch==.t.)
         this:parseLine()
      else
         this:parseFalseLine()
      endif
      w:=this:getParserBuffer()
   elseif (this:branch==.t.)
      if (nil!=(w:=super:readInput()))
         tkId:=TOKEN.w:id
         tkStr:=TOKEN.w:str
         if (tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
             (tkId==TKID_CHAR .and. tkStr==";"))
           this:soreleje:=.t.
         endif
      endif
   else
      this:sorNyel()
      w:=this:getParserBuffer()
      this:soreleje:=.t.
   endif
   
   if (w!=nil .and. TOKEN.w:id==TKID_EOS)
      this:lastEos:=w
   endif
return w

//*******************************************************************

//*******************************************************************
implement parseLine()
// Csak akkor szabad h�vni, ha a parseBuffer �res �s sor elej�n 
// vagyunk.
local state,tkId,tkStr
#ifdef ID_STRING
   #define ST_L_START "start"
   #define ST_L_HMARK "hessmark"
#else
   #define ST_L_START 1
   #define ST_L_HMARK 2
#endif
local ujsor,hmarkItem
local str
               
   state:=ST_L_START
   this:soreleje:=.f.
   // this:errorItem:=nil
   this:rds()
   while(this:item!=nil)    
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS
      if (state==ST_L_START)
         if (ujsor .or.;
             (tkId==TKID_CHAR .and. tkStr==";"))
            // V�ge.
            this:soreleje:=.t.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
            // state:=ST_L_START
         elseif (tkId==TKID_CHAR .and. tkStr=="#")
            // Kezd�dik az elemz�s.
            state:=ST_L_HMARK
            hmarkItem:=this:item
         else
            // �tengedni az eg�szet.
            this:soreleje:=.f.
            // A readItem nem h�vja a parseline-t, ha nem sor
            // elej�n van.
            exit
         endif
      elseif (state==ST_L_HMARK)
         if (ujsor .or.;
             (tkId==TKID_CHAR .and. tkStr==";"))
            // Hib�s, �s v�ge is van.
            this:soreleje:=.t.
            this:errorgen(hmarkItem,HPRERR_INVALIDMN)
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
            // state:=ST_L_HMARK
         elseif (tkId==TKID_NEV)
            // Meg kell n�zni, milyen n�v, �s aszerint folytatni
            // az elemz�st.
            tkStr:=lower(tkStr)
            if (matchShortNames(tkStr,"define",4))
               this:parseDefine(this:item)
            elseif (matchShortNames(tkStr,"undef",4))
               this:parseUndef(this:item)
            elseif (matchShortNames(tkStr,"xtranslate",4) .or.;
                    matchShortNames(tkStr,"xcommand",4) .or.;
                    matchShortNames(tkStr,"translate",4) .or.;
                    matchShortNames(tkStr,"command",4))
               this:parseTranslate(this:item)
            elseif (matchShortNames(tkStr,"error",4))
               hmarkitem:=this:item
               this:rds()
               str:=""
               this:sorNyel({|x| str+=x})
               this:errorGen(hmarkitem,HPRERR_USER,{str})
            elseif (matchShortNames(tkStr,"stdout",4))
               this:rds()
               this:sorNyel({|x| outstd(x)})
               outstd(newline())
            elseif (matchShortNames(tkStr,"ifdef",4))
               this:parseIfdef(this:item,IFDEFTYPE_IFDEF)
            elseif (matchShortNames(tkStr,"ifndef",4))
               this:parseIfdef(this:item,IFDEFTYPE_IFNDEF)
            elseif (matchShortNames(tkStr,"else",4))
               // Elm�letileg ide csak akkor j�het�nk, ha az igaz 
               // �gr�l �rkezt�nk.
               this:parseElse(this:item)
            elseif (matchShortNames(tkStr,"endif",4))
               // Elm�letileg ide csak akkor j�het�nk, ha az igaz 
               // �gr�l �rkezt�nk.
               this:parseEndif(this:item)
            elseif (matchShortNames(tkStr,"include",4))
               this:parseInclude(this:item)
            else
               this:errorgen(this:item,HPRERR_INVALIDMN,{tkStr})
               this:sorNyel()
            endif
            this:soreleje:=.t.
            exit
         else
            // Hib�s, a v�g�ig le kell nyelni.
            this:soreleje:=.f.
            this:errorgen(this:item,HPRERR_INVALIDMN,{tkStr})
            exit
         endif
      else
         ? "HPARSER:parseLine(): Ismeretlen �llapot: ",state
         errorlevel(1)
         quit
      endif
      this:rds()
   end while
return nil

//*******************************************************************
implement parseFalseLine()
// Olyan sor elemz�se, ami egy #if hamis �g�n van.
// Ilyenkor csak a #if, #endif-eket figyelj�k, egy�bk�nt minden sort
// �resre reduk�lunk.
local state,tkId,tkStr
#ifdef ID_STRING
   #define ST_FL_START "start"
   #define ST_FL_HMARK "hessmark"
#else
   #define ST_FL_START 1
   #define ST_FL_HMARK 2
#endif
local ujsor,hmarkItem
               
   state:=ST_FL_START
   this:soreleje:=.f.
   // this:errorItem:=nil
   this:rds()
   while(this:item!=nil)    
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS
      if (state==ST_FL_START)
         if (ujsor .or.;
             (tkId==TKID_CHAR .and. tkStr==";"))
            // V�ge.
            this:soreleje:=.t.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
            // state:=ST_FL_START
         elseif (tkId==TKID_CHAR .and. tkStr=="#")
            // Kezd�dik az elemz�s.
            hmarkItem:=this:item
            state:=ST_FL_HMARK
         else
            // �tengedni az eg�szet.
            this:soreleje:=.f.
            // A readItem nem h�vja a parseline-t, ha nem sor
            // elej�n van.
            exit
         endif
      elseif (state==ST_FL_HMARK)
         if (ujsor .or.;
             (tkId==TKID_CHAR .and. tkStr==";"))
            // Hib�s, �s v�ge is van.
            this:soreleje:=.t.
            this:errorgen(hmarkItem,HPRERR_INVALIDMN)
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
            state:=ST_FL_HMARK
         elseif (tkId==TKID_NEV)
            // Meg kell n�zni, milyen n�v, �s aszerint folytatni
            // az elemz�st.
            tkStr:=lower(tkStr)
            if (matchShortNames(tkStr,"ifdef",4))
               this:parseIfdef(this:item,IFDEFTYPE_NONE)
            elseif (tkStr=="ifndef")
               this:parseIfdef(this:item,IFDEFTYPE_NONE)
            elseif (matchShortNames(tkStr,"else",4))
               if (this:branch!=nil)
                  this:parseElse(this:item)
               // else
                  // Lenyeli.
               endif
            elseif (matchShortNames(tkStr,"endif",4))
               this:parseEndif(this:item)
            else
               // A rossz neveket ilyenkor elfogadja.
            endif
            exit
         else
            // Hib�s, a v�g�ig le kell nyelni.
            this:errorgen(this:item,HPRERR_INVALIDMN,{tkStr})
            exit
         endif
      else
         ? "HPARSER:parseFalseLine(): Ismeretlen �llapot: ",state
         errorlevel(1)
         quit
      endif
      this:rds()
   end while
   this:sorNyel()
   this:soreleje:=.t.
return nil

//*******************************************************************
implement errorgen(token,errorCode,params)
#ifdef OLD
local t

   t:=TOKEN.token:copyToken(TKID_ERROR)
   // TOKEN.t:error:={this:name,errorcode,;
   //                 prsErrStr(errorcode)+if(empty(errStr),""," "+errStr)}
   TOKEN.t:error:=errorcode
   this:putParserBuffer(t)
   this:errorItem:=t
#endif

   // Egyenl�re nem adunk �t param�tert.
   aadd(this:errorStream,;
        C.PRSERR:onewFromToken(errorCode,params,token))
return nil

//*******************************************************************
implement incompErrorgen(token,errorcode,params)
   // this:errorgen(token,errorcode,params)
return .t.

//*******************************************************************
implement addXTranslate(mnameToken,leftSide,rightSide)
// Az mNameToken jelzi, hogy hol volt a defin�ci�.
local w,cmdType,tkStr

   tkStr:=lower(TOKEN.mnameToken:str)
   
   if (matchShortNames(tkStr,"xtranslate",4))
      cmdType:=XTRTYPE_XTRANSLATE
   elseif (matchShortNames(tkStr,"xcommand",4))
      cmdType:=XTRTYPE_XCOMMAND
   elseif (matchShortNames(tkStr,"translate",4))
      cmdType:=XTRTYPE_TRANSLATE
   elseif (matchShortNames(tkStr,"command",4))
      cmdType:=XTRTYPE_COMMAND
   else
      cmdType:=XTRTYPE_XTRANSLATE
   endif

   w:=C.EXTRDICT:onew(cmdType,leftSide,rightSide,;
                      TOKEN.mnameToken:file,;
                      TOKEN.mnameToken:line,;
                      TOKEN.mnameToken:pos)

   // this:clearParserBuffer()
   if (!empty(EXTRDICT.w:undefRMarkers))
      aeval(EXTRDICT.w:undefRMarkers,;
         {|x| this:errorgen(mnameToken,HPRERR_XTRUNDEFRM,;
                            {RMARKER.x:getName()})})
      // outerr(EXTRDICT.w:printStr(),newline())
   else
      XTRDICT.(this:xtrdict):add(w)
   endif
   
return nil

//*******************************************************************
implement sorNyel(kiirBlock)
// T�rli a parserBuffer-t, majd a sor v�g�ig lenyeli az item-eket.
// A sorv�gjelet a parserpuffer-ben hagyja.
// Ha az errorItem nem nil, akkor azt m�g a sorv�gjel el� beteszi.
// Ha a kiirBlock egy block, akkor a token stringeket �tadja neki.

local tkId

   this:clearParserBuffer()
   if (this:item==nil)
      this:item:=this:readInput()
   endif
   while(this:item!=nil)
      tkId:=TOKEN.(this:item):id
      if (tkId==TKID_UJSOR .or.;
          tkId==TKID_BOS .or.;
          tkId==TKID_EOS)
         exit
      endif
      
      if (kiirBlock!=nil)
         eval(kiirBlock,TOKEN.(this:item):getStr())
      endif
      this:item:=this:readInput()
   end while
   
   // if (this:errorItem!=nil)
   //    this:putParserBuffer(this:errorItem)
   // endif
   if (this:item!=nil)
      this:putParserBuffer(this:item)
   endif
   
return nil

//*******************************************************************
implement parseTranslate(mNameToken)
// A # xtranslate etc, elemz�je.
/*
   �ltal�nos szab�lyok:
  
   - A bal oldalon egym�sba lehet skatuly�zni a '[]'-ket, a jobb 
     oldalon nem.
     
   - Egyenl�re :

     - a jobb oldalon csak a regular match markert (<n�v>) �s a list
       match markert (<n�v,...>)-t fogadja el,

     - a bal oldalon pedig csak a regular result markert (<n�v>) 
       fogadja el.
       


   A bal oldal is �s a jobb oldal is egy-egy token lista.
   A markerek spec tokenek lesznek (le�r�kl�nk a token oszt�lyb�l, ha
   kell.)
   
   Az alternalt�v�k a bal oldalon olyan tokenek, amik token list�k
   list�j�t tartalmazz�k.
   
   A '\' v�d� karakter a k�vetkez� karaktereket v�di:
   
   '<','[','#','\'
   
   Ha bekapcsoljuk az extend kapcsol�t (ekkor a preprocesszorban
   olyan funkci�k is �letbe l�pnek, amik az eredeti preprocesszorban
   nincsenek meg), akkor m�g v�di a k�vetkez� karaktereket:
   
   ']'
   
   Mj.: A ']' v�d�se az�rt sz�ks�ges, mert a bal oldalon egym�sba
        lehet skatuly�zni a '[]'-ket.
   Mj2.: A '='-t nem kell v�deni, a spec ugyanaz, ha v�di a '\' vagy
         ha nem.

   Az eld�nt�sre v�r, hogy egy nyit� '[' n�lk�li ']'-t elfogadjunk-e
   rendes karakternek.        
*/
local leftSide,rightSide

   if (nil!=(leftSide:=this:parseLTranslate(mNameToken)))
      if (nil!=(rightSide:=this:parseRTranslate(mNameToken)))
         this:addXTranslate(mNameToken,leftSide,rightSide)
      endif
   endif
return nil
   
//*******************************************************************
static function closeAlter(tokenList,malterStack)
local alter,itemMalterStack,t,w,i

   // Lez�rjuk az elemzett alternet�v�t.
   
   // Kivessz�k a malterstack-b�l azt, amit le kell z�rni.
   alter:=tokenList
   itemMalterStack:=atail(malterStack)
   tokenList:=itemMalterStack[1]
   // Most a tokenList-ben lev� tokenek azok, amik akkor 
   // voltak, amikor az alternat�v�t elkezdt�k elemzni.
   t:=nil
   // Megkeress�k, hogy alterset van-e a tokenlista v�g�n.
   for i:=len(tokenList) to 1 step -1
      w:=tokenList[i]
      if (TOKEN.w:id==TKID_URES)
         // �tugorjuk
      elseif (TOKEN.w:id==TKID_MALTERSET)
         // A token lista v�g�n egy alterset van, ehhez 
         // hozz�venni a mi alternat�v�nka is.
         t:=w
         // A v�g�r�l az �reseket t�r�lj�k.
         asize(tokenList,i)
         exit
      else
         exit
      endif
   end for
   if (t==nil)
      // Ha nincs MALTRSET objektumunk, akkor csin�lunk, 
      // �s a token lista v�g�re tessz�k.
      t:=C.MALTRSET:copyFromToken(itemMalterStack[2],TKID_MALTERSET,nil)
      aadd(tokenList,t)
   endif
   MALTRSET.t:addAlter(alter)
   asize(malterStack,len(malterStack)-1)
return tokenList

//*******************************************************************
implement parseLTranslate(mnameToken)
// A # xtranslate bal oldal�nak az elemz�je.
// Ret: tokenList, ha indulhat a jobb oldal, nil, ha nem.

local state,tkId,tkStr
// Az malterStack ilyen elemekb�l �ll: {savedTokenList,'[' token}
local malterStack:={} 
local alter,itemMalterStack
local matchMarkerName
local restrictMlist:={},restrictItem:=nil
local t,w
local tokenList:={}
local eqList,ujsor
local bSlash

#ifdef ID_STRING
   #define ST_TL_START                            "START"
   #define ST_TL_MATCH_MARKER_START               "MATCH_MARKER_START"
   #define ST_TL_EQCHAR                           "EQCHAR"
   #define ST_TL_MATCH_MARKER_NAME                "MATCH_MARKER_NAME"
   #define ST_TL_WILD_MATCH_MARKER_START          "WILD_MATCH_MARKER_START"
   #define ST_TL_WILD_MATCH_MARKER_NAME           "WILD_MATCH_MARKER_NAME"
   #define ST_TL_WILD_MATCH_MARKER_NAMESTAR       "WILD_MATCH_MARKER_NAMESTAR"
   #define ST_TL_EXT_EXPR_MATCH_MARKER_START      "EXT_EXPR_MATCH_MARKER_START"
   #define ST_TL_EXT_EXPR_MATCH_MARKER_NAME       "EXT_EXPR_MATCH_MARKER_NAME"
   #define ST_TL_EXT_EXPR_MATCH_MARKER_NAMEPARENT "EXT_EXPR_MATCH_MARKER_PARENT"
   #define ST_TL_LIST_MATCH_MARKER_DOTS           "LIST_MATCH_MARKER_DOTS"
   #define ST_TL_LIST_MATCH_MARKER_DOTS1          "LIST_MATCH_MARKER_DOTS1"
   #define ST_TL_LIST_MATCH_MARKER_DOTS2          "LIST_MATCH_MARKER_DOTS2"
   #define ST_TL_LIST_MATCH_MARKER_DOTS3          "LIST_MATCH_MARKER_DOTS3"
   #define ST_TL_RESTRICTED_MATCH_MARKER_WORDLIST "RESTRICTED_MATCH_MARKER_WORDLIST"
#else
   #define ST_TL_START                             1
   #define ST_TL_MATCH_MARKER_START                2
   #define ST_TL_EQCHAR                            3
   #define ST_TL_MATCH_MARKER_NAME                 4
   #define ST_TL_WILD_MATCH_MARKER_START           5
   #define ST_TL_WILD_MATCH_MARKER_NAME            6
   #define ST_TL_WILD_MATCH_MARKER_NAMESTAR        7
   #define ST_TL_EXT_EXPR_MATCH_MARKER_START       8
   #define ST_TL_EXT_EXPR_MATCH_MARKER_NAME        9
   #define ST_TL_EXT_EXPR_MATCH_MARKER_NAMEPARENT 10
   #define ST_TL_LIST_MATCH_MARKER_DOTS           11
   #define ST_TL_LIST_MATCH_MARKER_DOTS1          12
   #define ST_TL_LIST_MATCH_MARKER_DOTS2          13
   #define ST_TL_LIST_MATCH_MARKER_DOTS3          14
   #define ST_TL_RESTRICTED_MATCH_MARKER_WORDLIST 15
#endif
               
   state:=ST_TL_START
   this:rds()
   bSlash:=.f.
   while(this:item!=nil)    
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS
      if (state==ST_TL_START)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Maradunk.
            aadd(tokenList,this:item)
         elseif (tkId==TKID_CHAR .and. tkStr=="[")
            // Ez egy indul� alternat�va a bal oldalon. Ha a token 
            // list�ban az utols� token egy MALTRSET (match 
            // alternative token), akkor az �j alternat�v�t majd 
            // hozz� csatoljuk.
            if (bSlash)
               aadd(tokenList,this:item)
               bSlash:=.f.
            else
               aadd(malterStack,{tokenList,this:item})
               tokenList:={}
            endif
         elseif (tkId==TKID_CHAR .and. tkStr=="]")
            // Az aktu�lis token sor egy alternat�va lesz.
            if (len(malterStack)<=0)
               // Hiba: van lez�r� ']', �gy, hogy nincs nyit� '['.
               // Ezt a spec elfogadja, mi is elfogadjuk.
               aadd(tokenList,this:item)
               bSlash:=.f.
            else
               if (bSlash)
                  aadd(tokenList,this:item)
                  bSlash:=.f.
                  // Tov�bb megy�nk!
               endif
                  
               // Lez�rjuk az elemzett alternet�v�t.
               tokenList:=closeAlter(tokenList,malterStack)
               #ifdef OLD
               
               // Kivessz�k a malterstack-b�l azt, amit le kell z�rni.
               alter:=tokenList
               itemMalterStack:=atail(malterStack)
               tokenList:=itemMalterStack[1]
               // Most a tokenList-ben lev� tokenek azok, amik akkor 
               // voltak, amikor az alternat�v�t elkezdt�k elemzni.
               if (len(tokenList)<=0)
                  // Nincs token ==> Nincs alterset sem a v�g�n.
                  t:=nil
               else
                  w:=atail(tokenList)
                  if (TOKEN.w:id==TKID_MALTERSET)
                     // A token lista v�g�n egy alterset van, ehhez 
                     // hozz�venni a mi alternat�v�nka is.
                     t:=w
                  else
                     // A token lista v�g�n nem alterset van.
                     t:=nil
                  endif
               endif
               if (t==nil)
                  // Ha nincs MALTRSET objektumunk, akkor csin�lunk, 
                  // �s a token lista v�g�re tessz�k.
                  t:=C.MALTRSET:copyFromToken(itemMalterStack[2],TKID_MALTERSET,nil)
                  aadd(tokenList,t)
               endif
               MALTRSET.t:addAlter(alter)
               asize(malterStack,len(malterStack)-1)
               #endif
            endif
         elseif (tkId==TKID_CHAR .and. tkStr=="<")
            // Ez egy marker.
            if (bSlash)
               aadd(tokenList,this:item)
               bSlash:=.f.
            else
               state:=ST_TL_MATCH_MARKER_START
            endif
         elseif (tkId==TKID_CHAR .and. tkStr=="=")
            // Ez vagy egy '=' jel vagy egy '=>'-nek az els� r�sze.
            if (bSlash)
               aadd(tokenList,this:item)
               bSlash:=.f.
               // Tov�bb megy�nk!
            endif
            state:=ST_TL_EQCHAR
            eqList:={this:item}
         elseif (tkId==TKID_CHAR .and. tkStr=="\")
            if (bSlash)
               aadd(tokenList,this:item)
               bSlash:=.f.
            else
               bSlash:=.t.
            endif
            // Maradunk.
         else
            // Ez aktu�lisan elemzett list�ba ker�l.
            bSlash:=.f.
            aadd(tokenList,this:item)
         endif
      elseif (state==ST_TL_MATCH_MARKER_START)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_NEV)
            // A match marker neve.
            matchMarkerName:=this:item
            state:=ST_TL_MATCH_MARKER_NAME
         elseif (tkId==TKID_CHAR .and. tkStr=="*")
            // Ez egy wild match marker lesz.
            state:=ST_TL_WILD_MATCH_MARKER_START
         elseif (tkId==TKID_CHAR .and. tkStr=="(")
            // Ez egy extended expression match marker lesz.
            state:=ST_TL_EXT_EXPR_MATCH_MARKER_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_MATCH_MARKER_NAME)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_CHAR .and. tkStr==",")
            // List match marker
            state:=ST_TL_LIST_MATCH_MARKER_DOTS
         elseif (tkId==TKID_CHAR .and. tkStr==":")
            // Restricted match marker
            state:=ST_TL_RESTRICTED_MATCH_MARKER_WORDLIST
            restrictMList:={}
            restrictItem:=nil
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Ez egy regular match marker volt. Elk�sz�tj�k a tokent.
            aadd(tokenList,;
                 C.MMARKER:copyFromToken(matchMarkerName,;
                                         TKID_REGULAR_MATCH_MARKER,;
                                         nil))
            state:=ST_TL_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_WILD_MATCH_MARKER_START)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_NEV)
            // A match marker neve.
            matchMarkerName:=this:item
            state:=ST_TL_WILD_MATCH_MARKER_NAME
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_WILD_MATCH_MARKER_NAME)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_CHAR .and. tkStr=="*")
            state:=ST_TL_WILD_MATCH_MARKER_NAMESTAR
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_WILD_MATCH_MARKER_NAMESTAR)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Ez egy wild match marker volt. Elk�sz�tj�k a tokent.
            aadd(tokenList,;
                 C.MMARKER:copyFromToken(matchMarkerName,;
                                         TKID_WILD_MATCH_MARKER,;
                                         nil))
            state:=ST_TL_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_EXT_EXPR_MATCH_MARKER_START)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_NEV)
            // A match marker neve.
            matchMarkerName:=this:item
            state:=ST_TL_EXT_EXPR_MATCH_MARKER_NAME
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_EXT_EXPR_MATCH_MARKER_NAME)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_CHAR .and. tkStr==")")
            state:=ST_TL_EXT_EXPR_MATCH_MARKER_NAMEPARENT
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_EXT_EXPR_MATCH_MARKER_NAMEPARENT)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Ez egy extended expression match marker volt. Elk�sz�tj�k a tokent.
            aadd(tokenList,;
                 C.MMARKER:copyFromToken(matchMarkerName,;
                                         TKID_EXT_EXPR_MATCH_MARKER,;
                                         nil))
            state:=ST_TL_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_LIST_MATCH_MARKER_DOTS)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_CHAR .and. tkStr==".")
            state:=ST_TL_LIST_MATCH_MARKER_DOTS1
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_LIST_MATCH_MARKER_DOTS1)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_CHAR .and. tkStr==".")
            state:=ST_TL_LIST_MATCH_MARKER_DOTS2
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_LIST_MATCH_MARKER_DOTS2)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_CHAR .and. tkStr==".")
            state:=ST_TL_LIST_MATCH_MARKER_DOTS3
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_LIST_MATCH_MARKER_DOTS3)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Ez egy List match marker marker volt. Elk�sz�tj�k a tokent.
            aadd(tokenList,;
                 C.MMARKER:copyFromToken(matchMarkerName,;
                                         TKID_LIST_MATCH_MARKER,;
                                         nil))
            state:=ST_TL_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_RESTRICTED_MATCH_MARKER_WORDLIST)
         // Ez nem j�, mert itt nem csak szavak, hanem b�rmilyen
         // token lista lehet. Egyenl�re az egy elem� token
         // list�kat engedj�k meg, de ezt nem ellen�rizz�k.
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
            // Maradunk.
         elseif (tkId==TKID_CHAR .and. tkStr==",")
            // A listaelemek kihagy�s�t a spec megengedi.
            if (restrictItem==nil)
               restrictItem:=""
            endif
            aadd(restrictMList,restrictItem)
            restrictItem:=""
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Ez egy Restricted match marker volt. Elk�sz�tj�k a tokent.
            if (restrictItem!=nil)
               aadd(restrictMList,restrictItem)
            endif
            t:=C.RSMMARKR:copyFromToken(matchMarkerName,TKID_RESTRICTED_MATCH_MARKER,nil)
            RSMMARKR.t:wordList:=restrictMList
            aadd(tokenList,t)
            restrictMList:={}
            restrictItem:=nil
            state:=ST_TL_START
         elseif (tkId==TKID_NEV)
            restrictItem:=tkStr
            // Maradunk, mert a listaelemek kihagy�s�t a spec 
            // megengedi.
         elseif (tkId==TKID_STRING .or. tkId==TKID_SZAMTOMB .or. tkId==TKID_CHAR)
            restrictItem:=tkStr
            // Maradunk, mert a listaelemek kihagy�s�t a spec 
            // megengedi.
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TL_EQCHAR)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
            aadd(eqList,this:item)
         elseif (tkId==TKID_CHAR .and. tkStr == '\')
            bSlash:=.t.
         elseif (tkId==TKID_CHAR .and. tkStr == '>')
            // Ez a hat�rol�, v�ltunk.
            // A bal oldalt eltessz�k, �s j�n a jobb oldal.
            if (bSlash)
               aeval(eqList,{|x| aadd(tokenList,x)})
               eqList:={}
               aadd(tokenList,this:item)
               bSlash:=.f.
               state:=ST_TL_START
            else
               // Itt meg kell n�zni nem vagyunk-e egy 
               // alternat�v�ban.
               if (len(malterStack)>0)
                  this:errorgen(this:item,HPRERR_XTRLUNCLOSED)
                  while(!empty(malterStack))
                     tokenList:=closeAlter(tokenList,malterStack)
                  end while
               endif
               return tokenList
            endif
         elseif (tkId==TKID_CHAR .and. tkStr=='=')
            // Ezt a spec hib�nak tekinti, de val�j�ban nem az.
            // Azt mondja r�, hogy hi�nyz� '=>'
            if (bSlash)
               aeval(eqList,{|x| aadd(tokenList,x)})
               eqList:={}
               aadd(tokenList,this:item)
               bSlash:=.f.
               state:=ST_TL_START
            else          
               if (this:incompErrorgen(this:item,HPRERR_SXTRANSLATE))
                  exit
               endif
               // Ez m�g megy a bal oldalhoz.
               aeval(eqList,{|x| aadd(tokenList,x)})
               eqList:={this:item}
            endif
         else
            // B�rmi m�s, vissza az eg�sz.
            aeval(eqList,{|x| aadd(tokenList,x)})
            eqList:={}
            this:unrds()
            state:=ST_TL_START
         endif
      else
         ? "HPARSER:parseLTranslate(): Ismeretlen �llapot: ",state
         errorlevel(1)
         quit
      endif
      this:rds()
   end while
   this:sorNyel()
   
return nil

//*******************************************************************
implement parseRTranslate(mnameToken)
// A # xtranslate jobb oldal�nak az elemz�je.

local state,tkId,tkStr,ujsor

#ifdef ID_STRING
   #define ST_TR_START                            "START"
   #define ST_TR_RESULT_MARKER_START              "RESULT_MARKER_START"
   #define ST_TR_DUMB_STR_RESULT_MARKER_START     "DUMB_STR_RESULT_MARKER_START"
   #define ST_TR_DUMB_STR_RESULT_MARKER_STARTNAME "DUMB_STR_RESULT_MARKER_STARTNAME"
   #define ST_TR_DUMB_STR_RESULT_MARKER_NAME      "DUMB_STR_RESULT_MARKER_NAME"
   #define ST_TR_REGULAR_RESULT_MARKER_NAME       "REGULAR_RESULT_MARKER_NAME"
   #define ST_TR_STRINGIFY_RESULT_MARKER_NAME     "STRINGIFY_RESULT_MARKER_NAME"
   #define ST_TR_SMART_STR_RESULT_MARKER_START    "SMART_STR_RESULT_MARKER_START"
   #define ST_TR_SMART_STR_RESULT_MARKER_NAMEPAR  "SMART_STR_RESULT_MARKER_NAMEPAR"
   #define ST_TR_SMART_STR_RESULT_MARKER_NAME     "SMART_STR_RESULT_MARKER_NAME"
   #define ST_TR_BLOCKIFY_RESULT_MARKER_START     "BLOCKIFY_RESULT_MARKER_START"
   #define ST_TR_BLOCKIFY_RESULT_MARKER_NAME      "BLOCKIFY_RESULT_MARKER_NAME"
   #define ST_TR_BLOCKIFY_RESULT_MARKER_NAMEPAR   "BLOCKIFY_RESULT_MARKER_NAMEPAR"
   #define ST_TR_LOGIFY_RESULT_MARKER_START       "LOGIFY_RESULT_MARKER_START"
   #define ST_TR_LOGIFY_RESULT_MARKER_NAME        "LOGIFY_RESULT_MARKER_NAME"
   #define ST_TR_LOGIFY_RESULT_MARKER_NAMEPAR     "LOGIFY_RESULT_MARKER_NAMEPAR"
#else
   #define ST_TR_START                             1
   #define ST_TR_RESULT_MARKER_START               2
   #define ST_TR_DUMB_STR_RESULT_MARKER_START      3
   #define ST_TR_DUMB_STR_RESULT_MARKER_STARTNAME  4
   #define ST_TR_DUMB_STR_RESULT_MARKER_NAME       5
   #define ST_TR_REGULAR_RESULT_MARKER_NAME        6
   #define ST_TR_STRINGIFY_RESULT_MARKER_NAME      7
   #define ST_TR_SMART_STR_RESULT_MARKER_START     8
   #define ST_TR_SMART_STR_RESULT_MARKER_NAMEPAR   9
   #define ST_TR_SMART_STR_RESULT_MARKER_NAME     10
   #define ST_TR_BLOCKIFY_RESULT_MARKER_START     11
   #define ST_TR_BLOCKIFY_RESULT_MARKER_NAME      12
   #define ST_TR_BLOCKIFY_RESULT_MARKER_NAMEPAR   13
   #define ST_TR_LOGIFY_RESULT_MARKER_START       14
   #define ST_TR_LOGIFY_RESULT_MARKER_NAME        15
   #define ST_TR_LOGIFY_RESULT_MARKER_NAMEPAR     16
#endif

local tokenList:={}
local saveTokenList:=nil // Ide mentj�k el a tokenList-et, amikor 
                         // alternat�v�t elemz�nk.
   
local t
local dumbList // dumb stringify result markerek elemz�s�n�l '#' �s
               // az ut�na k�vetkez� token lista a '<'-ig.
               
local resultMarkerName
local w
local bSlash

   // Drop spaces            
   this:rds()
   while(this:item!=nil .and. TOKEN.(this:item):id==TKID_URES)
      this:rds()
   end while
   
   state:=ST_TR_START
   bSlash:=.f.
   while(this:item!=nil)    
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS
      if (state==ST_TR_START)
         if (ujsor)
            // V�ge van.
            this:sorNyel()
            return tokenlist
         elseif (tkId==TKID_URES)
            // Maradunk
            if (!bSlash)
               aadd(tokenList,this:item)
            endif
         elseif (tkId==TKID_CHAR .and. tkStr=="[")
            // Ez egy indul� alternat�va a jobb oldalon. 
            if (bSlash)
               aadd(tokenList,this:item)
               bSlash:=.f.
            elseif (saveTokenList!=nil)
               // Hiba: Egym�sba skatuly�zott []
               this:errorgen(this:item,HPRERR_XTRRNESTED)
            else
               saveTokenList:=tokenList
               tokenList:={}
            endif
         elseif (tkId==TKID_CHAR .and. tkStr=="]")
            // Az aktu�lis token sor egy alternat�va lesz.
            if (saveTokenList==nil)
               // Hiba: van lez�r� ']', �gy, hogy nincs nyit� '['.
               // Ezt a spec elfogadja, mi is elfogadjuk.
               aadd(tokenList,this:item)
               bSlash:=.f.
            else
               if (bSlash)
                  aadd(tokenList,this:item)
                  bSlash:=.f.
               endif
               if (!empty(tokenList))
                   t:=C.RALTER:copyFromToken(tokenList[1],TKID_RALTER,nil)
                   RALTER.t:tokenList:=tokenList
                   aadd(saveTokenList,t)
               endif
               tokenList:=saveTokenList
               saveTokenList:=nil
            endif
         elseif (tkId==TKID_CHAR .and. tkStr=="<")
            // Ez egy marker.
            if (bSlash)
               aadd(tokenList,this:item)
               bSlash:=.f.
            else
               state:=ST_TR_RESULT_MARKER_START
            endif
         elseif (tkId==TKID_CHAR .and. tkStr=="#")
            // Ez lehet egy dumb stringify result marker.
            if (bSlash)
               aadd(tokenList,this:item)
               bSlash:=.f.
            else
               state:=ST_TR_DUMB_STR_RESULT_MARKER_START
               dumbList:={this:item}
            endif
         elseif (tkId==TKID_CHAR .and. tkStr=="\")
            // Ez v�d� karakter, de csak a cthis:shelterChar-ban 
            // szerepl�ket v�di, egy�bk�nt marad norm�l karakter.
            // Mj.: Ezt ki kell pr�b�lni.
            // Egyenl�re nem dolgozzuk fel.
            if (bSlash)
               aadd(tokenList,this:item)
               bSlash:=.f.
            else
               bSlash:=.t.
            endif
            // Maradunk.
         else
            // Ez aktu�lisan elemzett list�ba ker�l.
            aadd(tokenList,this:item)
            bSlash:=.f.
         endif
      elseif (state==ST_TR_DUMB_STR_RESULT_MARKER_START)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
            aadd(dumbList,this:item)
         elseif (tkId==TKID_CHAR .and. tkStr=="<")
            // Ez dumb stringify result marker.
            state:=ST_TR_DUMB_STR_RESULT_MARKER_STARTNAME
         else
            // K�z�ns�ges karakter. Vissza az eg�sz.
            aeval(dumbList,{|x| aadd(tokenList,x)})
            this:unrds()
            state:=ST_TR_START
         endif
      elseif (state==ST_TR_DUMB_STR_RESULT_MARKER_STARTNAME)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_NEV)
            // A result marker neve.
            resultMarkerName:=this:item
            state:=ST_TR_DUMB_STR_RESULT_MARKER_NAME
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_DUMB_STR_RESULT_MARKER_NAME)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Itt a v�ge.
            aadd(tokenList,;
                 C.RMARKER:copyFromToken(resultMarkerName,;
                                         TKID_DUMB_STR_RESULT_MARKER,;
                                         nil))
            resultMarkerName:=nil
            state:=ST_TR_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_RESULT_MARKER_START)
         if (ujsor)
            // Hib�s �s v�ge is van.
            this:errorgen(mnameToken,HPRERR_XTRANSLATEEOL)
            exit
         elseif (tkId==TKID_URES)
            // Eldobjuk.
         elseif (tkId==TKID_NEV)
            // A result marker neve.
            resultMarkerName:=this:item
            state:=ST_TR_REGULAR_RESULT_MARKER_NAME
         elseif (tkId==TKID_STRING)
            // A result marker neve, ez egy Normal stringify result 
            // marker
            resultMarkerName:=this:item
            state:=ST_TR_STRINGIFY_RESULT_MARKER_NAME
         elseif (tkId==TKID_CHAR .and. tkStr=="(")
            // Ez egy Smart stringify result marker lesz.
            state:=ST_TR_SMART_STR_RESULT_MARKER_START
         elseif (tkId==TKID_CHAR .and. tkStr=="{")
            // Ez egy Blockify result marker lesz.
            state:=ST_TR_BLOCKIFY_RESULT_MARKER_START
         elseif (tkId==TKID_CHAR .and. tkStr==".")
            // Ez egy Logify result marker lesz.
            state:=ST_TR_LOGIFY_RESULT_MARKER_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_REGULAR_RESULT_MARKER_NAME)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Itt a v�ge.
            aadd(tokenList,;
                 C.RMARKER:copyFromToken(resultMarkerName,;
                                         TKID_REGULAR_RESULT_MARKER,;
                                         nil))
            resultMarkerName:=nil
            state:=ST_TR_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_STRINGIFY_RESULT_MARKER_NAME)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Itt a v�ge.
            w:=TOKEN.resultMarkerName:str
            // w:=substr(w,2,len(w)-2)
            aadd(tokenList,;
                 C.RMARKER:copyFromToken(resultMarkerName,;
                                         TKID_STRINGIFY_RESULT_MARKER,;
                                         w))
            resultMarkerName:=nil
            state:=ST_TR_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_SMART_STR_RESULT_MARKER_START)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_NEV)
            resultMarkerName:=this:item
            state:=ST_TR_SMART_STR_RESULT_MARKER_NAME
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_SMART_STR_RESULT_MARKER_NAME)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr==")")
            state:=ST_TR_SMART_STR_RESULT_MARKER_NAMEPAR
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_SMART_STR_RESULT_MARKER_NAMEPAR)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Itt a v�ge.
            aadd(tokenList,;
                 C.RMARKER:copyFromToken(resultMarkerName,;
                                         TKID_SMART_STR_RESULT_MARKER,;
                                         nil))
            resultMarkerName:=nil
            state:=ST_TR_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_BLOCKIFY_RESULT_MARKER_START)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_NEV)
            resultMarkerName:=this:item
            state:=ST_TR_BLOCKIFY_RESULT_MARKER_NAME
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_BLOCKIFY_RESULT_MARKER_NAME)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr=="}")
            state:=ST_TR_BLOCKIFY_RESULT_MARKER_NAMEPAR
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_BLOCKIFY_RESULT_MARKER_NAMEPAR)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Itt a v�ge.
            aadd(tokenList,;
                 C.RMARKER:copyFromToken(resultMarkerName,;
                                         TKID_BLOCKIFY_RESULT_MARKER,;
                                         nil))
            resultMarkerName:=nil
            state:=ST_TR_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_LOGIFY_RESULT_MARKER_START)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_NEV)
            resultMarkerName:=this:item
            state:=ST_TR_LOGIFY_RESULT_MARKER_NAME
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_LOGIFY_RESULT_MARKER_NAME)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr==".")
            state:=ST_TR_LOGIFY_RESULT_MARKER_NAMEPAR
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      elseif (state==ST_TR_LOGIFY_RESULT_MARKER_NAMEPAR)
         if (ujsor)
             // V�ge van.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr==">")
            // Itt a v�ge.
            aadd(tokenList,;
                 C.RMARKER:copyFromToken(resultMarkerName,;
                                         TKID_LOGIFY_RESULT_MARKER,;
                                         nil))
            resultMarkerName:=nil
            state:=ST_TR_START
         else
            // B�rmi m�s => rossz.
            this:errorgen(this:item,HPRERR_SXTRANSLATE)
            exit
         endif
      else
         ? "HPARSER:parseRTranslate(): Ismeretlen �llapot: ",state
         errorlevel(1)
         quit
      endif
      this:rds()
   end while
   this:sorNyel()
   
return nil

//*******************************************************************
implement chkEndOfFile()
local file, line,pos,i

   // A spec erre nem mond hib�t (!)
   // return nil // Incompatibility.
   
   if (len(this:ifStack)<=0)
      // Minden rendben.
      return nil
   endif

   if (this:lastEos!=nil)
      file:=TOKEN.(this:lastEos):file
      line:=TOKEN.(this:lastEos):line
      pos :=TOKEN.(this:lastEos):pos
   else
      file:=this:name
      line:=0
      pos :=0
   endif
   for i:=1 to len(this:ifStack)
      // Sajnos a poz�ci�t egyenl�re nem t�roljuk.
      // Az�rt nem errorgen, mert itt nincs mindig token.
      this:addError(C.PRSERR:onew(HPRERR_ENDIFMISSING,nil,; // Nincs token.
                    file,line,pos))
   end for
   
return nil


//*******************************************************************
implement parseInclude(mnameToken)
// A #include elemz�je.
// A fil�spec ut�n mindent lenyel a sor v�g�ig.

local tkId,tkStr
local item,name
local ujsor
local w,wErrCode

   this:rds()
   while(this:item!=nil)    
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS
      if (ujsor)
         // Hib�s �s v�ge is van.
         this:errorgen(mnameToken,HPRERR_INCLUDE)
         exit
      elseif (tkId==TKID_URES)
         // Maradunk.
      elseif (tkId==TKID_STRING)
         // Ez a neve
         item:=this:item
         this:sorNyel()
         // name:=substr(tkStr,2,len(tkStr)-2)
         name:=tkStr
         if (0!=(w:=INCL.(this:inclObj):openIncludeFile(name)))
             if (w==1)
                wErrCode:=HPRERR_INCLUDEOPEN
             elseif (w==2)
                wErrCode:=HPRERR_INCLUDEFIND
             else
                wErrCode:=HPRERR_INCLUDENEST
             endif
             
             this:errorGen(item,wErrCode,{name})
         endif
         return nil
      else
         // Hib�s.
         this:errorgen(this:item,HPRERR_INCLUDE)
         exit
      endif
      this:rds()
   end while
   this:sorNyel()
   
return nil

//*******************************************************************

