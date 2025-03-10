/*
 * C O N F I G . H
 *
 * Last Modified on Mon Mar 10 15:08:59 2025
 *
 */

#include <stdio.h>

#ifndef  CONFIG_H
#define  CONFIG_H

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!(FALSE))
#endif

struct positionParam {
  char *  paramNameStr;
  char *  helpStr;
};

struct optFlg {
  int active;
  char *  optID;
  char *  helpStr;
};

struct optChr {
  int active;
  char *  optID;
  char *  helpStr;
  int  optionChr;
};

struct optStr {
  int active;
  char *  optID;
  char *  helpStr;
  char *  optionStr;
};

struct optInt {
  int active;
  char *  optID;
  char *  helpStr;
  int defaultVal;
  int mostPosLimit;
  int mostNegLimit;
  int optionInt;
};

struct optLng {
  int active;
  char *  optID;
  char *  helpStr;
  long defaultVal;
  long mostPosLimit;
  long mostNegLimit;
  long optionLng;
};

struct optDbl {
  int active;
  char *  optID;
  char *  helpStr;
  double defaultVal;
  double mostPosLimit;
  double mostNegLimit;
  double optionDbl;
};

// Command Line Options Configuration Data
struct config {
  struct positionParam posParam1;  /* (posParam1) Optional Name(s) of File(s) to process */
  struct optFlg A;  /* (ascii) ...... enable ascii column in output mode */
  struct optFlg C;  /* (code_point) ...... enable utf code point colummn in output */
  struct optFlg D;  /* (debug) ...... enable debug output mode */
  struct optFlg h;  /* (help) ...... this help / usage information */
  struct optFlg H;  /* (hex) ...... enable hexadecimal column in output */
  struct optFlg I;  /* (index) ...... enable index column in output mode */
  struct optStr o;  /* (output_file) TXT .. send the output to a file named TXT */
  struct optFlg v;  /* (verbose) ...... enable more verbose information output */
  struct optFlg V;  /* (version) ...... enable version information output */
  struct optInt w;  /* (width) INT .. output INT bytes per line - where 1 <= INT <= 128 */
};

// getopt() option string
#define OPTIONS ":ACDhHIo:vVw:"

void  usage ( struct config *  optStructPtr, char *  exeName );
void  initConfiguration ( struct config *  optStructPtr );
int  setConfiguration ( int  argc, char *  argv[], struct config *  optStructPtr );
void  configuration_status( struct config *  opt );

#endif
