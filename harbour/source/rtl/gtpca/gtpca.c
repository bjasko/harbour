/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Video subsystem for ANSI terminals
 *
 * Copyright 2000 David G. Holm <dholm@jsd-llc.com>
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

/*
 *  This module is partially based on VIDMGR by Andrew Clarke and modified
 *  for the Harbour project
 */

/* NOTE: User programs should never call this layer directly! */

#if defined(__GNUC__) && ! defined(__MINGW32__)
   #include <unistd.h>
   #if defined(__DJGPP__) || defined(__CYGWIN__) || defined(__EMX__)
      #include <io.h>
   #endif
#else
   #include <io.h>
#endif

#include <ctype.h>
#include <string.h>

#include "hbapigt.h"
#include "hbset.h"
#include "inkey.ch"

static USHORT s_usRow, s_usCol, s_usMaxRow, s_usMaxCol;
static int s_iFilenoStdin, s_iFilenoStdout, s_iFilenoStderr;
static int s_iAttribute;
static BOOL s_bColor;
static char s_szSpaces[] = "                                                                                    "; /* 84 spaces */

static void hb_gt_AnsiGetCurPos( USHORT * row, USHORT * col );

static USHORT s_uiDispCount;

void hb_gt_Init( int iFilenoStdin, int iFilenoStdout, int iFilenoStderr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Init()"));
   s_iAttribute = 0;
   s_iFilenoStdin = iFilenoStdin;
   s_iFilenoStdout = iFilenoStdout;
   s_iFilenoStderr = iFilenoStderr;
   s_usRow = s_usCol = 0;
   hb_gt_AnsiGetCurPos( &s_usRow, &s_usCol );
#ifdef OS_UNIX_COMPATIBLE
   s_usMaxRow = 23;
   s_bColor = FALSE;
#else
   s_usMaxRow = 24;
   s_bColor = TRUE;
#endif
   s_usMaxCol = 79;
   fprintf( stdout, "\x1B[=7h" ); /* Enable line wrap (for OUTSTD() and OUTERR()) */
   
   hb_mouse_Init();
}

void hb_gt_Exit( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Exit()"));
   
   hb_mouse_Exit();
   /* TODO: */
}

static void hb_gt_AnsiGetCurPos( USHORT * row, USHORT * col )
{
   if( isatty( s_iFilenoStdin ) && isatty( s_iFilenoStdout ) )
   {
      USHORT ch, value = 0, index = 0;
      fprintf( stdout, "\x1B[6n" );
      do
      {
        ch = getc( stdin );
        if( isdigit( ch ) )
        {
           value = ( value * 10 ) + ( ch - '0' );
        }
        else if( ch == ';' )
        {
           *row = value - 1;
           value = 0;
        }
      }
      while( ch != 'R' && index < 10 );
      *col = value - 1;
   }
}

static void hb_gt_AnsiSetAttributes( BYTE attr )
{
   static const int color[] = { 0, 4, 2, 6, 1, 5, 3, 7 };
   int bg_color = 40 + color[ ( attr & 0x70 ) >> 4 ];
   int fg_color = 30 + color[ attr & 0x07 ];
   int special = 0;
   if( attr & 0x08 ) special = 1;
   else if( attr & 0x80 )
   {
      if( hb_set.HB_SET_INTENSITY ) special = 1;
      else special = 5;
   }
   fprintf( stdout, "\x1B[%d;%d;%dm", special, fg_color, bg_color );
}

