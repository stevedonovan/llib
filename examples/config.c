#include <stdlib.h>
#include <llib/str.h>
#include <llib/file.h>
#include <llib/array.h>

str_t config_gets(SMap m, str_t key, str_t def) {
    str_t res = str_lookup(m,key);
    if (! res)
        res = def;
    return str_new(res);
}

int config_geti(SMap m, str_t key, int def) {
    str_t s = str_lookup(m,key);
    if (! s) {
        return def;
    } else {
        return atoi(s);
    }
}

double config_getf(SMap m, str_t key, double def) {
    str_t s = str_lookup(m,key);
    if (! s) {
        return def;
    } else {
        return atof(s);
    }
}

char** config_gets_arr(SMap m, str_t key) {
    str_t s = str_lookup(m,key);
    if (! s) {
        return array_new(char*,0);
    } else {
        return str_split(s,",");
    }    
}

int* config_geti_arr(SMap m, str_t key) {
    char** parts = config_gets_arr(m,key);
    MAPA(int,res,atoi(_),parts);
    unref(parts);
    return res;
}

double* config_getf_arr(SMap m, str_t key) {
    char** parts = config_gets_arr(m,key);
    MAPA(double,res,atof(_),parts);
    unref(parts);
    return res;
}

char** config_read(str_t file) {
    FILE *in = fopen(file,"r");
    char*** ss = smap_new(true);
    char line[256];
    while (file_gets(in,line,sizeof(line))) {
        char *p = strchr(line,'#');       
        if (p) {
            *p = '\0';
        }
        if (str_is_blank(line))
            continue;        
        char **parts = str_split(line,"=");
        str_trim(parts[1]);
        smap_add(ss,str_ref(parts[0]),str_ref(parts[1]));
        unref(parts);
    }
    fclose(in);
    return smap_close(ss);    
}

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
    test(argv[1]);
    printf("kount = %d\n",obj_kount());
    return 0;
}
