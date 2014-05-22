/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <stdio.h>
#include <llib/obj.h>

typedef struct {
    int *ages;
} Bonzo;

void Bonzo_dispose(Bonzo *self) {
    printf("disposing Bonzo\n");
    unref(self->ages);
}

Bonzo *Bonzo_new(int *ages) {
    Bonzo *self = obj_new(Bonzo,Bonzo_dispose);
    self->ages = ref(ages);
    return self;
}

void test_bonzo() {
    int AGES[] = {45,42,30,16,17};
    int *ages = array_new(int,5);
    FOR(i,5) ages[i] = AGES[i];
    Bonzo *b = Bonzo_new(ages);
    unref(ages);
    printf("%d %d\n",b->ages[0],b->ages[1]);
    unref(b);
}

typedef char *Str;

#define discard obj_unref_v

void test_string() {
    Str s = str_new("hello dolly");
    printf("s '%s' length %d\n",s,str_len(s));
    Str slong = str_new("hello dammit");
    FOR_PTRZ(char,p,slong) printf("%c ",*p);
    printf("\n");
    discard(s,slong);
}

void test_strings() {
    Str *strs = array_new_ref(Str,4);
    #define S str_new
    strs[0] = S("hello");
    strs[1] = S("dolly");
    strs[2] = S("so");
    strs[3] = S("fine");
    FOR_PTRZ(Str,p,strs) printf("%s\n",*p);
    unref(strs);
}

int main() {
    int *pa = array_new(int,10);
    int *pb = ref(pa);
    double *p = array_new(double,40);
    long vals[] = {10,2,5,11,6};
    long *sl = array_new_copy(long,vals,5);
    array_sort(sl,ARRAY_INT,false, 0);
    FOR(i,array_len(sl)) {
        printf("%d ",(int)sl[i]);
    }
    printf("\n");
    printf("len %d\n",array_len(pa));
    test_bonzo();
    test_string();
    test_strings();
    discard(pb,pa,p,sl);
    printf("kount %d\n",obj_kount());
    return 0;
}
