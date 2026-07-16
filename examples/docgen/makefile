#CC=gcc
LLIB=../..
ifeq ($(DEBUG),)
    OPT = -O2
else
    OPT = -g
endif
CFLAGS=-std=c99 $(OPT) -Wall -Wextra -I$(LLIB)
LFLAGS=-L$(LLIB)/llib -lllib -lm

docgen: docgen.c $(LLIB)/llib/libllib.a
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm -f docgen
