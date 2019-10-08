#include <iostream>
#include <stdexcept>
#include "GZReader.h"


GZReader::GZReader(char* path){
    std::cout << "Building reader for " << path << "\n";
    file = gzopen(path, "r");
    if (!file) {
        fprintf(stderr, "****Error: Could not open input file '%s'.\n\n", path);
    }
    eof = false;
    buffer = "";
    tmp_buffer = new char[BUFFER_SIZE+1];
    this->path = path;
}

GZReader::~GZReader(){
    //msg("destructing gzreader instance");
    delete(path);
    delete(tmp_buffer);
    gzclose(file);
    //msg("closed gzfile");
}

std::string GZReader::readline(){
    size_t new_line_pos = std::string::npos;
    while (new_line_pos == std::string::npos)
    {
        new_line_pos = buffer.find("\n");
        if(eof){
            //msg("All data has been already read.");
            break;
        }
        if(new_line_pos == std::string::npos){
            //char* more
            //msg("allocated temp char buffer");
            int n = more_buffer();
            //msg("read temp char buffer");
            buffer.append(tmp_buffer);
            //msg("increased buffer");
            //delete(more);
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
    if(eof && buffer.length() == 1){
        if(buffer[0] == '\n'){
            buffer = "";
        }
    }
    return line;
}

std::string* GZReader::read4(){
    std::string* strings = new std::string[4];
    string base_error = string("Number of lines in ") + string(path) + string(" not a multiple of 4.");
    strings[0] = readline();
    if(strings[0].length() == 0){
        throw EmptyStreamException("Reading from empty stream.");
    }
    strings[1] = readline();
    if(strings[1].length() == 0){
        throw LoadException(base_error + string("\nLast line: ") + strings[0]);
    }
    strings[2] = readline();
    if(strings[2].length() == 0){
        throw LoadException(base_error + string("\nLast line: ") + strings[1]);
    }
    strings[3] = readline();
    if(strings[3].length() == 0){
        throw LoadException(base_error + string("\nLast line: ") + strings[2]);
    }

    return strings;
}

int GZReader::more_buffer(){
    int chars_read = gzread(file, tmp_buffer, BUFFER_SIZE);
    if(chars_read < BUFFER_SIZE){
        eof = true;
    }else if(chars_read > BUFFER_SIZE){
        error("MORE CHARACTERS READ THAN BUFFER SIZE!");
        exit(EXIT_FAILURE);
    }
    tmp_buffer[chars_read] = '\0';
    return chars_read;
}

bool GZReader::reached_end(){
    return eof && buffer.length() == 0;
}

LoadException::LoadException(const std::string& message){
    this->message_ = message;
}

EmptyStreamException::EmptyStreamException(const std::string& message){
    this->message_ = message;
}