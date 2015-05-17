# -*- mode: makefile -*-
# 
# Makefile for compiling 'htu21d' on Raspberry Pi. 
#
# Sun Nov 30 14:47:26 CET 2014
# Edit: Sat May 16 23:22:15 CEST 2015
# Jaakko Koivuniemi

CXX           = gcc
CXXFLAGS      = -g -O -Wall 
LD            = gcc
LDFLAGS       = -lsqlite3 -lm -O

%.o : %.c
	$(CXX) $(CXXFLAGS) -c $<

all: htu21d 

htu21d: insertSQLite.o readSQLiteTime.o readregister.o resetchip.o writefile.o htu21d.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o

