/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include "obj.h"

// this needed for qsort_r
#define __USE_GNU
#include <stdlib.h>
#include <string.h>


// sorting and searching arrays //////////
// We use qsort_r for maximum flexibily, but unfortunately nobody can agree on the signatures involved
// http://stackoverflow.com/questions/4300896/how-portable-is-the-re-entrant-qsort-r-function-compared-to-qsort
// Fortunately, we only have to do this nonsense once.

#define DEREF(P,offs)  *((intptr_t*)(P+offs))

#ifndef __linux__
typedef int (*CMPFN)( void *context, const void *m1, const void *m2);

#ifdef _WIN32
#ifndef _MSC_VER
// mingw just doesn't have this prototype...
void qsort_s(void *base, size_t num, size_t width, CMPFN compar, void *context);
#endif
#define qsort_x(base,nel,width,thunk,compar) qsort_s(base,nel,width,compar,thunk)
#else
#define qsort_x qsort_r
#endif

static int plain_cmp(intptr_t offs, const char *m1, const char  *m2) {
    return DEREF(m1,offs) - DEREF(m2,offs);
}

static int string_cmp(intptr_t offs, const char **m1, const char **m2) {
    return strcmp(*(m1+offs),*(m2+offs));
}

static int plain_cmp_desc(intptr_t offs, const char *m1, const char  *m2) {
    return DEREF(m2,offs) - DEREF(m1,offs);
}

static int string_cmp_desc(intptr_t offs, const char **m1, const char **m2) {
    return strcmp(*(m2+offs),*(m1+offs));
}
#else
// GNU just gets it wrong here, seeing that BSD invented qsort_r
typedef int (*CMPFN)(const void *m1, const void *m2, void *context);
#define qsort_x(base,nel,width,thunk,compar) qsort_r(base,nel,width,compar,thunk)

static int plain_cmp(const char *m1, const char  *m2, intptr_t offs) {
    return DEREF(m1,offs) - DEREF(m2,offs);
}

static int string_cmp(const char **m1, const char **m2, intptr_t offs) {
    return strcmp(*(m1+offs),*(m2+offs));
}

static int plain_cmp_desc(const char *m1, const char  *m2, intptr_t offs) {
    return DEREF(m2,offs) - DEREF(m1,offs);
}

static int string_cmp_desc(const char **m1, const char **m2, intptr_t offs) {
    return strcmp(*(m2+offs),*(m1+offs));
}

#endif

static CMPFN cmpfn(ElemKind kind, bool desc) {
    if (desc)
        return kind==ARRAY_INT ? (CMPFN)plain_cmp_desc : (CMPFN)string_cmp_desc;
    else
        return kind==ARRAY_INT ? (CMPFN)plain_cmp : (CMPFN)string_cmp;
}

void array_sort(void *P, ElemKind kind, bool desc, int ofs) {
    int len = array_len(P), nelem = obj_elem_size(P);
    qsort_x(P,len,nelem,(void*)ofs,cmpfn(kind,desc));
}

