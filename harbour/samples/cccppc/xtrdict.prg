/*
 * $Id$
 */

//*******************************************************************
// xtrdict.prg: #xtranslate sz�t�r
// 1999, Csisz�r Levente

//*******************************************************************
#include "ctoken.ch"
#include "objgen.ch"

//*******************************************************************
#include "token.och"
#include "extrdict.och"
#include "mmarker.och"

//*******************************************************************

#define _XTRDICT_PRG_
// #define _IMPLEMENT_ONEW_

#include "xtrdict.och"


//*******************************************************************
implement oinit()

   super:oinit()
   
   this:cmdict:={}
   this:trdict:={}
   this:cmdictTree:={}
   this:trdictTree:={}
return nil

//*******************************************************************
implement add(extrdict)
   if (EXTRDICT.extrdict:cmdType==XTRTYPE_XCOMMAND .or.;
       EXTRDICT.extrdict:cmdType==XTRTYPE_COMMAND)
      aadd(this:cmdict,extrdict)
      addToTree(this:cmdictTree,extrdict)
   else
      aadd(this:trdict,extrdict)
      addToTree(this:trdictTree,extrdict)
   endif
return nil

//*******************************************************************
implement printStr(printBlock,xcmd)
local i,w
local str:=""

   if (!empty(xcmd))

      for i:=1 to len(this:cmdict)
         w:=toStr(i)+": "+EXTRDICT.(this:cmdict[i]):printStr()
         if (printBlock!=nil)
            eval(printBlock,w)
         else
            str+=w+newline()
         endif
      end for
   else
      for i:=1 to len(this:trdict)
         w:=toStr(i)+": "+EXTRDICT.(this:trdict[i]):printStr()
         if (printBlock!=nil)
            eval(printBlock,w)
         else
            str+=w+newline()
         endif
      end for
   endif
return str

//*******************************************************************
implement getExtrList(item,xcmd)
// Egyenl�re az �sszes extrdict-et adja, k�s�bb majd gyors�tunk.
   if (!empty(xcmd))
      return this:cmdict
   endif
return this:trdict
//*******************************************************************

//*******************************************************************
//
//                          Fa �p�t�s
//
//*******************************************************************

//*******************************************************************
static function eqNextToken(t1,t2)
// Kider�ti, hogy a t1, t2 tokenek azonosak-e, ha next tokenek.
local tk1Id

   if (t1==nil)
      return t2==nil
   elseif (t2==nil)
      return .f.
   endif
   tk1Id:=TOKEN.t1:id
   if (tk1Id==TKID_NEV .or.;
       tk1Id==TKID_SZAMTOMB .or.;
       tk1Id==TKID_STRING .or.;
       tk1Id==TKID_CHAR)
      return tk1Id==TOKEN.t2:id .and.;
              lower(TOKEN.t1:str)==lower(TOKEN.t2:str) .and.;
              TOKEN.t1:eqType==TOKEN.t2:eqType
   endif
return .f.

//*******************************************************************
static function addToTree(treeNodeList,extrDict)
// Az extrdict bal oldal�lt hozz�veszi a f�hoz.
// Az (�j) lez�r� elembe be�rja az extrDict-et. (Mj.: Ez nem 
// felt�tlen�l lev�l)
/*
   tree:=nodeList
   node:={token,nodeList[,extrDict]}
   nodeList:={node1,...}
   Mj.: Ha egy node-ba betesz�nk egy extrDict-et, akkor a nodeList-et
        t�r�lni kell, mert soha nem fog �rv�nyre jutni (�s hib�t is
        okoz, ha itt megpr�b�lunk tov�bb menni).
        Ha m�gis meg akarjuk tartani, akkor a r�gi nodeList-et
        �t kell tenni m�shova.
        
   Egyenl�re az alternat�v�kat mindig egyben kezelj�k, �s
   k�t alternat�va soha sem azonos, �s 'v�laszt� vonalk�nt'
   viselkednek, vagyis, a k�t oldalukra es� tokenek soha sem
   teintend�k azonosnak.
   
   Mj.: Jobb lenne az alternat�v�kat �gank�nt bele illeszteni a 
        f�ba, �gy, hogy az �res �gat is beleillesztj�k, de ekkor
        az ism�tl�d�seket csak akkor tudjuk felvenni ha a f�b�l
        gr�fot csin�lunk (lesz benne k�r).
        
   Mj.: K�t alternat�v�t azonosnak tekinthetn�nk, ha minden �gukon
        minden tokenlista ugyanaz. De ekkor is v�laszt� vonalk�nt 
        kell, hogy viselkedjenek.
        
   A k�s�bb hozz�adott node-ok vannak el�r�bb a nodeList-ekben.
*/

