/*
 * $Id$
 */

// What32
// Text display functions

#define HB_OS_WIN_32_USED
#define _WIN32_WINNT   0x0400

//#include <shlobj.h>
#include <windows.h>
#include "item.api"
//#include "hbapiitm.h"
#include "hbapi.h"
//#include "hbvm.h"
//#include "hbstack.h"

extern PHB_ITEM Rect2Array( RECT *rc  );
extern BOOL Array2Rect(PHB_ITEM aRect, RECT *rc );
extern PHB_ITEM Point2Array( POINT *pt  );
extern BOOL Array2Point(PHB_ITEM aPoint, POINT *pt );
extern BOOL Array2Size(PHB_ITEM aSize, SIZE *siz );
extern PHB_ITEM Size2Array( SIZE *siz  );
extern void Point2ArrayEx( POINT *pt  , PHB_ITEM aPoint);
extern void Rect2ArrayEx( RECT *pt  , PHB_ITEM aRect);
extern void Size2ArrayEx( SIZE *siz  ,  PHB_ITEM aSize);


//-----------------------------------------------------------------------------

// WINGDIAPI BOOL WINAPI TextOutA( IN HDC, IN int, IN int, IN LPCSTR, IN int);

// syntax
// TextOut(hDC, x, y, cStr) -> lSuccess

HB_FUNC( TEXTOUT )
{

   hb_retl( TextOut((HDC) hb_parnl( 1 )   ,   // handle of device context
                    hb_parni( 2 )         ,       // x-coordinate of starting position
                    hb_parni( 3 )         ,     // y-coordinate of starting position
                    (LPCTSTR) hb_parcx( 4 ),     // address of string
                    hb_parclen( 4 )            // number of characters in string
                   )
          );
}
//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI PolyTextOutA( IN HDC, IN CONST POLYTEXTA *, IN int);

