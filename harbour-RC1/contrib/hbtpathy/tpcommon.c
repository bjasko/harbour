/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Telepathy emulation library
 *
 * Copyright 2000, 2001 Dan Levitt <dan@boba-fett.net>
 *
 * Copyright 2004, 2005 - Maurilio Longo <maurilio.longo@libero.it>
 * www - http://www.xharbour.org
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

/* ----- platform neutral C code -------------------------------------- */

#define _CLIPDEFS_H     // Don't ridefine basic types

#include "hbapifs.h"
#include "extend.api"

/* crctab calculated by Mark G. Mendel, Network Systems Corporation */
static unsigned short crctab[ 256 ] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};


/* updcrc() macro by Pete Disdale */
#define updcrc(cp, crc)  ( ( crc << 8 ) ^ ( crctab[ ( ( crc >> 8 ) ^ cp ) & 0xFF ] ) )


HB_FUNC( P_CRC16 ) {

   char *ptr = _parc( 1 );
   int count = _parclen( 1 );

   register unsigned short crc = 0;

   while ( count-- > 0 ) {
      crc = updcrc( *ptr++, crc );
   }

   /* swap Hi and Lo byte */
   _retnl( ( crc >> 8 ) | ( ( crc << 8 ) & 0xFF00 ) );
}



/* Taken from: contrib/unicode/hbcrc32.c

 * Harbour Unicode Support
 *
 * Source codes for functions:
 *    HB_CRC32()
 *    HB_NCRC32()
 *
 * Copyright 2004 Dmitry V. Korzhov <dk@april26.spb.ru>
 * www - http://www.harbour-project.org
*/

#define CRC32INIT    ( 0xFFFFFFFFL )

