/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Default RDD module
 *
 * Copyright 1999 Bruno Cantero <bruno@issnet.net>
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
 *
 * The following functions are added by
 *       Horacio Roldan <harbour_ar@yahoo.com.ar>
 * hb_waCloseAux()
 *
 */

#include <ctype.h>
#include "hbapi.h"
#include "hbinit.h"
#include "hbvm.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbset.h"
#include "hbrddwrk.h"


/*
 * -- METHODS --
 */

/*
 * Determine logical beginning of file.
 */
ERRCODE hb_waBof( AREAP pArea, BOOL * pBof )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waBof(%p, %p)", pArea, pBof));

   * pBof = pArea->fBof;
   return SUCCESS;
}

/*
 * Determine logical end of file.
 */
ERRCODE hb_waEof( AREAP pArea, BOOL * pEof )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waEof(%p, %p)", pArea, pEof));

   * pEof = pArea->fEof;
   return SUCCESS;
}

/*
 * Determine outcome of the last search operation.
 */
ERRCODE hb_waFound( AREAP pArea, BOOL * pFound )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waFound(%p, %p)", pArea, pFound));

   * pFound = pArea->fFound;
   return SUCCESS;
}

/*
 * Reposition cursor relative to current position.
 */
ERRCODE hb_waSkip( AREAP pArea, LONG lToSkip )
{
   LONG lSkip;

   HB_TRACE(HB_TR_DEBUG, ("hb_waSkip(%p, %ld)", pArea, lToSkip));

   /* Flush record and exit */
   if( lToSkip == 0 )
      return SELF_SKIPRAW( pArea, 0 );

   pArea->fTop = pArea->fBottom = FALSE;

   if( lToSkip > 0 )
      lSkip = 1;
   else
   {
      lSkip = -1;
      lToSkip *= -1;
   }
   while( lToSkip-- > 0 )
   {
      SELF_SKIPRAW( pArea, lSkip );
      SELF_SKIPFILTER( pArea, lSkip );
      if( pArea->fBof || pArea->fEof )
         break;
   }

   /* Update Bof and Eof flags */
   if( lToSkip < 0 )
      pArea->fEof = FALSE;
   else if( lToSkip > 0 )
      pArea->fBof = FALSE;

   return SUCCESS;
}

/*
 * Reposition cursor respecting any filter setting.
 */
ERRCODE hb_waSkipFilter( AREAP pArea, LONG lUpDown )
{
   BOOL bTop, bBottom, bOutOfRange, bDeleted;
   PHB_ITEM pResult;
   ERRCODE uiError;

   HB_TRACE(HB_TR_DEBUG, ("hb_waSkipFilter(%p, %ld)", pArea, lUpDown));

   if( !hb_set.HB_SET_DELETED && pArea->dbfi.itmCobExpr == NULL )
      return SUCCESS;

   lUpDown = ( lUpDown > 0  ?  1 : -1 );
      /* Since lToSkip is passed to SkipRaw, it should never request more than
         a single skip.
         The implied purpose of hb_waSkipFilter is to get off of a "bad" record
         after a skip was performed, NOT to skip lToSkip filtered records.
      */

   bTop = pArea->fTop;
   bBottom = pArea->fBottom;
   bOutOfRange = FALSE;
   while( TRUE )
   {
      if( pArea->fBof || pArea->fEof )
      {
         bOutOfRange = TRUE;
         break;
      }

      /* SET FILTER TO */
      if( pArea->dbfi.itmCobExpr )
      {
         pResult = hb_vmEvalBlock( pArea->dbfi.itmCobExpr );
         if( HB_IS_LOGICAL( pResult ) && !hb_itemGetL( pResult ) )
         {
            SELF_SKIPRAW( pArea, lUpDown );
            continue;
         }
      }

      /* SET DELETED */
      if( hb_set.HB_SET_DELETED )
      {
         SELF_DELETED( pArea, &bDeleted );
         if( bDeleted )
         {
            SELF_SKIPRAW( pArea, lUpDown );
            continue;
         }
      }
      break;
   }
   if( bOutOfRange )
   {
      /*
         TODO: these calls to SELF_GOTO are redundant; in most cases
         we are already at EOF from the skips above, and GO 0 is not necessary.
         We should take a closer look at these. --BH
      */
      if( bTop && lUpDown > 0 )
         uiError = SELF_GOTO( pArea, 0 );
      else if( bBottom && lUpDown < 0 )
         uiError = SELF_GOTO( pArea, 0 );
      else if( lUpDown < 0 )
      {
         pArea->fEof = FALSE;
         uiError = SELF_GOTOP( pArea );
         pArea->fBof = TRUE;
      }
      else
      {
         uiError = SELF_GOTO( pArea, 0 );
         pArea->fBof = FALSE;
      }
   }
   else
      uiError = SUCCESS;

   return uiError;
}

