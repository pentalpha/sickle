#include <iostream>
#include "GZReader.h"


GZReader::GZReader(char* path){
    std::cout << "Building reader for " << path << "\n";
    file = gzopen(path, "r");
    if (!file) {
        fprintf(stderr, "****Error: Could not open input file '%s'.\n\n", path);
    }
    eof = false;
    buffer = "";
}

GZReader::~GZReader(){
    msg("destructing gzreader instance");
    //gzclose(file);
}

std::string GZReader::readline(){
    size_t new_line_pos = std::string::npos;
    while (new_line_pos == std::string::npos)
    {
        new_line_pos = buffer.find("\n");
        if(eof){
            //msg("End of file, breaking loop.");
            break;
        }
        if(new_line_pos == std::string::npos){
            char* more = new char[BUFFER_SIZE+1];
            //msg("allocated temp char buffer");
            int n = more_buffer(more);
            //msg("read temp char buffer");
            buffer.append(more);
            //msg("appended temp char buffer");
            delete(more);
            //msg("deleted temp char buffer");
        }
    }
    std::string line;
    if(new_line_pos == std::string::npos){
        line = buffer.substr(0,buffer.length());
        buffer = "";
    }else if(new_line_pos == buffer.length()-1){
        line = buffer.substr(0,buffer.length()-1);
        buffer = "";
    }else{
        //msg(std::string("buffer:") + buffer);
        line = buffer.substr(0, new_line_pos);
        //msg(std::string("line:") + line);
        buffer = buffer.substr(new_line_pos+1, buffer.length());
        //msg(std::string("newbuffer:") + buffer);
    }
    return line;
}

std::string* GZReader::read4(){
    std::string* strings = new std::string[4];
    strings[0] = readline();
    strings[1] = readline();
    strings[2] = readline();
    strings[3] = readline();
    return strings;
}

int GZReader::more_buffer(char * more){
    int chars_read = gzread(file, more, BUFFER_SIZE);
    if(chars_read < BUFFER_SIZE){
        eof = true;
    }else if(chars_read > BUFFER_SIZE){
        error("MORE CHARACTERS READ THAN BUFFER SIZE!");
        exit(EXIT_FAILURE);
    }
    more[chars_read] = '\0';
    return chars_read;
}

bool GZReader::reached_end(){
    return eof && buffer.length() == 0;
}