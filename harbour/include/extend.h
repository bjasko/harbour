/*
 * $Id$
 */

#ifndef _EXTEND_H
#define _EXTEND_H

#define FILE _FILE     /* to avoid conflicts with Harbour File() */

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#undef FILE

typedef struct          /* symbol support structure */
{
   char * szName;       /* the name of the symbol */
   char   cScope;       /* the scope of the symbol */
   HARBOURFUNC pFunPtr; /* function address for function symbol table entries */
   void * pDynSym;      /* pointer to its dynamic symbol if defined */
} SYMBOL, * PSYMBOL;

/* Harbour Functions scope */
#define FS_PUBLIC       0
#define FS_STATIC       2
#define FS_INIT         8
#define FS_EXIT        16
#define FS_MESSAGE     32

void VirtualMachine( PBYTE pCode, PSYMBOL pSymbols );  /* invokes the virtual machine */

/* items types */
#define IT_NIL       0x0000
#define IT_INTEGER   0x0002
#define IT_LONG      0x0008
#define IT_DOUBLE    0x0010
#define IT_DATE      0x0020
#define IT_LOGICAL   0x0080
#define IT_SYMBOL    0x0100
#define IT_ALIAS     0x0200
#define IT_BLOCK     0x1000
#define IT_STRING    0x0400
#define IT_BYREF     0x2000
#define IT_ARRAY     0x8000
#define IT_OBJECT    IT_ARRAY
#define IT_NUMERIC  ( IT_INTEGER | IT_LONG | IT_DOUBLE )
#define IT_ANY       0xFFFF

struct _CODEBLOCK;	/* forward declaration */

typedef struct     /* items hold at the virtual machine stack */
{
   WORD wType;     /* type of the item */
   WORD wLength;   /* length of the item */
   WORD wDec;      /* decimal places in a numeric double item */
   union {         /* different things may be holded here */
	  char * szText;   /* string values */
	  int iNumber;     /* int values */
	  long lNumber;    /* long values */
	  double dNumber;  /* double values */
	  int iLogical;    /* logical values */
	  long lDate;      /* date values */
	  PSYMBOL pSymbol; /* functions call symbol */
	  struct _CODEBLOCK * pCodeblock;/* pointer to a codeblock structure */
	  WORD wItem;      /* variable by reference, stack offset */
	  void * pBaseArray; /* array base */
   } value;
   WORD wBase;     /* stack frame number of items position for a function call */
   WORD wLine;     /* currently processed PRG line number */
   WORD wParams;   /* number of received parameters for a function call */
} ITEM, * PITEM;

typedef struct
{
   PITEM pItems;               /* pointer to the array items */
   ULONG ulLen;                /* number of items in the array */
   WORD  wHolders;             /* number of holders of this array */
   WORD  wClass;               /* offset to the classes base if it is an object */
} BASEARRAY, * PBASEARRAY;

typedef struct     /* stack managed by the virtual machine */
{
   PITEM pItems;   /* pointer to the stack items */
   PITEM pPos;     /* pointer to the latest used item */
   LONG  wItems;   /* total items that may be holded on the stack */
   ITEM  Return;   /* latest returned value */
   PITEM pBase;    /* stack frame position for the current function call */
   PITEM pEvalBase;/* stack frame position for the evaluated codeblock */
   int  iStatics;  /* statics base for the current function call */
   char szDate[ 9 ]; /* last returned date from _pards() yyyymmdd format */
} STACK;

typedef struct
{
   WORD     wArea;      /* Workarea number */
   WORD     wMemvar;    /* Index number into memvars ( publics & privates ) array */
   PSYMBOL  pSymbol;    /* pointer to its relative local symbol */
   HARBOURFUNC pFunPtr; /* Pointer to the function address */
} DYNSYM, * PDYNSYM;    /* dynamic symbol structure */

/* internal structure for codeblocks */
typedef struct _CODEBLOCK
{
  BYTE * pCode;       /* codeblock pcode */
  PITEM pItems;       /* table with referenced local variables */
  WORD wLocals;       /* number of referenced local variables */
  WORD wDetached;     /* holds if pItems table variables values */
  PSYMBOL pSymbols;   /* codeblocks symbols */
  WORD wRefBase;      /* stack frame position for referenced local variables */
  int iStatBase;      /* static base for function where CB was created */
  long lCounter;      /* numer of references to this codeblock */
} CODEBLOCK, * PCODEBLOCK;

