// A re-implementation of pkg-config using llib.
// This has no external dynamic dependencies other than libc!
// Steve Donovan, c 2015
// BSD licence
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <llib/str.h>
#include <llib/template.h>
#include <llib/value.h>
#include <llib/file.h>
#include <llib/array.h>
#include <llib/arg.h>

static str_t subst (str_t s, char*** vars);
static void seq_adda_unique (str_t **ss, str_t *extra);
static str_t seq_concat(str_t **ss);

#define MAX_PATH 256
#define LINE_SIZE 1024

// pkg-config looks a package NAME as a file NAME.pc on its package path

static str_t default_paths[] = {
   "/usr/lib/pkgconfig","/usr/share/pkgconfig",
    "/usr/lib/x86_64-linux-gnu/pkgconfig", // <- configure!
    "/usr/lib/i386-linux-gnu/pkgconfig",
   "/usr/local/lib/pkgconfig","usr/local/share/pkgconfig"
};

static str_t *paths;
static bool verbose;

// Like with executable paths, you may add to the package path with the 
// environment variable `PKG_CONFIG_PATH`, separated with colons on Unix
// and semi-colons on Windows.
static void init_paths() {
    if (! paths) {
        paths = array_new_copy(str_t,default_paths,sizeof(default_paths)/sizeof(str_t));
        str_t extra = getenv("PKG_CONFIG_PATH");
        if (extra) {
            str_t *old = paths;
            char **extra_entries = str_split(extra,":");
            str_t **ss = seq_new_array(paths);
            seq_adda(ss,extra_entries,-1);
            paths = (str_t*)seq_array_ref(ss);
            unref(old);
            unref(extra_entries);    
        }
    }
}

str_t search_pkg_path(str_t name) {
    char path[MAX_PATH];
    FOR(i,array_len(paths)) {
        snprintf(path,MAX_PATH,"%s/%s.pc",paths[i],name);
        if (file_exists(path,"r")) {
            return str_new(path);
        }         
    }
    return NULL; // no can do
}

void kount() { fprintf(stderr,"kount = %d\n",obj_kount()); }

// pkg-config's --list-all option lists all .pc files found on the system, with their descriptions
// So we go through the package path and make an array containing the full path of all .pc files found.
void list_pkg_modules() {
    char mask[MAX_PATH];
    str_t **ss = seq_new_ref(str_t);
    FOR(i,array_len(paths)) {
        snprintf(mask,MAX_PATH,"%s/*.pc",paths[i]);
        char **in_dir = file_files_in_dir(mask,false);
        if (in_dir && array_len(in_dir) > 0) {
            seq_adda (ss, in_dir, -1);
            unref(in_dir);
        }        
    }
    str_t *files = (str_t*) seq_array_ref(ss);
    FOR(i,array_len(files)) {
        FILE *f = fopen(files[i],"r");
        char* name = file_basename(files[i]);
        // semi-barbaric method of removing .pc extension
        name[strlen(name)-3] = '\0';
        while (file_gets(f,mask,MAX_PATH)) {
            int idx = 0;
            if (str_contains(mask,"Description: ",&idx)) {
                printf("%s\t%s\n",name,mask+idx);                
                break;
            }
        }
        unref(name);
        fclose(f);
    }        
    unref(files);
}

//////// Parsing .pc files /////////
// This is relatively straightforward - the delicate stuff involves checking versioned dependencies
// and bringing in their headers and libraries.

typedef struct PkgConfig_ {
    str_t pc;
    str_t filename;
    str_t name;
    str_t description;
    str_t URL;
    str_t version;
    // these are resizeable arrays (seqs) - pointers to arrays of strings.
    str_t **libs;
    str_t **cflags;
    str_t **libs_private;
    char ***vars;
} PkgConfig;

// Any llib object may have a dispose function. Fairly tedious, but at least
// we don't have to worry about exactly how to dispose of various objects!
static void PkgConfig_dispose(PkgConfig *pkg) {
    unref(pkg->pc);
    unref(pkg->filename);
    unref(pkg->name);
    unref(pkg->description);
    unref(pkg->URL);
    unref(pkg->version);
    unref(pkg->libs);
    unref(pkg->cflags);
    unref(pkg->libs_private);
    unref(pkg->vars);
}

// when processing keyword lines, we first do any variable substitution.
// Then split into words and append to the array. 
void subst_and_split(str_t **ss, str_t v, char*** vars) {
    str_t s = subst(v,vars);
    char **res = str_split(s," ");
    seq_adda(ss, res, -1);
    unref(res);
    unref(s);
}

static char ***pkg_map;

static void intialize_pkg_map() {
    pkg_map = smap_new(true);
}

static str_t process_pkg_requires (PkgConfig *pkg, char *s, bool is_private);

