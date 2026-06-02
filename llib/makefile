# building LLIB

ifeq ($(DEBUG),)
    CFLAGS = -std=c99 -O2 -Wall
else
    CFLAGS = -std=c99 -g
endif

OBJS=obj.o list.o file.o scan.o map.o str.o sort.o value.o template.o json.o \
arg.o json-parse.o json-data.o seq.o smap.o xml.o table.o farr.o pool.o \
interface.o filew.o file_fmt.o config.o

all: $(OBJS)
	ar rcu libllib.a $(OBJS) && ranlib libllib.a

clean:
	rm *.o *.a


