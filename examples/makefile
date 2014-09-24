#CC=gcc
LLIB=..
CFLAGS=-std=c99  -O2 -Wall -I$(LLIB)
LFLAGS=-L$(LLIB)/llib -lllib -lm
CCC=$(CC) $(CFLAGS)

$(P): $(P).c
	$(CCC) $< -o $@ $(LFLAGS)

clean:
	rm $(P)
