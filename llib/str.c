/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

////  ### String Manipulation.
//
// When building up strings use a _strbuf_, which is a sequence of chars; can add characters
// and strings, formatted strings (like `str_fmt`) and also insert and remove substrings.
// The actual string is always `*S` but use `strbuf_tostring` to properly clean up and
// dispose of the sequence.
//
// `str_split` creates a refcounted array of strings, splitting using a delimiter. `str_concat`
// works the other way, joining an array of strings with a separator.
//
// There are searching operations which return a boolean or integer index,
// loosely based on C++'s `std::string` methods.
//
// _smaps_ ('simple maps') are arrays of strings where the odd indices
// are keys and the even indices are values. `str_lookup` does a
// linear search, which is simple and sufficient for small maps. A convenient
// way to build these maps is to start with `smap_new` and use either `smap_add`
// to simply add, or `smap_put` to update/add, key/value pairs.
//
// See `test-str.c`
// @module str

// for strtok_r
#define _BSD_SOURCE

#include <stdio.h>
#include <string.h>
#include "str.h"

#define BUFSZ 256

/// String buffers.
// @section buffers

/// new string buffer.
// Currently, string buffers are just sequences of chars.
char **strbuf_new(void) {
    return seq_new(char);
}

/// convert a string buffer to a string.
// @tparam char** sp
// @treturn char*
// @function strbuf_tostring

/// append a character to a string buffer.
// @tparam char** sp
// @tparam char ch
// @function strbuf_add

static void sb_adds(char **sp, str_t ss, int lss) {
    Seq *s = (Seq *)sp;
    int la = array_len(s->arr);
    int lass = la + lss;
    if (lass > s->cap) {
        seq_resize(s,lass);
    }
    memcpy((char*)s->arr + la, ss, lss);
    array_len(s->arr) = lass;
    ((char*)s->arr)[lass] = '\0';
}

/// append a string to a string buffer.
void strbuf_adds(char **sp, str_t ss) {
    sb_adds(sp,ss,strlen(ss));
}

/// append formatted results to a string buffer.
// Note that the result must be less than `BUFSZ` (default 256)
void strbuf_addf(char **sp, str_t fmt, ...) {
    va_list ap;
    char buff[BUFSZ];
    va_start(ap, fmt);
    (void)vsnprintf(buff, BUFSZ, fmt, ap);
    va_end(ap);
    strbuf_adds(sp,buff);
}

/// append a range of chars from a string.
void strbuf_addr(char **sp, str_t s, int i1, int i2) {
    sb_adds(sp,s+i1,i2-i1);
}

/// append a string surronded by spaces.
// useful when building up a string of tokens separated by spaces.
void strbuf_addsp(char **ss, str_t s) {
    if (s && *s) {
        strbuf_add(ss,' ');
        strbuf_adds(ss,s);
        strbuf_add(ss,' ');
    }
} 

