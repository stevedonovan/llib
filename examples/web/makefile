LLIB=../..
LLIBP=$(LLIB)/llib-p
PFILES=$(LLIBP)/socket.c $(LLIBP)/http.c $(LLIBP)/select.c
CFLAGS=-std=c99  -O2 -Wall -I$(LLIB)
LFLAGS=-L$(LLIB)/llib -lllib -lm
CCC=$(CC) $(CFLAGS)

$(P): $(P).c
	$(CCC) $< -o $@ $(PFILES) $(LFLAGS)

clean:
	rm $(P)
