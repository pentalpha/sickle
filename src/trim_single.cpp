#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <stdio.h>
#include "FQEntry.h"
#include <getopt.h>
#include <iostream>
#include <queue>
#include "sickle.h"
#include "print_record.h"
#include "trim_single.h"

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
--quiet, Don't print out any trimming information\n\
--help, display this help and exit\n\
--version, output version information and exit\n\n");

    if (msg) fprintf(stderr, "%s\n\n", msg);
    exit(status);
}

Trim_Single::Trim_Single(){
    msg("Building trimmer");
    threads=DEFAULT_THREADS; //TODO: add thread number argument
    batch_len=DEFAULT_BATCH_LEN; //TODO: add batch length argument

    qualtype = -1;
    length_threshold = 20;
    qual_threshold = 20;
    no_fiveprime = 0;
    trunc_n = 0;
    debug = 0;
    input = NULL;
    outfile = NULL;
    outfile_gzip = NULL;
    outfn = NULL;
    infn = NULL;
    quiet = 0;
    gzip_output = 0;
    msg("Finished build trimmer");
}

int Trim_Single::parse_args(int argc, char *argv[]){
    int optc;
    extern char *optarg;

    std::cout << "Setting se trimming params\n";
    while (1) {
        int option_index = 0;
        optc = getopt_long(argc, argv, "df:t:o:q:l:zxng", single_long_options, &option_index);

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

    msg("Creating queues");
    std::vector<std::queue<FQEntry*>* > queues;
    std::vector<long> queue_lens;
    for (int i = 0; i < threads; i++){
        queues.push_back(new std::queue<FQEntry*>());
        queue_lens.push_back(0);
    }
    msg("Finished creating queues");

    while(true){
        msg("Reading new batch");
        long chars_read_for_batch = 0;
        int max_queue_len = batch_len / threads;
        while(!input->reached_end() || chars_read_for_batch < batch_len){
            FQEntry* fqrec;
            try{
                fqrec = new FQEntry(0, input);
            }catch(LoadException &ex){
                error(ex.what());
                break;
            }catch(EmptyStreamException &ex){
                //error(ex.what());
                break;
            }

            int read_len = fqrec->seq.length();
            chars_read_for_batch += read_len;
            int smallest_queue = 0;
            bool fitted_in_a_queue = false;
            for(unsigned i = 0; i < queues.size(); i++){
                std::queue<FQEntry*>* queue = queues[i];
                if(queue_lens[i] + read_len < max_queue_len){
                    queue->push(fqrec);
                    queue_lens[i] += read_len;
                    fitted_in_a_queue = true;
                }
                if(queue_lens[i] < queue_lens[smallest_queue]){
                    smallest_queue = i;
                }
                if(fitted_in_a_queue) break;
            }

            if(!fitted_in_a_queue){
                queues[smallest_queue]->push(fqrec);
                queue_lens[smallest_queue] += read_len;
            }
        }

        if(chars_read_for_batch == 0){
            break;
        }

        for(int thread_n = 0; thread_n < threads; thread_n++){
            processing_thread(queues[thread_n], thread_n);
        }
    }

    if (!quiet) fprintf(stdout, "\nSE input file: %s\n\nTotal FastQ records: %d\nFastQ records kept: %d\nFastQ records discarded: %d\n\n", infn, total, kept, discard);

    //kseq_destroy(fqrec);
    //delete(fqrec);
    //gzclose(input);
    close_streams();

    return EXIT_SUCCESS;
}

void Trim_Single::processing_thread(std::queue<FQEntry*>* local_queue, int thread_n){
    msg(string("Starting batch processing for thread ") + to_string(thread_n));
    FQEntry* fqrec;
    cutsites *p1cut;
    while (true) {
        if(local_queue->empty()){
            msg(string("reached batch end for thread ")+ to_string(thread_n));
            break;
        }else{
            //msg("reading entry for new cycle");
            fqrec = local_queue->front();
            local_queue->pop();
            //msg("read entry for new cycle");
        }
        //msg("running sliding window");
        p1cut = sliding_window(*fqrec);
        total++;
        if (debug) printf("P1cut: %d,%d\n", p1cut->five_prime_cut, p1cut->three_prime_cut);
        output_single(fqrec, p1cut);
        free(p1cut);
        free(fqrec);
    }
}

void Trim_Single::output_single(FQEntry* fqrec, cutsites* p1cut){
    /* if sequence quality and length pass filter then output record, else discard */
    if (p1cut->three_prime_cut >= 0) {
            if (!gzip_output) {
                /* This print statement prints out the sequence string starting from the 5' cut */
                /* and then only prints out to the 3' cut, however, we need to adjust the 3' cut */
                /* by subtracting the 5' cut because the 3' cut was calculated on the original sequence */

                print_record (outfile, *fqrec, p1cut);
            } else {
                print_record_gzip (outfile_gzip, *fqrec, p1cut);
            }

            kept++;
    }else{
        discard++;
    }
}

int Trim_Single::init_streams(){
    msg("Initializing streams");
    input = new GZReader(infn);
    if (!input) {
        fprintf(stderr, "****Error: Could not open input file '%s'.\n\n", infn);
        return EXIT_FAILURE;
    }

    if (!gzip_output) {
        outfile = fopen(outfn, "w");
        if (!outfile) {
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

    msg("Finished initializing streams");
    return 0;
}

void Trim_Single::close_streams(){
    msg("closing gzreader");
    delete(input);

    if (!gzip_output){
        msg("closing outfile");
        fclose(outfile);
    }else{
        msg("closing gz outfile");
        gzclose(outfile_gzip);
    }
}