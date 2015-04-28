#CC=gcc
LLIB=..
CFLAGS=-std=c99  -O2 -Wall -I$(LLIB)
LFLAGS=$(LLIB)/llib-p/libllibp.a $(LLIB)/llib/libllib.a  -lm

$(P): $(P).c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm $(P)
