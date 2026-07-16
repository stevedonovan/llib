#CC=gcc
LLIB=../..
CFLAGS=-std=c99 -O2 -Wall -I$(LLIB)
LFLAGS=-L$(LLIB)/llib -lllib -lm

noter: noter.c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)
	strip $@

clean:
	rm noter
