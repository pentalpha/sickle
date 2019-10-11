#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <algorithm>
#include <functional>
#include "FQEntry.h"
#include "sickle.h"
#include "trim_paired.h"

static struct option paired_long_options[] = {
    {"qual-type", required_argument, 0, 't'},
    {"pe-file1", required_argument, 0, 'f'},
    {"pe-file2", required_argument, 0, 'r'},
    {"pe-combo", required_argument, 0, 'c'},
    {"output-pe1", required_argument, 0, 'o'},
    {"output-pe2", required_argument, 0, 'p'},
    {"output-single", required_argument, 0, 's'},
    {"output-combo", required_argument, 0, 'm'},
    {"qual-threshold", required_argument, 0, 'q'},
    {"length-threshold", required_argument, 0, 'l'},
    {"no-fiveprime", no_argument, 0, 'x'},
    {"truncate-n", no_argument, 0, 'n'},
    {"gzip-output", no_argument, 0, 'g'},
    {"output-combo-all", required_argument, 0, 'M'},
    {"quiet", no_argument, 0, 'z'},
    {"threads", no_argument, 0, 'a'},
    {"batch", no_argument, 0, 'b'},
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {NULL, 0, NULL, 0}
};

void Trim_Paired::usage (int status, char const *msg) {

    fprintf(stderr, "\nIf you have separate files for forward and reverse reads:\n");
    fprintf(stderr, "Usage: %s pe [options] -f <paired-end forward fastq file> -r <paired-end reverse fastq file> -t <quality type> -o <trimmed PE forward file> -p <trimmed PE reverse file> -s <trimmed singles file>\n\n", PROGRAM_NAME);
    fprintf(stderr, "If you have one file with interleaved forward and reverse reads:\n");
    fprintf(stderr, "Usage: %s pe [options] -c <interleaved input file> -t <quality type> -m <interleaved trimmed paired-end output> -s <trimmed singles file>\n\n\
If you have one file with interleaved reads as input and you want ONLY one interleaved file as output:\n\
Usage: %s pe [options] -c <interleaved input file> -t <quality type> -M <interleaved trimmed output>\n\n", PROGRAM_NAME, PROGRAM_NAME);
    fprintf(stderr, "Options:\n\
Paired-end separated reads\n\
--------------------------\n\
-f, --pe-file1, Input paired-end forward fastq file (Input files must have same number of records)\n\
-r, --pe-file2, Input paired-end reverse fastq file\n\
-o, --output-pe1, Output trimmed forward fastq file\n\
-p, --output-pe2, Output trimmed reverse fastq file. Must use -s option.\n\n\
Paired-end interleaved reads\n\
----------------------------\n");
    fprintf(stderr,"-c, --pe-combo, Combined (interleaved) input paired-end fastq\n\
-m, --output-combo, Output combined (interleaved) paired-end fastq file. Must use -s option.\n\
-M, --output-combo-all, Output combined (interleaved) paired-end fastq file with any discarded read written to output file as a single N. Cannot be used with the -s option.\n\n\
Global options\n\
--------------\n\
-t, --qual-type, Type of quality values (solexa (CASAVA < 1.3), illumina (CASAVA 1.3 to 1.7), sanger (which is CASAVA >= 1.8)) (required)\n");
    fprintf(stderr, "-s, --output-single, Output trimmed singles fastq file\n\
-q, --qual-threshold, Threshold for trimming based on average quality in a window. Default 20.\n\
-l, --length-threshold, Threshold to keep a read based on length after trimming. Default 20.\n\
-x, --no-fiveprime, Don't do five prime trimming.\n\
-n, --truncate-n, Truncate sequences at position of first N.\n\
-a, --threads, Number of threads to use. Default and minimum: 1.\n\
-b, --batch, MB of data to read from the input file at each cycle.\n\
\tThe greater the value, the greater the memory usage. The value, multiplied by 1024^2, must be \n\
\tbigger than the lenght of the longest read. Minimum 1. Default: 1000.\n");


    fprintf(stderr, "-g, --gzip-output, Output gzipped files.\n--quiet, do not output trimming info\n\
--help, display this help and exit\n\
--version, output version information and exit\n\n");

    if (msg) fprintf(stderr, "%s\n\n", msg);
    exit(status);
}

