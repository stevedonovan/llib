/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef __LLIB_STR_H
#define __LLIB_STR_H
#include "obj.h"

char *str_fmt(const char *fmt,...);
int str_findstr(const char *s, const char *sub);
int str_findch(const char *s, char ch);
int str_find_first_of(const char *s, const char *ps);
int str_find_first_not_of(const char *s, const char *ps);
char ** str_split(const char *s, const char *delim);
char *str_concat(char **ss, const char *delim);
char **str_strings(char *p,...);

char **strbuf_new(void);
void strbuf_adds(char **sp, const char *ss);
void strbuf_addf(char **sp, const char *fmt, ...);
char *strbuf_insert_at(char **sp, int pos, const char *src, int sz);
char *strbuf_erase(char **sp, int pos, int len);
char *strbuf_replace(char **sp, int pos, int len, const char *s);

struct StrTempl_;
typedef struct StrTempl_ *StrTempl;
typedef char *(*StrLookup) (void *obj, char *key);

StrTempl str_templ_new(const char *templ);
char *str_templ_subst_using(StrTempl stl, StrLookup lookup, void *data);
char *str_templ_subst(StrTempl stl, char **substs) ;
#endif
