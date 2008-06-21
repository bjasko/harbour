/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * Header file for common macros
 *
 * Copyright 1999 {list of individual authors and e-mail addresses}
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

#ifndef HB_COMMON_CH_
#define HB_COMMON_CH_

/* Friendly logical aliases */
#define TRUE                    .T.
#define FALSE                   .F.
#define YES                     .T.
#define NO                      .F.

/* Type checking macros */
#translate ISNIL( <xValue> )       => ( <xValue> == NIL )
#translate ISARRAY( <xValue> )     => ( ValType( <xValue> ) == "A" )
#translate ISBLOCK( <xValue> )     => ( ValType( <xValue> ) == "B" )
#translate ISCHARACTER( <xValue> ) => ( ValType( <xValue> ) == "C" )
#translate ISDATE( <xValue> )      => ( ValType( <xValue> ) == "D" )
#translate ISLOGICAL( <xValue> )   => ( ValType( <xValue> ) == "L" )
#translate ISMEMO( <xValue> )      => ( ValType( <xValue> ) == "M" )
#translate ISNUMBER( <xValue> )    => ( ValType( <xValue> ) == "N" )
#translate ISOBJECT( <xValue> )    => ( ValType( <xValue> ) == "O" ) 

/* DEFAULT and UPDATE commands */
#xcommand DEFAULT <v1> TO <x1> [, <vn> TO <xn> ] => ;
                                IF <v1> == NIL ; <v1> := <x1> ; END ;
                                [; IF <vn> == NIL ; <vn> := <xn> ; END ]

#command UPDATE <v1> IF <exp> TO <v2> => ;
                                IF <exp> ; <v1> := <v2> ; END

/* To suppress unused variable -w2 warnings. The code snippet will be 
   optimized out by the compiler, so it won't cause any overhead. 
   It can be used in codeblocks, too. */
/* Please keep it synced with the similar #define in hbclass.ch */
#define HB_SYMBOL_UNUSED( symbol )  ( symbol := ( symbol ) )

/* HASH autoadd options */
#define HB_HAUTOADD_NEVER       0x00
#define HB_HAUTOADD_ACCESS      0x01
#define HB_HAUTOADD_ASSIGN      0x02
#define HB_HAUTOADD_ALWAYS      ( HB_HAUTOADD_ACCESS + HB_HAUTOADD_ASSIGN )
#define HB_HAUTOADD_REFERENCE   HB_HAUTOADD_ALWAYS

#endif /* HB_COMMON_CH_ */
