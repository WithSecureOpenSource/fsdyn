#ifndef __FSDYN_INTSET__
#define __FSDYN_INTSET__

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct intset intset_t;

intset_t *make_intset(size_t size);
void destroy_intset(intset_t *s);
void intset_fill(intset_t *s);
void intset_add(intset_t *s, unsigned elem);
void intset_remove(intset_t *s, unsigned elem);
bool intset_has(intset_t *s, unsigned elem);
/*
 * Return the smallest element (not) in the set greater than or equal
 * to the given element, or -1 if no such element exists.
 */
int intset_find_next_hit(intset_t *s, unsigned elem);
int intset_find_next_miss(intset_t *s, unsigned elem);

static inline bool intset_empty(intset_t *s)
{
    return intset_find_next_hit(s, 0) < 0;
}

#ifdef __cplusplus
}
#endif

#endif
