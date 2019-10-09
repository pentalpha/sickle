#include <iostream>
#include <stdexcept>
#include "GZReader.h"


GZReader::GZReader(char* path, int batch_len){
    std::cout << "Building reader for " << path << "\n";
    file = gzopen(path, "r");
    if (!file) {
        fprintf(stderr, "****Error: Could not open input file '%s'.\n\n", path);
    }
    eof = false;
    this->path = path;
    this->batch_len = batch_len;
}

GZReader::~GZReader(){
    //msg("destructing gzreader instance");
    delete(path);
    gzclose(file);
    //msg("closed gzfile");
}

Batch* GZReader::get_batch()
{
    if(eof) return NULL;
    auto[chars, n_chars] = read_n_chars(batch_len);
    Batch* batch = new Batch(chars);
    if(batch->has_lines()){
        return batch;
    }else{
        return NULL;
    }
}

Batch* GZReader::get_batch(string remains)
{
    if(eof && remains.length()==0) return NULL;
    auto[chars, n_chars] = read_n_chars(batch_len);
    string* str;
    if(eof){
        str = new string(remains + string(chars) + "\n");
    }else{
        str = new string(remains + string(chars));
    }
    delete(chars);
    Batch* batch = new Batch(str->c_str());
    if(batch->has_lines()){
        return batch;
    }else{
        return NULL;
    }
}

tuple<const char*, int> GZReader::read_n_chars(int n_chars){
    msg("Reading buffer");
    char* big_buffer = new char[n_chars+1];
    int chars_read = gzread(file, big_buffer, n_chars);
    if(chars_read < n_chars){
        eof = true;
    }else if(chars_read > n_chars){
        error("MORE CHARACTERS READ THAN BUFFER SIZE!");
        exit(EXIT_FAILURE);
    }
    big_buffer[chars_read] = '\0';
    msg(string("Read ")+to_string(chars_read)+string(" chars"));
    msg("Finished reading buffer");
    return {big_buffer, chars_read};
}

LoadException::LoadException(const std::string& message){
    this->message_ = message;
}

EmptyStreamException::EmptyStreamException(const std::string& message){
    this->message_ = message;
}

bool GZReader::reached_end(){
    return eof;
}

/*

int GZReader::buffer_len(){
    return buffer.size()-buffer_start;
}

void GZReader::make_lines_from_buffer(){
    msg("Adding lines");
    int newline = find_newline_in_buffer();
    int lines_added = 0;
    while(newline != -1){
        if(newline > 0){
            std::string_view view{buffer.c_str(), buffer.size()};
            lines.push(view.substr(buffer_start, newline));
            //lines.push(new string(buffer.substr(0, newline)));
            lines_added += 1;
            buffer_start+=newline+1;
            //buffer.erase(0, newline+1);
            //msg(to_string(lines_added));
        }else{
            //buffer.erase(0, 1);
            buffer_start+=1;
        }
        newline = find_newline_in_buffer();
    }
    if(eof){
        if(buffer_len()>0){
            if(buffer.at(buffer.length()-1) == '\n'){
                newline = buffer.length()-1;
            }else{
                newline = buffer.length();
            }
            if(newline > 0){
                std::string_view view{buffer.c_str(), buffer.size()};
                lines.push(view.substr(buffer_start, newline));
                lines_added += 1;
            }
            //buffer.erase();
        }
    }
    msg("Added lines");
    msg(to_string(lines_added));
}

int GZReader::find_newline_in_buffer(){
    for(int i = buffer_start; i < buffer.size(); i++){
        if(buffer.at(i) == '\n'){
            return i;
        }
    }
    return -1;
}

std::string_view GZReader::readline(){

    if(lines.empty()){
        if(!eof){
            read_n_chars(batch_len);
        }
        make_lines_from_buffer();
        if(lines.empty()){
            return "";
        }else{
            return readline();
        }
    }else{
        std::string_view line = lines.front();
        lines.pop();
        return line;
    }
}

std::string_view* GZReader::read4(){
    std::string_view* strings = new std::string_view[4];
    string base_error = string("Number of lines in ") + string(path) + string(" not a multiple of 4.");
    strings[0] = readline();
    if(strings[0].length() == 0){
        throw EmptyStreamException("Reading from empty stream.");
    }
    if(strings[0].at(0) != '@'){
        throw LoadException(string("Invalid char at the beggining of ID.") + string(strings[0]));
    }
    strings[1] = readline();
    if(strings[1].length() == 0){
        throw LoadException(base_error + string("\nLast line: ") + string(strings[0]));
    }
    strings[2] = readline();
    if(strings[2].length() == 0){
        throw LoadException(base_error + string("\nLast line: ") + string(strings[1]));
    }
    strings[3] = readline();
    if(strings[3].length() == 0){
        throw LoadException(base_error + string("\nLast line: ") + string(strings[2]));
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
}*/