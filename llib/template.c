/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/**** 
### String Templates.

In their simplest form they allow substitution of placeholders like
`$(var)` with the value of `var` determined by a lookup, which is
defined as a function plus an object. The placeholders can be changed
if the default clashes with the target language syntax.

    StrTempl *st = `str_templ_new`("Hello $(P)$(name), how is $(home)?",NULL);
    // using an array of key/value pairs...(uses `str_lookup`)
    char *tbl1[] = {"name","Dolly","home","here","P","X",NULL};
    char *S = `str_templ_subst`(st,tbl1);
    assert(`str_eq`(S,"Hello XDolly, how is here?"));
    // using a map
    Map *m = `map_new_str_str`();
    `map_put`(m,"name","Monique");
    `map_put`(m,"home","Paris");
    `map_put`(m,"P","!");
    S = `str_templ_subst_using`(st,(StrLookup)map_get,m);
    assert(`str_eq`(S,"Hello !Monique, how is Paris?"));
    

Subtemplates can be defined; for instance this template generates an HTML list:

    "<ul>$(for ls |<li>_</li>)</ul>"
    
The special `for` form iterates over a List or an array. The special variable `\_` means 
"use each value of the array";  otherwise the _iterable_ must be a map-like object.

When expanding a template you pass a map-like object, either an actual `Map`
or an array of key/value pairs (a _simple map_).

See `test-template.c`
*/

#include <assert.h>
#include <stdio.h> // for now
#include "str.h"
#include "template.h"
#include "value.h"
#include "interface.h"

struct StrTempl_ {
    char *str;
    char **parts;
    StrTempl ***subt;  // all subtemplates
    // current substitution data
    StrLookup lookup;
    void *data;
    // non-NULL if we're a subtemplate
    StrTempl *parent;
};

static char*** builtin_funs;
static int templ_instances;

static void StrTempl_dispose (StrTempl *stl) {
    obj_unref_v(stl->str, stl->parts, stl->subt);
    --templ_instances;
    if (templ_instances == 0)
        obj_unref(builtin_funs);
}

#define MARKER '\01'

const char TVAR='\01',TFUN='\02',TTPL='\03',TMARKER='\04';

typedef char *Str;

static char *balanced(char *T, char openp, char closep) {
    char ch;
    int level = 1;
    while ((ch=*T) != 0 && level > 0) {
        if (ch == openp)
            ++level;
        else
        if (ch == closep)
            --level;
        ++T;
    }
    return T-1;  // *T must be closep
}

static char *spaces_left(char *p) {
    while (*p == ' ')
        --p;
    return p;
}

static char *advance_to(char *s, char ch) {
    while (*s && *s != ch)
        ++s;
    return s;
}

static void templ_initialize();

/// new template from string with variables to be expanded.
// `$(var)` is the default, if `markers` is NULL, but if `markers`
// was "@<>" then it would use '@' as the escape and "<>" as the
// brackets.
StrTempl *str_templ_new(const char *templ, const char *markers) {
    StrTempl *stl = obj_new(struct StrTempl_,StrTempl_dispose);
    ++templ_instances;
    templ_initialize();
    if (! markers)
        markers = "$()";
    char esc = markers[0], openp = markers[1], closep = markers[2];
    // we make our own copy of the string
    // and will break it up into this sequence
    Str **ss = seq_new(Str);
    stl->str = str_new(templ);
    stl->subt = NULL;
    stl->lookup = NULL;
    stl->data = NULL;
    stl->parent = NULL;
    char *T = stl->str;
    while (true) {
        char *S = strchr(T,esc), *p, *st, *s, mark;
        if (S)  *S = '\0';
        // plain string chunk
        seq_add(ss, T);
        if (! S)   break;
        // $(key) expansion
        T = S + 1;
        assert(*T == openp);
        // grabbing stuff inside $(...)
        S = balanced(T+1,openp,closep);
        assert(*S == closep);
        *S = '\0'; // closep!
        // might contain a subtemplate!
        p = spaces_left(S-1);
        if (*p == '|') { // right-hand boundary
            *p = '\0';
            mark = TTPL;
            char *s = advance_to(T+1,'|'); // left-hand boundary
            assert(*s == '|');
            st = s + 1; // now points to subtemplate
            s = spaces_left(s-1) + 1;
            *s = '\0';
           // st = s + 2;
        }  else {
            *(p+1) = '\0';
            st = NULL;
            mark = TVAR;
        }
        s = strchr(T,' ');
        if (s) {
            *s = '\0';
            if (mark == TVAR)
                mark = TFUN;
        }
        assert(*T == openp);
        *T = mark;
        seq_add(ss,T);
        if (st) {
            StrTempl *subt = str_templ_new(st,markers);
            subt->parent = stl;
            seq_add(ss,(char*)subt);
            // keep references to subtemplates for later disposal
            if (! stl->subt)
                stl->subt = seq_new_ref(StrTempl*);
            seq_add(stl->subt,subt);      
        }
        T = S + 1;
        if (! *T) break;
    }
    stl->parts = (Str*)seq_array_ref(ss);
    return stl;
}