// a note on error handling: pkg-config is traditionally a silent program that only emits error messages
// on request.  No printing to the error stream takes place here outside main()!  `read_pkg` returns an _error value_
// when it's unhappy.  These are basically C strings but are labeled with a different dynamic type, so 
// that `value_is_error` is true.  In this way, our function can return a PkgConfig object if successful,
// or return an error value.

PkgConfig *read_pkg (str_t pc) {
    char line [LINE_SIZE];
    str_t err = NULL;
    
    // cache package objects based on their package name.
    PkgConfig *parsed = smap_get(pkg_map, pc);
    if (parsed) {
        return parsed;
    }    
    
    str_t filename = search_pkg_path(pc);
    if (! filename) { // we have searched in vain...
        return (PkgConfig*)value_errorf("Cannot find package '%s'",pc);
    }
    
    // note the indirection! We use this llib function because it returns a useful error when it fails,
    // and the resulting file object is closed when it is unref'd.
    FILE** f = file_fopen(filename,"r");
    if (value_is_error(f)) {
        return (PkgConfig*)f;
    }
    
    char*** vars = smap_new(true);
    
    if (verbose)
        fprintf(stderr,"loading %s\n",pc);
    
    PkgConfig *pkg = obj_new(PkgConfig,PkgConfig_dispose);
    memset(pkg,0,sizeof(PkgConfig));
    pkg->filename = filename;
    pkg->vars = vars;
    pkg->libs = seq_new_ref(str_t);
    pkg->cflags = seq_new_ref(str_t);
    pkg->libs_private = seq_new_ref(str_t);
    pkg->pc = str_ref(pc);
    smap_add(pkg_map,str_ref(pc),pkg);
 
    while (file_gets(*f,line,LINE_SIZE)) {
        // get rid of comments and blank lines
        char *p = strchr(line,'#');
        if (p)  *p = '\0';
        str_trim(line);
        if (*line == '\0') continue;
        
        // variables are immediately followed by '=' and their value.
        // All values are passed through variable substitution in terms of previously
        // defined variables
        p = strchr(line,'=');
        if (p && isalnum(*(p-1))) {
            *p++ = '\0';
            // simple linear lookup map does fine for us here...
            smap_put(vars,str_new(line),subst(p,vars));
            continue;            
        }
        
        // keyword values: these are words followed by a colon, either followed by a space
        // and some text or nothing.
        p = strchr(line,':');
        if (p && (*(p+1) == ' '  || *(p+1) == '\0')) {
            *p++ = '\0';
            if (! *p)
                continue;
            str_t keyword = line;
            str_t value = str_new(p+1) ;
            if (str_eq(keyword, "Name")) {
                pkg->name = ref(value);
            } else
            if (str_eq(keyword, "Description")) {
                pkg->description = ref(value);
            } else
            if (str_eq(keyword, "Version")) {
                pkg->version = ref(value);
            } else
            if (str_eq(keyword, "Requires") || str_eq(keyword, "Requires.private")) {
                char *s = (char*)subst(value,pkg->vars);
                err = process_pkg_requires(pkg,s, str_eq(keyword, "Requires.private"));
                unref(s);
                if (err) 
                    break;
            } else
            if (str_eq(keyword, "Conflicts")) {
                puts("NYI");
            } else
            if (str_eq(keyword, "Libs")) {
                subst_and_split(pkg->libs,value,vars);
            } else
            if (str_eq(keyword, "Libs.private")) {
                subst_and_split(pkg->libs_private,value,vars);
            } else
            if (str_eq(keyword, "Cflags")) {
                subst_and_split(pkg->cflags,value,vars);
            } else
            if (str_eq(keyword, "URL")) {
                pkg->URL = ref(value);      
            } else {
                err = value_errorf("%s: unknown keyword '%s'\n",filename,keyword);
                break;
            }
            unref(value);
        } else {
            err = value_errorf("%s: garbage line '%s'\n",filename,line);
            break;
        }
    }
    
    // These fields are compulsory, so it's an immediate error
    if (! err && ! (pkg->name && pkg->description && pkg->version)) {
        err = value_errorf("%s: must have Name, Description and Version fields",filename);
    }
    
    unref(f);  // closes the underlying FILE*
    
    if (err) {
        unref(pkg);
        return (PkgConfig*)err;
    } else {
        return pkg;
    }
}

