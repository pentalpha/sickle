PROGRAM_NAME = sickle
VERSION = 1.33
CXX = g++
CXXFLAGS = -std=c++11 -Wall -DVERSION=$(VERSION)
DEBUG = -g
OPT = -O1
ARCHIVE = $(PROGRAM_NAME)_$(VERSION)
LDFLAGS=
LIBS = -lz
SDIR = src

.PHONY: clean default build distclean dist debug

default: build

sliding.o: $(SDIR)/sliding.cpp $(SDIR)/trim.h
	$(CXX) $(CXXFLAGS) $(OPT) -c $(SDIR)/$*.cpp

trim_single.o: $(SDIR)/trim_single.cpp $(SDIR)/trim.h
	$(CXX) $(CXXFLAGS) $(OPT) -c $(SDIR)/$*.cpp

trim_paired.o: $(SDIR)/trim_paired.cpp $(SDIR)/trim.h
	$(CXX) $(CXXFLAGS) $(OPT) -c $(SDIR)/$*.cpp

sickle.o: $(SDIR)/sickle.cpp $(SDIR)/sickle.h
	$(CXX) $(CXXFLAGS) $(OPT) -c $(SDIR)/$*.cpp

print_record.o: $(SDIR)/print_record.cpp $(SDIR)/print_record.h
	$(CXX) $(CXXFLAGS) $(OPT) -c $(SDIR)/$*.cpp

GZReader.o: $(SDIR)/GZReader.cpp $(SDIR)/GZReader.h
	$(CXX) $(CXXFLAGS) $(OPT) -c $(SDIR)/$*.cpp

FQEntry.o: $(SDIR)/FQEntry.cpp $(SDIR)/FQEntry.h
	$(CXX) $(CXXFLAGS) $(OPT) -c $(SDIR)/$*.cpp

clean:
	rm -rf *.o $(SDIR)/*.gch ./sickle

distclean: clean
	rm -rf *.tar.gz

dist:
	tar -zcf $(ARCHIVE).tar.gz src Makefile README.md sickle.xml LICENSE

build: GZReader.o FQEntry.o sliding.o trim_single.o trim_paired.o sickle.o print_record.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OPT) $? -o sickle $(LIBS)

debug:
	$(MAKE) build "CXXFLAGS=-Wall -pedantic -D__STDC_LIMIT_MACROS -g -DDEBUG"

