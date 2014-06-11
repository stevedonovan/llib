// This demonstrates the llib command-line argument parser.
// The flags are declared as regular C variables, and their exported name
// and type is given in an array of ArgFlags structs.
#include <stdio.h>
#include <llib/arg.h>

int lines;
FILE *file;
bool verbose, print_lines;

PValue args[] = {
    "// cmd: show n lines from top of a file",
    "int lines=10; // -n number of lines to print",&lines,
    "bool verbose=false; // -v controls verbosity",&verbose,
    "bool lineno; // -l output line numbers",&print_lines,
    "infile #1=stdin; // file to dump",&file,
    NULL
};

int main(int argc,  const char **argv)
{
    arg_command_line(args,argv);
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
