#include <stdio.h>
#include <llib/str.h>
#include <llib/json.h>
#include <llib/list.h>
#include <llib/map.h>
#include <llib/interface.h>

int main() 
{
    char** ss = str_strings("one","two","three",NULL);
    Iterator* it = interface_get_iterator(ss);
    char* s, *t;
    while (it->next(it,&s)) {
        printf("got '%s'\n",s);
    }
    void *smap = VMS("one","1","two","2","three","3");
    it = interface_get_iterator(smap);
    while (it->nextpair(it,&s,&t)) {
        printf("'%s': '%s'\n",s,t);
    }
    ObjLookup L = interface_get_lookup(smap);
    printf("lookup '%s'\n",(char*)L(smap,"two"));
    List *ls = list_new_str();
    list_add(ls, "ein");
    list_add(ls, "zwei");
    list_add(ls, "drei");
    it = interface_get_iterator(ls);
    while (it->next(it,&s)) {
        printf("got '%s'\n",s);
    }
    Map *m = map_new_str_str();
    map_put(m,"one","1");
    map_put(m,"two","2");
    map_put(m,"three","3");
    it = interface_get_iterator(m);
    while (it->nextpair(it,&s,&t)) {
        printf("'%s': '%s'\n",s,t);
    }    
    return 0;
}


