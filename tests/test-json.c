/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <stdio.h>
#include <llib/list.h>
#include <llib/map.h>
#define LLIB_JSON_EASY
#include <llib/json.h>

//const char *js = "{'one':[10,100], 'two':2, 'three':'hello'}";
//const char *js = "{'one':1, 'two':2, 'three':'hello'}";
//const char *js = "[10,20,30]";
//const char *js = "[{'zwei':2.'twee':2},10,{'A':10,'B':[1,2]}]";
const char *js = "[{'zwei':2,'twee':2},10,{'A':10,'B':[2,20]}]";


int main()
{
    PValue *va = array_new_ref(PValue,7);
    va[0] = value_str("hello dolly");
    va[1] = value_float(4.2);

    List *ls = list_new_str();
    list_add_items(ls, "bonzo","dog");

    va[2] = value_list(ls,ValueString);

    List *li = list_new_ptr();
    list_add(li,(void*)103);
    list_add(li,(void*)20);
    va[3] = value_list(li ,ValueInt);

    Map *m = map_new_str_ptr();
    map_puti(m,"frodo",54);
    map_puti(m,"bilbo",112);
    va[4] = value_map(m,ValueInt);

    double *ai = array_new(double,2);
    ai[0] = 10.0;
    ai[1] = 20;
    va[5] = value_array(ai,ValueFloat);

    short *si = array_new(short,3);
    si[0] = 10;
    si[1] = 100;
    si[2] = 1000;
    va[6] = value_array(si,ValueInt);

    PValue v = value_array(va,ValueValue);

    char *s = json_tostring(v);

    printf("got '%s'\n",s);

    PValue e = value_error("completely borked");
    if (value_is_error(e)) {
        printf("here is an error: %s\n",value_as_string(e));
    }
    dispose(v,s, e);

    v = VM(
        "one",VI(10),
        "two",VM("float",VF(1.2)),
        "three",VA(VI(1),VI(2),VI(3)),
        "four",VB(true)
    );
    s = json_tostring(v);

    printf("got '%s'\n",s);

    dispose(s,v);

    printf("count = %d\n",obj_kount());

    v = json_parse_string(js);
    s = json_tostring(v);
    puts(s);
    dispose(s,v);

    printf("count = %d\n",obj_kount());
    return 0;
}

