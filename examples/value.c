/* This demonstrates a simple 'variant' type for containing semi-arbitrary
*   values, and how they can be streamed out in JSON format.
*/
#include <stdio.h>
#include <stdlib.h>
#include <llib/list.h>
#include <llib/str.h>
#include <llib/map.h>
#include <llib/value.h>

#include <stdarg.h>

void dump(void *p, void *q) { printf("[%p] %p\n",p,q); }

void count(int *pi, void *p) { *pi = *pi + 1; }

void obj_apply_v_varargs(void *o, PFun fn,va_list ap);

int count_varargs(va_list ap) {
    int n = 0;   
    obj_apply_v_varargs(&n,(PFun)count,ap);
    return n;
}

void put(void ***pv, void *p) { **pv = p; *pv = *pv + 1; }

PValue value_simple_map_(void *P,...) {
    int n;
    va_list ap;
    va_start(ap,P);
    n = count_varargs(ap);
    va_end(ap);

    void **ms = array_new(void*,n);
    void **pms = ms;
    va_start(ap,P);
    obj_apply_v_varargs(&pms,(PFun)put,ap);
    va_end(ap);
    
    PValue v = value_new(ValueValue,ValueSimpleMap);
    v->v.ptr = ms;
    return v;
}

PValue value_array_values_ (void *P,...) {
    int n;
    va_list ap;
    va_start(ap,P);
    n = count_varargs(ap);
    va_end(ap);

    void **ms = array_new_ref(void*,n);
    void **pms = ms;
    va_start(ap,P);
    obj_apply_v_varargs(&pms,(PFun)put,ap);
    va_end(ap);
    
    PValue v = value_new(ValueValue,ValueArray);
    v->v.ptr = ms;
    return v;
}

#define value_simple_map(...) value_simple_map_(NULL,__VA_ARGS__,NULL)
#define value_simple_array(...) value_array_values_(NULL,__VA_ARGS__,NULL)

#include <assert.h>

typedef char *Str, **SStr;

void dump_array(SStr s, PValue vl);
void dump_list(SStr s, PValue vl);
void dump_map(SStr s, PValue vl);
void dump_simple_map (SStr p, PValue vi);

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
            addf("\"%s\"",str);
            break;
        case ValueFloat:
            addf("%0.16g",f);
            break;
        case ValueNull:
            addf("null",ptr);
            break;
        case ValueBool:
            strbuf_adds(s, v->v.i ? "true" : "false");
            break;
        case ValueValue:
            dump_value(s,v->v.v);
            break;
        case ValuePointer:
        case ValueRef:
            if (v->v.ptr == NULL) {
                strbuf_adds(s,"null");
            } else {
                addf("%p",ptr);
            }
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
    case ValueSimpleMap:
        dump_simple_map(s,v);
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
    strbuf_add(s,'[');
    FOR_LIST(item,li) {        
        vs.v.ptr = item->data;
        dump_value(s,&vs);
        if (i++ < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,']');    
}

void dump_map(SStr s, PValue vl)
{
    Map *m = value_as_map(vl);
    int ni = map_size(m) - 1, i = 0;
    Value vs;
    vs.vc = ValueScalar;
    vs.type = vl->type;
    strbuf_add(s,'{');
    FOR_MAP(iter,m) {        
        vs.v.ptr = iter->value;
        strbuf_addf(s,"\"%s\":",(char*)iter->key);
        dump_value(s,&vs);
        if (i++ < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,'}');    
}

void dump_simple_map (SStr s, PValue vi)
{
    char **ms = (char**)vi->v.ptr;
    int n = array_len(ms)/2, i = 0;
    int ni = n - 1;
    Value vs;
    vs.vc = ValueScalar;
    vs.type = ValueValue;
    strbuf_add(s,'{');
    for (char **P = ms; *P; P += 2) {        
        vs.v.ptr = *(P+1);
        strbuf_addf(s,"\"%s\":",*P);
        dump_value(s,&vs);
        if (i++ < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,'}');    
}

void dump_array(SStr s, PValue vl)
{
    char *aa = (char*)value_as_array(vl);
    Value vs;
    vs.vc = ValueScalar;
    vs.type = vl->type;
    strbuf_add(s,'[');
    int n = array_len(aa), ni = n - 1;
    int nelem = obj_elem_size(aa);
    char *P = aa;
    FOR(i,n) {
        if (vs.type == ValueFloat) {
            double val;
            if (nelem == sizeof(float)) {
                val = *(float*)P;
            } else {
                val = *(double*)P;
            }
            vs.v.f = val;
        } else
        if (vs.type == ValueInt) {
            long long ival;
            switch (nelem) {
            case 1:  ival = *(unsigned char*)P; break;
            case 2: ival = *(short*)P; break;
            case 4: ival = *(int*)P; break;
            case 8: ival = *(long long *)P; break;
            default: ival = 0; break;  //??
            }
            vs.v.i = ival;
        } else {
            vs.v.ptr = *(void**)P;
        }
        dump_value(s,&vs);
        P += nelem;
        if (i < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,']');   
}

char *value_as_json(PValue v) {
    SStr s = strbuf_new();
    dump_value(s,v);
    return (char*)seq_array_ref(s);
}

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
    
    char *s = value_as_json(v);
    
    printf("got '%s'\n",s);
    
    PValue e = value_error("completely borked");
    if (value_is_error(e)) {
        printf("here is an error: %s\n",value_as_string(e));
    }
    dispose(v,s, e);
    
    #define VM   value_simple_map
    #define VI value_int
    #define VS value_string
    #define VF value_float
    #define VB value_bool
    #define VA value_simple_array
    
    v = VM(
        "one",VI(10),
        "two",VM("float",VF(1.2)),
        "three",VA(VI(1),VI(2),VI(3)),
        "four",VB(true)
    );
    s = value_as_json(v);
    
    printf("got '%s'\n",s);
    
    dispose(v, s);
    
    printf("count = %d\n",obj_kount());
    return 0;
}

