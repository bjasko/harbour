/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for cross-compatibility between different Harbour flavours
 *
 * Copyright 1999-2007 {list of individual authors and e-mail addresses}
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

#ifdef __HARBOUR__

#ifdef __XHARBOUR__

   #include "gtinfo.ch"
   #include "gfx.ch"

   #xtranslate hb_gtSys                => gtSys
   #xtranslate hb_gtInfo([<x,...>])    => gtInfo(<x>)
   #xtranslate hb_gtVersion([<x>])     => hb_gt_Version(<x>)

   #xtranslate hb_ScrMaxRow()          => gtInfo( HB_GTI_SCREENHEIGHT )
   #xtranslate hb_ScrMaxCol()          => gtInfo( HB_GTI_SCREENWIDTH )
   #xtranslate MaxRow(.T.)             => gtInfo( HB_GTI_SCREENHEIGHT )
   #xtranslate MaxCol(.T.)             => gtInfo( HB_GTI_SCREENWIDTH )

   #xtranslate hb_dbPack()             => __dbPack()
   #xtranslate hb_dbZap()              => __dbZap()
   #xtranslate hb_dbDrop([<x,...>])    => dbDrop(<x>)
   #xtranslate hb_dbExists([<x,...>])  => dbExists(<x>)
   #xtranslate hb_FieldLen([<x>])      => FieldLen(<x>)
   #xtranslate hb_FieldDec([<x>])      => FieldDec(<x>)
   #xtranslate hb_FieldType([<x>])     => FieldType(<x>)

   #xtranslate hb_isregex([<x>])       => hb_isregexstring(<x>)
   #xtranslate hb_pvalue([<x,...>])    => pvalue(<x>)
   #xtranslate hb_methodName([<x,...>])=> methodName(<x>)
   #xtranslate hb_libLoad([<x,...>])   => libLoad(<x>)
   #xtranslate hb_libFree([<x,...>])   => libFree(<x>)
   #xtranslate hb_adler32([<x,...>])   => hb_checksum(<x>)
   #xtranslate hb_setLastKey([<x,...>])=> setLastKey(<x>)
   #xtranslate hb_CStr([<x,...>])      => CStr(<x>)
   #xtranslate hb_DirExists(<x>)       => IsDirectory(<x>)
   #xtranslate hb_rddInfo([<x,...>])   => rddInfo(<x>)
   #xtranslate hb_idleSleep([<x,...>]) => SecondsSleep(<x>)
   #xtranslate hb_UserName()           => NetName(1)
   #xtranslate hb_FSize(<x>)           => FileSize(<x>)
   #xtranslate hb_WildMatch([<x,...>]) => WildMatch(<x>)
   #xtranslate hb_Deserialize(<x>)     => hb_DeserialNext(<x>)
   #xtranslate hb_Adler32([<x,...>])   => hb_Checksum(<x>)

   #xtranslate hb_HexToNum([<c,...>])  => HexToNum(<c>)
   #xtranslate hb_NumToHex([<n,...>])  => NumToHex(<n>)
   #xtranslate hb_HexToStr([<c,...>])  => HexToStr(<c>)
   #xtranslate hb_StrToHex([<c,...>])  => StrToHex(<c>)

   #xtranslate hb_AScan([<x,...>])     => AScan(<x>)
   #xtranslate hb_RAScan([<x,...>])    => RAScan(<x>)
   #xtranslate hb_AIns([<x,...>])      => AIns(<x>)
   #xtranslate hb_ADel([<x,...>])      => ADel(<x>)
   #xtranslate hb_At([<x,...>])        => At(<x>)

   #xtranslate hb_ISPOINTER( <xValue> )=> ISPOINTER( <xValue> )

   #xtranslate hb_IniSetComment([<x,...>]) => hb_SetIniComment(<x>)
   #xtranslate hb_IniRead([<x,...>])       => hb_ReadIni(<x>)
   #xtranslate hb_IniWrite([<x,...>])      => hb_WriteIni(<x>)

   /* Some statement endings */
   #xcommand ENDSEQUENCE => END
   #xcommand ENDSWITCH => END
   #xcommand END SWITCH => END
   #xcommand ENDWITH => END
   #xcommand END WITH => END
   #xcommand END OBJECT => END

   #ifndef HB_SYMBOL_UNUSED
      #define HB_SYMBOL_UNUSED( symbol )  ( symbol := ( symbol ) )
   #endif

