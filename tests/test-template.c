/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <llib/template.h>
#include <llib/str.h>
#include <llib/list.h>
#include <llib/map.h>
#define LLIB_JSON_EASY
#include <llib/json.h>

const char *tpl = "Hello $(P)$(name), how is $(home)?";

void simple_case()
{
    StrTempl st = str_templ_new(tpl,NULL);
    char *tbl1[] = {"name","Dolly","home","here","P","X",NULL};
    char *S = str_templ_subst(st,tbl1);
    assert(str_eq(S,"Hello XDolly, how is here?"));
    unref(S);

    char **tbl2 = str_strings("name","Jim","home","joburg","P","Y",NULL);
    S = str_templ_subst(st,tbl2);
    assert(str_eq(S,"Hello YJim, how is joburg?"));
    dispose(S,tbl2);

    Map *m = map_new_str_str();
    map_put(m,"name","Monique");
    map_put(m,"home","Paris");
    map_put(m,"P","!");
    S = str_templ_subst_using(st,(StrLookup)map_get,m);
    assert(str_eq(S,"Hello !Monique, how is Paris?"));
    dispose(S,m,st);
}

static char *l_getenv (void *data, char *key) {
    return getenv(key);
}

void using_environment()
{
    StrTempl st = str_templ_new("@{ROME} and @{PATH}","@{}");
    char *S = str_templ_subst_using(st, (StrLookup)l_getenv, NULL);
    printf("got '%s'\n",S);
    dispose(st,S);
}

const char *Tp =
"<h2>$(title)</h2>\n"
"<ul>\n$(for items |"
"<li><a src='$(url)'>$(title)</a></li>\n"
"|)</ul>\n";

const char *js = "{'title':'Pages','items':[{'url':'index.html','title':'Home'},"
    "{'url':'catalog.html','title':'Links'}]}";

void using_json()
{
    PValue v = VM(
        "title",VS("Pages"),
        "items",VA(
            VMS("url","index.html","title","Home"),
            VMS("url","catalog.html","title","Links")
        )
    );
    StrTempl st = str_templ_new(Tp,NULL);
    char *s = str_templ_subst_values(st,v);
    puts(s);
    dispose(s,v);
    v = json_parse_string(js);
    if (value_is_error(v)) {
        fprintf(stderr,"error '%s'\n",value_as_string(v));
    } else {
        s = str_templ_subst_values(st,v);
        puts(s);
        unref(s);
    }
    dispose(st,v);
}

const char *templ = "Hello $(name), how is $(home) at $(i age) ?\n"
    "$(with sub| $(A)=$(B)|)$(name)\n"
    "$(for items | hello $(X)|)\n"
    "$(for ls | $(X) is here |) thanks!\n"
    "$(if es |Hola|)$(else |Hello $(home)|) $(name)";

typedef char *Str;

int main()
{
    using_json();
    using_environment();
    simple_case();

    // NULL means we are happy with $() escapes
    StrTempl st = str_templ_new(templ,NULL);
    Str sbl[] = {
        "A","alpha",
        "B","beta",
        NULL
    };

    Str **smap = array_new_ref(Str*,2);
    smap[0] = str_strings("X","vex",NULL);
    smap[1] = str_strings("X","hex",NULL);

    List *ls = list_new_ref();
    Map *m = map_new_str_str();
    map_put(m,"X","vex");
    list_add(ls,m);
    m = map_new_str_str();
    map_put(m,"X","hex");
    list_add(ls,m);

    Str tbl[] = {
        "name","Dolly",
        "home","here",
        "age",(char*)42,
        "sub",(char*)sbl,
        "items",(char*)smap,
        "ls",(char*)ls,
       // "es","si",
        NULL
    };

    Str S1 = str_templ_subst(st,tbl);
    printf("got '%s'\n",S1);
    Str S2 = str_templ_subst(st,tbl);
    assert(str_eq(S1,S2));
    dispose(S1,S2,st,smap,ls);
    printf("kount = %d\n",obj_kount());
    return 0;
}
