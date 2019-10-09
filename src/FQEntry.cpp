#include <iostream>
#include <assert.h>
#include "FQEntry.h"
#include "sickle.h"

using namespace std;

FQEntry::FQEntry(int previous, GZReader *reader){
    //msg("new fqentry");
    position = previous + 1;
    
    std::string* strs = reader->read4();

    this->name = strs[0];
    this->seq = strs[1];
    this->comment = strs[2];
    this->qual = strs[3];

    validate();
}

FQEntry::FQEntry(const FQEntry& other){
    position = other.position;
    this->name = other.name;
    assert(this->name.length());
    this->seq = other.seq;
    assert(this->seq.length());
    this->comment = other.comment;
    assert(this->comment.length());
    this->qual = other.qual;
    assert(this->qual.length());
}

FQEntry& FQEntry::operator=(const FQEntry& other) {
    position = other.position;
    this->name = other.name;
    assert(this->name.length());
    this->seq = other.seq;
    assert(this->seq.length());
    this->comment = other.comment;
    assert(this->comment.length());
    this->qual = other.qual;
    assert(this->qual.length());
    return (*this);
}

FQEntry::FQEntry(){
    position = 0;
    this->name = "";
    this->seq = "";
    this->qual = "";
    this->comment = "";
}

void FQEntry::validate(){
    string actual_seq = string("In ")+name+string("(line ")+to_string((position*4)-1)+string(")");
    if(name.length() <= 1){
        error(actual_seq);
        error("Sequence ID is to short.");
        error(string("ID:") + name);
        error(string("Sequence: ") + seq);
        error(string("Comment: ") + comment);
        error(string("Qualities: ") + qual);
        exit(EXIT_FAILURE);
    }

    if(seq.length() < 1){
        error(actual_seq);
        error("Sequence line is empty");
        exit(EXIT_FAILURE);
    }

    if(qual.length() < 1){
        error(actual_seq);
        error("Quality line is empty.");
        exit(EXIT_FAILURE);
    }

    if(qual.length() != seq.length()){
        error(actual_seq);
        error("Sequence and quality lines have different lengths:");
        error(seq);
        error(qual);
        exit(EXIT_FAILURE);
    }

    //msg("FQEntry validated");
}

/*FQEntry::~FQEntry(){
    msg("calling destroyer for");
    msg(name);
    msg(seq);
    msg(comment);
    msg(qual);
}*/