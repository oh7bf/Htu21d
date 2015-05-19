# -*- mode: makefile -*-
# 
# Makefile for compiling 'htu21d' on Raspberry Pi. 
#
# Sun Nov 30 14:47:26 CET 2014
# Edit: Tue May 19 21:53:16 CEST 2015
# Jaakko Koivuniemi

CXX           = gcc
CXXFLAGS      = -g -O -Wall 
LD            = gcc
LDFLAGS       = -lsqlite3 -lm -O

%.o : %.c
	$(CXX) $(CXXFLAGS) -c $<

all: htu21d 

htu21d: InsertSQLite.o ReadSQLiteTime.o ReadRegister.o WriteCommand.o WriteFile.o Htu21dTemperature.o htu21d.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o