BOOL hb_gt_AdjustPos( BYTE * pStr, ULONG ulLen )
{
   USHORT row = s_usRow;
   USHORT col = s_usCol;
   ULONG ulCount;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_AdjustPos(%s, %lu)", pStr, ulLen ));

   for( ulCount = 0; ulCount < ulLen; ulCount++ )
   {
      switch( *pStr++  )
      {
         case HB_CHAR_BEL:
            break;

         case HB_CHAR_BS:
            if( col )
               col--;
            else
            {
               col = s_usMaxCol;
               if( row )
                  row--;
            }
            break;

         case HB_CHAR_LF:
            if( row < s_usMaxRow )
               row++;
            break;

         case HB_CHAR_CR:
            col = 0;
            break;

         default:
            if( col < s_usMaxCol )
               col++;
            else
            {
               col = 0;
               if( row < s_usMaxRow )
                  row++;
            }
      }
   }
   hb_gt_SetPos( row, col, HB_GT_SET_POS_AFTER );
   return TRUE;
}

int hb_gt_ExtendedKeySupport()
{
   return 0;
}

#ifdef HARBOUR_GCC_OS2
   #include "..\..\kbdos2.gcc"
#else
int hb_gt_ReadKey( HB_inkey_enum eventmask )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_ReadKey(%d)", (int) eventmask));
   HB_SYMBOL_UNUSED( eventmask );
   /* TODO: */
   return 0;
}
#endif

BOOL hb_gt_IsColor( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_IsColor()"));
   /* TODO: */
   return s_bColor;
}

USHORT hb_gt_GetScreenWidth( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetScreenWidth()"));
   /* TODO: */
   return s_usMaxCol + 1;
}

USHORT hb_gt_GetScreenHeight( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetScreenHeight()"));
   /* TODO: */
   return s_usMaxRow + 1;
}

void hb_gt_SetPos( SHORT iRow, SHORT iCol, SHORT iMethod )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetPos(%hd, %hd, %hd)", iRow, iCol, iMethod));

   HB_SYMBOL_UNUSED( iMethod );

   if( iRow < 0 ) iRow = 0;
   else if( iRow > s_usMaxRow ) iRow = s_usMaxRow;
   if( iCol < 0 ) iCol = 0;
   else if( iCol > s_usMaxCol ) iCol = s_usMaxCol;
   s_usRow = iRow;
   s_usCol = iCol;
   fprintf( stdout, "\x1B[%d;%dH", s_usRow + 1, s_usCol + 1 );
}

SHORT hb_gt_Row( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Row()"));
   return s_usRow;
}

SHORT hb_gt_Col( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Col()"));
   return s_usCol;
}


void hb_gt_Scroll( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE attr, SHORT sVert, SHORT sHoriz )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Scroll(%hu, %hu, %hu, %hu, %d, %hd, %hd)", usTop, usLeft, usBottom, usRight, (int) attr, sVert, sHoriz));
   HB_SYMBOL_UNUSED( attr );
   if( sVert == 0 && sHoriz == 0 )
   {
      /* Clear */
      if( usTop == 0 && usLeft == 0 && usBottom >= s_usMaxRow && usRight >= s_usMaxCol )
      {
         /* Clear the entire screen */
         fprintf( stdout, "\x1B[2J" );
      }
      else
      {
         /* Clear a screen region */
         USHORT i;
         for( i = usTop; i <= usBottom; i++ )
         {
            hb_gt_Puts( i, usLeft, s_iAttribute, ( BYTE * )s_szSpaces, (usRight - usLeft ) + 1 );
         }
      }
   }
   else
   {
      if( usTop == 0 && usLeft == 0 && usBottom >= s_usMaxRow && usRight >= s_usMaxCol )
      {
         if( sVert > 0 && sHoriz == 0 )
         {
            /* Scroll the entire screen up */
            fprintf( stdout, "\x1B[25;80" );
            while( sVert-- ) fputc( '\n', stdout );
         }
         else
         {
            /* TODO: Scroll the entire screen any direction other than up */
         }
      }
      else
      {
         /* TODO: Scroll a screen region */
      }
   }
}

USHORT hb_gt_GetCursorStyle( void )
{
   /* TODO: What shape is the cursor? */
   USHORT uiStyle = 0;
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetCursorStyle()"));
   return uiStyle;
}

