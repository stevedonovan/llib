CC=gcc
LIB=../llib/libllib.a
CFLAGS=-std=c99  -O2 -Wall -I..
LFLAGS=-L../llib -lllib

words.exe: words.c
	$(CC) $(CFLAGS) words.c -o words $(LFLAGS)
