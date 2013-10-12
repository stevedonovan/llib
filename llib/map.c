/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/****

Associative arrays implemented as binary trees.

Maps use the same basic structure as lists, since most of the requirements are similar:
comparison functions (which are like `strcmp`) and key allocation. The nodes have the same
structure, so structs with `LIST_HEADER` can be also put into maps, provided the next
field is the key.

Keys are always pointers, but like with string lists char pointers are a special case.

If the keys aren't strings, then the comparison function is simply the less-than operator.
This will work fine if simple equality defines a match, as with pointers and integers.

@module map
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private_list.h"
#include "map.h"

#define out(x) fprintf(stderr,"%s = %x\n",#x,(intptr)x);

// first becomes the tree root
#define root(m) ((m)->first)
// the pointer value type is put into the pointer value last
#define vtype(m) ((intptr)(m)->last)
#define set_vtype(m,t) ((m)->last=(ListIter)t)

#define key_data item_data

enum MapType {
    MAP_KEY_POINTER = LIST_PTR, MAP_KEY_STRING = LIST_STRING
};

enum MapValue {
    MAP_NODE = 0, MAP_STRING = 1, MAP_POINTER = 2, MAP_REF = 3,
};

static void dispose_map_entries(Map *m, PEntry node) {
    intptr vt = vtype(m), kt = m->flags;
    //printf("%d %d %p %d\n",vt,kt,node,node->data);

    // we are a container with string keys
    if (kt == MAP_KEY_STRING && vt != MAP_NODE) {
        obj_unref(node->key);
    }
    if (vt == MAP_NODE) { // struct map
        obj_unref(node);
    } else {
        if (vt & MAP_STRING) // container disposes of references
            obj_unref(node->data);
        free(node);
    }
}

void map_clear(Map *m) {
    if (root(m) != NULL) {
        map_visit(m,map_first(m),(MapCallback)dispose_map_entries,1);
        root(m) = NULL;
    }
}

Map *map_new(int ktype, enum MapValue vtype) {
    Map *m = list_new (ktype);
    (void)obj_set_dispose_fun(m, (DisposeFn)map_clear);
    set_vtype(m,vtype);
    return m;
}

/// Making New Maps
// @section new

/// make a new struct map.
// Like lists, such maps own the objects.
// @param strkey whether the key is a string or not
Map *map_new_node(bool strkey) {
    return map_new(LIST_NODE | (strkey ? LIST_STR : LIST_PTR) ,MAP_NODE);
}

/// make a new map with string keys and pointer values.
// Such keys are treated as in string lists; they are allocated and owned
// by the map; the values are always pointers.
Map *map_new_str_ptr() {
    return map_new(LIST_STRING, MAP_POINTER);
}

/// make a new map with string keys and refcounted values
Map *map_new_str_ref() {
    return map_new(LIST_STRING, MAP_REF);
}

/// make a new map with string keys and string values.
Map *map_new_str_str() {
    return map_new(LIST_STRING, MAP_STRING);
}

/// make a new map with pointer keys and pointer values.
// It's possible to use integers up to uintptr_t size as well.
Map *map_new_ptr_ptr() {
    return map_new(LIST_PTR, MAP_POINTER);
}

/// make a new map with pointer keys and refcounted values.
Map *map_new_ptr_ref() {
    return map_new(LIST_PTR, MAP_REF);
}

/// make a new map with pointer keys and string values.
Map *map_new_ptr_str() {
    return map_new(LIST_PTR, MAP_STRING);
}

/// Insertion, removal and retrieval
// @section put

/// get the data associated with this tree node.
// if it's a container, then we return the node's data,
// otherwise the node itself is the data
void *map_value_data (Map *m, PEntry item) {
    if (vtype(m)) return item->data;
    return (void*)item;
}

/// get the root map node.
// You will need this for `map_visit`.
PEntry map_first(Map *m) {
    return (PEntry)root(m);
}

void map_free_item(Map *m, PEntry me) {
    list_free_item((List*)m,(ListIter)me); // handles disposing the key, if necessary
}

// this does the work of inserting an item into the tree.
// There are two cases:
//  (1) the map data is a pointer to a struct with a map/list header; it is its own key.
//  (2) the map key and data are both pointers. But they may be different kinds
//      of pointers, e.g. strings for keys, plain pointers/ints for values
static PEntry put_item(Map *m, void *key, void *data, int pointer_type) {
    PEntry item = (PEntry)private_new_item(m,pointer_type ? (PEntry)key : data,sizeof(MapEntry));
    // for a pointer/string keyed map, this returns a MapEntryDefault struct,
    // and the data must also be a pointer or a string, which is put into the 'mdata' field
    if (pointer_type) {
        if (pointer_type == MAP_STRING)  // strings _may_ need copying..
            data = str_cpy((char*)data);
        item->data = data;
        //printf("key %d data %d\n",key,data);
    } else { // otherwise the data contains its own key
        key = item->key;
    }
    item->_left = NULL;
    item->_right = NULL;
    ++m->size;
    if (! root(m)) { // first item!
        root(m) = (ListIter) item;
    } else {
        PEntry P = (PEntry)root(m);
        PEntry last = P;
        int order, last_order = 0;
        ListCmpFun compare = m->compare;
        while (P) {
            order = compare(P->key,key);
            if (order == 0) { // gotcha - overwrite existing entry in map
                if (pointer_type) {
                    P->data = item->data;
                    map_free_item(m,item);
                } else { // put new node inplace
                    item->_left = P->_left;
                    item->_right = P->_right;
                    if (last_order > 0) { // we are a left child
                        last->_left = item;
                    } else {
                        last->_right = item;
                    }
                    obj_unref(P);
                }
                --m->size;
                return P;
            } else
            if (order > 0) { // we go down the left side
                P = P->_left;
                if (! P)  // and insert if we hit the edge
                    last->_left = item;
            } else { // down the right side, ditto
                P = P->_right;
                if (! P)
                    last->_right = item;
            }
            last = P;
            last_order = order;
        }
    }
    return item;
}

/// insert a map struct.
// That is, it is 'derived' from the MAP_HEADER type.
PEntry map_put_struct(Map *m, void *data) {
    if (vtype(m)) return NULL;  // can't use this for pointer maps
    return put_item(m, NULL, data, 0);
}

/// insert a number of map structs, ending in NULL.
// @tparam Map*
// param ...  the struct pointers
// @function map_put_structs

/// insert pointer data using a pointer key.
// String and pointer/integer keys are handled differently.
PEntry map_put(Map *m, void* key, void *data) {
    if (! vtype(m)) return NULL; // can't use this for struct maps
    return put_item(m, key, data, vtype(m));
}

/// insert an array of key/value pairs.
// Use `map_put_structs` for struct maps.
void map_put_keyvalues(Map *m, MapKeyValue *mkv) {
    for(; mkv->key; ++mkv) {
        map_put(m,mkv->key,mkv->value);
    }
}

static PEntry map_find(Map *m, void *key, PEntry** parent_edge) {
    PEntry node = (PEntry)root(m);
    if (! node) return NULL;  // we're empty!
    PEntry last = NULL;
    bool left = true;
    ListCmpFun compare = m->compare;
    while (node) {
        int order = compare(node->key,key);
        if (order == 0) { // gotcha!
            if (parent_edge) {
                if (left)
                    *parent_edge = &last->_left;
                else
                    *parent_edge = &last->_right;
            }
            return node;
        } else {
            last = node;
            left = order > 0;
            if (left) {
                node = node->_left;
            } else {
                node = node->_right;
            }
        }
    }
    return NULL;
}


/// get the value associated with a key.
// @return the value, or NULL if not found, or the value was NULL.
void *map_get(Map *m, void *key) {
    PEntry node = map_find(m,key,NULL);
    if (! node) return NULL;
    return map_value_data(m,node);
}

/// get an integer value from a map.
// @tparam Map* m
// @param key cast to void*
// @treturn int
// @function map_geti

/// put an integer value into a map.
// @tparam Map* m
// @param key cast to void*
// @tparam value cast to void*
// @function map_puti

/// does the map contain this key?
// @treturn bool `true` if the key is found, even if the value was NULL.
bool map_contains(Map *m, void *key) {
    PEntry node = map_find(m,key,NULL);
    return node != NULL;
}

static PEntry left_most_edge (PEntry node) {
    PEntry P = node, last;
    while (P) {
        last = P;
        P = P->_left;
    }
    return last;
}

/// remove the key and value from the map.
PEntry map_remove(Map *m, void *key) {
    PEntry *parent_edge;
    PEntry node = map_find(m,key,&parent_edge);
    if (! node) return NULL;
    -- m->size;
    if (! parent_edge) { // last chap in map
        root(m) = NULL;
        return node;
    }
    if (! node->_right) {
        *parent_edge = node->_left;
    } else {
        PEntry nright = node->_right;
        *parent_edge = nright;
        left_most_edge(nright)->_left = node->_left;
    }
    return node;
}

/// remove key/value, freeing any allocated memory.
// @return true if the key was found.
bool map_delete(Map *m, void *key) {
    PEntry node = map_remove(m,key);
    if (! node) return false;
    map_free_item(m,node);
    return true;
}

/// Iterating over maps
// @section iter

/// in-order traversal of a map.
// @param data arbitrary data for callback
// @param node where to start (use `map_first` for all)
// @param fun callback which will receive data and current node
// @param order -1 for pre-order, 0 for in-order, +1 for post-order
void map_visit(void *data, PEntry node, MapCallback fun, int order) {
    if (order < 0)
        fun(data,node);

    if (node->_left)
        map_visit(data,node->_left,fun,order);

    if (order == 0)
        fun(data,node);

    if (node->_right)
        map_visit(data,node->_right,fun,order);

    if (order > 0)
        fun(data,node);
}

struct ArrayMapIter {
    MapKeyValue *kp;
    Map *m;
};

static void add_keypair(struct ArrayMapIter *ami, PEntry node) {
    MapKeyValue *kp = ami->kp;
    kp->key = node->key;
    kp->value = map_value_data(ami->m,node);
    ami->kp = kp + 1;
}

/// Get the key/value pairs of a map as an array.
MapKeyValue *map_to_array(Map *m)
{
    MapKeyValue *res = array_new(MapKeyValue,map_size(m));
    struct ArrayMapIter *ami = obj_new(struct ArrayMapIter,NULL);
    ami->kp = res;
    ami->m = m;
    map_visit(ami,(PEntry)root(m),(MapCallback)add_keypair,0);
    obj_unref(ami);
    return res;
}

// we keep a stack of nodes and whether they have been visited.
// We'll reuse the default map entry for this purpose!

static void push_stack(MapIter iter, PEntry node, bool visited) {
    PEntry med = obj_new(MapEntry,NULL);
    med->key = (void*)visited;
    med->data = node;
    list_add(iter->vstack,med);
}

#define right_visited(item)  ((item)->key != NULL)

static PEntry pop_stack(MapIter iter) {
    return (PEntry) list_pop(iter->vstack);
}

static void go_down_left (MapIter iter) {
    PEntry node = iter->node;
    while (node->_left) {
        push_stack(iter,node,false);
        node = node->_left;
    }
    iter->node = node;
}

static void map_iter_free(MapIter iter) {
    obj_unref(iter->vstack);
}

/// convenient struct for initializing maps.
// @tfield void* key
// @tfield void* value
// @table MapKeyValue

/// map iterator type.
// @tfield void* key
// @tfield void* value
// @table MapIter

/// create a new map iterator positioned on the mininum node.
MapIter map_iter_new (Map *m, void *pkey, void *pvalue) {
    if (root(m) == NULL) // empty map
        return NULL;
    MapIter iter = obj_new(MapIterStruct,map_iter_free);
    iter->map = m;
    iter->node = (PEntry)root(m);
    iter->vstack = list_new_node(false);
    iter->pkey = (void**)pkey;
    iter->pvalue = (void**)pvalue;
    go_down_left(iter);
    iter->key = iter->node->key;
    iter->value = map_value_data(m,iter->node);
    if (iter->pkey) {
        *iter->pkey = iter->key;
        *iter->pvalue = iter->value;
    }
    return iter;
}

/// advance the map iterator to the next node.
MapIter map_iter_next (MapIter iter) {
    PEntry node = iter->node;
    // basic strategy is to revisit the left branch for the next node in order.
    // However, if we do have a right node, mark it as visited and go down the left.
    if (node->_right) {
        iter->node = node->_right;
        push_stack(iter,node,true);
        go_down_left(iter);
        node = iter->node;
    } else {// look for an univisited node on the stack
        PEntry item = pop_stack(iter);
        while (item) {
            if (! right_visited(item))
                break;
            obj_unref(item);
            item = pop_stack(iter);
        }
        if (item != NULL) { // gotcha!
            node = (PEntry)item->data;
            obj_unref(item);
        } else {// no more stack, we're finished
            obj_unref(iter);
            return NULL;
        }
        iter->node = node;
    }
    iter->key = node->key;
    iter->value = map_value_data(iter->map,node);
    if (iter->pkey) {
        *iter->pkey = iter->key;
        *iter->pvalue = iter->value;
    }
    return iter;
}

/// for-statement for iterating over a map.
// @tparam MapIter var the loop variable
// @tparam m the map
// @macro FOR_MAP

/// for-statement for iterating over typed key/value pairs
// @tparam K key the key variable
// @tparam V value the value variable
// @tparam m the map
// @macro FOR_MAP_KEYVALUE

