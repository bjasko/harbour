/*
 * $Id$
 */

//*******************************************************************
// mparser.prg: A mparser oszt�ly implement�ci�ja.
// 1999, Csisz�r Levente

// A sorokban a makr�kat (define, command, xcommand, translate,
// xtranslate) helyettes�ti.
// A PARSER-t�l �r�k�l.
// A n�v �s a sor (lparser) elemz� �ltal k�sz�tett tokeneket v�r az 
// inputr�l. �ltal�ban a hparser ut�n van.


/*
   Algoritmus:
      Olvas az inputj�r�l addig, am�g el nem d�nti, hogy 
      helyettes�teni kell vagy nem. Ha igen, akkor a helyettes�t�s
      eredm�ny�t visszarakja az inputj�ra, ha pedig nem
      kellett helyettes�teni, akkor azt kirakja az outputra.
      Ezekut�n mindig nil-t ad olvas�sra.
*/

// #define DEBUG
//*******************************************************************
#include "debug.ch"


//*******************************************************************
#include "objgen.ch"

//*******************************************************************
#define _STRICT_PARENT_
#include "token.och"
#include "tokenst.och"
#include "edefdict.och"
#include "defdict.och"
#include "extrdict.och"
#include "xtrdict.och"
#include "reader.och"
#include "mmarker.och"
#include "tbuffer.och"
// #include "prtree.och"

#include "maltrset.och"
#include "rsmmarkr.och"
// #include "prserr.och"

//*******************************************************************
#include "cr_lf.ch"
#include "ctoken.ch"
// #include "prserr.ch"

//*******************************************************************
// �sszehasonl�tja a (tkId, tkStr)-t egy karaterrel.
#define eqTkChar(tkId,tkStr,aChar)         ((tkId)==TKID_CHAR .and.;
                                           (tkStr)==(aChar))
                                    
//*******************************************************************
// Megn�zi, hogy, ha a token TKID_CHAR, akkor a tkStr benne van-e
// az aString-ben.
#define eqTkInCharList(tkId,tkStr,aString) ((tkId)==TKID_CHAR .and.;
                                            (tkStr)$(aString))


#define PVT_IDX    1
#define PVT_NAME   2

//*******************************************************************
#define _MPARSER_PRG_
#define _IMPLEMENT_ONEW_

#include "mparser.och"

//*******************************************************************
// implement oinit(inputReader,name,defdict,errorStream)
implement oinit(inputReader,name,errorStream)
           
   super:oinit(inputReader,name,errorStream)
   // this:errorItem:=nil
   // this:defdict:=defdict
   
return this
   
//*******************************************************************
implement startMakroBuf(item)
   if (item==nil)
      this:makroBuf:={}
   else
      this:makroBuf:={item}
   endif
return nil

//*******************************************************************
implement rdsMakroBuf()
local w

   w:=this:rds()
   if (this:item!=nil)
      aadd(this:makroBuf,this:item)
   endif
return w
 
//*******************************************************************
implement unrdsMakroBuf(n)
local w,m

   w:=this:unrds(n)
   m:=len(this:makroBuf)-w
   asize(this:makroBuf,if(m>0,m,0))
   
return w

//*******************************************************************
implement readItem()
   outerr("MPARSER.o:readItem(): Ez a m�velet nem h�vhat�!",crlf())
return super:readItem()

#ifdef OLD
local w,tkId,tkStr
   
   /*
      Olvas a puffer-be:
      - ha n�v, �s az szerepel a define sz�t�rban, akkor
         akkor ind�t egy define elemz�t.
         Ez az elemz� elmenti a parserBuffer-t, majd elv�gzi
         a define objektum elemz�s�t.
   */

   while(nil==(w:=this:getParserBuffer()) .and.;
         nil!=(w:=super:readInput()) .and.;
         TOKEN.w:id==TKID_NEV)
                     
      this:item:=w
      // Ez itt nem j�het ki!
      outerr("mparser: readItem",crlf())
      this:putParserBuffer(w)
      this:parseFun()
      
   end while
return w
#endif

//*******************************************************************
implement parseFun(edefdict)
/*
 Ez v�gzi a t�nyleges elemz�st.
 A this:item-t elemzi, sz�ks�g eset�n m�g olvashat.
 Elemezi az inputon a <n�v>'('<param1>,...')' param�tereket. 
 Az edefdict-nek a <n�v>-hez tartoz� makr� defin�ci�nak kell
 lennie.

 Ret: {sikeres,itemLista}
 Ha sikeres volt, akkor a sikeres==.t., �s az itemLista a csere
    eredm�nye.
 Ha nem volt sikeres, akkor a sikeres==.f., �s az itemLista a
    beolvasott (el�reolvasott) itemek list�ja.
 A parserBufferben csak egy token lehet, ami az item-ben is
 van.
*/

// Mj.: Elemz�skor elfogadja az �res param�tereket is, ezeket csak a
//      helyettes�t�skor sz�rj�k ki. Ez az�rt van �gy, mert �res
//      param�ter �gy is lehet �res, hogy maga a token lista csupa
//      �res tokenb�l �ll.

local state,tkId,tkStr,ujsor
local tList,i
local success,params,currentParam
local parentStack
#define STF_START  "start"
#define STF_EXPR   "expr"
#define STF_PARENT "parent"
               
   tkId:=TOKEN.(this:item):id
   tkStr:=TOKEN.(this:item):str
   
   if (EDEFDICT.edefdict:params==nil)
      // Param�terek nincsenek.
      // Ez csak egy <n�v> -> <token1>,... t�pus� helyettes�t�s.
      this:unputParserBuffer()
      return {.t.,EDEFDICT.edefdict:change()}
   endif

   // Most elemezz�k a '('<param1>,...')' -t.   
                
   parentStack:={} // Itt vannak a z�r�jelek.
   state:=STF_START
   success:=.f.
   this:rds()
   while(this:item!=nil)    
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
             (tkId==TKID_CHAR .and. tkStr==";")
      
      if (state==STF_START)
         if (tkId==TKID_URES)
            // Maradunk
            // state:=state
         elseif (tkId==TKID_CHAR .and. tkStr=="(")
            // Kezd�dik az elemz�s.
            currentParam:={}
            params:={}
            state:=STF_EXPR
         else
            // V�ge. Ide tartozik a sorv�gjel is.
            exit
         endif
      elseif (state==STF_EXPR)
         if (tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
             (tkId==TKID_CHAR .and. tkStr==";"))
            // V�ge.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
            // state:=state
            aadd(currentParam,this:item)
         elseif (tkId==TKID_CHAR .and. tkStr==")")
            // V�ge a param�ter list�nak.
            aadd(params,currentParam)
            success:=.t.
            exit
         elseif (tkId==TKID_CHAR .and. tkStr==",")
            // �j param�ter kezd�dik.
            aadd(params,currentParam)
            currentParam:={}
            state:=STF_EXPR
         elseif (tkId==TKID_CHAR .and.;
                 (tkStr=="(" .or. tkStr=="{" .or. tkStr=="["))
            // El kell menni a k�vetkez� csuk�ig, k�zben figyelni,
            // hogy rendesen z�r�jelezve van-e.
            aadd(currentParam,this:item)
            aadd(parentStack,thisclass:getCloseParent(tkStr))
            state:=STF_PARENT
         else
            // Ez a param�terhez tartozik.
            aadd(currentParam,this:item)
         endif
      elseif (state==STF_PARENT)
         if (ujsor)
            // V�ge.
            exit
         elseif (tkId==TKID_URES)
            // Maradunk
            // state:=state
            aadd(currentParam,this:item)
         elseif (tkId==TKID_CHAR .and.;
                 (tkStr=="(" .or. tkStr=="{" .or. tkStr=="["))
            // El kell menni a k�vetkez� csuk�ig, k�zben figyelni,
            // hogy rendesen z�r�jelezve van-e.
            aadd(currentParam,this:item)
            aadd(parentStack,thisclass:getCloseParent(tkStr))
            // Maradunk
         elseif (tkId==TKID_CHAR .and. tkStr==alast(parentStack))
            aadd(currentParam,this:item)
            asize(parentStack,len(parentStack)-1)
            if (len(parentStack)==0)
               state:=STF_EXPR
            endif
         elseif (tkId==TKID_NEV)
            aadd(currentParam,this:item)
         else
            // Ez a param�terhez tartozik.
            aadd(currentParam,this:item)
         endif
      else
         ? "MPARSER:parseFun(): Ismeretlen �llapot: ",state
         errorlevel(1)
         quit
      endif
      this:rds()
   end while
   
   if (success .and.;
       len(params)==1 .and.;
       0==ascan(params[1],{|x| !TOKEN.x:id==TKID_URES}))
      asize(params,0)
   endif

   if (success .and. len(params)==len(EDEFDICT.edefdict:params))
      // Siker�lt az elemz�s, most meg kell csin�lni a cser�t.
      for i:=1 to len(params)
         params[i]:=thisclass:trimTokenList(params[i])
         if (empty(params[i]))
            return {.f.,this:arrayParserBuffer()}
         endif
      end for
      #ifdef DEBUG
      outerr(crlf())
      outerr("define change: ",EDEFDICT.edefdict:printStr(),crlf())
      for i:=1 to len(params)
         outerr("params[",i,"]: ")
         aeval(params[i],{|t| outerr(TOKEN.t:getStr())})
         outerr(crlf())
      end for
      #endif
      return {.t.,EDEFDICT.edefdict:change(params)}
   endif
   // Nem siker�lt az elemz�s.
