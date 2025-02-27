/*
 * U D M P . C
 *
 * Last Modified on Thu Feb 27 21:43:14 2025
 *
 * Displays UTF-8 char/symbols in a file or from stdin
 *
 * It can optional display the content of the files as columns of; -
 *  Decimal offset, Hex data, ASCII chars, UTF-8 char/symbols, Code Points
 *
 * Only appears to work properly on MacOS. The wide
 * character code on linux has not produced the same
 * kind of results that the routines in MacOS do.
 * 
 * Compile with; -
 *  python3 write_config.py
 *  cc -Wall -pedantic -o udmp udmp.c config.c genFun.c
 * 
 * For help on useage run with; -
 *  ./udmp -h
 * 
 */


#include <limits.h>	/* mbrtowc() */
#include <locale.h>	/* setlocale() */
#include <stdio.h>	/* printf(), fwprintf(), fflush() */
#include <stdlib.h>	/* free() */
#include <wchar.h>	/* mbrtowc(), fwprintf() */
#include <unistd.h>	/* getopt() */
#include <string.h>	/* strdup() */
#include <libgen.h>	/* basename() */
#include "config.h"	/* struct config */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!(FALSE))
#endif

#define VERSION_INFO "0v1"
#define ARRAY_SIZE 128
#define BYTEMASK 0xFF

char *  exePath = NULL;
char *  exeName = NULL;

int  processNonConfigCommandLineParameters( struct config *  cfg, int  frstIndx, int  lstIndx, char *  cmdLnStrngs[] );
void  setExecutableName( char *  argv[] );


void  cleanupStorage( void )  {
	if( exePath != NULL )  free(( void *) exePath );
}


int  main (int argc, char *  argv[])  {
  struct config  config;
  int indexToFirstNonConfig;
  int index;

  /* Ensure any allocated memory is free'd */
	atexit( cleanupStorage );
  /* setup the name of this program */
  setExecutableName( argv );
  /* set defaults for all configuration options */
  initConfiguration( &config );
  /* set any configuration options that have been specified in the command line */
  indexToFirstNonConfig = setConfiguration( argc, argv, &config );
  if ( config.D.active )  {
    configuration_status( &config);
    fflush( stdout);
  }
  /* if -V, -v or -D then show version information */
  if ( config.V.active || config.v.active || config.D.active )
    printf( "%s version %s\n", exeName, VERSION_INFO );
  /* if -h specified then show the help/usage information and finish */
  if ( config.h.active )  usage( &config, exeName );
  else  {
    if ( config.D.active )  {
      for ( index = indexToFirstNonConfig; index < argc; index++)
        printf ( "Debug: Non-option argument ( %d ): \"%s\"\n", index, argv[index]);
      fflush( stdout );
    }
  /* Ensure characters are treated as wide characters from now on */
    setlocale( LC_CTYPE, "UTF-8" );

	/* Attempt to dump Hex and Ascii and return 0 if successful */
	  return( processNonConfigCommandLineParameters( &config, indexToFirstNonConfig, argc - 1, argv ));
  }
  return 0;
}


void  setExecutableName( char *  argv[] ) {
/* Isolate the name of the executable */
  if(( exePath = strdup( argv[0] )) == NULL )
    perror( "Warning: Unable to create duplicate of path to this executable" );
  else if(( exeName = basename( exePath )) == NULL )  {
    perror( "Warning: Unable to obtain the name of this executable" );
  }
  if ( exeName == NULL )  exeName = argv[ 0 ];
}


int  getCountBytes( FILE *  fp, unsigned char *  bytePtr, int  count )  {
   int  byte, bytesGotten;

  for ( bytesGotten = 0; (bytesGotten < count) && (( byte = fgetc(fp)) != EOF); bytesGotten++ )
    *bytePtr++ = ( unsigned char ) byte & 0xff;
  return ( bytesGotten );
}


