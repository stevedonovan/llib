/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#define strtoll _strtoi64
#define snprintf _snprintf
#endif

static void Value_dispose(PValue v) {
    if (v->vc != ValueScalar || (v->type & ValueRef)) {
        obj_unref(v->v.ptr);
    }
}

bool value_object(void *obj) {
    return obj_is_instance(obj,"Value");
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

PValue value_bool (bool i) {
    PValue v = value_new(ValueBool,ValueScalar);
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

#define str_eq(s1,s2) (strcmp((s1),(s2))==0)

static PValue conversion_error(const char *s, const char *t) {
    char buff[50];
    snprintf(buff,sizeof(buff),"'%s' is not a valid %s\n",s,t);
    return value_error(buff);
}

// convert a string into a value of the desired type
PValue value_parse(const char *str, ValueType type) {
    v_int_t ival;
    v_float_t fval;
    char *endptr;
    switch(type) {
    case ValueString:
        return value_str(str);
    case ValueInt:
        ival = strtoll(str, &endptr,10);
        if (*endptr)
            return conversion_error(endptr,"int");
        return value_int(ival);
    case ValueFloat:
        fval = strtod(str, &endptr);
        if (*endptr)
            return conversion_error(endptr,"float");
        return value_float(fval);
    case ValueBool:
        return value_bool(str_eq(str,"true"));
    case ValueNull:
        if (! str_eq(str,"null"))
            return value_error("only 'null' allowed");
        return value_new(ValueNull,ValueScalar);
    default:
        return value_error("cannot parse this type");
    }
}

// Default representation of a value as a string.
// Only applies to scalar values (if you want to show arrays & maps
// use json module). Returns a pointer to a static buffer, so don't mess with it!
const char *value_tostring(PValue v) {
    static char buff[35];
    if (v->vc != ValueScalar)
        return "<not a scalar>";
    switch(v->type) {
    #define outf(fmt,F) snprintf(buff,sizeof(buff),fmt,v->v.F),buff
    case ValueInt:
        return outf("%d",i);
    case ValueString:
    case ValueError:
        return v->v.str;
    case ValueFloat:
        return outf("%0.16g",f);
    case ValueNull:
        return "null";
    case ValueBool:
        return v->v.i ? "true" : "false";
    case ValueValue:
        return value_tostring(v->v.v);
    case ValuePointer:
    case ValueRef:
        if (v->v.ptr == NULL) {
            return "<null>";
        } else {
            return outf("%p",ptr);
        }
    default:
        return "?";
    }
}