PITEM _param( WORD wParam, WORD wType ); /* retrieve a generic parameter */
char * _parc( WORD wParam, ... );  /* retrieve a string parameter */
ULONG _parclen( WORD wParam, ... );  /* retrieve a string parameter length */
char * _pards( WORD wParam, ... ); /* retrieve a date as a string yyyymmdd */
int _parl( WORD wParam, ... );     /* retrieve a logical parameter as an int */
double _parnd( WORD wParam, ... ); /* retrieve a numeric parameter as a double */
int _parni( WORD wParam, ... );    /* retrieve a numeric parameter as a integer */
long _parnl( WORD wParam, ... );   /* retrieve a numeric parameter as a long */
WORD _parinfo( WORD wParam );  /* Determine the param count or data type */
WORD _pcount( void );          /* returns the number of suplied parameters */
void _ret( void );             /* post a NIL return value */
void _retc( char * szText );   /* returns a string */
void _retclen( char * szText, ULONG ulLen ); /* returns a string with a specific length */
void _retds( char * szDate );  /* returns a date, must use yyyymmdd format */
void _retl( int iTrueFalse );  /* returns a logical integer */
void _retni( int iNumber );    /* returns a integer number */
void _retnl( long lNumber );   /* returns a long number */
void _retnd( double dNumber ); /* returns a double */
void _storc( char * szText, WORD wParam, ... ); /* stores a szString on a variable by reference */
void _storclen( char * fixText, WORD wLength, WORD wParam, ... ); /* stores a fixed length string on a variable by reference */
void _stords( char * szDate, WORD wParam, ... ); /* szDate must have yyyymmdd format */
void _storl( int iLogical, WORD wParam, ... ); /* stores a logical integer on a variable by reference */
void _storni( int iValue, WORD wParam, ... ); /* stores an integer on a variable by reference */
void _stornd( double dValue, WORD wParam, ... ); /* stores a double on a variable by reference */
void _stornl( long lValue, WORD wParam, ... ); /* stores a long on a variable by reference */

void * _xgrab( ULONG lSize );   /* allocates memory */
void * _xrealloc( void * pMem, ULONG lSize );   /* reallocates memory */
void _xfree( void * pMem );    /* frees memory */
void ItemCopy( PITEM pDest, PITEM pSource );
void ItemRelease( PITEM pItem );

void hb_arrayNew( PITEM pItem, ULONG ulLen ); /* creates a new array */
void hb_arrayGet( PITEM pArray, ULONG ulIndex, PITEM pItem ); /* retrieves an item */
ULONG hb_arrayLen( PITEM pArray ); /* retrives the array len */
void hb_arraySet( PITEM pArray, ULONG ulIndex, PITEM pItem ); /* sets an array element */
void hb_arraySize( PITEM pArray, ULONG ulLen ); /* sets the array total length */
void hb_arrayRelease( PITEM pArray ); /* releases an array - don't call it - use ItemRelease() !!! */
char *hb_arrayGetString( PITEM pArray, ULONG ulIndex ); /* retrieves the string contained on an array element */
ULONG hb_arrayGetStringLen( PITEM pArray, ULONG ulIndex ); /* retrieves the string length contained on an array element */
int  hb_arrayGetType( PITEM pArray, ULONG ulIndex );
void hb_arrayDel( PITEM pArray, ULONG ulIndex );
PITEM hb_arrayClone( PITEM pArray );
void hb_arrayAdd( PITEM pArray, PITEM pItemValue );

int  hb_itemStrCmp( PITEM pFirst, PITEM pSecond, BOOL bForceExact ); /* our string compare */
char * hb_str( PITEM pNumber, PITEM pWidth, PITEM pDec ); /* convert number to string */
BOOL hb_strempty( char * szText, ULONG ulLen );
long hb_dateEncode( long lDay, long lMonth, long lYear );
void hb_dateDecode( long julian, long * plDay, long * plMonth, long * plYear );

HARBOURFUNC GetMethod( PITEM pObject, PSYMBOL pSymMsg ); /* returns the method pointer of a object class */
char * _GetClassName( PITEM pObject ); /* retrieves an object class name */

/* dynamic symbol table management */
PDYNSYM GetDynSym( char * szName );   /* finds and creates a dynamic symbol if not found */
PDYNSYM NewDynSym( PSYMBOL pSymbol ); /* creates a new dynamic symbol based on a local one */
PDYNSYM FindDynSym( char * szName );  /* finds a dynamic symbol */

/* error API */
PITEM _errNew( void );
PITEM _errPutDescription( PITEM pError, char * szDescription );
PITEM _errPutFileName( PITEM pError, char * szFileName );
PITEM _errPutGenCode( PITEM pError, USHORT uiGenCode );
PITEM _errPutOperation( PITEM pError, char * szOperation );
PITEM _errPutOsCode( PITEM pError, USHORT uiOsCode );
PITEM _errPutSeverity( PITEM pError, USHORT uiSeverity );
PITEM _errPutSubCode( PITEM pError, USHORT uiSubCode );
PITEM _errPutSubSystem( PITEM pError, char * szSubSystem );
PITEM _errPutTries( PITEM pError, USHORT uiTries );
WORD _errLaunch( PITEM pError );
void _errRelease( PITEM pError );

#endif

