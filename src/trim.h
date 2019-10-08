#ifndef _TRIM_
#define _TRIM_

#include "FQEntry.h"

int single_main (int argc, char *argv[]);
int paired_main (int argc, char *argv[]);
cutsites* sliding_window (FQEntry &fqrec, int qualtype, int length_threshold, int qual_threshold, int no_fiveprime, int trunc_n, int debug);
#endif