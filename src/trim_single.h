#ifndef _TRIMSINGLE_
#define _TRIMSINGLE_

#include "trim.h"
#include <queue>

class Trim_Single : public Abstract_Trimmer{
public:
    Trim_Single();
    int parse_args(int argc, char *argv[]);

    int trim_main();
    void processing_thread(std::queue<FQEntry*>* local_queue, int thread_n);
    void usage(int status, char const *msg);
    void output_single(FQEntry* fqrec, cutsites* p1cut);
    int init_streams();
    void close_streams();
};

#endif