return {.f.,this:arrayParserBuffer()}

//*******************************************************************
// implement printDefDict()
// return DEFDICT.(this:defdict):printStr()

//*******************************************************************
cimplement getCloseParent(aChar)
// Ha az aChar egy nyit� z�r�jel, akkor a a csuk� p�rj�t adja, 
// egy�bk�nt nil-t.

   if (aChar=="(")
      return ")"
   elseif (aChar=="{")
      return "}"
   elseif (aChar=="[")
      return "]"
   endif
return nil

//*******************************************************************
cimplement trimTokenList(tList)
// A tList elej�r�l �s v�g�r�l elt�vol�tja az �res tokeneket.
// Ret: a tList.
local i
 
   while(!empty(tList))
      if (TOKEN.(tList[1]):id!=TKID_URES)
         exit
      endif
      adel(tList,1)
      asize(tList,len(tList)-1)
   end while
   
   for i:=len(tList) to 1 step -1
      if (TOKEN.(tList[i]):id!=TKID_URES)
         asize(tList,i)
         exit
      endif
   end for

return tList

//*******************************************************************
cimplement parse(item,inputReader,name,defdict,edefdict,xtrdict,mi,errorStream,ujsor,trPrsAlg)
/*
 Itt az item csak TOKEN lehet vagy nil.
 Az inputReader-nek kell ind�tania az �j elemz�ket.
 
 Az algoritmus:
    Megn�zi a define sz�t�rban, ha megvan, akkor ind�t egy
    elemz�t r�, ha az sikeres, akkor visszat�r, ha nem sikeres, akkor
    visszateszi a beolvasott token sort az inputra, elkezdi ind�tani
    r� az xtranslate makr�kat. 
    Egy xtranslate makr� ind�t�sa:
    
       - Ind�tja a makr�t, ha sikeres, akkor visszat�r, ha nem, 
         akkor a token sort visszateszi az inputra.

    A maxim�lis hossz�s�g� tokensort meg kell �rizni, �s
    azt adni teljes sikertelens�g eset�n.
    M�sik lehet�s�g, hogy ravaszkodunk: beiktatunk egy �tmeneti
    puffert az inputunk el� (ez az mcontrol!), �s 
    azt adjuk sikertelens�g eset�n.
*/

// Nem teljesen korrekt, mert az elemz�t minden alkalommal legy�rtja.

local othis,w,match,xtrList,xcmList
local i,j
   
   if (nil==item)
      return {item}
   endif
      
   match:={.f.,{item}}
   // if (TOKEN.item:id==TKID_NEV .and.;
   //     nil!=(edefdict:=DEFDICT.(defdict):atKey(TOKEN.item:str)))
   if (edefdict!=nil)
       
      othis:=class:onew(inputReader,name,defdict,xtrdict,errorStream)
      othis:item:=item
      othis:putParserBuffer(item)
      match:=othis:parseFun(edefdict)
      // Az othis inputj�ra visszatett tokeneket itt ki kellene venni!
      // Persze most ilyen nem lehet, mert ebb�l az objektumb�l senki
      // sem olvas.
      if (match[1])
         // Az illesz�s sikeres volt.
         return match
      endif
   endif

   // #define XTR_SEQ
   if (trPrsAlg==TRPRA_SEQ)
      // Szekvenci�lis illeszt�s.
      xtrList:=XTRDICT.xtrdict:getExtrList(item)
      if (ujsor)
         xcmList:=XTRDICT.xtrdict:getExtrList(item,.t.)
         xtrList:=aconcatenate(xcmList,xtrList)
         //xtrList:=aconcatenate(xtrList,xcmList)
      endif
      
      // H�tulr�l el�re haladunk.
      for i:=len(xtrList) to 1 step -1
         // Az els� maga az item, ez�rt azt nem kell visszatenni.
         for j:=len(match[2]) to 2 step -1
            READER.inputReader:unread(match[2][j])
         end for
         othis:=class:onew(inputReader,name,defdict,xtrdict,errorStream)
         othis:item:=item
         othis:putParserBuffer(item)
         
         match:=othis:parseXtr(xtrList[i])
      
         // Az othis inputj�ra visszatett tokeneket itt ki kellene venni!
         // Persze most ilyen nem lehet, mert ebb�l az objektumb�l senki
         // sem olvas.
      
         if (match[1])
            // Az illesz�s sikeres volt.
            return match
         endif
      end for
   elseif (mi[1]!=0 .or. mi[2]!=0)
      // Fa bej�r�sos illeszt�s.
   
      // El�sz�r az xtranslate-eket illesztj�k.
      for j:=len(match[2]) to 2 step -1
         READER.inputReader:unread(match[2][j])
      end for
      if (mi[1]!=0)
         othis:=class:onew(inputReader,name,defdict,xtrdict,errorStream)
         othis:item:=item
         othis:putParserBuffer(item)
      
         match:=othis:parseXtrTree(XTRDICT.xtrdict:trdictTree,mi[1],.f.)
   
         // Az othis inputj�ra visszatett tokeneket itt ki kellene venni!
         // Persze most ilyen nem lehet, mert ebb�l az objektumb�l senki
         // sem olvas.
         
         if (match[1])
            // Az illesz�s sikeres volt.
            return match
         endif
      endif
      
      if (ujsor .and. mi[2]!=0)
         // Azut�n az xcommand-okat.
         for j:=len(match[2]) to 2 step -1
            READER.inputReader:unread(match[2][j])
         end for
         othis:=class:onew(inputReader,name,defdict,xtrdict,errorStream)
         othis:item:=item
         othis:putParserBuffer(item)
         
         match:=othis:parseXtrTree(XTRDICT.xtrdict:cmdictTree,mi[2],.t.)
         
         // Az othis inputj�ra visszatett tokeneket itt ki kellene venni!
         // Persze most ilyen nem lehet, mert ebb�l az objektumb�l senki
         // sem olvas.
         
         if (match[1])
            // Az illesz�s sikeres volt.
            return match
         endif
      endif
   // else
   //    for j:=len(match[2]) to 2 step -1
   //       READER.inputReader:unread(match[2][j])
   //    end for
   endif
return match

//*******************************************************************
static function nextLeftToken(o)
// Az o �gy n�z ki: {iLeft,leftTokenlist}
// Az iLeft mindig a beolvasand�ra mutat
local t
   
   while(o[1]<=len(o[2]))
      t:=o[2][o[1]]
      o[1]++
      if (TOKEN.t:id!=TKID_URES)
         return t
      endif
   end while
return nil

//*******************************************************************
static function addMatchParam(paramValues,idx,name,tokenList)
   if (paramValues[1]==PVT_IDX)
      if (len(paramValues[2])<idx)
         asize(paramValues[2],idx)
      endif
      if (paramValues[2][idx]==nil)
         paramValues[2][idx]:={tokenList}
      else
         aadd(paramValues[2][idx],tokenList)
      endif
   else
      aadd(paramValues[2],{name,tokenList})
   endif
return nil

//*******************************************************************
implement parseTokenList(leftTokenList,cmd4,paramValues,toEOL)
/*
 Ez v�gzi a t�nyleges elemz�st. Az input folyamot megpr�b�lja 
 illeszteni a leftTokeList-re.
 A this:item-t elemzi, sz�ks�g eset�n m�g olvashat.
 Amikor h�vjuk, akkor a parserBuffer-ben csak a this:item-nek szabad
 lennie.
 Ha a toEOL nem �res, akkor csak akkor sikeres az illeszt�s, ha
 a sor v�gig ment.
 
 Ha a cmd4 nem �res, akkor a neveket 4 hossz�ra v�gva hasonl�tja
 �ssze.

 Ret: 0: Ha nem siker�lt az illeszt�s.
         Ekkor a parserBuffer-ben vannak a beolvasott tokenek.

      1: Ha siker�lt az illeszt�s, de nem ment el�re. (Pl. egy �res
         alternat�va.

      2: Ha siker�lt az illeszt�s �s el�re is ment.
*/   

/*
   Megy�nk v�gig az extrdict bal oldal�n �s:
   
      - Ha normal token (nem match token �s nem maltrset), akkor
        annak illeszkednie kell, ha nem, akkor nem illeszkedett
        jelz�ssel visszat�r.
        
      - Ha match token, akkor arra ind�tunk egy k�l�n match 
        token elemz�t. Ez az elemz� megkapja a k�vetkez� tokent.
        
*/

