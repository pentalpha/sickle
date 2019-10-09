#ifndef _FQ_ENTRY_
#define _FQ_ENTRY_

#include <string>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>
#include <getopt.h>
#include <vector>
#include "GZReader.h"

class FQEntry{
public:
    FQEntry(int previous, GZReader *reader);
    FQEntry(const FQEntry &other);
    FQEntry();
    FQEntry& operator=(const FQEntry& other);
    //~FQEntry();
    std::string name;
    std::string comment;
    std::string seq;
    std::string qual;
    int position;
    void validate();
};

#endif