int  removeCntBytesFromTheStartOfTheArrayAndReplaceThemWithNewEndBytes(
  FILE *  fp, unsigned char  byteArray[], int  rmCnt, int  targetCnt, int  activeBytesInBuffer )  {
  unsigned char *  rdPtr;
  unsigned char *  wrtPtr;
  int  i, newBytesInTheArray;

  newBytesInTheArray = 0;
#ifdef DEBUG
  fprintf( stderr, "\nEntry: removeCnt...( .. remove cnt: %d, target cnt: %d, active byte cnt %d\n", rmCnt, targetCnt, activeBytesInBuffer );
#endif
  if ( rmCnt > 0 )  {
    rdPtr = byteArray + rmCnt;
    wrtPtr = byteArray;
    for ( i = targetCnt - rmCnt; i > 0; i-- )  *wrtPtr++ = *rdPtr++; /* copy valid bytes left down to start of array */
    newBytesInTheArray = getCountBytes( fp, wrtPtr, rmCnt );
  }
#ifdef DBEUG
  fprintf( stderr, "\nExit: removeCnt...( .. remove cnt: %d, target cnt: %d) new bytes: %d\n", rmCnt, targetCnt, newBytesInTheArray );
#endif
  return( newBytesInTheArray + activeBytesInBuffer);
}


/* Check for byte that should never occur in valid UTF-8 i.e. 0xC0, 0xC1, 0xF5-0xFF */
int  illegalByteFound( unsigned char  byte )  {
  return(( byte == ( unsigned char ) 0xC0 ) ||
    ( byte == ( unsigned char ) 0xC1 ) ||
    ( byte > ( unsigned char ) 0xF4 ));
}


/* Check for a continuation byte that should never occur at the start of a UTF-8 char */
int  illegalStartByte( unsigned char  byte )  {
  return(( byte >= ( unsigned char ) 0x80 ) &&
    ( byte <= ( unsigned char ) 0xBF ));
}


/* determine size of UTF-8 character - ugly brute force method with inadequate checking */
size_t  sizeOf_UTF8_Char( unsigned char  utf8_chr[] )  {
  if ( utf8_chr[ 0 ] <= ( unsigned char ) 0x7F )  return(( size_t) 1 );  /* Quickly deal with ASCII */
  else if ( illegalStartByte( utf8_chr[ 0 ]))  return(( size_t) 0 );  /* check utf-8 for continuation byte at start of char */
  else if ( illegalByteFound( utf8_chr[ 0 ]))  return(( size_t) 0 );  /* check utf-8 for illegal 0xC0, 0xC1 or 0xF5-0xFF */
  else if ((( utf8_chr[ 0 ] & ( unsigned char ) 0xE0 ) == 0xC0 ) &&   /* Does it have a valid start byte for 2 byte char */
    (( utf8_chr[ 1 ] & ( unsigned char ) 0xC0 ) == 0x80 ))  return(( size_t) 2 );
  else if ((( utf8_chr[ 0 ] & ( unsigned char ) 0xF0 ) == 0xE0 ) &&   /* Does it have a valid start byte for 3 byte char */
    (( utf8_chr[ 1 ] & ( unsigned char ) 0xC0 ) == 0x80 ) &&
    (( utf8_chr[ 2 ] & ( unsigned char ) 0xC0 ) == 0x80 ))  return(( size_t) 3 );
  else if ((( utf8_chr[ 0 ] & ( unsigned char ) 0xF8 ) == 0xF0 ) &&   /* Does it have a valid start byte for 4 byte char */
    (( utf8_chr[ 1 ] & ( unsigned char ) 0xC0 ) == 0x80 ) &&
    (( utf8_chr[ 2 ] & ( unsigned char ) 0xC0 ) == 0x80 ) &&
    (( utf8_chr[ 3 ] & ( unsigned char ) 0xC0 ) == 0x80 ))  return(( size_t) 4 );
  else  return(( size_t) 0 );  /* not valid utf-8 */
}


