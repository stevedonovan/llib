/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
Support for refcounted objects.

These are (currently) allocated with `malloc` and freed with `free`, but in addition always
carry a refcount (see `obj_refcount`).  `obj_ref` increments this count, and `obj_unref`
decrements this count; when the count becomes zero, the object is freed.  If the object
has an associated dispose function specified in `obj_new`, then this will be called first.

Unless `OBJ_REF_ABBREV` is defined, these are also defined as `ref` and `unref`; there is
a bulk `dispose` for multiple objects.

_Arrays_ are refcounted _containers_;  if created with `array_new_ref`, they will unref
their elements when disposed.  They carry their own size, accessible with `array_len`.

_Sequences_ are a wrapper around arrays, and are resizeable. `seq_add` appends new values
to a sequence. A sequence `s` has the type `T**`; it is a pointer to an array of T.  The underlying
array can always be accessed with `*s`.

@module obj
*/

#include "obj.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#define MAX_PTRS 10000

#define out(x) fprintf(stderr,"%s = %x\n",#x,(intptr)x);

// Generally one can't depend on malloc or other allocators returning pointers
// within a given range. So we keep an array of 'our' pointers, which we know
// for a fact were allocated using `new_obj`
static int kount = 0;
static int max_p = 0;
static void *our_ptrs[MAX_PTRS];

static int our_ptr_idx(void *p) {
    FOR(i,max_p) {
        if (our_ptrs[i] == p)
            return i;
    }
    return -1;
}

static void add_our_ptr(void *p) {
    int idx = our_ptr_idx(p);
    if (idx == -1) {
        our_ptrs[max_p] = p;
        ++max_p;
        assert(max_p < MAX_PTRS);
    } else {
        our_ptrs[idx] = p;
    }
    ++kount;
}

static void remove_our_ptr(void *p) {
    int ptr_idx = our_ptr_idx(p);
    assert(ptr_idx != -1); // might not be one of ours!
    our_ptrs[ptr_idx] = NULL;
    --kount;
}

int obj_kount() { return kount; }

static ObjHeader *new_obj(int size) {
    void *obj = malloc(size + sizeof(ObjHeader));
    add_our_ptr(obj);
    return (ObjHeader *)obj;
}

/// refcount of an object.
// Will return -1 if it isn't one of ours!
int obj_refcount (void *p)
{
    if (p == NULL) return -1;
    ObjHeader *pr = obj_header_(p);
    if (our_ptr_idx(pr) != -1) {
        return pr->_ref;
    } else {
        return -1;
    }
}

#define PTR_FROM_HEADER(h) ((void*)(h+1))
#define HEADER_FROM_PTR(P) ((ObjHeader*)P-1)

/// allocate a new refcounted object.
// @tparam type T
// @tparam DisposeFn optional destructior
// @treturn T*
// @function obj_new

void *obj_new_(int size, DisposeFn dtor) {
    ObjHeader *h = new_obj(size);
    h->x.dtor = dtor;
    h->mlen = size;
    h->_ref = 1;
    h->is_array = 0;
    h->is_ref_container = 0;
    return PTR_FROM_HEADER(h);
}

DisposeFn obj_set_dispose_fun(const void *P,DisposeFn dtor) {
    ObjHeader *h = obj_header_(P);
    DisposeFn old = h->x.dtor;
    h->x.dtor = dtor;
    return old;
}

// Ref counted objects are either arrays or structs.
// If they are arrays of refcounted objects, then
// this has to unref those objects. For structs,
// there may be an explicit destructor.

void obj_free_(const void *P) {
    ObjHeader *pr = obj_header_(P);
    if (pr->is_array) {
        if (pr->is_ref_container) {
            void **arr = (void**)P;
            for (int i = 0,n = pr->x.len; i < n; i++) {
                obj_unref(arr[i]);
            }
        }
    } else {
        DisposeFn dtor = pr->x.dtor;
        if (dtor)
            dtor((void*)P);
    }
    free(pr);
    remove_our_ptr(pr);
}

// Reference counting

