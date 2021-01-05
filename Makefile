CC = gcc

option=-c

all:	fs.o mem_alloc.o
		$(CC) babystep3.o mem_alloc.o -o  all

mem_alloc.o:	fs.c mymalloc.h
		$(CC) $(option) mem_alloc.c

fs.o: fs.c mymalloc.h 
		$(CC) $(option) fs.c

clean: 
		rm -rf *o all
