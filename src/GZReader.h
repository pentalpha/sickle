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
    int more_buffer(char * more);
    std::string buffer;
    gzFile file;
    bool eof;
};

#endif