/*
 * $Id$
 */

/*
 * Harbour Project source code:
 * SQLite3 library low level (client api) interface code
 *
 * Copyright 2007 P.Chornyj <myorg63@mail.ru>
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
 */

#ifndef HBSQLIT3_CH_
#define HBSQLIT3_CH_

#xtranslate DB_IS_OPEN( <db> ) => !Empty( <db> ) 
#xtranslate STMT_IS_PREPARED( <stmt> ) => !Empty( <stmt> ) 

/* Fundamental Datatypes */
#define SQLITE_INTEGER      1
#define SQLITE_FLOAT        2
#ifdef SQLITE_TEXT
#undef SQLITE_TEXT
#else
#define SQLITE_TEXT         3
#endif
#define SQLITE3_TEXT        3
#define SQLITE_BLOB         4
#define SQLITE_NULL         5

#define SQLITE_OK           0   /* Successful result */

/* Beginning-of-Error-Codes */
#define SQLITE_ERROR        1   /* SQL error or missing database */
#define SQLITE_INTERNAL     2   /* NOT USED. Internal logic error in SQLite */
#define SQLITE_PERM         3   /* Access permission denied */
#define SQLITE_ABORT        4   /* Callback routine requested an abort */
#define SQLITE_BUSY         5   /* The database file is locked */
#define SQLITE_LOCKED       6   /* A table in the database is locked */
#define SQLITE_NOMEM        7   /* A malloc() failed */
#define SQLITE_READONLY     8   /* Attempt to write a readonly database */
#define SQLITE_INTERRUPT    9   /* Operation terminated by sqlite3_interrupt()*/
#define SQLITE_IOERR       10   /* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT     11   /* The database disk image is malformed */
#define SQLITE_NOTFOUND    12   /* NOT USED. Table or record not found */
#define SQLITE_FULL        13   /* Insertion failed because database is full */
#define SQLITE_CANTOPEN    14   /* Unable to open the database file */
#define SQLITE_PROTOCOL    15   /* NOT USED. Database lock protocol error */
#define SQLITE_EMPTY       16   /* Database is empty */
#define SQLITE_SCHEMA      17   /* The database schema changed */
#define SQLITE_TOOBIG      18   /* String or BLOB exceeds size limit */
#define SQLITE_CONSTRAINT  19   /* Abort due to constraint violation */
#define SQLITE_MISMATCH    20   /* Data type mismatch */
#define SQLITE_MISUSE      21   /* Library used incorrectly */
#define SQLITE_NOLFS       22   /* Uses OS features not supported on host */
#define SQLITE_AUTH        23   /* Authorization denied */
#define SQLITE_FORMAT      24   /* Auxiliary database format error */
#define SQLITE_RANGE       25   /* 2nd parameter to sqlite3_bind out of range */
#define SQLITE_NOTADB      26   /* File opened that is not a database file */
#define SQLITE_ROW         100  /* sqlite3_step() has another row ready */
#define SQLITE_DONE        101  /* sqlite3_step() has finished executing */
/* End-of-Error-Codes */

/* Combination of the following bit values are used 
   as the third argument to the sqlite3_open_v2() interface */
#define SQLITE_OPEN_READONLY         1
#define SQLITE_OPEN_READWRITE        2
#define SQLITE_OPEN_CREATE           4
#define SQLITE_OPEN_DELETEONCLOSE    8
#define SQLITE_OPEN_EXCLUSIVE        16
#define SQLITE_OPEN_MAIN_DB          256
#define SQLITE_OPEN_TEMP_DB          512
#define SQLITE_OPEN_TRANSIENT_DB     1024
#define SQLITE_OPEN_MAIN_JOURNAL     2048
#define SQLITE_OPEN_TEMP_JOURNAL     4096
#define SQLITE_OPEN_SUBJOURNAL       8192
#define SQLITE_OPEN_MASTER_JOURNAL   16384

#endif
