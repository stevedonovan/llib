# building LLIB
CC=gcc
CFLAGS=-std=c99 -O2 -Wall

OBJS=obj.o list.o file.o scan.o map.o str.o sort.o value.o template.o json.o \
arg.o json-data.o seq.o smap.o xml.o table.o

all: $(OBJS)
	ar rcu libllib.a $(OBJS) && ranlib libllib.a

clean:
	rm *.o *.a