void hb_gt_SetCursorStyle( USHORT style )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetCursorStyle(%hu)", style));
   HB_SYMBOL_UNUSED( style );
}

static void hb_gt_xPutch( USHORT usRow, USHORT usCol, BYTE attr, BYTE byChar )
{
   char tmp[ 2 ];

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_xPutch(%hu, %hu, %d, %i)", usRow, usCol, (int) attr, byChar));

   /* TOFIX: add correct support for a single byte instead of a string 
    */
   tmp[ 0 ] = byChar;
   tmp[ 1 ] = '\0';

   /* Disable line wrap, set the new cursor position, send the string, then
      enable line wrap (for OUTSTD() and OUTERR() ) */
   hb_gt_AnsiSetAttributes( attr );
   fprintf( stdout, "\x1B[=7l\x1B[%d;%dH%s\x1B[=7h", usRow + 1, usCol + 1, tmp );

   /* Restore whatever used to be at the termination position */
   /* Update the cursor position */
   s_usRow = usRow;
   s_usCol = usCol + 1;
   if( s_usCol > s_usMaxCol ) s_usCol = s_usMaxCol;
}

void hb_gt_PutCharAttr( SHORT uiRow, SHORT uiCol, BYTE byChar, BYTE byAttr )
{
   char tmp[ 2 ];

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_PutCharAttr(%hu, %hu, %i, %d)", uiRow, uiCol, byChar, (int) byAttr));

   /* TOFIX: add correct support for a single byte instead of a string 
    */
   tmp[ 0 ] = byChar;
   tmp[ 1 ] = '\0';

   /* Disable line wrap, set the new cursor position, send the string, then
      enable line wrap (for OUTSTD() and OUTERR() ) */
   hb_gt_AnsiSetAttributes( byAttr );
   fprintf( stdout, "\x1B[=7l\x1B[%d;%dH%s\x1B[=7h", uiRow + 1, uiCol + 1, tmp );

   /* Restore whatever used to be at the termination position */
   /* Update the cursor position */
   s_usRow = uiRow;
   s_usCol = uiCol + 1;
   if( s_usCol > s_usMaxCol ) s_usCol = s_usMaxCol;
}

void hb_gt_PutChar( SHORT uiRow, SHORT uiCol, BYTE byChar )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_PutChar(%hu, %hu, %i)", uiRow, uiCol, byChar));

   /* TODO */

   HB_SYMBOL_UNUSED( uiRow );
   HB_SYMBOL_UNUSED( uiCol );
   HB_SYMBOL_UNUSED( byChar );
}

void hb_gt_PutAttr( SHORT uiRow, SHORT uiCol, BYTE byAttr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_PutAttr(%hu, %hu, %d)", uiRow, uiCol, (int) byAttr));

   /* TODO */

   HB_SYMBOL_UNUSED( uiRow );
   HB_SYMBOL_UNUSED( uiCol );
   HB_SYMBOL_UNUSED( byAttr );
}

void hb_gt_GetCharAttr( SHORT uiRow, SHORT uiCol, BYTE * pbyChar, BYTE * pbyAttr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetCharAttr(%hu, %hu, %p, %p)", uiRow, uiCol, pbyChar, pbyAttr));

   /* TODO */

   HB_SYMBOL_UNUSED( uiRow );
   HB_SYMBOL_UNUSED( uiCol );
   HB_SYMBOL_UNUSED( pbyChar );
   HB_SYMBOL_UNUSED( pbyAttr );
}

void hb_gt_GetChar( SHORT uiRow, SHORT uiCol, BYTE * pbyChar )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetChar(%hu, %hu, %p)", uiRow, uiCol, pbyChar));

   /* TODO */

   HB_SYMBOL_UNUSED( uiRow );
   HB_SYMBOL_UNUSED( uiCol );
   HB_SYMBOL_UNUSED( pbyChar );
}

