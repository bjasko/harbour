/*
 * $Id$
 */
/*
 * Harbour Project source code:
 * hbmlang.c Hbmake detection language function
 *
 * Copyright 2000,2001 Luiz Rafael Culik <culik@sl.conex.net>
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


#define HB_OS_WIN_32_USED
#include <hbapi.h>
#include <stdio.h>

HB_FUNC(GETUSERLANG)
{
   long lRet ;

#if defined(HB_OS_WIN_32) && (!defined(__RSXNT__)) && (!defined(__CYGWIN__))

   {
   
      LANGID pLang=GetSystemDefaultLangID();

      switch(pLang) {

         case 0x0416:
         case 0x0816:
         {
            lRet=1;
         }

         break;

         case 0x0409 : 
         case 0x0809 : 
         case 0x0c09 : 
         case 0x1009 : 
         case 0x1409 : 
         case 0x1809 : 
         case 0x1c09 : 
         case 0x2009 : 
         case 0x2409 : 
         case 0x2809 : 
         case 0x2c09 :
         {
            lRet=2;
         }

         break;

         case 0x040a :
         case 0x080a :   
         case 0x0c0a :   
         case 0x100a :   
         case 0x140a :   
         case 0x180a :   
         case 0x1c0a :   
         case 0x200a :   
         case 0x240a :   
         case 0x280a :   
         case 0x2c0a :   
         case 0x300a :   
         case 0x340a :   
         case 0x380a :   
         case 0x3c0a :   
         case 0x400a :   
         case 0x440a :   
         case 0x480a :   
         case 0x4c0a :   
         case 0x500a :
         {
            lRet=3;
         }        
         break;

      default:

      lRet=2;

      break; 

      }                  

   }
#else
   lRet = 2 ;
#endif
     hb_retnl( lRet ); 
}