/*

HB_FUNC( POLYTEXTOUTA )
{
   CONST POLYTEXTA ;

   // Your code goes here

   hb_retl( PolyTextOutA( (HDC) hb_parnl( 1 ), &POLYTEXTA, hb_parni( 3 ) ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI ExtTextOutA( IN HDC, IN int, IN int, IN UINT, IN CONST RECT *, IN LPCSTR, IN UINT, IN CONST INT *);

// syntax:
// ExtTextOut(hDC, x, y, fuFlags, [aRect], cStr, [aDx] ) -> lSuccess
// note: make sure that the aDx is of correct length , if passed


HB_FUNC( EXTTEXTOUT )
{
   RECT  rc    ;
   INT * lpDx = NULL;
   BOOL rcOk   ;
   UINT iCount ;
   UINT i      ;
   char * cText = hb_parcx( 6 );

   rcOk = ( ISARRAY(5) && Array2Rect(hb_param(5, HB_IT_ARRAY), &rc) ) ;

   if ( ISARRAY(7) )
   {
       iCount = hb_parinfa(7,0) ;
       lpDx = (INT *) hb_xgrab( iCount * sizeof( INT ) ) ;
       for ( i=0 ; i < iCount ; i++ )
       {
          *(lpDx+i) = hb_parni( 7,i+1) ;
       }
   }

   hb_retl( ExtTextOut( (HDC) hb_parnl( 1 )     ,
                         hb_parni( 2 )          ,
                         hb_parni( 3 )          ,
                         (UINT) hb_parni( 4 )   ,
                         rcOk ? &rc : NULL      ,
                         (LPCSTR) cText         ,
                         (UINT) strlen( cText ) ,
                         ISARRAY(7) ? lpDx : NULL
                         ) ) ;

   if (ISARRAY(7))
       hb_xfree(lpDx) ;

}

//-----------------------------------------------------------------------------
// int DrawText( HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat );

// syntax
// DrawText( hDC, cStr, aRect, [uFormat]) -> nTextHeight, or 0

HB_FUNC( DRAWTEXT )
{
   char *cText = hb_parcx( 2 );
   RECT rc;

   if ( ISARRAY( 3 ) && Array2Rect( hb_param( 3, HB_IT_ARRAY ), &rc ) )
      hb_retni( DrawText(
               (HDC) hb_parnl( 1 ),   // handle of device context
               (LPCTSTR) cText,           // address of string
               strlen( cText ),         // number of characters in string
               &rc,
               ISNIL(4) ? DT_LEFT : hb_parni( 4 ) ) ) ;
   else
      hb_retni( 0 );
}

//-----------------------------------------------------------------------------
// WINUSERAPI int WINAPI DrawTextExA( IN HDC, IN OUT LPSTR, IN int, IN OUT LPRECT, IN UINT, IN LPDRAWTEXTPARAMS);

// syntax
// DrawText( hDC, cStr, aRect, [uFormat],[DTParams]) -> nTextHeight, or 0

HB_FUNC( DRAWTEXTEX )
{
   char *cText = (char *) hb_parcx( 2 );
   RECT rc;
   DRAWTEXTPARAMS *dtp = NULL;

   if ( ISCHAR( 5 ))
      dtp = (DRAWTEXTPARAMS *) hb_parc( 5 ); //hb_param( 5, HB_IT_STRING )->item.asString.value;

   if ( ISARRAY( 3 ) && Array2Rect( hb_param( 3, HB_IT_ARRAY ), &rc ) )
      hb_retni( DrawTextEx( (HDC) hb_parnl( 1 ),     // handle of device context
                            (LPTSTR) cText    ,     // address of string
                            strlen( cText )    ,     // number of characters in string
                            (LPRECT) &rc,
                            ISNIL(4) ? DT_LEFT : hb_parni( 4 )       ,
                            ISCHAR(5) ? (LPDRAWTEXTPARAMS) dtp : NULL
                           ) ) ;
   else
      hb_retni( 0 ) ;
}


//-----------------------------------------------------------------------------
// WINUSERAPI LONG WINAPI TabbedTextOutA( IN HDC hDC, IN int X, IN int Y, IN LPCSTR lpString, IN int nCount, IN int nTabPositions, IN CONST INT *lpnTabStopPositions, IN int nTabOrigin);

// Syntax
// TabbedTextOut( hDC, x, y, cStr, aTabs, nOrigin )-> DWORD of width and height, or 0

HB_FUNC( TABBEDTEXTOUT )
{
   char *cText = hb_parcx( 4 );
   int iCount  ;
   int *aiTabs ;
   int i       ;

   if ( ISARRAY( 5 ) )
   {
      iCount = hb_parinfa(5,0) ;
      aiTabs = (INT *) hb_xgrab( iCount * sizeof( INT ) ) ;
      for ( i=0 ; i < iCount ; i++ )
      {
        *(aiTabs+i) = hb_parni( 5, i+1 ) ;
      }

      hb_retnl( (LONG) TabbedTextOut( (HDC) hb_parnl( 1 )  ,
                                      hb_parni( 2 )        ,
                                      hb_parni( 3 )        ,
                                      (LPCSTR) cText       ,
                                      strlen(cText)        ,
                                      iCount               ,
                                      aiTabs               ,
                                      hb_parni( 6 )
                                    ) ) ;
      hb_xfree( aiTabs ) ;

   }
   else
      hb_retnl( 0 ) ;
}

//-----------------------------------------------------------------------------
// WINGDIAPI int WINAPI GetTextFaceA( IN HDC, IN int, OUT LPSTR);

// Syntax
// GetTextFace(hDC) -> cTextFace , or NIL

HB_FUNC( GETTEXTFACE )
{
  char *cText = (char*) hb_xgrab(MAX_PATH);
  int iRet ;

  iRet = GetTextFace( (HDC) hb_parnl( 1 ), MAX_PATH , cText );
  if ( iRet )
     hb_retclen( cText, iRet ) ;

  hb_xfree( cText ) ;

}



//-----------------------------------------------------------------------------
// WINUSERAPI DWORD WINAPI GetTabbedTextExtentA( IN HDC hDC, IN LPCSTR lpString, IN int nCount, IN int nTabPositions, IN CONST INT *lpnTabStopPositions);

// Syntax
// GetTabbedTextExtent( hDC,cStr, aTabs )-> DWORD of width and height, or 0

HB_FUNC( GETTABBEDTEXTEXTENT )
{
   char *cText ;
   int iCount  ;
   int *aiTabs ;
   int i       ;

   if ( ISARRAY( 3 ) )
   {
      iCount = hb_parinfa(3,0) ;
      aiTabs = (INT *) hb_xgrab( iCount * sizeof( INT ) ) ;
      for ( i=0 ; i < iCount ; i++ )
      {
        *(aiTabs+i) = hb_parni( 3, i+1 ) ;
      }
      cText = hb_parcx( 2 );
      hb_retnl( (LONG) GetTabbedTextExtent( (HDC) hb_parnl( 1 )  ,
                                            (LPCTSTR) cText      ,
                                            strlen(cText)        ,
                                            iCount               ,
                                            aiTabs
                                          ) ) ;


      hb_xfree( aiTabs ) ;

   }
   else
      hb_retnl( 0 ) ;
}

//-----------------------------------------------------------------------------
// BOOL GetTextMetrics( HDC hdc, LPTEXTMETRIC lptm  );

// Syntax
// GetTextMetrics(hDC) -> TEXTMETRIC_Structure_Buffer, or NIL

HB_FUNC( GETTEXTMETRICS )
{
   TEXTMETRIC tm ;

   if ( GetTextMetrics( (HDC) hb_parnl( 1 ), &tm ) )
      hb_retclen( (char *) &tm, sizeof( TEXTMETRIC ) ) ;
}

//-----------------------------------------------------------------------------
// WINGDIAPI UINT APIENTRY GetOutlineTextMetricsA( IN HDC, IN UINT, OUT LPOUTLINETEXTMETRICA);

/*

HB_FUNC( GETOUTLINETEXTMETRICSA )
{
   LPOUTLINETEXTMETRICA lpoutLinetExtMetrica ;

   // Your code goes here

   hb_retni( GetOutlineTextMetricsA( (HDC) hb_parnl( 1 ) ,
                                     (UINT) hb_parni( 2 ),
                                     lpoutLinetExtMetrica
                                     ) ) ;
}
*/

