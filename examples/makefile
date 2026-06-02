#CC=gcc
LLIB=..
ifeq ($(DEBUG),)
    OPT = -O2
else
    OPT = -g
endif
CFLAGS=-std=c99  $(OPT) -Wall -I$(LLIB)
LFLAGS=-L$(LLIB)/llib -lllib -lm

$(P): $(P).c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm $(P)
