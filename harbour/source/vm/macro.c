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

#include "hbmacro.h"
#include "hbcomp.h"
#include "hbstack.h"
#include "hbmemvar.ch"   /* for values returned by hb_memvarScope() */

#ifdef HB_MACRO_STATEMENTS
  #include "hbpp.h"
#endif

extern HB_FUNC( __MVPUBLIC );

/* .and. & .or. expressions shortcuts - the expression optimiser needs
 * a global variable
 */
BOOL hb_comp_bShortCuts = TRUE;

typedef struct HB_MEXPR_ {
   HB_EXPR Expression;
   struct HB_MEXPR_ *pPrev;
} HB_MEXPR, *HB_MEXPR_PTR;

/* various flags for macro compiler */
static ULONG s_macroFlags = HB_SM_SHORTCUTS;
static HB_MEXPR_PTR s_macroAlloc = NULL;

static void hb_macroUseAliased( HB_ITEM_PTR, HB_ITEM_PTR, int, BYTE );
static void hb_compMemvarCheck( char * szVarName, HB_MACRO_DECL );

/* ************************************************************************* */

/* Allocates memory for Expression holder structure and stores it
 * on the linked list
*/
HB_EXPR_PTR hb_macroExprNew()
{
   HB_MEXPR_PTR pMExpr;
   
   pMExpr = (HB_MEXPR_PTR) hb_xgrab( sizeof( HB_MEXPR ) );
   if( s_macroAlloc == NULL )
   {
      s_macroAlloc = pMExpr;
      pMExpr->pPrev = NULL;
   }
   else
   {
      pMExpr->pPrev = s_macroAlloc;
      s_macroAlloc = pMExpr;
   }
   return &s_macroAlloc->Expression;
}

/* ************************************************************************* */

/* Compile passed string into a pcode buffer
 *
 * 'pMacro' - pointer to HB_MACRO structure that will hold all information
 *    nedded for macro compilation and evaluation
 * 'szString'  - a string to compile
 * 'iFlag' - specifies if compiled code should generate pcodes either for push
 *    operation (for example: var :=&macro) or for pop operation (&macro :=var)
 */
