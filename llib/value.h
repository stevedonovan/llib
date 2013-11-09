/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_VALUE_H
#define _LLIB_VALUE_H

#include "obj.h"

typedef enum ValueContainer_ {
    ValueScalar,
    ValueArray,
    ValueList,
    ValueMap,
    ValueSimpleMap,
} ValueContainer;

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

#ifndef _LLIB_LIST_H
struct List_;
typedef struct List_ List;
#define LLIB_LIST_TYPEDEF
#endif

#ifndef _LLIB_MAP_H
typedef struct List_ Map;
#define LLIB_MAP_TYPEDEF
#endif

typedef long long v_int_t;
typedef double v_float_t;

typedef struct Value_ {
    ValueContainer vc;
    ValueType type;
    union {
        const char *str;
        v_int_t i;
        v_float_t f;
        List *ls;
        Map *map;
        struct Value_ *v;
        void *ptr;
    } v;
} Value, *PValue;

#define value_is_error(v) ((v)->type==ValueError)
#define value_is_string(v) ((v)->type==ValueString)
#define value_is_float(v) ((v)->type==ValueFloat)
#define value_is_int(v) ((v)->type==ValueInt)
#define value_is_bool(v) ((v)->type==ValueBool)
#define value_is_value(v) ((v)->type==ValueValue)
#define value_is_ref() ((v)->type & ValueRef != 0)
#define value_is_list(v) ((v)->vc==ValueList)
#define value_is_array(v) ((v)->vc==ValueArray)
#define value_is_map(v) ((v)->vc==ValueMap)

#define value_as_string(vv) ((vv)->v.str)
#define value_as_int(vv) ((vv)->v.i)
#define value_as_bool value_as_int
#define value_as_float(vv) ((vv)->v.f)
#define value_as_value(vv) ((vv)->v.v)
#define value_as_array(vv) ((vv)->v.ptr)
#define value_as_list(vv) ((vv)->v.ls)
#define value_as_map(vv) ((vv)->v.map)
#define value_as_pointer(vv) ((vv)->v.ptr)

#define value_errorf(fmt,...) value_error(str_fmt(fmt,__VA_ARGS__))

PValue value_new(ValueType type, ValueContainer vc);
bool value_object(void *obj);
PValue value_str (const char *str);
PValue value_error (const char *msg);
PValue value_float (double x);
PValue value_int (long long i);
PValue value_bool (bool i);
PValue value_value (PValue V);
PValue value_list (List *ls, ValueType type);
PValue value_array (void *p, ValueType type);
PValue value_map (Map *m, ValueType type);

// for consistency
#define value_string value_str

PValue value_parse(const char *str, ValueType type);
const char *value_tostring(PValue v);

#endif
