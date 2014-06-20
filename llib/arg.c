/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
### Command-line Argument Parser.

This allows you to bind variables to command-line arguments, providing
the flag name/type, a shortcut, a pointer to the variable, and some help text.
For this we write _specifications_ (which are strings containing pseudo-C declarations
followed by a comment, which may specify a shortcut).

The argument parser follows GNU conventions; long flags with double-hyphen,
which may have short single-letter forms with a single-hyphen. Short flags may
be combined ("-abc") and may be followed immediately by their value ("-n10").

    #include <stdio.h>
    #include <llib/arg.h>
    
    int lines;
    FILE *file;
    bool print_lines;
    
    PValue args[] = {
        "int lines=10; // -n number of lines to print",&lines,
        "bool lineno; // -l output line numbers",&print_lines,
        "infile #1=stdin; // file to dump",&file,
        NULL
    };
    
    int main(int argc,  const char **argv)
    {
        `arg_command_line`(args,argv);
        char buff[512];
        int i = 1;
        while (fgets(buff,sizeof(buff),file)) {
            if (print_lines)
                printf("%03d\t%s",i,buff);
            else
                printf("%s",buff);
            if (i++ == lines)
                break;
        }
        fclose(file);
        return 0;
    }    

If you now call `arg_command_line(args,argv)` these variables will be
bound; note how both type and optional default value are specified. Names like '#1'
refer to the first non-flag argument and so forth.  Both '--lines' and '-n' can be 
used to set the integer variable `lines`.  If a flag has no default, then it is an error.
Plain boolean flags have an implicit default of `false`.

Help usage is automatically generated from these specifications.

If a conversion is not possible (not a integer, file cannot be opened, etc)
then the program will exit, showing the help.

@{cmd.c} is a basic example; @{testa.c} shows how to 
define flags and commands as functions, as well as defining a simple REPL.

`arg_bind_values` may be used directly to process an array of name/value
pairs, for instance in `config.c`
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define INSIDE_ARG_C
#include "arg.h"


enum {
    ValueFileIn = 0x200,
    ValueFileOut = 0x201
};

enum {
    FlagUsed = 1, // flag was explicitly referenced
    FlagValue = 2, // user data is a value object, not raw
    FlagCommand = 4, // represents a command
    FlagFunction = 8, // this flag is implemented as a function
    FlagNeedsArgument = 16, // flag is not bool or a function of no arguments
    FlagIsArray = 32 // variable is an array of values of this type
};

struct FlagEntry_ {
    // fields mirroring the spec string (publically available)
    str_t name, help, tname;
    str_t defname;
    char alias,arrsep;
    void *pflag; // pointer to user data
    ValueType type;
    PValue defval;    // default value, NULL otherwise
    str_t error;
    int flags;
    FlagEntry **args;  // array for parameters for _named command_
};

#define arg_is_command(fe) ((fe)->flags & FlagCommand)
#define arg_is_used(fe) ((fe)->flags & FlagUsed)
#define arg_set_used(fe) ((fe)->flags |= FlagUsed)
static void arg_set_unused(FlagEntry *fe) {
    if (arg_is_used(fe))
        fe->flags -= 1;
}

#define C (char*)
char *typenames[] = {
   "int",C ValueInt,
    "float",C ValueFloat,
    "string",C ValueString,
    "bool",C ValueBool,
    "infile",C ValueFileIn,
    "outfile",C ValueFileOut,
    "void",C ValuePointer,
    NULL
};
#undef C

static ValueType parse_type(str_t name)
{
    return (ValueType)(intptr_t)str_lookup(typenames,name);
}

static void *value_parse_ex(str_t s, int type) {
    if (type == ValueFileIn) {
        if (str_eq(s,"stdin"))  {
            return stdin;
        } else {
            FILE *in = fopen(s,"r");
            if (! in)
                return value_errorf("cannot open '%s' for reading",s);
            else
                return in;
        }
    } else
    if (type == ValueFileOut) {
        if (str_eq(s,"stdout"))  {
            return stdout;
        } else {
            FILE *out = fopen(s,"w");
            if (! out)
                return value_errorf("cannot open '%s' for writing",s);
            else
                return out;
        }
    } else {
        return value_parse(s,(ValueType)type);
    }
}

