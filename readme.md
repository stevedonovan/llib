## A Compact C Utilities Library

Since we are now in the 21st century, this uses C99, but is compatible with
MSVC when compiled as C++.  This leads to some 'unnecessary' casting to keep
C++ happy, but working in the intersection between C99 and MSVC C++ makes this
library very portable.

It has been tested on Windows 7 64-bit (mingw and MSVC), and both Linux 32-bit and
64-bit.

There are a number of kitchen-sink extended C libraries like glibc and the
Apache Portable Runtime but these are big and awkward to use on all platforms.
llib aims to be small as possible and intended for _static_ linking with your
application (the BSD licensing should help with this).

llib provides a refcounting mechanism, extended string functions,dynamically-resizable arrays,
doubly-linked lists and maps using binary trees. The aim of this first release is
not to produce the most efficient code possible but to explore the API and validate
the refcounting semantics.

A note on style: although C has been heavily influenced by C++ over the years,
it remains C. So judicious use of statement macros like `FOR` is fine, if they
are suitably hygenic:

```C
#define FOR(i,n) for (int i = 0, n_ = (n); i < n_; i++)
```

## Reference Counting

llib provides refcounted arrays, which contain their own size. You do not ever use
`free` on objects created by llib, and instead use `obj_unref`.

```C
#include <stdio.h>
#include <llib/obj.h>

int main()
{
    int *ai = array_new(int,10);
    for (int i = 0, n = array_len(ai); i < n; ++i)
        ai[i] = 10.0*i;
    ...
    obj_unref(ai);
    return 0;
}
```
C does not have smart pointers, but dumb pointers with hidden secrets. All objects
are over-allocated, with a hidden header behind the pointer. If the object is an
array, it keeps track of its size, otherwise contains a pointer to a function
which acts as a 'destructor' for the object. Objects start out with a reference
count of 1, and the macro `obj_ref` increments this count; if un-referenced and the
count is zero, then we call the destructor and free the memory.


```C
typedef struct ObjHeader_ {
    union {
      DisposeFn dtor;
      uint32 len;
    } x;
    unsigned int _ref:15;
    unsigned int is_array:1;
    unsigned int is_ref_container:1;
    unsigned int mlen:15;
} ObjHeader;
```

This allows for objects to safely _share_ other objects without having to fully
take ownership.  Reference counting cuts down on the amount of 'defensive copying'
needed in typical code.

Unless you specifically define `OBJ_REF_ABBREV`, the basic operations are named
`ref` and `unref`; there is also an alias `dispose` for `obj_unref_v` which
un-references multiple objects.

It is straightforward to create new objects which fit with llib's object scheme:

```C
typedef struct {
    int *ages;
} Bonzo;

static void Bonzo_dispose(Bonzo *self) {
    printf("disposing Bonzo\n");
    unref(self->ages);
}

Bonzo *Bonzo_new(int *ages) {
    Bonzo *self = obj_new(Bonzo,Bonzo_dispose);
    self->ages = ref(ages);
    return self;
}

void test_bonzo() {
    int AGES[] = {45,42,30,16,17};
    int *ages = array_new(int,5);

    FOR(i,5) ages[i] = AGES[i];  //??

    Bonzo *b = Bonzo_new(ages);

    unref(ages);

    printf("%d %d\n",b->ages[0],b->ages[1]);

    unref(b);
}
```

There is nothing special about such structures; when creating them with the macro
`obj_new` you can provide a dispose function. In this case, `Bonzo` objects
reference the array of ages, and they un-reference this array when they are finally
disposed.  The test function releases the array `ages`, and thereafter the only
reference to `ages` is kept within the struct.

This scheme will only add a few Kb code to an application. ??

Arrays can be _reference containers_ which hold refcounted objects:

```C
    Bonzo *dogs = array_new_ref(Bonzo,3);
    // see? They all happily share the same ages array!
    dogs[0] = Bonzo_new(ages);
    dogs[1] = Bonzo_new(ages);
    dogs[2] = Bonzo_new(ages);
    ...
    // this un-references the Bonzo objects
    // only then the array will die
    unref(dogs);
```

## Sequences

llib provides resizable arrays, called 'seqs'.

```C
float **s = seq_new(float);
seq_add(s,1.2);
seq_add(s,5.2);
....
float *arr = ref(*s);
unref(s);

// array still lives!
FOR(i,array_len(arr)) printf("%f ",arr[i]);
printf("\n");

// alternative macro
// FOR(float,pf,arr) printf("%f ",*pf);

```

You treat seqs like a pointer to an array, and use `seq_add` to ensure that the
array is resized when needed.  `*s` is _always_ a valid llib array, and in particular
`array_len` returns the correct size.

A seq keeps a reference to this array, and to get a reference to the array you
can just say `ref(*s)` and then it's fine to dispose of the seq itself. The function
`seq_array_ref` combines these two operations of sharing the array and disposing
the seq, plus resizing to fit.

