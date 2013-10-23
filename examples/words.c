#include <stdio.h>
#include <stdlib.h>
#include <llib/scan.h>
#include <llib/map.h>

int main()
{
    char word[100];
    ScanState *ts = scan_new_from_file("../readme.md");
    if (ts == NULL) {
        printf("could not open this file\n");
        return 1;
    }

    Map *m = map_new_str_ptr();
    int k = 0;
    while (scan_next_iden(ts,word,sizeof(word))) {
        map_puti(m,word,  map_geti(m,word) + 1);
        ++k;
    }
    
    MapKeyValue *pkv = map_to_array(m);    
    int sz = array_len(pkv);
    printf("unique words %d  out of %d\n",sz,k);

    // sort by value, descending, and pick first 10 values
    array_sort_struct_ptr (pkv,true,MapKeyValue,value);    
    
    FOR(i,10) {
        printf("%s\t%d\n",(char*)pkv[i].key,(int)pkv[i].value);
    }

    dispose(m,ts,pkv);
    printf("remaining %d\n",obj_kount());
    return 0;
}