void hb_gt_GetAttr( SHORT uiRow, SHORT uiCol, BYTE * pbyAttr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetAttr(%hu, %hu, %p)", uiRow, uiCol, pbyAttr));

   /* TODO */

   HB_SYMBOL_UNUSED( uiRow );
   HB_SYMBOL_UNUSED( uiCol );
   HB_SYMBOL_UNUSED( pbyAttr );
}

void hb_gt_Puts( USHORT usRow, USHORT usCol, BYTE attr, BYTE * str, ULONG len )
{
   /* Because Clipper strings don't have to be null terminated, add a null
      terminating character after saving what used to be at the termination
      position, because it might not even be part of the string object */
   char save = str[ len ];
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Puts(%hu, %hu, %d, %p, %lu, %s)", usRow, usCol, (int) attr, str, len, str));
   str[ len ] = '\0';
   /* Disable line wrap, set the new cursor position, send the string, then
      enable line wrap (for OUTSTD() and OUTERR() ) */
   hb_gt_AnsiSetAttributes( attr );
   fprintf( stdout, "\x1B[=7l\x1B[%d;%dH%s\x1B[=7h", usRow + 1, usCol + 1, str );
   /* Restore whatever used to be at the termination position */
   str[ len ] = save;
   /* Update the cursor position */
   s_usRow = usRow;
   s_usCol = usCol + ( USHORT )len;
   if( s_usCol > s_usMaxCol ) s_usCol = s_usMaxCol;
}

int hb_gt_RectSize( USHORT rows, USHORT cols )
{
   return rows * cols * 2;
}

void hb_gt_GetText( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE *dest )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetText(%hu, %hu, %hu, %hu, %p)", usTop, usLeft, usBottom, usRight, dest));
   HB_SYMBOL_UNUSED( usTop );
   HB_SYMBOL_UNUSED( usLeft );
   HB_SYMBOL_UNUSED( usBottom );
   HB_SYMBOL_UNUSED( usRight );
   HB_SYMBOL_UNUSED( dest );
   /* TODO: */
}

void hb_gt_PutText( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE *src )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_PutText(%hu, %hu, %hu, %hu, %p)", usTop, usLeft, usBottom, usRight, src) );
   HB_SYMBOL_UNUSED( usTop );
   HB_SYMBOL_UNUSED( usLeft );
   HB_SYMBOL_UNUSED( usBottom );
   HB_SYMBOL_UNUSED( usRight );
   HB_SYMBOL_UNUSED( src );
   /* TODO: */
}

void hb_gt_SetAttribute( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE attr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetAttribute(%hu, %hu, %hu, %hu, %d)", usTop, usLeft, usBottom, usRight, (int) attr) );
   HB_SYMBOL_UNUSED( usTop );
   HB_SYMBOL_UNUSED( usLeft );
   HB_SYMBOL_UNUSED( usBottom );
   HB_SYMBOL_UNUSED( usRight );
   HB_SYMBOL_UNUSED( attr );
   /* TODO: */
   s_iAttribute = attr;
}

void hb_gt_DispBegin( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_DispBegin()"));

   ++s_uiDispCount;

   /* TODO: */
}

void hb_gt_DispEnd( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_DispEnd()"));

   --s_uiDispCount;

   /* TODO: here we flush the buffer, and restore normal screen writes */
}

BOOL hb_gt_SetMode( USHORT uiRows, USHORT uiCols )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetMode(%hu, %hu)", uiRows, uiCols) );
   HB_SYMBOL_UNUSED( uiRows );
   HB_SYMBOL_UNUSED( uiCols );
   /* TODO: */
   return 0;   /* 0 = Ok, other = Fail */
}

BOOL hb_gt_GetBlink()
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetBlink()"));
   /* TODO: */
   return 1;               /* 0 = blink, 1 = intens      */
}

void hb_gt_SetBlink( BOOL bBlink )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetBlink(%d)", (int) bBlink) );
   HB_SYMBOL_UNUSED( bBlink );
   /* TODO: */
}

