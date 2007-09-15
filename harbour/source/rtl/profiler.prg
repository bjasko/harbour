/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Profiler reporting classes
 *
 * Copyright 2001,2002 Dave Pearson <davep@davep.org>
 * http://www.davep.org/
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

/* Rationale:
 *
 * There are three aspects to profiling:
 *
 * 1) Gathering profile information.
 *
 * 2) Taking a snapshot of an application's profile information.
 *
 * 3) Reporting on the data gathered in the snapshot.
 *
 * Point 1 is handled in harbour's virtual machine. This source aims to
 * provide code for performing points 2 and 3. A class is provided to
 * gather, hold and manipulate the profile snapshot and a hierarchy of
 * classes exist for reporting on that snapshot. The reporting classes are
 * designed such that they are easy to inherit from and improve upon.
 *
 * The idea behind all of this is that the the design should provide all the
 * necessary building blocks for writing profiler reporters that suite the
 * user's needs (perhaps as part of an extended debugger or something).
 *
 */

/* Notes:
 *
 * As much as possible, the profiler class and the profile report classes
 * attempt to turn off the profiler to ensure that we don't get some sort of
 * Heisenberg effect. In other words, we don't want the profiler showing up
 * in the profiler.
 *
 * Many of the "Protected:" scope specifiers in the source have been
 * commented out where there's a problem with scope in harbour's class
 * system. Note that those comments will be removed when the bug is fixed.
 *
 */

/* TODO:
 *
 * o Handle any TODO: items in the source.
 * o Document the classes and the class hierarchy.
 *
 */

/* Thanks:
 *
 * Thanks to Antonio and Patrick for agreeing to replace the old profile
 * reporting code with this approach.
 */

#include "hbclass.ch"
#include "fileio.ch"
#include "common.ch"

#ifdef __TEST__

#include "inkey.ch"

Function Main()
Local oProfile := HBProfile():new()
Local n

   // Turn on profiling.
   __setProfiler( .T. )

   // Make sure we've got something to see timewise.
   DrawScreen( "Doing nothing for a couple of seconds" )
   DoNothingForTwoSeconds()

   // Make sure we've got something to see callwise.
   For n := 1 To 500
      CallMe500Times()
   Next

   // Take a profile snapshot.
   oProfile:gather()

   // Report on calls greater than 0
   DrawScreen( "All methods/functions called one or more times" )
   memoedit( HBProfileReportToString():new( oProfile:callSort() ):generate( {| o | o:nCalls > 0 } ), 1,,,, .F. )

   // Sorted by name
   DrawScreen( "All methods/functions called one or more times, sorted by name" )
   memoedit( HBProfileReportToString():new( oProfile:nameSort() ):generate( {| o | o:nCalls > 0 } ), 1,,,, .F. )

   // Sorted by time
   DrawScreen( "All methods/functions taking measurable time, sorted by time" )
   memoedit( HBProfileReportToString():new( oProfile:timeSort() ):generate( {| o | o:nTicks > 0 } ), 1,,,, .F. )

   // TBrowse all calls greater than 0
   DrawScreen( "TBrowse all methods/functions called one or more times" )
   Browser( HBProfileReportToTBrowse():new( oProfile:callSort() ):generate( {| o | o:nCalls > 0 }, 1 ) )

   // Some closing stats
   DrawScreen( "Totals" )
   @ 2, 0 Say "  Total Calls: " + str( oProfile:totalCalls() )
   @ 3, 0 Say "  Total Ticks: " + str( oProfile:totalTicks() )
   @ 4, 0 Say "Total Seconds: " + str( oProfile:totalSeconds() )

Return NIL

Static Function DrawScreen( cTitle )

   Scroll()

   @ 0, 0 SAY PadR( cTitle, MaxCol() + 1 ) COLOR "N/W"

Return NIL

Function DoNothingForTwoSeconds()

   Inkey( 2 )

Return NIL

Function CallMe500Times()
Return NIL

Static Function Browser( oBrowse )
Local lBrowsing := .T.
Local nKey

   Do While lBrowsing

      oBrowse:forceStable()

      nKey := Inkey( 0 )

      Do Case

         Case nKey == K_ESC
            lBrowsing := .F.

         Case nKey == K_DOWN
            oBrowse:down()

         Case nKey == K_UP
            oBrowse:up()

         Case nKey == K_LEFT
            oBrowse:left()

         Case nKey == K_RIGHT
            oBrowse:right()

         Case nKey == K_PGDN
            oBrowse:pageDown()

         Case nKey == K_PGUP
            oBrowse:pageUp()

         // And so on.... (not really necessary for this test)

      EndCase

   EndDo

