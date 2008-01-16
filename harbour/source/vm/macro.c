/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Macro compiler main file
 *
 * Copyright 1999 Ryszard Glab <rglab@imid.med.pl>
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

/* this #define HAVE TO be placed before all #include directives
 */
#define HB_MACRO_SUPPORT

#include "hbvmopt.h"
#include "hbmacro.h"
#include "hbcomp.h"
#include "hbstack.h"

#ifdef HB_MACRO_STATEMENTS
  #include "hbpp.h"
#endif

/* various flags for macro compiler */
#ifdef HB_C52_STRICT
static ULONG s_macroFlags = HB_SM_SHORTCUTS;
#else
static ULONG s_macroFlags = HB_SM_SHORTCUTS | HB_SM_HARBOUR;
#endif

/* ************************************************************************* */

/* Compile passed string into a pcode buffer
 *
 * 'pMacro' - pointer to HB_MACRO structure that will hold all information
 *    nedded for macro compilation and evaluation
 * 'szString'  - a string to compile
 * 'iFlag' - specifies if compiled code should generate pcodes either for push
 *    operation (for example: var :=&macro) or for pop operation (&macro :=var)
 */
static int hb_macroParse( HB_MACRO_PTR pMacro )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroParse(%p)", pMacro));

   /* initialize the output (pcode) buffer - it will be filled by yacc */
   pMacro->pCodeInfo = (HB_PCODE_INFO_PTR ) hb_xgrab( sizeof( HB_PCODE_INFO ) );
   pMacro->pCodeInfo->lPCodeSize = HB_PCODE_SIZE;
   pMacro->pCodeInfo->lPCodePos  = 0;
   pMacro->pCodeInfo->fVParams   = FALSE;
   pMacro->pCodeInfo->pLocals    = NULL;
   pMacro->pCodeInfo->pPrev      = NULL;
   pMacro->pCodeInfo->pCode      = ( BYTE * ) hb_xgrab( HB_PCODE_SIZE );

   /* reset the type of compiled expression - this should be filled after
    * successfully compilation
    */
   pMacro->pError = NULL;
   pMacro->uiListElements = 0;
   pMacro->exprType = HB_ET_NONE;

   return hb_macroYYParse( pMacro );
}

/* releases all memory allocated for macro evaluation
 * NOTE:
 *    Only members of HB_MACRO structure are deallocated
 *    the 'pMacro' pointer is not released - it can be a pointer
 *    to a memory allocated on the stack.
 */
void hb_macroDelete( HB_MACRO_PTR pMacro )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroDelete(%p)", pMacro));

   hb_xfree( (void *) pMacro->pCodeInfo->pCode );
   hb_xfree( (void *) pMacro->pCodeInfo );
   if( pMacro->pError )
      hb_errRelease( pMacro->pError );
   if( pMacro->Flags & HB_MACRO_DEALLOCATE )
      hb_xfree( pMacro );
}

/* checks if a correct ITEM was passed from the virtual machine eval stack
 */
static BOOL hb_macroCheckParam( HB_ITEM_PTR pItem )
{
   BOOL bValid = TRUE;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroCheckParam(%p)", pItem));

   if( ! HB_IS_STRING(pItem) )
   {
      HB_ITEM_PTR pResult = hb_errRT_BASE_Subst( EG_ARG, 1065, NULL, "&", 0 );

      bValid = FALSE;
      if( pResult )
      {
         hb_stackPop();
         hb_vmPush( pResult );
         hb_itemRelease( pResult );
         bValid = TRUE;
      }
   }
   return bValid;
}

/* It handles an error generated during checking of expression type
 */
static HB_ERROR_HANDLE( hb_macroErrorType )
{
   HB_MACRO_PTR pMacro = ( HB_MACRO_PTR ) ErrorInfo->Cargo;

   /* copy error object for later diagnostic usage */
   if( !pMacro->pError )
      pMacro->pError = hb_itemNew( ErrorInfo->Error );

   pMacro->status &= ~HB_MACRO_CONT;

   /* ignore rest of compiled code */
   hb_vmRequestEndProc();

   return NULL;   /* ignore this error */
}


/* Executes pcode compiled by macro compiler
 *
 * pMacro is a pointer to HB_MACRO structure created by macro compiler
 *
 */
void hb_macroRun( HB_MACRO_PTR pMacro )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroRun(%p)", pMacro));

   hb_vmExecute( pMacro->pCodeInfo->pCode, NULL );
}

static void hb_macroSyntaxError( HB_MACRO_PTR pMacro )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroSyntaxError(%p)", pMacro));

   if( pMacro && pMacro->pError )
   {
      HB_TRACE(HB_TR_DEBUG, ("hb_macroSyntaxError.(%s)", pMacro->string));

      hb_errLaunch( pMacro->pError );
      hb_errRelease( pMacro->pError );
      pMacro->pError = NULL;
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_SYNTAX, 1449, NULL, "&", 0 );

      if( pResult )
      {
         hb_vmPush( pResult );
         hb_itemRelease( pResult );
      }
   }
}

/* This replaces all '&var' or '&var.' occurences within a given string
 * with the value of variable 'var' if this variable exists and contains
 * a string value. The value of variable is also searched for
 * occurences of macro operator and if it is found then it is expanded
 * until there is no more macro operators.
 * NOTE:
 *    this does not evaluate a macro expression - there is a simple text
 *    substitution only
 * NOTE:
 *    hb_macroTextSubst returns either a pointer that points to the passed
 *    string if there was no macro operator in it or a pointer to a new
 *    allocated memory with expanded string if there was a macro operator
 *    in passed string.
 * NOTE:
 *    Clipper restarts scanning of the text from the beginning of
 *    inserted text after macro expansion, for example:
 *    PRIVATE a:='&', b:='c'
 *    PRIVATE &a.b   // this will create 'c' variable
 *
 *    PRIVATE a:=0, b:='b', ab:='c'
 *    PRIVATE &a&b   //this will cause syntax error '&'
 *
 */
