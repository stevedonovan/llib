# building LLIB
CC=gcc
CFLAGS=-std=c99 -O2 -Wall

OBJS=obj.o list.o file.o scan.o map.o str.o sort.o value.o template.o json.o

all: $(OBJS)
	ar rcu libllib.a $(OBJS) && ranlib libllib.a

clean:
	rm *.o *.a


