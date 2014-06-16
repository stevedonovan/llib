/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <llib/file.h>

typedef char *Str;

void reading_lines(Str file)
{
    FILE *f = fopen(file,"r");
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

#define DUMPS(expr) printf("%s = '%s'\n",#expr,expr)

void path_manipulation(const char *path)
{
    if (! path)
#ifdef _WIN32
        path = "c:\\users\\steve\\myfiles\\bonzo.txt";
#else
        path = "/home/steve/myfiles/bonzo.txt";
#endif
    DUMPS(file_basename(path));
    DUMPS(file_dirname(path));
    DUMPS(file_extension(path));
    DUMPS(file_replace_extension(path,".tmp"));
    DUMPS(file_expand_user("~/myfile"));
}

int main(int argc, char **argv)
{
    char *files[] = {"Makefile","makefile",NULL};
    if (file_exists("test-file.c","r"))
        puts("file exists");
    printf("makefile was '%s'\n",file_exists_any("r",files));
    text_from_file();
    reading_lines("test-file.c");
    getting_files("*.c");
    printf("remaining %d\n",obj_kount());
    path_manipulation(NULL);
    char *res = file_command("which test-file.exe");
    printf("[%s]\n",res);
    return 0;
}
