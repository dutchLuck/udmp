/*
 * C O N F I G . C
 *
 * Last Modified on Mon Jun 23 19:14:05 2025
 *
 * 2025-Jun-22 Added Chr option handling
 */

#include <stdio.h>    /* printf() */
#include <stdlib.h>   /* floating point double */
#include <unistd.h>   /* getopt() */
#include <ctype.h>    /* isprint()*/
#include <limits.h>   /* INT_MIN INT_MAX LONG_MIN LONG_MAX */
#include <float.h>    /* DBL_MIN DBL_MAX */
#include <string.h>   /* strchr() */
#include "config.h"   /* struct config */
#include "genFun.h"   /* convertOptionStringToInteger() limitIntegerValueToEqualOrWithinRange() */


int  configureIntegerOption( struct optInt *  intStructPtr, char *  intString ) {
  intStructPtr->active = TRUE;
  intStructPtr->optionInt = convertOptionStringToInteger( intStructPtr->defaultVal, intString,
    intStructPtr->optID, &intStructPtr->active, TRUE );
  intStructPtr->optionInt = limitIntegerValueToEqualOrWithinRange( intStructPtr->optionInt,
    intStructPtr->mostNegLimit, intStructPtr->mostPosLimit );
  return( intStructPtr->active );
}


int  configureLongOption( struct optLng *  longStructPtr, char *  longString ) {
  longStructPtr->active = TRUE;
  longStructPtr->optionLng = convertOptionStringToLong( longStructPtr->defaultVal, longString,
    longStructPtr->optID, &longStructPtr->active, TRUE );
  longStructPtr->optionLng = limitLongValueToEqualOrWithinRange( longStructPtr->optionLng,
    longStructPtr->mostNegLimit, longStructPtr->mostPosLimit );
  return( longStructPtr->active );
}


int  configureDoubleOption( struct optDbl *  dblStructPtr, char *  dblString ) {
  dblStructPtr->active = TRUE;
  dblStructPtr->optionDbl = convertOptionStringToDouble( dblStructPtr->defaultVal, dblString,
    dblStructPtr->optID, &dblStructPtr->active, TRUE );
  dblStructPtr->optionDbl = limitDoubleValueToEqualOrWithinRange( dblStructPtr->optionDbl,
    dblStructPtr->mostNegLimit, dblStructPtr->mostPosLimit );
  return( dblStructPtr->active );
}


int  configureChrOption( struct optChr *  chrStructPtr, char *  chrString )  {
  size_t  len;

  len = strlen( chrString );
  chrStructPtr->active = TRUE;
  chrStructPtr->optionChr = ( int ) *chrString;
  if (( *chrString == '\\' ) && ( len > ( size_t ) 1 ) && ( chrString[ 1 ] != '\\'))  {
    switch ( chrString[ 1 ] )  {
      case '0' : chrStructPtr->optionChr = '\0'; break;   /* NULL */
      case 'a' : chrStructPtr->optionChr = '\a'; break;   /* audible bell */
      case 'b' : chrStructPtr->optionChr = '\b'; break;   /* backspace */
      case 'f' : chrStructPtr->optionChr = '\f'; break;   /* form-feed */
      case 'n' : chrStructPtr->optionChr = '\n'; break;   /* newline */
      case 'r' : chrStructPtr->optionChr = '\r'; break;   /* carriage return */
      case 't' : chrStructPtr->optionChr = '\t'; break;   /* horizontal tab */
      case 'x' :
      case 'X' : {
        if ( len > ( size_t ) 3 )  {
          chrStructPtr->optionChr = convert2HexChrNibblesToInt( chrString + 2 );
        }
        else if ( len > ( size_t ) 2 )  {
          chrStructPtr->optionChr = convertHexChrNibbleToInt( chrString + 2 );
        }
        break;
      }
      default :  chrStructPtr->optionChr = ','; break;    /* reinstate comma */
    }
    chrStructPtr->optionChr = limitIntegerValueToEqualOrWithinRange( chrStructPtr->optionChr , 0, 127 );
  }
  return( chrStructPtr->active );
}
