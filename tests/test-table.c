/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <llib/table.h>

typedef char *Str;

int main(int argc, char**argv)
{
    Str file = argv[1] ? argv[1] : "test.csv";
    Table *t = table_new_from_file(file, TableCsv | TableAll);
    if (t->error) {
        fprintf(stderr,"%s\n",t->error);
        return 1;
    }
    table_convert_cols(t,0,TableString,1,TableInt,-1);
    table_generate_columns(t);
    
    Str *R = t->rows[0];
    for (Str *P = t->col_names; *P; ++P,++R)
        printf("'%s' (%s),",*P,*R);
    printf("\n");
    
    int *ages = (int*)t->cols[1];
    char **names = (char**)t->cols[0];
    FOR(i,array_len(ages))   printf("%d ",ages[i]);
    printf("\n");    
    FOR(i,array_len(names))   printf("%s ",names[i]);
    printf("\n");
    unref(t);
    printf("kount = %d\n", obj_kount());
    return 0;
}
