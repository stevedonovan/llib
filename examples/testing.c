#include <stdlib.h>
#include <llib/file.h>
#include <llib/str.h>

str_t pat = "$ ./";
#ifdef _WIN32
int pat_offs = 0;
#else
int pat_offs = -2;
#endif

#define errorf(fmt,...) fprintf(stderr,fmt,__VA_ARGS__)

int main(int argc, char **argv)
{
    char line[256];
    int pat_len = strlen(pat) + pat_offs;
    FILE* f = fopen(argc > 1 ? argv[1] : "test.file","r");
    int L = 0;
    while (file_gets(f,line,sizeof(line))) {
        L++;
        char *p = strstr(line,pat);
        char *cmd = p + pat_len;
        printf("cmd: %s\n",cmd);
        char **out = file_command_lines(cmd);
        while (*out && file_gets(f,line,sizeof(line))) {
            L++;
            if (! *out || ! str_eq(line,*out)) {
                errorf("mismatch line %d\n\t%s\n\t%s\n",L,line,*out);
                exit(1);
            }
            ++out;
        }
    }
    fclose(f);
    return 0;
}
