#ifndef __FSDYN_AVLTREE__
#define __FSDYN_AVLTREE__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AVL trees for C.
 */

/*
 * This opaque datatype represents the AVL tree.
 */
typedef struct avl_tree avl_tree_t;

/*
 * This opaque datatype represents a key-value association in the AVL
 * tree.
 */
typedef struct avl_elem avl_elem_t;

/*
 * Create an avl_tree_t object.
 *
 * The key comparator cmp() return value is as with memcmp().
 */
avl_tree_t *make_avl_tree(int (*cmp)(const void *, const void *));

/*
 * Destroy an avl_tree_t structure. The key and value objects contained
 * in the tree are left intact.
 */
void destroy_avl_tree(avl_tree_t *tree);

/*
 * Destroy an orphaned key-value association.
 *
 * You may call this function only for elements returned by the
 * avl_tree_put(), avl_tree_pop*() or avl_tree_detach() functions.
 */
void destroy_avl_element(avl_elem_t *element);

/*
 * Return the number of elements in the tree.
 */
size_t avl_tree_size(avl_tree_t *tree);

/*
 * Return the key of the element.
 */
const void *avl_elem_get_key(avl_elem_t *element);

/*
 * Return the value of the element.
 */
const void *avl_elem_get_value(avl_elem_t *element);

/*
 * Retrieve an element based on a key. Return NULL if no matching
 * element is found. The returned element is valid until the tree is
 * modified.
 */
avl_elem_t *avl_tree_get(avl_tree_t *tree, const void *key);

/*
 * Get the last element whose key is less than the given key, or NULL if
 * no such element is found. The returned element is valid until the
 * tree is modified.
 */
avl_elem_t *avl_tree_get_before(avl_tree_t *tree, const void *key);

/*
 * Get the last element whose key is less than or equal to the given
 * key, or NULL if no such element is found. The returned element is
 * valid until the tree is modified.
 */
avl_elem_t *avl_tree_get_on_or_before(avl_tree_t *tree, const void *key);

/*
 * Get the first element whose key is greater than the given key, or
 * NULL if no such element is found. The returned element is valid until
 * the tree is modified.
 */
avl_elem_t *avl_tree_get_after(avl_tree_t *tree, const void *key);

/*
 * Get the first element whose key is greater than or equal to the given
 * key, or NULL if no such element is found. The returned element is
 * valid until the tree is modified.
 */
avl_elem_t *avl_tree_get_on_or_after(avl_tree_t *tree, const void *key);

/*
 * Return the first element of the tree, or NULL if the tree is empty.
 * The returned element is valid until the tree is modified.
 */
avl_elem_t *avl_tree_get_first(avl_tree_t *tree);

/*
 * Return the last element of the tree, or NULL if the tree is empty.
 * The returned element is valid until the tree is modified.
 */
avl_elem_t *avl_tree_get_last(avl_tree_t *tree);

/*
 * Return the successor of the given element or NULL if element is the
 * last element. The returned element is valid until the tree is
 * modified.
 */
avl_elem_t *avl_tree_next(avl_elem_t *element);

/*
 * Return the predecessor of the given element or NULL if element is the
 * first element. The returned element is valid until the tree is
 * modified.
 */
avl_elem_t *avl_tree_previous(avl_elem_t *element);

/*
 * Insert an element into the tree. The key field of the element must be
 * set before calling avl_tree_put() and must not be altered while the
 * element is in the tree.
 *
 * If another element with the same key is already stored in the tree,
 * it is replaced; the previous element is detached and returned and
 * should be deallocated with destroy_avl_element(). Otherwise, NULL is
 * returned.
 *
 * If value itself is already stored in the tree (at key),
 * avl_tree_put() has no effect and NULL is returned.
 */
avl_elem_t *avl_tree_put(avl_tree_t *tree, const void *key, const void *value);

/*
 * Detach an element from the tree.
 *
 * You must not call avl_tree_detach() for an element that is not in the
 * tree.
 *
 * You must call destroy_avl_element() on element once done with it.
 */
void avl_tree_detach(avl_tree_t *tree, avl_elem_t *element);

/*
 * Detach and destroy an element from the tree.
 *
 * You must not call avl_tree_remove() for an element that is not in the
 * tree.
 */
void avl_tree_remove(avl_tree_t *tree, avl_elem_t *element);

/*
 * Look for an element with a key and detach it from the tree. The
 * element is returned.
 *
 * If no matching element is found, avl_tree_pop() returns NULL.
 */
avl_elem_t *avl_tree_pop(avl_tree_t *tree, const void *key);

/*
 * Detach the first element and return it. Return NULL if the tree is
 * empty. */
avl_elem_t *avl_tree_pop_first(avl_tree_t *tree);

/*
 * Detach the last element and return it. Return NULL if the tree is
 * empty. */
avl_elem_t *avl_tree_pop_last(avl_tree_t *tree);

/*
 * Return a nonzero value if and only if the tree is empty.
 */
int avl_tree_empty(avl_tree_t *tree);

/*
 * Make a shallow copy of the tree.
 */
avl_tree_t *avl_tree_copy(avl_tree_t *tree);

#ifdef __cplusplus
}
#endif

#endif
