/*
 * $Id$
 */

#include "common.ch"

function Test( cParam )

   LOCAL cFile := "C:\harbour\bin\harbour.exe"

   DEFAULT cParam TO cFile

   ? FilePath( cParam )
   ? FileBase( cParam )
   ? FileExt( cParam )
   ? FileDrive( cParam )

return nil
