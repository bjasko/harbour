/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    DLL call demonstration.
 *
 * Copyright 2008 Viktor Szakats <harbour.01 syenar.hu>
 * www - http://www.harbour-project.org
 *
 */

#define MB_OK                       0x00000000
#define MB_OKCANCEL                 0x00000001
#define MB_ABORTRETRYIGNORE         0x00000002
#define MB_YESNOCANCEL              0x00000003
#define MB_YESNO                    0x00000004
#define MB_RETRYCANCEL              0x00000005
#define MB_CANCELTRYCONTINUE        0x00000006
#define MB_ICONHAND                 0x00000010
#define MB_ICONQUESTION             0x00000020
#define MB_ICONEXCLAMATION          0x00000030
#define MB_ICONASTERISK             0x00000040
#define MB_USERICON                 0x00000080
#define MB_DEFBUTTON2               0x00000100
#define MB_DEFBUTTON3               0x00000200
#define MB_DEFBUTTON4               0x00000300
#define MB_SYSTEMMODAL              0x00001000
#define MB_TASKMODAL                0x00002000
#define MB_HELP                     0x00004000
#define MB_NOFOCUS                  0x00008000
#define MB_SETFOREGROUND            0x00010000
#define MB_DEFAULT_DESKTOP_ONLY     0x00020000
#define MB_TOPMOST                  0x00040000
#define MB_RIGHT                    0x00080000
#define MB_RTLREADING               0x00100000

PROCEDURE Main()
   LOCAL hDLL
   LOCAL cData

   IF File( "pscript.dll" )
      hDLL := DllLoad( "pscript.dll" )
      cData := Space( 24 )
      DllCall( hDll, NIL, "PSGetVersion", @cData )
      ? ">" + cData + "<"
      DllUnload( hDLL )

      // ; Testing failure 1
      hDLL := DllLoad( "pscript.dll" )
      cData := Space( 24 )
      DllCall( hDll, NIL, "PSGet__Version", @cData )
      ? ">" + cData + "<"
      DllUnload( hDLL )
   ENDIF

   // ; Testing failure 2
   hDLL := DllLoad( "nothere.dll" )
   cData := Space( 24 )
   DllCall( hDll, NIL, "PSGetVersion", @cData )
   ? cData
   DllUnload( hDLL )

   ? "MsgBox:", DllCall( "user32.dll", NIL, "MessageBoxA", 0, "Hello world!", "Harbour sez", hb_bitOr( MB_OKCANCEL, MB_ICONEXCLAMATION, MB_HELP ) )

   IF File( "libcurl.dll" )
      hDLL := DllLoad( "libcurl.dll" )
      ? GetProcAddress( hDLL, "curl_version" )
      // ; This one doesn't work.
      ? CallDllTyped( 10 /* return string */, GETPROCADDRESS( hDLL, "CURL_VERSION" ) )
      DllUnload( hDLL )
   ENDIF

   RETURN