Trim_Paired::Trim_Paired(){
    threads=1;
    batch_len=1024*1024*DEFAULT_BATCH_LEN;

    input = NULL;          /* forward input file handle */
    input2 = NULL;          /* reverse input file handle */
    input_inter = NULL;          /* combined input file handle */
    outfile = NULL;      /* forward output file handle */
    outfile2 = NULL;      /* reverse output file handle */
    outfile_combo = NULL;         /* combined output file handle */
    outfile_single = NULL;        /* single output file handle */
    outfile_gzip = NULL;
    outfile2_gzip = NULL;
    combo_gzip = NULL;
    single_gzip = NULL;
    debug = 0;
    qualtype = -1;
    outfn = NULL;        /* forward file out name */
    outfn2 = NULL;        /* reverse file out name */
    outfnc = NULL;        /* combined file out name */
    sfn = NULL;           /* single/combined file out name */
    infn = NULL;         /* forward input filename */
    infn2 = NULL;         /* reverse input filename */
    infnc = NULL;         /* combined input filename */

    length_threshold = 20;
    qual_threshold = 20;
    
    quiet = 0;
    no_fiveprime = 0;
    trunc_n = 0;
    gzip_output = 0;

    combo_all = 0;
    combo_s = 0;
}

int Trim_Paired::parse_args(int argc, char *argv[]){
    int optc;
    extern char *optarg;
    while (1) {
        int option_index = 0;
        optc = getopt_long(argc, argv, "df:r:c:t:o:p:m:M:s:q:a:b:l:xng", paired_long_options, &option_index);

        if (optc == -1)
            break;

        switch (optc) {
            if (paired_long_options[option_index].flag != 0)
                break;

        case 'f':
            infn = (char *) malloc(strlen(optarg) + 1);
            strcpy(infn, optarg);
            break;

        case 'r':
            infn2 = (char *) malloc(strlen(optarg) + 1);
            strcpy(infn2, optarg);
            break;

        case 'c':
            infnc = (char *) malloc(strlen(optarg) + 1);
            strcpy(infnc, optarg);
            break;

        case 't':
            if (!strcmp(optarg, "illumina")) qualtype = ILLUMINA;
            else if (!strcmp(optarg, "solexa")) qualtype = SOLEXA;
            else if (!strcmp(optarg, "sanger")) qualtype = SANGER;
            else {
                fprintf(stderr, "Error: Quality type '%s' is not a valid type.\n", optarg);
                return EXIT_FAILURE;
            }
            break;

        case 'o':
            outfn = (char *) malloc(strlen(optarg) + 1);
            strcpy(outfn, optarg);
            break;

        case 'p':
            outfn2 = (char *) malloc(strlen(optarg) + 1);
            strcpy(outfn2, optarg);
            break;

        case 'm':
            outfnc = (char *) malloc(strlen(optarg) + 1);
            strcpy(outfnc, optarg);
            combo_s = 1;
            break;

        case 'M':
            outfnc = (char *) malloc(strlen(optarg) + 1);
            strcpy(outfnc, optarg);
            combo_all = 1;
            break;

        case 's':
            sfn = (char *) malloc(strlen(optarg) + 1);
            strcpy(sfn, optarg);
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

        case_GETOPT_HELP_CHAR(usage);
        case_GETOPT_VERSION_CHAR(PROGRAM_NAME, VERSION, AUTHORS);

        case '?':
            usage(EXIT_FAILURE, NULL);
            break;

        default:
            usage(EXIT_FAILURE, NULL);
            break;
        }
    }

    /* required: qualtype */
    if (qualtype == -1) {
        usage(EXIT_FAILURE, "****Error: Quality type is required.");
        return EXIT_FAILURE;
    }

    /* make sure minimum input filenames are specified */
    if (!infn && !infnc) {
        usage(EXIT_FAILURE, "****Error: Must have either -f OR -c argument.");
        return EXIT_FAILURE;
    }
    return 0;
}

