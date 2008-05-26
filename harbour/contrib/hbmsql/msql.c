/*
 * $Id$
 */


/*
 * Harbour Project source code:
 * mSQL DBMS low level (client api) interface code.
 *
 * Copyright 2000 Maurilio Longo <maurilio.longo@libero.it>
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


/* NOTE: we need this to prevent base types redefinition */
#define _CLIPDEFS_H

#include "extend.api"
#include "item.api"

#ifdef HB_OS_WIN_32
   /* This is needed by msql.h to indicate the platform. */
   #define _OS_WIN32
#endif

#include "msql.h"

HB_FUNC(MSQLCONNEC) // int msqlConnect(char *)
{
   _retni(msqlConnect(_parc(1)));
}


HB_FUNC(MSQLCLOSE) // void msqlClose(int)
{
   msqlClose(_parni(1));
   _ret();
}


HB_FUNC(MSQLSELECT) // int msqlSelectDB(int, char *)
{
   _retni(msqlSelectDB(_parni(1), _parc(2)));
}


HB_FUNC(MSQLQUERY) // int msqlQuery(int, char *)
{
   _retni(msqlQuery(_parni(1), _parc(2)));
}


HB_FUNC(MSQLSTORER) // m_result *msqlStoreResult()
{
   _retnl((long) msqlStoreResult());
}


HB_FUNC(MSQLFREER) // void msqlFreeResult(m_result *)
{
   msqlFreeResult((m_result *)_parnl(1));
   _ret();
}


/* NOTE: need number of retrieved fields */
HB_FUNC(MSQLFETCHR) // m_row msqlFetchRow(m_result *, int)
{
   m_result *mresult = (m_result *)_parnl(1);
   int num_fields = _parnl(2);

   ITEM aRow = _itemArrayNew(num_fields);
   ITEM temp;
   m_row mrow;
   int i;

   mrow = msqlFetchRow(mresult);

   for (i = 0; i < num_fields; i++) {

      /* if field is not empty */
      if (mrow[i] != NULL) {
         temp = _itemPutC(NULL, mrow[i]);
       } else {
         temp = _itemPutC(NULL, "");
       }

      _itemArrayPut(aRow, i + 1, temp);
      _itemRelease(temp);
   }
   _itemReturn(aRow);
   _itemRelease(aRow);
}


HB_FUNC(MSQLDATASE) // void msqlDataSeek(m_result *, int)
{
   msqlDataSeek((m_result *)_parnl(1), _parni(2));
   _ret();
}


HB_FUNC(MSQLNUMROW) // int msqlNumRows(m_result *)
{
   _retni(msqlNumRows(((m_result *)_parnl(1))));
}


HB_FUNC(MSQLFETCHF) // m_field *msqlFetchField(m_result *)
{
   /* NOTE: m_field structure of mSQL 2.x has 5 members */
   ITEM aField = _itemArrayNew(5);

   ITEM temp;
   m_field *mfield;

   mfield = msqlFetchField((m_result *)_parnl(1));
   if (!(mfield == NULL)) {
      temp = _itemPutC(NULL, mfield->name);
      _itemArrayPut(aField, 1, temp);
      _itemRelease(temp);
      temp = _itemPutC(NULL, mfield->table);
      _itemArrayPut(aField, 2, temp);
      _itemRelease(temp);
      temp = _itemPutNL(NULL, mfield->type);
      _itemArrayPut(aField, 3, temp);
      _itemRelease(temp);
      temp = _itemPutNL(NULL, mfield->length);
      _itemArrayPut(aField, 4, temp);
      _itemRelease(temp);
      temp = _itemPutNL(NULL, mfield->flags);
      _itemArrayPut(aField, 5, temp);
      _itemRelease(temp);
   }
   _itemReturn(aField);
   _itemRelease(aField);

}


HB_FUNC(MSQLFIELDS) // void msqlFieldSeek(m_result *, int)
{
   msqlFieldSeek((m_result *)_parnl(1), _parni(2));
   _ret();
}


HB_FUNC(MSQLNUMFIE) // int msqlNumFields(m_result *)
{
   _retni(msqlNumFields(((m_result *)_parnl(1))));
}


HB_FUNC(MSQLLISTFI) // m_result *msqlListFields(int, char *);
{
   _retnl((long) msqlListFields(_parni(1), _parc(2)));
}


HB_FUNC(MSQLGETERR) // char *msqlGetErrMsg(char *);
{
   _retc(msqlGetErrMsg(NULL));
}


HB_FUNC(MSQLLISTDB) // m_result * msqlListDBs(int);
{
   int sock = _parnl(1);
   m_result *mresult;
   m_row mrow;
   long nr, i;
   ITEM aDBs;
   ITEM temp;

   mresult = msqlListDBs(sock);
   nr = msqlNumRows(mresult);
   aDBs = _itemArrayNew(nr);

   for (i = 0; i < nr; i++) {
      mrow = msqlFetchRow(mresult);
      temp = _itemPutC(NULL, mrow[0]);

      _itemArrayPut(aDBs, i + 1, temp);
      _itemRelease(temp);
   }

   msqlFreeResult(mresult);
   _itemReturn(aDBs);
   _itemRelease(aDBs);
}


HB_FUNC(MSQLLISTTA) // m_result * msqlListTables(int);
{
   int sock = _parnl(1);
   m_result *mresult;
   m_row mrow;
   long nr, i;
   ITEM aTables;
   ITEM temp;

   mresult = msqlListTables(sock);
   nr = msqlNumRows(mresult);
   aTables = _itemArrayNew(nr);

   for (i = 0; i < nr; i++) {

      mrow = msqlFetchRow(mresult);
      temp = _itemPutC(NULL, mrow[0]);

      _itemArrayPut(aTables, i + 1, temp);
      _itemRelease(temp);
   }

   msqlFreeResult(mresult);
   _itemReturn(aTables);
   _itemRelease(aTables);
}

