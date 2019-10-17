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

int main (int argc, char *argv[]) {
	msg("Main func");
	int retval=0;

	if (argc < 2 || (strcmp (argv[1],"pe") != 0
		&& strcmp (argv[1],"se") != 0
		&& strcmp (argv[1],"--version") != 0
		&& strcmp (argv[1],"--help") != 0)) {
		main_usage (EXIT_FAILURE);
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
