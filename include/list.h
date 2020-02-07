#ifndef __FSDYN_LIST__
#define __FSDYN_LIST__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Lists for C.
 */

/*
 * This opaque datatype represents the list.
 */
typedef struct list list_t;

/*
 * This opaque datatype represents a membership in the list.
 */
typedef struct list_elem list_elem_t;

/*
 * Create a list_t object.
 */
list_t *make_list(void);

/*
 * Destroy a list_t structure. The objects contained in the list are
 * left intact.
 */
void destroy_list(list_t *list);

/*
 * Return the value of the element (the list member object).
 */
const void *list_elem_get_value(list_elem_t *element);

/*
 * Return the first element of the list, or NULL if the list is empty.
 */
list_elem_t *list_get_first(list_t *list);

/*
 * Return the last element of the list, or NULL if the list is empty.
 */
list_elem_t *list_get_last(list_t *list);

/*
 * Return the first element holding the given value or NULL if no such
 * element is found.
 */
list_elem_t *list_get(list_t *list, const void *value);

/*
 * Return the nth element or NULL if n < 0 or n >= list size.
 */
list_elem_t *list_get_by_index(list_t *list, int idx);

/*
 * Return the successor of the given element or NULL if element is the
 * last element.
 */
list_elem_t *list_next(list_elem_t *element);

/*
 * Return the predecessor of the given element or NULL if element is the
 * first element.
 */
list_elem_t *list_previous(list_elem_t *element);

/*
 * Insert an element to end of the list.
 */
list_elem_t *list_append(list_t *list, const void *value);

/*
 * Insert an element to beginning of the list.
 */
list_elem_t *list_prepend(list_t *list, const void *value);

/*
 * Insert an element in the list before the given element. The
 * behaviour is undefined if 'next' does not belong to 'list'.
 */
list_elem_t *list_insert_before(list_t *list, const void *value,
                                list_elem_t *next);

/*
 * Remove an element from the list.
 */
void list_remove(list_t *list, list_elem_t *element);

/*
 * Remove the first element of the list and return its value or NULL if
 * the list is empty.
 */
const void *list_pop_first(list_t *list);

/*
 * Remove the last element of the list and return its value or NULL if
 * the list is empty.
 */
const void *list_pop_last(list_t *list);

/*
 * Return a nonzero value if and only if the list is empty.
 */
int list_empty(list_t *list);

/*
 * Return the size of the list in O(1).
 */
size_t list_size(list_t *list);

/*
 * Invoke a function for each element from first to last.
 * For example, this statement calls fsfree for each list element [value}:
 *
 *     list_foreach(list, (void *) fsfree, NULL);
 */
void list_foreach(list_t *list, void (*f)(const void *value, void *arg),
                  void *arg);

/*
 * Make a shallow copy of the list.
 */
list_t *list_copy(list_t *list);

#ifdef __cplusplus
}
#endif

#endif
