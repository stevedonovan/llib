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

/// new string buffer
char **strbuf_new(void) {
    return seq_new(char);
}

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

    va_start(ap, fmt);
    size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    str = str_new_size(size);

    va_start(ap, fmt);
    size = vsnprintf(str, size, fmt, ap);
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
char ** str_split(const char *s, const char *delim) {
    char ***ss = seq_new(char*);
    char *sc = str_new(s), *saveptr;
    char *t = strtok_r(sc,delim,&saveptr);
    while (t != NULL) {
        seq_add(ss,t);
        t = strtok_r(NULL,delim,&saveptr);
    }
    char **res = (char**)seq_array_ref(ss);
//    obj_unref(sc);
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
    obj_unref(sizes);
    return res;
}

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
    res = (char**)array_new_(sizeof(char*),n,false);
    res[0] = first;
    n = 1;
    va_start(ap,first);
    while ((S = va_arg(ap,char*)) != NULL)  {
        res[n++] = S;
    }
    va_end(ap);
    return res;
}

/// template substtitutions
// @type template

struct StrTempl_ {
    char *str;
    char **parts;
};

static void StrTempl_dispose (StrTempl stl) {
    obj_unref_v(stl->str, stl->parts);
}

#define MARKER '\01'

#include <assert.h>

typedef char *Str;

/// new template from string with `$(var)` variables
StrTempl str_templ_new(const char *templ) {
    StrTempl stl = obj_new(struct StrTempl_,StrTempl_dispose);
    // we make our own copy of the string
    // and will break it up into this sequence
    Str **ss = seq_new(Str);
    stl->str = str_new(templ);
    char *T = stl->str;
    while (true) {
        int endp, dollar = str_findch(T,'$');
        if (dollar > -1)
            T[dollar] = '\0';
        //  plain string chunk
        seq_add(ss, T);
        // $(key) expansion
        if (dollar == -1)
            break;
        T += dollar + 1;
        assert(*T == '(');
        *T = MARKER;
        endp = str_findch(T,')');
        T[endp] = '\0';
        seq_add(ss,T);
        T += endp + 1;
        if (! *T) break;
    }
    stl->parts = (Str*)seq_array_ref(ss);
    return stl;
}

/// substitute variables in template using a lookup function.
char *str_templ_subst_using(StrTempl stl, StrLookup lookup, void *data) {
    int n = array_len(stl->parts);
    Str *out = array_new(Str,n);

    FOR (i, n) { // over all the parts of the template
        char *part = stl->parts[i];
        if (*part == MARKER) {
            part = lookup (data, part+1);
            if (! part)
                part = "";
        }
        out[i] = part;
    }

    Str res = str_concat(out,NULL);
    obj_unref(out);
    return res;
}

static char *pair_lookup(char **substs, char *name) {
    for (char **S = substs;  *S; S += 2) {
        if (strcmp(*S,name)==0)
            return *(S+1);
    }
    return NULL;
}

/// substitute the variables using an array of keys and pairs.
char *str_templ_subst(StrTempl stl, char **substs) {
    return str_templ_subst_using(stl,(StrLookup)pair_lookup,substs);
}
