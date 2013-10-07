/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <llib/file.h>

typedef char *Str;

void reading_lines(Str file)
{
    FILE *f = fopen_or_die(file,"r");
    printf("size was %d bytes\n",file_size(file));
    Str *lines = file_getlines(f);
    printf("no of lines %d \n",array_len(lines));
    fclose(f);
    unref(lines);
}

void getting_files(char *ext)
{
    Str *lines = file_files_in_dir(ext,1);
    FOR_ARR(Str,p,lines)
        printf("'%s'\n",*p);
    unref(lines);
}

void text_from_file()
{
    char *res = file_read_all("hello.txt",true);
    for (char *p = res; *p; ++p)
        printf("%02X ",*p);
    printf("\n");
    printf("got '%s'\n",res);
    unref(res);
}

int main(int argc, char **argv)
{
    text_from_file();
    reading_lines("test-file.c");
    getting_files("*.c");
    printf("remaining %d\n",obj_kount());
    return 0;
}
