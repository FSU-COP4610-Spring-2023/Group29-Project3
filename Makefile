# Name: Ethan Burke, Abraham Beltran, and Daniel Pijeira
# Professor: Andy Wang, PhD
# Class: COP 4610
# Project: 3
# Description: This is the Project 3 makefile.

BINS = filesys
C = gcc
CFLAGS = -std=gnu99 -Wall -pedantic -g

all: $(BINS)

filesys: filesys.c
	$(C) $(CFLAGS) -o $(BINS) filesys.c

clean:
	rm $(BINS)