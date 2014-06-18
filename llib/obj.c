/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
### Support for Refcounted Objects.

These are (currently) allocated with `malloc` and freed with `free`, but in addition always
carry a refcount (see `obj_refcount`).  `obj_ref` increments this count, and `obj_unref`
decrements this count; when the count becomes zero, the object is freed.  If the object
has an associated dispose function specified in `obj_new`, then this will be called first.

Unless `OBJ_REF_ABBREV` is defined, these are also defined as `ref` and `unref`; there is
a bulk `dispose` for multiple objects.

llib objects may specify a _dispose_ function:

    typedef struct {
        int age;
        char *name;
    } Person;
    
    static void Person_dispose(Person *p) {
        obj_unref(p->name);
    }
    
    Person *person_new(int age, char *name) {
        Person *p = obj_new(Person,Person_dispose);
        p->age = age;
        p->name = str_ref(name);
        return p;
    }

In this typical scenario, the dispose function decrements the reference
count of the `name` field when the `Person` is disposed;  this will of course
only dispose the string if it hasn't already been referenced elsewhere, which
permits flexible sharing of data.  (We use `str_ref` specifically here because the
argument `name` may not be already a ref-counted string.)

_Arrays_ carry their own size, accessible with `array_len`; 
if created with `array_new_ref`, they will unref their elements when disposed.

_Sequences_ are resizeable arrays. `seq_add` appends a new value
to a sequence. A sequence `s` is a pointer to an array of T.  The underlying
array can always be accessed with `*s`, and the array will be sized-to-fit when
`seq_array_ref` is called.

See `test-obj.c` and `test-seq.c`

@module obj
*/

#include "obj.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#define MAX_PTRS 10000

/// standard for-loop.
// @param i loop variable.
// @param n loop count.
// @macro FOR

/// for-loop over array.
// @tparam type T base type
// @tparam type* loop pointer
// @tparam type* array
// @macro FOR_ARR
// @usage FOR_ARR(int,pi,arr) printf("%d ",*pi);

// number of created 'live' objects -- access with obj_kount()
static int kount = 0;

#ifdef LLIB_PTR_LIST
// Generally one can't depend on malloc or other allocators returning pointers
// within a given range. So we keep an array of 'our' pointers, which we know
// for a fact were allocated using `new_obj`
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
    // look for first empty slot, or a new one if none found.
    int idx = our_ptr_idx(NULL);
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
#define our_ptr(p) (our_ptr_idx(p) != -1)
#else
// if the libc supports, here is a simpler scheme which assumes that 'our' pointers are always
// between the lowest and highest pointer value allocated so far.  This scheme of course gives a false
// positive if a pointer was just allocated with plain malloc, so be careful about using obj_unref
static void *low_ptr, *high_ptr;

static void add_our_ptr(void *p) {
    if (! low_ptr) {
        low_ptr = p;
        high_ptr = p;
    } else {
        if (p < low_ptr)
            low_ptr = p;
        else if (p > high_ptr)
            high_ptr = p;
    }
    ++kount;
}

static int our_ptr (void *p) {
    return p >= low_ptr && p <= high_ptr;
}

static void remove_our_ptr(void *p) {
    --kount;
}
#endif

int obj_kount() { return kount; }

#ifdef LLIB_DEBUG
static bool s_do_free=true;
#endif

static void s_free(void *p, void *obj) {
    free(obj);
}

static void *s_alloc(void *p, int size) {
    return malloc(size);
};

ObjAllocator obj_default_allocator = {
  s_alloc, s_free, NULL
};

DisposeFn _pool_filter, _pool_cleaner;

#define PTR_FROM_HEADER(h) ((void*)(((ObjHeader*)(h))+1))
#define HEADER_FROM_PTR(P) ((ObjHeader*)P-1)

// all llib objects are allocated here
static ObjHeader *new_obj(int size, ObjType *t) {
    size += sizeof(ObjHeader);
    void *obj;

    if (! t->alloc) {
        obj = malloc(size);        
    } else {
        obj = t->alloc->alloc(t->alloc,size);
    }
    add_our_ptr(obj);
#ifdef LLIB_DEBUG
    ++t->instances;
#endif
    if (_pool_filter)
        _pool_filter(PTR_FROM_HEADER(obj));
    return (ObjHeader *)obj;
}

