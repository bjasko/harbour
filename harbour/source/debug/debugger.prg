/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * The Debugger
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version, with one exception:
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

/* NOTE: Don't use SAY/DevOut()/DevPos() for screen output, otherwise
         the debugger output may interfere with the applications output
         redirection, and is also slower. [vszakats] */
#pragma -es0
#include "hbclass.ch"
#include "hbmemvar.ch"
#include "box.ch"
#include "inkey.ch"
#include "common.ch"
#include "setcurs.ch"

#define ALTD_DISABLE   0
#define ALTD_ENABLE    1

static s_oDebugger
static s_lExit := .F.
Static nDump
memvar __DbgStatics

procedure AltD( nAction )
   do case
      case nAction == nil
           if SET( _SET_DEBUG )
              s_lExit := .f.
              __dbgEntry( ProcLine( 1 ) )
           endif

      case nAction == ALTD_DISABLE
           SET( _SET_DEBUG, .F. )

      case nAction == ALTD_ENABLE
           SET( _SET_DEBUG, .T. )
   endcase

return

procedure __dbgEntry( uParam1, uParam2, uParam3 )  // debugger entry point

   local cModuleName
   local nStaticsBase, nStaticIndex, cStaticName
   local cLocalName, nLocalIndex
   local nAt

   do case
      case ValType( uParam1 ) == "C"   // called from hvm.c hb_vmModuleName()
           cModuleName := uParam1
           if ! s_lExit
              if s_oDebugger == nil
                 s_oDebugger := TDebugger():New()
                 s_oDebugger:Activate( cModuleName )
              else
                 if ! s_oDebugger:lTrace
                    s_oDebugger:ShowCode( cModuleName )
                 else
                    ASize( s_oDebugger:aCallStack, Len( s_oDebugger:aCallStack ) + 1 )
                    AIns( s_oDebugger:aCallStack, 1 )
                 endif
                 s_oDebugger:LoadVars()
              endif
           endif

      case ValType( uParam1 ) == "N"   // called from hvm.c both hb_vmDebuggerShowLines()
           public __DbgStatics         // hb_vmStaticName() and hb_vmLocalName()
           if Type( "__DbgStatics" ) == "L"
              __DbgStatics := {}
           endif

           if ProcName( 1 ) == "(_INITSTATICS)"
              nStaticsBase := uParam1
              cStaticName  := uParam2
              if AScan( __DbgStatics, { | a | a[ 1 ] == nStaticsBase } ) == 0
                 AAdd( __DbgStatics, { nStaticsBase, { cStaticName } } )
              else
                 AAdd( ATail( __DbgStatics )[ 2 ], cStaticName )
              endif
              return  // We can not use s_oDebugger yet, so we return
           endif

           if s_oDebugger != nil
              if PCount() == 3 // called from hvm.c hb_vmLocalName() and hb_vmStaticName()
                 if uParam3 == 1 // in-function static variable
                    cStaticName  := uParam2
                    nStaticIndex := uParam1
                    if s_oDebugger:lShowStatics
                       if ( nAt := AScan( s_oDebugger:aVars,; // Is there another var with this name ?
                            { | aVar | aVar[ 1 ] == cStaticName } ) ) != 0
                          s_oDebugger:aVars[ nAt ] := { cStaticName, nStaticIndex, "Static" }
                       else
                          AAdd( s_oDebugger:aVars, { cStaticName, nStaticIndex, "Static" } )
                       endif
                    endif
                 else            // local variable
                    cLocalName  := uParam2
                    nLocalIndex := uParam1
                    if s_oDebugger:lShowLocals
                       if ( nAt := AScan( s_oDebugger:aVars,; // Is there another var with this name ?
                            { | aVar | aVar[ 1 ] == cLocalName } ) ) != 0
                          s_oDebugger:aVars[ nAt ] := { cLocalName, nLocalIndex, "Local", ProcName( 1 ) }
                       else
                          AAdd( s_oDebugger:aVars, { cLocalName, nLocalIndex, "Local", ProcName( 1 ) } )
                       endif
                    endif
                    AAdd( s_oDebugger:aCallStack[ 1 ][ 2 ], { cLocalName, nLocalIndex, "Local", ProcName( 1 ) } )
                 endif
                 if s_oDebugger:oBrwVars != nil
                    s_oDebugger:oBrwVars:RefreshAll()
                 endif
                 return
              endif

              if s_oDebugger:lTrace
                 if s_oDebugger:nTraceLevel < Len( s_oDebugger:aCallStack )
                    return
                 else
                    s_oDebugger:lTrace := .f.
                 endif
              endif

              if s_oDebugger:lGo
                 s_oDebugger:lGo := ! s_oDebugger:IsBreakPoint( uParam1 )
              endif

              if ! s_oDebugger:lGo .or. InvokeDebug() .or. ;
                 ProcName( 1 ) == "ALTD"  // debugger invoked from AltD( 1 )
                 s_oDebugger:lGo := .F.
                 s_oDebugger:SaveAppStatus()
                 s_oDebugger:GoToLine( uParam1 )
                 s_oDebugger:HandleEvent()
              endif
           endif

      otherwise   // called from hvm.c hb_vmDebuggerEndProc()
         if Empty( ProcName( 1 ) ) // ending (_INITSTATICS)
            return
         endif
         if s_oDebugger != nil
            s_oDebugger:EndProc()
            s_oDebugger:LoadVars()
         endif
   endcase

return

CLASS TDebugger

   DATA   aWindows, nCurrentWindow
   DATA   oPullDown
   DATA   oWndCode, oWndCommand, oWndStack, oWndVars
   DATA   oBar, oBrwText, cPrgName, oBrwStack, oBrwVars, aVars
   DATA   cImage
   DATA   cAppImage, nAppRow, nAppCol, cAppColors, nAppCursor
   DATA   aBreakPoints, aCallStack, aColors
   DATA   aLastCommands, nCommand, oGetListCommand
   DATA   lAnimate, lEnd, lGo, lTrace, lCaseSensitive, lMonoDisplay, lSortVars
   DATA   cSearchString, cPathForFiles, cSettingsFileName
   DATA   nTabWidth, nSpeed
   DATA   lShowPublics, lShowPrivates, lShowStatics, lShowLocals, lAll
   DATA   nTraceLevel

   METHOD New()
   METHOD Activate( cModuleName )

   METHOD All()

   METHOD Animate() INLINE ::lAnimate := .t., ::Step()

   METHOD BarDisplay()
   METHOD BuildCommandWindow()

   METHOD ClrModal() INLINE iif( ::lMonoDisplay, "N/W, W+/W, W/N, W+/N",;
                                "N/W, R/W, N/BG, R/BG" )

   METHOD CodeWindowProcessKey( nKey )
   METHOD Colors()
   METHOD CommandWindowProcessKey( nKey )
   METHOD EditColor( nColor, oBrwColors )
   METHOD EditSet( nSet, oBrwSets )
   METHOD EditVar( nVar )
   METHOD EndProc()
   METHOD Exit() INLINE ::lEnd := .t.
   METHOD Go() INLINE ::RestoreAppStatus(), ::lGo := .t., DispEnd(), ::Exit()
   METHOD GoToLine( nLine )
   METHOD HandleEvent()
   METHOD Hide()
   METHOD HideVars()
   METHOD InputBox( cMsg, uValue, bValid )
   METHOD IsBreakPoint( nLine )
   METHOD LoadSettings()
   METHOD LoadVars()

   METHOD Local()

   METHOD MonoDisplay()
   METHOD NextWindow()
   METHOD Open()
   METHOD OSShell()

   METHOD PathForFiles() INLINE ;
          ::cPathForFiles := ::InputBox( "Search path for source files:",;
                                         ::cPathForFiles )

   METHOD PrevWindow()
   METHOD Private()
   METHOD Public()
   METHOD RestoreAppStatus()
   METHOD RestoreSettings()
   METHOD SaveAppStatus()
   METHOD SaveSettings()
   METHOD Show()
   METHOD ShowAppScreen()
   METHOD ShowCallStack()
   METHOD ShowCode( cModuleName )
   METHOD ShowHelp( nTopic ) INLINE __dbgHelp( nTopic )
   METHOD ShowVars()

