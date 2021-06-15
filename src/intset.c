#include "intset.h"

#include <limits.h>

#include "fsalloc.h"
#include "fsdyn_version.h"
#include "intset_imp.h"

#ifndef LONG_BIT
#define LONG_BIT (sizeof(unsigned long) * CHAR_BIT)
#endif
#define BITS_TO_LONGS(bits) ((bits + LONG_BIT - 1) / LONG_BIT)

intset_t *make_intset(size_t size)
{
    intset_t *s = fsalloc(sizeof *s);
    s->size = size;
    s->data = fscalloc(BITS_TO_LONGS(size), sizeof *s->data);
    return s;
}

void destroy_intset(intset_t *s)
{
    fsfree(s->data);
    fsfree(s);
}

void intset_fill(intset_t *s)
{
    unsigned i;
    for (i = 0; i < BITS_TO_LONGS(s->size); i++)
        s->data[i] = ~0;
}

void intset_add(intset_t *s, unsigned elem)
{
    if (elem < s->size)
        s->data[elem / LONG_BIT] |= 1UL << elem % LONG_BIT;
}

void intset_remove(intset_t *s, unsigned elem)
{
    if (elem < s->size)
        s->data[elem / LONG_BIT] &= ~(1UL << elem % LONG_BIT);
}

bool intset_has(intset_t *s, unsigned elem)
{
    if (elem < s->size)
        return s->data[elem / LONG_BIT] & (1UL << elem % LONG_BIT);
    else
        return false;
}

static int intset_find_next(intset_t *s, unsigned elem, bool hit)
{
    if (elem >= s->size)
        return -1;
    unsigned long x = s->data[elem / LONG_BIT];
    if (!hit)
        x = ~x;
    int r = elem % LONG_BIT;
    x &= ~0UL << r;
    elem -= r;
    while (!x) {
        elem += LONG_BIT;
        if (elem >= s->size)
            return -1;
        x = s->data[elem / LONG_BIT];
        if (!hit)
            x = ~x;
    }
    elem += __builtin_ctzl(x);
    if (elem >= s->size)
        return -1;
    return elem;
}

int intset_find_next_hit(intset_t *s, unsigned elem)
{
    return intset_find_next(s, elem, true);
}

int intset_find_next_miss(intset_t *s, unsigned elem)
{
    return intset_find_next(s, elem, false);
}
