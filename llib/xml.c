/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/// a simple XML parser and pretty-printer.
// Intended as a small portable way to access common configuration file formats.
// not for heavy lifting!

#include <stdio.h>
#include "scan.h"
#include "value.h"
#include "str.h"

typedef char *Str;

static PValue not_expecting(int t, int x) {
    if (t != x)
        return value_errorf("expected '%c' got '%c'",x,t);
    else
        return NULL;
}

static char buff[512];

static Str get_tag_name(ScanState *ts) {
    Str *res = strbuf_new();
    while (true) {
        Str name = scan_get_str(ts);
        strbuf_adds(res,name);
        int t = scan_next(ts);
        if (t != '-' && t != ':')
            break;
        strbuf_add(res,t);
        scan_next(ts);
    }
    return strbuf_tostring(res);
}

#define expect(t,ch) if ((err = not_expecting(t,ch)) != NULL)   goto cleanup;

static PValue parse_xml(ScanState *ts, bool is_data) {
    PValue err;
    if (ts->type != '<')
        return value_error("expecting '<'");
 again:
    scan_next(ts);
    if (ts->type != T_IDEN) {
        if (ts->type == '!') {
            scan_get_upto(ts,"-->",buff,sizeof(buff));
            scan_next(ts);
            goto again;
        } else
            return value_error("expecting name after '<'");
    }
    PValue ** elem = seq_new(PValue);
    char*** attribs = NULL;
    Str name = get_tag_name(ts);
    if (ts->type == T_IDEN) { // attributes!
        attribs = smap_new(true);
        do {
            Str key, value;
            key = get_tag_name(ts);
            expect(ts->type,'=');
            //~ expect(scan_next(ts),'=');
            if (scan_next(ts) != T_STRING) {
                err = value_error("expecting quoted value after =");
                goto cleanup;
            }
            value = scan_get_str(ts);
            smap_put(attribs, key,value);
            scan_next(ts);
            if (! (ts->type == '>' || ts->type == T_IDEN)) {
                err = value_errorf("expected '>' or IDEN, got '%c'",ts->type);
                goto cleanup;
            }
        } while (ts->type == T_IDEN);
    }
    if (ts->type == '>') {
        while (true) {
            if (scan_peek(ts,0) != '<') { // there will be text
                char **ss = seq_new(char);
                char ch;
                while ((ch = scan_getch(ts)) != '<') {
                    seq_add(ss,ch);
                }
                scan_advance(ts,-1);
                Str txt = (Str)seq_array_ref(ss);
                if (is_data && ! str_is_blank(txt)) {
                    seq_add(elem,txt);
                } else {
                    obj_unref(txt);
                }
            }
            scan_next(ts);
            expect(ts->type,'<');
            if (scan_peek(ts,0) == '/') { // finished...
                scan_next(ts);
                break;
            }
            PValue e = parse_xml(ts, is_data);
            if (value_is_error(e)) {
                err = e;
                goto cleanup;
            }
            seq_add(elem, e);
        }
    }
    if (ts->type == '/') {
        scan_next(ts);
        if (ts->type == T_IDEN) {
            Str mname = get_tag_name(ts);
            if (! str_eq(mname,name)) {
                err = value_errorf("closing '%s' with '%s'",name,mname);
                goto cleanup;
            }
            //scan_next(ts);
        }
        expect(ts->type,'>');

        seq_insert(elem,0,&name,1);
        if (attribs) {
            Str *attr = smap_close(attribs);
            seq_insert(elem,1,&attr,1);
        }
        return seq_array_ref(elem);
    } else
        err = not_expecting(ts->type,'/');
    cleanup:
    obj_unref_v(name, attribs, elem);
    return err;
}

#undef expect

static PValue parse_xml_from_scan(ScanState *ts, bool is_data) {
    PValue res;
    scan_next(ts);
    if (scan_peek(ts,0) == '?') {
        char ch;
        while ((ch=scan_getch(ts)) != '<') ;
        scan_advance(ts,-1);
        scan_next(ts);
    }
    res = parse_xml(ts,is_data);
    obj_unref(ts);
    return res;
}

/// convert an XML string to data.
PValue xml_parse_string(const char *str, bool is_data) {
    return parse_xml_from_scan(scan_new_from_string(str),is_data);
}

/// convert an XML file to data.
PValue xml_parse_file(const char *file, bool is_data) {
    ScanState *st = scan_new_from_file(file);
    if (! st)
        return value_errorf("cannot open '%s'",file);
    return parse_xml_from_scan(st,is_data);
}

/// tag name of an element.
const char *xml_tag(PValue* doc) {
    return (const char*)doc[0];
}

/// attributes of an element (as an SMap).
char** xml_attribs(PValue* doc) {
    if (doc[1] && value_is_simple_map(doc[1]))
        return (char**)doc[1];
    return NULL;
}

/// children nodes of an element; returns length in `plen`.
PValue *xml_children(PValue* doc, int* plen) {
    PValue *res = doc + 1;
    *plen = array_len(doc);
    if (! res) return NULL;
    if (value_is_simple_map(*res)) {
        ++res;
        --(*plen);
    }
    return res;
}

static void tostring_xml(PValue data, char** ss, int level, int indent) {
    int n;
    PValue *doc = (PValue*)data;
    const char *name = xml_tag(doc);
    strbuf_add(ss, '<');
    strbuf_adds(ss, name);
    char **attr = xml_attribs(doc);
    PValue *kids = xml_children(doc,&n);
    if (attr) { // how a 'simple map' is laid out
        for (; *attr; attr += 2) {
            strbuf_addf(ss," %s='%s'",*attr,*(attr+1));
        }
    }
    if (kids) {
        strbuf_add(ss, '>');
        if (indent)
            strbuf_addf(ss,"\n%.*c",level*indent,' '); //"                                       ");
        for (; *kids; ++kids) {
            PValue elem = *kids;
            if (value_is_string(elem))
                strbuf_adds(ss, (const char*)elem);
            else
                tostring_xml(elem, ss, level+1, indent);
        }
        if (indent)
            strbuf_addf(ss,"\n%.*c",level*indent,' '); //"                                       ");

        strbuf_addf(ss, "</%s>",name);
    } else {
        strbuf_adds(ss,"/>");
    }
}

/// Pretty-print an XML document
char *xml_tostring(PValue doc, int indent) {
    char** ss = strbuf_new();
    tostring_xml(doc,ss,1, indent);
    return strbuf_tostring(ss);
}