void hb_gt_Tone( double dFrequency, double dDuration )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Tone(%lf, %lf)", dFrequency, dDuration));

   /* TODO: Implement this */

   HB_SYMBOL_UNUSED( dFrequency );
   HB_SYMBOL_UNUSED( dDuration );
}

char * hb_gt_Version( void )
{
   return "Harbour Terminal: PC ANSI";
}

USHORT hb_gt_DispCount()
{
   return s_uiDispCount;
}


void hb_gt_Replicate( USHORT uiRow, USHORT uiCol, BYTE byAttr, BYTE byChar, ULONG nLength )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Replicate(%hu, %hu, %i, %i, %lu)", uiRow, uiCol, byAttr, byChar, nLength));

   {
      while( nLength-- )
         hb_gt_xPutch( uiRow, uiCol++, byAttr, byChar );
   }
}

USHORT hb_gt_Box( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right,
                  BYTE * szBox, BYTE byAttr )
{
   USHORT ret = 1;
   SHORT Row;
   SHORT Col;
   SHORT Height;
   SHORT Width;

   if( Left >= 0 || Left < hb_gt_GetScreenWidth()
   || Right >= 0 || Right < hb_gt_GetScreenWidth()
   || Top >= 0 || Top < hb_gt_GetScreenHeight()
   || Bottom >= 0 || Bottom < hb_gt_GetScreenHeight() )
   {

      /* Ensure that box is drawn from top left to bottom right. */
      if( Top > Bottom )
      {
         SHORT tmp = Top;
         Top = Bottom;
         Bottom = tmp;
      }
      if( Left > Right )
      {
         SHORT tmp = Left;
         Left = Right;
         Right = tmp;
      }

      /* Draw the box or line as specified */
      Height = Bottom - Top + 1;
      Width  = Right - Left + 1;

      hb_gt_DispBegin();

      if( Height > 1 && Width > 1 && Top >= 0 && Top < hb_gt_GetScreenHeight() && Left >= 0 && Left < hb_gt_GetScreenWidth() )
         hb_gt_xPutch( Top, Left, byAttr, szBox[ 0 ] ); /* Upper left corner */

      Col = ( Height > 1 ? Left + 1 : Left );
      if(Col < 0 )
      {
         Width += Col;
         Col = 0;
      }
      if( Right >= hb_gt_GetScreenWidth() )
      {
         Width -= Right - hb_gt_GetScreenWidth();
      }

      if( Col <= Right && Col < hb_gt_GetScreenWidth() && Top >= 0 && Top < hb_gt_GetScreenHeight() )
         hb_gt_Replicate( Top, Col, byAttr, szBox[ 1 ], Width + ( (Right - Left) > 1 ? -2 : 0 ) ); /* Top line */

      if( Height > 1 && (Right - Left) > 1 && Right < hb_gt_GetScreenWidth() && Top >= 0 && Top < hb_gt_GetScreenHeight() )
         hb_gt_xPutch( Top, Right, byAttr, szBox[ 2 ] ); /* Upper right corner */

      if( szBox[ 8 ] && Height > 2 && Width > 2 )
      {
         for( Row = Top + 1; Row < Bottom; Row++ )
         {
            if( Row >= 0 && Row < hb_gt_GetScreenHeight() )
            {
               Col = Left;
               if( Col < 0 )
                  Col = 0; /* The width was corrected earlier. */
               else
                  hb_gt_xPutch( Row, Col++, byAttr, szBox[ 7 ] ); /* Left side */
               hb_gt_Replicate( Row, Col, byAttr, szBox[ 8 ], Width - 2 ); /* Fill */
               if( Right < hb_gt_GetScreenWidth() )
                  hb_gt_xPutch( Row, Right, byAttr, szBox[ 3 ] ); /* Right side */
            }
         }
      }
      else
      {
         for( Row = ( Width > 1 ? Top + 1 : Top ); Row < ( (Right - Left ) > 1 ? Bottom : Bottom + 1 ); Row++ )
         {
            if( Row >= 0 && Row < hb_gt_GetScreenHeight() )
            {
               if( Left >= 0 && Left < hb_gt_GetScreenWidth() )
                  hb_gt_xPutch( Row, Left, byAttr, szBox[ 7 ] ); /* Left side */
               if( ( Width > 1 || Left < 0 ) && Right < hb_gt_GetScreenWidth() )
                  hb_gt_xPutch( Row, Right, byAttr, szBox[ 3 ] ); /* Right side */
            }
         }
      }

      if( Height > 1 && Width > 1 )
      {
         if( Left >= 0 && Bottom < hb_gt_GetScreenHeight() )
            hb_gt_xPutch( Bottom, Left, byAttr, szBox[ 6 ] ); /* Bottom left corner */

         Col = Left + 1;
         if( Col < 0 )
            Col = 0; /* The width was corrected earlier. */

         if( Col <= Right && Bottom < hb_gt_GetScreenHeight() )
            hb_gt_Replicate( Bottom, Col, byAttr, szBox[ 5 ], Width - 2 ); /* Bottom line */

         if( Right < hb_gt_GetScreenWidth() && Bottom < hb_gt_GetScreenHeight() )
            hb_gt_xPutch( Bottom, Right, byAttr, szBox[ 4 ] ); /* Bottom right corner */
      }
      hb_gt_DispEnd();
      ret = 0;
   }

   return ret;
}

