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
private:
    int more_buffer();
    std::string buffer;
    gzFile file;
    bool eof;
    char* tmp_buffer;
};

#endif