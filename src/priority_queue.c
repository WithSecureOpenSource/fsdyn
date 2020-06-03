#include <limits.h>
#include <stdint.h>
#include "priority_queue.h"
#include "fsalloc.h"
#include "avltree_version.h"

struct priorq {
    int (*cmp)(const void *elem1, const void *elem2, void *obj);
    void (*reloc)(const void *elem, void *loc, void *obj);
    void *obj;
    const void **storage;
    size_t capacity;
    size_t max_capacity;
};

static void dummy_reloc(const void *value, void *loc, void *obj)
{
}

priorq_t *make_priority_queue_2(int (*cmp)(const void *elem1,
                                           const void *elem2, void *obj),
                                void (*reloc)(const void *elem, void *loc,
                                              void *obj),
                                void *obj)
{
    priorq_t *prq = fsalloc(sizeof *prq);
    prq->cmp = cmp;
    prq->obj = obj;
    prq->reloc = reloc ? reloc : dummy_reloc;
    prq->storage = NULL;
    prq->capacity = 0;
    prq->max_capacity = 0;
    return prq;
}

priorq_t *make_priority_queue(int (*cmp)(const void *, const void *),
                              void (*reloc)(const void *, void *loc))
{
    return make_priority_queue_2((void *) cmp, (void *) reloc, NULL);
}

void destroy_priority_queue(priorq_t *prq)
{
    fsfree(prq->storage);
    fsfree(prq);
}

static void assign(priorq_t *prq, size_t slot, const void *value)
{
    prq->storage[slot] = value;
    prq->reloc(value, (void *) (intptr_t) slot, prq->obj);
}

static size_t raise(priorq_t *prq, size_t slot, const void *value)
{
    void *obj = prq->obj;
    while (slot) {
        size_t parent = (slot - 1) / 2;
        if (prq->cmp(value, prq->storage[parent], obj) >= 0)
            break;
        assign(prq, slot, prq->storage[parent]);
        slot = parent;
    }
    return slot;
}

void priorq_enqueue(priorq_t *prq, const void *value)
{
    if (prq->capacity == prq->max_capacity) {
        size_t n = 1;
        while (n < prq->capacity + 1) {
            if (n > SIZE_MAX / 2) {
                n = SIZE_MAX;
                break;
            }
            n <<= 1;
        }
        prq->max_capacity = n;
        prq->storage =
            fsrealloc(prq->storage, prq->max_capacity * sizeof *prq->storage);
    }
    size_t slot = prq->capacity++;
    assign(prq, raise(prq, slot, value), value);
}

size_t priorq_size(priorq_t *prq)
{
    return prq->capacity;
}

bool priorq_empty(priorq_t *prq)
{
    return priorq_size(prq) == 0;
}

static void lower(priorq_t *prq, size_t slot, const void *value)
{
    void *obj = prq->obj;
    for (;;) {
        size_t left = slot * 2 + 1;
        if (left >= prq->capacity)
            break;
        size_t branch;
        size_t right = left + 1;
        if (right >= prq->capacity ||
            prq->cmp(prq->storage[left], prq->storage[right], obj) < 0)
            branch = left;
        else branch = right;
        if (prq->cmp(value, prq->storage[branch], obj) <= 0)
            break;
        assign(prq, slot, prq->storage[branch]);
        slot = branch;
    }
    assign(prq, slot, value);
}

const void *priorq_dequeue(priorq_t *prq)
{
    switch (prq->capacity) {
        case 0:
            return NULL;
        case 1:
            return prq->storage[--prq->capacity];
        default:
            ;
    }
    const void *value = prq->storage[0];
    lower(prq, 0, prq->storage[--prq->capacity]);
    return value;
}

const void *priorq_peek(priorq_t *prq)
{
    if (priorq_empty(prq))
        return NULL;
    return prq->storage[0];
}

const void *priorq_pop(priorq_t *prq)
{
    if (priorq_empty(prq))
        return NULL;
    return prq->storage[--prq->capacity];
}

const void *priorq_remove(priorq_t *prq, void *loc)
{
    size_t slot = (intptr_t) loc;
    const void *value = prq->storage[slot];
    const void *other = prq->storage[--prq->capacity];
    lower(prq, raise(prq, slot, other), other);
    return value;
}
