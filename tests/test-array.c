#include <stdio.h>
#include <assert.h>
#include <llib/str.h>
#include <llib/array.h>
#include <llib/dbg.h>

#define array_new_init(T,A) array_new_copy(T,A,sizeof(A)/sizeof(T))

void test_map() {
    char** ss = str_strings("one","two","three",NULL);
    MAPA(int,ssl,strlen(_),ss);
    assert(ssl[0] == 3);
    assert(ssl[1] == 3);
    assert(ssl[2] == 5);
    
    int nn[] = {1,2,22,1,40,3};
    int *na = array_new_init(int,nn);
    
    // MAPAR because we want the result to be a ref container..
    MAPAR(char*,sna,str_fmt("%02d",_),na);
    
    #define cat(arr) str_concat(arr," ")
    assert(str_eq(cat(sna),"01 02 22 01 40 03"));

    // collecting the strings that match a condition
    // (again, we want a ref container as a result)
    FILTAR(char*,sna2,sna,_[1]=='2');
    assert(str_eq(cat(sna2),"02 22"));
    

    FILTA(int,nam,na,  _ < 10);
    char **sn = strbuf_new();
    FORA(nam,strbuf_addf(sn,"%02d ",_));
    assert(str_eq(*sn,"01 02 01 03 "));
    
}

void test_finda() {
    char** exts = str_strings(".o",".d",".obj",NULL);
    str_t extensions[] = {".o",".d",".obj",NULL};
    str_t file = "bonzo.d";
    // use with llib arrays, with start index
    FINDA(idx,exts,0,str_ends_with(file,_));
    assert(idx == 1);
    // use with any NULL-terminated array
    FINDZ(i,extensions,str_ends_with(file,_));
    assert(i == 1);
    
    int nums[] = {-1,-2,2,-1,4,3};
    int *arr = array_new_init(int,nums);

    // unrolling a loop for searching through an array
    // we need different names because FINDA/Z declares a new index.
    FINDA(i1,arr,0, _ > 0);
    assert(i1 == 2);
    FINDA(i2,arr,i1+1, _ > 0);
    assert(i2 == 4);
    FINDA(i3,arr,i2+1, _ > 0);
    assert(i3 == 5);
}

int main()
{    
    test_map();
    test_finda();
    printf("ok\n");
    return 0;
}