/// refcount of an object.
// Will return -1 if it isn't one of ours!
// @within RTTI
int obj_refcount (const void *p)
{
    if (p == NULL) return -1;
    ObjHeader *pr = obj_header_(p);
    if (our_ptr(pr)) {
        return pr->_ref;
    } else {
        return -1;
    }
}

/// length of array.
// @tparam void* arr
// @treturn int
// @function array_len
// @within RTTI

/// is this an array?.
// @tparam void* arr
// @treturn bool
// @function obj_is_array
// @within RTTI

/// is this a reference container array?
// @tparam void* arr
// @treturn bool
// @function obj_ref_array
// @within RTTI

/// type of object.
// @tparam void* arr
// @treturn ObjType*
// @function obj_type
// @within RTTI


// Type descripters are kept in an array
#define LLIB_TYPE_MAX 4096

ObjType obj_types[LLIB_TYPE_MAX];
const ObjType *obj_types_ptr = obj_types;

int obj_types_size = 8;

typedef ObjType *OTP;

OTP obj_type_(ObjHeader *h) {
    return &obj_types[h->type];
}

OTP type_from_dtor(const char *name, DisposeFn dtor) {
    if (dtor) {
        for(OTP pt = obj_types; pt->name; ++pt) {
            if (pt->dtor == dtor)
                return pt;
        }
    } else { // no dispose fun, so let's match by name
        for(OTP pt = obj_types; pt->name; ++pt) {
            if (strcmp(pt->name,name) == 0)
                return pt;
        }
    }
    return NULL;
}

OTP obj_new_type(int size, const char *type, DisposeFn dtor) {
    OTP t = &obj_types[obj_types_size];
    t->name = type;
    t->dtor = dtor;
    t->mlem = size;
    t->idx = obj_types_size++;
    // just in case...
    obj_types[obj_types_size].name = NULL;
    return t;
}

// the type system needs to associate certain common types with fixed slots
static bool initialized = false;

static ObjType obj_types_initialized[] = {
    {"char",NULL,NULL,1,0},
    {"echar_",NULL,NULL,1,1},
    {"int",NULL,NULL,sizeof(int),2},
    {"long long",NULL,NULL,sizeof(long long),3},
    {"double",NULL,NULL,sizeof(double),4},
    {"float",NULL,NULL,sizeof(float),5},
    {"bool",NULL,NULL,sizeof(bool),6},
    {"MapKeyValue",NULL,NULL,sizeof(MapKeyValue),7}
};

static void initialize_types() {
    initialized = true;
    memcpy(obj_types,obj_types_initialized,sizeof(obj_types_initialized));
}

/// allocate a new refcounted object.
// @tparam type T
// @tparam DisposeFn optional destructior
// @treturn T*
// @function obj_new
// @within New

void *obj_new_(int size, const char *type, DisposeFn dtor) {
    if (! initialized) {
        initialize_types();
    }
    OTP t = type_from_dtor(type,dtor);
    if (! t)
        t = obj_new_type(size,type,dtor);

    ObjHeader *h = new_obj(size,t);
    h->_len = 0;
    h->_ref = 1;
    h->is_array = 0;
    h->is_ref_container = 0;
    h->type = t->idx;
    void *P = PTR_FROM_HEADER(h);
#ifdef LLIB_DEBUG_VERBOSE
    fprintf(stderr,"obj %s %p\n",type,P);
#endif
    return P;
}

// getting element size is now a little more indirect...

/// size of this object.
// For arrays, this is size of base type.
// @within RTTI
int obj_elem_size(void *P) {
    ObjHeader *h = obj_header_(P);
    return obj_type_(h)->mlem;
}

/// whether an object matches a type by name.
// @within RTTI
bool obj_is_instance(const void *P, const char *name) {
    if (obj_refcount(P) == -1)
        return false;
    return strcmp(obj_type(P)->name,name) == 0;
}