static char * hb_macroTextSubst( char * szString, ULONG *pulStringLen )
{
   char * szResult;
   ULONG ulResStrLen;
   ULONG ulResBufLen;
   ULONG ulCharsLeft;
   char * pHead;
   char * pTail;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroTextSubst(%s, %li)", szString, *pulStringLen));

   pHead = (char *) memchr( (void *) szString, '&', *pulStringLen );
   if( pHead == NULL )
      return szString;  /* no more processing is required */

   /* initial length of the string and the result buffer (it can contain null bytes) */
   ulResBufLen = ulResStrLen = *pulStringLen;
   /* initial buffer for return value */
   szResult = (char *) hb_xgrab( ulResBufLen + 1 );

   /* copy the input string with trailing zero byte
    */
   memcpy( szResult, szString, ulResStrLen + 1 );
   /* switch the pointer so it will point into the result buffer
    */
   pHead = szResult + ( pHead - szString );

   do
   {
      /* store the position where '&' was found so we can restart scanning
       * from this point after macro expansion
       */
      pTail = pHead;
      /* check if the next character can start a valid identifier
      * (only _a-zA-Z are allowed)
      */
      ++pHead;    /* skip '&' character */
      if( *pHead == '_' ||
          ( *pHead >= 'A' && *pHead <= 'Z' ) ||
          ( *pHead >= 'a' && *pHead <= 'z' ) )
      {
         /* extract a variable name */
         /* NOTE: the extracted name can be longer then supported maximal
          * length of identifiers (HB_SYMBOL_NAME_LEN) - only the max allowed
          * are used for name lookup however the whole string is replaced
          */
         ULONG ulNameLen = 1;
         char * pName = pHead;

         while( *++pHead && ( *pHead == '_' ||
                              ( *pHead >= 'A' && *pHead <= 'Z' ) ||
                              ( *pHead >= 'a' && *pHead <= 'z' ) ||
                              ( *pHead >= '0' && *pHead <= '9' ) ) )
         {
            ++ulNameLen;
         }
         /* pHead points now at the character that terminated a variable name */

         /* NOTE: '_' is invalid variable name
          */
         if( ulNameLen > 1 || *pName != '_' )
         {
            /* this is not the "&_" string */
            char * szValPtr;
            ULONG ulValLen;

            /* Get a pointer to the string value stored in this variable
             * or NULL if variable doesn't exist or doesn't contain a string
             * value.
             * NOTE: This doesn't create a copy of the value then it
             * shouldn't be released here.
             */
            ulValLen = ulNameLen;   /* the length of name */
            szValPtr = hb_memvarGetStrValuePtr( pName, &ulValLen );
            if( szValPtr )
            {
               if( *pHead == '.' )
               {
                  /* we have stopped at the macro terminator '.' - skip it */
                  ++pHead;
                  ++ulNameLen;
               }
               ++ulNameLen;   /* count also the '&' character */

               /* number of characters left on the right side of a variable name */
               ulCharsLeft = ulResStrLen - ( pHead - szResult );

               /* NOTE:
                * if a replacement string is shorter then the variable
                * name then we don't have to reallocate the result buffer:
                * 'ulResStrLen' stores the current length of a string in the buffer
                * 'ulResBufLen' stores the length of the buffer
                */
               if( ulValLen > ulNameLen )
               {
                  ulResStrLen += ( ulValLen - ulNameLen );
                  if( ulResStrLen > ulResBufLen )
                  {
                     ULONG ulHead = pHead - szResult;
                     ULONG ulTail = pTail - szResult;
                     ulResBufLen = ulResStrLen;
                     szResult = ( char * ) hb_xrealloc( szResult, ulResBufLen + 1 );
                     pHead = szResult + ulHead;
                     pTail = szResult + ulTail;
                  }
               }
               else
                  ulResStrLen -= ( ulNameLen - ulValLen );

               /* move bytes located on the right side of a variable name */
               memmove( pTail + ulValLen, pHead, ulCharsLeft + 1 );
               /* copy substituted value */
               memcpy( pTail, szValPtr, ulValLen );
               /* restart scanning from the beginning of replaced string */
               /* NOTE: This causes that the following code:
                *    a := '&a'
                *    var := '&a.b'
                * is the same as:
                *    var := '&ab'
                */
               pHead = pTail;
            }
         }
      }
      ulCharsLeft = ulResStrLen - ( pHead - szResult );
   }
   while( ulCharsLeft && ( pHead = (char *) memchr( (void *)pHead, '&', ulCharsLeft ) ) != NULL );

   if( ulResStrLen < ulResBufLen )
   {
      /* result string is shorter then allocated buffer -
       * cut it to a required length
       */
      szResult = ( char * ) hb_xrealloc( szResult, ulResStrLen + 1 );
   }
   szResult[ ulResStrLen ] = 0;  /* place terminating null character */
   /* return a length of result string */
   *pulStringLen = ulResStrLen;

   return szResult;   /* a new memory buffer was allocated */
}


/* NOTE:
 *   This will be called when macro variable or macro expression is
 * placed on the right side of the assignment or when it is used as
 * a parameter.
 * PUSH operation
 * iContext contains additional info when HB_SM_XBASE is enabled
 *  = 0 - in Clipper strict compatibility mode
 *  = HB_P_MACROPUSHLIST
 *  = HB_P_MACROPUSHPARE
 *
 * iContext contains HB_P_MACROPUSHPARE if a macro is used inside a codeblock
 * EVAL( {|| &macro} )
 *
 */

void hb_macroGetValue( HB_ITEM_PTR pItem, BYTE iContext, BYTE flags )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroGetValue(%p)", pItem));

   if( hb_macroCheckParam( pItem ) )
   {
      HB_MACRO struMacro;
      int iStatus;
      BOOL fFree;
#ifdef HB_MACRO_STATEMENTS
      char * pText;
      char * pOut;
#endif
      struMacro.mode       = HB_MODE_MACRO;
      struMacro.supported  = (flags & HB_SM_RT_MACRO) ? s_macroFlags : flags;
      struMacro.Flags      = HB_MACRO_GEN_PUSH;
      struMacro.uiNameLen  = HB_SYMBOL_NAME_LEN;
      struMacro.status     = HB_MACRO_CONT;
      struMacro.length     = pItem->item.asString.length;
      /*
       * Clipper appears to expand nested macros staticly vs. by
       * Macro Parser, f.e.:
       *       PROCEDURE Main()
       *          LOCAL cText
       *          cText := "( v := 'A' ) + &v"
       *          M->v := "'B'"
       *          ? "Macro:", cText
       *          ? "Result:", &cText
       *          ? "Type:", type(cText)
       *       RETURN
       */
      struMacro.string = hb_macroTextSubst( pItem->item.asString.value, &struMacro.length );
      fFree = struMacro.string != pItem->item.asString.value;

      if( iContext != 0 )
      {
         /*
          * If compiled in xbase compatibility mode:
          * macro := "1,2"
          * funCall( &macro )  ==>  funCall( 1, 2 )
          * { &macro }  ==>  { 1, 2 }
          * var[ &macro ]  ==>  var[ 1, 2 ]
          * var := (somevalue, &macro)  ==> var := 2
          *
          * Always:
          * macro := "1,2"
          * EVAL( {|| &macro} )
          *
          */
         struMacro.Flags |= HB_MACRO_GEN_LIST;
         if( iContext == HB_P_MACROPUSHPARE )
         {
            struMacro.Flags |= HB_MACRO_GEN_PARE;
         }
      }

#ifdef HB_MACRO_STATEMENTS
      if( struMacro.supported & HB_SM_PREPROC )
      {
         char * ptr;
         int slen;

         pText = ( char * ) hb_xgrab( HB_PP_STR_SIZE );
         pOut = ( char * ) hb_xgrab( HB_PP_STR_SIZE );
         ptr = pText;
         slen = HB_MIN( strlen( struMacro.string ), HB_PP_STR_SIZE - 1 );
         memcpy( pText, struMacro.string, slen );
         pText[ slen ] = 0;
         memset( pOut, 0, HB_PP_STR_SIZE );

         HB_SKIPTABSPACES( ptr );

         if( !hb_pp_topDefine )
         {
            hb_pp_Table();
         }

         hb_pp_ParseExpression( ptr, pOut );
         struMacro.string = pText;
         struMacro.length = strlen( pText );
      }
#endif

      iStatus = hb_macroParse( &struMacro );
#ifdef HB_MACRO_STATEMENTS
      if( struMacro.supported & HB_SM_PREPROC )
      {
         hb_xfree( pText );
         hb_xfree( pOut );
      }
#endif
      hb_stackPop();    /* remove compiled string */
      if( iStatus == HB_MACRO_OK && ( struMacro.status & HB_MACRO_CONT ) )
      {
         hb_macroRun( &struMacro );

         if( iContext == HB_P_MACROPUSHLIST )
            hb_vmPushLong( struMacro.uiListElements + 1 );
      }
      else
         hb_macroSyntaxError( &struMacro );

      if( fFree )
         hb_xfree( struMacro.string );

      hb_macroDelete( &struMacro );
   }
}

