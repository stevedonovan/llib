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

static void insert_remove(void *sp, int pos, void *ins, int sz) {
    Seq *s = (Seq *)sp;
    char *P = (char*)s->arr;
    ObjHeader *pr = obj_header_(P);
    int on = pr->_len, mlen = obj_elem_size(P);
    if (pos < 0) {
        pos = on + pos; 
        if (ins) // so that -1 means 'insert after last element'
            ++pos;
    }
    if (pos > on) // no op...
        return;
    
    if (sz == -1)
        sz = array_len(ins);
    int sze = sz*mlen;

    if (ins) {  // make some room!    
        seq_resize(s, on + sz);
        P = (char*)s->arr;
        // bump ref count for new elements
        if (pr->is_ref_container)  {
            void **objs = (void**)ins;
            FOR(i,sz) obj_incr_(objs[i]);
        }
    } else {
        // or deref elements being deleted,
        if (pr->is_ref_container)  {
            void **objs = ((void**)P) + pos;
            FOR(i,sz) obj_unref(objs[i]);
        }        
    }
    
    pos *= mlen;
    P += pos; // insertion point
    
    // move rest of array up/down
    char *P1 = ins ? P+sze: P;
    char *P2 = ins ? P : P+sze;
    memmove(P1,P2,on*mlen-pos+1);
    
    if (ins) {
        memcpy(P,ins,sze);
    } else {
        sz = -sz;
    }
    pr->_len += sz;
}

void seq_remove(void *sp, int pos, int len) {
    insert_remove(sp,pos,NULL,len);
}

void seq_insert(void *sp, int pos, void *src, int sz) {
    if (src)
        insert_remove(sp,pos,src,sz);
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
