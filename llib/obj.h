/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_OBJ_H
#define _LLIB_OBJ_H
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef long long int64;
typedef void (*DisposeFn)(void*);
typedef void (*PtrFun)(void*);
typedef void *(*PFun)(void *,...);
typedef void (*FreeFn)(void *,void *);
typedef void *(*AllocFn)(void*,int);

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef struct ObjAllocator_ {
    AllocFn alloc;
    FreeFn free;
    void *data;
} ObjAllocator;

typedef struct ObjHeader_ {
    unsigned int type:14;
    unsigned int is_array:1;
    unsigned int is_ref_container:1;
    unsigned int _ref:16;
    unsigned int _len:32;
} ObjHeader;

typedef struct ObjType_ {
    const char *name;
    DisposeFn dtor;
    ObjAllocator *alloc;
    uint16 mlem;
    uint16 idx;
#ifdef DEBUG
    int instances;
#endif
} ObjType;

enum {
    OBJ_CHAR_T = 0,
    OBJ_ECHAR_T = 1,
    OBJ_INT_T = 2,
    OBJ_LLONG_T = 3,
    OBJ_DOUBLE_T = 4,
    OBJ_FLOAT_T = 5,
    OBJ_BOOL_T = 6,
    OBJ_KEYVALUE_T = 7
};

typedef struct {
    void *key;
    void *value;
} MapKeyValue;

#if defined(_M_X64) || defined(__amd64__)
#define LLIB_64_BITS
typedef long long intptr;
#else
typedef int intptr;
#endif

#define obj_header_(P) ((ObjHeader*)(P)-1)
#define array_len(P) (obj_header_(P)->_len)
#define obj_is_array(P) (obj_header_(P)->is_array)
#define obj_ref_array(P) (obj_header_(P)->is_ref_container)
#define obj_type_index(P) (obj_header_(P)->type)
#define obj_type(P) obj_type_(obj_header_(P))

#define obj_new(T,dtor) (T*)obj_new_(sizeof(T),#T,(DisposeFn)dtor)
#define obj_ref(P) (obj_incr_(P), P)
#define obj_unref_v(...) obj_apply_varargs(NULL,(PFun)obj_unref,__VA_ARGS__,NULL)
#define array_new(T,sz) (T*)array_new_(sizeof(T),#T,sz,0)
#define array_new_ref(T,sz) (T*)array_new_(sizeof(T),#T,sz,1)
#define array_new_copy(T,buff,sz) (T*)array_new_copy_(sizeof(T),#T,sz,0,buff)

#define FOR(i,n) for (int i = 0, n_ = (n); i < n_; i++)
#define FOR_PTRZ(T,var,expr) for(T *var = expr; *var; var++)
#define FOR_ARR(T,p,arr) for(T *p=(arr),*e_=p+array_len(p); p!=e_; p++)

#define str_sub (char*)array_copy
#define str_len array_len

#define obj_scoped __attribute((cleanup(__auto_unref)))

#ifndef OBJ_REF_ABBREV
#define ref obj_ref
#define unref obj_unref
#define dispose obj_unref_v
#define scoped obj_scoped
#endif

#define obj_pool_count(P) (array_len(**((void****)P) ))

#ifdef DEBUG
void obj_dump_types(bool all);
const char *obj_type_name(void *P);
#ifdef LLIB_PTR_LIST
void obj_dump_pointers();
#endif
void obj_dump_all();
#endif
int obj_kount();
void *obj_pool();
ObjType *obj_type_(ObjHeader *h);
ObjType *obj_new_type(int size, const char *type, DisposeFn dtor);
int obj_elem_size(void *P);
void *obj_new_(int size, const char *type,DisposeFn dtor);
bool obj_is_instance(const void *P, const char *name);
void obj_incr_(const void *P);
void obj_unref(const void *P);
void obj_apply_varargs(void *o, PFun fn,...);
void __auto_unref(void *p) ;

#define obj_apply(dest,s,src,g) obj_apply_(dest,(PFun)s,src,(PFun)g)
void obj_apply_ (void *dest, PFun setter, void *src, PFun getter);
int obj_refcount (const void *P);

void *array_new_(int mlen, const char *name, int len, int ref);
void *array_new_copy_ (int mlen, const char *name, int len, int ref, void *P);
void *array_copy(void *P, int i1, int i2);
void *array_resize(void *P, int newsz);
char *str_new(const char *s);
char *str_new_size(int sz);
const char *str_ref(const char *s);
char *str_cpy(const char *s);

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

#define seq_new(T) (T**)seq_new_(sizeof(T),#T,0)
#define seq_new_ref(T) (T**)seq_new_(sizeof(T),#T,1)
#define seq_new_str() (char***)seq_new_(sizeof(char*),"char",1)
#define seq_add(s,v) {int idx_=seq_next_(s); (*(s)) [idx_] = (v);}
#define seq_add_items(s,...)   obj_apply_varargs(s,(PFun)seq_add_ptr,__VA_ARGS__,NULL)
#define seq_add2(s,x1,x2) {seq_add(s,x1); seq_add(s,x2);}

void *seq_new_(int nlem, const char *name, int ref);
int seq_next_(void *s);
void seq_add_ptr(void *sp, void *p);
void seq_add_str(void *sp, const char*p);
void seq_resize(Seq *s, int nsz);
void seq_remove(void *sp, int pos, int len);
void seq_insert(void *sp, int pos, void *src, int sz);
void seq_adda(void *sp, void *buff, int sz);
void *seq_array_ref(void *sp);

#endif