Return NIL

#endif

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileEntity

Create Class HBProfileEntity

   Exported:

      Var cName    ReadOnly
      Var nCalls   ReadOnly
      Var nTicks   ReadOnly

      Access nSeconds
      Access nMeanTicks
      Access nMeanSeconds

      Method init
      Method describe

Endclass

/////

Method init( cName, aInfo ) Class HBProfileEntity

   ::cName  := cName
   ::nCalls := aInfo[ 1 ]
   ::nTicks := aInfo[ 2 ]

Return Self

/////

Access nSeconds Class HBProfileEntity
Return HB_Clocks2Secs( ::nTicks )

/////

Access nMeanTicks Class HBProfileEntity
Return iif( ::nCalls == 0, 0, ::nTicks / ::nCalls )

/////

Access nMeanSeconds Class HBProfileEntity
Return iif( ::nCalls == 0, 0, ::nSeconds / ::nCalls )

/////

Method describe Class HBProfileEntity
Return "Base Entity"

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileFunction

Create Class HBProfileFunction Inherit HBProfileEntity

   Exported:

      Method describe

Endclass

/////

Method describe Class HBProfileFunction
Return "Function"

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileMethod

Create Class HBProfileMethod Inherit HBProfileEntity

   Exported:

      Method describe

Endclass

/////

Method describe Class HBProfileMethod
Return "Method"

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileOPCode

Create Class HBProfileOPCode Inherit HBProfileEntity

   Exported:

      Method describe

Endclass

/////

Method describe Class HBProfileOPCode
Return "OPCode"

////////////////////////////////////////////////////////////////////////////
// Class: HBProfile

Create Class HBProfile

   Exported:

      Var aProfile

      Method init
      Method gather
      Method forEach
      Method sort
      Method nameSort
      Method callSort
      Method timeSort
      Method totalCalls
      Method totalTicks
      Method totalSeconds

   Protected:

      Method gatherFunctions
      Method gatherMethods
      Method reset
      Method ignoreSymbol

Endclass

/////

Method init Class HBProfile
Local lProfile := __setProfiler( .F. )

   ::reset()

   __setProfiler( lProfile )

Return Self

/////

Method reset Class HBProfile

   ::aProfile := {}

Return Self

/////

Method ignoreSymbol( cSymbol ) Class HBProfile
Local cProfPrefix := "HBPROFILE"
Return Left( cSymbol, Len( cProfPrefix ) ) == cProfPrefix .Or. cSymbol == "__SETPROFILER"

/////

Method gatherFunctions Class HBProfile
Local lProfile  := __setProfiler( .F. )
Local nSymCount := __DynSCount()
Local cName
Local n

   // For each known symbol.
   // TODO: Question: Will the symbol count have changed because
   //                 we've created variables?
   For n := 1 To nSymCount

      // Is the symbol a function?
      If __DynSIsFun( n )

         // If we're not ignoring the symbol...
         If !::ignoreSymbol( cName := __DynSGetName( n ) )
            // Yes, it is, add it to the profile.
            AAdd( ::aProfile, HBProfileFunction():new( cName, __DynSGetPrf( n ) ) )
         EndIf

      EndIf

   Next

   __setProfiler( lProfile )

Return Self

/////

Method gatherMethods Class HBProfile
Local lProfile  := __setProfiler( .F. )
Local n         := 1
Local cClass
Local nMembers
Local aMembers
Local nMember

   // For each class in the environment...
   Do While !Empty( cClass := __className( n ) )

      // If we're not ignoring the class' methods...
      If !::ignoreSymbol( cClass )

         // Collect class members.
         nMembers := Len( aMembers := __classSel( n ) )

         For nMember := 1 To nMembers

            // If we've got a member name...
            If !empty( aMembers[ nMember ] )
               // Add it to the profile.
               AAdd( ::aProfile, HBProfileMethod():new( cClass + ":" + aMembers[ nMember ], __GetMsgPrf( n, aMembers[ nMember ] ) ) )
            EndIf

         Next

      EndIf

      ++n

   EndDo

   __setProfiler( lProfile )

Return Self

/////

Method gather Class HBProfile
Local lProfile  := __setProfiler( .F. )

   // Reset the profile.
   ::reset()

   // Gather function calls
   ::gatherFunctions()

   // Gather method calls
   ::gatherMethods()   

   __setProfiler( lProfile )