// #define STX_START               "START"
#ifdef OLD
local tkId,tkStr,ujsor
local tList,i
local oLeftTList, leftToken, leftTkId, leftTkStr
local match
local w
local nUres//, utNemUresItem

   oLeftTList:={1,leftTokenList}
 
   // state:=STX_START
   // tokenList:={}
   if (nil==(leftToken:=nextLeftToken(oLeftTList)))
      // �res ==> Nem illeszkedik semmire.
      return .f.      
   endif

   nUres:=0
   // utNemUresItem:=this:item
   
   while(this:item!=nil)
      leftTkId:=TOKEN.leftToken:id
      leftTkStr:=TOKEN.leftToken:str
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
             (tkId==TKID_CHAR .and. tkStr==";")
      
      if (tkId==TKID_URES)
         // Maradunk
         // aadd(tokenList,this:item)
         nUres++
      elseif (ujsor)
         // V�ge. 
         // Illeszkedett, ha a leftToken egy alternat�va, mert r� az
         // �res is illeszkedik, nem illeszkedett, ha nem az.
         // Ha illeszkedett, akkor a v�g�r�l az �jsort �s az �reseket
         // vissza kell tenni az inputra.
         if (leftTkId==TKID_MALTERSET)
            this:unrds(nUres+1)
            // this:item:=utNemUresItem
            // Itt nem kell a toEOL-t vizsg�lni.
            return .t.
         endif
         exit
      else
         if (leftTkId==TKID_MALTERSET)
            this:mleftXMRToken(leftToken,cmd4,paramValues,nUres)
            match:=.t.
            this:makroBuf:=nil
         elseif (leftTkId==TKID_REGULAR_MATCH_MARKER)
            match:=this:mleftXRMMToken(leftToken,cmd4)
         elseif (leftTkId==TKID_WILD_MATCH_MARKER)
            match:=this:mleftXWMToken(leftToken)
         elseif (leftTkId==TKID_EXT_EXPR_MATCH_MARKER)
            match:=this:mleftXEEMToken(leftToken)
         elseif (leftTkId==TKID_LIST_MATCH_MARKER)
            match:=this:mleftXLMToken(leftToken,cmd4)
         elseif (leftTkId==TKID_RESTRICTED_MATCH_MARKER)
            match:=this:mleftXRSMMToken(leftToken,cmd4)
         else
            // Egy�b illeszked�s.
            match:=this:mleftXNToken(leftToken,cmd4)
            this:makroBuf:=nil
         endif
         nUres:=0
         if (!match)
            // Az illeszt�s nem siker�lt ==> V�ge.
            exit
         endif
         // Az illeszt�s siker�lt, az eredm�ny a tokenList-ben van,
         // megy�nk tov�bb.
         if (this:makroBuf!=nil)
            addMatchParam(paramValues,;
                          MMARKER.leftToken:mNum,;
                          MMARKER.leftToken:getName(),;
                          this:makroBuf)
         endif
         leftToken:=nextLeftToken(oLeftTList)
      endif
      if (nil==leftToken)
         // V�gig �rt�nk a tokenlist�n.
         // Ha a toEOL nem �res, akkor meg kell n�zni, hogy a sor 
         // v�g�n vagyunk-e vagy a sor v�g�ig csak �resek vannak-e.
         if (!empty(toEOL))
            this:rds()
            while(this:item!=nil)
               tkId:=TOKEN.(this:item):id
               tkStr:=TOKEN.(this:item):str
               ujsor:=tkId==TKID_UJSOR .or.;
                      tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
                      (tkId==TKID_CHAR .and. tkStr==";")
               if (tkId==TKID_URES)
                  // Megy�nk tov�bb.
                  nUres++
               elseif (ujsor)
                  this:unrds(nUres+1)
                  return .t.
               else
                  return .f.
               endif
               this:rds()
            end while
         endif
         return .t.
      endif
      this:rds()
   end while
   
return .f.
#endif

local oLeftTList,leftToken,retVal,gone

   oLeftTList:={1,leftTokenList}

   if (nil==(leftToken:=nextLeftToken(oLeftTList)))
      // �res ==> Nem illeszkedik semmire.
      return 0
   endif

   gone:=.f.   
   while(0!=(retVal:=matchLeftToken(this,leftToken,cmd4,paramValues,toEOL)))
      if (retVal==2)
         gone:=.t.
      endif
      if (nil==(leftToken:=nextLeftToken(oLeftTList)))
         if (nilLeftToken(this,toEOL))
            return if(gone,2,1)
         endif
         // Nem illeszkedett.
         return 0
      endif
      this:rds()
   end while

   // Az illeszt�s nem siker�lt.
return 0

//*******************************************************************
static function nilLeftToken(this,toEOL)
// Akkor kell h�vni, amikor a leftToken nil
local tkId,tkStr,ujsor,nUres
                       
   nUres:=0
   // V�gig �rt�nk a tokenlist�n.
   // Ha a toEOL nem �res, akkor meg kell n�zni, hogy a sor 
   // v�g�n vagyunk-e vagy a sor v�g�ig csak �resek vannak-e.
   if (!empty(toEOL))
      this:rds()
      while(this:item!=nil)
         tkId:=TOKEN.(this:item):id
         tkStr:=TOKEN.(this:item):str
         ujsor:=tkId==TKID_UJSOR .or.;
                tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
                (tkId==TKID_CHAR .and. tkStr==";")
         if (tkId==TKID_URES)
            // Megy�nk tov�bb.
            nUres++
         elseif (ujsor)
            this:unrds(nUres+1)
            return .t.
         else
            // Az illeszt�s nem siker�lt.
            return .f.
         endif
         this:rds()
      end while
   endif
return .t.

//*******************************************************************
static function matchLeftToken(this,leftToken,cmd4,paramValues,toEOL)
// Illeszt egy matchToken-t az inputra.
// A leftToken-nek nem nil-nek kell lennie.
// Ret: 0: Ha nem siker�lt az illeszt�s.
//         Ekkor a parserBuffer-ben vannak a beolvasott tokenek.
//
//      1: Ha siker�lt az illeszt�s, de nem ment el�re. (Pl. egy �res
//         alternat�va.
//
//      2: Ha siker�lt az illeszt�s �s el�re is ment.
local tkId,tkStr,ujsor
local tList,i
local oLeftTList, leftTkId, leftTkStr
local match
local w
local nUres
local gone:=.t.

   nUres:=0
   while(this:item!=nil)
      leftTkId:=TOKEN.leftToken:id
      leftTkStr:=TOKEN.leftToken:str
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
             (tkId==TKID_CHAR .and. tkStr==";")
      
      if (tkId==TKID_URES)
         // Maradunk
         // aadd(tokenList,this:item)
         nUres++
      elseif (ujsor)
         // V�ge. 
         // Illeszkedett, ha a leftToken egy alternat�va, mert r� az
         // �res is illeszkedik, nem illeszkedett, ha nem az.
         // Ha illeszkedett, akkor a v�g�r�l az �jsort �s az �reseket
         // vissza kell tenni az inputra.
         if (leftTkId==TKID_MALTERSET)
            this:unrds(nUres+1)
            // this:item:=utNemUresItem
            // Itt nem kell a toEOL-t vizsg�lni.
            // Illeszkedik, de nem ment el�re.
            return 1
         endif
         // Az illeszt�s nem siker�lt.
         return 0 //exit
      else
         if (leftTkId==TKID_MALTERSET)
            gone:=this:mleftXMRToken(leftToken,cmd4,paramValues,nUres)
            match:=.t.
            this:makroBuf:=nil
         elseif (leftTkId==TKID_REGULAR_MATCH_MARKER)
            match:=this:mleftXRMMToken(leftToken,cmd4)
         elseif (leftTkId==TKID_WILD_MATCH_MARKER)
            match:=this:mleftXWMToken(leftToken)
         elseif (leftTkId==TKID_EXT_EXPR_MATCH_MARKER)
            match:=this:mleftXEEMToken(leftToken)
         elseif (leftTkId==TKID_LIST_MATCH_MARKER)
            match:=this:mleftXLMToken(leftToken,cmd4)
         elseif (leftTkId==TKID_RESTRICTED_MATCH_MARKER)
            match:=this:mleftXRSMMToken(leftToken,cmd4)
         else
            // Egy�b illeszked�s.
            match:=this:mleftXNToken(leftToken,cmd4)
            this:makroBuf:=nil
         endif
         nUres:=0
         if (!match)
            // Az illeszt�s nem siker�lt ==> V�ge.
            return 0 // exit
         endif
         // Az illeszt�s siker�lt, az eredm�ny a tokenList-ben van,
         // megy�nk tov�bb.
         if (this:makroBuf!=nil)
            addMatchParam(paramValues,;
                          MMARKER.leftToken:mNum,;
                          MMARKER.leftToken:getName(),;
                          this:makroBuf)
         endif
         // Tov�bb.
         return if(gone,2,1)
         // leftToken:=nextLeftToken(oLeftTList)
      endif
      this:rds()
   end while

   // Egy olyan sor, aminek nincs sorv�gjele.
   if (leftTkId==TKID_MALTERSET)
      this:unrds(nUres+1)
      // this:item:=utNemUresItem
      // Itt nem kell a toEOL-t vizsg�lni.
      // Illeszkedik, de nem ment el�re.
      return 1
   endif
   // Az illeszt�s nem siker�lt.
