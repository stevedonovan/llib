#ifndef __RX_H
#define __RX_H

#include <sys/types.h>
#include <regex.h>

typedef struct Regex_ {
    regex_t *rx;
    regmatch_t *matches;
    // supporting groups
    int offs;
    str_t s;
} Regex;

enum {
    RX_ICASE = 1,
    RX_LUA = 2,
    RX_NEWLINE = 4
};

#define rx_match(R,s) rx_find(R,s,NULL,NULL)

Regex *rx_new(str_t fmt, int cflags);
char **rx_match_groups(Regex *R, str_t s);
bool rx_find(Regex *R, str_t s, int *pi1, int *pi2);
int rx_group_count(Regex *R);
char *rx_group(Regex *R, int idx);
char* rx_gsub (Regex *R, str_t s, StrLookup lookup, void *data);
bool rx_subst (Regex *R, str_t s, char** ss, int *pi1);

#endif

