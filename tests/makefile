# building tests
CC=gcc
LIB=../llib/libllib.a
EXT=.exe
CFLAGS=-std=c99  -O2 -Wall -I..
LFLAGS=-L../llib -lllib

ifdef ComSpec
	EXT=.exe
    L=
else
	EXT=
    L=./
endif

OBJ=test-obj$(EXT)
LIST=test-list$(EXT)
MAP=test-map$(EXT)
SEQ=test-seq$(EXT)
FILE=test-file$(EXT)
SCAN=test-scan$(EXT)
STR=test-str$(EXT)

all: $(OBJ) $(LIST) $(MAP) $(SEQ) $(FILE) $(SCAN) $(STR)
	$(L)test-obj > test-obj-output
	$(L)test-list > test-list-output
	$(L)test-seq > test-seq-output
	$(L)test-file > test-file-output
	$(L)test-scan > test-scan-output
	$(L)test-str > test-str-output

$(OBJ): test-obj.c $(LIB)
	$(CC) $(CFLAGS) test-obj.c -o test-obj $(LFLAGS)

$(LIST): test-list.c $(LIB)
	$(CC) $(CFLAGS) test-list.c -o test-list $(LFLAGS)

$(MAP): test-map.c $(LIB)
	$(CC) $(CFLAGS) test-map.c -o test-map $(LFLAGS)

$(SEQ): test-seq.c $(LIB)
	$(CC) $(CFLAGS) test-seq.c -o test-seq $(LFLAGS)

$(FILE): test-file.c $(LIB)
	$(CC) $(CFLAGS) test-file.c -o test-file $(LFLAGS)

$(SCAN): test-scan.c $(LIB)
	$(CC) $(CFLAGS) test-scan.c -o test-scan $(LFLAGS)

$(STR): test-str.c $(LIB)
	$(CC) $(CFLAGS) test-str.c -o test-str $(LFLAGS)






