/*
 * llib little C library
 * BSD licence
 * Copyright Steve Donovan, 2013
 */

/// Parsing JSON
// @submodule json

#include "json.h"
#include "scan.h"
#include "str.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// important thing to remember about this parser is that it assumes that
// the token state has already been advanced with `scan_next`.
static PValue json_parse(ScanState *ts, bool vs5) {
    char *key;
    PValue val, err = NULL;
    int t = (int)ts->type;
    switch (t) {
    case '{':
    case '[': {
        void ***ss = seq_new_ref(void *);
        bool ismap = t == '{';
        int endt = ismap ? '}' : ']'; // corresponding close char
        int nfloats = 0;
        t = scan_next(ts);  // either close or first entry
        while (t != endt) { // while there are entries in map or list
            // a map has a key followed by a colon...
            if (ismap) {
                int tn;
                key = scan_get_str(ts);
                tn = scan_next(ts);
                if ((t != T_STRING && !vs5) || tn != ':') {
                    obj_unref(key);
                    err = value_error("expected 'key':<value> in map");
                    break;
                }
                seq_add(ss, key);
                scan_next(ts); // after ':' is the value...
            }

            // the value returned may be an error...
            val = json_parse(ts, vs5);
            if (value_is_error(val)) {
                err = val;
                break;
            }
            seq_add(ss, val);
            if (value_is_float(val))
                ++nfloats;

            t = scan_next(ts); // should be separator or close
            if (t == ',') {    // move to next item (key or value)
                t = scan_next(ts);
            } else if (t != endt && !vs5) { // otherwise finished, or an error!
                err = value_errorf("expecting ',' or '%c', got '%c'", endt, t);
                break;
            }
        }
        if (err) {
            obj_unref(ss); // clean up the temporary array...
            return err;
        }
        void **vals = (void **)seq_array_ref(ss);
        if (ismap) {
            obj_type_index(vals) = OBJ_KEYVALUE_T;
        } else {
            int n = array_len(vals);
            if (n == nfloats && !vs5) {
                double *arr = array_new(double, n);
                FOR(i, n)
                arr[i] = value_as_float(vals[i]);
                obj_unref(vals);
                vals = (void **)arr;
            }
        }
        return (void *)vals;
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
        scan_get_tok(ts, buff, sizeof(buff));
        if (str_eq(buff, "null")) {
            return NULL;
        } else if (str_eq(buff, "true") || str_eq(buff, "false")) {
            return value_bool(str_eq(buff, "true"));
        } else {
            return value_errorf("unknown token '%s'", buff);
        }
    }
    default:
        return value_errorf("got '%c'; expecting '{' or '['", t);
    }
}

PValue parse_json(ScanState *st, bool vs5) {
    PValue res;
    scan_next(st);
    res = json_parse(st, vs5);
    if (value_is_error(res)) {
        PValue err = value_errorf("line %d: %s", st->line, res);
        obj_unref(res);
        res = err;
    }
    return res;
}

/// convert a string to JSON data.
// As a special optimization, arrays consisting only of numbers
// will be read in as primitive arrays of `double`.
PValue json_parse_string(const char *str) {
    PValue res;
    ScanState *st = scan_new_from_string(str);
    scan_set_flags(st, C_STRING);
    res = parse_json(st, false);
    obj_unref(st);
    return res;
}

/// convert a string to JSON data using extended JSON5 format
PValue json_parse_string_vs5(const char *str) {
    PValue res;
    ScanState *st = scan_new_from_string(str);
    scan_set_flags(st, C_IDEN | C_STRING);
    scan_set_line_comment(st, "//");
    res = parse_json(st, true);
    obj_unref(st);
    return res;
}

/// convert a file to JSON data.
PValue json_parse_file(const char *file) {
    PValue res;
    ScanState *st = scan_new_from_file(file);
    if (!st)
        return value_errorf("cannot open '%s'", file);
    scan_set_flags(st, C_STRING);
    res = parse_json(st, false);
    obj_unref(st);
    return res;
}

/// convert a file to JSON data using extended JSON5 format
PValue json_parse_file_vs5(const char *file) {
    PValue res;
    ScanState *st = scan_new_from_file(file);
    if (!st)
        return value_errorf("cannot open '%s'", file);
    scan_set_flags(st, C_IDEN | C_STRING);
    scan_set_line_comment(st, "//");
    res = parse_json(st, true);
    obj_unref(st);
    return res;
}
