/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
### Reading Simple Configuration Files.

`config_read` will read a configuration file in so-called property format;
Blank lines and comments beginning with '#' are ignored; otherwise lines
are in the form "key=value".  It returns the result as a simple map.

The various `config_gets*` functions will look up and convert values,
returning a default if the key is not found.  The `_arr` variants will
return an array, where values are delimited by commas; the default is
the empty array.

`test-config.c` shows the usage; `config.c` shows an alternative way
using the `arg` module where variables are bound directly to properties.
*/

#include <stdlib.h>
#include "config.h"
#include "file.h"
#include "array.h"

/// get a string value, with default.
str_t config_gets(SMap m, str_t key, str_t def) {
    str_t res = str_lookup(m,key);
    if (! res)
        res = str_ref(def);
    return str_new(res);
}

/// get an int value, with default.
int config_geti(SMap m, str_t key, int def) {
    str_t s = str_lookup(m,key);
    if (! s) {
        return def;
    } else {
        return atoi(s);
    }
}

/// get a double value, with default.
double config_getf(SMap m, str_t key, double def) {
    str_t s = str_lookup(m,key);
    if (! s) {
        return def;
    } else {
        return atof(s);
    }
}

/// get an array of strings, with default.
char** config_gets_arr(SMap m, str_t key) {
    str_t s = str_lookup(m,key);
    if (! s) {
        return array_new(char*,0);
    } else {
        return str_split(s,",");
    }    
}

/// get an array of ints, with default.
int* config_geti_arr(SMap m, str_t key) {
    char** parts = config_gets_arr(m,key);
    MAPA(int,res,atoi(_),parts);
    unref(parts);
    return res;
}

/// get an array of double, with default.
double* config_getf_arr(SMap m, str_t key) {
    char** parts = config_gets_arr(m,key);
    MAPA(double,res,atof(_),parts);
    unref(parts);
    return res;
}

/// read a configuration file, skipping # comments and blank lines.
char** config_read(str_t file) {
    FILE *in = fopen(file,"r");
    char*** ss = smap_new(true);
    char line[256];
    while (file_gets(in,line,sizeof(line))) {
        char *p = strchr(line,'#');       
        if (p) {
            *p = '\0';
        }
        if (str_is_blank(line))
            continue;        
        char **parts = str_split(line,"=");
        str_trim(parts[1]);
        smap_add(ss,str_ref(parts[0]),str_ref(parts[1]));
        unref(parts);
    }
    fclose(in);
    return smap_close(ss);    
}