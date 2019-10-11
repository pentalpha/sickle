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
    //Batch(const char * buffer, bool eof = false);
    Batch(vector<const char*>* lines);
    Batch(vector<const char*>* lines, vector<const char*>* previous_lines);
    bool has_lines();

    string_view next_line();

    const char * get_remainder();

    int sequences_len;
    int n_lines();
    void free_this();
private:
    //void splitSVPtr(std::string_view delims = " ");
    //string_view content, remaining;
    vector<string_view> lines;
    size_t last_line;
    vector<const char*>* lines_raw;
    vector<const char*>* previous_lines;

    //void make_lines();

    //int get_next_newline(size_t from);

    //tuple<string_view, string_view> view_and_remainder(const char * chars,
    //    unsigned last_newline, unsigned total_len);

    //tuple<string_view, string_view> view_and_remainder(const char * chars,
    //    unsigned total_len);

    //tuple<string_view, string_view> view_and_remainder(const char * chars);
};

#endif