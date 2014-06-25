#CC=gcc
LIB=../llib/libllib.a
CFLAGS=-std=c99  -O2 -Wall -I..
LFLAGS=-L../llib -lllib
CCC=$(CC) $(CFLAGS)

$(P): $(P).c
	$(CCC) $< -o $@ $(LFLAGS)
