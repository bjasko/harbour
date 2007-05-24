/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    small and MT safe lexer for macro compiler
 *
 * Copyright 2006 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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

#define HB_MACRO_SUPPORT

#include "hbmacro.h"
#include "hbcomp.h"
#include "hbdate.h"
#include "macroy.h"

#define HB_LEX_ISFIRSTIDCHAR(c)     ( ( (c) >= 'A' && (c) <= 'Z' ) || \
                                      ( (c) >= 'a' && (c) <= 'z' ) || \
                                        (c) == '_' )
#define HB_LEX_ISDIGIT(c)           ( (c) >= '0' && (c) <= '9' )
#define HB_LEX_ISHEXDIGIT(c)        ( ( (c) >= '0' && (c) <= '9' ) || \
                                      ( (c) >= 'A' && (c) <= 'F' ) || \
                                      ( (c) >= 'a' && (c) <= 'f' ) )
#define HB_LEX_ISNEXTIDCHAR(c)      ( HB_LEX_ISFIRSTIDCHAR(c) || \
                                      HB_LEX_ISDIGIT(c) )

typedef struct _HB_MACRO_LEX
{
   char *   pString;
   char *   pDst;
   ULONG    ulLen;
   ULONG    ulSrc;
   BOOL     quote;
   char     pBuffer[ 2 ];
}
HB_MACRO_LEX, * PHB_MACRO_LEX;

BOOL hb_macroLexNew( HB_MACRO_PTR pMacro )
{
   if( pMacro->length )
   {
      /*
       * the total maximum size for parsed tokens delimited with ASCII NUL
       * cannot be bigger then the size of macro string because only
       * identifiers, strings, macrovars and macrotexts have to be returned
       * as string and all these tokens have to be separated by some non
       * value tokens or strings which will have not used delimiters
       */
      pMacro->pLex = hb_xgrab( sizeof( HB_MACRO_LEX ) + pMacro->length );
      ( ( PHB_MACRO_LEX ) pMacro->pLex )->pString = pMacro->string;
      ( ( PHB_MACRO_LEX ) pMacro->pLex )->ulLen   = pMacro->length;
      ( ( PHB_MACRO_LEX ) pMacro->pLex )->ulSrc = 0;
      ( ( PHB_MACRO_LEX ) pMacro->pLex )->quote = TRUE;
      ( ( PHB_MACRO_LEX ) pMacro->pLex )->pDst  =
                                 ( ( PHB_MACRO_LEX ) pMacro->pLex )->pBuffer;
      return TRUE;
   }

   return FALSE;
}

void hb_macroLexDelete( HB_MACRO_PTR pMacro )
{
   if( pMacro->pLex )
   {
      hb_xfree( pMacro->pLex );
      pMacro->pLex = NULL;
   }
}

static void hb_lexIdentCopy( PHB_MACRO_LEX pLex )
{
   while( pLex->ulSrc < pLex->ulLen )
   {
      char ch = pLex->pString[ pLex->ulSrc ];
      if( ch >= 'a' && ch <= 'z' )
         *pLex->pDst++ = ch - ( 'a' - 'A' );
      else if( ( ch >= 'A' && ch <= 'Z' ) ||
               ( ch >= '0' && ch <= '9' ) || ch == '_' )
         *pLex->pDst++ = ch;
      else
         break;
      pLex->ulSrc++;
   }
}

static int hb_lexStringCopy( YYSTYPE *yylval_ptr, HB_MACRO_PTR pMacro,
                             PHB_MACRO_LEX pLex, char cDelim )
{
   pLex->quote = FALSE;
   yylval_ptr->valChar.string = pLex->pDst;
   while( pLex->ulSrc < pLex->ulLen )
   {
      char ch = pLex->pString[ pLex->ulSrc++ ];
      if( ch == cDelim )
      {
         yylval_ptr->valChar.length = pLex->pDst - yylval_ptr->valChar.string;
         *pLex->pDst++ = '\0';
         return LITERAL;
      }
      *pLex->pDst++ = ch;
   }
   yylval_ptr->valChar.length = pLex->pDst - yylval_ptr->valChar.string;
   *pLex->pDst++ = '\0';
   hb_macroError( EG_SYNTAX, pMacro );
   return LITERAL;
}

