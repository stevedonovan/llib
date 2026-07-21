#CC=gcc
LLIB=..
ifeq ($(DEBUG),)
    OPT = -O2
else
    OPT = -g
endif
CFLAGS=-std=c2x  $(OPT) -Wall -I$(LLIB)
LFLAGS=-L$(LLIB)/llib -lllib -lm

all: cexpr getdoc

cexpr: cexpr.c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

getdoc: getdoc.c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm cexpr getdoc