// Ref counted objects are either arrays or structs.
// If they are arrays of refcounted objects, then
// this has to unref those objects. For structs,
// there may be an explicit destructor.
static void obj_free_(ObjHeader *h, const void *P) {
    OTP t = obj_type_(h);
    if (h->is_array) { // arrays may be reference containers
        if (h->is_ref_container) {
          void **arr = (void**)P;
          for (int i = 0, n = h->_len; i < n; i++) {
                obj_unref(arr[i]);
          }
        }
    } else { // otherwise there may be a custom dispose operation for the type
        if (t->dtor)
          t->dtor((void*)P);
    }

    remove_our_ptr(h);

#ifdef LLIB_DEBUG
    --t->instances;
#endif

    // if the object pool is active, then remove our pointer from it!
    if (_pool_cleaner)
        _pool_cleaner((void*)P);

    // the object's type might have a custom allocator
    if (t->alloc) {
        t->alloc->free(t->alloc,h);
    } else {
#ifdef LLIB_DEBUG
        if (s_do_free)
#endif        
            free(h);
    }
}

// Reference counting

/// increase reference count (`ref`).
// @tparam T object
// @treturn T
// @function obj_ref
// @within References

void obj_incr_(const void *P) {
    ObjHeader *h = obj_header_(P);
    // if the object pool is active, then remove our pointer from it!
#ifdef LLIB_DEBUG_VERBOSE    
    fprintf(stderr,"+ref %p\n",P);
#endif
    if (_pool_cleaner && h->_ref == 1) {
        _pool_cleaner((void*)P);
    }    
    ++(h->_ref);
}

