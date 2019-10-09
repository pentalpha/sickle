#ifndef _LINES_BATCH_
#define _LINES_BATCH_

#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <cstring>
#include <assert.h>
#include <iostream>
#include "sickle.h"

using namespace std;

class Batch{
public:
    Batch(const char * buffer);

    bool has_lines();

    string_view next_line();

    const char * get_remainder();

    int sequences_len;
    int n_lines();
private:
    string_view content, remaining;
    vector<string_view> lines;
    size_t last_line;

    void make_lines();

    int get_next_newline(size_t from);

    tuple<string_view, string_view> view_and_remainder(const char * chars,
        unsigned last_newline, unsigned total_len);

    tuple<string_view, string_view> view_and_remainder(const char * chars,
        unsigned total_len);

    tuple<string_view, string_view> view_and_remainder(const char * chars);
};

#endif