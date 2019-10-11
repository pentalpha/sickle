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

/*
Batch::Batch(const char * buffer, bool eof){
    msg("Making buffer");
    if(!eof){
        auto [view1, view2] = view_and_remainder(buffer);
        this->content = view1;
        this->remaining = view2;
    }else{
        content = string_view{buffer, strlen(buffer)};
        remaining = string_view{"", 0};
    }
    
    sequences_len = 0;
    msg("Make lines?");
    if(content.length()>0){
        make_lines();
        //splitSVPtr("\n");
    }else{
        msg("No, empty content");
    }
    last_line = -1;
    msg(string("Made buffer with ") + to_string(lines.size()) + string(" lines"));
    assert(lines.size() % 4 == 0);
}

const char * Batch::get_remainder(){
    return string(remaining).c_str();
}

//this was supposed to be faster than make_lines, but it's not
void Batch::splitSVPtr(std::string_view delims)
{
	//std::vector<std::string_view> output;
	//output.reserve(str.size() / 2);

	for (auto first = content.data(), second = content.data(), last = first + content.size();
        second != last && first != last;
        first = second + 1)
    {
		second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

		if (first != second)
			lines.emplace_back(first, second - first);
	}
}

void Batch::make_lines(){
    msg("Making lines");
    int next_newline = get_next_newline(0);
    size_t from = 0;
    while(next_newline != -1){
        //msg(string("Next \\n at ") + to_string(next_newline) + string(" from ") + to_string(from));
        string_view view = content.substr(from,next_newline-from);
        //msg(string("New line: ") + string(view));
        if(view.length()>0){
            if(lines.size() % 4 == 1){
                sequences_len += view.length();
            }else if(lines.size() % 4 == 0){
                if(view.at(0) != '@'){
                    error("Invalid ID: ");
                    error(string(view));
                    error("Previous line:");
                    error(string(lines.front()));
                    exit(EXIT_FAILURE);
                }
            }
            lines.push_back(view);
        }
        from = next_newline+1;
        next_newline = get_next_newline(from);
    }
    //msg(string("Next \\n at ") + to_string(next_newline) + string(" from ") + to_string(from));
    string_view view = content.substr(from,content.length()-from);
    if(view.length()>0){
        lines.push_back(view);
    }
    msg("Made lines");
}

int Batch::get_next_newline(size_t from){
    for(size_t i = from; i < content.length(); i++){
        if(content.at(i) == '\n'){
            return (int)i;
        }
    }
    return -1;
}

tuple<string_view, string_view> Batch::view_and_remainder(const char * chars,
    unsigned last_newline, unsigned total_len)
{
    string_view total_view{chars, total_len};
    string_view view;
    string_view remain;
    if(last_newline==total_len){
        view = total_view.substr(0,0);
        remain = total_view;
        //msg("\\n not present");
    }else{
        view = total_view.substr(0, last_newline);
        if(total_len-1 == last_newline){
            remain = total_view.substr(0, 0);
            //msg("\\n in last char");
        }else{
            remain = total_view.substr(last_newline+1, total_len);
            //msg("\\n in the middle");
        }
    }
    return {view, remain};
}

tuple<string_view, string_view> Batch::view_and_remainder(const char * chars,
    unsigned total_len)
{
    unsigned lines = 0;
    for(unsigned i = 1; i < total_len; i++){
        if(chars[i] == '\n'){
            if(chars[i-1] != '\n'){
                lines += 1;
            }
        }
    }
    unsigned lines_to_skip = lines % 4;
    unsigned last_n = total_len;
    for(int i = last_n-1; i >= 0; i--){
        if(chars[i] == '\n'){
            if(i>=1){
                if(chars[i-1] != '\n'){
                    if(lines_to_skip == 0){
                        last_n = i;
                        break;
                    }else{
                        lines_to_skip -= 1;
                    }
                }
            }else{
                if(lines_to_skip == 0){
                    last_n = i;
                    break;
                }else{
                    lines_to_skip -= 1;
                }
            }
        }
    }
    return view_and_remainder(chars, last_n, total_len);
}

tuple<string_view, string_view> Batch::view_and_remainder(const char * chars)
{
    return view_and_remainder(chars, strlen(chars));
}*/