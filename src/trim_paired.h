#ifndef _TRIMPAIRED_
#define _TRIMPAIRED_

#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include "trim.h"


class Trim_Paired : public Abstract_Trimmer{
public:
    Trim_Paired();
    int parse_args(int argc, char *argv[]);

    int trim_main();
    void usage(int status, char const *msg);

protected:
    std::string get_read_string(FQEntry* read, cutsites* cs);
    int init_streams();
    void processing_thread(
        std::vector<FQEntry*>* local_queue, std::vector<FQEntry*>* local_queue2,
        bool* filtered1, bool* filtered2,
        cutsites** cutsites1, cutsites** cutsites2,
        long last_index, int thread_n
    );
    void close_streams();
    void output_paired(std::vector<std::vector<FQEntry*>* > queues, std::vector<std::vector<FQEntry*>* > queue2,
        bool** filtered_reads, bool** filtered_reads2, 
        cutsites*** saved_cutsites, cutsites*** saved_cutsites2,
        vector<long> last_index);
    GZReader* input2;
    GZReader* input_inter;
    FILE *outfile2;      /* reverse output file handle */
    FILE *outfile_combo;         /* combined output file handle */
    FILE *outfile_single;
    gzFile outfile2_gzip;
    gzFile combo_gzip;
    gzFile single_gzip;
    int combo_s, combo_all;
    
    char *outfn2;        /* reverse file out name */
    char *outfnc;        /* combined file out name */
    char *sfn;           /* single/combined file out name */
    char *infn2;         /* reverse input filename */
    char *infnc;         /* combined input filename */

    int kept_p;
    int discard_p;
    int kept_s1;
    int kept_s2;
    int discard_s1;
    int discard_s2;
};

#endif