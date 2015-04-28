# building LLIB
CC=gcc
CFLAGS=-std=c99 -O2 -Wall -I..

OBJS=http.o rx.o select.o socket.o

all: $(OBJS)
	ar rcu libllibp.a $(OBJS) && ranlib libllibp.a

clean:
	rm *.o *.a


