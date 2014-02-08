#ifndef _LLIB_TABLE_H
#define _LLIB_TABLE_H
#include <stdio.h>
#include "obj.h"

typedef const char *(*TableConvFun)(const char *str, void *res);

typedef struct Table_ {
    char **col_names;
    int ncols, nrows;
    char ***rows;
    void ***cols;
    const char *error;
    const char *delim;
    TableConvFun *row_conv;
    FILE *in;
    int opts;
} Table;

enum {
    TableTab = 0,
    TableComma = 1,
    TableColumnNames = 2,
    TableCsv = TableComma + TableColumnNames,
    TableColumns = 4,
    TableAll = 8,
    TableString = 0,
    TableInt = 1,
    TableFloat = 2,
    TableCustom = 3
};

Table *table_new(int opts);
int table_add_row(void *d, int ncols, char **row, char **columns);
bool table_finish_rows(Table *T);

void table_convert_cols (Table *T,...);
bool table_generate_columns (Table *T);

Table* table_new_from_stream(FILE *in, int opts);
Table* table_new_from_file(const char *fname, int opts);
#endif