//-----------------------------------------------------------------------------

// Syntax
// GetTextExtentPoint32( hDC, cStr ) -> aSize, or NIL

HB_FUNC( GETTEXTEXTENTPOINT32 )
{
   char * pstr = hb_parcx(2);
   SIZE sz;
   PHB_ITEM aMetr ;

   if ( GetTextExtentPoint32( (HDC) hb_parnl(1), pstr, strlen( pstr ), &sz ) )
   {
      aMetr = Size2Array( &sz ) ;
      _itemReturn( aMetr );
      _itemRelease( aMetr );
   }

}


//-----------------------------------------------------------------------------
// WINGDIAPI int WINAPI GetBkMode( IN HDC);


HB_FUNC( GETBKMODE )
{
   hb_retni( GetBkMode( (HDC) hb_parnl( 1 ) ) ) ;
}

//-----------------------------------------------------------------------------
// WINGDIAPI int WINAPI SetBkMode(IN HDC, IN int);


HB_FUNC( SETBKMODE )
{
   hb_retni( SetBkMode( (HDC) hb_parnl( 1 ), hb_parni( 2 ) ) ) ;
}

//-----------------------------------------------------------------------------
// WINGDIAPI UINT WINAPI GetTextAlign( IN HDC);


HB_FUNC( GETTEXTALIGN )
{
   hb_retni( GetTextAlign( (HDC) hb_parnl( 1 ) ) ) ;
}

//-----------------------------------------------------------------------------
// WINGDIAPI UINT WINAPI SetTextAlign(IN HDC, IN UINT);


HB_FUNC( SETTEXTALIGN )
{
   hb_retni( SetTextAlign( (HDC) hb_parnl( 1 ), (UINT) hb_parni( 2 ) ) ) ;
}

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI SetTextJustification(IN HDC, IN int, IN int);


HB_FUNC( SETTEXTJUSTIFICATION )
{
   hb_retl( SetTextJustification( (HDC) hb_parnl( 1 ),
                                  hb_parni( 2 )      ,
                                  hb_parni( 3 )
                                  ) ) ;
}


//-----------------------------------------------------------------------------
// WINGDIAPI int WINAPI GetTextCharacterExtra( IN HDC);


HB_FUNC( GETTEXTCHARACTEREXTRA )
{
   hb_retni( GetTextCharacterExtra( (HDC) hb_parnl( 1 ) ) ) ;
}

//-----------------------------------------------------------------------------
// WINGDIAPI int WINAPI SetTextCharacterExtra(IN HDC, IN int);


HB_FUNC( SETTEXTCHARACTEREXTRA )
{
   hb_retni( SetTextCharacterExtra( (HDC) hb_parnl( 1 ), hb_parni( 2 ) ) ) ;
}



//-----------------------------------------------------------------------------
// WINGDIAPI int WINAPI GetTextCharset( IN HDC hdc);


HB_FUNC( GETTEXTCHARSET )
{
   hb_retni( GetTextCharset( (HDC) hb_parnl( 1 ) ) ) ;
}


//-----------------------------------------------------------------------------
// WINUSERAPI BOOL WINAPI GrayStringA( IN HDC hDC, IN HBRUSH hBrush, IN GRAYSTRINGPROC lpOutputFunc, IN LPARAM lpData, IN int nCount, IN int X, IN int Y, IN int nWidth, IN int nHeight);