int Trim_Paired::trim_main() {
    total=0;
    kept_p = 0;
    discard_p = 0;
    kept_s1 = 0;
    kept_s2 = 0;
    discard_s1 = 0;
    discard_s2 = 0;

    int res = init_streams();
    if(res != 0){
        return res;
    }
    std::vector<std::vector<FQEntry*>* > queues;
    std::vector<std::vector<FQEntry*>* > queues2;

    std::vector<long> queue_lens;
    std::vector<long> queue_lens2;

    std::vector<long> last_item;

    bool** filtered_reads1 = new bool*[threads];
    bool** filtered_reads2 = new bool*[threads];

    cutsites*** saved_cutsites1 = new cutsites**[threads];
    cutsites*** saved_cutsites2 = new cutsites**[threads];

    for (int i = 0; i < threads; i++){
        queues.push_back(new std::vector<FQEntry*>());
        queue_lens.push_back(0);
        last_item.push_back(-1);
    }
    for (int i = 0; i < threads; i++){
        queues2.push_back(new std::vector<FQEntry*>());
        queue_lens2.push_back(0);
    }

    Batch* batch = NULL;
    Batch* batch2 = NULL;
    while(true){
        for (int i = 0; i < threads; i++){
            last_item[i] =  -1;
            queue_lens[i] =  0;
            queue_lens2[i] = 0;
        }

        //msg("Reading new batch");
        batch = input->get_batch_buffering_lines();
        //msg("Read new batch");
        if(batch == NULL){
            msg("No batch returned, exiting.");
            break;
        }

        if(!input_inter){
            //msg("Reading batch2");
            batch2 = input2->get_batch_buffering_lines();
            //msg("Read batch2");

            if(batch2 == NULL){
                //msg("No batch2 returned, exiting.");
                break;
            }else{
                if(batch2->n_lines() != batch->n_lines()){
                    error("Batch2 and Batch1 have different lengths, exiting");
                    break;
                }
            }
        }else{
            //msg("No need for batch2");
        }
        int chars_read_from_batch = 0;
        int max_queue_len = batch->sequences_len / threads;

        FQEntry* fqrec = NULL;
        FQEntry* fqrec2 = NULL;
        //msg("Reading reads from batch");
        while(batch->has_lines()){
            if(!input_inter){
                if(chars_read_from_batch > batch_len){
                    break;
                }
            }else{
                if(chars_read_from_batch > batch_len){
                    break;
                }
            }

            //msg("Reading read1");
            //if(input == NULL) msg("input1 is null");
            if(fqrec){
                fqrec = new FQEntry(fqrec->position, batch);
            }else{
                fqrec = new FQEntry(0, batch);
            }

            //msg("Reading read2");
            if(input_inter && !batch->has_lines()){
                error("Reading interleaved pair: read1 loaded, but no read2 to load. Maybe it's not an interleaved file?");
                exit(EXIT_FAILURE);
            }
            if(fqrec2){
                if(input_inter){
                    fqrec2 = new FQEntry(fqrec->position, batch);
                }else{
                    fqrec2 = new FQEntry(fqrec2->position, batch2);
                }
            }else{
                if(input_inter){
                    fqrec2 = new FQEntry(0, batch);
                }else{
                    fqrec2 = new FQEntry(0, batch2);
                }
            }

            //msg("Reads read");

            int read_len = fqrec->seq.length();
            int read_len2 = fqrec2->seq.length();
            chars_read_from_batch += read_len;

            int smallest_queue = 0;
            bool fitted_in_a_queue = false;

            //msg("Looking for queue to fit read into it");
            for(unsigned i = 0; i < queues.size(); i++){
                /*std::vector<FQEntry*>* queue = queues[i];
                std::vector<FQEntry*>* queue2 = queues2[i];
                if(queue_lens[i] + read_len < max_queue_len)
                {
                    queue->emplace(queue->begin() + last_item[i]+1, fqrec);
                    queue_lens[i] += read_len;
                    queue2->emplace(queue2->begin() + last_item[i]+1, fqrec2);
                    queue_lens2[i] += read_len2;
                    fitted_in_a_queue = true;
                    last_item[i] += 1;
                }*/
                if(queue_lens[i] < queue_lens[smallest_queue]){
                    smallest_queue = i;
                }
                //if(fitted_in_a_queue) break;
            }
            //msg("Found");
            if(!fitted_in_a_queue){
                //msg("Fitting into smallest queue");
                queues[smallest_queue]->emplace(queues[smallest_queue]->begin() + last_item[smallest_queue]+1, fqrec);
                queue_lens[smallest_queue] += read_len;

                queues2[smallest_queue]->emplace(queues2[smallest_queue]->begin() + last_item[smallest_queue]+1, fqrec2);
                queue_lens2[smallest_queue] += read_len2;

                last_item[smallest_queue] += 1;
            }
            //msg("Stored read");
        }
        //msg("Finished reading batch");

        if(chars_read_from_batch == 0){
            msg("Empty batch, finishing program.");
            break;
        }else{
            for (int i = 0; i < threads; i++){
                filtered_reads1[i] = new bool[last_item[i]+1];
                filtered_reads2[i] = new bool[last_item[i]+1];
                bool* array = filtered_reads1[i];
                memset(array, false, sizeof(array[0])*last_item[i]+1);
                bool* array2 = filtered_reads2[i];
                memset(array2, false, sizeof(array2[0])*last_item[i]+1);
                /*for(int j = 0; j < last_item[i]+1; j++){
                    if(array[j] != false){
                        error("Array is not all false");
                        exit(EXIT_FAILURE);
                    }
                }*/
                saved_cutsites1[i] = new cutsites*[last_item[i]+1];
                saved_cutsites2[i] = new cutsites*[last_item[i]+1];
            }

            //msg("Processing threads:");
            msg(to_string(threads));
            vector<thread> running;

            for(int thread_n = 0; thread_n < threads; thread_n++){
                running.push_back(thread(&Trim_Paired::processing_thread,
                    this,
                    queues[thread_n], queues2[thread_n],
                    filtered_reads1[thread_n], filtered_reads2[thread_n],
                    saved_cutsites1[thread_n], saved_cutsites2[thread_n],
                    last_item[thread_n], thread_n
                ));
            }

            //msg("Joining all");
            std::for_each(running.begin(),running.end(), std::mem_fn(&std::thread::join));

            output_paired(queues, queues2, filtered_reads1, filtered_reads2,
                saved_cutsites1, saved_cutsites2, last_item);
        }
    }

    if (!quiet) {
        if (infn && infn2) fprintf(stdout, "\nPE forward file: %s\nPE reverse file: %s\n", infn, infn2);
        if (infnc) fprintf(stdout, "\nPE interleaved file: %s\n", infnc);
        fprintf(stdout, "\nTotal input FastQ records: %d (%d pairs)\n", total, (total / 2));
        fprintf(stdout, "\nFastQ paired records kept: %d (%d pairs)\n", kept_p, (kept_p / 2));
        if (input_inter) fprintf(stdout, "FastQ single records kept: %d\n", (kept_s1 + kept_s2));
        else fprintf(stdout, "FastQ single records kept: %d (from PE1: %d, from PE2: %d)\n", (kept_s1 + kept_s2), kept_s1, kept_s2);

        fprintf(stdout, "FastQ paired records discarded: %d (%d pairs)\n", discard_p, (discard_p / 2));

        if (input_inter) fprintf(stdout, "FastQ single records discarded: %d\n\n", (discard_s1 + discard_s2));
        else fprintf(stdout, "FastQ single records discarded: %d (from PE1: %d, from PE2: %d)\n\n", (discard_s1 + discard_s2), discard_s1, discard_s2);
    }

    close_streams();

    return EXIT_SUCCESS;
}