/* convert UTF-8 character to Code Point - again ugly brute force method */
size_t  conv_UTF8_ToCodePoint( unsigned char  utf8_chr[] )  {
  if ( utf8_chr[ 0 ] <= ( unsigned char ) 0x7F )
    return(( size_t) utf8_chr[ 0 ]);  /* return ASCII character */
  else if ( sizeOf_UTF8_Char( utf8_chr ) == ( size_t) 0 )
    return(( size_t) 0xFFFD );  /* not valid utf-8 so use replacement char symbol ? in a diamond */
  else if ((( utf8_chr[ 0 ] & ( unsigned char ) 0xE0 ) == 0xC0 ) &&
    (( utf8_chr[ 1 ] & ( unsigned char ) 0xC0 ) == 0x80 ))
      return(( size_t)(
        ( utf8_chr[ 0 ] & 0x1F ) << 6 ) |
        ( utf8_chr[ 1 ] & 0x3f ));
  else if ((( utf8_chr[ 0 ] & ( unsigned char ) 0xF0 ) == 0xE0 ) &&
    (( utf8_chr[ 1 ] & ( unsigned char ) 0xC0 ) == 0x80 ) &&
    (( utf8_chr[ 2 ] & ( unsigned char ) 0xC0 ) == 0x80 ))
      return(( size_t)(
        (( utf8_chr[ 0 ] & 0x0F ) << 12 ) |
        (( utf8_chr[ 1 ] & 0x3f ) << 6 ) |
        ( utf8_chr[ 2 ] & 0x3f )));
  else if ((( utf8_chr[ 0 ] & ( unsigned char ) 0xF8 ) == 0xF0 ) &&
    (( utf8_chr[ 1 ] & ( unsigned char ) 0xC0 ) == 0x80 ) &&
    (( utf8_chr[ 2 ] & ( unsigned char ) 0xC0 ) == 0x80 ) &&
    (( utf8_chr[ 3 ] & ( unsigned char ) 0xC0 ) == 0x80 ))
      return(( size_t)(
        (( utf8_chr[ 0 ] & 0x07 ) << 18 ) |
        (( utf8_chr[ 1 ] & 0x3f ) << 12 ) |
        (( utf8_chr[ 2 ] & 0x3f ) << 6 ) |
        ( utf8_chr[ 3 ] & 0x3f )));
  else  return(( size_t) 0xFFFD );  /* not valid utf-8 so use replacement char symbol ? in a diamond */
}


/* convert Code Point to UTF-8 character - again somewhat ugly brute force method */
unsigned char *  convCodePointToUTF8( wchar_t codePoint, unsigned char  utf8_chr[] )  {
  unsigned char *  mbPtr = utf8_chr;

  if ( codePoint < (wchar_t) 0x80 ) {  /* Effectively an ASCII character U+0000 to U+007F */
    mbPtr[ 0 ] = ( unsigned char ) codePoint;
    mbPtr[ 1 ] = mbPtr[ 2 ] = mbPtr[ 3 ] = mbPtr[ 4 ] = ( unsigned char ) 0;
    return( utf8_chr );
  }
  else if ( codePoint < (wchar_t) 0x800 ) {  /* U+0080 to U+07FF */
    mbPtr[ 0 ] = 0xC0 | ( unsigned char )(( codePoint >> 6 ) & 0x1F );
    mbPtr[ 1 ] = 0x80 | ( unsigned char )( codePoint & 0x3F );
    mbPtr[ 2 ] = mbPtr[ 3 ] = mbPtr[ 4 ] = ( unsigned char ) 0;
    return( utf8_chr );
  }
  else if ( codePoint < (wchar_t) 0x10000 ) {  /* U+0800 to U+FFFF */
    mbPtr[ 0 ] = 0xE0 | ( unsigned char )(( codePoint >> 12 ) & 0x0F );
    mbPtr[ 1 ] = 0x80 | ( unsigned char )(( codePoint >> 6 ) & 0x3F );
    mbPtr[ 2 ] = 0x80 | ( unsigned char )( codePoint & 0x3F );
    mbPtr[ 3 ] = mbPtr[ 4 ] = ( unsigned char ) 0;
    return( utf8_chr );
  }
  else if ( codePoint < (wchar_t) 0x110000 ) {  /* U+10000 to U+10FFFF */
    mbPtr[ 0 ] = 0xF0 | ( unsigned char )(( codePoint >> 18 ) & 0x07 );
    mbPtr[ 1 ] = 0x80 | ( unsigned char )(( codePoint >> 12 ) & 0x3F );
    mbPtr[ 2 ] = 0x80 | ( unsigned char )(( codePoint >> 6 ) & 0x3F );
    mbPtr[ 3 ] = 0x80 | ( unsigned char )( codePoint & 0x3F );
    mbPtr[ 4 ] = ( unsigned char ) 0;
    return( utf8_chr );  /* U+10000 to U+10FFFF */
  }
  return(( unsigned char * ) NULL );  /* no utf-8 conversion done */
}