/*
 * Add a field to the WorkArea.
 */
ERRCODE hb_waAddField( AREAP pArea, LPDBFIELDINFO pFieldInfo )
{
   ULONG ulSize;
   LPFIELD pField;

   HB_TRACE(HB_TR_DEBUG, ("hb_waAddField(%p, %p)", pArea, pFieldInfo));

   /* Validate the name of field */
   ulSize = strlen( ( char * ) pFieldInfo->atomName );
   hb_strLTrim( ( char * ) pFieldInfo->atomName, &ulSize );
   ulSize = hb_strRTrimLen( ( char * ) pFieldInfo->atomName, ulSize, TRUE );
   if( !ulSize )
      return FAILURE;
   /* This line writes to the protected memory
    pFieldInfo->atomName[ulSize] = '\0'; */

   pField = pArea->lpFields + pArea->uiFieldCount;
   if( pArea->uiFieldCount > 0 )
      ( ( LPFIELD ) ( pField - 1 ) )->lpfNext = pField;
   pField->sym = ( void * ) hb_dynsymGet( ( char * ) pFieldInfo->atomName );
   pField->uiType = pFieldInfo->uiType;
   pField->uiTypeExtended = pFieldInfo->uiTypeExtended;
   pField->uiLen = pFieldInfo->uiLen;
   pField->uiDec = pFieldInfo->uiDec;
   pField->uiArea = pArea->uiArea;
   pArea->uiFieldCount ++;
   return SUCCESS;
}

/*
 * Add all fields defined in an array to the WorkArea.
 */
ERRCODE hb_waCreateFields( AREAP pArea, PHB_ITEM pStruct )
{
   USHORT uiItems, uiCount, uiLen, uiDec;
   DBFIELDINFO pFieldInfo;
   PHB_ITEM pFieldDesc;
   int iData;

   HB_TRACE(HB_TR_DEBUG, ("hb_waCreateFields(%p, %p)", pArea, pStruct));

   uiItems = ( USHORT ) hb_arrayLen( pStruct );
   SELF_SETFIELDEXTENT( pArea, uiItems );

   pFieldInfo.uiTypeExtended = 0;
   for( uiCount = 0; uiCount < uiItems; uiCount++ )
   {
      pFieldDesc = hb_arrayGetItemPtr( pStruct, uiCount + 1 );
      pFieldInfo.atomName = ( BYTE * ) hb_arrayGetCPtr( pFieldDesc, 1 );
      iData = hb_arrayGetNI( pFieldDesc, 3 );
      if( iData < 0 )
         iData = 0;
      uiLen = pFieldInfo.uiLen = ( USHORT ) iData;
      iData = hb_arrayGetNI( pFieldDesc, 4 );
      if( iData < 0 )
         iData = 0;
      uiDec = ( USHORT ) iData;
      pFieldInfo.uiDec = 0;
      iData = toupper( hb_arrayGetCPtr( pFieldDesc, 2 )[ 0 ] );
      switch( iData )
      {
         case 'C':
            pFieldInfo.uiType = HB_IT_STRING;
            pFieldInfo.uiLen = uiLen + uiDec * 256;
            break;

         case 'L':
            pFieldInfo.uiType = HB_IT_LOGICAL;
            pFieldInfo.uiLen = 1;
            break;

         case 'M':
            pFieldInfo.uiType = HB_IT_MEMO;
            pFieldInfo.uiLen = 10;
            break;

         case 'D':
            pFieldInfo.uiType = HB_IT_DATE;
            pFieldInfo.uiLen = 8;
            break;

         case 'N':
            pFieldInfo.uiType = HB_IT_LONG;
            if( uiLen > 20 )
               return FAILURE;
            else
               pFieldInfo.uiDec = uiDec;
            break;

         default:
            return FAILURE;
      }
      /* Add field */
      if( SELF_ADDFIELD( pArea, &pFieldInfo ) == FAILURE )
         return FAILURE;
   }
   return SUCCESS;
}

