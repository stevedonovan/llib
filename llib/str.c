/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

////  string manipulation.
// @module str

// for strtok_r
#define _BSD_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "str.h"

#define BUFSZ 256

/// String buffers.
// @section buffers

/// new string buffer.
// CUrrently, string buffers are just sequences of chars.
char **strbuf_new(void) {
    return seq_new(char);
}

/// append a character to a string buffer.
// @tparam char** sp
// @tparam char ch
// @function strbuf_add

/// append a string to a string buffer.
void strbuf_adds(char **sp, const char *ss) {
    Seq *s = (Seq *)sp;
    int la = array_len(s->arr) , lss = strlen(ss);
    int lass = la + lss;
    if (lass > s->cap) {
        seq_resize(s,lass);
    }
    memcpy((char*)s->arr + la, ss, lss+1);
    array_len(s->arr) = lass;
}

/// append formatted results to a string buffer.
void strbuf_addf(char **sp, const char *fmt, ...) {
    va_list ap;
    char buff[BUFSZ];
    va_start(ap, fmt);
    (void)vsnprintf(buff, BUFSZ, fmt, ap);
    va_end(ap);
    strbuf_adds(sp,buff);
}

/// insert a string into a string buffer at `pos`.
// May specify the number of characters to be copied `sz`; if this is -1
// then use the ordinary length of the string.
char *strbuf_insert_at(char **sp, int pos, const char *src, int sz) {
    Seq *s = (Seq *)sp;
    int on = array_len(s->arr), len = sz==-1 ? strlen(src) : sz;
    // make some room!
    seq_resize(s, on + sz);
    char *P = (char*)s->arr + pos; // insertion point
    // move rest of string up
    memmove(P+len,P,on-pos+1);
    // and copy!
    memmove(P,src,len);
    return P;
}

/// erase `len` chars from `pos`.
char *strbuf_erase(char **sp, int pos, int len) {
    Seq *s = (Seq *)sp;
    char *P = (char*)s->arr;
    int on = array_len(P);
    P += pos;
    memmove(P,P+len,on-pos+1);
    return (char*)s->arr;
}

/// replace `len` chars from `pos` with the string `s`.
char *strbuf_replace(char **sp, int pos, int len, const char *s) {
    strbuf_erase(sp,pos,len);
    return strbuf_insert_at(sp,pos,s,-1);
}

static int offset_str(const char *P, const char *Q) {
    if (P == NULL)
        return -1;
    return (intptr)P - (intptr)Q;
}

/// Extra string functions.
// @section strings

/// safe verson of `sprintf` which returns an allocated string.
char *str_fmt(const char *fmt,...) {
    va_list ap;
    int size;
    char *str;

    // needed size of buffer...
    va_start(ap, fmt);
    size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    str = str_new_size(size);

    va_start(ap, fmt);
    size = vsnprintf(str, size+1, fmt, ap);
    va_end(ap);
    return str;
}

/// find substring `sub` in the string.
int str_findstr(const char *s, const char *sub) {
    const char *P = strstr(s,sub);
    return offset_str(P,s);
}

/// find character `ch` in the string.
int str_findch(const char *s, char ch) {
    const char *P = strchr(s,ch);
    return offset_str(P,s);
}

/// does the string start with this prefix?
bool str_starts_with(const char *s, const char *prefix) {
    return strncmp(s,prefix,strlen(prefix)) == 0;
}

/// does the string end with this postfix?
bool str_ends_with(const char *s, const char *postfix) {
    const char *P = strstr(s,postfix);
    if (! P)
        return false;
    return str_eq(P,postfix);
}

/// find first character that is in the string `ps`
int str_find_first_of(const char *s, const char *ps) {
    return offset_str(strpbrk(s,ps),s);
}

/// find first character that is _not_ in the string `ps`
int str_find_first_not_of(const char *s, const char *ps) {
    int sz = strspn(s,ps);
    if (sz == 0)
        return -1;
    else
        return sz;
}

static const char *whitespace = " \t\r\n";

/// trim a string in-inplace
void str_trim(char *s) {
    int sz = strspn(s,whitespace);
    if (sz > 0)
        memmove(s,s+sz,strlen(s)-sz+1);
    char *p = s + strlen(s) - 1;
    while (*p && strchr(whitespace,*p) != NULL)
        --p;
    *(p+1) = 0;
}

/// does a string only consist of blank characters?
bool str_is_blank(const char *s) {
    return strspn(s,whitespace) == strlen(s);
}


// " one "
// sz = 1; srlen = 5. Move 5 chars, get "one "

#ifdef _WIN32
#ifdef _MSC_VER
#define strtok_r strtok_s
#else
// see http://stackoverflow.com/questions/12975022/strtok-r-for-mingw
static char* strtok_r(char *str,const char *delim,char **nextp) {
    char *ret;
    if (str == NULL)  {
        str = *nextp;
    }
    str += strspn(str, delim);
    if (*str == '\0') {
        return NULL;
    }
    ret = str;
    str += strcspn(str, delim);
    if (*str)   {
        *str++ = '\0';
    }
    *nextp = str;
    return ret;
}
#endif
#endif

/// split a string using a set of delimiters.
// Returns a ref array of ref strings.
char ** str_split(const char *s, const char *delim) {
    char ***ss = seq_new_ref(char*);
    // make our own copy of the string...
    char *sc = str_new(s), *saveptr;
    char *t = strtok_r(sc,delim,&saveptr);
    while (t != NULL) {
        seq_add(ss,str_new(t));
        t = strtok_r(NULL,delim,&saveptr);
    }
    char **res = (char**)seq_array_ref(ss);
    res[array_len(res)] = NULL;
    obj_unref(sc);
    return res;
}

/// concatenate an array of strings.
// Asummes that the array is refcounted
char *str_concat(char **ss, const char *delim) {
    int sz = 0, nm1, n = array_len(ss), nd = delim ? strlen(delim) : 0;
    char *res, *q;
    int **ssizes = seq_new(int);
    int *sizes;

    // our total size for allocation
    FOR (i,n) {
        int len = strlen(ss[i]);
        seq_add(ssizes,len);
        sz += len;
        if (delim)
            sz += nd;
    }

    res = str_new_size(sz);
    q = res;
    sizes = *ssizes;
    nm1 = n - 1;
    FOR (i,n) {
        memcpy(q,ss[i],sizes[i]);
        q += sizes[i];
        if (delim && i < nm1) {
            memcpy(q,delim,nd);
            q += nd;
        }
    }
    *q = '\0';
    obj_unref(ssizes);
    return res;
}

// useful function for making quick arrays of strings.
// Note that this is _not_ a refcounted array.
char **str_strings(char *first,...) {
    va_list ap;
    char *S;
    int n = 1;
    char **res;
    va_start(ap,first);
    while (va_arg(ap,char*) != NULL)  {
        ++n;
    }
    va_end(ap);
    res = array_new(char*,n);
    res[0] = first;
    n = 1;
    va_start(ap,first);
    while ((S = va_arg(ap,char*)) != NULL)  {
        res[n++] = S;
    }
    va_end(ap);
    return res;
}

