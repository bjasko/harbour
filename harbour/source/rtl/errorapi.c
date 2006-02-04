/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The Error API
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
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
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    DOSERROR()
 *    __ERRINHANDLER()
 *    __ERRRT_BASE()
 *    __ERRRT_SBASE()
 *    hb_errLaunch()
 *    hb_errLaunchSubst()
 *    hb_errGetFlags()
 *    hb_errPutFlags()
 *    hb_errRT_New()
 *    hb_errRT_New_Subst()
 *    hb_errRT_BASE()
 *    hb_errRT_BASE_Ext1()
 *    hb_errRT_BASE_Subst()
 *
 * See doc/license.txt for licensing terms.
 *
 */

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbapilng.h"
#include "hbvm.h"
#include "hbstack.h"

/* This is added to be able to detect a recursive error, and not let Harbour
   go into an infinite loop, this is an emulated version of the Clipper
   "Unrecoverable error 650: Processor stack fault" internal error, but
   better shows what is really the problem. [vszakats] */
#define HB_ERROR_LAUNCH_MAX 8

static HB_ERROR_INFO_PTR s_errorHandler = NULL;
static HB_ITEM_PTR s_errorBlock;
static int     s_iLaunchCount = 0;
static USHORT  s_uiErrorDOS = 0; /* The value of DOSERROR() */

HB_FUNC_EXTERN( ERRORNEW );

/* NOTE: This is called via its symbol name, so we should make sure
         that it gets linked. WARNING ! DON'T make this function static.
         [vszakats] */
void hb_errForceLink()
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errForceLink()"));

   HB_FUNCNAME( ERRORNEW )();
}

/* There's a similar undocumented, internal functions in CA-Cl*pper named
   ErrorInHandler(). [vszakats] */

HB_FUNC( __ERRINHANDLER )
{
   hb_errInternal( HB_EI_ERRRECFAILURE, NULL, NULL, NULL );
}

HB_FUNC( ERRORBLOCK )
{
   PHB_ITEM pNewErrorBlock = hb_param( 1, HB_IT_BLOCK );

   hb_itemReturn( s_errorBlock );
   if( pNewErrorBlock )
   {
      hb_itemCopy( s_errorBlock, pNewErrorBlock );
   }
}

/* set new low-level error launcher (C function) and return
 * handler currently active
 */
HB_ERROR_INFO_PTR hb_errorHandler( HB_ERROR_INFO_PTR pNewHandler )
{
   HB_ERROR_INFO_PTR pOld = s_errorHandler;

   if( pNewHandler )
      pNewHandler->Previous = s_errorHandler;
   s_errorHandler = pNewHandler;

   return pOld;
}

/* TOFIX: Make it Clipper compatible. [vszakats] */

HB_FUNC( DOSERROR )
{
   hb_retni( s_uiErrorDOS );

   if( ISNUM( 1 ) )
      s_uiErrorDOS = ( USHORT ) hb_parni( 1 );
}

void hb_errInit( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errInit()"));

   /* initialize an item
    * NOTE: hb_itemClear() cannot be used to initialize an item because
    * memory occupied by the item can contain garbage bits
   */
   s_errorBlock = hb_itemNew( NULL );
}

void hb_errExit( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errExit()"));

   hb_itemRelease( s_errorBlock );
}

PHB_ITEM hb_errNew( void )
{
   PHB_ITEM pReturn;

   HB_TRACE(HB_TR_DEBUG, ("hb_errNew()"));

   pReturn = hb_itemNew( NULL );

   hb_vmPushSymbol( hb_dynsymGetSymbol( "ERRORNEW" ) );
   hb_vmPushNil();
   hb_vmDo( 0 );

   hb_itemCopy( pReturn, hb_stackReturnItem() );

   return pReturn;
}