static bool arg_is_flag(FlagEntry *pfd) {
    return pfd->args == NULL || (pfd->flags & FlagFunction);
}

// this gives us a name (optional) and a type, which may have a default value.
static bool parse_flag(str_t arg, FlagEntry *pfd)
{
    char **parts = str_split(arg," ");

    if (array_len(parts) > 1) { // type name ...
        pfd->tname = parts[0];
        pfd->type = parse_type(parts[0]);
        if (pfd->type == 0) goto out;
        char *rest = parts[1];
        char *is_arr = strstr(rest,"[]");
        if (strchr(rest,'=') && ! is_arr) { //  type name=default
            char **nparts = str_split(rest,"=");
            pfd->defname = nparts[1];
            pfd->defval = value_parse_ex(nparts[1],pfd->type);
            pfd->name = ref(nparts[0]);
        } else { // type name ([]))
            if (is_arr) { // type name[](=split)
                pfd->flags |= FlagIsArray;
                *is_arr = '\0';
                // array split by separator, not by using multiple flag values
                is_arr += 2;
                if (*is_arr == '=')
                    pfd->arrsep = *(is_arr+1);
            }
            if (pfd->flags & FlagIsArray) {
                pfd->defval = array_new(PValue,0);
            } else
            if (pfd->type == ValueBool) { // bool has sensible default 'false'
                pfd->defval = value_bool(false);
                pfd->defname = "false";
            } else {
                pfd->defval = NULL;
            }
            pfd->name = obj_ref(rest);
        }
        //obj_unref(parts);
    } else { // must be just a type
        pfd->type = parse_type(arg);
        pfd->name = NULL;
    }
out:
    if (pfd->type==0) {
        pfd->error = (const char*)value_errorf("'%s' is not a known type",parts[0]);
    }
    return ! pfd->error;
}

static FlagEntry *new_flag_entry() {
    FlagEntry *pfd = obj_new(FlagEntry,NULL);
    memset(pfd,0,sizeof(FlagEntry));
    return pfd;
}

// Representation of commands: these are a generalization of flags, which have an
// args array of subflag entries. These are bound to an allocated array of values.

static FlagEntry *parse_spec(str_t spec)
{
    FlagEntry *pfd = new_flag_entry();
    if (strchr(spec,')')) { // describes a function flag or command
        char **parts = str_split(spec,"()"); 
        char **tname = str_split(parts[0]," ");
        // we now have 'type', name and formal args
        pfd->flags |= FlagCommand;
        pfd->name = tname[1];
        if (! str_eq(tname[0],"cmd")) {
            pfd->flags |= FlagFunction;
            if (! str_eq(tname[0],"void"))
                parts[1] = str_fmt("%s x",tname[0]);
            // so int flag() -> flag(int x)
        }
        if (parts[1]) {
            char **fargs = str_split(parts[1],",");
            pfd->flags |= FlagNeedsArgument;
            // will be subflags for each argument to that function
            FlagEntry **A = pfd->args = array_new(FlagEntry*,array_len(fargs));
            for (char **F = fargs; *F; ++F, ++A) {
                FlagEntry *afe = new_flag_entry();
                *A = afe;
                afe->flags = FlagValue;
                if (! parse_flag(*F,afe)) {
                    pfd->error = afe->error;
                    break;
                }
            }
        }  else {
            pfd->args = array_new(FlagEntry*,0);
        }
    } else { // a flag
        parse_flag(spec,pfd);
        pfd->args = NULL;
        if (pfd->type != ValueBool || (pfd->flags & FlagIsArray))
            pfd->flags |= FlagNeedsArgument;
    }
    return pfd;
}