/*   METHOD Sort() INLINE ASort( ::aVars,,, {|x,y| x[1] < y[1] } ),;
                        ::lSortVars := .t.,;
                        iif( ::oBrwVars != nil, ::oBrwVars:RefreshAll(), nil ),;
          iif( ::oWndVars != nil .and. ::oWndVars:lVisible, ::oBrwVars:ForceStable(),)*/
   METHOD Sort() INLINE ASort( ::aVars,,, {|x,y| x[1] < y[1] } ),;
                        ::lSortVars := .t.,;
                        iif( ::oBrwVars != nil, ::oBrwVars:RefreshAll(), nil ),;
          iif( ::oWndVars != nil .and. ::oWndVars:lVisible, iif(!::lGo,::oBrwVars:ForceStable(),),)

   METHOD Speed() INLINE ;
          ::nSpeed := ::InputBox( "Step delay (in tenths of a second)",;
                                  ::nSpeed )

   METHOD Static()

   METHOD Step() INLINE ::RestoreAppStatus(), ::Exit()

   METHOD TabWidth() INLINE ;
          ::nTabWidth := ::InputBox( "Tab width", ::nTabWidth )

   METHOD ToggleBreakPoint()

   METHOD Trace() INLINE ::lTrace := .t., ::nTraceLevel := Len( ::aCallStack ),;
                         __Keyboard( Chr( 255 ) ) //forces a Step()

   METHOD ViewSets()
   METHOD WndVarsLButtonDown( nMRow, nMCol )
   METHOD LineNumbers()          // Toggles numbering of source code lines
   METHOD Locate()
   METHOD FindNext()
   METHOD FindPrevious()
   METHOD SearchLine()
   METHOD ToggleCaseSensitive() INLINE ::lCaseSensitive := !::lCaseSensitive
   METHOD ShowWorkAreas() INLINE __dbgShowWorkAreas( Self )

ENDCLASS


METHOD New() CLASS TDebugger

   s_oDebugger := Self

   ::aColors := {"W+/BG","N/BG","R/BG","N+/BG","W+/B","GR+/B","W/B","N/W","R/W","N/BG","R/BG"}
   ::lMonoDisplay      := .f.
   ::aWindows          := {}
   ::nCurrentWindow    := 1
   ::lAnimate          := .f.
   ::lEnd              := .f.
   ::lTrace            := .f.
   ::aBreakPoints      := {}
   ::aCallStack        := {}
   ::lGo               := .f.
   ::aVars             := {}
   ::lCaseSensitive    := .f.
   ::cSearchString     := ""
   ::cPathForFiles     := ""
   ::nTabWidth         := 4
   ::nSpeed            := 0
   ::lShowPublics      := .f.
   ::lShowPrivates     := .f.
   ::lShowStatics      := .f.
   ::lShowLocals       := .f.
   ::lAll              := .f.
   ::lSortVars         := .f.

   ::cSettingsFileName := "init.cld"
   if File( ::cSettingsFileName )
      ::LoadSettings()
   endif

   ::oPullDown      := __dbgBuildMenu( Self )

   ::oWndCode       := TDbWindow():New( 1, 0, MaxRow() - 6, MaxCol() )
   ::oWndCode:Cargo       := { ::oWndCode:nTop, ::oWndCode:nLeft }
   ::oWndCode:bKeyPressed := { | nKey | ::CodeWindowProcessKey( nKey ) }
   ::oWndCode:bGotFocus   := { || ::oGetListCommand:SetFocus(), SetCursor( SC_SPECIAL1 ), ;
                              SetPos( ::oWndCode:Cargo[1],::oWndCode:Cargo[2] ) }
   ::oWndCode:bLostFocus  := { || ::oWndCode:Cargo[1] := Row(), ::oWndCode:Cargo[2] := Col(), ;
                              SetCursor( SC_NONE ) }
   /*
   ::oWndCode:bPainted := { || ::oBrwText:SetColor( __DbgColors()[ 2 ] + "," + ;
                               __DbgColors()[ 5 ] + "," + __DbgColors()[ 3 ] + "," + ;
                               __DbgColors()[ 6 ] ),;
                               iif( ::oBrwText != nil, ::oBrwText:RefreshWindow(), nil ) }
   */

   AAdd( ::aWindows, ::oWndCode )

   ::BuildCommandWindow()

return Self

METHOD Activate( cModuleName ) CLASS TDebugger

   ::Show(.t.)
   ::ShowCode( cModuleName )
   ::ShowCallStack()
   ::ShowVars()
   ::RestoreAppStatus()

return nil

METHOD All() CLASS TDebugger

   ::lShowPublics := ::lShowPrivates := ::lShowStatics := ;
   ::lShowLocals := ::lAll := ! ::lAll

   iif( ::lAll, ::ShowVars(), ::HideVars() )

return nil

METHOD BarDisplay() CLASS TDebugger

   local cClrItem   := __DbgColors()[ 8 ]
   local cClrHotKey := __DbgColors()[ 9 ]

   DispBegin()
   SetColor( cClrItem )
   @ MaxRow(), 0 CLEAR TO MaxRow(), MaxCol()

   DispOutAt( MaxRow(),  0,;
   "F1-Help F2-Zoom F3-Repeat F4-User F5-Go F6-WA F7-Here F8-Step F9-BkPt F10-Trace",;
   cClrItem )
   DispOutAt( MaxRow(),  0, "F1", cClrHotKey )
   DispOutAt( MaxRow(),  8, "F2", cClrHotKey )
   DispOutAt( MaxRow(), 16, "F3", cClrHotKey )
   DispOutAt( MaxRow(), 26, "F4", cClrHotKey )
   DispOutAt( MaxRow(), 34, "F5", cClrHotKey )
   DispOutAt( MaxRow(), 40, "F6", cClrHotKey )
   DispOutAt( MaxRow(), 46, "F7", cClrHotKey )
   DispOutAt( MaxRow(), 54, "F8", cClrHotKey )
   DispOutAt( MaxRow(), 62, "F9", cClrHotKey )
   DispOutAt( MaxRow(), 70, "F10", cClrHotKey )
   DispEnd()

return nil

METHOD BuildCommandWindow() CLASS TDebugger

   local GetList := {}, oGet
   local cCommand

   ::oWndCommand := TDbWindow():New( MaxRow() - 5, 0, MaxRow() - 1, MaxCol(),;
                                    "Command" )

   ::oWndCommand:bGotFocus   := { || ::oGetListCommand:SetFocus(), SetCursor( SC_NORMAL ) }
   ::oWndCommand:bLostFocus  := { || SetCursor( SC_NONE ) }
   ::oWndCommand:bKeyPressed := { | nKey | ::CommandWindowProcessKey( nKey ) }
   ::oWndCommand:bPainted    := { || DispOutAt( ::oWndCommand:nBottom - 1,;
                             ::oWndCommand:nLeft + 1, "> ", __DbgColors()[ 2 ] ),;
                        oGet:ColorDisp( Replicate( __DbgColors()[ 2 ] + ",", 5 ) ),;
                        hb_ClrArea( ::oWndCommand:nTop + 1, ::oWndCommand:nLeft + 1,;
                        ::oWndCommand:nBottom - 2, ::oWndCommand:nRight - 1,;
                        iif( ::lMonoDisplay, 15, HB_ColorToN( __DbgColors()[ 2 ] ) ) ) }
   AAdd( ::aWindows, ::oWndCommand )

   ::aLastCommands := {}
   ::nCommand := 0

   cCommand := Space( ::oWndCommand:nRight - ::oWndCommand:nLeft - 3 )
   // We don't use the GET command here to avoid the painting of the GET
   AAdd( GetList, oGet := Get():New( ::oWndCommand:nBottom - 1, ::oWndCommand:nLeft + 3,;
         { | u | iif( PCount() > 0, cCommand := u, cCommand ) }, "cCommand" ) )
   oGet:ColorSpec := Replicate( __DbgColors()[ 2 ] + ",", 5 )
   ::oGetListCommand := HBGetList():New( GetList )

return nil

METHOD CodeWindowProcessKey( nKey ) CLASS TDebugger

   do case
      case nKey == K_HOME
           ::oBrwText:GoTop()

      case nKey == K_END
           ::oBrwText:GoBottom()

      case nKey == K_LEFT
           ::oBrwText:Left()

      case nKey == K_RIGHT
           ::oBrwText:Right()

      case nKey == K_UP
           ::oBrwText:Up()

      case nKey == K_DOWN
           ::oBrwText:Down()

      case nKey == K_PGUP
           ::oBrwText:PageUp()

      case nKey == K_PGDN
           ::oBrwText:PageDown()

   endcase

