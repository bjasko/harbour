/*
 * $Id$
 */

PROCEDURE MAIN()
LOCAL dDate
LOCAL A

   SET DATE FORMAT TO "YYYY.MM.DD"
   SET CENTURY ON

   dDate := 0d20051112
   ? "Should be '2005.11.12' :", dDate

   dDate := 0d18341112
   ? "Should be '1834.11.12' :", dDate

   dDate := 0d20040229 + 1
   ? "Should be '2004.03.01' :", dDate

   dDate := 0d20040229 - 1
   ? "Should be '2004.02.28' :", dDate

   ? "Should be '4' :", 0d20040229 - 0d20040225
   ? "Should be '0' :", 0d20040229 - 0d20040229
   
   dDate := 0d20000229
   ? "Should be '2000.02.29' :", dDate

   a := '0d20040229+1'
   ? "Should be '2004.03.01' :", &a

   a := "DATE() - 0d20051112"
   ? "Number of days from 2005.11.12:", &a

   RETURN
