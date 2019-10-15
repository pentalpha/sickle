#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <zlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <thread>
#include <algorithm>
#include <iostream>
#include <cstring>

// for mmap:
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <queue>
#include <vector>
#include "GZReader.h"
#include "sickle.h"
#include "trim.h"
#include "trim_single.h"
#include "trim_paired.h"

void main_usage (int status) {

	fprintf (stdout, "\nUsage: %s <command> [options]\n\
\n\
Command:\n\
pe\tpaired-end sequence trimming\n\
se\tsingle-end sequence trimming\n\
\n\
--help, display this help and exit\n\
--version, output version information and exit\n\n", PROGRAM_NAME);

	exit (status);
}

void free_vector(vector<const char*>* lines){
	for(int i = 0; i < lines->size(); i++){
		delete(lines->at(i));
	}
	delete(lines);
}


void read_allocating(char const *path, int buff_size, vector<size_t>* empty){
	FILE* fp = fopen(path, "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	//char* line = NULL;
	size_t len = 0;
	size_t lines = 0;
	while (true) {
		//printf("%s", line);
		char* line = NULL;
		if(getline(&line, &len, fp) == -1){
			break;
		}else{
			lines++;
			//free(line);
			empty->push_back(lines);
		}
	}

	fclose(fp);
	//if (line){
	//	free(line);
	//}
	msg(to_string(lines));
}

void consumer(vector<size_t>* empty){
	int last_consumed = 0;
	while(true){
		
	}
}

static uintmax_t wc(char const *fname, int buff_size)
{
	msg("started wc");
    int BUFF_SIZE = buff_size*1024;
    int fd = open(fname, O_RDONLY);
    if(fd == -1){
		msg(to_string(fd));
		error("open");
		return 0;
	}else{
		msg("opened file");
	}

    /* Advise the kernel of our access pattern.  */
    posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL
	msg("Advised kernel");
    char buf[BUFF_SIZE + 1];
    uintmax_t lines = 0;
	size_t bytes_read = read(fd, buf, BUFF_SIZE);
	size_t total_bytes = 0;
	msg("Read first bytes");
    while(true)
    {
        if(bytes_read == (size_t)-1){
            msg("read failed");
			break;
		}else if (!bytes_read){
			msg("no bytes read, leaving");
            break;
		}else{
			//msg("counting lines");
        	for(char *p = buf; (p = (char*) memchr(p, '\n', (buf + bytes_read) - p)); ++p){
            	++lines;
			}

			bytes_read = read(fd, buf, BUFF_SIZE);
			//total_bytes += bytes_read;
		}
    }

	msg("finished wc");
	msg(to_string(total_bytes));
    return lines;
}

int test_gzreader_lines(char* path, char* batch_len){
	int batch_in_bytes = atoi(batch_len)*1024*1024;
	/*GZReader input(path, batch_in_bytes);
	int all_lines = 0;
	int batchs = 0;
	Batch* batch;
	while(!input.reached_end()){
		batch = input.get_batch_buffering_lines();
		all_lines += batch->n_lines();
        if(batch == NULL){
            msg("No batch returned, exiting.");
            break;
        }
		//msg(to_string(all_lines));
		batch->free_this();
		delete(batch);
	}
	std::cout << "total of " << all_lines << " lines" << std::endl;
	*/
	/*gzFile file = gzopen(path, "r");
	if (!file) {
        fprintf(stderr, "****Error: Could not open input file '%s'.\n\n", path);
    }
	msg("Allocating buffer");
	char* big_buffer = new char[batch_in_bytes+1];
	msg("Buffer allocated");
	unsigned long total_chars = 0;
	while(true){
		int chars_read = gzread(file, big_buffer, batch_in_bytes);
		if(chars_read < batch_in_bytes){
			break;
		}
		total_chars += chars_read;
		big_buffer[chars_read] = '\0';
	}
	msg(to_string((float)batch_in_bytes / (float)total_chars)+string(" was the len of the batch size."));*/
	/*int lines = wc(path, atoi(batch_len));
	msg(to_string(lines));*/
	msg("1");
	vector<size_t>* empty = new vector<size_t>;
	msg("1");
	std::thread producer(read_allocating, path, atoi(batch_len), empty);
	msg("1");
	producer.join();
	msg("1");
	//read_allocating(path, atoi(batch_len));
	exit(0);
}


int main (int argc, char *argv[]) {
	msg("Main func");
	int retval=0;

	if (argc < 2 || (strcmp (argv[1],"pe") != 0
		&& strcmp (argv[1],"se") != 0
		&& strcmp (argv[1],"--version") != 0
		&& strcmp (argv[1],"--help") != 0)) {
		if (strcmp (argv[1],"gzreader") == 0) {
			test_gzreader_lines (argv[2], argv[3]);
		}else{
			main_usage (EXIT_FAILURE);
		}
	}

	if (strcmp (argv[1],"--version") == 0) {
		fprintf(stdout, "%s version %0.2f\nCopyright (c) 2011 The Regents of University of California, \
		Davis Campus.\n%s is free software and comes with ABSOLUTELY NO WARRANTY.\nDistributed under the\
		 MIT License.\n\nWritten by %s\n", PROGRAM_NAME, VERSION, PROGRAM_NAME, AUTHORS);
		exit (EXIT_SUCCESS);
	} else if (strcmp (argv[1],"--help") == 0) {
		main_usage (EXIT_SUCCESS);
	} else if (strcmp (argv[1],"pe") == 0 || strcmp (argv[1],"se") == 0) {
		msg("Initializing trimmer.");
		if (strcmp (argv[1],"pe") == 0){
			Trim_Paired trimmer;
			msg("Initialized trimmer.");
			msg("Parsing trimmer arguments");
			retval = trimmer.parse_args(argc, argv);
			if(retval != 0) return retval;
			msg("Finished parsing trimmer arguments");

			msg("Starting to trim!");
			retval = trimmer.trim_main();
		}else{
			msg("It will be single.");
			Trim_Single trimmer;
			msg("Initialized trimmer.");
			msg("Parsing trimmer arguments");
			retval = trimmer.parse_args(argc, argv);
			if(retval != 0) return retval;
			msg("Finished parsing trimmer arguments");

			msg("Starting to trim!");
			retval = trimmer.trim_main();
		}
		return retval;
	}

	return 0;
}
