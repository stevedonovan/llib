#include "str.h"

// Look up a value in a 'simple map'.
// These are arrays of strings where the odd entries are keys
// and the even entries are values.
char *str_lookup(SMap substs, const char *name) {
    for (const char **S = substs;  *S; S += 2) {
        if (strcmp(*S,name)==0)
            return (char*)*(S+1);
    }
    return NULL;
}

char*** smap_new(bool ref) {
    if (ref)
        return seq_new(char*);
    else
        return seq_new_ref(char*);
}

void smap_put(char*** smap, const char *name, void *data) {
    seq_add(smap,(char*)name);
    seq_add(smap,(char*)data);
}

char** smap_close(char*** smap) {
    char** res = (char**)seq_array_ref(smap);
    obj_type_index(res) = OBJ_KEYVALUE_T;
    return res;
}
