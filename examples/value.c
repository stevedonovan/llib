/* This demonstrates a simple 'variant' type for containing semi-arbitrary
*   values, and how they can be streamed out in JSON format.
*/
#include <stdio.h>
#include <stdlib.h>
#include <llib/obj.h>
#include <llib/list.h>
#include <llib/str.h>
#include <llib/map.h>

typedef enum ValueContainer_ {
    ValueScalar,
    ValueArray,
    ValueList,
    ValueMap
} ValueContainer;

#define VALUE_REF 0x100

typedef enum ValueType_ {
    ValueNone = 0,
    ValueString = VALUE_REF + 1,
    ValueError= VALUE_REF + 2,
    ValueFloat = 1,
    ValueInt = 2,
    ValueValue = VALUE_REF + 3
} ValueType;

#ifdef LLIB_64_BITS
typedef double floatptr;
#else
typedef float floatptr;
#endif

typedef struct Value_ {
    ValueContainer vc;
    ValueType type;
    union {
        const char *str;
        intptr i;
        floatptr f;
        List *ls;
        Map *map;
        struct Value_ *v;
        void *ptr;
    } v;
} Value, *PValue;

void Value_dispose(PValue v) {
    if (v->vc != ValueScalar || (v->type & VALUE_REF)) {
        //printf("disposing %d %d\n",v->type,v->vc);
        obj_unref(v->v.ptr);
    }
}

PValue value_new(ValueType type, ValueContainer vc) {
    PValue v = obj_new(Value,Value_dispose);
    v->type = type;
    v->vc = vc;
    return v;
}

PValue value_str (const char *str) {
    PValue v = value_new(ValueString,ValueScalar);
    v->v.str = str_cpy((char*)str);
    return v;
}

PValue value_error (const char *msg) {
    PValue v = value_str(msg);
    v->type = ValueError;
    return v;
}

#define value_is_error(v) ((v)->type==ValueError)
#define value_is_string(v) ((v)->type==ValueString)
#define value_is_float(v) ((v)->type==ValueFloat)
#define value_is_int(v) ((v)->type==ValueInt)
#define value_is_value(v) ((v)->type==ValueValue)
#define value_is_list(v) ((v)->vc==ValueList)
#define value_is_array(v) ((v)->vc==ValueArray)
#define value_is_map(v) ((v)->vc==ValueMap)

#define value_as_string(vv) ((vv)->v.str)
#define value_as_int(vv) ((vv)->v.i)
#define value_as_float(vv) ((vv)->v.f)
#define value_as_value(vv) ((vv)->v.v)
#define value_as_array(vv) ((vv)->v.ptr)
#define value_as_list(vv) ((vv)->v.ls)
#define value_as_map(vv) ((vv)->v.map)
#define value_as_pointer(vv) ((vv)->v.ptr)

PValue value_float (double x) {
    PValue v = value_new(ValueFloat,ValueScalar);
    v->v.f = x;
    return v;
}

PValue value_int (long long i) {
    PValue v = value_new(ValueInt,ValueScalar);
    v->v.i = i;
    return v;
}

PValue value_value (PValue V) {
    PValue v = value_new(ValueValue,ValueScalar);
    v->v.v = V;
    return v;
}

PValue value_list (List *ls, ValueType type) {
    PValue v = value_new(type,ValueList);
    v->v.ls = ls;
    return v;
}

PValue value_array (void *p, ValueType type) {
    PValue v = value_new(type,ValueArray);
    v->v.ptr = p;
    return v;
}

PValue value_map (Map *m, ValueType type) {
    PValue v = value_new(type,ValueMap);
    v->v.map = m;
    return v;
}

#include <assert.h>

typedef char *Str, **SStr;

void dump_array(SStr s, PValue vl);
void dump_list(SStr s, PValue vl);
void dump_map(SStr s, PValue vl);

void dump_value(SStr s, PValue v)
{
    switch (v->vc) {
    case ValueScalar:
        switch (v->type) {
        #define addf(fmt,F) strbuf_addf(s,fmt,v->v.F)
        case ValueInt:
            addf("%d",i);
            break;
        case ValueString:
        case ValueError:
            addf("'%s'",str);
            break;
        case ValueFloat:
            addf("%f",f);
            break;
        case ValueNone:
            addf("?%p?",ptr);
            break;
        case ValueValue:
            dump_value(s,v->v.v);
            break;
        }    
        #undef addf
        break;
    case ValueList:
        dump_list(s,v);
        break;
    case ValueArray:
        dump_array(s,v);
        break;
    case ValueMap:
        dump_map(s,v);
        break;
    }
}

void dump_list(SStr s, PValue vl)
{
    List *li = value_as_list(vl);
    int ni = list_size(li) - 1, i = 0;
    Value vs;
    vs.vc = ValueScalar;
    vs.type = vl->type;
    strbuf_adds(s,"[");
    FOR_LIST(item,li) {        
        vs.v.ptr = item->data;
        dump_value(s,&vs);
        if (i++ < ni)
            strbuf_adds(s,",");
    }
    strbuf_adds(s,"]");    
}

void dump_map(SStr s, PValue vl)
{
    Map *m = value_as_map(vl);
    int ni = map_size(m) - 1, i = 0;
    Value vs;
    vs.vc = ValueScalar;
    vs.type = vl->type;
    strbuf_adds(s,"{");
    FOR_MAP(iter,m) {        
        vs.v.ptr = iter->value;
        strbuf_addf(s,"\"%s\":",(char*)iter->key);
        dump_value(s,&vs);
        if (i++ < ni)
            strbuf_adds(s,",");
    }
    strbuf_adds(s,"}");    
}

void dump_array(SStr s, PValue vl)
{
    void **aa = (void**)value_as_array(vl);
    Value vs;
    vs.vc = ValueScalar;
    vs.type = vl->type;
    strbuf_adds(s,"{");
    int n = array_len(aa), ni = n - 1;
    FOR(i,n) {
        vs.v.ptr = aa[i];
        dump_value(s,&vs);
        if (i < ni)
            strbuf_adds(s,",");
    }
   strbuf_adds(s,"}");   
}

int main()
{
    PValue *va = array_new_ref(PValue,6);
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
    
    floatptr *ai = array_new(floatptr,2);
    ai[0] = 10.0;
    ai[1] = 20;
    va[5] = value_array(ai,ValueFloat);

    PValue v = value_array(va,ValueValue);    
    SStr s = strbuf_new();
    dump_value(s,v);
    printf("got '%s'\n",*s);
    
    PValue e = value_error("completely borked");
    if (value_is_error(e)) {
        printf("here is an error: %s\n",value_as_string(e));
    }
    dispose(v, s, e);
    printf("count = %d\n",obj_kount());
    return 0;
}

