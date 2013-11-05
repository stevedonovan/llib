/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef __LLIB_STR_H
#define __LLIB_STR_H
#include "obj.h"
#include <string.h>

#define str_eq(s1,s2) (strcmp((s1),(s2))==0)

char *str_fmt(const char *fmt,...);
int str_findstr(const char *s, const char *sub);
int str_findch(const char *s, char ch);
bool str_starts_with(const char *s, const char *prefix);
bool str_ends_with(const char *s, const char *postfix);
int str_find_first_of(const char *s, const char *ps);
int str_find_first_not_of(const char *s, const char *ps);
char ** str_split(const char *s, const char *delim);
char *str_concat(char **ss, const char *delim);
char **str_strings(char *p,...);
char *str_lookup(char **substs, char *name);

char **strbuf_new(void);
#define strbuf_add seq_add
void strbuf_adds(char **sp, const char *ss);
void strbuf_addf(char **sp, const char *fmt, ...);
char *strbuf_insert_at(char **sp, int pos, const char *src, int sz);
char *strbuf_erase(char **sp, int pos, int len);
char *strbuf_replace(char **sp, int pos, int len, const char *s);

#endif
