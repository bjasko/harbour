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
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
 *    rewritten in C ERROR class and all hb_errGet*() and hb_errPut*()
 *    functions
 *
 * See doc/license.txt for licensing terms.
 *
 */

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbapilng.h"
#include "hbapicls.h"
#include "hbvm.h"
#include "hbstack.h"

/* This is added to be able to detect a recursive error, and not let Harbour
   go into an infinite loop, this is an emulated version of the Clipper
   "Unrecoverable error 650: Processor stack fault" internal error, but
   better shows what is really the problem. [vszakats] */
#define HB_ERROR_LAUNCH_MAX 8

/* Error class instance variables offsets */
#define HB_TERROR_CARGO             1
#define HB_TERROR_ARGS              2
#define HB_TERROR_FLAGS             3
#define HB_TERROR_DESCRIPTION       4
#define HB_TERROR_FILENAME          5
#define HB_TERROR_GENCODE           6
#define HB_TERROR_OPERATION         7
#define HB_TERROR_OSCODE            8
#define HB_TERROR_SEVERITY          9
#define HB_TERROR_SUBCODE           10
#define HB_TERROR_SUBSYSTEM         11
#define HB_TERROR_TRIES             12

#define HB_TERROR_IVARCOUNT         12


const char hb_errFuncName = 0;

static HB_ERROR_INFO_PTR s_errorHandler = NULL;
static PHB_ITEM s_errorBlock = NULL;
static PHB_ITEM s_pError = NULL;
static int     s_iLaunchCount = 0;
static USHORT  s_uiErrorDOS = 0; /* The value of DOSERROR() */

HB_FUNC_EXTERN( ERRORNEW );
static HB_SYMB  s_symErrorNew = { "ERRORNEW", {HB_FS_PUBLIC|HB_FS_LOCAL}, {HB_FUNCNAME( ERRORNEW )}, NULL };


static BOOL hb_errGetNumCode( int * piValue, const char * szOperation )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_NUMERIC );

   if( pItem )
      *piValue = hb_itemGetNI( pItem );
   else
   {
      pItem = hb_errRT_BASE_Subst( EG_ARG, 0, NULL, szOperation,
                                   HB_ERR_ARGS_BASEPARAMS );
      if( !pItem )
      {
         *piValue = 0;
         return FALSE;
      }

      if( !HB_IS_NUMERIC( pItem ) )
         hb_errInternal( HB_EI_ERRRECFAILURE, NULL, NULL, NULL );

      *piValue = hb_itemGetNI( pItem );
      hb_itemRelease( pItem );
   }

   return TRUE;
}


HB_FUNC_STATIC( CARGO )
{
   hb_itemReturn( hb_errGetCargo( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _CARGO )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ANY );

   if( pItem )
      hb_errPutCargo( hb_stackSelfItem(), pItem );

   hb_itemReturn( pItem );
}


HB_FUNC_STATIC( ARGS )
{
   hb_itemReturn( hb_errGetArgs( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _ARGS )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ARRAY );

   if( pItem )
      hb_errPutArgsArray( hb_stackSelfItem(), pItem );

   hb_itemReturn( pItem );
}


HB_FUNC_STATIC( CANDEFAULT )
{
   hb_retl( ( hb_errGetFlags( hb_stackSelfItem() ) & EF_CANDEFAULT ) != 0 );
}

HB_FUNC_STATIC( _CANDEFAULT )
{
   if( ISLOG( 1 ) )
   {
      PHB_ITEM pError = hb_stackSelfItem();
      BOOL fCan = hb_parl( 1 );
      
      if( fCan )
         hb_errPutFlags( pError, hb_errGetFlags( pError ) | EF_CANDEFAULT );
      else
         hb_errPutFlags( pError, hb_errGetFlags( pError ) & ~EF_CANDEFAULT );

      hb_retl( fCan );
   }
}


HB_FUNC_STATIC( CANRETRY )
{
   hb_retl( ( hb_errGetFlags( hb_stackSelfItem() ) & EF_CANRETRY ) != 0 );
}

