/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <llib/list.h>

typedef const char *Str;

// any struct we want to link in a list needs to start with LIST_HEADER

typedef struct {
    LIST_HEADER;
    Str name;
    int age;
} Person;

void Person_dispose (Person *p) {
    printf("disposing %s\n",p->name);
    obj_unref((void *)p->name);
}

Person *Person_new (Str name, int age) {
    Person *p = obj_new (Person,Person_dispose);
    p->name = str_new(name);
    p->age = age;
    return p;
}

#define list_get_cast(T,ls,idx) ((T*)list_get(ls,idx))
#define EQ(s1,s2) (strcmp((s1),(s2))==0)

void test_person_list() {
    List *pl = list_new_node (true);
    list_add (pl, Person_new ("bill",20));
    list_add (pl, Person_new ("jane",22));

    assert (EQ(list_get_cast(Person,pl,-2)->name,"bill"));

    Person *pbill = (Person*)list_find(pl,"bill");
    assert (pbill->age == 20);

    FOR_LIST_ITEM(Person,p,pl)
        printf("person %s %d\n",p->name,p->age);

    Person **ppl = (Person **)list_to_array(pl);
    printf("%s %d ?\n",ppl[0]->name,ppl[0]->age);
    obj_unref(ppl);
    obj_unref(pl);
}

void compare_int_list(List *ls, int *arr) {
    int n;
    FOR_LIST_T(int,n,ls)
        assert(n == *arr++);
}

void dump_int_list(List *ls) {
    int n;
    FOR_LIST_T(int,n,ls)
        printf("%d ",n);
    printf("\n");
}

void test_int_list() {
    List *li  = list_new_ptr();
    #define D (void *)
    list_add(li, D 10);
    list_add(li, D 20);
    list_add(li, D 30);
    // intended for pointers, works with non-zero integers as well
    // the casts to void * is needed for this to work on 64-bit
    // systems where the default int is smaller than a pointer.
    list_add_items(li,D 40,D 50,D 60);

    // indexing is slow in general, unless we're close to one of the ends
    assert ( (int)list_get(li,-1) == 60 );
    assert ( (int)list_get(li,1) == 20 );

    int o1[] = {10,20,30,40,50,60};
    compare_int_list(li,o1);

    list_remove_value(li,D 20);
    list_remove_value(li,D 22); // fails harmlessly

    List *l2 = list_new_ptr();
    list_add_items(l2,1,2,3);

    // moves all items over to new list
    list_extend_move(li,l2);
    assert(list_size(l2)==0);

    int o2[] = {10,30,40,50,60,1,2,3};
    compare_int_list(li,o2);

    // equivalent to li[1:-2] in Python
    List *sub = list_slice(li,list_iter(li,1),list_iter(li,-2));

    // would not be a list if we could not insert stuff arbitrarily!
    list_insert(sub,list_start(sub),D 555);

    // insert 44 before 40
    list_insert(sub,list_find(sub,D 40),D 44);

    int o3[] = {555,30,44,40,50,60,1,2};
    compare_int_list(sub,o3);

    List *s = list_new_ptr();
    #define add(v) list_add_sorted(s,D v)
    add(3);
    add(10);
    add(2);
    add(5);
    add(6);
    add(1);
    #undef add

    int o4[] = {1,2,3,5,6,10};
    compare_int_list(s,o4);

    // it's safe to remove items with this statement,
    // but you do need to know the magic 'p_' name of the iterator
    int n;
    FOR_LIST_T(int,n,s)
        if (n == 3 || n == 6)
            list_delete(s,p_);
    dump_int_list(s);

    dispose(li,l2,s,sub);
}

// big difference between this and the Person example is that
// this is a plain refcounted object which is _contained_ in the list.

int fook = 0;

typedef struct {
    Str name;
} Foo;

void Foo_dispose(Foo *p)
{
    --fook;
    unref(p->name);
}

Foo *Foo_new(Str name)
{
    Foo * f = obj_new(Foo,Foo_dispose);
    f->name = str_ref((char*)name);
    printf("ref %d\n",obj_refcount((void*)f->name));

    ++fook;
    return f;
}

void test_ref_list()
{
    List *ff = list_new_ref();
    Foo *f1 = Foo_new("one");
    Foo *f2 = Foo_new("two");
    list_add_items(ff,f1,f2);
    unref(ff);
    //printf("fook %d!\n",fook);
    assert(fook == 0);
}

// list wrappers allow floating-point numbers to be safely put in lists.
// (Otherwise, conversions to void* mess them up)
void test_wrapper()
{
    float ** pw = (float**)listw_new();
    listw_add(pw, 10);
    listw_add(pw, 20);
    listw_add(pw, 30);
    listw_add(pw, 40);
    listw_insert(pw, listw_first(pw), 5);
    printf("first %f\n",listw_get(pw,0));
    printf("last %f\n",listw_get(pw,-1));
    FOR_LISTW(p, pw)
        printf("%f\n",**pw);
    unref(pw);
}


int main() {

    printf("int lists\n");
    test_int_list();

    printf("Person lists\n");
    test_person_list();

    printf("lists of references\n");
    test_ref_list();

    printf("wrapper lists\n");
    test_wrapper();
    printf("kount %d\n",obj_kount());
}
