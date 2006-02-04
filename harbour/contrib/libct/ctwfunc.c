#include "hbapi.h"
#include "hbapigt.h"
#include "ctwin.h"

extern int hb_gt_GetClearColor( void );
extern int hb_gt_GetClearChar( void );
extern void hb_gt_SetClearColor( int );
extern void hb_gt_SetClearChar( int );
extern void hb_gt_Flush( void );

HB_FUNC( CTWINIT )
{
   hb_retl( hb_ctw_Init() );
}

HB_FUNC( SETCLEARA )
{
   int iRet = hb_gt_GetClearColor();

   if( ISNUM( 1 ) )
   {
      int iNew = hb_parni( 1 );
      if( iNew >= 0 )
         hb_gt_SetClearColor( iNew );
   }
   hb_retni( iRet );
}

HB_FUNC( SETCLEARB )
{
   int iRet = hb_gt_GetClearChar();

   if( ISNUM( 1 ) )
   {
      int iNew = hb_parni( 1 );
      if( iNew >= 0 )
         hb_gt_SetClearChar( iNew );
   }
   hb_retni( iRet );
}

HB_FUNC( WSETSHADOW )
{
   hb_retni( hb_ctw_SetShadowAttr( ISNUM( 1 ) ? hb_parni( 1 ) : -2 ) );
}

HB_FUNC( WSETMOVE )
{
   hb_retl( hb_ctw_SetMoveMode( ISLOG( 1 ) ? hb_parl( 1 ) : -1 ) != 0 );
}

HB_FUNC( WSTEP )
{
   if( ISNUM( 1 ) && ISNUM( 2 ) )
      hb_retni( hb_ctw_SetMoveStep( hb_parni( 1 ), hb_parni( 2 ) ) );
   else
      hb_retni( -1 );
}

HB_FUNC( WMODE )
{
   hb_retni( hb_ctw_SetBorderMode( ISLOG( 1 ) ? ( hb_parl( 1 ) ? 1 : 0 ) : -1,
                                   ISLOG( 2 ) ? ( hb_parl( 2 ) ? 1 : 0 ) : -1,
                                   ISLOG( 3 ) ? ( hb_parl( 3 ) ? 1 : 0 ) : -1,
                                   ISLOG( 4 ) ? ( hb_parl( 4 ) ? 1 : 0 ) : -1 ) );
}

HB_FUNC( WBOARD )
{
   int iResult;

   iResult = hb_ctw_SetWindowBoard( hb_parni( 1 ), hb_parni( 2 ),
                                    ISNUM( 3 ) ? hb_parni( 3 ) : hb_gtMaxRow(),
                                    ISNUM( 4 ) ? hb_parni( 4 ) : hb_gtMaxCol() );
   hb_gt_Flush();
   hb_retni( iResult );
}

HB_FUNC( WOPEN )
{
   int iResult;

   iResult = hb_ctw_CreateWindow( hb_parni( 1 ), hb_parni( 2 ),
                                  hb_parni( 3 ), hb_parni( 4 ),
                                  hb_parl( 5 ) );
   hb_gt_Flush();
   hb_retni( iResult );
}

HB_FUNC( WCLOSE )
{
   int iResult;

   iResult = hb_ctw_CloseWindow( hb_ctw_CurrentWindow() );
   hb_gt_Flush();
   hb_retni( iResult );
}

HB_FUNC( WACLOSE )
{
   int iResult;

   iResult = hb_ctw_CloseAllWindows();
   hb_gt_Flush();
   hb_retni( iResult );
}

HB_FUNC( WSELECT )
{
   int iResult;

   if( ISNUM( 1 ) )
      iResult = hb_ctw_SelectWindow( hb_parni( 1 ) );
   else
      iResult =  hb_ctw_CurrentWindow();
   hb_gt_Flush();
   hb_retni( iResult );
}

HB_FUNC( WNUM )
{
   hb_retni( hb_ctw_MaxWindow() );
}

