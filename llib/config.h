/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef __LLIB_CONFIG_H
#define __LLIB_CONFIG_H
#include <stdio.h>
#include "str.h"

enum {
    CONFIG_DELIM_EQUALS=1,
    CONFIG_DELIM_SPACE=2,
    CONFIG_COMMENT_ONLY_FIRST=4
};

str_t config_gets(SMap m, str_t key, str_t def); 
int config_geti(SMap m, str_t key, int def);
double config_getf(SMap m, str_t key, double def);
char** config_gets_arr(SMap m, str_t key);
int* config_geti_arr(SMap m, str_t key);
double* config_getf_arr(SMap m, str_t key);
char** config_read_stream(FILE *in, int flags);
char** config_read_opt (str_t file, int flags);
char** config_read(str_t file);

#endif