/*
 * Determine the number of fields in the WorkArea.
 */
ERRCODE hb_waFieldCount( AREAP pArea, USHORT * uiFields )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waFieldCount(%p, %p)", pArea, uiFields));

   * uiFields = pArea->uiFieldCount;
   return SUCCESS;
}

/*
 * Retrieve information about a field.
 */
ERRCODE hb_waFieldInfo( AREAP pArea, USHORT uiIndex, USHORT uiType, PHB_ITEM pItem )
{
   LPFIELD pField;

   HB_TRACE(HB_TR_DEBUG, ("hb_waFieldInfo(%p, %hu, %hu, %p)", pArea, uiIndex, uiType, pItem));

   if( uiIndex > pArea->uiFieldCount )
      return FAILURE;

   pField = pArea->lpFields + uiIndex - 1;
   switch( uiType )
   {
      case DBS_NAME:
         hb_itemPutC( pItem, ( ( PHB_DYNS ) pField->sym )->pSymbol->szName );
         break;

      case DBS_TYPE:
         switch( pField->uiType )
         {
            case HB_IT_STRING:
               hb_itemPutC( pItem, "C" );
               break;

            case HB_IT_LOGICAL:
               hb_itemPutC( pItem, "L" );
               break;

            case HB_IT_MEMO:
               hb_itemPutC( pItem, "M" );
               break;

            case HB_IT_DATE:
               hb_itemPutC( pItem, "D" );
               break;

            case HB_IT_LONG:
               hb_itemPutC( pItem, "N" );
               break;

            default:
               hb_itemPutC( pItem, "U" );
               break;
         }
         break;

      case DBS_LEN:
         hb_itemPutNL( pItem, pField->uiLen );
         break;

      case DBS_DEC:
         hb_itemPutNL( pItem, pField->uiDec );
         break;

      default:
         return FAILURE;

   }
   return SUCCESS;
}

/*
 * Determine the name associated with a field number.
 */
ERRCODE hb_waFieldName( AREAP pArea, USHORT uiIndex, void * szName )
{
   LPFIELD pField;

   HB_TRACE(HB_TR_DEBUG, ("hb_waFieldName(%p, %hu, %p)", pArea, uiIndex, szName));

   if( uiIndex > pArea->uiFieldExtent )
      return FAILURE;

   pField = pArea->lpFields + uiIndex - 1;
   /*
   strncpy( ( char * ) szName, ( ( PHB_DYNS ) pField->sym )->pSymbol->szName,
            HARBOUR_MAX_RDD_FIELDNAME_LENGTH );
   */
   strncpy( ( char * ) szName, ( ( PHB_DYNS ) pField->sym )->pSymbol->szName,
            pArea->uiMaxFieldNameLength );
   return SUCCESS;
}

/*
 * Establish the extent of the array of fields for a WorkArea.
 */
ERRCODE hb_waSetFieldExtent( AREAP pArea, USHORT uiFieldExtent )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waSetFieldExtent(%p, %hu)", pArea, uiFieldExtent));

   pArea->uiFieldExtent = uiFieldExtent;

   /* Alloc field array */
   pArea->lpFields = ( LPFIELD ) hb_xgrab( uiFieldExtent * sizeof( FIELD ) );
   memset( pArea->lpFields, 0, uiFieldExtent * sizeof( FIELD ) );

   return SUCCESS;
}

/*
 * Obtain the alias of the WorkArea.
 */
ERRCODE hb_waAlias( AREAP pArea, BYTE * szAlias )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waAlias(%p, %p)", pArea, szAlias));

   szAlias[0] = '\0';

   strncat( ( char * ) szAlias, ( ( PHB_DYNS ) pArea->atomAlias )->pSymbol->szName,
            HARBOUR_MAX_RDD_ALIAS_LENGTH );
   return SUCCESS;
}

/*
 * Close the table in the WorkArea.
 */
short hb_waCloseAux ( AREAP pArea, int nChildArea );

ERRCODE hb_waClose( AREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waClose(%p)", pArea));

   /* Clear items */
   SELF_CLEARFILTER( pArea );
   SELF_CLEARREL( pArea );
   SELF_CLEARLOCATE( pArea );

   if( pArea->uiParents > 0 )
   {
      /* Clear relations that has this area as a child */
      hb_rddIterateWorkAreas ( hb_waCloseAux, pArea->uiArea );
   }

   ( ( PHB_DYNS ) pArea->atomAlias )->hArea = 0;
   return SUCCESS;
}

