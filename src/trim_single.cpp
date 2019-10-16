#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <iostream>
#include <queue>
#include <sstream>
#include <thread>
#include <functional>
#include <chrono>

#include "FQEntry.h"
#include "sickle.h"
#include "trim_single.h"
#include "GZReader.h"

static struct option single_long_options[] = {
    {"fastq-file", required_argument, 0, 'f'},
    {"output-file", required_argument, 0, 'o'},
    {"qual-type", required_argument, 0, 't'},
    {"qual-threshold", required_argument, 0, 'q'},
    {"length-threshold", required_argument, 0, 'l'},
    {"no-fiveprime", no_argument, 0, 'x'},
    {"discard-n", no_argument, 0, 'n'},
    {"gzip-output", no_argument, 0, 'g'},
    {"quiet", no_argument, 0, 'z'},
    {"threads", no_argument, 0, 'a'},
    {"batch", no_argument, 0, 'b'},
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {NULL, 0, NULL, 0}
};

void Trim_Single::usage(int status, char const *msg) {

    fprintf(stderr, "\nUsage: %s se [options] -f <fastq sequence file> -t <quality type> -o <trimmed fastq file>\n\
\n\
Options:\n\
-f, --fastq-file, Input fastq file (required)\n\
-t, --qual-type, Type of quality values (solexa (CASAVA < 1.3), illumina (CASAVA 1.3 to 1.7), sanger (which is CASAVA >= 1.8)) (required)\n\
-o, --output-file, Output trimmed fastq file (required)\n", PROGRAM_NAME);

    fprintf(stderr, "-q, --qual-threshold, Threshold for trimming based on average quality in a window. Default 20.\n\
-l, --length-threshold, Threshold to keep a read based on length after trimming. Default 20.\n\
-x, --no-fiveprime, Don't do five prime trimming.\n\
-n, --trunc-n, Truncate sequences at position of first N.\n\
-g, --gzip-output, Output gzipped files.\n\
-a, --threads, Number of threads to use. Default and minimum: 1.\n\
-b, --batch, MB of data to read from the input file at each cycle.\n\
\tThe greater the value, the greater the memory usage. The value, multiplied by 1024^2, must be \n\
\tbigger than the lenght of the longest read. Minimum 1. Default: 512.\n\
--quiet, Don't print out any trimming information\n\
--help, display this help and exit\n\
--version, output version information and exit\n\n");

    if (msg) fprintf(stderr, "%s\n\n", msg);
    exit(status);
}

Trim_Single::Trim_Single(){
    //msg("Building trimmer");
    threads=1;
    batch_len=1024*1024*DEFAULT_BATCH_LEN;

    qualtype = -1;
    length_threshold = 20;
    qual_threshold = 20;
    no_fiveprime = 0;
    trunc_n = 0;
    debug = 0;
    input = NULL;
    outfile_gzip = NULL;
    outfn = NULL;
    infn = NULL;
    quiet = 0;
    gzip_output = 0;
    //msg("Finished build trimmer");
}

int Trim_Single::parse_args(int argc, char *argv[]){
    int optc;
    extern char *optarg;

    std::cout << "Setting se trimming params\n";
    while (1) {
        int option_index = 0;
        optc = getopt_long(argc, argv, "df:t:o:q:a:b:l:zxng", single_long_options, &option_index);

        if (optc == -1)
            break;

        switch (optc) {
            if (single_long_options[option_index].flag != 0)
                break;

        case 'f':
            infn = (char *) malloc(strlen(optarg) + 1);
            strcpy(infn, optarg);
            break;

        case 't':
            if (!strcmp(optarg, "illumina"))
                qualtype = ILLUMINA;
            else if (!strcmp(optarg, "solexa"))
                qualtype = SOLEXA;
            else if (!strcmp(optarg, "sanger"))
                qualtype = SANGER;
            else {
                fprintf(stderr, "Error: Quality type '%s' is not a valid type.\n", optarg);
                return EXIT_FAILURE;
            }
            break;

        case 'o':
            outfn = (char *) malloc(strlen(optarg) + 1);
            strcpy(outfn, optarg);
            break;

        case 'q':
            qual_threshold = atoi(optarg);
            if (qual_threshold < 0) {
                fprintf(stderr, "Quality threshold must be >= 0\n");
                return EXIT_FAILURE;
            }
            break;

        case 'l':
            length_threshold = atoi(optarg);
            if (length_threshold < 0) {
                fprintf(stderr, "Length threshold must be >= 0\n");
                return EXIT_FAILURE;
            }
            break;

        case 'x':
            no_fiveprime = 1;
            break;

        case 'n':
            trunc_n = 1;
            break;

        case 'g':
            gzip_output = 1;
            break;

        case 'z':
            quiet = 1;
            break;

        case 'd':
            debug = 1;
            break;

        case 'a':
            threads = atoi(optarg);
            break;

        case 'b':
            batch_len = 1024*1024*(atoi(optarg));
            break;

        case_GETOPT_HELP_CHAR(usage)
        case_GETOPT_VERSION_CHAR(PROGRAM_NAME, VERSION, AUTHORS);

        case '?':
            usage(EXIT_FAILURE, NULL);
            break;

        default:
            usage(EXIT_FAILURE, NULL);
            break;
        }
    }


    if (qualtype == -1 || !infn || !outfn) {
        usage(EXIT_FAILURE, "****Error: Must have quality type, input file, and output file.");
    }

    if (!strcmp(infn, outfn)) {
        fprintf(stderr, "****Error: Input file is same as output file.\n\n");
        return EXIT_FAILURE;
    }
    return 0;
}

