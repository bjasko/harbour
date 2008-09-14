/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Windows registry API
 *
 * Copyright 2008 Viktor Szakats <harbour.01 syenar.hu>
 * Copyright 2004 Peter Rees <peter@rees.co.nz>
 *                Rees Software & Systems Ltd
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option )
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.   If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/ ).
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
 * not apply to the code that you add in this way.   To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
*/

#include "common.ch"
#include "hbwin.ch"

/* ------------------------------------------------------------------- */

/* Usage: w32_regPathSplit( cRegPath, @nHKEY, @cKey, @cEntry ) */
PROCEDURE w32_regPathSplit( cRegPath, nHKEY, cKey, cEntry )
   LOCAL cHKEY
   LOCAL tmp

   nHKEY := HKEY_CURRENT_USER
   cKey := ""
   cEntry := ""

   tmp := At( "\", cRegPath )
   IF tmp > 0
      cHKEY := Left( cRegPath, tmp - 1 )
      cRegPath := SubStr( cRegPath, tmp + 1 )

      tmp := RAt( "\", cRegPath )
      IF tmp > 0
         cKey := Left( cRegPath, tmp - 1 )
         cEntry := SubStr( cRegPath, tmp + 1 )
      ELSE
         cEntry := cRegPath
      ENDIF

      /* Len( <literal> ) is optimized to a number by Harbour at compile time. */
      DO CASE
      CASE Left( cHKEY, Len( "HKCU"                  ) ) == "HKCU"                  ; nHKEY := HKEY_CURRENT_USER
      CASE Left( cHKEY, Len( "HKLM"                  ) ) == "HKLM"                  ; nHKEY := HKEY_LOCAL_MACHINE
      CASE Left( cHKEY, Len( "HKCR"                  ) ) == "HKCR"                  ; nHKEY := HKEY_CLASSES_ROOT
      CASE Left( cHKEY, Len( "HKU"                   ) ) == "HKU"                   ; nHKEY := HKEY_USERS
      CASE Left( cHKEY, Len( "HKPD"                  ) ) == "HKPD"                  ; nHKEY := HKEY_PERFORMANCE_DATA
      CASE Left( cHKEY, Len( "HKCC"                  ) ) == "HKCC"                  ; nHKEY := HKEY_CURRENT_CONFIG
      CASE Left( cHKEY, Len( "HKDD"                  ) ) == "HKDD"                  ; nHKEY := HKEY_DYN_DATA
      CASE Left( cHKEY, Len( "HKEY_CURRENT_USER"     ) ) == "HKEY_CURRENT_USER"     ; nHKEY := HKEY_CURRENT_USER
      CASE Left( cHKEY, Len( "HKEY_LOCAL_MACHINE"    ) ) == "HKEY_LOCAL_MACHINE"    ; nHKEY := HKEY_LOCAL_MACHINE
      CASE Left( cHKEY, Len( "HKEY_CLASSES_ROOT"     ) ) == "HKEY_CLASSES_ROOT"     ; nHKEY := HKEY_CLASSES_ROOT
      CASE Left( cHKEY, Len( "HKEY_USERS"            ) ) == "HKEY_USERS"            ; nHKEY := HKEY_USERS
      CASE Left( cHKEY, Len( "HKEY_PERFORMANCE_DATA" ) ) == "HKEY_PERFORMANCE_DATA" ; nHKEY := HKEY_PERFORMANCE_DATA
      CASE Left( cHKEY, Len( "HKEY_CURRENT_CONFIG"   ) ) == "HKEY_CURRENT_CONFIG"   ; nHKEY := HKEY_CURRENT_CONFIG
      CASE Left( cHKEY, Len( "HKEY_DYN_DATA"         ) ) == "HKEY_DYN_DATA"         ; nHKEY := HKEY_DYN_DATA
      ENDCASE
   ENDIF

   RETURN

FUNCTION w32_regRead( cRegPath )
   LOCAL nHKEY, cKey, cEntry

   w32_regPathSplit( cRegPath, @nHKEY, @cKey, @cEntry )

   RETURN GetRegistry( nHKEY, cKey, cEntry )

FUNCTION w32_regWrite( cRegPath, xValue )
   LOCAL nHKEY, cKey, cEntry

   w32_regPathSplit( cRegPath, @nHKEY, @cKey, @cEntry )

   RETURN SetRegistry( nHKEY, cKey, cEntry, xValue )

/* ------------------------------------------------------------------- */

/* Predefined Value Types. from winnt.h */
#define KEY_QUERY_VALUE                1
#define KEY_SET_VALUE                  2
#define KEY_CREATE_SUB_KEY             4
#define KEY_ENUMERATE_SUB_KEYS         8
#define KEY_NOTIFY                     16
#define KEY_CREATE_LINK                32

#define REG_NONE                       0   // No value type
#define REG_SZ                         1   // Unicode nul terminated string
#define REG_EXPAND_SZ                  2   // Unicode nul terminated string (with environment variable references)
#define REG_BINARY                     3   // Free form binary
#define REG_DWORD                      4   // 32-bit number
#define REG_DWORD_LITTLE_ENDIAN        4   // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN           5   // 32-bit number
#define REG_LINK                       6   // Symbolic Link (unicode)
#define REG_MULTI_SZ                   7   // Multiple Unicode strings
#define REG_RESOURCE_LIST              8   // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR   9   // Resource list in the hardware description
#define REG_RESOURCE_REQUIREMENTS_LIST 10

#define ERROR_SUCCESS                  0

FUNCTION QueryRegistry( nHKEYHandle, cKeyName, cEntryName, xValue, lSetIt )
   LOCAL xKey := GetRegistry( nHKEYHandle, cKeyName, cEntryName )

   LOCAL cValType := VALTYPE( xValue )
   LOCAL rVal

   DEFAULT lSetIT TO .F.

   IF cValType == "L"
      xValue := IIF( xValue, 1, 0 )
      cValType := VALTYPE( xValue )
   ELSEIF cValType == "D"
      xValue := DTOS( xValue )
      cValType := VALTYPE( xValue )
   ENDIF

   rVal := ( xKey != NIL .AND. xValue != NIL .AND. cValType == VALTYPE( xKey ) .AND. xValue == xKey )
   IF ! rVal .AND. lSetIt
      rVal := SetRegistry( nHKEYHandle, cKeyName, cEntryName, xValue )
   ENDIF

   RETURN rVal

FUNCTION GetRegistry( nHKEYHandle, cKeyName, cEntryName )
   LOCAL xRetVal := NIL
   LOCAL nKeyHandle := 0
   LOCAL nValueType

   DEFAULT nHKeyHandle TO 0

   IF win32_RegOpenKeyEx( nHKEYHandle, cKeyName, 0, KEY_QUERY_VALUE, @nKeyHandle ) == ERROR_SUCCESS

      nValueType := 0
      /* retrieve the length of the value */
      IF win32_RegQueryValueEx( nKeyHandle, cEntryName, 0, @nValueType, @xRetVal ) > 0

         IF nValueType == REG_DWORD .OR. ;
            nValueType == REG_DWORD_LITTLE_ENDIAN .OR. ;
            nValueType == REG_DWORD_BIG_ENDIAN .OR. ;
            nValueType == REG_BINARY
            xRetVal := BIN2U( xRetVal )
         ELSE
            xRetVal := STRTRAN( xRetVal, CHR( 0 ) )
         ENDIF
      ENDIF

      win32_RegCloseKey( nKeyHandle )
   ENDIF

   RETURN xRetVal

FUNCTION SetRegistry( nHKEYHandle, cKeyName, cEntryName, xValue )
   LOCAL cName
   LOCAL nValueType
   LOCAL rVal := .F.
   LOCAL cType
   LOCAL nKeyHandle := 0
   LOCAL nResult := 1

   DEFAULT nHKeyHandle TO 0

   IF win32_RegCreateKeyEx( nHKEYHandle, cKeyName, 0, 0, 0, KEY_SET_VALUE, 0, @nKeyHandle, @nResult ) == ERROR_SUCCESS

      /* no support for Arrays, Codeblock ... */
      cType := VALTYPE( xValue )

      DO CASE
      CASE cType == "L"
         nValueType := REG_DWORD
         cName := IIF( xValue, 1, 0 )
      CASE cType == "D"
         nValueType := REG_SZ
         cName := DTOS( xValue )
      CASE cType == "N"
         nValueType := REG_DWORD
         cName := xValue
      CASE cType $ "CM"
         nValueType := REG_SZ
         cName := xValue
      ENDCASE

      IF cName != NIL
         rVal := ( win32_RegSetValueEx( nKeyHandle, cEntryName, 0, nValueType, cName ) == ERROR_SUCCESS )
      ENDIF

      win32_RegCloseKey( nKeyHandle )
   ENDIF

   RETURN rVal