/* NOTE:
 *   This will be called when macro variable or macro expression is
 * placed on the left side of the assignment
 * POP operation
 */
void hb_macroSetValue( HB_ITEM_PTR pItem, BYTE flags )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroSetValue(%p)", pItem));

   if( hb_macroCheckParam( pItem ) )
   {
      HB_MACRO struMacro;
      int iStatus;

      struMacro.mode       = HB_MODE_MACRO;
      struMacro.supported  = (flags & HB_SM_RT_MACRO) ? s_macroFlags : flags;
      struMacro.Flags      = HB_MACRO_GEN_POP;
      struMacro.uiNameLen  = HB_SYMBOL_NAME_LEN;
      struMacro.status     = HB_MACRO_CONT;
      struMacro.string     = pItem->item.asString.value;
      struMacro.length     = pItem->item.asString.length;

      iStatus = hb_macroParse( &struMacro );

      hb_stackPop();    /* remove compiled string */
      if( iStatus == HB_MACRO_OK && ( struMacro.status & HB_MACRO_CONT ) )
         hb_macroRun( &struMacro );
      else
         hb_macroSyntaxError( &struMacro );

      hb_macroDelete( &struMacro );
   }
}

/*
 * Compile and run:
 *    &alias->var or
 *    alias->&var
 * NOTE:
 *    Clipper implements these two cases as: &( alias +'->' + variable )
 *    This causes some non expected behaviours, for example:
 *    A :="M + M"
 *    ? &A->&A
 *    is the same as:
 *    &( "M + M->M + M" )
 *    instead of
 *    &( "M + M" ) -> &( "M + M" )
 */
static void hb_macroUseAliased( HB_ITEM_PTR pAlias, HB_ITEM_PTR pVar, int iFlag, BYTE bSupported )
{
   if( HB_IS_STRING( pAlias ) && HB_IS_STRING( pVar ) )
   {
      /* grab memory for "alias->var"
      */
      ULONG ulLen = pAlias->item.asString.length + pVar->item.asString.length + 2;
      char * szString = ( char * ) hb_xgrab( ulLen + 1 );
      HB_MACRO struMacro;
      int iStatus;

      memcpy( szString, pAlias->item.asString.value, pAlias->item.asString.length );
      szString[ pAlias->item.asString.length ]     = '-';
      szString[ pAlias->item.asString.length + 1 ] = '>';
      memcpy( szString + pAlias->item.asString.length + 2, pVar->item.asString.value, pVar->item.asString.length );
      szString[ ulLen ] = '\0';

      struMacro.mode       = HB_MODE_MACRO;
      struMacro.supported  = (bSupported & HB_SM_RT_MACRO) ? s_macroFlags : bSupported;
      struMacro.Flags      = iFlag;
      struMacro.uiNameLen  = HB_SYMBOL_NAME_LEN;
      struMacro.status     = HB_MACRO_CONT;
      struMacro.string     = szString;
      struMacro.length     = ulLen;
      iStatus = hb_macroParse( &struMacro );
      hb_xfree( szString );
      struMacro.string = NULL;

      hb_stackPop();    /* remove compiled variable name */
      hb_stackPop();    /* remove compiled alias */

      if( iStatus == HB_MACRO_OK && ( struMacro.status & HB_MACRO_CONT ) )
         hb_macroRun( &struMacro );
      else
         hb_macroSyntaxError( &struMacro );

      hb_macroDelete( &struMacro );
   }
   else if( hb_macroCheckParam( pVar ) )
   {
      /* only right side of alias operator is a string - macro-compile
       * this part only
       */
      HB_MACRO struMacro;
      int iStatus;

      struMacro.mode       = HB_MODE_MACRO;
      struMacro.supported  = (bSupported & HB_SM_RT_MACRO) ? s_macroFlags : bSupported;
      struMacro.Flags      = iFlag | HB_MACRO_GEN_ALIASED;
      struMacro.uiNameLen  = HB_SYMBOL_NAME_LEN;
      struMacro.status     = HB_MACRO_CONT;
      struMacro.string     = pVar->item.asString.value;
      struMacro.length     = pVar->item.asString.length;
      iStatus = hb_macroParse( &struMacro );

      hb_stackPop();    /* remove compiled string */

      if( iStatus == HB_MACRO_OK && ( struMacro.status & HB_MACRO_CONT ) )
         hb_macroRun( &struMacro );
      else
         hb_macroSyntaxError( &struMacro );

      hb_macroDelete( &struMacro );
   }
}

/* Compiles and run an aliased macro expression - generated pcode
 * pops a value from the stack
 *    &alias->var := any
 *    alias->&var := any
 */
void hb_macroPopAliasedValue( HB_ITEM_PTR pAlias, HB_ITEM_PTR pVar, BYTE flags )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroPopAliasedValue(%p, %p)", pAlias, pVar));

   hb_macroUseAliased( pAlias, pVar, HB_MACRO_GEN_POP, flags );
}

/* Compiles and run an aliased macro expression - generated pcode
 * pushes a value onto the stack
 *    any := &alias->var
 *    any := alias->&var
 */
void hb_macroPushAliasedValue( HB_ITEM_PTR pAlias, HB_ITEM_PTR pVar, BYTE flags )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroPushAliasedValue(%p, %p)", pAlias, pVar));

   hb_macroUseAliased( pAlias, pVar, HB_MACRO_GEN_PUSH, flags );
}

/* Check for '&' operator and replace it with a macro variable value
 * Returns: the passed string if there is no '&' operator (pbNewString:=FALSE)
 * new string if a valid macro text substitution was found (and sets
 * pbNewString to TRUE)
*/
char * hb_macroExpandString( char *szString, ULONG ulLength, BOOL *pfNewString )
{
   char *szResultString;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroExpandString(%s,%lu,%p)", szString, ulLength, pfNewString));

   if( szString )
      szResultString = hb_macroTextSubst( szString, &ulLength );
   else
      szResultString = szString;
   *pfNewString = ( szString != szResultString );
   return szResultString;
}

char * hb_macroTextSymbol( char *szString, ULONG ulLength, BOOL *pfNewString )
{
   char *szResult = NULL;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroTextSymbol(%s,%lu,%p)", szString, ulLength, pfNewString));

   if( szString )
   {
      ULONG ulLen = 0;

      szResult = hb_macroTextSubst( szString, &ulLength );

      while( ulLength && ( szResult[ 0 ] == ' ' || szResult[ 0 ] == '\t' ) )
      {
         ++szResult;
         ++szString;
         --ulLength;
      }

      while( ulLength && ( szResult[ ulLength - 1 ] == ' ' ||
                           szResult[ ulLength - 1 ] == '\t' ) )
         --ulLength;

      /* NOTE: This uses _a-zA-Z0-9 pattern to check for a valid name
       * "_" is not valid macro string
       */
      while( ulLen < ulLength )
      {
         char c = szResult[ ulLen ];
         if( c >= 'a' && c <= 'z' )
         {
            if( szResult == szString )
            {
               szResult = ( char * ) hb_xgrab( ulLength + 1 );
               memcpy( szResult, szString, ulLength );
               szResult[ ulLength ] = '\0';
            }
            szResult[ ulLen ] = c - ( 'a' - 'A' );
         }
         else if( ! ( c == '_' || ( c >= 'A' && c <= 'Z' ) ||
                      ( ulLen && ( c >= '0' && c <= '9' ) ) ) )
         {
            break;
         }
         ++ulLen;
      }
      if( ulLen == ulLength && ulLen > ( ULONG ) ( szResult[ 0 ] == '_' ? 1 : 0 ) )
      {
         if( ulLen > HB_SYMBOL_NAME_LEN )
            ulLen = HB_SYMBOL_NAME_LEN;
         if( szResult[ ulLen ] )
         {
            if( szResult == szString )
            {
               szResult = ( char * ) hb_xgrab( ulLen + 1 );
               memcpy( szResult, szString, ulLen );
            }
            szResult[ ulLen ] = '\0';
         }
      }
      else if( szResult != szString )
      {
         hb_xfree( szResult );
         szResult = NULL;
      }
   }
   *pfNewString = szResult && szString != szResult;
   return szResult;
}

