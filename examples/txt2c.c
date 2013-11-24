/* A straightforward little program that helps with the awkwardness
 * of encoding text as a valid C string.

*/
#include <stdio.h>
#include<llib/str.h>

#define BUFSZ 256

int main(int argc, char**argv)
{
    FILE *in, *out;
    in = fopen(argv[1],"r");
    if (argv[2])
        out = fopen(argv[2],"w");
    else
        out = stdout;

    char buff[BUFSZ];
    fprintf(out,"const char *text = ");
    while (fgets(buff,BUFSZ,in)) {
        char **s = strbuf_new();
        char *p = buff;
        while (*p) {
            switch(*p) {
            case '\\': strbuf_adds(s,"\\\\"); break;
            case '\"': strbuf_adds(s,"\\\""); break;
            case '\n': strbuf_adds(s,"\\n"); break;
            case '\t': strbuf_adds(s,"\\t"); break;
            default: strbuf_add(s,*p);
            }
            ++p;
        }
        fprintf(out,"\n\t\"%s\"",*s);
    }
    fprintf(out,";\n\n");
    fclose(in);
    fclose(out);
    return 0;
}

