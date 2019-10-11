#ifndef _TRIMSINGLE_
#define _TRIMSINGLE_

#include <vector>
#include "trim.h"

class Trim_Single : public Abstract_Trimmer{
public:
    Trim_Single();
    int parse_args(int argc, char *argv[]);

    int trim_main();
    void processing_thread(std::vector<FQEntry*>* local_queue, bool* filtered, 
        cutsites** saved_cutsites, long last_index, int thread_n);
    void usage(int status, char const *msg);
    void output_single(std::vector<std::vector<FQEntry*>* > queues, bool** filtered_reads, 
        cutsites*** saved_cutsites, vector<long> last_index, Batch* batch);
    int init_streams();
    void close_streams();
};

#endif