return nil

METHOD Colors() CLASS TDebugger

   local oWndColors := TDbWindow():New( 4, 5, 16, MaxCol() - 5,;
                                        "Debugger Colors[1..11]", ::ClrModal() )
   local aColors := { "Border", "Text", "Text High", "Text PPO", "Text Selected",;
                      "Text High Sel.", "Text PPO Sel.", "Menu", "Menu High",;
                      "Menu Selected", "Menu High Sel." }

   local oBrwColors := TBrowseNew( oWndColors:nTop + 1, oWndColors:nLeft + 1,;
                                 oWndColors:nBottom - 1, oWndColors:nRight - 1 )
   local n := 1
   local nWidth := oWndColors:nRight - oWndColors:nLeft - 1
   local oCol

   if ::lMonoDisplay
      Alert( "Monochrome display" )
      return nil
   endif
   oBrwColors:Cargo :={ 1,{}} // Actual highligthed row
   oBrwColors:ColorSpec := ::ClrModal()
   oBrwColors:GOTOPBLOCK := { || oBrwColors:cargo[ 1 ]:= 1 }
   oBrwColors:GoBottomBlock := { || oBrwColors:cargo[ 1 ]:= Len(oBrwColors:cargo[ 2 ][ 1 ])}
   oBrwColors:SkipBlock := { |nPos| ( nPos:= ArrayBrowseSkip(nPos, oBrwColors), oBrwColors:cargo[ 1 ]:= ;
   oBrwColors:cargo[ 1 ] + nPos,nPos ) }

   oBrwColors:AddColumn( ocol := TBColumnNew( "", { || PadR( aColors[ oBrwColors:Cargo[1] ], 14 ) } ) )
   oCol:DefColor:={1,2}
   aadd(oBrwColors:Cargo[2],acolors)
   oBrwColors:AddColumn( oCol := TBColumnNew( "",;
                       { || PadR( '"' + ::aColors[ oBrwColors:Cargo[1] ] + '"', nWidth - 15 ) } ) )
   aadd(oBrwColors:Cargo[2],acolors)
   oCol:DefColor:={1,3}
   ocol:width:=50
   oBrwColors:autolite:=.f.

   oWndColors:bPainted    := { || oBrwColors:ForceStable(),RefreshVarsS(oBrwColors)}

   oWndColors:bKeyPressed := { | nKey | SetsKeyPressed( nKey, oBrwColors,;
                               Len( aColors ), oWndColors, "Debugger Colors",;
                               { || ::EditColor( oBrwColors:Cargo[1], oBrwColors ) } ) }
   oWndColors:ShowModal()

   ::oPullDown:LoadColors()
   ::oPullDown:Refresh()
   ::BarDisplay()

   for n := 1 to Len( ::aWindows )
      ::aWindows[ n ]:LoadColors()
      ::aWindows[ n ]:Refresh()
   next

return nil

METHOD CommandWindowProcessKey( nKey ) CLASS TDebugger

   local cCommand, cResult, oE
   local bLastHandler

   do case
      case nKey == K_UP
           if ::nCommand > 0
              ::oGetListCommand:oGet:VarPut( ::aLastCommands[ ::nCommand ] )
              ::oGetListCommand:oGet:Buffer := ::aLastCommands[ ::nCommand ]
              ::oGetListCommand:oGet:Pos := 1
              ::oGetListCommand:oGet:Display()
              if ::nCommand > 1
                 ::nCommand--
              endif
           endif

      case nKey == K_DOWN
           if ::nCommand > 0 .AND. ::nCommand <= Len( ::aLastCommands )
              ::oGetListCommand:oGet:VarPut( ::aLastCommands[ ::nCommand ] )
              ::oGetListCommand:oGet:Buffer := ::aLastCommands[ ::nCommand ]
              ::oGetListCommand:oGet:Pos := 1
              ::oGetListCommand:oGet:Display()
              if ::nCommand < Len( ::aLastCommands )
                 ::nCommand++
              endif
           endif

      case nKey == K_ENTER
           cCommand := ::oGetListCommand:oGet:VarGet()
           AAdd( ::aLastCommands, cCommand )
           ::nCommand++
           ::oWndCommand:ScrollUp( 1 )

           bLastHandler := ErrorBlock({ |objErr| BREAK (objErr) })

           if SubStr( LTrim( cCommand ), 1, 2 ) == "? "
              begin sequence
                  cResult := ValToStr( &( AllTrim( SubStr( LTrim( cCommand ), 3 ) ) ) )

              recover using oE
                  cResult := "Command error: " + oE:description

              end sequence

           else
              cResult := "Command error"

           endif

           ErrorBlock(bLastHandler)

           DispOutAt( ::oWndCommand:nBottom - 1, ::oWndCommand:nLeft + 1,;
              Space( ::oWndCommand:nRight - ::oWndCommand:nLeft - 1 ),;
              __DbgColors()[ 2 ] )
           DispOutAt( ::oWndCommand:nBottom - 1, ::oWndCommand:nLeft + 3, cResult,;
              __DbgColors()[ 2 ] )
           ::oWndCommand:ScrollUp( 1 )
           DispOutAt( ::oWndCommand:nBottom - 1, ::oWndCommand:nLeft + 1, "> ",;
              __DbgColors()[ 2 ] )
           cCommand := Space( ::oWndCommand:nRight - ::oWndCommand:nLeft - 3 )
           ::oGetListCommand:oGet:VarPut( cCommand )
           ::oGetListCommand:oGet:Buffer := cCommand
           ::oGetListCommand:oGet:Pos := 1
           ::oGetListCommand:oGet:Display()

      otherwise
          ::oGetListCommand:GetApplyKey( nKey )
   endcase

return nil

METHOD EditColor( nColor, oBrwColors ) CLASS TDebugger

   local GetList    := {}
   local lPrevScore := Set( _SET_SCOREBOARD, .f. )
   local lPrevExit  := Set( _SET_EXIT, .t. )
   local cColor     := PadR( '"' + ::aColors[ nColor ] + '"',;
                             oBrwColors:aColumns[ 2 ]:Width )

   oBrwColors:RefreshCurrent()
   oBrwColors:ForceStable()

   SetCursor( SC_NORMAL )
   @ Row(), Col() + 15 GET cColor COLOR SubStr( ::ClrModal(), 5 ) ;
      VALID iif( Type( cColor ) != "C", ( Alert( "Must be string" ), .f. ), .t. )

   READ
   SetCursor( SC_NONE )

   Set( _SET_SCOREBOARD, lPrevScore )
   Set( _SET_EXIT, lPrevExit )

   if LastKey() != K_ESC
      ::aColors[ nColor ] := &cColor
   endif

   oBrwColors:RefreshCurrent()
   oBrwColors:ForceStable()

return nil

METHOD EditSet( nSet, oBrwSets ) CLASS TDebugger

   local GetList    := {}
   local lPrevScore := Set( _SET_SCOREBOARD, .f. )
   local lPrevExit  := Set( _SET_EXIT, .t. )
   local cSet       := PadR( ValToStr( Set( nSet ) ), oBrwSets:aColumns[ 2 ]:Width )

   oBrwSets:RefreshCurrent()
   oBrwSets:ForceStable()

   SetCursor( SC_NORMAL )
   @ Row(), Col() GET cSet COLOR SubStr( ::ClrModal(), 5 )

   READ
   SetCursor( SC_NONE )

   Set( _SET_SCOREBOARD, lPrevScore )
   Set( _SET_EXIT, lPrevExit )

   if LastKey() != K_ESC
      Set( nSet, &cSet )
   endif

   oBrwSets:RefreshCurrent()
   oBrwSets:ForceStable()

return nil