/* compile a string and return a pcode to push a value of expression
 * NOTE: it can be called to implement an index key evaluation
 * use hb_macroRun() to evaluate a compiled pcode
 */
HB_MACRO_PTR hb_macroCompile( char * szString )
{
   HB_MACRO_PTR pMacro;
   int iStatus;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroCompile(%s)", szString));

   pMacro = ( HB_MACRO_PTR ) hb_xgrab( sizeof( HB_MACRO ) );
   pMacro->mode      = HB_MODE_MACRO;
   pMacro->supported = s_macroFlags;
   pMacro->Flags     = HB_MACRO_DEALLOCATE | HB_MACRO_GEN_PUSH |
                       HB_MACRO_GEN_LIST | HB_MACRO_GEN_PARE;
   pMacro->uiNameLen = HB_SYMBOL_NAME_LEN;
   pMacro->status    = HB_MACRO_CONT;
   pMacro->string    = szString;
   pMacro->length    = strlen( szString );

   iStatus = hb_macroParse( pMacro );
   if( ! ( iStatus == HB_MACRO_OK && ( pMacro->status & HB_MACRO_CONT ) ) )
   {
      hb_macroDelete( pMacro );
      pMacro = NULL;
   }

   return pMacro;
}

/* This function handles a macro function calls, e.g. var :=&macro()
 * and creating memvar variables using PUBLIC/PRIVATE command
 * PUBLIC &macro
 *
 * 'pItem' points to a ITEM that contains a string value which after
 *    text substitution will return a function name
 */
void hb_macroPushSymbol( HB_ITEM_PTR pItem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroPushSymbol(%p)", pItem));

   if( hb_macroCheckParam( pItem ) )
   {
      char * szString;
      BOOL fNewBuffer;

      szString = hb_macroTextSymbol( pItem->item.asString.value,
                                     pItem->item.asString.length,
                                     &fNewBuffer );
      if( szString )
      {
         HB_DYNS_PTR pDynSym = hb_dynsymGetCase( szString );

         hb_stackPop();    /* remove compiled string */
         /* NOTE: checking for valid function name (valid pointer) is done
          * in hb_vmDo()
          */
         hb_vmPushSymbol( pDynSym->pSymbol );  /* push compiled symbol instead of a string */
      }
      else
      {
         hb_stackPop();    /* remove compiled string */
         hb_macroSyntaxError( NULL );
      }
      if( fNewBuffer )
         hb_xfree( szString );   /* free space allocated in hb_macroTextSymbol */
   }
}

/* Macro text substitution
 *
 * 'pItem' points to a ITEM that contains a string value which after
 *    text substitution will be returned
 */
void hb_macroTextValue( HB_ITEM_PTR pItem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroTextValue(%p)", pItem));

   if( hb_macroCheckParam( pItem ) )
   {
      char * szString;
      ULONG ulLength = pItem->item.asString.length;

      szString = hb_macroTextSubst( pItem->item.asString.value, &ulLength );

      if( szString != pItem->item.asString.value )
      {
         /* replace the old value on the eval stack with the new one
         */
         hb_itemPutCPtr( pItem, szString, ulLength );
      }
      /*
       * else
       *    leave original value on the eval stack - there was no '&' operator
       *    inside a string
       */
   }
}

char * hb_macroGetType( HB_ITEM_PTR pItem )
{
   char * szType;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroGetType(%p)", pItem));

   if( hb_macroCheckParam( pItem ) )
   {
      HB_MACRO struMacro;
      int iStatus;

      struMacro.mode       = HB_MODE_MACRO;
      struMacro.supported  = s_macroFlags;
      struMacro.Flags      = HB_MACRO_GEN_PUSH | HB_MACRO_GEN_TYPE;
      struMacro.uiNameLen  = HB_SYMBOL_NAME_LEN;
      struMacro.status     = HB_MACRO_CONT;
      struMacro.string     = pItem->item.asString.value;
      struMacro.length     = pItem->item.asString.length;
      iStatus = hb_macroParse( &struMacro );

      if( iStatus == HB_MACRO_OK )
      {
         /* passed string was successfully compiled
          */
         if( struMacro.exprType == HB_ET_CODEBLOCK )
         {
            /* Clipper ignores any undeclared symbols or UDFs if the
             * compiled expression is a valid codeblock
            */
            szType ="B";
         }
         else if( struMacro.status & HB_MACRO_UNKN_SYM )
         {
            /* request for a symbol that is not in a symbol table or
             * for a variable that is not visible
             */
            szType = "U";
         }
         else if( struMacro.status & HB_MACRO_UDF )
         {
            szType = "UI";  /* UDF function was used - cannot determine a type */
         }
         else if( struMacro.status & HB_MACRO_CONT )
         {
            /* OK - the pcode was generated and it can be evaluated
            */
            HB_ERROR_INFO struErr;
            HB_ERROR_INFO_PTR pOld;

            /* Set our temporary error handler. We do not need any error
             * messages here - we need to know only if evaluation was
             * successfull. If evaluation was successfull then the data type
             * of expression can be determined.
             */
            struErr.Func  = hb_macroErrorType;
            struErr.Cargo = ( void * ) &struMacro;
            pOld = hb_errorHandler( &struErr );
            hb_macroRun( &struMacro );
            hb_errorHandler( pOld );

            if( struMacro.status & HB_MACRO_CONT )
            {
               /* Evaluation was successfull
                * Now the value of expression is placed on the eval stack -
                * check its type and pop it from the stack
                */
               szType = hb_itemTypeStr( hb_stackItemFromTop( -1 ) );
               hb_stackPop();
            }
            else
            {
               /* something unpleasant happened during macro evaluation */
               if( struMacro.pError )
               {
                  ULONG ulGenCode;
                  
                  ulGenCode = hb_errGetGenCode( struMacro.pError );
                  if( ulGenCode == EG_NOVAR || ulGenCode == EG_NOALIAS )
                  {
                     /* Undeclared variable returns 'U' in Clipper */
                     szType = "U";
                  }
                  else
                     szType = "UE";
               }
               else
                  szType = "UE";
            }
         }
         else
         {
            szType = "UE";
         }
      }
      else
         szType = "UE";  /* syntax error during compilation */

      hb_macroDelete( &struMacro );
   }
   else
      szType = "U";

   return szType;
}

/*
 * Set macro capabilities if flag > 0 or get current macro capabilities
 * if flag == 0
 */
ULONG hb_macroSetMacro( BOOL bSet, ULONG flag )
{
   ULONG ulCurrentFlags = s_macroFlags;

   if( flag > 0 )
   {
      if( bSet )
         s_macroFlags |= flag;
      else
         s_macroFlags &= ~flag;
   }

   return ulCurrentFlags;
}

