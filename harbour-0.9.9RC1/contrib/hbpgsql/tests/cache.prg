/*
 * $Id$
 *
 * This samples show how to use dbf to cache postgres records.
 *
 */

#include "common.ch"

#define CONNECTION_OK                   0
#define CONNECTION_BAD                  1
#define CONNECTION_STARTED              2
#define CONNECTION_MADE                 3
#define CONNECTION_AWAITING_RESPONSE    4
#define CONNECTION_AUTH_OK              5
#define CONNECTION_SETENV               6
#define CONNECTION_SSL_STARTUP          7
#define CONNECTION_NEEDED               8

#define PGRES_EMPTY_QUERY               0
#define PGRES_COMMAND_OK                1
#define PGRES_TUPLES_OK                 2
#define PGRES_COPY_OUT                  3
#define PGRES_COPY_IN                   4
#define PGRES_BAD_RESPONSE              5
#define PGRES_NONFATAL_ERROR            6
#define PGRES_FATAL_ERROR               7

#define PQTRANS_IDLE                    0
#define PQTRANS_ACTIVE                  1
#define PQTRANS_INTRANS                 2
#define PQTRANS_INERROR                 3
#define PQTRANS_UNKNOWN                 4


#define DB_ALIAS 1
#define DB_FILE  2
#define DB_QUERY 3
#define DB_ROW   4
#define DB_FETCH 5

STATIC oServer
STATIC aTableTemp := {}
STATIC aTempDBF   := {}

Function Main()
    Local i
    Local cQuery
    Local conn, res
  
    SetMode( 25, 80 )
        
    if SQLConnect( '127.0.0.1', 'test', 'sysadm', 'masterkey' ) 
        QuickQuery('DROP TABLE test')
        
        cQuery := 'CREATE TABLE test ( '
        cQuery += '  codigo integer primary key, '
        cQuery += '  descri char(50), '
        cQuery += '  email varchar(50) ) '    
        SQLQuery(cQuery)

        SQLOpen( 'nomes', 'SELECT * FROM test')
        
        for i := 1 to 50
            append blank
            replace codigo with i
            replace descri with 'test ' + str(i)        
        next
    
        SQLApplyUpdates()
    
        cQuery := 'SELECT * FROM test WHERE codigo >= :1 ORDER BY codigo'
        cQuery := SQLPrepare( cQuery, 1 )
        SQLOpen( 'nomes', cQuery)
    
        Do while  ! Eof()
            ? recno(), nomes->Codigo, nomes->descri, nomes->email
          
            if recno() == 10
                delete
            endif
          
            if recno() == 20 
                REPLACE email WITH 'teste'
            endif
    
            SQLFetch()
        enddo
    
        SQLApplyUpdates()        
    endif  
Return SQLGarbageCollector()



/* Put theses functions in a library */



Function SQLApplyUpdates()
  Local cAlias := Upper(Alias())
  Local i, x
  Local aField := {}
  Local oQuery
  Local oRow
  Local lUpdate
  Local lError := .F.
  Local cError 
  
  i := ASCAN(aTableTemp, {|aVal| aVal[DB_ALIAS] == cAlias})   
  
  IF i != 0

    oQuery := aTableTemp[i, 3]



    FOR i := 1 TO Lastrec()

    

      DBGoto(i)

      

      IF i > oQuery:Lastrec()

        /* Verifica se eh um registro novo */
        if ! Deleted()

            oRow := oQuery:GetBlankRow()



            FOR x := 1 TO FCount()

              if oRow:Fieldpos( Fieldname(x) ) != 0

                oRow:FieldPut(Fieldname(x), Fieldget(x))

              endif            

            NEXT


            oQuery:Append(oRow)        

            cError := oQuery:Error()

            lError := oQuery:NetErr()

        endif            

      ELSE

          

        oRow := oQuery:GetRow(i)

        lUpdate := .F.



        IF Deleted()

          oQuery:Delete(oRow)          

          cError := oQuery:Error()

          lError := oQuery:NetErr()

        

        ELSE

          /* Faz update, mas compara quais campos sao diferentes */

          FOR x := 1 TO Fcount()

            if oRow:Fieldpos( Fieldname(x) ) != 0

                if .not. (Fieldget(x) == oRow:Fieldget(Fieldname(x)))

                    oRow:Fieldput(Fieldname(x), Fieldget(x))

                    lUpdate := .t.

                endif

            endif                

          NEXT

          

          IF lUpdate

            oQuery:Update(oRow)           

            cError := oQuery:Error()

            lError := oQuery:NetErr()

          END

        END

      END

      

      if lError

        exit

      end

    NEXT

  END


  IF lError
    Alert(cError)
  END
    
