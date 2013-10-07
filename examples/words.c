#include <stdio.h>
#include <llib/scan.h>
#include <llib/map.h>

int main()
{
    char word[100];
    Map *m = map_new_str_ptr();
    ScanState *ts = scan_new_from_file("test1.c");
    while (scan_next_iden(ts,word,sizeof(word))) {
        map_puti(m,word,map_geti(m,word)+1);
    }
    MapKeyValue *pkv = map_to_array(m), *p;
    
    printf("size %d\n",array_len(pkv));
    for (p = pkv; p->key; ++p)
        printf("%s=%d ",p->key,p->value);
    printf("\n");

    dispose(m,ts,pkv);
    printf("remaining %d\n",obj_kount());
    return 0;
}