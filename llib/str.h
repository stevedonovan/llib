/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef __LLIB_STR_H
#define __LLIB_STR_H
#include "obj.h"
#include <string.h>
#include <stdarg.h>

// easier to type ;)
typedef const char * str_t;

#ifndef STRLOOKUP_DEFINED
typedef char *(*StrLookup) (void *obj, const char *key);
#define STRLOOKUP_DEFINED
#endif

#define str_eq(s1,s2) (strcmp((s1),(s2))==0)

typedef char **SMap;

char*** smap_new(bool ref);
void smap_add(char*** smap, str_t name, const void *data);
void smap_put(char*** smap, str_t name, const void *data);
void *smap_get(char*** smap, str_t name);
int smap_len(char*** smap);
char** smap_close(char*** smap);
#define smap_add_ref(sm,name,data) smap_add(sm,str_ref(name),str_ref(data))
#define smap_put_ref(sm,name,data) smap_put(sm,str_ref(name),str_ref(data))

#define FOR_SMAP(k,v,sm) for(char **p_=(sm), *k=*p_,*v=*(p_+1); \
 (v=*(p_+1),k=*p_); p_+=2)

char *str_vfmt(str_t fmt,va_list ap);
char *str_fmt(str_t fmt,...);
int str_findstr(str_t s, str_t sub);
int str_findch(str_t s, char ch);
str_t str_end(str_t s);
bool str_starts_with(str_t s, str_t prefix);
bool str_ends_with(str_t s, str_t postfix);
int str_find_first_of(str_t s, str_t ps);
int str_find_first_not_of(str_t s, str_t ps);
bool str_is_blank(str_t s);
void str_trim(char *s);
char* str_sub(str_t s, int i1, int i2);
char ** str_split(str_t s, str_t delim);
char *str_concat(char **ss, str_t delim);
char **str_strings(char *p,...);

void **str_lookup_ptr(char** substs, str_t name);
void *str_lookup(SMap substs, str_t name);
char *str_gets(SMap substs, str_t name);

char **strbuf_new(void);
#define strbuf_add seq_add
void strbuf_adds(char **sp, str_t ss);
void strbuf_addf(char **sp, str_t fmt, ...);
void strbuf_addr(char **sp, str_t s, int i1, int i2);
char *strbuf_insert_at(char **sp, int pos, str_t src, int sz);
char *strbuf_erase(char **sp, int pos, int len);
char *strbuf_replace(char **sp, int pos, int len, str_t s);
#define strbuf_tostring(sp) (char*)seq_array_ref(sp)

#endif