HB_FUNC( WBOX )
{
   static const char * pWBoxFrames[] = {
            _B_DOUBLE,        /* 0  WB_DOUBLE_CLEAR */
            _B_SINGLE,        /* 1  WB_SINGLE_CLEAR */
            _B_DOUBLE_SINGLE, /* 2  WB_DOUBLE_SINGLE_CLEAR */
            _B_SINGLE_DOUBLE, /* 3  WB_SINGLE_DOUBLE_CLEAR */

            _B_DOUBLE,        /* 4  WB_DOUBLE */
            _B_SINGLE,        /* 5  WB_SINGLE */
            _B_DOUBLE_SINGLE, /* 6  WB_DOUBLE_SINGLE */
            _B_SINGLE_DOUBLE, /* 7  WB_SINGLE_DOUBLE */

            "��������",       /* 8  WB_HALF_FULL_CLEAR */
            "��������",       /* 9  WB_HALF_CLEAR */
            "��������",       /* 10 WB_FULL_HALF_CLEAR */
            "��������",       /* 11 WB_FULL_CLEAR */

            "��������",       /* 12 WB_HALF_FULL */
            "��������",       /* 13 WB_HALF */
            "��������",       /* 14 WB_FULL_HALF */
            "��������"  };    /* 15 WB_FULL */

   BYTE * szBox, szBoxBuf[ 10 ];
   int iResult;

   if( ISCHAR( 1 ) )
   {
      szBox = ( BYTE * ) hb_parc( 1 );
   }
   else
   {
      int iFrame = hb_parni( 1 );

      if( iFrame < 0 || iFrame > 15 )
         iFrame = 0;
      memcpy( szBoxBuf, pWBoxFrames[ iFrame ], 9 );
      if( ( iFrame & 4 ) == 0 )
      {
         szBoxBuf[ 8 ] = hb_gt_GetClearChar();
      }
      szBoxBuf[ 9 ] = '0';
      szBox = szBoxBuf;
   }

   iResult = hb_ctw_AddWindowBox( hb_ctw_CurrentWindow(), szBox );
   hb_gt_Flush();
   hb_retni( iResult );
}

HB_FUNC( WFORMAT )
{
   hb_retni( hb_ctw_ChangeMargins( hb_ctw_CurrentWindow(),
                                   hb_parni( 1 ), hb_parni( 1 ),
                                   hb_parni( 2 ), hb_parni( 3 ) ) );
}

HB_FUNC( WROW )
{
   int iTop, iLeft, iBottom, iRight;

   hb_ctw_GetWindowCords( hb_ctw_CurrentWindow(), hb_parl( 1 ), &iTop, &iLeft, &iBottom, &iRight );
   hb_retni( iTop );
}

HB_FUNC( WCOL )
{
   int iTop, iLeft, iBottom, iRight;

   hb_ctw_GetWindowCords( hb_ctw_CurrentWindow(), hb_parl( 1 ), &iTop, &iLeft, &iBottom, &iRight );
   hb_retni( iLeft );
}

HB_FUNC( WLASTROW )
{
   int iTop, iLeft, iBottom, iRight;

   hb_ctw_GetWindowCords( hb_ctw_CurrentWindow(), hb_parl( 1 ), &iTop, &iLeft, &iBottom, &iRight );
   hb_retni( iBottom );
}

HB_FUNC( WLASTCOL )
{
   int iTop, iLeft, iBottom, iRight;

   hb_ctw_GetWindowCords( hb_ctw_CurrentWindow(), hb_parl( 1 ), &iTop, &iLeft, &iBottom, &iRight );
   hb_retni( iRight );
}

HB_FUNC( WFROW )
{
   int iTop, iLeft, iBottom, iRight;

   hb_ctw_GetFormatCords( hb_ctw_CurrentWindow(), hb_parl( 1 ), &iTop, &iLeft, &iBottom, &iRight );
   hb_retni( iTop );
}

HB_FUNC( WFCOL )
{
   int iTop, iLeft, iBottom, iRight;

   hb_ctw_GetFormatCords( hb_ctw_CurrentWindow(), hb_parl( 1 ), &iTop, &iLeft, &iBottom, &iRight );
   hb_retni( iLeft );
}

HB_FUNC( WFLASTROW )
{
   int iTop, iLeft, iBottom, iRight;

   hb_ctw_GetFormatCords( hb_ctw_CurrentWindow(), hb_parl( 1 ), &iTop, &iLeft, &iBottom, &iRight );
   hb_retni( iBottom );
}

HB_FUNC( WFLASTCOL )
{
   int iTop, iLeft, iBottom, iRight;

   hb_ctw_GetFormatCords( hb_ctw_CurrentWindow(), hb_parl( 1 ), &iTop, &iLeft, &iBottom, &iRight );
   hb_retni( iRight );
}

HB_FUNC( WCENTER )
{
   hb_retni( hb_ctw_CenterWindow( hb_ctw_CurrentWindow(), hb_parl( 1 ) ) );
}

HB_FUNC( WMOVE )
{
   hb_retni( hb_ctw_MoveWindow( hb_ctw_CurrentWindow(),
                                hb_parni( 1 ), hb_parni( 2 ) ) );
}
