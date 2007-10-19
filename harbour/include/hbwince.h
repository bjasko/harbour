/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * 
 *
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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

#ifndef HB_WINCE_H_
#define HB_WINCE_H_

#if defined(HB_WINCE)
#  undef  OS_HAS_DRIVE_LETTER

/* defined(__CEGCC__) || defined(__MINGW32CE__) */

#if defined(__MINGW32CE__)
typedef long clock_t;
extern clock_t clock( void );
#endif

extern int remove( const char *filename );
extern int access( const char *pathname, int mode );
extern int system( const char *string );
extern char *strerror( int errnum );

#endif /* HB_WINCE */

#if defined(HB_OS_WIN_32)

extern wchar_t * hb_mbtowc( const char *srcA );
extern char * hb_wctomb( const wchar_t *srcW );
extern void hb_mbtowccpy( wchar_t *dstW, const char *srcA, unsigned long ulLen );
extern void hb_mbtowcset( wchar_t *dstW, const char *srcA, unsigned long ulLen );
extern void hb_wctombget( char *dstA, const wchar_t *srcW, unsigned long ulLen );

#if defined(UNICODE)

#define HB_TCHAR_CPTO(d,s,l)        hb_mbtowccpy(d,s,l)
#define HB_TCHAR_GETFROM(d,s,l)     hb_wctombget(d,s,l)
#define HB_TCHAR_SETTO(d,s,l)       hb_mbtowcset(d,s,l)
#define HB_TCHAR_CONVTO(s)          hb_mbtowc(s)
#define HB_TCHAR_CONVFROM(s)        hb_wctomb(s)
#define HB_TCHAR_FREE(s)            hb_xfree(s)

#else

#define HB_TCHAR_CPTO(d,s,l)        hb_strncpy(d,s,l)
#define HB_TCHAR_SETTO(d,s,l)       memcpy(d,s,l)
#define HB_TCHAR_GETFROM(d,s,l)     memcpy(d,s,l)
#define HB_TCHAR_CONVTO(s)          (s)
#define HB_TCHAR_CONVFROM(s)        (s)
#define HB_TCHAR_FREE(s)            HB_SYMBOL_UNUSED(s)

#endif /* UNICODE */

#endif /* HB_OS_WIN_32 */

#endif /* HB_WINCE_H_ */