HB_FUNC_STATIC( _CANRETRY )
{
   if( ISLOG( 1 ) )
   {
      PHB_ITEM pError = hb_stackSelfItem();
      BOOL fCan = hb_parl( 1 );
      
      if( fCan )
         hb_errPutFlags( pError, hb_errGetFlags( pError ) | EF_CANRETRY );
      else
         hb_errPutFlags( pError, hb_errGetFlags( pError ) & ~EF_CANRETRY );

      hb_retl( fCan );
   }
}


HB_FUNC_STATIC( CANSUBST )
{
   hb_retl( ( hb_errGetFlags( hb_stackSelfItem() ) & EF_CANSUBSTITUTE ) != 0 );
}

HB_FUNC_STATIC( _CANSUBST )
{
   if( ISLOG( 1 ) )
   {
      PHB_ITEM pError = hb_stackSelfItem();
      BOOL fCan = hb_parl( 1 );
      
      if( fCan )
         hb_errPutFlags( pError, hb_errGetFlags( pError ) | EF_CANSUBSTITUTE );
      else
         hb_errPutFlags( pError, hb_errGetFlags( pError ) & ~EF_CANSUBSTITUTE );

      hb_retl( fCan );
   }
}


HB_FUNC_STATIC( DESCRIPTION )
{
   hb_retc( hb_errGetDescription( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _DESCRIPTION )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ANY );

   if( pItem && HB_IS_STRING( pItem ) )
      hb_errPutDescription( hb_stackSelfItem(), hb_itemGetCPtr( pItem ) );

   hb_itemReturn( pItem );
}


HB_FUNC_STATIC( FILENAME )
{
   hb_retc( hb_errGetFileName( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _FILENAME )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ANY );

   if( pItem && HB_IS_STRING( pItem ) )
      hb_errPutFileName( hb_stackSelfItem(), hb_itemGetCPtr( pItem ) );

   hb_itemReturn( pItem );
}


HB_FUNC_STATIC( OPERATION )
{
   hb_retc( hb_errGetOperation( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _OPERATION )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ANY );

   if( pItem && HB_IS_STRING( pItem ) )
      hb_errPutOperation( hb_stackSelfItem(), hb_itemGetCPtr( pItem ) );

   hb_itemReturn( pItem );
}


HB_FUNC_STATIC( SUBSYSTEM )
{
   hb_retc( hb_errGetSubSystem( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _SUBSYSTEM )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ANY );

   if( pItem && HB_IS_STRING( pItem ) )
      hb_errPutSubSystem( hb_stackSelfItem(), hb_itemGetCPtr( pItem ) );

   hb_itemReturn( pItem );
}