static PValue *allocate_args(FlagEntry *pfd, ArgState *cmds) {
    PValue *res = array_new_ref(PValue,array_len(pfd->args)+1), *P;
    res[0] = cmds; // first entry will be ArgState
    P = res + 1;
    // bind_value calls will result in each entry of this array being populated with values.
    for (FlagEntry **F = pfd->args; *F; ++F, ++P) {
        (*F)->pflag = P;
    }
    return res;
}

typedef void *(*CmdFun)(void **);

static PValue call_command(FlagEntry *pfd, PValue *args) {
    int na = 1, nf = array_len(pfd->args);
    while (args[na])
        ++na;
    if (na-1 < nf) { // complete any arguments with default values
        // args[0] is args data, so like with argv the actual arguments start with 1...
        for (int i = na; i < nf+1; ++i) {
            PValue def = pfd->args[i-1]->defval;
            if (def)
                args[i] = obj_ref(def);
            else
                return value_errorf("no argument #%d provided for '%s'\n",i,pfd->name);
        }
    }
    return ((CmdFun)pfd->pflag) (args);
}

#define CAST(T,P) *((T*)P)
static void set_value(void *P, PValue v, bool use_value) {
    //printf("setting %p with value %p %d %d\n",P,v,use_value, value_as_int(v));
    if (use_value) {
        CAST(PValue,P) = v;
    } else
    if (value_is_string(v)) {
        CAST(str_t,P) = (str_t)v;
    } else
    if (value_is_int(v)) {
        CAST(int,P) = value_as_int(v);
    } else
    if (value_is_float(v)) {
        CAST(double,P) = value_as_float(v);
    } else
    if (value_is_bool(v)) {
        CAST(bool,P) = value_as_bool(v);
    } else {
        CAST(PValue,P) = v;
    }
}

/// extract raw values from args array.
// Convenient way to unpack the array of values passed to commands and function flags.
// @usage PValue two(PValue *args) {  double x;  char *name;  arg_get_values(args,&x,&name); ...
void arg_get_values(PValue *vals,...) {
    va_list ap;
    va_start(ap,vals);
    for(int i = 1; i < array_len(vals); i++) { // args[0] is flag data...
        void *P = va_arg(ap,void*);
        set_value(P,vals[i],false);
    }
    va_end(ap);
}

static PValue bind_value(FlagEntry *pfd, str_t arg) {
    PValue v;
    int flags = pfd->flags;
    bool is_array = (flags & FlagIsArray) != 0;
    bool is_direct_array = is_array && pfd->arrsep;

    if (arg && ! is_direct_array) {
        if (pfd->type == ValueBool) // again, bools are special...
            arg = "true";
        v = value_parse_ex(arg,pfd->type);
    } else {
        v = obj_ref(pfd->defval);
    }

    if (value_is_error(v))
        return v;

    if (is_array && arg) {
        if (is_direct_array) {
            char sep[2];
            sep[0] = pfd->arrsep;
            sep[1] = '\0';
            char **parts = str_split(arg,sep);
            PValue *varr = array_new(PValue,array_len(parts));
            FOR(i,array_len(varr)) {
                varr[i] = value_parse_ex(parts[i],pfd->type);
            }
            CAST(PValue,pfd->pflag) = varr;
        } else { // each repeated flag adds a new value
            PValue **vseq;
            if (! (flags & FlagUsed)) { // create a sequence initially
                vseq = seq_new(PValue);
                CAST(PValue,pfd->pflag) = vseq;
            } else {
                vseq = (PValue**)CAST(PValue,pfd->pflag);
            }
            // and add each new value to that sequence!
            seq_add(vseq,v);
        }
    } else {
        set_value(pfd->pflag, v, flags & FlagValue);
    }
    return v;
}