// tbd

/*

HB_FUNC( GRAYSTRING )
{
   GRAYSTRINGPROC lpOutputFunc ;

   // Your code goes here

   hb_retl( GrayString( (HDC) hb_parnl( 1 )   ,
                        (HBRUSH) hb_parnl( 2 ),
                        lpOutputFunc          ,
                        (LPARAM) hb_parnl( 4 ),
                        hb_parni( 5 )         ,
                        hb_parni( 6 )         ,
                        hb_parni( 7 )         ,
                        hb_parni( 8 )         ,
                        hb_parni( 9 )
                      ) ) ;
}

*/


//-----------------------------------------------------------------------------
// WINGDIAPI BOOL APIENTRY GetCharABCWidthsA( IN HDC, IN UINT, IN UINT, OUT LPABC);

/*

HB_FUNC( GETCHARABCWIDTHSA )
{
   LPABC lpabc ;

   // Your code goes here

   hb_retl( GetCharABCWidthsA( (HDC) hb_parnl( 1 ) ,
                               (UINT) hb_parni( 2 ),
                               (UINT) hb_parni( 3 ),
                               lpabc
                               ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL APIENTRY GetCharABCWidthsFloatA( IN HDC, IN UINT, IN UINT, OUT LPABCFLOAT);

/*

HB_FUNC( GETCHARABCWIDTHSFLOATA )
{
   LPABCFLOAT lpabcFloat ;

   // Your code goes here

   hb_retl( GetCharABCWidthsFloatA( (HDC) hb_parnl( 1 ) ,
                                    (UINT) hb_parni( 2 ),
                                    (UINT) hb_parni( 3 ),
                                    lpabcFloat
                                    ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI GetCharABCWidthsI( IN HDC, IN UINT, IN UINT, IN LPWORD, OUT LPABC);

/*

HB_FUNC( GETCHARABCWIDTHSI )
{
   LPWORD lpWord ;
   LPABC  lpabc  ;

   // Your code goes here

   hb_retl( GetCharABCWidthsI( (HDC) hb_parnl( 1 ) ,
                               (UINT) hb_parni( 2 ),
                               (UINT) hb_parni( 3 ),
                               lpWord              ,
                               lpabc
                               ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI DWORD WINAPI GetCharacterPlacementA( IN HDC, IN LPCSTR, IN int, IN int, IN OUT LPGCP_RESULTSA, IN DWORD);

/*

HB_FUNC( GETCHARACTERPLACEMENTA )
{
   LPGCP_RESULTSA lpgcp_resultsa ;

   // Your code goes here

   hb_retnl( (LONG) GetCharacterPlacementA( (HDC) hb_parnl( 1 )  ,
                                            (LPCSTR) hb_parcx( 2 ),
                                            hb_parni( 3 )        ,
                                            hb_parni( 4 )        ,
                                            lpgcp_resultsa       ,
                                            (DWORD) hb_parnl( 6 )
                                            ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI GetCharWidth32A( IN HDC, IN UINT, IN UINT, OUT LPINT);

/*

HB_FUNC( GETCHARWIDTH32A )
{
   LPINT lpInt ;

   // Your code goes here

   hb_retl( GetCharWidth32A( (HDC) hb_parnl( 1 ) ,
                             (UINT) hb_parni( 2 ),
                             (UINT) hb_parni( 3 ),
                             lpInt
                             ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI GetCharWidthA( IN HDC, IN UINT, IN UINT, OUT LPINT);

/*

HB_FUNC( GETCHARWIDTHA )
{
   LPINT lpInt ;

   // Your code goes here

   hb_retl( GetCharWidthA( (HDC) hb_parnl( 1 ) ,
                           (UINT) hb_parni( 2 ),
                           (UINT) hb_parni( 3 ),
                           lpInt
                           ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL APIENTRY GetCharWidthFloatA( IN HDC, IN UINT, IN UINT, OUT PFLOAT);

/*

HB_FUNC( GETCHARWIDTHFLOATA )
{
   PFLOAT pFloat ;

   // Your code goes here

   hb_retl( GetCharWidthFloatA( (HDC) hb_parnl( 1 ) ,
                                (UINT) hb_parni( 2 ),
                                (UINT) hb_parni( 3 ),
                                pFloat
                                ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI GetCharWidthI( IN HDC, IN UINT, IN UINT, IN LPWORD, OUT LPINT);

/*

HB_FUNC( GETCHARWIDTHI )
{
   LPWORD lpWord ;
   LPINT  lpInt  ;

   // Your code goes here

   hb_retl( GetCharWidthI( (HDC) hb_parnl( 1 ) ,
                           (UINT) hb_parni( 2 ),
                           (UINT) hb_parni( 3 ),
                           lpWord              ,
                           lpInt
                           ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI DWORD WINAPI GetKerningPairsA( IN HDC, IN DWORD, OUT LPKERNINGPAIR);

/*

HB_FUNC( GETKERNINGPAIRSA )
{
   LPKERNINGPAIR lpkerningpair ;

   // Your code goes here

   hb_retnl( (LONG) GetKerningPairsA( (HDC) hb_parnl( 1 )  ,
                                      (DWORD) hb_parnl( 2 ),
                                      lpkerningpair
                                      ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI int WINAPI GetTextCharsetInfo( IN HDC hdc, OUT LPFONTSIGNATURE lpSig, IN DWORD dwFlags);

/*

HB_FUNC( GETTEXTCHARSETINFO )
{
   LPFONTSIGNATURE lpSig   ;

   // Your code goes here

   hb_retni( GetTextCharsetInfo( (HDC) hb_parnl( 1 )  ,
                                 lpSig                ,
                                 (DWORD) hb_parnl( 3 )
                                 ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL APIENTRY GetTextExtentExPointA( IN HDC, IN LPCSTR, IN int, IN int, OUT LPINT, OUT LPINT, OUT LPSIZE );

/*

HB_FUNC( GETTEXTEXTENTEXPOINTA )
{
   LPINT  lpInt1 ;
   LPINT  lpInt2 ;
   LPSIZE lpSize ;

   // Your code goes here

   hb_retl( GetTextExtentExPointA( (HDC) hb_parnl( 1 )  ,
                                   (LPCSTR) hb_parcx( 2 ),
                                   hb_parni( 3 )        ,
                                   hb_parni( 4 )        ,
                                   lpInt1               ,
                                   lpInt2               ,
                                   lpSize
                                   ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI GetTextExtentExPointI( IN HDC, IN LPWORD, IN int, IN int, OUT LPINT, OUT LPINT, OUT LPSIZE);

/*

HB_FUNC( GETTEXTEXTENTEXPOINTI )
{
   LPWORD lpWord ;
   LPINT  lpInt1 ;
   LPINT  lpInt2 ;
   LPSIZE lpSize ;

   // Your code goes here

   hb_retl( GetTextExtentExPointI( (HDC) hb_parnl( 1 ),
                                   lpWord             ,
                                   hb_parni( 3 )      ,
                                   hb_parni( 4 )      ,
                                   lpInt1             ,
                                   lpInt2             ,
                                   lpSize
                                   ) ) ;
}

*/


