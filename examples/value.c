#include <stdio.h>
#include <stdlib.h>
#include <llib/obj.h>
#include <llib/list.h>

typedef enum ValueType_ {
    ValueNone,
    ValueString,
    ValueFloat,
    ValueInt,
    ValueList,
    ValueValue
} ValueType;

typedef struct Value_ {
    ValueType type;
    union {
        const char *str;
        long long i;
        double f;
        List *ls;
        struct Value_ *v;
        void *ptr;
    } v;
} Value, *PValue;

void Value_dispose(PValue v) {
    if (v->type == ValueString || v->type == ValueList || v->type == ValueValue) {
        printf("disposing %d\n",v->type);
        obj_unref(v->v.ptr);
    }
}

PValue value_new(ValueType type) {
    PValue v = obj_new(Value,Value_dispose);
    v->type = type;
    return v;
}

PValue value_str (const char *str) {
    PValue v = value_new(ValueString);
    v->v.str = str_cpy(str);
    return v;
}

PValue value_float (double x) {
    PValue v = value_new(ValueFloat);
    v->v.f = x;
    return v;
}

PValue value_int (long long i) {
    PValue v = value_new(ValueInt);
    v->v.i = i;
    return v;
}

PValue value_value (PValue V) {
    PValue v = value_new(ValueValue);
    v->v.v = V;
    return v;
}

PValue value_list (List *ls) {
    PValue v = value_new(ValueList);
    v->v.ls = ls;
    return v;
}

/*
void dump_value(PValue v) {
    if (v->type == ValueList) {
        printf("[");
        FOR_LIST(item,v->v.ls) {
            dump_value((PValue)item->data)
        }
    }
}
*/

int main()
{
    List *lv = list_new_ref();

    PValue vs = value_str("hello dolly");
    PValue  vf = value_float(4.2);
    printf("vs = %s vf = %f\n",vs->v.str, vf->v.f);
    
    list_add_items(lv,vs,vf);
    
    List *ls = list_new_str();
    list_add_items(ls, "bonzo","dog");
    list_add(lv, value_list(ls));    
    
    PValue lsv = value_list(lv);
    dispose(lsv);
    printf("count = %d\n",obj_kount());
    return 0;
}