void  outputSpacePadding( int  padCnt, FILE *  ofp)  {
  int  index;

  for ( index = 0; index < padCnt; ++index)  fputwc( L' ', ofp );   /* Output the required number of spaces */
}


void  outputWideCharArrayAsCodePointsIfEnabled( struct config *  cfg, int  wChrIn_wcArray, wchar_t *  wcArray, FILE *  ofp )  {
  int  wcArrayIndex;

  /* Output the code point representation unless disabled */
  if ( cfg->C.active )  {
    /* Output the code points */
    fwprintf( ofp, L" ");
    for ( wcArrayIndex = 0; wcArrayIndex < wChrIn_wcArray; wcArrayIndex++)  {
      fwprintf( ofp, L" X+%05x", wcArray[ wcArrayIndex ] );
    }
  }
}


int  outputWideCharArrayWithSubstitutesForLineFormatDisrupters( int  wChrIn_wcArray, wchar_t *  wcArray, FILE *  ofp )  {
  int  index;
  int  substitutions;   /* keep a count of substitution in the output */
  wchar_t  wchar;

  /* be hyper-careful in case it makes some difference to force output of any previous left to right text */
  /* before possibly sending out right to left text UTF-8 text in the following loop */
  fflush( ofp);
  substitutions = 0;
  for ( index = 0; index < wChrIn_wcArray; index++)  {
    if ( wcArray[ index ] == L'\377' )  {
      fputwc( 0x2412, ofp );      /* An ASCII DEL */
      substitutions += 1;
    }
    else if (( wcArray[ index ] >= L'\0' ) && ( wcArray[ index ] <= L' ' ))  {  /* ASCII control characters including space */
      wchar = ( wchar_t )( 0x2400 + wcArray[ index ] );
      fputwc( wchar, ofp );
      substitutions += 1;
    }
    else fputwc( wcArray[ index ], ofp );   /* Output the character/symbol ( if not line disrupter control character)*/
  }
  return( substitutions );
}


int  outputWideCharArray( struct config *  cfg, int  wChrIn_wcArray, wchar_t *  wcArray, FILE *  ofp )  {
  int  index;
  int  substitutes = 0;

  /* If more information is requested i.e. index, hex dump or ascii dump then format the output ignoring CR & LF etc. */
  if ( cfg->I.active || cfg->H.active || cfg->A.active || cfg->C.active )
    substitutes = outputWideCharArrayWithSubstitutesForLineFormatDisrupters( wChrIn_wcArray, wcArray, ofp );
  else  {   /* Output the characters / symbols according to their own formatting */
    for ( index = 0; index < wChrIn_wcArray; index++)
      fputwc( wcArray[ index ], ofp );   /* Output the character/symbol */
  }
  /* book-end the wide character output with a flush of the output buffer */
  fflush( ofp);
  return( substitutes );
}


