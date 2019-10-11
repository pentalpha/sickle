#ifndef _GZREADER_
#define _GZREADER_

#include <string>
#include <zlib.h>
#include <queue>
#include <string_view>
#include <tuple>
#include "sickle.h"
#include "Batch.h"

using namespace std;

class GZReader{
public:
    GZReader(char* path, int batch_len, bool interleaved = false);
    ~GZReader();
    //std::string_view readline();
    //std::string_view* read4();
    Batch* get_batch_buffering_lines();
    vector<const char*>* read_lines();
    bool reached_end();

    //int buffer_len();
    char* path;
private:
    tuple<const char*, int> read_n_chars(int n_chars);
    vector<const char*>* last_remainder = NULL;
    gzFile file;
    bool eof;
    int batch_len;
    char* last_lines_buffer = NULL;
    int min_lines_in_batch;
    //int more_buffer();
    //int find_newline_in_buffer();
    //void make_lines_from_buffer();

    //std::string buffer;
    //std::queue<string_view> lines;
    
    //char* tmp_buffer;
    
    //int buffer_start;
};

class LoadException: public std::exception {
private:
    std::string message_;
public:
    explicit LoadException(const std::string& message);
    virtual const char* what() const throw() {
        return message_.c_str();
    }
};

class EmptyStreamException: public std::exception {
private:
    std::string message_;
public:
    explicit EmptyStreamException(const std::string& message);
    virtual const char* what() const throw() {
        return message_.c_str();
    }
};

#endif