static int hb_lexStringExtCopy( YYSTYPE *yylval_ptr, HB_MACRO_PTR pMacro,
                                PHB_MACRO_LEX pLex )
{
   ULONG ulLen;
   pLex->quote = FALSE;
   yylval_ptr->valChar.string = pLex->pDst;
   while( pLex->ulSrc < pLex->ulLen )
   {
      char ch = pLex->pString[ pLex->ulSrc++ ];
      if( ch == '\\' )
      {
         if( pLex->ulSrc < pLex->ulLen )
         {
            *pLex->pDst++ = ch;
            ch = pLex->pString[ pLex->ulSrc++ ];
         }
      }
      else if( ch == '"' )
      {
         ulLen = pLex->pDst - yylval_ptr->valChar.string;
         *pLex->pDst++ = '\0';
         hb_strRemEscSeq( yylval_ptr->valChar.string, &ulLen );
         yylval_ptr->valChar.length = ( int ) ulLen;
         return LITERAL;
      }
      *pLex->pDst++ = ch;
   }
   ulLen = pLex->pDst - yylval_ptr->valChar.string;
   *pLex->pDst++ = '\0';
   hb_strRemEscSeq( yylval_ptr->valChar.string, &ulLen );
   yylval_ptr->valChar.length = ( int ) ulLen;
   hb_macroError( EG_SYNTAX, pMacro );
   return LITERAL;
}

static int hb_lexNumConv( YYSTYPE *yylval_ptr, PHB_MACRO_LEX pLex, ULONG ulLen )
{
   HB_LONG lNumber;
   double dNumber;
   int iDec, iWidth;

   if( hb_compStrToNum( pLex->pString + pLex->ulSrc, ulLen,
                        &lNumber, &dNumber, &iDec, &iWidth ) )
   {
      yylval_ptr->valDouble.dNumber = dNumber;
      yylval_ptr->valDouble.bDec = iDec;
      yylval_ptr->valDouble.bWidth = iWidth;
      pLex->ulSrc += ulLen;
      return NUM_DOUBLE;
   }
   else
   {
      yylval_ptr->valLong.lNumber = lNumber;
      yylval_ptr->valLong.bWidth = iWidth;
      pLex->ulSrc += ulLen;
      return NUM_LONG;
   }
}

