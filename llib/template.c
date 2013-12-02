/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/**** String templates.
In their simplest form they allow substitution of placeholders like
`$(var)` with the value of `var` determined by a lookup, which is
defined as a function plus an object. The placeholders can be changed
if the default clashes with the target language syntax.
*/

#include <assert.h>
#include <stdio.h> // for now
#include "str.h"
#include "template.h"
#include "list.h"
#include "map.h"
#include "value.h"

char *str_lookup(char **substs, char *name) ;

struct StrTempl_ {
    char *str;
    char **parts;
    List *subt;  // all subtemplates
    // current substitution data
    StrLookup lookup;
    void *data;
    // non-NULL if we're a subtemplate
    StrTempl parent;
};

static void StrTempl_dispose (StrTempl stl) {
    obj_unref_v(stl->str, stl->parts, stl->subt);
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

static char *spaces_right(char *p) {
    while (*p == ' ')
        --p;
    return p;
}

static char *advance_to(char *s, char ch) {
    while (*s && *s != ch)
        ++s;
    return s;
}

/// new template from string with variables to be expanded.
// `$(var)` is the default, if `markers` is NULL, but if `markers`
// was "@<>" then it would use '@' as the escape and "<>" as the
// brackets.
StrTempl str_templ_new(const char *templ, const char *markers) {
    StrTempl stl = obj_new(struct StrTempl_,StrTempl_dispose);
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
        p = spaces_right(S-1);
        if (*p == '|') {
            *p = '\0';
            mark = TTPL;
            char *s = advance_to(T+1,'|');
            assert(*s == '|');
            st = s + 1;
            s = spaces_right(s-1) + 1;
            *s = '\0';
           // st = s + 2;
        }  else {
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
            StrTempl subt = str_templ_new(st,markers);
            if (! stl->subt)
                stl->subt = list_new_ref();
            list_add(stl->subt,subt);
            subt->parent = stl;
            seq_add(ss,(char*)subt);
        }
        T = S + 1;
        if (! *T) break;
    }
    stl->parts = (Str*)seq_array_ref(ss);
    return stl;
}

typedef char *(*TemplateFun)(void *arg, StrTempl stl);

static char *i_impl (void *arg, StrTempl stl) {
    return str_fmt("%d", (intptr)arg);
}

static StrLookup get_lookup(void *item);

static char *with_impl (void *arg, StrTempl stl) {
    return str_templ_subst_using(stl,get_lookup(arg),arg);
}

struct Iter_;

typedef bool (*IterFun)(struct Iter_ *it, void **pv);

typedef struct Iter_ {
    IterFun next;
    void **P;
    int n;
} *Iter;

static Iter get_iterator (void *obj);

static char *for_impl (void *arg, StrTempl stl) {
    StrLookup mlookup = NULL;
    Iter a = get_iterator(arg);
    Str *out = array_new_ref(Str,a->n);
    void *item;
    int i = 0;
    while (a->next(a,&item)) {
        if (i == 0) // assume all items have same type
            mlookup = get_lookup(item);
        out[i++] = str_templ_subst_using(stl,mlookup,item);
    }
    Str res = str_concat(out,"");
    obj_unref(out);
    obj_unref(a);
    return res;
}

static char *subst_using_parent(StrTempl stl) {
    StrTempl pst = stl->parent;
    return str_templ_subst_using(stl,pst->lookup,pst->data);
}

static intptr picked = false;
static List *if_stack = NULL;

static char *if_impl (void *arg, StrTempl stl) {
    if (! if_stack)
        if_stack = list_new_ptr();
    list_add(if_stack, (void*)picked);
    picked = arg != NULL;  // how we define truthiness....
    if (picked)
        return subst_using_parent(stl);
    else
        return str_new("");
}

static char *else_impl (void *arg, StrTempl stl) {
    if (! picked)
        return subst_using_parent(stl);
    else
        return str_new("");
    picked = (intptr)list_pop(if_stack);
}

#define C (char*)
char *builtin_funs[] = {
    "i",C i_impl,
    "with",C with_impl,
    "for",C for_impl,
    "if",C if_impl,
    "else",C else_impl,
    NULL
};
#undef C

#define str_eq2(s1,s2) ((s1)[0]==(s2)[0] && (s1)[1]==(s2)[1])

/// substitute variables in template using a lookup function.
// `lookup` works with `data`, so that to use a Map you can pass
// `map_get` together with the map.
char *str_templ_subst_using(StrTempl stl, StrLookup lookup, void *data) {
    int n = array_len(stl->parts);
    Str *out = array_new(Str,n);
    char *part;
    char mark = 0;
    List *strs = NULL;
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
                TemplateFun fn = (TemplateFun)str_lookup (builtin_funs,part);
                char *arg = part + strlen(part) + 1;
                assert(fn != NULL);
                // always looks up the argument in current context
                if (! str_eq2(arg,"_"))
                    arg = lookup (data, arg);
                else
                    arg = (char*)data;
                part = fn(arg, (StrTempl)stl->parts[i+1]);
                ref_str = true;
            } else {
                // note that '_' stands for the data, without lookup
                if (! str_eq2(part,"_"))
                    part = lookup (data, part);
                else
                    part = (char*)data;
            }
            if (! part)
                part = "";
            else if (value_is_box(part)) {
                part = (char*)value_tostring((PValue)part);
                ref_str = true;
            }
            if (ref_str) {
                if (! strs)
                    strs = list_new_ref();
                list_add(strs,part);
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
char *str_templ_subst(StrTempl stl, char **substs) {
    return str_templ_subst_using(stl,(StrLookup)str_lookup,substs);
}

/// substitute using boxed values.
// The value `v` can either represent a `Map` or be a simple
// pair array.
char *str_templ_subst_values(StrTempl st, PValue v) {
    StrLookup lookup;
    if (value_is_map(v))
        lookup = (StrLookup)map_get;
    else
        lookup = (StrLookup)str_lookup;

    return str_templ_subst_using(st,lookup,v);
}

static bool iter_arr_next(Iter it, void **pv) {
    if (it->n == 0)
        return false;
    --it->n;
    *pv = *(it->P)++;
    return true;
}

static bool iter_list_next(Iter it, void **pv) {
    ListIter li = (ListIter)it->P;
    if (! li)
        return false;
    *pv = li->data;
    li = li->_next;
    it->P = (void**)li;
    return true;
}

static Iter get_iterator (void *obj) {
    int n;
    Iter ai = obj_new(struct Iter_,NULL);
    if (obj_refcount(obj) == -1) {
        // _assumed_ to be an array...
        char **A = (char**)obj;
        n = 0;
        for (char** a = A; *a; ++a)
            ++n;
        ai->next = iter_arr_next;
    } else {
        if (list_object(obj)) {
            List *L = (List*)obj;
            n = list_size(L);
            ai->next = iter_list_next;
            obj = list_start(L);
        } else { // must be an array
            n = array_len(obj);
            ai->next = iter_arr_next;
        }
    }
    ai->n = n;
    ai->P = (void**)obj;
    return ai;
}


static StrLookup get_lookup(void *item) {
    if (map_object(item))
        return (StrLookup)map_get;
    else
        return (StrLookup)str_lookup; // default 'simple map'
}