void  outputAsciiArrayIfRequired( struct config *  cfg, int  nChrIn_ncArray, unsigned char *  ncArray, int postPad, FILE *  ofp )  {
  int  index;
  unsigned char *  asciiChrPtr;
  wchar_t  wchar;

  if ( cfg->A.active )  {
    asciiChrPtr = ncArray;
    for ( index = 0; index < nChrIn_ncArray; index++, asciiChrPtr++ )  {
      if ( *asciiChrPtr == 0x7F )  {
        wchar = ( wchar_t ) 0x2421;
        fputwc( wchar, ofp );
      }
      else if( *asciiChrPtr > 0x7f )  {
        fputwc( L'.', ofp);
      }
      else if (( *asciiChrPtr >= 0 ) && ( *asciiChrPtr <= 0x20 ))  {
        wchar = ( wchar_t )( 0x2400 + *asciiChrPtr );
        fputwc( wchar, ofp );
      }
      else  fputwc( *asciiChrPtr, ofp );
    }
    /* ensure gap to start of next column ( utf-8 characters / symbols ) */
    outputSpacePadding( postPad + 2, ofp);
    fflush( ofp);
  }
}


void  debug_inArrayInternalFormat( unsigned char  inArray[], FILE *  dbg_fp )  {
  int  i;
  size_t  codePoint;
  size_t  wCharLen, local_wCharLen;
  unsigned char  testArray[ 5 ];
  mbstate_t  mbs2;

  fflush( dbg_fp );   /* be careful in-case debug info isn't going to stdout like we assume else where */
  wprintf( L"\nDebug: inArray: multi-byte string: " );
  for ( i = 0; i < 4; i++ )  fwprintf( dbg_fp, L"%02x ", inArray[ i ]);
  fwprintf( dbg_fp, L"=> Code Point:" );
  fwprintf( dbg_fp, L" U+%05x ", ( codePoint = conv_UTF8_ToCodePoint( inArray )));
  fwprintf( dbg_fp, L"=> Character/Symbol: '" );
  if ( outputWideCharArrayWithSubstitutesForLineFormatDisrupters( 1, ( wchar_t * ) &codePoint, dbg_fp ) == 0 )
    fwprintf( dbg_fp, L"' (no " );
  else  fwprintf( dbg_fp, L"' (" );
  fwprintf( dbg_fp, L"substitution made), UTF-8 string: " );
  if ( convCodePointToUTF8( codePoint, testArray ) != NULL )  {
    for ( i = 0; i < 4; i++ )  fwprintf( dbg_fp, L" %02x", testArray[ i ]);
  }
  fputwc( L'\n', dbg_fp );
  memset( &mbs2, 0, sizeof(mbs2));
  wCharLen = mbrlen((char *) inArray, ( size_t ) 4, &mbs2 );  /* get number of bytes in the character with stdlib call */
  local_wCharLen = sizeOf_UTF8_Char( inArray );   /* get number of bytes in the character with in-house written function */
  if ( wCharLen != local_wCharLen )
    fwprintf( dbg_fp, L"Debug: multi-byte char length from mbrlen() is %lu bytes vs local: %lu\n", wCharLen, local_wCharLen );
  fflush( dbg_fp );   /* be careful in-case debug info isn't going to stdout like we assume else where */
}


