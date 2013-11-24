/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/****
Extended file handling.

*/

#ifdef _WIN32
#define DIR "dir /b"
#define PWD "%cd%\\"
#define DIR_SEP '\\'
#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif
#else
#define DIR "ls"
#define PWD "$PWD/"
// popen/pclose not part of C99 <stdio.h>
#define DIR_SEP '/'
#define _BSD_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"

#define LINESIZE 512
#define MAX_PATH 256

static void strip_eol(char *buff) {
    int last = strlen(buff)-1;
    if (buff[last] == '\n')
        buff[last] = '\0';
    --last;
    if (buff[last] == '\r')
        buff[last] = '\0';
}

/// like fgets, except trims (\r)\n.
char *file_gets(FILE *f, char *buff, int bufsize) {
    if (! fgets(buff,bufsize,f))
        return NULL;
    strip_eol(buff);
    return buff;
}

/// like `file_gets` but returns a refcounted string.
char *file_getline(FILE *f) {
    char buff[LINESIZE];
    if (! file_gets(f,buff,sizeof(buff)))
        return NULL;
    return str_new(buff);
}

/// old friend `fopen` with dignified suicide.
FILE *fopen_or_die(const char *file, const char *mode) {
    FILE *f = fopen(file,mode);
    if (f == NULL) {
        perror("fopen");
        exit(1);
    }
    return f;
}

static int size_of_file(FILE *fp) {
    int sz = fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return sz;
}

/// size of a file.
// Will return -1 if the file cannot be opened for reading.
int file_size(const char *file)
{
    int sz;
    FILE *fp = fopen(file,"rb");
    if (! fp)
        return -1;
    sz = size_of_file(fp);
    fclose(fp);
    return sz;
}

/// read the contents of a file.
// If `text` is true, will also strip any \r\n at the end.
char *file_read_all(const char *file, bool text) {
    FILE *fp = fopen(file,text ? "r" : "rb");
    if (! fp)
        return NULL;
    int sz = size_of_file(fp);
    char *res = str_new_size(sz);
    int nr = fread(res,1,sz,fp);
    if (nr != sz) {
        fprintf(stderr,"file_read_all: got %d expected %d\n",nr,sz);
    }
    if (text) {
        strip_eol(res);
    }
    fclose(fp);
    return res;
}

typedef char *Str;

/// all the files from a file.
char **file_getlines(FILE *f) {
    Str **lines = seq_new_ref(Str);
    obj_apply(lines,seq_add_ptr,f,file_getline);
    return (Str*)seq_array_ref(lines);
}

/// output of a command as lines.
// Will capture stderr as well.
char **file_command_lines(const char *cmd) {
    char buff[MAX_PATH];
    sprintf(buff,"%s 2>&1",cmd);
    FILE *out = popen(buff,"r");
    Str *lines = file_getlines(out);
    pclose(out);
    return lines;
}

/// all the files matching a mask.
char **file_files_in_dir(const char *mask, int abs) {
    char buff[MAX_PATH];
    if (abs) {
        sprintf(buff,"%s %s%s",DIR,PWD,mask);
    } else {
        sprintf(buff,"%s %s",DIR,mask);
    }
    Str *lines = file_command_lines(buff);
    if (array_len(lines) == 1 && strstr(lines[0],"No such file") != NULL) {
        obj_unref(lines);
        return NULL;
    } else {
        return lines;
    }
}

/// Operations on File paths.
// Unlike the POSIX functions basename and dirname,
// these functions are guaranteed not to touch the passed string,
// and will always return a refcounted string.
// @section paths

static const char *after_dirsep(const char *path) {
    char *p = strrchr(path,DIR_SEP);
    return p ? p+1 : path;
}

/// file part of a path.
// E.g. '/my/path/bonzo.dog' => 'bonzo.dog'
char *file_basename(const char *path) {
    return str_cpy(after_dirsep(path));
}

/// file part of a path.
// E.g. '/my/path/bonzo.dog' => '/my/path'
char *file_dirname(const char *path) {
    const char *p = after_dirsep(path);
    int sz = p ? (intptr)p - (intptr)path : 0;
    char *res = str_new_size(sz);
    strncpy(res,path,sz);
    return res;
}

/// extension of a path.
// Note: this will ignore any periods in the path itself.
char *file_extension(const char *path) {
    const char *p = after_dirsep(path);
    p = strchr(p,'.');
    if (p)
        return str_cpy(p);
    else
        return str_cpy("");
}

/// replace existing extension of path.
// `ext` may be the empty string, and `path` may not have
// a extension.
char *file_replace_extension(const char *path, const char *ext) {
    const char *p = after_dirsep(path);
    char *res;
    int sz, next = strlen(ext);
    p = strchr(p,'.');
    if (p) { // we already have an extension
        sz = (intptr)p - (intptr)path;
    } else {
        sz = strlen(path);
    }
    res = str_new_size(sz + next);
    strncpy(res,path,sz);
    strcpy(res+sz,ext);
    return res;
}

