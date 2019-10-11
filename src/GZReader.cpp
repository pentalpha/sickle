#include <iostream>
#include <stdexcept>
#include "GZReader.h"


GZReader::GZReader(char* path, int batch_len, bool interleaved){
    if(interleaved){
        min_lines_in_batch = 8;
    }else{
        min_lines_in_batch = 4;
    }
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

Batch* GZReader::get_batch_buffering_lines()
{
    if(eof) return NULL;
    vector<const char*>* lines = read_lines();
    if(lines->size() > 0){
        Batch* batch;
        batch = new Batch(lines);
        return batch;
        return NULL;
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

vector<const char*>* GZReader::read_lines(){
    vector<const char*>* lines = new vector<const char*>();
    int remaining = batch_len;
    char *buffer = last_lines_buffer;
    if(buffer == NULL){
        buffer = new char[batch_len];
    }
    char* line;
    size_t line_len;
    if(last_remainder != NULL){
        for(int i = 0; i < last_remainder->size(); i++){
            const char* line = (*last_remainder)[i];
            line_len = strlen(line);
            remaining -= line_len;
            lines->push_back(line);
        }
    }
    do{
        if(!gzgets(file,buffer,batch_len)){
            eof = true;
            break;
        }
        line_len = strlen(buffer)-1;
        remaining -= line_len;
        if(buffer[line_len-1] != '\n'){
            line_len += 1;
        }
        line = new char[line_len+1];
        strncpy(line, buffer, line_len);
        line[line_len-1] = '\0';
        //msg(line);
        lines->push_back(line);
        //msg("stored new line");
    }while(remaining > 0);
    /*if(buffer){
        delete(buffer);
    }*/

    //msg("read lines from buffer");
    if(last_remainder != NULL){
        //msg("deleting last remainder");
        delete(last_remainder);
        last_remainder = NULL;
        //msg("deleted it");
    }
    int extra_lines = lines->size() % min_lines_in_batch;
    if(extra_lines > 0 && lines->size() > 0){
        //msg("adding extra lines");
        //msg(to_string(extra_lines));
        //msg(to_string(lines->size()));
        last_remainder = new vector<const char*>(extra_lines);
        for(int i = extra_lines-1; i >= 0; i--){
            //msg(to_string(i));
            //msg("to");
            //msg(to_string(lines->size()-1));
            //msg("value:");
            const char* value = lines->at(lines->size()-1);
            //msg(value);
            (*last_remainder)[i]= value;
            //msg("pushed");
            lines->pop_back();
            //msg("erased value");
            //last_remainder[i] = lines[last_remainder->size()-extra_lines+i];
        }
        //msg("added them");
        //msg(to_string(last_remainder->size()));
        /*for(int i = 0; i < extra_lines; i++){
            lines->erase(lines->end());
        }*/
        //msg("erased from lines");
    }

    return lines;
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