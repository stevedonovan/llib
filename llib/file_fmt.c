/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/// Vararg versions of file commands
// @submodule file

#include <stdarg.h>
#define _BSD_SOURCE
#include "file.h"
#include "str.h"

static FILE *file_popen_vfmt(const char *fmt, const char *how, va_list ap) {
    char *cmd = str_vfmt(fmt,ap);
    printf("got '%s'\n",cmd);
    FILE *res = popen(cmd, how);
    obj_unref(cmd);
    return res;
}

/// `popen` using varargs.
FILE *file_popen_fmt(const char *fmt, const char *how, ...) {
    va_list ap;
    va_start(ap,how);
    FILE *res = file_popen_vfmt(fmt, how,ap);
    va_end(ap);
    return res;
}

/// output of a command as text, varags version.
// Will return "" if the command does not return anything
// Only first line! Use file_command_lines for the rest!
char *file_command_fmt(const char *fmt,...) {
    va_list ap;
    va_start(ap,fmt);
    FILE *out = file_popen_vfmt(fmt,"r",ap);
    va_end(ap);
    char *text = file_getline(out);    
    pclose(out);
    return text ? text : str_new("");
}

