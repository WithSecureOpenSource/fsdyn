#ifndef __FSDYN_INTEGER__
#define __FSDYN_INTEGER__

/*
 * Cosmetic functions for using unboxed integers with dynamic data
 * structures.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The mental model is that every int value has a corresponding,
 * interned data object somewhere in RAM. That data object is of the
 * opaque integer_t type.
 *
 * (The down-to-earth view is that integer_t is just syntactic sugar
 * on top of the void pointer.)
 */
typedef struct integer integer_t;
typedef struct unsigned_ unsigned_t;

/* Retrieve the integer_t object corresponding to value.
 *
 * (IOW, cast the int value into a pointer.)
 */
static inline integer_t *as_integer(intptr_t value)
{
    return (integer_t *) value;
}

/* Retrieve the unsigned_t object corresponding to value.
 *
 * (IOW, cast the unsigned value into a pointer.)
 */
static inline unsigned_t *as_unsigned(uintptr_t value)
{
    return (unsigned_t *) value;
}

/* Convert the integer_t object back to an int value.
 *
 * (IOW, cast the pointer into an int.)
 */
static inline intptr_t as_intptr(const integer_t *integer)
{
    return (intptr_t) integer;
}

/* Convert the unsigned_t object back to an unsigned int value.
 *
 * (IOW, cast the pointer into an unsigned.)
 */
static inline uintptr_t as_uintptr(const unsigned_t *unsigned_)
{
    return (uintptr_t) unsigned_;
}

/* Compare two integer_t objects. */
int integer_cmp(const void *a, const void *b);

/* Compare two unsigned_t objects. */
int unsigned_cmp(const void *a, const void *b);

#ifdef __cplusplus
}
#endif

#endif