These can also explicitly be _reference containers_ which derereference their objects
afterwards if you use `seq_new_ref`.

## Linked Lists

A doubly-linked list is a very useful data structure which offers fast insertion at
arbitrary posiitions.  It is sufficiently simple that it is continuously
reinvented, which is one of the endemic C diseases.

```C
    List *l = list_new_str();
    list_add (l,"two");
    list_add (l,"three");
    list_insert_front(l,"one");
    printf("size %d 2nd is '%s'\n",list_size(l),list_get(l,1));
    FOR_LIST(pli, l)
        printf("'%s' ",pli->data);
    printf("\n");
    unref(l);
    printf("remaining %d\n",obj_kount());
    return 0;
//    size 3 2nd is 'two'
//    'one' 'two' 'three'
//    remaining 0
```

A list of strings is a ref container, but with the added thing that if we try to add
a string which is not _one of ours_ then a proper refcounted copy is made. So it is
safe to add strings from any source, such as a local buffer on the heap. These are all
properly disposed of with the list.

Generally, containers in C are annoying, because of the need for typecasts. (Already
with `-Wall` GCC is giving us some warnings about those `printf` flags.)  Integer
types can fit into the pointer payload fine enough, but it isn't possible to
directly insert floating-point numbers.  List wrappers do a certain amount of pointer
aliasing magic for us:

```C
    float ** pw = (float**)listw_new();
    listw_add(pw, 10);
    listw_add(pw, 20);
    listw_add(pw, 30);
    listw_add(pw, 40);
    listw_insert(pw, listw_first(pw), 5);
    printf("first %f\n",listw_get(pw,0));
    FOR_LISTW(p, pw)
        printf("%f\n",**pw);

```

They are declared as if they were seqs (pointers to arrays) and there's then
a way to iterate over typed values.

## Maps

Maps may have two kinds of keys; integer/pointer, and strings. Like string lists,
the latter own their key strings and they will be safely disposed of later. They
may contain integer/pointer values, or string values. The difference again is
with the special semantics needed to own arbitrary strings.

Typecasting is again irritating, so there are macros `map_puti` etc for the
common integer-valued case:

```C
    Map *m = map_new_str_ptr();
    map_puti(m,"hello",23);
    map_puti(m,"alice",10);
    map_puti(m,"frodo",2353);

    printf("lookup %d\n",map_geti(m,"alice"));

    map_remove(m,"alice");
    FOR_MAP(mi,m)
        printf("key %s value %d\n",mi->key,mi->value);
    unref(m);
```

The implementation in llib is a binary tree - not in general the best, but it works
reliably and has defined iteration order.

Maps can be initialized from arrays of `Mapkeyvalue` structs. Afterwards, such an
array can be generated using `map_to_array`:

```C
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
```

## Strings

Strings are the usual nul-terminated char arrays, but llib refcounted strings are arrays and so
already know their size through `array_len` (which will be faster than `strlen` for long strings.)
It's generally hard to know portably if an arbitrary pointer is heap-allocated, so there's
`str_ref` which does the 'one of mine?' check and makes a refcounted copy if not, otherwise acts
just like `obj_ref`.

`str_fmt` is a convenient and safe way to do `sprintf`, and will return a refcounted string.

String support in C is famously minimalistic, so llib adds some extensions. `str_split` uses a
delimiter to split a string into an array using delimiter chars. The array is a ref container so the
strings will be disposed with it:

```
    Str* words = str_split("alpha, beta, gamma",", ");
    assert(array_len(words) == 3);
    assert(str_eq(words[0],"alpha"));
    assert(str_eq(words[1],"beta"));
    assert(str_eq(words[2],"gamma"));
    unref(words);
```

Building up strings is a common need, and llib provides two ways to do it.  If you already have
an array of strings then feed it to `str_concat` with a delimiter - it is the inverse operation
to `str_split`. If the string is built up in an ad-hoc fashion then use the `strbuf_*` functions.
A `string buffer` is basically a sequence, so that `strbuf_add` is exactly the same as `seq_add`
for character arrays.  `strbuf_adds` adds a string, and `strbuf_addf` is equivalent to
`strbuf_adds(ss,str_fmt(fmt,...))`.

They are used for operations which modify strings, like inserting, removing and replacing, and
resemble the [similar methods](?) of C++'s `std::string`.

Then there are operations on strings which don't modify them:

```
    const char *S = "hello dolly";
    int p = str_findstr(S,"doll");
    assert(p == 6);
    p = str_findch(S,'d');
    assert(p == 6);
    p = str_find_first_of("hello dolly"," ");
    assert(p == 5);
    assert(str_starts_with(S,"hell"));
    assert(str_ends_with(S,"dolly"));
```

They're simple wrappers over the old `strchr`, `strstr` etc functions that return offsets
rather than pointers. This is a more appropriate style for refcounted strings where you
want to only use the allocated 'root' pointer.


