/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_OBJ_H
#define _LLIB_OBJ_H
typedef unsigned int uint32;
typedef void (*DisposeFn)(void*);
typedef void (*PtrFun)(void *);
typedef void *(*PFun)(void *,...);

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef struct ObjHeader_ {
    union {
      DisposeFn dtor;
      uint32 len;
    } x;
    unsigned int _ref:15;
    unsigned int is_array:1;
    unsigned int is_ref_container:1;
    unsigned int mlen:15;
} ObjHeader;

#if defined(_M_X64) || defined(__amd64__)
#define LLIB_64_BITS
typedef long long intptr;
#else
typedef int intptr;
#endif

#define obj_header_(P) ((ObjHeader*)(P)-1)
#define array_len(P) ( obj_header_(P)->x.len )
#define obj_is_array(P) ( obj_header_(P)->is_array )
#define obj_elem_size(P) ( obj_header_(P)->mlen )
#define obj_ref_array(P) ( obj_header_(P)->is_ref_container )

#define obj_new(T,dtor) (T*)obj_new_(sizeof(T),(DisposeFn)dtor)
#define obj_ref(P) (obj_incr_(P), P)
#define obj_unref_v(...) obj_apply_varargs(NULL,(PFun)obj_unref,__VA_ARGS__,NULL)
#define array_new(T,sz) (T*)array_new_(sizeof(T),sz,0)
#define array_new_ref(T,sz) (T*)array_new_(sizeof(T),sz,1)
#define array_new_copy(T,buff,sz) (T*)array_new_copy_(sizeof(T),sz,0,buff)

#define FOR(i,n) for (int i = 0, n_ = (n); i < n_; i++)
#define FOR_PTRZ(T,var,expr) for(T *var = expr; *var; var++)
#define FOR_ARR(T,p,arr) for(T *p=(arr),*e_=p+array_len(p); p!=e_; p++)

#define str_sub (char*)array_copy
#define str_len array_len

#ifndef OBJ_REF_ABBREV
#define ref obj_ref
#define unref obj_unref
#define dispose obj_unref_v
#endif

int obj_kount();
void *obj_new_(int size, DisposeFn dtor);
void obj_incr_(const void *P);
void obj_unref(const void *P);
DisposeFn obj_set_dispose_fun(const void *P,DisposeFn dtor);

void obj_apply_varargs(void *o, PFun fn,...);

#define obj_apply(dest,s,src,g) obj_apply_(dest,(PFun)s,src,(PFun)g)
void obj_apply_ (void *dest, PFun setter, void *src, PFun getter);
int obj_refcount (void *P);

void *array_new_(int mlen, int len, int ref);
void *array_new_copy_ (int mlen, int len, int ref, void *P);
void *array_copy(void *P, int i1, int i2);
void *array_resize(void *P, int newsz);
void *obj_copy(void *P);
char *str_new(const char *s);
char *str_new_size(int sz);
char *str_ref(char *s);
char *str_cpy(char *s);

typedef enum {
    ARRAY_INT = 0,
    ARRAY_STRING = 1
} ElemKind;

void array_sort(void *P, ElemKind kind, bool desc, int offs) ;

#define OBJ_STRUCT_OFFS(T,f) ( (intptr)(&((T*)0)->f) )

#define array_sort_struct_ptr(P,desc,T,fname) array_sort(P,ARRAY_INT,desc,OBJ_STRUCT_OFFS(T,fname))
#define array_sort_struct_str(P,desc,T,fname) array_sort(P,ARRAY_STRING,desc,OBJ_STRUCT_OFFS(T,fname))

// this may be safely cast to a pointer to the array
typedef struct Seq_ {
    void *arr;
    int cap;
} Seq;

#define seq_new(T) (T**)seq_new_(sizeof(T),0)
#define seq_new_ref(T) (T**)seq_new_(sizeof(T),1)
#define seq_new_str() (char***)seq_new_(sizeof(char*),1)
#define seq_add(s,v) {int idx_=seq_next_(s); (*(s)) [idx_] = (v);}
#define seq_add_items(s,...)   obj_apply_varargs(s,(PFun)seq_add_ptr,__VA_ARGS__,NULL)

void *seq_new_(int nlem, int ref);
int seq_next_(void *s);
void seq_add_ptr(void *sp, void *p);
void seq_add_str(void *sp, const char*p);
void seq_resize(Seq *s, int nsz);
void *seq_array_ref(void *sp);

#endif