int  outputData( struct config *  cfg, FILE *  fp, FILE *  ofp )  {      /* produce wide character output from a file */
  int  i, c, rmCnt, activeBytesInBuffer, chrIn_cArray, size;
  int  byteCnt, lineCnt;
  int  wChrIn_wcArray, wChrOutput, substitutions;
  unsigned char  inArray[ 5 ];
  unsigned char *  inPtr;
  char  pStr[ 80 ];    /* Storage for perror() string */
  unsigned char  cArray[ ARRAY_SIZE ];     /* Storage for byte size characters */
  wchar_t  wcArray[ ARRAY_SIZE ]; /* Storage for wide characters */
  wchar_t *  wcPtr;
  mbstate_t  mbs;
  size_t  wCharLen, local_wCharLen;

  c = wChrIn_wcArray = wChrOutput = substitutions = chrIn_cArray = byteCnt = lineCnt = 0;
  activeBytesInBuffer = 0;
  rmCnt = 4;  /* Need a complete fill of inArray() */
  wcPtr = wcArray;
  memset( &mbs, 0, sizeof(mbs));
  size = ( cfg->w.active ) ? cfg->w.optionInt : cfg->w.defaultVal;
  /* Loop to read bytes from a file into an array - try to ensure 4 unprocessed bytes in the Array each time */
  while (( activeBytesInBuffer = removeCntBytesFromTheStartOfTheArrayAndReplaceThemWithNewEndBytes(
      fp, inArray, rmCnt, 4, activeBytesInBuffer )) > 0 )  {   /* stop loop when no unprocessed bytes available */
    /* The inArray has least 1 byte available to process and hopefully a UTF-8 char ( of 1 to 4 bytes ) */
    inPtr = inArray;
    if ( cfg->D.active )    /* Provide more info on the low level data */
      debug_inArrayInternalFormat( inArray, stdout );   /* send debug info stdout */
    wCharLen = mbrtowc( wcPtr, (char *) inArray, ( size_t ) 4, &mbs );      /* put valid utf-8 mb char into wide chr string */
    local_wCharLen = sizeOf_UTF8_Char( inArray );   /* get number of bytes in the character with in-house written function */
    if (( wCharLen > (size_t) 0 ) && ( wCharLen <= (size_t) 4 ))  wcPtr++;  /* success so increment pointer */
    else  {   /* might be a correctly determined error on MacOS or incorrectly flagged on linux so try local work-around */
      *wcPtr = conv_UTF8_ToCodePoint( inArray );  /* try to work around linux difficulties with local routine */
      if ( *wcPtr == 0xfffd )  wCharLen = (size_t) -1;  /* flag error in local conversion */
      else  {
	wCharLen = local_wCharLen;    /* procede with the assumption that the local routine worked correctly */
	wcPtr++;
      }
    }
    if (( wCharLen != local_wCharLen ) && cfg->v.active)
      fprintf( stderr, "\nWarning: length of UTF-8 character discrepancy between stdlib mbrtowc(): %lu and local: %lu)\n", wCharLen, local_wCharLen );
    if ( cfg->D.active )  {
      wprintf( L"\nDebug: Wide Char length from mbrtowc()) is %lu (local calc is %lu)\n", wCharLen, local_wCharLen );
    }
    /* Handle an encoding error if there was one */
    if ( wCharLen == (size_t) -1 )  {
      if ( cfg->v.active )  {
        snprintf( pStr, 80, "\nError: Encoding Error (starting) at byte count %d for char 0x%02x", byteCnt, *inArray );
        perror( pStr );       /* report error to stderr */
      }
      *wcPtr++ = 0xfffd;    /* substitute a question mark in-place of the invalid wide character */
      substitutions += 1;   /* track number of substutions */
      wCharLen = 1;
    } /* encoding error */
    /* Handle an incomplete encoding error if there was one */
    if ( wCharLen == (size_t) -2 )  {
      if ( cfg->v.active )  {
        snprintf( pStr, 80, "\nError: Incomplete Error starting at byte count %d for char 0x%02x", byteCnt, *inArray );
        perror( pStr );
      } /* incomplete error */
      wCharLen = 1;
    }  /* incomplete decode - more bytes needed */
    if ( wCharLen == (size_t) 0 )  {
      if ( cfg->D.active )  wprintf( L"\nDebug: found wide null character\n");
      *wcPtr++ = L'\0';    /* substitute a NULL in-place of the 0x00 (NULL) character */
      wCharLen = 1;
    }   /* found wide null character */
    wChrIn_wcArray += 1;
    for ( i = ( int ) wCharLen; i > 0; i-- )  {
      c = ( int ) *inPtr++;   /* get first byte as a narrow char */
      /* Output a new line if any of -IHAU options are activated and output the Decimal Index if required */
      if ( chrIn_cArray == 0 )  {
        if (( lineCnt > 0 ) && ( cfg->I.active || cfg->H.active || cfg->A.active || cfg->C.active ))
          fputwc( L'\n', ofp );
        if ( cfg->I.active )  fwprintf( ofp, L"%10d  ", byteCnt );    /* Output Decimal Index and 2 space gap if enabled */
      }
      byteCnt += 1;   /* update the byte count after the index is output, if it is required */
      /* Output the Hex representation of each byte */
      c &= BYTEMASK;    /* Ensure int is between 0x00 and 0xff */
      if ( cfg->H.active )  fwprintf( ofp, L"%02x ", c );    /* Output Hex if enabled */
      /* Accumulate an array of bytes to be treated as Ascii char equivalent to the bytes */
      cArray[ chrIn_cArray++ ] = ( unsigned char ) c;
      /* Output the Ascii equivalents if line of Hex is complete */
      if ( chrIn_cArray == size )  {
        if ( cfg->H.active)  fwprintf( ofp, L" ");    /* ensure 2 space gap if hex already output */
        outputAsciiArrayIfRequired( cfg, chrIn_cArray, cArray, 0, ofp );
        /* Output the wide characters */
        substitutions += outputWideCharArray( cfg, wChrIn_wcArray, wcArray, ofp );
        wChrOutput += wChrIn_wcArray;   /* keep running total of characters/symbols output */
        /* Output the wide characters in code point form if enabled */
        if ( cfg->C.active )  outputSpacePadding( size - wChrIn_wcArray, ofp );
        outputWideCharArrayAsCodePointsIfEnabled( cfg, wChrIn_wcArray, wcArray, ofp );
        lineCnt += 1;
        chrIn_cArray = wChrIn_wcArray = 0;
        wcPtr = wcArray;
      }
    }
    rmCnt = ( int ) wCharLen;
    activeBytesInBuffer -= rmCnt;
  }
  /* Handle incomplete last line of output if there is one */
  if ( chrIn_cArray > 0 )  {  /* if incomplete and if there is a hex column, it will need padding */
    if ( cfg->H.active )  outputSpacePadding( 3 * ( size - chrIn_cArray) + 1, ofp);
    outputAsciiArrayIfRequired( cfg, chrIn_cArray, cArray, size - chrIn_cArray, ofp );
  }
  if ( wChrIn_wcArray > 0 ) {
    /* Output the characters / symbols */
    substitutions += outputWideCharArray( cfg, wChrIn_wcArray, wcArray, ofp );
    wChrOutput += wChrIn_wcArray;   /* keep running total of characters/symbols output */
    /* Output the code point representation if enabled */
    if ( cfg->C.active )  outputSpacePadding( size - wChrIn_wcArray, ofp);
    outputWideCharArrayAsCodePointsIfEnabled( cfg, wChrIn_wcArray, wcArray, ofp );
  }
  if(( byteCnt > 0 ) && ( cfg->I.active || cfg->H.active || cfg->A.active || cfg->C.active ))  {
    fputwc( L'\n', ofp );
    lineCnt += 1;
  }
	if( cfg->D.active || cfg->v.active )  {
    fwprintf( ofp, L"Info: %d Bytes, %d Code Points output, %d substitutions\n", byteCnt, wChrOutput, substitutions );
  }
  fflush( ofp );
  return( byteCnt );
}