HB_FUNC( HB_SETMACRO )
{
   int iPrmCnt = hb_pcount();

   if( iPrmCnt > 0 )
   {
       ULONG ulFlags = ( ULONG ) hb_parnl( 1 );
       PHB_ITEM pValue;

       switch( ulFlags )
       {
          case HB_SM_HARBOUR:
             /* enable/disable extended Harbour compatibility */
             hb_retl( s_macroFlags & ulFlags );
             pValue = hb_param( 2, HB_IT_LOGICAL );
             if( pValue )
                hb_macroSetMacro( hb_itemGetL( pValue ), ulFlags );
             break;

          case HB_SM_XBASE:
             /* enable/disable extended xbase compatibility */
             hb_retl( s_macroFlags & ulFlags );
             pValue = hb_param( 2, HB_IT_LOGICAL );
             if( pValue )
                hb_macroSetMacro( hb_itemGetL( pValue ), ulFlags );
             break;

          case HB_SM_PREPROC :
             /* enable/disable preprocessing before compilation */
             hb_retl( s_macroFlags & ulFlags );
             pValue = hb_param( 2, HB_IT_LOGICAL );
             if( pValue )
                hb_macroSetMacro( hb_itemGetL( pValue ), ulFlags );
             break;

          case HB_SM_ARRSTR :
             /* enable/disable processing of strings as an array of bytes */
             hb_retl( s_macroFlags & ulFlags );
             pValue = hb_param( 2, HB_IT_LOGICAL );
             if( pValue )
                hb_macroSetMacro( hb_itemGetL( pValue ), ulFlags );
             break;

          case HB_SM_SHORTCUTS:
             /* enable/disable support for shortcut logical operators */
             hb_retl( s_macroFlags & ulFlags );
             pValue = hb_param( 2, HB_IT_LOGICAL );
             if( pValue )
                hb_macroSetMacro( hb_itemGetL( pValue ), ulFlags );
             break;

          default:
              ;/* do nothing */
      }
   }
   else
      hb_ret();    /* return NIL */
}

/* ************************************************************************* */

/* returns the order + 1 of a variable if defined or zero */
static int hb_macroLocalVarGetPos( char * szVarName, HB_COMP_DECL )
{
   int iVar = 1;
   HB_CBVAR_PTR pVars = HB_PCODE_DATA->pLocals;

   while( pVars )
   {
      if( pVars->szName && ! strcmp( pVars->szName, szVarName ) )
         return iVar;
      else
      {
         if( pVars->pNext )
         {
            pVars = pVars->pNext;
            iVar++;
         }
         else
            return 0;
      }
   }
   return 0;
}

BOOL hb_macroIsValidMacroText( char * szText, ULONG ulLen )
{
   if( ulLen )
   {
      while( --ulLen )
      {
         if( *szText++ == '&' )
         {
            char ch = *szText;
            if( ( ch >= 'A' && ch <= 'Z' ) ||
                ( ch >= 'a' && ch <= 'z' ) || ch == '_' )
               return TRUE;
         }
      }
   }

   return FALSE;
}

ULONG hb_macroGenJump( LONG lOffset, HB_COMP_DECL )
{
   if( lOffset == 0 )
      hb_macroGenPCode4( HB_P_JUMPFAR, 0, 0, 0, HB_COMP_PARAM );
   else if( HB_LIM_INT8( lOffset ) )
      hb_macroGenPCode2( HB_P_JUMPNEAR, HB_LOBYTE( lOffset ), HB_COMP_PARAM );
   else if( HB_LIM_INT16( lOffset ) )
      hb_macroGenPCode3( HB_P_JUMP, HB_LOBYTE( lOffset ), HB_HIBYTE( lOffset ), HB_COMP_PARAM );
   else if( HB_LIM_INT24( lOffset ) )
      hb_macroGenPCode4( HB_P_JUMPFAR, HB_LOBYTE( lOffset ), HB_HIBYTE( lOffset ), HB_ULBYTE( lOffset ), HB_COMP_PARAM );
   else
      hb_macroError( HB_MACRO_TOO_COMPLEX, HB_COMP_PARAM );

   return HB_PCODE_DATA->lPCodePos - 3;
}

ULONG hb_macroGenJumpFalse( LONG lOffset, HB_COMP_DECL )
{
   if( lOffset == 0 )
      hb_macroGenPCode4( HB_P_JUMPFALSEFAR, 0, 0, 0, HB_COMP_PARAM );
   else if( HB_LIM_INT8( lOffset ) )
      hb_macroGenPCode2( HB_P_JUMPFALSENEAR, HB_LOBYTE( lOffset ), HB_COMP_PARAM );
   else if( HB_LIM_INT16( lOffset ) )
      hb_macroGenPCode3( HB_P_JUMPFALSE, HB_LOBYTE( lOffset ), HB_HIBYTE( lOffset ), HB_COMP_PARAM );
   else if( HB_LIM_INT24( lOffset ) )
      hb_macroGenPCode4( HB_P_JUMPFALSEFAR, HB_LOBYTE( lOffset ), HB_HIBYTE( lOffset ), HB_ULBYTE( lOffset ), HB_COMP_PARAM );
   else
      hb_macroError( HB_MACRO_TOO_COMPLEX, HB_COMP_PARAM );

   return HB_PCODE_DATA->lPCodePos - 3;
}

ULONG hb_macroGenJumpTrue( LONG lOffset, HB_COMP_DECL )
{
   if( lOffset == 0 )
      hb_macroGenPCode4( HB_P_JUMPTRUEFAR, 0, 0, 0, HB_COMP_PARAM );
   else if( HB_LIM_INT8( lOffset ) )
      hb_macroGenPCode2( HB_P_JUMPTRUENEAR, HB_LOBYTE( lOffset ), HB_COMP_PARAM );
   else if( HB_LIM_INT16( lOffset ) )
      hb_macroGenPCode3( HB_P_JUMPTRUE, HB_LOBYTE( lOffset ), HB_HIBYTE( lOffset ), HB_COMP_PARAM );
   else if( HB_LIM_INT24( lOffset ) )
      hb_macroGenPCode4( HB_P_JUMPTRUEFAR, HB_LOBYTE( lOffset ), HB_HIBYTE( lOffset ), HB_ULBYTE( lOffset ), HB_COMP_PARAM );
   else
      hb_macroError( HB_MACRO_TOO_COMPLEX, HB_COMP_PARAM );

   return HB_PCODE_DATA->lPCodePos - 3;
}

void hb_macroGenJumpThere( ULONG ulFrom, ULONG ulTo, HB_COMP_DECL )
{
   BYTE * pCode = HB_PCODE_DATA->pCode;
   LONG lOffset = ulTo - ulFrom + 1;

   if( HB_LIM_INT24( lOffset ) )
      HB_PUT_LE_UINT24( &pCode[ ulFrom ], lOffset );
   else
      hb_macroError( HB_MACRO_TOO_COMPLEX, HB_COMP_PARAM );
}

void hb_macroGenJumpHere( ULONG ulOffset, HB_COMP_DECL )
{
   hb_macroGenJumpThere( ulOffset, HB_PCODE_DATA->lPCodePos, HB_COMP_PARAM );
}

/*
 * Function generates pcode for passed memvar name
 */
