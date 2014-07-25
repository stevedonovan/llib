/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
### Boxing and Unboxing Primitive Types.

All llib objects are by defiintion _values_;  primitive types like ints (`long long`) ,
floats (`double`) and bools must be _boxed_ first.  Boxed types are pointers-to-primitive,
which are not arrays; `value_is_box` becomes true.  To box a value use `value_int`,
`value_float` and `value_bool`;  to check the type use the equivalent `value_is_TYPE`
functions, and to extract the value use `value_as_TYPE`.

Error values are strings with a distinct type, so we have `value_error` and  `value_is_error`.

`value_parse` converts strings to values using a known value type; `value_tostring` will
convert a value to a default string representation.  More complicated value types like arrays
don't have a unique representation as strings, so see `json_tostring` and `xml_tostring`.

See `test-json.c` for how values are used in practice.

*/

#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#define strtoll _strtoi64
#define snprintf _snprintf
#endif

/// Querying Type
// @section querying

/// is this a boxed value?
bool value_is_box(PValue v) {
    return v!=NULL && array_len(v)==1 && ! obj_is_array(v);
}

static bool check_type(PValue v, int ttype) {
    return value_is_box(v) && obj_type_index(v) == ttype;
}

/// is this value a string?
bool value_is_string(PValue v) {
    return v!=NULL && obj_is_array(v) && obj_type_index(v) == OBJ_CHAR_T;
}

/// is this value an error string?
bool value_is_error(PValue v) {
    return v!=NULL && obj_is_array(v) && obj_type_index(v) == OBJ_ECHAR_T;
}

/// does this value contain a `double`?
bool value_is_float(PValue v) {
    return check_type(v,OBJ_DOUBLE_T);
}

/// does this value contain a `long long`?
bool value_is_int(PValue v) {
    return check_type(v,OBJ_LLONG_T);
}

/// does this value contain a `bool`?
bool value_is_bool(PValue v) {
    return check_type(v,OBJ_BOOL_T);
}

/// does this value represent a _simple map_?
bool value_is_simple_map(PValue v) {
    return v!=NULL && obj_type_index(v) == OBJ_KEYVALUE_T;
}

static void obj_set_type(PValue P,int t) {
    ObjHeader* h = obj_header_(P);
    h->type = t;
}

/// Boxing Values
// @section Boxing

/// make a error value.
PValue value_error (const char *msg) {
    PValue v = str_new(msg);
    obj_set_type(v,OBJ_ECHAR_T);
    return v;
}

/// box a `double`.
PValue value_float (double x) {
    double *px = array_new(double,1);
    *px = x;
    obj_is_array(px) = 0;
    return (PValue)px;
}

/// box a `long long`.
PValue value_int (long long i) {
    long long *px = array_new(long long,1);
    *px = i;
    obj_is_array(px) = 0;
    return (PValue)px;
}

/// box a `bool`.
PValue value_bool (bool i) {
    bool *px = array_new(bool,1);
    *px = i;
    obj_is_array(px) = 0;
    return (PValue)px;
}

/// Converting to and from Strings
// @section converting

#define str_eq(s1,s2) (strcmp((s1),(s2))==0)

static PValue conversion_error(const char *s, const char *t) {
    char buff[50];
    snprintf(buff,sizeof(buff),"'%s' is not a valid %s",s,t);
    return value_error(buff);
}

/// convert a string into a value of the desired type.
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
        return value_bool(str_eq(str,"true") || str_eq(str,"1"));
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
        snprintf(buff,sizeof(buff),"%s(%p)",obj_typename(v),v);
    }
    return S(buff);
}