/// increase reference count (`ref`).
// @tparam T object
// @treturn T
// @function obj_ref

void obj_incr_(const void *P) {
    ObjHeader *pr = obj_header_(P);
    ++(pr->_ref);
}

/// decrease reference count (`unref`).
// When this goes to zero, free the object and call the
// dispose function, if any.
void obj_unref(const void *P) {
    if (P == NULL) return;
    ObjHeader *pr = obj_header_(P);
    --(pr->_ref);
    if (pr->_ref == 0)
        obj_free_(P);
}

void obj_apply_v_varargs(void *o, PFun fn,va_list ap) {
    void *P;
    while ((P = va_arg(ap,void*)) != NULL)  {
        if (o == NULL)
            fn(P);
        else
            fn(o,P);
    }
}

/// decrease ref count for multiple values (`dispose`).
// @param ... objects
// @function obj_unref_v

/// apply a function to all arguments.
// If `o` is not `NULL`, then call `fn(o,arg)`, otherwise
// call `fn(arg)`.
// @param o optional object
// @param fn
// @param ... extra arguments, ending with `NULL`.
void obj_apply_varargs(void *o, PFun fn,...) {
    va_list ap;
    va_start(ap,fn);
    obj_apply_v_varargs(o,fn,ap);
    va_end(ap);
}

typedef unsigned char byte;

//? allocates len+1 - ok?

/// create an array.
// @tparam type T
// @tparam int size
// @function new_array

/// create an array of refcounted objects.
// @tparam type T
// @tparam int size
// @function new_array_ref

void *array_new_(int mlen, int len, int isref) {
    ObjHeader *h = new_obj(mlen*(len+1));
    byte *P;
    h->x.len = len;
    h->mlen = mlen;
    h->_ref = 1;
    h->is_array = 1;
    h->is_ref_container = isref;
    P = (byte*)PTR_FROM_HEADER(h);
    memset(P+mlen*len,0,mlen);
    return P;
}

/// new array from type.
// @tparam T type name of type
// @int sz size of array
// @treturn T*
// @function array_new

/// new array of refcounted objects.
// @tparam T type name of type
// @int sz size of array
// @treturn T*
// @function array_new_ref

/// new array from buffer.
// @tparam T type name of type
// @tparam T* buff
// @int sz size of array
// @function array_new_copy

/// length of an array.
// tparam type* array
// @function array_len

void *array_new_copy_ (int mlen, int len, int isref, void *P) {
    void *arr = array_new_(mlen,len,isref);
    memcpy(arr,P,mlen*len);
    return arr;
}


/// get a slice of an array.
// @param P the array
// @param i1 lower bound
// @param i2 upper bound (can be -1 for the rest)
void * array_copy(void *P, int i1, int i2) {
    ObjHeader *pr = obj_header_(P);
    int mlem = pr->mlen;
    int offs = i1*mlem;
    int len = i2 == -1 ? pr->x.len : i2-i1;
    void *newp = array_new_(mlem,len,pr->is_ref_container);
    memcpy(newp,(char*)P+offs,mlem*len);
    return newp;
}

/// resize an array.
// @param P the array
// @param newsz the new size
void * array_resize(void *P, int newsz) {
    ObjHeader *pr = obj_header_(P);
    void *newp = array_new_(pr->mlen,newsz,pr->is_ref_container);
    memcpy(newp,P,pr->mlen*pr->x.len);
    // if old ref array is going to die, make sure it doesn't dispose our elements
    pr->is_ref_container = 0;
    obj_unref(P);
    return newp;
}

void * obj_copy(void *P) {
    ObjHeader *pr = obj_header_(P);
    void *newp = obj_new_(pr->mlen,pr->x.dtor);
    memcpy(newp,P,pr->mlen);
    return newp;
}

// Arrays of char can be directly used as C strings,
// since they always have a trailing zero byte.

/// create a refcounted string copy.
char *str_new(const char *s) {
    int sz = strlen(s);
    char *ns = str_new_size(sz);
    memcpy(ns,s,sz);
    return ns;
}

