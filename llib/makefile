# building LLIB
CC=gcc
CFLAGS=-std=c99 -O2 -Wall

all: obj.o list.o file.o scan.o map.o str.o sort.o
	ar rcu libllib.a obj.o list.o file.o scan.o map.o str.o sort.o && ranlib libllib.a

clean:
	rm *.o *.a


