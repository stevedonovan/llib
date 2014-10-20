/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/**** 
### String Templates.

In their simplest form they allow substitution of placeholders like
`$(var)` with the value of `var` determined by a lookup, which is
defined as a function plus an object.

    StrTempl *st = `str_templ_new`("Hello $(P)$(name), how is $(home)?",NULL);
    // using an array of key/value pairs...(uses `str_lookup`)
    char *tbl1[] = {"name","Dolly","home","here","P","X",NULL};
    char *S = `str_templ_subst`(st,tbl1);
    assert(`str_eq`(S,"Hello XDolly, how is here?"));
    // using a map - explicit lookup function and object
    Map *m = `map_new_str_str`();
    `map_put`(m,"name","Monique");
    `map_put`(m,"home","Paris");
    `map_put`(m,"P","!");
    S = `str_templ_subst_using`(st,(StrLookup)map_get,m);
    assert(`str_eq`(S,"Hello !Monique, how is Paris?"));
    
`str_templ_subst_value` will use `interface_get_lookup` to see if the object
implements `Accessor`, so the last could be simply written `str_templ_subst_value(st)`
since `Map` defines that interface.

The placeholders can be changed if the default clashes with
the target language syntax, e.g. `str_templ_new(str,"@<>")`. Then your template
will look like "this is @<home>, welcome @<name>".

Subtemplates can be defined; for instance this template generates an HTML list.

    "<ul>$(for ls |<li>$(_)</li>|)</ul>"
    
There is an alternative syntax using a single colon instead of bracketting
pipe characters.

    "<ul>$(for ls: <li>$(_)</li>)</ul>"
    
The special `for` form iterates over an `Iterable` object like a `List` or an array.

The special variable `\_` refers to each value of the `Iterable`; if we're iterating
over a map-like object, then it will be the _key_;  use `$([\_])` for the _value_.

See `test-template.c`
*/

#include <ctype.h>
#include <stdio.h> // for now
#include "str.h"
#include "template.h"
#include "value.h"
#include "interface.h"

struct StrTempl_ {
    char *str;
    char **parts;
    StrTempl ***subt;  // all subtemplates
    const char *markers;
    // current substitution data
    StrLookup lookup;
    void *data;
    void *bound_data;
    // non-NULL if we're a subtemplate
    StrTempl *parent;
};

static char*** builtin_funs;
static char*** macros;
static int templ_instances;
static bool** if_stack = NULL;

