#include "str.h"

// Look up a value in a 'simple map'.
// These are arrays of strings where the odd entries are keys
// and the even entries are values.
char **str_lookup_ptr(char** substs, const char *name) {
    for (char **S = substs;  *S; S += 2) {
        if (strcmp(*S,name)==0)
            return (S+1);
    }
    return NULL;
}

char *str_lookup(char** substs, const char *name) {
    char **pref = str_lookup_ptr(substs,name);
    if (! pref)
        return NULL;
    return *pref;
}

char*** smap_new(bool ref) {
    if (ref)
        return seq_new_ref(char*);
    else
        return seq_new(char*);
}

void smap_add(char*** smap, const char *name, const void *data) {
    seq_add(smap,(char*)name);
    seq_add(smap,(char*)data);
}

void smap_put(char*** smap, const char *name, const void *data) {
    char **ps = *smap;
    char **pref = str_lookup_ptr(ps,name);
    if (pref) {
        if (obj_ref_array(ps))
            obj_unref(*ps);
        *ps = data;
    } else {
        smap_add(smap,name,data);
    }
}

void *smap_get(char*** smap, const char *name) {
    return (void*)str_lookup(*smap,name);
}

char** smap_close(char*** smap) {
    char** res = (char**)seq_array_ref(smap);
    obj_type_index(res) = OBJ_KEYVALUE_T;
    return res;
}

int smap_len(char*** smap) {
    return array_len(*smap)/2;
}
