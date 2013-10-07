/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <llib/map.h>

typedef struct {
    LIST_HEADER;
    const char *name;  // this must be the key!
    int age;
    bool mf;
} Data;

void Data_dispose(Data *d)
{
    printf("data dispose '%s'\n",d->name);
    unref((char*)d->name);
}

Data *Data_new(const char *name, int age, bool mf)
{
    Data *obj = obj_new(Data,Data_dispose);
    obj->name = str_ref((char*)name);
    obj->age = age;
    obj->mf = mf;
    return obj;
}

void dump(Data* d) {
    if (d == NULL) { printf("result was NULL\n"); return; }
    printf("%s (%d) %s\n",d->name,d->age, d->mf ? "male" : "female");
}

void dumpi(void *k, void *v) {
    printf("%d:%d ",k,v);
}

#define P (void *)
#define D (Data*)
#define S (const char*)

void struct_maps()
{
    // here we have a custom type which has a LIST_HEADER and a string key
    // `true` indicates that we should use string ordering.
    Map *map = map_new_node(true);

    // since the key is part of the data we don't use `map_put`. `map_put_struct` is used
    // for inserting single values.
    map_put_structs(map,
        Data_new("bob",22,1),
        Data_new("alice",21,0),
        Data_new("roger",40,1),
        Data_new("liz",16,0),
        NULL
    );
    printf("size was %d\n",map_size(map));

    // you can now look up data using the key
    dump(D map_get(map,"alice"));
    dump(D map_get(map,"roger"));

    Data *rog = (Data*)map_remove(map,"roger");
    printf("size was %d\n",map_size(map));

    FOR_MAP(iter,map) {
        printf("[%s]=%d,",iter->key,((Data*)(iter->value))->age );
    }
    printf("\n");
    dispose(map,rog);
}

void count() {
    printf("kount %d\n",obj_kount());
}

// string maps own both the key and the value; the MapKeyValue
// struct can always be used to initialize maps and as a
// convenient array representation.
void string_maps()
{
    MapKeyValue mk[] = {
        {"alpha","A"},
        {"beta","B"},
        {"gamma","C"},
        {NULL,NULL}
    };
    Map *m = map_new_str_str();
    map_put_keyvalues(m,mk);

    MapKeyValue *pkv = map_to_array(m);

    for (MapKeyValue *p = pkv; p->key; ++p)
        printf("%s='%s' ",p->key,p->value);
    printf("\n");

    unref(m);
    unref(pkv);
}

void int_maps()
{
    // we put integers into a 'pointer' map
    Map *m = map_new_ptr_ptr();
    #define puti(i) map_puti(m,i,i)
    puti(10);
    puti(5);
    puti(7);
    puti(4);
    puti(2);
    puti(3);
    puti(8);
    puti(6);
    puti(20);
    puti(8);
    #undef puti

    map_remove(m,P 5);
    FOR_MAP(iter,m) {
        printf("[%d]=%d,",iter->key,iter->value);
    }
    printf("\n");
    unref(m);
}

int main () {
    struct_maps();
    int_maps();
    string_maps();
    printf("kount %d\n",obj_kount());
    return 0;
}

