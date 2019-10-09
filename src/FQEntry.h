#ifndef _FQ_ENTRY_
#define _FQ_ENTRY_

#include <string>
#include <string_view>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>
#include <getopt.h>
#include <vector>
#include "Batch.h"

class FQEntry{
public:
    FQEntry(int previous, Batch *reader);
    FQEntry(const FQEntry &other);
    FQEntry();
    FQEntry& operator=(const FQEntry& other);
    //~FQEntry();
    std::string_view name;
    std::string_view comment;
    std::string_view seq;
    std::string_view qual;
    int position;
    void validate();
};

#endif