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
    {"pe-interleaved", required_argument, 0, 'c'},
    {"output-pe1", required_argument, 0, 'o'},
    {"output-pe2", required_argument, 0, 'p'},
    {"output-single", required_argument, 0, 's'},
    {"output-interleaved", required_argument, 0, 'm'},
    {"qual-threshold", required_argument, 0, 'q'},
    {"length-threshold", required_argument, 0, 'l'},
    {"no-fiveprime", no_argument, 0, 'x'},
    {"truncate-n", no_argument, 0, 'n'},
    {"gzip-output", no_argument, 0, 'g'},
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
Usage: %s pe [options] -c <interleaved input file> -t <quality type> -m <interleaved trimmed output>\n\n", PROGRAM_NAME, PROGRAM_NAME);
    fprintf(stderr, "Options:\n\
Paired-end separated reads\n\
--------------------------\n\
-f, --pe-file1, Input paired-end forward fastq file (Input files must have same number of records)\n\
-r, --pe-file2, Input paired-end reverse fastq file\n\
-o, --output-pe1, Output trimmed forward fastq file\n\
-p, --output-pe2, Output trimmed reverse fastq file. Must use -s option.\n\n\
Paired-end interleaved reads\n\
----------------------------\n");
    fprintf(stderr,"-c, --pe-interleaved, Combined (interleaved) input paired-end fastq\n\
-m, --output-interleaved, Output combined (interleaved) paired-end fastq file. Must use -s option.\n\
--------------\n\
-t, --qual-type, Type of quality values (solexa (CASAVA < 1.3), illumina (CASAVA 1.3 to 1.7), sanger (which is CASAVA >= 1.8)) (required)\n");
    fprintf(stderr, "-s, --output-single, Output trimmed singles fastq file\n\
-q, --qual-threshold, Threshold for trimming based on average quality in a window. Default 20.\n\
-l, --length-threshold, Threshold to keep a read based on length after trimming. Default 20.\n\
-x, --no-fiveprime, Don't do five prime trimming.\n\
-n, --truncate-n, Truncate sequences at position of first N.\n\
-a, --threads, Number of threads to use. Default and minimum: Available cores - 1.\n\
-b, --batch, maximum MB of data to read from the input file at each cycle.\n\
\tThe greater the value, the greater the memory usage can be. The value, multiplied by 1024^2, must be \n\
\tbigger than the lenght of the longest read. Minimum 1. Default: 512.\n");


    fprintf(stderr, "-g, --gzip-output, Output gzipped files.\n--quiet, do not output trimming info\n\
--help, display this help and exit\n\
--version, output version information and exit\n\n");

    if (msg) fprintf(stderr, "%s\n\n", msg);
    exit(status);
}

Trim_Paired::Trim_Paired(){
    threads=DEFAULT_THREADS;
    batch_len=1024*1024*DEFAULT_BATCH_LEN;

    input = NULL;          /* forward input file handle */
    input2 = NULL;          /* reverse input file handle */
    input_inter = NULL;          /* interleaved input file handle */
    outfile_gzip = NULL;
    outfile2_gzip = NULL;
    interleaved_gzip = NULL;
    single_gzip = NULL;
    debug = 0;
    qualtype = -1;
    outfn = NULL;        /* forward file out name */
    outfn2 = NULL;        /* reverse file out name */
    outfnc = NULL;        /* interleaved file out name */
    sfn = NULL;           /* single file out name */
    infn = NULL;         /* forward input filename */
    infn2 = NULL;         /* reverse input filename */
    infnc = NULL;         /* interleaved input filename */

    length_threshold = 20;
    qual_threshold = 20;
    
    quiet = 0;
    no_fiveprime = 0;
    trunc_n = 0;
    gzip_output = 0;
    interleaved_s = 0;
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
            interleaved_s = 1;
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
    if(infnc)
        batch_len = recommended_batch_len(infnc, batch_len);
    else if(infn){
        batch_len = recommended_batch_len(infn, batch_len);
    }

    return 0;
}