HB_FUNC_STATIC( GENCODE )
{
   hb_retni( hb_errGetGenCode( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _GENCODE )
{
   int iValue;

   if( hb_errGetNumCode( &iValue, "GENCODE" ) )
   {
      hb_errPutGenCode( hb_stackSelfItem(), ( USHORT ) iValue );
      hb_errPutDescription( hb_stackSelfItem(),
                            hb_langDGetErrorDesc( iValue ) );
   }

   hb_retni( iValue );
}


HB_FUNC_STATIC( OSCODE )
{
   hb_retni( hb_errGetOsCode( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _OSCODE )
{
   int iValue;

   if( hb_errGetNumCode( &iValue, "OSCODE" ) )
      hb_errPutOsCode( hb_stackSelfItem(), ( USHORT ) iValue );

   hb_retni( iValue );
}


HB_FUNC_STATIC( SUBCODE )
{
   hb_retni( hb_errGetSubCode( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _SUBCODE )
{
   int iValue;

   if( hb_errGetNumCode( &iValue, "SUBCODE" ) )
      hb_errPutSubCode( hb_stackSelfItem(), ( USHORT ) iValue );

   hb_retni( iValue );
}


HB_FUNC_STATIC( SEVERITY )
{
   hb_retni( hb_errGetSeverity( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _SEVERITY )
{
   int iValue;

   if( hb_errGetNumCode( &iValue, "SEVERITY" ) )
      hb_errPutSeverity( hb_stackSelfItem(), ( USHORT ) iValue );

   hb_retni( iValue );
}


HB_FUNC_STATIC( TRIES )
{
   hb_retni( hb_errGetTries( hb_stackSelfItem() ) );
}

HB_FUNC_STATIC( _TRIES )
{
   int iValue;

   if( hb_errGetNumCode( &iValue, "TRIES" ) )
      hb_errPutTries( hb_stackSelfItem(), ( USHORT ) iValue );

   hb_retni( iValue );
}


static USHORT hb_errClassCreate( void )
{
   USHORT usClassH = hb_clsCreate( HB_TERROR_IVARCOUNT, "ERROR" );
                     
   hb_clsAdd( usClassH, "ARGS"          , HB_FUNCNAME( ARGS )         );
   hb_clsAdd( usClassH, "_ARGS"         , HB_FUNCNAME( _ARGS )        );
   hb_clsAdd( usClassH, "CANDEFAULT"    , HB_FUNCNAME( CANDEFAULT )   );
   hb_clsAdd( usClassH, "_CANDEFAULT"   , HB_FUNCNAME( _CANDEFAULT )  );
   hb_clsAdd( usClassH, "CANRETRY"      , HB_FUNCNAME( CANRETRY )     );
   hb_clsAdd( usClassH, "_CANRETRY"     , HB_FUNCNAME( _CANRETRY )    );
   hb_clsAdd( usClassH, "CANSUBSTITUTE" , HB_FUNCNAME( CANSUBST )     );
   hb_clsAdd( usClassH, "_CANSUBSTITUTE", HB_FUNCNAME( _CANSUBST )    );
   hb_clsAdd( usClassH, "CARGO"         , HB_FUNCNAME( CARGO )        );
   hb_clsAdd( usClassH, "_CARGO"        , HB_FUNCNAME( _CARGO )       );
   hb_clsAdd( usClassH, "DESCRIPTION"   , HB_FUNCNAME( DESCRIPTION )  );
   hb_clsAdd( usClassH, "_DESCRIPTION"  , HB_FUNCNAME( _DESCRIPTION ) );
   hb_clsAdd( usClassH, "FILENAME"      , HB_FUNCNAME( FILENAME )     );
   hb_clsAdd( usClassH, "_FILENAME"     , HB_FUNCNAME( _FILENAME )    );
   hb_clsAdd( usClassH, "GENCODE"       , HB_FUNCNAME( GENCODE )      );
   hb_clsAdd( usClassH, "_GENCODE"      , HB_FUNCNAME( _GENCODE )     );
   hb_clsAdd( usClassH, "OPERATION"     , HB_FUNCNAME( OPERATION )    );
   hb_clsAdd( usClassH, "_OPERATION"    , HB_FUNCNAME( _OPERATION )   );
   hb_clsAdd( usClassH, "OSCODE"        , HB_FUNCNAME( OSCODE )       );
   hb_clsAdd( usClassH, "_OSCODE"       , HB_FUNCNAME( _OSCODE )      );
   hb_clsAdd( usClassH, "SEVERITY"      , HB_FUNCNAME( SEVERITY )     );
   hb_clsAdd( usClassH, "_SEVERITY"     , HB_FUNCNAME( _SEVERITY )    );
   hb_clsAdd( usClassH, "SUBCODE"       , HB_FUNCNAME( SUBCODE )      );
   hb_clsAdd( usClassH, "_SUBCODE"      , HB_FUNCNAME( _SUBCODE )     );
   hb_clsAdd( usClassH, "SUBSYSTEM"     , HB_FUNCNAME( SUBSYSTEM )    );
   hb_clsAdd( usClassH, "_SUBSYSTEM"    , HB_FUNCNAME( _SUBSYSTEM )   );
   hb_clsAdd( usClassH, "TRIES"         , HB_FUNCNAME( TRIES )        );
   hb_clsAdd( usClassH, "_TRIES"        , HB_FUNCNAME( _TRIES )       );

   return usClassH;
}

HB_FUNC( ERRORNEW )
{
   hb_itemReturnRelease( hb_errNew() );
}

/* There's a similar undocumented, internal function in CA-Cl*pper named
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

PHB_ITEM hb_errorBlock( void )
{
   return s_errorBlock;
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

   /* error function */
   hb_dynsymNew( &s_symErrorNew );

   /* initialize an item for error block */
   s_errorBlock = hb_itemNew( NULL );

   /* Create error class and base object */
   s_pError = hb_itemNew( NULL );
   hb_clsAssociate( hb_errClassCreate() );
   hb_itemMove( s_pError, hb_stackReturnItem() );
}

void hb_errExit( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errExit()"));

   hb_itemRelease( s_errorBlock );
   s_errorBlock = NULL;
   hb_itemRelease( s_pError );
   s_pError = NULL;
}

PHB_ITEM hb_errNew( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errNew()"));

   if( !s_pError || !HB_IS_OBJECT( s_pError ) )
      hb_errInternal( HB_EI_ERRRECFAILURE, NULL, NULL, NULL );

   return hb_arrayClone( s_pError );
}

USHORT hb_errLaunch( PHB_ITEM pError )
{
   USHORT uiAction = E_DEFAULT; /* Needed to avoid GCC -O2 warning */

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

      /* set DOSERROR() to last OS error code */
      s_uiErrorDOS = hb_errGetOsCode( pError );

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

      if( hb_vmRequestQuery() != 0 )
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

      /* set DOSERROR() to last OS error code */
      s_uiErrorDOS = hb_errGetOsCode( pError );

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

      if( hb_vmRequestQuery() != 0 )
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

PHB_ITEM hb_errGetCargo( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetCargo(%p)", pError));

   return hb_arrayGetItemPtr( pError, HB_TERROR_CARGO );
}

PHB_ITEM hb_errPutCargo( PHB_ITEM pError, PHB_ITEM pCargo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutCargo(%p, %p)", pError, pCargo));

   hb_arraySet( pError, HB_TERROR_CARGO, pCargo );

   return pError;
}

PHB_ITEM hb_errGetArgs( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetArgs(%p)", pError));

   return hb_arrayGetItemPtr( pError, HB_TERROR_ARGS );
}

PHB_ITEM hb_errPutArgsArray( PHB_ITEM pError, PHB_ITEM pArgs )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutArgsArray(%p, %p)", pError, pArgs));

   hb_arraySet( pError, HB_TERROR_ARGS, pArgs );

   return pError;
}

char * hb_errGetDescription( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetDescription(%p)", pError));

   return hb_arrayGetCPtr( pError, HB_TERROR_DESCRIPTION );
}

PHB_ITEM hb_errPutDescription( PHB_ITEM pError, const char * szDescription )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutDescription(%p, %s)", pError, szDescription));

   hb_arraySetC( pError, HB_TERROR_DESCRIPTION, szDescription );

   return pError;
}

char * hb_errGetFileName( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetFileName(%p)", pError));

   return hb_arrayGetCPtr( pError, HB_TERROR_FILENAME );
}

PHB_ITEM hb_errPutFileName( PHB_ITEM pError, const char * szFileName )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutFileName(%p, %s)", pError, szFileName));

   hb_arraySetC( pError, HB_TERROR_FILENAME, szFileName );

   return pError;
}

USHORT hb_errGetGenCode( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetGenCode(%p)", pError));

   return hb_arrayGetNI( pError, HB_TERROR_GENCODE );
}

PHB_ITEM hb_errPutGenCode( PHB_ITEM pError, USHORT uiGenCode )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutGenCode(%p, %hu)", pError, uiGenCode));

   hb_arraySetNI( pError, HB_TERROR_GENCODE, uiGenCode );

   return pError;
}

char * hb_errGetOperation( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetOperation(%p)", pError));

   return hb_arrayGetCPtr( pError, HB_TERROR_OPERATION );
}

PHB_ITEM hb_errPutOperation( PHB_ITEM pError, const char * szOperation )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutOperation(%p, %s)", pError, szOperation));

   if( szOperation == HB_ERR_FUNCNAME )
   {
      PHB_SYMB pSym = hb_itemGetSymbol( hb_stackBaseItem() );
      if( pSym )
         szOperation = pSym->szName;
   }

   hb_arraySetC( pError, HB_TERROR_OPERATION, szOperation );

   return pError;
}

USHORT hb_errGetOsCode( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetOsCode(%p)", pError));

   return hb_arrayGetNI( pError, HB_TERROR_OSCODE );
}

PHB_ITEM hb_errPutOsCode( PHB_ITEM pError, USHORT uiOsCode )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutOsCode(%p, %hu)", pError, uiOsCode));

   hb_arraySetNI( pError, HB_TERROR_OSCODE, uiOsCode );

   return pError;
}

USHORT hb_errGetSeverity( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetSeverity(%p)", pError));

   return hb_arrayGetNI( pError, HB_TERROR_SEVERITY );
}

PHB_ITEM hb_errPutSeverity( PHB_ITEM pError, USHORT uiSeverity )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutSeverity(%p, %hu)", pError, uiSeverity));

   hb_arraySetNI( pError, HB_TERROR_SEVERITY, uiSeverity );

   return pError;
}

USHORT hb_errGetSubCode( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetSubCode(%p)", pError));

   return hb_arrayGetNI( pError, HB_TERROR_SUBCODE );
}

PHB_ITEM hb_errPutSubCode( PHB_ITEM pError, USHORT uiSubCode )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutSubCode(%p, %hu)", pError, uiSubCode));

   hb_arraySetNI( pError, HB_TERROR_SUBCODE, uiSubCode );

   return pError;
}

