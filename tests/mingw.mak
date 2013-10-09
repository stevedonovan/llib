# building tests
CC=gcc
LIB=../llib/libllib.a
EXT=.exe
CFLAGS=-std=c99  -O2 -Wall -I..
LFLAGS=-L../llib -lllib

OBJ=test-obj.exe
LIST=test-list.exe
MAP=test-map.exe
SEQ=test-seq.exe
FILE=test-file.exe
SCAN=test-scan.exe
STR=test-str.exe

all: $(OBJ) $(LIST) $(MAP) $(SEQ) $(FILE) $(SCAN) $(STR)
	test-obj > test-obj-output
	test-list > test-list-output
	test-seq > test-seq-output
	test-file > test-file-output
	test-scan > test-scan-output
	test-str > test-str-output

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






