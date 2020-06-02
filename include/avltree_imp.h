struct avl_tree {
    int (*cmp)(const void *, const void *, void *);
    void *obj;
    avl_elem_t *root;
    size_t size;
};

struct avl_elem {
    const void *key, *value;
    avl_elem_t *left, *right, *parent;
    int balance;
};
