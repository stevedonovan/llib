/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
## Reading CSV Files.

This reads tab- or comma-delimited data, optionally treating the first row
as a set of headers (`TableCsv` implies both commas and header row).
The rows are then available as an array of string arrays.

If `TableColumns` is also specified, it will additionally create columns,
which will be converted into `float` arrays if they appear to be numbers.

Given a simple CSV file like this:

    Name,Age
    Bonzo,12
    Alice,16
    Frodo,46
    Bilbo,144
    
then the most straightforward way to read it would be:

    Table *t = `table_new_from_file`("test.csv", TableCsv | TableAll | TableColumns);
    if (t->error) { // note how you handle errors: file not found, conversion failed
        fprintf(stderr,"%s\n",t->error);
        return 1;
    }
    // printing out first row together with column names
    char **R = t->rows[0];
    for (char **P = t->col_names; *P; ++P,++R)
        printf("'%s' (%s),",*P,*R);
    printf("\n");
    
    // getting the second column with default conversion (float)
    float *ages = (float*)t->cols[1];
    
With `table_convert_cols` you can customize column conversion, and even
provide your own conversion functions.  These look like this:

    const char *int_convert(const char *str, void *res) {
        \*((int*)res) = strtol(str,&endptr,10);
        return *endptr ? endptr : NULL;
    }

That is, they set the value and return an _error_ if conversion is impossible.

See `test-table.c` for an example with custom conversions, and
`test-sqlite3-table.c` for a case where the table is built up
using `table_add_row` (which by design matches the required signature
for `sqlite3_exec`.)
*/

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

/// create a new empty table.
// `opts` are a set of flags:  `TableTab`, `TableComma`, `TableColumnNames`,
// `TableCsv` (implying last two), `TableColumns` (create columns as well).
Table *table_new(int opts) {
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

/// custom conversion for table columns.
// Normally, `table` attempts to convert numerical columns
// to arrays of `float`. With this you can force the conversion
// to `int` arrays or leave them as strings, etc
//
// Each entry is a zero-based column index, and a type, one of
// `TableString`, `TableFloat`, `TableInt` and `TableCustom`. 
// If it's a custom conversion, then follow `TableCustom` with
// a `TableConvFun`.
// The argument list ends with -1, which is an invalid column index.
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

/// Explicitly create columns.
// Only does this if the flags have `TableColumns` set.
// This is implicitly called by `table_read_all` and `table_finish_rows`.
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

/// Read all of a table into rows.
// If flag has `TableColumns` set, create columns as well.
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

/// explicitly add new rows to a table.
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

/// explicitly finish off a table created with `table_add_row`.
bool table_finish_rows(Table *T) {
    Strings* rows = (Strings*)seq_array_ref(T->rows);
    T->rows = rows;
    return table_generate_columns(T);
}

/// Create a table from an opened file stream.
// `opts` have same meaning as for `table_new`.
// If unsuccesful, the table's `error` field will be non-NULL.
Table* table_new_from_stream(FILE *in, int opts) {
    Table *T = table_new(opts);
    T->in = in;
    table_read_col_names(T);
    return T;
}

/// Create a table from a file path.
// `opts` have same meaning as for `table_new`.
// If unsuccesful, the table's `error` field will be non-NULL.
Table* table_new_from_file(const char *fname, int opts) {
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