/// decrease reference count (`unref`).
// When this goes to zero, free the object and call the
// dispose function, if any.
// @within References
void obj_unref(const void *P) {
    if (P == NULL) return;
    ObjHeader *h = obj_header_(P);
#ifdef LLIB_DEBUG
#ifdef LLIB_DEBUG_VERBOSE
    fprintf(stderr,"-ref %p\n",P);
#endif
    // this check is only really useful if LLIB_PTR_LIST is used
    if (! our_ptr(h)) {
        fprintf(stderr,"llib: unref of non ref-counted pointer\n");
        abort();        
    }
    // catches already unreferenced pointers...
    if (h->_ref == 0) {
        fprintf(stderr,"llib: unref of dead pointer\n");
        abort();
    }
#endif
    --(h->_ref);
    if (h->_ref == 0) {
#ifdef LLIB_DEBUG_VERBOSE    
        fprintf(stderr,"freed %p\n",P);
#endif
        obj_free_(h,P);        
    }
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
// @within References
// @function obj_unref_v

/// apply a function to all arguments.
// If `o` is not `NULL`, then call `fn(o,arg)`, otherwise
// call `fn(arg)`.
// @within Utilities
void obj_apply_varargs(void *o, PFun fn,...) {
    va_list ap;
    va_start(ap,fn);
    obj_apply_v_varargs(o,fn,ap);
    va_end(ap);
}

#ifdef LLIB_DEBUG
void obj_dump_types(bool all) {
    printf("+++ llib types\n");
    FOR(i,obj_types_size) {
        ObjType *t = &obj_types[i];
        if (all || t->instances > 0) {
            printf("%3d (%s) size %d",t->idx, t->name,t->mlem);
            if (t->instances > 0)
                printf(" --> %d alive\n",t->instances);
            else
                printf("\n");
        }
    }
    printf("+++\n");
}

static int *snp;

void obj_snapshot_create() {
    if (snp)
        free(snp);
    snp = (int*)malloc(sizeof(int)*LLIB_TYPE_MAX);
    FOR(i,LLIB_TYPE_MAX)
        snp[i] = obj_types[i].instances;
}

void obj_snapshot_dump() {
    if (! snp)
        return;
    printf("+++ new objects\n");
    FOR(i,LLIB_TYPE_MAX) {
        ObjType *T = &obj_types[i];
        if (T->instances > 0 && snp[i] == 0)
            printf("%3d (%s) %d\n",i,T->name,T->instances);
    }
    printf("+++ deleted objects\n");
    FOR(i,LLIB_TYPE_MAX) {
        ObjType *T = &obj_types[i];
        if (T->instances == 0 && snp[i] > 0)
            printf("%3d (%s) %d\n",i,T->name,T->instances);
    }
    printf("+++ changed objects\n");
    FOR(i,LLIB_TYPE_MAX) {
        ObjType *T = &obj_types[i];
        if (T->instances > 0 && snp[i] > 0 && T->instances != snp[i])
            printf("%3d (%s) %d -> %d\n",i,T->name,snp[i],T->instances);
    }
}

// these non-macro versions are useful when in a debugger such as GDB
int a_len(void *a) { return array_len(a); }
ObjHeader* o_hdr(void *a) { return obj_header_(a); }
ObjType *o_type(void *a) { return obj_type(a); }

const char *obj_type_name(void *P) {
    static char buff[256];
    char *b = buff;
    b += sprintf(buff,"(%s*)",obj_type(P)->name);
    if (obj_is_array(P))
        sprintf(b,"[%d]",array_len(P));
    return buff;
}

void obj_dump_ptr(void *P) {
    int c = obj_refcount(P);
    if (c != -1)
        printf("%p ref %d type %s\n",P,c,obj_type_name(P));
    else
        printf("not one of ours\n");
}

#ifdef LLIB_PTR_LIST
void obj_dump_pointers() {
    printf("+++ llib objects\n");
    FOR(i,max_p) {
        if (our_ptrs[i] != NULL) {
            void *P = (void*)((ObjHeader*)our_ptrs[i] + 1);
            obj_dump_ptr(P);            
        }
    }
    printf("+++\n");
}

static void **s_ptrs;

void obj_snap_ptrs_create() {
    if (s_ptrs)
        free(s_ptrs);
    s_ptrs = (void**)malloc(sizeof(void*)*MAX_PTRS);
    FOR(i,MAX_PTRS)
        s_ptrs[i] = our_ptrs[i];
}

void obj_snap_ptrs_dump() {
    if (! s_ptrs)
        return;
    FOR(i,MAX_PTRS) {
        if (our_ptrs[i] != NULL && s_ptrs[i] == NULL) {
            void *P = (void*)((ObjHeader*)our_ptrs[i] + 1);
            obj_dump_ptr(P);
        }
    }
}
#endif

void obj_dump_all() {
    obj_dump_types(false);
#ifdef LLIB_PTR_LIST
    obj_dump_pointers();
#endif
}

void obj_free_set(bool set) {
    s_do_free = set;
}
#endif

typedef unsigned char byte;

//? allocates len+1 - ok?

void *array_new_(int mlen, const char *name, int len, int isref) {
    if (! initialized) {
        initialize_types();
    }

    OTP t = type_from_dtor(name,NULL);
    if (! t)
        t = obj_new_type(mlen,name,NULL);

    ObjHeader *h = new_obj(mlen*(len+1),t);
    byte *P;
    h->type = t->idx;
    h->_len = len;
    h->_ref = 1;
    h->is_array = 1;
    h->is_ref_container = isref;
    P = (byte*)PTR_FROM_HEADER(h);
    if (isref) {  // ref arrays are fully zeroed out
        memset(P,0,mlen*(len+1));
    } else { // otherwise just the last element
        memset(P+mlen*len,0,mlen);
    }
#ifdef LLIB_DEBUG_VERBOSE    
    fprintf(stderr,"arr %s %p[%d] %d\n",name,P,len,isref);
#endif    
    return P;
}

/// new array from type.
// @tparam T type name of type
// @int sz size of array
// @treturn T*
// @function array_new
// @within New

/// new array of refcounted objects.
// @tparam T type name of type
// @int sz size of array
// @treturn T*
// @function array_new_ref
// @within New

/// new array from buffer.
// @tparam T type name of type
// @tparam T* buff
// @int sz size of array
// @function array_new_copy
// @within New

void *array_new_copy_ (int mlen, const char *name, int len, int isref, void *P) {
    void *arr = array_new_(mlen,name,len,isref);
    memcpy(arr,P,mlen*len);
    return arr;
}

/// get a slice of an array.
// @param P the array
// @param i1 lower bound
// @param i2 upper bound (can be -ve; -1 is up to end, -2 is one less than end, etc)
// @within Array
void * array_copy(void *P, int i1, int i2) {
    ObjHeader *pr = obj_header_(P);
    OTP t = obj_type_(pr);
    int mlem = t->mlem;
    int offs = i1*mlem;
    int len = i2 < 0 ? pr->_len+i2+1 : i2-i1;
    bool is_ref = pr->is_ref_container != 0;
    void *newp = array_new_(mlem,t->name,len,is_ref);
    if (is_ref) { // update reference counts if needed
        void **ptr = (void**)P+offs, **op = (void**)newp;
        FOR(i,len)
            *op++ = obj_ref(*ptr++);
    } else {
        memcpy(newp,(char*)P+offs,mlem*len);
    }
    return newp;
}

/// resize an array.
// @param P the array
// @param newsz the new size
// @within Array
void * array_resize(void *P, int newsz) {
    ObjHeader *pr = obj_header_(P);
    OTP t = obj_type_(pr);
    int mlen = t->mlem;
    void *newp = array_new_(mlen,t->name,newsz,pr->is_ref_container);
    memcpy(newp,P,mlen*pr->_len);
    // if old ref array is going to die, make sure it doesn't dispose our elements
    pr->is_ref_container = 0;
    obj_unref(P);
    return newp;
}

// Arrays of char can be directly used as C strings,
// since they always have a trailing zero byte.

/// create a refcounted string copy.
// @within New
char *str_new(const char *s) {
    int sz = strlen(s);
    char *ns = str_new_size(sz);
    memcpy(ns,s,sz);
    return ns;
}

/// create a refcounted string of given size
// @within New
char *str_new_size(int sz) {
    return array_new(char,sz);
}

/// increase the refcount of a string.
// @within References
const char *str_ref(const char *s) {
    int rc = obj_refcount(s);
    if (rc == -1)
        return str_new(s);
    else
        return (const char*)obj_ref(s);
}

/// make a refcounted string from an arbitrary string.
// @within New
char *str_cpy(const char *s) {
    int rc = obj_refcount(s);
    if (rc == -1) {
        return str_new(s);
    } else {
        return (char*)s;
    }
}

/// sort an array.
// @tparam T* P the array
// @int kind  either `ARRAY_INT` or `ARRAY_STR`
// @int ofs offset into each item
// @function array_sort
// @within Array

/// sort an array of structs by integer/pointer field
// @tparam T* A the array
// @tparam T type the struct
// @param field the name of the struct field
// @function array_sort_struct_ptr
// @within Array

/// sort an array of structs by string field
// @tparam T* A the array
// @tparam T type the struct
// @param field the name of the struct field
// @function array_sort_struct_str
// @within Array

/// get from a source and put to a destination.
// @param dest destination object
// @param setter function of dest and the gotten object
// @param src source object
// @param getter function of `src` to call repeatedly until it returns `NULL`
// @function obj_apply
// @within Utilities
void obj_apply_ (void *dest, PFun setter, void *src, PFun getter)
{
    void *p;
    while ((p = getter(src)) != NULL) {
        setter(dest,p);
    }
}

/// Sequences.
// @section sequences

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

void *seq_new_(int nlem, const char *name, int isref) {
    Seq *s = obj_new(Seq,seq_dispose);
    s->arr = array_new_(nlem,name,INITIAL_CAP,isref);
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

/// add a value to a sequence.
// @param s the sequence
// @param v the value
// @function seq_add

/// add arbitrary number of values to a sequence.
// The values are implicitly terminated with `NULL`, so
// this won't work for arbitrary integers.
// @param s the sequence
// @param .... the values
// @function seq_add_items

/// remove values from a sequence.
// @param s the sequence
// @int pos position
// @int len number of values
// @function seq_remove

/// insert values into a sequence.
// @param s the sequence
// @int pos position
// @param array of values
// @int sz number of values (-1 means use `array_len`)
// @function seq_insert

/// extend sequence with array of values.
// @param s the sequence
// @param array of values
// @int sz number of values (-1 means use `array_len`)
// @function seq_adda

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
// (This will transfer ownership, shrink to fit,
// and unref the sequence.)
void *seq_array_ref(void *sp) {
    Seq *s = (Seq *)sp;
    int len = array_len(s->arr);
    if (len < s->cap) {
        s->arr = array_resize(s->arr, len);
    }
    void *arr = s->arr;
    s->arr = NULL;
    obj_unref(s);
    return arr;
}