METHOD EditVar( nVar ) CLASS TDebugger

   local cVarName   := ::aVars[ nVar ][ 1 ]
   local uVarValue  := ::aVars[ nVar ][ 2 ]
   local cVarType   := ::aVars[ nVar ][ 3 ]
   local nProcLevel := 1
   local aArray

   if ::aVars[ nVar ][ 3 ] == "Local"
      while ProcName( nProcLevel ) != ::aVars[ nVar ][ 4 ]
         nProcLevel++
      end
      uVarValue := __vmVarLGet( nProcLevel, ::aVars[ nVar ][ 2 ] )
   endif

   if ::aVars[ nVar ][ 3 ] == "Static"
      uVarValue := __vmVarSGet( ::aVars[ nVar ][ 2 ] )
   endif

   uVarValue := ::InputBox( cVarName, ValToStr( uVarValue ) )

   if LastKey() != K_ESC
      do case
         case uVarValue == "{ ... }"
               cVarType := ::aVars[ nVar ][ 3 ]

               do case
                  case cVarType == "Local"
                     aArray := __vmVarLGet( nProcLevel, ::aVars[ nVar ][ 2 ] )

                  case cVarType == "Static"
                     aArray := __vmVarSGet( ::aVars[ nVar ][ 2 ] )

                  otherwise
                     aArray := ::aVars[ nVar ][ 2 ]
               endcase

               if Len( aArray ) > 0
                  __DbgArrays( aArray, cVarName )
               else
                  Alert( "Array is empty" )
               endif

         case Upper( SubStr( uVarValue, 1, 5 ) ) == "CLASS"
              do case
                 case cVarType == "Local"
                    __DbgObject( __vmVarLGet( nProcLevel, ::aVars[ nVar ][ 2 ] ), cVarName )

                 case cVarType == "Static"
                    __DbgObject( __vmVarSGet( ::aVars[ nVar ][ 2 ] ), cVarName )

                 otherwise
                    __DbgObject( ::aVars[ nVar ][ 2 ], cVarName )
              endcase

         otherwise
            do case
               case cVarType == "Local"
                  __vmVarLSet( nProcLevel, ::aVars[ nVar ][ 2 ], &uVarValue )

               case cVarType == "Static"
                  __vmVarSSet( ::aVars[ nVar ][ 2 ], &uVarValue )

               otherwise
                  ::aVars[ nVar ][ 2 ] := &uVarValue
                  &( ::aVars[ nVar ][ 1 ] ) := ::aVars[ nVar ][ 2 ]
            endcase
      endcase
   endif

   ::oBrwVars:RefreshCurrent()
   ::oBrwVars:ForceStable()

return nil

METHOD EndProc() CLASS TDebugger

   if Len( ::aCallStack ) > 1
      ADel( ::aCallStack, 1 )
      ASize( ::aCallStack, Len( ::aCallStack ) - 1 )
      if ::oBrwStack != nil .and. ! ::lTrace
         ::oBrwStack:RefreshAll()
      endif
   endif

return nil

METHOD HandleEvent() CLASS TDebugger

   local nPopup, oWnd
   local nKey, nMRow, nMCol, n

   if ::lAnimate
      if ::nSpeed != 0
         Inkey( ::nSpeed / 10 )
      endif
      if NextKey() == K_ALT_D
         ::lAnimate := .f.
      endif
      KEYBOARD Chr( 255 ) // Forces a Step(). Only 0-255 range is supported
   endif

   ::lEnd := .f.

   while ! ::lEnd

      nKey := InKey( 0, INKEY_ALL )

      do case
         case nKey == K_ALT_X
              s_oDebugger:Exit()
              s_oDebugger:Hide()
              __Quit()

         case ::oPullDown:IsOpen()
              ::oPullDown:ProcessKey( nKey )
              if ::oPullDown:nOpenPopup == 0 // Closed
                 ::aWindows[ ::nCurrentWindow ]:SetFocus( .t. )
              endif

         case nKey == K_LDBLCLK
              if MRow() == 0

              elseif MRow() == MaxRow()

              else
                 nMRow := MRow()
                 nMCol := MCol()
                 for n := 1 to Len( ::aWindows )
                    if ::aWindows[ n ]:IsOver( nMRow, nMCol )
                       if ! ::aWindows[ n ]:lFocused
                          ::aWindows[ ::nCurrentWindow ]:SetFocus( .f. )
                          ::nCurrentWindow := n
                          ::aWindows[ n ]:SetFocus( .t. )
                       endif
                       ::aWindows[ n ]:LDblClick( nMRow, nMCol )
                       n := Len( ::aWindows ) + 1
                    endif
                 next
              endif

         case nKey == K_LBUTTONDOWN
              if MRow() == 0
                 if ( nPopup := ::oPullDown:GetItemOrdByCoors( 0, MCol() ) ) != 0
                    SetCursor( SC_NONE )
                    ::oPullDown:ShowPopup( nPopup )
                 endif

              elseif MRow() == MaxRow()

              else
                 nMRow := MRow()
                 nMCol := MCol()
                 for n := 1 to Len( ::aWindows )
                    if ::aWindows[ n ]:IsOver( nMRow, nMCol )
                       if ! ::aWindows[ n ]:lFocused
                          ::aWindows[ ::nCurrentWindow ]:SetFocus( .f. )
                          ::nCurrentWindow := n
                          ::aWindows[ n ]:SetFocus( .t. )
                       endif
                       ::aWindows[ n ]:LButtonDown( nMRow, nMCol )
                       n := Len( ::aWindows ) + 1
                    endif
                 next
              endif

         case nKey == K_RBUTTONDOWN

         case nKey == K_ESC
              ::RestoreAppStatus()
              s_oDebugger := nil
              s_lExit := .T.
              DispEnd()
              ::Exit()

         case nKey == K_UP .or. nKey == K_DOWN .or. nKey == K_HOME .or. ;
              nKey == K_END .or. nKey == K_ENTER .or. nKey == K_PGDN .or. nKey == K_PGUP
              oWnd := ::aWindows[ ::nCurrentWindow ]
              oWnd:KeyPressed( nKey )


         case nKey == K_F1
              ::ShowHelp()

         case nKey == K_F4
              ::ShowAppScreen()

         case nKey == K_F5
              ::Go()

         case nKey == K_F6
              ::ShowWorkAreas()

         case nKey == K_F8 .or. nKey == 255
              ::Step()

         case nKey == K_F9
              ::ToggleBreakPoint()

         case nKey == K_F10
              ::Trace()

         case nKey == K_TAB
              ::NextWindow()

         case nKey == K_SH_TAB
              ::PrevWindow()

         case ::oWndCommand:lFocused .and. nKey < 272 // Alt
              ::oWndCommand:KeyPressed( nKey )

         otherwise
              if ( nPopup := ::oPullDown:GetHotKeyPos( __dbgAltToKey( nKey ) ) ) != 0
                 if ::oPullDown:nOpenPopup != nPopup
                    SetCursor( SC_NONE )
                    ::oPullDown:ShowPopup( nPopup )
                 endif
              endif
      endcase
   end

return nil

METHOD Hide() CLASS TDebugger

   RestScreen( ,,,, ::cAppImage )
   ::cAppImage := nil
   SetColor( ::cAppColors )
   SetCursor( ::nAppCursor )

return nil

METHOD MonoDisplay() CLASS TDebugger

   local n

   ::lMonoDisplay := ! ::lMonoDisplay

   ::oPullDown:LoadColors()
   ::oPullDown:Refresh()

   ::BarDisplay()

   for n := 1 to Len( ::aWindows )
      ::aWindows[ n ]:LoadColors()
      ::aWindows[ n ]:Refresh()
   next

return nil

METHOD NextWindow() CLASS TDebugger

   local oWnd

   if Len( ::aWindows ) > 0
      oWnd := ::aWindows[ ::nCurrentWindow++ ]
      oWnd:SetFocus( .f. )
      if ::nCurrentWindow > Len( ::aWindows )
         ::nCurrentWindow := 1
      endif
      while ! ::aWindows[ ::nCurrentWindow ]:lVisible
         ::nCurrentWindow++
         if ::nCurrentWindow > Len( ::aWindows )
            ::nCurrentWindow := 1
         endif
      end
      oWnd := ::aWindows[ ::nCurrentWindow ]
      oWnd:SetFocus( .t. )
   endif

return nil

METHOD PrevWindow() CLASS TDebugger

   local oWnd

   if Len( ::aWindows ) > 0
      oWnd := ::aWindows[ ::nCurrentWindow-- ]
      oWnd:SetFocus( .f. )
      if ::nCurrentWindow < 1
         ::nCurrentWindow := Len( ::aWindows )
      endif
      while ! ::aWindows[ ::nCurrentWindow ]:lVisible
         ::nCurrentWindow--
         if ::nCurrentWindow < 1
            ::nCurrentWindow := Len( ::aWindows )
         endif
      end
      oWnd := ::aWindows[ ::nCurrentWindow ]
      oWnd:SetFocus( .t. )
   endif

return nil

