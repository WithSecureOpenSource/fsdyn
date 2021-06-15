#ifndef __FSDYN_HASHTABLE__
#define __FSDYN_HASHTABLE__

#include <stdint.h>
#include <stdlib.h>

#include "integer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Hash tables for C.
 */

/*
 * This opaque datatype represents the hash table.
 */
typedef struct hash_table hash_table_t;

/*
 * This opaque datatype represents a key-value association in the hash
 * table.
 */
typedef struct hash_elem hash_elem_t;

/*
 * Create a hash_table_t object.
 *
 * The maximum number of elements the hash table is expected to hold is
 * specified with capacity. The hash table can hold more elements but at
 * the expense of performance.
 *
 * A key hash function hash() should scramble keys effectively.
 *
 * The key comparator cmp() must return 0 for equal keys and a nonzero
 * value for unequal keys.
 */
hash_table_t *make_hash_table(size_t capacity, uint64_t (*hash)(const void *),
                              int (*cmp)(const void *, const void *));

/*
 * Destroy an hash_table_t structure. The key and value objects
 * contained in the hash table are left intact.
 */
void destroy_hash_table(hash_table_t *table);

/*
 * Destroy an orphaned key-value association.
 *
 * You may call this function only for elements returned by the
 * hash_table_put(), hash_table_pop*() or hash_table_detach() functions.
 * The key and value are left intact.
 */
void destroy_hash_element(hash_elem_t *element);

/*
 * Return the number of elements in the hash table.
 */
size_t hash_table_size(hash_table_t *table);

/*
 * Return the key of the element.
 */
const void *hash_elem_get_key(hash_elem_t *element);

/*
 * Return the value of the element.
 */
const void *hash_elem_get_value(hash_elem_t *element);

/*
 * Retrieve an element based on a key. Return NULL if no matching
 * element is found. The returned element is valid until the hash table
 * is modified.
 */
hash_elem_t *hash_table_get(hash_table_t *table, const void *key);

/*
 * Insert an element into the hash table. The key field of the element
 * must be set before calling hash_table_put() and must not be altered
 * while the element is in the hash table.
 *
 * If another element with the same key is already stored in the hash
 * table, it is replaced; the previous element is detached and returned
 * and should be deallocated with destroy_hash_element(). Otherwise,
 * NULL is returned.
 *
 * If value itself is already stored in the hash table (at key),
 * hash_table_put() has no effect and NULL is returned.
 */
hash_elem_t *hash_table_put(hash_table_t *table, const void *key,
                            const void *value);

/*
 * Detach an element from the hash table.
 *
 * You must not call hash_table_remove() for an element that is not in
 * the hash table.
 *
 * You must call destroy_hash_element() on element once done with it.
 */
void hash_table_detach(hash_table_t *table, hash_elem_t *element);

/*
 * Detach and destroy an element from the hash table.
 *
 * You must not call hash_table_remove() for an element that is not in
 * the hash table.
 */
void hash_table_remove(hash_table_t *table, hash_elem_t *element);

/*
 * Look for an element with a key and detach it from the hash table. The
 * element is returned.
 *
 * If no matching element is found, hash_table_pop() returns NULL.
 */
hash_elem_t *hash_table_pop(hash_table_t *table, const void *key);

/*
 * Detach a random element and return it. Return NULL if the hash table
 * is empty. */
hash_elem_t *hash_table_pop_any(hash_table_t *table);

/*
 * Return a random element. Return NULL if the hash table is empty. See
 * also hash_table_get_other().
 */
hash_elem_t *hash_table_get_any(hash_table_t *table);

/*
 * Return some other random element. Return NULL if there is no other
 * element left. You can traverse the whole hash table (in random order)
 * by calling hash_table_get_any() and then repeatedly calling
 * hash_table_get_other().
 */
hash_elem_t *hash_table_get_other(hash_elem_t *element);

/*
 * Return a nonzero value if and only if the hash table is empty.
 */
int hash_table_empty(hash_table_t *table);

/*
 * A function to hash a NUL-terminated string.
 */
uint64_t hash_string(const char *s);

/*
 * A function to hash an integer.
 */
uint64_t hash_integer(integer_t *value);

/*
 * A function to hash an unsigned integer.
 */
uint64_t hash_unsigned(unsigned_t *value);

/*
 * A function to hash a binary blob.
 */
uint64_t hash_blob(const void *blob, size_t size);

#ifdef __cplusplus
}
#endif

#endif