short hb_waCloseAux ( AREAP pArea, int nChildArea )
{
   USHORT uiPrevArea, uiArea;
   LPDBRELINFO lpdbRelation, lpdbRelPrev, lpdbRelTmp;

   uiArea = ( USHORT ) nChildArea;
   if ( pArea->lpdbRelations )
   {
      uiPrevArea = hb_rddGetCurrentWorkAreaNumber();
      lpdbRelation = pArea->lpdbRelations;
      lpdbRelPrev = NULL;
      while ( lpdbRelation ) {
         if ( lpdbRelation->lpaChild->uiArea == uiArea ) {
            /* Clear this relation */
            hb_rddSelectWorkAreaNumber( lpdbRelation->lpaChild->uiArea );
            SELF_CHILDEND( lpdbRelation->lpaChild, lpdbRelation );
            hb_rddSelectWorkAreaNumber( uiPrevArea );
            if( lpdbRelation->itmCobExpr )
            {
               hb_itemRelease( lpdbRelation->itmCobExpr );
            }
            if( lpdbRelation->abKey )
               hb_itemRelease( lpdbRelation->abKey );
            lpdbRelTmp = lpdbRelation;
            if ( lpdbRelPrev )
               lpdbRelPrev->lpdbriNext = lpdbRelation->lpdbriNext;
            else
               pArea->lpdbRelations = lpdbRelation->lpdbriNext;
            lpdbRelation = lpdbRelation->lpdbriNext;
            hb_xfree( lpdbRelTmp );
         }
         else
         {
            lpdbRelPrev  = lpdbRelation;
            lpdbRelation = lpdbRelation->lpdbriNext;
         }
      }
   }
   return 1;
}

/*
 * Retrieve information about the current driver.
 */
ERRCODE hb_waInfo( AREAP pArea, USHORT uiIndex, PHB_ITEM pItem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waInfo(%p, %hu, %p)", pArea, uiIndex, pItem));
   HB_SYMBOL_UNUSED( pArea );

   if( uiIndex == DBI_ISDBF || uiIndex == DBI_CANPUTREC )
   {
      hb_itemPutL( pItem, FALSE );
      return SUCCESS;
   }
   else
      return FAILURE;
}

/*
 * Retrieve information about the current order that SELF could not.
 * Called by SELF_ORDINFO if uiIndex is not supported.
 */
#ifdef HB_COMPAT_C53
ERRCODE hb_waOrderInfo( AREAP pArea, USHORT index, LPDBORDERINFO param )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waOrderInfo(%p, %hu, %p)", pArea, index, param));
   HB_SYMBOL_UNUSED( pArea );
   HB_SYMBOL_UNUSED( index );

   if ( param->itmResult )
      hb_itemRelease( param->itmResult );
   hb_errRT_DBCMD( EG_ARG, EDBCMD_BADPARAMETER, NULL, "ORDERINFO" );
   return FAILURE;
}
#endif

/*
 * Clear the WorkArea for use.
 */
ERRCODE hb_waNewArea( AREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waNewArea(%p)", pArea));

   pArea->valResult = hb_itemNew( NULL );
   pArea->lpdbRelations = NULL;
   pArea->uiParents = 0;
   pArea->uiMaxFieldNameLength = 10;

   return SUCCESS;
}

ERRCODE hb_waOrderCondition( AREAP pArea, LPDBORDERCONDINFO param )
{
   if( pArea->lpdbOrdCondInfo )
   {
      if( pArea->lpdbOrdCondInfo->abFor )
         hb_xfree( pArea->lpdbOrdCondInfo->abFor );
      if( pArea->lpdbOrdCondInfo->abWhile )
         hb_xfree( pArea->lpdbOrdCondInfo->abWhile );
      if( pArea->lpdbOrdCondInfo->itmCobFor )
      {
         hb_itemRelease( pArea->lpdbOrdCondInfo->itmCobFor );
      }
      if( pArea->lpdbOrdCondInfo->itmCobWhile )
      {
         hb_itemRelease( pArea->lpdbOrdCondInfo->itmCobWhile );
      }
      if( pArea->lpdbOrdCondInfo->itmCobEval )
      {
         hb_itemRelease( pArea->lpdbOrdCondInfo->itmCobEval );
      }
      hb_xfree( pArea->lpdbOrdCondInfo );
   }
   pArea->lpdbOrdCondInfo = param;

   return SUCCESS;
}

