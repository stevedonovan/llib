# building tests
#CC=gcc
LIB=../llib/libllib.a
CFLAGS=-std=c99  -O2 -Wall -I..
LFLAGS=-Wl,-s -L../llib -lllib
CCC=$(CC) $(CFLAGS)

EXES=test-obj test-list test-map test-seq test-file \
	test-scan test-str test-template \
	test-json test-xml test-table test-pool test-config \
    testa testing test-array test-interface

all: $(EXES)
	ls -l $(EXES)
	./testing

testing: testing.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-obj: test-obj.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-list: test-list.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-map: test-map.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-seq: test-seq.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-file: test-file.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-scan: test-scan.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-str: test-str.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-template: test-template.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-json: test-json.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-xml: test-xml.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-table: test-table.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-pool: test-pool.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-config: test-config.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-sqlite3-table: test-sqlite3-table.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS) -lsqlite3

testa: testa.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)
	
test-array: test-array.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-interface: test-interface.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)
	
clean:
	rm $(EXES)




