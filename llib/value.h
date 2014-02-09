/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_VALUE_H
#define _LLIB_VALUE_H

#include "obj.h"

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

#define value_is_list(v) list_object(v)
#define value_is_array(v) obj_is_array(v)
#define value_is_map(v) map_object(v)

#define value_errorf(fmt,...) value_error(str_fmt(fmt,__VA_ARGS__))

#define value_as_int(P) (int)(*(long long*)(P))
#define value_as_float(P) (*(double*)(P))
#define value_as_bool(P) (*(bool*)(P))
#define value_as_string(P) ((char*)P)

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
