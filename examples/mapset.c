// generalized set operations
#include <stdio.h>
#include <llib/str.h>
#include <llib/json.h>
#include <llib/interface.h>

enum {
  IN_BOTH = 1,
  IN_FIRST = 2,
  IN_SECOND = 4
};


char **mapset (void *A, void *B, int flags) {
    bool condn = flags & IN_BOTH;
    if (flags & IN_SECOND) {
        void *tmp = B;
        B = A;
        A = tmp;
    }
    Iterator *Ai = interface_get_iterator(A);
    StrLookup Blookup = (StrLookup)interface_get_lookup(B);
    void *item;
    char ***res = NULL;
    while (Ai->next(Ai,&item)) {
        if ((Blookup(B,(str_t)item) != NULL) == condn) {
            if (! res)
                res = seq_new(char*);
            seq_add(res,(char*)item);
        }
    }
    if (! res) {
        return NULL;
    } else {
        return (char**)seq_array_ref(res);
    }
}

void do_it (void *A, void *B, int flags) {
    char **ABi = mapset(A,B,flags);
    if (ABi) {
       printf("got %s\n",json_tostring(ABi));
        //~ FOR(i, array_len(ABi)) printf("'%s',",ABi[i]);
        //~ printf("\n");
    } else {
        printf("no match\n");
    }
}

int main()
{
    PValue A = VMS("ONE","1","TWO","2","FOUR","4");
    PValue B = VMS("THREE","3","TWO","2","FOUR","vier");

    do_it(A,B,IN_BOTH);
    do_it(A,B,IN_FIRST);
    do_it(A,B,IN_SECOND);

    return 0;
}