#else

   #include "hbgtinfo.ch"
   #include "hbgfx.ch"

   /* these are used _by_ MaxRow()/MaxCol() */
   #define GTI_WINDOW         0  /* Maximum window size ('window' in CT terms) */
   #define GTI_SCREEN         1  /* Maximum screen size ('Screen' in CT terms) */
   #define GTI_CLIENT         2  /* Maximum possible client size of a window */
   #define GTI_MAX            3  /* Maximum possible window size (in Windows) */

   #xtranslate gtSys                   => hb_gtSys
   #xtranslate gtInfo([<x,...>])       => hb_gtInfo(<x>)
   #xtranslate hb_gt_Version([<x>])    => hb_gtVersion(<x>)

   #xtranslate gtSetClipboard(<x>)     => hb_gtInfo( HB_GTI_CLIPBOARDDATA, <x> )
   #xtranslate gtGetClipboard()        => hb_gtInfo( HB_GTI_CLIPBOARDDATA )
   #xtranslate gtGetClipBoardSize()    => Len( hb_gtInfo( HB_GTI_CLIPBOARDDATA ) )
   #xtranslate gtPasteClipBoard([<n>]) => hb_gtInfo( HB_GTI_CLIPBOARDPAST )
   #xtranslate gtProcessMessages()     => NextKey()
   #xtranslate gfxPrimitive([<x,...>]) => hb_gfxPrimitive(<x>)
   #xtranslate gfxText([<x,...>])      => hb_gfxText(<x>)
   #xtranslate MaxRow(.T.)             => hb_gtInfo( HB_GTI_VIEWPORTHEIGHT )
   #xtranslate MaxCol(.T.)             => hb_gtInfo( HB_GTI_VIEWPORTWIDTH )

   #xtranslate hb_isregexstring([<x>]) => hb_isregex(<x>)
   #xtranslate pvalue([<x,...>])       => hb_pvalue(<x>)
   #xtranslate methodName([<x,...>])   => hb_methodName(<x>)
   #xtranslate libLoad([<x,...>])      => hb_libLoad(<x>)
   #xtranslate libFree([<x,...>])      => hb_libFree(<x>)
   #xtranslate hb_checksum([<x,...>])  => hb_adler32(<x>)
   #xtranslate setLastKey([<x,...>])   => hb_setLastKey(<x>)
   #xtranslate CStr([<x,...>])         => hb_CStr(<x>)
   #xtranslate IsDirectory(<x>)        => hb_DirExists(<x>)
   #xtranslate SecondsSleep([<x,...>]) => hb_idleSleep(<x>)
   #xtranslate NetName(<n>)            => iif( ValType( <n> ) == "N" .AND. <n> == 1, hb_UserName(), NetName() )
   #xtranslate FileSize(<x>)           => hb_FSize(<x>)
   #xtranslate WildMatch([<x,...>])    => hb_WildMatch(<x>)
   #xtranslate hb_DeserialNext(<x>)    => hb_Deserialize(<x>)
   #xtranslate hb_Checksum([<x,...>])  => hb_Adler32(<x>)

   #xtranslate HexToNum([<c,...>])     => hb_HexToNum(<c>)
   #xtranslate NumToHex([<n,...>])     => hb_NumToHex(<n>)
   #xtranslate HexToStr([<c,...>])     => hb_HexToStr(<c>)
   #xtranslate StrToHex([<c,...>])     => hb_StrToHex(<c>)

   #xtranslate AScan(<a>,<b>,[<c>],[<d>],<e>) => hb_AScan(<a>,<b>,<c>,<d>,<e>)
   #xtranslate RAScan([<x,...>])              => hb_RAScan(<x>)
   #xtranslate AIns(<a>,<n>,[<x,...>])        => hb_AIns(<a>,<n>,<x>)
   #xtranslate ADel(<a>,<n>,<l>)              => hb_ADel(<a>,<n>,<l>)
   #xtranslate At(<a>,<b>,[<x,...>])          => hb_At(<a>,<b>,<x>)

   #xtranslate ISPOINTER( <xValue> )   => hb_ISPOINTER( <xValue> )

   #xtranslate hb_SetIniComment([<x,...>]) => hb_IniSetComment(<x>)
   #xtranslate hb_ReadIni([<x,...>])       => hb_IniRead(<x>)
   #xtranslate hb_WriteIni([<x,...>])      => hb_IniWrite(<x>)

   #xtranslate Str(<x>,[<y>],[<y>],<z>)=> iif(<z>, LTrim(Str(<x>)), Str(<x>))
   #xtranslate hb_CMDARGARGV([<x,...>])=> hb_ARGV(0)

   /* Hash item functions */
   #xtranslate HASH([<x,...>])         => hb_HASH(<x>)
   #xtranslate HHASKEY([<x,...>])      => hb_HHASKEY(<x>)
   #xtranslate HGETPOS([<x,...>])      => hb_HPOS(<x>)
   #xtranslate HGET([<x,...>])         => hb_HGET(<x>)
   #xtranslate HSET([<x,...>])         => hb_HSET(<x>)
   #xtranslate HDEL([<x,...>])         => hb_HDEL(<x>)
   #xtranslate HGETKEYAT([<x,...>])    => hb_HKEYAT(<x>)
   #xtranslate HGETVALUEAT([<x,...>])  => hb_HVALUEAT(<x>)
   #xtranslate HSETVALUEAT([<x,...>])  => hb_HVALUEAT(<x>)
   #xtranslate HGETPAIRAT([<x,...>])   => hb_HPAIRAT(<x>)
   #xtranslate HDELAT([<x,...>])       => hb_HDELAT(<x>)
   #xtranslate HGETKEYS([<x,...>])     => hb_HKEYS(<x>)
   #xtranslate HGETVALUES([<x,...>])   => hb_HVALUES(<x>)
   #xtranslate HFILL([<x,...>])        => hb_HFILL(<x>)
   #xtranslate HCLONE([<x,...>])       => hb_HCLONE(<x>)
   #xtranslate HCOPY([<x,...>])        => hb_HCOPY(<x>)
   #xtranslate HMERGE([<x,...>])       => hb_HMERGE(<x>)
   #xtranslate HEVAL([<x,...>])        => hb_HEVAL(<x>)
   #xtranslate HSCAN([<x,...>])        => hb_HSCAN(<x>)
   #xtranslate HSETCASEMATCH([<x,...>])=> hb_HSETCASEMATCH(<x>)
   #xtranslate HGETCASEMATCH([<x,...>])=> hb_HCASEMATCH(<x>)
   #xtranslate HSETAUTOADD([<x,...>])  => hb_HSETAUTOADD(<x>)
   #xtranslate HGETAUTOADD([<x,...>])  => hb_HAUTOADD(<x>)
   #xtranslate HALLOCATE([<x,...>])    => hb_HALLOCATE(<x>)
   #xtranslate HDEFAULT([<x,...>])     => hb_HDEFAULT(<x>)

   /* Inet functions */
   #xtranslate INETINIT([<x,...>])                => hb_INETINIT(<x>)
   #xtranslate INETCLEANUP([<x,...>])             => hb_INETCLEANUP(<x>)
   #xtranslate INETCREATE([<x,...>])              => hb_INETCREATE(<x>)
   #xtranslate INETCLOSE([<x,...>])               => hb_INETCLOSE(<x>)
   #xtranslate INETFD([<x,...>])                  => hb_INETFD(<x>)
   #xtranslate INETSTATUS([<x,...>])              => hb_INETSTATUS(<x>)
   #xtranslate INETERRORCODE([<x,...>])           => hb_INETERRORCODE(<x>)
   #xtranslate INETERRORDESC([<x,...>])           => hb_INETERRORDESC(<x>)
   #xtranslate INETCLEARERROR([<x,...>])          => hb_INETCLEARERROR(<x>)
   #xtranslate INETCOUNT([<x,...>])               => hb_INETCOUNT(<x>)
   #xtranslate INETADDRESS([<x,...>])             => hb_INETADDRESS(<x>)
   #xtranslate INETPORT([<x,...>])                => hb_INETPORT(<x>)
   #xtranslate INETSETTIMEOUT([<x,...>])          => hb_INETTIMEOUT(<x>)
   #xtranslate INETGETTIMEOUT([<x,...>])          => hb_INETTIMEOUT(<x>)
   #xtranslate INETCLEARTIMEOUT([<x,...>])        => hb_INETCLEARTIMEOUT(<x>)
   #xtranslate INETSETTIMELIMIT([<x,...>])        => hb_INETTIMELIMIT(<x>)
   #xtranslate INETGETTIMELIMIT([<x,...>])        => hb_INETTIMELIMIT(<x>)
   #xtranslate INETCLEARTIMELIMIT([<x,...>])      => hb_INETCLEARTIMELIMIT(<x>)
   #xtranslate INETSETPERIODCALLBACK([<x,...>])   => hb_INETPERIODCALLBACK(<x>)
   #xtranslate INETGETPERIODCALLBACK([<x,...>])   => hb_INETPERIODCALLBACK(<x>)
   #xtranslate INETCLEARPERIODCALLBACK([<x,...>]) => hb_INETCLEARPERIODCALLBACK(<x>)
   #xtranslate INETRECV([<x,...>])                => hb_INETRECV(<x>)
   #xtranslate INETRECVALL([<x,...>])             => hb_INETRECVALL(<x>)
   #xtranslate INETRECVLINE([<x,...>])            => hb_INETRECVLINE(<x>)
   #xtranslate INETRECVENDBLOCK([<x,...>])        => hb_INETRECVENDBLOCK(<x>)
   #xtranslate INETDATAREADY([<x,...>])           => hb_INETDATAREADY(<x>)
   #xtranslate INETSEND([<x,...>])                => hb_INETSEND(<x>)
   #xtranslate INETSENDALL([<x,...>])             => hb_INETSENDALL(<x>)
   #xtranslate INETGETHOSTS([<x,...>])            => hb_INETGETHOSTS(<x>)
   #xtranslate INETGETALIAS([<x,...>])            => hb_INETGETALIAS(<x>)
   #xtranslate INETSERVER([<x,...>])              => hb_INETSERVER(<x>)
   #xtranslate INETACCEPT([<x,...>])              => hb_INETACCEPT(<x>)
   #xtranslate INETCONNECT([<x,...>])             => hb_INETCONNECT(<x>)
   #xtranslate INETCONNECTIP([<x,...>])           => hb_INETCONNECTIP(<x>)
   #xtranslate INETDGRAMBIND([<x,...>])           => hb_INETDGRAMBIND(<x>)
   #xtranslate INETDGRAM([<x,...>])               => hb_INETDGRAM(<x>)
   #xtranslate INETDGRAMSEND([<x,...>])           => hb_INETDGRAMSEND(<x>)
   #xtranslate INETDGRAMRECV([<x,...>])           => hb_INETDGRAMRECV(<x>)
   #xtranslate INETCRLF([<x,...>])                => hb_INETCRLF(<x>)
   #xtranslate ISINETSOCKET([<x,...>])            => HB_INETISSOCKET(<x>)
   #xtranslate INETDESTROY([<x,...>])             => iif( HB_INETISSOCKET( <x> ), hb_INETCLOSE( <x> ), )

   /* THROW => generate error */
   #xtranslate THROW(<oErr>) => (Eval(ErrorBlock(), <oErr>), Break(<oErr>))

   /* TEXT INTO <varname> */
   #xcommand TEXT INTO <v> => #pragma __text|<v>+=%s+HB_OSNEWLINE();<v>:=""

   /* SWITCH ... ; case ... ; DEFAULT ; ... ; END */
   #xcommand DEFAULT => OTHERWISE

   /* FOR EACH hb_enumIndex() */
   #xtranslate hb_enumIndex(<!v!>) => <v>:__enumIndex()

   /* TRY / CATCH / FINALLY / END */
   #xcommand TRY  => BEGIN SEQUENCE WITH {|oErr| Break( oErr )}
   #xcommand CATCH [<!oErr!>] => RECOVER [USING <oErr>] <-oErr->
   #xcommand FINALLY => ALWAYS

   /* EXTENDED CODEBLOCKs */
   #xtranslate \<|[<x,...>]| => {|<x>|
   #xcommand > [<*x*>]       => } <x>


   /* xHarbour operators: IN, HAS, LIKE, >>, <<, |, &, ^^ */
   #translate ( <exp1> IN <exp2> )     => ( (<exp1>) $ (<exp2>) )
   #translate ( <exp1> HAS <exp2> )    => ( HB_REGEXHAS( (<exp2>), (<exp1>) ) )
   #translate ( <exp1> LIKE <exp2> )   => ( HB_REGEXLIKE( (<exp2>), (<exp1>) ) )
   #translate ( <exp1> \<\< <exp2> )   => ( HB_BITSHIFT( (<exp1>), (<exp2>) ) )
   #translate ( <exp1> >> <exp2> )     => ( HB_BITSHIFT( (<exp1>), -(<exp2>) ) )
   #translate ( <exp1> | <exp2> )      => ( HB_BITOR( (<exp1>), (<exp2>) ) )
   #translate ( <exp1> & <exp2> )      => ( HB_BITAND( (<exp1>), (<exp2>) ) )
   #translate ( <exp1> ^^ <exp2> )     => ( HB_BITXOR( (<exp1>), (<exp2>) ) )

#endif

#endif /* __HARBOUR__ */
