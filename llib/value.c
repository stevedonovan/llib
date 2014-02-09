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

bool value_is_box(PValue v) {
    return array_len(v)==1 && ! obj_is_array(v);
}

static bool check_type(PValue v, int ttype) {
    return value_is_box(v) && obj_type_index(v) == ttype;
}

bool value_is_string(PValue v) {
    return obj_is_array(v) && obj_type_index(v) == OBJ_CHAR_T;
}

bool value_is_error(PValue v) {
    return obj_is_array(v) && obj_type_index(v) == OBJ_ECHAR_T;
}

bool value_is_float(PValue v) {
    return check_type(v,OBJ_DOUBLE_T);
}

bool value_is_int(PValue v) {
    return check_type(v,OBJ_LLONG_T);
}

bool value_is_bool(PValue v) {
    return check_type(v,OBJ_BOOL_T);
}

bool value_is_simple_map(PValue v) {
    return obj_type_index(v) == OBJ_KEYVALUE_T;
}

void obj_set_type(void *P,int t) {
    ObjHeader* h = obj_header_(P);
    h->type = t;
}

PValue value_error (const char *msg) {
    PValue v = str_new(msg);
    obj_set_type(v,OBJ_ECHAR_T);
    return v;
}

PValue value_float (double x) {
    double *px = array_new(double,1);
    *px = x;
    obj_is_array(px) = 0;
    return (PValue)px;
}

PValue value_int (long long i) {
    long long *px = array_new(long long,1);
    *px = i;
    obj_is_array(px) = 0;
    return (PValue)px;
}

PValue value_bool (bool i) {
    bool *px = array_new(bool,1);
    *px = i;
    obj_is_array(px) = 0;
    return (PValue)px;
}

#define str_eq(s1,s2) (strcmp((s1),(s2))==0)

static PValue conversion_error(const char *s, const char *t) {
    char buff[50];
    snprintf(buff,sizeof(buff),"'%s' is not a valid %s",s,t);
    return value_error(buff);
}

//. convert a string into a value of the desired type.
PValue value_parse(const char *str, ValueType type) {
    long long ival;
    double fval;
    char *endptr;
    switch(type) {
    case ValueString:
        return (void*)str_new(str);
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
        return NULL;//*
    default:
        return value_error("cannot parse this type");
    }
}

#define S str_new

#ifdef _WIN32
#define LONG_LONG_FMT "%I64d"
#else
#define LONG_LONG_FMT "%lld"
#endif

/// Default representation of a value as a string.
// Only applies to scalar values (if you want to show arrays & maps
// use json module).
const char *value_tostring(PValue v) {
    char buff[65];
    if (v == NULL)
        return S("null");
    if (obj_refcount(v) == -1)
        return S("<NAO>");  // Not An Object
    int typeslot = obj_type_index(v);
    if (!  value_is_array(v)) {
        switch(typeslot) {
        #define outf(fmt,T) snprintf(buff,sizeof(buff),fmt,*((T*)v))
        case OBJ_LLONG_T:
            outf(LONG_LONG_FMT,int64);
            break;
        case OBJ_DOUBLE_T:
            outf("%0.16g",double);
            break;
        case OBJ_BOOL_T:
            return S((*(bool*)v) ? "true" : "false");
        }
    } else if (typeslot == OBJ_CHAR_T || typeslot == OBJ_ECHAR_T) {
        return str_ref((char*)v);  // already a string object
    } else {
        snprintf(buff,sizeof(buff),"%s(%p)",obj_type(v)->name,v);
    }
    return S(buff);
}

