//
// $Id$
//

// Testing AltD()
// Notice you have to compile it using /b

function Main()

   AltD( 1 )   // Enables the debugger. Press F5 to go

   Alert( "debugger enabled" )

   AltD()      // Invokes the debugger

   Alert( "debugger invoked" )

return nil