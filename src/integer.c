#include "integer.h"
#include "avltree_version.h"

int integer_cmp(const void *a, const void *b)
{
    if (as_intptr(a) < as_intptr(b))
        return -1;
    if (as_intptr(a) > as_intptr(b))
        return 1;
    return 0;
}

int unsigned_cmp(const void *a, const void *b)
{
    if (as_intptr(a) < as_uintptr(b))
        return -1;
    if (as_uintptr(a) > as_uintptr(b))
        return 1;
    return 0;
}