METHOD Show() CLASS TDebugger

   ::cAppImage  := SaveScreen()
   ::nAppRow    := Row()
   ::nAppCol    := Col()
   ::cAppColors := SetColor()
   ::nAppCursor := SetCursor( SC_NONE )

   ::oPullDown:Display()
   ::oWndCode:Show( .t. )
   ::oWndCommand:Show()
   DispOutAt( ::oWndCommand:nBottom - 1, ::oWndCommand:nLeft + 1, ">" )

   ::BarDisplay()

return nil

METHOD ShowAppScreen() CLASS TDebugger

   ::cImage := SaveScreen()
   RestScreen( 0, 0, MaxRow(), MaxCol(), ::cAppImage )

   if LastKey() == K_LBUTTONDOWN
      InKey( 0, INKEY_ALL )
      InKey( 0, INKEY_ALL )
   else
      InKey( 0, INKEY_ALL )
   endif

   while LastKey() == K_MOUSEMOVE
      InKey( 0, INKEY_ALL )
   end
   RestScreen( 0, 0, MaxRow(), MaxCol(), ::cImage )

return nil

METHOD ShowCallStack() CLASS TDebugger

   local n := 1
   local oCol

   if ::oWndStack == nil
      ::oWndCode:nRight -= 16
      ::oBrwText:Resize(,,, ::oBrwText:nRight - 16)
      ::oWndCode:SetFocus( .t. )
      ::oWndStack := TDbWindow():New( 1, MaxCol() - 15, MaxRow() - 6, MaxCol(),;
                                     "Calls" )
      AAdd( ::aWindows, ::oWndStack )
      ::oBrwStack := TBrowseNew( 2, MaxCol() - 14, MaxRow() - 7, MaxCol() - 1 )//2
      ::oBrwStack:ColorSpec := ::aColors[ 3 ] + "," + ::aColors[ 4 ] + "," + ::aColors[ 5 ]
      ::oBrwStack:GoTopBlock := { || n := 1 }
      ::oBrwStack:GoBottomBlock := { || n := Len( ::aCallStack ) }
      ::oBrwStack:SkipBlock := { | nSkip, nPos | nPos := n,;
                             n := iif( nSkip > 0, Min( Len( ::aCallStack ), n + nSkip ),;
                             Max( 1, n + nSkip ) ), n - nPos }

      ::oBrwStack:Cargo := 1 // Actual highligthed row
      ::oBrwStack:autolite := .F.
      ::oBrwStack:colPos:=1
      ::oBrwStack:freeze:=1

      ::oBrwStack:AddColumn( oCol:=TBColumnNew( "",  { || PadC( ::aCallStack[ n ][ 1 ], 14 ) } ) )
      ocol:ColorBlock := { || { iif( n == ::oBrwStack:Cargo, 2, 1 ), 3 } }
      ocol:Defcolor :=    { 2,1 }

      ::oWndStack:bPainted := { || ::oBrwStack:ColorSpec := __DbgColors()[ 2 ] + "," + ;
                                  __DbgColors()[ 5 ] + "," + __DbgColors()[ 4 ],;
                                  ::oBrwStack:RefreshAll(), ::oBrwStack:ForceStable() }
      ::oBrwStack:ForceStable()
      ::oWndStack:Show( .f. )
   endif

return nil

METHOD LoadSettings() CLASS TDebugger

   local cInfo := MemoRead( ::cSettingsFileName )
   local n, cLine, nColor

   for n := 1 to MLCount( cInfo )
      cLine := MemoLine( cInfo, 120, n )
      do case
         case Upper( SubStr( cLine, 1, 14 ) ) == "OPTIONS COLORS"
            cLine := SubStr( cLine, At( "{", cLine ) + 1 )
            nColor := 1
            while nColor < 12
               if At( ",", cLine ) != 0
                  ::aColors[ nColor ] := ;
                     StrTran( SubStr( cLine, 1, At( ",", cLine ) - 1 ), '"', "" )
                  cLine := SubStr( cLine, At( ",", cLine ) + 1 )
               else
                  ::aColors[ nColor ] := ;
                     StrTran( SubStr( cLine, 1, At( "}", cLine ) - 1 ), '"', "" )
               endif
               nColor++
            end
         case Upper( SubStr( cLine, 1, 11 ) ) == "OPTIONS TAB"
            cLine := SubStr( cLine, 12, 3)
            ::nTabWidth := val(cLine)
         case Upper( SubStr( cLine, 1, 12 ) ) == "OPTIONS PATH"
            cLine := SubStr( cLine, 13, 120)
            ::cPathForFiles := alltrim( cLine )
         case Upper( SubStr( cLine, 1, 14 ) ) == "MONITOR STATIC"
            ::lShowStatics := .t.
         case Upper( SubStr( cLine, 1, 14 ) ) == "MONITOR PUBLIC"
            ::lShowPublics := .t.
         case Upper( SubStr( cLine, 1, 13 ) ) == "MONITOR LOCAL"
            ::lShowLocals := .t.
         case Upper( SubStr( cLine, 1, 15 ) ) == "MONITOR PRIVATE"
            ::lShowPrivates := .t.
      endcase
   next

return nil

METHOD LoadVars() CLASS TDebugger // updates monitored variables

   local nCount, n, m, xValue, cName
   local cStaticName, nStaticIndex, nStaticsBase

   ::aVars := {}

   if ::lShowPublics
      nCount := __mvDbgInfo( HB_MV_PUBLIC )
      for n := nCount to 1 step -1
         xValue := __mvDbgInfo( HB_MV_PUBLIC, n, @cName )
         if cName != "__DBGSTATICS"  // reserved public used by the debugger
            AAdd( ::aVars, { cName, xValue, "Public" } )
         endif
      next
   endif

   if ::lShowPrivates
      nCount := __mvDbgInfo( HB_MV_PRIVATE )
      for n := nCount to 1 step -1
         xValue := __mvDbgInfo( HB_MV_PRIVATE, n, @cName )
         AAdd( ::aVars, { cName, xValue, "Private" } )
      next
   endif

   if ::lShowStatics
      if Type( "__DbgStatics" ) == "A"
         for n := 1 to Len( __DbgStatics )
            for m := 1 to Len( __DbgStatics[ n ][ 2 ] )
               cStaticName  := __DbgStatics[ n ][ 2 ][ m ]
               nStaticIndex := __DbgStatics[ n ][ 1 ] + m
               AAdd( ::aVars, { cStaticName, nStaticIndex, "Static" } )
            next
         next
      endif
   endif

   if ::lShowLocals
      for n := 1 to Len( ::aCallStack[ 1 ][ 2 ] )
         AAdd( ::aVars, ::aCallStack[ 1 ][ 2 ][ n ] )
      next
   endif

   if ::lSortVars
      ::Sort()
   endif

return nil

