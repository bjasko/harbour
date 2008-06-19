/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * dbf2msql.prg - converts a .dbf file into a mSQL table
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


procedure main(c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11)

   local cTok, nTok := 1
   local cHostName, cDataBase, cTable, cFile
   local aDbfStruct, i
   local lCreateTable := .F.
   local oServer, oTable, oRecord

   SET CENTURY ON
   SET EPOCH TO 1960

   if PCount() < 8
      help()
      quit
   endif

   i := 1
   // Scan parameters and setup workings
   while (i <= PCount())

      cTok := hb_PValue(i++)

      do case
      case cTok == "-h"
         cHostName := hb_PValue(i++)

      case cTok == "-d"
         cDataBase := hb_PValue(i++)

      case cTok == "-t"
         cTable := hb_PValue(i++)

      case cTok == "-f"
         cFile := hb_PValue(i++)

      case cTok == "-c"
         lCreateTable := .T.

      otherwise
         help()
         quit
      endcase
   enddo

   dbUseArea(.T.,, cFile, "dbffile",, .T.)
   aDbfStruct := dbffile->(dbStruct())

   oServer := TmSQLServer():New(cHostName)
   if oServer:NetErr()
      ? oServer:Error()
      quit
   endif

   oServer:SelectDB(cDataBase)
   if oServer:NetErr()
      ? oServer:Error()
      quit
   endif

   if lCreateTable
      if Ascan(oServer:ListTables(), cTable) > 0
         oServer:DeleteTable(cTable)
         if oServer:NetErr()
            ? oServer:Error()
            quit
         endif
      endif
      oServer:CreateTable(cTable, aDbfStruct)
      if oServer:NetErr()
         ? oServer:Error()
         quit
      endif
   endif

   // Initialize mSQL table
   oTable := oServer:Query("SELECT * FROM " + cTable + " LIMIT 1")
   if oTable:NetErr()
      Alert(oTable:Error())
      quit
   endif

   while !dbffile->(eof()) .AND. Inkey() != 27

      oRecord := oTable:GetBlankRow()

      for i := 1 to dbffile->(FCount())
         oRecord:FieldPut(i, dbffile->(FieldGet(i)))
      next

      oTable:Append(oRecord)
      if oTable:NetErr()
         Alert(oTable:Error())
      endif

      dbffile->(dbSkip())

      DevPos(Row(), 1)
      DevOut("imported recs: " + Str(dbffile->(RecNo())))
   enddo

   dbffile->(dbCloseArea())
   oTable:Destroy()
   oServer:Destroy()
return


procedure Help()

   ? "dbf2msql - dbf file to mSQL table conversion utility"
   ? "-h hostname "
   ? "-d name of database to use"
   ? "-t name of table to add records to"
   ? "-c delete existing table and create a new one"
   ? "-f name of .dbf file to import"
   ? "all parameters but -c are mandatory"
   ? ""

return
