#ifndef PRINT_RECORD_H
#define PRINT_RECORD_H

#include <stdio.h>
#include <zlib.h>
#include "FQEntry.h"

void print_record (FILE *fp, FQEntry &fqr, cutsites *cs);
void print_record_gzip (gzFile fp, FQEntry &fqr, cutsites *cs);
void print_record_N (FILE *fp, FQEntry &fqr, int qualtype);
void print_record_N_gzip (gzFile fp, FQEntry &fqr, int qualtype);

#endif /* PRINT_RECORD_H */