int Trim_Paired::recommended_batch_len(const char* path, int max_batch_len){
    std::uintmax_t min = 20;
    std::uintmax_t max = (unsigned) max_batch_len / 2;
    std::uintmax_t size = std::experimental::filesystem::file_size(path);

    std::uintmax_t recommended = size / 8;

    if(recommended < min){
        msg(string("Batch size is ") + to_string(min / (1024*1024)) + string("MB"));
        return (int)min;
    }else if(recommended > max){
        msg(string("Batch size is ") + to_string(max / (1024*1024)) + string("MB"));
        return (int)max;
    }else{
        msg(string("Batch size is ") + to_string(recommended / (1024*1024)) + string("MB"));
        return (int)recommended;
    }
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
    
    vector<thread> output_threads;
    while(true){
        //lock_guard<mutex> guard(batch_lock);
        msg("Starting batch variables");
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
        int last_read_position = 0;
        int last_read_position2 = 0;

        for (int i = 0; i < threads; i++){
            last_item[i] =  -1;
            queue_lens[i] =  0;
            queue_lens2[i] = 0;
        }

        msg("Reading new batch");
        batch = input->get_batch_buffering_lines();
        //msg("Read new batch");
        if(batch == NULL){
            msg("No more data, finishing program.");
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
        int last_queue = 0;
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
            fqrec = new FQEntry(last_read_position, batch);
            last_read_position = fqrec->position;

            //msg("Reading read2");
            if(input_inter && !batch->has_lines()){
                error("Reading interleaved pair: read1 loaded, but no read2 to load. Maybe it's not an interleaved file?");
                exit(EXIT_FAILURE);
            }
            if(input_inter){
                fqrec2 = new FQEntry(last_read_position, batch);
                last_read_position = fqrec2->position;
            }else{
                fqrec2 = new FQEntry(last_read_position2, batch2);
                last_read_position2 = fqrec2->position;
            }

            //msg("Reads read");

            int read_len = fqrec->seq.length();
            int read_len2 = fqrec2->seq.length();
            chars_read_from_batch += read_len;

            int smallest_queue = 0;
            bool fitted_in_a_queue = false;

            long next_index = last_item[last_queue] + 1;

            if(next_index < queues[last_queue]->size()){
                (*queues[last_queue])[next_index] = fqrec;
                (*queues2[last_queue])[next_index] = fqrec2;
            }else{
                queues[last_queue]->push_back(fqrec);
                queues2[last_queue]->push_back(fqrec2);
            }

            queue_lens[last_queue] += read_len;
            queue_lens2[last_queue] += read_len2;

            last_item[last_queue] += 1;

            last_queue = (last_queue+1) % threads;
        }
        msg("Finished reading batch");

        if(chars_read_from_batch == 0){
            msg("No more data, finishing program.");
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

            msg("Processing threads:");
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

            writing_results_flag = true;
            output_threads.push_back(thread(&Trim_Paired::output_paired,
                this,
                queues, queues2, filtered_reads1, filtered_reads2,
                saved_cutsites1, saved_cutsites2, last_item)
            );
            //output_paired(queues, queues2, filtered_reads1, filtered_reads2,
            //    saved_cutsites1, saved_cutsites2, last_item);
        }
    }

    msg("Waiting for output threads");
    for(int i = 0; i < output_threads.size(); i++){
        output_threads[i].join();
    }

    while(writing_results_flag){
        this_thread::sleep_for(chrono::milliseconds(100));
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
    int kept_p = 0;
    int kept_s1 = 0;
    int kept_s2 = 0;
    int discard_p = 0;
    int discard_s1 = 0;
    int discard_s2 = 0;

    msg("Making results string");
    std::stringstream fq1, fq2, singles;
    
    for (size_t i = 0; i < threads; i++){
        //msg("Results from thread ");
        //msg(to_string(i));
        for (size_t j = 0; j < queues[i]->size(); j++)
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
            delete(read1);
            delete(read2);
            free(cs1);
            free(cs2);
        }
        delete(queues[i]);
        delete(queues2[i]);
        delete(filtered_reads[i]);
        delete(filtered_reads2[i]);
        free(saved_cutsites[i]);
        free(saved_cutsites2[i]);
        //msg("Read results from thread");
    }
    free(saved_cutsites);
    free(saved_cutsites2);
    msg("Finished results string");

    lock_guard<mutex> guard(batch_lock);
    this->kept_p += kept_p;
    this->kept_s1 += kept_s1;
    this->kept_s2 += kept_s2;
    this->discard_p += discard_p;
    this->discard_s1 += discard_s1;
    this->discard_s2 += discard_s2;

    total = kept_p + kept_s1 + kept_s2 + discard_p + discard_s1 + discard_s2;
    
    //msg("Outputing");
    if (!gzip_output) {
        //msg("Writing plain text");
        if(input_inter){
            //msg("Interleaved output");
            outfile_interleaved << fq1.str();
            if (sfn) outfile_single << singles.str();
            //fprintf(outfile_interleaved, "%s", fq1.str());
        }else{
            //msg("Separate outputs");
            outfile << fq1.str();
            //fprintf(outfile, "%s", );
            outfile2 << fq2.str();
            //fprintf(outfile2, "%s", );
            if (sfn) outfile_single << singles.str();
            //fprintf(outfile_single, "%s", fq2.str());
        }
    } else {
        if(input_inter){
            gzprintf(interleaved_gzip, fq1.str().c_str());
            if (sfn) gzprintf(single_gzip, singles.str().c_str());
        }else{
            gzprintf(outfile_gzip, fq1.str().c_str());
            gzprintf(outfile2_gzip, fq2.str().c_str());
            if (sfn) gzprintf(single_gzip, singles.str().c_str());
        }
    }
    //msg("Finished outputing results");
    writing_results_flag = false;
}