static void hb_macroMemvarGenPCode( BYTE bPCode, char * szVarName, HB_COMP_DECL )
{
   BYTE byBuf[ sizeof( HB_DYNS_PTR ) + 1 ];
   HB_DYNS_PTR pSym;

   if( HB_MACRO_DATA->Flags & HB_MACRO_GEN_TYPE )
   {
      /* we are determining the type of expression (called from TYPE() function)
       * then we shouldn't create the requested variable if it doesn't exist
       */
      pSym = hb_dynsymFind( szVarName );
      if( !pSym )
      {
         HB_MACRO_DATA->status |= HB_MACRO_UNKN_VAR;
         pSym = hb_dynsymGetCase( szVarName );
      }
   }
   else
      /* Find the address of passed symbol - create the symbol if doesn't exist
       * (Clipper compatibility). */
      pSym = hb_dynsymGetCase( szVarName );

   byBuf[ 0 ] = bPCode;
   HB_PUT_PTR( &byBuf[ 1 ], pSym );
   hb_macroGenPCodeN( byBuf, sizeof( byBuf ), HB_COMP_PARAM );
}

/* generates the pcode to push a symbol on the virtual machine stack */
void hb_macroGenPushSymbol( char * szSymbolName, BOOL bFunction, HB_COMP_DECL )
{
   BYTE byBuf[ sizeof( HB_DYNS_PTR ) + 1 ];
   HB_DYNS_PTR pSym;

   if( HB_MACRO_DATA->Flags & HB_MACRO_GEN_TYPE )
   {
      /* we are determining the type of expression (called from TYPE() function)
       */
      pSym = hb_dynsymFind( szSymbolName );
      if( ! pSym )
      {
         HB_MACRO_DATA->status |= HB_MACRO_UNKN_SYM;
         HB_MACRO_DATA->status &= ~HB_MACRO_CONT;  /* don't run this pcode */
         /*
          * NOTE: the compiled pcode will be not executed then we can ignore
          * NULL value for pSym
          */
      }
      else if( bFunction )
      {
         if( pSym->pSymbol->value.pFunPtr == NULL )
         {
            /* static functions are not allowed in macro */
            HB_MACRO_DATA->status |= HB_MACRO_UNKN_SYM;
            HB_MACRO_DATA->status &= ~HB_MACRO_CONT;  /* don't run this pcode */
         }
      }
   }
   else
      pSym = hb_dynsymGetCase( szSymbolName );

   byBuf[ 0 ] = HB_P_MPUSHSYM;
   HB_PUT_PTR( &byBuf[ 1 ], pSym );
   hb_macroGenPCodeN( byBuf, sizeof( byBuf ), HB_COMP_PARAM );
}

/* generates the pcode to push a long number on the virtual machine stack */
void hb_macroGenPushLong( HB_LONG lNumber, HB_COMP_DECL )
{
   if( lNumber == 0 )
   {
      hb_macroGenPCode1( HB_P_ZERO, HB_COMP_PARAM );
   }
   else if( lNumber == 1 )
   {
      hb_macroGenPCode1( HB_P_ONE, HB_COMP_PARAM );
   }
   else if( HB_LIM_INT8( lNumber ) )
   {
      hb_macroGenPCode2( HB_P_PUSHBYTE, (BYTE) lNumber, HB_COMP_PARAM );
   }
   else if( HB_LIM_INT16( lNumber ) )
   {
      hb_macroGenPCode3( HB_P_PUSHINT, HB_LOBYTE( lNumber ), HB_HIBYTE( lNumber ), HB_COMP_PARAM );
   }
   else if( HB_LIM_INT32( lNumber ) )
   {
      BYTE pBuffer[ 5 ];
      pBuffer[ 0 ] = HB_P_PUSHLONG;
      HB_PUT_LE_UINT32( pBuffer + 1, lNumber );
      hb_macroGenPCodeN( pBuffer, sizeof( pBuffer ), HB_COMP_PARAM );
   }
   else
   {
      BYTE pBuffer[ 9 ];
      pBuffer[ 0 ] = HB_P_PUSHLONGLONG;
      HB_PUT_LE_UINT64( pBuffer + 1, lNumber );
      hb_macroGenPCodeN( pBuffer, sizeof( pBuffer ), HB_COMP_PARAM );
   }
}

/* generates the pcode to push a date on the virtual machine stack */
void hb_macroGenPushDate( HB_LONG lNumber, HB_COMP_DECL )
{
   BYTE pBuffer[ 5 ];
   
   pBuffer[ 0 ] = HB_P_PUSHDATE;
   HB_PUT_LE_UINT32( pBuffer + 1, lNumber );
   hb_macroGenPCodeN( pBuffer, sizeof( pBuffer ), HB_COMP_PARAM );
}

/* sends a message to an object */
void hb_macroGenMessage( char * szMsgName, BOOL bIsObject, HB_COMP_DECL )
{
   BYTE byBuf[ sizeof( HB_DYNS_PTR ) + 1 ];

   /* Find the address of passed symbol - create the symbol if doesn't exist
    */
   HB_DYNS_PTR pSym = hb_dynsymGetCase( szMsgName );

   byBuf[ 0 ] = HB_P_MMESSAGE;
   HB_PUT_PTR( &byBuf[ 1 ], pSym );
   hb_macroGenPCodeN( byBuf, sizeof( byBuf ), HB_COMP_PARAM );

   HB_SYMBOL_UNUSED( bIsObject );   /* used in full compiler only */
}

/* generates an underscore-symbol name for a data assignment */
void hb_macroGenMessageData( char * szMsg, BOOL bIsObject, HB_COMP_DECL )
{
   char * szResult;
   int iLen;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroGenMessageData(%s)", szMsg));

   iLen = ( int ) strlen( szMsg );
   if( iLen > HB_SYMBOL_NAME_LEN - 1 )
      iLen = HB_SYMBOL_NAME_LEN - 1;
   szResult = ( char * ) hb_xgrab( iLen + 2 );
   szResult[ 0 ] = '_';
   memcpy( szResult + 1, szMsg, iLen );
   szResult[ iLen + 1 ] = '\0';
   hb_macroGenMessage( szResult, bIsObject, HB_COMP_PARAM );
   hb_xfree( szResult );
}

/* generates the pcode to pop a value from the virtual machine stack onto a variable */
void hb_macroGenPopVar( char * szVarName, HB_COMP_DECL )
{
   int iVar;

   iVar = hb_macroLocalVarGetPos( szVarName, HB_COMP_PARAM );
   if( iVar )
   {
      /* this is a codeblock parameter */
      hb_macroGenPCode3( HB_P_POPLOCAL, HB_LOBYTE( iVar ), HB_HIBYTE( iVar ), HB_COMP_PARAM );
   }
   else
   {
      /* TODO: memvars created inside TYPE() function should have PUBLIC scope */
      hb_macroMemvarGenPCode( HB_P_MPOPMEMVAR, szVarName, HB_COMP_PARAM );
   }
}

/* generates the pcode to pop a value from the virtual machine stack onto
 * an aliased variable
 */
