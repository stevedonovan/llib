#include <stdio.h>
#include <assert.h>
#include <llib/str.h>
#include <llib/json.h>
#include <llib/list.h>
#include <llib/map.h>
#include <llib/interface.h>

// defining a new interface, Stringer

typedef struct {
    char* (*tostring) (void *o);
    void* (*parse) (const char *s); // optional
} Stringer;

// implement tostring
static char* list_tostring(void *o) {
    return str_fmt("List[%d]",list_size((List*)o));
}

static Stringer s_list = {
    list_tostring,
    NULL  // we can choose not to implement parse
};

void defining_an_interface() {
    // register the interface type
    obj_new_type(Stringer,NULL);
    
    // List implements Stringer
    interface_add(interface_typeof(Stringer), interface_typeof(List), &s_list);
    
    List *ls = list_new_str();
    list_add(ls, "ein");
    list_add(ls, "zwei");
    list_add(ls, "drei");

    // call the interface 'methods'
    Stringer* s = (Stringer*) interface_get(interface_typeof(Stringer),ls);
    assert(str_eq(s->tostring(ls),"List[3]"));
}

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
    
    defining_an_interface();
    return 0;
}


