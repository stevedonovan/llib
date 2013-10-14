/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

// template support for str

#include "str.h"

struct StrTempl_ {
    char *str;
    char **parts;
};

static void StrTempl_dispose (StrTempl stl) {
    obj_unref_v(stl->str, stl->parts);
}

#define MARKER '\01'

#include <assert.h>

typedef char *Str;

/// new template from string with `$(var)` variables
StrTempl str_templ_new(const char *templ, const char *markers) {
    StrTempl stl = obj_new(struct StrTempl_,StrTempl_dispose);
    if (! markers)
        markers = "$()";
    char esc = markers[0], openp = markers[1], closep = markers[2];
    // we make our own copy of the string
    // and will break it up into this sequence
    Str **ss = seq_new(Str);
    stl->str = str_new(templ);
    char *T = stl->str;
    while (true) {
        int endp, dollar = str_findch(T,esc);
        if (dollar > -1)
            T[dollar] = '\0';
        //  plain string chunk
        seq_add(ss, T);
        // $(key) expansion
        if (dollar == -1)
            break;
        T += dollar + 1;
        assert(*T == openp);
        *T = MARKER;
        endp = str_findch(T,closep);
        T[endp] = '\0';
        seq_add(ss,T);
        T += endp + 1;
        if (! *T) break;
    }
    stl->parts = (Str*)seq_array_ref(ss);
    return stl;
}

/// substitute variables in template using a lookup function.
char *str_templ_subst_using(StrTempl stl, StrLookup lookup, void *data) {
    int n = array_len(stl->parts);
    Str *out = array_new(Str,n);

    FOR (i, n) { // over all the parts of the template
        char *part = stl->parts[i];
        if (*part == MARKER) {
            part = lookup (data, part+1);
            if (! part)
                part = "";
        }
        out[i] = part;
    }

    Str res = str_concat(out,NULL);
    obj_unref(out);
    return res;
}

static char *pair_lookup(char **substs, char *name) {
    for (char **S = substs;  *S; S += 2) {
        if (strcmp(*S,name)==0)
            return *(S+1);
    }
    return NULL;
}

/// substitute the variables using an array of keys and pairs.
char *str_templ_subst(StrTempl stl, char **substs) {
    return str_templ_subst_using(stl,(StrLookup)pair_lookup,substs);
}
