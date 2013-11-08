/* This demonstrates a simple 'variant' type for containing semi-arbitrary
*   values, and how they can be streamed out in JSON format.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "list.h"
#include "str.h"
#include "map.h"
#include "scan.h"

#include "json.h"

PValue value_array_values_ (intptr sm,...) {
    int n = 0, i = 0;
    int ismap = sm & 0x1;
    int isstr = sm & 0x2;
    void *P;
    va_list ap;
    va_start(ap,sm);
    while ((P = va_arg(ap,void*)) != NULL)
        ++n;
    va_end(ap);

    void** ms = array_new_ref(void*,n);
    va_start(ap,sm);
    while ((P = va_arg(ap,void*)) != NULL)  {
        if ((ismap && i % 2 == 0) || isstr)
            P = str_cpy((char*)P); // C++ being picky here...
        ms[i++] = P;
    }
    va_end(ap);

    PValue v = value_new(isstr ? ValueString : ValueValue,ismap ? ValueSimpleMap : ValueArray);
    v->v.ptr = ms;
    return v;
}

typedef char *Str, **SStr;

static void dump_array(SStr s, PValue vl);
static void dump_list(SStr s, PValue vl);
static void dump_map(SStr s, PValue vl);
static void dump_simple_map (SStr p, PValue vi);

static void dump_value(SStr s, PValue v)
{
    switch (v->vc) {
    case ValueScalar:
        switch (v->type) {
        #define addf(fmt,F) strbuf_addf(s,fmt,v->v.F)
        case ValueInt:
            addf("%d",i);
            break;
        case ValueString:
        case ValueError:
            addf("\"%s\"",str);
            break;
        case ValueFloat:
            addf("%0.16g",f);
            break;
        case ValueNull:
            addf("null",ptr);
            break;
        case ValueBool:
            strbuf_adds(s, v->v.i ? "true" : "false");
            break;
        case ValueValue:
            dump_value(s,v->v.v);
            break;
        case ValuePointer:
        case ValueRef:
            if (v->v.ptr == NULL) {
                strbuf_adds(s,"null");
            } else {
                addf("%p",ptr);
            }
            break;
        }
        #undef addf
        break;
    case ValueList:
        dump_list(s,v);
        break;
    case ValueArray:
        dump_array(s,v);
        break;
    case ValueMap:
        dump_map(s,v);
        break;
    case ValueSimpleMap:
        dump_simple_map(s,v);
        break;
    }
}

static void dump_list(SStr s, PValue vl)
{
    List *li = value_as_list(vl);
    int ni = list_size(li) - 1, i = 0;
    Value vs;
    vs.vc = ValueScalar;
    vs.type = vl->type;
    strbuf_add(s,'[');
    FOR_LIST(item,li) {
        vs.v.ptr = item->data;
        dump_value(s,&vs);
        if (i++ < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,']');
}

static void dump_map(SStr s, PValue vl)
{
    Map *m = value_as_map(vl);
    int ni = map_size(m) - 1, i = 0;
    Value vs;
    vs.vc = ValueScalar;
    vs.type = vl->type;
    strbuf_add(s,'{');
    FOR_MAP(iter,m) {
        vs.v.ptr = iter->value;
        strbuf_addf(s,"\"%s\":",(char*)iter->key);
        dump_value(s,&vs);
        if (i++ < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,'}');
}

static void dump_simple_map (SStr s, PValue vi)
{
    char **ms = (char**)vi->v.ptr;
    int n = array_len(ms)/2, i = 0;
    int ni = n - 1;
    Value vs;
    vs.vc = ValueScalar;
    vs.type = ValueValue;
    strbuf_add(s,'{');
    for (char **P = ms; *P; P += 2) {
        vs.v.ptr = *(P+1);
        strbuf_addf(s,"\"%s\":",*P);
        dump_value(s,&vs);
        if (i++ < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,'}');
}

static void dump_array(SStr s, PValue vl)
{
    char *aa = (char*)value_as_array(vl);
    Value vs;
    vs.vc = ValueScalar;
    vs.type = vl->type;
    strbuf_add(s,'[');
    int n = array_len(aa), ni = n - 1;
    int nelem = obj_elem_size(aa);
    char *P = aa;
    FOR(i,n) {
        if (vs.type == ValueFloat) {
            double val;
            if (nelem == sizeof(float)) {
                val = *(float*)P;
            } else {
                val = *(double*)P;
            }
            vs.v.f = val;
        } else
        if (vs.type == ValueInt) {
            long long ival;
            switch (nelem) {
            case 1:  ival = *(unsigned char*)P; break;
            case 2: ival = *(short*)P; break;
            case 4: ival = *(int*)P; break;
            case 8: ival = *(long long *)P; break;
            default: ival = 0; break;  //??
            }
            vs.v.i = ival;
        } else {
            vs.v.ptr = *(void**)P;
        }
        dump_value(s,&vs);
        P += nelem;
        if (i < ni)
            strbuf_add(s,',');
    }
    strbuf_add(s,']');
}

// convert a Value rep of a JSON object to a string.
char *json_tostring(PValue v) {
    SStr s = strbuf_new();
    dump_value(s,v);
    return (char*)seq_array_ref(s);
}

// important thing to remember about this parser is that it assumes that
// the token state has already been advanced with scan_next.
static PValue json_parse(ScanState *ts) {
    char *key;
    PValue val, err = NULL;
    int t = (int)ts->type;
    switch(t) {
    case '{':
    case '[': {
        void*** ss = seq_new_ref(void*);
        int endt = t == '{' ? '}' : ']';  // corresponding close char        
        t = scan_next(ts);  // either close or first entry
        while (t != endt) { // while there are entries in map or list
            // a map has a key followed by a colon...
            if (endt == '}') { 
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
        PValue v = value_new(ValueValue,t=='}' ? ValueSimpleMap : ValueArray);
        v->v.ptr = seq_array_ref(ss);
        return v;
    }
    case T_END:
        return value_error("unexpected end of stream");
    // scalars......
    case T_STRING:
        return value_str(scan_get_str(ts));
    case T_NUMBER:
        return value_float(scan_get_number(ts));
    case T_IDEN: {
        char buff[20];
        scan_get_tok(ts,buff,sizeof(buff));
        if (str_eq(buff,"null")) {
            return value_new(ValueNull,ValueScalar);
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

// convert a string to JSON data.
PValue json_parse_string(const char *str) {
    PValue res;
    ScanState *st = scan_new_from_string(str);
    scan_next(st);
    res = json_parse(st);
    obj_unref(st);
    return res;
}

// convert a file to JSON data.
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

