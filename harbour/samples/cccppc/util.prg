/*
 * $Id$
 */

//********************************************************************
// util.prg: General utilities.
// 1999, Csisz�r Levente


//********************************************************************
function alast(anArray)
return atail(anArray)
// return anArray[len(anArray)]

//********************************************************************
function aappend(anArray,appArray)
local n

   asize(anArray,(n:=len(anArray))+len(appArray))
   acopy(appArray,anArray,,,n+1)
return anArray

//********************************************************************
function axappend(anArray,appArray,from,len)
local n

   if (from==nil)
      from:=1
   endif
   if (len==nil)
      len:=len(appArray)-from+1
   endif
   asize(anArray,(n:=len(anArray))+len)
   acopy(appArray,anArray,from,len,n+1)
return anArray

//********************************************************************
function apop(anArray)
local w

   w:=alast(anArray)
   asize(anArray,len(anArray)-1)
return w

//********************************************************************
function adrop(anArray)
   asize(anArray,len(anArray)-1)
return nil

//********************************************************************
function aread(anArray)
local w

   w:=anArray[1]
   adel(anArray,1)
   asize(anArray,len(anArray)-1)
return w

//********************************************************************
function asub(anArray,pos,len)
// Az anArray pos poz�ci�n kezd�d� len hossz� r�sz�t adja.

local w

   if (pos==nil)
      pos:=1
   elseif (pos>len(anArray))
      return {}
   endif
   if (len==nil .or.;
       pos+len-1>len(anArray))
      len:=len(anArray)-pos+1
   endif
   w:=array(len)
   acopy(anArray,w,pos,len)
return w 

//********************************************************************
function aconcatenate(t1,t2)
// A t1 �s a t2 konkaten�ci�j�t adja.

local wt,t1len

   t1len:=len(t1)
   wt:=array(t1len+len(t2))
   acopy(t1,wt)
   acopy(t2,wt,nil,nil,t1len+1)
return wt

//********************************************************************
function axconcatenate(t1,t1from,t1len,t2,t2from,t2len)
// A t1 t1from-t�l kezd�d� t1len hossz� darabj�nak �s a t2 t2from-t�l
// kezd�d� t2len hossz� konkaten�ci�j�t adja.
// Minden param�tert k�telez� megadni, az �rv�nyess�g�ket nem 
// vizsg�lja.

local wt

   wt:=array(t1len+t2len)
   acopy(t1,wt,t1from,t1len)
   acopy(t2,wt,t2from,t2len,t1len+1)
return wt

//********************************************************************
function aunget(t,e)
// A t array elej�re besz�ra az e-t. A t hossza eggyel n�, az els� 
// eleme az e lesz.

   aadd(t,nil)
   ains(t,1)
   t[1]:=e
return t

//********************************************************************
function axinsert(t,pos,n)
// A 't' 'pos' poz�ci�ja el� besz�r n �res helyet. A t hossza n-el n�.
   if (n<=0)
      return t
   endif
   asize(t,len(t)+n)
   for n:=n to 1 step -1
      ains(t,pos)
   end for
return t

#ifdef OLD
local wt
   wt:=array(len(t)+n)
   acopy(t,wt,1,pos-1)
   acopy(t,wt,pos,,pos+n)
return wt
#endif
//********************************************************************
function xnumat(ch,str)
// A ch h�nyszor van meg az str-ben.

local i,n:=0

   if (!ch$str)
      return 0
   endif
   for i:=1 to len(str)
      if (ch==substr(str,i,1))
         n++
      endif
   end for
return n

//*******************************************************************
function toExprStr(val,deep)
// A val-t kifejez�ss� alak�tja.
// A blokkokat block, az objektumokat pedig object v�ltoz�val
// helyettes�ti. A t�mb�ket kifejti maximum TOEXPRSTR_MAXDEEP 
// m�lys�gig, ha ezt t�ll�pi, akkor a too_many_nested_arrays v�ltoz�t
// �rja a t�mb hely�re.

#define TOEXPRSTR_MAXDEEP 100

