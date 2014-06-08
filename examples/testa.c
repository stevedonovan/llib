// The llib command-line parser also allows flags to be implemented as functions.
// It is then a small step towards providing an interactive command-line.
#include <stdio.h>
#include <llib/file.h>
#include <llib/str.h>
#include <llib/value.h>
#include <llib/arg.h>

static char **incdirs;
static char **string_args;
static int a;
static bool interactive;

PValue test(PValue *args) {
    printf("gotcha! %d %d\n",value_as_int(args[1]),a);
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
    return str_fmt("kount %d",obj_kount());
    //printf("kount %d\n",obj_kount());
    //return NULL;
}

ArgFlags args[] = {
    {"string include[]",'I',&incdirs,"include directory"},
    {"--test(int i=0)",'T',test,"test function flag"},
    {"--two(float x,string name)",'2',two,"test cmd_get_values"},
    {"--kount()",'k',kount,"referenced object count"},
    {"int a=0",'a',&a,"flag value"},
    {"bool interactive=false",'i',&interactive,"interactive mode"},
    {"string #1[]",0,&string_args,"array of string args"},
    {NULL,0,NULL,NULL}
};

int main(int argc,  const char **argv)
{
    ArgState *state = arg_command_line(args,argv);
    if (! interactive) {
        if (array_len(incdirs) > 0) {
            printf("the include paths\n");
            FOR_ARR (char*,P,incdirs)
                printf("'%s'\n",*P);
        }
        if (array_len(string_args) > 0) {
            printf("the string args\n");
            FOR_ARR (char*,P,string_args)
                printf("'%s'\n",*P);
        }
        printf("flag a is %d\n",a);
    } else {
        char *line;
        arg_functions_as_commands(state);    
        printf("> ");
        while ((line = file_getline(stdin)) != NULL) {
            char **parts = str_split(line," ");
            // args_process assumes args start at second element, hence -1 here
            PValue v = arg_process(state,parts-1);
            if (v != NULL) {
                printf("%s\n",value_tostring(v));
                unref(v);
            }
            dispose(parts,line);
            printf("> ");
        }        
    }
    return 0;
}