METHOD ShowVars() CLASS TDebugger

   local n
   local nWidth
   Local oCol
   local lRepaint := .f.

   if ::lGo
      return nil
   endif

   if ::oWndVars == nil

      n := 1

      ::LoadVars()
      ::oWndVars := TDbWindow():New( 1, 0, Min( 7, Len( ::aVars ) + 2 ),;
         MaxCol() - iif( ::oWndStack != nil, ::oWndStack:nWidth(), 0 ),;
         "Monitor:" + iif( ::lShowLocals, " Local", "" ) + ;
         iif( ::lShowStatics, " Static", "" ) + iif( ::lShowPrivates, " Private", "" ) + ;
         iif( ::lShowPublics, " Public", "" ) )

      ::oWndCode:nTop += ::oWndVars:nBottom
      ::oBrwText:Resize( ::oBrwText:nTop + ::oWndVars:nBottom )
      ::oBrwText:RefreshAll()
      ::oWndCode:SetFocus( .t. )

      ::oWndVars:Show( .f. )
      AAdd( ::aWindows, ::oWndVars )
      ::oWndVars:bLButtonDown := { | nMRow, nMCol | ::WndVarsLButtonDown( nMRow, nMCol ) }
      ::oWndVars:bLDblClick   := { | nMRow, nMCol | ::EditVar( n ) }
      ::oWndVars:bPainted     := { || if(Len( ::aVars ) > 0, ( ::obrwVars:ForceStable(),RefreshVarsS(::oBrwVars) ),) }

      ::oBrwVars := TBrowseNew( 2, 1, ::oWndVars:nBottom - 1, MaxCol() - iif( ::oWndStack != nil,;
                               ::oWndStack:nWidth(), 0 ) - 1 )
      ::oBrwVars:Cargo :={ 1,{}} // Actual highligthed row
      ::oBrwVars:ColorSpec := ::aColors[ 2 ] + "," + ::aColors[ 5 ] + "," + ::aColors[ 3 ]
      ::oBrwVars:GOTOPBLOCK := { || ::oBrwVars:cargo[ 1 ] := Min( 1, Len( ::aVars ) ) }
      ::oBrwVars:GoBottomBlock := { || ::oBrwVars:cargo[ 1 ] := Len( ::aVars ) }
      ::oBrwVars:SkipBlock = { | nSkip, nOld | nOld := ::oBrwVars:Cargo[ 1 ],;
                               ::oBrwVars:Cargo[ 1 ] += nSkip,;
                               ::oBrwVars:Cargo[ 1 ] := Min( Max( ::oBrwVars:Cargo[ 1 ], 1 ),;
                                                             Len( ::aVars ) ),;
                               If( Len( ::aVars ) > 0, ::oBrwVars:Cargo[ 1 ] - nOld, 0 ) }


      ::oWndVars:bKeyPressed := { | nKey | ( iif( nKey == K_DOWN ;
      , ::oBrwVars:Down(), nil ), iif( nKey == K_UP, ::oBrwVars:Up(), nil ) ;
      , iif( nKey == K_PGDN, ::oBrwVars:PageDown(), nil ) ;
      , iif( nKey == K_PGUP, ::oBrwVars:PageUp(), nil ) ;
      , iif( nKey == K_HOME, ::oBrwVars:GoTop(), nil ) ;
      , iif( nKey == K_END, ::oBrwVars:GoBottom(), nil ) ;
      , iif( nKey == K_ENTER, ::EditVar( ::oBrwVars:Cargo[1] ), nil ), ::oBrwVars:ForceStable() ) }

      nWidth := ::oWndVars:nWidth() - 1
      ::oBrwVars:AddColumn( oCol:=TBColumnNew( "",  { || If( Len( ::aVars ) > 0, AllTrim( Str( ::oBrwVars:Cargo[1] -1 ) ) + ") " + ;
         PadR( GetVarInfo( ::aVars[ Max( ::oBrwVars:Cargo[1], 1 ) ] ),;
         ::oWndVars:nWidth() - 5 ), "" ) } ) )
      AAdd(::oBrwVars:Cargo[2],::avars)
      oCol:DefColor:={2,1}
      if Len( ::aVars ) > 0
         ::oBrwVars:ForceStable()
      endif
   else
      ::LoadVars()

      ::oWndVars:cCaption := "Monitor:" + ;
      iif( ::lShowLocals, " Local", "" ) + ;
      iif( ::lShowStatics, " Static", "" ) + ;
      iif( ::lShowPrivates, " Private", "" ) + ;
      iif( ::lShowPublics, " Public", "" )

      if Len( ::aVars ) == 0
         if ::oWndVars:nBottom - ::oWndVars:nTop > 1
            ::oWndVars:nBottom := ::oWndVars:nTop + 1
            lRepaint := .t.
         endif
      endif
      if Len( ::aVars ) > ::oWndVars:nBottom - ::oWndVars:nTop - 1
         ::oWndVars:nBottom := ::oWndVars:nTop + Min( Len( ::aVars ) + 1, 7 )
         ::oBrwVars:nBottom := ::oWndVars:nBottom - 1
         ::oBrwVars:Configure()
         lRepaint := .t.
      endif
      if Len( ::aVars ) < ::oWndVars:nBottom - ::oWndVars:nTop - 1
         ::oWndVars:nBottom := ::oWndVars:nTop + Len( ::aVars ) + 1
         ::oBrwVars:nBottom := ::oWndVars:nBottom - 1
         ::oBrwVars:Configure()
         lRepaint := .t.
      endif
      if ! ::oWndVars:lVisible
         ::oWndCode:nTop := ::oWndVars:nBottom + 1
         ::oBrwText:Resize( ::oWndVars:nBottom + 2 )
         ::oWndVars:Show()
      else
         if lRepaint
            ::oWndCode:nTop := ::oWndVars:nBottom + 1
            ::oBrwText:Resize( ::oWndCode:nTop + 1 )
            ::oWndCode:Refresh()
            ::oWndVars:Refresh()
         endif
      endif
      if Len( ::aVars ) > 0
         ::oBrwVars:RefreshAll()
         ::oBrwVars:ForceStable()
      endif
   endif

return nil

static function GetVarInfo( aVar )

   local nProcLevel := 1

   do case
      case aVar[ 3 ] == "Local"
           while ProcName( nProcLevel ) != aVar[ 4 ]
              nProcLevel++
           end
           return aVar[ 1 ] + " <Local, " + ;
                  ValType( __vmVarLGet( nProcLevel, aVar[ 2 ] ) ) + ;
                  ">: " + ValToStr( __vmVarLGet( nProcLevel, aVar[ 2 ] ) )

      case aVar[ 3 ] == "Public" .or. aVar[ 3 ] == "Private"
           return aVar[ 1 ] + " <" + aVar[ 3 ] + ", " + ValType( aVar[ 2 ] ) + ;
                  ">: " + ValToStr( aVar[ 2 ] )

      case aVar[ 3 ] == "Static"
           return aVar[ 1 ] + " <Static, " + ;
                  ValType( __vmVarSGet( aVar[ 2 ] ) ) + ;
                  ">: " + ValToStr( __vmVarSGet( aVar[ 2 ] ) )
   endcase

return ""

static function CompareLine( Self )

return { | a | a[ 1 ] == Self:oBrwText:nRow }  // it was nLine

METHOD ShowCode( cModuleName ) CLASS TDebugger

   local cFunction := SubStr( cModuleName, RAt( ":", cModuleName ) + 1 )
   local cPrgName  := SubStr( cModuleName, 1, RAt( ":", cModuleName ) - 1 )

   ASize( ::aCallStack, Len( ::aCallStack ) + 1 )
   AIns( ::aCallStack, 1 )
   ::aCallStack[ 1 ] := { cFunction, {} } // function name and locals array
if !::lGo
   if ::oWndStack != nil
      ::oBrwStack:RefreshAll()
   endif

   if cPrgName != ::cPrgName
      if ! File( cPrgName ) .and. ! Empty( ::cPathForFiles )
         if File( ::cPathForFiles + cPrgName )
            cPrgName = ::cPathForFiles + cPrgName
         endif
      endif
      ::cPrgName := cPrgName
      ::oBrwText := TBrwText():New( ::oWndCode:nTop + 1, ::oWndCode:nLeft + 1,;
                   ::oWndCode:nBottom - 1, ::oWndCode:nRight - 1, ::cPrgName,;
                   __DbgColors()[ 2 ] + "," + __DbgColors()[ 5 ] + "," + ;
                   __DbgColors()[ 3 ] + "," + __DbgColors()[ 6 ] )

      ::oWndCode:SetCaption( ::cPrgName )
   endif
endif
return nil

METHOD Open() CLASS TDebugger

   local cFileName := ::InputBox( "Please enter the filename", Space( 30 ) )

return nil

METHOD OSShell() CLASS TDebugger

   local cImage := SaveScreen()
   local cColors := SetColor()
   local cOs    := Upper( OS() )
   local cShell
   local bLastHandler := ErrorBlock({ |objErr| BREAK (objErr) })
   local oE

   SET COLOR TO "W/N"
   CLS
   SetCursor( SC_NORMAL )

   begin sequence
      if At("WINDOWS", cOs) != 0 .OR. At("DOS", cOs) != 0 .OR. At("OS/2", cOs) != 0
         cShell := GetEnv("COMSPEC")
         RUN ( cShell )
      elseif At("LINUX", cOs) != 0
         cShell := GetEnv("SHELL")
         RUN ( cShell )
      else
         Alert( "Not implemented yet!" )

      endif

   recover using oE
      Alert("Error: " + oE:description)

   end sequence

   ErrorBlock(bLastHandler)

   SetCursor( SC_NONE )
   RestScreen( ,,,, cImage )
   SetColor( cColors )

return nil

METHOD HideVars() CLASS TDebugger

   ::oWndVars:Hide()
   ::oWndCode:nTop := 1
   ::oWndCode:SetFocus( .t. )
   ::oBrwText:Resize( 2 )

return nil

