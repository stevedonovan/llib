// This demonstrates the llib command-line argument parser.
// The flags are declared as regular C variables, and their exported name
// and type is given in an array of ArgFlags structs.
#include <stdio.h>
#include <llib/arg.h>

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

int main(int argc,  const char **argv)
{
    args_command_line(args,argv);
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
