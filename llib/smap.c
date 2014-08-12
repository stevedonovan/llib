#include "str.h"

/// Simple Maps
// @submodule str

/// Iterate over a simple map
// @param k variable for key
// @param v variable for value
// @macro FOR_SMAP

/// Simple Maps.
// These are arrays of strings where the odd entries are keys
// and the even entries are values.
// @section smap

/// Look up a string in a smap returning pointer to entry.
char **str_lookup_ptr(char** substs, const char *name) {
    for (char **S = substs;  *S; S += 2) {
        if (strcmp(*S,name)==0)
            return (S+1);
    }
    return NULL;
}

/// Look up a string key in a smap returning associated value.
char *str_lookup(char** substs, const char *name) {
    char **pref = str_lookup_ptr(substs,name);
    if (! pref)
        return NULL;
    return *pref;
}

/// New simple map builder.
char*** smap_new(bool ref) {
    char*** res;
    if (ref)
        res = seq_new_ref(char*);
    else
        res = seq_new(char*);
    (*res)[0] = NULL;
    return res;
}

/// Add a key/value pair.
void smap_add(char*** smap, const char *name, const void *data) {
    seq_add(smap,(char*)name);
    seq_add(smap,(char*)data);
    char** arr = *smap;
    if (! obj_ref_array(arr))
        arr[array_len(arr)] = NULL;
}

/// Update/insert a key/value pair.
void smap_put(char*** smap, const char *name, const void *data) {
    char **ps = *smap;
    char **pref = str_lookup_ptr(ps,name);
    if (pref) {
        if (obj_ref_array(ps))
            obj_unref(*ps);
        *ps = (char*)data;
    } else {
        smap_add(smap,name,data);
    }
}

/// Get the value associated with `name`.
void *smap_get(char*** smap, const char *name) {
    return (void*)str_lookup(*smap,name);
}

/// close the sequence, returning a simple map.
char** smap_close(char*** smap) {
    char** res = (char**)seq_array_ref(smap);
    obj_type_index(res) = OBJ_KEYVALUE_T;
    return res;
}

/// number of entries.
int smap_len(char*** smap) {
    return array_len(*smap)/2;
}


/// add referenced key/value pair.
// @tparam str_t name
// @tparam str_t data
// @function smap_put_ref

/// update/insert a referenced key/value pair.
// @tparam str_t name
// @tparam str_t data
// @function smap_put_ref