USHORT hb_errLaunch( PHB_ITEM pError )
{
   USHORT uiAction = E_DEFAULT; /* Needed to avoid GCC -O2 warning */
   USHORT usRequest;

   HB_TRACE(HB_TR_DEBUG, ("hb_errLaunch(%p)", pError));

   if( pError )
   {
      PHB_ITEM pResult;

      /* Check if we have a valid error handler */

      if( hb_itemType( s_errorBlock ) != HB_IT_BLOCK )
         hb_errInternal( HB_EI_ERRNOBLOCK, NULL, NULL, NULL );

      /* Check if the error launcher was called too many times recursively */

      if( s_iLaunchCount == HB_ERROR_LAUNCH_MAX )
         hb_errInternal( HB_EI_ERRTOOMANY, NULL, NULL, NULL );

      /* Launch the error handler: "lResult := EVAL( ErrorBlock(), oError )" */

      s_iLaunchCount++;

      if( s_errorHandler )
      {
         /* there is a low-level error handler defined - use it instead
          * of normal Harbour-level one
          */
         s_errorHandler->Error = pError;
         s_errorHandler->ErrorBlock = s_errorBlock;
         pResult = (s_errorHandler->Func)( s_errorHandler );
         s_errorHandler->Error = NULL;
      }
      else
         pResult = hb_itemDo( s_errorBlock, 1, pError );

      s_iLaunchCount--;

      /* Check results */

      usRequest = hb_vmRequestQuery();
      if( usRequest == HB_QUIT_REQUESTED )
      {
         if( pResult )
             hb_itemRelease( pResult );
         hb_errRelease( pError );
         hb_vmQuit();
      }
      else if( usRequest == HB_BREAK_REQUESTED || usRequest == HB_ENDPROC_REQUESTED )
      {
         if( pResult )
             hb_itemRelease( pResult );
         uiAction = E_BREAK;
      }
      else if( pResult )
      {
         BOOL bFailure = FALSE;
         USHORT uiFlags = hb_errGetFlags( pError );

         /* If the error block didn't return a logical value, */
         /* or the canSubstitute flag has been set, consider it as a failure */

         if( hb_itemType( pResult ) != HB_IT_LOGICAL || ( uiFlags & EF_CANSUBSTITUTE ) )
            bFailure = TRUE;
         else
         {
            uiAction = hb_itemGetL( pResult ) ? E_RETRY : E_DEFAULT;

            if( ( uiAction == E_DEFAULT && !( uiFlags & EF_CANDEFAULT ) ) ||
                ( uiAction == E_RETRY   && !( uiFlags & EF_CANRETRY   ) ) )
               bFailure = TRUE;
         }

         hb_itemRelease( pResult );

         if( bFailure )
            hb_errInternal( HB_EI_ERRRECFAILURE, NULL, NULL, NULL );

         /* Add one try to the counter. */

         if( uiAction == E_RETRY )
            hb_errPutTries( pError, hb_errGetTries( pError ) + 1 );
      }
      else
         hb_errInternal( HB_EI_ERRRECFAILURE, NULL, NULL, NULL );
   }
   else
      uiAction = E_RETRY; /* Clipper does this, undocumented */

   return uiAction;
}

/* This error launcher should be used in those situations, where the error
   handler is expected to return a value to be substituted as the result of
   a failed operation. [vszakats] */

/* NOTE: This should only be called when the EF_CANSUBSTITUE flag was set
         Since it this case the error handler will return the value
         to be substituted. [vszakats] */

/* NOTE: The item pointer returned should be hb_itemRelease()-d by the
         caller if it was not NULL. [vszakats] */

PHB_ITEM hb_errLaunchSubst( PHB_ITEM pError )
{
   PHB_ITEM pResult;
   USHORT usRequest;

   HB_TRACE(HB_TR_DEBUG, ("hb_errLaunchSubst(%p)", pError));

   if( pError )
   {
      /* Check if we have a valid error handler */

      if( hb_itemType( s_errorBlock ) != HB_IT_BLOCK )
         hb_errInternal( HB_EI_ERRNOBLOCK, NULL, NULL, NULL );

      /* Check if the error launcher was called too many times recursively */

      if( s_iLaunchCount == HB_ERROR_LAUNCH_MAX )
         hb_errInternal( HB_EI_ERRTOOMANY, NULL, NULL, NULL );

      /* Launch the error handler: "xResult := EVAL( ErrorBlock(), oError )" */

      s_iLaunchCount++;

      if( s_errorHandler )
      {
         /* there is a low-level error handler defined - use it instead
          * of normal Harbour-level one
          */
         s_errorHandler->Error = pError;
         s_errorHandler->ErrorBlock = s_errorBlock;
         pResult = ( s_errorHandler->Func )( s_errorHandler );
         s_errorHandler->Error = NULL;
      }
      else
         pResult = hb_itemDo( s_errorBlock, 1, pError );

      s_iLaunchCount--;

      /* Check results */

      usRequest = hb_vmRequestQuery();
      if( usRequest == HB_QUIT_REQUESTED )
      {
         if( pResult )
            hb_itemRelease( pResult );
         hb_errRelease( pError );
         hb_vmQuit();
      }
      else if( usRequest == HB_BREAK_REQUESTED || usRequest == HB_ENDPROC_REQUESTED )
      {
         if( pResult )
            hb_itemRelease( pResult );
         pResult = NULL;
      }
      else
      {
         /* If the canSubstitute flag has not been set,
            consider it as a failure. */

         if( ! ( hb_errGetFlags( pError ) & EF_CANSUBSTITUTE ) )
            hb_errInternal( HB_EI_ERRRECFAILURE, NULL, NULL, NULL );
      }
   }
   else
      pResult = hb_itemNew( NULL );

   return pResult;
}