local str
local type:=valType(val)

   if (type=="N")
      return allTrim(str(val))
   elseif (type=="D")
      return 'stod("'+dtos(val)+'")'
   elseif (type=="L")
      return if(val,".T.",".F.")
   elseif (type=="C" .or. type=="M")
      if (!'"'$val)
         return '"'+val+'"'
      endif
      // Itt meg kellen n�zni, van-e benne "'", �s ha van, akkor 
      // felbontogatni.
      return "'"+val+"'"
   elseif (type=="B")
      return "block"
   elseif (type=="O")
      return "object"
   elseif (type=="A")
      if (deep==nil)
         deep:=0
      endif
      
      if (deep++>TOEXPRSTR_MAXDEEP)
         return "too_many_nested_arrays"
      endif
      
      str:=nil
      aeval(val,{|x| if(str==nil,;
                        str:=toExprStr(x,deep),;
                        str+=","+toExprStr(x,deep))})
      return "{"+str+"}"
   elseif (val==nil)
      return "nil"
   endif
return ""


//********************************************************************
static function findOpt(ot,opt,levag)
// Ha a lev�g igaz, akkor azokat az opci�kat, amiknek lehetnek
// param�terei lev�gva keresi.
local i

   if (0==(i:=ascan(ot,{|x| x[1]==if(levag==.t. .and. x[2]!=nil,;
                                     left(opt,len(x)),;
                                     opt)})))
      return nil
   endif
return ot[i]

//********************************************************************
function parseOpt(opt,paramArray,hibaSzoveg)
/*
   Opci�k elemz�se.
   
   opt:=<optSpec> ',' ...
   
   optSpec:=<optName>[:]
   
   <optName>:={'-' <shortName>| '--' <longName>}
   
   Az opt spcifik�ci�kat vessz�vel kell elv�lasztani egym�st�l.
   
   A '-'-al kezd�d� opci� n�v r�vid form�t (pl. -o) jelent
   A '--'-al kezd�d� opci� n�v hossz� form�t (pl. --output) jelent
   
   A ':' az optSpec v�g�n azt jelzi, hogy ut�na �llhat egy param�ter.
   R�vid form�n�l ez az opci�val �sszevonva (-oproba) vagy az opci� 
   ut�n (-o proba) �llhat. Ha a param�ter hi�nyzik, akkor betesz
   egy ""-t.
   
   Hossz� form�n�l egy '=' ut�n (--output=proba) vagy az opci� 
   m�g�tt (--output proba) �llhat.
   
   A sima neveket a nem b�ntja, a '--' ut�n nem v�gez elemz�st.
   
   A paramArray-ban a felismert opci�kat mindig elv�lasztja a 
   param�tereit�l.
   
   Ha egy r�vid opci� egy bet�s �s nem tartozik hozz� param�ter, 
   akkor az ilyen opci�kat megengedi egybe �rni: grep -il
   Mj.: Ezeket sz�tv�lasztja -i -l -re.
   
   Az �nmag�ban �ll� '-' -t nem t�rli, viszont, ha egy param�tert 
   ig�nyl� opci� ut�n '-'-os opci� �ll, akkor egy ""-t betesz opci�nak.
   
   Pl.: parseOpt("-o:,--output:",;
                {"-oproba1","-o","proba2",;
                 "--output=proba3","--output","proba4",;
                 "--",;
                 "-oproba5"})
        
        Eredm�nye:
        
        {"-o","proba1",;                 
         "-o","proba2",;                 
         "--output","proba3",;
         "--output","proba4",;
         "--",;
         "-oproba5";
       }                 
       
       lesz. 
       Mj.: A -oproba5 az�rt maradt egyben, mert a '--' ut�n m�r
            nem v�gez elemz�st.
            
            
*/

