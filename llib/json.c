/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
### Generating and Reading JSON.

`json_parse_string` and `json_parse_file` will return an llib object, with maps as
'simple maps' (arrays where a string key is followed by a value), and
arrays containing values.

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

This header brings in these abbreviations, unless you explicitly define `LLIB_NO_VALUE_ABBREV`.

Constructing JSON data then is straightforward (note how primitive values need to be boxed):

    PValue v = VM(
        "one",VI(10),
        "two",VM("float",VF(1.2)),
        "three",VA(VI(1),VI(2),VI(3)),
        "four",VB(true)
    );

Look at `value` for boxing support.  See `test-json.c`.

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "list.h"
#include "str.h"
#include "map.h"
#include "scan.h"

#include "json.h"

typedef char *Str, **SStr;

static void dump_array(SStr s, PValue vl);
static void dump_list(SStr s, PValue vl);
static void dump_map(SStr s, PValue vl);
static void dump_simple_map (SStr p, PValue vi);

static void dump_value(SStr s, PValue v)
{
    if (v == NULL) {
        strbuf_adds(s,"null");
        return;
    }
    if (obj_refcount(v) == -1)  { // not one of ours, treat as integer
        strbuf_addf(s,"%d",(intptr_t)v);
        return;
    }

    int typeslot = obj_type_index(v);
    if (value_is_array(v)) {
        if (typeslot == OBJ_CHAR_T || typeslot == OBJ_ECHAR_T) {
            strbuf_addf(s,"\"%s\"",v);
        } else
        if (typeslot == OBJ_KEYVALUE_T) {
            dump_simple_map(s,v);
        } else {
            dump_array(s,v);
        }
        return;
    }

    switch (typeslot) {
    #define addf(fmt,T) strbuf_addf(s,fmt,*((T*)v))
    case OBJ_LLONG_T:
        addf("%d",int64);
        return;
    case OBJ_DOUBLE_T:
        addf("%0.16g",double);
        return;
    case OBJ_BOOL_T:
        strbuf_adds(s, (*(bool*)v) ? "true" : "false");
        return;
    #undef addf
    }
    // otherwise, containers!
    if (value_is_list(v)) {
        dump_list(s,v);
    } else
    if (value_is_map(v)) {
        dump_map(s,v);
    } else {
        strbuf_addf(s,"%s(%p)",obj_type(v)->name,v);
    }
}