// How to compare version strings: the basic idea is to treat them as numbers where
// the integers between the periods are base 1024 'digits'.  So 1.2 is 1024*1 + 2.
// The shenanigans with lmax are needed because we must have e.g. 1.10 > 1.1.92 !
static bool version_pkg_matches(str_t found_vs, str_t needed_vs, str_t op) {
    char **parts1 = str_split(found_vs,".");
    char **parts2 = str_split(needed_vs,".");
    int l1 = array_len(parts1), l2 = array_len(parts2);
    int lmax = l1;
    uint64_t res1 = 0, res2 = 0, power = 1;
    if (l2 > l1)  lmax = l2;
    
    for (int i = lmax-1; i >= 0; i--) {
        if (i < l2)
            res2 += atoi(parts2[i])*power;            
        if (i < l1)
            res1 += atoi(parts1[i])*power;
        power *= 1024;
    }
    unref(parts1);
    unref(parts2);
    
    uint64_t found = res1, needed = res2;
    if (str_eq(op,"=")) {
        return found == needed;
    } else
    if (str_eq(op,">=")) {
        return found >= needed;
    } else
    if (str_eq(op,"<=")) {
        return found <= needed;
    } else {
        return false;
    }
}

// this is the tricky heart of the problem - any package may require other packages
// which may need to satisfy a version constraint, e.g. boo >= 0.9.
// We 'inherit' compile and link flags from required packages. 
//
// The manual, although well done and comprehensive, is not a full description of the actual
// behaviour of pkg-config. For instance, pkg-config lets 'private' packages add to include path,
// to accomodate some upstream packages. (see https://github.com/NixOS/nixpkgs/issues/292)
// Also, although the manual clearly says that required items are comma-separated, this isn't
// true in practice. So we ignore commas and parse space-separated tokens. In the require line
// 'boo foo >= 1.1 bar' it's clear that 'boo' and 'bar' have no version constraint, since they are
// not followed by a relational operator.

static str_t process_pkg_requires (PkgConfig *pkg, char *s, bool is_private) {
    str_t err = NULL;
    // not _always_ comma-separated! So get rid of commas!
    FORAP(s, if(*_ == ',')  *_ = ' ');
    // and go through space-separated tokens
    char **parts = str_split(s," ");
    char **P = parts;
    while (*P) {
        str_t op = NULL, vs = NULL, package = P[0];
        // PACKAGE OPERATOR VERSION?
        if (P[1] && str_find_first_of(P[1],"=<>") != -1) {
            op = P[1];
            vs = P[2];        
            P += 2;
        }    
        bool seen = smap_get(pkg_map, package) != NULL;
        if (verbose && pkg != NULL) {
            fprintf(stderr,"%s requires %s %s\n",pkg->pc,package, seen ? "SEEN" : "");
        }
        
        if (seen) { // don't bother if we've already seen this...the flags have already been included
            ++P;
            continue;
        }
        
        // read the required package - may not succeed!
        PkgConfig *required_pkg = read_pkg(package);
        if (value_is_error(required_pkg)) {
            err = (str_t)required_pkg;
        } else // and even so, does it match required version?
        if (op && ! version_pkg_matches(required_pkg->version,vs,op)) {
            if (pkg != NULL) {
                err = value_errorf("%s required %s (%s), version mismatch '%s %s %s'",
                        pkg->pc,required_pkg->pc,vs,
                        required_pkg->version,op,vs);
            } else {
                err = value_errorf("version %s %s %s of %s required",
                        required_pkg->version,op,vs,
                        required_pkg->pc);                
            }
        }
        if (err) {
            return err;
        }
        if (pkg != NULL) {
            // finally may add any compile or link flags to the current package's flags, if they exist!
            // To prevent nonsensically long output, only add if not already present.
            if (! is_private) {
                seq_adda_unique(pkg->cflags, *required_pkg->cflags);
                seq_adda_unique(pkg->libs, *required_pkg->libs);
                if (verbose)
                    fprintf(stderr,"updated public %s from %s\n",pkg->pc, required_pkg->pc);
            } else {
                seq_adda_unique(pkg->libs, *required_pkg->libs_private);
                seq_adda_unique(pkg->cflags, *required_pkg->cflags);
                if (verbose)
                    fprintf(stderr,"updated private %s from %s\n",pkg->pc, required_pkg->pc);
            }
        }
        ++P;
    }
    unref(parts);
    return NULL;
}


void print_pkg_variables(PkgConfig *pkg) {    
    for (char **p = *pkg->vars; *p; p+=2) {
        printf("%s\n",*p);
    }    
}

void print_pkg_var_value(PkgConfig *pkg, str_t var) {
    str_t v = smap_get(pkg->vars,var);
    printf("%s\n",v ? v : "");
}

void set_pkg_var_value(PkgConfig *pkg, str_t var, str_t val) {
    smap_put(pkg->vars,var,val);
}

