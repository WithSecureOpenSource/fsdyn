#include <stdbool.h>
#include "fsalloc.h"
#include "list.h"
#include "hashtable.h"
#include "hashtable_imp.h"
#include "fsdyn_version.h"

/* Values from: https://planetmath.org/goodhashtableprimes */
static const size_t GOOD_SIZES[] = {
    53,
    97,
    193,
    389,
    769,
    1543,
    3079,
    6151,
    12289,
    24593,
    49157,
    98317,
    196613,
    393241,
    786433,
    1572869,
    3145739,
    6291469,
    12582917,
    25165843,
    50331653,
    100663319,
    201326611,
    402653189,
    805306457,
    1610612741,
    0
};

hash_table_t *make_hash_table(size_t capacity,
                              uint64_t (*hash)(const void *),
                              int (*cmp)(const void *, const void *))
{
    hash_table_t *table = fsalloc(sizeof *table);
    const size_t *goodp;
    for (goodp = GOOD_SIZES; goodp[1] && goodp[0] < capacity; goodp++)
        ;
    capacity = *goodp;
    table->capacity = capacity;
    table->size = 0;
    table->hash = hash;
    table->cmp = cmp;
    table->table = fscalloc(capacity, sizeof *table->table);
    return table;
}

void destroy_hash_element(hash_elem_t *element)
{
    fsfree(element);
}

void destroy_hash_table(hash_table_t *table)
{
    size_t i;
    for (i = 0; i < table->capacity; i++)
        if (table->table[i] != NULL) {
            list_elem_t *le;
            for (le = list_get_first(table->table[i]); le; le = list_next(le))
                destroy_hash_element((hash_elem_t *) list_elem_get_value(le));
            destroy_list(table->table[i]);
        }
    fsfree(table->table);
    fsfree(table);
}

static void add_element(hash_table_t *hash_table, size_t list_index,
                        const void *key, const void *value)
{
    hash_elem_t *element = fsalloc(sizeof *element);
    element->key = key;
    element->value = value;
    element->hash_table = hash_table;
    element->list_index = list_index;
    element->loc = list_append(hash_table->table[list_index], element);
}

size_t hash_table_size(hash_table_t *table)
{
    return table->size;
}

const void *hash_elem_get_key(hash_elem_t *element)
{
    return element->key;
}

const void *hash_elem_get_value(hash_elem_t *element)
{
    return element->value;
}

hash_elem_t *hash_table_get(hash_table_t *table, const void *key)
{
    list_t *list = table->table[table->hash(key) % table->capacity];
    if (list == NULL)
        return NULL;
    list_elem_t *le;
    for (le = list_get_first(list); le != NULL; le = list_next(le)) {
        hash_elem_t *element = (hash_elem_t *) list_elem_get_value(le);
        if (table->cmp(key, element->key) == 0)
            return element;
    }
    return NULL;
}

hash_elem_t *hash_table_put(hash_table_t *table, const void *key,
                            const void *value)
{
    size_t i = table->hash(key) % table->capacity;
    list_t *list = table->table[i];
    if (list == NULL) {
        list = table->table[i] = make_list();
        add_element(table, i, key, value);
        table->size++;
        return NULL;
    }
    list_elem_t *le;
    for (le = list_get_first(list); le != NULL; le = list_next(le)) {
        hash_elem_t *old = (hash_elem_t *) list_elem_get_value(le);
        if (table->cmp(key, old->key) == 0) {
            if (hash_elem_get_value(old) == value)
                return NULL;
            list_remove(list, le);
            add_element(table, i, key, value);
            return old;
        }
    }
    add_element(table, i, key, value);
    table->size++;
    return NULL;
}

void hash_table_detach(hash_table_t *table, hash_elem_t *element)
{
    list_remove(element->hash_table->table[element->list_index], element->loc);
    table->size--;
}

void hash_table_remove(hash_table_t *table, hash_elem_t *element)
{
    hash_table_detach(table, element);
    destroy_hash_element(element);
}

hash_elem_t *hash_table_pop(hash_table_t *table, const void *key)
{
    hash_elem_t *element = hash_table_get(table, key);
    if (element != NULL)
        hash_table_detach(table, element);
    return element;
}

hash_elem_t *hash_table_pop_any(hash_table_t *table)
{
    size_t i;
    for (i = 0; i < table->capacity; i++) {
        list_t *list = table->table[i];
        if (list != NULL && !list_empty(list)) {
            table->size--;
            return (hash_elem_t *) list_pop_first(list);
        }
    }
    return NULL;
}

static hash_elem_t *get_next(hash_table_t *table, size_t i)
{
    for (; i < table->capacity; i++) {
        list_t *list = table->table[i];
        if (list != NULL && !list_empty(list))
            return (hash_elem_t *) list_elem_get_value(list_get_first(list));
    }
    return NULL;
}

hash_elem_t *hash_table_get_any(hash_table_t *table)
{
    return get_next(table, 0);
}

hash_elem_t *hash_table_get_other(hash_elem_t *element)
{
    list_elem_t *le = list_next(element->loc);
    if (le)
        return (hash_elem_t *) list_elem_get_value(le);
    return get_next(element->hash_table, element->list_index + 1);
}

int hash_table_empty(hash_table_t *table)
{
    return table->size == 0;
}

uint64_t hash_string(const char *s)
{
    uint64_t hash = 0;
    while (*s)
        hash = hash * 9973 + *s++ + 9999991;
    return hash;
}

uint64_t hash_integer(integer_t *value)
{
    return as_intptr(value);
}

uint64_t hash_unsigned(unsigned_t *value)
{
    return as_uintptr(value);
}

uint64_t hash_blob(const void *blob, size_t size)
{
    const uint8_t *p = blob;
    uint64_t hash = 0;
    while (size--)
        hash = hash * 9973 + *p++ + 9999991;
    return hash;
}