/*
 * Release all references to a WorkArea.
 */
ERRCODE hb_waRelease( AREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waRelease(%p)", pArea));

   /* Free all allocated pointers */
   if( pArea->lpFields )
      hb_xfree( pArea->lpFields );
   if( pArea->valResult )
      hb_itemRelease( pArea->valResult );
   if( pArea->lpdbOrdCondInfo )
      hb_waOrderCondition( pArea,NULL );
   hb_xfree( pArea );
   return SUCCESS;
}

/*
 * Retrieve the size of the WorkArea structure.
 */
ERRCODE hb_waStructSize( AREAP pArea, USHORT * uiSize )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waStrucSize(%p, %p)", pArea, uiSize));
   HB_SYMBOL_UNUSED( pArea );

   * uiSize = sizeof( AREA );
   return SUCCESS;
}

/*
 * Obtain the name of replaceable database driver (RDD) subsystem.
 */
ERRCODE hb_waSysName( AREAP pArea, BYTE * pBuffer )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waSysName(%p, %p)", pArea, pBuffer));
   HB_SYMBOL_UNUSED( pArea );

   pBuffer[ 0 ] = 0;
   return SUCCESS;
}

/*
 * Evaluate code block for each record in WorkArea.
 */
ERRCODE hb_waEval( AREAP pArea, LPDBEVALINFO pEvalInfo )
{
   BOOL bFor, bWhile;
   ULONG ulNext;

   HB_TRACE(HB_TR_DEBUG, ("hb_waEval(%p, %p)", pArea, pEvalInfo));

   ulNext = 0;
   if( pEvalInfo->dbsci.itmRecID )
   {
      SELF_GOTO( pArea, hb_itemGetNL( pEvalInfo->dbsci.itmRecID ) );
      if( !pArea->fEof )
      {
         if( pEvalInfo->dbsci.itmCobWhile )
            bWhile = hb_itemGetL( hb_vmEvalBlock( pEvalInfo->dbsci.itmCobWhile ) );
         else
            bWhile = TRUE;

         if( pEvalInfo->dbsci.itmCobFor )
            bFor = hb_itemGetL( hb_vmEvalBlock( pEvalInfo->dbsci.itmCobFor ) );
         else
            bFor = TRUE;

         if( bWhile && bFor )
            hb_vmEvalBlock( pEvalInfo->itmBlock );
      }
      return SUCCESS;
   }

   if( !pEvalInfo->dbsci.itmCobWhile &&
         (!pEvalInfo->dbsci.fRest || !hb_itemGetL( pEvalInfo->dbsci.fRest ) ) &&
         !pEvalInfo->dbsci.lNext )
      SELF_GOTOP( pArea );

   if( pEvalInfo->dbsci.lNext )
      ulNext = hb_itemGetNL( pEvalInfo->dbsci.lNext );

   while( !pArea->fEof )
   {
      if( pEvalInfo->dbsci.lNext && ulNext-- < 1 )
         break;

      if( pEvalInfo->dbsci.itmCobWhile )
      {
         bWhile = hb_itemGetL( hb_vmEvalBlock( pEvalInfo->dbsci.itmCobWhile ) );
         if( !bWhile )
            break;
      }
      else
         bWhile = TRUE;

      if( pEvalInfo->dbsci.itmCobFor )
         bFor = hb_itemGetL( hb_vmEvalBlock( pEvalInfo->dbsci.itmCobFor ) );
      else
         bFor = TRUE;

      if( bFor && bWhile )
         hb_vmEvalBlock( pEvalInfo->itmBlock );
      SELF_SKIP( pArea, 1 );
   }

   return SUCCESS;
}

/*
 * Copy one or more records from one WorkArea to another.
 */
ERRCODE hb_waTrans( AREAP pArea, LPDBTRANSINFO pTransInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waTrans(%p, %p)", pArea, pTransInfo));
   HB_SYMBOL_UNUSED( pArea );
   HB_SYMBOL_UNUSED( pTransInfo );

   printf( "\nTODO: hb_waTrans()\n" );
   return SUCCESS;
}

/*
 * Copy a record to another WorkArea.
 */
ERRCODE hb_waTransRec( AREAP pArea, LPDBTRANSINFO pTransInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waTransRec(%p, %p)", pArea, pTransInfo));
   HB_SYMBOL_UNUSED( pArea );
   HB_SYMBOL_UNUSED( pTransInfo );

   printf( "\nTODO: hb_waTransRec()\n" );
   return SUCCESS;
}

