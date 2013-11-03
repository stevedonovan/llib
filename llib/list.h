#ifndef _LLIB_LIST_H
#define _LLIB_LIST_H

#include "obj.h"

#define LIST_HEADER struct _ListEntry* _prev; struct _ListEntry* _next

// all node structures must start like this...

typedef struct _ListEntry {
    LIST_HEADER;
    void *data;
} ListEntry, *ListIter;

typedef bool (*ListSearchFn) (ListIter,void*);
typedef bool (*ListPred)(const void *data);
typedef int (*ListCmpFun)(const void *p1, const void *p2);
typedef ListCmpFun ListEqualsFun;
#define LIST_CMP (ListCmpFun)

typedef struct List_ {
    ListEntry *first;
    ListEntry *last;
    int size;
#ifdef _INSIDE_LIST_C
_INSIDE_LIST_C
#endif
} List;

enum {
    LIST_PTR = 0,
    LIST_NODE = 1,
    LIST_REF = 2,
    LIST_STR = 4,
    LIST_STRING = LIST_STR + LIST_REF
};

#define FOR_LIST_ITEM(type,var,list) for (type *var = (type*)list->first; var != NULL; var = (type*)var->_next)
#define FOR_LIST(var,list) FOR_LIST_ITEM(ListEntry,var,list)
#define FOR_LIST_REV(type,var,list) for (type *var = (type*)list->last; var != NULL; var = (type*)var->_prev)
#define FOR_LIST_FROM(type,var,item,list) for (type *var = (type*)item; var != NULL; var = (type*)var->_next)
#define FOR_LIST_REV_FROM(type,var,item,list) for (type *var = (type*)item; var != NULL; var = (type*)var->_prev)

#define LLIB_NEXTVAR_(T,var) next_=(p_?(var=(T)p_->data,p_->_next):NULL)

#define FOR_LIST_T(T,var,ls) \
  for (ListIter p_=(ls)->first,LLIB_NEXTVAR_(T,var); \
  p_ != NULL; \
  (p_=next_,LLIB_NEXTVAR_(T,var)))

#define FOR_LISTW(var,lw)\
for (ListIter var = (ListIter)listw_first(lw);\
(var!=NULL) && (*((void**)lw)=&var->data); var = (ListIter)var->_next)

List *list_new (int flags);
ListIter private_new_item(List *ls, void *data, int size);
bool list_object (void *obj);
List *list_new_ptr();
List *list_new_str();
List *list_new_ref();
List *list_new_node(bool str);
int list_size(List *ls);
void * list_item_data (List *ls, void *item);
ListIter list_start(List *ls);
ListIter list_end(List *ls);
ListIter list_iter_next(ListIter le);
ListIter list_iter_prev(ListIter le);
ListIter list_add (List *ls, void *data);
ListIter list_insert(List *ls, ListIter before, void *data);
ListIter list_insert_front(List *ls, void *data);
ListIter list_add_sorted(List *ls, void *data);
ListIter list_remove(List *ls, ListIter item);
void *list_delete(List *ls, ListIter item) ;
void * list_pop(List *ls);
ListIter list_find(List *ls, void *data);
ListIter list_find_if (List *ls, ListSearchFn fn, void *data);
ListIter list_iter(List *ls, int idx);
void *list_get(List *ls, int idx);
List *list_filter(List *other, ListPred fun);
List *list_filter_if(List *other, ListSearchFn fun, void *data);
List *list_copy(List *other);
ListIter list_extend_move(List *ls, List *other);
ListIter list_extend_copy(List *ls, List *other);
bool list_advance(List *ls, ListIter*iter, int len);
List* list_slice(List *other, ListIter begin, ListIter end);
List* list_slice_n (List *other, ListIter begin, int len);
void list_erase(List *ls, ListIter begin, ListIter end);
void list_erase_n(List *ls, ListIter begin, int n);
void list_item_compare(List *ls, ListCmpFun cmp);
void list_item_equals(List *ls, ListEqualsFun cmp);
bool list_remove_value(List *ls, void *data);
void list_free_item(List *ls, ListIter item);

void** list_to_array(List *ls);
ListIter list_new_item(List *ls, int sz);
List *list_new_from_array(int type, void **arr, int sz);

void *listw_wrap(List *ls);
void *listw_new();
void listw_add_(void *p);
void listw_insert_(void *p, ListIter before);
List* listw_list(void *p);
ListIter listw_first(void *p);
ListIter listw_iter(void *p,int idx);

#define list_add_items(ls,...)   obj_apply_varargs(ls,(PFun)list_add,__VA_ARGS__,NULL)

#define listw_add(lw,v) {listw_add_(lw); **lw = v;}
#define listw_insert(lw,before,v) {listw_insert_(lw,before); **lw = v; }
#define listw_get(lw,idx) (listw_iter(lw,idx), **lw)

#endif