// llib has a somewhat unconvential way of parsing command-line arguments, based on
// the idea that program usage (which you ought always write) should be parseable. This is
// obviously easier in a dynamic language (e.g. the lapp Lua library in Penlight) but
// here the old dog C shows its surprising flexibility.
//
// The `arg` module is fed an array of argument specifications; usually the spec is 
// followed by a pointer to the variable which we wish to _bind_ that argument to.
// Boolean flags aren't followed by values, and if a flag has a default then it
// will not be a _required_ flag.  Array flags (with '[]') may be used multiple times.

static bool modversion;
static bool print_errors;
static bool cflags;
static bool libs;
static bool ppath;
static bool print_variables;
static bool list_all;
static str_t variable, define_variable;
static str_t *exists;
static str_t* packages;

void* args[] = {
    "// pkgconfig: show build information from .pc files",
    "bool modversion; // show version information of packages",&modversion,
    "bool print-errors; // verbose errors (normally silent)",&print_errors,
    "bool cflags; // flags needed to compile against packages",&cflags,
    "bool libs; // flags needed to link against packages",&libs,
    "bool print-variables; // print all variables defined in packages",&print_variables,
    "bool list-all; // list all modules found on package path",&list_all,
    "string variable=''; // value of variable defined in package's .pc file",&variable,
    "string exists[]; // does package exist with possible required version?",&exists,
    "string define_variable=''; // overide variable value in package's .pc file",&define_variable,
    "bool path; // just show full paths of .pc files",&ppath,
    "string #1[]; // packages",&packages,
    NULL
};

int main(int argc, const char **argv)
{    
    ArgState *as = arg_command_line(args,argv);
    init_paths();
    
    verbose = getenv("PKG_CONFIG_DEBUG_SPEW") != NULL;
    if (verbose)
        print_errors = true;
    
    intialize_pkg_map();
    
    if (list_all) {
        list_pkg_modules();
        return 0;
    }
    
    // --exists is either followed by a package name, or a constraint like 'foo >= 1.0';
    // there may be multiple such.
    if (array_len(exists) > 0) {
        bool ok = true;
        FOR (i, array_len(exists)) {
            str_t arg = exists[i];
            void *res;
            if (strchr(arg,' ') != NULL) {
                res = (void*)process_pkg_requires(NULL,str_new(arg),false);
            } else {
                res = read_pkg(arg);
            }
            ok = ok  && ! value_is_error(res);
        }
        return ok ? 0 : 1;
    }
    
    if (array_len(packages)==0) {
        arg_quit(as,"must provide packages, unless --list-all or --exists",true);
    }
    
    bool output = false;
    
    kount();
    FOR (i,array_len(packages)) {
        str_t package = packages[i];
        if (ppath) { // this is a useful extension - equivalent to 'which' for .pc files!
            str_t filename = search_pkg_path(package);
            if (filename) {
                puts(filename);
            } else {
                return 1;
            }                
            continue;
        }
        
        PkgConfig *pkg = read_pkg(package);
        if (value_is_error(pkg)) {
            if (print_errors)
                fprintf(stderr,"error: %s\n",(str_t)pkg);
            return 1;
        }
        if (*define_variable) {
            str_t *parts = (str_t*)str_split(define_variable,"=");
            set_pkg_var_value(pkg,parts[0],parts[1]);
            unref(parts);
        }
        if (*variable) {
            print_pkg_var_value(pkg,variable);
        } else
        if (print_variables) {
            print_pkg_variables(pkg);
        } else {            
            if (cflags) {
                str_t flagstr = seq_concat(pkg->cflags);
                printf("%s ",flagstr);
                unref(flagstr);
                output = true;
            }
            if (libs) {
                str_t flagstr = seq_concat(pkg->libs);
                printf("%s ",flagstr);
                unref(flagstr);
                output = true;
            }            
        }
    }
    unref(pkg_map);
    kount();
    if (output) {
        printf("\n");
        fflush(stdout);
    }

    return 0;
}

////// Some useful utilities

// we use llib's template support to do the necessary ${var} substitutions.
// The variables are a 'simple map', (which is a pointer to an array of strings where
// odd entries are keys and even entries are values)
static str_t subst (str_t s, char*** vars) {
    StrTempl *T = str_templ_new(s,"${}");
    str_t res = str_templ_subst_using(T,(StrLookup)smap_get,vars);
    unref(T);
    return res;
}

// seq_adda adds an array at the end of a seq - this specialised version
// does not add duplicated strings.
static void seq_adda_unique (str_t **ss, str_t *extra) {
    for (str_t *P = extra; *P; ++P)  {
        str_t s = *P;
        // but have we already seen this flag?
        FINDA(idx,*ss,0,str_eq(_,s));
        if (idx == -1) {
            seq_add(ss,ref(s));
        }
    }
}

// this conveniently does the dereferencing (seqs are pointers to arrays) and makes a 
// string with items separated by spaces.
static str_t seq_concat(str_t **ss) {
    return str_concat((char**)*ss," ");
}