return 0 //exit


//*******************************************************************
static function markParamValues(paramValues)
local mark,i

   mark:=array(len(paramValues[2]))
   for i:=1 to len(paramValues[2])
      if (paramValues[2][i]!=nil)
         mark[i]:=len(paramValues[2][i])
      endif
   end for
return mark

//*******************************************************************
static function restoreParamValues(paramValues,mark)
local i

   for i:=1 to len(mark)
      if (mark[i]==nil)
         paramValues[2][i]:=nil
      else
         asize(paramValues[2][i],mark[i])
      endif
   end for
   
   for i:=len(mark)+1 to len(paramValues[2])
      paramValues[2][i]:=nil
   endfor
      
return nil

//*******************************************************************
function parseDictTree(this,dictTree,cmd4,paramValues,toEOL,mi)
/*
   Elemez egy dictTree-t. V�gigpr�b�lja a fa minden �g�t.
   Elv�gzi a helyettes�t�st �s, hogy folytathassa az elemz�st, ha
   a helyettes�t�s nem v�gezhet� el.
   Ha az mi meg van adva, akkor az mi index� node-t�l indul.
   tree:=nodeList
   node:={token,nodeList[,extrDict]}
   nodeList:={node1,...}
   
   Ret: nil, ha nem siker�lt illeszteni, vagy nem lehetett 
             helyettes�teni.
        tokenList: A helyettes�t�s eredm�nye, ha siker�lt illeszteni
                   �s siker�lt helyettes�teni.
*/
local l,item,markParamValues
local i//,retVal
local extrDict,befejezheto
local mL,mItem,mMarkParamValues
local resultTkList

   l:=TBUFFER.(this:parserBuffer):bItemNumber()
   item:=this:item

   // Megy�nk a node-okon a legfels� szinten, ha tal�lunk 
   // illeszked�st, akkor a fa tov�bbi r�sz�t is illesztj�k, ha
   // nem, akkor visszal�p�nk.
   markParamValues:=markParamValues(paramValues)
   for i:=if(empty(mi),1,mi) to len(dictTree)
      if (0!=(/*retVal:=*/matchLeftToken(this,dictTree[i][1],cmd4,paramValues,toEOL)))
         // Illeszkedett
         // Megn�zz�k, hogy lehet-e tov�bb menni.
         befejezheto:=len(dictTree[i])>=3 .and. dictTree[i][3]!=nil
         if (len(dictTree[i][2])>0)
            // Megy�nk lefel� a f�ban ezen a node-on.
            if (befejezheto)
               mL:=TBUFFER.(this:parserBuffer):bItemNumber()
               mItem:=this:item
               mMarkParamValues:=markParamValues(paramValues)
            endif
            this:rds()
            if (nil!=(resultTkList:=parseDictTree(this,dictTree[i][2],cmd4,paramValues,toEOL)))
               // Siker�lt.
               return resultTkList
            endif
            if (befejezheto)
               // Vissza kell l�pni.
               this:unrds(TBUFFER.(this:parserBuffer):bItemNumber()-mL)
               this:item:=mItem
               restoreParamValues(paramValues,mMarkParamValues)
            endif
         endif

         // Megn�zz�k, hogy itt be lehet-e fejezni.
         if (befejezheto)
            // Be lehet fejezni!
            if (nilLeftToken(this,toEOL))
               extrDict:=dictTree[i][3]
               asize(paramValues[2],EXTRDICT.extrdict:numMatchMarkers)
               if (nil!=(resultTkList:=EXTRDICT.extrdict:change(paramValues[2])))
                  return resultTkList
               endif
               // Nem siker�lt az elemz�s.
            endif
            // Vissza kell l�pni.
         endif
         this:unrds(TBUFFER.(this:parserBuffer):bItemNumber()-l)
         this:item:=item
         restoreParamValues(paramValues,markParamValues)
         // Nem siker�lt.
      // elseif (retVal)
      //    // Siker�lt.
      //    // Ide nem j�het!
      //    outerr("parseDictTree: retVal==.t.: Ide nem j�het!",newline())
      //    this:unrds(TBUFFER.(this:parserBuffer):bItemNumber()-l)
      //    this:item:=item
      //    restoreParamValues(paramValues,markParamValues)
      //    return nil
      else
         // Itt vissza kell l�pni, �s �jra pr�b�lkozni.
         this:unrds(TBUFFER.(this:parserBuffer):bItemNumber()-l)
         this:item:=item
         restoreParamValues(paramValues,markParamValues)
      endif
   end while
   // A befejez�st itt nem kell n�zni, mert a legfels� szinten
   // nem lehet befejezni (az az �res szab�ly lenne), az als�bb
   // szinteken pedig a h�v� n�zi.
return nil
      
//*******************************************************************
implement parseXtr(extrdict)
/*
 Ez v�gzi a t�nyleges elemz�st.
 A this:item-t elemzi, sz�ks�g eset�n m�g olvashat.
 Elemezi az inputon az extrdict tokenjeit. 

 Ret: {sikeres,itemLista}
 Ha sikeres volt, akkor a sikeres==.t., �s az itemLista a csere
    eredm�nye.
 Ha nem volt sikeres, akkor a sikeres==.f., �s az itemLista a
    beolvasott (el�reolvasott) itemek list�ja.
 A parserBufferben csak egy token lehet, ami az item-ben is
 van.
*/   

local paramValues,w
#ifdef DEBUG
local i
#endif

   paramValues:={PVT_IDX,array(EXTRDICT.extrdict:numMatchMarkers)}
   if (0!=this:parseTokenList(EXTRDICT.extrdict:leftSide,;
                           EXTRDICT.extrdict:cmdType==XTRTYPE_COMMAND .or.;
                           EXTRDICT.extrdict:cmdType==XTRTYPE_TRANSLATE,;
                           paramValues,;
                           EXTRDICT.extrdict:cmdType==XTRTYPE_XCOMMAND .or.;
                           EXTRDICT.extrdict:cmdType==XTRTYPE_COMMAND))
      // Siker�lt az elemz�s, most meg kell csin�lni a cser�t.
      #ifdef DEBUG
      outerr(crlf())
      outerr("xtranslate change: ",EXTRDICT.extrdict:printStr(),crlf())
      // for i:=1 to len(tokenList)
      //    outerr("left[",i,"]: ",TOKEN.tokenList[i]:getStr(),crlf())
      // end for
      #endif
      if (nil!=(w:=EXTRDICT.extrdict:change(paramValues[2])))
         return {.t.,w}
      endif
   endif
   // Nem siker�lt az elemz�s.
return {.f.,this:arrayParserBuffer()}
   
//*******************************************************************
implement parseXtrTree(trdictTree,mi,toEOL)
/*
 Ez v�gzi a t�nyleges elemz�st.
 A this:item-t elemzi, sz�ks�g eset�n m�g olvashat.
 Elemezi az inputon az extrdict tokenjeit. 
 Az mi index� node-t�l indul.

 Ret: {sikeres,itemLista}
 Ha sikeres volt, akkor a sikeres==.t., �s az itemLista a csere
    eredm�nye.
 Ha nem volt sikeres, akkor a sikeres==.f., �s az itemLista a
    beolvasott (el�reolvasott) itemek list�ja.
 A parserBufferben csak egy token lehet, ami az item-ben is
 van.
*/   

local paramValues,w//,extrDict
#ifdef DEBUG
local i
#endif

   paramValues:={PVT_IDX,{}}
   if (nil!=(w:=parseDictTree(this,trdictTree,;
                          .f.,;
                          paramValues,;
                          toEOL,mi)))
      
      // Siker�lt az elemz�s, a cser�t a parserDict()-nek kell
      // csin�lnia, hogy tov�bb folytathassa az elemz�st, ha
      // a helyettes�t�st nem lehet elv�gezni.
      #ifdef OLD
      asize(paramValues[2],EXTRDICT.extrdict:numMatchMarkers)
      #ifdef DEBUG
      outerr(crlf())
      outerr("xtranslate change: ",EXTRDICT.extrdict:printStr(),crlf())
      // for i:=1 to len(tokenList)
      //    outerr("left[",i,"]: ",TOKEN.tokenList[i]:getStr(),crlf())
      // end for
      #endif
      if (nil!=(w:=EXTRDICT.extrdict:change(paramValues[2])))
         return {.t.,w}
      endif
      #endif
      return {.t.,w}
   endif
   // Nem siker�lt az elemz�s.
