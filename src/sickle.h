#ifndef SICKLE_H
#define SICKLE_H

#include <limits.h>
#include <zlib.h>
#include <iostream>

#define BUFFER_SIZE 512

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "sickle"
#endif

#ifndef AUTHORS
#define AUTHORS "Nikhil Joshi, UC Davis Bioinformatics Core\n"
#endif

#ifndef VERSION
#define VERSION 0.0
#endif

/* Options drawn from GNU's coreutils/src/system.h */
/* These options are defined so as to avoid conflicting with option
values used by commands */
enum {
  GETOPT_HELP_CHAR = (CHAR_MIN - 2),
  GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};
#ifndef GETOPT_HELP_OPTION_DECL
#define GETOPT_HELP_OPTION_DECL \
"help", no_argument, NULL, GETOPT_HELP_CHAR
#endif
#ifndef GETOPT_VERSION_OPTION_DECL
#define GETOPT_VERSION_OPTION_DECL \
"version", no_argument, NULL, GETOPT_VERSION_CHAR
#endif
#ifndef case_GETOPT_HELP_CHAR
#define case_GETOPT_HELP_CHAR(Usage_call) \
Usage_call(EXIT_SUCCESS, NULL);
#endif
#ifndef case_GETOPT_VERSION_CHAR
#define case_GETOPT_VERSION_CHAR(Program_name, Version, Authors) \
fprintf(stdout, "%s version %0.3f\nCopyright (c) 2011 The Regents " \
"of University of California, Davis Campus.\n" \
"%s is free software and comes with ABSOLUTELY NO WARRANTY.\n"\
"Distributed under the MIT License.\n\nWritten by %s\n", \
Program_name, Version, Program_name, Authors); \
exit(EXIT_SUCCESS);
#endif
/* end code drawn from system.h */

typedef enum {
  PHRED,
  SANGER,
  SOLEXA,
  ILLUMINA
} quality_type;

static const char typenames[4][10] = {
	{"Phred"},
	{"Sanger"},
	{"Solexa"},
	{"Illumina"}
};

#ifndef Q_OFFSET
#define Q_OFFSET 0
#endif
#ifndef Q_MIN
#define Q_MIN 1
#endif
#ifndef Q_MAX
#define Q_MAX 2
#endif

static const int quality_constants[4][3] = {
  /* offset, min, max */
  {0, 4, 60}, /* PHRED */
  {33, 33, 126}, /* SANGER */
  {64, 58, 112}, /* SOLEXA; this is an approx; the transform is non-linear */
  {64, 64, 110} /* ILLUMINA */
};

typedef struct __cutsites_ {
    int five_prime_cut;
	int three_prime_cut;
} cutsites;

#ifndef _DEBUGMODE_
#define _DEBUGMODE_ true
#endif

inline void msg(const char * content){
  if(_DEBUGMODE_) {
    std::cout << "[DEBUGGING] " << content << std::endl;
    std::flush(std::cout);
  }
}

inline void msg(std::string content){
  msg(content.c_str());
}

inline void error(const char * content){
    std::cerr << "[ERROR] " << content << std::endl;
    std::flush(std::cerr);
}
inline void error(std::string content){
    std::cerr << "[ERROR] " << content << std::endl;
    std::flush(std::cerr);
}

#endif /*SICKLE_H*/