METHOD InputBox( cMsg, uValue, bValid ) CLASS TDebugger

   local nTop    := ( MaxRow() / 2 ) - 5
   local nLeft   := ( MaxCol() / 2 ) - 25
   local nBottom := ( MaxRow() / 2 ) - 3
   local nRight  := ( MaxCol() / 2 ) + 25
   local cType   := ValType( uValue )
   local uTemp   := PadR( uValue, nRight - nLeft - 1 )
   local GetList := {}
   local nOldCursor
   local lScoreBoard := Set( _SET_SCOREBOARD, .f. )
   local oWndInput := TDbWindow():New( nTop, nLeft, nBottom, nRight, cMsg,;
                                       ::oPullDown:cClrPopup )

   oWndInput:lShadow := .t.
   oWndInput:Show(.t.)

   if bValid == nil
      @ nTop + 1, nLeft + 1 GET uTemp COLOR "," + __DbgColors()[ 5 ]
   else
      @ nTop + 1, nLeft + 1 GET uTemp VALID bValid COLOR "," + __DbgColors()[ 5 ]
   endif

   nOldCursor := SetCursor( SC_NORMAL )
   READ
   SetCursor( nOldCursor )
   oWndInput:Hide()
   Set( _SET_SCOREBOARD, lScoreBoard )

   do case
      case cType == "C"
           uTemp := AllTrim( uTemp )

      case cType == "D"
           uTemp := CToD( uTemp )

      case cType == "N"
           uTemp := Val( uTemp )

   endcase

return iif( LastKey() != K_ESC, uTemp, uValue )


METHOD IsBreakPoint( nLine ) CLASS TDebugger

return AScan( ::aBreakPoints, { | aBreak | aBreak[ 1 ] == nLine } ) != 0


METHOD GotoLine( nLine ) CLASS TDebugger

   if ::oBrwVars != nil
      ::ShowVars()
   endif

   ::oBrwText:GotoLine( nLine )

   if ::oBrwStack != nil .and. ! ::oBrwStack:Stable
      ::oBrwStack:ForceStable()
   endif

return nil

METHOD Local() CLASS TDebugger

   ::lShowLocals := ! ::lShowLocals

   if ::lShowPublics .or. ::lShowPrivates .or. ::lShowStatics .or. ::lShowLocals
      ::ShowVars()
   else
      ::HideVars()
   endif

return nil

METHOD Private() CLASS TDebugger

   ::lShowPrivates := ! ::lShowPrivates

   if ::lShowPublics .or. ::lShowPrivates .or. ::lShowStatics .or. ::lShowLocals
      ::ShowVars()
   else
      ::HideVars()
   endif

return nil

METHOD Public() CLASS TDebugger

   ::lShowPublics := ! ::lShowPublics

   if ::lShowPublics .or. ::lShowPrivates .or. ::lShowStatics .or. ::lShowLocals
      ::ShowVars()
   else
      ::HideVars()
   endif

return nil

METHOD RestoreAppStatus() CLASS TDebugger

   ::cImage := SaveScreen()
   DispBegin()
   RestScreen( 0, 0, MaxRow(), MaxCol(), ::cAppImage )
   SetPos( ::nAppRow, ::nAppCol )
   SetColor( ::cAppColors )
   SetCursor( ::nAppCursor )

return nil

METHOD RestoreSettings() CLASS TDebugger

   local n

   ::cSettingsFileName := ::InputBox( "File name", ::cSettingsFileName )

   if LastKey() != K_ESC
      ::LoadSettings()

      ::oPullDown:LoadColors()
      ::oPullDown:Refresh()
      ::BarDisplay()
      ::ShowVars()
      for n := 1 to Len( ::aWindows )
        ::aWindows[ n ]:LoadColors()
        ::aWindows[ n ]:Refresh()
      next
   endif

return nil

METHOD SaveAppStatus() CLASS TDebugger

   ::cAppImage  := SaveScreen()
   ::nAppRow    := Row()
   ::nAppCol    := Col()
   ::cAppColors := SetColor()
   ::nAppCursor := SetCursor()
   RestScreen( 0, 0, MaxRow(), MaxCol(), ::cImage )
   // SetCursor( SC_NONE )
   DispEnd()

return nil

METHOD SaveSettings() CLASS TDebugger

   local cInfo := "", n, oWnd

   ::cSettingsFileName := ::InputBox( "File name", ::cSettingsFileName )

   if LastKey() != K_ESC

      if ! Empty( ::cPathForFiles )
         cInfo += "Options Path " + ::cPathForFiles + HB_OsNewLine()
      endif

      cInfo += "Options Colors {"
      for n := 1 to Len( ::aColors )
         cInfo += '"' + ::aColors[ n ] + '"'
         if n < Len( ::aColors )
            cInfo += ","
         endif
      next
      cInfo += "}" + HB_OsNewLine()

      if ::lMonoDisplay
         cInfo += "Options mono " + HB_OsNewLine()
      endif

      if ::nSpeed != 0
         cInfo += "Run Speed " + AllTrim( Str( ::nSpeed ) ) + HB_OsNewLine()
      endif

      for n := 1 to Len( ::aWindows )
         oWnd := ::aWindows[ n ]
         cInfo += "Window Size " + AllTrim( Str( oWnd:nBottom - oWnd:nTop + 1 ) ) + " "
         cInfo += AllTrim( Str( oWnd:nRight - oWnd:nLeft + 1 ) ) + HB_OsNewLine()
         cInfo += "Window Move " + AllTrim( Str( oWnd:nTop ) ) + " "
         cInfo += AllTrim( Str( oWnd:nLeft ) ) + HB_OsNewLine()
         cInfo += "Window Next" + HB_OsNewLine()
      next

      if ::nTabWidth != 4
         cInfo += "Options Tab " + AllTrim( Str( ::nTabWidth ) ) + HB_OsNewLine()
      endif

      if ::lShowStatics
         cInfo += "Monitor Static" + HB_OsNewLine()
      endif

      if ::lShowPublics
         cInfo += "Monitor Public" + HB_OsNewLine()
      endif

      if ::lShowLocals
         cInfo += "Monitor Local" + HB_OsNewLine()
      endif

      if ::lShowPrivates
         cInfo += "Monitor Private" + HB_OsNewLine()
      endif

      MemoWrit( ::cSettingsFileName, cInfo )
   endif

return nil

METHOD Static() CLASS TDebugger

   ::lShowStatics := ! ::lShowStatics

   if ::lShowPublics .or. ::lShowPrivates .or. ::lShowStatics .or. ::lShowLocals
      ::ShowVars()
   else
      ::HideVars()
   endif

return nil

METHOD ToggleBreakPoint() CLASS TDebugger

   local nAt := AScan( ::aBreakPoints, { | aBreak | aBreak[ 1 ] == ;
                       ::oBrwText:nRow } ) // it was nLine

   if nAt == 0
      AAdd( ::aBreakPoints, { ::oBrwText:nRow, ::cPrgName } )     // it was nLine
      ::oBrwText:ToggleBreakPoint(::oBrwText:nRow, .T.)
   else
      ADel( ::aBreakPoints, nAt )
      ASize( ::aBreakPoints, Len( ::aBreakPoints ) - 1 )
      ::oBrwText:ToggleBreakPoint(::oBrwText:nRow, .F.)
   endif

   ::oBrwText:RefreshCurrent()

return nil

