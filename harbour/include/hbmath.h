/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Harbour math functions and API
 *
 * Copyright 2001 IntTec GmbH, Neunlindenstr 32, 79106 Freiburg, Germany
 *        Author: Martin Vogel <vogel@inttec.de>
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

#ifndef HB_MATH_H_
#define HB_MATH_H_

#include "hbapi.h"

#if defined(__DJGPP__)
#include <libm/math.h>
#else
#include <math.h>
#endif

#if defined(HB_EXTERN_C)
extern "C" {
#endif

#if defined(__WATCOMC__)
   #define HB_MATH_HANDLER
   #define exception _exception
#elif defined(__BORLANDC__)
   #if (__BORLANDC__ == 1328) && defined(__cplusplus)
      /* NOTE: There seem to be a bug in Borland C++ 5.3 C++ mode which prevents
               the redefinition of matherr, because nor "_exception" neither
               "exception" will work. [vszakats] */
   #else
      #define HB_MATH_HANDLER
      #define matherr _matherr
      /* NOTE: This is needed for Borland C++ 5.5 in C++/STDC mode. [vszakats] */
      #if (__BORLANDC__ >= 1360)
         #define exception _exception
      #endif
   #endif
#elif defined(__MINGW32__)
   #define HB_MATH_HANDLER
   #define matherr _matherr
   #define exception _exception
#elif defined(__DJGPP__)
   #define HB_MATH_HANDLER
   _LIB_VERSION_TYPE _LIB_VERSION = _XOPEN_;
#endif

typedef struct _HB_MATH_EXCEPTION
{
  int    type;
  char * funcname;
  char * error;
  double arg1;
  double arg2;
  double retval;
  int    retvalwidth;
  int    retvaldec;
  int    handled;
} HB_MATH_EXCEPTION;

typedef int (* HB_MATH_HANDLERPROC)(HB_MATH_EXCEPTION * err);

extern void hb_mathResetError (void);
extern int hb_mathGetLastError (HB_MATH_EXCEPTION * phb_exc);
extern int hb_mathIsMathErr (void);

extern int hb_mathSetDefErrMode (int imode);
extern int hb_mathGetDefErrMode (void);
extern int hb_matherr (HB_MATH_EXCEPTION * pexc);

extern HB_MATH_HANDLERPROC hb_mathSetHandler (HB_MATH_HANDLERPROC handlerproc);
extern HB_MATH_HANDLERPROC hb_mathGetHandler (void);

/* include defines from math.ch */
#include <math.ch>

#if defined(HB_EXTERN_C)
}
#endif

#endif /* HB_MATH_H_ */