local tokenList, lastNode,i,j
local megvan
local t,tkId,tkStr,tkEqType
local tn,tkNextTk

   tokenList:=EXTRDICT.extrDict:leftSide
   lastNode:=nil
    
   for i:=1 to len(tokenList)
      t:=tokenList[i]
      tkId:=TOKEN.t:id
      tkStr:=TOKEN.t:str
      tkEqType:=TOKEN.t:eqType
      if (tkId==TKID_MALTERSET)
         lastNode:={t,{}}
         aunget(treeNodeList,lastNode)
         treeNodeList:=treeNodelist[1][2]
      elseif (tkId==TKID_NEV .or.;
              tkId==TKID_SZAMTOMB .or.;
              tkId==TKID_STRING .or.;
              tkId==TKID_CHAR)
               
         // Az els� malterset-ig keres�nk egy olyan tokent, ami 
         // azonos vele.
         /*
            Mj.: Az eqType hi�ba EQTYPE_4LEN, akkor sem lehet 4-re
                 csonk�tva vizsg�lni.
         */
         // Egyenl�re mindegyik case insensitive.
         megvan:=.f.
         for j:=1 to len(treeNodeList)
            tn:=treeNodeList[j][1]
            if (TOKEN.tn:id==TKID_MALTERSET)
               // Meg�llunk, �j �g lefel�.
               exit
            elseif (TOKEN.tn:id==tkId .and.;
                   lower(TOKEN.tn:str)==lower(tkStr) .and.;
                   TOKEN.tn:eqType==tkEqType)
               // Megvan! Megy�nk lefel� a f�n.
               megvan:=.t.
               lastNode:=treeNodelist[j]
               treeNodeList:=treeNodelist[j][2]
               exit
            endif 
         end for
         if (!megvan)
            // Nem tal�lt semmit, �j �g lefel�.
            lastNode:={t,{}}
            aunget(treeNodeList,lastNode)
            treeNodelist:=treeNodelist[1][2]
         endif
      elseif (C.MMARKER:isMatchMarker(t))
         // Match markerek.
         // Itt a t�pusnak az index-nek �s a nextToken-ek kell egyeznie.
         // Mj.: Az, hogy a nextTokennek milyen match markerekn�l 
         //      kell egyeznie, nincs kitesztelve!
         // 
         tkNextTk:=MMARKER.t:nextToken
         megvan:=.f.
         for j:=1 to len(treeNodeList)
            tn:=treeNodeList[j][1]
            if (TOKEN.tn:id==TKID_MALTERSET)
               // Meg�llunk, �j �g lefel�.
               exit
            elseif (TOKEN.tn:id==tkId .and.;
                    ;//MMARKER.tn:getName()==MMARKER.t:getName() .and.;
                    MMARKER.tn:mNum==MMARKER.t:mNum .and.;
                    eqNextToken(tkNextTk,MMARKER.tn:nextToken))
               // Megvan!
               megvan:=.t.
               lastNode:=treeNodelist[j]
               treeNodeList:=treeNodelist[j][2]
               exit
            endif 
         end for
         if (!megvan)
            // Nem tal�lt semmit, �j �g lefel�.
            lastNode:={t,{}}
            aunget(treeNodeList,lastNode)
            treeNodeList:=treeNodelist[1][2]
         endif
      else
         // Minden m�st kihagyunk. Ebben benne van az �res is.
      endif
   end for
   // V�ge.
   // Ilyenkor az utols� node-ban t�r�lni kell a nodeList-et, �s
   // az extrdict-et, ha van, �s betenni egy �res nodeList-et, 
   // �s a mi extrDict-�nket.
   if (lastNode!=nil)
      asize(lastNode,3)
      asize(lastNode[2],0)
      lastNode[3]:=extrDict
   // else
      // Az extrDict �res volt. Az �resek defin�ci� szerint semmire
      // sem illeszkednek.
  endif
return nil

//*******************************************************************

