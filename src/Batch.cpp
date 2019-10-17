#include "Batch.h"
#include <algorithm>

using namespace std;

Batch::Batch(vector<const char*>* lines_raw){
    //msg("Making buffer");
    this->lines_raw = lines_raw;
    last_line = -1;
    sequences_len = 0;
    //int lines_with_n = 0;
    for(int i = 0; i < lines_raw->size(); i++){
        const char* line_raw = lines_raw->at(i);
        lines.push_back(string_view{line_raw});
        sequences_len += lines.back().length();
        //if(lines.back().back() == '\n') lines_with_n++;
    }
    //msg(string("Lines with \\n at the end: ") + to_string(lines_with_n));
    //msg(string("Made buffer with ") + to_string(lines.size()) + string(" lines"));
    assert(lines.size() % 4 == 0);
}

Batch::Batch(vector<const char*>* lines_raw, vector<const char*>* previous_lines){
    //msg("Making buffer");
    this->lines_raw = lines_raw;
    //msg("linew_raw");
    //msg(to_string(lines_raw->size()));
    //msg(to_string(lines_raw->size()%4));
    this->previous_lines = previous_lines;
    //msg("previous_lines");
    //msg(to_string(previous_lines->size()));
    last_line = -1;
    sequences_len = 0;
    //int lines_with_n = 0;
    for(int i = 0; i < previous_lines->size(); i++){
        const char* line_raw = previous_lines->at(i);
        lines.push_back(string_view{line_raw});
        sequences_len += lines.back().length();
        //if(lines.back().back() == '\n') lines_with_n++;
    }
    for(int i = 0; i < lines_raw->size(); i++){
        const char* line_raw = lines_raw->at(i);
        lines.push_back(string_view{line_raw});
        sequences_len += lines.back().length();
        //if(lines.back().back() == '\n') lines_with_n++;
    }
    //msg(string("Lines with \\n at the end: ") + to_string(lines_with_n));
    //msg(string("Made buffer with ") + to_string(lines.size()) + string(" lines"));
    assert(lines.size() % 4 == 0);
}

void Batch::free_this(){
    for(int i = 0; i < lines_raw->size(); i++){
		delete(lines_raw->at(i));
	}
	delete(lines_raw);
}

int Batch::n_lines(){
    return lines.size();
}

bool Batch::has_lines(){
    return last_line+1 < lines.size();
}

string_view Batch::next_line(){
    assert(has_lines());
    last_line += 1;
    return lines.at(last_line);
}
