#include <stdio.h>
#include <llib/config.h>
#include <llib/array.h>

void test (str_t file) {
    scoped_pool;
    char **S = config_read(file);
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
    test(argv[1] ? argv[1] : "config.cfg");
    printf("kount = %d\n",obj_kount());
    return 0;
}