// set any default values, and complain if required flags/parameters are not found!
static PValue finish_off_entry(FlagEntry *fe) {
    if (! arg_is_command(fe))  { // just for flags...
            if (! arg_is_used(fe)) {  // copy over default, if present...
                if (! fe->defval)
                    return value_errorf("'%s' is a required flag",fe->name);
                else
                    return bind_value(fe,NULL);
            } else  // finalize arrays 
            if (fe->flags & FlagIsArray) {
                PValue *varr;
                if (fe->arrsep) {
                    varr = CAST(PValue,fe->pflag);
                } else {  // the sequence must become a proper array
                    PValue **vseq = (PValue**)fe->pflag;
                    varr = (PValue*)seq_array_ref(vseq);
                }
                int n = array_len(varr);
                if (n > 0 && value_is_box(varr[0])) {
                    PValue ptr;
                    // boxed values get unpacked specially
                    if (value_is_int(varr[0])) {
                        int *res = array_new(int,n);
                        FOR(i,n) res[i] = value_as_int(varr[i]);
                        ptr = res;
                    } else
                    if (value_is_bool(varr[0])) {
                        bool *res = array_new(bool,n);
                        FOR(i,n) res[i] = value_as_bool(varr[i]);
                        ptr = res;
                    } else {
                        double *res = array_new(double,n);
                        FOR(i,n) res[i] = value_as_float(varr[i]);
                        ptr = res;
                    }
                    CAST(PValue,fe->pflag) = ptr;
                } else { // empty or array of objects
                    CAST(PValue,fe->pflag) = varr;
                }
            }
    }
    return NULL;
}

#undef CAST

static FlagEntry *lookup(ArgState *cmds, str_t name) {
    return (FlagEntry*)str_lookup(cmds->cmd_map,name);
}

static void print_type(FlagEntry *fe) {
    if (fe->defname) {
        printf("%s",fe->defname);
    } else {
        printf("%s",fe->tname);
        if (fe->flags & FlagIsArray)
            printf("...");
    }    
}

static void *help(void **args) {
    ArgState *fd = (ArgState*)args[0];
    FlagEntry **cmds = (FlagEntry**)fd->cmds, **pc;
    if (fd->usage) {
        printf("%s\n",fd->usage);
    }
    if (fd->has_commands) {
        printf("Commands:\n");
        for (pc = cmds; *pc; ++pc) {
            FlagEntry *fe = *pc;
            if (! arg_is_flag(fe)) {
                printf("\t%s (",fe->name);
                for (FlagEntry **afe = fe->args; *afe; ++afe) {
                    print_type(*afe);
                    if (*(afe+1))
                        printf(",");
                }                
                printf(")\t%s\n",fe->help);
            }
        }
    }
    printf("Flags:\n");
    for (pc = cmds; *pc; ++pc) {
        FlagEntry *fe = *pc;
        if (arg_is_flag(fe)) {            
            printf("\t--%s",fe->name);
            if (fe->alias)
                printf(",-%c",fe->alias);
            if (fe->tname) {
                printf(" (");
                print_type(fe);
                printf(")");
            }
            printf("\t%s\n",fe->help);
        }
    }
    // that is, we want to immediately bail out with no error condition...
    return value_error("ok");
}

// how command/flag arguments are encoded in the map
static char *argument_name(str_t prefix, int karg) {
    return str_fmt("%s#%d",prefix,karg);    
}

static ArgState *parse_error(ArgState *res, str_t context, str_t msg) {
    res->error = str_fmt("'%s': %s",context,msg);
    return res;
}

static char* skip_ws(char *p) {
    while (*p && *p == ' ')
        ++p;
    if (! *p)
        return NULL;
    return p;
}