static int hb_macroParse( HB_MACRO_PTR pMacro, char * szString )
{
   int iResult;
   HB_MEXPR_PTR pMacroExprList;
   
   /* update the current status for logical shortcuts */
   hb_comp_bShortCuts = pMacro->supported & HB_SM_SHORTCUTS;

   /* initialize the input buffer - it will be scanned by lex */
   pMacro->string = szString;
   pMacro->length = strlen( szString );
   pMacro->pos    = 0;
   pMacro->bShortCuts = hb_comp_bShortCuts;
   pMacro->pError   = NULL;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroParse(%p, %s)", pMacro, szString));

   /* initialize the output (pcode) buffer - it will be filled by yacc */
   pMacro->pCodeInfo = (HB_PCODE_INFO_PTR ) hb_xgrab( sizeof( HB_PCODE_INFO ) );
   pMacro->pCodeInfo->lPCodeSize = HB_PCODE_SIZE;
   pMacro->pCodeInfo->lPCodePos  = 0;
   pMacro->pCodeInfo->pLocals    = NULL;
   pMacro->pCodeInfo->pPrev      = NULL;
   HB_TRACE(HB_TR_DEBUG, ("hb_macroParse.(%p, %s)", pMacro, szString));
   pMacro->pCodeInfo->pCode      = ( BYTE * ) hb_xgrab( HB_PCODE_SIZE );

   /* reset the type of compiled expression - this should be filled after
    * successfully compilation
    */
   pMacro->exprType = HB_ET_NONE;

   /* Deallocate all memory used by expression optimizer */
   pMacroExprList = s_macroAlloc; /* save current list for recurse entry */
   s_macroAlloc = NULL;
   iResult = hb_macroYYParse( pMacro );
   if( s_macroAlloc )
   {
      HB_MEXPR_PTR pMExpr = s_macroAlloc;
      do
      {
         hb_compExprDelete( &pMExpr->Expression, pMacro );
         pMExpr = pMExpr->pPrev;
      }
      while( pMExpr );
      do
      {
         pMExpr = s_macroAlloc->pPrev;
         hb_xfree( s_macroAlloc );
         s_macroAlloc = pMExpr;
      }
      while( s_macroAlloc );
   }
   s_macroAlloc = pMacroExprList;
   
   return iResult;
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

/* It handles an error generated during macro evaluation
 */
static HB_ERROR_HANDLE( hb_macroErrorEvaluation )
{
   HB_ITEM_PTR pResult = hb_itemDo( ErrorInfo->ErrorBlock, 1, ErrorInfo->Error );

   /* In a special case when QUIT is requested then there is no return
    * to code where macro evaluation was called. We have to
    * release all used memory here.
    */
   if( hb_vmRequestQuery() == HB_QUIT_REQUESTED )
      hb_macroDelete( ( HB_MACRO_PTR ) ErrorInfo->Cargo );

   return pResult;
}

/* It handles an error generated during checking of expression type
 */
static HB_ERROR_HANDLE( hb_macroErrorType )
{
   HB_MACRO_PTR pMacro = ( HB_MACRO_PTR ) ErrorInfo->Cargo;

   /* copy error object for later diagnostic usage */   
   pMacro->pError = hb_itemNew( ErrorInfo->Error );
   pMacro->status &= ~HB_MACRO_CONT;
   /* ignore rest of compiled code
    */
   hb_vmRequestEndProc();
   return NULL;      /* ignore this error */
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

/* evaluate a macro-cmpiled code and discard it
 */
static void hb_macroEvaluate( HB_MACRO_PTR pMacro )
{
   HB_ERROR_INFO struErr;
   HB_ERROR_INFO_PTR pOld;

   struErr.Func  = hb_macroErrorEvaluation;
   struErr.Cargo = ( void * ) pMacro;
   pOld = hb_errorHandler( &struErr );
   hb_macroRun( pMacro );
   hb_errorHandler( pOld );
   hb_macroDelete( pMacro );
}


static void hb_macroSyntaxError( HB_MACRO_PTR pMacro )
{
   HB_ITEM_PTR pResult;
   HB_ITEM_PTR pError = NULL;
   
   HB_TRACE(HB_TR_DEBUG, ("hb_macroSyntaxError(%p)", pMacro));

   if( pMacro )
   {
      HB_TRACE(HB_TR_DEBUG, ("hb_macroSyntaxError.(%s)", pMacro->string));

      pError = pMacro->pError;
      hb_macroDelete( pMacro );
   }

   if( pError )
   {
      hb_errLaunch( pError );
      hb_errRelease( pError );
   }
   else
   {
      pResult = hb_errRT_BASE_Subst( EG_SYNTAX, 1449, NULL, "&", 0 );

      if( pResult )
      {
         hb_vmPush( pResult );
         hb_itemRelease( pResult );
      }
   }
}

/* Check if passed string is a valid function or variable name
 */
BOOL hb_macroIsIdent( char * szString )
{
   char * pTmp = szString;
   BOOL bIsIdent = FALSE;

   /* NOTE: This uses _a-zA-Z0-9 pattern to check for a valid name
    */
   if( *pTmp )
   {
      if( ! ( pTmp[ 0 ] == '_' && pTmp[ 1 ] == 0 ) )
      {
         /* this is not a "_" string
          */
         if( *pTmp == '_' || (*pTmp >= 'A' && *pTmp <= 'Z') || (*pTmp >= 'a' && *pTmp <= 'z') )
         {
            ++pTmp;
            while( *pTmp && (*pTmp == '_' || (*pTmp >= 'A' && *pTmp <= 'Z') || (*pTmp >= 'a' && *pTmp <= 'z') || (*pTmp >= '0' && *pTmp <= '9')) )
               ++pTmp;
            /* the name is valid if pTmp is at the end of a string
            */
            bIsIdent = (*pTmp ? FALSE : TRUE );
         }
      }
   }

   return bIsIdent;
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
char * hb_macroTextSubst( char * szString, ULONG *pulStringLen )
{
   char * szResult;
   ULONG ulResStrLen;
   ULONG ulResBufLen;
   ULONG ulCharsLeft;
   char * pHead;
   char * pTail;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroTextSubst(%s, %li)", szString, *pulStringLen));

   pHead = (char *)memchr( (void *)szString, '&', *pulStringLen );
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
      if( *pHead == '_' || (*pHead >= 'A' && *pHead <= 'Z') || (*pHead >= 'a' && *pHead <= 'z') )
      {
         /* extract a variable name */
         /* NOTE: the extracted name can be longer then supported maximal
          * length of identifiers (HB_SYMBOL_NAME_LEN) - only the max allowed
          * are used for name lookup however the whole string is replaced
          */
         ULONG ulNameLen = 0;
         char * pName = pHead;

         while( *pHead && (*pHead == '_' || (*pHead >= 'A' && *pHead <= 'Z') || (*pHead >= 'a' && *pHead <= 'z') || (*pHead >= '0' && *pHead <= '9')) )
         {
            ++pHead;
            ++ulNameLen;
         }
         /* pHead points now at the character that terminated a variable name */

         /* NOTE: '_' is invalid variable name
          */
         if( ! ( *pName == '_' && ulNameLen == 1 ) )
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
 *  = HB_P_MACROPUSHARG - in xbase compatibility mode
 *  = HB_P_MACROPUSHLIST
 *  = HB_P_MACROPUSHINDEX
 *  = HB_P_MACROPUSHPARE
 *
 * iContext contains HB_P_MACROPUSHPARE if a macro is used inside a codeblock
 * EVAL( {|| &macro} )
 *
 */
   /* TODO: remove these externals */
   extern int hb_vm_aiExtraParams[HB_MAX_MACRO_ARGS], hb_vm_iExtraParamsIndex;
   extern int hb_vm_aiExtraElements[HB_MAX_MACRO_ARGS], hb_vm_iExtraElementsIndex;
   extern int hb_vm_iExtraIndex;

   extern PHB_SYMB hb_vm_apExtraParamsSymbol[HB_MAX_MACRO_ARGS];

void hb_macroGetValue( HB_ITEM_PTR pItem, BYTE iContext, BYTE flags )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_macroGetValue(%p)", pItem));

   if( hb_macroCheckParam( pItem ) )
   {
      HB_MACRO struMacro;
      int iStatus;
      char * szString = pItem->item.asString.value;
#ifdef HB_MACRO_STATEMENTS
      char * pText;
      char * pOut;
#endif
      struMacro.Flags         = HB_MACRO_GEN_PUSH;
      struMacro.uiNameLen     = HB_SYMBOL_NAME_LEN;
      struMacro.status        = HB_MACRO_CONT;
      struMacro.iListElements = 0;
      struMacro.supported     = (flags & HB_SM_RT_MACRO) ? s_macroFlags : flags;

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
         slen = HB_MIN( strlen( szString ), HB_PP_STR_SIZE - 1 );
         memcpy( pText, szString, slen );
         pText[ slen ] = 0;
         memset( pOut, 0, HB_PP_STR_SIZE );

         HB_SKIPTABSPACES( ptr );

         if( !hb_pp_topDefine )
         {
            hb_pp_Table();
         }

         hb_pp_ParseExpression( ptr, pOut );
         szString = pText;
      }
#endif

      iStatus = hb_macroParse( &struMacro, szString );

      if( iContext && ( ( hb_vm_iExtraParamsIndex == HB_MAX_MACRO_ARGS ) || ( hb_vm_iExtraElementsIndex >= HB_MAX_MACRO_ARGS ) ) )
      {
         hb_macroSyntaxError( &struMacro );
      }

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
         hb_macroEvaluate( &struMacro );

         if( iContext && struMacro.iListElements > 0 )
         {
            if( iContext == HB_P_MACROPUSHARG )
            {
               hb_vm_aiExtraParams[hb_vm_iExtraParamsIndex] = struMacro.iListElements;
               hb_vm_apExtraParamsSymbol[hb_vm_iExtraParamsIndex++] = NULL;
            }
            else if( iContext == HB_P_MACROPUSHLIST )
            {
               hb_vm_aiExtraElements[hb_vm_iExtraElementsIndex - 1] += struMacro.iListElements;
            }
            else if( iContext == HB_P_MACROPUSHINDEX )
            {
               hb_vm_iExtraIndex = struMacro.iListElements;
            }
         }
      }
      else
         hb_macroSyntaxError( &struMacro );
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
      char * szString = pItem->item.asString.value;
      HB_MACRO struMacro;
      int iStatus;

      struMacro.Flags      = HB_MACRO_GEN_POP;
      struMacro.uiNameLen  = HB_SYMBOL_NAME_LEN;
      struMacro.status     = HB_MACRO_CONT;
      struMacro.supported  = (flags & HB_SM_RT_MACRO) ? s_macroFlags : flags;
      iStatus = hb_macroParse( &struMacro, szString );

      hb_stackPop();    /* remove compiled string */
      if( iStatus == HB_MACRO_OK && ( struMacro.status & HB_MACRO_CONT ) )
      {
         hb_macroEvaluate( &struMacro );
      }
      else
         hb_macroSyntaxError( &struMacro );
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
      char * szString = ( char * ) hb_xgrab( pAlias->item.asString.length + pVar->item.asString.length + 3 );
      HB_MACRO struMacro;
      int iStatus;

      memcpy( szString, pAlias->item.asString.value, pAlias->item.asString.length );
      szString[ pAlias->item.asString.length ]     = '-';
      szString[ pAlias->item.asString.length + 1 ] = '>';
      memcpy( szString + pAlias->item.asString.length + 2, pVar->item.asString.value, pVar->item.asString.length );
      szString[ pAlias->item.asString.length + 2 + pVar->item.asString.length ] = '\0';

      struMacro.Flags      = iFlag;
      struMacro.uiNameLen  = HB_SYMBOL_NAME_LEN;
      struMacro.status     = HB_MACRO_CONT;
      struMacro.supported  = (bSupported & HB_SM_RT_MACRO) ? s_macroFlags : bSupported;
      iStatus = hb_macroParse( &struMacro, szString );
      hb_xfree( szString );
      struMacro.string = NULL;

      hb_stackPop();    /* remove compiled variable name */
      hb_stackPop();    /* remove compiled alias */

      if( iStatus == HB_MACRO_OK && ( struMacro.status & HB_MACRO_CONT ) )
      {
         hb_macroEvaluate( &struMacro );
      }
      else
         hb_macroSyntaxError( &struMacro );
   }
   else if( hb_macroCheckParam( pVar ) )
   {
      /* only right side of alias operator is a string - macro-compile
       * this part only
       */
      HB_MACRO struMacro;
      int iStatus;
      char * szString = pVar->item.asString.value;

      struMacro.Flags      = iFlag | HB_MACRO_GEN_ALIASED;
      struMacro.uiNameLen  = HB_SYMBOL_NAME_LEN;
      struMacro.status     = HB_MACRO_CONT;
      struMacro.supported  = (bSupported & HB_SM_RT_MACRO) ? s_macroFlags : bSupported;
      iStatus = hb_macroParse( &struMacro, szString );

      hb_stackPop();    /* remove compiled string */

      if( iStatus == HB_MACRO_OK && ( struMacro.status & HB_MACRO_CONT ) )
      {
         hb_macroEvaluate( &struMacro );
      }
      else
         hb_macroSyntaxError( &struMacro );
   }
}

/* Check for '&' operator and replace it with a macro variable value
 * Returns: the passed string if there is no '&' operator (pbNewString:=FALSE)
 * new string if a valid macro text substitution was found (and sets
 * pbNewString to TRUE)
*/
char * hb_macroExpandString( char *szString, ULONG ulLength, BOOL *pbNewString )
{
   char *szResultString;
   HB_TRACE(HB_TR_DEBUG, ("hb_macroExpandString(%s)", szString));

   if( szString )
      szResultString = hb_macroTextSubst( szString, &ulLength );
   else
      szResultString = szString;
   *pbNewString = ( szString != szResultString );
   return szResultString;
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
   pMacro->Flags = HB_MACRO_DEALLOCATE | HB_MACRO_GEN_PUSH;
   pMacro->uiNameLen  = HB_SYMBOL_NAME_LEN;
   pMacro->status     = HB_MACRO_CONT;
   pMacro->supported  = s_macroFlags;

   iStatus = hb_macroParse( pMacro, szString );
   if( ! ( iStatus == HB_MACRO_OK && ( pMacro->status & HB_MACRO_CONT ) ) )
   {
      hb_macroDelete( pMacro );
      hb_xfree( pMacro );
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
      BOOL bNewBuffer;
      ULONG ulLength = pItem->item.asString.length;

      szString = hb_macroTextSubst( pItem->item.asString.value, &ulLength );
      bNewBuffer = ( szString != pItem->item.asString.value );

      if( hb_macroIsIdent( szString ) )
      {
         HB_DYNS_PTR pDynSym =  hb_dynsymGet( szString );

         hb_stackPop();    /* remove compiled string */
         /* NOTE: checking for valid function name (valid pointer) is done
          * in hb_vmDo()
          */
         hb_vmPushMacroSymbol( pDynSym->pSymbol );  /* push compiled symbol instead of a string */

         if( bNewBuffer )
            hb_xfree( szString );   /* free space allocated in hb_macroTextSubst */
      }
      else
      {
         hb_stackPop();    /* remove compiled string */
         if( bNewBuffer )
            hb_xfree( szString );   /* free space allocated in hb_macroTextSubst */
         hb_macroSyntaxError( NULL );
      }
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
      char * szString = pItem->item.asString.value;

      struMacro.Flags      = HB_MACRO_GEN_PUSH | HB_MACRO_GEN_TYPE;
      struMacro.uiNameLen  = HB_SYMBOL_NAME_LEN;
      struMacro.status     = HB_MACRO_CONT;
      struMacro.supported  = s_macroFlags;
      iStatus = hb_macroParse( &struMacro, szString );

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
                  if( ulGenCode == EG_NOVAR )
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

      if( struMacro.pError )
         hb_itemRelease( struMacro.pError );
      struMacro.pError = NULL;
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
             {
                BOOL bSet = hb_itemGetL( pValue );
                hb_macroSetMacro( bSet && hb_vmFlagEnabled(HB_VMFLAG_ARRSTR), ulFlags );
             }
             break;

          case HB_SM_SHORTCUTS:
             /* enable/disable support for shortcut logical operators */
             hb_retl( s_macroFlags & ulFlags );
             pValue = hb_param( 2, HB_IT_LOGICAL );
             if( pValue )
             {
                hb_macroSetMacro( hb_itemGetL( pValue ), ulFlags );
                hb_comp_bShortCuts = s_macroFlags & ulFlags;
             }
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
int hb_compLocalVarGetPos( char * szVarName, HB_MACRO_DECL )
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


ULONG hb_compGenJump( LONG lOffset, HB_MACRO_DECL )
{
   /* TODO: We need a longer offset (longer then two bytes)
    */
   if ( ! HB_LIM_INT16( lOffset ) )
      hb_macroError( HB_MACRO_TOO_COMPLEX, HB_MACRO_PARAM );

   hb_compGenPCode3( HB_P_JUMP, HB_LOBYTE( lOffset ), HB_HIBYTE( lOffset ), HB_MACRO_PARAM );

   return HB_PCODE_DATA->lPCodePos - 2;
}

ULONG hb_compGenJumpFalse( LONG lOffset, HB_MACRO_DECL )
{
   /* TODO: We need a longer offset (longer then two bytes)
    */
   if ( ! HB_LIM_INT16( lOffset ) )
      hb_macroError( HB_MACRO_TOO_COMPLEX, HB_MACRO_PARAM );

   hb_compGenPCode3( HB_P_JUMPFALSE, HB_LOBYTE( lOffset ), HB_HIBYTE( lOffset ), HB_MACRO_PARAM );

   return HB_PCODE_DATA->lPCodePos - 2;
}

void hb_compGenJumpThere( ULONG ulFrom, ULONG ulTo, HB_MACRO_DECL )
{
   BYTE * pCode = HB_PCODE_DATA->pCode;
   LONG lOffset = ulTo - ulFrom + 1;

   /* TODO: We need a longer offset (longer then two bytes)
    */
   if ( ! HB_LIM_INT16( lOffset ) )
      hb_macroError( HB_MACRO_TOO_COMPLEX, HB_MACRO_PARAM );

   pCode[ ( ULONG ) ulFrom ]     = HB_LOBYTE( lOffset );
   pCode[ ( ULONG ) ulFrom + 1 ] = HB_HIBYTE( lOffset );
}

void hb_compGenJumpHere( ULONG ulOffset, HB_MACRO_DECL )
{
   hb_compGenJumpThere( ulOffset, HB_PCODE_DATA->lPCodePos, HB_MACRO_PARAM );
}

ULONG hb_compGenJumpTrue( LONG lOffset, HB_MACRO_DECL )
{
   /* TODO: We need a longer offset (longer then two bytes)
    */
   if ( ! HB_LIM_INT16( lOffset ) )
      hb_macroError( HB_MACRO_TOO_COMPLEX, HB_MACRO_PARAM );

   hb_compGenPCode3( HB_P_JUMPTRUE, HB_LOBYTE( lOffset ), HB_HIBYTE( lOffset ), HB_MACRO_PARAM );

   return HB_PCODE_DATA->lPCodePos - 2;
}

/* Checks if there is a visible memvar variable
 * szVarName = variable name
*/
static void hb_compMemvarCheck( char * szVarName, HB_MACRO_DECL )
{
   if( HB_MACRO_DATA->Flags & HB_MACRO_GEN_TYPE )
   {
      /* Test if variable exist if called from TYPE() function only */
      if( !( HB_MACRO_DATA->status & (HB_MACRO_UNKN_VAR | HB_MACRO_UNKN_SYM) ) )
      {
         /* checking for variable is quite expensive than don't check it
          * if there are already some undefined symbols or variables
         */
         if( hb_memvarScope( szVarName, strlen( szVarName ) + 1 ) <= HB_MV_ERROR )
         {
            if( ! hb_dynsymFind( szVarName ) )
            {
               /* there is no memvar or field variable visible at this moment */
                HB_MACRO_DATA->status |= HB_MACRO_UNKN_VAR;
                HB_MACRO_DATA->status &= ~HB_MACRO_CONT;  /* don't run this pcode */
            }
         }
      }
   }
}

/*
 * Function generates pcode for passed memvar name
 */
void hb_compMemvarGenPCode( BYTE bPCode, char * szVarName, HB_MACRO_DECL )
{
   HB_DYNS_PTR pSym;

   if( HB_MACRO_DATA->Flags & HB_MACRO_GEN_TYPE )
   {
      /* we are determining the type of expression (called from TYPE() function)
       * then we shouldn't create the requested variable if it doesn't exist
       */
      pSym = hb_dynsymFind( szVarName );
      if( ! pSym )
         HB_MACRO_DATA->status |= HB_MACRO_UNKN_VAR;
   }
   /* Find the address of passed symbol - 
    * create the symbol if doesn't exist (Clipper compatibility)
   */
   pSym = hb_dynsymGet( szVarName );
   hb_compGenPCode1( bPCode, HB_MACRO_PARAM );
   {
      BYTE byBuf[ sizeof( HB_DYNS_PTR ) ];

      HB_PUT_PTR( byBuf, pSym );
      hb_compGenPCodeN( byBuf, sizeof( pSym ), HB_MACRO_PARAM );
   }
   /* hb_compGenPCodeN( ( BYTE * )( &pSym ), sizeof( pSym ), HB_MACRO_PARAM ); */
}

/* generates the pcode to push a symbol on the virtual machine stack */
void hb_compGenPushSymbol( char * szSymbolName, BOOL bFunction, BOOL bAlias, HB_MACRO_DECL )
{
   HB_DYNS_PTR pSym;

   HB_SYMBOL_UNUSED( bAlias );

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
         if( pSym && hb_dynsymScope(pSym, HB_FS_STATIC) )
         {
            /* static functions are not allowed in macro */
            HB_MACRO_DATA->status |= HB_MACRO_UNKN_SYM;
            HB_MACRO_DATA->status &= ~HB_MACRO_CONT;  /* don't run this pcode */
         }
      }
   }
   else
      pSym = hb_dynsymGet( szSymbolName );     

   hb_compGenPCode1( HB_P_MPUSHSYM, HB_MACRO_PARAM );
   {
      BYTE byBuf[ sizeof( HB_DYNS_PTR ) ];

      HB_PUT_PTR( byBuf, pSym );
      hb_compGenPCodeN( byBuf, sizeof( pSym ), HB_MACRO_PARAM );
   }
   /* hb_compGenPCodeN( ( BYTE * ) &pSym, sizeof( pSym ), HB_MACRO_PARAM ); */
}

/* generates the pcode to push a long number on the virtual machine stack */
void hb_compGenPushLong( HB_LONG lNumber, HB_MACRO_DECL )
{
   if( lNumber == 0 )
   {
      hb_compGenPCode1( HB_P_ZERO, HB_MACRO_PARAM );
   }
   else if( lNumber == 1 )
   {
      hb_compGenPCode1( HB_P_ONE, HB_MACRO_PARAM );
   }
   else if( HB_LIM_INT8( lNumber ) )
   {
      hb_compGenPCode2( HB_P_PUSHBYTE, (BYTE) lNumber, HB_MACRO_PARAM );
   }
   else if( HB_LIM_INT16( lNumber ) )
   {
      hb_compGenPCode3( HB_P_PUSHINT, HB_LOBYTE( lNumber ), HB_HIBYTE( lNumber ), HB_MACRO_PARAM );
   }
   else if( HB_LIM_INT32( lNumber ) )
   {
      BYTE pBuffer[ 5 ];
      pBuffer[ 0 ] = HB_P_PUSHLONG;
      HB_PUT_LE_UINT32( pBuffer + 1, lNumber );
      hb_compGenPCodeN( pBuffer, sizeof( pBuffer ), HB_MACRO_PARAM );
   }
   else
   {
      BYTE pBuffer[ 9 ];
      pBuffer[ 0 ] = HB_P_PUSHLONGLONG;
      HB_PUT_LE_UINT64( pBuffer + 1, lNumber );
      hb_compGenPCodeN( pBuffer, sizeof( pBuffer ), HB_MACRO_PARAM );
   }
}


/* sends a message to an object */
void hb_compGenMessage( char * szMsgName, HB_MACRO_DECL )
{
   /* Find the address of passed symbol - create the symbol if doesn't exist
    */
   HB_DYNS_PTR pSym = hb_dynsymGet( szMsgName );

   hb_compGenPCode1( HB_P_MMESSAGE, HB_MACRO_PARAM );
   {
      BYTE byBuf[ sizeof( HB_DYNS_PTR ) ];

      HB_PUT_PTR( byBuf, pSym );
      hb_compGenPCodeN( byBuf, sizeof( pSym ), HB_MACRO_PARAM );
   }
   /* hb_compGenPCodeN( ( BYTE * ) &pSym, sizeof( pSym ), HB_MACRO_PARAM ); */
}

/* generates an underscore-symbol name for a data assignment */
void hb_compGenMessageData( char * szMsg, HB_MACRO_DECL )
{
   char * szResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_compGenMessageData(%s)", szMsg));

   szResult = ( char * ) hb_xgrab( strlen( szMsg ) + 2 );

   strcpy( szResult, "_" );
   strcat( szResult, szMsg );

   hb_compGenMessage( szResult, HB_MACRO_PARAM );
   hb_xfree( szResult );
}

/* generates the pcode to pop a value from the virtual machine stack onto a variable */
void hb_compGenPopVar( char * szVarName, HB_MACRO_DECL )
{
   int iVar;

   iVar = hb_compLocalVarGetPos( szVarName, HB_MACRO_PARAM );
   if( iVar )
   {
      /* this is a codeblock parameter */
      hb_compGenPCode3( HB_P_POPLOCAL, HB_LOBYTE( iVar ), HB_HIBYTE( iVar ), HB_MACRO_PARAM );
   }
   else
   {
      hb_compMemvarGenPCode( HB_P_MPOPMEMVAR, szVarName, HB_MACRO_PARAM );
      hb_compMemvarCheck( szVarName, HB_MACRO_PARAM );
   }
}

/* generates the pcode to pop a value from the virtual machine stack onto
 * an aliased variable
 */
void hb_compGenPopAliasedVar( char * szVarName,
                              BOOL bPushAliasValue,
                              char * szAlias,
                              long lWorkarea, HB_MACRO_DECL )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_compGenPopAliasedVar(%s->%s)",szAlias,szVarName));

   if( bPushAliasValue )
   {
      if( szAlias )
      {
         if( szAlias[ 0 ] == 'M' && szAlias[ 1 ] == '\0' )
         {  /* M->variable */
            hb_compMemvarGenPCode( HB_P_MPOPMEMVAR, szVarName, HB_MACRO_PARAM );
            hb_compMemvarCheck( szVarName, HB_MACRO_PARAM );
         }
         else
         {
            int iCmp = strncmp( szAlias, "MEMVAR", 4 );
            if( iCmp == 0 )
                  iCmp = strncmp( szAlias, "MEMVAR", strlen( szAlias ) );
            if( iCmp == 0 )
            {  /* MEMVAR-> or MEMVA-> or MEMV-> */
               hb_compMemvarGenPCode( HB_P_MPOPMEMVAR, szVarName, HB_MACRO_PARAM );
               hb_compMemvarCheck( szVarName, HB_MACRO_PARAM );
            }
            else
            {  /* field variable */
               iCmp = strncmp( szAlias, "FIELD", 4 );
               if( iCmp == 0 )
                  iCmp = strncmp( szAlias, "FIELD", strlen( szAlias ) );
               if( iCmp == 0 )
               {  /* FIELD-> */
                  hb_compMemvarGenPCode( HB_P_MPOPFIELD, szVarName, HB_MACRO_PARAM );
               }
               else
               {  /* database alias */
                  hb_compGenPushSymbol( szAlias, FALSE, TRUE, HB_MACRO_PARAM );
                  hb_compMemvarGenPCode( HB_P_MPOPALIASEDFIELD, szVarName, HB_MACRO_PARAM );
               }
            }
         }
      }
      else
      {
         hb_compGenPushLong( lWorkarea, HB_MACRO_PARAM );
         hb_compMemvarGenPCode( HB_P_MPOPALIASEDFIELD, szVarName, HB_MACRO_PARAM );
      }
   }
   else
   {
      /* Alias is already placed on stack
       * NOTE: An alias will be determined at runtime then we cannot decide
       * here if passed name is either a field or a memvar
       */
      hb_compMemvarGenPCode( HB_P_MPOPALIASEDVAR, szVarName, HB_MACRO_PARAM );
      hb_compMemvarCheck( szVarName, HB_MACRO_PARAM );
   }
}

/* generates the pcode to push a nonaliased variable value to the virtual
 * machine stack
 */
void hb_compGenPushVar( char * szVarName, HB_MACRO_DECL )
{
   int iVar;

   iVar = hb_compLocalVarGetPos( szVarName, HB_MACRO_PARAM );
   if( iVar )
   {
      /* this is a codeblock parameter */
      hb_compGenPCode3( HB_P_PUSHLOCAL, HB_LOBYTE( iVar ), HB_HIBYTE( iVar ), HB_MACRO_PARAM );
   }
   else
   {
      hb_compMemvarGenPCode( HB_P_MPUSHVARIABLE, szVarName, HB_MACRO_PARAM );
      hb_compMemvarCheck( szVarName, HB_MACRO_PARAM );
   }
}

/* generates the pcode to push a variable by reference to the virtual machine stack */
void hb_compGenPushVarRef( char * szVarName, HB_MACRO_DECL )
{
   USHORT iVar;

   iVar = hb_compLocalVarGetPos( szVarName, HB_MACRO_PARAM );
   if( iVar )
      hb_compGenPCode3( HB_P_PUSHLOCALREF, HB_LOBYTE( iVar ), HB_HIBYTE( iVar ), HB_MACRO_PARAM );
   else
   {
      hb_compMemvarGenPCode( HB_P_MPUSHMEMVARREF, szVarName, HB_MACRO_PARAM );
      hb_compMemvarCheck( szVarName, HB_MACRO_PARAM );
   }
}

 /* generates the pcode to push an aliased variable value to the virtual
  * machine stack
  */
void hb_compGenPushAliasedVar( char * szVarName,
                               BOOL bPushAliasValue,
                               char * szAlias,
                               long lWorkarea, HB_MACRO_DECL )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_compGenPushAliasedVar(%s->%s)",szAlias,szVarName));

   if( bPushAliasValue )
   {
      if( szAlias )
      {
         /* myalias->var
         * FIELD->var
         * MEMVAR->var
         */
         if( szAlias[ 0 ] == 'M' && szAlias[ 1 ] == '\0' )
         {  /* M->variable */
            hb_compMemvarGenPCode( HB_P_MPUSHMEMVAR, szVarName, HB_MACRO_PARAM );
            hb_compMemvarCheck( szVarName, HB_MACRO_PARAM );
         }
         else
         {
            int iCmp = strncmp( szAlias, "MEMVAR", 4 );
            if( iCmp == 0 )
                  iCmp = strncmp( szAlias, "MEMVAR", strlen( szAlias ) );
            if( iCmp == 0 )
            {  /* MEMVAR-> or MEMVA-> or MEMV-> */
               hb_compMemvarGenPCode( HB_P_MPUSHMEMVAR, szVarName, HB_MACRO_PARAM );
               hb_compMemvarCheck( szVarName, HB_MACRO_PARAM );
            }
            else
            {  /* field variable */
               iCmp = strncmp( szAlias, "FIELD", 4 );
               if( iCmp == 0 )
                  iCmp = strncmp( szAlias, "FIELD", strlen( szAlias ) );
               if( iCmp == 0 )
               {  /* FIELD-> */
                  hb_compMemvarGenPCode( HB_P_MPUSHFIELD, szVarName, HB_MACRO_PARAM );
               }
               else
               {  /* database alias */
                  hb_compGenPushSymbol( szAlias, FALSE, TRUE, HB_MACRO_PARAM );
                  hb_compMemvarGenPCode( HB_P_MPUSHALIASEDFIELD, szVarName, HB_MACRO_PARAM );
               }
            }
         }
      }
      else
      {
         hb_compGenPushLong( lWorkarea, HB_MACRO_PARAM );
         hb_compMemvarGenPCode( HB_P_MPUSHALIASEDFIELD, szVarName, HB_MACRO_PARAM );
      }
   }
   else
   {
      /* Alias is already placed on stack
       * NOTE: An alias will be determined at runtime then we cannot decide
       * here if passed name is either a field or a memvar
       */
      hb_compMemvarGenPCode( HB_P_MPUSHALIASEDVAR, szVarName, HB_MACRO_PARAM );
      hb_compMemvarCheck( szVarName, HB_MACRO_PARAM );
   }
}

