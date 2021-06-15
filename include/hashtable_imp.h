#include "list.h"

struct hash_table {
    size_t capacity, size;
    uint64_t (*hash)(const void *);
    int (*cmp)(const void *, const void *);
    list_t **table; /* of hash_elem_t */
};

struct hash_elem {
    hash_table_t *hash_table;
    size_t list_index;
    list_elem_t *loc;
    const void *key, *value;
};