local ot,wOptArray,wOpt,pOpt,i,j,pos,result
local otOssze,w
local attesz

   hibaSzoveg:=""
   wOptArray:=felbont(opt,",")
   ot:={}
   otOssze:=""
   for i:=1 to len(wOptArray)
      wOpt:=alltrim(wOptArray[i])
      if (right(wOpt,1)==":")
         // Param�teres.
         aadd(ot,{left(wOpt,len(wOpt)-1),":"})
      else
         // Nem param�teres
         if (len(wOpt)==2 .and.;
             left(wOpt,1)=="-" .and.;
             !right(wOpt,1)=="-")
            // �sszevonhat� r�vid opci�.
            otOssze+=right(wOpt,1)
         else
            // Nem vonhat� �ssze. Nincs param�ter.
            aadd(ot,{wOpt,nil})
         endif
      endif
   end for
   
   result:={}
   attesz:=.f.
   for i:=1 to len(paramArray)
      if (paramArray[i]=="--")
         attesz:=.t.
      endif
      if (attesz)
         aadd(result,paramArray[i])
      elseif (left(paramArray[i],2)=="--")
         // Hossz� param�ter.
         if (nil!=(pOpt:=findOpt(ot,paramArray[i])))
            // Megvagyunk.
            aadd(result,paramArray[i])
            // Ha param�teres, akkor meg kell n�zni mi a k�vetkez�.
            if (pOpt[2]!=nil .and.;
                (i+1>len(paramArray) .or.;
                left(paramArray[i+1],1)=='-'))
               aadd(result,"")
            endif
         elseif (0==(pos:=at(paramArray[i],"="))) // Van-e benne '='?
            // Nincs benne '=', �s nincs is meg, hib�s param�ter!
            hibaSzoveg+="Unknown option: "+paramArray[i]+crlf()
         elseif (nil==(pOpt:=findOpt(ot,left(paramArray[i],pos-1))))
            // Nincs meg a param�ter.
            hibaSzoveg+="Unknown option: "+paramArray[i]+newline()
         else
            // Felbontjuk
            aadd(result,left(paramArray[i],pos-1))
            aadd(result,substr(paramArray[i],pos+1))
         endif
      elseif (left(paramArray[i],1)=="-")
         // R�vid param�ter.
         if (nil!=(pOpt:=findOpt(ot,paramArray[i])))
            // Megvagyunk, nem kell semmit sem csin�lni.
            aadd(result,paramArray[i])
            // Ha param�teres, akkor meg kell n�zni mi a k�vetkez�.
            if (pOpt[2]!=nil .and.;
                (i+1>len(paramArray) .or.;
                left(paramArray[i+1],1)=='-'))
               aadd(result,"")
            endif
         elseif (nil!=(pOpt:=findOpt(ot,paramArray[i],.t.)))
            // Megvan, de lehet ut�na opci�.
            aadd(result,left(paramArray[i],len(pOpt[1])))
            w:=substr(paramArray[i],len(pOpt[1])+1)
            if (len(w)>0)
               aadd(result,w)
            endif
         elseif (paramArray[i]=="-")
            aadd(result,paramArray[i])
         else
            // Lehet m�g r�vid �sszevonhat�k sorozata.
            for j:=2 to len(paramArray[i])
               w:=substr(paramArray[i],j,1)
               if (w$otOssze)
                  aadd(result,"-"+w)
               else
                  hibaSzoveg+="Unknown option: -"+w+newline()
               endif
            end for
         endif
      else
         // Nem opci�, sim�n �ttessz�k.
         aadd(result,paramArray[i])
      endif
   end for

return result
      
//********************************************************************
function stringifyStr(str,safe)
// Ki�rhat� stringet csin�l az str-b�l, vagyis el� �s m�g� teszi a 
// hat�rol� jeleket. Ha a safe igaz, akkor a '['-t �s a ']'-t
// nem haszn�lja.
   if ('"' $ str)
      if ("'" $ str)
         // A spec szerint ez akkor is �gy marad, ha van ']' az 
         // str-ben.
         // Itt azt lehetne csin�lni, hogy sz�tv�gjuk a "'" 
         // karakterekn�l a stringet �s az �gy kapott stringeket 
         // �sszeadjuk, �s az eg�sz kifejez�st z�r�jelbe tessz�k.
         // Incompatibility
         if (!empty(safe))
            return nil
         endif
         str:="["+str+"]"
      else
         str:="'"+str+"'"
      endif
   else
      str:='"'+str+'"'
   endif
   
return str

//*******************************************************************
function beloleAll(mi,mibol)
// Az mi string �sszes karaktere benne kell, hogy legyen a mibol
// stringben.
local i
   for i:=1 to len(mi)
      if !(substr(mi,i,1)$mibol)
         return .f.
      endif
   next
return .t.

//*******************************************************************
function matchShortNames(name,tName,l)
// Az elemzend� n�v illeszkedik-e a c�l n�vre.
// Sajnos nem 'l' hosszan, hanem minimum l hosszan kell hasonl�tani,
// �s nem mindegy, hogy mi illeszkedik mire.

   // Ez kell!!!
   if (len(name)>len(tName))
      return .f.
   endif
   
   if (empty(l) .or. len(name)<l)
      return lower(name)==lower(tName)
   endif
   
//   Mj.: Ez l�nyeg�ben egy '=' (�sszehasonl�t�s), de az '=' 
//        tiltott.
return (lower(name)==lower(left(tName,len(name))))

//********************************************************************