void hb_errRelease( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errRelease(%p)", pError));

   /* NOTE: NULL pointer is checked by hb_itemRelease() [vszakats] */
   hb_itemRelease( pError );
}

char * hb_errGetDescription( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetDescription(%p)", pError));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "DESCRIPTION" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   return hb_itemGetCPtr( hb_stackReturnItem() );
}

PHB_ITEM hb_errPutDescription( PHB_ITEM pError, const char * szDescription )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutDescription(%p, %s)", pError, szDescription));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_DESCRIPTION" ) );
   hb_vmPush( pError );
   hb_vmPushString( ( char * ) szDescription, strlen( szDescription ) );
   hb_vmDo( 1 );

   return pError;
}

char * hb_errGetFileName( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetFileName(%p)", pError));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "FILENAME" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   return hb_itemGetCPtr( hb_stackReturnItem() );
}

PHB_ITEM hb_errPutFileName( PHB_ITEM pError, const char * szFileName )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutFileName(%p, %s)", pError, szFileName));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_FILENAME" ) );
   hb_vmPush( pError );
   hb_vmPushString( ( char * ) szFileName, strlen( szFileName ) );
   hb_vmDo( 1 );

   return pError;
}

USHORT hb_errGetGenCode( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetGenCode(%p)", pError));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "GENCODE" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   return hb_itemGetNI( hb_stackReturnItem() );
}

PHB_ITEM hb_errPutGenCode( PHB_ITEM pError, USHORT uiGenCode )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutGenCode(%p, %hu)", pError, uiGenCode));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_GENCODE" ) );
   hb_vmPush( pError );
   hb_vmPushInteger( uiGenCode );
   hb_vmDo( 1 );

   return pError;
}

char * hb_errGetOperation( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetOperation(%p)", pError));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "OPERATION" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   return hb_itemGetCPtr( hb_stackReturnItem() );
}

PHB_ITEM hb_errPutOperation( PHB_ITEM pError, const char * szOperation )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutOperation(%p, %s)", pError, szOperation));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_OPERATION" ) );
   hb_vmPush( pError );
   hb_vmPushString( ( char * ) szOperation, strlen( szOperation ) );
   hb_vmDo( 1 );

   return pError;
}

USHORT hb_errGetOsCode( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetOsCode(%p)", pError));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "OSCODE" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   return hb_itemGetNI( hb_stackReturnItem() );
}

PHB_ITEM hb_errPutOsCode( PHB_ITEM pError, USHORT uiOsCode )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutOsCode(%p, %hu)", pError, uiOsCode));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_OSCODE" ) );
   hb_vmPush( pError );
   hb_vmPushInteger( uiOsCode );
   hb_vmDo( 1 );

   return pError;
}

USHORT hb_errGetSeverity( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetSeverity(%p)", pError));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "SEVERITY" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   return hb_itemGetNI( hb_stackReturnItem() );
}

PHB_ITEM hb_errPutSeverity( PHB_ITEM pError, USHORT uiSeverity )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutSeverity(%p, %hu)", pError, uiSeverity));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_SEVERITY" ) );
   hb_vmPush( pError );
   hb_vmPushInteger( uiSeverity );
   hb_vmDo( 1 );

   return pError;
}