int Trim_Paired::init_streams(){
    msg("Opening files");
    if (infnc) {      /* using interleaved input file */

        if (infn || infn2 || outfn || outfn2) {
            usage(EXIT_FAILURE, "****Error: Cannot have -f, -r, -o, or -p options with -c.");
            return EXIT_FAILURE;
        }

        input_inter = new GZReader(infnc, batch_len, true);
        if (!input_inter) {
            fprintf(stderr, "****Error: Could not open interleaved input file '%s'.\n\n", infnc);
            return EXIT_FAILURE;
        }
        input = input_inter;

        /* get interleaved output file */
        if (!gzip_output) {
            outfile_interleaved.open(outfnc);
            //outfile_interleaved = fopen(outfnc, "w");
            if (!outfile_interleaved) {
                fprintf(stderr, "****Error: Could not open interleaved output file '%s'.\n\n", outfnc);
                return EXIT_FAILURE;
            }
        } else {
            interleaved_gzip = gzopen(outfnc, "w");
            if (!interleaved_gzip) {
                fprintf(stderr, "****Error: Could not open interleaved output file '%s'.\n\n", outfnc);
                return EXIT_FAILURE;
            }
        }

    } else {     /* using forward and reverse input files */

        if (infn && (!infn2 || !outfn || !outfn2 || !sfn)) {
            usage(EXIT_FAILURE, "****Error: Using the -f option means you must have the -r, -o, -p, and -s options.");
            return EXIT_FAILURE;
        }

        if (infn && (infnc || interleaved_s)) {
            usage(EXIT_FAILURE, "****Error: The -f option cannot be used in combination with -c, -m, or -M.");
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
            //outfile = fopen(outfn, "w");
            outfile.open(outfn);
            if (!outfile) {
                fprintf(stderr, "****Error: Could not open output file '%s'.\n\n", outfn);
                return EXIT_FAILURE;
            }
            outfile2.open(outfn2);
            //outfile2 = fopen(outfn2, "w");
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
    if (sfn) {
        if (!gzip_output) {
            outfile_single.open(sfn);
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


    msg("Opened files");
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
        outfile_single.close();
    }
    //msg("Deleted single outputs");

    //msg("Deleted interleaved outputs");

    if (!gzip_output) {
        if(outfile) outfile.close();
        if(outfile2) outfile2.close();
    } else {
        if(outfile_gzip) gzclose(outfile_gzip);
        if(outfile2_gzip) gzclose(outfile2_gzip);
    }

    msg("Closed all files");
}