return {.f.,this:arrayParserBuffer()}
   
//*******************************************************************
function isMatchNToken(tkId,tkStr,mTkId,mTkStr,cmd4)
// Egy norm�l token illeszkedi-e egy norm�l (bal oldalon l�v�) 
// tokenre.
// Mj.: A tk �s az mTk nem cser�lhet� fel egym�ssal!

   if (tkId==TKID_NEV)
      return mTkId==TKID_NEV .and. compareWNames(tkStr,mTkStr,cmd4)
   elseif (tkId==TKID_STRING)
      return mTkId==TKID_STRING .and. compareWNames(tkStr,mTkStr,cmd4)
   endif
return tkId==mTkId .and. tkStr==mTkStr

//*******************************************************************
implement mleftXNToken(leftToken,cmd4)
/*
   Match left xtranslate normal token.
   Norm�l token (nem match �s nem malterset) elemz�.
   A this:item-ben lev� tokent vizsg�lja, m�g olvashat. 
   Ha az illeszt�s sikertelen, akkor a plusz beolvasott tokeneket
   visszateszi az inputra �s a this:item-et vissza�ll�tja.
*/
local tkId, tkStr

   tkId:=TOKEN.(this:item):id
   tkStr:=TOKEN.(this:item):str
   
   if (isMatchNToken(tkId,tkStr,;
                     TOKEN.leftToken:id,TOKEN.leftToken:str,;
                     TOKEN.leftToken:eqType/*cmd4*/))
      return .t.
   endif
return .f.

//*******************************************************************
static function connectParamValues(paramValues,wParamValues)
local i

   if (paramValues[1]==PVT_IDX)
      if (len(paramValues[2])<len(wParamValues[2]))
         asize(paramValues[2],len(wParamValues[2]))
      endif
      for i:=1 to len(wParamValues[2])
         if (wParamValues[2][i]!=nil)
            if (paramValues[2][i]==nil)
               paramValues[2][i]:=wParamValues[2][i]
            else
               aappend(paramValues[2][i],wParamValues[2][i])
            endif
         endif
      end for
   else
      aappend(paramValues[2],wParamValues[2])
   endif
return nil

//*******************************************************************
implement mleftXMRToken(leftToken,cmd4,paramValues,nUres)
// MALTERSET
/*
   Az algoritmus:
   
      Megy�nk az alternat�v�kon, �s megpr�b�ljuk illeszteni.
      Ha nem siker�lt, vissza az inputra a beolvasott tokeneket,
      �s vessz�k a k�vetkez�t.
      Ha siker�lt, akkor el�r�l kezdj�k v�gign�zni az alternat�v�kat.
      Mj.: Ez v�ltozhat
      Ha az alternat�v�k egyike sem illeszkedett, akkor tov�bbmegy�nk.
      Mj.: Az illeszt�s mindig sikeres, mert az �res is benne van.
           a lehet�s�gek k�z�tt, csak ilyenkor az olvasott
           tokent vissza kell tenni az inputra.
*/
local oldParserBuffer
local item,i,leftTokenList
local wParamValues
local gone,iGone

   gone:=.f.
   oldParserBuffer:=this:parserBuffer
   this:unrds()
   this:parserBuffer:=C.TBUFFER:onew()
   this:rds()
   item:=this:item
   leftTokenList:=MALTRSET.leftToken:alterset
   i:=1
   while(i<=len(leftTokenList))
      if (paramValues[1]==PVT_IDX)
         wParamValues:={paramValues[1],array(len(paramValues[2]))}
      else
         wParamValues:={paramValues[1],{}}
      endif
      if (2==(iGone:=this:parseTokenList(leftTokenList[i],cmd4,wParamValues)))
         // Siker�lt illeszteni.
         // A mostani parserBuffer-t hozz� kell adni a r�gihez.
         TBUFFER.oldParserBuffer:appendBuffer(this:parserBuffer)
         // A param�tereket hozz� kell adni a r�gihez.
         connectParamValues(paramValues,wParamValues)
         
         // �jra v�gign�zz�k a list�t.
         this:parserBuffer:=C.TBUFFER:onew()
         this:rds()   
         item:=this:item
         i:=1
         nUres:=0
         gone:=.t.
      else
         // A mostani parserBuffert pedig el kell dobni (a tartalm�t
         // visszatenni az inputra), a item-et pedig vissza kell 
         // �ll�tani.
         this:unrds(TBUFFER.(this:parserBuffer):bItemNumber()-1)
         this:item:=item
         i++
      endif
   end while
   // Nem lehet tov�bb menni, de az illeszt�s az�rt sikeres!
   this:unrds()
   this:parserBuffer:=oldParserBuffer
   // A buffer v�g�n lev� <space>-kat vissza kell rakni az
   // inputra.
   this:unrds(nUres)
return gone

//*******************************************************************
static function compareWNames(name,trName,cmd4)
// A name (ami az elemzend� sorban van) illeszkedik-e a translate
// bal oldal�n lev� token-re.
// Sajnos nem n�gy hosszan, hanem minimum 4 hosszan kell hasonl�tani,
// �s nem mindegy, hogy mi illeszkedik mire.

   // Ez kell!!!
   if (len(name)>len(trName))
      return .f.
   endif
   
   if (empty(cmd4) .or. len(name)<4)
      return lower(name)==lower(trName)
   endif
   
//   Mj.: Ez l�nyeg�ben egy '=' (�sszehasonl�t�s), de az '=' 
//        obsoleted.
return (lower(name)==lower(left(trName,len(name))))

//*******************************************************************
static function matchLookForward(tkId,tkStr,forwTkId,forwTkStr,forwT,cmd4)

   if (forwTkId==TKID_RESTRICTED_MATCH_MARKER)
      // A spec nem n�z el�re ebben az esetben, ez�rt ez nem kell.
      #ifdef OLD
      // Sajnos ez egyenl�re nem j�, mert a wordList-ben tokeneknek
      // kellene lennie, hogy a stringeket k�l�n kezelhess�k, 
      // ezenk�v�l az �resekn�l meg kellene �llni, etc.
      // De ez egyenl�re nincs kitesztelve.
      // Ez�rt egyenl�re a stringeket kihagyjuk.
      if (tkId==TKID_NEV .or.;
          tkId==TKID_SZAMTOMB .or.;
          ;//tkId==TKID_STRING .or.;
          tkId==TKID_CHAR)
         return 0!=ascan(RSMMARKR.forwT:wordList,;
                         {|x| compareWNames(tkStr,x,RSMMARKR.forwT:eqType/*cmd4*/)})
      endif
      #endif
      return .f.
   elseif (forwTkId==TKID_NEV .or. forwTkID==TKID_STRING)
      // A neveket �s a stringeket 'case insensitive' m�don kell 
      // �sszehasonl�tani.
      return tkId==forwTkId .and. compareWNames(tkStr,forwTkStr,RSMMARKR.forwT:eqType/*cmd4*/)
   endif

   // Itt m�g lehetnek csavar�sok pl. a sz�mokat �rt�k�k szerint, 
   // vagy a stringeket �gy, hogy nem sz�m�t mivel hat�rolt�k etc.
return tkId==forwTkId .and. tkStr==forwTkStr
   
//*******************************************************************
implement mleftXRMMToken(leftToken,cmd4)
/*
   Regular Match marker.
   Addig megy, am�g egy kifejez�s tart, vagy el nem �ri a 
   leftToken:nextToken-t.
   Sajnos a specifik�ci� nem k�vetkezetes: 
   Pl: Ez nem egy kifejez�s: 'a*+b', ez viszont igen: 'a*(+b)'

   A cmd4 az el�ren�z�shez kell.    
*/
local state

#define STXRMM_START       "start"
#define STXRMM_PARENT      "parent"