USHORT hb_errGetSubCode( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetSubCode(%p)", pError));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "SUBCODE" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   return hb_itemGetNI( hb_stackReturnItem() );
}

PHB_ITEM hb_errPutSubCode( PHB_ITEM pError, USHORT uiSubCode )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutSubCode(%p, %hu)", pError, uiSubCode));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_SUBCODE" ) );
   hb_vmPush( pError );
   hb_vmPushInteger( uiSubCode );
   hb_vmDo( 1 );

   return pError;
}

char * hb_errGetSubSystem( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetSubSytem(%p)", pError));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "SUBSYSTEM" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   return hb_itemGetCPtr( hb_stackReturnItem() );
}

PHB_ITEM hb_errPutSubSystem( PHB_ITEM pError, const char * szSubSystem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutSubSytem(%p, %s)", pError, szSubSystem));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_SUBSYSTEM" ) );
   hb_vmPush( pError );
   hb_vmPushString( ( char * ) szSubSystem, strlen( szSubSystem ) );
   hb_vmDo( 1 );

   return pError;
}

USHORT hb_errGetTries( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetTries(%p)", pError));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "TRIES" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   return hb_itemGetNI( hb_stackReturnItem() );
}

PHB_ITEM hb_errPutTries( PHB_ITEM pError, USHORT uiTries )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutTries(%p, %hu)", pError, uiTries));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_TRIES" ) );
   hb_vmPush( pError );
   hb_vmPushInteger( uiTries );
   hb_vmDo( 1 );

   return pError;
}