char * hb_errGetSubSystem( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetSubSytem(%p)", pError));

   return hb_arrayGetCPtr( pError, HB_TERROR_SUBSYSTEM );
}

PHB_ITEM hb_errPutSubSystem( PHB_ITEM pError, const char * szSubSystem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutSubSytem(%p, %s)", pError, szSubSystem));

   hb_arraySetC( pError, HB_TERROR_SUBSYSTEM, szSubSystem );

   return pError;
}

USHORT hb_errGetTries( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetTries(%p)", pError));

   return hb_arrayGetNI( pError, HB_TERROR_TRIES );
}

PHB_ITEM hb_errPutTries( PHB_ITEM pError, USHORT uiTries )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutTries(%p, %hu)", pError, uiTries));

   hb_arraySetNI( pError, HB_TERROR_TRIES, uiTries );

   return pError;
}

USHORT hb_errGetFlags( PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errGetFlags(%p)", pError));

   return hb_arrayGetNI( pError, HB_TERROR_FLAGS );
}

PHB_ITEM hb_errPutFlags( PHB_ITEM pError, USHORT uiFlags )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_errPutFlags(%p, %hu)", pError, uiFlags));

   uiFlags &= EF_CANRETRY | EF_CANSUBSTITUTE | EF_CANDEFAULT;
   hb_arraySetNI( pError, HB_TERROR_FLAGS, uiFlags );

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
   hb_errPutArgsArray( pError, pArray );

   /* Release the Array. */
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
   hb_errPutArgsArray( pError, pArray );

   /* Release the Array. */
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
                  ( USHORT ) ( hb_pcount() > 5 && hb_parni( 5 ) > 0 ? 1 : 0 ),
                  hb_param( 6, HB_IT_ANY ) );
}

