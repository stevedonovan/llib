#CC=gcc
LLIB=../..
CFLAGS=-std=c99  -O2 -Wall -I$(LLIB)
LFLAGS=-L$(LLIB)/llib -lllib -lm

pkgconfig: pkgconfig.c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)
	strip $@

clean:
	rm pkgconfig

