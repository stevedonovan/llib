/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
Doubly-linked lists.

 A list is either a container (has pointer or integer values) stored inside
`ListEntry` nodes as the `data` field, or the objects themselves are nodes (declared with
LIST_HEADER up front in the struct).

The nodes of a container list are not ref-counted and are simply freed.
The contained values may be ref-counted and will then be unref'd when the
container is disposed.

Container lists are either pointer-like or string-like; if LIST_STRING flag bit
is set then we'll use `strcmp` for ordering and finding, otherwise simple ordering
is used, which is also fine for integer values (up to intptr_t or uintptr_t in
 size)

@module list
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "private_list.h"
#include "list.h"

void list_free_item(List *ls, ListIter item) {
    if (list_is_container(ls)) {
        if (ls->flags & LIST_REF) {
            obj_unref (item->data);
        }
        free(item);
    } else {
        obj_unref(item);
    }
}

ListIter list_new_item(List *ls, int sz) {
    return (ListIter)malloc(sz);
}

static void List_dispose(List *self) {
    list_erase(self,self->first,NULL);
}

static int simple_pointer_compare(void *p1, void *p2) {
    return (intptr)p1 - (intptr)p2;
}

static int simple_pointer_equals(void *p1, void *p2) {
    return p1 == p2;
}

static int string_equals(const char *s1, const char *s2) {
    return strcmp(s1,s2) == 0;
}

List *list_new (int flags) {
    List *self = obj_new(List,List_dispose);
    self->flags = flags;
    if (flags & LIST_STR) {
        self->compare = LIST_CMP strcmp;
        self->equals = LIST_CMP string_equals;
    } else {
        self->compare = LIST_CMP simple_pointer_compare;
        self->equals = LIST_CMP simple_pointer_equals;
    }
    self->first = NULL;
    self->last = NULL;
    self->size = 0;
    return self;
}

/// simple list containing pointers or integers.
List *list_new_ptr() { return list_new (LIST_PTR); }

/// list containing refcounted strings.
List *list_new_str() { return list_new (LIST_STRING); }

/// list containing any other refcounted objects.
List *list_new_ref() { return list_new (LIST_REF); }

/// list made of suitable structs with `LIST_HEADER`.
List *list_new_node(bool str) {
    return list_new (LIST_NODE | (str ? LIST_STR : 0));
}

List *private_clone_list(List *other) {
    List *ls = list_new(other->flags);
    // these may have been set to custom functions
    ls->compare = other->compare;
    ls->equals = other->equals;
    return ls;
}

ListIter private_new_item(List *ls, void *data, int size) {
    if (list_is_node(ls)) {  // the data _is_ the node!
        return (ListIter)data;
    }
    ListIter node = list_new_item(ls,size);
    if (ls->flags & LIST_STRING) {
        data = str_cpy ((char*)data);
    }
    node->data = data;
    return node;
}

/// size of list.
int list_size(List *ls) {
    return ls->size;
}

/// data of list item.
void * list_item_data (List *ls, void *item) {
    if (list_is_node(ls))
        return item;
    else
        return ((ListIter)item)->data;
}

/// first list item.
ListIter list_start(List *ls) { return ls->first; }

/// last list item.
ListIter list_end(List *ls) { return ls->last; }

/// advance to next item.
ListIter list_iter_next(ListIter le) { return le->_next; }

/// move to previous item.
ListIter list_iter_prev(ListIter le) { return le->_prev; }

/// add data to the end of the list.
ListIter list_add (List *ls, void *data) {
    ListIter node = private_new_item(ls,data,sizeof(ListEntry));
    ListIter last = ls->last;
    ++ls->size;
    node->_next = NULL;
    node->_prev = last;
    if (last) last->_next = node;
    if (! ls->first)  ls->first = node;
    ls->last = node;
    return node;
}

/// append a number of data items to a list.
// Assumes that the last argument is NULL, so not so
// useful for lists of arbitrary integers.
// @tparam List* the list
// @param ... values (must be pointer sized!)
// @function list_add_items

/// inserts data _before_ `before`
ListIter list_insert(List *ls, ListIter before, void *data) {
    if (ls->size == 0) return list_add(ls,data);
    ListIter ndata = private_new_item(ls,data,sizeof(ListEntry));
    ListIter prev = before->_prev;
    ++ls->size;
    before->_prev = ndata;
    ndata->_next = before;
    ndata->_prev = prev;
    if (prev) prev->_next = ndata;
    else ls->first = ndata;
    return ndata;
}

