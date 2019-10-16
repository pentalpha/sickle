#ifndef _TRIM_
#define _TRIM_

#include <fstream>
#include "FQEntry.h"
#include "GZReader.h"

class Abstract_Trimmer{
public:
    virtual int parse_args(int argc, char *argv[]) = 0;
    virtual int trim_main() = 0;
    virtual void usage(int status, char const *msg) = 0;
protected:
    cutsites* sliding_window(FQEntry &fqrec);
    int get_quality_num (char qualchar, FQEntry &fqrec, int pos);
    int qualtype;
    int length_threshold;
    int qual_threshold;
    int no_fiveprime;
    int trunc_n;
    int debug;

    int threads, batch_len;

    GZReader* input;
    std::ofstream outfile;
    gzFile outfile_gzip;
    char *outfn;
    char *infn;
    int quiet;
    int gzip_output;

    int kept;
    int discard;
    int total;

    bool writing_results_flag;
};

#endif