/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef __LLIB_TEMPLATE_H
#define __LLIB_TEMPLATE_H
#include "obj.h"
#include "value.h"

struct StrTempl_;
typedef struct StrTempl_ *StrTempl;
typedef char *(*StrLookup) (void *obj, char *key);

StrTempl str_templ_new(const char *templ, const char *markers);
char *str_templ_subst_using(StrTempl stl, StrLookup lookup, void *data);
char *str_templ_subst(StrTempl stl, char **substs);
char *str_templ_subst_values(StrTempl st, PValue v);

#define str_templ_subst_map(stl,m) str_templ_subst_using(stl,(StrLookup)map_get,m)

#endif