Return ! lError

  
Procedure SQLCloseTemp( cAlias )
  Local x

  IF ! Empty(Select(cAlias))
    CLOSE &calias
  END
  
  x := ASCAN(aTableTemp, {|aVal| aVal[DB_ALIAS] == cAlias})         
  
  IF ! Empty(x)
      ADel( aTableTemp, x )
      //ASize( aTableTemp, Len(aTableTemp) - 1 )
  END     
Return 

  
Procedure SQLGarbageCollector()
  Local i
  Local oQuery
  
  DBCloseAll()
  
  FOR i := 1 TO Len(aTableTemp)
    /* Apaga arquivos dbfs criados */    
    FErase(aTableTemp[i, DB_FILE]) 
    oQuery := aTableTemp[i, DB_QUERY]
    
    IF ! ISNIL(oQuery)
      oQuery:Destroy()
    END
    
  NEXT
  
  FOR i := 1 TO Len(aTempDBF)
    IF File(aTempDBF[i])
        FErase(aTempDBF[i]) 
    END
    
    IF File(strtran(aTempDBF[i], '.tmp', '.dbf'))
        FErase(strtran(aTempDBF[i], '.tmp', '.dbf'))
    END

    IF File(strtran(aTempDBF[i], '.tmp', '.dbt'))
        FErase(strtran(aTempDBF[i], '.tmp', '.dbt'))
    END
  NEXT
  
  aTableTemp := {}
  aTempDBF   := {}  
Return 

  
Function SQLFetch( fetchall )
  Local oQuery
  Local oRow
  Local cAlias := Upper(Alias())
  Local i, x, y
  Local nPos := 0
  Local lEof := .F.
  
  Local cString := ""
  Local aStruct
  
  Default Fetchall TO .f.
  
  /* Procura pela tabela no array */
  i := ASCAN(aTableTemp, {|aVal| aVal[DB_ALIAS] == cAlias})         
  
  IF i != 0
    /* Traz registros da base de dados */
    
    oQuery := aTableTemp[i, DB_QUERY]
    nPos   := aTableTemp[i, DB_ROW] + 1
 
    if Fetchall
        aTableTemp[i, DB_FETCH] := .t.
    end               
    
    IF oQuery:Lastrec() >= nPos
    
      y := nPos
      
      while nPos <= IIF( FetchAll, oQuery:Lastrec(), y )
        oRow := oQuery:GetRow(nPos)
        DBAppend()

        FOR x := 1 TO oRow:FCount()
          FieldPut( FieldPos( oRow:FieldName(x) ), oRow:FieldGet(x) )
        NEXT
      
        aTableTemp[i, DB_ROW] := nPos
        nPos++
      end
      
    ELSE
      // Posiciona registro no eof
      DBSkip()
      
    END
    
    lEof := nPos > oQuery:Lastrec()
  END
return lEof


Procedure SQLFetchAll()
  SQLFetch(.t.); DBGotop()
Return
  