Return Self

/////

Method forEach( b ) Class HBProfile
Local lProfile := __setProfiler( .F. )

   AEval( ::aProfile, b )

   __setProfiler( lProfile )

Return Self

/////

Method sort( b ) Class HBProfile
Local lProfile := __setProfiler( .F. )

   ASort( ::aProfile,,, b )

   __setProfiler( lProfile )

Return Self

/////

Method nameSort Class HBProfile
Local lProfile := __setProfiler( .F. )

   ::sort( {| oX, oY | oX:cName < oY:cName } )

   __setProfiler( lProfile )

Return Self

/////

Method callSort Class HBProfile
Local lProfile := __setProfiler( .F. )

   ::sort( {| oX, oY | oX:nCalls > oY:nCalls } )

   __setProfiler( lProfile )

Return Self

/////

Method timeSort Class HBProfile
Local lProfile := __setProfiler( .F. )

   ::sort( {| oX, oY | oX:nTicks > oY:nTicks } )

   __setProfiler( lProfile )

Return Self

/////

Method totalCalls Class HBProfile
Local lProfile := __setProfiler( .F. )
Local nCalls   := 0

   ::forEach( {| o | nCalls += o:nCalls } )

   __setProfiler( lProfile )

Return nCalls

/////

Method totalTicks Class HBProfile
Local lProfile := __setProfiler( .F. )
Local nTicks   := 0

   ::forEach( {| o | nTicks += o:nTicks } )

   __setProfiler( lProfile )

Return nTicks

/////

Method totalSeconds Class HBProfile
Local lProfile := __setProfiler( .F. )
Local nSeconds := 0

   ::forEach( {| o | nSeconds += o:nSeconds } )

   __setProfiler( lProfile )

Return nSeconds

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileLowLevel

Create Class HBProfileLowLevel Inherit HBProfile

   Exported:

      Method gather

   Protected:

      Method gatherOPCodes

Endclass

/////

Method gather Class HBProfileLowLevel
Local lProfile := __setProfiler( .F. )

   // Gather functions and methods.
   ::super:gather()

   // Also gather opcodes.   
   ::gatherOPCodes()

   __setProfiler( lProfile )

Return Self

/////

Method gatherOPCodes Class HBProfileLowLevel
Local nMax := __opcount()
Local cName
Local nOP

   // Loop over all the harbour OP codes. Note that they start at 0.
   For nOP := 0 To ( nMax - 1 )
      // If we're not ignoring this opcode.
      If !::ignoreSymbol( cName := "OPCODE( " + PadL( nOP, 3 ) + " )" )
         // Add it to the profile.
         AAdd( ::aProfile, HBProfileOPCode():new( cName, __OpGetPrf( nOP ) ) )
      EndIf
   Next

Return Self

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileReport

Create Class HBProfileReport

   Protected:

      Var oProfile

      Method writeLines
      Method header
      Method emitHeader
      Method line
      Method emitLine

   Exported:

      Method init
      Method generate

Endclass

/////

Method init( oProfile ) Class HBProfileReport
Local lProfile := __setProfiler( .F. )

   ::oProfile := oProfile

   __setProfiler( lProfile )

Return Self

/////

Method writeLines( aLines ) Class HBProfileReport

   AEval( aLines, {| c | QOut( c ) } )

Return Self

/////

Method header Class HBProfileReport
Return { "Name                                Type       Calls    Ticks       Seconds",;
         "=================================== ========== ======== =========== ===========" }

/////

Method emitHeader Class HBProfileReport

   ::writeLines( ::header() )

Return Self

/////

Method line( oEntity ) Class HBProfileReport
Return { PadR( oEntity:cName,      35 ) + " " + ;
         PadR( oEntity:describe(),  8 ) + " " + ;
         PadL( oEntity:nCalls,     10 ) + " " + ;
         PadL( oEntity:nTicks,     11 ) + " " + ;
         Str( oEntity:nSeconds,    11, 2 ) }

/////

Method emitLine( oEntity ) Class HBProfileReport

   ::writeLines( ::line( oEntity ) )

Return Self

/////

Method generate( bFilter ) Class HBProfileReport
Local lProfile := __setProfiler( .F. )

   Default bFilter To {|| .T. }

   ::emitHeader():oProfile:forEach( {| o | iif( Eval( bFilter, o ), ::emitLine( o ), NIL ) } )

   __setProfiler( lProfile )

Return Self

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileReportToFile

Create Class HBProfileReportToFile Inherit HBProfileReport

   Protected:

      Var hFile

      Method writeLines

   Exported:

      Method generate