local tkId,tkStr,ujsor
local parentStack
// local iForwRead
local clfBuf,clfNumBuf,elvalaszt
local clf
local endToken,endTkId,endTkStr
               
   this:startMakrobuf(this:item)
   clfBuf:={}
   clfNumBuf:={}
        
   // iForwRead:=1
   
   // Itt ki kell sz�rni az '='-t, mert azzal kifejez�s nem kezd�dhet.
   // Kihaszn�ljuk, hogy this:item �jsor �s �res nem lehet.
   if (TOKEN.(this:item):id==TKID_CHAR .and.;
       TOKEN.(this:item):classify=='=')
      // Vajon itt kell unrds()?
      this:unrdsMakroBuf()
      return .f.
   endif

   endToken:=MMARKER.leftToken:nextToken
   if (MMARKER.leftToken:nextToken!=nil)
      endTkId:=TOKEN.(MMARKER.leftToken:nextToken):id
      endTkStr:=TOKEN.(MMARKER.leftToken:nextToken):str
   else
      endTkId:=nil
      endTkStr:=nil
   endif
   
   elvalaszt:=nil   
   state:=STXRMM_START
   while(this:item!=nil)
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
             (tkId==TKID_CHAR .and. tkStr==";")

      PDEBUG(outerr("mleftXRMMToken: ","state: ",state, "tkId: ",tkId, "tkStr: ", tkStr,crlf()))
      if (state==STXRMM_START)
         if (ujsor)
            // V�ge!
            exit
         elseif (matchLookForward(tkId,tkStr,endTkId,endTkStr,endToken,cmd4))
         // elseif (tkId==endTkId .and. tkStr==endTkStr)
            // Ez itt probl�m�s, mert nevekn�l, vagy stringekn�l nem 
            // tudjuk, hogyan kell az �sszehasonl�t�st csin�lni.
            // (case sensitive, nevekn�l n�gy karakter hosszan is
            // lehet (translate,command)
            // V�ge!
            // Mj.: Nem biztos, hogy j� a clBuf-beli visszal�p�seket 
            // kezelni kellene
            exit
         elseif (eqTkInCharList(tkId,tkStr,','))
            // V�ge!
            exit
         elseif (eqTkInCharList(tkId,tkStr,')}]'))
            // V�ge!
            exit
         elseif (tkId==TKID_URES)
            // Maradunk.
         elseif (eqTkInCharList(tkId,tkStr,'({['))
            // El�sz�r megn�zz�k, hogy j�het-e itt z�r�jel.
            aadd(clfBuf,TKCL_PARENT)
            // Amikor kij�n a z�r�jelb�l, ide be fogja �rni a helyes
            // sz�mot.            
            aadd(clfNumBuf,len(this:makroBuf))
            if (nil!=(elvalaszt:=exprChk(clfBuf)))
               // Nem j�het.
               exit
            endif
            state:=STXRMM_PARENT
            parentStack:={thisclass:getCloseParent(tkStr)}
         elseif (nil!=(clf:=TOKEN.(this:item):classify))
            // Hozz�vessz�k a clfBuf-hoz, �s megn�zz�k, hogy
            // j�het-e.
            if (clf=='++' .and. len(clfBuf)>0 .and. atail(clfBuf)=='++')
               clfNumBuf[len(clfNumBuf)]:=len(this:makroBuf)
            else
               aadd(clfBuf,clf)
               aadd(clfNumBuf,len(this:makroBuf))
               if (nil!=(elvalaszt:=exprChk(clfBuf)))
                  // Nem j�het. 
                  // Az elvalaszt-n�l van a kifejez�shat�r.
                  exit
               endif
            endif
         else
            // Ide nem j�het�nk, egyenl�re elengedj�k, b�rmi is van 
            // itt.
         endif
      elseif (state==STXRMM_PARENT)
         // Mj.: A clfNumBuf v�g�n a makr�buf hossz�t az�rt kell
         //      vezetni, hogy az �res helyeket a v�g�n le tudjuk 
         //      v�gni.
         if (ujsor)
            // V�ge! 
            exit
         elseif (tkId==TKID_URES)
            // Maradunk.
         elseif (eqTkInCharList(tkId,tkStr,'({['))
            aadd(parentStack,thisclass:getCloseParent(tkStr))
            clfNumBuf[len(clfNumBuf)]:=len(this:makroBuf)
         elseif (eqTkInCharList(tkId,tkStr,')}]'))
            // Ha nem illeszkedik a stack-re, akkor nem vessz�k
            // figyelembe, ha illeszkedik, akkor a stack-r�l 
            // levessz�k a legfels� elemet.
            clfNumBuf[len(clfNumBuf)]:=len(this:makroBuf)
            if (atail(parentStack)==tkStr)
               adrop(parentStack)
               if (empty(parentStack))
                  state:=STXRMM_START
               endif
            endif
         else
            // B�rmi m�s, folytat�dik az elemz�s.
            clfNumBuf[len(clfNumBuf)]:=len(this:makroBuf)
         endif
      else
         ? "MPARSER:mleftXRMMToken(): Ismeretlen �llapot: ",state
         errorlevel(1)
         quit
      endif
      this:rdsMakroBuf()
      // iForwRead++
   end while
        
   if (elvalaszt==nil)
      // Sor v�ge, vessz�, etc.
      // Ha a clfBuf nem �res, akkor az utols�ra l�p�nk vissza, ha 
      // �res, nem illeszkedett!
      if (len(clfBuf)==0)
         this:unrdsMakroBuf(len(this:makroBuf))
         return .f.
      endif
      // M�g meg kell n�zni, hogy van-e kifejez�shat�r, �gy, hogy
      // nem lehet folytatni.
      aadd(clfBuf,"newline")
      aadd(clfNumBuf,len(this:makroBuf))
      if (nil==(elvalaszt:=exprChk(clfBuf)))
         elvalaszt:=len(clfBuf)-1
      endif
   endif   
   // Ilyenkor az clfNumBuf[elv�laszt] azt mondja meg, hogy hova
   // kell visszal�pn�nk. Ennyi tokennek kell maradnia a 
   // makroBuf-ban.
   this:unrdsMakroBuf(len(this:makroBuf)-clfnumBuf[elvalaszt])
   // aappend(tokenList,this:makroBuf)
   
return !empty(this:makroBuf)