int  processA_SingleCommandLineParameter( struct config *  cfg, FILE *  ofp, char *  nameStrng ) {
	int  returnVal;
	long  byteCnt;
	FILE *  fp;

/* Open the file for reading */
	returnVal = (( fp = fopen( nameStrng, "r" )) == NULL );
	if( returnVal )  {
		fprintf( stderr, "Warning: Unable to open a file named \"%s\" for reading\n", nameStrng );
		perror( nameStrng );
	}
	else  {	/* Begin file opened ok block */
	  if( cfg->D.active || cfg->v.active )
    	fwprintf( ofp, L"Info: Start of File \"%s\"\n", nameStrng );
		byteCnt = outputData( cfg, fp, ofp );   /* if byte count returned is negetive then an error occurred */
	  /* Close the file just processed */
		returnVal = ( fclose( fp ) != 0 );
	  if( returnVal )  {
  		fprintf( stderr, "Warning: Unable to close the file named \"%s\"\n", nameStrng );
			perror( nameStrng );
		}
    if( byteCnt < 0 ) {
  		fprintf( stderr, "Warning: Failure while processing the file named \"%s\"\n", nameStrng );
      byteCnt *= -1;    /* convert error indication to a positive byte count */
      returnVal = TRUE;
    } 
	  if( cfg->D.active || cfg->v.active )
    	fwprintf( ofp, L"Info: End of File \"%s\" (%ld data bytes processed)\n", nameStrng, byteCnt );
	}	/* End of file opened ok block */
	return( returnVal );
}