/* pushes a logical value on the virtual machine stack , */
void hb_compGenPushLogical( int iTrueFalse, HB_MACRO_DECL )
{
   if( iTrueFalse )
      hb_compGenPCode1( HB_P_TRUE, HB_MACRO_PARAM );
   else
      hb_compGenPCode1( HB_P_FALSE, HB_MACRO_PARAM );
}

/* generates the pcode to push a double number on the virtual machine stack */
void hb_compGenPushDouble( double dNumber, BYTE bWidth, BYTE bDec, HB_MACRO_DECL )
{
   BYTE pBuffer[ sizeof( double ) + sizeof( BYTE ) + sizeof( BYTE ) + 1 ];

   pBuffer[ 0 ] = HB_P_PUSHDOUBLE;
   HB_PUT_LE_DOUBLE( &( pBuffer[ 1 ] ), dNumber );
   pBuffer[ 1 + sizeof( double ) ] = bWidth;
   pBuffer[ 1 + sizeof( double ) + sizeof( BYTE ) ] = bDec;

   hb_compGenPCodeN( pBuffer, 1 + sizeof( double ) + sizeof( BYTE ) + sizeof( BYTE ), HB_MACRO_PARAM );
}

void hb_compGenPushFunCall( char * szFunName, HB_MACRO_DECL )
{
   char * szFunction;

   szFunction = hb_compReservedName( szFunName );
   if( szFunction )
   {
      /* Abbreviated function name was used - change it for whole name
       */
      hb_compGenPushSymbol( szFunction, TRUE, FALSE, HB_MACRO_PARAM );
   }
   else
   {
      HB_MACRO_DATA->status |= HB_MACRO_UDF; /* this is used in hb_macroGetType */
      hb_compGenPushSymbol( szFunName, TRUE, FALSE, HB_MACRO_PARAM );
   }
}

