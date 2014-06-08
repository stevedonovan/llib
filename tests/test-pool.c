#include <stdio.h>
#include <llib/str.h>
#include <llib/dbg.h>

void throw_away_loads_of_objects()
{
    scoped_pool;
    void **arr = array_new(void*,12);
    FOR(i,array_len(arr))
        arr[i] = str_fmt("and %d ...",i);
}

void be_irresponsible_with_strings()
{
    scoped_pool;
    printf("%s %s\n",str_new("hello"),str_new("dolly"));
}

void make_orphans_freely()
{
    scoped_pool; 
    double *p = array_new(double,10);
    be_irresponsible_with_strings();
    char **ss = strbuf_new();
    strbuf_adds(ss,"one");
    strbuf_adds(ss,"two");
    DS(strbuf_tostring(ss));
    throw_away_loads_of_objects();    
}

int main()
{    
    make_orphans_freely();
    DI(obj_kount());
    return 0;
}