void Trim_Paired::processing_thread(
        std::vector<FQEntry*>* local_queue, std::vector<FQEntry*>* local_queue2,
        bool* filtered1, bool* filtered2,
        cutsites** cutsites1, cutsites** cutsites2,
        long last_index, int thread_n)
{
    assert(local_queue != NULL && local_queue2 != NULL);

    msg(string("Processing thread ") + to_string(thread_n) + string(", read pairs: ") + to_string(last_index+1));
    FQEntry* fqrec1;
    FQEntry* fqrec2;

    assert(local_queue2->size() == local_queue->size());
    for(int i = 0; i <= last_index; i++){
        fqrec1 = local_queue->at(i);
        fqrec2 = local_queue2->at(i);
        cutsites1[i] = sliding_window(*fqrec1);
        if(!(cutsites1[i]->three_prime_cut >= 0)) filtered1[i] = true;
        cutsites2[i] = sliding_window(*fqrec2);
        if(!(cutsites2[i]->three_prime_cut >= 0)) filtered2[i] = true;
    }
}

std::string Trim_Paired::get_read_string(FQEntry* read, cutsites* cs){
    std::stringstream to_print;
    to_print << read->name << "\n";
    to_print << read->seq.substr(cs->five_prime_cut, cs->three_prime_cut - cs->five_prime_cut) << "\n";
    to_print << read->comment << "\n";
    to_print << read->qual.substr(cs->five_prime_cut, cs->three_prime_cut - cs->five_prime_cut) << "\n";
    return to_print.str();
}