/* generates the pcode to push a string on the virtual machine stack */
void hb_compGenPushString( char * szText, ULONG ulStrLen, HB_MACRO_DECL )
{
   hb_compGenPCode3( HB_P_MPUSHSTR, HB_LOBYTE( ulStrLen ), HB_HIBYTE( ulStrLen ), HB_MACRO_PARAM );
   hb_compGenPCodeN( ( BYTE * ) szText, ulStrLen, HB_MACRO_PARAM );
}


void hb_compGenPCode1( BYTE byte, HB_MACRO_DECL )
{
   HB_PCODE_INFO_PTR pFunc = HB_PCODE_DATA;

   if( ( pFunc->lPCodeSize - pFunc->lPCodePos ) < 1 )
      pFunc->pCode = ( BYTE * ) hb_xrealloc( pFunc->pCode, pFunc->lPCodeSize += HB_PCODE_SIZE );

   pFunc->pCode[ pFunc->lPCodePos++ ] = byte;
}

void hb_compGenPCode2( BYTE byte1, BYTE byte2, HB_MACRO_DECL )
{
   HB_PCODE_INFO_PTR pFunc = HB_PCODE_DATA;

   if( ( pFunc->lPCodeSize - pFunc->lPCodePos ) < 2 )
      pFunc->pCode = ( BYTE * ) hb_xrealloc( pFunc->pCode, pFunc->lPCodeSize += HB_PCODE_SIZE );

   pFunc->pCode[ pFunc->lPCodePos++ ] = byte1;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte2;
}