void hb_macroGenPopAliasedVar( char * szVarName,
                               BOOL bPushAliasValue,
                               char * szAlias,
                               HB_LONG lWorkarea, HB_COMP_DECL )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroGenPopAliasedVar(%s->%s)",szAlias,szVarName));

   if( bPushAliasValue )
   {
      if( szAlias )
      {
         int iLen = strlen( szAlias );

         if( szAlias[ 0 ] == 'M' && ( iLen == 1 ||
             ( iLen >= 4 && iLen <= 6 && strncmp( szAlias, "MEMVAR", iLen ) == 0 ) ) )
         {  /* M-> or MEMV-> or MEMVA-> or MEMVAR-> variable */
            /* TODO: memvars created inside TYPE() function should have PUBLIC scope */
            hb_macroMemvarGenPCode( HB_P_MPOPMEMVAR, szVarName, HB_COMP_PARAM );
         }
         else if( iLen >= 4 && iLen <= 5 && strncmp( szAlias, "FIELD", iLen ) == 0 )
         {  /* FIELD-> */
            hb_macroMemvarGenPCode( HB_P_MPOPFIELD, szVarName, HB_COMP_PARAM );
         }
         else
         {  /* database alias */
            hb_macroGenPushSymbol( szAlias, FALSE, HB_COMP_PARAM );
            hb_macroMemvarGenPCode( HB_P_MPOPALIASEDFIELD, szVarName, HB_COMP_PARAM );
         }
      }
      else
      {
         hb_macroGenPushLong( lWorkarea, HB_COMP_PARAM );
         hb_macroMemvarGenPCode( HB_P_MPOPALIASEDFIELD, szVarName, HB_COMP_PARAM );
      }
   }
   else
   {
      /* Alias is already placed on stack
       * NOTE: An alias will be determined at runtime then we cannot decide
       * here if passed name is either a field or a memvar
       */
      /* TODO: memvars created inside TYPE() function should have PUBLIC scope */
      hb_macroMemvarGenPCode( HB_P_MPOPALIASEDVAR, szVarName, HB_COMP_PARAM );
   }
}

/* generates the pcode to push a nonaliased variable value to the virtual
 * machine stack
 */
void hb_macroGenPushVar( char * szVarName, BOOL bMacroVar, HB_COMP_DECL )
{
   int iVar;

   HB_SYMBOL_UNUSED( bMacroVar );

   iVar = hb_macroLocalVarGetPos( szVarName, HB_COMP_PARAM );
   if( iVar )
   {
      /* this is a codeblock parameter */
      hb_macroGenPCode3( HB_P_PUSHLOCAL, HB_LOBYTE( iVar ), HB_HIBYTE( iVar ), HB_COMP_PARAM );
   }
   else
   {
      hb_macroMemvarGenPCode( HB_P_MPUSHVARIABLE, szVarName, HB_COMP_PARAM );
   }
}

/* generates the pcode to push a variable by reference to the virtual machine stack */
void hb_macroGenPushVarRef( char * szVarName, HB_COMP_DECL )
{
   USHORT iVar;

   iVar = hb_macroLocalVarGetPos( szVarName, HB_COMP_PARAM );
   if( iVar )
      hb_macroGenPCode3( HB_P_PUSHLOCALREF, HB_LOBYTE( iVar ), HB_HIBYTE( iVar ), HB_COMP_PARAM );
   else
   {
      hb_macroMemvarGenPCode( HB_P_MPUSHMEMVARREF, szVarName, HB_COMP_PARAM );
   }
}

/* generates the pcode to push a variable by reference to the virtual machine stack */
void hb_macroGenPushMemvarRef( char * szVarName, HB_COMP_DECL )
{
   hb_macroMemvarGenPCode( HB_P_MPUSHMEMVARREF, szVarName, HB_COMP_PARAM );
}

/* generates the pcode to push an aliased variable value to the virtual
 * machine stack
 */
void hb_macroGenPushAliasedVar( char * szVarName,
                                BOOL bPushAliasValue,
                                char * szAlias,
                                HB_LONG lWorkarea, HB_COMP_DECL )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroGenPushAliasedVar(%s->%s)",szAlias,szVarName));

   if( bPushAliasValue )
   {
      if( szAlias )
      {
         /* myalias->var
         * FIELD->var
         * MEMVAR->var
         */
         int iLen = strlen( szAlias );

         if( szAlias[ 0 ] == 'M' && ( iLen == 1 ||
             ( iLen >= 4 && iLen <= 6 && strncmp( szAlias, "MEMVAR", iLen ) == 0 ) ) )
         {  /* M-> or MEMV-> or MEMVA-> or MEMVAR-> variable */
            hb_macroMemvarGenPCode( HB_P_MPUSHMEMVAR, szVarName, HB_COMP_PARAM );
         }
         else if( iLen >= 4 && iLen <= 5 && strncmp( szAlias, "FIELD", iLen ) == 0 )
         {  /* FIELD-> */
            hb_macroMemvarGenPCode( HB_P_MPUSHFIELD, szVarName, HB_COMP_PARAM );
         }
         else
         {  /* database alias */
            hb_macroGenPushSymbol( szAlias, FALSE, HB_COMP_PARAM );
            hb_macroMemvarGenPCode( HB_P_MPUSHALIASEDFIELD, szVarName, HB_COMP_PARAM );
         }
      }
      else
      {
         hb_macroGenPushLong( lWorkarea, HB_COMP_PARAM );
         hb_macroMemvarGenPCode( HB_P_MPUSHALIASEDFIELD, szVarName, HB_COMP_PARAM );
      }
   }
   else
   {
      /* Alias is already placed on stack
       * NOTE: An alias will be determined at runtime then we cannot decide
       * here if passed name is either a field or a memvar
       */
      hb_macroMemvarGenPCode( HB_P_MPUSHALIASEDVAR, szVarName, HB_COMP_PARAM );
   }
}

/* pushes a logical value on the virtual machine stack , */
void hb_macroGenPushLogical( int iTrueFalse, HB_COMP_DECL )
{
   if( iTrueFalse )
      hb_macroGenPCode1( HB_P_TRUE, HB_COMP_PARAM );
   else
      hb_macroGenPCode1( HB_P_FALSE, HB_COMP_PARAM );
}

/* generates the pcode to push a double number on the virtual machine stack */
void hb_macroGenPushDouble( double dNumber, BYTE bWidth, BYTE bDec, HB_COMP_DECL )
{
   BYTE pBuffer[ sizeof( double ) + sizeof( BYTE ) + sizeof( BYTE ) + 1 ];

   pBuffer[ 0 ] = HB_P_PUSHDOUBLE;
   HB_PUT_LE_DOUBLE( &( pBuffer[ 1 ] ), dNumber );
   pBuffer[ 1 + sizeof( double ) ] = bWidth;
   pBuffer[ 1 + sizeof( double ) + sizeof( BYTE ) ] = bDec;

   hb_macroGenPCodeN( pBuffer, 1 + sizeof( double ) + sizeof( BYTE ) + sizeof( BYTE ), HB_COMP_PARAM );
}

void hb_macroGenPushFunSym( char * szFunName, HB_COMP_DECL )
{
   char * szFunction;

   szFunction = hb_compReservedName( szFunName );
   if( szFunction )
   {
      /* Abbreviated function name was used - change it for whole name
       */
      hb_macroGenPushSymbol( szFunction, TRUE, HB_COMP_PARAM );
   }
   else
   {
      HB_MACRO_DATA->status |= HB_MACRO_UDF; /* this is used in hb_macroGetType */
      hb_macroGenPushSymbol( szFunName, TRUE, HB_COMP_PARAM );
   }
}

void hb_macroGenPushFunCall( char * szFunName, HB_COMP_DECL )
{
   hb_macroGenPushFunSym( szFunName, HB_COMP_PARAM );
   hb_macroGenPCode1( HB_P_PUSHNIL, HB_COMP_PARAM );
}

void hb_macroGenPushFunRef( char * szFunName, HB_COMP_DECL )
{
   char * szFunction;

   /* if abbreviated function name was used - change it for whole name */
   szFunction = hb_compReservedName( szFunName );
   hb_macroGenPushSymbol( szFunction ? szFunction : szFunName, TRUE, HB_COMP_PARAM );
}

