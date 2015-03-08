/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/****
### Convenience API for POSIX regular expressions

These are innspired by the Lua string API in two ways; first, you may
write your regexes in Lua form like `%d+` instead of '\\[[digit]]` and
second, there are find and gsub functions.


*/

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

/// create a regex object. The flags are a combination
// of RX_ICASE (ignore case),  RX_NEWLINE (^$ matches lines) and RX_LUA,
// where the awkward '\\' can be written '%', and the Lua character classes
// 's','d','x','a' and 'w' are replaced by the POSIX classes "space","digit","xdigit",
// "alpha" and "alnum". (This is _just_ a string substitution and the regex
// semantics are not changed in any way!) 
//
// If the compilation fails, then the result is an error object (i.e. `value_is_error(res)`
// is true)
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

/// return any matches as an array of strings.
// The first match is the whole regex, and subsequent 
// matches are any explicit groups.
// On no match, return `NULL`
char **rx_match_groups(Regex *R, str_t s) {
    if (r_exec(R,s)) {
        int n = array_len(R->matches);
        char **res = array_new_ref(char*,n);
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

/// return the matching index range.
// This behaves rather like Lua's `string.find`, except that
// the start and end indices are returned in the last two arguments.
// If these arguments are not NULL, then the first `pi1` _must_ be
// initialized to a valid index, even if 0.
// If they are NULL then `rx_find` just indicates whether a match
// took place or not. 
//
// This is useful for scanning through a string looking for similar patterns.
// @usage
//    Regex *R = rx_new("[a+z]+",0);
//    str_t text = ""aa bb cc ddeeee";
//    int i1 = 0, i2;  // i1 is initialized!
//    while (rx_find(R,text,&i1,&i2)) {
//        printf("got %d %d '%s'\n",i1,i2,str_sub(text,i1,i2));
//        i1 = i2;
//    }

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

/// whether a string matches a regex or not.
// @param R
// @string s
// @treturn bool
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

// globally replaces all matches of `R` in `s`.
// In the first case, `lookup` is a function that operates with `data`
// like `str_lookup` or `map_get`. 
// If this is NULL, then the data must be a _string_, containing
// group references like %1 etc.
//
// You may prefer to use the `rx_lookup` and `rx_string` convenience macros
// for the last two arguments.
//
// @usage
//    char *aa[] = {"one","1","two","2",NULL};
//    R = rx_new("\\$([a-z]+)",0);
//    char *s = rx_gsub(R,"$one = $two(doo)",(StrLookup)str_lookup,aa);
//    assert(str_eq(s,"1 = 2(doo)"));
// @usage
//    R = rx_new("(%w+)=(%w+)",RX_LUA);
//    s = rx_gsub(R,"bonzo=dog,frodo=20",NULL,"'%1':%2");
//    assert(str_eq(s,"'bonzo':dog,'frodo':20"));
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
        str_t ms=NULL, ls=NULL;
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

/// substitute explicitly.
// Rather than passing the matching substring to a function,
// this allows you to write it out as a loop, which can be more
// convenient in C.
// @usage
//    Regex *R = rx_new("%$%((%a+)%)",RX_LUA);
//    str_t text = "here is $(HOME) for $(USER)!";
//    int i = 0;
//    char **ss = strbuf_new();
//    while (rx_subst(R,text,ss,&i)) {
//        str_t var = rx_group(R,1);
//        strbuf_adds(ss,getenv(var));
//        unref(var);
//    }
//    char *res = strbuf_tostring(ss);
//    printf("subst '%s'\n",res);
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

