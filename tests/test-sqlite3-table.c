/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <sqlite3.h>
#include "table.h"
#include <llib/dbg.h>

typedef char *Str;

int main(int argc, char**argv)
{  
    int err;
    sqlite3 *conn;
    char *errmsg;
    
    err = sqlite3_open("test.sl3",&conn);

    Table *t = table_new(TableCsv | TableColumns);
    
    err = sqlite3_exec(conn,"select * from test",
        table_add_row,t,&errmsg);
    
    if (err) {
        printf("err '%s'\n",errmsg);
        return 1;
    }

    table_finish_rows(t);
    
    DUMPA(float,(float*)t->cols[1],"%f");
  
    sqlite3_close(conn);
    unref(t);
    
    DI(obj_kount());
    return 0;
}
