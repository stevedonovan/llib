#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "file.h"
#include "value.h"

typedef struct {
    FILE *f;
} FileW;

static void FileW_dispose(FileW *pf) {
    printf("dispose '%p'\n",pf->f);
    fclose(pf->f);
}

FILE **file_fopen(const char *file, const char *how) {
    FILE *f = fopen(file,how);
    if (! f) {
        return (FILE**)value_error(strerror(errno));
    }
    FileW *res = obj_new(FileW,FileW_dispose);
    res->f = f;
    return (FILE**)res;
}