static ULONG crc32tbl[ 256 ] = {
0x00000000l,0x77073096l,0xEE0E612Cl,0x990951BAl,0x076DC419l,0x706AF48Fl,0xE963A535l,0x9E6495A3l,
0x0EDB8832l,0x79DCB8A4l,0xE0D5E91El,0x97D2D988l,0x09B64C2Bl,0x7EB17CBDl,0xE7B82D07l,0x90BF1D91l,
0x1DB71064l,0x6AB020F2l,0xF3B97148l,0x84BE41DEl,0x1ADAD47Dl,0x6DDDE4EBl,0xF4D4B551l,0x83D385C7l,
0x136C9856l,0x646BA8C0l,0xFD62F97Al,0x8A65C9ECl,0x14015C4Fl,0x63066CD9l,0xFA0F3D63l,0x8D080DF5l,
0x3B6E20C8l,0x4C69105El,0xD56041E4l,0xA2677172l,0x3C03E4D1l,0x4B04D447l,0xD20D85FDl,0xA50AB56Bl,
0x35B5A8FAl,0x42B2986Cl,0xDBBBC9D6l,0xACBCF940l,0x32D86CE3l,0x45DF5C75l,0xDCD60DCFl,0xABD13D59l,
0x26D930ACl,0x51DE003Al,0xC8D75180l,0xBFD06116l,0x21B4F4B5l,0x56B3C423l,0xCFBA9599l,0xB8BDA50Fl,
0x2802B89El,0x5F058808l,0xC60CD9B2l,0xB10BE924l,0x2F6F7C87l,0x58684C11l,0xC1611DABl,0xB6662D3Dl,
0x76DC4190l,0x01DB7106l,0x98D220BCl,0xEFD5102Al,0x71B18589l,0x06B6B51Fl,0x9FBFE4A5l,0xE8B8D433l,
0x7807C9A2l,0x0F00F934l,0x9609A88El,0xE10E9818l,0x7F6A0DBBl,0x086D3D2Dl,0x91646C97l,0xE6635C01l,
0x6B6B51F4l,0x1C6C6162l,0x856530D8l,0xF262004El,0x6C0695EDl,0x1B01A57Bl,0x8208F4C1l,0xF50FC457l,
0x65B0D9C6l,0x12B7E950l,0x8BBEB8EAl,0xFCB9887Cl,0x62DD1DDFl,0x15DA2D49l,0x8CD37CF3l,0xFBD44C65l,
0x4DB26158l,0x3AB551CEl,0xA3BC0074l,0xD4BB30E2l,0x4ADFA541l,0x3DD895D7l,0xA4D1C46Dl,0xD3D6F4FBl,
0x4369E96Al,0x346ED9FCl,0xAD678846l,0xDA60B8D0l,0x44042D73l,0x33031DE5l,0xAA0A4C5Fl,0xDD0D7CC9l,
0x5005713Cl,0x270241AAl,0xBE0B1010l,0xC90C2086l,0x5768B525l,0x206F85B3l,0xB966D409l,0xCE61E49Fl,
0x5EDEF90El,0x29D9C998l,0xB0D09822l,0xC7D7A8B4l,0x59B33D17l,0x2EB40D81l,0xB7BD5C3Bl,0xC0BA6CADl,
0xEDB88320l,0x9ABFB3B6l,0x03B6E20Cl,0x74B1D29Al,0xEAD54739l,0x9DD277AFl,0x04DB2615l,0x73DC1683l,
0xE3630B12l,0x94643B84l,0x0D6D6A3El,0x7A6A5AA8l,0xE40ECF0Bl,0x9309FF9Dl,0x0A00AE27l,0x7D079EB1l,
0xF00F9344l,0x8708A3D2l,0x1E01F268l,0x6906C2FEl,0xF762575Dl,0x806567CBl,0x196C3671l,0x6E6B06E7l,
0xFED41B76l,0x89D32BE0l,0x10DA7A5Al,0x67DD4ACCl,0xF9B9DF6Fl,0x8EBEEFF9l,0x17B7BE43l,0x60B08ED5l,
0xD6D6A3E8l,0xA1D1937El,0x38D8C2C4l,0x4FDFF252l,0xD1BB67F1l,0xA6BC5767l,0x3FB506DDl,0x48B2364Bl,
0xD80D2BDAl,0xAF0A1B4Cl,0x36034AF6l,0x41047A60l,0xDF60EFC3l,0xA867DF55l,0x316E8EEFl,0x4669BE79l,
0xCB61B38Cl,0xBC66831Al,0x256FD2A0l,0x5268E236l,0xCC0C7795l,0xBB0B4703l,0x220216B9l,0x5505262Fl,
0xC5BA3BBEl,0xB2BD0B28l,0x2BB45A92l,0x5CB36A04l,0xC2D7FFA7l,0xB5D0CF31l,0x2CD99E8Bl,0x5BDEAE1Dl,
0x9B64C2B0l,0xEC63F226l,0x756AA39Cl,0x026D930Al,0x9C0906A9l,0xEB0E363Fl,0x72076785l,0x05005713l,
0x95BF4A82l,0xE2B87A14l,0x7BB12BAEl,0x0CB61B38l,0x92D28E9Bl,0xE5D5BE0Dl,0x7CDCEFB7l,0x0BDBDF21l,
0x86D3D2D4l,0xF1D4E242l,0x68DDB3F8l,0x1FDA836El,0x81BE16CDl,0xF6B9265Bl,0x6FB077E1l,0x18B74777l,
0x88085AE6l,0xFF0F6A70l,0x66063BCAl,0x11010B5Cl,0x8F659EFFl,0xF862AE69l,0x616BFFD3l,0x166CCF45l,
0xA00AE278l,0xD70DD2EEl,0x4E048354l,0x3903B3C2l,0xA7672661l,0xD06016F7l,0x4969474Dl,0x3E6E77DBl,
0xAED16A4Al,0xD9D65ADCl,0x40DF0B66l,0x37D83BF0l,0xA9BCAE53l,0xDEBB9EC5l,0x47B2CF7Fl,0x30B5FFE9l,
0xBDBDF21Cl,0xCABAC28Al,0x53B39330l,0x24B4A3A6l,0xBAD03605l,0xCDD70693l,0x54DE5729l,0x23D967BFl,
0xB3667A2El,0xC4614AB8l,0x5D681B02l,0x2A6F2B94l,0xB40BBE37l,0xC30C8EA1l,0x5A05DF1Bl,0x2D02EF8Dl };



#define updcrc32(cp, crc)  ( crc32tbl[ ( crc ^ cp ) & 0xff ] ^ ( crc >> 8 ) )


HB_FUNC( P_CRC32 ) {

   char *ptr = _parc( 1 );
   int count = _parclen( 1 );

   register ULONG crc = CRC32INIT;

   while ( count-- > 0 ) {
      crc = updcrc32( *ptr++, crc );
   }

   _retnl( crc ^ CRC32INIT );
}