static void dump_list(SStr s, PValue vl)
{
    List *li = (List*)vl;
    int ni = list_size(li) - 1, i = 0;
    strbuf_add(s,'[');
    FOR_LIST(item,li) {
        dump_value(s,item->data);
        if (i++ < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,']');
}

static void dump_map(SStr s, PValue vl)
{
    Map *m = (Map*)vl;
    int ni = map_size(m) - 1, i = 0;
    strbuf_add(s,'{');
    FOR_MAP(iter,m) {
        strbuf_addf(s,"\"%s\":",(char*)iter->key);
        dump_value(s,iter->value);
        if (i++ < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,'}');
}

static void dump_simple_map (SStr s, PValue vi)
{
    char **ms = (char**)vi;
    int n = array_len(ms)/2, i = 0;
    int ni = n - 1;
    strbuf_add(s,'{');
    for (char **P = ms; *P; P += 2) {
        strbuf_addf(s,"\"%s\":",*P);
        dump_value(s,*(P+1));
        if (i++ < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,'}');
}

static void dump_array(SStr s, PValue vl) {
    char *aa = (char*)vl;
    strbuf_add(s,'[');
    int n = array_len(aa), ni = n - 1;
    int nelem = obj_elem_size(aa);
    int type = obj_type_index(aa);
    char *P = aa;
    FOR(i,n) {
        void *data = *(void**)P;
        if (obj_refcount(data) == -1) {
            // must be a number...
            if (type == OBJ_FLOAT_T || type == OBJ_DOUBLE_T) {
                double val;
                if (nelem == sizeof(float)) {
                    val = *(float*)P;
                } else {
                    val = *(double*)P;
                }
                strbuf_addf(s,"%0.16g",val);
            } else {
                long long ival;
                switch (nelem) {
                case 1:  ival = *(unsigned char*)P; break;
                case 2: ival = *(short*)P; break;
                case sizeof(int): ival = *(int*)P; break;
                case 8: ival = *(int64*)P; break;
                default: ival = 0; break;  //??
                }
                strbuf_addf(s,"%d",ival);
            }
        } else {
            dump_value(s,data);
        }
        P += nelem;
        if (i < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,']');
}

/// convert an llib value rep into a JSON string.
// Lists, Maps, Simple Maps and Arrays are understood as containers.
// Arrays of primitives are properly handled.
char *json_tostring(PValue v) {
    SStr s = strbuf_new();
    dump_value(s,v);
    return (char*)seq_array_ref(s);
}

// important thing to remember about this parser is that it assumes that
// the token state has already been advanced with `scan_next`.
static PValue json_parse(ScanState *ts) {
    char *key;
    PValue val, err = NULL;
    int t = (int)ts->type;
    switch(t) {
    case '{':
    case '[': {
        void*** ss = seq_new_ref(void*);
        bool ismap = t == '{';
        int endt = ismap ? '}' : ']';  // corresponding close char
        int nfloats = 0;
        t = scan_next(ts);  // either close or first entry
        while (t != endt) { // while there are entries in map or list
            // a map has a key followed by a colon...
            if (ismap) {
                int tn;
                key = scan_get_str(ts);
                tn = scan_next(ts);
                if (t != T_STRING || tn != ':') {
                    obj_unref(key);
                    err = value_error("expected 'key':<value> in map");
                    break;
                }
                seq_add(ss,key);
                scan_next(ts); // after ':' is the value...
            }

            // the value returned may be an error...
            val = json_parse(ts);
            if (value_is_error(val)) {
                err = val;
                break;
            }
            seq_add(ss,val);
            if (value_is_float(val))
                ++nfloats;

            t = scan_next(ts); // should be separator or close
            if (t == ',') { // move to next item (key or value)
                t = scan_next(ts);
            } else
            if (t != endt) { // otherwise finished, or an error!
                err = value_errorf("expecting ',' or '%c', got '%c'",endt,t);
                break;
            }
        }
        if (err) {
            obj_unref(ss); // clean up the temporary array...
            return err;
        }
        void** vals = (void**)seq_array_ref(ss);
        if (ismap) {
            obj_type_index(vals) = OBJ_KEYVALUE_T;
        } else {
            int n = array_len(vals);
            if (n == nfloats) {
                double *arr = array_new(double,n);
                FOR(i,n)
                    arr[i] = value_as_float(vals[i]);
                obj_unref(vals);
                vals = (void**)arr;
            }
        }
        return (void*)vals;
    }
    case T_END:
        return value_error("unexpected end of stream");
    // scalars......
    case T_STRING:
        return scan_get_str(ts);
    case T_NUMBER:
        return value_float(scan_get_number(ts));
    case T_IDEN: {
        char buff[20];
        scan_get_tok(ts,buff,sizeof(buff));
        if (str_eq(buff,"null")) {
            return NULL;
        } else
        if (str_eq(buff,"true") || str_eq(buff,"false")) {
            return value_bool(str_eq(buff,"true"));
        } else {
            return value_errorf("unknown token '%s'",buff);
        }
    } default:
        return value_errorf("got '%c'; expecting '{' or '['",t);
    }
}

/// convert a string to JSON data.
// As a special optimization, arrays consisting only of numbers
// will be read in as primitive arrays of `double`.
PValue json_parse_string(const char *str) {
    PValue res;
    ScanState *st = scan_new_from_string(str);
    scan_next(st);
    res = json_parse(st);
    obj_unref(st);
    return res;
}

/// convert a file to JSON data.
PValue json_parse_file(const char *file) {
    PValue res;
    ScanState *st = scan_new_from_file(file);
    if (! st)
        return value_errorf("cannot open '%s'",file);
    scan_next(st);
    res = json_parse(st);
    obj_unref(st);
    return res;
}