HB_FUNC( __ERRRT_SBASE )
{
   hb_errRT_BASE_SubstR( ( ULONG ) hb_parnl( 1 ),
                         ( ULONG ) hb_parnl( 2 ),
                         hb_parc( 3 ),
                         hb_parc( 4 ),
                         ( USHORT ) ( hb_pcount() > 5 && hb_parni( 5 ) > 0 ? 1 : 0 ),
                         hb_param( 6, HB_IT_ANY ) );
}

USHORT hb_errRT_BASE( ULONG ulGenCode, ULONG ulSubCode, const char * szDescription, const char * szOperation, ULONG ulArgCount, ... )
{
   USHORT uiAction;
   PHB_ITEM pError;

   PHB_ITEM pArray;
   va_list va;
   ULONG ulArgPos;

   /* I replaced EF_CANRETRY with EF_NONE for Clipper compatibility
    * If it's wrong and I missed sth please fix me, Druzus.
    */
   pError = hb_errRT_New( ES_ERROR, HB_ERR_SS_BASE, ulGenCode, ulSubCode, szDescription, szOperation, 0, EF_NONE /* EF_CANRETRY */ );

   /* Build the array from the passed arguments. */
   if( ulArgCount == 0 )
   {
      pArray = NULL;
   }
   else if( ulArgCount == HB_ERR_ARGS_BASEPARAMS )
   {
      if( hb_pcount() == 0 )
         pArray = NULL;
      else
         pArray = hb_arrayBaseParams();
   }
   else if( ulArgCount == HB_ERR_ARGS_SELFPARAMS )
   {
      pArray = hb_arraySelfParams();
   }
   else
   {
      pArray = hb_itemArrayNew( ulArgCount );

      va_start( va, ulArgCount );
      for( ulArgPos = 1; ulArgPos <= ulArgCount; ulArgPos++ )
      {
         PHB_ITEM pArg = va_arg( va, PHB_ITEM );
         if( pArg )
            hb_itemArrayPut( pArray, ulArgPos, pArg );
      }
      va_end( va );
   }
   if( pArray )
   {
      /* Assign the new array to the object data item. */
      hb_errPutArgsArray( pError, pArray );

      /* Release the Array. */
      hb_itemRelease( pArray );
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

   /* Build the array from the passed arguments. */
   if( ulArgCount == 0 )
   {
      pArray = NULL;
   }
   else if( ulArgCount == HB_ERR_ARGS_BASEPARAMS )
   {
      if( hb_pcount() == 0 )
         pArray = NULL;
      else
         pArray = hb_arrayBaseParams();
   }
   else if( ulArgCount == HB_ERR_ARGS_SELFPARAMS )
   {
      pArray = hb_arraySelfParams();
   }
   else
   {
      pArray = hb_itemArrayNew( ulArgCount );

      va_start( va, ulArgCount );
      for( ulArgPos = 1; ulArgPos <= ulArgCount; ulArgPos++ )
      {
         PHB_ITEM pArg = va_arg( va, PHB_ITEM );
         if( pArg )
            hb_itemArrayPut( pArray, ulArgPos, pArg );
      }
      va_end( va );
   }
   if( pArray )
   {
      /* Assign the new array to the object data item. */
      hb_errPutArgsArray( pError, pArray );

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

   /* Build the array from the passed arguments. */
   if( ulArgCount == 0 )
   {
      pArray = NULL;
   }
   else if( ulArgCount == HB_ERR_ARGS_BASEPARAMS )
   {
      if( hb_pcount() == 0 )
         pArray = NULL;
      else
         pArray = hb_arrayBaseParams();
   }
   else if( ulArgCount == HB_ERR_ARGS_SELFPARAMS )
   {
      pArray = hb_arraySelfParams();
   }
   else
   {
      pArray = hb_itemArrayNew( ulArgCount );

      va_start( va, ulArgCount );
      for( ulArgPos = 1; ulArgPos <= ulArgCount; ulArgPos++ )
      {
         PHB_ITEM pArg = va_arg( va, PHB_ITEM );
         if( pArg )
            hb_itemArrayPut( pArray, ulArgPos, pArg );
      }
      va_end( va );
   }
   if( pArray )
   {
      /* Assign the new array to the object data item. */
      hb_errPutArgsArray( pError, pArray );

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

   PHB_ITEM pArray;
   va_list va;
   ULONG ulArgPos;

   pError = hb_errRT_New_Subst( ES_ERROR, HB_ERR_SS_BASE, ulGenCode, ulSubCode, szDescription, szOperation, 0, EF_NONE );

   /* Build the array from the passed arguments. */
   if( ulArgCount == 0 )
   {
      pArray = NULL;
   }
   else if( ulArgCount == HB_ERR_ARGS_BASEPARAMS )
   {
      if( hb_pcount() == 0 )
         pArray = NULL;
      else
         pArray = hb_arrayBaseParams();
   }
   else if( ulArgCount == HB_ERR_ARGS_SELFPARAMS )
   {
      pArray = hb_arraySelfParams();
   }
   else
   {
      pArray = hb_itemArrayNew( ulArgCount );

      va_start( va, ulArgCount );
      for( ulArgPos = 1; ulArgPos <= ulArgCount; ulArgPos++ )
      {
         PHB_ITEM pArg = va_arg( va, PHB_ITEM );
         if( pArg )
            hb_itemArrayPut( pArray, ulArgPos, pArg );
      }
      va_end( va );
   }
   if( pArray )
   {
      /* Assign the new array to the object data item. */
      hb_errPutArgsArray( pError, pArray );

      /* Release the Array. */
      hb_itemRelease( pArray );
   }

   /* Ok, launch... */
   hb_itemReturnRelease( hb_errLaunchSubst( pError ) );
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
