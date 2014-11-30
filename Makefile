# -*- mode: makefile -*-
# 
# Makefile for compiling 'htu21d' on Raspberry Pi. 
#
# Sun Nov 30 14:47:26 CET 2014
# Edit: 
# Jaakko Koivuniemi

CXX           = gcc
CXXFLAGS      = -g -O -Wall 
LD            = gcc
LDFLAGS       = -lm -O

%.o : %.c
	$(CXX) $(CXXFLAGS) -c $<

all: htu21d 

htu21d: htu21d.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o