/// add data in sorted order.
ListIter list_add_sorted(List *ls, void *data) {
    FOR_LIST(item,ls) {
        if (ls->compare(item->data,data) > 0) {
            return list_insert(ls, item, data);
        }
    }
    return list_add(ls,data);
}

/// removes item from list.
ListIter list_remove(List *ls, ListIter item) {
    if (item == NULL) return NULL;
    ListIter prev = item->_prev, next = item->_next;
    if (prev) {
        prev->_next = next;
    } else {
        ls->first = next;
    }
    if (next) {
        next->_prev = prev;
    } else {
        ls->last = prev;
    }
    -- ls->size;
    return item;
}

/// remove last item.
ListIter list_remove_last(List *ls) {
    return list_remove(ls,ls->last);
}

/// remove the last item and return its value.
// For container lists, this frees the item.
void * list_pop(List *ls) {
    ListIter le = list_remove_last(ls);
    if (le) {
        void *data = list_item_data(ls,le);
        if (list_is_container(ls))
            list_free_item(ls,le);
        return data;
    } else {
        return NULL;
    }
}

/*
 The list equals operation is distinct from the list compare operation
 because if the nodes are the objects, then we typically want equality to
 mean matching on some field of the object.
*/

/// return an iterator to a given data value.
// (You must define the comparison operation @{list_item_equals} to use this
// if the nodes are your custom data.)
// Use @{list_item_data} generally to extract the data, unless you know that the
// list entries are themselves your data.
ListIter list_find(List *ls, void *data) {
    FOR_LIST(item,ls) {
        if (ls->equals(item->data,data)) {
            return item;
        }
    }
    return NULL;
}

ListIter list_find_if (List *ls, ListSearchFn fn, void *data) {
    FOR_LIST(item,ls) {
        if (fn(item,data)) {
            return item;
        }
    }
    return NULL;
}

/// get iter at index.
// Negative indices count from the back, so -1 is the last entry, etc.
// Not particularly efficient for arbitrary indexing since this is O(N).
ListIter list_iter(List *ls, int idx) {
    int i = 0;
    if (idx < 0) {
        idx =  -1 - idx;
        FOR_LIST_REV(ListEntry,item,ls) {
            if (i++ == idx) return item;
        }
    } else {
        FOR_LIST(item,ls) {
            if (i++ == idx) return item;
        }
    }
    return NULL;
}

/// get data at index.
// This is only different from @{list_iter} when it's a container list.
void *list_get(List *ls, int idx) {
    ListIter le = list_iter(ls,idx);
    return le ? list_item_data(ls,le) : NULL;
}

/// make a new list containing list data that match a predicate.
List *list_filter(List *other, ListPred fun) {
    List *ls = private_clone_list(other);
    bool isref = ls->flags & LIST_REF;
    FOR_LIST(item,other) {
        if (! fun || (fun && fun(item->data))) {
            if (isref)
                item->data = obj_ref(item->data);
            list_add(ls,item->data);
        }
    }
    return ls;
}

/// make a new list of the list items that match a predicate.
List *list_filter_if(List *other, ListSearchFn fun, void *data) {
    List *ls = private_clone_list(other);
    FOR_LIST(item,other) {
        if (fun(item,data))
            list_add(ls,obj_ref(data));
    }
    return ls;
}

/// make a copy of another list.
List* list_copy(List *other) {
    return list_filter(other,NULL);
}

/// extend a list by appending another list's contents, emptying it.
ListIter list_extend_move(List *ls, List *other) {
    if (ls->flags != other->flags) return NULL;  // other list has wrong type!
    ListIter last1 = ls->last, first2 = other->first;
    ls->last = other->last;
    last1->_next = first2;
    first2->_prev = last1;
    ls->size += other->size;
    other->size = 0;
    other->first = other->last = NULL;
    return ls->last;
}

/// extend a list by appending another list's contents, copying it.
ListIter list_extend_copy(List *ls, List *other) {
    List *copy = list_copy(other);
    ListIter res = list_extend_move(ls,copy);
    obj_unref(copy); // copy is now empty
    return res;
}

/// move a list iterator along a given number of times.
bool list_advance(List *ls, ListIter *iter, int len) {
    int i = 0;
    FOR_LIST_FROM(ListEntry,item,*iter,ls) {
        if (++i == len) {
            *iter = item;
            return true;
        }
    }
    return false;
}

