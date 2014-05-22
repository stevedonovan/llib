/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <stdio.h>
#include <assert.h>
#include <llib/str.h>

typedef char *Str;

void test_concat()
{
    Str** sseq = seq_new_str();
    seq_add_str(sseq,"hello ");
    seq_add_str(sseq,"dolly");
    seq_add_str(sseq,str_fmt(" it's so %d",42));
    char *res = str_concat(*sseq,",");
    assert(str_eq(res,"hello ,dolly, it's so 42"));
    dispose(sseq,res);
}

void test_split()
{
    Str* words = str_split("alpha, beta, gamma",", ");
    assert(array_len(words) == 3);
    assert(str_eq(words[0],"alpha"));
    assert(str_eq(words[1],"beta"));
    assert(str_eq(words[2],"gamma"));
    dispose(words);
}

int main()
{
    // building up strings
    Str* ss = strbuf_new();
    strbuf_adds(ss,"hello");
    strbuf_adds(ss,"dolly");
    strbuf_addf(ss," the answer is %d",42);
    assert(str_eq(*ss,"hellodolly the answer is 42"));

    // inserting and removing
    strbuf_insert_at(ss,2,"woo",-1);
    assert(str_eq(*ss,"hewoollodolly the answer is 42"));
    strbuf_erase(ss,2,3);
    assert(str_eq(*ss,"hellodolly the answer is 42"));
    strbuf_replace(ss,1,3,"foobar");
    assert(str_eq(*ss,"hfoobarodolly the answer is 42"));

    char *s = str_fmt("we got '%s'",*ss);
    assert(str_eq(s,"we got 'hfoobarodolly the answer is 42'"));

    test_concat();

    int p = str_findstr(*ss,"doll");
    assert(p == 8);
    const char *S = "hello dolly";
    p = str_findch(S,'d');
    assert(p == 6);
    p = str_find_first_of(S," ");
    assert(p == 5);

    assert(str_starts_with(S,"hell"));
    assert(str_ends_with(S,"dolly"));
    assert(! str_starts_with(S,"llo"));
    unref(s);
    s = strbuf_tostring(ss);
    unref(s);
    //unref(ss);

    test_split();

    s = str_new("  hello dolly ");
    str_trim(s);
    assert(str_eq(s,"hello dolly"));
    
    Str s1 = str_sub(s,0,2), s2 = str_sub(s,2,-1);
    //printf("substr (0,2) '%s' (3,-1) '%s'\n",s1,s2);
    assert(str_eq(s1,"he"));
    assert(str_eq(s2,"llo dolly"));
    
    dispose(s,s1,s2 );

    printf("kount %d\n",obj_kount());
    return 0;
}
