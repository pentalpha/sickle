#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include "FQEntry.h"
#include "sickle.h"
#include "print_record.h"
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
-n, --truncate-n, Truncate sequences at position of first N.\n");


    fprintf(stderr, "-g, --gzip-output, Output gzipped files.\n--quiet, do not output trimming info\n\
--help, display this help and exit\n\
--version, output version information and exit\n\n");

    if (msg) fprintf(stderr, "%s\n\n", msg);
    exit(status);
}

Trim_Paired::Trim_Paired(){
    threads=DEFAULT_THREADS; //TODO: add thread number argument
    batch_len=DEFAULT_BATCH_LEN; //TODO: add batch length argument

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
        optc = getopt_long(argc, argv, "df:r:c:t:o:p:m:M:s:q:l:xng", paired_long_options, &option_index);

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

    std::vector<std::queue<FQEntry*>* > queues;
    std::vector<std::queue<FQEntry*>* > queues2;
    std::vector<long> queue_lens;
    std::vector<long> queue_lens2;
    for (int i = 0; i < threads; i++){
        queues.push_back(new std::queue<FQEntry*>());
        queue_lens.push_back(0);
    }
    for (int i = 0; i < threads; i++){
        queues2.push_back(new std::queue<FQEntry*>());
        queue_lens2.push_back(0);
    }

    while(true){
        msg("Reading new batch");
        long chars_read_for_queue = 0;
        long chars_read_for_queue2 = 0;
        int max_queue_len = batch_len / threads;
        while(true){
            if(!input_inter){
                if(chars_read_for_queue > batch_len){
                    break;
                }
            }else{
                if(chars_read_for_queue > batch_len || chars_read_for_queue2 > batch_len){
                    break;
                }
            }

            //msg("Reading read1");
            //if(input == NULL) msg("input1 is null");
            FQEntry* fqrec;
            
            if(!input->reached_end()){
                //msg("Reading from input1");
                try{
                    fqrec = new FQEntry(0, input);
                }catch(LoadException &ex){
                    error(ex.what());
                    break;
                }catch(EmptyStreamException &ex){
                    //error(ex.what());
                    break;
                }
            }else{
                //msg("End of input1");
                if(!input_inter){
                    if(!input2->reached_end()){
                        fprintf(stderr, "Warning: PE file 1 is shorter than PE file 2. Disregarding rest of PE file 2.\n");
                    }
                }
                break;
            }

            //msg("Reading read2");
            FQEntry* fqrec2;
            if (input_inter) {
                if(!input->reached_end()){
                    try{
                        fqrec2 = new FQEntry(0, input);
                    }catch(LoadException &ex){
                        error(ex.what());
                        break;
                    }catch(EmptyStreamException &ex){
                        //error(ex.what());
                        break;
                    }
                }else{
                    fprintf(stderr, "Warning: Interleaved PE file has uneven number of lines. Disregarding the last line.\n");
                    break;
                }
            }else{
                if(!input2->reached_end()){
                    try{
                        fqrec2 = new FQEntry(0, input2);
                    }catch(LoadException &ex){
                        error(ex.what());
                        break;
                    }catch(EmptyStreamException &ex){
                        //error(ex.what());
                        break;
                    }
                }else{
                    fprintf(stderr, "Warning: PE file 2 is shorter than PE file 1. Disregarding rest of PE file 1.\n");
                    break;
                }
            }

            //msg("Reads read");

            int read_len = fqrec->seq.length();
            int read_len2 = fqrec2->seq.length();
            chars_read_for_queue += read_len;
            chars_read_for_queue2 += read_len2;

            int smallest_queue = 0;
            bool fitted_in_a_queue = false;

            //msg("Looking for queue to fit read into it");
            for(unsigned i = 0; i < queues.size(); i++){
                std::queue<FQEntry*>* queue = queues[i];
                std::queue<FQEntry*>* queue2 = queues2[i];
                if(queue_lens[i] + read_len < max_queue_len){
                    queue->push(fqrec);
                    queue_lens[i] += read_len;
                    queue2->push(fqrec2);
                    queue_lens2[i] += read_len2;

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

                queues2[smallest_queue]->push(fqrec2);
                queue_lens2[smallest_queue] += read_len2;
            }
            //msg("Stored read");
        }
        msg("Finished reading batch");

        if(chars_read_for_queue == 0){
            msg("Empty batch, finishing program.");
            break;
        }else{
            msg("Processing threads:");
            msg(to_string(threads));
            for(int thread_n = 0; thread_n < threads; thread_n++){
                processing_thread(queues[thread_n], queues2[thread_n], thread_n);
            }
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

void Trim_Paired::processing_thread(std::queue<FQEntry*>* local_queue, std::queue<FQEntry*>* local_queue2, int thread_n){
    assert(local_queue != NULL && local_queue2 != NULL);

    msg(string("Processing thread ") + to_string(thread_n) + string(", read pairs: ") + to_string(local_queue->size()));
    FQEntry* fqrec1;
    FQEntry* fqrec2;

    assert(local_queue2->size() == local_queue->size());

    while (!local_queue->empty()) {
        //msg("Retrieving reads from queues");
        fqrec1 = local_queue->front();
        local_queue->pop();
        fqrec2 = local_queue2->front();
        local_queue2->pop();

        //msg("Sliding reads");
        cutsites *p1cut = sliding_window(*fqrec1);
        cutsites *p2cut = sliding_window(*fqrec2);
        total += 2;

        if (debug) printf("p1cut: %d,%d\n", p1cut->five_prime_cut, p1cut->three_prime_cut);
        if (debug) printf("p2cut: %d,%d\n", p2cut->five_prime_cut, p2cut->three_prime_cut);

        //msg("Outputing");
        output_paired(fqrec1, fqrec2, p1cut, p2cut);

        free(p1cut);
        free(p2cut);
        free(fqrec1);
        free(fqrec2);
    }
}

void Trim_Paired::output_paired(FQEntry* fqrec1, FQEntry* fqrec2, cutsites *p1cut, cutsites *p2cut){
    /* The sequence and quality print statements below print out the sequence string starting from the 5' cut */
        /* and then only print out to the 3' cut, however, we need to adjust the 3' cut */
        /* by subtracting the 5' cut because the 3' cut was calculated on the original sequence */

        /* if both sequences passed quality and length filters, then output both records */
        if (p1cut->three_prime_cut >= 0 && p2cut->three_prime_cut >= 0) {
            if (!gzip_output) {
                if (input_inter) {
                    print_record (outfile_combo, *fqrec1, p1cut);
                    print_record (outfile_combo, *fqrec2, p2cut);
                } else {
                    print_record (outfile, *fqrec1, p1cut);
                    print_record (outfile2, *fqrec2, p2cut);
                }
            } else {
                if (input_inter) {
                    print_record_gzip (combo_gzip, *fqrec1, p1cut);
                    print_record_gzip (combo_gzip, *fqrec2, p2cut);
                } else {
                    print_record_gzip (outfile_gzip, *fqrec1, p1cut);
                    print_record_gzip (outfile2_gzip, *fqrec2, p2cut);
                }
            }

            kept_p += 2;
        }

        /* if only one sequence passed filter, then put its record in singles and discard the other */
        /* or put an "N" record in if that option was chosen. */
        else if (p1cut->three_prime_cut >= 0 && p2cut->three_prime_cut < 0) {
            if (!gzip_output) {
                if (combo_all) {
                    print_record (outfile_combo, *fqrec1, p1cut);
                    print_record_N (outfile_combo, *fqrec2, qualtype);
                } else {
                    print_record (outfile_single, *fqrec1, p1cut);
                }
            } else {
                if (combo_all) {
                    print_record_gzip (combo_gzip, *fqrec1, p1cut);
                    print_record_N_gzip (combo_gzip, *fqrec2, qualtype);
                } else {
                    print_record_gzip (single_gzip, *fqrec1, p1cut);
                }
            }

            kept_s1++;
            discard_s2++;
        }

        else if (p1cut->three_prime_cut < 0 && p2cut->three_prime_cut >= 0) {
            if (!gzip_output) {
                if (combo_all) {
                    print_record_N (outfile_combo, *fqrec1, qualtype);
                    print_record (outfile_combo, *fqrec2, p2cut);
                } else {
                    print_record (outfile_single, *fqrec2, p2cut);
                }
            } else {
                if (combo_all) {
                    print_record_N_gzip (combo_gzip, *fqrec1, qualtype);
                    print_record_gzip (combo_gzip, *fqrec2, p2cut);
                } else {
                    print_record_gzip (single_gzip, *fqrec2, p2cut);
                }
            }

            kept_s2++;
            discard_s1++;
        } else {
            /* If both records are to be discarded, but the -M option */
            /* is being used, then output two "N" records */
            if (combo_all) {
                if (!gzip_output) {
                    print_record_N (outfile_combo, *fqrec1, qualtype);
                    print_record_N (outfile_combo, *fqrec2, qualtype);
                } else {
                    print_record_N_gzip (combo_gzip, *fqrec1, qualtype);
                    print_record_N_gzip (combo_gzip, *fqrec2, qualtype);
                }
            }

            discard_p += 2;
        }
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

        input_inter = new GZReader(infnc);
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

        input = new GZReader(infn);
        if (!input) {
            fprintf(stderr, "****Error: Could not open input file '%s'.\n\n", infn);
            return EXIT_FAILURE;
        }

        input2 = new GZReader(infn2);
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