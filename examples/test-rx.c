#include <stdio.h>
#include <stdlib.h>
#include <llib/str.h>
#include <llib/value.h>
#include <llib/file.h>
#include <llib-p/rx.h>

void test_subst() {
    Regex *R = rx_new("%$%((%a+)%)",RX_LUA);
    str_t text = "here is $(HOME) for $(USER)!";
    int i = 0;
    char **ss = strbuf_new();
    while (rx_subst(R,text,ss,&i)) {
        str_t var = rx_group(R,1);
        strbuf_adds(ss,getenv(var));
        unref(var);
    }
    char *res = strbuf_tostring(ss);
    printf("subst '%s'\n",res);
    dispose(R,res);
}

static Regex *R;

bool test_match(str_t s) {
    char **mm = rx_match_groups(R,s);
    if (! mm) // no match possible....
        return false;
    int i = 0;
    for (char **M = mm; *M; ++M)
        printf("match %d '%s'\n",i++,*M);
    return true;
}

int main() {
    
    test_subst();
    
    R = rx_new("[a-z]+",0);
    test_match("abc");
    test_match("abcd");
    
    int i1 = 0, i2;
    str_t text = "aa bb cc ddeeee";
    while (rx_find(R,text,&i1,&i2)) {
        printf("got %d %d '%s'\n",i1,i2,str_sub(text,i1,i2));
        i1 = i2;
    }
    
    char *aa[] = {"one","1","two","2",NULL};
    R = rx_new("%$([a-z]+)",RX_LUA);
    char *s = rx_gsub(R,"$one = $two(doo)",(StrLookup)str_lookup,aa);
    printf("got '%s'\n",s);
    s = rx_gsub(R,"got $one,$two",(StrLookup)str_lookup,aa);
    printf("got '%s'\n",s);
    
    R = rx_new("(%w+)=(%w+)",RX_LUA);
    s = rx_gsub(R,"bonzo=dog,frodo=20",NULL,"\"%1\":%2");
    printf("got '%s'\n",s);
    
    return 0;
}
