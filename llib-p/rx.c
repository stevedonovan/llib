#include <stdio.h>
#include <stdlib.h>
#include <llib/str.h>
#include <llib/value.h>
#include "rx.h"

static str_t r_error(int code, regex_t *rx) {
    char buff[256];
    regerror(code,rx,buff,sizeof(buff));
    return value_error(buff);
}

static str_t percent_subst(str_t pattern) {
    char **res = strbuf_new();
    str_t p = pattern;
    bool inside_bracket = false;
    while (*p) {
        char ch = *p;
        if (ch == '%') {
            str_t klass = NULL;
            ++p; // eat special char
            switch(*p) {
                case 's':   klass = "space"; break;
                case 'd':   klass = "digit"; break;
                case 'x':   klass = "xdigit"; break;
                case 'a':   klass = "alpha"; break;
                case 'w':  klass = "alnum"; break;
                case '%':
                    ch = '%';
                    break;
                default:
                    ch = '\\';
                    // special case: not a bracket!
                    if (*p == '[' || *p == ']') {
                        strbuf_add(res,'\\');
                        ch = *p;
                    } else {
                        --p; // wasn't special after all...
                    }
            }
            if (klass) {
                if (inside_bracket)
                    strbuf_addf(res,"[:%s:]",klass);
                else
                    strbuf_addf(res,"[[:%s:]]",klass);
            } else {
                strbuf_add(res,ch);
            }
        } else {
            if (ch == '[')  {
                inside_bracket = true;
            } else
            if (ch == ']') {
                inside_bracket = false;
            }            
            strbuf_add(res,ch);
        }
        p++;
    }
    return strbuf_tostring(res);    
}

static void Regex_dispose(Regex *R) {
    obj_unref(R->matches);
    regfree(R->rx);
}

Regex *rx_new(str_t fmt, int cflags) {
    regex_t *rx = malloc(sizeof(regex_t));
    str_t pattern = fmt;
    int flags = REG_EXTENDED;
    if (cflags & RX_ICASE)
        flags |= REG_ICASE;
    if (cflags & RX_NEWLINE)
        flags |= REG_NEWLINE;
    if (cflags  & RX_LUA) {
        pattern = percent_subst(fmt);
        //~ printf("converted %s\n",pattern);
    }
    int res = regcomp(rx, pattern, flags);
    if (cflags  & RX_LUA)
        obj_unref(pattern);    
    if (res != 0) { // bombed out, baby!        
        return (Regex*)r_error(res,rx);
    } else {
        Regex *R = obj_new(Regex,Regex_dispose);
        R->rx = rx;
        R->matches = array_new(regmatch_t,rx->re_nsub+1);
        R->offs = 0;
        R->s = NULL;
        return R;
    }
}

static bool r_exec (Regex *R, str_t s) {
    regex_t *rx = R->rx;
    int n = array_len(R->matches);    
    int res = regexec(rx, s, n, R->matches, 0);
    if (res != 0) {
        return false;
        //~ if (res == REG_ESPACE) {
            //~ return (char**)r_error(res,rx);
    } else { // matched!
        return true;
    }    
}

char **rx_match_groups(Regex *R, str_t s) {
    if (r_exec(R,s)) {
        int n = array_len(R->matches);
        char **res = array_new(char*,n);
        regmatch_t *M = R->matches;
        FOR(i,n) {            
            res[i] = str_sub(s,M->rm_so,M->rm_eo);
            ++M;
        }
        return res;  
    } else { // no match...        
        return NULL;
    }
}

bool rx_find(Regex *R, str_t s, int *pi1, int *pi2) {
    int i1 = pi1 ? *pi1 : 0;
    int len = strlen(s);
    if (i1 >= len)
        i1 = len;
    if (r_exec(R,s+i1)) {
        if (pi1) {
            regmatch_t *M = R->matches;
            *pi1 = M->rm_so + i1;
            *pi2 = M->rm_eo + i1;            
        }
        R->offs = i1;
        R->s  = s;
        return true;
    } else {
        return false;    
    }
}

#define rx_match(R,s) rx_find(R,s,NULL,NULL)

/// number of groups?
int rx_group_count(Regex *R) {
    return array_len(R->matches) - 1;
}

/// n-th group of match as a string.
char *rx_group(Regex *R, int idx) {
    regmatch_t *M = &R->matches[idx];
    return str_sub(R->s,M->rm_so+R->offs,M->rm_eo+R->offs);    
}

// turn an index 'n' into the nth captured string
static str_t lookup_capture(Regex *R, str_t cap) {
    int idx = (int)*cap - (int)'0' ;  // '1' -> 1, etc
    return rx_group(R,idx);
}

char* rx_gsub(Regex *R, str_t s, StrLookup lookup, void *data) {
    int i1 = 0, i2 = 0,i0 = 0;
    char** ss = strbuf_new();
    Regex *OR = NULL;
    str_t out = NULL;
    if (! lookup) { // then the data is a pattern containing capture refs
        OR = rx_new("%([0-9])",0);
        out = (str_t)data;
    }
    while (rx_find(R,s,&i1,&i2)) {
        str_t ms, ls;
        if (i1 > 0) // text between any matches
            strbuf_addr(ss,s,i0,i1);
        if (out) { // output pattern (e.g. %2=%1)
            ls = rx_gsub(OR,out,(StrLookup)lookup_capture,R);
        } else { // explicit lookup function
            // if there's any captures, use first for lookup
            if (rx_group_count(R) > 0) {
                ms = rx_group(R,1);
            } else {
                ms = str_sub(s,i1,i2);
            }
            ls = lookup(data, ms);
            if (ls == NULL)
                ls = "???";            
        }
        strbuf_adds(ss,ls);
        i0 = i2;
        i1 = i2;
        obj_unref(ms);
        if (out)
            obj_unref(ls);
    }
    // and the tail...
    strbuf_adds(ss,s + i2); // i2 is not modified when match fails...
    if (OR)
        obj_unref(OR);
    return strbuf_tostring(ss);    
}

bool rx_subst(Regex *R, str_t s, char** ss, int *pi1) {
    int i2, i0 = *pi1;
    if (rx_find(R,s,pi1,&i2)) {
        if (*pi1 > 0) 
            strbuf_addr(ss,s,i0,*pi1);
        *pi1 = i2;
        return true;
    } else {
        strbuf_adds(ss,s + *pi1);
        return false;
    }
}