USHORT hb_gt_BoxD( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right, BYTE * pbyFrame, BYTE byAttr )
{
   return hb_gt_Box( Top, Left, Bottom, Right, pbyFrame, byAttr );
}

USHORT hb_gt_BoxS( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right, BYTE * pbyFrame, BYTE byAttr )
{
   return hb_gt_Box( Top, Left, Bottom, Right, pbyFrame, byAttr );
}

USHORT hb_gt_HorizLine( SHORT Row, SHORT Left, SHORT Right, BYTE byChar, BYTE byAttr )
{
   USHORT ret = 1;
   if( Row >= 0 && Row < hb_gt_GetScreenHeight() )
   {
      if( Left < 0 )
         Left = 0;
      else if( Left >= hb_gt_GetScreenWidth() )
         Left = hb_gt_GetScreenWidth() - 1;
   
      if( Right < 0 )
         Right = 0;
      else if( Right >= hb_gt_GetScreenWidth() )
         Right = hb_gt_GetScreenWidth() - 1;

      if( Left < Right )
         hb_gt_Replicate( Row, Left, byAttr, byChar, Right - Left + 1 );
      else
         hb_gt_Replicate( Row, Right, byAttr, byChar, Left - Right + 1 );
      ret = 0;
   }
   return ret;
}

USHORT hb_gt_VertLine( SHORT Col, SHORT Top, SHORT Bottom, BYTE byChar, BYTE byAttr )
{
   USHORT ret = 1;
   SHORT Row;

   if( Col >= 0 && Col < hb_gt_GetScreenWidth() )
   {
      if( Top < 0 )
         Top = 0;
      else if( Top >= hb_gt_GetScreenHeight() )
         Top = hb_gt_GetScreenHeight() - 1;

      if( Bottom < 0 )
         Bottom = 0;
      else if( Bottom >= hb_gt_GetScreenHeight() )
         Bottom = hb_gt_GetScreenHeight() - 1;

      if( Top <= Bottom )
         Row = Top;
      else
      {
         Row = Bottom;
         Bottom = Top;
      }
      while( Row <= Bottom )
         hb_gt_xPutch( Row++, Col, byAttr, byChar );
      ret = 0;
   }
   return ret;
}

BOOL hb_gt_PreExt()
{
   return TRUE;
}

BOOL hb_gt_PostExt()
{
   return TRUE;
}

BOOL hb_gt_Suspend()
{
   return TRUE;
}

BOOL hb_gt_Resume()
{
   return TRUE;
}
