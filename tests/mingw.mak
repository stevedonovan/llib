# building tests
CC=gcc
LIB=../llib/libllib.a
EXT=.exe
CFLAGS=-std=c99  -O2 -Wall -I..
LFLAGS=-L../llib -lllib
CCC=$(CC) $(CFLAGS)

EXES=test-obj.exe test-list.exe test-map.exe test-seq.exe test-file.exe \
	test-scan.exe test-str.exe test-template.exe \
	test-json.exe test-xml.exe test-table.exe test-pool.exe test-config.exe \
	testa.exe testing.exe

all: $(EXES)
	dir $(EXES)
	testing

testing.exe: testing.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-ob.exe: test-obj.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-list.exe: test-list.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-map.exe: test-map.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-seq.exe: test-seq.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-file.exe: test-file.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-scan.exe: test-scan.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-str.exe: test-str.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-template.exe: test-template.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-json.exe: test-json.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-xml.exe: test-xml.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-table.exe: test-table.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-pool.exe: test-pool.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

test-config.exe: test-config.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

testa: testa.c $(LIB)
	$(CCC) $< -o $@ $(LFLAGS)

clean:
	del $(EXES)


