#CC=gcc
LLIB=..
CFLAGS=-std=c99  -O2 -Wall -I$(LLIB)
LFLAGS=-L$(LLIB)/llib -lllib -lm

$(P): $(P).c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm $(P)