void hb_compGenPCode3( BYTE byte1, BYTE byte2, BYTE byte3, HB_MACRO_DECL )
{
   HB_PCODE_INFO_PTR pFunc = HB_PCODE_DATA;

   if( ( pFunc->lPCodeSize - pFunc->lPCodePos ) < 3 )
      pFunc->pCode = ( BYTE * ) hb_xrealloc( pFunc->pCode, pFunc->lPCodeSize += HB_PCODE_SIZE );

   pFunc->pCode[ pFunc->lPCodePos++ ] = byte1;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte2;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte3;
}

void hb_compGenPCode4( BYTE byte1, BYTE byte2, BYTE byte3, BYTE byte4, HB_MACRO_DECL )
{
   HB_PCODE_INFO_PTR pFunc = HB_PCODE_DATA;

   if( ( pFunc->lPCodeSize - pFunc->lPCodePos ) < 4 )
      pFunc->pCode = ( BYTE * ) hb_xrealloc( pFunc->pCode, pFunc->lPCodeSize += HB_PCODE_SIZE );

   pFunc->pCode[ pFunc->lPCodePos++ ] = byte1;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte2;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte3;
   pFunc->pCode[ pFunc->lPCodePos++ ] = byte4;
}

void hb_compGenPCodeN( BYTE * pBuffer, ULONG ulSize, HB_MACRO_DECL )
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