int Trim_Single::trim_main() {
    std::cout << "trim_main()\n";

    kept=0;
    discard=0;
    total=0;

    int res = init_streams();
    if(res != 0){
        return res;
    }

    //msg("Creating queues");
    std::vector<std::vector<FQEntry*>* > queues;
    std::vector<long> queue_lens;
    std::vector<long> last_item;
    for (int i = 0; i < threads; i++){
        queues.push_back(new std::vector<FQEntry*>());
        queue_lens.push_back(0);
        last_item.push_back(-1);
    }
    bool** filtered_reads = new bool*[threads];
    cutsites*** saved_cutsites = new cutsites**[threads];
    //msg("Finished creating queues");
    Batch* batch = NULL;
    int last_read_position = 0;
    while(true){
        for (int i = 0; i < threads; i++){
            last_item[i] = -1;
            queue_lens[i] = 0;
        }
        msg("Reading new batch");
        batch = input->get_batch_buffering_lines();

        if(batch == NULL){
            msg("No batch returned, exiting.");
            break;
        }
        lock_guard<mutex> guard(batch_lock);
        //msg("Got the lock to process the batch");
        int batch_len = batch->sequences_len;
        //msg("batch seq len is ");
        //msg(to_string(batch_len));
        long chars_read_from_batch = 0;
        int max_queue_len = batch_len / threads;
        /*msg("Reading big batch");
        input->read_n_chars(batch_len);
        msg("Finished reading big batch");*/
        FQEntry* fqrec = NULL;
        //msg("Starting fqentries");
        int last_queue = threads-1;
        
        while(batch->has_lines()){
            fqrec = new FQEntry(last_read_position, batch);
            last_read_position = fqrec->position;

            int read_len = fqrec->seq.length();
            chars_read_from_batch += read_len;
            int smallest_queue = 0;
            bool fitted_in_a_queue = false;
            last_queue += 1;
            int next_queue = (last_queue + 1)%threads;
            int next_index = last_item[next_queue]+1;
            if(next_index < queues[next_queue]->size()){
                //msg("index");
                //msg(to_string(next_index));
                //msg("to");
                //msg(to_string(next_queue));
                //msg("of size");
                //msg(to_string(queues[next_queue]->size()));
                (*queues[next_queue])[next_index] = fqrec;
            }else{
                //msg("to");
                //msg(to_string(next_queue));
                queues[next_queue]->push_back(fqrec);
            }
            //queues[next_queue]->emplace(queues[next_queue]->begin()+last_item[next_queue]+1, fqrec);
            queue_lens[next_queue] += read_len;

            //fitted_in_a_queue = true;
            last_item[next_queue] += 1;

            //if(!fitted_in_a_queue){
                
            //}
        }

        for (int i = 0; i < threads; i++){
            filtered_reads[i] = new bool[queues[i]->size()];
            bool* array = filtered_reads[i];
            memset(array, false, sizeof(array[0])*queues[i]->size());
            /*for(int j = 0; j < queues[i]->size(); j++){
                if(array[j] != false){
                    error("Array is not all false");
                    exit(EXIT_FAILURE);
                }
            }*/
            saved_cutsites[i] = new cutsites*[queues[i]->size()];
            //msg(string("queue len for ") + to_string(i) + string(" is ") + to_string(queue_lens[i]));
            //msg(string("queue size for ") + to_string(i) + string(" is ") + to_string(queues[i]->size()));
        }

        msg(string("Batch length of ") + to_string(chars_read_from_batch));
        //msg(string("Max is ") + to_string(batch_len));

        if(chars_read_from_batch == 0){
            break;
        }

        //msg("Starting threads");
        vector<thread> running;
        for(int thread_n = 0; thread_n < threads; thread_n++){
            running.push_back(thread(&Trim_Single::processing_thread,
                this,
                queues[thread_n], filtered_reads[thread_n], saved_cutsites[thread_n], last_item[thread_n], thread_n)
            );
            //processing_thread(queues[thread_n], filtered_reads[thread_n], saved_cutsites[thread_n], thread_n);
        }

        //msg("Joining all");
        std::for_each(running.begin(),running.end(), std::mem_fn(&std::thread::join));

        writing_results_flag = true;
        thread output_thread(&Trim_Single::output_single,
            this,
            queues, filtered_reads, saved_cutsites, last_item, batch);
        output_thread.detach();
    }

    //#TODO: USELESS! the program still closes before the output finishs
    while(writing_results_flag){
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    if (!quiet) fprintf(stdout, "\nSE input file: %s\n\nTotal FastQ records: %d\nFastQ records kept: %d\nFastQ records discarded: %d\n\n", infn, total, kept, discard);

    //kseq_destroy(fqrec);
    //delete(fqrec);
    //gzclose(input);
    close_streams();

    return EXIT_SUCCESS;
}

void Trim_Single::processing_thread(std::vector<FQEntry*>* local_queue, bool* filtered,
    cutsites** saved_cutsites, long last_index, int thread_n)
{
    msg(string("Processing thread ") + to_string(thread_n) + string(", reads: ") + to_string(last_index+1));
    FQEntry* fqrec;
    //cutsites *p1cut;
    for(int i = 0; i <= last_index; i++){
        fqrec = local_queue->at(i);
        //msg("running sliding window");
        saved_cutsites[i] = sliding_window(*fqrec);
        //if (debug) printf("P1cut: %d,%d\n", p1cut->five_prime_cut, p1cut->three_prime_cut);
        if(!(saved_cutsites[i]->three_prime_cut >= 0)) filtered[i] = true;
        //output_single(fqrec, p1cut);
        //free(p1cut);
    }
}

void Trim_Single::output_single(std::vector<std::vector<FQEntry*>* > queues,
    bool** filtered_reads, cutsites*** saved_cutsites,
    vector<long> last_index, Batch* batch)
{
    std::stringstream to_print;
    msg("Making results string");
    lock_guard<mutex> guard(batch_lock);
    //msg("Got the lock to actually make it");
    for (size_t i = 0; i < threads; i++){
        //msg("Processing thread");
        if(!queues[i]->empty()){
            for (size_t j = 0; j <= last_index[i]; j++)
            {
                //msg("Parsing read");
                FQEntry* read = queues[i]->at(j);
                cutsites* cs = saved_cutsites[i][j];
                if(filtered_reads[i][j]){
                    discard++;
                }else{
                    to_print << read->name << "\n";
                    to_print << read->seq.substr(cs->five_prime_cut, cs->three_prime_cut - cs->five_prime_cut) << "\n";
                    to_print << read->comment << "\n";
                    to_print << read->qual.substr(cs->five_prime_cut, cs->three_prime_cut - cs->five_prime_cut) << "\n";
                    kept++;
                }
                delete (read);
                //free(read);
                free(cs);
                //msg("Parsed read");
            }
        }
        //msg("Processed thread");
    }

    total = kept + discard;
    guard.~lock_guard();
    //msg("Finished results string");
    //#TODO:NOT WRITING ANYTHING
    msg("Outputing");
    if (!gzip_output) {
        msg(string("writing to ") + string(outfn));
        outfile << to_print.str();
        //fprintf(outfile, "%s", to_print.str() );
    } else {
        gzprintf(outfile_gzip, to_print.str().c_str());
    }

    msg("Deleting batch");
    batch->free_this();
    delete(batch);
    msg("Batch deleted");
    //msg("Writing flag");
    writing_results_flag = false;
    msg("Finished outputing results");
}

int Trim_Single::init_streams(){
    msg("Initializing streams");
    input = new GZReader(infn, batch_len, false);
    if (!input) {
        fprintf(stderr, "****Error: Could not open input file '%s'.\n\n", infn);
        return EXIT_FAILURE;
    }

    if (!gzip_output) {
        //outfile = fopen(outfn, "w");
        outfile.open(outfn);
        if (outfile.fail()) {
            fprintf(stderr, "****Error: Could not open output file '%s'.\n\n", outfn);
            return EXIT_FAILURE;
        }
    } else {
        outfile_gzip = gzopen(outfn, "w");
        if (!outfile_gzip) {
            fprintf(stderr, "****Error: Could not open output file '%s'.\n\n", outfn);
            return EXIT_FAILURE;
        }
    }

    //msg("Finished initializing streams");
    return 0;
}

void Trim_Single::close_streams(){
    //msg("closing gzreader");
    delete(input);

    if (!gzip_output){
        //msg("closing outfile");
        outfile.close();
    }else{
        //msg("closing gz outfile");
        gzclose(outfile_gzip);
    }
}