static void StrTempl_dispose (StrTempl *stl) {
    obj_unref_v(stl->str, stl->parts, stl->subt);
    --templ_instances;
    if (templ_instances == 0) {
        obj_unref(builtin_funs);
        obj_unref(macros);
        obj_unref(if_stack);
        builtin_funs = NULL;
    }
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

static bool isspace_(int c) { return c == ' '; }

static char *spaces_left(char *p) {
    while (isspace_(*p))
        --p;
    return p;
}

static char *spaces_right(char *p) {
    while (*p && isspace_(*p))
        ++p;
    return p;
}

static char *advance_to(char *s, char* endc) {
    char ch1 = endc[0], ch2 = endc[1];
    ++s;
    while (*s && ! (*s == ch1 || *s == ch2 ))
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
    char *err = NULL;
    // we make our own copy of the string
    // and will break it up into this sequence
    Str **ss = seq_new(Str);
    stl->str = str_new(templ);
    stl->subt = NULL;
    stl->lookup = NULL;
    stl->data = NULL;
    stl->bound_data = NULL;
    stl->parent = NULL;
    stl->markers = markers;
    char *T = stl->str;
    while (true) {
        char *S = strchr(T,esc), *p, *st, *s, mark;
        if (S)  *S = '\0';
        // plain string chunk
        seq_add(ss, T);
        if (! S)   break;
        // $(key) expansion
        T = S + 1;
        if (*T != openp) {
            err = "no open bracket after escape";
            goto error;
        }
        // grabbing stuff inside $(...)
        S = balanced(T+1,openp,closep);
        if (*S != closep) {
            err = "no close bracket in substitution";
            goto error;
        }
        *S = '\0'; // closep!
        // might contain a subtemplate inside |...|
        p = spaces_left(S-1);
        if (*p == '|') { // right-hand boundary
            *p = '\0';
            mark = TTPL;
            char *s = advance_to(T,"|"); // left-hand boundary
            if (*s != '|') {
                err = "no left | in subtemplate";
                goto error;                
            }
            st = s + 1; // now points to subtemplate
            s = spaces_left(s-1) + 1;
            *s = '\0';
        } else { // colon notation
            char *np = advance_to(T," :"); // past '(', upto space or colon
            np = spaces_right(np); // might just be spaces...
            st = NULL;
            
            if (*np && *np != ':') { // fun followed by arg
                if (*np == '"') { // ... quoted value
                   np = advance_to(np,"\"");
                   if (! *np) {
                       err = "no end quote in string";
                       goto error;
                   }
                   ++np; // past closing quote
                } else { // ... word
                    np = advance_to(np," :");
                }
            }
            
            if (*np) { // something follows arg. But is it just space?
                char *endp = np;
                if (*np && *np == ':')
                    st = np+1;
                *endp = '\0';
            }
            
            if (st) {
                mark = TTPL;
            } else {
                *(p+1) = '\0';
                st = NULL;
                mark = TVAR;
            }
        }
        s = strchr(T,' ');
        if (s) { // there is an argument....
            *s = '\0';
            if (mark == TVAR)
                mark = TFUN;
        }
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
error:
    obj_unref(ss);
    obj_unref(stl);
    return (StrTempl*)value_error(err);
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

static char *if_impl (void *arg, StrTempl *stl) {
    seq_add(if_stack, picked);
    picked = arg != NULL;  // how we define truthiness....
    if (picked)
        return subst_using_parent(stl);
    else
        return str_new("");
}

static char *else_impl (void *arg, StrTempl *stl) {
    char* res;
    if (! picked)
        res = subst_using_parent(stl);
    else
        res = str_new("");
    picked = seq_pop(if_stack);
    return res;
}

static char *def_impl (void *arg, StrTempl *stl) {
    str_templ_add_macro((const char*)arg, stl, NULL);
    return str_new("");
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
    str_templ_add_builtin("def", def_impl);
    macros = smap_new(false);
    if_stack = seq_new(bool);
}

void str_templ_add_builtin(const char *name, TemplateFun fun) {
    smap_add(builtin_funs, name, (const void*)fun);
}

void str_templ_add_macro(const char *name, StrTempl *stl, void *data) {
    smap_add(macros, name, stl);
    if (data)
        stl->bound_data = data;
}

#define str_eq2(s1,s2) ((s1)[0]==(s2)[0] && (s1)[1]==(s2)[1])

static char *do_lookup(char *part, StrTempl *stl, StrLookup lookup, void *data) {
    char *dot = strchr(part,'.');
    if (dot) {
        *dot='\0';
        ++dot;
        void *T = do_lookup(part,stl,lookup,data);
        if (! T)
            return NULL;
        return do_lookup(dot,stl,lookup,T);
    }
    if (isdigit(*part)) {
        if (*part == '0') // synonym for _
            return (char*)data;
        else
            return (char*)((void**)data)[(int)*part - (int)'1'];                
    } else
    if (str_eq(part,"[_]")) { // used when iterating over a map; `_` is key, `[_]` is value
    	stl = stl->parent;
    	if (stl && stl->lookup)
    		return stl->lookup(stl->data, (char*)data);
    	else
    		return NULL;
    } else
    if (! str_eq2(part,"_")) {  // note that '_' stands for the data, without lookup
        char *res = NULL;
        if (lookup)
        	res = lookup (data, part);
        // which might fail; if there's a parent context, look there
        stl = stl->parent;
        while (! res && stl) {
            res = stl->lookup(stl->data,part);
            stl = stl->parent;
        }
        return res;                    
    } else { // _ means the object itself
        return (char*)data;
    }
}

/// substitute variables in template using a lookup function.
// `lookup` works with `data`, so that to use a Map you can pass
// `map_get` together with the map.
char *str_templ_subst_using(StrTempl *stl, StrLookup lookup, void *data) {
    int n = array_len(stl->parts);
    Str *out = array_new(Str,n);
    char *part, *err;
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
            StrTempl *macro = (StrTempl*)smap_get(macros,part);            
            if (mark != TVAR) { // function-style evaluation
                TemplateFun fn = NULL;
                if (! macro)
                    fn = (TemplateFun)smap_get(builtin_funs,part);
                char *arg = part + strlen(part) + 1;
                if (fn == NULL && macro == NULL) {
                    err = (char*)value_errorf("'%s' is not a function or macro",part);
                    goto error;
                }
                // always looks up the argument in current context, unless it's quoted
                if (*arg == '\"') {
                    ++arg;
                    arg[strlen(arg)-1] = '\0';
                } else {
                    arg = do_lookup(arg,stl,lookup,data);
                }
                if (fn) {
                    part = fn(arg, (StrTempl*)stl->parts[i+1]);
                } else {
                    part = str_templ_subst_values(macro, arg);
                }
                ref_str = true;
            } else {
                if (! macro) {
                    part = do_lookup(part,stl,lookup,data);
                } else {
                    part = str_templ_subst_values(macro, data);
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
    return res;
error:
    obj_unref(out);
    obj_unref(strs);
    return err;
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
    if (st->bound_data)
        v = st->bound_data;
    StrLookup lookup = (StrLookup)interface_get_lookup(v);
    return str_templ_subst_using(st,lookup,v);
}