void Trim_Paired::output_paired(std::vector<std::vector<FQEntry*>* > queues, std::vector<std::vector<FQEntry*>* > queues2,
        bool** filtered_reads, bool** filtered_reads2,
        cutsites*** saved_cutsites, cutsites*** saved_cutsites2,
        vector<long> last_index)
{
    msg("Making results string");
    std::stringstream fq1, fq2, singles;
    for (size_t i = 0; i < queues.size(); i++){
        //msg("Results from thread ");
        //msg(to_string(i));
        if(queues[i]->size() > last_index[i]){
            for (size_t j = 0; j <= last_index[i]; j++)
            {   
                //msg("Reading data");
                bool r1 = !filtered_reads[i][j];
                bool r2 = !filtered_reads2[i][j];
                FQEntry* read1 = queues[i]->at(j);
                cutsites* cs1 = saved_cutsites[i][j];
                FQEntry* read2 = queues2[i]->at(j);
                cutsites* cs2 = saved_cutsites2[i][j];
                //msg("Read entry data");
                if(r1 && r2){
                    //msg("Writing both");
                    fq1 << get_read_string(read1, cs1);
                    if(input_inter){
                        fq1 << get_read_string(read2, cs2);
                    }else{
                        fq2 << get_read_string(read2, cs2);
                    }
                    kept_p += 2;
                }else if(r1 || r2){
                    if(r1){
                        //msg("Writing r1");
                        singles << get_read_string(read1, cs1);
                        kept_s1++;
                        discard_s2++;
                    }else{
                        //msg("Writing r2");
                        singles << get_read_string(read2, cs2);
                        kept_s2++;
                        discard_s1++;
                    }
                }else{
                    //msg("Writing none");
                    discard_p += 2;
                }
            }
        }else{
            //msg("Empty thread, ignoring");
        }
        //msg("Read results from thread");
    }
    msg("Finished results string");

    total = kept_p + kept_s1 + kept_s2 + discard_p + discard_s1 + discard_s2;
    
    msg("Outputing");
    if (!gzip_output) {
        msg("Writing plain text");
        if(input_inter){
            msg("Interleaved output");
            fprintf(outfile_combo, "%s", fq1.str());
            if(combo_all){
                fprintf(outfile_combo, "%s", singles.str());
            }
        }else{
            msg("Separate outputs");
            fprintf(outfile, "%s", fq1.str());
            fprintf(outfile2, "%s", fq2.str());
            if(!combo_all){
                fprintf(outfile_single, "%s", fq2.str());
            }else{
                fprintf(outfile_combo, "%s", fq2.str());
            }
        }
    } else {
        if(input_inter){
            gzprintf(combo_gzip, "%s", fq1.str());
            if(combo_all){
                gzprintf(combo_gzip, "%s", singles.str());
            }
        }else{
            gzprintf(outfile_gzip, "%s", fq1.str());
            gzprintf(outfile2_gzip, "%s", fq2.str());
            if(!combo_all){
                gzprintf(single_gzip, "%s", fq2.str());
            }else{
                gzprintf(combo_gzip, "%s", fq2.str());
            }
        }
    }
    msg("Finished outputing results");
}