/*
 * Report end of relation.
 */
ERRCODE hb_waChildEnd( AREAP pArea, LPDBRELINFO pRelInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waChildEnd(%p, %p)", pArea, pRelInfo));
   HB_SYMBOL_UNUSED( pRelInfo );

   pArea->uiParents --;
   return SUCCESS;
}

/*
 * Report initialization of a relation.
 */
ERRCODE hb_waChildStart( AREAP pArea, LPDBRELINFO pRelInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waChildStart(%p, %p)", pArea, pRelInfo));
   HB_SYMBOL_UNUSED( pRelInfo );

   pArea->uiParents ++;
   return SUCCESS;
}

/*
 * Force relational movement in child WorkAreas.
 */
ERRCODE hb_waSyncChildren( AREAP pArea )
{

   LPDBRELINFO lpdbRelation;
   HB_TRACE(HB_TR_DEBUG, ("hb_waSyncChildren(%p)", pArea));

   lpdbRelation = pArea->lpdbRelations;
   while( lpdbRelation )
   {
      SELF_CHILDSYNC( lpdbRelation->lpaChild, lpdbRelation );
      lpdbRelation = lpdbRelation->lpdbriNext;
   }

   return SUCCESS;
}

/*
 * Clear all relations in the specified WorkArea.
 */
ERRCODE hb_waClearRel( AREAP pArea )
{
   LPDBRELINFO lpdbRelation, lpdbRelPrev;
   int iCurrArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_waClearRel(%p)", pArea ));

   iCurrArea = hb_rddGetCurrentWorkAreaNumber();

   /* Free all relations */
   lpdbRelation = pArea->lpdbRelations;
   while( lpdbRelation )
   {
      hb_rddSelectWorkAreaNumber( lpdbRelation->lpaChild->uiArea);
      SELF_CHILDEND( lpdbRelation->lpaChild, lpdbRelation );
      hb_rddSelectWorkAreaNumber( iCurrArea );

      if( lpdbRelation->itmCobExpr )
      {
         hb_itemRelease( lpdbRelation->itmCobExpr );
      }
      if( lpdbRelation->abKey )
         hb_itemRelease( lpdbRelation->abKey );

      lpdbRelPrev = lpdbRelation;
      lpdbRelation = lpdbRelation->lpdbriNext;
      hb_xfree( lpdbRelPrev );
   }
   pArea->lpdbRelations = NULL;

   return SUCCESS;
}

/*
 * Obtain the workarea number of the specified relation.
 */
ERRCODE hb_waRelArea( AREAP pArea, USHORT uiRelNo, void * pRelArea )
{
   LPDBRELINFO lpdbRelations;
   USHORT uiIndex = 1;
   USHORT* pWA = (USHORT *) pRelArea ;
   /*TODO: Why pRelArea declared as void*? This creates casting hassles.*/

   HB_TRACE(HB_TR_DEBUG, ("hb_waRelArea(%p, %hu, %p)", pArea, uiRelNo, pRelArea));

   *pWA = 0;
   lpdbRelations = pArea->lpdbRelations;
   while( lpdbRelations )
   {
      if ( uiIndex++ == uiRelNo )
      {
         *pWA = lpdbRelations->lpaChild->uiArea;
         break;
      }
      lpdbRelations = lpdbRelations->lpdbriNext;
   }
   return *pWA ? SUCCESS : FAILURE ;
}

/*
 * Evaluate a block against the relation in specified WorkArea.
 */
ERRCODE hb_waRelEval( AREAP pArea, LPDBRELINFO pRelInfo )
{
   int iCurrArea;
   PHB_ITEM pResult;
   HB_TRACE(HB_TR_DEBUG, ("hb_waRelEval(%p, %p)", pArea, pRelInfo));

   iCurrArea = hb_rddGetCurrentWorkAreaNumber();
   hb_rddSelectWorkAreaNumber( pRelInfo->lpaParent->uiArea );
   pResult = hb_vmEvalBlock( pRelInfo->itmCobExpr );
   hb_rddSelectWorkAreaNumber( iCurrArea );
   if( SELF_SEEK( pArea, 0, pResult, 0 ) == SUCCESS )
      return SUCCESS;
   else
      return FAILURE;
}

/*
 * Obtain the character expression of the specified relation.
 */
