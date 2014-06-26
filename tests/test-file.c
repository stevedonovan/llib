/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <assert.h>
#include <string.h>
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
    Str *lines = file_files_in_dir(ext,false);
    array_sort(lines,ARRAY_STRING,false,0);
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

#ifdef _WIN32
Str paths[] = {
    "c:\\users\\steve\\myfiles\\bonzo.txt",
    "bonzo.txt",
    "c:\\users\\steve\\myfiles\\",
    ".txt",
    "c:\\users\\steve\\myfiles\\bonzo.tmp"
};
#else
Str paths[] = {
    "/home/steve/myfiles/bonzo.txt",
    "bonzo.txt",
    "/home/steve/myfiles/",
    ".txt",
    "/home/steve/myfiles/bonzo.tmp"
};
#endif

int eq(const char *s1, const char *s2) {
    //printf("'%s' == '%s'?\n",s1,s2);
    return strcmp(s1,s2) == 0;
}

void path_manipulation()
{
    Str path = paths[0];
    assert(eq(file_basename(path),paths[1]));
    assert(eq(file_dirname(path),paths[2]));
    assert(eq(file_extension(path),paths[3]));
    assert(eq(file_replace_extension(path,".tmp"),paths[4]));
    //DUMPS(file_expand_user("~/myfile"));
}

int main(int argc, char **argv)
{
    if (file_exists("test-file.c","r"))
        puts("file exists");
    printf("makefile was '%s'\n",file_exists_any("r","proj.mak","makefile"));
    text_from_file();
    reading_lines("test-file.c");
    getting_files("*.c");
    printf("remaining %d\n",obj_kount());
    path_manipulation(NULL);
//    char *res = file_command("which test-file.exe");
//    printf("[%s]\n",res);
    return 0;
}