int Trim_Paired::init_streams(){
    if (infnc) {      /* using combined input file */

        if (infn || infn2 || outfn || outfn2) {
            usage(EXIT_FAILURE, "****Error: Cannot have -f, -r, -o, or -p options with -c.");
            return EXIT_FAILURE;
        }

        if ((combo_all && combo_s) || (!combo_all && !combo_s)) {
            usage(EXIT_FAILURE, "****Error: Must have only one of either -m or -M options with -c.");
            return EXIT_FAILURE;
        }

        if ((combo_s && !sfn) || (combo_all && sfn)) {
            usage(EXIT_FAILURE, "****Error: -m option must have -s option, and -M option cannot have -s option.");
            return EXIT_FAILURE;
        }

        /* check for duplicate file names */
        if (!strcmp(infnc, outfnc) || (combo_s && (!strcmp(infnc, sfn) || !strcmp(outfnc, sfn)))) {
            fprintf(stderr, "****Error: Duplicate filename between combo input, combo output, and/or single output file names.\n\n");
            return EXIT_FAILURE;
        }

        /* get combined output file */
        if (!gzip_output) {
            outfile_combo = fopen(outfnc, "w");
            if (!outfile_combo) {
                fprintf(stderr, "****Error: Could not open combo output file '%s'.\n\n", outfnc);
                return EXIT_FAILURE;
            }
        } else {
            combo_gzip = gzopen(outfnc, "w");
            if (!combo_gzip) {
                fprintf(stderr, "****Error: Could not open combo output file '%s'.\n\n", outfnc);
                return EXIT_FAILURE;
            }
        }

        input_inter = new GZReader(infnc, batch_len, true);
        if (!input_inter) {
            fprintf(stderr, "****Error: Could not open combined input file '%s'.\n\n", infnc);
            return EXIT_FAILURE;
        }
        input = input_inter;

    } else {     /* using forward and reverse input files */

        if (infn && (!infn2 || !outfn || !outfn2 || !sfn)) {
            usage(EXIT_FAILURE, "****Error: Using the -f option means you must have the -r, -o, -p, and -s options.");
            return EXIT_FAILURE;
        }

        if (infn && (infnc || combo_all || combo_s)) {
            usage(EXIT_FAILURE, "****Error: The -f option cannot be used in combination with -c, -m, or -M.");
            return EXIT_FAILURE;
        }

        if (!strcmp(infn, infn2) || !strcmp(infn, outfn) || !strcmp(infn, outfn2) ||
            !strcmp(infn, sfn) || !strcmp(infn2, outfn) || !strcmp(infn2, outfn2) || 
            !strcmp(infn2, sfn) || !strcmp(outfn, outfn2) || !strcmp(outfn, sfn) || !strcmp(outfn2, sfn)) {

            fprintf(stderr, "****Error: Duplicate input and/or output file names.\n\n");
            return EXIT_FAILURE;
        }

        input = new GZReader(infn, batch_len);
        if (!input) {
            fprintf(stderr, "****Error: Could not open input file '%s'.\n\n", infn);
            return EXIT_FAILURE;
        }

        input2 = new GZReader(infn2, batch_len);
        if (!input2) {
            fprintf(stderr, "****Error: Could not open input file '%s'.\n\n", infn2);
            return EXIT_FAILURE;
        }

        if (!gzip_output) {
            outfile = fopen(outfn, "w");
            if (!outfile) {
                fprintf(stderr, "****Error: Could not open output file '%s'.\n\n", outfn);
                return EXIT_FAILURE;
            }

            outfile2 = fopen(outfn2, "w");
            if (!outfile2) {
                fprintf(stderr, "****Error: Could not open output file '%s'.\n\n", outfn2);
                return EXIT_FAILURE;
            }
        } else {
            outfile_gzip = gzopen(outfn, "w");
            if (!outfile_gzip) {
                fprintf(stderr, "****Error: Could not open output file '%s'.\n\n", outfn);
                return EXIT_FAILURE;
            }

            outfile2_gzip = gzopen(outfn2, "w");
            if (!outfile2_gzip) {
                fprintf(stderr, "****Error: Could not open output file '%s'.\n\n", outfn2);
                return EXIT_FAILURE;
            }

        }
    }

    /* get singles output file handle */
    if (sfn && !combo_all) {
        if (!gzip_output) {
            outfile_single = fopen(sfn, "w");
            if (!outfile_single) {
                fprintf(stderr, "****Error: Could not open single output file '%s'.\n\n", sfn);
                return EXIT_FAILURE;
            }
        } else {
            single_gzip = gzopen(sfn, "w");
            if (!single_gzip) {
                fprintf(stderr, "****Error: Could not open single output file '%s'.\n\n", sfn);
                return EXIT_FAILURE;
            }
        }
    }

    return 0;
}

void Trim_Paired::close_streams(){
    //msg("Closing paired end streams");
    if (input_inter) {
        //msg("Deleting interleaved reader");
        delete(input_inter);
    } else {
        //msg("Deleting paired readers");
        delete(input);
        delete(input2);
    }
    //msg("Deleted readers");

    if(single_gzip){
        gzclose(single_gzip);
    }
    if(outfile_single){
        fclose(outfile_single);
    }
    //msg("Deleted single outputs");

    if(combo_gzip){
        gzclose(combo_gzip);
    }
    if(outfile_combo){
        fclose(outfile_combo);
    }

    //msg("Deleted combo outputs");

    if (!gzip_output) {
        if(outfile) fclose(outfile);
        if(outfile2) fclose(outfile2);
    } else {
        if(outfile_gzip) gzclose(outfile_gzip);
        if(outfile2_gzip) gzclose(outfile2_gzip);
    }

    msg("Closed all streams");
}