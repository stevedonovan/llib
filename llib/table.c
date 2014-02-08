#include <stdlib.h>
#include <stdarg.h>
#include "str.h"
#include "file.h"
#include "table.h"

static void Table_dispose(Table *t) {
    obj_unref_v(t->col_names, t->rows, t->cols, t->error);
}

typedef char *Str;
typedef char **Strings;

Table *table_new(TableOptions opts) {
    Table *T = obj_new(Table,Table_dispose);
    memset(T,0,sizeof(Table));
    T->opts = opts;
    T->delim = (opts & TableComma) ? "," : "\t";
    return T;
}

static char *endptr;

const char *float_convert(const char *str, void *res) {
    *((float*)res) = strtod(str,&endptr);
    return *endptr ? endptr : NULL;
}

const char *int_convert(const char *str, void *res) {
    *((int*)res) = strtol(str,&endptr,10);
    return *endptr ? endptr : NULL;
}

const char *no_convert(const char *str, void *res) {
    *((const char**)res)  = str;
    return NULL;
}

void table_convert_cols (Table *T,...) {
    int ncols = T->ncols;
    TableConvFun *conversions = array_new(TableConvFun,ncols);
    TableConvFun conv = NULL;
    va_list ap;
    memset(conversions,0,sizeof(TableConvFun)*ncols);
    va_start(ap,T);
    while (true) {
        int idx = va_arg(ap,int);
        if (idx == -1)
            break;
        int how = va_arg(ap,int);
        switch(how) {
        case TableString:  conv = no_convert; break;
        case TableFloat:  conv = float_convert; break;
        case TableInt: conv = int_convert; break;
        case TableCustom: conv = va_arg(ap,TableConvFun); break;
        }
        conversions[idx] = conv;
    }
    T->row_conv = conversions;
    T->opts |= TableColumns;
    va_end(ap);
}

bool table_generate_columns (Table *T) {
    // generate columns if requested!
    if (T->opts & TableColumns) {
        const char *err;
        int ncols = T->ncols, nrows = T->nrows;
        // (this array is guaranteed to be NULLed out since it's a reference array)
        void ***cols = array_new_ref(void**,ncols);
        // a temporary horrible hack: an alias for columns of 32-bit values.
        int **icols = (int**)cols;
        TableConvFun *conv = T->row_conv;
        T->cols = cols;
        FOR(ir,nrows) {
            Strings row = T->rows[ir];
            if (ir == 0) {
                if (! conv) { // auto-conversion case: any float columns?
                    float tmp;
                    conv = T->row_conv = array_new(TableConvFun,ncols);
                    FOR(ic,ncols) {
                        err = float_convert(row[ic], &tmp);
                        if (err) {
                            conv[ic] = no_convert;
                            cols[ic] = (void**)array_new(char*,nrows);
                        } else {
                            conv[ic] = float_convert;
                            cols[ic] = (void**)array_new(float,nrows);
                        }
                    }
                } else { // we've been given the types of particular columns to be extracted
                    FOR(ic,ncols) {
                        if (! conv[ic]) continue;
                        if (conv[ic] == no_convert)
                            cols[ic] = (void**)array_new(char*,nrows);
                        else 
                        if (conv[ic] == int_convert)
                            cols[ic] = (void**)array_new(int,nrows);
                        else
                            cols[ic] = (void**)array_new(float,nrows);
                    }
                }
            }
            FOR(ic,ncols) {
                if (conv[ic]) {
                    if (conv[ic] == no_convert) 
                        err = conv[ic](row[ic], &cols[ic][ir]);
                    else
                        err = conv[ic](row[ic], &icols[ic][ir]);
                    if (err) {
                        T->error = str_fmt("conversion error: '%s' at row %d col %d",err,ir+1,ic+1);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool table_read_all(Table *T) {
    int ncols = T->ncols;
    // read all the lines
    Strings lines = file_getlines(T->in);
    fclose(T->in);
    T->in = NULL;

    int nrows = array_len(lines);
    while (strlen(lines[nrows-1]) == 0) {
        --nrows;
    }
    T->nrows = nrows;

    // and split these lines into the rows
    T->rows = array_new_ref(Strings,nrows);
    FOR (i,nrows) {
        Strings row  = str_split(lines[i],T->delim);
        int ncol = array_len(row);
        if (! ncols) {
            ncols = ncol;
            T->ncols = ncols;
        }
        if (array_len(row) != ncols) {
            T->error = str_fmt("row %d has %d items instead of %d",i+1,array_len(row),ncols);
            return false;
        }
        T->rows[i] = row;
    }
    obj_unref(lines);

    return table_generate_columns(T);
}

void table_read_col_names(Table *T) {
    if (T->opts & TableColumnNames) {
        T->col_names = str_split(file_getline(T->in),T->delim);
    }
    if (T->opts & TableAll) {
        table_read_all(T);
    }
    //return T;
}

static Strings strings_copy(Strings s, int n) {
    Strings res = array_new_ref(Str,n);
    FOR(i,n) {
        res[i] = str_new(s[i]);
    }
    return res;
}

int table_add_row(void *d, int ncols, char **row, char **columns) {
    Table *T = (Table*)d;
    if (T->nrows == 0) {
        T->col_names = strings_copy(columns,ncols);
        T->ncols = ncols;
        T->rows = (Strings*)seq_new(Strings);
    }
    ++T->nrows;
    row = strings_copy(row,ncols);
    seq_add((Strings**)T->rows,row);
    return 0;
}

bool table_finish_rows(Table *T) {
    Strings* rows = (Strings*)seq_array_ref(T->rows);
    T->rows = rows;
    return table_generate_columns(T);
}

Table* table_new_from_stream(FILE *in, TableOptions opts) {
    Table *T = table_new(opts);
    T->in = in;
    table_read_col_names(T);
    return T;
}

Table* table_new_from_file(const char *fname, TableOptions opts) {
    Table *T = table_new(opts);
    FILE *in = fopen(fname,"r");
    if (! in) {
        T->error = str_fmt("cannot open '%s'",fname);
        return T;
    }
    T->in = in;
    table_read_col_names(T);
    return T;
}

