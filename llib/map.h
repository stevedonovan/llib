/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_MAP_H
#define _LLIB_MAP_H

#include "list.h"

typedef struct MapEntry_ {
    struct MapEntry_ *_left, *_right;
    void *key;
    void *data;
} MapEntry, *PEntry;

// we'll reuse the same structure as a List!
#ifndef LLIB_MAP_TYPEDEF
typedef List Map;
#endif

#define map_size list_size

typedef struct MapIter_{
    void **pkey;
    void **pvalue;
    void *key;
    void *value;
    Map *map;
    PEntry node;
    List *vstack;
} MapIterStruct, *MapIter;

#define map_geti(m,s) ((intptr)map_get(m,(void*)(s)))
#define map_puti(m,k,v) map_put(m,(void*)(k),(void*)(v))

#define map_put_structs(m,...) obj_apply_varargs(m,(PFun)map_put_struct,__VA_ARGS__,NULL)

Map *map_new_node(bool strkey);
Map *map_new_str_ptr();
Map *map_new_str_ref();
Map *map_new_str_str();
Map *map_new_ptr_ptr();
Map *map_new_ptr_ref();
Map *map_new_ptr_str();

bool map_object (void *obj);

PEntry map_first(Map *m);
void *map_value_data (Map *m, PEntry item);
void map_free_item(Map *m, PEntry item);
PEntry map_put_struct(Map *m, void *data);
PEntry map_put(Map *m, void* key, void *data);
void map_put_keyvalues(Map *m, MapKeyValue *mkv);
void *map_get(Map *m, void *key);
bool map_contains(Map *m, void *key);
PEntry map_remove(Map *m, void *key) ;
bool map_delete(Map *m, void *key);

typedef void (*MapCallback)(void *,PEntry);

void map_visit(void *data, PEntry node, MapCallback fun, int order);

MapIter map_iter_new (Map *m, void *pkey, void *pvalue);
MapIter map_iter_next (MapIter iter);

#define FOR_MAP(iter,map) for (MapIter iter = map_iter_new(map,NULL,NULL);\
iter; iter = map_iter_next(iter))

#define FOR_MAP_KEYVALUE(k,v,map) for (MapIter iter_ = map_iter_new(map,&k,&v);\
iter_; iter_ = map_iter_next(iter_))


MapKeyValue *map_to_array(Map *m);

#endif