/// parse flag specification array.
ArgState *arg_parse_spec(PValue *flagspec)
{
    ArgState *res = obj_new(ArgState,NULL);
    memset(res,0,sizeof(ArgState));
    
    int nspec = 0;
    char* usage = (char*)*flagspec;
    if (str_starts_with(usage,"//")) {
        usage = skip_ws(usage+2);
        res->usage = usage;
        flagspec++;
    }
    for (PValue *cf = flagspec; *cf; ++cf)
        ++nspec;
    char **specs = array_new(char*,nspec+2);
    int k = 2;
    specs[0] = str_new("void help(); // -h help on commands and flags");
    specs[1] = (char*)help;
    for (PValue *cf = flagspec; *cf; cf += 2, k += 2) {
        specs[k] = str_new((char*)*cf);
        specs[k+1] = (char*) *(cf+1);
    }
    nspec /= 2;
    
    char ***cmds = smap_new(false);
    FlagEntry **ppfd = array_new(FlagEntry*,nspec+1);
    int i  = 0;
    for (char **cf = specs; *cf; cf += 2) {
        char *spec = (char*)*cf;
        char *comment = strchr(spec,';');
        if (! comment)
            return parse_error(res,spec,"semi-colon required!");
        *comment = '\0'; 
        // cool, spec is now the variable specification
        // now hunt for the comment
        comment++;
        comment = strstr(comment,"//");
        if (! comment)
            return parse_error(res,comment,"// comment expected");
        comment = skip_ws(comment + 2);
        if (! comment)
            return parse_error(res,spec,"no help text");
        
        str_trim(spec);
        FlagEntry *pfd = parse_spec(spec);
        if (pfd->error) {
            res->error = pfd->error;
            return res;
        }
        
        if (*comment == '-') {
            ++comment;
            if (*comment == '-')
                return parse_error(res,comment,"single-dash shortcut expected");
            pfd->alias = *comment;
            comment += 2;  // skip flag and space
        }
        pfd->help = comment;
        pfd->pflag = *(cf+1);
        ppfd[i++] = pfd;

        // update map of command names and their aliases
        smap_add(cmds,pfd->name,pfd);
        if (pfd->alias)
            smap_add(cmds,str_fmt("%c",pfd->alias),pfd);

        // commands have arguments, which we also need to store here...
        if (pfd->args) {
            int karg = 1;
            for (FlagEntry **F = pfd->args; *F; ++F, ++karg) {
                smap_add(cmds,argument_name(pfd->name,karg), *F);
            }
            if (! (pfd->flags & FlagFunction))
                res->has_commands = true;
        }
    }
    ppfd[i] = NULL;
    res->cmd_map = smap_close(cmds);
    res->cmds = ppfd;
    return res;
}

// all function/free arguments are bound by their compound name
static PValue bind_argument(ArgState *cmds, str_t name, int karg, str_t arg, FlagEntry **pfe) {
    char *aname = argument_name(name, karg);
    FlagEntry * fe = lookup(cmds,aname);
    if (! fe)
        return value_errorf("no argument %s",aname);
    if (pfe)
        *pfe = fe;
    obj_unref(aname);
    return bind_value(fe,arg);
}

static PValue finish_off_all_entries(ArgState *cmds) {
    for (FlagEntry **all = (FlagEntry **)cmds->cmds; *all; ++all) {
        PValue val = finish_off_entry(*all);
        if (value_is_error(val))
            return val;
    }
    return NULL;
}

/// Bind a set of variables to their values, using a simple map of name/value pairs.
PValue arg_bind_values(ArgState *cmds, SMap sm) {
    FOR_SMAP(key,val,sm) {
        FlagEntry *fe = lookup(cmds,key);
        if (! fe)
            return value_errorf("no such value %s",key);
        PValue res = bind_value(fe,val);
        if (value_is_error(res))
            return res;
        arg_set_used(fe);
    }
    return finish_off_all_entries(cmds);
}

/// reset used state of flags, needed to reparse commands.
void arg_reset_used(ArgState *cmds) {
    for (FlagEntry **all = (FlagEntry **)cmds->cmds; *all; ++all) {
        arg_set_unused (*all);
    }
}

