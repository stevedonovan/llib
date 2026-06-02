/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_VALUE_H
#define _LLIB_VALUE_H

#include "map.h"

typedef enum ValueType_ {
    ValueNull = 0,
    ValueRef = 0x100,
    ValueString = ValueRef + 1,
    ValueError= ValueRef + 2,
    ValueFloat = 1,
    ValueInt = 2,
    ValueValue = ValueRef + 3,
    ValuePointer = 3,
    ValueBool = 4
} ValueType;

typedef void *PValue;

static inline bool value_is_list(PValue v) {
    return list_object(v);
}

static inline bool value_is_array(PValue v) {
    return obj_is_array(v);
}

static inline bool value_is_map(PValue v) {
    return map_object(v);
}

#define value_errorf(fmt,...) value_error(str_fmt(fmt,__VA_ARGS__))

static inline int value_as_int(PValue v) {
    return (int)(*(long long*)v);
}

static inline double value_as_float(PValue v) {
    return *(double*)v;
}

static inline bool value_as_bool(PValue v) {
    return *(bool*)v;
}

static inline char *value_as_string(PValue v) {
    return (char*)v;
}

bool value_is_string(PValue v);
bool value_is_error(PValue v);
PValue value_error (const char *msg);
bool value_is_float(PValue v);
PValue value_float (double x);
bool value_is_int(PValue v);
PValue value_int (long long i);
bool value_is_bool(PValue v);
PValue value_bool (bool i);
bool value_is_box(PValue v);
bool value_is_simple_map(PValue v);

PValue value_parse(const char *str, ValueType type);
const char *value_tostring(PValue v);

#endif