//*******************************************************************
#ifdef OLD
// Ez eg�sz j�l m�k�dik, de sokat elront.
// implement mleftXRMMToken(leftToken,tokenList)
// /*
//    Regular Match marker.
//    Addig megy, am�g egy kifejez�s tart, vagy el nem �ri a 
//    leftToken-ben megadott lez�r� tokent.
//    Sajnos a specifik�ci� nem k�vetkezetes: 
//    Pl: Ez nem egy kifejez�s: 'a*+b', ez viszont igen: 'a*(+b)'
//     
// */
// 
// /*
//    A kifejez�s elemz�s a k�vetkez�k�ppen megy:
//    
//    Az elemek:
//       <Kifejez�s token>: olyan token, ami meg�llja a hely�t �n�ll�
//                          kifejez�sk�nt.
//       <El�revetett un�ris oper�tor>:
//                        '&','++','--','+','-','@','.not.','!'
//       <H�travetett un�ris oper�tor>:
//                        '--','++'
//       <bin�ris oper�tor>:
//                       '$','%','*','**','^','+','-','->','.and.','.or.',
//                       '/',':',':=',
//                       '<','<=','<>','!=','#','=','==','>','>='
//                       
//       Speci�lis dolgok:
//         <sz�m>['.'[<sz�m>]]
//         
// 
//       Helyettes�t�sek: 
//          '.not.' -> '!'
//          '**'    -> '^'
//          
//       Megjegyz�sek: 
//            
//       - A <sz�m>.and. kifejez�sben az els� '.' a .and.-hoz 
//         tartozik, mert a .and. foglalt n�v. Ugyanez vonatkozik 
//         a '.or.','.not.','.t.','.f.'-re. A kis �s a nagybet�ket
//         nem k�l�nb�zteti meg.
//            
//       - A k�t '&' jel ugyan�gy megjegyz�s, mint a '//'
//       
//       - Egy oper�tornak tekinti a k�vetkez� tokenek tetsz. 
//         kombin�ci�j�t: 
// 
//         '&','++','--','@','$','->','.and.','.or.','.not.','!',':=',
//         '<','<=','<>','!=','#','=','==','>','>=','.'
//          
//         Mj.: A fenti oper�torokat un�ris oper�tornak tekinti.
//             
// 
//         Mj2.: A fentiek k�z�l a k�vetkez�k maradtak ki:
//               '+','-','*','**','^','/','%'
//         
//       - Speci�lisan kezelt tokenek:
//         '+': Csak bin�ris lehet, el�jel nem lehet.
//         '-': Bin�ris �s el�jel lehet.
//         '*','**', '^', '/','%': csak bin�ris oper�torok lehetnek.
//            Mj.: A '*','/'-nek van egy speci�lis esete l�sd 'Speci�lis
//                 esetek'.
//            
//       - Speci�lis esetek:
//         <b�rmi> '*' '/' <n�v> : Ezt elfogadja egy kifejez�snek, de
//         pl.; a '*' '/' '*' '/'-t m�r nem. 
//         Mj.: A <n�v> hely�n nem �llhat semmit m�s, m�g �j sor sem!
//         
//       - Az illeszt�st �s az xtranslate elemz�st is tokenesen 
//         csin�lja (argh...) Pl.: ez a k�t xtranslate parancs k�l�nb�zik
//         egym�st�l:
//         
//         #xtranslate HUHU <a> :=  <b> => let(@<a>,<b>)
//         #xtranslate HUHU <a> : = <b> => let(@<a>,<b>)
// 
//         Mj.: A m�sodikat el sem fogadja 'hi�nyz� =>' hibajelz�ssel.
// 
//  
//         
//    Lehets�ges, hogy a kifejez�shat�rok oldal�r�l jobban meg lehet fogni
//    a dolgot:
//    
//    <bin�ris oper�tor>:='*'|'**'|'^'|'/'|'%'
//    
//    <Z�r�jelezett kifejez�s>: '()', '{}','[]' k�z�tt lev� tetsz karakter
//                            sorozat. Az z�r�jeleken bel�l soha nincs 
//                            kifejez�shat�r.
// 
//    <p�r n�lk�li csuk� z�r�jel>: ')', '}',']' karater, amihez nincs 
//                                 megfelel� nyit� z�r�jel.
//    
//    Ekkor a kifejez�s hat�rok:
//    
//    <sz�m vagy n�v> <kifejez�shat�r> <sz�m vagy n�v>
//    '+' <kifejez�shat�r> <�res> '+'
//    '+' <kifejez�shat�r> <bin�ris oper�tor>
//    '-' <kifejez�shat�r> <�res> '+'
//    '-' <kifejez�shat�r> <bin�ris oper�tor>
//    '-' <kifejez�shat�r> <�res> '-' <�res> '-'
//    <Z�r�jelezett kifejez�s> <kifejez�shat�r> <n�v vagy sz�m>
//    <kifejez�shat�r> <p�r n�lk�li z�r�jel>
//    <bin�ris oper�tor> <kifejez�shat�r> <bin�ris oper�tor>
//       Kiv�tel: '*' '/' <n�v> // Itt nincs kifejez�s hat�r.
//    <bin�ris oper�tor> <kifejez�shat�r> '+'
//    <bin�ris oper�tor> <kifejez�shat�r> <nem '-',<sz�m>,<n�v>,<nyit� z�r�jel>.>
//    <kifejez�shat�r> ','
//    
//    Speci�lis esetek: 
//    
//    
//    '~': Ezt t�rli (?)
//    '`': Ezt string hat�rol�nak (") tekinti.
//    '.','|': Ezeket nem tekinti hat�rol�nak.
//    '&&': Ez egysoros megjegyz�s, olyan, mint a '//'
//          
//    B�rmilyen egy�b esetben nincs kifejez�s hat�r.
// 
// */
// 
// local state
// 
// #define isTkIdNevSzam(tkId)  ((tkId)==TKID_NEV .or.;
//                               (tkid)==TKID_SZAMTOMB)
//                             
// #define isTkIdLiteral(tkId)  (isTkIdNevSzam(tkId) .or.;
//                              (tkid)==TKID_STRING)
//                             
// #define isTkClassify(tkId,token,clsfy)  ((tkId)==TKID_CHAR .and.;
//                                          TOKEN.(token):classify==(clsfy))
//                             
// #define STXRMM_XSTART      "xstart"
// #define STXRMM_START       "start"
// #define STXRMM_LITERAL     "literal"
// #define STXRMM_PLUS        "plus"
// #define STXRMM_MINUS       "minus"
// #define STXRMM_MINUS2      "minus2"
// #define STXRMM_PARENT      "parent"
// #define STXRMM_CSILLAG     "csillag"
// #define STXRMM_CSILLAGPER  "csillagper"
// #define STXRMM_BINARY      "binary"
//            
// local tkId,tkStr,ujsor
// local parentStack
// local iForwRead
// local clfBuf
//                
//    iForwRead:=1
//    
//    this:startMakrobuf(this:item)
//    state:=STXRMM_XSTART
//    clfBuf:={}
//    
//    while(this:item!=nil)
//       tkId:=TOKEN.(this:item):id
//       tkStr:=TOKEN.(this:item):str
//       ujsor:=tkId==TKID_UJSOR .or.;
//              tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
//              (tkId==TKID_CHAR .and. tkStr==";")
// 
//       PDEBUG(outerr("state: ",state, "tkId: ",tkId, "tkStr: ", tkStr,crlf()))
//       if (state==STXRMM_XSTART)
//          if (ujsor)
//             // V�ge!
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (isTkClassify(tkId,this:item,'='))
//             // V�ge!
//             exit
//          else
//             iForwRead:=0
//             this:unrdsMakroBuf()
//             state:=STXRMM_START
//          endif
//       elseif (state==STXRMM_START)
//          if (ujsor)
//             // V�ge!
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (isTkIdLiteral(tkId))
//             iForwRead:=0
//             state:=STXRMM_LITERAL
//          elseif (eqTkInCharList(tkId,tkStr,'({['))
//             iForwRead:=0
//             state:=STXRMM_PARENT
//             parentStack:={thisclass:getCloseParent(tkStr)}
//          elseif (eqTkInCharList(tkId,tkStr,')}]'))
//             // V�ge (!)
//             exit
//          elseif (eqTkChar(tkId,tkStr,','))
//             // V�ge (!)
//             exit
//          elseif (isTkClassify(tkId,this:item,'!'))
//             // Ezut�n soha nincs kifejez�s hat�r.
//             iForwRead:=0
//             // Maradunk.
//          elseif (isTkClassify(tkId,this:item,'@'))
//             // Ezut�n soha nincs kifejez�s hat�r.
//             iForwRead:=0
//             // Maradunk.
//          elseif (isTkClassify(tkId,this:item,'%'))
//             // Hasonl� a '/'-hez �s a '*'-hoz, de nincs kiv�tel.
//             // Ut�na j�het: '&', '- nev', '- sz�m', 'nev','szam',
//             //              'z�r�jel', '.t.'
//             iForwRead:=0
//             // state:=STXRMM_PERCENT
//             state:=STXRMM_BINARY
//          elseif (isTkClassify(tkId,this:item,'^'))
//             // Hasonl� a '/'-hez �s a '*'-hoz, de nincs kiv�tel.
//             // Ut�na j�het: '&', '- nev', '- sz�m', 'nev','szam',
//             //              'z�r�jel', '.t.'
//             iForwRead:=0
//             // state:=STXRMM_POW
//             state:=STXRMM_BINARY
//          elseif (isTkClassify(tkId,this:item,'&'))
//             // Mindent elfogad.
//             iForwRead:=0
//          elseif (eqTkChar(tkId,tkStr,'*'))
//             // A '*' '/' <name> speci�lis eset kezel�se.
//             iForwRead:=0
//             state:=STXRMM_CSILLAG
//             PDEBUG(outerr("start->csillag"+crlf()))
//          elseif (eqTkChar(tkId,tkStr,'-'))
//             iForwRead:=0
//             state:=STXRMM_MINUS
//          elseif (eqTkChar(tkId,tkStr,'+'))
//             iForwRead:=0
//             state:=STXRMM_PLUS
//          elseif (isTkClassify(tkId,this:item,'='))
//             // Mivel nem ez az els� karakter, b�rmit el lehet 
//             // fogadni.
//             iForwRead:=0
//             // Maradunk.
//          /*
//          elseif (isTkClassify(tkId,this:item,'/'))
//          elseif (isTkClassify(tkId,this:item,'++'))
//          elseif (tkId==TKID_NEV)
//          elseif (tkId==TKID_SZAMTOMB)
//          elseif (tkId==TKID_STRING)
//          */
//          else
//             // B�rmi m�s, maradunk.
//             iForwRead:=0
//          endif
//       elseif (state==STXRMM_LITERAL)
//          if (ujsor)
//             // V�ge!
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (isTkIdLiteral(tkId))
//             // V�ge
//             exit
//          else
//             // B�rmi m�s: �jra fel kell dolgozni.
//             iForwRead--
//             this:unrdsMakroBuf()
//             state:=STXRMM_START
//          endif
//       elseif (state==STXRMM_PLUS)
//          if (ujsor)
//             // V�ge!
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (eqTkChar(tkId,tkStr,'+'))
//             // V�ge.
//             exit
//          elseif (thisclass:isTkBinaryOp(tkId,tkStr))
//             // *, **, ^,/,%
//             // V�ge.
//             exit
//          else
//             // B�rmi m�s: �jra fel kell
//             // dolgozni.
//             iForwRead--
//             this:unrdsMakroBuf()
//             state:=STXRMM_START
//          endif
//       elseif (state==STXRMM_MINUS)
//          if (ujsor)
//             // V�ge!
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (eqTkChar(tkId,tkStr,'+'))
//             // V�ge.
//             exit
//          elseif (thisclass:isTkBinaryOp(tkId,tkStr))
//             // *, **, ^,/,%
//             // V�ge.
//             exit
//          elseif (eqTkChar(tkId,tkStr,'-'))
//             state:=STXRMM_MINUS2
//          else
//             // B�rmi m�s: �jra fel kell dolgozni.
//             iForwRead--
//             this:unrdsMakroBuf()
//             state:=STXRMM_START
//          endif
//       elseif (state==STXRMM_MINUS2)
//          if (ujsor)
//             // V�ge! 
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (isTkIdNevSzam(tkId)) // A string itt nem j�!
//             // Elmegy.
//             iForwRead:=0
//             state:=STXRMM_LITERAL
//          else
//             // B�rmi m�s: kifejez�shat�r az els� minusz ut�n!
//             exit
//          endif
//       elseif (state==STXRMM_PARENT)
//          if (ujsor)
//             // V�ge! 
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (eqTkInCharList(tkId,tkStr,'({['))
//             iForwRead:=0
//             aadd(parentStack,thisclass:getCloseParent(tkStr))
//          elseif (eqTkInCharList(tkId,tkStr,')}]'))
//             // Ha nem illeszkedik a stack-re, akkor nem vessz�k
//             // figyelembe, ha illeszkedik, akkor a stack-r�l 
//             // levessz�k a legfels� elemet.
//             iForwRead:=0
//             if (atail(parentStack)==tkStr)
//                adrop(parentStack)
//                if (empty(parentStack))
//                   state:=STXRMM_START
//                endif
//             endif
//          else
//             // B�rmi m�s, folytat�dik az elemz�s.
//             iForwRead:=0
//          endif
//       elseif (state==STXRMM_CSILLAG)
//          if (ujsor)
//             // V�ge! 
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (eqTkChar(tkId,tkStr,'/'))
//             state:=STXRMM_CSILLAGPER
//             PDEBUG(outerr("csillag->csillagper"+crlf()))
//          else
//             // Az aktu�lis item-et a binaris oper�torok elemz�j�vel 
//             // kell elemeztetni.
//             iForwRead--
//             this:unrdsMakroBuf()
//             state:=STXRMM_BINARY
//          endif
//       elseif (state==STXRMM_CSILLAGPER)
//          if (ujsor)
//             // V�ge! 
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (tkId==TKID_NEV)
//             // Ok. Elker�lt�k a kifejez�shat�rt.
//             // Gyors�tunk, egyb�l a liter�lhoz megy�nk.
//             iForwRead:=0
//             state:=STXRMM_LITERAL
//             PDEBUG(outerr("csillagper->literal"+crlf()))
//          else
//             // A '*' �s a '/' k�z�tt van a kifejez�shat�r.
//             exit
//          endif
//       elseif (state==STXRMM_BINARY)
//          if (ujsor)
//             // V�ge! 
//             exit
//          elseif (tkId==TKID_URES)
//             // Maradunk.
//          elseif (tkId==TKID_NEV)
//             // Ok. Elker�lt�k a kifejez�shat�rt.
//             iForwRead:=0
//             state:=STXRMM_LITERAL
//          else
//             // A '*' �s a '/' k�z�tt van a kifejez�shat�r.
//             exit
//          endif
//       else
//          ? "MPARSER:mleftXRMMToken(): Ismeretlen �llapot: ",state
//          errorlevel(1)
//          quit
//       endif
//       this:rdsMakroBuf()
//       iForwRead++
//    end while
//         
//    this:unrdsMakroBuf(iForwRead)
//    aappend(tokenList,this:makroBuf)
//    
// return !empty(this:makroBuf)
#endif