/// parse the given command-line args, given the processed state.
// Call `arg_parse_spec` first to call this directly.
PValue arg_process(ArgState *cmds ,  const char**argv)
{
    PValue val;
    int flags;
    FlagEntry *fune, *cmd_fe;
    str_t prefix = "";
    PValue *cmd_parms = NULL;
    char *rest_arg = NULL;
    static char tmp[] = {0,0};
    int i = 1; // argv[0] is always program...
    int karg = 1;  // non-flag arguments
    
    if (cmds->has_commands && (argv[i] && *argv[i]  != '-')) {
        if (! argv[i])
            return value_error("expecting command");
        cmd_fe = lookup(cmds,argv[i]);
        if (! cmd_fe || ! (cmd_fe->flags & FlagCommand))
            return value_errorf("unknown command '%s'",argv[i]);
        prefix = cmd_fe->name;
        cmd_parms = allocate_args(cmd_fe,cmds);
        ++i;
    }
    while (argv[i]) { // std guarantees that argv ends with NULL
        bool long_flag;
        char *arg = (char*)argv[i];
        if (*arg == '-') { // flag
            char *pname = tmp;
            ++arg;
            *pname = *arg;
            long_flag = *arg=='-';
            if (long_flag)
                pname = arg+1;

            while (*pname) { // short flags may be combined in a chain...( -abc )
                fune = lookup(cmds,pname);
                if (! fune)
                    return value_errorf("unknown flag '%s'",pname);
                flags = fune->flags;
                
                bool needs_argument = flags & FlagNeedsArgument;

                // a chain of short flags must end if one needs an argument... (-vo file)
                if (needs_argument && ! long_flag) {
                    long_flag = true;
                    if (arg[1]) // anything following the flag is its argument ( -n10 )
                        rest_arg = arg+1;
                }

                if (needs_argument)  {
                    if (rest_arg) { // don't need to advance to get argument!
                        arg = rest_arg;
                        rest_arg = NULL;
                    } else {
                        arg = (char*)argv[++i];
                    }
                }
                if (flags & FlagFunction) { // a function flag (may have argument)
                    PValue *fun_args = allocate_args(fune,cmds);
                    if (needs_argument) {
                        val = bind_argument(cmds,fune->name,1,arg,NULL);
                        if (value_is_error(val))
                            return val;                        
                    }
                    val  = call_command(fune,fun_args);
                    fun_args[0] = NULL;
                    obj_unref(fun_args);
                } else { // flag is bound to a variable
                    val = bind_value(fune,arg);
                }
                if (value_is_error(val))
                    return val;                
                arg_set_used(fune);
                if (long_flag)
                    break;
                ++arg;  // next short flag
                *pname = *arg;
            }
        } else { // parameter or command
            val = bind_argument(cmds,prefix,karg,arg,&fune);
            if (value_is_error(val))
                return val;
            arg_set_used(fune);
            if (! (fune->flags & FlagIsArray))
                ++karg;
        }
        ++i;  // next argument
    }

    val = finish_off_all_entries(cmds);
    if (value_is_error(val))
        return val;

    if (cmd_parms) { // was a command; we can now call it..
        val = call_command(cmd_fe,cmd_parms);
        cmd_parms[0] = NULL;        
        obj_unref(cmd_parms);
        if (value_is_error(val))
            return val;
        if (! (cmd_fe->flags & FlagFunction) )
            return value_error("ok");
        return val;
    }
    return NULL; // meaning OK ....    
}

/// quit the application with a message, optionally showing help.
void arg_quit(ArgState *cmds, str_t msg, bool show_help) {
    if (*msg)
        fprintf(stderr,"error: %s\n",msg);
    if (show_help)
        help((void**)&cmds);
    exit(1);
}

/// parse command-line arguments, binding flags to their values.
// It assumes that `argv` terminates in `NULL`, which is 
// according to the C specification. Any error will cause the
// application to quit, showing help.
ArgState *arg_command_line(void** argspec, const char** argv) {
    ArgState *cmds = arg_parse_spec(argspec);
    if (cmds->error) {
        arg_quit(cmds,cmds->error,false);
    }
    char *res = (char*)arg_process(cmds,argv);
    if (res) {
        if (value_is_error(res)) {
            if (str_eq(res,"ok")) {
                exit(0);
            }
            arg_quit(cmds,res,true);
        }
    }
    return cmds;
}