ERRCODE hb_waRelText( AREAP pArea, USHORT uiRelNo, void * pExpr )
{
   LPDBRELINFO lpdbRelations;
   USHORT uiIndex = 1;
   char* pBuf = (char*) pExpr;  /*TODO: Why is the string buffer declared as void*? This creates casting hassles.*/

   HB_TRACE(HB_TR_DEBUG, ("hb_waRelText(%p, %hu, %p)", pArea, uiRelNo, pExpr));

   *pBuf = 0;
   lpdbRelations = pArea->lpdbRelations;

   while( lpdbRelations )
   {
      if ( uiIndex++ == uiRelNo )
      {
         strcpy(pBuf, hb_itemGetCPtr( lpdbRelations->abKey) );
         break;
         /* TODO: Verify buffer size is big enough ?? */
      }
      lpdbRelations = lpdbRelations->lpdbriNext;
   }
   return *pBuf ? SUCCESS : FAILURE ;
}

/*
 * Set a relation in the parent file.
 */
ERRCODE hb_waSetRel( AREAP pArea, LPDBRELINFO lpdbRelInf )
{
   LPDBRELINFO lpdbRelations;

   HB_TRACE(HB_TR_DEBUG, ("hb_waSetRel(%p, %p)", pArea, lpdbRelInf));

   lpdbRelations = pArea->lpdbRelations;
   if( ! lpdbRelations )
   {
      pArea->lpdbRelations = ( LPDBRELINFO ) hb_xgrab( sizeof( DBRELINFO ) );
      lpdbRelations = pArea->lpdbRelations;
   }
   else
   {
      while( lpdbRelations->lpdbriNext )
         lpdbRelations = lpdbRelations->lpdbriNext;
      lpdbRelations->lpdbriNext = ( LPDBRELINFO ) hb_xgrab( sizeof( DBRELINFO ) );
      lpdbRelations = lpdbRelations->lpdbriNext;
   }
   lpdbRelations->lpaParent = pArea;
   lpdbRelations->lpaChild = lpdbRelInf->lpaChild;
   lpdbRelations->itmCobExpr = lpdbRelInf->itmCobExpr;
   lpdbRelations->abKey = lpdbRelInf->abKey;
   lpdbRelations->lpdbriNext = lpdbRelInf->lpdbriNext;

   SELF_CHILDSTART( ( AREAP ) lpdbRelInf->lpaChild,lpdbRelations );

   return SUCCESS;
}

/*
 * Clear the active filter expression.
 */
ERRCODE hb_waClearFilter( AREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waClearFilter(%p)", pArea));

   /* Free all items */
   if( pArea->dbfi.itmCobExpr )
   {
      hb_itemRelease( pArea->dbfi.itmCobExpr );
      pArea->dbfi.itmCobExpr = NULL;
   }
   if( pArea->dbfi.abFilterText )
   {
      hb_itemRelease( pArea->dbfi.abFilterText );
      pArea->dbfi.abFilterText = NULL;
   }

   return SUCCESS;
}

/*
 * Clear the active locate expression.
 */
ERRCODE hb_waClearLocate( AREAP pArea )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waClearLocate(%p)", pArea));

   /* Free all items */
   if( pArea->dbsi.itmCobFor )
   {
      hb_itemRelease( pArea->dbsi.itmCobFor );
      pArea->dbsi.itmCobFor = NULL;
   }
   if( pArea->dbsi.lpstrFor )
   {
      hb_itemRelease( pArea->dbsi.lpstrFor );
      pArea->dbsi.lpstrFor = NULL;
   }
   if( pArea->dbsi.itmCobWhile )
   {
      hb_itemRelease( pArea->dbsi.itmCobWhile );
      pArea->dbsi.itmCobWhile = NULL;
   }
   if( pArea->dbsi.lpstrWhile )
   {
      hb_itemRelease( pArea->dbsi.lpstrWhile );
      pArea->dbsi.lpstrWhile = NULL;
   }
   if( pArea->dbsi.lNext )
   {
      hb_itemRelease( pArea->dbsi.lNext );
      pArea->dbsi.lNext = NULL;
   }
   if( pArea->dbsi.itmRecID )
   {
      hb_itemRelease( pArea->dbsi.itmRecID );
      pArea->dbsi.itmRecID = NULL;
   }
   if( pArea->dbsi.fRest )
   {
      hb_itemRelease( pArea->dbsi.fRest );
      pArea->dbsi.fRest = NULL;
   }

   return SUCCESS;
}