//-----------------------------------------------------------------------------
// WINGDIAPI BOOL APIENTRY GetTextExtentPointA( IN HDC, IN LPCSTR, IN int, OUT LPSIZE );

/*

HB_FUNC( GETTEXTEXTENTPOINTA )
{
   LPSIZE lpSize ;

   // Your code goes here

   hb_retl( GetTextExtentPointA( (HDC) hb_parnl( 1 )  ,
                                 (LPCSTR) hb_parcx( 2 ),
                                 hb_parni( 3 )        ,
                                 lpSize
                                 ) ) ;
}

*/

//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI GetTextExtentPointI( IN HDC, IN LPWORD, IN int, OUT LPSIZE);

/*

HB_FUNC( GETTEXTEXTENTPOINTI )
{
   LPWORD lpWord ;
   LPSIZE lpSize ;

   // Your code goes here

   hb_retl( GetTextExtentPointI( (HDC) hb_parnl( 1 ),
                                 lpWord             ,
                                 hb_parni( 3 )      ,
                                 lpSize
                                 ) ) ;
}

*/



//-----------------------------------------------------------------------------
// WINGDIAPI BOOL WINAPI TranslateCharsetInfo( IN OUT DWORD FAR *lpSrc, OUT LPCHARSETINFO lpCs, IN DWORD dwFlags);

/*

HB_FUNC( TRANSLATECHARSETINFO )
{
   LPCHARSETINFO lpCs    ;

   // Your code goes here

   hb_retl( TranslateCharsetInfo( (DWORD) hb_parnl( 1 ),
                                  &lpCs                ,
                                  (DWORD) hb_parnl( 3 )
                                  ) ) ;
}

*/