Function SQLOpen( cAlias, cQuery, xFetch )
  Local cFile
  Local Result := .t.
  Local i, x
  Local oServer
  Local oQuery
  Local aStrudbf
  Local lFetch
  Local cOrder
  
  oServer := SQLCurrentServer()
  cAlias := Upper(cAlias)
   
  /* Procura por query na area temporaria */
  x := ASCAN(aTableTemp, {|aVal| aVal[DB_ALIAS] == cAlias})         
  
  IF ! Empty(x)
      oQuery := aTableTemp[x, 3]
      oQuery:Destroy()
  END     
    
  IF ISNIL(cQuery)
      cQuery := 'SELECT * FROM ' + cAlias + ' ORDER BY ' + cOrder
  END
    
  cQuery := cQuery
  oQuery := oServer:Query(cQuery)

  IF oQuery:NetErr() 
    Alert(oQuery:Error())
    RETURN .F.
  END  
    
  IF Empty(Select(cAlias))
    /* Pega estrutura da base de dados */      
    aStrudbf := oQuery:Struct()      
    
    /* Cria tabela */
    cFile := TempFile()
    DBCreate( cFile, aStrudbf )

    /* Abre Tabela */
    DBUseArea(.T., NIL, cFile, cAlias, .F.)    
  
  ELSE
    SELECT &cAlias
    Zap
    
  END

  IF ! ISNIL(xFetch) 
     lFetch := xFetch
  ELSE     
     lFetch := .F.    
  END

  /* Se nao houver query na area temporaria entao adiciona, caso contrario, apenas atualiza */
  IF Empty(x)    
    AADD( aTableTemp, { cAlias,; // Table Name
                        cFile,;  // Temporary File Name
                        oQuery,; // Object Query
                        0,;      // Current Row
                        lFetch } ) // Fetch Status                        
  ELSE
  
    aTableTemp[ x, DB_QUERY ] := oQuery
    aTableTemp[ x, DB_ROW ]   := 0
    aTableTemp[ x, DB_FETCH ] := lFetch        
                            
  END

  /* Traz registros da base de dados */
  SQLFetch(lFetch)  
  
  IF lFetch
    DBGotop()
  END

Return result

  
Function SQLConnect( cServer, cDatabase, cUser, cPassword, cSchema )
  Local lRetval := .t.

  oServer := TPQServer():New(cServer, cDatabase, cUser, cPassWord, 5432, cSchema) 
  if oServer:NetErr() 
     Alert(oServer:Error())
     lRetval := .f.    
  end   
  oServer:lAllCols := .F.  
Return lRetval


Procedure SQLDestroy()
  if ! ISNIL(oServer)
    oServer:Destroy()
  end
return

  
Function SQLCurrentServer
 Return oServer

  
Function SQLQuery( cQuery )
  Local oQuery

  oQuery := oServer:Query(cQuery)
  IF oQuery:NetErr() 
    Alert(cQuery + ':' + oQuery:Error())
  END 
Return oQuery


Function SQLExecQuery( cQuery )
  Local oQuery
  Local result := .T.

  oQuery := oServer:Query(cQuery)
  IF oQuery:NetErr() 
    Alert('Nao foi poss�vel executar ' + cQuery + ':' + oQuery:Error())
    
    result := .F.
    
  ELSE  
    oQuery:Destroy()
    
  END  
Return result

    
Function SQLPrepare( cQuery, x01, x02, x03, x04, x05, x06, x07, x08, x09, x10,; 
                             x11, x12, x13, x14, x15, x16, x17, x18, x19, x20,; 
                             x21, x22, x23, x24, x25, x26, x27, x28, x29, x30,; 
                             x31, x32, x33, x34, x35, x36, x37, x38, x39, x40,; 
                             x41, x42, x43, x44, x45, x46, x47, x48, x49, x50,; 
                             x51, x52, x53, x54, x55, x56, x57, x58, x59, x60,; 
                             x61, x62, x63, x64, x65, x66, x67, x68, x69, x70,; 
                             x71, x72, x73, x74, x75, x76, x77, x78, x79, x80,; 
                             x81, x82, x83, x84, x85, x86, x87, x88, x89, x90,; 
                             x91, x92, x93, x94, x95, x96, x97, x98, x99, x100)
  Local i, x

  if Pcount() >= 2
    /* Limpa espacos desnecessarios */
    do while at( Space(2), cQuery ) != 0
        cQuery := strtran( cQuery, Space(2), Space(1) )
    enddo
            
    /* Coloca {} nos parametros */                
    for i := 1 to Pcount() - 1
        if ! empty(x := at( ':' + ltrim(str(i)), cQuery))
            cQuery := stuff( cQuery, x, 0, '{' )
            cQuery := stuff( cQuery, x + len(ltrim(str(i))) + 2, 0, '}' )
        endif
    next                
    
    /* Substitui parametros por valores passados */
    for i := 2 to PCount()
      x := PValue(i)
      
      if ! ISNIL(x) .and. Empty(x)
        x := 'null'
        
      elseif valtype(x) == 'N'
        x := ltrim(str(x))
       
      elseif valtype(x) == 'D'
        x := DtoQ(x)        
       
      elseif valtype(x) == 'L'
        x := IIF( x, "'t'", "'f'" )
       
      elseif valtype(x) == "C" .or. valtype(x) == 'M'
        x := StoQ(Trim(x))

      else
        x := 'null'       
      end
     
      cQuery := strtran(cQuery, '{:' + ltrim(str(i-1)) + '}', x)
    next            
  end
  
  cQuery := strtran(cQuery, '==', '=')
  cQuery := strtran(cQuery, '!=', '<>')
  cQuery := strtran(cQuery, '.and.', 'and')
  cQuery := strtran(cQuery, '.or.', 'or')
  cQuery := strtran(cQuery, '.not.', 'not')  