/*
 * Return filter condition of the specified WorkArea.
 */
ERRCODE hb_waFilterText( AREAP pArea, PHB_ITEM pFilter )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waFilterText(%p, %p)", pArea, pFilter));

   if( pArea->dbfi.abFilterText )
      hb_itemCopy( pFilter, pArea->dbfi.abFilterText );

   return SUCCESS;
}

/*
 * Set the filter condition for the specified WorkArea.
 */
ERRCODE hb_waSetFilter( AREAP pArea, LPDBFILTERINFO pFilterInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waSetFilter(%p, %p)", pArea, pFilterInfo));

   /* Clear the active filter expression */
   SELF_CLEARFILTER( pArea );

   if( pFilterInfo->itmCobExpr )
   {
      pArea->dbfi.itmCobExpr = hb_itemNew( pFilterInfo->itmCobExpr );
   }

   if( pFilterInfo->abFilterText )
      pArea->dbfi.abFilterText = hb_itemNew( pFilterInfo->abFilterText );

   return SUCCESS;
}

/*
 * Set the locate scope for the specified WorkArea.
 */
ERRCODE hb_waSetLocate( AREAP pArea, LPDBSCOPEINFO pScopeInfo )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waSetLocate(%p, %p)", pArea, pScopeInfo));

   /* Clear the active locate expression */
   SELF_CLEARLOCATE( pArea );

   if( pScopeInfo->itmCobFor )
   {
      pArea->dbsi.itmCobFor = hb_itemNew( pScopeInfo->itmCobFor );
   }

   if( pScopeInfo->lpstrFor )
      pArea->dbsi.lpstrFor = hb_itemNew( pScopeInfo->lpstrFor );

   if( pScopeInfo->itmCobWhile )
   {
      pArea->dbsi.itmCobWhile = hb_itemNew( pScopeInfo->itmCobWhile );
   }

   if( pScopeInfo->lpstrWhile )
      pArea->dbsi.lpstrWhile = hb_itemNew( pScopeInfo->lpstrWhile );

   if( pScopeInfo->lNext )
      pArea->dbsi.lNext = hb_itemNew( pScopeInfo->lNext );

   if( pScopeInfo->itmRecID )
      pArea->dbsi.itmRecID = hb_itemNew( pScopeInfo->itmRecID );

   if( pScopeInfo->fRest )
      pArea->dbsi.fRest = hb_itemNew( pScopeInfo->fRest );

   return SUCCESS;
}

/*
 * Compile a character expression.
 */
ERRCODE hb_waCompile( AREAP pArea, BYTE * pExpr )
{
   HB_MACRO_PTR pMacro;

   HB_TRACE(HB_TR_DEBUG, ("hb_waCompile(%p, %p)", pArea, pExpr));

   pMacro = hb_macroCompile( ( char * ) pExpr );
   if( pMacro )
   {
      if( ! pArea->valResult )
         pArea->valResult = hb_itemNew( NULL );

      pArea->valResult = hb_itemPutPtr( pArea->valResult, ( void * ) pMacro );
      return SUCCESS;
   }
   else
      return FAILURE;
}

/*
 * Raise a runtime error.
 */
ERRCODE hb_waError( AREAP pArea, PHB_ITEM pError )
{
   char * szRddName;

   HB_TRACE(HB_TR_DEBUG, ("hb_waError(%p, %p)", pArea, pError));

   szRddName = ( char * ) hb_xgrab( HARBOUR_MAX_RDD_DRIVERNAME_LENGTH + 1 );
   if( ( pArea )->lprfsHost->sysName )
      SELF_SYSNAME( pArea, ( BYTE * ) szRddName );
   else
      strcpy( szRddName, "???DRIVER" );
   hb_errPutSeverity( pError, ES_ERROR );
   hb_errPutSubSystem( pError, szRddName );
   hb_xfree( szRddName );
   return hb_errLaunch( pError );
}

/*
 * Evaluate a code block.
 */
ERRCODE hb_waEvalBlock( AREAP pArea, PHB_ITEM pBlock )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_waEvalBlock(%p, %p)", pArea, pBlock));

   if( ! pArea->valResult )
      pArea->valResult = hb_itemNew( NULL );

   hb_itemCopy( pArea->valResult, hb_vmEvalBlock( pBlock ) );
   return SUCCESS;
}
