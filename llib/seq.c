/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <string.h>
#include "obj.h"

// inserting, removing and appending are fairly straightforward operations,
// except when our seqs are reference containers.  Then _removing_ elements
// must decrease their refcount and _adding_ elements must increase their refcount.
// Otherwise, we have a leak if the seq was the sole owner of removed elements,
// and a nasty double deref if the inserted container array was subsequently disposed.

void seq_remove(void *sp, int pos, int sz) {
    Seq *s = (Seq *)sp;
    char *P = (char*)s->arr;
    ObjHeader *pr = obj_header_(P);
    // original length, element size
    int len = pr->_len, msize = obj_elem_size(P);
    if (pos < 0)  // so that -1 means 'remove last element' etc
        pos = len + pos;

    if (pos > len || pos < 0 || sz < 0 || sz > len) // no op...
        return;

    if (pr->is_ref_container)  { // derefence removed eleemnts
        void **objs = ((void**)P) + pos;
        FOR(i,sz) obj_unref(objs[i]);
    }

    pr->_len -= sz;
    // switch to byte offsets
    sz *= msize;
    pos *= msize;
    len *= msize;
    P += pos; // delete point

    // move rest of array down and resize array
    memmove(P,P+sz,len-pos-sz);

}

void seq_insert(void *sp, int pos, void *src, int sz) {
    Seq *s = (Seq *)sp;
    char *P = (char*)s->arr;
    ObjHeader *pr = obj_header_(P);
    // original length, element size
    int len = pr->_len, msize = obj_elem_size(P);
    bool is_ref = pr->is_ref_container;
    if (pos < 0) { // so that -1 means 'insert after last element' etc
        pos = len + pos + 1;
    }
    if (pos > len || pos < 0) // no op...
        return;

    if (sz == -1) // assume we're a llib array in this case
        sz = array_len(src);

    // make some room, if needed. Maybe new array!
    seq_resize(s, len + sz);
    P = (char*)s->arr;
    array_len(P) = len + sz;

    if (is_ref)  { // bump ref count for new elements
        void **objs = (void**)src;
        FOR(i,sz) obj_incr_(objs[i]);
    }

    // switch to byte offsets
    sz *= msize;
    pos *= msize;
    len *= msize;
    P += pos; // insertion point

    // move rest of array up and insert values
    memmove(P+sz,P,len-pos);
    memcpy(P,src,sz);

}

void seq_adda(void *sp, void *buff, int sz) {
    Seq *s = (Seq *)sp;
    ObjHeader *pr = obj_header_(s->arr);
    int la = pr->_len, mlem = obj_elem_size(s->arr);
    int lss = sz==-1 ? array_len(buff) : sz;
    int lass = la + lss;
    bool was_ref = pr->is_ref_container;
    if (lass > s->cap) {
        seq_resize(s,lass);
    }

    // if we're an array of refcounted objects, then we have to increment
    // refcount of any new elements.  It's assumed that no-one would be
    // so foolish as to try add an array of non-refcounted objects ;)
    if (was_ref) {
        void **Q = (void**)buff, **P = ((void**)s->arr) + la;
        for (; *Q; Q++) {
            *P++ = obj_ref(*Q);
        }
        *P = NULL;
    } else { // otherwise, may safely copy like a savage...
        memcpy((char*)s->arr + la*mlem, buff, lss*mlem);
    }
    array_len(s->arr) = lass;
}