//*******************************************************************
implement mleftXWMToken(leftToken)
/*
   Wild matchmarker.
   
   Egy '()' k�z� z�rt sorozat, vagy a leghosszabb space mentes
   sorozat.
   Mj.: Ha '('-al kezd�dik, �s nincs lez�rva, akkor is '()'-nek veszi.
   Mj2.: A t�bbi z�r�jelet nem veszi figyelembe.
   Mj3.: Nem n�z el�re.
    
*/
local tkId,tkStr,ujsor
               
   this:startMakrobuf(this:item)
   
   while(this:item!=nil)
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
             (tkId==TKID_CHAR .and. tkStr==";")

      PDEBUG(outerr("mleftXWMToken: "+"tkId: ",tkId, "tkStr: ", tkStr,crlf()))
      if (ujsor)
         // V�ge!
         this:unrdsMakroBuf()
         exit
      endif
      this:rdsMakroBuf()
   end while
        
return !empty(this:makroBuf)


//*******************************************************************
implement mleftXEEMToken(leftToken)
/*
   Extended Expression match marker.
   
   Egy '()' k�z� z�rt sorozat, vagy a leghosszabb space mentes
   sorozat.
   Mj.: Ha '('-al kezd�dik, �s nincs lez�rva, akkor is '()'-nek veszi.
   Mj2.: A t�bbi z�r�jelet nem veszi figyelembe.
   Mj3.: Nem n�z el�re.
    
*/
local state

#define STXEEM_START       "start"
#define STXEEM_PARENT      "parent"
#define STXEEM_SOROZAT     "sorozat"

local tkId,tkStr,ujsor
local numParent
               
   this:startMakrobuf(this:item)
   
   state:=STXEEM_START
   while(this:item!=nil)
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
             (tkId==TKID_CHAR .and. tkStr==";")

      PDEBUG(outerr("mleftXEEMToken: ","state: ",state, "tkId: ",tkId, "tkStr: ", tkStr,crlf()))
      if (state==STXEEM_START)
         if (ujsor)
            // V�ge!
            this:unrdsMakroBuf()
            exit
         elseif (tkId==TKID_URES)
           // Lenyeli
         elseif (eqTkInCharList(tkId,tkStr,'('))
            // Z�r�jelezett m�d.
            numParent:=1
            state:=STXEEM_PARENT
         else
            // Space n�lk�li sorozat m�d
            state:=STXEEM_SOROZAT
         endif
      elseif (state==STXEEM_PARENT)
         if (ujsor)
            // V�ge! 
            this:unrdsMakroBuf()
            exit
         elseif (eqTkInCharList(tkId,tkStr,'('))
            numParent++
         elseif (eqTkInCharList(tkId,tkStr,')'))
            numParent--
            if (numParent<=0)
               exit
            endif
         else
            // B�rmi m�s, folytat�dik az elemz�s.
            // Ebben benne van az �res is.
         endif
      elseif (state==STXEEM_SOROZAT)
         if (ujsor)
            // V�ge! 
            this:unrdsMakroBuf()
            exit
         elseif (tkId==TKID_URES)
            // V�ge! 
            this:unrdsMakroBuf()
            exit
         else
            // B�rmi m�s, folytat�dik az elemz�s.
         endif
      else
         ? "MPARSER:mleftXEEMToken(): Ismeretlen �llapot: ",state
         errorlevel(1)
         quit
      endif
      this:rdsMakroBuf()
   end while
        
return !empty(this:makroBuf)

//*******************************************************************
implement mleftXLMToken(leftToken,cmd4)
// List match marker
// Az �res param�tert (,,) megengedi.
local params:={}
local iForwRead
local tkId,tkStr,ujsor


   this:mleftXRMMToken(leftToken,cmd4)
   this:rds()
   aadd(params,this:makroBuf)
             
   iForwRead:=1
   
   while(this:item!=nil)
      tkId:=TOKEN.(this:item):id
      tkStr:=TOKEN.(this:item):str
      ujsor:=tkId==TKID_UJSOR .or.;
             tkId==TKID_BOS .or. tkId==TKID_EOS .or.;
             (tkId==TKID_CHAR .and. tkStr==";")
   
      if (ujsor)
         // V�ge!
         exit
      elseif (tkId==TKID_URES)
         // Maradunk.
      elseif (eqTkChar(tkId,tkStr,','))
         this:rds()
         if (this:mleftXRMMToken(leftToken,cmd4))
            aadd(params,this:makroBuf)
         else
            aadd(params,{})
         endif
         iForwRead:=0
      else
         // B�rmi m�s, v�ge.
         exit
      endif
      iForwRead++
      this:rds()
   end while
   
   this:unrds(iForwRead)
   if (len(params)==1 .and. len(params[1])==0)
      return .f.
   endif
   this:makroBuf:=params
return .t.

//*******************************************************************
implement mleftXRSMMToken(leftToken,cmd4)
/*
   Restricted match marker.
   
   Csan nevekre, sz�mokra �s karakterekre illeszt�nk.
*/
local tkId, tkStr,wl,i

   this:startMakrobuf(this:item)
   
   tkId:=TOKEN.(this:item):id
   tkStr:=TOKEN.(this:item):str
   
   if !(tkId==TKID_NEV .or.;
       tkId==TKID_SZAMTOMB .or.;
       ;//tkId==TKID_STRING .or.;
       tkId==TKID_CHAR)
      return .f.
   endif
   
   wl:=RSMMARKR.leftToken:wordList
   if (empty(wl))
      return .f.
   endif
   
   for i:=1 to len(wl)
      if (empty(wl[i]))
         this:unrdsMakrobuf()
         return .t.
      endif
      if (compareWNames(tkStr,wl[i],TOKEN.leftToken:eqType/*cmd4*/))
         return .t.
      endif
   end for
return .f.

//*******************************************************************
/*
cimplement isTkBinaryOp(tkId,tkStr)
// Ez a nem tokeniz�lt v�ltozat.

   if (!tkId==TKID_CHAR)
      return .f.
   endif
   
return tkStr$"*^/%"
*/   
//*******************************************************************


