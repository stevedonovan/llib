#include <stdlib.h>
#include <stdarg.h>
#include "json.h"

// this will take varargs and either make an array or simple map
// out of them. In addition, string arrays can be treated specially.
PValue value_array_values_ (intptr sm,...) {
    int n = 0, i = 0;
    int ismap = sm & 0x1;
    int isstr = sm & 0x2;
    void *P;

    // ok, how many items in the array/map?
    va_list ap;
    va_start(ap,sm);
    while ((P = va_arg(ap,void*)) != NULL)
        ++n;
    va_end(ap);

    void** ms = array_new_ref(void*,n);
    va_start(ap,sm);
    while ((P = va_arg(ap,void*)) != NULL)  {
        if ((ismap && i % 2 == 0) || isstr)
            P = str_cpy((char*)P); // C++ being picky here...
        ms[i++] = P;
    }
    va_end(ap);

    // 'simple' maps are arrays. But by flagging them as having
    // a special type the value system knows that they should be
    // used _as_ a map.
    if (ismap) {
        obj_type_index(ms) = OBJ_KEYVALUE_T;
    }
    return ms;
}
