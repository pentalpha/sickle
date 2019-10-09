#ifndef _GZREADER_
#define _GZREADER_

#include <string>
#include <zlib.h>
#include "sickle.h"

using namespace std;

class GZReader{
public:
    GZReader(char* path);
    ~GZReader();
    std::string readline();
    std::string* read4();
    bool reached_end();
    char* path;
private:
    int more_buffer();
    std::string buffer;
    gzFile file;
    bool eof;
    char* tmp_buffer;
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