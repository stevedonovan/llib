/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef __FILE_H
#define __FILE_H
#include <stdio.h>
#include "obj.h"

bool file_exists(const char *path, const char *rw);
const char *file_exists_any(const char *rw, char **files);
char *file_gets(FILE *f, char *buff, int bufsize);
char *file_getline(FILE *f);
char *file_read_all(const char *file, bool text);
FILE **file_fopen(const char *file, const char *how);
int file_size(const char *file);
char **file_getlines(FILE *f);
char *file_command(const char *cmd);
char **file_command_lines(const char *cmd);
char **file_files_in_dir(const char *dirname, int abs);

char *file_basename(const char *path);
char *file_dirname(const char *path);
char *file_extension(const char *path);
char *file_replace_extension(const char *path, const char *ext);
char *file_expand_user(const char *path);
#endif