static char *i_impl (void *arg, StrTempl *stl) {
    return str_fmt("%d", (intptr_t)arg);
}

static char *with_impl (void *arg, StrTempl *stl) {
    return str_templ_subst_using(stl,(StrLookup)interface_get_lookup(arg),arg);
}

static char *for_impl (void *arg, StrTempl *stl) {
    StrLookup mlookup = NULL;
    Iterator *a = interface_get_iterator(arg);
    Str *out = array_new_ref(Str,a->len);
    void *item;
    int i = 0;
    while (a->next(a,&item)) {
        if (i == 0) // assume all items have same type
            mlookup = (StrLookup)interface_get_lookup(item);
        out[i++] = str_templ_subst_using(stl,mlookup,item);
    }
    Str res = str_concat(out,"");
    obj_unref(out);
    obj_unref(a);
    return res;
}

static char *subst_using_parent(StrTempl *stl) {
    StrTempl *pst = stl->parent;
    return str_templ_subst_using(stl,pst->lookup,pst->data);
}

static bool picked = false;
static bool** if_stack = NULL;

static char *if_impl (void *arg, StrTempl *stl) {
    if (! if_stack)
        if_stack = seq_new(bool);
    seq_add(if_stack, picked);
    picked = arg != NULL;  // how we define truthiness....
    if (picked)
        return subst_using_parent(stl);
    else
        return str_new("");
}

static char *else_impl (void *arg, StrTempl *stl) {
    if (! picked)
        return subst_using_parent(stl);
    else
        return str_new("");
    picked = seq_pop(if_stack);
}

static void templ_initialize() {
    if (builtin_funs)
        return;
    builtin_funs = smap_new(false);
    str_templ_add_builtin("i", i_impl);
    str_templ_add_builtin("with", with_impl);
    str_templ_add_builtin("for", for_impl);
    str_templ_add_builtin("if", if_impl);
    str_templ_add_builtin("else", else_impl);
}

void str_templ_add_builtin(const char *name, TemplateFun fun) {
    smap_add(builtin_funs, name, (const void*)fun);
}

#define str_eq2(s1,s2) ((s1)[0]==(s2)[0] && (s1)[1]==(s2)[1])

/// substitute variables in template using a lookup function.
// `lookup` works with `data`, so that to use a Map you can pass
// `map_get` together with the map.
char *str_templ_subst_using(StrTempl *stl, StrLookup lookup, void *data) {
    int n = array_len(stl->parts);
    Str *out = array_new(Str,n);
    char *part;
    char mark = 0;
    char*** strs = NULL;
    stl->lookup = lookup;
    stl->data = data;

    FOR (i, n) { // over all the parts of the template
        // if it had a subtemplate, must skip it...
        if (mark == TTPL) {
            mark = 0;
            out[i] = "";
            continue;
        }
        part = stl->parts[i];
        mark= *part;
        if (mark > 0 && mark < TMARKER) {
            bool ref_str = false;
            ++part;
            if (mark != TVAR) { // function-style evaluation
                TemplateFun fn = (TemplateFun)smap_get(builtin_funs,part);
                char *arg = part + strlen(part) + 1;
                assert(fn != NULL);
                // always looks up the argument in current context, unless it's quoted
                if (*arg == '\"') {
                  ++arg;
                  arg[strlen(arg)-1] = '\0';
                } else if (! str_eq2(arg,"_")) {
                    arg = lookup (data, arg);
                } else {
                    arg = (char*)data;
                }
                part = fn(arg, (StrTempl*)stl->parts[i+1]);
                ref_str = true;
            } else {
                // note that '_' stands for the data, without lookup
                if (! str_eq2(part,"_")) {
                    char *res = lookup (data, part);
                    // which might fail; if there's a parent context, look there
                    if (! res && stl->parent) {
                        res = stl->parent->lookup(stl->parent->data,part);
                    }
                    part = res;                    
                } else {
                    part = (char*)data;
                }
            }
            if (! part)
                part = "";
            else if (value_is_box(part)) {
                part = (char*)value_tostring((PValue)part);
                ref_str = true;
            }
            if (ref_str) { // collect strings that we'll need to dispose later...
                if (! strs)
                    strs = seq_new_ref(char*);
                seq_add(strs,part);
            }
        }
        out[i] = part;
    }

    Str res = str_concat(out,NULL);
    obj_unref(out);
    obj_unref(strs);
    if (if_stack) {
        obj_unref(if_stack);
        if_stack = NULL;
    }
    return res;
}

/// substitute the variables using an array of keys and pairs.
// That is, a _simple map_.
char *str_templ_subst(StrTempl *stl, char **substs) {
    return str_templ_subst_using(stl,(StrLookup)str_lookup,substs);
}

/// substitute using boxed values.
// The value `v` can be any object supporting the `Accessor` interface,
// or a simple map.
char *str_templ_subst_values(StrTempl *st, PValue v) {
    StrLookup lookup = (StrLookup)interface_get_lookup(v);
    return str_templ_subst_using(st,lookup,v);
}