METHOD ViewSets() CLASS TDebugger

   local oWndSets := TDbWindow():New( 1, 8, MaxRow() - 2, MaxCol() - 8,;
                                      "System Settings[1..47]", ::ClrModal() )
   local aSets := { "Exact", "Fixed", "Decimals", "DateFormat", "Epoch", "Path",;
                    "Default", "Exclusive", "SoftSeek", "Unique", "Deleted",;
                    "Cancel", "Debug", "TypeAhead", "Color", "Cursor", "Console",;
                    "Alternate", "AltFile", "Device", "Extra", "ExtraFile",;
                    "Printer", "PrintFile", "Margin", "Bell", "Confirm", "Escape",;
                    "Insert", "Exit", "Intensity", "ScoreBoard", "Delimeters",;
                    "DelimChars", "Wrap", "Message", "MCenter", "ScrollBreak",;
                    "EventMask", "VideoMode", "MBlockSize", "MFileExt",;
                    "StrictRead", "Optimize", "Autopen", "Autorder", "AutoShare" }

   local oBrwSets := TBrowseNew( oWndSets:nTop + 1, oWndSets:nLeft + 1,;
                                 oWndSets:nBottom - 1, oWndSets:nRight - 1 )
   local n := 1
   local nWidth := oWndSets:nRight - oWndSets:nLeft - 1
   local oCol
   oBrwSets:Cargo :={ 1,{}} // Actual highligthed row
   oBrwSets:autolite:=.f.
   oBrwSets:ColorSpec := ::ClrModal()
   oBrwSets:GOTOPBLOCK := { || oBrwSets:cargo[ 1 ]:= 1 }
   oBrwSets:GoBottomBlock := { || oBrwSets:cargo[ 1 ]:= Len(oBrwSets:cargo[ 2 ][ 1 ])}
   oBrwSets:SKIPBLOCK := { |nPos| ( nPos:= ArrayBrowseSkip(nPos, oBrwSets), oBrwSets:cargo[ 1 ]:= ;
   oBrwSets:cargo[ 1 ] + nPos,nPos ) }
   oBrwSets:AddColumn( ocol := TBColumnNew( "", { || PadR( aSets[ oBrwSets:cargo[ 1 ] ], 12 ) } ) )
   aadd(oBrwSets:Cargo[2],asets)
   ocol:defcolor:={1,2}
   oBrwSets:AddColumn( oCol := TBColumnNew( "",;
                       { || PadR( ValToStr( Set( oBrwSets:cargo[ 1 ]  ) ), nWidth - 13 ) } ) )
   ocol:defcolor:={1,3}
   ocol:width:=40
   oWndSets:bPainted    := { || oBrwSets:ForceStable(),RefreshVarsS(oBrwSets)}
   oWndSets:bKeyPressed := { | nKey | SetsKeyPressed( nKey, oBrwSets, Len( aSets ),;
                            oWndSets, "System Settings",;
                            { || ::EditSet( n, oBrwSets ) } ) }

   SetCursor( SC_NONE )
   oWndSets:ShowModal()

return nil

METHOD WndVarsLButtonDown( nMRow, nMCol ) CLASS TDebugger

   if nMRow > ::oWndVars:nTop .and. ;
      nMRow < ::oWndVars:nBottom .and. ;
      nMCol > ::oWndVars:nLeft .and. ;
      nMCol < ::oWndVars:nRight
      if nMRow - ::oWndVars:nTop >= 1 .and. ;
         nMRow - ::oWndVars:nTop <= Len( ::aVars )
         while ::oBrwVars:RowPos > nMRow - ::oWndVars:nTop
            ::oBrwVars:Up()
            ::oBrwVars:ForceStable()
         end
         while ::oBrwVars:RowPos < nMRow - ::oWndVars:nTop
            ::oBrwVars:Down()
            ::oBrwVars:ForceStable()
         end
      endif
   endif

return nil

static procedure SetsKeyPressed( nKey, oBrwSets, nSets, oWnd, cCaption, bEdit )


   local nSet := oBrwSets:cargo[1]
   local cTemp:=str(nSet,4)

   Local nRectoMove
   do case
      case nKey == K_UP
              oBrwSets:Up()
      case nKey == K_DOWN
              oBrwSets:Down()
      case nKey == K_HOME .or. (nKey == K_CTRL_PGUP) .or. (nKey == K_CTRL_HOME)
              oBrwSets:GoTop()
      case nKey == K_END .or. (nkey == K_CTRL_PGDN) .or. (nkey == K_CTRL_END )
              oBrwSets:GoBottom()
      Case nKey == K_PGDN
              oBrwSets:pageDown()
      Case nKey == K_PGUP
              OBrwSets:PageUp()

      case nKey == K_ENTER
           if bEdit != nil
              Eval( bEdit )
           endif
           if LastKey() == K_ENTER
              KEYBOARD Chr( K_DOWN )
           endif

   endcase
      RefreshVarsS(oBrwSets)

      oWnd:SetCaption( cCaption + "[" + AllTrim( Str( oBrwSets:Cargo[1] ) ) + ;
                       ".." + AllTrim( Str( nSets ) ) + "]" )

return
static procedure SetsKeyVarPressed( nKey, oBrwSets, nSets, oWnd, bEdit )
   Local nRectoMove
   local nSet := oBrwSets:Cargo[1]
   do case
      case nKey == K_UP
              oBrwSets:Up()

      case nKey == K_DOWN
              oBrwSets:Down()

      case nKey == K_HOME .or. (nKey == K_CTRL_PGUP) .or. (nKey == K_CTRL_HOME)
              oBrwSets:GoTop()
      case nKey == K_END .or. (nkey == K_CTRL_PGDN) .or. (nkey == K_CTRL_END )
              oBrwSets:GoBottom()

      Case nKey == K_PGDN
              oBrwSets:pageDown()
      Case nKey == K_PGUP
              OBrwSets:PageUp()

      case nKey == K_ENTER
           if bEdit != nil
              Eval( bEdit )
           endif
           if LastKey() == K_ENTER
              KEYBOARD Chr( K_DOWN )
           endif

   endcase

return


static function ValToStr( uVal )

   local cType := ValType( uVal )
   local cResult := "U"

   do case
      case uVal == nil
           cResult := "NIL"

      case cType == "A"
           cResult := "{ ... }"

      Case cType  =="B"
         cResult:= "{ || ... }"

      case cType $ "CM"
           cResult := '"' + uVal + '"'

      case cType == "L"
           cResult := iif( uVal, ".T.", ".F." )

      case cType == "D"
           cResult := DToC( uVal )

      case cType == "N"
           cResult := AllTrim( Str( uVal ) )

      case cType == "O"
           cResult := "Class " + uVal:ClassName() + " object"
   endcase

return cResult


METHOD LineNumbers() CLASS TDebugger

   ::oBrwText:lLineNumbers := !::oBrwText:lLineNumbers
   ::oBrwText:RefreshAll()

return Self

METHOD Locate( nMode ) CLASS TDebugger

   local cValue

   DEFAULT nMode TO 0

   cValue := ::InputBox( "Search string", ::cSearchString )

   if empty( cValue )
      return nil
   endif

   ::cSearchString := cValue

return ::oBrwText:Search( ::cSearchString, ::lCaseSensitive, 0 )

METHOD FindNext() CLASS TDebugger

   local lFound

   if Empty( ::cSearchString )
      lFound := ::Locate( 1 )
   else
      lFound := ::oBrwText:Search( ::cSearchString, ::lCaseSensitive, 1 )
   endif

return lFound

METHOD FindPrevious() CLASS TDebugger

   local lFound

   if Empty( ::cSearchString )
      lFound := ::Locate( 2 )
   else
      lFound := ::oBrwText:Search( ::cSearchString, ::lCaseSensitive, 2 )
   endif

return lFound

METHOD SearchLine() CLASS TDebugger

   local cLine

   cLine := ::InputBox( "Line number", "1" )

   if Val( cLine ) > 0
      ::oBrwText:GotoLine ( Val( cLine ) )
   endif

return nil

/* Not declared - it seems ToggleCaseSensitive() replaced this!!!
METHOD CaseSensitive() CLASS TDebugger

   ::lCaseSensitive := !::lCaseSensitive

return nil
*/

function __DbgColors()

return iif( ! s_oDebugger:lMonoDisplay, s_oDebugger:aColors,;
           { "W+/N", "W+/N", "N/W", "N/W", "N/W", "N/W", "W+/N",;
             "N/W", "W+/W", "W/N", "W+/N" } )

function __Dbg()

return s_oDebugger

static function myColors( oBrowse, aColColors )
   local i
   local nColPos := oBrowse:colpos

   for i := 1 to len( aColColors )
      oBrowse:colpos := aColColors[i]
      oBrowse:hilite()
   next

   oBrowse:colpos := nColPos
return Nil

static procedure RefreshVarsS( oBrowse )

   local nLen := Len(oBrowse:aColumns)

   if ( nLen == 2 )
      oBrowse:dehilite():colpos:=2
   endif
   oBrowse:dehilite():forcestable()
   if ( nLen == 2 )
      oBrowse:hilite():colpos:=1
   endif
   oBrowse:hilite()
   return

static function ArrayBrowseSkip( nPos, oBrwSets, n )

   return iif( oBrwSets:cargo[ 1 ] + nPos < 1, 0 - oBrwSets:cargo[ 1 ] + 1 , ;
      iif( oBrwSets:cargo[ 1 ] + nPos > Len(oBrwSets:cargo[ 2 ][ 1 ]), ;
      Len(oBrwSets:cargo[ 2 ][ 1 ]) - oBrwSets:cargo[ 1 ], nPos ) )