Return cQuery

   
/* Pega resultado de uma sequence */   
Function SQLSequence( Sequence_name )
    Local nValue
    nValue := Val(QuickQuery("SELECT nextval(" + StoQ(sequence_name) + ")" ))
Return nValue
           

Function SQLStartTrans()
    if PQtransactionstatus(oServer:pDB) != PQTRANS_INTRANS
        oServer:StartTransaction()
    endif        
Return nil

                    
Function SQLInTrans( lStart )
    Local result
    result := (PQtransactionstatus(oServer:pDB) == PQTRANS_INTRANS)       
Return result           


Function SQLCommitTrans()
    oServer:Commit()
Return nil    

           
Function SQLRollbackTrans()
    oServer:rollback()
Return nil    
           
           
/* Faz querie que retorna apenas 1 valor de coluna */
Function QuickQuery( cQuery )
   Local pQuery
   Local result := ""
   Local temp, aTemp
   Local x, y   

   pQuery := PQexec( oServer:pDB, cQuery )

   if PQresultstatus(pQuery) == PGRES_TUPLES_OK
        if PQLastrec(pQuery) != 0
            if PQFcount(pQuery) == 1 .and. PQLastrec(pQuery) == 1
                temp := PQGetValue( pQuery, 1, 1 )
                result := iif( temp == NIL, "", temp )
            else                    
                result := {}
                for x := 1 to PQLastrec(pQuery)
                    aTemp := {}
                    for y := 1 to PQfcount(pQuery)
                        temp := PQGetValue( pQuery, x, y )
                        aadd( aTemp, iif( temp == NIL, "", temp ) ) 
                    next
                    aadd(result, aTemp)
                next                
            endif                
        endif
   endif    
   PQclear(pQuery)           
Return result     
                        

Procedure MakeDBF( cAlias, aStructure, aIndex )
  Local cFile, i, cIndex, cKey

  Default aIndex TO {}
  
  cFile := TempFile()
  DBCreate( cFile, aStructure )

  /* Abre Tabela */
  DBUseArea(.T., NIL, cFile, cAlias, .F.)

  For i := 1 to Len(aIndex)
    cKey := aIndex[i]
    cIndex := TempFile()
    Index On &cKey To &cIndex

    aadd( aTempDBF, cIndex)    
  Next

  AADD( aTempDBF, cFile )
return
  
             
Function DirTmp()
  Local xDirectory
  xDirectory := IIF(Empty(Getenv("TMP")), Getenv("TEMP"), Getenv("TMP"))

  IF Empty(xDirectory); xDirectory := ''; END

  IF ';' $ xDirectory
    xDirectory := LEFT( xDirectory, AT( ';', xDirectory ) - 1 )
  END

  RETURN xDirectory + IIF( Right(xDirectory, 1) != '\' .and. ! Empty(xDirectory), '\', '' )


Function TempFile( cPath, cExt )
  Local cString
 
  Default cPath to DirTmp(),;
          cExt  to 'tmp'
  
  cString := cPath + strzero(int(hb_random(val(strtran(time(), ":", "")))), 8) + '.' + cExt
  
  DO WHILE File( cString )
    cString := cPath + strzero(int(hb_random(val(strtran(time(), ":", "")))), 8) + '.' + cExt
  END
Return  cString
  
  
Function DtoQ(cData)
Return "'" + Str(Year(cData),4) + "-" + StrZero(Month(cData), 2) + "-" + StrZero(Day(cData), 2) + "'"

Function StoQ(cData)
Return "'" + cData + "'"