void hb_macroError( int iError, HB_MACRO_DECL )
{
   HB_MACRO_DATA->status |= iError;
   HB_MACRO_DATA->status &= ~HB_MACRO_CONT;  /* clear CONT bit */
}

/*
 * Start a new pcode buffer for a codeblock
*/
void hb_compCodeBlockStart( HB_MACRO_DECL )
{
   HB_PCODE_INFO_PTR pCB;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroCodeBlockStart(%p)", HB_MACRO_PARAM));

   pCB = ( HB_PCODE_INFO_PTR ) hb_xgrab( sizeof( HB_PCODE_INFO ) );

   /* replace current pcode buffer with the new one
    */
   pCB->pPrev = HB_PCODE_DATA;
   HB_PCODE_DATA = pCB;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroCodeBlockStart.(%p)", HB_MACRO_PARAM));
   pCB->pCode = ( BYTE * ) hb_xgrab( HB_PCODE_SIZE );
   pCB->lPCodeSize = HB_PCODE_SIZE;
   pCB->lPCodePos  = 0;
   pCB->pLocals    = NULL;
}

void hb_compCodeBlockEnd( HB_MACRO_DECL )
{
   HB_PCODE_INFO_PTR pCodeblock;   /* pointer to the current codeblock */
   USHORT wSize;
   USHORT wParms = 0;   /* number of codeblock parameters */
   HB_CBVAR_PTR pVar;

   HB_TRACE(HB_TR_DEBUG, ("hb_macroCodeBlockEnd(%p)", HB_MACRO_PARAM));

   /* a currently processed codeblock */
   pCodeblock = HB_PCODE_DATA;

   /* return to pcode buffer of a codeblock in which the current
    * codeblock was defined
    */
   HB_PCODE_DATA = pCodeblock->pPrev;

   /* generate a proper codeblock frame with a codeblock size and with
    * a number of expected parameters
    */
   /*QUESTION: would be 64kB enough for a codeblock size?
    * we are assuming now a USHORT for a size of codeblock
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
   wSize = ( USHORT ) pCodeblock->lPCodePos + 6;

   /*NOTE: HB_P_MPUSHBLOCK differs from HB_P_PUSHBLOCK - the pcode
    * is stored in dynamic memory pool instead of static memory
    */
   hb_compGenPCode3( HB_P_MPUSHBLOCK, HB_LOBYTE( wSize ), HB_HIBYTE( wSize ), HB_MACRO_PARAM );
   hb_compGenPCode1( HB_LOBYTE( wParms ), HB_MACRO_PARAM );
   hb_compGenPCode1( HB_HIBYTE( wParms ), HB_MACRO_PARAM );

   /* copy a codeblock pcode buffer */
   hb_compGenPCodeN( pCodeblock->pCode, pCodeblock->lPCodePos, HB_MACRO_PARAM );
   hb_compGenPCode1( HB_P_ENDBLOCK, HB_MACRO_PARAM ); /* finish the codeblock */

   /* free memory allocated for a codeblock */
   hb_xfree( ( void * ) pCodeblock->pCode );
   hb_xfree( ( void * ) pCodeblock );
}
