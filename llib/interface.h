/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_INTERFACE_H
#define _LLIB_INTERFACE_H
#include "obj.h"

#define interface_typeof(T) interface_typeof_(#T)
#define interface_get_by_name(T,obj) (T*)interface_get(interface_typeof(T),obj)

typedef void* (*ObjLookup)(const void *o, const void *key);

typedef struct Accessor_ {
    ObjLookup lookup;
} Accessor;

typedef struct Iterator_ Iterator;

typedef struct Iterable_ {
    Iterator* (*init)(const void *o);
    int (*len)(const void *iter);
} Iterable;

struct Iterator_ {
    bool (*next)(Iterator *iter, void *pval);
    bool (*nextpair)(Iterator *iter, void *pkey, void *pval); // optional
    int len;  // may be -1 meaning 'unknown'
};

int interface_typeof_(const char *tname);
void interface_add(int itype, int type, void *funs);
void* interface_get(int itype, const void *obj);
ObjLookup interface_get_lookup(const void *P);
Iterator* interface_get_iterator(const void *P);

#endif
