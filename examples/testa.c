// The llib command-line parser also allows flags to be implemented as functions.
// It is then a small step towards providing an interactive command-line.
#include <stdio.h>
#include <llib/file.h>
#include <llib/str.h>
#include <llib/value.h>
#include <llib/arg.h>

static const char **incdirs;
static const char **string_args;
static int a;
static bool interactive;

PValue test(PValue *args) {
    printf("gotcha! %d %d\n",value_as_int(args[1]),a);
    printf("the include paths\n");
    FOR_ARR (str_t,P,incdirs)
        printf("'%s'\n",*P);
    return NULL;
}

PValue two(PValue *args) {
    double x;
    char *name;
    arg_get_values(args,&x,&name);
    printf("x = %f, name = '%s'\n",x,name);
    return NULL;
}

PValue kount(PValue *args) {
    // the llib debug options
    printf("kount %d\n",obj_kount());
#ifdef LLIB_DEBUG    
    // this tells you what was created/destroyed by type and number
    obj_snapshot_dump();
    obj_snapshot_create();
#endif
#ifdef LLIB_PTR_LIST
    // if llib is managing an explicit list of used pointers, can tell us exactly what they are
    obj_snap_ptrs_dump();
    obj_snap_ptrs_create();
#endif
    return NULL;
}

PValue flag(PValue *args) {
    printf("flag %f\n",value_as_float(args[1]));
    return NULL;
}

PValue args[] = {
    // commands
    "cmd test(int i=0); // -T test command",test,
    "cmd two(float x,string name); // -2 test cmd_get_values",two,
    // flags implemented with functions
    "void kount(); // -k referenced object count flag",kount,
    "float flag(); // -f function flag taking float..",flag,
    // regular bound flags
    "string include[]; // -I include directories", &incdirs,
    "int a=0; // flag value",&a,
    "bool interactive=false; // -i interactive mode",&interactive,
    // other arguments, as string array
    "string #1[]; // array of string args",&string_args,
    NULL
};

int main(int argc,  const char **argv)
{
    ArgState *state = arg_command_line(args,argv);
    if (! interactive) {
        if (array_len(incdirs) > 0) {
            printf("the include paths\n");
            FOR_ARR (str_t,P,incdirs)
                printf("'%s'\n",*P);
        }
        if (array_len(string_args) > 0) {
            printf("the string args\n");
            FOR_ARR (str_t,P,string_args)
                printf("'%s'\n",*P);
        }
        printf("flag a is %d\n",a);
    } else {
        char *line;
            printf("> ");
        while ((line = file_getline(stdin)) != NULL) {
            char **parts = str_split(line," ");
            // args_process assumes args start at second element, hence -1 here
            PValue v = arg_process(state,(const char**)parts-1);
            if (v != NULL) {
                printf("%s\n",value_tostring(v));
                unref(v);
            }
            dispose(parts,line);
            printf("> ");
            arg_reset_used(state);
        }        
    }
    return 0;
}