/// create a refcounted string of given size
char *str_new_size(int sz) {
    return array_new(char,sz);
}

/// increase the refcount of a string.
char *str_ref(char *s) {
    int rc = obj_refcount(s);
    if (rc == -1)
        return str_new(s);
    else
        return (char*)obj_ref(s);
}

/// make a refcounted string from an arbitrary string.
char *str_cpy(char *s) {
    int rc = obj_refcount(s);
    //printf("'%s' %p %d\n",s,s,rc);
    if (rc == -1) {
        return str_new(s);
    } else {
        return s;
    }
}

/// sort an array.
// @tparam T* P the array
// @int kind  either `ARRAY_INT` or `ARRAY_STR`
// @int ofs offset into each item
// @function array_sort

/// sort an array of structs by integer/pointer field
// @tparam T* A the array
// @tparam T type the struct
// @param field the name of the struct field
// @function array_sort_struct_ptr

/// sort an array of structs by string field
// @tparam T* A the array
// @tparam T type the struct
// @param field the name of the struct field
// @function array_sort_struct_str

/// get from a source and put to a destination.
// @param dest destination object
// @param setter function of dest and the gotten object
// @param src source object
// @param getter function of `src` to call repeatedly until it returns `NULL`
// @function obj_apply
void obj_apply_ (void *dest, PFun setter, void *src, PFun getter)
{
    void *p;
    while ((p = getter(src)) != NULL) {
        setter(dest,p);
    }
}

// sequences

const int INITIAL_CAP = 4, GROW_CAP = 2;

void seq_dispose(Seq *s)
{
    obj_unref(s->arr);
}

/// create a plain sequence of a type
// @param T type
// @function seq_new

/// create a sequence of a refcounted type
// @param T type
// @function seq_new_ref

void *seq_new_(int nlem, int isref) {
    Seq *s = obj_new(Seq,seq_dispose);
    s->arr = array_new_(nlem,INITIAL_CAP,isref);
    s->cap = INITIAL_CAP;// our capacity
    array_len(s->arr) = 0;// our size
    return (void*)s;
}

void seq_resize(Seq *s, int nsz) {
    // this function does not shrink!
    if (nsz <= s->cap)
        return;
    // best size is next power of two
    int result = 1;
    while (result < nsz) result <<= 1;
    nsz = result;
    s->arr = array_resize(s->arr, nsz);
    s->cap = nsz;
}

/// add an arbitrary value to a sequence
// @param s the sequence
// @param v the value
// @function seq_add

int seq_next_(void *sp) {
    Seq *s = (Seq *)sp;
    void *a = s->arr;
    int len = array_len(a);
    if (len == s->cap) {
        s->cap *= GROW_CAP;
        s->arr = array_resize(s->arr, s->cap);
    }
    // the idea is that the seq's array has _always_ got the correct length
    array_len(s->arr) = len + 1;
    return len;
}

/// add a pointer value to a sequence.
void seq_add_ptr(void *sp, void *p)  {
    int idx = seq_next_(sp);
    void **a = (void **)((Seq*)sp)->arr;
    a[idx] = p;
}

void seq_add_str(void *sp, const char *p) {
    int idx = seq_next_(sp);
    char **a = (char **)((Seq*)sp)->arr;
    a[idx] = (char*)str_cpy((char*)p);
}

/// get the array from a sequence.
// (This will transfer ownership and unref the sequence.)
// @param sp the sequence
void *seq_array_ref(void *sp) {
    Seq *s = (Seq *)sp;
//    /*
    int len = array_len(s->arr);
    if (len < s->cap) {
        s->arr = array_resize(s->arr, len);
    }
  //  */
    obj_incr_(s->arr);
    obj_unref(sp);
    return s->arr;
}

/// standard for-loop.
// @param i loop variable.
// @param n loop count.
// @macro FOR

/// for-loop over array.
// @tparam type T base type
// @tparam type* loop pointer
// @tparam type* array
// @macro FOR_ARR
// @usage FOR(int,pi,arr) printf("%d ",*pi);