int hb_macrolex( YYSTYPE *yylval_ptr, HB_MACRO_PTR pMacro )
{
   PHB_MACRO_LEX pLex = ( PHB_MACRO_LEX ) pMacro->pLex;

   while( pLex->ulSrc < pLex->ulLen )
   {
      char ch = pLex->pString[ pLex->ulSrc++ ];
      switch( ch )
      {
         case ' ':
         case '\t':
         case '\r':
            break;

         case '$':
         case ',':
         case '|':
         case '@':
         case '(':
         case '{':
         case ';':
         case '\n':
            pLex->quote = TRUE;
            return ch;

         case ')':
         case '}':
         case ']':
            pLex->quote = FALSE;
            return ch;

         case '#':
            pLex->quote = TRUE;
            return NE1;

         case '!':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return NE2;
            }
            return NOT;

         case '<':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '>' )
            {
               pLex->ulSrc++;
               return NE2;
            }
            else if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return LE;
            }
            return '<';

         case '>':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return GE;
            }
            return '>';

         case '=':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return EQ;
            }
            else if( pLex->pString[ pLex->ulSrc ] == '>' && HB_SUPPORT_HARBOUR )
            {
               pLex->ulSrc++;
               return HASHOP;
            }
            return '=';

         case '+':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '+' )
            {
               pLex->ulSrc++;
               return INC;
            }
            else if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return PLUSEQ;
            }
            return '+';

         case '-':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '-' )
            {
               pLex->ulSrc++;
               return DEC;
            }
            else if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return MINUSEQ;
            }
            else if( pLex->pString[ pLex->ulSrc ] == '>' )
            {
               pLex->ulSrc++;
               return ALIASOP;
            }
            return '-';

         case '*':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '*' )
            {
               pLex->ulSrc++;
               if( pLex->pString[ pLex->ulSrc ] == '=' )
               {
                  pLex->ulSrc++;
                  return EXPEQ;
               }
               return POWER;
            }
            else if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return MULTEQ;
            }
            return '*';

         case '/':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return DIVEQ;
            }
            return '/';

         case '%':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return MODEQ;
            }
            return '%';

         case '^':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return EXPEQ;
            }
            return POWER;

         case ':':
            pLex->quote = TRUE;
            if( pLex->pString[ pLex->ulSrc ] == '=' )
            {
               pLex->ulSrc++;
               return INASSIGN;
            }
            else if( pLex->pString[ pLex->ulSrc ] == ':' )
            {
               yylval_ptr->string = "SELF";
               return IDENTIFIER;
            }
            return ':';

         case '.':
            pLex->quote = TRUE;
            if( pLex->ulSrc < pLex->ulLen &&
                HB_LEX_ISDIGIT( pLex->pString[ pLex->ulSrc ] ) )
            {
               ULONG ul = pLex->ulSrc;
               while( ++ul < pLex->ulLen &&
                      HB_LEX_ISDIGIT( pLex->pString[ ul ] ) );
               ul -= --pLex->ulSrc;
               return hb_lexNumConv( yylval_ptr, pLex, ul );
            }
            if( pLex->ulLen - pLex->ulSrc >= 4 &&
                pLex->pString[ pLex->ulSrc + 3 ] == '.' )
            {
               if( ( pLex->pString[ pLex->ulSrc + 0 ] | ('a' - 'A') ) == 'a' &&
                   ( pLex->pString[ pLex->ulSrc + 1 ] | ('a' - 'A') ) == 'n' &&
                   ( pLex->pString[ pLex->ulSrc + 2 ] | ('a' - 'A') ) == 'd' )
               {
                  pLex->ulSrc += 4;
                  return AND;
               }
               if( ( pLex->pString[ pLex->ulSrc + 0 ] | ('a' - 'A') ) == 'n' &&
                   ( pLex->pString[ pLex->ulSrc + 1 ] | ('a' - 'A') ) == 'o' &&
                   ( pLex->pString[ pLex->ulSrc + 2 ] | ('a' - 'A') ) == 't' )
               {
                  pLex->ulSrc += 4;
                  return NOT;
               }
            }
            if( pLex->ulLen - pLex->ulSrc >= 3 &&
                pLex->pString[ pLex->ulSrc + 2 ] == '.' )
            {
               if( ( pLex->pString[ pLex->ulSrc + 0 ] | ('a' - 'A') ) == 'o' &&
                   ( pLex->pString[ pLex->ulSrc + 1 ] | ('a' - 'A') ) == 'r' )
               {
                  pLex->ulSrc += 3;
                  return OR;
               }
            }
            if( pLex->ulLen - pLex->ulSrc >= 2 &&
                pLex->pString[ pLex->ulSrc + 1 ] == '.' )
            {
               if( ( pLex->pString[ pLex->ulSrc ] | ('a' - 'A') ) == 't' ||
                   ( pLex->pString[ pLex->ulSrc ] | ('a' - 'A') ) == 'y' )
               {
                  pLex->quote = FALSE;
                  pLex->ulSrc += 2;
                  return TRUEVALUE;
               }
               if( ( pLex->pString[ pLex->ulSrc ] | ('a' - 'A') ) == 'f' ||
                   ( pLex->pString[ pLex->ulSrc ] | ('a' - 'A') ) == 'n' )
               {
                  pLex->quote = FALSE;
                  pLex->ulSrc += 2;
                  return FALSEVALUE;
               }
               if( pLex->pString[ pLex->ulSrc ] == '.' )
               {
                  pLex->ulSrc += 2;
                  return EPSILON;
               }
            }
            return '.';

         case '[':
            if( pLex->quote )
               return hb_lexStringCopy( yylval_ptr, pMacro, pLex, ']' );
            pLex->quote = TRUE;
            return '[';

         case '`':
         case '\'':
            return hb_lexStringCopy( yylval_ptr, pMacro, pLex, '\'' );

         case '"':
            return hb_lexStringCopy( yylval_ptr, pMacro, pLex, '"' );

         case '&':
            if( pLex->ulSrc < pLex->ulLen )
            {
               if( HB_LEX_ISFIRSTIDCHAR( pLex->pString[ pLex->ulSrc ] ) )
               {
                  /* [&<keyword>[.[<nextidchars>]]]+ */
                  int iParts = 0;
                  pLex->quote = FALSE;
                  yylval_ptr->string = pLex->pDst;
                  pLex->ulSrc--;
                  do
                  {
                     ++iParts;
                     *pLex->pDst++ = '&';
                     pLex->ulSrc++;
                     hb_lexIdentCopy( pLex );
                     if( pLex->pString[ pLex->ulSrc ] == '.' )
                     {
                        ++iParts;
                        *pLex->pDst++ = '.';
                        pLex->ulSrc++;
                        hb_lexIdentCopy( pLex );
                     }
                  }
                  while( pLex->ulLen - pLex->ulSrc > 1 &&
                         pLex->pString[ pLex->ulSrc ] == '&' &&
                         HB_LEX_ISFIRSTIDCHAR( pLex->pString[ pLex->ulSrc + 1 ] ) );
                  if( iParts == 2 && *( pLex->pDst - 1 ) == '.' )
                  {
                     pLex->pDst--;
                     iParts = 1;
                  }
                  *pLex->pDst++ = '\0';
                  if( iParts == 1 )
                  {
                     yylval_ptr->string++;
                     if( pLex->pDst - yylval_ptr->string > HB_SYMBOL_NAME_LEN + 1 )
                        yylval_ptr->string[ HB_SYMBOL_NAME_LEN ] = '\0';
                     return MACROVAR;
                  }
                  return MACROTEXT;
               }
               else if( pLex->pString[ pLex->ulSrc ] == '\'' ||
                        pLex->pString[ pLex->ulSrc ] == '"' ||
                        pLex->pString[ pLex->ulSrc ] == '[' )
                  hb_macroError( EG_SYNTAX, pMacro );
            }
            pLex->quote = TRUE;
            return '&';

         default:
            if( HB_LEX_ISDIGIT( ch ) )
            {
               ULONG ul = pLex->ulSrc;

               pLex->quote = FALSE;
               if( ch == '0' && ul < pLex->ulLen )
               {
                  if( pLex->pString[ ul ] == 'd' || pLex->pString[ ul ] == 'D' )
                  {
                     while( ++ul < pLex->ulLen &&
                            HB_LEX_ISDIGIT( pLex->pString[ ul ] ) );
                     if( ul - pLex->ulSrc == 9 )
                     {
                        int year, month, day;
                        hb_dateStrGet( pLex->pString + pLex->ulSrc + 1,
                                       &year, &month, &day );
                        yylval_ptr->valLong.lNumber =
                                       hb_dateEncode( year, month, day );
                        pLex->ulSrc = ul;
                        return NUM_DATE;
                     }
                     ul = pLex->ulSrc;
                  }
                  else if( pLex->pString[ ul ] == 'x' ||
                           pLex->pString[ ul ] == 'X' )
                  {
                     while( ++ul < pLex->ulLen &&
                            HB_LEX_ISHEXDIGIT( pLex->pString[ ul ] ) );
                     if( ul == pLex->ulSrc + 1 )
                        --ul;
                  }
                  else
                  {
                     while( ul < pLex->ulLen &&
                            HB_LEX_ISDIGIT( pLex->pString[ ul ] ) )
                        ++ul;
                     if( pLex->ulLen - ul > 1 && pLex->pString[ ul ] == '.' &&
                         HB_LEX_ISDIGIT( pLex->pString[ ul + 1 ] ) )
                     {
                        while( ++ul < pLex->ulLen &&
                               HB_LEX_ISDIGIT( pLex->pString[ ul ] ) );
                     }
                  }
               }
               else
               {
                  while( ul < pLex->ulLen &&
                         HB_LEX_ISDIGIT( pLex->pString[ ul ] ) )
                     ++ul;
                  if( pLex->ulLen - ul > 1 && pLex->pString[ ul ] == '.' &&
                      HB_LEX_ISDIGIT( pLex->pString[ ul + 1 ] ) )
                  {
                     while( ++ul < pLex->ulLen &&
                            HB_LEX_ISDIGIT( pLex->pString[ ul ] ) );
                  }
               }
               ul -= --pLex->ulSrc;
               return hb_lexNumConv( yylval_ptr, pLex, ul );
            }
            else if( HB_LEX_ISFIRSTIDCHAR( ch ) )
            {
               ULONG ulLen;
               pLex->quote = FALSE;
               yylval_ptr->string = pLex->pDst;
               *pLex->pDst++ = ch - ( ( ch >= 'a' && ch <= 'z' ) ? 'a' - 'A' : 0 );
               hb_lexIdentCopy( pLex );
               if( pLex->ulLen - pLex->ulSrc > 1 &&
                   pLex->pString[ pLex->ulSrc ] == '&' &&
                   HB_LEX_ISFIRSTIDCHAR( pLex->pString[ pLex->ulSrc + 1 ] ) )
               {
                  /* [<keyword>][&<keyword>[.[<nextidchars>]]]+ */
                  do
                  {
                     *pLex->pDst++ = '&';
                     pLex->ulSrc++;
                     hb_lexIdentCopy( pLex );
                     if( pLex->pString[ pLex->ulSrc ] == '.' )
                     {
                        *pLex->pDst++ = '.';
                        pLex->ulSrc++;
                        hb_lexIdentCopy( pLex );
                     }
                  }
                  while( pLex->ulLen - pLex->ulSrc > 1 &&
                         pLex->pString[ pLex->ulSrc ] == '&' &&
                         HB_LEX_ISFIRSTIDCHAR( pLex->pString[ pLex->ulSrc + 1 ] ) );
                  *pLex->pDst++ = '\0';
                  return MACROTEXT;
               }
               ulLen = pLex->pDst - yylval_ptr->string;
               *pLex->pDst++ = '\0';
               if( ulLen == 1 )
               {
                  if( yylval_ptr->string[ 0 ] == 'E' &&
                      pLex->ulLen > pLex->ulSrc &&
                      pLex->pString[ pLex->ulSrc ] == '"' )
                  {
                     pLex->ulSrc++;
                     return hb_lexStringExtCopy( yylval_ptr, pMacro, pLex );
                  }
               }
               else if( ulLen == 2 )
               {
                  if( yylval_ptr->string[ 0 ] == 'I' &&
                      yylval_ptr->string[ 1 ] == 'F' )
                     return IIF;
               }
               else if( ulLen == 3 )
               {
                  if( yylval_ptr->string[ 0 ] == 'I' &&
                      yylval_ptr->string[ 1 ] == 'I' &&
                      yylval_ptr->string[ 2 ] == 'F' )
                     return IIF;
                  else if( yylval_ptr->string[ 0 ] == 'N' &&
                           yylval_ptr->string[ 1 ] == 'I' &&
                           yylval_ptr->string[ 2 ] == 'L' )
                     return NIL;
               }
               else /* ulLen >= 4 */
               {
                  switch( yylval_ptr->string[ 0 ] )
                  {
                     case '_':
                        if( ulLen <= 6 && memcmp( "FIELD", yylval_ptr->string + 1,
                                                  ulLen - 1 ) == 0 )
                           return FIELD;
                        break;
                     case 'F':
                        if( ulLen <= 5 && memcmp( "IELD", yylval_ptr->string + 1,
                                                  ulLen - 1 ) == 0 )
                           return FIELD;
                        break;
                     case 'Q':
                        if( ulLen == 5 && memcmp( "SELF", yylval_ptr->string + 1,
                                                  ulLen - 1 ) == 0 )
                        {
                           while( pLex->ulSrc < pLex->ulLen &&
                                  ( pLex->pString[ pLex->ulSrc ] == ' ' ||
                                    pLex->pString[ pLex->ulSrc ] == '\t' ) )
                              pLex->ulSrc++;
                           if( pLex->ulSrc < pLex->ulLen &&
                               pLex->pString[ pLex->ulSrc ] == '(' )
                           {
                              ULONG ul = pLex->ulSrc;
                              while( ++ul < pLex->ulLen )
                              {
                                 if( pLex->pString[ ul ] == ')' )
                                 {
                                    pLex->ulSrc = ul + 1;
                                    return SELF;
                                 }
                                 else if( pLex->pString[ ul ] != ' ' &&
                                          pLex->pString[ ul ] != '\t' )
                                    break;
                              }
                           }
                        }
                        break;
                  }
                  if( pLex->pDst - yylval_ptr->string > HB_SYMBOL_NAME_LEN + 1 )
                     yylval_ptr->string[ HB_SYMBOL_NAME_LEN ] = '\0';
               }
               return IDENTIFIER;
            }
            return ch;
      }
   }

   return 0;
}
