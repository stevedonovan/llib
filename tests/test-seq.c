/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <stdio.h>
#include <llib/obj.h>

typedef struct {
    int val;
} Foo;

void foo_dispose(Foo *p)
{
    printf("disposing foo %d\n",p->val);
}

static int kount = 0;

Foo *foo_new()
{
    Foo *obj = obj_new(Foo,foo_dispose);
    obj->val = ++kount;
    return obj;
}

#define DUMP(T,F,arr) FOR_ARR(T,p_,arr) \
    printf(F,*p_);  printf("\n");

int main()
{
// /*
    int **fs = seq_new(int);
    seq_add(fs, 10);
    seq_add(fs, 20);
    seq_add(fs, 30);
    seq_add(fs, 40);
    seq_add(fs, 50);
    FOR (i,22)
        seq_add(fs,i);
    printf("size %d \n",array_len(*fs));
    DUMP(int,"%d ",*fs);
    obj_unref(fs);

    double **ss = seq_new(double);
    seq_add(ss,1.0);
    seq_add(ss,2.0);
    DUMP(double,"%f ",*ss)

    // ref seqs are containers for ref objects
    typedef Foo *PFoo;
    PFoo **sf = seq_new_ref(PFoo);
    FOR (i,6)
        seq_add(sf,foo_new());
    FOR_ARR(PFoo,p,*sf)
        printf("%d ",(*p)->val);
    printf("\n");
    PFoo f = ref((*sf)[2]);
    // and disposing them will dispose their objects
    unref(sf);
    printf("now explicitly release f\n");
    unref(f);
    return 0;
}
