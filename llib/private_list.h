#ifndef __PRIVATE_LIST_H
#define _PRIVATE_LIST_H

// part of List* private to this implementation
#define _INSIDE_LIST_C \
    int flags; \
    ListCmpFun compare; \
    ListEqualsFun equals;

#define list_is_node(ls) ((ls)->flags & LIST_NODE)
#define list_is_container(ls) (! list_is_node(ls))

#define LW2L_(wp) (((List**)wp)[1])

#endif