/// create a new list from another between two iterators.
List* list_slice(List *other, ListIter begin, ListIter end) {
    List *ls = private_clone_list(other);
    bool isc = list_is_container(ls);
    bool isref = ls->flags & LIST_REF;
    void *data;
    FOR_LIST_FROM(ListEntry,item,begin,other) {
        if (isc) {
            data = item->data;
            if (isref)
                obj_ref(data);
        } else {
            data = item;
        }
        list_add(ls, data);
        if (item == end) break;
    }
    return ls;
}

/// extract a new list from a start iterator for a given number of items.
List* list_slice_n (List *other, ListIter begin, int len) {
    ListIter end = begin;
    list_advance(other,&end,len);
    return  list_slice(other,begin,end);
}

/// erase items between two iterators.
// Opposite operation to `list_slice`
void list_erase(List *ls, ListIter begin, ListIter end) {
    ListIter item = begin, next;
    while (item && item != end) {
        next = item->_next;
        list_free_item(ls,item);
        item = next;
    }
}

/// erase n items from an iterator.
void list_erase_n(List *ls, ListIter begin, int n) {
    ListIter end = begin;
    list_advance(ls,&end,n);
    list_erase(ls,begin,end);
}

/// set the comparison operation for the objects in this list.
void list_item_compare(List *ls, ListCmpFun cmp) {
    ls->compare = cmp;
}

/// set the equality operation for the objects in this list.
void list_item_equals(List *ls, ListEqualsFun cmp) {
    ls->equals = cmp;
}

/// remove an item by data value.
bool list_remove_value(List *ls, void *data) {
    ListIter node = list_find(ls,data);
    if (node) {
        list_remove(ls,node);
        list_free_item(ls,node);
        return true;
    }
    return false;
}

/// make an array out of a list.
// @return an array of  data objects
void** list_to_array(List *ls) {
    void **arr = array_new(void*,list_size(ls));
    int i = 0;
    FOR_LIST(item,ls) {
        void *data = list_item_data(ls,item);
        arr[i++] = data;
    }
    return arr;
}

/// make a list out of an array of pointers or strings.
// @param type either LIST_POINTER or LIST_STRING
// @param arr the array
// @param sz size of array; if -1 then assume this is a NULL-terminated array
List *list_new_from_array(int type, void **arr, int sz) {
    List *ls = list_new(type);
    sz = sz == -1 ? array_len(arr) : sz;
    for (int i = 0; i < sz; i++) {
        void *data = arr[i];
        if (type & LIST_REF)
            obj_ref(data);
        list_add(ls,data);
    }
    return ls;
}

///////// List Wrappers //////

typedef struct LWrap_{
    void *data;
    List *ls;
} LWrap;

static void LWrap_dispose(LWrap *lw) {
    obj_unref(lw->ls);
}

void *listw_wrap(List *ls) {
    LWrap *lw = obj_new(LWrap,LWrap_dispose);
    lw->ls = obj_ref(ls);
    return lw;
}

void *listw_new() {
    LWrap *lw = obj_new(LWrap,LWrap_dispose);
    lw->ls = list_new_ptr();
    return lw;
}

void listw_add_(void *p) {
    LWrap *lw = (LWrap *)p;
    ListIter node = list_add(lw->ls,NULL);
    lw->data = &node->data;
}

void listw_insert_(void *p, ListIter before) {
    LWrap *lw = (LWrap *)p;
    ListIter node = list_insert(lw->ls,before,NULL);
    lw->data = &node->data;
}

List* listw_list(void *p) {
    LWrap *lw = (LWrap *)p;
    return ref(lw->ls);
}

ListIter listw_first(void *p) {
    return LW2L_(p)->first;
}

ListIter listw_iter(void *p,int idx) {
    ListIter node = list_iter(LW2L_(p),idx);
    *((void**)p) = &node->data;
    return node;
}

/// Statement Macros
// @section macros

/// iterate over list items.
// Directly useful if this is not a container list.
// @tparam type T
// @tparam T*  var points to each item
// @tparam List* the list
// @macro FOR_LIST_ITEM

/// iterate over default list items.
// These have a `data` field defined which can be cast to desired type.
// @tparam ListIter var
// @tparam List* the list
// @macro FOR_LIST

// iterate over list items from the back to front.
// @tparam type T
// @tparam T*  var points to each item
// @tparam List* the list
// @macro FOR_LIST_REV

// iterate over list items from a given item.
// @tparam type T
// @tparam T*  var points to each item
// @tparam ListIter item start
// @tparam List* the list
// @macro FOR_LIST_FROM

