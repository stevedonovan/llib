# building LLIB
CC=gcc
CFLAGS=-std=c99 -O2 -Wall -DLLIB_PTR_LIST

OBJS=obj.o list.o file.o scan.o map.o str.o sort.o value.o template.o json.o \
arg.o json-parse.o json-data.o seq.o smap.o xml.o table.o farr.o pool.o \
interface.o filew.o config.o

all: $(OBJS)
	ar rcu libllib.a $(OBJS) && ranlib libllib.a

clean:
	del *.o *.a


