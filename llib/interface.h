/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_INTERFACE_H
#define _LLIB_INTERFACE_H
#include "obj.h"

typedef void* (*ObjLookup)(const void *o, const void *key);

typedef struct Accessor_ {
    ObjLookup lookup;
} Accessor;

typedef struct Iterator_ Iterator;

typedef struct Iterable_ {
    Iterator* (*init)(const void *o);
} Iterable;

struct Iterator_ {
    bool (*next)(Iterator *iter, void *pval);
    bool (*nextpair)(Iterator *iter, void *pkey, void *pval); // optional
};

void interface_add(int itype, int type, void *funs);
void* interface_get(int itype, const void *obj);
ObjLookup interface_get_lookup(const void *P);
Iterator* interface_get_iterator(const void *P);

#endif
