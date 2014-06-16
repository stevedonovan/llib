// Alternative way of using config; take the resulting simple map
// and pass it through the arg parser. This handles things like
// array and default values transparently!

#include <stdio.h>
#include <llib/config.h>
#include <llib/dbg.h>
#include <llib/arg.h>

char* name;
int a;
int *ids;
double b;

PValue vars[] = {
   "string name; // name of server",&name,
    "int a=0; // priority level",&a,
    "int ids[]=,; // acceptable id values",&ids,
    "float b; // fudge factor",&b,    
    NULL
};

int main(int argc,  const char **argv)
{
    char **S = config_read(argv[1] ? argv[1] : "test.cfg");
    ArgState *state = arg_parse_spec(vars);
    PValue err = arg_bind_values(state,S);
    if (err) {
        fprintf(stderr,"error: %s\n",value_as_string(err));
        return 1;
    }
    
    DS(name);
    DI(a);
    DUMPA(int,ids,"%d");
    DF(b);
        
    return 0;
}