/* generates the pcode to push a string on the virtual machine stack */
void hb_macroGenPushString( char * szText, ULONG ulStrLen, HB_COMP_DECL )
{
   if( ulStrLen <= UINT24_MAX )
   {
      if( ulStrLen <= USHRT_MAX )
         hb_macroGenPCode3( HB_P_MPUSHSTR, HB_LOBYTE( ulStrLen ), HB_HIBYTE( ulStrLen ), HB_COMP_PARAM );
      else 
         hb_macroGenPCode4( HB_P_MPUSHSTRLARGE, HB_LOBYTE( ulStrLen ), HB_HIBYTE( ulStrLen ), HB_ULBYTE( ulStrLen ), HB_COMP_PARAM );
      hb_macroGenPCodeN( ( BYTE * ) szText, ulStrLen, HB_COMP_PARAM );
   }
   else
      hb_macroError( HB_MACRO_TOO_COMPLEX, HB_COMP_PARAM );
}

void hb_macroGenPCode1( BYTE byte, HB_COMP_DECL )
{
   HB_PCODE_INFO_PTR pFunc = HB_PCODE_DATA;

   if( ( pFunc->lPCodeSize - pFunc->lPCodePos ) < 1 )
      pFunc->pCode = ( BYTE * ) hb_xrealloc( pFunc->pCode, pFunc->lPCodeSize += HB_PCODE_SIZE );

   pFunc->pCode[ pFunc->lPCodePos++ ] = byte;
}

void hb_macroGenPCode2( BYTE byte1, BYTE byte2, HB_COMP_DECL )
{
   HB_PCODE_INFO_PTR pFunc = HB_PCODE_DATA;

   if( ( pFunc->lPCodeSize - pFunc->lPCodePos ) < 2 )
      pFunc->pCode = ( BYTE * ) hb_xrealloc( pFunc->pCode, pFunc->lPCodeSize += HB_PCODE_SIZE );

   pFunc->pCode[ pFunc->lPCodePos++ ] = byte1;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte2;
}

void hb_macroGenPCode3( BYTE byte1, BYTE byte2, BYTE byte3, HB_COMP_DECL )
{
   HB_PCODE_INFO_PTR pFunc = HB_PCODE_DATA;

   if( ( pFunc->lPCodeSize - pFunc->lPCodePos ) < 3 )
      pFunc->pCode = ( BYTE * ) hb_xrealloc( pFunc->pCode, pFunc->lPCodeSize += HB_PCODE_SIZE );

   pFunc->pCode[ pFunc->lPCodePos++ ] = byte1;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte2;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte3;
}

void hb_macroGenPCode4( BYTE byte1, BYTE byte2, BYTE byte3, BYTE byte4, HB_COMP_DECL )
{
   HB_PCODE_INFO_PTR pFunc = HB_PCODE_DATA;

   if( ( pFunc->lPCodeSize - pFunc->lPCodePos ) < 4 )
      pFunc->pCode = ( BYTE * ) hb_xrealloc( pFunc->pCode, pFunc->lPCodeSize += HB_PCODE_SIZE );

   pFunc->pCode[ pFunc->lPCodePos++ ] = byte1;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte2;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte3;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte4;
}

void hb_macroGenPCodeN( BYTE * pBuffer, ULONG ulSize, HB_COMP_DECL )
{
   HB_PCODE_INFO_PTR pFunc = HB_PCODE_DATA;

   if( pFunc->lPCodePos + ulSize > pFunc->lPCodeSize )
   {
      /* not enough free space in pcode buffer - increase it */
      pFunc->lPCodeSize += ( ( ( ulSize / HB_PCODE_SIZE ) + 1 ) * HB_PCODE_SIZE );
      pFunc->pCode = ( BYTE * ) hb_xrealloc( pFunc->pCode, pFunc->lPCodeSize );
   }

   memcpy( pFunc->pCode + pFunc->lPCodePos, pBuffer, ulSize );
   pFunc->lPCodePos += ulSize;
}

/* ************************************************************************* */

void hb_macroError( int iError, HB_COMP_DECL )
{
   HB_MACRO_DATA->status |= iError;
   HB_MACRO_DATA->status &= ~HB_MACRO_CONT;  /* clear CONT bit */
}

/*
 * Start a new pcode buffer for a codeblock
*/
void hb_macroCodeBlockStart( HB_COMP_DECL )
{
   HB_PCODE_INFO_PTR pCB;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroCodeBlockStart(%p)", HB_COMP_PARAM));

   pCB = ( HB_PCODE_INFO_PTR ) hb_xgrab( sizeof( HB_PCODE_INFO ) );

   /* replace current pcode buffer with the new one
    */
   pCB->pPrev = HB_PCODE_DATA;
   HB_PCODE_DATA = pCB;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroCodeBlockStart.(%p)", HB_COMP_PARAM));
   pCB->pCode = ( BYTE * ) hb_xgrab( HB_PCODE_SIZE );
   pCB->lPCodeSize = HB_PCODE_SIZE;
   pCB->lPCodePos  = 0;
   pCB->fVParams   = FALSE;
   pCB->pLocals    = NULL;
}

void hb_macroCodeBlockEnd( HB_COMP_DECL )
{
   HB_PCODE_INFO_PTR pCodeblock;   /* pointer to the current codeblock */
   ULONG ulSize;
   USHORT wParms = 0;   /* number of codeblock parameters */
   HB_CBVAR_PTR pVar;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroCodeBlockEnd(%p)", HB_COMP_PARAM));

   /* a currently processed codeblock */
   pCodeblock = HB_PCODE_DATA;

   /* return to pcode buffer of a codeblock in which the current
    * codeblock was defined
    */
   HB_PCODE_DATA = pCodeblock->pPrev;

   /* generate a proper codeblock frame with a codeblock size and with
    * a number of expected parameters
    */

   /* Count the number of codeblock parameters */
   pVar = pCodeblock->pLocals;
   while( pVar )
   {
      pVar = pVar->pNext;
      ++wParms;
   }

   /*NOTE:  6 = HB_P_MPUSHBLOCK + USHORT( size ) + USHORT( wParams ) + _ENDBLOCK
    * runtime compiled codeblock cannot reference local variables defined in a
    * function
    */
   ulSize = ( ULONG ) pCodeblock->lPCodePos + 6;

   /*NOTE: HB_P_MPUSHBLOCK differs from HB_P_PUSHBLOCK - the pcode
    * is stored in dynamic memory pool instead of static memory
    */
   if( ulSize <= USHRT_MAX )
      hb_macroGenPCode3( HB_P_MPUSHBLOCK, HB_LOBYTE( ulSize ), HB_HIBYTE( ulSize ), HB_COMP_PARAM );
   else
   {
      ++ulSize;
      hb_macroGenPCode4( HB_P_MPUSHBLOCKLARGE, HB_LOBYTE( ulSize ), HB_HIBYTE( ulSize ), HB_ULBYTE( ulSize ), HB_COMP_PARAM );
   }
   hb_macroGenPCode2( HB_LOBYTE( wParms ), HB_HIBYTE( wParms ), HB_COMP_PARAM );

   /* copy a codeblock pcode buffer */
   hb_macroGenPCodeN( pCodeblock->pCode, pCodeblock->lPCodePos, HB_COMP_PARAM );
   hb_macroGenPCode1( HB_P_ENDBLOCK, HB_COMP_PARAM ); /* finish the codeblock */

   /* free memory allocated for a codeblock */
   hb_xfree( ( void * ) pCodeblock->pCode );
   hb_xfree( ( void * ) pCodeblock );
}
