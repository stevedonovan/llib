#include <string.h>
#include "obj.h"

static void insert_remove(void *sp, int pos, void *ins, int sz) {
    Seq *s = (Seq *)sp;
    char *P = (char*)s->arr;
    ObjHeader *pr = obj_header_(P);
    ObjType *t = obj_type_(pr);
    int on = pr->_len, mlen = t->mlem;
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
    }
    
    pos *= mlen;
    P += pos; // insertion point
    
    // move rest of array up/down
    char *P1 = ins ? P+sze: P;
    char *P2 = ins ? P : P+sze;
    memmove(P1,P2,on*mlen-pos+1);
    
    if (ins)
        memcpy(P,ins,sze);
    else
        sz = -sz;
    pr->_len += sz;
}

void seq_remove(void *sp, int pos, int len) {
    insert_remove(sp,pos,NULL,len);
}

void seq_insert(void *sp, int pos, void *src, int sz) {
    if (src)
        insert_remove(sp,pos,src,sz);
}

