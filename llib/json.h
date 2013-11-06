/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef _LLIB_JSON_H
#define _LLIB_JSON_H

#include "value.h"

PValue value_array_values_ (intptr sm,...);

#define value_map_of_values(...) value_array_values_(1,__VA_ARGS__,NULL)
#define value_array_of_values(...) value_array_values_(0,__VA_ARGS__,NULL)
#define value_map_of_str(...) value_array_values_(3,__VA_ARGS__,NULL)
#define value_array_of_str(...) value_array_values_(2,__VA_ARGS__,NULL)


char *json_tostring(PValue v);
PValue json_parse_string(const char *str);
PValue json_parse_file(const char *file);

#ifdef LLIB_JSON_EASY
#define VM value_map_of_values
#define VMS value_map_of_str
#define VI value_int
#define VS value_str
#define VF value_float
#define VB value_bool
#define VA value_array_of_values
#define VAS value_array_of_str
#endif

#endif


