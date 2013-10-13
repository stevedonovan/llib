/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <stdio.h>
#include <llib/str.h>
#include <llib/map.h>

typedef char *Str;

const char *templ = "Hello $(P)$(name), how is $(home)?";

void test_template()
{
    StrTempl st = str_templ_new(templ,NULL);
    char *tbl1[] = {"name","Dolly","home","here","P","X",NULL};
    char *S = str_templ_subst(st,tbl1);
    printf("T1 '%s'\n",S);
    unref(S);
    char **tbl2 = str_strings("name","Jim","home","joburg","P","Y",NULL);
    S = str_templ_subst(st,tbl2);
    printf("T2 '%s'\n",S);
    dispose(S,tbl2);
    Map *m = map_new_str_str();
    map_put(m,"name","Monique");
    map_put(m,"home","Paris");
    map_put(m,"P","!");
    S = str_templ_subst_using(st,(StrLookup)map_get,m);
    printf("T3 '%s'\n",S);
    dispose(S,m,st);
}

void test_concat()
{
    Str** sseq = seq_new_str();
    seq_add_str(sseq,"hello ");
    seq_add_str(sseq,"dolly");
    seq_add_str(sseq,str_fmt(" it's so %d",42));
    char *res = str_concat(*sseq,",");
    printf("cat:%s\n",res);
    dispose(sseq,res);
}

void test_split()
{
    Str* words = str_split("alpha, beta, gamma",", ");
    FOR_ARR (Str,w,words)
        printf("[%s] ",*w);
    printf("\n");
    dispose(words);
}

int main()
{
    test_template();

    // building up strings
    Str* ss = strbuf_new();
    strbuf_adds(ss,"hello");
    strbuf_adds(ss,"dolly");
    strbuf_addf(ss," the answer is %d",42);
    printf("got '%s'\n",*ss);

    // inserting and removing
    strbuf_insert_at(ss,2,"woo",-1);
    printf("ins '%s'\n",*ss);
    strbuf_erase(ss,2,3);
    printf("era '%s'\n",*ss);
    strbuf_replace(ss,1,3,"foobar");
    printf("rep '%s'\n",*ss);

    char *s = str_fmt("we got '%s'",*ss);
    printf("fmt:%s\n",s);

    test_concat();

    int p = str_findstr(*ss,"doll");
    printf("pos %d\n",p);
    p = str_findch("hello dolly",'d');
    printf("pos %d\n",p);
    p = str_find_first_of("hello dolly"," ");
    printf("pos %d\n",p);
    
    printf("yes %d\n", str_starts_with("hello dolly","hell"));
    printf("yes %d\n", str_ends_with("hello dolly","dolly"));
    printf("no %d\n", str_starts_with("hello dolly","llo"));
    
    test_split();

    dispose(s, ss);
    printf("kount %d\n",obj_kount());
    return 0;
}
