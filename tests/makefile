# building tests
CC=gcc
LIB=../llib/libllib.a
CFLAGS=-std=c99  -O2 -Wall -I..
LFLAGS=-Wl,-s -L../llib -lllib

EXES=test-obj test-list test-map test-seq test-file \
	test-scan test-str test-template \
	test-json test-xml test-table test-pool

all: $(EXES)
	./test-obj > test-obj-output
	./test-list > test-list-output
	./test-seq > test-seq-output
	./test-file > test-file-output
	./test-scan > test-scan-output
	./test-str > test-str-output
	./test-template > test-template-output
	./test-json > test-json-output
	./test-xml > test-xml-output
	./test-table > test-table-output
    ./test-pool > test-pool-output
	ls -l $(EXES)

test-obj: test-obj.c $(LIB)
	$(CC) $(CFLAGS) test-obj.c -o test-obj $(LFLAGS)

test-list: test-list.c $(LIB)
	$(CC) $(CFLAGS) test-list.c -o test-list $(LFLAGS)

test-map: test-map.c $(LIB)
	$(CC) $(CFLAGS) test-map.c -o test-map $(LFLAGS)

test-seq: test-seq.c $(LIB)
	$(CC) $(CFLAGS) test-seq.c -o test-seq $(LFLAGS)

test-file: test-file.c $(LIB)
	$(CC) $(CFLAGS) test-file.c -o test-file $(LFLAGS)

test-scan: test-scan.c $(LIB)
	$(CC) $(CFLAGS) test-scan.c -o test-scan $(LFLAGS)

test-str: test-str.c $(LIB)
	$(CC) $(CFLAGS) test-str.c -o test-str $(LFLAGS)

test-template: test-template.c $(LIB)
	$(CC) $(CFLAGS) test-template.c -o test-template $(LFLAGS)

test-json: test-json.c $(LIB)
	$(CC) $(CFLAGS) test-json.c -o test-json $(LFLAGS)

test-xml: test-xml.c $(LIB)
	$(CC) $(CFLAGS) test-xml.c -o test-xml $(LFLAGS)

test-table: test-table.c $(LIB)
	$(CC) $(CFLAGS) test-table.c -o test-table $(LFLAGS)

test-pool: test-pool.c $(LIB)
	$(CC) $(CFLAGS) test-pool.c -o test-pool $(LFLAGS)

clean:
	rm $(EXES)