Endclass

/////

Method writeLines( aLines ) Class HBProfileReportToFile

   If ::hFile != F_ERROR
      AEval( aLines, {| c | FWrite( ::hFile, c + HB_OSNewLine() ) } )
   EndIf

Return Self

/////

Method generate( bFilter, cFile ) Class HBProfileReportToFile
Local lProfile := __setProfiler( .F. )

   Default cFile To "hbprof.txt"

   If ( ::hFile := fcreate( cFile ) ) != F_ERROR
      ::super:generate( bFilter )
      fclose( ::hFile )
   Else
      // TODO: Throw an error
   EndIf

   __setProfiler( lProfile )

Return Self

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileReportToArray

Create Class HBProfileReportToArray Inherit HBProfileReport

   Protected:

      Var aReport

      Method writeLines

   Exported:

      Method generate

Endclass

/////

Method writeLines( aLines ) Class HBProfileReportToArray

   AEval( aLines, {| c | AAdd( ::aReport, c ) } )

Return Self

/////

Method generate( bFilter ) Class HBProfileReportToArray

   ::aReport := {}
   ::super:generate( bFilter )

Return ::aReport

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileReportToString

Create Class HBProfileReportToString Inherit HBProfileReportToArray

   Exported:

      Method generate

Endclass

/////

Method generate( bFilter ) Class HBProfileReportToString
Local cReport := ""

   AEval( ::super:generate( bFilter ), {| c | cReport += c + HB_OSNewLine() } )

Return cReport

////////////////////////////////////////////////////////////////////////////
// Class: HBProfileReportToTBrowse


Create Class HBProfileReportToTBrowse Inherit HBProfileReportToArray

   Protected:

      Var nEntity

      Method emitHeader
      Method emitLine
      Method addColumns

   Exported:

      Method generate
      Method currentEntity

Endclass

/////

Method emitHeader Class HBProfileReportToTBrowse

   // No header required.

Return Self

/////

Method emitLine( oEntity ) Class HBProfileReportToTBrowse

   // Don't "emit" anything, simply add the entity to the array.
   AAdd( ::aReport, oEntity )

Return Self

/////

Method generate( bFilter, nTop, nLeft, nBottom, nRight ) Class HBProfileReportToTBrowse
Local lProfile := __setProfiler( .F. )
Local oBrowse

   // Start with the first entity.
   ::nEntity := 1

   // Generate the array.
   ::super:generate( bFilter )

   // Build the browse.
   oBrowse := TBrowseNew( nTop, nLeft, nBottom, nRight )

   oBrowse:goTopBlock    := {|| ::nEntity := 1 }
   oBrowse:goBottomBlock := {|| ::nEntity := Len( ::aReport ) }
   oBrowse:skipBlock     := {| nSkip, nPos | nPos := ::nEntity,                                   ;
                                             ::nEntity := iif( nSkip > 0,                         ;
                                                      Min( Len( ::aReport ), ::nEntity + nSkip ), ;
                                                      Max( 1, ::nEntity + nSkip ) ), ::nEntity - nPos }

   ::addColumns( oBrowse )

   __setProfiler( lProfile )

Return oBrowse

/////

Method addColumns( oBrowse ) Class HBProfileReportToTBrowse

   oBrowse:addColumn( TBColumnNew( "Name",         {|| PadR( ::currentEntity():cName,        35    ) } ) )
   oBrowse:addColumn( TBColumnNew( "Type",         {|| PadR( ::currentEntity():describe(),    8    ) } ) )
   oBrowse:addColumn( TBColumnNew( "Calls",        {|| PadL( ::currentEntity():nCalls,       10    ) } ) )
   oBrowse:addColumn( TBColumnNew( "Ticks",        {|| PadL( ::currentEntity():nTicks,       11    ) } ) )
   oBrowse:addColumn( TBColumnNew( "Seconds",      {|| Str(  ::currentEntity():nSeconds,     11, 2 ) } ) )
   oBrowse:addColumn( TBColumnNew( "Mean;Ticks",   {|| Str(  ::currentEntity():nMeanTicks,   11, 2 ) } ) )
   oBrowse:addColumn( TBColumnNew( "Mean;Seconds", {|| Str(  ::currentEntity():nMeanSeconds, 11, 2 ) } ) )

Return Self

/////

Method currentEntity Class HBProfileReportToTBrowse
Return ::aReport[ ::nEntity ]

/*
 * profiler.prg ends here.
 */