int  processNonConfigCommandLineParameters( struct config *  cfg, int  frstIndx, int  lstIndx, char *  cmdLnStrngs[] )  {
  int  returnVal;	/* returnVal is incremented for each error */
  int  result, indx;
  int  chrCnt;
  FILE *  ofp = stdout;

  if( cfg->D.active )  {
    wprintf( L"Debug: Executing: processNonSwitchCommandLineParameters()\n" );
    wprintf( L"Debug: first index is %d and last index is %d\n", frstIndx, lstIndx );
    for( indx = frstIndx; indx <= lstIndx; indx++ )
      wprintf( L"Debug: cmdLnStrngs[ %d ] string is \"%s\"\n", indx, cmdLnStrngs[ indx ] );
  }
  returnVal = ( cfg->o.active && (( cfg->o.optionStr == ( char * ) NULL ) || ( *cfg->o.optionStr == '\0' )));
  if( returnVal )
    fprintf( stderr, "Warning: -o specified, but no output file name specified - aborting\n" );
  else  {
    /* Attempt to open the output file if required */
    returnVal = ( cfg->o.active ) ? (( ofp = fopen( cfg->o.optionStr, "w" )) == NULL ) : 0;
    if( returnVal )  {
      fprintf( stderr, "Warning: Unable to open a file named \"%s\" for writing output- aborting\n", cfg->o.optionStr );
      if( cfg->D.active )  perror( cfg->o.optionStr );
    }
    else  {
      if((( lstIndx + 1 ) == frstIndx ) ||
        (( lstIndx  == frstIndx ) && ( strncmp( cmdLnStrngs[ frstIndx ], "-", 1 ) == 0 )))  {
        /* There are no files specified in the command line or the filename is "-" so process stdin */
        if( cfg->D.active )
          wprintf( L"Debug: Reading bytes from stdin - mark end of any manually typed input with EOT (i.e. ^D)\n" );
        chrCnt = outputData( cfg, stdin, ofp );
        if( cfg->D.active || cfg->v.active )  wprintf( L"Processed %d chars from stdin\n", chrCnt );
      }
      else  {
        /* Process each file specified in the command line */
        for( indx = frstIndx; indx <= lstIndx; indx++ )  {
    			result = processA_SingleCommandLineParameter( cfg, ofp, cmdLnStrngs[ indx ] );
		    	returnVal += result;	/* Count the unsuccessful results */
    			if( cfg->D.active )
            wprintf( L"Debug: Processing \"%s\" returned result %d, returnVal %d\n", cmdLnStrngs[ indx ], result, returnVal );
        }
      }
      fflush( ofp );  /* ensure output queue is flushed */
      /* Close output file specified in the command line, if there was one */
      result = ( cfg->o.active ) ? (fclose( ofp ) != 0) : 0;
      if( result )  {
        fprintf( stderr, "Warning: Attempt to close the output file named \"%s\" failed\n", cfg->o.optionStr );
        if( cfg->D.active )  perror( cfg->o.optionStr );
      }
      returnVal += result;	/* Count any unsuccessful fclose() on output file */
    }
  }
  fflush( stderr );
  return( returnVal );
}
