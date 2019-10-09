#include <iostream>
#include <assert.h>
#include "FQEntry.h"
#include "sickle.h"

using namespace std;

FQEntry::FQEntry(int previous, Batch *reader){
    //msg("new fqentry");
    position = previous + 1;

    this->name = reader->next_line();
    this->seq = reader->next_line();
    this->comment = reader->next_line();
    this->qual = reader->next_line();

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
    //string actual_seq = string("In ")+string(name)+string("(line ")+to_string((position*4)-4)+string(")");
    if(name.length() <= 1){
        string actual_seq = string("In ")+string(name)+string("(line ")+to_string((position*4)-4)+string(")");
        error(actual_seq);
        error("Sequence ID is to short.");
        error(string("ID:") + string(name));
        error(string("Sequence: ") + string(seq));
        error(string("Comment: ") + string(comment));
        error(string("Qualities: ") + string(qual));
        exit(EXIT_FAILURE);
    }

    if(name.at(0) != '@'){
        string actual_seq = string("In ")+string(name)+string("(line ")+to_string((position*4)-4)+string(")");
        error(actual_seq);
        error("Invalid char at the beggining of ID.");
        error(string("Sequence: ") + string(seq));
        error(string("Comment: ") + string(comment));
        error(string("Qualities: ") + string(qual));
        exit(EXIT_FAILURE);
    }

    if(seq.length() < 1){
        //error(actual_seq);
        error("Sequence line is empty");
        exit(EXIT_FAILURE);
    }

    if(qual.length() < 1){
        //error(actual_seq);
        error("Quality line is empty.");
        exit(EXIT_FAILURE);
    }

    if(qual.length() != seq.length()){
        //error(actual_seq);
        error("Sequence and quality lines have different lengths:");
        error(string(seq));
        error(string(qual));
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