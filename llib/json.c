/*
 * llib little C library
 * BSD licence
 * Copyright Steve Donovan, 2013
 */

/***
### Generating and Reading JSON.

`json_parse_string` and `json_parse_file` will return an llib object, with maps
as 'simple maps' (arrays where a string key is followed by a value), and arrays
containing values. They will return an error object if they fail.  If an array
only contains numbers, then they will be unboxed and the result is a simple
array of doubles.

`json_tostring` will convert llib values into JSON format.

This interface also defines convenient constructors for generating dynamic data
in the correct form for conversion to JSON:

    #ifndef LLIB_NO_VALUE_ABBREV
    #define VM value_map_of_values
    #define VMS value_map_of_str
    #define VI value_int
    #define VS str_ref
    #define VF value_float
    #define VB value_bool
    #define VA value_array_of_values
    #define VAS value_array_of_str
    #endif

This header brings in these abbreviations, unless you explicitly define
`LLIB_NO_VALUE_ABBREV`.

Constructing JSON data then is straightforward (note how primitive values need
to be boxed):

    PValue v = VM(
        "one",VI(10),
        "two",VM("float",VF(1.2)),
        "three",VA(VI(1),VI(2),VI(3)),
        "four",VB(true)
    );

Look at `value` for boxing support.  See `test-json.c`.

*/

#include "json.h"
#include "interface.h"
#include "str.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef char *Str, **SStr;

static void add_indent(SStr s, int indent) {
    FOR(i, indent) { strbuf_adds(s, "  "); }
}

static void dump_array(SStr s, PValue vl, int indent);
static void dump_value(SStr s, PValue v, int indent) {
    bool indenting = indent > 0;
    if (v == NULL) {
        strbuf_adds(s, "null");
        return;
    }
    if (obj_refcount(v) == -1) { // not one of ours, treat as integer
        strbuf_addf(s, "%d", (intptr_t)v);
        return;
    }

    int typeslot = obj_type_index(v);
    if (value_is_array(v)) {
        if (typeslot == OBJ_CHAR_T || typeslot == OBJ_ECHAR_T) {
            char *str = (char *)v;
            strbuf_add(s, '"');
            for (char *p = str; *p; p++) {
                switch (*p) {
                case '\\':
                    strbuf_adds(s, "\\\\");
                    break;
                case '"':
                    strbuf_adds(s, "\\\"");
                    break;
                case '\n':
                    strbuf_adds(s, "\\n");
                    break;
                case '\r':
                    strbuf_adds(s, "\\r");
                    break;
                case '\t':
                    strbuf_adds(s, "\\t");
                    break;
                default:
                    strbuf_add(s, *p);
                }
            }
            strbuf_add(s, '"');
            return;
        } else if (typeslot != OBJ_KEYVALUE_T) {
            dump_array(s, v, indent);
            return;
        }
    }

    // Object is Iterable?
    Iterator *iter = interface_get_iterator(v);
    if (iter) {
        int ni = iter->len;
        bool ismap = iter->nextpair != NULL;
        strbuf_add(s, ismap ? '{' : '[');
        if (indenting) {
            strbuf_add(s, '\n');
            ++indent;
        }
        FOR(i, ni) {
            PValue val, key;
            add_indent(s, indent);
            if (ismap) {
                iter->nextpair(iter, &key, &val);
                strbuf_addf(s, "\"%s\": ", key);
            } else {
                iter->next(iter, &val);
            }
            dump_value(s, val, indent);
            if (i < ni - 1)
                strbuf_add(s, ',');
            if (indenting)
                strbuf_add(s, '\n');
        }
        obj_unref(iter);
        add_indent(s, indent - 1);
        strbuf_add(s, ismap ? '}' : ']');
        // if (indenting) {
        //     strbuf_add(s, '\n');
        // }
    } else {
        switch (typeslot) {
#define addf(fmt, T) strbuf_addf(s, fmt, *((T *)v))
        case OBJ_LLONG_T:
            addf("%d", int64_t);
            return;
        case OBJ_DOUBLE_T:
            addf("%0.16g", double);
            return;
        case OBJ_BOOL_T:
            strbuf_adds(s, (*(bool *)v) ? "true" : "false");
            return;
        default:
            strbuf_addf(s, "%s(%p)", obj_typename(v), v);
            return;
#undef addf
        }
    }
}

static void dump_array(SStr s, PValue vl, int indent) {
    bool indenting = indent > 0;
    char *aa = (char *)vl;
    strbuf_add(s, '[');
    if (indenting) {
        strbuf_add(s, '\n');
        ++indent;
    }

    int n = array_len(aa), ni = n - 1;
    int nelem = obj_elem_size(aa);
    int type = obj_type_index(aa);
    char *P = aa;
    FOR(i, n) {
        add_indent(s, indent);
        void *data = *(void **)P;
        if (obj_refcount(data) == -1) {
            // must be a number...
            if (type == OBJ_FLOAT_T || type == OBJ_DOUBLE_T) {
                double val;
                if (nelem == sizeof(float)) {
                    val = *(float *)P;
                } else {
                    val = *(double *)P;
                }
                strbuf_addf(s, "%0.16g", val);
            } else {
                long long ival;
                switch (nelem) {
                case 1:
                    ival = *(unsigned char *)P;
                    break;
                case 2:
                    ival = *(short *)P;
                    break;
                case sizeof(int):
                    ival = *(int *)P;
                    break;
                case 8:
                    ival = *(int64_t *)P;
                    break;
                default:
                    ival = 0;
                    break; //??
                }
                strbuf_addf(s, "%d", ival);
            }
        } else {
            dump_value(s, data, indent);
        }
        P += nelem;
        if (i < ni)
            strbuf_add(s, ',');
        if (indenting)
            strbuf_add(s, '\n');
    }
    if (indenting)
        --indent;
    add_indent(s, indent);
    strbuf_add(s, ']');
}

/// convert an llib value rep into a JSON string.
// Lists, Maps, Simple Maps and Arrays are understood as containers.
// Arrays of primitives are properly handled.
char *json_tostring(PValue v) { return json_to_pretty_string(v, 0); }

/// convert value into pretty JSON string
char *json_to_pretty_string(PValue v, int indent) {
    SStr s = strbuf_new();
    add_indent(s, indent);
    dump_value(s, v, indent);
    return (char *)seq_array_ref(s);
}
