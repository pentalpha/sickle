#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>
#include <unistd.h>
#include "sickle.h"
#include "FQEntry.h"


void print_record (FILE *fp, FQEntry &fqr, cutsites *cs) {
    fprintf(fp, "%s", fqr.name.c_str());
    if (fqr.comment.length()) fprintf(fp, " %s\n", fqr.comment.c_str());
    else fprintf(fp, "\n");
    fprintf(fp, "%s\n", fqr.seq.substr(cs->five_prime_cut, cs->three_prime_cut - cs->five_prime_cut).c_str());
    fprintf(fp, "+\n");
    fprintf(fp, "%s\n", fqr.qual.substr(cs->five_prime_cut, cs->three_prime_cut - cs->five_prime_cut).c_str());
}

void print_record_gzip (gzFile fp, FQEntry &fqr, cutsites *cs) {
    gzprintf(fp, "%s", fqr.name.c_str());
    if (fqr.comment.length()) gzprintf(fp, " %s\n", fqr.comment.c_str());
    else gzprintf(fp, "\n");
    gzprintf(fp, "%s\n", fqr.seq.substr(cs->five_prime_cut, cs->three_prime_cut - cs->five_prime_cut).c_str());
    gzprintf(fp, "+\n");
    gzprintf(fp, "%s\n", fqr.qual.substr(cs->five_prime_cut, cs->three_prime_cut - cs->five_prime_cut).c_str());
}

void print_record_N (FILE *fp, FQEntry &fqr, int qualtype) {
    fprintf(fp, "%s", fqr.name.c_str());
    if (fqr.comment.length()) fprintf(fp, " %s\n", fqr.comment.c_str());
    else fprintf(fp, "\n");
    fprintf(fp, "N\n+\n%c\n", quality_constants[qualtype][Q_MIN]);
}

void print_record_N_gzip (gzFile fp, FQEntry &fqr, int qualtype) {
    gzprintf(fp, "%s", fqr.name.c_str());
    if (fqr.comment.length()) gzprintf(fp, " %s\n", fqr.comment.c_str());
    else gzprintf(fp, "\n");
    gzprintf(fp, "N\n+\n%c\n", quality_constants[qualtype][Q_MIN]);
}

