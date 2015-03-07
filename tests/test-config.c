#include <stdio.h>
#include <llib/config.h>
#include <llib/str.h>
#include <llib/array.h>

void test (str_t file, int flags) {
    scoped_pool;
    char **S = config_read_opt(file,flags);
    FOR_SMAP(key,val,S) {
        printf("%s:'%s'\n",key,val);
    }    
    
    printf("name %s\n",config_gets(S,"name","?"));
    printf("a %d\n",config_geti(S,"a",-1));
    
    int* ids = config_geti_arr(S,"ids");
    FORA(ids,printf("%d ",_));
    printf("\n");    
}


int main(int argc,  const char **argv)
{
    const char *file = argv[1] ? argv[1] : "config.cfg";
    test(file,str_findstr(file,"space") == -1 ? CONFIG_DELIM_EQUALS : CONFIG_DELIM_SPACE);
    printf("kount = %d\n",obj_kount());
    return 0;
}
