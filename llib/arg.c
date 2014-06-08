/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
### Command-line Argument Parser

This allows you to bind variables to command-line arguments, providing
the flag name/type, a shortcut, a pointer to the variable, and some help text.

```C
int lines;
FILE *file;
bool verbose, print_lines;

ArgFlags args[] = {
    {"int lines=10",'n',&lines,"number of lines to print"},
    {"bool verbose=false",'v',&verbose,"controls verbosity"},
    {"bool lineno",'l',&print_lines,"output line numbers"},
    {"infile #1=stdin",0,&file,"file to dump"},
    {NULL,0,NULL,NULL}
};

```

If you now call `arg_command_line(args,argv)` these variables will be
bound; note how both type and optional default value are specified. Names like '#1'
refer to the first non-flag argument and so forth.  Both '--lines' and '-n' can be 
used to set the integer variable `lines'.

If a conversion is not possible (not a integer, file cannot be opened, etc)
then the program will exit.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "arg.h"

typedef char* Str;
typedef const char* CStr;

enum {
    ValueFileIn = 0x200,
    ValueFileOut = 0x201
};

enum {
    FlagUsed = 1, // flag was explicitly referenced
    FlagValue = 2, // user data is a value object, not rawa
    FlagCommand = 4, // represents a command
    FlagFunction = 8, // this flag is implemented as a function
    FlagNeedsArgument = 16, // flag is bool, or a function of no arguments
    FlagIsArray = 32 // variable is an array of values of this type
};

typedef struct FlagEntry_ FlagEntry;

struct FlagEntry_ {
    void *pflag; // pointer to user data
    Str name;
    ValueType type;
    ArgFlags *arg_spec;
    PValue defval;    // default value, NULL otherwise
    const char *error;
    int flags;
    FlagEntry **args;  // array for parameters for _named command_
};

#define arg_is_command(fe) ((fe)->flags & FlagCommand)
#define arg_is_used(fe) ((fe)->flags & FlagUsed)
#define arg_set_used(fe) ((fe)->flags |= FlagUsed)

#define C (char*)
char *typenames[] = {
   "int",C ValueInt,
    "float",C ValueFloat,
    "string",C ValueString,
    "bool",C ValueBool,
    "infile",C ValueFileIn,
    "outfile",C ValueFileOut,
    NULL
};
#undef C

static ValueType parse_type(const char *name)
{
    return (ValueType)(intptr)str_lookup(typenames,name);
}

static void *value_parse_ex(const char *s, int type) {
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
    return pfd->args == NULL;
}

// this gives us a name (optional) and a type, which may have a default value.
static bool parse_flag(const char *arg, FlagEntry *pfd)
{
    char **parts = str_split(arg," ");

    if (array_len(parts) > 1) { // type name ...
        pfd->type = parse_type(parts[0]);
        if (pfd->type == 0) goto out;
        char *rest = parts[1];
        if (strchr(rest,'=')) { //  type name=default
            char **nparts = str_split(rest,"=");
            pfd->defval = value_parse_ex(nparts[1],pfd->type);
            pfd->name = ref(nparts[0]);
        } else { // type name ([]))
            if (str_ends_with(rest,"[]")) { // (type name[])
                pfd->flags |= FlagIsArray;
                *strchr(rest,'[') = '\0';
            }
            if (pfd->type == ValueBool) // bool has sensible default 'false'
                pfd->defval = value_bool(false);
            else
            if (pfd->flags & FlagIsArray)
                pfd->defval = array_new(PValue,0);
            else
                pfd->defval = NULL;
            pfd->name = obj_ref(rest);
        }
        obj_unref(parts);
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

// Representation of commands: these are a generalization of flags, which have an
// args array of subflag entries. These are bound to an allocated array of values.

static FlagEntry *parse_spec(const char *spec)
{
    FlagEntry *pfd = obj_new(FlagEntry,NULL);
    pfd->flags = 0;
    pfd->error = NULL;
    if (strchr(spec,')')) { // describes a function
        char **parts = str_split(spec,"()");
        pfd->flags |= FlagCommand;
        pfd->name = parts[0];
        if (str_starts_with(pfd->name,"--")) {
            pfd->flags |= FlagFunction;
            pfd->name += 2;
        }
        if (parts[1]) {
            char **fargs = str_split(parts[1],",");
            pfd->flags |= FlagNeedsArgument;
            // will be subflags for each argument to that function
            FlagEntry **A = pfd->args = array_new(FlagEntry*,array_len(fargs));
            for (char **F = fargs; *F; ++F, ++A) {
                FlagEntry *afe = obj_new(FlagEntry,NULL);
                afe->error = NULL;
                *A = afe;
                afe->flags |= FlagValue;
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
        if (pfd->type != ValueBool)
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
                args[i] = def;
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
        CAST(CStr,P) = (CStr)v;
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
void arg_get_values(PValue *vals,...) {
    va_list ap;
    va_start(ap,vals);
    for(int i = 1; i < array_len(vals); i++) { // args[0] is flag data...
        void *P = va_arg(ap,void*);
        set_value(P,vals[i],false);
    }
    va_end(ap);
}

static PValue bind_value(FlagEntry *pfd, const char *arg) {
    PValue v;
    int flags = pfd->flags;

    if (arg) {
        if (pfd->type == ValueBool) // again, bools are special...
            arg = "true";
        v = value_parse_ex(arg,pfd->type);
    } else {
        v = pfd->defval;
    }

    if (value_is_error(v))
        return v;

    if (flags & FlagIsArray && arg) {
        PValue **vseq;
        if (! (flags & FlagUsed)) { // create a sequence initially
            vseq = seq_new(PValue);
            CAST(PValue,pfd->pflag) = vseq;
        } else {
            vseq = (PValue**)CAST(PValue,pfd->pflag);
        }
        // and add each new value to that sequence!
        seq_add(vseq,v);
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
            } else  // finalize arrays - the sequence must become a proper array
            if (fe->flags & FlagIsArray) {
                PValue **vseq = (PValue**)CAST(PValue,fe->pflag);
                CAST(PValue,fe->pflag) = seq_array_ref(vseq);
            }
    }
    return NULL;
}

#undef CAST

void arg_functions_as_commands(ArgState *cmds) {
    FlagEntry **F = (FlagEntry**)cmds->cmds;
    cmds->has_commands = true;
    for (; *F; ++F) {
        if ((*F)->flags & FlagFunction)
            (*F)->flags &= (FlagFunction|FlagCommand);
    }
}

static FlagEntry *lookup(ArgState *cmds, const char *name)
{
    return (FlagEntry*)str_lookup(cmds->cmd_map,name);
}

static void *help(void **args) {
    ArgState *fd = (ArgState*)args[0];
    FlagEntry **cmds = (FlagEntry**)fd->cmds;
    if (fd->has_commands) {
        printf("Commands:\n");
        for (; *cmds; ++cmds) {
            if (! arg_is_flag(*cmds))
                printf("\t%s\t%s\n",(*cmds)->arg_spec->spec, (*cmds)->arg_spec->help);
        }
    }
    printf("Flags:\n");
    for (cmds = (FlagEntry**)fd->cmds; *cmds; ++cmds) {
        if (arg_is_flag(*cmds))
            printf("\t--%s\t%s\n",(*cmds)->name, (*cmds)->arg_spec->help);
    }
    return value_error("");
}

static ArgFlags help_spec =   {"--help()",'h',&help,"help on commands and flags"};

ArgState *arg_parse_spec(ArgFlags *flagspec)
{
    ArgState *res = obj_new(ArgState,NULL);
    memset(res,0,sizeof(ArgState));
    int nspec = 0;
    for (ArgFlags *cf = flagspec; cf->spec; ++cf)
        ++nspec;

    char ***cmds = smap_new(false);
    FlagEntry **ppfd = array_new(FlagEntry*,nspec+1);
    FOR(i,nspec+1) {
        ArgFlags *cf = (i < nspec) ? &flagspec[i] : &help_spec;
        FlagEntry *pfd = parse_spec(cf->spec);
        if (pfd->error) {
            res->error = pfd->error;
            return res;
        }
        pfd->arg_spec = cf;
        pfd->pflag = cf->flagptr;
        ppfd[i] = pfd;

        // update map of command names and their aliases
        smap_add(cmds,pfd->name,pfd);
        if (cf->alias)
            smap_add(cmds,str_fmt("%c",cf->alias),pfd);

        // commands have arguments, which we also need to store here...
        if (pfd->args) {
            int karg = 1;
            for (FlagEntry **F = pfd->args; *F; ++F, ++karg) {
                smap_add(cmds,str_fmt("%s#%d",pfd->name,karg), *F);
            }
            if (! (pfd->flags & FlagFunction))
                res->has_commands = true;
        }
    }

    res->cmd_map = (SMap)seq_array_ref(cmds);
    res->cmds = ppfd;
    return res;
}

PValue arg_process(ArgState *cmds ,  const char **argv)
{
    PValue val;
    int flags;
    FlagEntry *fune, *cmd_fe;
    const char *prefix = "";
    PValue *cmd_parms = NULL;
    char *rest_arg = NULL;
    static char tmp[] = {0,0};
    int i = 1; // argv[0] is always program...
    int karg = 1;  // non-flag arguments
    if (cmds->has_commands) {
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

                // a chain of short flags must end if one needs an argument... (-vo file)
                if ((flags & FlagNeedsArgument) && ! long_flag) {
                    long_flag = true;
                    if (arg[1]) // anything following the flag is its argument ( -n10 )
                        rest_arg = arg+1;
                }

                if (flags & FlagFunction) { // a function flag
                    if (cmd_parms) { // already one pending., so call it
                        val = call_command(cmd_fe,cmd_parms);
                        if (value_is_error(val))
                            return val;
                    }
                    karg = 1;
                    cmd_fe = fune;
                    prefix = cmd_fe->name;
                    cmd_parms = allocate_args(cmd_fe,cmds);
                } else { // flag is bound to a variable (bool flags toggle default value)
                    if (flags & FlagNeedsArgument)  {
                        if (rest_arg) { // don't need to advance to get argument!
                            arg = rest_arg;
                            rest_arg = NULL;
                        } else
                            arg = (char*)argv[++i];
                    }
                    val = bind_value(fune,arg);
                    if (value_is_error(val))
                        return val;
                }
                fune->flags |= FlagUsed;
                if (long_flag)
                    break;
                ++arg;  // next short flag
                *pname = *arg;
            }
        } else { // parameter or command
            char *aname = str_fmt("%s#%d",prefix,karg);
            fune = lookup(cmds,aname);
            if (! fune)
                return value_errorf("no argument %s",aname);
            val = bind_value(fune,arg);
            if (value_is_error(val))
                return val;
            arg_set_used(fune);
            if (! (fune->flags & FlagIsArray))
                ++karg;
        }
        ++i;  // next argument
    }

    for (FlagEntry **all = (FlagEntry **)cmds->cmds; *all; ++all) {
        val = finish_off_entry(*all);
        if (val && value_is_error(val))
            return val;
    }

    if (cmd_parms) {
        val = call_command(cmd_fe,cmd_parms);
        if (value_is_error(val))
            return val;
        if (! (cmd_fe->flags & FlagFunction) )
            return value_error("ok");
        return val;
    }
    return NULL; // meaning OK ....
}

ArgState *arg_command_line(ArgFlags *argspec, const char **argv) {
    ArgState *cmds = arg_parse_spec(argspec);
    if (cmds->error) {
        fprintf(stderr,"error: %s\n",cmds->error);
        exit(1);
    }
    char *res = (char*)arg_process(cmds,argv);
    if (res) {
        if (value_is_error(res)) {
            if (str_eq(res,"ok")) {
                exit(0);
            }        
            if (*res)
                fprintf(stderr,"error: %s\n",res);
            exit(1);            
        }
    }
    return cmds;
}