/// insert a string into a string buffer at `pos`.
// May specify the number of characters to be copied `sz`; if this is -1
// then use the ordinary length of the string.
char *strbuf_insert_at(char **sp, int pos, str_t src, int sz) {
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
char *strbuf_replace(char **sp, int pos, int len, str_t s) {
    strbuf_erase(sp,pos,len);
    return strbuf_insert_at(sp,pos,s,-1);
}

static int offset_str(str_t P, str_t Q) {
    if (P == NULL)
        return -1;
    return (intptr_t)P - (intptr_t)Q;
}

/// Extra string functions.
// @section strings

// Keep MSVC happy, if necessary (defined only in 2013)
#ifndef va_copy
#define va_copy(dest, src) (dest = src)
#endif

/// safe verson of `vsprintf` which returns a refcounted string.
char *str_vfmt(str_t fmt,va_list ap) {
    int size;
    char *str;

    // needed size of buffer...
    // In GENERAL vsnprintf modifies its va_list!
    va_list aq;
    va_copy(aq, ap);
    size = vsnprintf(NULL, 0, fmt, aq);
    va_end(aq);
    str = str_new_size(size);
    size = vsnprintf(str, size+1, fmt, ap);
    return str;
}

/// safe verson of `sprintf` which returns a refcounted string.
char *str_fmt(str_t fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    char *res = str_vfmt(fmt,ap);
    va_end(ap);
    return res;    
}

/// extract a substring.
// Note that `i2` and 'i1' are indices which may be negative, so (0,-1)
// is a string copy and (1,-2) copies from 2nd to second-last character.
// @usage str_sub("hello",2,3) -> "l"
// @usage str_sub("hello",2,-1) -> "llo"
char* str_sub(str_t s, int i1, int i2) {
     int sz = strlen(s);
    if (i2 < 0)
        i2 = sz + i2 + 1;
    if (i1 < 0)
        i1  = sz + i1 + 1;
    if (i1 > sz)
        i1 = sz-1;
    if (i2 > sz)
        i2 = sz-1;
    int len = i2 - i1;
    char *res = str_new_size(len);
    memcpy(res,s+i1,len);
    return res;
}

static str_t whitespace = " \t\r\n";

/// trim a string in-place
void str_trim(char *s) {
    int sz = strspn(s,whitespace);
    if (sz > 0)
        memmove(s,s+sz,strlen(s)-sz+1);
    char *p = s + strlen(s) - 1;
    while (*p && strchr(whitespace,*p) != NULL)
        --p;
    *(p+1) = 0;
}

/// Finding things in strings.
// @section finding

/// get pointer to last char of string.
str_t str_end(str_t s) {
    int len = strlen(s);
    if (len == 0)
        return s;
    else
        return s + len - 1;
}

/// are these strings equal?
// @tparam str_t a
// @tparam str_t b
// @function str_eq

/// does the string start with this prefix?
bool str_starts_with(str_t s, str_t prefix) {
    return strncmp(s,prefix,strlen(prefix)) == 0;
}

/// does the string end with this postfix?
bool str_ends_with(str_t s, str_t postfix) {
    str_t se = str_end(s);
    str_t pe = str_end(postfix);
    while (se != s && pe != postfix) {
        if (*se != *pe)
            return false;
        --se;
        --pe;
    }
    return pe == postfix;
}

/// does a string only consist of blank characters?
bool str_is_blank(str_t s) {
    return strspn(s,whitespace) == strlen(s);
}

/// find substring `sub` in the string.
int str_findstr(str_t s, str_t sub) {
    str_t P = strstr(s,sub);
    return offset_str(P,s);
}

/// contains substring?
// also optionally return index past match.
bool str_contains(str_t s, str_t sub, int *after) {
    int idx = str_findstr(s,sub);
    if (idx == -1) {
        return false;
    }
    if (after) {
        *after = idx + strlen(sub);
    }
    return true;
}

/// find character `ch` in the string.
int str_findch(str_t s, char ch) {
    str_t P = strchr(s,ch);
    return offset_str(P,s);
}

/// find first character that is in the string `ps`
int str_find_first_of(str_t s, str_t ps) {
    return offset_str(strpbrk(s,ps),s);
}

/// find first character that is _not_ in the string `ps`
int str_find_first_not_of(str_t s, str_t ps) {
    int sz = strspn(s,ps);
    if (sz == 0)
        return -1;
    else
        return sz;
}

/// Splitting and Concatention
// @section concat

#ifdef _WIN32
#ifdef _MSC_VER
#define strtok_r strtok_s
#else
// see http://stackoverflow.com/questions/12975022/strtok-r-for-mingw
static char* strtok_r(char *str,str_t delim,char **nextp) {
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

/// split a string upto `nsplit` times using delimiters.
// Returns a ref array of ref strings.
char ** str_split_n(str_t s, str_t delim, int nsplit) {
    char ***ss = seq_new_ref(char*);
    // make our own copy of the string...
    char *sc = str_new(s), *saveptr;
    char *t = strtok_r(sc,delim,&saveptr);
    int i = 1;
    while (t != NULL) {
        seq_add(ss,str_new(t));
        ++i;
        if (nsplit && i > nsplit) {
        	char *rest = t+strlen(t)+1;
        	while (strchr(delim,*rest))
        		++rest;
        	seq_add(ss,str_new(rest));
        	break;
        } else {
        	t = strtok_r(NULL,delim,&saveptr);
        }
    }
    char **res = (char**)seq_array_ref(ss);
    res[array_len(res)] = NULL;
    obj_unref(sc);
    return res;
}

/// split a string using delimiters.
// Returns a ref array of ref strings.
char ** str_split(str_t s, str_t delim) {
	return str_split_n(s,delim,0);
}

/// concatenate an array of strings.
// Assumes that the array is refcounted
char *str_concat(char **ss, str_t delim) {
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

/// Allocating a simple array of strings.
// End the string arguments with a final `NULL`.
// Note that this array is _not_ a ref container; the
// string addresses are just copied over.
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

/// index of `s` in string array `strings`.
// -1 if not found. The string array must be NULL terminated.
int str_index(const char **strings, const char *s) {
    int i = 0;
    for (str_t *S = strings; *S; S++,i++) {
        if (str_eq(*S,s)) {
            return i;
        }
    }
    return -1;
}

/// 1-based index of `s` in an arbitrary number of string arguments.
// If we don't match, then return 0!  `str_eq_any` is a convenient
// replacement for a chain of string comparions. Also useful to parse enum values.
// @string s  the string to match
// @param ... the strings to match against
// @treturn int
// @function str_eq_any
// @usage  str_eq_any("foo","baz","foo","bar") == 2;
int str_eq_any_ (const char *s, ...) {
    va_list ap;
    va_start(ap,s);
    str_t S;
    int res = 0, i = 1;
    while ((S = va_arg(ap,str_t)) != NULL) {
        if (str_eq(S,s)) {
            res = i;
            break;
        }
        ++i;
    }
    va_end(ap);
    return res;
}
    
