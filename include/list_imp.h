struct list {
    list_elem_t *first, *last;
    size_t size;
};

struct list_elem {
    const void *value;
    list_elem_t *previous, *next;
};
