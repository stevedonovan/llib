/* Read all the words in a document and keep a
 * map of the unique words and the number of their occurances.
 *
 * We then get the map out as an array of MapKeyValue structs,
 * and sort this by value so we get the most common words first.
 * Then just dump out the top 10.
*/
#include <stdio.h>
#include <stdlib.h>
#include <llib/map.h>

int main(int argc, char **argv)
{
    char word[100];
    const char *file = argv[1] ? argv[1] : "../readme.md";
    FILE *in = fopen(file,"r");

    Map *m = map_new_str_ptr();
    int k = 0;
    while (fscanf(in,"%99s",word) == 1) {
        map_puti(m,word,  map_geti(m,word) + 1);
        ++k;
    }
    fclose(in);

    MapKeyValue *pkv = map_to_array(m);
    int sz = array_len(pkv);
    printf("unique words %d  out of %d\n",sz,k);

    // sort by value, descending, and pick first 10 values
    array_sort_struct_ptr (pkv,true,MapKeyValue,value);

    FOR(i,10) {
        printf("%s\t%d\n",(char*)pkv[i].key,(intptr)pkv[i].value);
    }

    dispose(m,pkv);
    printf("remaining %d\n",obj_kount());
    return 0;
}