USHORT hb_errGetFlags( PHB_ITEM pError )
{
   USHORT uiFlags = EF_NONE;

   HB_TRACE(HB_TR_DEBUG, ("hb_errGetFlags(%p)", pError));

   /* ; */

   hb_vmPushSymbol( hb_dynsymGetSymbol( "CANRETRY" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   if( hb_itemGetL( hb_stackReturnItem() ) )
      uiFlags |= EF_CANRETRY;

   /* ; */

   hb_vmPushSymbol( hb_dynsymGetSymbol( "CANSUBSTITUTE" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   if( hb_itemGetL( hb_stackReturnItem() ) )
      uiFlags |= EF_CANSUBSTITUTE;

   /* ; */

   hb_vmPushSymbol( hb_dynsymGetSymbol( "CANDEFAULT" ) );
   hb_vmPush( pError );
   hb_vmDo( 0 );

   if( hb_itemGetL( hb_stackReturnItem() ) )
      uiFlags |= EF_CANDEFAULT;

   /* ; */

   return uiFlags;
}

PHB_ITEM hb_errPutFlags( PHB_ITEM pError, USHORT uiFlags )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutFlags(%p, %hu)", pError, uiFlags));

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_CANRETRY" ) );
   hb_vmPush( pError );
   hb_vmPushLogical( ( uiFlags & EF_CANRETRY ) ? TRUE : FALSE );
   hb_vmDo( 1 );

   /* ; */

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_CANSUBSTITUTE" ) );
   hb_vmPush( pError );
   hb_vmPushLogical( ( uiFlags & EF_CANSUBSTITUTE ) ? TRUE : FALSE );
   hb_vmDo( 1 );

   /* ; */

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_CANDEFAULT" ) );
   hb_vmPush( pError );
   hb_vmPushLogical( ( uiFlags & EF_CANDEFAULT ) ? TRUE : FALSE );
   hb_vmDo( 1 );

   /* ; */

   return pError;
}

PHB_ITEM hb_errPutArgs( PHB_ITEM pError, ULONG ulArgCount, ... )
{
   PHB_ITEM pArray;
   ULONG ulArgPos;
   va_list va;

   HB_TRACE(HB_TR_DEBUG, ("hb_errPutArgs(%p, %hu, ...)", pError, ulArgCount));

   pArray = hb_itemArrayNew( ulArgCount );

   /* Build the array from the passed arguments. */

   va_start( va, ulArgCount );
   for( ulArgPos = 1; ulArgPos <= ulArgCount; ulArgPos++ )
      hb_itemArrayPut( pArray, ulArgPos, va_arg( va, PHB_ITEM ) );
   va_end( va );

   /* Assign the new array to the object data item. */

   hb_vmPushSymbol( hb_dynsymGetSymbol( "_ARGS" ) );
   hb_vmPush( pError );
   hb_vmPush( pArray );
   hb_vmDo( 1 );

   hb_itemRelease( pArray );

   return pError;
}

/* Wrappers for hb_errLaunch() */

PHB_ITEM hb_errRT_New(
   USHORT uiSeverity,
   const char * szSubSystem,
   ULONG  ulGenCode,
   ULONG  ulSubCode,
   const char * szDescription,
   const char * szOperation,
   USHORT uiOsCode,
   USHORT uiFlags )
{
   PHB_ITEM pError = hb_errNew();

   hb_errPutSeverity( pError, uiSeverity );
   hb_errPutSubSystem( pError, szSubSystem ? szSubSystem : HB_ERR_SS_BASE );
   hb_errPutGenCode( pError, ( USHORT ) ulGenCode );
   hb_errPutSubCode( pError, ( USHORT ) ulSubCode );
   hb_errPutDescription( pError, szDescription ? szDescription : ( const char * ) hb_langDGetItem( HB_LANG_ITEM_BASE_ERRDESC + ulGenCode ) );
   hb_errPutOperation( pError, szOperation ? szOperation : "" );
   hb_errPutOsCode( pError, uiOsCode );
   hb_errPutFlags( pError, uiFlags );

   return pError;
}

PHB_ITEM hb_errRT_New_Subst(
   USHORT uiSeverity,
   const char * szSubSystem,
   ULONG  ulGenCode,
   ULONG  ulSubCode,
   const char * szDescription,
   const char * szOperation,
   USHORT uiOsCode,
   USHORT uiFlags )
{
   PHB_ITEM pError = hb_errNew();

   hb_errPutSeverity( pError, uiSeverity );
   hb_errPutSubSystem( pError, szSubSystem ? szSubSystem : HB_ERR_SS_BASE );
   hb_errPutGenCode( pError, ( USHORT ) ulGenCode );
   hb_errPutSubCode( pError, ( USHORT ) ulSubCode );
   hb_errPutDescription( pError, szDescription ? szDescription : ( const char * ) hb_langDGetItem( HB_LANG_ITEM_BASE_ERRDESC + ulGenCode ) );
   hb_errPutOperation( pError, szOperation ? szOperation : "" );
   hb_errPutOsCode( pError, uiOsCode );
   hb_errPutFlags( pError, uiFlags | EF_CANSUBSTITUTE );

   return pError;
}

PHB_ITEM hb_errRT_SubstParams( const char *szSubSystem, ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation )
{
   PHB_ITEM pRetVal;
   PHB_ITEM pError;
   PHB_ITEM pArray;

   HB_TRACE_STEALTH( HB_TR_DEBUG, ( "hb_errRT_SubstParams()") );

   pError = hb_errRT_New_Subst( ES_ERROR, szSubSystem ? szSubSystem : HB_ERR_SS_BASE,
               ulGenCode, ulSubCode, szDescription, szOperation, 0, EF_NONE );

   pArray = hb_arrayBaseParams();

   /* Assign the new array to the object data item. */
   hb_vmPushSymbol( hb_dynsymGetSymbol( "_ARGS" ) );
   hb_vmPush( pError );
   hb_vmPush( pArray );
   hb_vmSend( 1 );

   hb_itemRelease( pArray );

   /* Ok, launch... */
   pRetVal = hb_errLaunchSubst( pError );

   hb_itemRelease( pError );

   return pRetVal;
}


HB_FUNC( __ERRRT_BASE )
{
   hb_errRT_BASE( ( ULONG ) hb_parnl( 1 ),
                  ( ULONG ) hb_parnl( 2 ),
                  hb_parc( 3 ),
                  hb_parc( 4 ),
                  ( USHORT ) hb_parni( 5 ),
                  hb_param( 6, HB_IT_ANY ) );
}

HB_FUNC( __ERRRT_SBASE )
{
   hb_errRT_BASE_SubstR( ( ULONG ) hb_parnl( 1 ),
                         ( ULONG ) hb_parnl( 2 ),
                         hb_parc( 3 ),
                         hb_parc( 4 ),
                         ( USHORT ) hb_parni( 5 ),
                         hb_param( 6, HB_IT_ANY ) );
}

USHORT hb_errRT_BASE( ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation, ULONG ulArgCount, ... )
{
   USHORT uiAction;
   PHB_ITEM pError;

   PHB_ITEM pArray, pArg;
   va_list va;
   ULONG ulArgPos;
   BOOL bRelease = TRUE;

   /* Build the array from the passed arguments. */
   va_start( va, ulArgCount );
   if( ( ulSubCode == 1001 || ulSubCode == 1004 || ulSubCode == 1005 ) && ulArgCount == 1 )
   {
      pArray = va_arg( va, PHB_ITEM );

      if( HB_IS_ARRAY( pArray ) )
      {
         bRelease = FALSE;
      }
      else
      {
         pArg = pArray;
         pArray = hb_itemArrayNew( 1 );
         hb_itemArrayPut( pArray, 1, pArg );
      }
   }
   else if ( ulArgCount == 0 )
   {
      pArray = NULL;
   }
   else
   {
      pArray = hb_itemArrayNew( ulArgCount );

      for( ulArgPos = 1; ulArgPos <= ulArgCount; ulArgPos++ )
      {
         hb_itemArrayPut( pArray, ulArgPos, va_arg( va, PHB_ITEM ) );
      }
   }
   va_end( va );

   /* I replaced EF_CANRETRY with EF_NONE for Clipper compatibility
    * If it's wrong and I missed sth please fix me, Druzus.
    */
   pError = hb_errRT_New( ES_ERROR, HB_ERR_SS_BASE, ulGenCode, ulSubCode, szDescription, szOperation, 0, EF_NONE /* EF_CANRETRY */ );

   if ( pArray )
   {
      /* Assign the new array to the object data item. */
      hb_vmPushSymbol( hb_dynsymGetSymbol( "_ARGS" ) );
      hb_vmPush( pError );
      hb_vmPush( pArray );
      hb_vmDo( 1 );

      /* Release the Array. */
      if( bRelease )
      {
         hb_itemRelease( pArray );
      }
   }

   /* Ok, launch... */
   uiAction = hb_errLaunch( pError );

   /* Release. */
   hb_errRelease( pError );

   return uiAction;
}

USHORT hb_errRT_BASE_Ext1( ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation, USHORT uiOsCode, USHORT uiFlags, ULONG ulArgCount, ... )
{
   USHORT uiAction;
   PHB_ITEM pError;

   PHB_ITEM pArray;
   va_list va;
   ULONG ulArgPos;

   pError = hb_errRT_New( ES_ERROR, HB_ERR_SS_BASE, ulGenCode, ulSubCode, szDescription, szOperation, uiOsCode, uiFlags );

   if ( ulArgCount > 0 )
   {
      pArray = hb_itemArrayNew( ulArgCount );

      /* Build the array from the passed arguments. */
      va_start( va, ulArgCount );
      for( ulArgPos = 1; ulArgPos <= ulArgCount; ulArgPos++ )
         hb_itemArrayPut( pArray, ulArgPos, va_arg( va, PHB_ITEM ) );
      va_end( va );

      /* Assign the new array to the object data item. */
      hb_vmPushSymbol( hb_dynsymGetSymbol( "_ARGS" ) );
      hb_vmPush( pError );
      hb_vmPush( pArray );
      hb_vmDo( 1 );

      /* Release the Array. */
      hb_itemRelease( pArray );
   }

   /* Ok, launch... */
   uiAction = hb_errLaunch( pError );

   hb_errRelease( pError );

   return uiAction;
}

PHB_ITEM hb_errRT_BASE_Subst( ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation, ULONG ulArgCount, ... )
{
   PHB_ITEM pRetVal;
   PHB_ITEM pError;

   PHB_ITEM pArray;
   va_list va;
   ULONG ulArgPos;

   pError = hb_errRT_New_Subst( ES_ERROR, HB_ERR_SS_BASE, ulGenCode, ulSubCode, szDescription, szOperation, 0, EF_NONE );

   if ( ulArgCount > 0 )
   {
      pArray = hb_itemArrayNew( ulArgCount );

      /* Build the array from the passed arguments. */
      va_start( va, ulArgCount );
      for( ulArgPos = 1; ulArgPos <= ulArgCount; ulArgPos++ )
         hb_itemArrayPut( pArray, ulArgPos, va_arg( va, PHB_ITEM ) );
      va_end( va );

      /* Assign the new array to the object data item. */
      hb_vmPushSymbol( hb_dynsymGetSymbol( "_ARGS" ) );
      hb_vmPush( pError );
      hb_vmPush( pArray );
      hb_vmDo( 1 );

      /* Release the Array. */
      hb_itemRelease( pArray );
   }

   /* Ok, launch... */
   pRetVal = hb_errLaunchSubst( pError );

   hb_errRelease( pError );

   return pRetVal;
}

void hb_errRT_BASE_SubstR( ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation, ULONG ulArgCount, ... )
{
   PHB_ITEM pError;

   PHB_ITEM pArray, pArg;
   va_list va;
   ULONG ulArgPos;
   BOOL bRelease = TRUE;

   /* Build the array from the passed arguments. */
   va_start( va, ulArgCount );
   if( ( ulSubCode == 1001 || ulSubCode == 1004 || ulSubCode == 1005 ) && ulArgCount == 1 )
   {
      pArray = va_arg( va, PHB_ITEM );

      if( HB_IS_ARRAY( pArray ) )
      {
         bRelease = FALSE;
      }
      else
      {
         pArg = pArray;
         pArray = hb_itemArrayNew( 1 );
         hb_itemArrayPut( pArray, 1, pArg );
      }
   }
   else if ( ulArgCount == 0 )
   {
      pArray = NULL;
   }
   else
   {
      pArray = hb_itemArrayNew( ulArgCount );

      for( ulArgPos = 1; ulArgPos <= ulArgCount; ulArgPos++ )
      {
         hb_itemArrayPut( pArray, ulArgPos, va_arg( va, PHB_ITEM ) );
      }
   }
   va_end( va );

   pError = hb_errRT_New_Subst( ES_ERROR, HB_ERR_SS_BASE, ulGenCode, ulSubCode, szDescription, szOperation, 0, EF_NONE );

   if ( pArray )
   {
      /* Assign the new array to the object data item. */
      hb_vmPushSymbol( hb_dynsymGetSymbol( "_ARGS" ) );
      hb_vmPush( pError );
      hb_vmPush( pArray );
      hb_vmDo( 1 );

      /* Release the Array. */
      if( bRelease )
      {
         hb_itemRelease( pArray );
      }
   }

   /* Ok, launch... */
   hb_itemRelease( hb_itemReturn( hb_errLaunchSubst( pError ) ) );
   hb_errRelease( pError );
}

USHORT hb_errRT_TERM( ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation, USHORT uiOSCode, USHORT uiFlags )
{
   USHORT uiAction;
   PHB_ITEM pError =
      hb_errRT_New( ES_ERROR, HB_ERR_SS_TERMINAL, ulGenCode, ulSubCode, szDescription, szOperation, uiOSCode, uiFlags );

   uiAction = hb_errLaunch( pError );

   hb_errRelease( pError );

   return uiAction;
}

USHORT hb_errRT_DBCMD( ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation )
{
   USHORT uiAction;
   PHB_ITEM pError =
      hb_errRT_New( ES_ERROR, HB_ERR_SS_DBCMD, ulGenCode, ulSubCode, szDescription, szOperation, 0, EF_NONE );

   uiAction = hb_errLaunch( pError );

   hb_errRelease( pError );

   return uiAction;
}

USHORT hb_errRT_DBCMD_Ext( ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation, USHORT uiFlags )
{
   USHORT uiAction;
   PHB_ITEM pError;

   pError = hb_errRT_New( ES_ERROR, HB_ERR_SS_DBCMD, ulGenCode, ulSubCode, (const char *)szDescription, (const char *)szOperation, 0, uiFlags );

   uiAction = hb_errLaunch( pError );

   hb_itemRelease( pError );

   return uiAction;
}

USHORT hb_errRT_TOOLS( ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation )
{
   USHORT uiAction;
   PHB_ITEM pError =
      hb_errRT_New( ES_ERROR, HB_ERR_SS_BASE, ulGenCode, ulSubCode, szDescription, szOperation, 0, EF_NONE );

   uiAction = hb_errLaunch( pError );

   hb_errRelease( pError );

   return uiAction;
}
