#ifndef __FSDYN_PRIORITY_QUEUE__
#define __FSDYN_PRIORITY_QUEUE__

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Priority queues (heaps) for C. */

typedef struct priorq priorq_t;

/* Create a priority queue.
 *
 * The element comparator cmp() compares the relative priority of two
 * elements. A negative value indicates that the first arguments has a
 * higher priority than the second argument. A zero value indicates an
 * equal priority.
 *
 * The optional reloc() function is called on an element whenever the
 * element is placed in a new location in the heap structure, which
 * can happen any time the priority queue is operated on. The function
 * should store the value of loc so it can be used in a subsequent
 * priorq_remove() call.
 *
 * The reloc() function can also be NULL, in which case
 * priorq_remove() cannot be used but elements can only be retrieved
 * in the strict priority order. */
priorq_t *make_priority_queue(int (*cmp)(const void *elem1, const void *elem2),
                              void (*reloc)(const void *elem, void *loc));

/* Create a priority queue.
 *
 * Like make_priority_queue() but the callback functions are given a
 * context argument. */
priorq_t *make_priority_queue_2(int (*cmp)(const void *elem1,
                                           const void *elem2, void *obj),
                                void (*reloc)(const void *elem, void *loc,
                                              void *obj),
                                void *obj);

/* Destroy the priority queue structure. The elements are not affected. */
void destroy_priority_queue(priorq_t *prq);

/* Enter an element into the priority queue. One element can be
 * enqueued twice, and two separate elements can have an equal
 * priority. */
void priorq_enqueue(priorq_t *prq, const void *value);

/* Return true if and only if the priority queue has no elements. */
bool priorq_empty(priorq_t *prq);

/* Return the number of elements in the priority queue. */
size_t priorq_size(priorq_t *prq);

/* Remove the element with the highest priority from the priority
 * queue and return it. The element that compares the least in terms
 * of cmp() is considered highest priority.
 *
 * If the priority queue is empty, NULL is returned. */
const void *priorq_dequeue(priorq_t *prq);

/* Remove some element from the priority queue and return it. The
 * function may execute faster than priorq_dequeue().
 *
 * If the priority queue is empty, NULL is returned. */
const void *priorq_pop(priorq_t *prq);

/* Remove the element with the given locator and return it. */
const void *priorq_remove(priorq_t *prq, void *loc);

#ifdef __cplusplus
}
#endif

#endif
