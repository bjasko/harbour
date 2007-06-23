/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    demonstration/test code for terminal character output and codepage
 *    translations
 *
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
 * www - http://www.harbour-project.org
 *
 */

#include "hbgtinfo.ch"

#define POL_MAZ   "������������������"
#define POL_ISO   "��ʣ�Ӧ������󶼿"
#define POL_852   "�������������䢘��"
#define POL_WIN   "��ʣ�ӌ������󜟿"

REQUEST HB_CODEPAGE_PLMAZ
REQUEST HB_CODEPAGE_PLISO
REQUEST HB_CODEPAGE_PL852
REQUEST HB_CODEPAGE_PLWIN

function main( cTermCP, cHostCP, lBoxChar )
local i, j, x

if empty( cTermCP )
    cTermCP := "PLISO"
else
    cTermCP := upper( cTermCP )
endif
if empty( cHostCP )
    cHostCP := "PLMAZ"
else
    cHostCP := upper( cHostCP )
endif
lBoxChar := !empty( lBoxChar )

HB_GTINFO( GTI_FONTSIZE, 20 )
HB_GTINFO( GTI_FONTWIDTH, 9 )
HB_GTINFO( GTI_FONTNAME, "fixed" )

HB_SETTERMCP( cTermCP, cHostCP, lBoxChar )

? OS(), VERSION(), DATE(), TIME()
? HB_GTVERSION(), HB_GTVERSION(1)
? "Host codpage: " + cHostCP + ", terminal codepage: " + cTermCP
?

//HB_GTINFO(GTI_COMPATBUFFER,.f.)
?
for i := 0 to 15
    for j := 0 to 15
        x := i * 16 + j
        dispout( "  " + chr( x ) )
    next
    ?
next
inkey(0)
?; dspboxch( "���Ŀ  ���ͻ  ���͸  ���ķ  ��� ����  � � � �" )
?; dspboxch( "� � �  � � �  ���Ĵ  ���Ķ  ���" )
?; dspboxch( "���Ĵ  ���͹  � � �  � � �  ��� ���" )
?; dspboxch( "� � �  � � �  ���͵  ���͹  ���" )
?; dspboxch( "�����  ���ͼ  ���;  ���Ľ  ��� � � �" )
?
? "ISO-8859-2: say[ " + POL_ISO + " ]"; dspboxch( ", box[ " + POL_ISO + " ]" )
? "   Mazovia: say[ " + POL_MAZ + " ]"; dspboxch( ", box[ " + POL_MAZ + " ]" )
? "   CP-1250: say[ " + POL_WIN + " ]"; dspboxch( ", box[ " + POL_WIN + " ]" )
? "    CP-852: say[ " + POL_852 + " ]"; dspboxch( ", box[ " + POL_852 + " ]" )
? chr(4)+chr(16)+chr(17)+chr(24)+chr(25)+chr(26)+chr(27)+chr(30)+chr(31)
HB_GTINFO(GTI_ESCDELAY,5)
inkey(0)
alert("ALERT BOX")
return nil

function dspboxch( cStr )
local i, r, c
for i := 1 to len( cStr )
    r:=row(); c:=col()
    dispbox( r, c, r, c, substr( cStr, i, 1 ) )
    setpos(r,c+1)
next
return nil
